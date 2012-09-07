/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "sed.h"
#include "../cmn/mem.h"
#include <qse/cmn/chr.h>

/* Define USE_REGEX to use regcomp(), regexec(), regfree() instead of TRE */
/* #define USE_REGEX */

#if defined(USE_REX)
#	include <qse/cmn/rex.h>
#else
#	if defined(QSE_CHAR_IS_MCHAR) && defined(USE_REGEX)
#		include <regex.h>
#	else
#		include <qse/cmn/tre.h>
#	endif
#endif

QSE_IMPLEMENT_COMMON_FUNCTIONS (sed)

static void free_command (qse_sed_t* sed, qse_sed_cmd_t* cmd);
static void free_all_command_blocks (qse_sed_t* sed);
static void free_all_cids (qse_sed_t* sed);
static void free_appends (qse_sed_t* sed);
static int emit_output (qse_sed_t* sed, int skipline);

#define EMPTY_REX ((void*)1)

#define SETERR0(sed,num,loc) \
do { qse_sed_seterror (sed, num, QSE_NULL, loc); } while (0)

#define SETERR1(sed,num,argp,argl,loc) \
do { \
	qse_cstr_t __ea__; \
	__ea__.ptr = argp; __ea__.len = argl; \
	qse_sed_seterror (sed, num, &__ea__, loc); \
} while (0)

static void free_all_cut_selector_blocks (qse_sed_t* sed, qse_sed_cmd_t* cmd);

qse_sed_t* qse_sed_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_sed_t* sed;

	sed = (qse_sed_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_sed_t) + xtnsize);
	if (sed == QSE_NULL) return QSE_NULL;

	if (qse_sed_init (sed, mmgr) <= -1)
	{
		QSE_MMGR_FREE (sed->mmgr, sed);
		return QSE_NULL;
	}

	return sed;
}

void qse_sed_close (qse_sed_t* sed)
{
	qse_sed_ecb_t* ecb;

	for (ecb = sed->ecb; ecb; ecb = ecb->next)
		if (ecb->close) ecb->close (sed);

	qse_sed_fini (sed);
	QSE_MMGR_FREE (sed->mmgr, sed);
}

int qse_sed_init (qse_sed_t* sed, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (sed, 0, QSE_SIZEOF(*sed));
	sed->mmgr = mmgr;
	sed->errstr = qse_sed_dflerrstr;

	if (qse_str_init (&sed->tmp.rex, mmgr, 0) <= -1) goto oops_1;
	if (qse_str_init (&sed->tmp.lab, mmgr, 0) <= -1) goto oops_2;

	if (qse_map_init (&sed->tmp.labs, mmgr,
		128, 70, QSE_SIZEOF(qse_char_t), 1) <= -1) goto oops_3;
	qse_map_setmancbs (
		&sed->tmp.labs, 
		qse_getmapmancbs(QSE_MAP_MANCBS_INLINE_KEY_COPIER)
	);

	/* init_append (sed); */
	if (qse_str_init (&sed->e.txt.hold, mmgr, 256) <= -1) goto oops_6;
	if (qse_str_init (&sed->e.txt.scratch, mmgr, 256) <= -1) goto oops_7;

	/* on init, the last points to the first */
	sed->cmd.lb = &sed->cmd.fb; 
	/* the block has no data yet */
	sed->cmd.fb.len = 0;

	/* initialize field buffers for cut */
	sed->e.cutf.cflds = QSE_COUNTOF(sed->e.cutf.sflds);
	sed->e.cutf.flds = sed->e.cutf.sflds;

	return 0;

oops_7:
	qse_str_fini (&sed->e.txt.hold);
oops_6:
	qse_map_fini (&sed->tmp.labs);
oops_3:
	qse_str_fini (&sed->tmp.lab);
oops_2:
	qse_str_fini (&sed->tmp.rex);
oops_1:
	SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
	return -1;
}

void qse_sed_fini (qse_sed_t* sed)
{
	free_all_command_blocks (sed);
	free_all_cids (sed);

	if (sed->e.cutf.flds != sed->e.cutf.sflds) 
		QSE_MMGR_FREE (sed->mmgr, sed->e.cutf.flds);

	qse_str_fini (&sed->e.txt.scratch);
	qse_str_fini (&sed->e.txt.hold);
	free_appends (sed);

	qse_map_fini (&sed->tmp.labs);
	qse_str_fini (&sed->tmp.lab);
	qse_str_fini (&sed->tmp.rex);
}

void qse_sed_setoption (qse_sed_t* sed, int option)
{
	sed->option = option;
}

int qse_sed_getoption (const qse_sed_t* sed)
{
	return sed->option;
}

#if defined(USE_REX)
qse_size_t qse_sed_getmaxdepth (const qse_sed_t* sed, qse_sed_depth_t id)
{
	return (id & QSE_SED_DEPTH_REX_BUILD)? sed->depth.rex.build:
	       (id & QSE_SED_DEPTH_REX_MATCH)? sed->depth.rex.match: 0;
}

void qse_sed_setmaxdepth (qse_sed_t* sed, int ids, qse_size_t depth)
{
	if (ids & QSE_SED_DEPTH_REX_BUILD) sed->depth.rex.build = depth;
	if (ids & QSE_SED_DEPTH_REX_MATCH) sed->depth.rex.match = depth;
}
#endif

static void* build_rex (
	qse_sed_t* sed, const qse_cstr_t* str, 
	int ignorecase, const qse_sed_loc_t* loc)
{
#if defined(USE_REX)
	void* rex;
	int opt = 0;

	if ((sed->option & QSE_SED_EXTENDEDREX) == 0) opt |= QSE_REX_NOBOUND;

	rex = qse_buildrex (
		sed->mmgr, sed->depth.rex.build,
		opt, str->ptr, str->len, QSE_NULL
	);
	if (rex == QSE_NULL)
	{
		SETERR1 (sed, QSE_SED_EREXBL, str->ptr, str->len, loc);
	}

	return rex;

#elif defined(QSE_CHAR_IS_MCHAR) && defined(USE_REGEX)

	regex_t* rex;
	qse_char_t* strz;
	int xopt = 0;

	if (ignorecase) xopt |= REG_ICASE;
	if (sed->option & QSE_SED_EXTENDEDREX) xopt |= REG_EXTENDED;

	rex = QSE_MMGR_ALLOC (sed->mmgr, QSE_SIZEOF(*rex));
	if (rex == QSE_NULL)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, loc);
		return QSE_NULL;
	}

	strz = qse_strxdup (str->ptr, str->len, sed->mmgr);
	if (strz == QSE_NULL)
	{
		QSE_MMGR_FREE (sed->mmgr, rex);
		SETERR0 (sed, QSE_SED_ENOMEM, loc);
		return QSE_NULL;
	}

	xopt = regcomp (rex, strz, xopt);
	QSE_MMGR_FREE (sed->mmgr, strz);

	if (xopt != 0)
	{
		QSE_MMGR_FREE (sed->mmgr, rex);
		SETERR1 (sed, QSE_SED_EREXBL, str->ptr, str->len, loc);
		return QSE_NULL;
	}

	return rex;

#else
	qse_tre_t* tre;
	int opt = 0;

	tre = qse_tre_open (sed->mmgr, 0);
	if (tre == QSE_NULL)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, loc);
		return QSE_NULL;
	}

	/* ignorecase is a compile option for TRE */
	if (ignorecase) opt |= QSE_TRE_IGNORECASE; 
	if (sed->option & QSE_SED_EXTENDEDREX) opt |= QSE_TRE_EXTENDED;
	if (sed->option & QSE_SED_NONSTDEXTREX) opt |= QSE_TRE_NONSTDEXT;

	if (qse_tre_compx (tre, str->ptr, str->len, QSE_NULL, opt) <= -1)
	{
		if (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)
			SETERR0 (sed, QSE_SED_ENOMEM, loc);
		else
			SETERR1 (sed, QSE_SED_EREXBL, str->ptr, str->len, loc);
		qse_tre_close (tre);
		return QSE_NULL;
	}

	return tre;
#endif
}

static QSE_INLINE void free_rex (qse_sed_t* sed, void* rex)
{
#if defined(USE_REX)
	qse_freerex (sed->mmgr, rex);
#elif defined(QSE_CHAR_IS_MCHAR) && defined(USE_REGEX)
	regfree (rex);
	QSE_MMGR_FREE (sed->mmgr, rex);
#else
	qse_tre_close (rex);
#endif
}

#if !defined(USE_REX)
static int matchtre (
	qse_sed_t* sed, qse_tre_t* tre, int opt, 
	const qse_cstr_t* str, qse_cstr_t* mat,
	qse_cstr_t submat[9], const qse_sed_loc_t* loc)
{
#if defined(QSE_CHAR_IS_MCHAR) && defined(USE_REGEX)
	regmatch_t match[10];
	qse_char_t* strz;
	int xopt = 0;

	if (opt & QSE_TRE_NOTBOL) xopt |= REG_NOTBOL;

	strz = qse_strxdup (str->ptr, str->len, sed->mmgr);
	if (strz == QSE_NULL)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, loc);
		return -1;	
	}
	xopt = regexec ((regex_t*)tre, strz, QSE_COUNTOF(match), match, xopt);
	QSE_MMGR_FREE (sed->mmgr, strz);
	if (xopt == REG_NOMATCH) return 0;
#else

	int n;
	qse_tre_match_t match[10] = { { 0, 0 }, };

	n = qse_tre_execx (tre, str->ptr, str->len, match, QSE_COUNTOF(match), opt);
	if (n <= -1)
	{
		qse_sed_errnum_t errnum;

		if (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMATCH) return 0;

		errnum = (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)? 
			QSE_SED_ENOMEM: QSE_SED_EREXMA;
		SETERR0 (sed, errnum, loc);
		return -1;	
	}
#endif

	QSE_ASSERT (match[0].rm_so != -1);
	if (mat)
	{
		mat->ptr = &str->ptr[match[0].rm_so];
		mat->len = match[0].rm_eo - match[0].rm_so;
	}

	if (submat)
	{
		int i;

		/* you must intialize submat before you pass into this 
		 * function because it can abort filling */
		for (i = 1; i < QSE_COUNTOF(match); i++)
		{
			if (match[i].rm_so != -1) 
			{
				submat[i-1].ptr = &str->ptr[match[i].rm_so];
				submat[i-1].len = match[i].rm_eo - match[i].rm_so;
			}
		}
	}
	return 1;
}
#endif

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
#define IS_LABCHAR(c) (!IS_CMDTERM(c) && !IS_WSPACE(c))

#define CURSC(sed) ((sed)->src.cc)
#define NXTSC(sed,c,errret) \
	do { if (getnextsc(sed,&(c)) <= -1) return (errret); } while (0)
#define NXTSC_GOTO(sed,c,label) \
	do { if (getnextsc(sed,&(c)) <= -1) goto label; } while (0)
#define PEEPNXTSC(sed,c,errret) \
	do { if (peepnextsc(sed,&(c)) <= -1) return (errret); } while (0)

static int open_script_stream (qse_sed_t* sed)
{
	qse_ssize_t n;

	sed->errnum = QSE_SED_ENOERR;
	n = sed->src.fun (sed, QSE_SED_IO_OPEN, &sed->src.arg, QSE_NULL, 0);
	if (n <= -1)
	{
		if (sed->errnum == QSE_SED_ENOERR)
			SETERR0 (sed, QSE_SED_EIOUSR, QSE_NULL);
		return -1;
	}

	sed->src.cur = sed->src.buf;
	sed->src.end = sed->src.buf;
	sed->src.cc  = QSE_CHAR_EOF;
	sed->src.loc.line = 1;
	sed->src.loc.colm = 0;

	if (n == 0) 
	{
		sed->src.eof = 1;
		return 0; /* end of file */
	}
	else
	{
		sed->src.eof = 0;
		return 1;
	}
}

static int close_script_stream (qse_sed_t* sed)
{
	qse_ssize_t n;

	sed->errnum = QSE_SED_ENOERR;
	n = sed->src.fun (sed, QSE_SED_IO_CLOSE, &sed->src.arg, QSE_NULL, 0);
	if (n <= -1)
	{
		if (sed->errnum == QSE_SED_ENOERR)
			SETERR0 (sed, QSE_SED_EIOUSR, QSE_NULL);
		return -1;
	}

	return 0;
}

static int read_script_stream (qse_sed_t* sed)
{
	qse_ssize_t n;

	sed->errnum = QSE_SED_ENOERR;
	n = sed->src.fun (
		sed, QSE_SED_IO_READ, &sed->src.arg, 
		sed->src.buf, QSE_COUNTOF(sed->src.buf)
	);
	if (n <= -1)
	{
		if (sed->errnum == QSE_SED_ENOERR)
			SETERR0 (sed, QSE_SED_EIOUSR, QSE_NULL);
		return -1; /* error */
	}

	if (n == 0)
	{
		/* don't change sed->src.cur and sed->src.end.
		 * they remain the same on eof  */
		sed->src.eof = 1;
		return 0; /* eof */
	}

	sed->src.cur = sed->src.buf;
	sed->src.end = sed->src.buf + n;
	return 1; /* read something */
}

static int getnextsc (qse_sed_t* sed, qse_cint_t* c)
{
	/* adjust the line and column number of the next
	 * character based on the current character */
	if (sed->src.cc == QSE_T('\n')) 
	{
		/* TODO: support different line end convension */
		sed->src.loc.line++;
		sed->src.loc.colm = 1;
	}
	else 
	{
		/* take note that if you keep on calling getnextsc()
		 * after QSE_CHAR_EOF is read, this column number
		 * keeps increasing also. there should be a bug of
		 * reading more than necessary somewhere in the code
		 * if this happens. */
		sed->src.loc.colm++;
	}

	if (sed->src.cur >= sed->src.end && !sed->src.eof) 
	{
		/* read in more character if buffer is empty */
		if (read_script_stream (sed) <= -1) return -1;
	}

	sed->src.cc = 
		(sed->src.cur < sed->src.end)? 
		(*sed->src.cur++): QSE_CHAR_EOF;

	*c = sed->src.cc;
	return 0;
}

static int peepnextsc (qse_sed_t* sed, qse_cint_t* c)
{
	if (sed->src.cur >= sed->src.end && !sed->src.eof) 
	{
		/* read in more character if buffer is empty.
		 * it is ok to fill the buffer in the peeping
		 * function if it doesn't change sed->src.cc. */
		if (read_script_stream (sed) <= -1) return -1;
	}

	/* no changes in line nubmers, the 'cur' pointer, and
	 * most importantly 'cc' unlike getnextsc(). */
	*c = (sed->src.cur < sed->src.end)?  (*sed->src.cur): QSE_CHAR_EOF;
	return 0;
}

static void free_address (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	if (cmd->a2.type == QSE_SED_ADR_REX)
	{
		QSE_ASSERT (cmd->a2.u.rex != QSE_NULL);
		if (cmd->a2.u.rex != EMPTY_REX)
			free_rex (sed, cmd->a2.u.rex);
		cmd->a2.type = QSE_SED_ADR_NONE;
	}
	if (cmd->a1.type == QSE_SED_ADR_REX)
	{
		QSE_ASSERT (cmd->a1.u.rex != QSE_NULL);
		if (cmd->a1.u.rex != EMPTY_REX)
			free_rex (sed, cmd->a1.u.rex);
		cmd->a1.type = QSE_SED_ADR_NONE;
	}
}

static int add_command_block (qse_sed_t* sed)
{
	qse_sed_cmd_blk_t* b;

	b = (qse_sed_cmd_blk_t*) QSE_MMGR_ALLOC (sed->mmgr, QSE_SIZEOF(*b));
	if (b == QSE_NULL)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
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
			if (cmd->u.text.ptr)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.text.ptr);
			break;

		case QSE_SED_CMD_READ_FILE:
		case QSE_SED_CMD_READ_FILELN:
		case QSE_SED_CMD_WRITE_FILE:
		case QSE_SED_CMD_WRITE_FILELN:
			if (cmd->u.file.ptr)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.file.ptr);
			break;

		case QSE_SED_CMD_BRANCH:
		case QSE_SED_CMD_BRANCH_COND:
			if (cmd->u.branch.label.ptr)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.branch.label.ptr);
			break;
	
		case QSE_SED_CMD_SUBSTITUTE:
			if (cmd->u.subst.file.ptr)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.subst.file.ptr);
			if (cmd->u.subst.rpl.ptr)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.subst.rpl.ptr);
			if (cmd->u.subst.rex && cmd->u.subst.rex != EMPTY_REX)
				free_rex (sed, cmd->u.subst.rex);
			break;

		case QSE_SED_CMD_TRANSLATE:
			if (cmd->u.transet.ptr)
				QSE_MMGR_FREE (sed->mmgr, cmd->u.transet.ptr);
			break;

		case QSE_SED_CMD_CUT:
			free_all_cut_selector_blocks (sed, cmd);
			break;

		default: 
			break;
	}
}

static void free_all_cids (qse_sed_t* sed)
{
	if (sed->src.cid == (qse_sed_cid_t*)&sed->src.unknown_cid) 
		sed->src.cid = sed->src.cid->next;

	while (sed->src.cid)
	{
		qse_sed_cid_t* next = sed->src.cid->next;
		QSE_MMGR_FREE (sed->mmgr, sed->src.cid);
		sed->src.cid = next;
	}
}

static int trans_escaped (qse_sed_t* sed, qse_cint_t c, qse_cint_t* ec, int* xamp)
{
	if (xamp) *xamp = 0;

	switch (c)
	{
		case QSE_T('a'):
			c = QSE_T('\a');
			break;
/*
Omitted for clash with regular expression \b.
		case QSE_T('b'):
			c = QSE_T('\b');
			break;
*/

		case QSE_T('f'):
			c = QSE_T('\f');
		case QSE_T('n'):
			c = QSE_T('\n');
			break;
		case QSE_T('r'):
			c = QSE_T('\r');
			break;
		case QSE_T('t'):
			c = QSE_T('\t');
			break;
		case QSE_T('v'):
			c = QSE_T('\v');
			break;

		case QSE_T('x'):
		{
			/* \xnn */
			int cc;
			qse_cint_t peeped;

			PEEPNXTSC (sed, peeped, -1);
			cc = QSE_XDIGITTONUM (peeped);
			if (cc <= -1) break;
			NXTSC (sed, peeped, -1); /* consume the character peeped */
			c = cc;

			PEEPNXTSC (sed, peeped, -1);
			cc = QSE_XDIGITTONUM (peeped);
			if (cc <= -1) break;
			NXTSC (sed, peeped, -1); /* consume the character peeped */
			c = (c << 4) | cc;

			/* let's indicate that '&' is built from \x26. */
			if (xamp && c == QSE_T('&')) *xamp = 1;
			break;
		}

#ifdef QSE_CHAR_IS_WCHAR
		case QSE_T('X'):
		{
			/* \Xnnnn or \Xnnnnnnnn for wchar_t */
			int cc, i;
			qse_cint_t peeped;

			PEEPNXTSC (sed, peeped, -1);
			cc = QSE_XDIGITTONUM (peeped);
			if (cc <= -1) break;
			NXTSC (sed, peeped, -1); /* consume the character peeped */
			c = cc;

			for (i = 1; i < QSE_SIZEOF(qse_char_t) * 2; i++)
			{
				PEEPNXTSC (sed, peeped, -1);
				cc = QSE_XDIGITTONUM (peeped);
				if (cc <= -1) break;
				NXTSC (sed, peeped, -1); /* consume the character peeped */
				c = (c << 4) | cc;
			}

			/* let's indicate that '&' is built from \x26. */
			if (xamp && c == QSE_T('&')) *xamp = 1;
			break;
		}
#endif
	}

	*ec = c;
	return 0;
}

static int pickup_rex (
	qse_sed_t* sed, qse_char_t rxend, 
	int replacement, const qse_sed_cmd_t* cmd, qse_str_t* buf)
{
	/* 
	 * 'replacement' indicates that this functions is called for 
	 * 'replacement' in 's/pattern/replacement'.
	 */

	qse_cint_t c;
	qse_size_t chars_from_opening_bracket = 0;
	int bracket_state = 0;

	qse_str_clear (buf);

	while (1)
	{
		NXTSC (sed, c, -1);

	shortcut:
		if (c == QSE_CHAR_EOF || IS_LINTERM(c))
		{
			if (cmd)
			{
				SETERR1 (
					sed, QSE_SED_ECMDIC, 
					&cmd->type, 1,
					&sed->src.loc
				);
			}
			else
			{
				SETERR1 (
					sed, QSE_SED_EREXIC, 
					QSE_STR_PTR(buf), QSE_STR_LEN(buf), 
					&sed->src.loc
				);
			}
			return -1;
		}

		if (c == rxend && bracket_state == 0) break;

		if (c == QSE_T('\\'))
		{
			qse_cint_t nc;

			NXTSC (sed, nc, -1);
			if (nc == QSE_CHAR_EOF /*|| IS_LINTERM(nc)*/)
			{
				if (cmd)
				{
					SETERR1 (
						sed, QSE_SED_ECMDIC, 
						&cmd->type, 1,
						&sed->src.loc
					);
				}
				else
				{
					SETERR1 (
						sed, QSE_SED_EREXIC,
						QSE_STR_PTR(buf),
						QSE_STR_LEN(buf),
						&sed->src.loc
					);
				}
				return -1;
			}

			if (bracket_state > 0 && nc == QSE_T(']'))
			{
				/*
				 * if 'replacement' is not set, bracket_state is alyway 0.
				 * so this block is never reached. 
				 *
				 * a backslashed closing bracket is seen. 
				 * it is not :]. if bracket_state is 2, this \] 
				 * makes an illegal regular expression. but,
				 * let's not care.. just drop the state to 0
				 * as if the outer [ is closed.
				 */
				if (chars_from_opening_bracket > 1) bracket_state = 0;
			}
	
			if (nc == QSE_T('\n')) c = nc;
			else
			{
				qse_cint_t ec;
				int xamp;

				if (trans_escaped (sed, nc, &ec, &xamp) <= -1) return -1;
				if (ec == nc || (xamp && replacement))
				{
					/* if the character after a backslash is not special 
					 * at the this layer, add the backslash into the 
					 * regular expression buffer as it is. 
					 *
					 * if \x26 is found in the replacement, i also need to 
					 * transform it to \& so that it is not treated as a 
					 * special &. 
					 */

					if (qse_str_ccat (buf, QSE_T('\\')) == (qse_size_t)-1)
					{
						SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
						return -1;
					}
				}
				c = ec;
			}
		}
		else if (!replacement) 
		{
			/* this block sets a flag to indicate that we are in [] 
			 * of a regular expression. */

			if (c == QSE_T('[')) 
			{
				if (bracket_state <= 0)
				{
					bracket_state = 1;
					chars_from_opening_bracket = 0;
				}
				else if (bracket_state == 1)
				{
					qse_cint_t nc;

					NXTSC (sed, nc, -1);
					if (nc == QSE_T(':')) bracket_state = 2;

					if (qse_str_ccat (buf, c) == (qse_size_t)-1)
					{
						SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
						return -1;
					}
					chars_from_opening_bracket++;
					c = nc;
					goto shortcut;
				}
			}
			else if (c == QSE_T(']'))
			{
				if (bracket_state == 1)
				{
					/* if it is the first character after [, 
					 * it is a normal character. */
					if (chars_from_opening_bracket > 1) bracket_state--;
				}
				else if (bracket_state == 2)
				{
					/* it doesn't really care if colon was for opening bracket 
					 * like in [[:]] */
					if (QSE_STR_LASTCHAR(buf) == QSE_T(':')) bracket_state--;
				}
			}
		}

		if (qse_str_ccat (buf, c) == (qse_size_t)-1)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
			return -1;
		}
		chars_from_opening_bracket++;
	} 

	return 0;
}

static QSE_INLINE void* compile_rex_address (qse_sed_t* sed, qse_char_t rxend)
{
	int ignorecase = 0;
	qse_cint_t peeped;

	if (pickup_rex (sed, rxend, 0, QSE_NULL, &sed->tmp.rex) <= -1)
		return QSE_NULL;

	if (QSE_STR_LEN(&sed->tmp.rex) <= 0) return EMPTY_REX;

	/* handle a modifer after having handled an empty regex.
	 * so a modifier is naturally disallowed for an empty regex. */
	PEEPNXTSC (sed, peeped, QSE_NULL);
	if (peeped == QSE_T('I')) 
	{
		ignorecase = 1;
		NXTSC (sed, peeped, QSE_NULL); /* consume the character peeped */
	}

	return build_rex (sed, QSE_STR_CSTR(&sed->tmp.rex), ignorecase, &sed->src.loc);
}

static qse_sed_adr_t* get_address (qse_sed_t* sed, qse_sed_adr_t* a, int extended)
{
	qse_cint_t c;

	c = CURSC (sed);
	if (c == QSE_T('$'))
	{
		a->type = QSE_SED_ADR_DOL;
		NXTSC (sed, c, QSE_NULL);
	}
	else if (c >= QSE_T('0') && c <= QSE_T('9'))
	{
		qse_size_t lno = 0;
		do
		{
			lno = lno * 10 + c - QSE_T('0');
			NXTSC (sed, c, QSE_NULL);
		}
		while (c >= QSE_T('0') && c <= QSE_T('9'));

		a->type = QSE_SED_ADR_LINE;
		a->u.lno = lno;
	}
	else if (c == QSE_T('/'))
	{
		/* /REGEX/ */
		a->u.rex = compile_rex_address (sed, c);
		if (a->u.rex == QSE_NULL) return QSE_NULL;
		a->type = QSE_SED_ADR_REX;
		NXTSC (sed, c, QSE_NULL);
	}
	else if (c == QSE_T('\\'))
	{
		/* \cREGEXc */
		NXTSC (sed, c, QSE_NULL);
		if (c == QSE_CHAR_EOF || IS_LINTERM(c))
		{
			SETERR1 (sed, QSE_SED_EREXIC, 
				QSE_T(""), 0, &sed->src.loc);
			return QSE_NULL;
		}

		a->u.rex = compile_rex_address (sed, c);
		if (a->u.rex == QSE_NULL) return QSE_NULL;
		a->type = QSE_SED_ADR_REX;
		NXTSC (sed, c, QSE_NULL);
	}
	else if (extended && (c == QSE_T('+') || c == QSE_T('~')))
	{
		qse_size_t lno = 0;

		a->type = (c == QSE_T('+'))? QSE_SED_ADR_RELLINE: QSE_SED_ADR_RELLINEM;

		NXTSC (sed, c, QSE_NULL);
		if (!(c >= QSE_T('0') && c <= QSE_T('9')))
		{
			SETERR0 (sed, QSE_SED_EA2MOI, &sed->src.loc);
			return QSE_NULL;
		}

		do
		{
			lno = lno * 10 + c - QSE_T('0');
			NXTSC (sed, c, QSE_NULL);
		}
		while (c >= QSE_T('0') && c <= QSE_T('9'));

		a->u.lno = lno;
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
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL); \
		goto errlabel; \
	} \
} while (0)

	qse_cint_t c;
	qse_str_t* t = QSE_NULL;

	t = qse_str_open (sed->mmgr, 0, 128);
	if (t == QSE_NULL) goto oops;

	c = CURSC (sed);

	do 
	{
		if (sed->option & QSE_SED_STRIPLS)
		{
			/* get the first non-space character */
			while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);
		}

		while (c != QSE_CHAR_EOF)
		{
			int nl = 0;

			if (c == QSE_T('\\'))
			{
				NXTSC_GOTO (sed, c, oops);
				if (c == QSE_CHAR_EOF) 
				{
					if (sed->option & QSE_SED_KEEPTBS) 
						ADD (sed, t, QSE_T('\\'), oops);
					break;
				}
			}
			else if (c == QSE_T('\n')) nl = 1; /* unescaped newline */

			ADD (sed, t, c, oops);

			if (c == QSE_T('\n'))
			{
				if (nl)
				{
					/* if newline is not escaped, stop */
					qse_cint_t dump;
					/* let's not pollute 'c' for ENSURELN check after done: */
					NXTSC_GOTO (sed, dump, oops);
					goto done;
				}

				/* else carry on reading the next line */
				NXTSC_GOTO (sed, c, oops);
				break;
			}

			NXTSC_GOTO (sed, c, oops);
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
	if (t) qse_str_close (t);
	return -1;

#undef ADD
}

static int get_label (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c;

	/* skip white spaces */
	c = CURSC (sed);
	while (IS_SPACE(c)) NXTSC (sed, c, -1);

	if (!IS_LABCHAR(c))
	{
		/* label name is empty */
		if (sed->option & QSE_SED_STRICT)
		{
			SETERR0 (sed, QSE_SED_ELABEM, &sed->src.loc);
			return -1;
		}

		/* empty label. noop command. don't register anything */
		qse_str_clear (&sed->tmp.lab);
	}
	else
	{
		qse_str_clear (&sed->tmp.lab);
		do
		{
			if (qse_str_ccat (&sed->tmp.lab, c) == (qse_size_t)-1) 
			{
				SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
				return -1;
			} 
			NXTSC (sed, c, -1);
		}
		while (IS_LABCHAR(c));

		if (qse_map_search (
			&sed->tmp.labs, 
			QSE_STR_PTR(&sed->tmp.lab),
			QSE_STR_LEN(&sed->tmp.lab)) != QSE_NULL)
		{
			SETERR1 (
				sed, QSE_SED_ELABDU,
				QSE_STR_PTR(&sed->tmp.lab),
				QSE_STR_LEN(&sed->tmp.lab),
				&sed->src.loc
			);
			return -1;
		}

		if (qse_map_insert (
			&sed->tmp.labs, 
			QSE_STR_PTR(&sed->tmp.lab), QSE_STR_LEN(&sed->tmp.lab),
			cmd, 0) == QSE_NULL)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
			return -1;
		}

	}

	while (IS_SPACE(c)) NXTSC (sed, c, -1);

	if (IS_CMDTERM(c)) 
	{
		if (c != QSE_T('}') && 
		    c != QSE_T('#') &&
		    c != QSE_CHAR_EOF) NXTSC (sed, c, -1);	
	}

	return 0;
}

static int terminate_command (qse_sed_t* sed)
{
	qse_cint_t c;

	c = CURSC (sed);
	while (IS_SPACE(c)) NXTSC (sed, c, -1);
	if (!IS_CMDTERM(c))
	{
		SETERR0 (sed, QSE_SED_ESCEXP, &sed->src.loc);
		return -1;
	}

	/* if the target is terminated by #, it should let the caller 
	 * to skip the comment text. so don't read in the next character.
	 * the same goes for brackets. */
	if (c != QSE_T('#') && 
	    c != QSE_T('{') &&
	    c != QSE_T('}') && 
	    c != QSE_CHAR_EOF) NXTSC (sed, c, -1);	
	return 0;
}

static int get_branch_target (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c;
	qse_str_t* t = QSE_NULL;
	qse_map_pair_t* pair;

	/* skip white spaces */
	c = CURSC(sed);
	while (IS_SPACE(c)) NXTSC (sed, c, -1);

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
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
		goto oops;
	}

	while (IS_LABCHAR(c))
	{
		if (qse_str_ccat (t, c) == (qse_size_t)-1) 
		{
			SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
			goto oops;
		} 

		NXTSC_GOTO (sed, c, oops);
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
	if (t) qse_str_close (t);
	return -1;
}

static int get_file (qse_sed_t* sed, qse_xstr_t* xstr)
{
	qse_cint_t c;
	qse_str_t* t = QSE_NULL;
	qse_size_t trailing_spaces = 0;

	/* skip white spaces */
	c = CURSC(sed);
	while (IS_SPACE(c)) NXTSC (sed, c, -1);

	if (IS_CMDTERM(c))
	{
		SETERR0 (sed, QSE_SED_EFILEM, &sed->src.loc);
		goto oops;	
	}

	t = qse_str_open (sed->mmgr, 0, 32);
	if (t == QSE_NULL) 
	{
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
		goto oops;
	}

	do
	{
		if (c == QSE_T('\0'))
		{
			/* the file name should not contain '\0' */
			SETERR0 (sed, QSE_SED_EFILIL, &sed->src.loc);
			goto oops;
		}

		if (IS_SPACE(c)) trailing_spaces++;
		else trailing_spaces = 0;

		if (c == QSE_T('\\'))
		{
			NXTSC_GOTO (sed, c, oops);
			if (c == QSE_T('\0') || c == QSE_CHAR_EOF || IS_LINTERM(c))
			{
				SETERR0 (sed, QSE_SED_EFILIL, &sed->src.loc);
				goto oops;
			}

			if (c == QSE_T('n')) c = QSE_T('\n');
		}

		if (qse_str_ccat (t, c) == (qse_size_t)-1) 
		{
			SETERR0 (sed, QSE_SED_ENOMEM, &sed->src.loc);
			goto oops;
		} 

		NXTSC_GOTO (sed, c, oops);
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
	if (t) qse_str_close (t);
	return -1;
}

#define CHECK_CMDIC(sed,cmd,c,action) \
do { \
	if (c == QSE_CHAR_EOF || IS_LINTERM(c)) \
	{ \
		SETERR1 (sed, QSE_SED_ECMDIC, \
			&cmd->type, 1, &sed->src.loc); \
		action; \
	} \
} while (0)

#define CHECK_CMDIC_ESCAPED(sed,cmd,c,action) \
do { \
	if (c == QSE_CHAR_EOF) \
	{ \
		SETERR1 (sed, QSE_SED_ECMDIC, \
			&cmd->type, 1, &sed->src.loc); \
		action; \
	} \
} while (0)

static int get_subst (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c, delim;
	qse_str_t* t[2] = { QSE_NULL, QSE_NULL };

	c = CURSC (sed);
	CHECK_CMDIC (sed, cmd, c, goto oops);

	delim = c;	
	if (delim == QSE_T('\\'))
	{
		/* backspace is an illegal delimiter */
		SETERR0 (sed, QSE_SED_EBSDEL, &sed->src.loc);
		goto oops;
	}

	t[0] = &sed->tmp.rex;
	qse_str_clear (t[0]);

	t[1] = qse_str_open (sed->mmgr, 0, 32);
	if (t[1] == QSE_NULL) 
	{
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
		goto oops;
	}

	if (pickup_rex (sed, delim, 0, cmd, t[0]) <= -1) goto oops;
	if (pickup_rex (sed, delim, 1, cmd, t[1]) <= -1) goto oops;

	/* skip spaces before options */
	do { NXTSC_GOTO (sed, c, oops); } while (IS_SPACE(c));

	/* get options */
	do
	{
		if (c == QSE_T('p')) 
		{
			cmd->u.subst.p = 1;
			NXTSC_GOTO (sed, c, oops);
		}
		else if (c == QSE_T('i') || c == QSE_T('I')) 
		{
			cmd->u.subst.i = 1;
			NXTSC_GOTO (sed, c, oops);
		}
		else if (c == QSE_T('g')) 
		{
			cmd->u.subst.g = 1;
			NXTSC_GOTO (sed, c, oops);
		}
		else if (c >= QSE_T('0') && c <= QSE_T('9'))
		{
			unsigned long occ;

			if (cmd->u.subst.occ != 0)
			{
				SETERR0 (sed, QSE_SED_EOCSDU, &sed->src.loc);
				goto oops;
			}

			occ = 0;

			do 
			{
				occ = occ * 10 + (c - QSE_T('0')); 
				if (occ > QSE_TYPE_MAX(unsigned short))
				{
					SETERR0 (sed, QSE_SED_EOCSTL, &sed->src.loc);
					goto oops;
				}
				NXTSC_GOTO (sed, c, oops);
			}
			while (c >= QSE_T('0') && c <= QSE_T('9'));

			if (occ == 0)
			{
				SETERR0 (sed, QSE_SED_EOCSZE, &sed->src.loc);
				goto oops;
			}

			cmd->u.subst.occ = occ;
		}
		else if (c == QSE_T('w'))
		{
			NXTSC_GOTO (sed, c, oops);
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

	if (QSE_STR_LEN(t[0]) <= 0) cmd->u.subst.rex = EMPTY_REX;
	else
	{
		cmd->u.subst.rex = build_rex (
			sed, QSE_STR_CSTR(t[0]), 
			cmd->u.subst.i, &sed->src.loc);
		if (cmd->u.subst.rex == QSE_NULL) goto oops;
	}

	qse_str_yield (t[1], &cmd->u.subst.rpl, 0);
	if (cmd->u.subst.g == 0 && cmd->u.subst.occ == 0) cmd->u.subst.occ = 1;

	qse_str_close (t[1]);
	return 0;

oops:
	if (t[1]) qse_str_close (t[1]);
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
		SETERR0 (sed, QSE_SED_EBSDEL, &sed->src.loc);
		goto oops;
	}

	t = qse_str_open (sed->mmgr, 0, 32);
	if (t == QSE_NULL) 
	{
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
		goto oops;
	}

	NXTSC_GOTO (sed, c, oops);
	while (c != delim)
	{
		qse_char_t b[2];

		CHECK_CMDIC (sed, cmd, c, goto oops);

		if (c == QSE_T('\\'))
		{
			NXTSC_GOTO (sed, c, oops);
			CHECK_CMDIC_ESCAPED (sed, cmd, c, goto oops);
			if (trans_escaped (sed, c, &c, QSE_NULL) <= -1) goto oops;
		}

		b[0] = c;
		if (qse_str_ncat (t, b, 2) == (qse_size_t)-1)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
			goto oops;
		}

		NXTSC_GOTO (sed, c, oops);
	}	

	NXTSC_GOTO (sed, c, oops);
	for (pos = 1; c != delim; pos += 2)
	{
		CHECK_CMDIC (sed, cmd, c, goto oops);

		if (c == QSE_T('\\'))
		{
			NXTSC_GOTO (sed, c, oops);
			CHECK_CMDIC_ESCAPED (sed, cmd, c, goto oops);
			if (trans_escaped (sed, c, &c, QSE_NULL) <= -1) goto oops;
		}

		if (pos >= QSE_STR_LEN(t))
		{
			/* source and target not the same length */
			SETERR0 (sed, QSE_SED_ETSNSL, &sed->src.loc);
			goto oops;
		}

		QSE_STR_CHAR(t,pos) = c;
		NXTSC_GOTO (sed, c, oops);
	}

	if (pos < QSE_STR_LEN(t))
	{
		/* source and target not the same length */
		SETERR0 (sed, QSE_SED_ETSNSL, &sed->src.loc);
		goto oops;
	}

	NXTSC_GOTO (sed, c, oops);
	if (terminate_command (sed) <= -1) goto oops;

	qse_str_yield (t, &cmd->u.transet, 0);
	qse_str_close (t);
	return 0;

oops:
	if (t) qse_str_close (t);
	return -1;
}

static int add_cut_selector_block (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_sed_cut_sel_t* b;

	b = (qse_sed_cut_sel_t*) QSE_MMGR_ALLOC (sed->mmgr, QSE_SIZEOF(*b));
	if (b == QSE_NULL)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
		return -1;
	}

	QSE_MEMSET (b, 0, QSE_SIZEOF(*b));
	b->next = QSE_NULL;
	b->len = 0;

	if (cmd->u.cut.fb == QSE_NULL) 
	{
		cmd->u.cut.fb = b;
		cmd->u.cut.lb = b;
	}
	else
	{
		cmd->u.cut.lb->next = b;
		cmd->u.cut.lb = b;
	}

	return 0;
}

static void free_all_cut_selector_blocks (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_sed_cut_sel_t* b, * next;

	for (b = cmd->u.cut.fb; b; b = next)
	{
		next = b->next;
		QSE_MMGR_FREE (sed->mmgr, b);
	}

	cmd->u.cut.lb = QSE_NULL;
	cmd->u.cut.fb = QSE_NULL;

	cmd->u.cut.count = 0;
	cmd->u.cut.fcount = 0;
	cmd->u.cut.ccount = 0;
}

static int get_cut (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c, delim;
	qse_size_t i;
	int sel = QSE_SED_CUT_SEL_CHAR;

	c = CURSC (sed);
	CHECK_CMDIC (sed, cmd, c, goto oops);

	delim = c;	
	if (delim == QSE_T('\\'))
	{
		/* backspace is an illegal delimiter */
		SETERR0 (sed, QSE_SED_EBSDEL, &sed->src.loc);
		goto oops;
	}

	/* initialize the delimeter to a space letter */
	for (i = 0; i < QSE_COUNTOF(cmd->u.cut.delim); i++)
		cmd->u.cut.delim[i] = QSE_T(' ');

	NXTSC_GOTO (sed, c, oops);
	while (1)
	{
		qse_size_t start = 0, end = 0;

#define MASK_START (1 << 1)
#define MASK_END (1 << 2)
#define MAX QSE_TYPE_MAX(qse_size_t)
		int mask = 0;

		while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);
		if (c == QSE_CHAR_EOF)
		{
			SETERR0 (sed, QSE_SED_ECSLNV, &sed->src.loc);
			goto oops;
		}

		if (c == QSE_T('d') || c == QSE_T('D'))
		{
			int delim_idx = (c == QSE_T('d'))? 0: 1;
			/* the next character is an input/output delimiter. */
			NXTSC_GOTO (sed, c, oops);
			if (c == QSE_CHAR_EOF)
			{
				SETERR0 (sed, QSE_SED_ECSLNV, &sed->src.loc);
				goto oops;
			}
			cmd->u.cut.delim[delim_idx] = c;
			NXTSC_GOTO (sed, c, oops);
		}
		else
		{
			if (c == QSE_T('c') || c == QSE_T('f'))
			{
				sel = c;
				NXTSC_GOTO (sed, c, oops);
				while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);
			}

			if (QSE_ISDIGIT(c))
			{
				do 
				{ 
					start = start * 10 + (c - QSE_T('0')); 
					NXTSC_GOTO (sed, c, oops);
				} 
				while (QSE_ISDIGIT(c));
	
				while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);
				mask |= MASK_START;

				if (start >= 1) start--; /* convert it to index */
			}
			else start = 0;

			if (c == QSE_T('-'))
			{
				NXTSC_GOTO (sed, c, oops);
				while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);

				if (QSE_ISDIGIT(c))
				{
					do 
					{ 
						end = end * 10 + (c - QSE_T('0')); 
						NXTSC_GOTO (sed, c, oops);
					} 
					while (QSE_ISDIGIT(c));
					mask |= MASK_END;
				}
				else end = MAX;

				while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);

				if (end >= 1) end--; /* convert it to index */
			}
			else end = start;

			if (!(mask & (MASK_START | MASK_END)))
			{
				SETERR0 (sed, QSE_SED_ECSLNV, &sed->src.loc);
				goto oops;
			}

			if (cmd->u.cut.lb == QSE_NULL ||
			    cmd->u.cut.lb->len >= QSE_COUNTOF(cmd->u.cut.lb->range))
			{
				if (add_cut_selector_block (sed, cmd) <= -1) goto oops;
			}

			cmd->u.cut.lb->range[cmd->u.cut.lb->len].id = sel;
			cmd->u.cut.lb->range[cmd->u.cut.lb->len].start = start;
			cmd->u.cut.lb->range[cmd->u.cut.lb->len].end = end;
			cmd->u.cut.lb->len++;

			cmd->u.cut.count++;
			if (sel == QSE_SED_CUT_SEL_FIELD) cmd->u.cut.fcount++;
			else cmd->u.cut.ccount++;
		}

		while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);

		if (c == QSE_CHAR_EOF)
		{
			SETERR0 (sed, QSE_SED_ECSLNV, &sed->src.loc);
			goto oops;
		}

		if (c == delim) break;

		if (c != QSE_T(',')) 
		{
			SETERR0 (sed, QSE_SED_ECSLNV, &sed->src.loc);
			goto oops;
		}
		NXTSC_GOTO (sed, c, oops); /* skip a comma */
	}

	/* skip spaces before options */
	do { NXTSC_GOTO (sed, c, oops); } while (IS_SPACE(c));

	/* get options */
	do
	{
		if (c == QSE_T('f')) 
		{
			cmd->u.cut.f = 1;
		}
		else if (c == QSE_T('w')) 
		{
			cmd->u.cut.w = 1;
		}
		else if (c == QSE_T('d'))
		{
			cmd->u.cut.d = 1;
		}
		else break;

		NXTSC_GOTO (sed, c, oops);
	}
	while (1);

	if (terminate_command (sed) <= -1) goto oops;
	return 0;

oops:
	free_all_cut_selector_blocks (sed, cmd);
	return -1;
}

/* process a command code and following parts into cmd */
static int get_command (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cint_t c;

	c = CURSC (sed);
	cmd->lid = sed->src.cid? ((const qse_char_t*)(sed->src.cid + 1)): QSE_NULL;
	cmd->loc = sed->src.loc;
	switch (c)
	{
		default:
		{
			qse_char_t cc = c;
			SETERR1 (sed, QSE_SED_ECMDNR, &cc, 1, &sed->src.loc);
			return -1;
		}

		case QSE_CHAR_EOF:
		case QSE_T('\n'):
			SETERR0 (sed, QSE_SED_ECMDMS, &sed->src.loc);
			return -1;	

		case QSE_T(':'):
			if (cmd->a1.type != QSE_SED_ADR_NONE)
			{
				/* label cannot have an address */
				SETERR1 (
					sed, QSE_SED_EA1PHB,
					&cmd->type, 1, &sed->src.loc
				);
				return -1;
			}

			cmd->type = QSE_SED_CMD_NOOP;

			NXTSC (sed, c, -1);
			if (get_label (sed, cmd) <= -1) return -1;

			c = CURSC (sed);
			while (IS_SPACE(c)) NXTSC (sed, c, -1);
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
				SETERR0 (sed, QSE_SED_EGRNTD, &sed->src.loc);
				return -1;
			}

			sed->tmp.grp.cmd[sed->tmp.grp.level++] = cmd;
			NXTSC (sed, c, -1);
			break;

		case QSE_T('}'):
		{
			qse_sed_cmd_t* tc;

			if (cmd->a1.type != QSE_SED_ADR_NONE)
			{
				qse_char_t tmpc = c;
				SETERR1 (
					sed, QSE_SED_EA1PHB,
					&tmpc, 1, &sed->src.loc
				);
				return -1;
			}

			cmd->type = QSE_SED_CMD_NOOP;

			if (sed->tmp.grp.level <= 0) 
			{
				/* group not balanced */
				SETERR0 (sed, QSE_SED_EGRNBA, &sed->src.loc);
				return -1;
			}

			tc = sed->tmp.grp.cmd[--sed->tmp.grp.level];
			tc->u.branch.target = cmd;

			NXTSC (sed, c, -1);
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
					&cmd->type, 1, &sed->src.loc
				);
				return -1;
			}

			NXTSC (sed, c, -1);
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
					&tmpc, 1, &sed->src.loc
				);
				return -1;
			}
		case QSE_T('c'):
		{
			cmd->type = c;

			NXTSC (sed, c, -1);
			while (IS_SPACE(c)) NXTSC (sed, c, -1);

			if (c != QSE_T('\\'))
			{
				if ((sed->option & QSE_SED_SAMELINE) && 
				    c != QSE_CHAR_EOF && c != QSE_T('\n')) 
				{
					/* allow text without a starting backslash 
					 * on the same line as a command */
					goto sameline_ok;
				}

				SETERR0 (sed, QSE_SED_EBSEXP, &sed->src.loc);
				return -1;
			}
		
			NXTSC (sed, c, -1);
			while (IS_SPACE(c)) NXTSC (sed, c, -1);

			if (c != QSE_CHAR_EOF && c != QSE_T('\n'))
			{
				if (sed->option & QSE_SED_SAMELINE) 
				{
					/* allow text with a starting backslash 
					 * on the same line as a command */
					goto sameline_ok;
				}

				SETERR0 (sed, QSE_SED_EGBABS, &sed->src.loc);
				return -1;
			}
			
			NXTSC (sed, c, -1); /* skip the new line */

		sameline_ok:
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
					&tmpc, 1, &sed->src.loc
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

		case QSE_T('z'):
			cmd->type = c;
			NXTSC (sed, c, -1); 
			if (terminate_command (sed) <= -1) return -1;
			break;

		case QSE_T('b'):
		case QSE_T('t'):
			cmd->type = c;
			NXTSC (sed, c, -1); 
			if (get_branch_target (sed, cmd) <= -1) return -1;
			break;

		case QSE_T('r'):
		case QSE_T('R'):
		case QSE_T('w'):
		case QSE_T('W'):
			cmd->type = c;
			NXTSC (sed, c, -1); 
			if (get_file (sed, &cmd->u.file) <= -1) return -1;
			break;

		case QSE_T('s'):
			cmd->type = c;
			NXTSC (sed, c, -1); 
			if (get_subst (sed, cmd) <= -1) return -1;
			break;

		case QSE_T('y'):
			cmd->type = c;
			NXTSC (sed, c, -1); 
			if (get_transet (sed, cmd) <= -1) return -1;
			break;

		case QSE_T('C'):
			cmd->type = c;
			NXTSC (sed, c, -1);
			if (get_cut (sed, cmd) <= -1) return -1;
			break;
	}

	return 0;
}

int qse_sed_comp (qse_sed_t* sed, qse_sed_io_fun_t inf)
{
	qse_cint_t c;
	qse_sed_cmd_t* cmd = QSE_NULL;
	qse_sed_loc_t a1_loc;

	if (inf == QSE_NULL)
	{
		qse_sed_seterrnum (sed, QSE_SED_EINVAL, QSE_NULL);
		return -1;	
	}

	/* free all the commands previously compiled */
	free_all_command_blocks (sed);
	QSE_ASSERT (sed->cmd.lb == &sed->cmd.fb && sed->cmd.lb->len == 0);

	/* free all the compilation identifiers */
	free_all_cids (sed);

	/* clear the label table */
	qse_map_clear (&sed->tmp.labs);

	/* clear temporary data */
	sed->tmp.grp.level = 0;
	qse_str_clear (&sed->tmp.rex);

	/* open script */
	sed->src.fun = inf;
	if (open_script_stream (sed) <= -1) return -1;
	NXTSC_GOTO (sed, c, oops);

	while (1)
	{
		int n;

		/* skip spaces including newlines */
		while (IS_WSPACE(c)) NXTSC_GOTO (sed, c, oops);

		/* check if the end has been reached */
		if (c == QSE_CHAR_EOF) break;

		/* check if the line is commented out */
		if (c == QSE_T('#'))
		{
			do NXTSC_GOTO (sed, c, oops); 
			while (!IS_LINTERM(c) && c != QSE_CHAR_EOF) ;
			NXTSC_GOTO (sed, c, oops);
			continue;
		}

		if (c == QSE_T(';')) 
		{
			/* semicolon without a address-command pair */
			NXTSC_GOTO (sed, c, oops);
			continue;
		}

		/* initialize the current command */
		cmd = &sed->cmd.lb->buf[sed->cmd.lb->len];
		QSE_MEMSET (cmd, 0, QSE_SIZEOF(*cmd));

		/* process the first address */
		a1_loc = sed->src.loc;
		if (get_address (sed, &cmd->a1, 0) == QSE_NULL) 
		{
			cmd = QSE_NULL;
			SETERR0 (sed, QSE_SED_EA1MOI, &sed->src.loc);
			goto oops;
		}

		c = CURSC (sed);
		if (cmd->a1.type != QSE_SED_ADR_NONE)
		{
			while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);

			if (c == QSE_T(',') ||
			    ((sed->option & QSE_SED_EXTENDEDADR) && c == QSE_T('~')))
			{
				qse_char_t delim = c;

				/* maybe an address range */
				do { NXTSC_GOTO (sed, c, oops); } while (IS_SPACE(c));

				if (get_address (sed, &cmd->a2, (sed->option & QSE_SED_EXTENDEDADR)) == QSE_NULL) 
				{
					QSE_ASSERT (cmd->a2.type == QSE_SED_ADR_NONE);
					SETERR0 (sed, QSE_SED_EA2MOI, &sed->src.loc);
					goto oops;
				}

				if (delim == QSE_T(','))
				{
					if (cmd->a2.type == QSE_SED_ADR_NONE)
					{
						SETERR0 (sed, QSE_SED_EA2MOI, &sed->src.loc);
						goto oops;
					}
					if (cmd->a2.type == QSE_SED_ADR_RELLINE || 
					    cmd->a2.type == QSE_SED_ADR_RELLINEM)
					{
						if (cmd->a2.u.lno <= 0) 
						{
							/* tranform 'addr1,+0' and 'addr1,~0' to 'addr1' */
							cmd->a2.type = QSE_SED_ADR_NONE;
						}
					}
				}
				else if ((sed->option & QSE_SED_EXTENDEDADR) && 
				         (delim == QSE_T('~')))
				{
					if (cmd->a1.type != QSE_SED_ADR_LINE || 
					    cmd->a2.type != QSE_SED_ADR_LINE)
					{
						SETERR0 (sed, QSE_SED_EA2MOI, &sed->src.loc);
						goto oops;
					}

					if (cmd->a2.u.lno > 0)
					{
						cmd->a2.type = QSE_SED_ADR_STEP;	
					}
					else
					{
						/* transform 'X,~0' to 'X' */
						cmd->a2.type = QSE_SED_ADR_NONE;
					}
				}

				c = CURSC (sed);
			}
			else cmd->a2.type = QSE_SED_ADR_NONE;
		}

		if (cmd->a1.type == QSE_SED_ADR_LINE && cmd->a1.u.lno <= 0)
		{
			if (cmd->a2.type == QSE_SED_ADR_STEP || 
			    ((sed->option & QSE_SED_EXTENDEDADR) && 
			     cmd->a2.type == QSE_SED_ADR_REX))
			{
				/* 0 as the first address is allowed in this two contexts.
				 *    0~step
				 *    0,/regex/
				 * '0~0' is not allowed. but at this point '0~0' 
				 * is already transformed to '0'. and disallowing it is 
				 * achieved gratuitously.
				 */
				/* nothing to do - adding negation to the condition dropped 
				 * code readability so i decided to write this part of code
				 * this way. 
				 */
			}
			else
			{
				SETERR0 (sed, QSE_SED_EA1MOI, &a1_loc);
				goto oops;
			}
		}

		/* skip white spaces */
		while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);

		if (c == QSE_T('!'))
		{
			/* allow any number of the negation indicators */
			do { 
				cmd->negated = !cmd->negated; 
				NXTSC_GOTO (sed, c, oops);
			} 
			while (c == QSE_T('!'));

			while (IS_SPACE(c)) NXTSC_GOTO (sed, c, oops);
		}
	
		n = get_command (sed, cmd);
		if (n <= -1) goto oops;

		c = CURSC (sed);

		/* cmd's end of life */
		cmd = QSE_NULL;

		/* increment the total numbers of complete commands */
		sed->cmd.lb->len++;
		if (sed->cmd.lb->len >= QSE_COUNTOF(sed->cmd.lb->buf))
		{
			/* the number of commands in the block has
			 * reaches the maximum. add a new command block */
			if (add_command_block (sed) <= -1) goto oops;
		}
	}

	if (sed->tmp.grp.level != 0)
	{
		SETERR0 (sed, QSE_SED_EGRNBA, &sed->src.loc);
		goto oops;
	}

	close_script_stream (sed);
	return 0;

oops:
	if (cmd) free_address (sed, cmd);
	close_script_stream (sed);
	return -1;
}

static int read_char (qse_sed_t* sed, qse_char_t* c)
{
	qse_ssize_t n;

	if (sed->e.in.xbuf_len == 0)
	{
		if (sed->e.in.pos >= sed->e.in.len)
		{
			sed->errnum = QSE_SED_ENOERR;
			n = sed->e.in.fun (
				sed, QSE_SED_IO_READ, &sed->e.in.arg, 
				sed->e.in.buf, QSE_COUNTOF(sed->e.in.buf)
			);
			if (n <= -1) 
			{
				if (sed->errnum == QSE_SED_ENOERR)
					SETERR0 (sed, QSE_SED_EIOUSR, QSE_NULL);
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
			SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
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

		n = sed->e.out.fun (
			sed, QSE_SED_IO_WRITE, &sed->e.out.arg,
			&sed->e.out.buf[pos], sed->e.out.len);

		if (n <= -1)
		{
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR0 (sed, QSE_SED_EIOUSR, QSE_NULL);
			return -1;
		}

		if (n == 0)
		{
			/* reached the end of file - this is also an error */
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR0 (sed, QSE_SED_EIOUSR, QSE_NULL);
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
	int flush_needed = 0;

	for (i = 0; i < len; i++)
	{
		/*if (write_char (sed, str[i]) <= -1) return -1;*/
		sed->e.out.buf[sed->e.out.len++] = str[i];
		if (sed->e.out.len >= QSE_COUNTOF(sed->e.out.buf))
		{
			if (flush (sed) <= -1) return -1;
			flush_needed = 0;
		}
		/* TODO: handle different line ending convension... */
		else if (str[i] == QSE_T('\n')) flush_needed = 1;
	}

	if (flush_needed && flush(sed) <= -1) return -1;
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

#define NTOC(n) (QSE_T("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ")[n])

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
				#ifdef QSE_CHAR_IS_MCHAR
					WRITE_CHAR (sed, QSE_T('\\'));
					WRITE_NUM (sed, (unsigned char)c, 8, QSE_SIZEOF(qse_char_t)*3);
				#else
					if (QSE_SIZEOF(qse_char_t) <= 2)
					{
						WRITE_STR (sed, QSE_T("\\u"), 2);
					}
					else
					{
						WRITE_STR (sed, QSE_T("\\U"), 2);
					}
					WRITE_NUM (sed, c, 16, QSE_SIZEOF(qse_char_t)*2);
				#endif
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
			SETERR0 (sed, QSE_SED_ENOMEM, &cmd->loc);
			return -1;
		}
	}

	ap = QSE_MAP_VPTR(pair);
	if (ap->handle == QSE_NULL)
	{
		sed->errnum = QSE_SED_ENOERR;
		ap->path = path;
		n = sed->e.out.fun (sed, QSE_SED_IO_OPEN, ap, QSE_NULL, 0);
		if (n <= -1)
		{
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR1 (sed, QSE_SED_EIOFIL, path, plen, &cmd->loc);
			else sed->errloc = cmd->loc;
			return -1;
		}
		if (n == 0)
		{
			/* EOF is returned upon opening a write stream.
			 * it is also an error as it can't write 
			 * a requested string */
			sed->e.out.fun (sed, QSE_SED_IO_CLOSE, ap, QSE_NULL, 0);
			ap->handle = QSE_NULL;
			SETERR1 (sed, QSE_SED_EIOFIL, path, plen, &cmd->loc);
			return -1;
		}
	}

	while (len > 0)
	{
		sed->errnum = QSE_SED_ENOERR;
		n = sed->e.out.fun (
			sed, QSE_SED_IO_WRITE, ap, (qse_char_t*)str, len);
		if (n <= -1) 
		{
			sed->e.out.fun (sed, QSE_SED_IO_CLOSE, ap, QSE_NULL, 0);
			ap->handle = QSE_NULL;
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR1 (sed, QSE_SED_EIOFIL, path, plen, &cmd->loc);
			sed->errloc = cmd->loc;
			return -1;
		}

		if (n == 0)
		{
			/* eof is returned on the write stream. 
			 * it is also an error as it can't write any more */
			sed->e.out.fun (sed, QSE_SED_IO_CLOSE, ap, QSE_NULL, 0);
			ap->handle = QSE_NULL;
			SETERR1 (sed, QSE_SED_EIOFIL, path, plen, &cmd->loc);
			return -1;
		}

		len -= n;
	}

	return 0;
}

static int write_file (
	qse_sed_t* sed, qse_sed_cmd_t* cmd, int first_line)
{
	qse_ssize_t n;
	qse_sed_io_arg_t arg;
#ifdef QSE_CHAR_IS_MCHAR
	qse_char_t buf[1024];
#else
	qse_char_t buf[512];
#endif

	arg.handle = QSE_NULL;
	arg.path = cmd->u.file.ptr;
	sed->errnum = QSE_SED_ENOERR;
	n = sed->e.in.fun (sed, QSE_SED_IO_OPEN, &arg, QSE_NULL, 0);
	if (n <= -1)
	{
		/*if (sed->errnum != QSE_SED_ENOERR)
		 *	SETERR0 (sed, QSE_SED_EIOUSR, &cmd->loc);
		 *return -1;*/
		/* it is ok if it is not able to open a file */
		return 0;	
	}
	if (n == 0) 
	{
		/* EOF - no data */
		sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &arg, QSE_NULL, 0);
		return 0;
	}

	while (1)
	{
		sed->errnum = QSE_SED_ENOERR;
		n = sed->e.in.fun (
			sed, QSE_SED_IO_READ, &arg, buf, QSE_COUNTOF(buf));
		if (n <= -1)
		{
			sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &arg, QSE_NULL, 0);
			if (sed->errnum == QSE_SED_ENOERR)
				SETERR1 (sed, QSE_SED_EIOFIL, cmd->u.file.ptr, cmd->u.file.len, &cmd->loc);
			else sed->errloc = cmd->loc;
			return -1;
		}
		if (n == 0) break;

		if (first_line)
		{
			qse_size_t i;

			for (i = 0; i < n; i++)
			{
				if (write_char (sed, buf[i]) <= -1) return -1;

				/* TODO: support different line end convension */
				if (buf[i] == QSE_T('\n')) goto done;
			}
		}
		else
		{
			if (write_str (sed, buf, n) <= -1) return -1;
		}
	}

done:
	sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &arg, QSE_NULL, 0);
	return 0;
}

static int link_append (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	if (sed->e.append.count < QSE_COUNTOF(sed->e.append.s))
	{
		/* link it to the static buffer if it is not full */
		sed->e.append.s[sed->e.append.count++].cmd = cmd;
	}
	else
	{
		qse_sed_app_t* app;

		/* otherwise, link it using a linked list */
		app = QSE_MMGR_ALLOC (sed->mmgr, QSE_SIZEOF(*app));
		if (app == QSE_NULL)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, &cmd->loc);
			return -1;
		}
		app->cmd = cmd;
		app->next = QSE_NULL;

		if (sed->e.append.d.tail == QSE_NULL)
			sed->e.append.d.head = app;
		else
			sed->e.append.d.tail->next = app;
		sed->e.append.d.tail = app;
		/*sed->e.append.count++; don't really care */
	}

	return 0;
}

static void free_appends (qse_sed_t* sed)
{
	qse_sed_app_t* app = sed->e.append.d.head;
	qse_sed_app_t* next;
	
	while (app)
	{	
		next = app->next;
		QSE_MMGR_FREE (sed->mmgr, app);
		app = next;
	}

	sed->e.append.d.head = QSE_NULL;
	sed->e.append.d.tail = QSE_NULL;
	sed->e.append.count = 0;		
}

static int emit_append (qse_sed_t* sed, qse_sed_app_t* app)
{
	switch (app->cmd->type)
	{
		case QSE_SED_CMD_APPEND:
			return write_str (sed, app->cmd->u.text.ptr, app->cmd->u.text.len);

		case QSE_SED_CMD_READ_FILE:
			return write_file (sed, app->cmd, 0);

		case QSE_SED_CMD_READ_FILELN:
			return write_file (sed, app->cmd, 1);

		default:
			QSE_ASSERTX (
				!"should never happen", 
				"app->cmd->type must be one of APPEND,READ_FILE,READ_FILELN"
			);
			SETERR0 (sed, QSE_SED_EINTERN, &app->cmd->loc);
			return -1;
	}
}

static int emit_appends (qse_sed_t* sed)
{
	qse_sed_app_t* app;
	qse_size_t i;

	for (i = 0; i < sed->e.append.count; i++)
	{
		if (emit_append (sed, &sed->e.append.s[i]) <= -1) return -1;
	}

	app = sed->e.append.d.head;
	while (app)
	{	
		if (emit_append (sed, app) <= -1) return -1;
		app = app->next;
	}

	return 0;
}

static const qse_char_t* trim_line (qse_sed_t* sed, qse_cstr_t* str)
{
	const qse_char_t* lineterm;

	str->ptr = QSE_STR_PTR(&sed->e.in.line);
	str->len = QSE_STR_LEN(&sed->e.in.line);

	/* TODO: support different line end convension */
	if (str->len > 0 && str->ptr[str->len-1] == QSE_T('\n')) 
	{
		str->len--;
		if (str->len > 0 && str->ptr[str->len-1] == QSE_T('\r')) 
		{
			lineterm = QSE_T("\r\n");
			str->len--;
		}
		else
		{
			lineterm = QSE_T("\n");
		}
	}
	else lineterm = QSE_NULL;

	return lineterm;
}

static int do_subst (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_cstr_t mat, pmat;
	int opt = 0, repl = 0, n;
#if defined(USE_REX)
	qse_rex_errnum_t errnum;
#endif
	const qse_char_t* lineterm;

	qse_cstr_t str, cur;
	const qse_char_t* str_end;
	qse_size_t m, i, max_count, sub_count;

	QSE_ASSERT (cmd->type == QSE_SED_CMD_SUBSTITUTE);

	qse_str_clear (&sed->e.txt.scratch);
#if defined(USE_REX)
	if (cmd->u.subst.i) opt = QSE_REX_IGNORECASE;
#endif

	lineterm = trim_line (sed, &str);
		
	str_end = str.ptr + str.len;
	cur = str;

	sub_count = 0;
	max_count = (cmd->u.subst.g)? 0: cmd->u.subst.occ;

	pmat.ptr = QSE_NULL;
	pmat.len = 0;

	/* perform test when cur_ptr == str_end also because
	 * end of string($) needs to be tested */
	while (cur.ptr <= str_end)
	{
#ifndef USE_REX
		qse_cstr_t submat[9];
		QSE_MEMSET (submat, 0, QSE_SIZEOF(submat));
#endif

		if (max_count == 0 || sub_count < max_count)
		{
			void* rex;

			if (cmd->u.subst.rex == EMPTY_REX)
			{
				rex = sed->e.last_rex;
				if (rex == QSE_NULL)
				{
					SETERR0 (sed, QSE_SED_ENPREX, &cmd->loc);
					return -1;
				}
			}
			else 
			{
				rex = cmd->u.subst.rex;
				sed->e.last_rex = rex;
			}

#if defined(USE_REX)
			n = qse_matchrex (
				sed->mmgr, 
				sed->depth.rex.match,
				rex, opt,
				&str, &cur, &mat, &errnum
			);
			if (n <= -1)
			{
				SETERR0 (sed, QSE_SED_EREXMA, &cmd->loc);
				return -1;
			}
#else
			n = matchtre (
				sed, rex,
				((str.ptr == cur.ptr)? opt: (opt | QSE_TRE_NOTBOL)),
				&cur, &mat, submat, &cmd->loc
			);
			if (n <= -1) return -1;
#endif
		}
		else n = 0;

		if (n == 0) 
		{
			/* no more match found */
			if (qse_str_ncat (
				&sed->e.txt.scratch,
				cur.ptr, cur.len) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
				return -1;
			}
			break;
		}

		if (mat.len == 0 && 
		    pmat.ptr && mat.ptr == pmat.ptr + pmat.len)
		{
			/* match length is 0 and the match is still at the
			 * end of the previous match */
			goto skip_one_char;
		}

		if (max_count > 0 && sub_count + 1 != max_count)
		{
			if (cur.ptr < str_end)
			{
				m = qse_str_ncat (
					&sed->e.txt.scratch,
					cur.ptr, mat.ptr-cur.ptr+mat.len
				);
				if (m == (qse_size_t)-1)
				{
					SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
					return -1;
				}
			}
		}
		else
		{
			repl = 1;

			if (cur.ptr < str_end)
			{
				m = qse_str_ncat (
					&sed->e.txt.scratch, cur.ptr, mat.ptr-cur.ptr
				);
				if (m == (qse_size_t)-1)
				{
					SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
					return -1;
				}
			}

			for (i = 0; i < cmd->u.subst.rpl.len; i++)
			{
				if ((i+1) < cmd->u.subst.rpl.len && 
				    cmd->u.subst.rpl.ptr[i] == QSE_T('\\'))
				{
					qse_char_t nc = cmd->u.subst.rpl.ptr[i+1];

#ifndef USE_REX
					if (nc >= QSE_T('1') && nc <= QSE_T('9'))
					{
						int smi = nc - QSE_T('1');
						m = qse_str_ncat (
							&sed->e.txt.scratch,
							submat[smi].ptr, submat[smi].len
						);
					}
					else
					{
#endif
						/* the know speical characters have been escaped
						 * in get_subst(). so i don't call trans_escaped() here */
						m = qse_str_ccat (&sed->e.txt.scratch, nc);
#ifndef USE_REX
					}
#endif

					i++;
				}
				else if (cmd->u.subst.rpl.ptr[i] == QSE_T('&'))
				{
					m = qse_str_ncat (
						&sed->e.txt.scratch,
						mat.ptr, mat.len);
				}
				else 
				{
					m = qse_str_ccat (
						&sed->e.txt.scratch,
						cmd->u.subst.rpl.ptr[i]);
				}

				if (m == (qse_size_t)-1)
				{
					SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
					return -1;
				}
			}
		}

		sub_count++;
		cur.len = cur.len - ((mat.ptr - cur.ptr) + mat.len);
		cur.ptr = mat.ptr + mat.len;

		pmat = mat;

		if (mat.len == 0)
		{
		skip_one_char:
			if (cur.ptr < str_end)
			{
				/* special treament is needed if the match length is 0 */
				m = qse_str_ncat (&sed->e.txt.scratch, cur.ptr, 1);
				if (m == (qse_size_t)-1)
				{
					SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
					return -1;
				}
			}

			cur.ptr++; cur.len--;
		}
	}

	if (lineterm)
	{
		m = qse_str_cat (&sed->e.txt.scratch, lineterm);
		if (m == (qse_size_t)-1)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
			return -1;
		}
	}

	qse_str_swap (&sed->e.in.line, &sed->e.txt.scratch);

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

		if (cmd->u.subst.file.ptr)
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

static int split_into_fields_for_cut (
	qse_sed_t* sed, qse_sed_cmd_t* cmd, const qse_cstr_t* str)
{
	qse_size_t i, x = 0, xl = 0;

	sed->e.cutf.delimited = 0;
	sed->e.cutf.flds[x].ptr = str->ptr;

	for (i = 0; i < str->len; )
	{
		int isdelim = 0;
		qse_char_t c = str->ptr[i++];

		if (cmd->u.cut.w)
		{ 
			/* the w option ignores the d specifier */
			if (QSE_ISSPACE(c))
			{
				/* the w option assumes the f option */
				while (i < str->len && QSE_ISSPACE(str->ptr[i])) i++;
				isdelim = 1;
			}
		}
		else
		{
			if (c == cmd->u.cut.delim[0])
			{
				if (cmd->u.cut.f)
				{
					/* fold consecutive delimiters */
					while (i < str->len && str->ptr[i] == cmd->u.cut.delim[0]) i++;
				}
				isdelim = 1;
			}
		}

		if (isdelim)
		{
			sed->e.cutf.flds[x++].len = xl;

			if (x >= sed->e.cutf.cflds)
			{
				qse_cstr_t* tmp;
				qse_size_t nsz;

				nsz = sed->e.cutf.cflds;
				if (nsz > 50000) nsz += 50000;
				else nsz *= 2;
				
				if (sed->e.cutf.flds == sed->e.cutf.sflds)
				{
					tmp = QSE_MMGR_ALLOC (sed->mmgr, QSE_SIZEOF(*tmp) * nsz);
					if (tmp == QSE_NULL) 
					{
						SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
						return -1;
					}
					QSE_MEMCPY (tmp, sed->e.cutf.flds, QSE_SIZEOF(*tmp) * sed->e.cutf.cflds);
				}
				else
				{
					tmp = QSE_MMGR_REALLOC (sed->mmgr, sed->e.cutf.flds, QSE_SIZEOF(*tmp) * nsz);
					if (tmp == QSE_NULL) 
					{
						SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
						return -1;
					}
				}

				sed->e.cutf.flds = tmp;
				sed->e.cutf.cflds = nsz;
			}

			xl = 0;
			sed->e.cutf.flds[x].ptr = &str->ptr[i];

			/* mark that this line is delimited at least once */
			sed->e.cutf.delimited = 1; 
		}
		else xl++;
	}

	sed->e.cutf.flds[x].len = xl;
	sed->e.cutf.nflds = ++x;

	return 0;
}

static int do_cut (qse_sed_t* sed, qse_sed_cmd_t* cmd)
{
	qse_sed_cut_sel_t* b;
	const qse_char_t* lineterm;
	qse_cstr_t str;
	int out_state;

	qse_str_clear (&sed->e.txt.scratch);

	lineterm = trim_line (sed, &str);

	if (str.len <= 0) goto done;

	if (cmd->u.cut.fcount > 0) 
	{
	    if (split_into_fields_for_cut (sed, cmd, &str) <= -1) goto oops;

		if (cmd->u.cut.d && !sed->e.cutf.delimited) 
		{
			/* if the 'd' option is set and the line is not 
			 * delimited by the input delimiter, delete the pattern
			 * space and finish the current cycle */
			qse_str_clear (&sed->e.in.line);
			return 0;
		}
	}

	out_state = 0;

	for (b = cmd->u.cut.fb; b; b = b->next)
	{
		qse_size_t i, s, e;

		for (i = 0; i < b->len; i++)
		{
			if (b->range[i].id == QSE_SED_CUT_SEL_CHAR)
			{
				s = b->range[i].start;
				e = b->range[i].end;

				if (s <= e)
				{
					if (s < str.len)
					{
						if (e >= str.len) e = str.len - 1;
						if ((out_state == 2 && qse_str_ccat (&sed->e.txt.scratch, cmd->u.cut.delim[1]) == (qse_size_t)-1) ||
						    qse_str_ncat (&sed->e.txt.scratch, &str.ptr[s], e - s + 1) == (qse_size_t)-1)
						{
							SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
							goto oops;
						}

						out_state = 1;
					}
				}
				else
				{
					if (e < str.len)
					{
						if (s >= str.len) s = str.len - 1;
						if ((out_state == 2 && qse_str_ccat (&sed->e.txt.scratch, cmd->u.cut.delim[1]) == (qse_size_t)-1) ||
						    qse_str_nrcat (&sed->e.txt.scratch, &str.ptr[e], s - e + 1) == (qse_size_t)-1)
						{
							SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
							goto oops;
						}

						out_state = 1;
					}
				}
			}
			else /*if (b->range[i].id == QSE_SED_CUT_SEL_FIELD)*/
			{
				s = b->range[i].start;
				e = b->range[i].end;

				if (s <= e)
				{
					if (s < str.len)
					{
						if (e >= sed->e.cutf.nflds) e = sed->e.cutf.nflds - 1;

						while (s <= e)
						{	
							if ((out_state > 0 && qse_str_ccat (&sed->e.txt.scratch, cmd->u.cut.delim[1]) == (qse_size_t)-1) ||
							    qse_str_ncat (&sed->e.txt.scratch, sed->e.cutf.flds[s].ptr, sed->e.cutf.flds[s].len) == (qse_size_t)-1)
							{
								SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
								goto oops;
							}
							s++;

							out_state = 2;
						}
					}
				}
				else
				{
					if (e < str.len)
					{
						if (s >= sed->e.cutf.nflds) s = sed->e.cutf.nflds - 1;

						while (e <= s)
						{	
							if ((out_state > 0 && qse_str_ccat (&sed->e.txt.scratch, cmd->u.cut.delim[1]) == (qse_size_t)-1) ||
							    qse_str_ncat (&sed->e.txt.scratch, sed->e.cutf.flds[e].ptr, sed->e.cutf.flds[e].len) == (qse_size_t)-1)
							{
								SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
								goto oops;
							}
							e++;

							out_state = 2;
						}
					}
				}	
			}
		}
	}

done:
	if (lineterm)
	{
		if (qse_str_cat (&sed->e.txt.scratch, lineterm) == (qse_size_t)-1)
		{
			SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
			return -1;
		}
	}

	qse_str_swap (&sed->e.in.line, &sed->e.txt.scratch);
	return 1;

oops:
	return -1;
}

static int match_a (qse_sed_t* sed, qse_sed_cmd_t* cmd, qse_sed_adr_t* a)
{
	switch (a->type)
	{
		case QSE_SED_ADR_LINE:
			return (sed->e.in.num == a->u.lno)? 1: 0;

		case QSE_SED_ADR_REX:
		{
#if defined(USE_REX)
			int n;
			qse_rex_errnum_t errnum;
			qse_cstr_t match;
#endif
			qse_cstr_t line;
			void* rex;

			QSE_ASSERT (a->u.rex != QSE_NULL);

			line.ptr = QSE_STR_PTR(&sed->e.in.line);
			line.len = QSE_STR_LEN(&sed->e.in.line);

			if (line.len > 0 &&
			    line.ptr[line.len-1] == QSE_T('\n')) 
			{
				line.len--;
				if (line.len > 0 && line.ptr[line.len-1] == QSE_T('\r')) line.len--;
			}

			if (a->u.rex == EMPTY_REX)
			{
				rex = sed->e.last_rex;
				if (rex == QSE_NULL)
				{
					SETERR0 (sed, QSE_SED_ENPREX, &cmd->loc);
					return -1;
				}
			}
			else 
			{
				rex = a->u.rex;
				sed->e.last_rex = rex;
			}
#if defined(USE_REX)
			n = qse_matchrex (
				sed->mmgr, 
				sed->depth.rex.match,
				rex, 0,
				&line, &line,
				&match, &errnum);
			if (n <= -1)
			{
				SETERR0 (sed, QSE_SED_EREXMA, &cmd->loc);
				return -1;
			}			

			return n;
#else
			return matchtre (sed, rex, 0, &line, QSE_NULL, QSE_NULL, &cmd->loc);
#endif

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

		case QSE_SED_ADR_RELLINE:
			/* this address type should be seen only when matching 
			 * the second address */
			QSE_ASSERT (cmd->state.a1_matched && cmd->state.a1_match_line >= 1);
			return (sed->e.in.num >= cmd->state.a1_match_line + a->u.lno)? 1: 0;

		case QSE_SED_ADR_RELLINEM:
		{
			/* this address type should be seen only when matching 
			 * the second address */
			qse_size_t tmp;

			QSE_ASSERT (cmd->state.a1_matched && cmd->state.a1_match_line >= 1);
			QSE_ASSERT (a->u.lno > 0);

			/* TODO: is it better to store this value some in the state
			 *       not to calculate this every time?? */
			tmp = (cmd->state.a1_match_line + a->u.lno) - 
			      (cmd->state.a1_match_line % a->u.lno);

			return (sed->e.in.num >= tmp)? 1: 0;
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
		QSE_ASSERT (cmd->a2.u.lno > 0);
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
					/* This check is needed because matching of the second 
					 * address could be skipped while it could match.
					 * 
					 * Consider commands like '1,3p;2N'.
					 * '3' in '1,3p' is skipped because 'N' in '2N' triggers
					 * reading of the third line.
					 *
					 * Unfortunately, I can't handle a non-line-number
					 * second address like this. If 'abcxyz' is given as the third 
					 * line for command '1,/abc/p;2N', 'abcxyz' is not matched
					 * against '/abc/'. so it doesn't exit the range.
					 */
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
				cmd->state.a1_match_line = sed->e.in.num;
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
			jumpto = &sed->cmd.quit;
			break;

		case QSE_SED_CMD_QUIT_QUIET:
			jumpto = &sed->cmd.quit_quiet;
			break;

		case QSE_SED_CMD_APPEND:
			if (link_append (sed, cmd) <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_INSERT:
			n = write_str (sed,
				cmd->u.text.ptr,
				cmd->u.text.len
			);
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_CHANGE:
			if (cmd->state.c_ready)
			{
				/* change the pattern space */
				n = qse_str_ncpy (
					&sed->e.in.line,
					cmd->u.text.ptr,
					cmd->u.text.len
				);
				if (n == (qse_size_t)-1) 
				{
					SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
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
			if (nl) 
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
			if (sed->lformatter)
			{
				n = sed->lformatter (
					sed,
					QSE_STR_PTR(&sed->e.in.line),
					QSE_STR_LEN(&sed->e.in.line),
					write_char
				);
			}
			else {
				n = write_str_clearly (
					sed,
					QSE_STR_PTR(&sed->e.in.line),
					QSE_STR_LEN(&sed->e.in.line)
				);
			}
			if (n <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_HOLD:
			/* copy the pattern space to the hold space */
			if (qse_str_ncpy (&sed->e.txt.hold, 	
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line)) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
				return QSE_NULL;	
			}
			break;
				
		case QSE_SED_CMD_HOLD_APPEND:
			/* append the pattern space to the hold space */
			if (qse_str_ncat (&sed->e.txt.hold, 	
				QSE_STR_PTR(&sed->e.in.line),
				QSE_STR_LEN(&sed->e.in.line)) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
				return QSE_NULL;	
			}
			break;

		case QSE_SED_CMD_RELEASE:
			/* copy the hold space to the pattern space */
			if (qse_str_ncpy (&sed->e.in.line,
				QSE_STR_PTR(&sed->e.txt.hold),
				QSE_STR_LEN(&sed->e.txt.hold)) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
				return QSE_NULL;	
			}
			break;

		case QSE_SED_CMD_RELEASE_APPEND:
			/* append the hold space to the pattern space */
			if (qse_str_ncat (&sed->e.in.line,
				QSE_STR_PTR(&sed->e.txt.hold),
				QSE_STR_LEN(&sed->e.txt.hold)) == (qse_size_t)-1)
			{
				SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
				return QSE_NULL;	
			}
			break;

		case QSE_SED_CMD_EXCHANGE:
			/* exchange the pattern space and the hold space */
			qse_str_swap (&sed->e.in.line, &sed->e.txt.hold);
			break;

		case QSE_SED_CMD_NEXT:
			if (emit_output (sed, 0) <= -1) return QSE_NULL;

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
			if (emit_output (sed, 1) <= -1) return QSE_NULL;

			n = read_line (sed, 1);
			if (n <= -1) return QSE_NULL;
			if (n == 0)
			{
				/* EOF is reached. */
				jumpto = &sed->cmd.over;
			}
			break;
				
		case QSE_SED_CMD_READ_FILE:
			if (link_append (sed, cmd) <= -1) return QSE_NULL;
			break;

		case QSE_SED_CMD_READ_FILELN:
			if (link_append (sed, cmd) <= -1) return QSE_NULL;
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
			if (do_subst (sed, cmd) <= -1) return QSE_NULL;
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
			if (len > 0 && ptr[len-1] == QSE_T('\n')) 
			{
				len--;
				if (len > 0 && ptr[len-1] == QSE_T('\r')) len--;
			}

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

		case QSE_SED_CMD_CLEAR_PATTERN:
			/* clear pattern space */
			qse_str_clear (&sed->e.in.line);
			break;

		case QSE_SED_CMD_CUT:
			n = do_cut (sed, cmd);
			if (n <= -1) return QSE_NULL;
			if (n == 0) jumpto = &sed->cmd.over; /* finish the current cycle */
			break;
	}

	if (jumpto == QSE_NULL) jumpto = cmd->state.next;
	return jumpto;
} 

static void close_outfile (qse_map_t* map, void* dptr, qse_size_t dlen)
{
	qse_sed_io_arg_t* arg = dptr;
	QSE_ASSERT (dlen == QSE_SIZEOF(*arg));

	if (arg->handle)
	{
		qse_sed_t* sed = *(qse_sed_t**)QSE_XTN(map);
		sed->e.out.fun (sed, QSE_SED_IO_CLOSE, arg, QSE_NULL, 0);
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

		if (sed->option & QSE_SED_EXTENDEDADR)
		{
			if (c->a2.type == QSE_SED_ADR_REX &&
			    c->a1.type == QSE_SED_ADR_LINE &&
			    c->a1.u.lno <= 0)
			{
				/* special handling for 0,/regex/ */
				c->state.a1_matched = 1;
				c->state.a1_match_line = 0;
			}
		}

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
						lab->ptr, lab->len, &c->loc
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
			         c->u.subst.file.ptr)
			{
				file = &c->u.subst.file;
			}
		
			if (file)
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

static int emit_output (qse_sed_t* sed, int skipline)
{
	int n;

	if (!skipline && !(sed->option & QSE_SED_QUIET))
	{
		/* write the pattern space */
		n = write_str (sed, 
			QSE_STR_PTR(&sed->e.in.line),
			QSE_STR_LEN(&sed->e.in.line));
		if (n <= -1) return -1;
	}

	if (emit_appends (sed) <= -1) return -1;
	free_appends (sed);

	/* flush the output stream in case it's not flushed 
	 * in write functions */
	n = flush (sed);
	if (n <= -1) return -1;

	return 0;
}


int qse_sed_exec (qse_sed_t* sed, qse_sed_io_fun_t inf, qse_sed_io_fun_t outf)
{
	qse_ssize_t n;
	int ret = 0;

	static qse_map_mancbs_t mancbs =
	{
		{
			QSE_MAP_COPIER_INLINE,
			QSE_MAP_COPIER_INLINE
		},
		{
			QSE_MAP_FREEER_DEFAULT,
			close_outfile
		},
		QSE_MAP_COMPER_DEFAULT,
		QSE_MAP_KEEPER_DEFAULT
#ifdef QSE_MAP_AS_HTB
		,
		QSE_MAP_SIZER_DEFAULT,
		QSE_MAP_HASHER_DEFAULT
#endif
	};

	sed->e.stopreq = 0;
	sed->e.last_rex = QSE_NULL;

	sed->e.subst_done = 0;

	free_appends (sed);
	qse_str_clear (&sed->e.txt.scratch);
	qse_str_clear (&sed->e.txt.hold);
	if (qse_str_ccat (&sed->e.txt.hold, QSE_T('\n')) == (qse_size_t)-1)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
		return -1;
	}

	sed->e.out.fun = outf;
	sed->e.out.eof = 0;
	sed->e.out.len = 0;
	if (qse_map_init (
		&sed->e.out.files, sed->mmgr, 
		128, 70, QSE_SIZEOF(qse_char_t), 1) <= -1)
	{
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
		return -1;
	}
	*(qse_sed_t**)QSE_XTN(&sed->e.out.files) = sed;
	qse_map_setmancbs (&sed->e.out.files, &mancbs);

	sed->e.in.fun = inf;
	sed->e.in.eof = 0;
	sed->e.in.len = 0;
	sed->e.in.pos = 0;
	sed->e.in.num = 0;
	if (qse_str_init (&sed->e.in.line, QSE_MMGR(sed), 256) <= -1)
	{
		qse_map_fini (&sed->e.out.files);
		SETERR0 (sed, QSE_SED_ENOMEM, QSE_NULL);
		return -1;
	}

	sed->errnum = QSE_SED_ENOERR;
	sed->e.in.arg.path = QSE_NULL;
	n = sed->e.in.fun (sed, QSE_SED_IO_OPEN, &sed->e.in.arg, QSE_NULL, 0);
	if (n <= -1)
	{
		ret = -1;
		if (sed->errnum == QSE_SED_ENOERR) 
			SETERR0 (sed, QSE_SED_EIOUSR, QSE_NULL);
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
	n = sed->e.out.fun (sed, QSE_SED_IO_OPEN, &sed->e.out.arg, QSE_NULL, 0);
	if (n <= -1)
	{
		ret = -1;
		if (sed->errnum == QSE_SED_ENOERR) 
			SETERR0 (sed, QSE_SED_EIOUSR, QSE_NULL);
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

	while (!sed->e.stopreq)
	{
#ifdef QSE_ENABLE_SEDTRACER
		if (sed->e.tracer) sed->e.tracer (sed, QSE_SED_EXEC_READ, QSE_NULL);
#endif

		n = read_line (sed, 0);
		if (n <= -1) { ret = -1; goto done; }
		if (n == 0) goto done;

		if (sed->cmd.fb.len > 0)
		{
			/* the first command block contains at least 1 command
			 * to execute. an empty script like ' ' has no commands,
			 * so we execute no commands */

			qse_sed_cmd_t* c, * j;

		again:
			c = &sed->cmd.fb.buf[0];

			while (c != &sed->cmd.over)
			{
#ifdef QSE_ENABLE_SEDTRACER
				if (sed->e.tracer) sed->e.tracer (sed, QSE_SED_EXEC_MATCH, c);
#endif

				n = match_address (sed, c);
				if (n <= -1) { ret = -1; goto done; }
	
				if (c->negated) n = !n;
				if (n == 0)
				{
					c = c->state.next;
					continue;
				}

#ifdef QSE_ENABLE_SEDTRACER
				if (sed->e.tracer) sed->e.tracer (sed, QSE_SED_EXEC_EXEC, c);
#endif
				j = exec_cmd (sed, c);
				if (j == QSE_NULL) { ret = -1; goto done; }
				if (j == &sed->cmd.quit_quiet) goto done;
				if (j == &sed->cmd.quit) 
				{ 
					if (emit_output (sed, 0) <= -1) ret = -1;
					goto done;
				}
				if (sed->e.stopreq) goto done;
				if (j == &sed->cmd.again) goto again;

				/* go to the next command */
				c = j;
			}
		}

#ifdef QSE_ENABLE_SEDTRACER
		if (sed->e.tracer) sed->e.tracer (sed, QSE_SED_EXEC_WRITE, QSE_NULL);
#endif
		if (emit_output (sed, 0) <= -1) { ret = -1; goto done; }
	}

done:
	qse_map_clear (&sed->e.out.files);
	sed->e.out.fun (sed, QSE_SED_IO_CLOSE, &sed->e.out.arg, QSE_NULL, 0);
done2:
	sed->e.in.fun (sed, QSE_SED_IO_CLOSE, &sed->e.in.arg, QSE_NULL, 0);
done3:
	qse_str_fini (&sed->e.in.line);
	qse_map_fini (&sed->e.out.files);
	return ret;
}

void qse_sed_stop (qse_sed_t* sed)
{
	sed->e.stopreq = 1;
}

int qse_sed_isstop (qse_sed_t* sed)
{
	return sed->e.stopreq;
}

qse_sed_lformatter_t qse_sed_getlformatter (qse_sed_t* sed)
{
	return sed->lformatter;
}

void qse_sed_setlformatter (qse_sed_t* sed, qse_sed_lformatter_t lf)
{
	sed->lformatter = lf;
}

const qse_char_t* qse_sed_getcompid (qse_sed_t* sed)
{
	return sed->src.cid? ((const qse_char_t*)(sed->src.cid + 1)): QSE_NULL;
}

const qse_char_t* qse_sed_setcompid (qse_sed_t* sed, const qse_char_t* id)
{
	qse_sed_cid_t* cid;
	qse_size_t len;
	
	if (sed->src.cid == (qse_sed_cid_t*)&sed->src.unknown_cid) 
	{
		/* if an error has occurred in a previously, you can't set it
		 * any more */
		return (const qse_char_t*)(sed->src.cid + 1);
	}

	if (id == QSE_NULL) id = QSE_T("");

	len = qse_strlen (id);
	cid = QSE_MMGR_ALLOC (sed->mmgr, 
		QSE_SIZEOF(*cid) + ((len + 1) * QSE_SIZEOF(*id)));
	if (cid == QSE_NULL) 
	{
		/* mark that an error has occurred */ 
		sed->src.unknown_cid.buf[0] = QSE_T('\0');
		cid = (qse_sed_cid_t*)&sed->src.unknown_cid;
	}
	else
	{
		qse_strcpy ((qse_char_t*)(cid + 1), id);
	}

	cid->next = sed->src.cid;
	sed->src.cid = cid;
	return (const qse_char_t*)(cid + 1);
}

qse_size_t qse_sed_getlinenum (qse_sed_t* sed)
{
	return sed->e.in.num;
}

void qse_sed_setlinenum (qse_sed_t* sed, qse_size_t num)
{
	sed->e.in.num = num;
}

qse_sed_ecb_t* qse_sed_popecb (qse_sed_t* sed)
{
	qse_sed_ecb_t* top = sed->ecb;
	if (top) sed->ecb = top->next;
	return top;
}

void qse_sed_pushecb (qse_sed_t* sed, qse_sed_ecb_t* ecb)
{
	ecb->next = sed->ecb;
	sed->ecb = ecb;
}

#ifdef QSE_ENABLE_SEDTRACER
qse_sed_exec_tracer_t qse_sed_getexectracer (qse_sed_t* sed)
{
	return sed->e.tracer;
}

void qse_sed_setexectracer (qse_sed_t* sed, qse_sed_exec_tracer_t tracer)
{
	sed->e.tracer = tracer;
}
#endif

