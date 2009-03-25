/*
 * $Id$
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "sed.h"
#include "../cmn/mem.h"
#include <qse/cmn/rex.h>

/* TODO: delete stdio.h */
#include <qse/utl/stdio.h>

/* POSIX http://www.opengroup.org/onlinepubs/009695399/utilities/sed.html 
 * ALSO READ
    http://www.freebsd.org/cgi/cvsweb.cgi/src/usr.bin/sed/POSIX?rev=1.4.6.1;content-type=text%2Fplain 
 */

QSE_IMPLEMENT_COMMON_FUNCTIONS (sed)

qse_sed_t* qse_sed_open (qse_mmgr_t* mmgr, qse_size_t xtn)
{
	qse_sed_t* sed;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	sed = (qse_sed_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_sed_t) + xtn);
	if (sed == QSE_NULL) return QSE_NULL;

	if (qse_sed_init (sed, mmgr) == QSE_NULL)
	{
		QSE_MMGR_FREE (sed->mmgr, sed);
		return QSE_NULL;
	}

	return sed;
}

void qse_sed_close (qse_sed_t* sed)
{
	qse_sed_fini (sed);
	QSE_MMGR_FREE (sed->mmgr, sed);
}

qse_sed_t* qse_sed_init (qse_sed_t* sed, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (sed, 0, sizeof(*sed));
	sed->mmgr = mmgr;

	if (qse_str_init (&sed->rexbuf, mmgr, 0) == QSE_NULL)
	{
		sed->errnum = QSE_SED_ENOMEM;
		return QSE_NULL;	
	}	

	if (qse_map_init (&sed->labs, mmgr, 128, 70) == QSE_NULL)
	{
		qse_str_fini (&sed->rexbuf);
		sed->errnum = QSE_SED_ENOMEM;
		return QSE_NULL;	
	}
	qse_map_setcopier (&sed->labs, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
        qse_map_setscale (&sed->labs, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	/* TODO: use different data structure... */
	sed->cmd.buf = QSE_MMGR_ALLOC (
		sed->mmgr, QSE_SIZEOF(qse_sed_cmd_t) * 1000);
	if (sed->cmd.buf == QSE_NULL)
	{
		qse_map_fini (&sed->labs);
		qse_str_fini (&sed->rexbuf);
		return QSE_NULL;
	}
	sed->cmd.cur = sed->cmd.buf;
	sed->cmd.end = sed->cmd.buf + 1000 - 1;

	return sed;
}

void qse_sed_fini (qse_sed_t* sed)
{
/* TODO: use different data sturect -> look at qse_sed_init */
qse_sed_cmd_t* c;
for (c = sed->cmd.buf; c != sed->cmd.cur; c++)
{
	switch (c->type)
	{
		case QSE_SED_CMD_A:
		case QSE_SED_CMD_C:
		case QSE_SED_CMD_I:
			if (c->u.text.ptr != QSE_NULL) 
				QSE_MMGR_FREE (sed->mmgr, c->u.text.ptr);
			break;

		case QSE_SED_CMD_B:
		case QSE_SED_CMD_T:
			if (c->u.branch.label.ptr != QSE_NULL) 
				QSE_MMGR_FREE (sed->mmgr, c->u.branch.label.ptr);
			break;
	
		case QSE_SED_CMD_Y:
			if (c->u.transet.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, c->u.transet.ptr);
			break;

		case QSE_SED_CMD_R:
		case QSE_SED_CMD_RR:
		case QSE_SED_CMD_W:
		case QSE_SED_CMD_WW:
			if (c->u.filename.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, c->u.filename.ptr);
			break;
	}
}
QSE_MMGR_FREE (sed->mmgr, sed->cmd.buf);	

	qse_map_fini (&sed->labs);
	qse_str_fini (&sed->rexbuf);
}

const qse_char_t* qse_sed_geterrmsg (qse_sed_t* sed)
{
	static const qse_char_t* errmsg[] =
	{
		QSE_T("no error"),
		QSE_T("out of memory"),
		QSE_T("too much text"),
		QSE_T("command not recognized"),
		QSE_T("command garbled"),
		QSE_T("regular expression build error"),
		QSE_T("address 1 prohibited"),
		QSE_T("address 2 prohibited"),
		QSE_T("a new line expected"),
		QSE_T("a backslash expected"),
		QSE_T("garbage after a backslash"),
		QSE_T("a semicolon expected"),
		QSE_T("label name too long"),
		QSE_T("empty label name"),
		QSE_T("duplicate label name"),
		QSE_T("empty file name"),
		QSE_T("illegal file name"),
		QSE_T("translation set not terminated"),
		QSE_T("strings in translation set not the same length"),
		QSE_T("group brackets not balanced"),
		QSE_T("group nesting too deep")
	};

	return (sed->errnum > 0 && sed->errnum < QSE_COUNTOF(errmsg))?
		errmsg[sed->errnum]: QSE_T("unknown error");	
}

void qse_sed_setoption (qse_sed_t* sed, int option)
{
	sed->option = option;
}

int qse_sed_getoption (qse_sed_t* sed)
{
	return sed->option;
}

/* get the current charanter of the source code */
#define CURSC(sed) \
	(((sed)->src.cur < (sed)->src.end)? (*(sed)->src.cur): QSE_CHAR_EOF)
/* advance the current pointer of the source code */
#define ADVSCP(sed) ((sed)->src.cur++)
#define NXTSC(sed) \
	(((++(sed)->src.cur) < (sed)->src.end)? (*(sed)->src.cur): QSE_CHAR_EOF)

/* check if c is a space character */
#define IS_SPACE(c) (c == QSE_T(' ') || c == QSE_T('\t'))
#define IS_LINTERM(c) (c == QSE_T('\n') || c == QSE_T('\r'))
#define IS_WSPACE(c) (IS_SPACE(c) || IS_LINTERM(c))

/* check if c is a label terminator excluding a space character */
#define IS_CMDTERM(c) \
	(c == QSE_CHAR_EOF || c == QSE_T('#') || \
	 c == QSE_T(';') || IS_LINTERM(c))

static void* compile_regex (qse_sed_t* sed, qse_char_t rxend)
{
	void* code;
	qse_cint_t c;

	qse_str_clear (&sed->rexbuf);

	for (;;)
	{
		ADVSCP (sed);
		c = CURSC (sed);
		if (c == QSE_CHAR_EOF || c == QSE_T('\n'))
		{
			sed->errnum = QSE_SED_ETMTXT;
			return QSE_NULL;
		}

		if (c == rxend) break;

		if (c == QSE_T('\\'))
		{
			ADVSCP (sed);
			c = CURSC (sed);
			if (c == QSE_CHAR_EOF || c == QSE_T('\n'))
			{
				sed->errnum = QSE_SED_ETMTXT;
				return QSE_NULL;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
			else if (c == QSE_T('r')) c = QSE_T('\r');
			// TODO: support more escaped characters??
		}

		if (qse_str_ccat (&sed->rexbuf, c) == (qse_size_t)-1)
		{
			sed->errnum = QSE_SED_ENOMEM;
			return QSE_NULL;
		}
	} 

	/* TODO: maximum depth - optionize the second  parameter */
	code = qse_buildrex (
		sed->mmgr, 0, 
		QSE_STR_PTR(&sed->rexbuf), 
		QSE_STR_LEN(&sed->rexbuf), 
		QSE_NULL);
	if (code == QSE_NULL)
	{
		sed->errnum = QSE_SED_EREXBL;
		return QSE_NULL;
	}

	sed->lastrex = code;
	return code;
}

static qse_sed_a_t* address (qse_sed_t* sed, qse_sed_a_t* a)
{
	qse_cint_t c;

	c = CURSC (sed);
	if (c == QSE_T('$'))
	{
		a->type = QSE_SED_A_DOL;
		ADVSCP (sed);
	}
	else if (c == QSE_T('/'))
	{
		ADVSCP (sed);
		if (compile_regex (sed, c) == QSE_NULL) 
			return QSE_NULL;

		a->u.rex = sed->lastrex;
		a->type = QSE_SED_A_REX;
	}
	else if (c >= QSE_T('0') && c <= QSE_T('9'))
	{
		qse_sed_line_t lno = 0;
		do
		{
			lno = lno * 10 + c - QSE_T('0');
			ADVSCP (sed);
		}
		while ((c = CURSC(sed)) >= QSE_T('0') && c <= QSE_T('9'));

		/* line number 0 is illegal */
		if (lno == 0) return QSE_NULL;

		a->type = QSE_SED_A_LINE;
		a->u.line = lno;
	}
	else if (c == QSE_T('\\'))
	{
		/* TODO */
	}
	else
	{
		a->type = QSE_SED_A_NONE;
	}

	return a;
}


/* get the text for the 'a', 'i', and 'c' commands.
 * POSIX:
 *  The argument text shall consist of one or more lines. Each embedded 
 *  <newline> in the text shall be preceded by a backslash. Other backslashes
 *  in text shall be removed, and the following character shall be treated
 *  literally. */
static int get_text (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
#define ADD(sed,str,c,errlabel) \
do { \
	if (qse_str_ccat (str, c) == (qse_size_t)-1) \
	{ \
		sed->errnum = QSE_SED_ENOMEM; \
		goto errlabel; \
	} \
} while (0)

	qse_cint_t c;
	qse_str_t* t = QSE_NULL;

	t = qse_str_open (sed->mmgr, 0, 128);
	if (t == QSE_NULL) goto oops;

	do 
	{
		c = CURSC (sed);

		if (sed->option & QSE_SED_STRIPLS)
		{
			/* get the first non-space character */
			while (IS_SPACE(c)) c = NXTSC (sed);
		}

		while (c != QSE_CHAR_EOF)
		{
			int nl = 0;

			if (c == QSE_T('\\'))
			{
				c = NXTSC (sed);
				if (c == QSE_CHAR_EOF) 
				{
					if (sed->option & QSE_SED_KEEPTBS) 
						ADD (sed, t, QSE_T('\\'), oops);

					break;
				}
			}
			else if (c == QSE_T('\n')) nl = 1;

			ADD (sed, t, c, oops);

			if (c == QSE_T('\n'))
			{
				ADVSCP (sed);
				if (nl) goto done;
				break;
			}

			c = NXTSC (sed);
		} 
	}
	while (c != QSE_CHAR_EOF);

done:
	if ((sed->option & QSE_SED_ENSURENL) && c != QSE_T('\n'))
	{
		ADD (sed, t, QSE_T('\n'), oops);
	}

	qse_str_yield (t, &cmd->u.text, 0);
	qse_str_close (t);
	return 0;

oops:
	if (t != QSE_NULL) qse_str_close (t);
	return -1;

#undef ADD
}

static int get_label (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c;
	qse_str_t* t = QSE_NULL; /* TODO: move this buffer to sed */

	/* skip white spaces */
	c = CURSC (sed);
	while (IS_SPACE(c)) c = NXTSC (sed);

	if (IS_CMDTERM(c))
	{
		/* label name is empty */
		sed->errnum = QSE_SED_ELABEM;
		goto oops;
	}

/* TODO: change t to qse_str_t t; and ues qse_str_yield(t) to remember
 * branch text - in that case make '\0' an illegal character for the label 
 * name or can remember the length for the text for '\0' to be legal */
	t = qse_str_open (sed->mmgr, 0, 32);
	if (t == QSE_NULL) 
	{
		sed->errnum = QSE_SED_ENOMEM;
		goto oops;
	}

	do
	{
		if (qse_str_ccat (t, c) == (qse_size_t)-1) 
		{
			sed->errnum = QSE_SED_ENOMEM;
			goto oops;
		} 
		c = NXTSC (sed);
	}
	while (!IS_CMDTERM(c) && !IS_SPACE(c)) ;

	if (qse_map_search (
		&sed->labs, QSE_STR_PTR(t), QSE_STR_LEN(t)) != QSE_NULL)
	{
		sed->errnum = QSE_SED_ELABDU;
		goto oops;
	}

	if (qse_map_insert (
		&sed->labs, QSE_STR_PTR(t), QSE_STR_LEN(t), cmd, 0) == QSE_NULL)
	{
		sed->errnum = QSE_SED_ENOMEM;
		goto oops;;
	}

	/* the label can be followed by a command on the same line without 
	 * a semicolon as in ':label p'. */
	if (c != QSE_T('#') && c != QSE_CHAR_EOF) ADVSCP (sed);	

	qse_str_close (t);
	return 0;

oops:
	if (t != QSE_NULL) qse_str_close (t);
	return -1;
}

static int terminate_command (qse_sed_t* sed)
{
	qse_cint_t c;

	c = CURSC (sed);
	while (IS_SPACE(c)) c = NXTSC (sed);
	if (!IS_CMDTERM(c))
	{
		sed->errnum = QSE_SED_ESCEXP;
		return -1;
	}

	/* if the target is terminated by #, it should let the caller 
	 * to skip the comment text. so don't read in the next character */
	if (c != QSE_T('#') && c != QSE_CHAR_EOF) ADVSCP (sed);	
	return 0;
}

static int get_branch_target (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c;
	qse_str_t* t = QSE_NULL;
	qse_map_pair_t* pair;

	/* skip white spaces */
	c = CURSC(sed);
	while (IS_SPACE(c)) c = NXTSC (sed);

	if (IS_CMDTERM(c))
	{
		/* no branch target is given -
		 * a branch command without a target should cause 
		 * sed to jump to the end of a script.
		 */
		cmd->u.branch.label.ptr = QSE_NULL;
		cmd->u.branch.label.len = 0;
		cmd->u.branch.target = QSE_NULL;
		return terminate_command (sed);
	}

	t = qse_str_open (sed->mmgr, 0, 32);
	if (t == QSE_NULL) 
	{
		sed->errnum = QSE_SED_ENOMEM;
		goto oops;
	}

	do
	{
		if (qse_str_ccat (t, c) == (qse_size_t)-1) 
		{
			sed->errnum = QSE_SED_ENOMEM;
			goto oops;
		} 

		c = NXTSC (sed);
	}
	while (!IS_CMDTERM(c) && !IS_SPACE(c));

	if (terminate_command (sed) == -1) goto oops;

	pair = qse_map_search (&sed->labs, QSE_STR_PTR(t), QSE_STR_LEN(t));
	if (pair == QSE_NULL)
	{
		/* label not resolved yet */
		qse_str_yield (t, &cmd->u.branch.label, 0);
		cmd->u.branch.target = QSE_NULL;
	}
	else
	{
		cmd->u.branch.label.ptr = QSE_NULL;
		cmd->u.branch.label.len = 0;
		cmd->u.branch.target = QSE_MAP_VPTR(pair);
	}
	
	qse_str_close (t);
	return 0;

oops:
	if (t != QSE_NULL) qse_str_close (t);
	return -1;
}

static int get_file_name (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c;
	qse_str_t* t = QSE_NULL;
	qse_size_t trailing_spaces = 0;

	/* skip white spaces */
	c = CURSC(sed);
	while (IS_SPACE(c)) c = NXTSC (sed);

	if (IS_CMDTERM(c))
	{
		sed->errnum = QSE_SED_EFILEM;
		goto oops;	
	}

	t = qse_str_open (sed->mmgr, 0, 32);
	if (t == QSE_NULL) 
	{
		sed->errnum = QSE_SED_ENOMEM;
		goto oops;
	}

	do
	{
		if (c == QSE_T('\0'))
		{
			/* the file name should not contain '\0' */
			sed->errnum = QSE_SED_EFILIL;
			goto oops;
		}

		if (IS_SPACE(c)) trailing_spaces++;
		else trailing_spaces = 0;

		if (c == QSE_T('\\'))
		{
			c = NXTSC (sed);
			if (c == QSE_T('\0') ||
			    c == QSE_CHAR_EOF || 
			    IS_LINTERM(c))
			{
				sed->errnum = QSE_SED_EFILIL;
				goto oops;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
			else if (c == QSE_T('r')) c = QSE_T('\r');
		}

		if (qse_str_ccat (t, c) == (qse_size_t)-1) 
		{
			sed->errnum = QSE_SED_ENOMEM;
			goto oops;
		} 

		c = NXTSC (sed);
	}
	while (!IS_CMDTERM(c));

	if (terminate_command (sed) == -1) goto oops;

	if (trailing_spaces > 0)
	{
		qse_str_setlen (t, QSE_STR_LEN(t) - trailing_spaces);
	}

	qse_str_yield (t, &cmd->u.filename, 0);
	qse_str_close (t);
	return 0;

oops:
	if (t != QSE_NULL) qse_str_close (t);
	return -1;
}

static int get_subst (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c, delim;
	qse_str_t* t = QSE_NULL;

	c = CURSC (sed);
	if (c == QSE_CHAR_EOF || IS_LINTERM(c))
	{
		//sed->errnum = QSE_SED_ESUNTR;
		goto oops;
	}

	delim = c;	
	if (delim == QSE_T('\\'))
	{
		/* illegal delimiter */
		//sed->errnum = QSE_SED_ESUILD;
		goto oops;
	}

	t = qse_str_open (sed->mmgr, 0, 32);
	if (t == QSE_NULL) 
	{
		sed->errnum = QSE_SED_ENOMEM;
		goto oops;
	}

	c = NXTSC (sed);
	while (c != delim)
	{
	}

oops:
	if (t != QSE_NULL) qse_str_close (t);
	return -1;
}

static int get_transet (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c, delim;
	qse_str_t* t = QSE_NULL;
	qse_size_t pos;

	c = CURSC (sed);
	if (c == QSE_CHAR_EOF || IS_LINTERM(c))
	{
		/* translation set terminated prematurely*/
		sed->errnum = QSE_SED_ETSNTR;
		goto oops;
	}

	delim = c;	

	t = qse_str_open (sed->mmgr, 0, 32);
	if (t == QSE_NULL) 
	{
		sed->errnum = QSE_SED_ENOMEM;
		goto oops;
	}

	c = NXTSC (sed);
	while (c != delim)
	{
		qse_char_t b[2];

		if (c == QSE_CHAR_EOF || IS_LINTERM(c))
		{
			sed->errnum = QSE_SED_ETSNTR;
			goto oops;
		}

		if (c == QSE_T('\\'))
		{
			c = NXTSC (sed);
			if (c == QSE_CHAR_EOF || IS_LINTERM(c))
			{
				sed->errnum = QSE_SED_ETSNTR;
				goto oops;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
			else if (c == QSE_T('r')) c = QSE_T('\r');
		}

		b[0] = c;
		if (qse_str_ncat (t, b, 2) == (qse_size_t)-1)
		{
			sed->errnum = QSE_SED_ENOMEM;
			goto oops;
		}

		c = NXTSC (sed);
	}	

	c = NXTSC (sed);
	for (pos = 1; c != delim; pos += 2)
	{
		if (c == QSE_CHAR_EOF || IS_LINTERM(c))
		{
			sed->errnum = QSE_SED_ETSNTR;
			goto oops;
		}

		if (c == QSE_T('\\'))
		{
			c = NXTSC (sed);
			if (c == QSE_CHAR_EOF || IS_LINTERM(c))
			{
				sed->errnum = QSE_SED_ETSNTR;
				goto oops;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
			else if (c == QSE_T('r')) c = QSE_T('\r');
		}

		if (pos >= QSE_STR_LEN(t))
		{
			/* source and target not the same length */
			sed->errnum = QSE_SED_ETSNSL;
			goto oops;
		}

		QSE_STR_CHAR(t,pos) = c;
		c = NXTSC (sed);
	}

	if (pos < QSE_STR_LEN(t))
	{
		/* source and target not the same length */
		sed->errnum = QSE_SED_ETSNSL;
		goto oops;
	}

	ADVSCP (sed);
	if (terminate_command (sed) == -1) goto oops;

	qse_str_yield (t, &cmd->u.transet, 0);
	qse_str_close (t);
	return 0;

oops:
	if (t != QSE_NULL) qse_str_close (t);
	return -1;
}

static int command (qse_sed_t* sed)
{
	qse_cint_t c;
	qse_sed_cmd_t* cmd = sed->cmd.cur;

restart:
	c = CURSC (sed);
	switch (c)
	{
		default:
qse_printf (QSE_T("command not recognized [%c]\n"), c);
			sed->errnum = QSE_SED_ECMDNR;
			return -1;

		case QSE_T(':'):
			/* label - this is not a command */
			cmd->type = c;
			if (cmd->a1.type != QSE_SED_A_NONE)
			{
				/* label cannot have an address */
				sed->errnum = QSE_SED_EA1PHB;
				return -1;
			}

			ADVSCP (sed);
			if (get_label (sed, cmd) == -1) return -1;
			goto restart;

		case QSE_T('{'):
			/* insert a negated branch command at the beginning 
			 * of a group. this way, all the commands in a group
			 * can be skipped. the branch target is set once a
			 * corresponding } is met. */
			cmd->type = QSE_SED_CMD_B;
			cmd->negfl = !cmd->negfl;

			if (sed->grplvl >= QSE_COUNTOF(sed->grpcmd))
			{
				/* group nesting too deep */
				sed->errnum = QSE_SED_EGRNTD;	
				return -1;
			}

			sed->grpcmd[sed->grplvl++] = cmd;
			ADVSCP (sed);
			break;

		case QSE_T('}'):
			if (sed->grplvl <= 0) 
			{
				/* group not balanced */
				sed->errnum = QSE_SED_EGRNBA;
				return -1;
			}

			sed->grpcmd[--sed->grplvl]->u.branch.target = cmd;
			ADVSCP (sed);
			return 0;

		case QSE_T('='):
		case QSE_T('q'):
		case QSE_T('Q'):
			cmd->type = c;
			if (cmd->a2.type != QSE_SED_A_NONE)
			{
				sed->errnum = QSE_SED_EA2PHB;
				return -1;
			}

			ADVSCP (sed);
			if (terminate_command (sed) == -1) return -1;
printf ("command %c\n", cmd->type);
			break;


		case QSE_T('a'):
		case QSE_T('i'):
		case QSE_T('c'):
		{
			cmd->type = c;

			/* TODO: this check for  A and I
			if (cmd->a2.type != QSE_SED_A_NONE)
			{
				sed->errnum = QSE_SED_EA2PHB;
				return -1;
			}
			*/

			c = NXTSC (sed);
			while (IS_SPACE(c)) c = NXTSC (sed);

			if (c != QSE_T('\\'))
			{
				sed->errnum = QSE_SED_EBSEXP;
				return -1;
			}
		
			c = NXTSC (sed);
			while (IS_SPACE(c)) c = NXTSC (sed);

			if (c != QSE_CHAR_EOF && c != QSE_T('\n'))
			{
				sed->errnum = QSE_SED_EGBABS;
				return -1;
			}
			
			ADVSCP (sed); /* skip the new line */

			/* get_text() starts from the next line */
			if (get_text (sed, cmd) == -1) return -1;

{
qse_char_t ttt[1000];
qse_fgets (ttt, QSE_COUNTOF(ttt), QSE_STDIN);
qse_printf (QSE_T("%s%s"), ttt, cmd->u.text.ptr);
}
			break;
		}


		case QSE_T('d'):
		case QSE_T('D'):
		case QSE_T('h'):
		case QSE_T('H'):
		case QSE_T('g'):
		case QSE_T('G'):
		case QSE_T('l'):
		case QSE_T('n'):
		case QSE_T('N'):
		case QSE_T('p'):
		case QSE_T('P'):
		case QSE_T('x'):
			cmd->type = c;
			ADVSCP (sed);
			if (terminate_command (sed) == -1) return -1;
printf ("command %c\n", cmd->type);
			break;

		case QSE_T('b'):
		case QSE_T('t'):
			cmd->type = c;
			ADVSCP (sed);
			if (get_branch_target (sed, cmd) == -1) return -1;
if (cmd->u.branch.label.ptr != NULL)
{
qse_printf (QSE_T("cmd->u.branch.label = [%.*s]\n"), 
	cmd->u.branch.label.len, cmd->u.branch.label.ptr);
}
else
{
qse_printf (QSE_T("cmd->u.branch.target = [%p]\n"), cmd->u.branch.target);
}
			break;

		case QSE_T('r'):
		case QSE_T('R'):
		case QSE_T('w'):
		case QSE_T('W'):
			cmd->type = c;
			ADVSCP (sed);
			if (get_file_name (sed, cmd) == -1) return -1;

qse_printf (QSE_T("cmd->u.filename= [%.*s]\n"), (int)cmd->u.filename.len, cmd->u.filename.ptr);
			break;


		case QSE_T('s'):
			cmd->type = c;
			ADVSCP (sed);
			if (get_subst (sed, cmd) == -1) return -1;
			break;

		case QSE_T('y'):
			cmd->type = c;
			ADVSCP (sed);
			if (get_transet (sed, cmd) == -1) return -1;
			break;
	}

	return 1;
}

static int compile_source (
	qse_sed_t* sed, const qse_char_t* ptr, qse_size_t len)
{
	qse_cint_t c;
	qse_sed_cmd_t* cmd = sed->cmd.cur;

	/* store the source code pointers */
	sed->src.ptr = ptr;
	sed->src.end = ptr + len;
	sed->src.cur = ptr;
	
	/* 
	 * # comment
	 * :label
	 * zero-address-command
	 * address[!] one-address-command
	 * address-range[!] address-range-command
	 */
	while (1)
	{
		int n;

		c = CURSC (sed);
		
		/* skip white spaces and comments*/
		while (IS_WSPACE(c)) c = NXTSC (sed);
		if (c == QSE_T('#'))
		{
			do c = NXTSC (sed); while (!IS_LINTERM(c));
			ADVSCP (sed);
			continue;
		}

		/* check if it has reached the end or is commented */
		if (c == QSE_CHAR_EOF) break;

		if (c == QSE_T(';')) 
		{
			/* semicolon without a address-command pair */
			ADVSCP (sed);
			continue;
		}

		/* initialize the current command */
		QSE_MEMSET (cmd, 0, QSE_SIZEOF(*cmd));

		/* process address */
		if (address (sed, &cmd->a1) == QSE_NULL) return -1;

		c = CURSC (sed);
		if (cmd->a1.type != QSE_SED_A_NONE)
		{
			/* if (cmd->a1.type == QSE_SED_A_LAST)
			{
				 // TODO: ????
			} */
			if (c == QSE_T(',') || c == QSE_T(';'))
			{
				/* maybe an address range */
				ADVSCP (sed);

				/* TODO: skip white spaces??? */
				if (address (sed, &cmd->a2) == QSE_NULL) 
					return -1;

				c = CURSC (sed);
			}
			else cmd->a2.type = QSE_SED_A_NONE;
		}

		/* skip white spaces */
		while (IS_SPACE(c)) c = NXTSC (sed);

		if (c == QSE_T('!'))
		{
			/* negate */
			cmd->negfl = 1;
		}

		n = command (sed);
		if (n == -1) return -1;
		if (n > 0)
		{
			QSE_ASSERT (n == 1);

			if (sed->cmd.cur >= sed->cmd.end)
			{
				/* TODO: too many commands. change errnum */
				sed->errnum = QSE_SED_ENOMEM; 
				return -1;
			}

			cmd = ++sed->cmd.cur;
		}
	}

	if (sed->grplvl != 0)
	{
		sed->errnum = QSE_SED_EGRNBA;	
		return -1;
	}

	return 0;
}

int qse_sed_compile (qse_sed_t* sed, const qse_char_t* sptr, qse_size_t slen)
{
	return compile_source (sed, sptr, slen);	
}
