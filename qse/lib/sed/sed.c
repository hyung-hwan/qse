/*
 * $Id: sed.c 203 2009-06-17 12:43:50Z hyunghwan.chung $
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

QSE_IMPLEMENT_COMMON_FUNCTIONS (sed)

static void free_command (qse_sed_t* sed, qse_sed_cmd_t* cmd);
static void free_all_command_blocks (qse_sed_t* sed);

static qse_sed_t* qse_sed_init (qse_sed_t* sed, qse_mmgr_t* mmgr);
static void qse_sed_fini (qse_sed_t* sed);

#define SETERR0(sed,num,line) \
do { qse_sed_seterror (sed, num, line, QSE_NULL); } while (0)

#define SETERR1(sed,num,line,argp,argl) \
do { \
	qse_cstr_t __qse__err__arg__; \
	__qse__err__arg__.ptr = argp; __qse__err__arg__.len = argl; \
	qse_sed_seterror (sed, num, line, &__qse__err__arg__); \
} while (0)

static const qse_char_t* dflerrstr (qse_sed_t* sed, qse_sed_errnum_t errnum)
{
	static const qse_char_t* errstr[] =
 	{
		QSE_T("no error"),
		QSE_T("insufficient memory"),
		QSE_T("command '${0}' not recognized"),
		QSE_T("command code missing"),
		QSE_T("command '${0}' incomplete"),
		QSE_T("regular expression '${0}' incomplete"),
		QSE_T("failed to compile regular expression '${0}'"),
		QSE_T("failed to match regular expression"),
		QSE_T("address 1 prohibited for '${0}'"),
		QSE_T("address 2 prohibited for '${0}'"),
		QSE_T("address 2 missing or invalid"),
		QSE_T("newline expected"),
		QSE_T("backslash expected"),
		QSE_T("backslash used as delimiter"),
		QSE_T("garbage after backslash"),
		QSE_T("semicolon expected"),
		QSE_T("empty label name"),
		QSE_T("duplicate label name '${0}'"),
		QSE_T("label '${0}' not found"),
		QSE_T("empty file name"),
		QSE_T("illegal file name"),
		QSE_T("strings in translation set not the same length"),
		QSE_T("group brackets not balanced"),
		QSE_T("group nesting too deep"),
		QSE_T("multiple occurrence specifiers"),
		QSE_T("occurrence specifier zero"),
		QSE_T("occurrence specifier too large"),
		QSE_T("io error with file '${0}'"),
		QSE_T("error returned by user io handler")
	};

	return (errnum >= 0 && errnum < QSE_COUNTOF(errstr))?
		errstr[errnum]: QSE_T("unknown error");
}

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

static qse_sed_t* qse_sed_init (qse_sed_t* sed, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (sed, 0, QSE_SIZEOF(*sed));
	sed->mmgr = mmgr;
	sed->errstr = dflerrstr;

	if (qse_str_init (&sed->tmp.rex, mmgr, 0) == QSE_NULL)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		return QSE_NULL;	
	}	

	if (qse_str_init (&sed->tmp.lab, mmgr, 0) == QSE_NULL)
	{
		qse_str_fini (&sed->tmp.lab);
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		return QSE_NULL;	
	}	

	if (qse_map_init (&sed->tmp.labs, mmgr, 128, 70) == QSE_NULL)
	{
		qse_str_fini (&sed->tmp.lab);
		qse_str_fini (&sed->tmp.rex);
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		return QSE_NULL;	
	}
	qse_map_setcopier (&sed->tmp.labs, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
        qse_map_setscale (&sed->tmp.labs, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	if (qse_lda_init (&sed->e.txt.appended, mmgr, 32) == QSE_NULL)
	{
		qse_map_fini (&sed->tmp.labs);
		qse_str_fini (&sed->tmp.lab);
		qse_str_fini (&sed->tmp.rex);
		return QSE_NULL;
	}

	if (qse_str_init (&sed->e.txt.read, mmgr, 256) == QSE_NULL)
	{
		qse_lda_fini (&sed->e.txt.appended);
		qse_map_fini (&sed->tmp.labs);
		qse_str_fini (&sed->tmp.lab);
		qse_str_fini (&sed->tmp.rex);
		return QSE_NULL;
	}

	if (qse_str_init (&sed->e.txt.held, mmgr, 256) == QSE_NULL)
	{
		qse_str_fini (&sed->e.txt.read);
		qse_lda_fini (&sed->e.txt.appended);
		qse_map_fini (&sed->tmp.labs);
		qse_str_fini (&sed->tmp.lab);
		qse_str_fini (&sed->tmp.rex);
		return QSE_NULL;
	}

	if (qse_str_init (&sed->e.txt.subst, mmgr, 256) == QSE_NULL)
	{
		qse_str_fini (&sed->e.txt.held);
		qse_str_fini (&sed->e.txt.read);
		qse_lda_fini (&sed->e.txt.appended);
		qse_map_fini (&sed->tmp.labs);
		qse_str_fini (&sed->tmp.lab);
		qse_str_fini (&sed->tmp.rex);
		return QSE_NULL;
	}

	/* on init, the last points to the first */
	sed->cmd.lb = &sed->cmd.fb; 
	/* the block has no data yet */
	sed->cmd.fb.len = 0;

	return sed;
}


static void qse_sed_fini (qse_sed_t* sed)
{
	free_all_command_blocks (sed);

	qse_str_fini (&sed->e.txt.subst);
	qse_str_fini (&sed->e.txt.held);
	qse_str_fini (&sed->e.txt.read);
	qse_lda_fini (&sed->e.txt.appended);

	qse_map_fini (&sed->tmp.labs);
	qse_str_fini (&sed->tmp.lab);
	qse_str_fini (&sed->tmp.rex);
}

void qse_sed_setoption (qse_sed_t* sed, int option)
{
	sed->option = option;
}

int qse_sed_getoption (qse_sed_t* sed)
{
	return sed->option;
}

qse_size_t qse_sed_getmaxdepth (qse_sed_t* sed, qse_sed_depth_t id)
{
	return (id & QSE_SED_DEPTH_REX_BUILD)? sed->depth.rex.build:
	       (id & QSE_SED_DEPTH_REX_MATCH)? sed->depth.rex.match: 0;
}

void qse_sed_setmaxdepth (qse_sed_t* sed, int ids, qse_size_t depth)
{
	if (ids & QSE_SED_DEPTH_REX_BUILD) sed->depth.rex.build = depth;
	if (ids & QSE_SED_DEPTH_REX_MATCH) sed->depth.rex.match = depth;
}

/* check if c is a space character */
#define IS_SPACE(c) ((c) == QSE_T(' ') || (c) == QSE_T('\t') || (c) == QSE_T('\r'))
#define IS_LINTERM(c) ((c) == QSE_T('\n'))
#define IS_WSPACE(c) (IS_SPACE(c) || IS_LINTERM(c))

/* check if c is a command terminator excluding a space character */
#define IS_CMDTERM(c) \
	(c == QSE_CHAR_EOF || c == QSE_T('#') || \
	 c == QSE_T(';') || IS_LINTERM(c) || \
	 c == QSE_T('{') || c == QSE_T('}'))
/* check if c can compose a label */
#define IS_LABC(c) \
	(QSE_ISALNUM(c) || c == QSE_T('.') || \
	 c == QSE_T('-') || c == QSE_T('_'))

#define CURSC(sed) ((sed)->src.cc)
#define NXTSC(sed)  getnextsc(sed)

static qse_cint_t getnextsc (qse_sed_t* sed)
{
	if (++sed->src.cur < sed->src.end) 
	{
		sed->src.cc = *(sed)->src.cur;
		/* TODO: support different line end convension */
		if (sed->src.cc == QSE_T('\n')) sed->src.lnum++;
	}
	else sed->src.cc = QSE_CHAR_EOF;

	return sed->src.cc;
}

static void free_address (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	if (cmd->a2.type == QSE_SED_ADR_REX)
	{
		QSE_ASSERT (cmd->a2.u.rex != QSE_NULL);
		qse_freerex (sed->mmgr, cmd->a2.u.rex);
		cmd->a2.type = QSE_SED_ADR_NONE;
	}
	if (cmd->a1.type == QSE_SED_ADR_REX)
	{
		QSE_ASSERT (cmd->a1.u.rex != QSE_NULL);
		qse_freerex (sed->mmgr, cmd->a1.u.rex);
		cmd->a1.type = QSE_SED_ADR_NONE;
	}
}

static int add_command_block (qse_sed_t* sed)
{
	qse_sed_cmd_blk_t* b;

	b = (qse_sed_cmd_blk_t*) QSE_MMGR_ALLOC (sed->mmgr, QSE_SIZEOF(*b));
	if (b == QSE_NULL)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		return -1;
	}

	QSE_MEMSET (b, 0, QSE_SIZEOF(*b));
	b->next = QSE_NULL;
	b->len = 0;

	sed->cmd.lb->next = b;
	sed->cmd.lb = b;

	return 0;
}

static void free_all_command_blocks (qse_sed_t* sed)
{
	qse_sed_cmd_blk_t* b;

	for (b = &sed->cmd.fb; b != QSE_NULL; )
	{
		qse_sed_cmd_blk_t* nxt = b->next;

		while (b->len > 0) free_command (sed, &b->buf[--b->len]);
		if (b != &sed->cmd.fb) QSE_MMGR_FREE (sed->mmgr, b);

		b = nxt;	
	}

	QSE_MEMSET (&sed->cmd.fb, 0, QSE_SIZEOF(sed->cmd.fb));
	sed->cmd.lb = &sed->cmd.fb;
	sed->cmd.lb->len = 0;
	sed->cmd.lb->next = QSE_NULL;
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

		case QSE_SED_CMD_READ_FILE:
		case QSE_SED_CMD_READ_FILELN:
		case QSE_SED_CMD_WRITE_FILE:
		case QSE_SED_CMD_WRITE_FILELN:
			if (cmd->u.file.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.file.ptr);
			break;

		case QSE_SED_CMD_BRANCH:
		case QSE_SED_CMD_BRANCH_COND:
			if (cmd->u.branch.label.ptr != QSE_NULL) 
				QSE_MMGR_FREE (sed->mmgr, cmd->u.branch.label.ptr);
			break;
	
		case QSE_SED_CMD_SUBSTITUTE:
			if (cmd->u.subst.file.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.subst.file.ptr);
			if (cmd->u.subst.rpl.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.subst.rpl.ptr);
			if (cmd->u.subst.rex != QSE_NULL)
				qse_freerex (sed->mmgr, cmd->u.subst.rex);
			break;

		case QSE_SED_CMD_TRANSLATE:
			if (cmd->u.transet.ptr != QSE_NULL)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.transet.ptr);
			break;

		default: 
			break;
	}
}

static void* compile_rex (qse_sed_t* sed, qse_char_t rxend)
{
	void* code;
	qse_cint_t c;

	qse_str_clear (&sed->tmp.rex);

	for (;;)
	{
		c = NXTSC (sed);
		if (c == QSE_CHAR_EOF || c == QSE_T('\n'))
		{
			qse_size_t lnum = sed->src.lnum;
			if (c == QSE_T('\n')) lnum--;
			SETERR1 (
				sed, QSE_SED_EREXIC, lnum,
				QSE_STR_PTR(&sed->tmp.rex),
				QSE_STR_LEN(&sed->tmp.rex)
			);
			return QSE_NULL;
		}

		if (c == rxend) break;

		if (c == QSE_T('\\'))
		{
			c = NXTSC (sed);
			if (c == QSE_CHAR_EOF || c == QSE_T('\n'))
			{
				qse_size_t lnum = sed->src.lnum;
				if (c == QSE_T('\n')) lnum--;
				SETERR1 (
					sed, QSE_SED_EREXIC, lnum,
					QSE_STR_PTR(&sed->tmp.rex),
					QSE_STR_LEN(&sed->tmp.rex)
				);
				return QSE_NULL;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
			/* TODO: support more escaped characters??  */
		}

		if (qse_str_ccat (&sed->tmp.rex, c) == (qse_size_t)-1)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, 0);
			return QSE_NULL;
		}
	} 

	code = qse_buildrex (
		sed->mmgr,
		sed->depth.rex.build,
		((sed->option&QSE_SED_REXBOUND)? 0:QSE_REX_BUILD_NOBOUND),
		QSE_STR_PTR(&sed->tmp.rex), 
		QSE_STR_LEN(&sed->tmp.rex), 
		QSE_NULL
	);
	if (code == QSE_NULL)
	{
		SETERR1 (
			sed, QSE_SED_EREXBL, sed->src.lnum,
			QSE_STR_PTR(&sed->tmp.rex),
			QSE_STR_LEN(&sed->tmp.rex)
		);
		return QSE_NULL;
	}

	return code;
}

static qse_sed_adr_t* get_address (qse_sed_t* sed, qse_sed_adr_t* a)
{
	qse_cint_t c;

	c = CURSC (sed);
	if (c == QSE_T('$'))
	{
		a->type = QSE_SED_ADR_DOL;
		NXTSC (sed);
	}
	else if (c == QSE_T('/'))
	{
		a->u.rex = compile_rex (sed, c);
		if (a->u.rex == QSE_NULL) return QSE_NULL;
		a->type = QSE_SED_ADR_REX;
		NXTSC (sed);
	}
	else if (c >= QSE_T('0') && c <= QSE_T('9'))
	{
		qse_size_t lno = 0;
		do
		{
			lno = lno * 10 + c - QSE_T('0');
			NXTSC (sed);
		}
		while ((c = CURSC(sed)) >= QSE_T('0') && c <= QSE_T('9'));

		/* line number 0 is illegal */
		if (lno == 0) return QSE_NULL;

		a->type = QSE_SED_ADR_LINE;
		a->u.lno = lno;
	}
	else if (c == QSE_T('\\'))
	{
		c = NXTSC (sed);
		if (c == QSE_CHAR_EOF || c == QSE_T('\n'))
		{
			qse_size_t lnum = sed->src.lnum;
			if (c == QSE_T('\n')) lnum--;
			SETERR1 (sed, QSE_SED_EREXIC, lnum, QSE_T(""), 0);
			return QSE_NULL;
		}

		a->u.rex = compile_rex (sed, c);
		if (a->u.rex == QSE_NULL) return QSE_NULL;
		a->type = QSE_SED_ADR_REX;
		NXTSC (sed);
	}
	else
	{
		a->type = QSE_SED_ADR_NONE;
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
		SETERR0 (sed, QSE_SED_ENOMEM, 0); \
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
				NXTSC (sed);
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
		/* TODO: support different line end convension */
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

	/* skip white spaces */
	c = CURSC (sed);
	while (IS_SPACE(c)) c = NXTSC (sed);

	if (!IS_LABC(c))
	{
		/* label name is empty */
		SETERR0 (sed, QSE_SED_ELABEM, sed->src.lnum);
		return -1;
	}

	qse_str_clear (&sed->tmp.lab);

	do
	{
		if (qse_str_ccat (&sed->tmp.lab, c) == (qse_size_t)-1) 
		{
			SETERR0 (sed, QSE_SED_ENOMEM, 0);
			return -1;
		} 
		c = NXTSC (sed);
	}
	while (IS_LABC(c));

	if (qse_map_search (
		&sed->tmp.labs, 
		QSE_STR_PTR(&sed->tmp.lab),
		QSE_STR_LEN(&sed->tmp.lab)) != QSE_NULL)
	{
		SETERR1 (sed, QSE_SED_ELABDU, sed->src.lnum, 
			QSE_STR_PTR(&sed->tmp.lab), QSE_STR_LEN(&sed->tmp.lab));
		return -1;
	}

	if (qse_map_insert (
		&sed->tmp.labs, 
		QSE_STR_PTR(&sed->tmp.lab), QSE_STR_LEN(&sed->tmp.lab),
		cmd, 0) == QSE_NULL)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		return -1;
	}

	while (IS_SPACE(c)) c = NXTSC (sed);

	if (IS_CMDTERM(c)) 
	{
		if (c != QSE_T('}') && 
		    c != QSE_T('#') &&
		    c != QSE_CHAR_EOF) NXTSC (sed);	
	}

	return 0;
}

static int terminate_command (qse_sed_t* sed)
{
	qse_cint_t c;

	c = CURSC (sed);
	while (IS_SPACE(c)) c = NXTSC (sed);
	if (!IS_CMDTERM(c))
	{
		SETERR0 (sed, QSE_SED_ESCEXP, sed->src.lnum);
		return -1;
	}

	/* if the target is terminated by #, it should let the caller 
	 * to skip the comment e.txt. so don't read in the next character.
	 * the same goes for brackets. */
	if (c != QSE_T('#') && 
	    c != QSE_T('{') &&
	    c != QSE_T('}') && 
	    c != QSE_CHAR_EOF) NXTSC (sed);	
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
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		goto oops;
	}

	while (IS_LABC(c))
	{
		if (qse_str_ccat (t, c) == (qse_size_t)-1) 
		{
			SETERR0 (sed, QSE_SED_ENOMEM, 0);
			goto oops;
		} 

		c = NXTSC (sed);
	}

	if (terminate_command (sed) <= -1) goto oops;

	pair = qse_map_search (&sed->tmp.labs, QSE_STR_PTR(t), QSE_STR_LEN(t));
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
		SETERR0 (
			sed, QSE_SED_EFILEM, 
			(IS_LINTERM(c)? sed->src.lnum-1: sed->src.lnum)
		);
		goto oops;	
	}

	t = qse_str_open (sed->mmgr, 0, 32);
	if (t == QSE_NULL) 
	{
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		goto oops;
	}

	do
	{
		if (c == QSE_T('\0'))
		{
			/* the file name should not contain '\0' */
			SETERR0 (sed, QSE_SED_EFILIL, sed->src.lnum);
			goto oops;
		}

		if (IS_SPACE(c)) trailing_spaces++;
		else trailing_spaces = 0;

		if (c == QSE_T('\\'))
		{
			c = NXTSC (sed);
			if (c == QSE_T('\0') || c == QSE_CHAR_EOF)
			{
				SETERR0 (sed, QSE_SED_EFILIL, sed->src.lnum);
				goto oops;
			}
			if (IS_LINTERM(c))
			{
				SETERR0 (sed, QSE_SED_EFILIL, sed->src.lnum - 1);
				goto oops;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
		}

		if (qse_str_ccat (t, c) == (qse_size_t)-1) 
		{
			SETERR0 (sed, QSE_SED_ENOMEM, sed->src.lnum);
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

#define CHECK_CMDIC(sed,cmd,c,action) \
do { \
	if (c == QSE_CHAR_EOF || IS_LINTERM(c)) \
	{ \
		SETERR1 (sed, QSE_SED_ECMDIC, \
			(IS_LINTERM(c)? sed->src.lnum-1:sed->src.lnum), \
			&cmd->type, 1); \
		action; \
	} \
} while (0)

static int get_subst (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c, delim;
	qse_str_t* t[2] = { QSE_NULL, QSE_NULL };
	int i;

	c = CURSC (sed);
	CHECK_CMDIC (sed, cmd, c, goto oops);

	delim = c;	
	if (delim == QSE_T('\\'))
	{
		/* backspace is an illegal delimiter */
		SETERR0 (sed, QSE_SED_EBSDEL, sed->src.lnum);
		goto oops;
	}

	t[0] = &sed->tmp.rex;
	qse_str_clear (t[0]);

	t[1] = qse_str_open (sed->mmgr, 0, 32);
	if (t[1] == QSE_NULL) 
	{
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		goto oops;
	}

	for (i = 0; i < 2; i++)
	{
		c = NXTSC (sed);

		while (c != delim)
		{
			CHECK_CMDIC (sed, cmd, c, goto oops);

			if (c == QSE_T('\\'))
			{
				c = NXTSC (sed);
				CHECK_CMDIC (sed, cmd, c, goto oops);
				if (c == QSE_T('n')) c = QSE_T('\n');
			}

			if (qse_str_ccat (t[i], c) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
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
				SETERR0 (sed, QSE_SED_EOCSDU, sed->src.lnum);
				goto oops;
			}

			occ = 0;

			do 
			{
				occ = occ * 10 + (c - QSE_T('0')); 
				if (occ > QSE_TYPE_MAX(unsigned short))
				{
					SETERR0 (sed, QSE_SED_EOCSTL, sed->src.lnum);
					goto oops;
				}
				c = NXTSC (sed); 
			}
			while (c >= QSE_T('0') && c <= QSE_T('9'));

			if (occ == 0)
			{
				SETERR0 (sed, QSE_SED_EOCSZE, sed->src.lnum);
				goto oops;
			}

			cmd->u.subst.occ = occ;
		}
		else if (c == QSE_T('w'))
		{
			NXTSC (sed);
			if (get_file (sed, &cmd->u.subst.file) <= -1) goto oops;
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
	cmd->u.subst.rex = qse_buildrex (
		sed->mmgr,
		sed->depth.rex.build,
		((sed->option&QSE_SED_REXBOUND)? 0:QSE_REX_BUILD_NOBOUND),
		QSE_STR_PTR(t[0]),
		QSE_STR_LEN(t[0]),
		QSE_NULL
	);
	if (cmd->u.subst.rex == QSE_NULL)
	{
		SETERR1 (
			sed, QSE_SED_EREXBL, sed->src.lnum,
			QSE_STR_PTR(t[0]), QSE_STR_LEN(t[0])
		);
		goto oops;
	}	

	qse_str_yield (t[1], &cmd->u.subst.rpl, 0);
	if (cmd->u.subst.g == 0 && cmd->u.subst.occ == 0) cmd->u.subst.occ = 1;

	qse_str_close (t[1]);
	return 0;

oops:
	if (t[1] != QSE_NULL) qse_str_close (t[1]);
	return -1;
}

static int get_transet (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c, delim;
	qse_str_t* t = QSE_NULL;
	qse_size_t pos;

	c = CURSC (sed);
	CHECK_CMDIC (sed, cmd, c, goto oops);

	delim = c;	
	if (delim == QSE_T('\\'))
	{
		/* backspace is an illegal delimiter */
		SETERR0 (sed, QSE_SED_EBSDEL, sed->src.lnum);
		goto oops;
	}

	t = qse_str_open (sed->mmgr, 0, 32);
	if (t == QSE_NULL) 
	{
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		goto oops;
	}

	c = NXTSC (sed);
	while (c != delim)
	{
		qse_char_t b[2];

		CHECK_CMDIC (sed, cmd, c, goto oops);

		if (c == QSE_T('\\'))
		{
			c = NXTSC (sed);
			CHECK_CMDIC (sed, cmd, c, goto oops);
			if (c == QSE_T('n')) c = QSE_T('\n');
		}

		b[0] = c;
		if (qse_str_ncat (t, b, 2) == (qse_size_t)-1)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, 0);
			goto oops;
		}

		c = NXTSC (sed);
	}	

	c = NXTSC (sed);
	for (pos = 1; c != delim; pos += 2)
	{
		CHECK_CMDIC (sed, cmd, c, goto oops);

		if (c == QSE_T('\\'))
		{
			c = NXTSC (sed);
			CHECK_CMDIC (sed, cmd, c, goto oops);
			if (c == QSE_T('n')) c = QSE_T('\n');
		}

		if (pos >= QSE_STR_LEN(t))
		{
			/* source and target not the same length */
			SETERR0 (sed, QSE_SED_ETSNSL, sed->src.lnum);
			goto oops;
		}

		QSE_STR_CHAR(t,pos) = c;
		c = NXTSC (sed);
	}

	if (pos < QSE_STR_LEN(t))
	{
		/* source and target not the same length */
		SETERR0 (sed, QSE_SED_ETSNSL, sed->src.lnum);
		goto oops;
	}

	NXTSC (sed);
	if (terminate_command (sed) <= -1) goto oops;

	qse_str_yield (t, &cmd->u.transet, 0);
	qse_str_close (t);
	return 0;

oops:
	if (t != QSE_NULL) qse_str_close (t);
	return -1;
}

/* process a command code and following parts into cmd */
static int get_command (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c;

	c = CURSC (sed);
	cmd->lnum = sed->src.lnum;
	switch (c)
	{
		default:
		{
			qse_char_t cc = c;
			SETERR1 (sed, QSE_SED_ECMDNR, sed->src.lnum, &cc, 1);
			return -1;
		}

		case QSE_CHAR_EOF:
			SETERR0 (sed, QSE_SED_ECMDMS, sed->src.lnum);
			return -1;	
		case QSE_T('\n'):
			SETERR0 (sed, QSE_SED_ECMDMS, sed->src.lnum-1);
			return -1;	

		case QSE_T(':'):
			if (cmd->a1.type != QSE_SED_ADR_NONE)
			{
				/* label cannot have an address */
				SETERR1 (
					sed, QSE_SED_EA1PHB,
					sed->src.lnum, &cmd->type, 1
				);
				return -1;
			}

			cmd->type = QSE_SED_CMD_NOOP;

			c = NXTSC (sed);
			if (get_label (sed, cmd) <= -1) return -1;

			c = CURSC (sed);
			while (IS_SPACE(c)) c = NXTSC(sed);
			break;

		case QSE_T('{'):
			/* insert a negated branch command at the beginning 
			 * of a group. this way, all the commands in a group
			 * can be skipped. the branch target is set once a
			 * corresponding } is met. */
			cmd->type = QSE_SED_CMD_BRANCH;
			cmd->negated = !cmd->negated;

			if (sed->tmp.grp.level >= QSE_COUNTOF(sed->tmp.grp.cmd))
			{
				/* group nesting too deep */
				SETERR0 (sed, QSE_SED_EGRNTD, sed->src.lnum);
				return -1;
			}

			sed->tmp.grp.cmd[sed->tmp.grp.level++] = cmd;
			NXTSC (sed);
			break;

		case QSE_T('}'):
		{
			qse_sed_cmd_t* tc;

			if (cmd->a1.type != QSE_SED_ADR_NONE)
			{
				qse_char_t tmpc = c;
				SETERR1 (
					sed, QSE_SED_EA1PHB,
					sed->src.lnum, &tmpc, 1
				);
				return -1;
			}

			cmd->type = QSE_SED_CMD_NOOP;

			if (sed->tmp.grp.level <= 0) 
			{
				/* group not balanced */
				SETERR0 (sed, QSE_SED_EGRNBA, sed->src.lnum);
				return -1;
			}

			tc = sed->tmp.grp.cmd[--sed->tmp.grp.level];
			tc->u.branch.target = cmd;

			NXTSC (sed);
			break;
		}

		case QSE_T('q'):
		case QSE_T('Q'):
			cmd->type = c;
			if (sed->option & QSE_SED_STRICT &&
			    cmd->a2.type != QSE_SED_ADR_NONE)
			{
				SETERR1 (
					sed, QSE_SED_EA2PHB,
					sed->src.lnum, &cmd->type, 1
				);
				return -1;
			}

			NXTSC (sed);
			if (terminate_command (sed) <= -1) return -1;
			break;

		case QSE_T('a'):
		case QSE_T('i'):
			if (sed->option & QSE_SED_STRICT &&
			    cmd->a2.type != QSE_SED_ADR_NONE)
			{
				qse_char_t tmpc = c;
				SETERR1 (
					sed, QSE_SED_EA2PHB,
					sed->src.lnum, &tmpc, 1
				);
				return -1;
			}
		case QSE_T('c'):
		{
			cmd->type = c;

			c = NXTSC (sed);
			while (IS_SPACE(c)) c = NXTSC (sed);

			if (c != QSE_T('\\'))
			{
				SETERR0 (
					sed, QSE_SED_EBSEXP, 
					(IS_LINTERM(c)? sed->src.lnum-1: sed->src.lnum)
				);
				return -1;
			}
		
			c = NXTSC (sed);
			while (IS_SPACE(c)) c = NXTSC (sed);

			if (c != QSE_CHAR_EOF && c != QSE_T('\n'))
			{
				SETERR0 (sed, QSE_SED_EGBABS, sed->src.lnum);
				return -1;
			}
			
			NXTSC (sed); /* skip the new line */

			/* get_text() starts from the next line */
			if (get_text (sed, cmd) <= -1) return -1;

			break;
		}

		case QSE_T('='):
			if (sed->option & QSE_SED_STRICT &&
			    cmd->a2.type != QSE_SED_ADR_NONE)
			{
				qse_char_t tmpc = c;
				SETERR1 (
					sed, QSE_SED_EA2PHB,
					sed->src.lnum, &tmpc, 1
				);
				return -1;
			}

		case QSE_T('d'):
		case QSE_T('D'):

		case QSE_T('p'):
		case QSE_T('P'):
		case QSE_T('l'):

		case QSE_T('h'):
		case QSE_T('H'):
		case QSE_T('g'):
		case QSE_T('G'):
		case QSE_T('x'):

		case QSE_T('n'):
		case QSE_T('N'):
			cmd->type = c;
			NXTSC (sed);
			if (terminate_command (sed) <= -1) return -1;
			break;

		case QSE_T('b'):
		case QSE_T('t'):
			cmd->type = c;
			NXTSC (sed);
			if (get_branch_target (sed, cmd) <= -1) return -1;
			break;

		case QSE_T('r'):
		case QSE_T('R'):
		case QSE_T('w'):
		case QSE_T('W'):
			cmd->type = c;
			NXTSC (sed);
			if (get_file (sed, &cmd->u.file) <= -1) return -1;
			break;

		case QSE_T('s'):
			cmd->type = c;
			NXTSC (sed);
			if (get_subst (sed, cmd) <= -1) return -1;
			break;

		case QSE_T('y'):
			cmd->type = c;
			NXTSC (sed);
			if (get_transet (sed, cmd) <= -1) return -1;
			break;
	}

	return 0;
}

int qse_sed_comp (qse_sed_t* sed, const qse_char_t* sptr, qse_size_t slen)
{
	qse_cint_t c;
	qse_sed_cmd_t* cmd;

	/* store the source code pointers */
	sed->src.ptr  = sptr;
	sed->src.end  = sptr + slen;
	sed->src.cur  = sptr;
	sed->src.lnum = 1;
	sed->src.cc = (slen > 0)? (*sptr): QSE_CHAR_EOF;
	
	/* free all the commands previously compiled */
	free_all_command_blocks (sed);
	QSE_ASSERT (sed->cmd.lb == &sed->cmd.fb && sed->cmd.lb->len == 0);
	cmd = &sed->cmd.lb->buf[sed->cmd.lb->len];

	/* clear the label table */
	qse_map_clear (&sed->tmp.labs);

	/* clear temporary data */
	sed->tmp.grp.level = 0;
	qse_str_clear (&sed->tmp.rex);

	while (1)
	{
		int n;

		c = CURSC (sed);
		
		/* skip white spaces and comments*/
		while (IS_WSPACE(c)) c = NXTSC (sed);
		if (c == QSE_T('#'))
		{
			do c = NXTSC (sed); 
			while (!IS_LINTERM(c) && c != QSE_CHAR_EOF);
			NXTSC (sed);
			continue;
		}

		/* check if it has reached the end or is commented */
		if (c == QSE_CHAR_EOF) break;

		if (c == QSE_T(';')) 
		{
			/* semicolon without a address-command pair */
			NXTSC (sed);
			continue;
		}

		/* initialize the current command */
		QSE_MEMSET (cmd, 0, QSE_SIZEOF(*cmd));

		/* process the first address */
		if (get_address (sed, &cmd->a1) == QSE_NULL) return -1;

		c = CURSC (sed);
		if (cmd->a1.type != QSE_SED_ADR_NONE)
		{
			while (IS_SPACE(c)) c = NXTSC (sed);

			if (c == QSE_T(',') ||
			    ((sed->option&QSE_SED_STARTSTEP) && c==QSE_T('~')))
			{
				qse_char_t delim = c;

				/* maybe an address range */
				do { c = NXTSC (sed); } while (IS_SPACE(c));

				if (get_address (sed, &cmd->a2) == QSE_NULL) 
				{
					QSE_ASSERT (cmd->a2.type == QSE_SED_ADR_NONE);
					free_address (sed, cmd);
					return -1;
				}

				if (delim == QSE_T(','))
				{
					if (cmd->a2.type == QSE_SED_ADR_NONE)
					{
						SETERR0 (sed, QSE_SED_EA2MOI, sed->src.lnum);
						free_address(sed, cmd);
						return -1;
					}
				}
				else if ((sed->option&QSE_SED_STARTSTEP) && 
				         (delim == QSE_T('~')))
				{
					if (cmd->a1.type != QSE_SED_ADR_LINE || 
					    cmd->a2.type != QSE_SED_ADR_LINE)
					{
						SETERR0 (sed, QSE_SED_EA2MOI, sed->src.lnum);
						free_address(sed, cmd);
						return -1;
					}

					cmd->a2.type = QSE_SED_ADR_STEP;	
				}

				c = CURSC (sed);
			}
			else cmd->a2.type = QSE_SED_ADR_NONE;
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
	
		n = get_command (sed, cmd);
		if (n <= -1) 
		{
			free_address (sed, cmd);
			return -1;
		}

		sed->cmd.lb->len++;
		if (sed->cmd.lb->len >= QSE_COUNTOF(sed->cmd.lb->buf))
		{
			if (add_command_block (sed) <= -1) return -1;
		}
		cmd = &sed->cmd.lb->buf[sed->cmd.lb->len];
	}

	if (sed->tmp.grp.level != 0)
	{
		SETERR0 (sed, QSE_SED_EGRNBA, sed->src.lnum);
		return -1;
	}

	return 0;
}

static int read_char (qse_sed_t* sed, qse_char_t* c)
{
	qse_ssize_t n;

	if (sed->e.in.xbuf_len == 0)
	{
		if (sed->e.in.pos >= sed->e.in.len)
		{
			sed->errnum = QSE_SED_ENOERR;
			sed->e.in.arg.u.r.buf = sed->e.in.buf;
			sed->e.in.arg.u.r.len = QSE_COUNTOF(sed->e.in.buf);
			n = sed->e.in.fun (
				sed, QSE_SED_IO_READ, &sed->e.in.arg
			);
			if (n <= -1) 
			{
				if (sed->errnum == QSE_SED_ENOERR)
					SETERR0 (sed, QSE_SED_EIOUSR, 0);
				return -1;
			}
	
			if (n == 0) return 0; /* end of file */
	
			sed->e.in.len = n;
			sed->e.in.pos = 0;
		}
	
		*c = sed->e.in.buf[sed->e.in.pos++];
		return 1;
	}
	else if (sed->e.in.xbuf_len > 0)
	{
		QSE_ASSERT (sed->e.in.xbuf_len == 1);
		*c = sed->e.in.xbuf[--sed->e.in.xbuf_len];
		return 1;
	}
	else /*if (sed->e.in.xbuf_len < 0)*/
	{
		QSE_ASSERT (sed->e.in.xbuf_len == -1);
		return 0;
	}	
}

static int read_file (
	qse_sed_t* sed, qse_sed_cmd_t* cmd,
	const qse_char_t* path, qse_size_t plen, int line)
{
	qse_ssize_t n;
	qse_sed_io_arg_t arg;
	qse_char_t buf[256];

	arg.path = path;
	sed->errnum = QSE_SED_ENOERR;
	n = sed->e.in.fun (sed, QSE_SED_IO_OPEN, &arg);
	if (n <= -1)
	{
		/*if (sed->errnum != QSE_SED_ENOERR)
		 *	SETERR0 (sed, QSE_SED_EIOUSR, cmd->lnum);
		 *return -1;*/
		/* it is ok if it is not able to open a file */
		return 0;	
	}
	if (n == 0) 
	{
		/* EOF - no data */
		sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &arg);
		return 0;
	}

	while (1)
	{
		arg.u.r.buf = buf;
		arg.u.r.len = QSE_COUNTOF(buf);

		sed->errnum = QSE_SED_ENOERR;
		n = sed->e.in.fun (sed, QSE_SED_IO_READ, &arg);
		if (n <= -1)
		{
			sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &arg);
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR1 (sed, QSE_SED_EIOFIL, 0, path, plen);
			sed->errlin = cmd->lnum;
			return -1;
		}
		if (n == 0) break;

		if (line)
		{
			qse_size_t i;

			for (i = 0; i < n; i++)
			{
				if (qse_str_ccat (&sed->e.txt.read, buf[i]) == (qse_size_t)-1)
				{
					sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &arg);
					SETERR0 (sed, QSE_SED_ENOMEM, cmd->lnum);
					return -1;
				}

				/* TODO: support different line end convension */
				if (buf[i] == QSE_T('\n')) goto done;
			}
		}
		else
		{
			if (qse_str_ncat (&sed->e.txt.read, buf, n) == (qse_size_t)-1)
			{
				sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &arg);
				SETERR0 (sed, QSE_SED_ENOMEM, cmd->lnum);
				return -1;
			}
		}
	}

done:
	sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &arg);
	return 0;
}

static int read_line (qse_sed_t* sed, int append)
{
	qse_size_t len = 0;
	qse_char_t c;
	int n;

	if (!append) qse_str_clear (&sed->e.in.line);
	if (sed->e.in.eof) 
	{
	#if 0
		/* no more input detected in the previous read.
		 * set eof back to 0 here so that read_char() is called
		 * if read_line() is called again. that way, the result
		 * of subsequent calls counts on read_char(). */
		sed->e.in.eof = 0; 
	#endif
		return 0;
	}

	while (1)
	{
		n = read_char (sed, &c);
		if (n <= -1) return -1;
		if (n == 0)
		{
			sed->e.in.eof = 1;
			if (len == 0) return 0;
			/*sed->e.in.eof = 1;*/
			break;
		}

		if (qse_str_ccat (&sed->e.in.line, c) == (qse_size_t)-1)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, 0);
			return -1;
		}
		len++;	

		/* TODO: support different line end convension */
		if (c == QSE_T('\n')) break;
	}

	sed->e.in.num++;
	sed->e.subst_done = 0;
	return 1;	
}

static int flush (qse_sed_t* sed)
{
	qse_size_t pos = 0;
	qse_ssize_t n;

	while (sed->e.out.len > 0)
	{
		sed->errnum = QSE_SED_ENOERR;
		sed->e.out.arg.u.w.data = &sed->e.out.buf[pos];
		sed->e.out.arg.u.w.len = sed->e.out.len;
		n = sed->e.out.fun (sed, QSE_SED_IO_WRITE, &sed->e.out.arg);

		if (n <= -1)
		{
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR0 (sed, QSE_SED_EIOUSR, 0);
			return -1;
		}

		if (n == 0)
		{
			/* reached the end of file - this is also an error */
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR0 (sed, QSE_SED_EIOUSR, 0);
			return -1;
		}

		pos += n;
		sed->e.out.len -= n;
	}

	return 0;
}

static int write_char (qse_sed_t* sed, qse_char_t c)
{
	sed->e.out.buf[sed->e.out.len++] = c;
	if (c == QSE_T('\n') ||
	    sed->e.out.len >= QSE_COUNTOF(sed->e.out.buf))
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

static int write_first_line (
	qse_sed_t* sed, const qse_char_t* str, qse_size_t len)
{
	qse_size_t i;
	for (i = 0; i < len; i++)
	{
		if (write_char (sed, str[i]) <= -1) return -1;
		/* TODO: handle different line ending convension... */
		if (str[i] == QSE_T('\n')) break;
	}
	return 0;
}

#define NTOC(n) (((n) >= 10)? (((n) - 10) + QSE_T('A')): (n) + QSE_T('0'))
static int write_num (qse_sed_t* sed, qse_ulong_t x, int base, int width)
{
	qse_ulong_t last = x % base;
	qse_ulong_t y = 0; 
	int dig = 0;

	QSE_ASSERT (base >= 2 && base <= 36);

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
		if (write_char (sed, NTOC(y % base)) <= -1) return -1;
		y = y / base;
		dig--;
	}

	while (dig > 0) 
	{ 
		dig--; 
		if (write_char (sed, QSE_T('0')) <= -1) return -1;
	}
	if (last < 0) last = -last;
	if (write_char (sed, NTOC(last)) <= -1) return -1;

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
#				ifdef QSE_CHAR_IS_MCHAR
					WRITE_CHAR (sed, QSE_T('\\'));
					WRITE_NUM (sed, c, 8, QSE_SIZEOF(qse_char_t)*3);
#				else
					if (QSE_SIZEOF(qse_char_t) <= 2)
					{
						WRITE_STR (sed, QSE_T("\\u"), 2);
					}
					else
					{
						WRITE_STR (sed, QSE_T("\\U"), 2);
					}
					WRITE_NUM (sed, c, 16, QSE_SIZEOF(qse_char_t)*2);
#				endif
				}
			}
		}
	}		

	if (len > 1 && end[-1] != QSE_T('\n')) 
		WRITE_STR (sed, QSE_T("$\n"), 2);

	return 0;
}

static int write_str_to_file (
	qse_sed_t* sed, qse_sed_cmd_t* cmd,
	const qse_char_t* str, qse_size_t len, 
	const qse_char_t* path, qse_size_t plen)
{
	qse_ssize_t n;
	qse_map_pair_t* pair;
	qse_sed_io_arg_t* ap;

	pair = qse_map_search (&sed->e.out.files, path, plen);
	if (pair == QSE_NULL)
	{
		qse_sed_io_arg_t arg;

		QSE_MEMSET (&arg, 0, QSE_SIZEOF(arg));
		pair = qse_map_insert (&sed->e.out.files,
			(void*)path, plen, &arg, QSE_SIZEOF(arg));
		if (pair == QSE_NULL)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, cmd->lnum);
			return -1;
		}
	}

	ap = QSE_MAP_VPTR(pair);
	if (ap->handle == QSE_NULL)
	{
		sed->errnum = QSE_SED_ENOERR;
		ap->path = path;
		n = sed->e.out.fun (sed, QSE_SED_IO_OPEN, ap);
		if (n <= -1)
		{
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR1 (sed, QSE_SED_EIOFIL, 0, path, plen);
			sed->errlin = cmd->lnum;
			return -1;
		}
		if (n == 0)
		{
			/* EOF is returned upon opening a write stream.
			 * it is also an error as it can't write 
			 * a requested string */
			sed->e.out.fun (sed, QSE_SED_IO_CLOSE, ap);
			ap->handle = QSE_NULL;
			SETERR1 (sed, QSE_SED_EIOFIL, cmd->lnum, path, plen);
			return -1;
		}
	}

	while (len > 0)
	{
		sed->errnum = QSE_SED_ENOERR;
		ap->u.w.data = str;
		ap->u.w.len = len;
		n = sed->e.out.fun (sed, QSE_SED_IO_WRITE, ap);
		if (n <= -1) 
		{
			sed->e.out.fun (sed, QSE_SED_IO_CLOSE, ap);
			ap->handle = QSE_NULL;
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR1 (sed, QSE_SED_EIOFIL, 0, path, plen);
			sed->errlin = cmd->lnum;
			return -1;
		}

		if (n == 0)
		{
			/* eof is returned on the write stream. 
			 * it is also an error as it can't write any more */
			sed->e.out.fun (sed, QSE_SED_IO_CLOSE, ap);
			ap->handle = QSE_NULL;
			SETERR1 (sed, QSE_SED_EIOFIL, cmd->lnum, path, plen);
			return -1;
		}

		len -= n;
	}

	return 0;
}

static int do_subst (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cstr_t mat, pmat;
	int opt = 0, repl = 0, n;
	qse_rex_errnum_t errnum;
	const qse_char_t* cur_ptr, * str_ptr, * str_end;
	qse_size_t cur_len, str_len, m, i;
	qse_size_t max_count, sub_count;

	QSE_ASSERT (cmd->type == QSE_SED_CMD_SUBSTITUTE);

	qse_str_clear (&sed->e.txt.subst);
	if (cmd->u.subst.i) opt = QSE_REX_MATCH_IGNORECASE;

	str_ptr = QSE_STR_PTR(&sed->e.in.line);
	str_len = QSE_STR_LEN(&sed->e.in.line);

	/* TODO: support different line end convension */
	if (str_len > 0 && str_ptr[str_len-1] == QSE_T('\n')) str_len--;

	str_end = str_ptr + str_len;
	cur_ptr = str_ptr;
	cur_len = str_len;

	sub_count = 0;
	max_count = (cmd->u.subst.g)? 0: cmd->u.subst.occ;

	pmat.ptr = QSE_NULL;
	pmat.len = 0;

	/* perform test when cur_ptr == str_end also because
	 * end of string($) needs to be tested */
	while (cur_ptr <= str_end)
	{
		if (max_count == 0 || sub_count < max_count)
		{
			n = qse_matchrex (
				sed->mmgr, 
				sed->depth.rex.match,
				cmd->u.subst.rex, opt,
				str_ptr, str_len,
				cur_ptr, cur_len,
				&mat, &errnum
			);
		}
		else n = 0;

		if (n <= -1)
		{
			SETERR0 (sed, QSE_SED_EREXMA, cmd->lnum);
			return -1;
		}

		if (n == 0) 
		{
			/* no more match found */
			if (qse_str_ncat (
				&sed->e.txt.subst,
				cur_ptr, cur_len) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
				return -1;
			}
			break;
		}

		if (mat.len == 0 && 
		    pmat.ptr != QSE_NULL && 
		    mat.ptr == pmat.ptr + pmat.len)
		{
			/* match length is 0 and the match is still at the
			 * end of the previous match */
			goto skip_one_char;
		}

		if (max_count > 0 && sub_count + 1 != max_count)
		{
			m = qse_str_ncat (
				&sed->e.txt.subst,
				cur_ptr, mat.ptr-cur_ptr+mat.len
			);

			if (m == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
				return -1;
			}
		}
		else
		{
			repl = 1;

			m = qse_str_ncat (
				&sed->e.txt.subst, cur_ptr, mat.ptr-cur_ptr);
			if (m == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
				return -1;
			}

			for (i = 0; i < cmd->u.subst.rpl.len; i++)
			{
				if ((i+1) < cmd->u.subst.rpl.len && 
				    cmd->u.subst.rpl.ptr[i] == QSE_T('\\') && 
				    cmd->u.subst.rpl.ptr[i+1] == QSE_T('&'))
				{
					m = qse_str_ccat (
						&sed->e.txt.subst, QSE_T('&'));
					i++;
				}
				else if (cmd->u.subst.rpl.ptr[i] == QSE_T('&'))
				{
					m = qse_str_ncat (
						&sed->e.txt.subst,
						mat.ptr, mat.len);
				}
				else 
				{
					m = qse_str_ccat (
						&sed->e.txt.subst,
						cmd->u.subst.rpl.ptr[i]);
				}

				if (m == (qse_size_t)-1)
				{
					SETERR0 (sed, QSE_SED_ENOMEM, 0);
					return -1;
				}
			}
		}

		sub_count++;
		cur_len = cur_len - ((mat.ptr - cur_ptr) + mat.len);
		cur_ptr = mat.ptr + mat.len;

		pmat = mat;

		if (mat.len == 0)
		{
		skip_one_char:
			/* special treament is need if the match length is 0 */

			m = qse_str_ncat (&sed->e.txt.subst, cur_ptr, 1);
			if (m == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
				return -1;
			}

			cur_ptr++; cur_len--;
		}
	}

	if (str_len < QSE_STR_LEN(&sed->e.in.line))
	{
		/* TODO: support different line ending convension */
		m = qse_str_ccat (&sed->e.txt.subst, QSE_T('\n'));
		if (m == (qse_size_t)-1)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, 0);
			return -1;
		}
	}

	qse_str_swap (&sed->e.in.line, &sed->e.txt.subst);

	if (repl)
	{
		if (cmd->u.subst.p)
		{
			n = write_str (
				sed, 
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line)
			);
			if (n <= -1) return -1;
		}

		if (cmd->u.subst.file.ptr != QSE_NULL)
		{
			n = write_str_to_file (
				sed, cmd,
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line),
				cmd->u.subst.file.ptr,
				cmd->u.subst.file.len
			);
			if (n <= -1) return -1;
		}

		sed->e.subst_done = 1;
	}

	return 0;
}

static int match_a (qse_sed_t* sed, qse_sed_cmd_t* cmd, qse_sed_adr_t* a)
{
	switch (a->type)
	{
		case QSE_SED_ADR_LINE:
			return (sed->e.in.num == a->u.lno)? 1: 0;

		case QSE_SED_ADR_REX:
		{
			int n;
			qse_cstr_t match;
			qse_str_t* line;
			qse_size_t llen;
			qse_rex_errnum_t errnum;

			QSE_ASSERT (a->u.rex != QSE_NULL);

			line = &sed->e.in.line;
			llen = QSE_STR_LEN(line);

			/* TODO: support different line end convension */
			if (llen > 0 && 
			    QSE_STR_CHAR(line,llen-1) == QSE_T('\n')) llen--;

			n = qse_matchrex (
				sed->mmgr, 
				sed->depth.rex.match,
				a->u.rex, 0,
				QSE_STR_PTR(line), llen, 
				QSE_STR_PTR(line), llen, 
				&match, &errnum);
			if (n <= -1)
			{
				SETERR0 (sed, QSE_SED_EREXMA, cmd->lnum);
				return -1;
			}			

			return n;
		}
		case QSE_SED_ADR_DOL:
		{
			qse_char_t c;
			int n;

			if (sed->e.in.xbuf_len < 0)
			{
				/* we know that we've reached eof as it has
				 * been done so previously */
				return 1;
			}

			n = read_char (sed, &c);
			if (n <= -1) return -1;

			QSE_ASSERT (sed->e.in.xbuf_len == 0);
			if (n == 0)
			{
				/* eof has been reached */
				sed->e.in.xbuf_len--;
				return 1;
			}
			else
			{
				sed->e.in.xbuf[sed->e.in.xbuf_len++] = c;
				return 0;
			}
		}

		default:
			QSE_ASSERT (a->type == QSE_SED_ADR_NONE);
			return 1; /* match */
	}
}

/* match an address against input.
 * return -1 on error, 0 on no match, 1 on match. */
static int match_address (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	int n;

	cmd->state.c_ready = 0;
	if (cmd->a1.type == QSE_SED_ADR_NONE)
	{
		QSE_ASSERT (cmd->a2.type == QSE_SED_ADR_NONE);
		cmd->state.c_ready = 1;
		return 1;
	}
	else if (cmd->a2.type == QSE_SED_ADR_STEP)
	{
		QSE_ASSERT (cmd->a1.type == QSE_SED_ADR_LINE);

		/* stepping address */
		cmd->state.c_ready = 1;
		if (sed->e.in.num < cmd->a1.u.lno) return 0;
		if ((sed->e.in.num - cmd->a1.u.lno) % cmd->a2.u.lno == 0) return 1;
		return 0;
	}
	else if (cmd->a2.type != QSE_SED_ADR_NONE)
	{
		/* two addresses */
		if (cmd->state.a1_matched)
		{
			n = match_a (sed, cmd, &cmd->a2);
			if (n <= -1) return -1;
			if (n == 0)
			{
				if (cmd->a2.type == QSE_SED_ADR_LINE &&
				    sed->e.in.num > cmd->a2.u.lno)
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
			n = match_a (sed, cmd, &cmd->a1);
			if (n <= -1) return -1;
			if (n == 0) 
			{
				return 0;
			}

			if (cmd->a2.type == QSE_SED_ADR_LINE &&
			    sed->e.in.num >= cmd->a2.u.lno) 
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

		n = match_a (sed, cmd, &cmd->a1);
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
		case QSE_SED_CMD_NOOP:
			break;
			
		case QSE_SED_CMD_QUIT:
			if (!(sed->option && QSE_SED_QUIET))
			{
				n = write_str (sed, 
					QSE_STR_PTR(&sed->e.in.line),
					QSE_STR_LEN(&sed->e.in.line));
				if (n <= -1) return QSE_NULL;
			}
		case QSE_SED_CMD_QUIT_QUIET:
			jumpto = &sed->cmd.quit;
			break;

		case QSE_SED_CMD_APPEND:
			if (qse_lda_insert (
				&sed->e.txt.appended,
				QSE_LDA_SIZE(&sed->e.txt.appended),	
				&cmd->u.text, 0) == (qse_size_t)-1) 
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
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
				/* change the pattern space */
				n = qse_str_ncpy (
					&sed->e.in.line,
					QSE_STR_PTR(&cmd->u.text),
					QSE_STR_LEN(&cmd->u.text));
				if (n == (qse_size_t)-1) 
				{
					SETERR0 (sed, QSE_SED_ENOMEM, 0);
					return QSE_NULL;
				}
			}
			else 
			{		
				qse_str_clear (&sed->e.in.line);
			}

			/* move past the last command so as to start 
			 * the next cycle */
			jumpto = &sed->cmd.over;
			break;

		case QSE_SED_CMD_DELETE_FIRSTLN:
		{
			qse_char_t* nl;

			/* delete the first line from the pattern space */
			nl = qse_strxchr (
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line),
				QSE_T('\n'));
			if (nl != QSE_NULL) 
			{
				/* if a new line is found. delete up to it  */
				qse_str_del (&sed->e.in.line, 0, 
					nl - QSE_STR_PTR(&sed->e.in.line) + 1);	

				if (QSE_STR_LEN(&sed->e.in.line) > 0)
				{
					/* if the pattern space is not empty,
					 * arrange to execute from the first
					 * command */
					jumpto = &sed->cmd.again;
				}
				else
				{
					/* finish the current cycle */
					jumpto = &sed->cmd.over;
				}
				break;
			}

			/* otherwise clear the entire pattern space below */
		}
		case QSE_SED_CMD_DELETE:
			/* delete the pattern space */
			qse_str_clear (&sed->e.in.line);

			/* finish the current cycle */
			jumpto = &sed->cmd.over;
			break;

		case QSE_SED_CMD_PRINT_LNNUM:
			if (write_num (sed, sed->e.in.num, 10, 0) <= -1) return QSE_NULL;
			if (write_char (sed, QSE_T('\n')) <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_PRINT:
			n = write_str (
				sed, 
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line)
			);
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_PRINT_FIRSTLN:
			n = write_first_line (
				sed, 
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line)
			);
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_PRINT_CLEARLY:
			n = write_str_clearly (
				sed,
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line)
			);
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_HOLD:
			/* copy the pattern space to the hold space */
			if (qse_str_ncpy (&sed->e.txt.held, 	
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line)) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
				return QSE_NULL;	
			}
			break;
				
		case QSE_SED_CMD_HOLD_APPEND:
			/* append the pattern space to the hold space */
			if (qse_str_ncat (&sed->e.txt.held, 	
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line)) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
				return QSE_NULL;	
			}
			break;

		case QSE_SED_CMD_RELEASE:
			/* copy the hold space to the pattern space */
			if (qse_str_ncpy (&sed->e.in.line,
				QSE_STR_PTR(&sed->e.txt.held),
				QSE_STR_LEN(&sed->e.txt.held)) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
				return QSE_NULL;	
			}
			break;

		case QSE_SED_CMD_RELEASE_APPEND:
			/* append the hold space to the pattern space */
			if (qse_str_ncat (&sed->e.in.line,
				QSE_STR_PTR(&sed->e.txt.held),
				QSE_STR_LEN(&sed->e.txt.held)) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, 0);
				return QSE_NULL;	
			}
			break;

		case QSE_SED_CMD_EXCHANGE:
			/* exchange the pattern space and the hold space */
			qse_str_swap (&sed->e.in.line, &sed->e.txt.held);
			break;

		case QSE_SED_CMD_NEXT:
			/* output the current pattern space */
			n = write_str (
				sed, 
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line)
			);
			if (n <= -1) return QSE_NULL;

			/* read the next line and fill the pattern space */
			n = read_line (sed, 0);
			if (n <= -1) return QSE_NULL;
			if (n == 0) 
			{
				/* EOF is reached. */
				jumpto = &sed->cmd.over;
			}
			break;

		case QSE_SED_CMD_NEXT_APPEND:
			/* append the next line to the pattern space */
			n = read_line (sed, 1);
			if (n <= -1) return QSE_NULL;
			if (n == 0)
			{
				/* EOF is reached. */
				jumpto = &sed->cmd.over;
			}
			break;
				
		case QSE_SED_CMD_READ_FILE:
			n = read_file (
				sed, cmd, cmd->u.file.ptr, cmd->u.file.len, 0);
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_READ_FILELN:
			n = read_file (
				sed, cmd, cmd->u.file.ptr, cmd->u.file.len, 1);
			if (n <= -1) return QSE_NULL; 
			break;

		case QSE_SED_CMD_WRITE_FILE:
			n = write_str_to_file (
				sed, cmd,
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line),
				cmd->u.file.ptr,
				cmd->u.file.len
			);
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_WRITE_FILELN:
		{
			const qse_char_t* ptr = QSE_STR_PTR(&sed->e.in.line);
			qse_size_t i, len = QSE_STR_LEN(&sed->e.in.line);
			for (i = 0; i < len; i++)
			{	
				/* TODO: handle different line end convension */
				if (ptr[i] == QSE_T('\n')) 
				{
					i++;
					break;
				}
			}

			n = write_str_to_file (
				sed, cmd, ptr, i,
				cmd->u.file.ptr,
				cmd->u.file.len
			);
			if (n <= -1) return QSE_NULL;
			break;
		}
			
		case QSE_SED_CMD_BRANCH_COND:
			if (!sed->e.subst_done) break;
			sed->e.subst_done = 0;
		case QSE_SED_CMD_BRANCH:
			QSE_ASSERT (cmd->u.branch.target != QSE_NULL);
			jumpto = cmd->u.branch.target;
			break;

		case QSE_SED_CMD_SUBSTITUTE:
			n = do_subst (sed, cmd);
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_TRANSLATE:
		{
			qse_char_t* ptr = QSE_STR_PTR(&sed->e.in.line);
			qse_size_t i, len = QSE_STR_LEN(&sed->e.in.line);

		/* TODO: sort cmd->u.transset and do binary search 
		 * when sorted, you can, before binary search, check 
		 * if ptr[i] < transet[0] || ptr[i] > transset[transset_size-1].
		 * if so, it has not mathing translation */

			/* TODO: support different line end convension */
			if (len > 0 && ptr[len-1] == QSE_T('\n')) len--;

			for (i = 0; i < len; i++)
			{
				const qse_char_t* tptr = cmd->u.transet.ptr;
				qse_size_t j, tlen = cmd->u.transet.len;
				for (j = 0; j < tlen; j += 2)
				{
					if (ptr[i] == tptr[j])
					{
						ptr[i] = tptr[j+1];
						break;
					}
				}
			}
			break;
		}
	}

	if (jumpto == QSE_NULL) jumpto = cmd->state.next;
	return jumpto;
} 

static void close_outfile (qse_map_t* map, void* dptr, qse_size_t dlen)
{
	qse_sed_io_arg_t* arg = dptr;
	QSE_ASSERT (dlen == QSE_SIZEOF(*arg));

	if (arg->handle != QSE_NULL)
	{
		qse_sed_t* sed = *(qse_sed_t**)QSE_XTN(map);
		sed->e.out.fun (sed, QSE_SED_IO_CLOSE, arg);
		arg->handle = QSE_NULL;
	}
}

static int init_command_block_for_exec (qse_sed_t* sed, qse_sed_cmd_blk_t* b)
{
	qse_size_t i;

	QSE_ASSERT (b->len <= QSE_COUNTOF(b->buf));

	for (i = 0; i < b->len; i++)
	{
		qse_sed_cmd_t* c = &b->buf[i];
		const qse_xstr_t* file = QSE_NULL;

		/* clear states */
		c->state.a1_matched = 0;
		c->state.c_ready = 0;

		/* let c point to the next command */
		if (i + 1 >= b->len)
		{
			if (b->next == QSE_NULL || b->next->len <= 0)
				c->state.next = &sed->cmd.over;
			else
				c->state.next = &b->next->buf[0];
		}
		else
		{
			c->state.next = &b->buf[i+1];
		}

		if ((c->type == QSE_SED_CMD_BRANCH ||
		     c->type == QSE_SED_CMD_BRANCH_COND) &&
		    c->u.branch.target == QSE_NULL)
		{
			/* resolve unresolved branch targets */
			qse_map_pair_t* pair;
			qse_xstr_t* lab = &c->u.branch.label;

			if (lab->ptr == QSE_NULL)
			{
				/* arrange to branch past the last */
				c->u.branch.target = &sed->cmd.over;
			}
			else
			{
				/* resolve the target */
			  	pair = qse_map_search (
					&sed->tmp.labs, lab->ptr, lab->len);
				if (pair == QSE_NULL)
				{
					SETERR1 (
						sed, QSE_SED_ELABNF,
						c->lnum, lab->ptr, lab->len
					);
					return -1;
				}

				c->u.branch.target = QSE_MAP_VPTR(pair);

				/* free resolved label name */ 
				QSE_MMGR_FREE (sed->mmgr, lab->ptr);
				lab->ptr = QSE_NULL;
				lab->len = 0;
			}
		}
		else
		{
			/* open output files in advance */
			if (c->type == QSE_SED_CMD_WRITE_FILE ||
			    c->type == QSE_SED_CMD_WRITE_FILELN)
			{
				file = &c->u.file;
			}
			else if (c->type == QSE_SED_CMD_SUBSTITUTE &&
			         c->u.subst.file.ptr != QSE_NULL)
			{
				file = &c->u.subst.file;
			}
		
			if (file != QSE_NULL)
			{
				/* call this function to an open output file */
				int n = write_str_to_file (
					sed, c, QSE_NULL, 0, 
					file->ptr, file->len
				);
				if (n <= -1) return -1;
			}
		}
	}

	return 0;
}

static int init_all_commands_for_exec (qse_sed_t* sed)
{
	qse_sed_cmd_blk_t* b;

	for (b = &sed->cmd.fb; b != QSE_NULL; b = b->next)
	{
		if (init_command_block_for_exec (sed, b) <= -1) return -1;
	}

	return 0;
}

int qse_sed_exec (qse_sed_t* sed, qse_sed_io_fun_t inf, qse_sed_io_fun_t outf)
{
	qse_ssize_t n;
	int ret = 0;

	sed->e.subst_done = 0;
	qse_lda_clear (&sed->e.txt.appended);
	qse_str_clear (&sed->e.txt.read);
	qse_str_clear (&sed->e.txt.subst);
	qse_str_clear (&sed->e.txt.held);
	if (qse_str_ccat (&sed->e.txt.held, QSE_T('\n')) == (qse_size_t)-1)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		return -1;
	}

	sed->e.out.fun = outf;
	sed->e.out.eof = 0;
	sed->e.out.len = 0;
	if (qse_map_init (&sed->e.out.files, sed->mmgr, 128, 70) == QSE_NULL)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		return -1;
	}
	*(qse_sed_t**)QSE_XTN(&sed->e.out.files) = sed;
	qse_map_setcopier (
		&sed->e.out.files, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
        qse_map_setscale (
		&sed->e.out.files, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));
	qse_map_setcopier (
		&sed->e.out.files, QSE_MAP_VAL, QSE_MAP_COPIER_INLINE);
	qse_map_setfreeer (
		&sed->e.out.files, QSE_MAP_VAL, close_outfile);

	sed->e.in.fun = inf;
	sed->e.in.eof = 0;
	sed->e.in.len = 0;
	sed->e.in.pos = 0;
	sed->e.in.num = 0;
	if (qse_str_init (&sed->e.in.line, QSE_MMGR(sed), 256) == QSE_NULL)
	{
		qse_map_fini (&sed->e.out.files);
		SETERR0 (sed, QSE_SED_ENOMEM, 0);
		return -1;
	}

	sed->errnum = QSE_SED_ENOERR;
	sed->e.in.arg.path = QSE_NULL;
	n = sed->e.in.fun (sed, QSE_SED_IO_OPEN, &sed->e.in.arg);
	if (n <= -1)
	{
		ret = -1;
		if (sed->errnum == QSE_SED_ENOERR) 
			SETERR0 (sed, QSE_SED_EIOUSR, 0);
		goto done3;
	}
	if (n == 0) 
	{
		/* EOF reached upon opening an input stream.
		 * no data to process. this is success */
		goto done2;
	}
	
	sed->errnum = QSE_SED_ENOERR;
	sed->e.out.arg.path = QSE_NULL;
	n = sed->e.out.fun (sed, QSE_SED_IO_OPEN, &sed->e.out.arg);
	if (n <= -1)
	{
		ret = -1;
		if (sed->errnum == QSE_SED_ENOERR) 
			SETERR0 (sed, QSE_SED_EIOUSR, 0);
		goto done2;
	}
	if (n == 0) 
	{
		/* still don't know if we will write something.
		 * just mark EOF on the output stream and continue */
		sed->e.out.eof = 1;
	}

	if (init_all_commands_for_exec (sed) <= -1)
	{
		ret = -1;
		goto done;
	}

	while (1)
	{
		qse_size_t i;

		n = read_line (sed, 0);
		if (n <= -1) { ret = -1; goto done; }
		if (n == 0) goto done;

		qse_lda_clear (&sed->e.txt.appended);
		qse_str_clear (&sed->e.txt.read);

		if (sed->cmd.fb.len > 0)
		{
			qse_sed_cmd_t* c, * j;

		again:
			c = &sed->cmd.fb.buf[0];

			while (c != &sed->cmd.over)
			{
				n = match_address (sed, c);
				if (n <= -1) { ret = -1; goto done; }
	
				if (c->negated) n = !n;
				if (n == 0)
				{
					c = c->state.next;
					continue;
				}

				j = exec_cmd (sed, c);
				if (j == QSE_NULL) { ret = -1; goto done; }
				if (j == &sed->cmd.quit) goto done;
				if (j == &sed->cmd.again) goto again;

				/* go to the next command */
				c = j;
			}
		}

		if (!(sed->option & QSE_SED_QUIET))
		{
			/* write the pattern space */
			n = write_str (sed, 
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line));
			if (n <= -1) { ret = -1; goto done; }
		}

		/* write text read in by the r command */
		n = write_str (
			sed, 
			QSE_STR_PTR(&sed->e.txt.read),
			QSE_STR_LEN(&sed->e.txt.read)
		);
		if (n <= -1) { ret = -1; goto done; }

		/* write appeneded text by the a command */
		for (i = 0; i < QSE_LDA_SIZE(&sed->e.txt.appended); i++)
		{
			qse_xstr_t* t = QSE_LDA_DPTR(&sed->e.txt.appended, i);
			n = write_str (sed, t->ptr, t->len);
			if (n <= -1) { ret = -1; goto done; }
		}

		/* flush the output stream in case it's not flushed 
		 * in write functions */
		n = flush (sed);
		if (n <= -1) { ret = -1; goto done; }
	}

done:
	qse_map_clear (&sed->e.out.files);
	sed->e.out.fun (sed, QSE_SED_IO_CLOSE, &sed->e.out.arg);
done2:
	sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &sed->e.in.arg);
done3:
	qse_str_fini (&sed->e.in.line);
	qse_map_fini (&sed->e.out.files);
	return ret;
}

qse_size_t qse_sed_getlinnum (qse_sed_t* sed)
{
	return sed->e.in.num;
}

void qse_sed_setlinnum (qse_sed_t* sed, qse_size_t num)
{
	sed->e.in.num = num;
}
