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
#include <qse/cmn/chr.h>

/* TODO: delete stdio.h */
#include <qse/utl/stdio.h>

QSE_IMPLEMENT_COMMON_FUNCTIONS (sed)

static void free_command (qse_sed_t* sed, qse_sed_cmd_t* cmd);

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

	if (qse_lda_init (&sed->text.appended, mmgr, 32) == QSE_NULL)
	{
		QSE_MMGR_FREE (sed->mmgr, sed->cmd.buf);
		qse_map_fini (&sed->labs);
		qse_str_fini (&sed->rexbuf);
		return QSE_NULL;
	}

	if (qse_str_init (&sed->text.held, mmgr, 256) == QSE_NULL)
	{
		qse_lda_fini (&sed->text.appended);
		QSE_MMGR_FREE (sed->mmgr, sed->cmd.buf);
		qse_map_fini (&sed->labs);
		qse_str_fini (&sed->rexbuf);
		return QSE_NULL;
	
	}

	return sed;
}

void qse_sed_fini (qse_sed_t* sed)
{
	qse_str_fini (&sed->text.held);
	qse_lda_fini (&sed->text.appended);

/* TODO: use different data sturect -> look at qse_sed_init */
qse_sed_cmd_t* c;
for (c = sed->cmd.buf; c != sed->cmd.cur; c++)
{
	free_command (sed, c);
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
		QSE_T("command missing"),
		QSE_T("command garbled"),
		QSE_T("regular expression build error"),
		QSE_T("regular expression match error"),
		QSE_T("address 1 prohibited"),
		QSE_T("address 2 prohibited"),
		QSE_T("a new line expected"),
		QSE_T("a backslash expected"),
		QSE_T("a backslash used as a delimiter"),
		QSE_T("garbage after a backslash"),
		QSE_T("a semicolon expected"),
		QSE_T("label name too long"),
		QSE_T("empty label name"),
		QSE_T("duplicate label name"),
		QSE_T("label not found"),
		QSE_T("empty file name"),
		QSE_T("illegal file name"),
		QSE_T("command not terminated properly"),
		QSE_T("strings in translation set not the same length"),
		QSE_T("group brackets not balanced"),
		QSE_T("group nesting too deep"),
		QSE_T("multiple occurrence specifier"),
		QSE_T("occurrence specifier is zero"),
		QSE_T("occurrence specifier too large"),
		QSE_T("error returned by user io handler")
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

static void free_address (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	if (cmd->a2.type == QSE_SED_A_REX)
	{
		QSE_ASSERT (cmd->a2.u.rex != QSE_NULL);
		qse_freerex (sed->mmgr, cmd->a2.u.rex);
		cmd->a2.type = QSE_SED_A_NONE;
	}
	if (cmd->a1.type == QSE_SED_A_REX)
	{
		QSE_ASSERT (cmd->a1.u.rex != QSE_NULL);
		qse_freerex (sed->mmgr, cmd->a1.u.rex);
		cmd->a1.type = QSE_SED_A_NONE;
	}
}

static void free_command (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	free_address (sed, cmd);

	switch (cmd->type)
	{
		case QSE_SED_CMD_APPEND:
		case QSE_SED_CMD_INSERT:
		case QSE_SED_CMD_CHANGE:
			if (cmd->u.text.ptr != QSE_NULL) 
				QSE_MMGR_FREE (sed->mmgr, cmd->u.text.ptr);
			break;

		case QSE_SED_CMD_BRANCH:
		case QSE_SED_CMD_T:
			if (cmd->u.branch.label.ptr != QSE_NULL) 
				QSE_MMGR_FREE (sed->mmgr, cmd->u.branch.label.ptr);
			break;
	
		case QSE_SED_CMD_S:
			if (cmd->u.subst.file.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.subst.file.ptr);
			if (cmd->u.subst.rpl.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.subst.rpl.ptr);
			if (cmd->u.subst.rex != QSE_NULL)
				qse_freerex (sed->mmgr, cmd->u.subst.rex);
			break;

		case QSE_SED_CMD_Y:
			if (cmd->u.transet.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.transet.ptr);
			break;

		case QSE_SED_CMD_R:
		case QSE_SED_CMD_RR:
		case QSE_SED_CMD_W:
		case QSE_SED_CMD_WW:
			if (cmd->u.file.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.file.ptr);
			break;
	}
}

static void* compile_rex (qse_sed_t* sed, qse_char_t rxend)
{
	void* code;
	qse_cint_t c;

	qse_str_clear (&sed->rexbuf);

	for (;;)
	{
		c = NXTSC (sed);
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
			// TODO: support more escaped characters??
		}

		if (qse_str_ccat (&sed->rexbuf, c) == (qse_size_t)-1)
		{
			sed->errnum = QSE_SED_ENOMEM;
			return QSE_NULL;
		}
	} 

	/* TODO: maximum depth - optionize the second  parameter */
qse_printf (QSE_T("rexbuff=>[%.*s]\n"), 
(int)QSE_STR_LEN(&sed->rexbuf),
QSE_STR_PTR(&sed->rexbuf));

	code = qse_buildrex (
		sed->mmgr, 0, 
		QSE_STR_PTR(&sed->rexbuf), 
		QSE_STR_LEN(&sed->rexbuf), 
		QSE_NULL
	);
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
		if (compile_rex (sed, c) == QSE_NULL) 
			return QSE_NULL;

		a->u.rex = sed->lastrex;
		a->type = QSE_SED_A_REX;
		ADVSCP (sed);
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
		c = NXTSC (sed);
		if (c == QSE_CHAR_EOF || c == QSE_T('\n'))
		{
			/* TODO: change error code - 
			 *       unterminated address regular expression */
			sed->errnum = QSE_SED_ETMTXT;
			return QSE_NULL;
		}

		if (compile_rex (sed, c) == QSE_NULL) 
			return QSE_NULL;

		a->u.rex = sed->lastrex;
		a->type = QSE_SED_A_REX;
		ADVSCP (sed);
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

	if (terminate_command (sed) <= -1) goto oops;

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

static int get_file (qse_sed_t* sed, qse_xstr_t* xstr)
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
		}

		if (qse_str_ccat (t, c) == (qse_size_t)-1) 
		{
			sed->errnum = QSE_SED_ENOMEM;
			goto oops;
		} 

		c = NXTSC (sed);
	}
	while (!IS_CMDTERM(c));

	if (terminate_command (sed) <= -1) goto oops;

	if (trailing_spaces > 0)
	{
		qse_str_setlen (t, QSE_STR_LEN(t) - trailing_spaces);
	}

	qse_str_yield (t, xstr, 0);
	qse_str_close (t);
	return 0;

oops:
	if (t != QSE_NULL) qse_str_close (t);
	return -1;
}

static int get_subst (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c, delim;
	qse_str_t* t[2] = { QSE_NULL, QSE_NULL };
	int i;

	c = CURSC (sed);
	if (c == QSE_CHAR_EOF || IS_LINTERM(c))
	{
		/* not terminated properly */
		sed->errnum = QSE_SED_ENOTRM;
		goto oops;
	}

	delim = c;	
	if (delim == QSE_T('\\'))
	{
		/* backspace is an illegal delimiter */
		sed->errnum = QSE_SED_EBSDEL;
		goto oops;
	}

	for (i = 0; i < 2; i++)
	{	
		t[i] = qse_str_open (sed->mmgr, 0, 32);
		if (t[i] == QSE_NULL) 
		{
			sed->errnum = QSE_SED_ENOMEM;
			goto oops;
		}
	}


	for (i = 0; i < 2; i++)
	{
		c = NXTSC (sed);

		while (c != delim)
		{
			if (c == QSE_CHAR_EOF || IS_LINTERM(c))
			{
				sed->errnum = QSE_SED_ENOTRM;
				goto oops;
			}

			if (c == QSE_T('\\'))
			{
				c = NXTSC (sed);
				if (c == QSE_CHAR_EOF || IS_LINTERM(c))
				{
					sed->errnum = QSE_SED_ENOTRM;
					goto oops;
				}
	
				if (c == QSE_T('n')) c = QSE_T('\n');
			}

			if (qse_str_ccat (t[i], c) == (qse_size_t)-1)
			{
				sed->errnum = QSE_SED_ENOMEM;
				goto oops;
			}

			c = NXTSC (sed);
		}	
	}

	/* skip spaces before options */
	do { c = NXTSC(sed); } while (IS_SPACE(c));

	/* get options */
	do
	{
		if (c == QSE_T('p')) 
		{
			cmd->u.subst.p = 1;
			c = NXTSC (sed);
		}
		else if (c == QSE_T('i')) 
		{
			cmd->u.subst.i = 1;
			c = NXTSC (sed);
		}
		else if (c == QSE_T('g')) 
		{
			cmd->u.subst.g = 1;
			c = NXTSC (sed);
		}
		else if (c >= QSE_T('0') && c <= QSE_T('9'))
		{
			unsigned long occ;

			if (cmd->u.subst.occ != 0)
			{
				sed->errnum = QSE_SED_EOCSDU;
				goto oops;
			}

			occ = 0;

			do 
			{
				occ = occ * 10 + (c - QSE_T('0')); 
				if (occ > QSE_TYPE_MAX(unsigned short))
				{
					sed->errnum = QSE_SED_EOCSTL;
					goto oops;
				}
				c = NXTSC (sed); 
			}
			while (c >= QSE_T('0') && c <= QSE_T('9'));

			if (occ == 0)
			{
				sed->errnum = QSE_SED_EOCSZE;
				goto oops;
			}

			cmd->u.subst.occ = occ;
		}
		else if (c == QSE_T('w'))
		{
			ADVSCP (sed);
			if (get_file (sed, &cmd->u.subst.file) <= -1) return -1;
			break;
		}
		else break;
	}
	while (1);

	/* call terminate_command() if the 'w' option is not specified.
	 * if the 'w' option is given, it is called in get_file(). */
	if (cmd->u.subst.file.ptr == QSE_NULL &&
	    terminate_command (sed) <= -1) goto oops;

	QSE_ASSERT (cmd->u.subst.rex == QSE_NULL);
qse_printf (QSE_T("buildrex 222222\n"));
	cmd->u.subst.rex = qse_buildrex (
		sed->mmgr, 0, 
		QSE_STR_PTR(t[0]),
		QSE_STR_LEN(t[0]),
		QSE_NULL
	);
	if (cmd->u.subst.rex == QSE_NULL)
	{
		sed->errnum = QSE_SED_EREXBL;
		goto oops;
	}	

	qse_str_yield (t[1], &cmd->u.subst.rpl, 0);
	if (cmd->u.subst.g == 0 && cmd->u.subst.occ == 0) cmd->u.subst.occ = 1;

	qse_str_close (t[1]);
	qse_str_close (t[0]);

	return 0;

oops:
	if (t[1] != QSE_NULL) qse_str_close (t[1]);
	if (t[0] != QSE_NULL) qse_str_close (t[0]);
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
		sed->errnum = QSE_SED_ENOTRM;
		goto oops;
	}

	delim = c;	
	if (delim == QSE_T('\\'))
	{
		/* backspace is an illegal delimiter */
		sed->errnum = QSE_SED_EBSDEL;
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
		qse_char_t b[2];

		if (c == QSE_CHAR_EOF || IS_LINTERM(c))
		{
			sed->errnum = QSE_SED_ENOTRM;
			goto oops;
		}

		if (c == QSE_T('\\'))
		{
			c = NXTSC (sed);
			if (c == QSE_CHAR_EOF || IS_LINTERM(c))
			{
				sed->errnum = QSE_SED_ENOTRM;
				goto oops;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
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
			sed->errnum = QSE_SED_ENOTRM;
			goto oops;
		}

		if (c == QSE_T('\\'))
		{
			c = NXTSC (sed);
			if (c == QSE_CHAR_EOF || IS_LINTERM(c))
			{
				sed->errnum = QSE_SED_ENOTRM;
				goto oops;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
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
	if (terminate_command (sed) <= -1) goto oops;

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
		case QSE_CHAR_EOF:
			sed->errnum = QSE_SED_ECMDMS;
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
			if (get_label (sed, cmd) <= -1) return -1;
			goto restart;

		case QSE_T('{'):
			/* insert a negated branch command at the beginning 
			 * of a group. this way, all the commands in a group
			 * can be skipped. the branch target is set once a
			 * corresponding } is met. */
			cmd->type = QSE_SED_CMD_BRANCH;
			cmd->negated = !cmd->negated;

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

		case QSE_T('q'):
		case QSE_T('Q'):
			cmd->type = c;
			if (cmd->a2.type != QSE_SED_A_NONE)
			{
				sed->errnum = QSE_SED_EA2PHB;
				return -1;
			}

			ADVSCP (sed);
			if (terminate_command (sed) <= -1) return -1;
qse_printf (QSE_T("command %c\n"), cmd->type);
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
			if (get_text (sed, cmd) <= -1) return -1;

			break;
		}

		case QSE_T('='):
			if (sed->option & QSE_SED_CLASSIC &&
			    cmd->a2.type != QSE_SED_A_NONE)
			{
				sed->errnum = QSE_SED_EA2PHB;
				return -1;
			}
		case QSE_T('p'):
		case QSE_T('P'):
		case QSE_T('l'):

		case QSE_T('d'):
		case QSE_T('D'):

		case QSE_T('h'):
		case QSE_T('H'):
		case QSE_T('g'):
		case QSE_T('G'):
		case QSE_T('x'):

		case QSE_T('n'):
		case QSE_T('N'):
			cmd->type = c;
			ADVSCP (sed);
			if (terminate_command (sed) <= -1) return -1;
			break;

		case QSE_T('b'):
		case QSE_T('t'):
			cmd->type = c;
			ADVSCP (sed);
			if (get_branch_target (sed, cmd) <= -1) return -1;
			break;

		case QSE_T('r'):
		case QSE_T('R'):
		case QSE_T('w'):
		case QSE_T('W'):
			cmd->type = c;
			ADVSCP (sed);
			if (get_file (sed, &cmd->u.file) <= -1) return -1;
			break;


		case QSE_T('s'):
			cmd->type = c;
			ADVSCP (sed);
			if (get_subst (sed, cmd) <= -1) return -1;
//qse_printf (QSE_T("rex= [%.*s]\n"), (int)cmd->u.subst.rex.len, cmd->u.subst.rex.ptr);
qse_printf (QSE_T("rpl= [%.*s]\n"), (int)cmd->u.subst.rpl.len, cmd->u.subst.rpl.ptr);
qse_printf (QSE_T("g=%u p=%u i=%u occ=%d\n"), 
	cmd->u.subst.g,
	cmd->u.subst.p,
	cmd->u.subst.i,
	cmd->u.subst.occ
);
qse_printf (QSE_T("w=[%.*s]\n"), 
	(int)cmd->u.subst.file.len,
	cmd->u.subst.file.ptr
);
			break;

		case QSE_T('y'):
			cmd->type = c;
			ADVSCP (sed);
			if (get_transet (sed, cmd) <= -1) return -1;
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
				{
					QSE_ASSERT (cmd->a2.type == QSE_SED_A_NONE);
					free_address (sed, cmd);
					return -1;
				}

				c = CURSC (sed);
			}
			else cmd->a2.type = QSE_SED_A_NONE;
		}

		/* skip white spaces */
		while (IS_SPACE(c)) c = NXTSC (sed);

		if (c == QSE_T('!'))
		{
			/* allow any number of the negation indicators */
			do { 
				cmd->negated = !cmd->negated; 
				c = NXTSC(sed);
			} 
			while (c== QSE_T('!'));

			while (IS_SPACE(c)) c = NXTSC (sed);
		}
	

		n = command (sed);
		if (n <= -1) 
		{
			free_address (sed, cmd);
			return -1;
		}
		if (n > 0)
		{
			QSE_ASSERT (n == 1);

			if (sed->cmd.cur >= sed->cmd.end)
			{
				/* TODO: too many commands. change errnum */
				free_command (sed, cmd);
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

static int read_char (qse_sed_t* sed, qse_char_t* c)
{
	qse_ssize_t n;

	if (sed->eio.in.xbuf_len == 0)
	{
		if (sed->eio.in.pos >= sed->eio.in.len)
		{
			n = sed->eio.in.f (
				sed, QSE_SED_IO_READ, 
				sed->eio.in.buf, QSE_COUNTOF(sed->eio.in.buf)
			);
	
			if (n <= -1) 
			{
				sed->errnum = QSE_SED_EIOUSR;
				return -1;
			}
	
			if (n == 0) return 0; /* end of file */
	
			sed->eio.in.len = n;
			sed->eio.in.pos = 0;
		}
	
		*c = sed->eio.in.buf[sed->eio.in.pos++];
		return 1;
	}
	else if (sed->eio.in.xbuf_len > 0)
	{
		QSE_ASSERT (sed->eio.in.xbuf_len == 1);
		*c = sed->eio.in.xbuf[--sed->eio.in.xbuf_len];
		return 1;
	}
	else /*if (sed->eio.in.xbuf_len < 0)*/
	{
		QSE_ASSERT (sed->eio.in.xbuf_len == -1);
		return 0;
	}	
}

static int read_line (qse_sed_t* sed)
{
	qse_char_t c;
	int n;

	qse_str_clear (&sed->eio.in.line);
	if (sed->eio.in.eof) 
	{
		/* no more input detected in the previous read.
		 * set eof back to 0 here so that read_char() is called
		 * if read_line() is called again. that way, the result
		 * of subsequent calls counts on read_char(). */
		sed->eio.in.eof = 0; 
		return 0;
	}

	while (1)
	{
		n = read_char (sed, &c);
		if (n <= -1) return -1;
		if (n == 0)
		{
			if (QSE_STR_LEN(&sed->eio.in.line) == 0) return 0;
			sed->eio.in.eof = 1;
			break;
		}

		if (qse_str_ccat (&sed->eio.in.line, c) == (qse_size_t)-1)
		{
			sed->errnum = QSE_SED_ENOMEM;
			return -1;
		}

		// TODO: different line convenstion
		if (c == QSE_T('\n')) break;
	}

	sed->eio.in.num++;
	return 1;	
}

static int flush (qse_sed_t* sed)
{
	qse_size_t pos = 0;
	qse_ssize_t n;

	while (sed->eio.out.len > 0)
	{
		n = sed->eio.out.f (
			sed, QSE_SED_IO_WRITE, 
			&sed->eio.out.buf[pos], sed->eio.out.len
		);

		if (n <= -1)
		{
			sed->errnum = QSE_SED_EIOUSR;
			return -1;
		}

		if (n == 0)
		{
			/* reached the end of file - anything to do? */
		}

		pos += n;
		sed->eio.out.len -= n;
	}

	return 0;
}

static int write_char (qse_sed_t* sed, qse_char_t c)
{
	sed->eio.out.buf[sed->eio.out.len++] = c;
	if (c == QSE_T('\n') ||
	    sed->eio.out.len >= QSE_COUNTOF(sed->eio.out.buf))
	{
		return flush (sed);
	}

	return 0;
}

static int write_str (qse_sed_t* sed, const qse_char_t* str, qse_size_t len)
{
	qse_size_t i;
	for (i = 0; i < len; i++)
	{
		if (write_char (sed, str[i]) <= -1) return -1;
	}
	return 0;
}

static int write_num (qse_sed_t* sed, qse_size_t x, int base, int width)
{
	qse_size_t last = x % base;
	qse_size_t y = 0; 
	int dig = 0;

	QSE_ASSERT (base >= 2 && base <= 10);

	if (x < 0) 
	{
		if (write_char (sed, QSE_T('-')) <= -1) return -1;
		if (width > 0) width--;
	}

	x = x / base;
	if (x < 0) x = -x;

	while (x > 0)
	{
		y = y * base + (x % base);
		x = x / base;
		dig++;
	}

	if (width > 0)
	{
		while (--width > dig)
		{
			if (write_char (sed, QSE_T('0')) <= -1) return -1;
		}
	}

	while (y > 0)
	{
		if (write_char (sed, (y % base) + QSE_T('0')) <= -1) return -1;
		y = y / base;
		dig--;
	}

	while (dig > 0) 
	{ 
		dig--; 
		if (write_char (sed, QSE_T('0')) <= -1) return -1;
	}
	if (last < 0) last = -last;
	if (write_char (sed, last + QSE_T('0')) <= -1) return -1;

	return 0;
}

#define WRITE_CHAR(sed,c) \
	do { if (write_char(sed,c) <= -1) return -1; } while (0)
#define WRITE_STR(sed,str,len) \
	do { if (write_str(sed,str,len) <= -1) return -1; } while (0)
#define WRITE_NUM(sed,num,base,width) \
	do { if (write_num(sed,num,base,width) <= -1) return -1; } while (0)

static int write_str_clearly (
	qse_sed_t* sed, const qse_char_t* str, qse_size_t len)
{
	const qse_char_t* p = str;
	const qse_char_t* end = str + len;

/* TODO: break down long lines.... */
	while (p < end)
	{
		qse_char_t c = *p++;

		switch (c)
		{
			case QSE_T('\\'):
				WRITE_STR (sed, QSE_T("\\\\"), 2);
				break;
			/*case QSE_T('\0'):
				WRITE_STR (sed, QSE_T("\\0"), 2);
				break;*/
			case QSE_T('\n'):
				WRITE_STR (sed, QSE_T("$\n"), 2);
				break;
			case QSE_T('\a'):
				WRITE_STR (sed, QSE_T("\\a"), 2);
				break;
			case QSE_T('\b'):
				WRITE_STR (sed, QSE_T("\\b"), 2);
				break;
			case QSE_T('\f'):
				WRITE_STR (sed, QSE_T("\\f"), 2);
				break;
			case QSE_T('\r'):
				WRITE_STR (sed, QSE_T("\\r"), 2);
				break;
			case QSE_T('\t'):
				WRITE_STR (sed, QSE_T("\\t"), 2);
				break;
			case QSE_T('\v'):
				WRITE_STR (sed, QSE_T("\\v"), 2);
				break;
			default:
			{
				if (QSE_ISPRINT(c)) WRITE_CHAR (sed, c);
				else
				{
					qse_size_t i;
					qse_byte_t* b = (qse_byte_t*)&c;
					WRITE_CHAR (sed, QSE_T('\\'));
					for (i = 0; i < QSE_SIZEOF(c)/QSE_SIZEOF(*b); i++)
					{
						WRITE_NUM (sed, b[i], 8, 3);
					}
				}
			}
		}
	}		

	if (len > 1 && end[-1] != QSE_T('\n')) 
		WRITE_STR (sed, QSE_T("$\n"), 2);

	return 0;
}

static int match_a (qse_sed_t* sed, qse_sed_a_t* a)
{
	switch (a->type)
	{
		case QSE_SED_A_LINE:
			return (sed->eio.in.num == a->u.line)? 1: 0;

		case QSE_SED_A_REX:
		{
			qse_str_t match;
			int errnum, n;
			qse_str_t* line;
			qse_size_t llen;

			QSE_ASSERT (a->u.rex != QSE_NULL);

			line = &sed->eio.in.line;
			llen = QSE_STR_LEN(line);

			/* TODO: support different line end scheme */
			if (llen > 0 && 
			    QSE_STR_CHAR(line,llen-1) == QSE_T('\n')) llen--;

			n = qse_matchrex (
				sed->mmgr,
				0,
				a->u.rex,
				0,
				QSE_STR_PTR(line),
				llen,
				&match.ptr, &match.len, &errnum);
			if (n <= -1)
			{
				sed->errnum = QSE_SED_EREXMA;
				return -1;
			}			

			return n;
		}
		case QSE_SED_A_DOL:
		{
			qse_char_t c;
			int n;

			if (sed->eio.in.xbuf_len < 0)
			{
				/* we know that we've reached eof as it has
				 * been done so previously */
				return 1;
			}

			n = read_char (sed, &c);
			if (n <= -1) return -1;

			QSE_ASSERT (sed->eio.in.xbuf_len == 0);
			if (n == 0)
			{
				/* eof has been reached */
				sed->eio.in.xbuf_len--;
				return 1;
			}
			else
			{
				sed->eio.in.xbuf[sed->eio.in.xbuf_len++] = c;
				return 0;
			}
		}
	}

	QSE_ASSERT (a->type == QSE_SED_A_NONE);
	return 1; /* match */
}

/* match an address against input.
 * return -1 on error, 0 on no match, 1 on match. */
static int match_address (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	int n;

	cmd->state.c_ready = 0;
	if (cmd->a1.type == QSE_SED_A_NONE)
	{
		QSE_ASSERT (cmd->a2.type == QSE_SED_A_NONE);
		cmd->state.c_ready = 1;
		return 1;
	}
	else if (cmd->a2.type != QSE_SED_A_NONE)
	{
		/* two addresses */
		if (cmd->state.a1_matched)
		{
			n = match_a (sed, &cmd->a2);
			if (n <= -1) return -1;
			if (n == 0)
			{
				if (cmd->a2.type == QSE_SED_A_LINE &&
				    sed->eio.in.num > cmd->a2.u.line)
				{
					/* exit the range */
					cmd->state.a1_matched = 0;
					return 0;
				}

				/* still in the range. return match 
				 * despite the actual mismatch */
				return 1;
			}

			/* exit the range */
			cmd->state.a1_matched = 0;
			cmd->state.c_ready = 1;
			return 1;
		}
		else 
		{
			n = match_a (sed, &cmd->a1);
			if (n <= -1) return -1;
			if (n == 0) 
			{
				return 0;
			}

			if (cmd->a2.type == QSE_SED_A_LINE &&
			    sed->eio.in.num >= cmd->a2.u.line) 
			{
				/* the line number specified in the second 
				 * address is equal to or less than the current
				 * line number. */
				cmd->state.c_ready = 1;
			}
			else
			{
				/* mark that the first is matched so as to
				 * move on to the range test */
				cmd->state.a1_matched = 1;
			}

			return 1;
		}
	}
	else
	{
		/* single address */
		cmd->state.c_ready = 1;

		n = match_a (sed, &cmd->a1);
		return (n <= -1)? -1:
		       (n ==  0)? 0: 1;
	}
}

static qse_sed_cmd_t* exec_cmd (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	int n;
	qse_sed_cmd_t* jumpto = QSE_NULL;

	switch (cmd->type)
	{
		case QSE_SED_CMD_QUIT:
			n = write_str (sed, 
				QSE_STR_PTR(&sed->eio.in.line),
				QSE_STR_LEN(&sed->eio.in.line));
			if (n <= -1) return QSE_NULL;
		case QSE_SED_CMD_QUIT_QUIET:
			jumpto = sed->cmd.cur + 1;
			break;

		case QSE_SED_CMD_APPEND:
			if (qse_lda_insert (
				&sed->text.appended,
				QSE_LDA_SIZE(&sed->text.appended),	
				&cmd->u.text, 0) == (qse_size_t)-1) 
			{
				sed->errnum = QSE_SED_ENOMEM;
				return QSE_NULL;
			}
			break;

		case QSE_SED_CMD_INSERT:
			n = write_str (sed,
				QSE_STR_PTR(&cmd->u.text),
				QSE_STR_LEN(&cmd->u.text));
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_CHANGE:
			if (cmd->state.c_ready)
			{
				/* change the input space */
				n = qse_str_ncpy (
					&sed->eio.in.line,
					QSE_STR_PTR(&cmd->u.text),
					QSE_STR_LEN(&cmd->u.text));
				if (n == (qse_size_t)-1) 
				{
					sed->errnum = QSE_SED_ENOMEM;
					return QSE_NULL;
				}

				/* move past the last command so as to start 
				 * the next cycle */
				jumpto = sed->cmd.cur;
			}
			else 
			{		
/* TODO: prearrange for  CHANGE to be executed on the lastline wihtout
         matchng the second address */
				qse_str_clear (&sed->eio.in.line);
			}

			break;

		case QSE_SED_CMD_DELETE:
			/* delete the input space */
			qse_str_clear (&sed->eio.in.line);
			/* move past the last command so as to start 
			 * the next cycle */
			jumpto = sed->cmd.cur;
			break;

		case QSE_SED_CMD_PRINT_LNUM:
			if (write_num (sed, sed->eio.in.num, 10, 0) <= -1) return QSE_NULL;
			if (write_char (sed, QSE_T('\n')) <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_PRINT:
			n = write_str (
				sed, 
				QSE_STR_PTR(&sed->eio.in.line),
				QSE_STR_LEN(&sed->eio.in.line)
			);
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_PRINT_CLEARLY:
			n = write_str_clearly (
				sed,
				QSE_STR_PTR(&sed->eio.in.line),
				QSE_STR_LEN(&sed->eio.in.line)
			);
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_HOLD:
			if (qse_str_ncpy (&sed->text.held, 	
				QSE_STR_PTR(&sed->eio.in.line),
				QSE_STR_LEN(&sed->eio.in.line)) == (qse_size_t)-1)
			{
				sed->errnum = QSE_SED_ENOMEM;
				return QSE_NULL;	
			}
			break;
				
		case QSE_SED_CMD_HOLD_APPEND:
			if (qse_str_ncat (&sed->text.held, 	
				QSE_STR_PTR(&sed->eio.in.line),
				QSE_STR_LEN(&sed->eio.in.line)) == (qse_size_t)-1)
			{
				sed->errnum = QSE_SED_ENOMEM;
				return QSE_NULL;	
			}
			break;

		case QSE_SED_CMD_RELEASE:
			if (qse_str_ncpy (&sed->eio.in.line,
				QSE_STR_PTR(&sed->text.held),
				QSE_STR_LEN(&sed->text.held)) == (qse_size_t)-1)
			{
				sed->errnum = QSE_SED_ENOMEM;
				return QSE_NULL;	
			}
			break;

		case QSE_SED_CMD_EXCHANGE:
			qse_str_swap (&sed->eio.in.line, &sed->text.held);
			break;

		case QSE_SED_CMD_RELEASE_APPEND:
			if (qse_str_ncat (&sed->eio.in.line,
				QSE_STR_PTR(&sed->text.held),
				QSE_STR_LEN(&sed->text.held)) == (qse_size_t)-1)
			{
				sed->errnum = QSE_SED_ENOMEM;
				return QSE_NULL;	
			}
			break;
				
		case QSE_SED_CMD_BRANCH:
			if (cmd->u.branch.target == QSE_NULL)
			{
				qse_map_pair_t* pair;
				qse_xstr_t* lab = &cmd->u.branch.label;

				QSE_ASSERT (lab->ptr != QSE_NULL && lab->len > 0);
				pair = qse_map_search (
					&sed->labs, lab->ptr, lab->len);
				if (pair == QSE_NULL)
				{
					sed->errnum = QSE_SED_ELABNF;
					return QSE_NULL;
				}

				cmd->u.branch.target = QSE_MAP_VPTR(pair);
			}

			jumpto = cmd->u.branch.target;
			break;
	}

	if (jumpto == NULL) jumpto = cmd + 1;
	return jumpto;
} 

int qse_sed_execute (qse_sed_t* sed, qse_sed_iof_t inf, qse_sed_iof_t outf)
{
	qse_sed_cmd_t* c, * j;
	qse_ssize_t n;
	int ret = 0;

	qse_lda_clear (&sed->text.appended);
	qse_str_clear (&sed->text.held);
	if (qse_str_ccat (&sed->text.held, QSE_T('\n')) == (qse_size_t)-1)
	{
		sed->errnum = QSE_SED_ENOMEM;
		return -1;
	}

	sed->eio.out.f = outf;
	sed->eio.out.eof = 0;
	sed->eio.out.len = 0;

	sed->eio.in.f = inf;
	sed->eio.in.eof = 0;
	sed->eio.in.len = 0;
	sed->eio.in.pos = 0;
	sed->eio.in.num = 0;
	if (qse_str_init (&sed->eio.in.line, QSE_MMGR(sed), 256) == QSE_NULL)
	{
		sed->errnum = QSE_SED_ENOMEM;
		return -1;
	}

	n = sed->eio.in.f (sed, QSE_SED_IO_OPEN, QSE_NULL, 0);
	if (n <= -1)
	{
		ret = -1;
		sed->errnum = QSE_SED_EIOUSR;
		goto done3;
	}
	if (n == 0) 
	{
		/* EOF reached upon opening an input stream.
		 * no data to process. this is success */
		goto done2;
	}
	
	n = sed->eio.out.f (sed, QSE_SED_IO_OPEN, QSE_NULL, 0);
	if (n <= -1)
	{
		ret = -1;
		sed->errnum = QSE_SED_EIOUSR;
		goto done2;
	}
	if (n == 0) 
	{
		/* still don't know if we will write something.
		 * just mark EOF on the output stream and continue */
		sed->eio.out.eof = 1;
	}

	/* clear states */
	for (c = sed->cmd.buf; c < sed->cmd.cur; c++) c->state.a1_matched = 0;

	while (1)
	{
		qse_size_t i;

		n = read_line (sed);
		if (n <= -1) { ret = -1; goto done; }
		if (n == 0) goto done;

		qse_lda_clear (&sed->text.appended);

		c = sed->cmd.buf;
		while (c < sed->cmd.cur)
		{
			n = match_address (sed, c);
			if (n <= -1) { ret = -1; goto done; }
	
			if (c->negated) n = !n;
			if (n == 0)
			{
				c++;
				continue;
			}

			j = exec_cmd (sed, c);
			if (j == QSE_NULL) { ret = -1; goto done; }
			if (j > sed->cmd.cur) 
			{
				/* finish the current cycle */
				QSE_ASSERT (j == sed->cmd.cur + 1);
				goto done;
			}

			/* go to the next command */
			c = j;
		}

		if (!(sed->option & QSE_SED_QUIET))
		{
			n = write_str (sed, 
				QSE_STR_PTR(&sed->eio.in.line),
				QSE_STR_LEN(&sed->eio.in.line));
			if (n <= -1) { ret = -1; goto done; }
		}

		for (i = 0; i < QSE_LDA_SIZE(&sed->text.appended); i++)
		{
			qse_xstr_t* t = QSE_LDA_DPTR(&sed->text.appended, i);
			n = write_str (sed, t->ptr, t->len);
			if (n <= -1) { ret = -1; goto done; }
		}

		/* flush the output stream in case it's not flushed 
		 * in write functions */
		n = flush (sed);
		if (n <= -1) goto done;
	}

done:
	sed->eio.out.f (sed, QSE_SED_IO_CLOSE, QSE_NULL, 0);
done2:
	sed->eio.in.f (sed, QSE_SED_IO_CLOSE, QSE_NULL, 0);
done3:
	qse_str_fini (&sed->eio.in.line);
	return ret;
}
