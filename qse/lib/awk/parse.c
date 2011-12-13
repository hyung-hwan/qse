/*
 * $Id: parse.c 554 2011-08-22 05:26:26Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include "awk.h"

enum tok_t
{
	TOK_EOF,
	TOK_NEWLINE,

	/* special token to direct the parser to include a file specified */
	TOK_INCLUDE,

	/* TOK_XXX_ASSNs should in sync 
	 * with assop in assign_to_opcode */
	TOK_ASSN,
	TOK_PLUS_ASSN,
	TOK_MINUS_ASSN,
	TOK_MUL_ASSN,
	TOK_DIV_ASSN,
	TOK_IDIV_ASSN,
	TOK_MOD_ASSN,
	TOK_EXP_ASSN,
	TOK_CONCAT_ASSN,
	TOK_RS_ASSN,
	TOK_LS_ASSN,
	TOK_BAND_ASSN,
	TOK_BXOR_ASSN,
	TOK_BOR_ASSN,

	TOK_EQ,
	TOK_NE,
	TOK_LE,
	TOK_LT,
	TOK_GE,
	TOK_GT,
	TOK_NM,   /* not match */
	TOK_LNOT, /* logical negation ! */
	TOK_PLUS,
	TOK_PLUSPLUS,
	TOK_MINUS,
	TOK_MINUSMINUS,
	TOK_MUL,
	TOK_DIV,
	TOK_IDIV,
	TOK_MOD,
	TOK_LOR,
	TOK_LAND,
	TOK_BOR,
	TOK_BXOR,
	TOK_BAND,
	TOK_TILDE, /* used for unary bitwise-not and regex match */
	TOK_RS,
	TOK_LS,
	TOK_IN,
	TOK_EXP,
	TOK_CONCAT,

	TOK_LPAREN,
	TOK_RPAREN,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_LBRACK,
	TOK_RBRACK,

	TOK_DOLLAR,
	TOK_COMMA,
	TOK_SEMICOLON,
	TOK_COLON,
	TOK_QUEST,
	TOK_ATSIGN,

	TOK_BEGIN,
	TOK_END,
	TOK_FUNCTION,

	TOK_LOCAL,
	TOK_GLOBAL,

	TOK_IF,
	TOK_ELSE,
	TOK_WHILE,
	TOK_FOR,
	TOK_DO,
	TOK_BREAK,
	TOK_CONTINUE,
	TOK_RETURN,
	TOK_EXIT,
	TOK_NEXT,
	TOK_NEXTFILE,
	TOK_NEXTOFILE,
	TOK_DELETE,
	TOK_RESET,
	TOK_PRINT,
	TOK_PRINTF,

	TOK_GETLINE,
	TOK_IDENT,
	TOK_INT,
	TOK_FLT,
	TOK_STR,
	TOK_REX,

	__TOKEN_COUNT__
};

enum
{
	PARSE_GBL,
	PARSE_FUNCTION,
	PARSE_BEGIN,
	PARSE_END,
	PARSE_BEGIN_BLOCK,
	PARSE_END_BLOCK,
	PARSE_PATTERN,
	PARSE_ACTION_BLOCK
};

enum
{
	PARSE_LOOP_NONE,
	PARSE_LOOP_WHILE,
	PARSE_LOOP_FOR,
	PARSE_LOOP_DOWHILE
};

typedef struct binmap_t binmap_t;

struct binmap_t
{
	int token;
	int binop;
};

static qse_awk_t* parse_progunit (qse_awk_t* awk);
static qse_awk_t* collect_globals (qse_awk_t* awk);
static void adjust_static_globals (qse_awk_t* awk);
static qse_size_t find_global (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len);
static qse_awk_t* collect_locals (
	qse_awk_t* awk, qse_size_t nlcls, qse_bool_t istop);

static qse_awk_nde_t* parse_function (qse_awk_t* awk);
static qse_awk_nde_t* parse_begin (qse_awk_t* awk);
static qse_awk_nde_t* parse_end (qse_awk_t* awk);
static qse_awk_chain_t* parse_action_block (
	qse_awk_t* awk, qse_awk_nde_t* ptn, qse_bool_t blockless);

static qse_awk_nde_t* parse_block_dc (
	qse_awk_t* awk, const qse_awk_loc_t* xloc, qse_bool_t istop);

static qse_awk_nde_t* parse_statement (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);

static qse_awk_nde_t* parse_expr_dc (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);

static qse_awk_nde_t* parse_logical_or (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_logical_and (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_in (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_regex_match (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_bitwise_or (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_bitwise_xor (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_bitwise_and (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_equality (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_relational (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_shift (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_concat (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_additive (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_multiplicative (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);

static qse_awk_nde_t* parse_unary (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_exponent (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_unary_exp (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_increment (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_primary (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_primary_ident (
	qse_awk_t* awk, const qse_awk_loc_t* xloc);

static qse_awk_nde_t* parse_hashidx (
	qse_awk_t* awk, qse_char_t* name, qse_size_t namelen, 
	const qse_awk_loc_t* xloc);
static qse_awk_nde_t* parse_fncall (
	qse_awk_t* awk, qse_char_t* name, qse_size_t namelen, 
	qse_awk_fnc_t* fnc, const qse_awk_loc_t* xloc, int noarg);

static int get_token (qse_awk_t* awk);
static int preget_token (qse_awk_t* awk);
static int get_rexstr (qse_awk_t* awk, qse_awk_tok_t* tok);

static int skip_spaces (qse_awk_t* awk);
static int skip_comment (qse_awk_t* awk);
static int classify_ident (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len);

static int deparse (qse_awk_t* awk);
static qse_htb_walk_t deparse_func (
	qse_htb_t* map, qse_htb_pair_t* pair, void* arg);
static int put_char (qse_awk_t* awk, qse_char_t c);
static int flush_out (qse_awk_t* awk);

typedef struct kwent_t kwent_t;

struct kwent_t 
{ 
	qse_cstr_t name;
	int type; 
	int valid; /* the entry is valid when this option is set */
};

static kwent_t kwtab[] = 
{
	/* keep this table in sync with the kw_t enums in <parse.h>.
	 * also keep it sorted by the first field for binary search */
	{ { QSE_T("BEGIN"),        5 }, TOK_BEGIN,       QSE_AWK_PABLOCK },
	{ { QSE_T("END"),          3 }, TOK_END,         QSE_AWK_PABLOCK },
	{ { QSE_T("break"),        5 }, TOK_BREAK,       0 },
	{ { QSE_T("continue"),     8 }, TOK_CONTINUE,    0 },
	{ { QSE_T("delete"),       6 }, TOK_DELETE,      0 },
	{ { QSE_T("do"),           2 }, TOK_DO,          0 },
	{ { QSE_T("else"),         4 }, TOK_ELSE,        0 },
	{ { QSE_T("exit"),         4 }, TOK_EXIT,        0 },
	{ { QSE_T("for"),          3 }, TOK_FOR,         0 },
	{ { QSE_T("function"),     8 }, TOK_FUNCTION,    0 },
	{ { QSE_T("getline"),      7 }, TOK_GETLINE,     QSE_AWK_RIO },
	{ { QSE_T("global"),       6 }, TOK_GLOBAL,      QSE_AWK_EXPLICIT },
	{ { QSE_T("if"),           2 }, TOK_IF,          0 },
	{ { QSE_T("in"),           2 }, TOK_IN,          0 },
	{ { QSE_T("include"),      7 }, TOK_INCLUDE,     QSE_AWK_INCLUDE },
	{ { QSE_T("local"),        5 }, TOK_LOCAL,       QSE_AWK_EXPLICIT },
	{ { QSE_T("next"),         4 }, TOK_NEXT,        QSE_AWK_PABLOCK },
	{ { QSE_T("nextfile"),     8 }, TOK_NEXTFILE,    QSE_AWK_PABLOCK },
	{ { QSE_T("nextofile"),    9 }, TOK_NEXTOFILE,   QSE_AWK_PABLOCK | QSE_AWK_NEXTOFILE },
	{ { QSE_T("print"),        5 }, TOK_PRINT,       QSE_AWK_RIO },
	{ { QSE_T("printf"),       6 }, TOK_PRINTF,      QSE_AWK_RIO },
	{ { QSE_T("reset"),        5 }, TOK_RESET,       QSE_AWK_RESET },
	{ { QSE_T("return"),       6 }, TOK_RETURN,      0 },
	{ { QSE_T("while"),        5 }, TOK_WHILE,       0 }
};

typedef struct global_t global_t;

struct global_t
{
	const qse_char_t* name;
	qse_size_t namelen;
	int valid;
};

static global_t gtab[] =
{
	{ QSE_T("ARGC"),         4,  0 },
	{ QSE_T("ARGV"),         4,  0 },

	/* output real-to-str conversion format for other cases than 'print' */
	{ QSE_T("CONVFMT"),      7,  0 },

	/* current input file name */
	{ QSE_T("FILENAME"),     8,  QSE_AWK_PABLOCK },

	/* input record number in current file */
	{ QSE_T("FNR"),          3,  QSE_AWK_PABLOCK },

	/* input field separator */
	{ QSE_T("FS"),           2,  0 },

	/* ignore case in string comparison */
	{ QSE_T("IGNORECASE"),  10,  0 },

	/* number of fields in current input record 
	 * NF is also updated if you assign a value to $0. so it is not
	 * associated with QSE_AWK_PABLOCK */
	{ QSE_T("NF"),           2,  0 },

	/* input record number */
	{ QSE_T("NR"),           2,  QSE_AWK_PABLOCK },

	/* current output file name */
	{ QSE_T("OFILENAME"),    9,  QSE_AWK_PABLOCK | QSE_AWK_NEXTOFILE },

	/* output real-to-str conversion format for 'print' */
	{ QSE_T("OFMT"),         4,  QSE_AWK_RIO}, 

	/* output field separator for 'print' */
	{ QSE_T("OFS"),          3,  QSE_AWK_RIO },

	/* output record separator. used for 'print' and blockless output */
	{ QSE_T("ORS"),          3,  QSE_AWK_RIO },

	{ QSE_T("RLENGTH"),      7,  0 },
	{ QSE_T("RS"),           2,  0 },

	{ QSE_T("RSTART"),       6,  0 },
	{ QSE_T("SUBSEP"),       6,  0 }
};

#define GET_CHAR(awk) \
	do { if (get_char(awk) <= -1) return -1; } while(0)

#define GET_CHAR_TO(awk,c) \
	do { \
		if (get_char(awk) <= -1) return -1; \
		c = (awk)->sio.last.c; \
	} while(0)

#define SET_TOKEN_TYPE(awk,tok,code) \
	do { (tok)->type = (code); } while (0)

#define ADD_TOKEN_CHAR(awk,tok,c) \
	do { \
		if (qse_str_ccat((tok)->name,(c)) == (qse_size_t)-1) \
		{ \
			qse_awk_seterror (awk, QSE_AWK_ENOMEM, QSE_NULL, &(tok)->loc); \
			return -1; \
		} \
	} while (0)

#define ADD_TOKEN_STR(awk,tok,s,l) \
	do { \
		if (qse_str_ncat((tok)->name,(s),(l)) == (qse_size_t)-1) \
		{ \
			qse_awk_seterror (awk, QSE_AWK_ENOMEM, QSE_NULL, &(tok)->loc); \
			return -1; \
		} \
	} while (0)

#define MATCH(awk,tok_type) ((awk)->tok.type == (tok_type))

#define MATCH_TERMINATOR_NORMAL(awk) \
	(MATCH((awk),TOK_SEMICOLON) || MATCH((awk),TOK_NEWLINE))

#define MATCH_TERMINATOR_RBRACE(awk) \
	((awk->option & QSE_AWK_NEWLINE) && MATCH((awk),TOK_RBRACE))

#define MATCH_TERMINATOR(awk) \
	(MATCH_TERMINATOR_NORMAL(awk) || MATCH_TERMINATOR_RBRACE(awk))

#define ISNOERR(awk) ((awk)->errinf.num == QSE_AWK_ENOERR)

#define CLRERR(awk) \
	qse_awk_seterror (awk, QSE_AWK_ENOERR, QSE_NULL, QSE_NULL)

#define SETERR_TOK(awk,code) \
	do { \
		qse_cstr_t __ea; \
		__ea.len = QSE_STR_LEN((awk)->tok.name); \
		__ea.ptr = QSE_STR_PTR((awk)->tok.name); \
		qse_awk_seterror (awk, code, &__ea, &(awk)->tok.loc); \
	} while (0)

#define SETERR_COD(awk,code) \
	qse_awk_seterror (awk, code, QSE_NULL, QSE_NULL)

#define SETERR_LOC(awk,code,loc) \
	qse_awk_seterror (awk, code, QSE_NULL, loc)

#define SETERR_ARG_LOC(awk,code,ep,el,loc) \
	do { \
		qse_cstr_t __ea; \
		__ea.len = (el); __ea.ptr = (ep); \
		qse_awk_seterror ((awk), (code), &__ea, (loc)); \
	} while (0)

#define SETERR_ARG(awk,code,ep,el) SETERR_ARG_LOC(awk,code,ep,el,QSE_NULL)

static QSE_INLINE int is_plain_var (qse_awk_nde_t* nde)
{
	return nde->type == QSE_AWK_NDE_GBL ||
	       nde->type == QSE_AWK_NDE_LCL ||
	       nde->type == QSE_AWK_NDE_ARG ||
	       nde->type == QSE_AWK_NDE_NAMED;
}

static QSE_INLINE int is_var (qse_awk_nde_t* nde)
{
	return nde->type == QSE_AWK_NDE_GBL ||
	       nde->type == QSE_AWK_NDE_LCL ||
	       nde->type == QSE_AWK_NDE_ARG ||
	       nde->type == QSE_AWK_NDE_NAMED ||
	       nde->type == QSE_AWK_NDE_GBLIDX ||
	       nde->type == QSE_AWK_NDE_LCLIDX ||
	       nde->type == QSE_AWK_NDE_ARGIDX ||
	       nde->type == QSE_AWK_NDE_NAMEDIDX;
}

static int get_char (qse_awk_t* awk)
{
	qse_ssize_t n;

	if (awk->sio.nungots > 0) 
	{
		/* there are something in the unget buffer */
		awk->sio.last = awk->sio.ungot[--awk->sio.nungots];
		return 0;
	}

	if (awk->sio.inp->b.pos >= awk->sio.inp->b.len)
	{
		CLRERR (awk);
		n = awk->sio.inf (
			awk, QSE_AWK_SIO_READ, awk->sio.inp,
			awk->sio.inp->b.buf, QSE_COUNTOF(awk->sio.inp->b.buf)
		);
		if (n <= -1)
		{
			if (ISNOERR(awk))
				SETERR_ARG (awk, QSE_AWK_EREAD, QSE_T("<SIN>"), 5);
			return -1;
		}

		if (n == 0)
		{
			awk->sio.last.c = QSE_CHAR_EOF;
			awk->sio.last.line = awk->sio.inp->line;
			awk->sio.last.colm = awk->sio.inp->colm;
			awk->sio.last.file = awk->sio.inp->name;
			return 0;
		}

		awk->sio.inp->b.pos = 0;
		awk->sio.inp->b.len = n;	
	}

	if (awk->sio.inp->last.c == QSE_T('\n'))
	{
		/* if the previous charater was a newline,
		 * increment the line counter and reset column to 1.
		 * incrementing it line number here instead of
		 * updating inp->last causes the line number for
		 * TOK_EOF to be the same line as the last newline. */
		awk->sio.inp->line++;
		awk->sio.inp->colm = 1;
	}
	
	awk->sio.inp->last.c = awk->sio.inp->b.buf[awk->sio.inp->b.pos++];
	awk->sio.inp->last.line = awk->sio.inp->line;
	awk->sio.inp->last.colm = awk->sio.inp->colm++;
	awk->sio.inp->last.file = awk->sio.inp->name;

	awk->sio.last = awk->sio.inp->last;
	return 0;
}

static void unget_char (qse_awk_t* awk, const qse_awk_sio_lxc_t* c)
{
	/* Make sure that the unget buffer is large enough */
	QSE_ASSERTX (awk->sio.nungots < QSE_COUNTOF(awk->sio.ungot), 
		"Make sure that you have increased the size of sio.ungot large enough");
	awk->sio.ungot[awk->sio.nungots++] = *c;
}

const qse_char_t* qse_awk_getgblname (
	qse_awk_t* awk, qse_size_t idx, qse_size_t* len)
{
	QSE_ASSERT (idx < QSE_LDA_SIZE(awk->parse.gbls));

	*len = QSE_LDA_DLEN(awk->parse.gbls,idx);
	return QSE_LDA_DPTR(awk->parse.gbls,idx);
}

void qse_awk_getkwname (qse_awk_t* awk, qse_awk_kwid_t id, qse_cstr_t* s)
{
	*s = kwtab[id].name;
}

static int parse (qse_awk_t* awk)
{
	int ret = -1; 
	qse_ssize_t op;

	QSE_ASSERT (awk->sio.inf != QSE_NULL);

	CLRERR (awk);
	op = awk->sio.inf (awk, QSE_AWK_SIO_OPEN, awk->sio.inp, QSE_NULL, 0);
	if (op <= -1)
	{
		/* cannot open the source file.
		 * it doesn't even have to call CLOSE */
		if (ISNOERR(awk)) 
			SETERR_ARG (awk, QSE_AWK_EOPEN, QSE_T("<SIN>"), 5);
		return -1;
	}

	adjust_static_globals (awk);

	/* the user io handler for the source code input returns 0 when
	 * it doesn't have any files to open. this is the same condition
	 * as the source code file is empty. so it will perform the parsing
	 * when op is positive, which means there are something to parse */
	if (op > 0)
	{
		/* get the first character */
		if (get_char(awk) <= -1) goto oops; 
		/* get the first token */
		if (get_token(awk) <= -1) goto oops;

		while (1) 
		{
			while (MATCH(awk,TOK_NEWLINE)) 
			{
				if (get_token(awk) <= -1) goto oops;
			}
			if (MATCH(awk,TOK_EOF)) break;

			if (parse_progunit(awk) == QSE_NULL) goto oops;
		}

		if ((awk->option & QSE_AWK_EXPLICIT) &&
		    !(awk->option & QSE_AWK_IMPLICIT))
		{
			/* ensure that all functions called are defined 
			 * in the EXPLICIT-only mode */

			qse_htb_pair_t* p;
			qse_size_t buckno;

			p = qse_htb_getfirstpair (awk->parse.funs, &buckno);
			while (p != QSE_NULL)
			{
				if (qse_htb_search (awk->tree.funs, 
					QSE_HTB_KPTR(p), QSE_HTB_KLEN(p)) == QSE_NULL)
				{
					
					qse_awk_nde_t* nde;

					/* see parse_fncall() for what is
					 * stored into awk->tree.funs */
					nde = (qse_awk_nde_t*)QSE_HTB_VPTR(p);

					SETERR_ARG_LOC (
						awk, 
						QSE_AWK_EFUNNF, 
						QSE_HTB_KPTR(p),
						QSE_HTB_KLEN(p),
						&nde->loc
					);

					goto oops;
				}

				p = qse_htb_getnextpair (awk->parse.funs, p, &buckno);
			}
		}
	}

	QSE_ASSERT (awk->tree.ngbls == QSE_LDA_SIZE(awk->parse.gbls));
	ret = 0;

oops:
	if (ret <= -1)
	{
		/* an error occurred and control has reached here
		 * probably, some included files might not have beed 
		 * closed. close them */
		while (awk->sio.inp != &awk->sio.arg)
		{
			qse_awk_sio_arg_t* next;

			/* nothing much to do about a close error */
			awk->sio.inf (
				awk, QSE_AWK_SIO_CLOSE, 
				awk->sio.inp, QSE_NULL, 0);

			next = awk->sio.inp->next;

			QSE_ASSERT (awk->sio.inp->name != QSE_NULL);
			QSE_MMGR_FREE (awk->mmgr, awk->sio.inp);

			awk->sio.inp = next;
		}
	}
	else if (ret == 0) 
	{
		/* no error occurred so far */
		QSE_ASSERT (awk->sio.inp == &awk->sio.arg);
		CLRERR (awk);
	}

	if (awk->sio.inf (
		awk, QSE_AWK_SIO_CLOSE, awk->sio.inp, QSE_NULL, 0) != 0)
	{
		if (ret == 0)
		{
			/* this is to keep the earlier error above
			 * that might be more critical than this */
			if (ISNOERR(awk)) 
				SETERR_ARG (awk, QSE_AWK_ECLOSE, QSE_T("<SIN>"), 5);
			ret = -1;
		}
	}

	if (ret <= -1) 
	{
		/* clear the parse tree partially constructed on error */
		qse_awk_clear (awk);
	}

	return ret;
}

int qse_awk_parse (qse_awk_t* awk, qse_awk_sio_t* sio)
{
	int n;

	QSE_ASSERTX (sio != QSE_NULL ,
		"the source code istream must be provided");
	QSE_ASSERTX (sio->in != QSE_NULL,
		"the source code input stream must be provided at least");
	if (sio == QSE_NULL || sio->in == QSE_NULL)
	{
		SETERR_COD (awk, QSE_AWK_EINVAL);
		return -1;
	}

	QSE_ASSERT (awk->parse.depth.cur.loop == 0);
	QSE_ASSERT (awk->parse.depth.cur.expr == 0);

	qse_awk_clear (awk);
	qse_htb_clear (awk->sio.names);
	awk->sio.inf = sio->in;
	awk->sio.outf = sio->out;

	n = parse (awk);
	if (n == 0  && awk->sio.outf != QSE_NULL) n = deparse (awk);

	QSE_ASSERT (awk->parse.depth.cur.loop == 0);
	QSE_ASSERT (awk->parse.depth.cur.expr == 0);

	return n;
}

static int begin_include (qse_awk_t* awk)
{
	qse_ssize_t op;
	qse_awk_sio_arg_t* arg = QSE_NULL;
	qse_htb_pair_t* pair = QSE_NULL;

	if (qse_strlen(QSE_STR_PTR(awk->tok.name)) != QSE_STR_LEN(awk->tok.name))
	{
		/* a '\0' character included in the include file name.
		 * we don't support such a file name */
		SETERR_ARG_LOC (
			awk, 
			QSE_AWK_EIONMNL,
			QSE_STR_PTR(awk->tok.name),
			qse_strlen(QSE_STR_PTR(awk->tok.name)),
			&awk->tok.loc
		);
		return -1;
	}

	/* store the file name to awk->sio.names */
	pair = qse_htb_ensert (
		awk->sio.names, 
		QSE_STR_PTR(awk->tok.name),
		QSE_STR_LEN(awk->tok.name) + 1, /* to include '\0' */
		QSE_NULL, 0
	);
	if (pair == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, &awk->ptok.loc);
		goto oops;
	}

	/*QSE_HTB_VPTR(pair) = QSE_HTB_KPTR(pair);
	QSE_HTB_VLEN(pair) = QSE_HTB_KLEN(pair);*/

	arg = (qse_awk_sio_arg_t*) QSE_MMGR_ALLOC (awk->mmgr, QSE_SIZEOF(*arg));
	if (arg == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, &awk->ptok.loc);
		goto oops;
	}

	QSE_MEMSET (arg, 0, QSE_SIZEOF(*arg));
	arg->name = QSE_HTB_KPTR(pair);

	CLRERR (awk);
	op = awk->sio.inf (awk, QSE_AWK_SIO_OPEN, arg, QSE_NULL, 0);
	if (op <= -1)
	{
		if (ISNOERR(awk)) SETERR_TOK (awk, QSE_AWK_EOPEN);
		else awk->errinf.loc = awk->tok.loc; /* adjust error location */
		goto oops;
	}

	if (op == 0)
	{
		CLRERR (awk);
		op = awk->sio.inf (awk, QSE_AWK_SIO_CLOSE, arg, QSE_NULL, 0);
		if (op != 0)
		{
			if (ISNOERR(awk)) SETERR_TOK (awk, QSE_AWK_ECLOSE);
			else awk->errinf.loc = awk->tok.loc;
			goto oops;
		}
	}

	arg->next = awk->sio.inp;
	awk->sio.inp = arg;
	awk->parse.depth.cur.incl++;

	awk->sio.inp->line = 1;
	awk->sio.inp->colm = 1;

	return 0;

oops:
	if (arg != QSE_NULL) QSE_MMGR_FREE (awk->mmgr, arg);
	return -1;
}

static int end_include (qse_awk_t* awk)
{
	int x;
	qse_awk_sio_arg_t* cur;

	if (awk->sio.inp == &awk->sio.arg) return 0; /* no include */

	/* if it is an included file, close it and
	 * retry to read a character from an outer file */

	CLRERR (awk);
	x = awk->sio.inf (
		awk, QSE_AWK_SIO_CLOSE, 
		awk->sio.inp, QSE_NULL, 0);

	/* if closing has failed, still destroy the
	 * sio structure first as normal and return
	 * the failure below. this way, the caller 
	 * does not call QSE_AWK_SIO_CLOSE on 
	 * awk->sio.inp again. */

	cur = awk->sio.inp;
	awk->sio.inp = awk->sio.inp->next;

	QSE_ASSERT (cur->name != QSE_NULL);
	QSE_MMGR_FREE (awk->mmgr, cur);
	awk->parse.depth.cur.incl--;

	if (x != 0)
	{
		/* the failure mentioned above is returned here */
		if (ISNOERR(awk))
			SETERR_ARG (awk, QSE_AWK_ECLOSE, QSE_T("<SIN>"), 5);
		return -1;
	}

	return 1; /* ended the included file successfully */
}

static qse_awk_t* parse_progunit (qse_awk_t* awk)
{
	/*
	@include "xxxx"
	global xxx, xxxx;
	BEGIN { action }
	END { action }
	pattern { action }
	function name (parameter-list) { statement }
	*/

	QSE_ASSERT (awk->parse.depth.cur.loop == 0);

retry:
	if ((awk->option & QSE_AWK_EXPLICIT) && MATCH(awk,TOK_GLOBAL)) 
	{
		qse_size_t ngbls;

		awk->parse.id.block = PARSE_GBL;

		if (get_token(awk) <= -1) return QSE_NULL;

		QSE_ASSERT (awk->tree.ngbls == QSE_LDA_SIZE(awk->parse.gbls));
		ngbls = awk->tree.ngbls;
		if (collect_globals (awk) == QSE_NULL) 
		{
			qse_lda_delete (
				awk->parse.gbls, ngbls, 
				QSE_LDA_SIZE(awk->parse.gbls) - ngbls);
			awk->tree.ngbls = ngbls;
			return QSE_NULL;
		}
	}
	else if (MATCH(awk,TOK_ATSIGN))
	{
		if (get_token(awk) <= -1) return QSE_NULL;

		if (MATCH(awk,TOK_INCLUDE))
		{
			if (awk->parse.depth.max.incl > 0 &&
			    awk->parse.depth.cur.incl >=  awk->parse.depth.max.incl)
			{
				SETERR_LOC (
					awk, QSE_AWK_EINCLTD, &awk->ptok.loc);
				return QSE_NULL;
			}

			if (get_token(awk) <= -1) return QSE_NULL;

			if (!MATCH(awk,TOK_STR))
			{
				SETERR_LOC (
					awk, QSE_AWK_EINCLSTR, &awk->ptok.loc);
				return QSE_NULL;
			}

			if (begin_include (awk) <= -1) return QSE_NULL;
			
			/* read the first meaningful token from the included file 
			 * and recheck it by jumping to retry: */
			do
			{
				if (get_token(awk) <= -1) return QSE_NULL; 
			}
			while (MATCH(awk,TOK_NEWLINE));

			goto retry;
		}
		else
		{
			SETERR_TOK (awk, QSE_AWK_EDIRECNR);
			return QSE_NULL;
		}
	}
	else if (MATCH(awk,TOK_FUNCTION)) 
	{
		awk->parse.id.block = PARSE_FUNCTION;
		if (parse_function (awk) == QSE_NULL) return QSE_NULL;
	}
	else if (MATCH(awk,TOK_BEGIN)) 
	{
		if ((awk->option & QSE_AWK_PABLOCK) == 0)
		{
			SETERR_TOK (awk, QSE_AWK_EKWFNC);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_BEGIN;
		if (get_token(awk) <= -1) return QSE_NULL; 

		if (MATCH(awk,TOK_NEWLINE) || MATCH(awk,TOK_EOF))
		{
			/* when QSE_AWK_NEWLINE is set,
	   		 * BEGIN and { should be located on the same line */
			SETERR_LOC (awk, QSE_AWK_EBLKBEG, &awk->ptok.loc);
			return QSE_NULL;
		}

		if (!MATCH(awk,TOK_LBRACE)) 
		{
			SETERR_TOK (awk, QSE_AWK_ELBRACE);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_BEGIN_BLOCK;
		if (parse_begin (awk) == QSE_NULL) return QSE_NULL;
	}
	else if (MATCH(awk,TOK_END)) 
	{
		if ((awk->option & QSE_AWK_PABLOCK) == 0)
		{
			SETERR_TOK (awk, QSE_AWK_EKWFNC);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_END;
		if (get_token(awk) <= -1) return QSE_NULL; 

		if (MATCH(awk,TOK_NEWLINE) || MATCH(awk,TOK_EOF))
		{
			/* when QSE_AWK_NEWLINE is set,
	   		 * END and { should be located on the same line */
			SETERR_LOC (awk, QSE_AWK_EBLKEND, &awk->ptok.loc);
			return QSE_NULL;
		}

		if (!MATCH(awk,TOK_LBRACE)) 
		{
			SETERR_TOK (awk, QSE_AWK_ELBRACE);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_END_BLOCK;
		if (parse_end (awk) == QSE_NULL) return QSE_NULL;
	}
	else if (MATCH(awk,TOK_LBRACE))
	{
		/* patternless block */
		if ((awk->option & QSE_AWK_PABLOCK) == 0)
		{
			SETERR_TOK (awk, QSE_AWK_EKWFNC);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_ACTION_BLOCK;
		if (parse_action_block (
			awk, QSE_NULL, QSE_FALSE) == QSE_NULL) return QSE_NULL;
	}
	else
	{
		/* 
		expressions 
		/regular expression/
		pattern && pattern
		pattern || pattern
		!pattern
		(pattern)
		pattern, pattern
		*/
		qse_awk_nde_t* ptn;

		if ((awk->option & QSE_AWK_PABLOCK) == 0)
		{
			SETERR_TOK (awk, QSE_AWK_EKWFNC);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_PATTERN;

		{
			qse_awk_loc_t eloc = awk->tok.loc;
			ptn = parse_expr_dc (awk, &eloc);
		}
		if (ptn == QSE_NULL) return QSE_NULL;

		QSE_ASSERT (ptn->next == QSE_NULL);

		if (MATCH(awk,TOK_COMMA))
		{
			if (get_token (awk) <= -1) 
			{
				qse_awk_clrpt (awk, ptn);
				return QSE_NULL;
			}	

			{
				qse_awk_loc_t eloc = awk->tok.loc;
				ptn->next = parse_expr_dc (awk, &eloc);
			}
			if (ptn->next == QSE_NULL) 
			{
				qse_awk_clrpt (awk, ptn);
				return QSE_NULL;
			}
		}

		if (MATCH(awk,TOK_NEWLINE) || MATCH(awk,TOK_EOF))
		{
			/* blockless pattern */
			qse_bool_t newline = MATCH(awk,TOK_NEWLINE);
			qse_awk_loc_t ploc = awk->ptok.loc;

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (parse_action_block (awk, ptn, QSE_TRUE) == QSE_NULL)
			{
				qse_awk_clrpt (awk, ptn);
				return QSE_NULL;	
			}

			if (newline)
			{
				if (get_token(awk) <= -1) 
				{
					/* ptn has been added to the chain. 
					 * it doesn't have to be cleared here
					 * as qse_awk_clear does it */
					/*qse_awk_clrpt (awk, ptn);*/
					return QSE_NULL;
				}	
			}

			if ((awk->option & QSE_AWK_RIO) != QSE_AWK_RIO)
			{
				/* blockless pattern requires QSE_AWK_RIO
				 * to be ON because the implicit block is
				 * "print $0" */
				SETERR_LOC (awk, QSE_AWK_ENOSUP, &ploc);
				return QSE_NULL;
			}
		}
		else
		{
			/* parse the action block */
			if (!MATCH(awk,TOK_LBRACE))
			{
				qse_awk_clrpt (awk, ptn);
				SETERR_TOK (awk, QSE_AWK_ELBRACE);
				return QSE_NULL;
			}

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (parse_action_block (awk, ptn, QSE_FALSE) == QSE_NULL) 
			{
				qse_awk_clrpt (awk, ptn);
				return QSE_NULL;	
			}
		}
	}

	return awk;
}

static qse_awk_nde_t* parse_function (qse_awk_t* awk)
{
	qse_char_t* name;
	qse_char_t* namedup;
	qse_size_t namelen;
	qse_awk_nde_t* body;
	qse_awk_fun_t* fun;
	qse_size_t nargs, g;
	qse_htb_pair_t* pair;

	/* eat up the keyword 'function' and get the next token */
	QSE_ASSERT (MATCH(awk,TOK_FUNCTION));
	if (get_token(awk) <= -1) return QSE_NULL;  

	/* match a function name */
	if (!MATCH(awk,TOK_IDENT)) 
	{
		/* cannot find a valid identifier for a function name */
		SETERR_TOK (awk, QSE_AWK_EFUNNAM);
		return QSE_NULL;
	}

	name = QSE_STR_PTR(awk->tok.name);
	namelen = QSE_STR_LEN(awk->tok.name);

	/* check if it is a builtin function */
	if (qse_awk_getfnc (awk, name, namelen) != QSE_NULL)
	{
		SETERR_ARG_LOC (
			awk, QSE_AWK_EFNCRED, name, namelen, &awk->tok.loc);
		return QSE_NULL;
	}

	/* check if it has already been defined as a function */
	if (qse_htb_search (awk->tree.funs, name, namelen) != QSE_NULL)
	{
		/* the function is defined previously */
		SETERR_ARG_LOC (
			awk, QSE_AWK_EFUNRED, name, namelen, &awk->tok.loc);
		return QSE_NULL;
	}

	/* check if it conflicts with a named variable */
	if (qse_htb_search (awk->parse.named, name, namelen) != QSE_NULL)
	{
		SETERR_ARG_LOC (
			awk, QSE_AWK_EVARRED, name, namelen, &awk->tok.loc);
		return QSE_NULL;
	}

	/* check if it coincides to be a global variable name */
	g = find_global (awk, name, namelen);
	if (g != QSE_LDA_NIL)
	{
		SETERR_ARG_LOC (
			awk, QSE_AWK_EGBLRED, name, namelen, &awk->tok.loc);
		return QSE_NULL;
	}

	/* clone the function name before it is overwritten */
	namedup = QSE_AWK_STRXDUP (awk, name, namelen);
	if (namedup == QSE_NULL) 
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, &awk->tok.loc);
		return QSE_NULL;
	}

	/* get the next token */
	if (get_token(awk) <= -1) 
	{
		QSE_AWK_FREE (awk, namedup);
		return QSE_NULL;  
	}

	/* match a left parenthesis */
	if (!MATCH(awk,TOK_LPAREN)) 
	{
		/* a function name is not followed by a left parenthesis */
		QSE_AWK_FREE (awk, namedup);
		SETERR_TOK (awk, QSE_AWK_ELPAREN);
		return QSE_NULL;
	}	

	/* get the next token */
	if (get_token(awk) <= -1) 
	{
		QSE_AWK_FREE (awk, namedup);
		return QSE_NULL;
	}

	/* make sure that parameter table is empty */
	QSE_ASSERT (QSE_LDA_SIZE(awk->parse.params) == 0);

	/* read parameter list */
	if (MATCH(awk,TOK_RPAREN)) 
	{
		/* no function parameter found. get the next token */
		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, namedup);
			return QSE_NULL;
		}
	}
	else 
	{
		while (1) 
		{
			qse_char_t* pa;
			qse_size_t pal;

			if (!MATCH(awk,TOK_IDENT)) 
			{
				QSE_AWK_FREE (awk, namedup);
				qse_lda_clear (awk->parse.params);
				SETERR_TOK (awk, QSE_AWK_EBADPAR);
				return QSE_NULL;
			}

			pa = QSE_STR_PTR(awk->tok.name);
			pal = QSE_STR_LEN(awk->tok.name);

			/* NOTE: the following is not a conflict. 
			 *       so the parameter is not checked against
			 *       global variables.
			 *  global x; 
			 *  function f (x) { print x; } 
			 *  x in print x is a parameter
			 */

			/* check if a parameter conflicts with the function 
			 * name or other parameters */
			if (((awk->option & QSE_AWK_STRICTNAMING) &&
			     qse_strxncmp (
				pa, pal, namedup, namelen) == 0) ||
			    qse_lda_search (awk->parse.params, 
				0, pa, pal) != QSE_LDA_NIL)
			{
				QSE_AWK_FREE (awk, namedup);
				qse_lda_clear (awk->parse.params);
				SETERR_ARG_LOC (
					awk, QSE_AWK_EDUPPAR, 
					pa, pal, &awk->tok.loc);
				return QSE_NULL;
			}

			/* push the parameter to the parameter list */
			if (QSE_LDA_SIZE(awk->parse.params) >= QSE_AWK_MAX_PARAMS)
			{
				QSE_AWK_FREE (awk, namedup);
				qse_lda_clear (awk->parse.params);
				SETERR_LOC (awk, QSE_AWK_EPARTM, &awk->tok.loc);
				return QSE_NULL;
			}

			if (qse_lda_insert (
				awk->parse.params, 
				QSE_LDA_SIZE(awk->parse.params), 
				pa, pal) == QSE_LDA_NIL)
			{
				QSE_AWK_FREE (awk, namedup);
				qse_lda_clear (awk->parse.params);
				SETERR_LOC (awk, QSE_AWK_ENOMEM, &awk->tok.loc);
				return QSE_NULL;
			}	

			if (get_token (awk) <= -1) 
			{
				QSE_AWK_FREE (awk, namedup);
				qse_lda_clear (awk->parse.params);
				return QSE_NULL;
			}	

			if (MATCH(awk,TOK_RPAREN)) break;

			if (!MATCH(awk,TOK_COMMA)) 
			{
				QSE_AWK_FREE (awk, namedup);
				qse_lda_clear (awk->parse.params);
				SETERR_TOK (awk, QSE_AWK_ECOMMA);
				return QSE_NULL;
			}

			do
			{
				if (get_token(awk) <= -1) 
				{
					QSE_AWK_FREE (awk, namedup);
					qse_lda_clear (awk->parse.params);
					return QSE_NULL;
				}
			}
			while (MATCH(awk,TOK_NEWLINE));
		}

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, namedup);
			qse_lda_clear (awk->parse.params);
			return QSE_NULL;
		}
	}

	/* function body can be placed on a different line 
	 * from a function name and the parameters even if
	 * QSE_AWK_NEWLINE is set. note TOK_NEWLINE is
	 * available only when the option is set. */
	while (MATCH(awk,TOK_NEWLINE))
	{
		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, namedup);
			qse_lda_clear (awk->parse.params);
			return QSE_NULL;
		}
	}

	/* check if the function body starts with a left brace */
	if (!MATCH(awk,TOK_LBRACE)) 
	{
		QSE_AWK_FREE (awk, namedup);
		qse_lda_clear (awk->parse.params);

		SETERR_TOK (awk, QSE_AWK_ELBRACE);
		return QSE_NULL;
	}
	if (get_token(awk) <= -1) 
	{
		QSE_AWK_FREE (awk, namedup);
		qse_lda_clear (awk->parse.params);
		return QSE_NULL; 
	}

	/* remember the current function name so that the body parser
	 * can know the name of the current function being parsed */
	awk->tree.cur_fun.ptr = namedup;
	awk->tree.cur_fun.len = namelen;

	/* actual function body */
	{
		qse_awk_loc_t xloc = awk->ptok.loc;
		body = parse_block_dc (awk, &xloc, QSE_TRUE);
	}

	/* clear the current function name remembered */
	awk->tree.cur_fun.ptr = QSE_NULL;
	awk->tree.cur_fun.len = 0;

	if (body == QSE_NULL) 
	{
		QSE_AWK_FREE (awk, namedup);
		qse_lda_clear (awk->parse.params);
		return QSE_NULL;
	}

	/* TODO: study furthur if the parameter names should be saved 
	 *       for some reasons - might be needed for deparsing output */
	nargs = QSE_LDA_SIZE(awk->parse.params);
	/* parameter names are not required anymore. clear them */
	qse_lda_clear (awk->parse.params);

	fun = (qse_awk_fun_t*) QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_fun_t));
	if (fun == QSE_NULL) 
	{
		QSE_AWK_FREE (awk, namedup);
		qse_awk_clrpt (awk, body);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, &awk->tok.loc);
		return QSE_NULL;
	}

	fun->name.ptr = QSE_NULL; /* function name is set below */
	fun->name.len = 0;
	fun->nargs = nargs;
	fun->body = body;

	pair = qse_htb_insert (awk->tree.funs, namedup, namelen, fun, 0);
	if (pair == QSE_NULL)
	{
		/* if qse_htb_insert() fails for other reasons than memory 
		 * shortage, there should be implementaion errors as duplicate
		 * functions are detected earlier in this function */
		QSE_AWK_FREE (awk, namedup);
		qse_awk_clrpt (awk, body);
		QSE_AWK_FREE (awk, fun);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, &awk->tok.loc);
		return QSE_NULL;
	}

	/* do some trick to save a string. make it back-point at the key part 
	 * of the pair */
	fun->name.ptr = QSE_HTB_KPTR(pair); 
	fun->name.len = QSE_HTB_KLEN(pair);
	QSE_AWK_FREE (awk, namedup);

	/* remove an undefined function call entry from the parse.fun table */
	qse_htb_delete (awk->parse.funs, fun->name.ptr, namelen);
	return body;
}

static qse_awk_nde_t* parse_begin (qse_awk_t* awk)
{
	qse_awk_nde_t* nde;
	qse_awk_loc_t xloc = awk->tok.loc;

	QSE_ASSERT (MATCH(awk,TOK_LBRACE));

	if (get_token(awk) <= -1) return QSE_NULL; 
	nde = parse_block_dc (awk, &xloc, QSE_TRUE);
	if (nde == QSE_NULL) return QSE_NULL;

	if (awk->tree.begin == QSE_NULL)
	{
		awk->tree.begin = nde;
		awk->tree.begin_tail = nde;
	}
	else
	{
		awk->tree.begin_tail->next = nde;
		awk->tree.begin_tail = nde;
	}

	return nde;
}

static qse_awk_nde_t* parse_end (qse_awk_t* awk)
{
	qse_awk_nde_t* nde;
	qse_awk_loc_t xloc = awk->tok.loc;

	QSE_ASSERT (MATCH(awk,TOK_LBRACE));

	if (get_token(awk) <= -1) return QSE_NULL; 
	nde = parse_block_dc (awk, &xloc, QSE_TRUE);
	if (nde == QSE_NULL) return QSE_NULL;

	if (awk->tree.end == QSE_NULL)
	{
		awk->tree.end = nde;
		awk->tree.end_tail = nde;
	}
	else
	{
		awk->tree.end_tail->next = nde;
		awk->tree.end_tail = nde;
	}
	return nde;
}

static qse_awk_chain_t* parse_action_block (
	qse_awk_t* awk, qse_awk_nde_t* ptn, qse_bool_t blockless)
{
	qse_awk_nde_t* nde;
	qse_awk_chain_t* chain;
	qse_awk_loc_t xloc = awk->tok.loc;

	if (blockless) nde = QSE_NULL;
	else
	{
		QSE_ASSERT (MATCH(awk,TOK_LBRACE));
		if (get_token(awk) <= -1) return QSE_NULL; 
		nde = parse_block_dc (awk, &xloc, QSE_TRUE);
		if (nde == QSE_NULL) return QSE_NULL;
	}

	chain = (qse_awk_chain_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_chain_t));
	if (chain == QSE_NULL) 
	{
		qse_awk_clrpt (awk, nde);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, &xloc);
		return QSE_NULL;
	}

	chain->pattern = ptn;
	chain->action = nde;
	chain->next = QSE_NULL;

	if (awk->tree.chain == QSE_NULL) 
	{
		awk->tree.chain = chain;
		awk->tree.chain_tail = chain;
		awk->tree.chain_size++;
	}
	else 
	{
		awk->tree.chain_tail->next = chain;
		awk->tree.chain_tail = chain;
		awk->tree.chain_size++;
	}

	return chain;
}

static qse_awk_nde_t* parse_block (
	qse_awk_t* awk, const qse_awk_loc_t* xloc, qse_bool_t istop) 
{
	qse_awk_nde_t* head, * curr, * nde;
	qse_awk_nde_blk_t* block;
	qse_size_t nlcls, nlcls_max, tmp;

	nlcls = QSE_LDA_SIZE(awk->parse.lcls);
	nlcls_max = awk->parse.nlcls_max;

	/* local variable declarations */
	if (awk->option & QSE_AWK_EXPLICIT) 
	{
		while (1) 
		{
			/* skip new lines before local declaration in a block*/
			while (MATCH(awk,TOK_NEWLINE))
			{
				if (get_token(awk) <= -1) return QSE_NULL;
			}

			if (!MATCH(awk,TOK_LOCAL)) break;

			if (get_token(awk) <= -1) 
			{
				qse_lda_delete (
					awk->parse.lcls, nlcls, 
					QSE_LDA_SIZE(awk->parse.lcls)-nlcls);
				return QSE_NULL;
			}

			if (collect_locals (awk, nlcls, istop) == QSE_NULL)
			{
				qse_lda_delete (
					awk->parse.lcls, nlcls, 
					QSE_LDA_SIZE(awk->parse.lcls)-nlcls);
				return QSE_NULL;
			}
		}
	}

	/* block body */
	head = QSE_NULL; curr = QSE_NULL;

	while (1) 
	{
		/* skip new lines within a block */
		while (MATCH(awk,TOK_NEWLINE))
		{
			if (get_token(awk) <= -1) return QSE_NULL;
		}

		/* if EOF is met before the right brace, this is an error */
		if (MATCH(awk,TOK_EOF)) 
		{
			qse_lda_delete (
				awk->parse.lcls, nlcls, 
				QSE_LDA_SIZE(awk->parse.lcls) - nlcls);
			if (head != QSE_NULL) qse_awk_clrpt (awk, head);
			SETERR_LOC (awk, QSE_AWK_EEOF, &awk->tok.loc);
			return QSE_NULL;
		}

		/* end the block when the right brace is met */
		if (MATCH(awk,TOK_RBRACE)) 
		{
			if (get_token(awk) <= -1) 
			{
				qse_lda_delete (
					awk->parse.lcls, nlcls, 
					QSE_LDA_SIZE(awk->parse.lcls)-nlcls);
				if (head != QSE_NULL) qse_awk_clrpt (awk, head);
				return QSE_NULL; 
			}

			break;
		}

		/* parse an actual statement in a block */
		{
			qse_awk_loc_t sloc = awk->tok.loc;
			nde = parse_statement (awk, &sloc);
		}

		if (nde == QSE_NULL) 
		{
			qse_lda_delete (
				awk->parse.lcls, nlcls, 
				QSE_LDA_SIZE(awk->parse.lcls)-nlcls);
			if (head != QSE_NULL) qse_awk_clrpt (awk, head);
			return QSE_NULL;
		}

		/* remove unnecessary statements such as adjacent 
		 * null statements */
		if (nde->type == QSE_AWK_NDE_NULL) 
		{
			qse_awk_clrpt (awk, nde);
			continue;
		}
		if (nde->type == QSE_AWK_NDE_BLK && 
		    ((qse_awk_nde_blk_t*)nde)->body == QSE_NULL) 
		{
			qse_awk_clrpt (awk, nde);
			continue;
		}
			
		if (curr == QSE_NULL) head = nde;
		else curr->next = nde;	
		curr = nde;
	}

	block = (qse_awk_nde_blk_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_blk_t));
	if (block == QSE_NULL) 
	{
		qse_lda_delete (
			awk->parse.lcls, nlcls, 
			QSE_LDA_SIZE(awk->parse.lcls)-nlcls);
		qse_awk_clrpt (awk, head);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	tmp = QSE_LDA_SIZE(awk->parse.lcls);
	if (tmp > awk->parse.nlcls_max) awk->parse.nlcls_max = tmp;

	/* remove all lcls to move it up to the top level */
	qse_lda_delete (awk->parse.lcls, nlcls, tmp - nlcls);

	/* adjust the number of lcls for a block without any statements */
	/* if (head == QSE_NULL) tmp = 0; */

	block->type = QSE_AWK_NDE_BLK;
	block->loc = *xloc;
	block->next = QSE_NULL;
	block->body = head;

	/* TODO: not only local variables but also nested blocks, 
	unless it is part of other constructs such as if, can be promoted 
	and merged to top-level block */

	/* migrate all block-local variables to a top-level block */
	if (istop) 
	{
		block->nlcls = awk->parse.nlcls_max - nlcls;
		awk->parse.nlcls_max = nlcls_max;
	}
	else 
	{
		/*block->nlcls = tmp - nlcls;*/
		block->nlcls = 0;
	}

	return (qse_awk_nde_t*)block;
}

static qse_awk_nde_t* parse_block_dc (
	qse_awk_t* awk, const qse_awk_loc_t* xloc, qse_bool_t istop) 
{
	qse_awk_nde_t* nde;
		
	if (awk->parse.depth.max.block > 0 &&
	    awk->parse.depth.cur.block >= awk->parse.depth.max.block)
	{
		SETERR_LOC (awk, QSE_AWK_EBLKNST, xloc);
		return QSE_NULL;
	}

	awk->parse.depth.cur.block++;
	nde = parse_block (awk, xloc, istop);
	awk->parse.depth.cur.block--;

	return nde;
}

int qse_awk_initgbls (qse_awk_t* awk)
{	
	int id;

	/* qse_awk_initgbls is not generic-purpose. call this from
	 * qse_awk_open only. */
	QSE_ASSERT (awk->tree.ngbls_base == 0 && awk->tree.ngbls == 0);

	awk->tree.ngbls_base = 0;
	awk->tree.ngbls = 0;

	for (id = QSE_AWK_MIN_GBL_ID; id <= QSE_AWK_MAX_GBL_ID; id++)
	{
		qse_size_t g;

		g = qse_lda_insert (
			awk->parse.gbls,
			QSE_LDA_SIZE(awk->parse.gbls),
			(qse_char_t*)gtab[id].name,
			gtab[id].namelen);
		if (g == QSE_LDA_NIL) return -1;

		QSE_ASSERT ((int)g == id);

		awk->tree.ngbls_base++;
		awk->tree.ngbls++;
	}

	QSE_ASSERT (awk->tree.ngbls_base == 
		QSE_AWK_MAX_GBL_ID-QSE_AWK_MIN_GBL_ID+1);
	return 0;
}

static void adjust_static_globals (qse_awk_t* awk)
{
	int id;

	QSE_ASSERT (awk->tree.ngbls_base >=
		QSE_AWK_MAX_GBL_ID - QSE_AWK_MAX_GBL_ID + 1);

	for (id = QSE_AWK_MIN_GBL_ID; id <= QSE_AWK_MAX_GBL_ID; id++)
	{
		if (gtab[id].valid != 0 && 
		    (awk->option & gtab[id].valid) != gtab[id].valid)
		{
			QSE_LDA_DLEN(awk->parse.gbls,id) = 0;
		}
		else
		{
			QSE_LDA_DLEN(awk->parse.gbls,id) = gtab[id].namelen;
		}
	}
}

static qse_size_t get_global (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	qse_size_t i;
	qse_lda_t* gbls = awk->parse.gbls;

	for (i = QSE_LDA_SIZE(gbls); i > 0; )
	{
		i--;

		if (qse_strxncmp (
			QSE_LDA_DPTR(gbls,i), QSE_LDA_DLEN(gbls,i), 
			name, len) == 0) return i;
	}

	return QSE_LDA_NIL;
}

static qse_size_t find_global (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	qse_size_t i;
	qse_lda_t* gbls = awk->parse.gbls;

	for (i = 0; i < QSE_LDA_SIZE(gbls); i++)
	{
		if (qse_strxncmp (
			QSE_LDA_DPTR(gbls,i), QSE_LDA_DLEN(gbls,i), 
			name, len) == 0) return i;
	}

	return QSE_LDA_NIL;
}

static int add_global (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len, 
	qse_awk_loc_t* xloc, int disabled)
{
	qse_size_t ngbls;

	/* check if it is a keyword */
	if (classify_ident (awk, name, len) != TOK_IDENT)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EKWRED, name, len, xloc);
		return -1;
	}

	/* check if it conflicts with a builtin function name */
	if (qse_awk_getfnc (awk, name, len) != QSE_NULL)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EFNCRED, name, len, xloc);
		return -1;
	}

	/* check if it conflicts with a function name */
	if (qse_htb_search (awk->tree.funs, name, len) != QSE_NULL) 
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, name, len, xloc);
		return -1;
	}

	/* check if it conflicts with a function name 
	 * caught in the function call table */
	if (qse_htb_search (awk->parse.funs, name, len) != QSE_NULL)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, name, len, xloc);
		return -1;
	}

	/* check if it conflicts with other global variable names */
	if (find_global (awk, name, len) != QSE_LDA_NIL)
	{ 
		SETERR_ARG_LOC (awk, QSE_AWK_EDUPGBL, name, len, xloc);
		return -1;
	}

#if 0	
	/* TODO: need to check if it conflicts with a named variable to 
	 * disallow such a program shown below (IMPLICIT & EXPLICIT on)
	 *  BEGIN {X=20; x(); x(); x(); print X}
	 *  global X;
	 *  function x() { print X++; }
	 */
	if (qse_htb_search (awk->parse.named, name, len) != QSE_NULL)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EVARRED, name, len, xloc);
		return -1;
	}
#endif

	ngbls = QSE_LDA_SIZE (awk->parse.gbls);
	if (ngbls >= QSE_AWK_MAX_GBLS)
	{
		SETERR_LOC (awk, QSE_AWK_EGBLTM, xloc);
		return -1;
	}

	if (qse_lda_insert (awk->parse.gbls, 
		QSE_LDA_SIZE(awk->parse.gbls), 
		(qse_char_t*)name, len) == QSE_LDA_NIL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return -1;
	}

	QSE_ASSERT (ngbls == QSE_LDA_SIZE(awk->parse.gbls) - 1);

	/* the disabled item is inserted normally but 
	 * the name length is reset to zero. */
	if (disabled) QSE_LDA_DLEN(awk->parse.gbls,ngbls) = 0;

	awk->tree.ngbls = QSE_LDA_SIZE (awk->parse.gbls);
	QSE_ASSERT (ngbls == awk->tree.ngbls-1);

	/* return the id which is the index to the gbl table. */
	return (int)ngbls;
}

int qse_awk_addgbl (qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	int n;

	if (len <= 0)
	{
		SETERR_COD (awk, QSE_AWK_EINVAL);
		return -1;
	}

	if (awk->tree.ngbls > awk->tree.ngbls_base) 
	{
		/* this function is not allowed after qse_awk_parse is called */
		SETERR_COD (awk, QSE_AWK_ENOPER);
		return -1;
	}

	n = add_global (awk, name, len, QSE_NULL, 0);

	/* update the count of the static globals. 
	 * the total global count has been updated inside add_global. */
	if (n >= 0) awk->tree.ngbls_base++; 

	return n;
}

#define QSE_AWK_NUM_STATIC_GBLS \
	(QSE_AWK_MAX_GBL_ID-QSE_AWK_MIN_GBL_ID+1)

int qse_awk_delgbl (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	qse_size_t n;

	if (awk->tree.ngbls > awk->tree.ngbls_base) 
	{
		/* this function is not allow after qse_awk_parse is called */
		SETERR_COD (awk, QSE_AWK_ENOPER);
		return -1;
	}

	n = qse_lda_search (awk->parse.gbls, 
		QSE_AWK_NUM_STATIC_GBLS, name, len);
	if (n == QSE_LDA_NIL)
	{
		SETERR_ARG (awk, QSE_AWK_ENOENT, name, len);
		return -1;
	}

	/* invalidate the name if deletion is requested.
	 * this approach does not delete the entry.
	 * if qse_delgbl() is called with the same name
	 * again, the entry will be appended again. 
	 * never call this funciton unless it is really required. */
	/*
	awk->parse.gbls.buf[n].name.ptr[0] = QSE_T('\0');
	awk->parse.gbls.buf[n].name.len = 0;
	*/
	n = qse_lda_uplete (awk->parse.gbls, n, 1);
	QSE_ASSERT (n == 1);

	return 0;
}

int qse_awk_findgbl (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	qse_size_t n;

	n = qse_lda_search (awk->parse.gbls, 
		QSE_AWK_NUM_STATIC_GBLS, name, len);
	if (n == QSE_LDA_NIL)
	{
		SETERR_ARG (awk, QSE_AWK_ENOENT, name, len);
		return -1;
	}

	return (int)n;
}

static qse_awk_t* collect_globals (qse_awk_t* awk)
{
	if (MATCH(awk,TOK_NEWLINE))
	{
		/* special check if the first name is on the 
		 * same line when QSE_AWK_NEWLINE is on */
		SETERR_COD (awk, QSE_AWK_EVARMS);
		return QSE_NULL;
	}

	while (1) 
	{
		if (!MATCH(awk,TOK_IDENT)) 
		{
			SETERR_TOK (awk, QSE_AWK_EBADVAR);
			return QSE_NULL;
		}

		if (add_global (
			awk,
			QSE_STR_PTR(awk->tok.name),
			QSE_STR_LEN(awk->tok.name),
			&awk->tok.loc, 0) <= -1) return QSE_NULL;

		if (get_token(awk) <= -1) return QSE_NULL;

		if (MATCH_TERMINATOR_NORMAL(awk)) 
		{
			/* skip a terminator (;, <NL>) */
			if (get_token(awk) <= -1) return QSE_NULL;
			break;
		}

		/*
		 * unlike collect_locals(), the right brace cannot
		 * terminate a global declaration as it can never be
		 * placed within a block. 
		 * so do not perform MATCH_TERMINATOR_RBRACE(awk))
		 */

		if (!MATCH(awk,TOK_COMMA)) 
		{
			SETERR_TOK (awk, QSE_AWK_ECOMMA);
			return QSE_NULL;
		}

		do
		{
			if (get_token(awk) <= -1) return QSE_NULL;
		} 
		while (MATCH(awk,TOK_NEWLINE));
	}

	return awk;
}

static qse_awk_t* collect_locals (
	qse_awk_t* awk, qse_size_t nlcls, qse_bool_t istop)
{
	if (MATCH(awk,TOK_NEWLINE))
	{
		/* special check if the first name is on the 
		 * same line when QSE_AWK_NEWLINE is on */
		SETERR_COD (awk, QSE_AWK_EVARMS);
		return QSE_NULL;
	}

	while (1) 
	{
		qse_xstr_t lcl;
		qse_size_t n;

		if (!MATCH(awk,TOK_IDENT)) 
		{
			SETERR_TOK (awk, QSE_AWK_EBADVAR);
			return QSE_NULL;
		}

		lcl = *QSE_STR_XSTR(awk->tok.name);

		/* check if it conflicts with a builtin function name 
		 * function f() { local length; } */
		if (qse_awk_getfnc (awk, lcl.ptr, lcl.len) != QSE_NULL)
		{
			SETERR_ARG_LOC (
				awk, QSE_AWK_EFNCRED, 
				lcl.ptr, lcl.len, &awk->tok.loc);
			return QSE_NULL;
		}

		if (istop)
		{
			/* check if it conflicts with a parameter name.
			 * the first level declaration is treated as the same
			 * scope as the parameter list */
			n = qse_lda_search (
				awk->parse.params, 0, lcl.ptr, lcl.len);
			if (n != QSE_LDA_NIL)
			{
				SETERR_ARG_LOC (
					awk, QSE_AWK_EPARRED, 
					lcl.ptr, lcl.len, &awk->tok.loc);
				return QSE_NULL;
			}
		}

		if (awk->option & QSE_AWK_STRICTNAMING)
		{
			/* check if it conflicts with the owning function */
			if (awk->tree.cur_fun.ptr != QSE_NULL)
			{
				if (qse_strxncmp (
					lcl.ptr, lcl.len,
					awk->tree.cur_fun.ptr,
					awk->tree.cur_fun.len) == 0)
				{
					SETERR_ARG_LOC (
						awk, QSE_AWK_EFUNRED, 	
						lcl.ptr, lcl.len, &awk->tok.loc);
					return QSE_NULL;
				}
			}
		}

		/* check if it conflicts with other local variable names */
		n = qse_lda_search (
			awk->parse.lcls, 
			nlcls,
			lcl.ptr, lcl.len);
		if (n != QSE_LDA_NIL)
		{
			SETERR_ARG_LOC (
				awk, QSE_AWK_EDUPLCL, 
				lcl.ptr, lcl.len, &awk->tok.loc);
			return QSE_NULL;
		}

		/* check if it conflicts with global variable names */
		n = find_global (awk, lcl.ptr, lcl.len);
		if (n != QSE_LDA_NIL)
		{
			if (n < awk->tree.ngbls_base)
			{
				/* it is a conflict only if it is one of a 
				 * static global variable */
				SETERR_ARG_LOC (
					awk, QSE_AWK_EDUPLCL, 
					lcl.ptr, lcl.len, &awk->tok.loc);
				return QSE_NULL;
			}
		}

		if (QSE_LDA_SIZE(awk->parse.lcls) >= QSE_AWK_MAX_LCLS)
		{
			SETERR_LOC (awk, QSE_AWK_ELCLTM, &awk->tok.loc);
			return QSE_NULL;
		}

		if (qse_lda_insert (
			awk->parse.lcls,
			QSE_LDA_SIZE(awk->parse.lcls),
			lcl.ptr, lcl.len) == QSE_LDA_NIL)
		{
			SETERR_LOC (awk, QSE_AWK_ENOMEM, &awk->tok.loc);
			return QSE_NULL;
		}

		if (get_token(awk) <= -1) return QSE_NULL;

		if (MATCH_TERMINATOR_NORMAL(awk)) 
		{
			/* skip the terminator (;, <NL>) */
			if (get_token(awk) <= -1) return QSE_NULL;
			break;
		}

		if (MATCH_TERMINATOR_RBRACE(awk))
		{
			/* should not skip } */
			break;
		}

		if (!MATCH(awk,TOK_COMMA))
		{
			SETERR_TOK (awk, QSE_AWK_ECOMMA);
			return QSE_NULL;
		}

		do
		{
			if (get_token(awk) <= -1) return QSE_NULL;
		}
		while (MATCH(awk,TOK_NEWLINE));
	}

	return awk;
}

static qse_awk_nde_t* parse_if (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* test = QSE_NULL;
	qse_awk_nde_t* then_part = QSE_NULL;
	qse_awk_nde_t* else_part = QSE_NULL;
	qse_awk_nde_if_t* nde_if;
	qse_awk_loc_t eloc, tloc;

	if (!MATCH(awk,TOK_LPAREN)) 
	{
		SETERR_TOK (awk, QSE_AWK_ELPAREN);
		return QSE_NULL;
	}
	if (get_token(awk) <= -1) return QSE_NULL;

	eloc = awk->tok.loc;
	test = parse_expr_dc (awk, &eloc);
	if (test == QSE_NULL) goto oops;

	if (!MATCH(awk,TOK_RPAREN)) 
	{
		SETERR_TOK (awk, QSE_AWK_ERPAREN);
		goto oops;
	}

	if (get_token(awk) <= -1) goto oops;

	tloc = awk->tok.loc;
	then_part = parse_statement (awk, &tloc);
	if (then_part == QSE_NULL) goto oops;

	/* skip any new lines before the else block */
	while (MATCH(awk,TOK_NEWLINE))
	{
		if (get_token(awk) <= -1) goto oops;
	} 

	if (MATCH(awk,TOK_ELSE)) 
	{
		if (get_token(awk) <= -1) goto oops;

		{
			qse_awk_loc_t eloc = awk->tok.loc;
			else_part = parse_statement (awk, &eloc);
			if (else_part == QSE_NULL) goto oops;
		}
	}

	nde_if = (qse_awk_nde_if_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(*nde_if));
	if (nde_if == QSE_NULL) 
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		goto oops;
	}

	nde_if->type = QSE_AWK_NDE_IF;
	nde_if->loc = *xloc;
	nde_if->next = QSE_NULL;
	nde_if->test = test;
	nde_if->then_part = then_part;
	nde_if->else_part = else_part;

	return (qse_awk_nde_t*)nde_if;

oops:
	if (else_part) qse_awk_clrpt (awk, else_part);
	if (then_part) qse_awk_clrpt (awk, then_part);
	if (test) qse_awk_clrpt (awk, test);
	return QSE_NULL;
}

static qse_awk_nde_t* parse_while (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* test, * body;
	qse_awk_nde_while_t* nde;

	if (!MATCH(awk,TOK_LPAREN)) 
	{
		SETERR_TOK (awk, QSE_AWK_ELPAREN);
		return QSE_NULL;
	}
	if (get_token(awk) <= -1) return QSE_NULL;

	{
		qse_awk_loc_t eloc = awk->tok.loc;
		test = parse_expr_dc (awk, &eloc);
	}
	if (test == QSE_NULL) return QSE_NULL;

	if (!MATCH(awk,TOK_RPAREN)) 
	{
		qse_awk_clrpt (awk, test);
		SETERR_TOK (awk, QSE_AWK_ERPAREN);
		return QSE_NULL;
	}

	if (get_token(awk) <= -1) 
	{
		qse_awk_clrpt (awk, test);
		return QSE_NULL;
	}

	{
		qse_awk_loc_t wloc = awk->tok.loc;
		body = parse_statement (awk, &wloc);
	}
	if (body == QSE_NULL) 
	{
		qse_awk_clrpt (awk, test);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_while_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_while_t));
	if (nde == QSE_NULL) 
	{
		qse_awk_clrpt (awk, body);
		qse_awk_clrpt (awk, test);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_WHILE;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	nde->test = test;
	nde->body = body;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_for (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* init = QSE_NULL, * test = QSE_NULL;
	qse_awk_nde_t* incr = QSE_NULL, * body = QSE_NULL;
	qse_awk_nde_for_t* nde_for; 
	qse_awk_nde_foreach_t* nde_foreach;

	if (!MATCH(awk,TOK_LPAREN))
	{
		SETERR_TOK (awk, QSE_AWK_ELPAREN);
		return QSE_NULL;
	}
	if (get_token(awk) <= -1) return QSE_NULL;
		
	if (!MATCH(awk,TOK_SEMICOLON)) 
	{
		/* this line is very ugly. it checks the entire next 
		 * expression or the first element in the expression
		 * is wrapped by a parenthesis */
		int no_foreach = MATCH(awk,TOK_LPAREN);

		{
			qse_awk_loc_t eloc = awk->tok.loc;
			init = parse_expr_dc (awk, &eloc);
			if (init == QSE_NULL) goto oops;
		}

		if (!no_foreach && init->type == QSE_AWK_NDE_EXP_BIN &&
		    ((qse_awk_nde_exp_t*)init)->opcode == QSE_AWK_BINOP_IN &&
		    is_plain_var(((qse_awk_nde_exp_t*)init)->left))
		{	
			/* switch to foreach - for (x in y) */
			
			if (!MATCH(awk,TOK_RPAREN))
			{
				SETERR_TOK (awk, QSE_AWK_ERPAREN);
				goto oops;
			}

			if (get_token(awk) <= -1) goto oops;
			
			{
				qse_awk_loc_t floc = awk->tok.loc;
				body = parse_statement (awk, &floc);
				if (body == QSE_NULL) goto oops;
			}

			nde_foreach = (qse_awk_nde_foreach_t*) QSE_AWK_ALLOC (
				awk, QSE_SIZEOF(*nde_foreach));
			if (nde_foreach == QSE_NULL)
			{
				SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
				goto oops;
			}

			nde_foreach->type = QSE_AWK_NDE_FOREACH;
			nde_foreach->loc = *xloc;
			nde_foreach->next = QSE_NULL;
			nde_foreach->test = init;
			nde_foreach->body = body;

			return (qse_awk_nde_t*)nde_foreach;
		}

		if (!MATCH(awk,TOK_SEMICOLON)) 
		{
			SETERR_TOK (awk, QSE_AWK_ESCOLON);
			goto oops;
		}
	}

	do
	{
		if (get_token(awk) <= -1)  goto oops;
		/* skip new lines after the first semicolon */
	} 
	while (MATCH(awk,TOK_NEWLINE));

	if (!MATCH(awk,TOK_SEMICOLON)) 
	{
		{
			qse_awk_loc_t eloc = awk->tok.loc;
			test = parse_expr_dc (awk, &eloc);
			if (test == QSE_NULL) goto oops;
		}

		if (!MATCH(awk,TOK_SEMICOLON)) 
		{
			SETERR_TOK (awk, QSE_AWK_ESCOLON);
			goto oops;
		}
	}

	do
	{
		if (get_token(awk) <= -1) goto oops;
		/* skip new lines after the second semicolon */
	}
	while (MATCH(awk,TOK_NEWLINE));
	
	if (!MATCH(awk,TOK_RPAREN)) 
	{
		{
			qse_awk_loc_t eloc = awk->tok.loc;
			incr = parse_expr_dc (awk, &eloc);
			if (incr == QSE_NULL) goto oops;
		}

		if (!MATCH(awk,TOK_RPAREN)) 
		{
			SETERR_TOK (awk, QSE_AWK_ERPAREN);
			goto oops;
		}
	}

	if (get_token(awk) <= -1) goto oops;

	{
		qse_awk_loc_t floc = awk->tok.loc;
		body = parse_statement (awk, &floc);
		if (body == QSE_NULL) goto oops;
	}

	nde_for = (qse_awk_nde_for_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(*nde_for));
	if (nde_for == QSE_NULL) 
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		goto oops;
	}

	nde_for->type = QSE_AWK_NDE_FOR;
	nde_for->loc = *xloc;
	nde_for->next = QSE_NULL;
	nde_for->init = init;
	nde_for->test = test;
	nde_for->incr = incr;
	nde_for->body = body;

	return (qse_awk_nde_t*)nde_for;

oops:
	if (init) qse_awk_clrpt (awk, init);
	if (test) qse_awk_clrpt (awk, test);
	if (incr) qse_awk_clrpt (awk, incr);
	if (body) qse_awk_clrpt (awk, body);
	return QSE_NULL;
}

static qse_awk_nde_t* parse_dowhile (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* test, * body;
	qse_awk_nde_while_t* nde;

	QSE_ASSERT (awk->ptok.type == TOK_DO);

	{
		qse_awk_loc_t dwloc = awk->tok.loc;
		body = parse_statement (awk, &dwloc);
	}
	if (body == QSE_NULL) return QSE_NULL;

	while (MATCH(awk,TOK_NEWLINE))
	{
		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, body);
			return QSE_NULL;
		}
	}

	if (!MATCH(awk,TOK_WHILE)) 
	{
		qse_awk_clrpt (awk, body);

		SETERR_TOK (awk, QSE_AWK_EKWWHL);
		return QSE_NULL;
	}

	if (get_token(awk) <= -1) 
	{
		qse_awk_clrpt (awk, body);
		return QSE_NULL;
	}

	if (!MATCH(awk,TOK_LPAREN)) 
	{
		qse_awk_clrpt (awk, body);

		SETERR_TOK (awk, QSE_AWK_ELPAREN);
		return QSE_NULL;
	}

	if (get_token(awk) <= -1) 
	{
		qse_awk_clrpt (awk, body);
		return QSE_NULL;
	}

	{
		qse_awk_loc_t eloc = awk->tok.loc;
		test = parse_expr_dc (awk, &eloc);
	}
	if (test == QSE_NULL) 
	{
		qse_awk_clrpt (awk, body);
		return QSE_NULL;
	}

	if (!MATCH(awk,TOK_RPAREN)) 
	{
		qse_awk_clrpt (awk, body);
		qse_awk_clrpt (awk, test);

		SETERR_TOK (awk, QSE_AWK_ERPAREN);
		return QSE_NULL;
	}

	if (get_token(awk) <= -1) 
	{
		qse_awk_clrpt (awk, body);
		qse_awk_clrpt (awk, test);
		return QSE_NULL;
	}
	
	nde = (qse_awk_nde_while_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_while_t));
	if (nde == QSE_NULL) 
	{
		qse_awk_clrpt (awk, body);
		qse_awk_clrpt (awk, test);

		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_DOWHILE;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	nde->test = test;
	nde->body = body;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_break (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_break_t* nde;

	QSE_ASSERT (awk->ptok.type == TOK_BREAK);
	if (awk->parse.depth.cur.loop <= 0) 
	{
		SETERR_LOC (awk, QSE_AWK_EBREAK, xloc);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_break_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_break_t));
	if (nde == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_BREAK;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_continue (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_continue_t* nde;

	QSE_ASSERT (awk->ptok.type == TOK_CONTINUE);
	if (awk->parse.depth.cur.loop <= 0) 
	{
		SETERR_LOC (awk, QSE_AWK_ECONTINUE, xloc);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_continue_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_continue_t));
	if (nde == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_CONTINUE;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_return (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_return_t* nde;
	qse_awk_nde_t* val;

	QSE_ASSERT (awk->ptok.type == TOK_RETURN);

	nde = (qse_awk_nde_return_t*) QSE_AWK_ALLOC (
		awk, QSE_SIZEOF(qse_awk_nde_return_t));
	if (nde == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_RETURN;
	nde->loc = *xloc;
	nde->next = QSE_NULL;

	if (MATCH_TERMINATOR(awk))
	{
		/* no return value */
		val = QSE_NULL;
	}
	else 
	{
		qse_awk_loc_t eloc = awk->tok.loc;
		val = parse_expr_dc (awk, &eloc);
		if (val == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;
		}
	}

	nde->val = val;
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_exit (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_exit_t* nde;
	qse_awk_nde_t* val;

	QSE_ASSERT (awk->ptok.type == TOK_EXIT);

	nde = (qse_awk_nde_exit_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_exit_t));
	if (nde == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_EXIT;
	nde->loc = *xloc;
	nde->next = QSE_NULL;

	if (MATCH_TERMINATOR(awk)) 
	{
		/* no exit code */
		val = QSE_NULL;
	}
	else 
	{
		qse_awk_loc_t eloc = awk->tok.loc;
		val = parse_expr_dc (awk, &eloc);
		if (val == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;
		}
	}

	nde->val = val;
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_next (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_next_t* nde;

	QSE_ASSERT (awk->ptok.type == TOK_NEXT);

	if (awk->parse.id.block == PARSE_BEGIN_BLOCK)
	{
		SETERR_LOC (awk, QSE_AWK_ENEXTBEG, xloc);
		return QSE_NULL;
	}
	if (awk->parse.id.block == PARSE_END_BLOCK)
	{
		SETERR_LOC (awk, QSE_AWK_ENEXTEND, xloc);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_next_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_next_t));
	if (nde == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}
	nde->type = QSE_AWK_NDE_NEXT;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_nextfile (
	qse_awk_t* awk, const qse_awk_loc_t* xloc, int out)
{
	qse_awk_nde_nextfile_t* nde;

	if (!out && awk->parse.id.block == PARSE_BEGIN_BLOCK)
	{
		SETERR_LOC (awk, QSE_AWK_ENEXTFBEG, xloc);
		return QSE_NULL;
	}
	if (!out && awk->parse.id.block == PARSE_END_BLOCK)
	{
		SETERR_LOC (awk, QSE_AWK_ENEXTFEND, xloc);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_nextfile_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_nextfile_t));
	if (nde == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_NEXTFILE;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	nde->out = out;
	
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_delete (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_delete_t* nde;
	qse_awk_nde_t* var;
	qse_awk_loc_t dloc;

	QSE_ASSERT (awk->ptok.type == TOK_DELETE);
	if (!MATCH(awk,TOK_IDENT)) 
	{
		SETERR_TOK (awk, QSE_AWK_EIDENT);
		return QSE_NULL;
	}

	dloc = awk->tok.loc;
	var = parse_primary_ident (awk, &dloc);
	if (var == QSE_NULL) return QSE_NULL;

	if (!is_var (var))
	{
		/* a normal identifier is expected */
		qse_awk_clrpt (awk, var);
		SETERR_LOC (awk, QSE_AWK_EDELETE, &dloc);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_delete_t*) QSE_AWK_ALLOC (
		awk, QSE_SIZEOF(qse_awk_nde_delete_t));
	if (nde == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_DELETE;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	nde->var = var;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_reset (qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_reset_t* nde;
	qse_awk_nde_t* var;
	qse_awk_loc_t rloc;

	QSE_ASSERT (awk->ptok.type == TOK_RESET);
	if (!MATCH(awk,TOK_IDENT)) 
	{
		SETERR_TOK (awk, QSE_AWK_EIDENT);
		return QSE_NULL;
	}

	rloc = awk->tok.loc;
	var = parse_primary_ident (awk, &rloc);
	if (var == QSE_NULL) return QSE_NULL;

	/* unlike delete, it must be followed by a plain variable only */
	if (!is_plain_var (var))
	{
		/* a normal identifier is expected */
		qse_awk_clrpt (awk, var);
		SETERR_LOC (awk, QSE_AWK_ERESET, &rloc);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_reset_t*) QSE_AWK_ALLOC (
		awk, QSE_SIZEOF(qse_awk_nde_reset_t));
	if (nde == QSE_NULL)
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_RESET;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	nde->var = var;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_print (
	qse_awk_t* awk, const qse_awk_loc_t* xloc, int type)
{
	qse_awk_nde_print_t* nde;
	qse_awk_nde_t* args = QSE_NULL; 
	qse_awk_nde_t* out = QSE_NULL;
	int out_type;

	if (!MATCH_TERMINATOR(awk) &&
	    !MATCH(awk,TOK_GT) &&
	    !MATCH(awk,TOK_RS) &&
	    !MATCH(awk,TOK_BOR) &&
	    !MATCH(awk,TOK_LOR)) 
	{
		qse_awk_nde_t* args_tail;
		qse_awk_nde_t* tail_prev;

		{
			qse_awk_loc_t eloc = awk->tok.loc;
			args = parse_expr_dc (awk, &eloc);
		}
		if (args == QSE_NULL) return QSE_NULL;

		args_tail = args;
		tail_prev = QSE_NULL;

		if (args->type != QSE_AWK_NDE_GRP)
		{
			/* args->type == QSE_AWK_NDE_GRP when print (a, b, c) 
			 * args->type != QSE_AWK_NDE_GRP when print a, b, c */
			
			while (MATCH(awk,TOK_COMMA))
			{
				do {
					if (get_token(awk) <= -1)
					{
						qse_awk_clrpt (awk, args);
						return QSE_NULL;
					}
				}
				while (MATCH(awk,TOK_NEWLINE));

				{
					qse_awk_loc_t eloc = awk->tok.loc;
					args_tail->next = parse_expr_dc (awk, &eloc);
				}
				if (args_tail->next == QSE_NULL)
				{
					qse_awk_clrpt (awk, args);
					return QSE_NULL;
				}

				tail_prev = args_tail;
				args_tail = args_tail->next;
			}
		}

		/* print 1 > 2 would print 1 to the file named 2. 
		 * print (1 > 2) would print (1 > 2) on the console */
		if (awk->ptok.type != TOK_RPAREN &&
		    args_tail->type == QSE_AWK_NDE_EXP_BIN)
		{
			int i;
			qse_awk_nde_exp_t* ep = (qse_awk_nde_exp_t*)args_tail;
			struct 
			{
				int opc;
				int out;
				int opt;
			} tab[] =
			{
				{ 
					QSE_AWK_BINOP_GT,     
					QSE_AWK_OUT_FILE,      
					0
				},
				{ 
					QSE_AWK_BINOP_RS, 
					QSE_AWK_OUT_APFILE,
					0
				},
				{
					QSE_AWK_BINOP_BOR,
					QSE_AWK_OUT_PIPE,
					0
				},
				{
					QSE_AWK_BINOP_LOR,
					QSE_AWK_OUT_RWPIPE,
					QSE_AWK_RWPIPE
				}
			};

			for (i = 0; i < QSE_COUNTOF(tab); i++)
			{
				if (ep->opcode == tab[i].opc)
				{
					qse_awk_nde_t* tmp;

					if (tab[i].opt && 
					    !(awk->option&tab[i].opt)) break;

					tmp = args_tail;

					if (tail_prev != QSE_NULL) 
						tail_prev->next = ep->left;
					else args = ep->left;

					out = ep->right;
					out_type = tab[i].out;

					QSE_AWK_FREE (awk, tmp);
					break;
				}
			}
		}
	}

	if (out == QSE_NULL)
	{
		out_type = MATCH(awk,TOK_GT)?       QSE_AWK_OUT_FILE:
		           MATCH(awk,TOK_RS)?   QSE_AWK_OUT_APFILE:
		           MATCH(awk,TOK_BOR)?      QSE_AWK_OUT_PIPE:
		           ((awk->option & QSE_AWK_RWPIPE) &&
			    MATCH(awk,TOK_LOR))?    QSE_AWK_OUT_RWPIPE:
		                                      QSE_AWK_OUT_CONSOLE;

		if (out_type != QSE_AWK_OUT_CONSOLE)
		{
			if (get_token(awk) <= -1)
			{
				if (args != QSE_NULL) qse_awk_clrpt (awk, args);
				return QSE_NULL;
			}

			{
				qse_awk_loc_t eloc = awk->tok.loc;
				out = parse_expr_dc (awk, &eloc);
			}
			if (out == QSE_NULL)
			{
				if (args != QSE_NULL) qse_awk_clrpt (awk, args);
				return QSE_NULL;
			}
		}
	}

	nde = (qse_awk_nde_print_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_print_t));
	if (nde == QSE_NULL) 
	{
		if (args != QSE_NULL) qse_awk_clrpt (awk, args);
		if (out != QSE_NULL) qse_awk_clrpt (awk, out);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	QSE_ASSERTX (
		type == QSE_AWK_NDE_PRINT || type == QSE_AWK_NDE_PRINTF, 
		"the node type should be either QSE_AWK_NDE_PRINT or QSE_AWK_NDE_PRINTF");

	if (type == QSE_AWK_NDE_PRINTF && args == QSE_NULL)
	{
		if (out != QSE_NULL) qse_awk_clrpt (awk, out);
		SETERR_LOC (awk, QSE_AWK_EPRINTFARG, xloc);
		return QSE_NULL;
	}

	nde->type = type;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	nde->args = args;
	nde->out_type = out_type;
	nde->out = out;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_statement_nb (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	/* parse a non-block statement */
	qse_awk_nde_t* nde;

	/* keywords that don't require any terminating semicolon */
	if (MATCH(awk,TOK_IF)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		return parse_if (awk, xloc);
	}
	else if (MATCH(awk,TOK_WHILE)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		
		awk->parse.depth.cur.loop++;
		nde = parse_while (awk, xloc);
		awk->parse.depth.cur.loop--;

		return nde;
	}
	else if (MATCH(awk,TOK_FOR)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;

		awk->parse.depth.cur.loop++;
		nde = parse_for (awk, xloc);
		awk->parse.depth.cur.loop--;

		return nde;
	}

	/* keywords that require a terminating semicolon */
	if (MATCH(awk,TOK_DO)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;

		awk->parse.depth.cur.loop++;
		nde = parse_dowhile (awk, xloc);
		awk->parse.depth.cur.loop--;

		return nde;
	}
	else if (MATCH(awk,TOK_BREAK)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_break (awk, xloc);
	}
	else if (MATCH(awk,TOK_CONTINUE)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_continue (awk, xloc);
	}
	else if (MATCH(awk,TOK_RETURN)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_return (awk, xloc);
	}
	else if (MATCH(awk,TOK_EXIT)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_exit (awk, xloc);
	}
	else if (MATCH(awk,TOK_NEXT)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_next (awk, xloc);
	}
	else if (MATCH(awk,TOK_NEXTFILE)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_nextfile (awk, xloc, 0);
	}
	else if (MATCH(awk,TOK_NEXTOFILE))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_nextfile (awk, xloc, 1);
	}
	else if (MATCH(awk,TOK_DELETE)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_delete (awk, xloc);
	}
	else if (MATCH(awk,TOK_RESET))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_reset (awk, xloc);
	}
	else if (MATCH(awk,TOK_PRINT))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_print (awk, xloc, QSE_AWK_NDE_PRINT);
	}
	else if (MATCH(awk,TOK_PRINTF))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_print (awk, xloc, QSE_AWK_NDE_PRINTF);
	}
	else 
	{
		nde = parse_expr_dc (awk, xloc);
	}

	if (nde == QSE_NULL) return QSE_NULL;

	if (MATCH_TERMINATOR_NORMAL(awk))
	{
		/* check if a statement ends with a semicolon or <NL> */
		if (get_token(awk) <= -1)
		{
			if (nde != QSE_NULL) qse_awk_clrpt (awk, nde);
			return QSE_NULL;
		}
	}
	else if (MATCH_TERMINATOR_RBRACE(awk))
	{
		/* do not skip the right brace as a statement terminator. 
		 * is there anything to do here? */
	}
	else
	{
		if (nde != QSE_NULL) qse_awk_clrpt (awk, nde);
		SETERR_LOC (awk, QSE_AWK_ESTMEND, &awk->ptok.loc);
		return QSE_NULL;
	}

	return nde;
}

static qse_awk_nde_t* parse_statement (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* nde;

	/* skip new lines before a statement */
	while (MATCH(awk,TOK_NEWLINE))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
	}

	if (MATCH(awk,TOK_SEMICOLON)) 
	{
		/* null statement */	
		nde = (qse_awk_nde_t*) 
			QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_t));
		if (nde == QSE_NULL) 
		{
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_NULL;
		nde->loc = *xloc;
		nde->next = QSE_NULL;

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;
		}
	}
	else if (MATCH(awk,TOK_LBRACE)) 
	{
		/* a block statemnt { ... } */
		qse_awk_loc_t tloc = awk->ptok.loc;
		if (get_token(awk) <= -1) return QSE_NULL; 
		nde = parse_block_dc (awk, &tloc, QSE_FALSE);
	}
	else 
	{
		/* the statement id held in awk->parse.id.stmt denotes
		 * the token id of the statement currently being parsed.
		 * the current statement id is saved here because the 
		 * statement id can be changed in parse_statement_nb.
		 * it will, in turn, call parse_statement which will
		 * eventually change the statement id. */
		int old_id = awk->parse.id.stmt;
		qse_awk_loc_t tloc = awk->tok.loc;

		/* set the current statement id */
		awk->parse.id.stmt = awk->tok.type;

		/* proceed parsing the statement */
		nde = parse_statement_nb (awk, &tloc);

		/* restore the statement id saved previously */
		awk->parse.id.stmt = old_id;
	}

	return nde;
}

static int assign_to_opcode (qse_awk_t* awk)
{
	/* synchronize it with qse_awk_assop_type_t in run.h */
	static int assop[] =
	{
		QSE_AWK_ASSOP_NONE,
		QSE_AWK_ASSOP_PLUS,
		QSE_AWK_ASSOP_MINUS,
		QSE_AWK_ASSOP_MUL,
		QSE_AWK_ASSOP_DIV,
		QSE_AWK_ASSOP_IDIV,
		QSE_AWK_ASSOP_MOD,
		QSE_AWK_ASSOP_EXP,
		QSE_AWK_ASSOP_CONCAT,
		QSE_AWK_ASSOP_RS,
		QSE_AWK_ASSOP_LS,
		QSE_AWK_ASSOP_BAND,
		QSE_AWK_ASSOP_BXOR,
		QSE_AWK_ASSOP_BOR
	};

	if (awk->tok.type >= TOK_ASSN &&
	    awk->tok.type <= TOK_BOR_ASSN)
	{
		return assop[awk->tok.type - TOK_ASSN];
	}

	return -1;
}

static qse_awk_nde_t* parse_expr_basic (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* nde, * n1, * n2;
	
	nde = parse_logical_or (awk, xloc);
	if (nde == QSE_NULL) return QSE_NULL;

	if (MATCH(awk,TOK_QUEST))
	{ 
		qse_awk_loc_t eloc;
		qse_awk_nde_cnd_t* cnd;

		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, nde);
			return QSE_NULL;
		}

		eloc = awk->tok.loc;	
		n1 = parse_expr_dc (awk, &eloc);
		if (n1 == QSE_NULL) 
		{
			qse_awk_clrpt (awk, nde);
			return QSE_NULL;
		}

		if (!MATCH(awk,TOK_COLON)) 
		{
			qse_awk_clrpt (awk, nde);
			qse_awk_clrpt (awk, n1);
			SETERR_TOK (awk, QSE_AWK_ECOLON);
			return QSE_NULL;
		}
		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, nde);
			qse_awk_clrpt (awk, n1);
			return QSE_NULL;
		}

		eloc = awk->tok.loc;
		n2 = parse_expr_dc (awk, &eloc);
		if (n2 == QSE_NULL)
		{
			qse_awk_clrpt (awk, nde);
			qse_awk_clrpt (awk, n1);
			return QSE_NULL;
		}

		cnd = (qse_awk_nde_cnd_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_cnd_t));
		if (cnd == QSE_NULL)
		{
			qse_awk_clrpt (awk, nde);
			qse_awk_clrpt (awk, n1);
			qse_awk_clrpt (awk, n2);
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		cnd->type = QSE_AWK_NDE_CND;
		cnd->loc = *xloc;
		cnd->next = QSE_NULL;
		cnd->test = nde;
		cnd->left = n1;
		cnd->right = n2;

		nde = (qse_awk_nde_t*)cnd;
	}

	return nde;
}

static qse_awk_nde_t* parse_expr (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* x, * y;
	qse_awk_nde_ass_t* nde;
	int opcode;

	x = parse_expr_basic (awk, xloc);
	if (x == QSE_NULL) return QSE_NULL;

	opcode = assign_to_opcode (awk);
	if (opcode <= -1) 
	{
		/* no assignment operator found. */
		return x;
	}

	QSE_ASSERT (x->next == QSE_NULL);
	if (!is_var(x) && x->type != QSE_AWK_NDE_POS) 
	{
		qse_awk_clrpt (awk, x);
		SETERR_LOC (awk, QSE_AWK_EASSIGN, xloc);
		return QSE_NULL;
	}

	if (get_token(awk) <= -1) 
	{
		qse_awk_clrpt (awk, x);
		return QSE_NULL;
	}

	{
		qse_awk_loc_t eloc = awk->tok.loc;
		y = parse_expr_dc (awk, &eloc);
	}
	if (y == QSE_NULL) 
	{
		qse_awk_clrpt (awk, x);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_ass_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_ass_t));
	if (nde == QSE_NULL) 
	{
		qse_awk_clrpt (awk, x);
		qse_awk_clrpt (awk, y);

		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_ASS;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	nde->opcode = opcode;
	nde->left = x;
	nde->right = y;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_expr_dc (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* nde;

	if (awk->parse.depth.max.expr > 0 &&
	    awk->parse.depth.cur.expr >= awk->parse.depth.max.expr)
	{
		SETERR_LOC (awk, QSE_AWK_EEXPRNST, xloc);
		return QSE_NULL;
	}

	awk->parse.depth.cur.expr++;
	nde = parse_expr (awk, xloc);
	awk->parse.depth.cur.expr--;

	return nde;
}

#define INT_BINOP_INT(x,op,y) \
	(((qse_awk_nde_int_t*)x)->val op ((qse_awk_nde_int_t*)y)->val)

#define INT_BINOP_FLT(x,op,y) \
	(((qse_awk_nde_int_t*)x)->val op ((qse_awk_nde_flt_t*)y)->val)

#define FLT_BINOP_INT(x,op,y) \
	(((qse_awk_nde_flt_t*)x)->val op ((qse_awk_nde_int_t*)y)->val)

#define FLT_BINOP_FLT(x,op,y) \
	(((qse_awk_nde_flt_t*)x)->val op ((qse_awk_nde_flt_t*)y)->val)

union folded_t
{
	qse_long_t l;
	qse_flt_t r;
};
typedef union folded_t folded_t;

static int fold_constants_for_binop (
	qse_awk_t* awk, qse_awk_nde_t* left, qse_awk_nde_t* right,
	int opcode, folded_t* folded)
{
	int fold = -1;

	/* TODO: can i shorten various comparisons below? 
 	 *       i hate to repeat similar code just for type difference */

	if (left->type == QSE_AWK_NDE_INT &&
	    right->type == QSE_AWK_NDE_INT)
	{
		fold = QSE_AWK_NDE_INT;
		switch (opcode)
		{
			case QSE_AWK_BINOP_PLUS:
				folded->l = INT_BINOP_INT(left,+,right);
				break;

			case QSE_AWK_BINOP_MINUS:
				folded->l = INT_BINOP_INT(left,-,right);
				break;

			case QSE_AWK_BINOP_MUL:
				folded->l = INT_BINOP_INT(left,*,right);
				break;

			case QSE_AWK_BINOP_DIV:
				if (INT_BINOP_INT(left,%,right))
				{
					folded->r = (qse_flt_t)((qse_awk_nde_int_t*)left)->val / 
					            (qse_flt_t)((qse_awk_nde_int_t*)right)->val;
					fold = QSE_AWK_NDE_FLT;
					break;
				}
				/* fall through here */
			case QSE_AWK_BINOP_IDIV:
				folded->l = INT_BINOP_INT(left,/,right);
				break;

			case QSE_AWK_BINOP_MOD:
				folded->l = INT_BINOP_INT(left,%,right);
				break;

			default:
				fold = -1;
				break;
		}
	}
	else if (left->type == QSE_AWK_NDE_FLT &&
	         right->type == QSE_AWK_NDE_FLT)
	{
		fold = QSE_AWK_NDE_FLT;
		switch (opcode)
		{
			case QSE_AWK_BINOP_PLUS:
				folded->r = FLT_BINOP_FLT(left,+,right);
				break;

			case QSE_AWK_BINOP_MINUS:
				folded->r = FLT_BINOP_FLT(left,-,right);
				break;

			case QSE_AWK_BINOP_MUL:
				folded->r = FLT_BINOP_FLT(left,*,right);
				break;

			case QSE_AWK_BINOP_DIV:
				folded->r = FLT_BINOP_FLT(left,/,right);
				break;

			case QSE_AWK_BINOP_IDIV:
				folded->l = (qse_long_t)FLT_BINOP_FLT(left,/,right);
				fold = QSE_AWK_NDE_INT;
				break;

			case QSE_AWK_BINOP_MOD:
				folded->r = awk->prm.math.mod (
					awk, 
					((qse_awk_nde_flt_t*)left)->val, 
					((qse_awk_nde_flt_t*)right)->val
				);
				break;

			default:
				fold = -1;
				break;
		}
	}
	else if (left->type == QSE_AWK_NDE_INT &&
	         right->type == QSE_AWK_NDE_FLT)
	{
		fold = QSE_AWK_NDE_FLT;
		switch (opcode)
		{
			case QSE_AWK_BINOP_PLUS:
				folded->r = INT_BINOP_FLT(left,+,right);
				break;

			case QSE_AWK_BINOP_MINUS:
				folded->r = INT_BINOP_FLT(left,-,right);
				break;

			case QSE_AWK_BINOP_MUL:
				folded->r = INT_BINOP_FLT(left,*,right);
				break;

			case QSE_AWK_BINOP_DIV:
				folded->r = INT_BINOP_FLT(left,/,right);
				break;

			case QSE_AWK_BINOP_IDIV:
				folded->l = (qse_long_t)
					((qse_flt_t)((qse_awk_nde_int_t*)left)->val / 
					 ((qse_awk_nde_flt_t*)right)->val);
				fold = QSE_AWK_NDE_INT;
				break;

			case QSE_AWK_BINOP_MOD:
				folded->r = awk->prm.math.mod (
					awk, 
					(qse_flt_t)((qse_awk_nde_int_t*)left)->val, 
					((qse_awk_nde_flt_t*)right)->val
				);
				break;

			default:
				fold = -1;
				break;
		}
	}
	else if (left->type == QSE_AWK_NDE_FLT &&
	         right->type == QSE_AWK_NDE_INT)
	{
		fold = QSE_AWK_NDE_FLT;
		switch (opcode)
		{
			case QSE_AWK_BINOP_PLUS:
				folded->r = FLT_BINOP_INT(left,+,right);
				break;

			case QSE_AWK_BINOP_MINUS:
				folded->r = FLT_BINOP_INT(left,-,right);
				break;

			case QSE_AWK_BINOP_MUL:
				folded->r = FLT_BINOP_INT(left,*,right);
				break;

			case QSE_AWK_BINOP_DIV:
				folded->r = FLT_BINOP_INT(left,/,right);
				break;

			case QSE_AWK_BINOP_IDIV:
				folded->l = (qse_long_t)
					(((qse_awk_nde_int_t*)left)->val / 
					 (qse_flt_t)((qse_awk_nde_int_t*)right)->val);
				fold = QSE_AWK_NDE_INT;
				break;

			case QSE_AWK_BINOP_MOD:
				folded->r = awk->prm.math.mod (
					awk, 
					((qse_awk_nde_flt_t*)left)->val, 
					(qse_flt_t)((qse_awk_nde_int_t*)right)->val
				);
				break;

			default:
				fold = -1;
				break;
		}
	}

	return fold;
}

static qse_awk_nde_t* new_exp_bin_node (	
	qse_awk_t* awk, const qse_awk_loc_t* loc,
	int opcode, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	qse_awk_nde_exp_t* tmp;

	tmp = (qse_awk_nde_exp_t*) QSE_AWK_ALLOC (awk, QSE_SIZEOF(*tmp));
	if (tmp == QSE_NULL)  
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, loc);
		return QSE_NULL;
	}
	
	tmp->type = QSE_AWK_NDE_EXP_BIN;
	tmp->loc = *loc;
	tmp->next = QSE_NULL;
	tmp->opcode = opcode; 
	tmp->left = left;
	tmp->right = right;

	return (qse_awk_nde_t*)tmp;
}

static qse_awk_nde_t* new_int_node (
	qse_awk_t* awk, qse_long_t lv, const qse_awk_loc_t* loc)
{
	qse_awk_nde_int_t* tmp;

	tmp = (qse_awk_nde_int_t*) QSE_AWK_ALLOC (awk, QSE_SIZEOF(*tmp));
	if (tmp == QSE_NULL) 
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, loc);
		return QSE_NULL;
	}

	QSE_MEMSET (tmp, 0, QSE_SIZEOF(*tmp));
	tmp->type = QSE_AWK_NDE_INT;
	tmp->loc = *loc;
	tmp->val = lv;

	return (qse_awk_nde_t*)tmp;
}

static qse_awk_nde_t* new_flt_node (	
	qse_awk_t* awk, qse_flt_t rv, const qse_awk_loc_t* loc)
{
	qse_awk_nde_flt_t* tmp;

	tmp = (qse_awk_nde_flt_t*) QSE_AWK_ALLOC (awk, QSE_SIZEOF(*tmp));
	if (tmp == QSE_NULL) 
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, loc);
		return QSE_NULL;
	}

	QSE_MEMSET (tmp, 0, QSE_SIZEOF(*tmp));
	tmp->type = QSE_AWK_NDE_FLT;
	tmp->loc = *loc;
	tmp->val = rv;

	return (qse_awk_nde_t*)tmp;
}

static QSE_INLINE void update_int_node (
	qse_awk_t* awk, qse_awk_nde_int_t* node, qse_long_t lv)
{
	node->val = lv;
	if (node->str)
	{
		QSE_AWK_FREE (awk, node->str);
		node->str = QSE_NULL;
		node->len = 0;
	}
}

static QSE_INLINE void update_flt_node (
	qse_awk_t* awk, qse_awk_nde_flt_t* node, qse_flt_t rv)
{
	node->val = rv;
	if (node->str)
	{
		QSE_AWK_FREE (awk, node->str);
		node->str = QSE_NULL;
		node->len = 0;
	}
}

static qse_awk_nde_t* parse_binary (
	qse_awk_t* awk, const qse_awk_loc_t* xloc, 
	int skipnl, const binmap_t* binmap,
	qse_awk_nde_t*(*next_level_func)(qse_awk_t*,const qse_awk_loc_t*))
{
	qse_awk_nde_t* left = QSE_NULL; 
	qse_awk_nde_t* right = QSE_NULL;

	left = next_level_func (awk, xloc);
	if (left == QSE_NULL) goto oops;

	do
	{
		qse_awk_loc_t rloc;
		const binmap_t* p = binmap;
		qse_bool_t matched = QSE_FALSE;
		int opcode, fold;
		folded_t folded;

		while (p->token != TOK_EOF)
		{
			if (MATCH(awk,p->token)) 
			{
				opcode = p->binop;
				matched = QSE_TRUE;
				break;
			}
			p++;
		}
		if (!matched) break;

		do 
		{
			if (get_token(awk) <= -1) goto oops;
		}
		while (skipnl && MATCH(awk,TOK_NEWLINE));

		rloc = awk->tok.loc;
		right = next_level_func (awk, &rloc);
		if (right == QSE_NULL) goto oops;

		fold = fold_constants_for_binop (awk, left, right, opcode, &folded);
		switch (fold)
		{
			case QSE_AWK_NDE_INT:
				if (fold == left->type)
				{
					qse_awk_clrpt (awk, right); 
					right = QSE_NULL;
					update_int_node (awk, (qse_awk_nde_int_t*)left, folded.l);
				}
				else if (fold == right->type)
				{
					qse_awk_clrpt (awk, left);
					update_int_node (awk, (qse_awk_nde_int_t*)right, folded.l);
					left = right;
					right = QSE_NULL;
				}
				else 
				{
					qse_awk_clrpt (awk, right); right = QSE_NULL;
					qse_awk_clrpt (awk, left); left = QSE_NULL;
	
					left = new_int_node (awk, folded.l, xloc);
					if (left == QSE_NULL) goto oops;
				}

				break;

			case QSE_AWK_NDE_FLT:
				if (fold == left->type)
				{
					qse_awk_clrpt (awk, right);
					right = QSE_NULL;
					update_flt_node (awk, (qse_awk_nde_flt_t*)left, folded.r);
				}
				else if (fold == right->type)
				{
					qse_awk_clrpt (awk, left); 
					update_flt_node (awk, (qse_awk_nde_flt_t*)right, folded.r);
					left = right; 
					right = QSE_NULL;
				}
				else 
				{
					qse_awk_clrpt (awk, right); right = QSE_NULL;
					qse_awk_clrpt (awk, left); left = QSE_NULL;

					left = new_flt_node (awk, folded.r, xloc);
					if (left == QSE_NULL) goto oops;
				}

				break;

			default:
			{
				qse_awk_nde_t* tmp;

				tmp = new_exp_bin_node (awk, xloc, opcode, left, right);
				if (tmp == QSE_NULL) goto oops;
				left = tmp;
				right = QSE_NULL;
				break;
			}
		}
	}
	while (1);

	return left;

oops:
	if (right) qse_awk_clrpt (awk, right);
	if (left) qse_awk_clrpt (awk, left);
	return QSE_NULL;
}

static qse_awk_nde_t* parse_logical_or (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_LOR, QSE_AWK_BINOP_LOR },
		{ TOK_EOF, 0 }
	};

	return parse_binary (awk, xloc, 1, map, parse_logical_and);
}

static qse_awk_nde_t* parse_logical_and (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_LAND, QSE_AWK_BINOP_LAND },
		{ TOK_EOF,  0 }
	};

	return parse_binary (awk, xloc, 1, map, parse_in);
}

static qse_awk_nde_t* parse_in (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	/* 
	static binmap_t map[] =
	{
		{ TOK_IN, QSE_AWK_BINOP_IN },
		{ TOK_EOF, 0 }
	};

	return parse_binary (awk, xloc, 0, map, parse_regex_match);
	*/

	qse_awk_nde_t* left = QSE_NULL; 
	qse_awk_nde_t* right = QSE_NULL;
	qse_awk_loc_t rloc;

	left = parse_regex_match (awk, xloc);
	if (left == QSE_NULL) goto oops;

	do 
	{
		qse_awk_nde_t* tmp;

		if (!MATCH(awk,TOK_IN)) break;

		if (get_token(awk) <= -1) goto oops;

		rloc = awk->tok.loc;
		right = parse_regex_match (awk, &rloc);
		if (right == QSE_NULL)  goto oops;

		if (!is_plain_var(right))
		{
			SETERR_LOC (awk, QSE_AWK_ENOTVAR, &rloc);
			goto oops;
		}

		tmp = new_exp_bin_node (
			awk, xloc, QSE_AWK_BINOP_IN, left, right);
		if (left == QSE_NULL) goto oops;

		left = tmp;
		right = QSE_NULL;
	}
	while (1);

	return left;

oops:
	if (right) qse_awk_clrpt (awk, right);
	if (left) qse_awk_clrpt (awk, left);
	return QSE_NULL;
}

static qse_awk_nde_t* parse_regex_match (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] =
	{
		{ TOK_TILDE, QSE_AWK_BINOP_MA },
		{ TOK_NM,    QSE_AWK_BINOP_NM },
		{ TOK_EOF,   0 },
	};

	return parse_binary (awk, xloc, 0, map, parse_bitwise_or);
}

static qse_awk_nde_t* parse_bitwise_or (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_BOR, QSE_AWK_BINOP_BOR },
		{ TOK_EOF, 0 }
	};

	return parse_binary (awk, xloc, 0, map, parse_bitwise_xor);
}

static qse_awk_nde_t* parse_bitwise_xor (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_BXOR, QSE_AWK_BINOP_BXOR },
		{ TOK_EOF,  0 }
	};

	return parse_binary (awk, xloc, 0, map, parse_bitwise_and);
}

static qse_awk_nde_t* parse_bitwise_and (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_BAND, QSE_AWK_BINOP_BAND },
		{ TOK_EOF,  0 }
	};

	return parse_binary (awk, xloc, 0, map, parse_equality);
}

static qse_awk_nde_t* parse_equality (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_EQ, QSE_AWK_BINOP_EQ },
		{ TOK_NE, QSE_AWK_BINOP_NE },
		{ TOK_EOF, 0 }
	};

	return parse_binary (awk, xloc, 0, map, parse_relational);
}

static qse_awk_nde_t* parse_relational (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_GT, QSE_AWK_BINOP_GT },
		{ TOK_GE, QSE_AWK_BINOP_GE },
		{ TOK_LT, QSE_AWK_BINOP_LT },
		{ TOK_LE, QSE_AWK_BINOP_LE },
		{ TOK_EOF, 0 }
	};

	return parse_binary (awk, xloc, 0, map, 
		((awk->option & QSE_AWK_EXTRAOPS)? parse_shift: parse_concat));
}

static qse_awk_nde_t* parse_shift (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_LS, QSE_AWK_BINOP_LS },
		{ TOK_RS, QSE_AWK_BINOP_RS },
		{ TOK_EOF, 0 }
	};

	return parse_binary (awk, xloc, 0, map, parse_concat);
}

static qse_awk_nde_t* parse_concat (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* left = QSE_NULL; 
	qse_awk_nde_t* right = QSE_NULL;

	left = parse_additive (awk, xloc);
	if (left == QSE_NULL) goto oops;

	do
	{
		qse_awk_nde_t* tmp;
		qse_awk_loc_t rloc;

		if (MATCH(awk,TOK_CONCAT))
		{
			if (get_token(awk) <= -1) goto oops;
		}
		else if (MATCH(awk,TOK_LPAREN) ||
		         MATCH(awk,TOK_DOLLAR) ||
		         MATCH(awk,TOK_PLUS) ||
		         MATCH(awk,TOK_MINUS) ||
		         MATCH(awk,TOK_PLUSPLUS) ||
		         MATCH(awk,TOK_MINUSMINUS) ||
		         MATCH(awk,TOK_LNOT) ||
		         awk->tok.type >= TOK_GETLINE)
		{
			/* TODO: is the check above sufficient? */
			if (!(awk->option & QSE_AWK_IMPLICIT)) break;
		}
		else break;

		rloc = awk->tok.loc;
		right = parse_additive (awk, &rloc);
		if (right == QSE_NULL) goto oops;

		tmp = new_exp_bin_node (
			awk, xloc, QSE_AWK_BINOP_CONCAT, left, right);
		if (left == QSE_NULL) goto oops;
		left = tmp;
		right = QSE_NULL;
	}
	while (1);

	return left;

oops:
	if (right) qse_awk_clrpt (awk, right);
	if (left) qse_awk_clrpt (awk, left);
	return QSE_NULL;
}

static qse_awk_nde_t* parse_additive (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_PLUS, QSE_AWK_BINOP_PLUS },
		{ TOK_MINUS, QSE_AWK_BINOP_MINUS },
		{ TOK_EOF, 0 }
	};

	return parse_binary (awk, xloc, 0, map, parse_multiplicative);
}

static qse_awk_nde_t* parse_multiplicative (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_MUL,  QSE_AWK_BINOP_MUL },
		{ TOK_DIV,  QSE_AWK_BINOP_DIV },
		{ TOK_IDIV, QSE_AWK_BINOP_IDIV },
		{ TOK_MOD,  QSE_AWK_BINOP_MOD },
		/* { TOK_EXP, QSE_AWK_BINOP_EXP }, */
		{ TOK_EOF, 0 }
	};

	return parse_binary (awk, xloc, 0, map, parse_unary);
}

static qse_awk_nde_t* parse_unary (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* left;
	qse_awk_loc_t uloc;
	int opcode;
	int fold;
	folded_t folded;

	opcode = (MATCH(awk,TOK_PLUS))?  QSE_AWK_UNROP_PLUS:
	         (MATCH(awk,TOK_MINUS))? QSE_AWK_UNROP_MINUS:
	         (MATCH(awk,TOK_LNOT))?  QSE_AWK_UNROP_LNOT:
	         ((awk->option & QSE_AWK_EXTRAOPS) && MATCH(awk,TOK_TILDE))? 
	                                   QSE_AWK_UNROP_BNOT: -1;

	/*if (opcode <= -1) return parse_increment (awk);*/
	if (opcode <= -1) return parse_exponent (awk, xloc);

	if (awk->parse.depth.max.expr > 0 &&
	    awk->parse.depth.cur.expr >= awk->parse.depth.max.expr)
	{
		SETERR_LOC (awk, QSE_AWK_EEXPRNST, xloc);
		return QSE_NULL;
	}

	if (get_token(awk) <= -1) return QSE_NULL;

	awk->parse.depth.cur.expr++;
	uloc = awk->tok.loc;
	left = parse_unary (awk, &uloc);
	awk->parse.depth.cur.expr--;
	if (left == QSE_NULL) return QSE_NULL;

	fold = -1;
	if (left->type == QSE_AWK_NDE_INT)
	{
		fold = QSE_AWK_NDE_INT;
		switch (opcode)
		{
			case QSE_AWK_UNROP_PLUS:
				folded.l = ((qse_awk_nde_int_t*)left)->val;
				break;

			case QSE_AWK_UNROP_MINUS:
				folded.l = -((qse_awk_nde_int_t*)left)->val;
				break;

			case QSE_AWK_UNROP_LNOT:
				folded.l = !((qse_awk_nde_int_t*)left)->val;
				break;

			case QSE_AWK_UNROP_BNOT:
				folded.l = ~((qse_awk_nde_int_t*)left)->val;
				break;

			default:
				fold = -1;
				break;
		}
	}
	else if (left->type == QSE_AWK_NDE_FLT)
	{
		fold = QSE_AWK_NDE_FLT;
		switch (opcode)
		{
			case QSE_AWK_UNROP_PLUS:
				folded.r = ((qse_awk_nde_flt_t*)left)->val;
				break;

			case QSE_AWK_UNROP_MINUS:
				folded.r = -((qse_awk_nde_flt_t*)left)->val;
				break;

			case QSE_AWK_UNROP_LNOT:
				folded.r = !((qse_awk_nde_flt_t*)left)->val;
				break;

			case QSE_AWK_UNROP_BNOT:
				folded.l = ~((qse_long_t)((qse_awk_nde_flt_t*)left)->val);
				fold = QSE_AWK_NDE_INT;
				break;

			default:
				fold = -1;
				break;
		}
	}

	switch (fold)
	{
		case QSE_AWK_NDE_INT:
			if (left->type == fold)
			{
				update_int_node (awk, (qse_awk_nde_int_t*)left, folded.l);
				return left;
			}
			else
			{
				QSE_ASSERT (left->type == QSE_AWK_NDE_FLT);
				qse_awk_clrpt (awk, left);
				return new_int_node (awk, folded.l, xloc);
			}

		case QSE_AWK_NDE_FLT:
			if (left->type == fold)
			{
				update_flt_node (awk, (qse_awk_nde_flt_t*)left, folded.r);
				return left;
			}
			else
			{
				QSE_ASSERT (left->type == QSE_AWK_NDE_INT);
				qse_awk_clrpt (awk, left);
				return new_flt_node (awk, folded.r, xloc);
			}

		default:
		{
			qse_awk_nde_exp_t* nde; 

			nde = (qse_awk_nde_exp_t*) 
				QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_exp_t));
			if (nde == QSE_NULL)
			{
				qse_awk_clrpt (awk, left);
				SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
				return QSE_NULL;
			}

			nde->type = QSE_AWK_NDE_EXP_UNR;
			nde->loc = *xloc;
			nde->next = QSE_NULL;
			nde->opcode = opcode;
			nde->left = left;
			nde->right = QSE_NULL;
	
			return (qse_awk_nde_t*)nde;
		}
	}
}

static qse_awk_nde_t* parse_exponent (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	static binmap_t map[] = 
	{
		{ TOK_EXP, QSE_AWK_BINOP_EXP },
		{ TOK_EOF, 0 }
	};

	return parse_binary (awk, xloc, 0, map, parse_unary_exp);
}

static qse_awk_nde_t* parse_unary_exp (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_exp_t* nde; 
	qse_awk_nde_t* left;
	qse_awk_loc_t uloc;
	int opcode;

	opcode = (MATCH(awk,TOK_PLUS))?  QSE_AWK_UNROP_PLUS:
	         (MATCH(awk,TOK_MINUS))? QSE_AWK_UNROP_MINUS:
	         (MATCH(awk,TOK_LNOT))?  QSE_AWK_UNROP_LNOT:
	         ((awk->option & QSE_AWK_EXTRAOPS) && MATCH(awk,TOK_TILDE))? 
	                                   QSE_AWK_UNROP_BNOT: -1;

	if (opcode <= -1) return parse_increment (awk, xloc);

	if (awk->parse.depth.max.expr > 0 &&
	    awk->parse.depth.cur.expr >= awk->parse.depth.max.expr)
	{
		SETERR_LOC (awk, QSE_AWK_EEXPRNST, xloc);
		return QSE_NULL;
	}

	if (get_token(awk) <= -1) return QSE_NULL;

	awk->parse.depth.cur.expr++;
	uloc = awk->tok.loc;
	left = parse_unary (awk, &uloc);
	awk->parse.depth.cur.expr--;
	if (left == QSE_NULL) return QSE_NULL;

	nde = (qse_awk_nde_exp_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_exp_t));
	if (nde == QSE_NULL)
	{
		qse_awk_clrpt (awk, left);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_EXP_UNR;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = QSE_NULL;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_increment (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_exp_t* nde;
	qse_awk_nde_t* left;
	int type, opcode, opcode1, opcode2;
	qse_awk_loc_t ploc;

	/* check for prefix increment operator */
	opcode1 = MATCH(awk,TOK_PLUSPLUS)? QSE_AWK_INCOP_PLUS:
	          MATCH(awk,TOK_MINUSMINUS)? QSE_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1)
	{
		/* there is a prefix increment operator */
		if (get_token(awk) <= -1) return QSE_NULL;
	}

	ploc = awk->tok.loc;
	left = parse_primary (awk, &ploc);
	if (left == QSE_NULL) return QSE_NULL;

	/* check for postfix increment operator */
	opcode2 = MATCH(awk,TOK_PLUSPLUS)? QSE_AWK_INCOP_PLUS:
	          MATCH(awk,TOK_MINUSMINUS)? QSE_AWK_INCOP_MINUS: -1;

	if ((awk->option & QSE_AWK_EXPLICIT) && !(awk->option & QSE_AWK_IMPLICIT))
	{	
		if (opcode1 != -1 && opcode2 != -1)
		{
			/* both prefix and postfix increment operator. 
			 * not allowed */
			qse_awk_clrpt (awk, left);
			SETERR_LOC (awk, QSE_AWK_EPREPST, xloc);
			return QSE_NULL;
		}
	}

	if (opcode1 == -1 && opcode2 == -1)
	{
		/* no increment operators */
		return left;
	}
	else if (opcode1 != -1) 
	{
		/* prefix increment operator.
		 * ignore a potential postfix operator */
		type = QSE_AWK_NDE_EXP_INCPRE;
		opcode = opcode1;
	}
	else if (opcode2 != -1) 
	{
		/* postfix increment operator */
		type = QSE_AWK_NDE_EXP_INCPST;
		opcode = opcode2;

		/* let's do it later 
		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL;
		}
		*/
	}

	if (!is_var(left) && left->type != QSE_AWK_NDE_POS)
	{
		if (type == QSE_AWK_NDE_EXP_INCPST)
		{
			/* For an expression like 1 ++y,
			 * left is 1. so we leave ++ for y. */
			return left;
		}
		else
		{
			qse_awk_clrpt (awk, left);
			SETERR_LOC (awk, QSE_AWK_EINCDECOPR, xloc);
			return QSE_NULL;
		}
	}

	if (type == QSE_AWK_NDE_EXP_INCPST)
	{
		/* consume the postfix operator */
		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL;
		}
	}

	nde = (qse_awk_nde_exp_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_exp_t));
	if (nde == QSE_NULL)
	{
		qse_awk_clrpt (awk, left);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = type;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = QSE_NULL;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_primary_nogetline (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	if (MATCH(awk,TOK_IDENT))  
	{
		return parse_primary_ident (awk, xloc);
	}
	else if (MATCH(awk,TOK_INT)) 
	{
		qse_awk_nde_int_t* nde;

		/* create the node for the literal */
		nde = (qse_awk_nde_int_t*)new_int_node (
			awk, 
			qse_awk_strxtolong (awk, 
				QSE_STR_PTR(awk->tok.name),
				QSE_STR_LEN(awk->tok.name), 
				0, QSE_NULL
			),
			xloc
		);
		if (nde == QSE_NULL) return QSE_NULL;

		/* remember the literal in the original form */
		nde->str = QSE_AWK_STRXDUP (awk,
			QSE_STR_PTR(awk->tok.name),
			QSE_STR_LEN(awk->tok.name));
		if (nde->str == QSE_NULL)
		{
			QSE_AWK_FREE (awk, nde);
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;			
		}
		nde->len = QSE_STR_LEN(awk->tok.name);

		QSE_ASSERT (
			QSE_STR_LEN(awk->tok.name) ==
			qse_strlen(QSE_STR_PTR(awk->tok.name)));

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, nde->str);
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;			
		}

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOK_FLT)) 
	{
		qse_awk_nde_flt_t* nde;

		/* create the node for the literal */
		nde = (qse_awk_nde_flt_t*) new_flt_node (
			awk, 
			qse_awk_strxtoflt (awk, 
				QSE_STR_PTR(awk->tok.name), 
				QSE_STR_LEN(awk->tok.name),
				QSE_NULL
			),
			xloc
		);
		if (nde == QSE_NULL) return QSE_NULL;

		/* remember the literal in the original form */
		nde->str = QSE_AWK_STRXDUP (awk,
			QSE_STR_PTR(awk->tok.name),
			QSE_STR_LEN(awk->tok.name));
		if (nde->str == QSE_NULL)
		{
			QSE_AWK_FREE (awk, nde);
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;			
		}
		nde->len = QSE_STR_LEN(awk->tok.name);

		QSE_ASSERT (
			QSE_STR_LEN(awk->tok.name) ==
			qse_strlen(QSE_STR_PTR(awk->tok.name)));

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, nde->str);
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;			
		}

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOK_STR))  
	{
		qse_awk_nde_str_t* nde;

		nde = (qse_awk_nde_str_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_str_t));
		if (nde == QSE_NULL)
		{
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_STR;
		nde->loc = *xloc;
		nde->next = QSE_NULL;
		nde->len = QSE_STR_LEN(awk->tok.name);
		nde->ptr = QSE_AWK_STRXDUP (awk,
			QSE_STR_PTR(awk->tok.name), nde->len);
		if (nde->ptr == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, nde);
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, nde->ptr);
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;			
		}

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOK_DIV) || MATCH(awk,TOK_DIV_ASSN))
	{
		qse_awk_nde_rex_t* nde;
		qse_awk_errnum_t errnum;

		/* the regular expression is tokenized here because 
		 * of the context-sensitivity of the slash symbol.
		 * if TOK_DIV is seen as a primary, it tries to compile
		 * it as a regular expression */
		qse_str_clear (awk->tok.name);

		if (MATCH(awk,TOK_DIV_ASSN) &&
 		   qse_str_ccat (awk->tok.name, QSE_T('=')) == (qse_size_t)-1)
		{
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		SET_TOKEN_TYPE (awk, &awk->tok, TOK_REX);
		if (get_rexstr (awk, &awk->tok) <= -1) return QSE_NULL;

		QSE_ASSERT (MATCH(awk,TOK_REX));

		nde = (qse_awk_nde_rex_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_rex_t));
		if (nde == QSE_NULL)
		{
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_REX;
		nde->loc = *xloc;
		nde->next = QSE_NULL;

		nde->len = QSE_STR_LEN(awk->tok.name);
		nde->ptr = QSE_AWK_STRXDUP (awk,
			QSE_STR_PTR(awk->tok.name),
			QSE_STR_LEN(awk->tok.name));
		if (nde->ptr == QSE_NULL)
		{
			QSE_AWK_FREE (awk, nde);
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		nde->code = QSE_AWK_BUILDREX (awk,
			QSE_STR_PTR(awk->tok.name), 
			QSE_STR_LEN(awk->tok.name), 
			&errnum);
		if (nde->code == QSE_NULL)
		{
			QSE_AWK_FREE (awk, nde->ptr);
			QSE_AWK_FREE (awk, nde);
			SETERR_LOC (awk, errnum, xloc);
			return QSE_NULL;
		}

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREEREX (awk, nde->code);
			QSE_AWK_FREE (awk, nde->ptr);
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;			
		}

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOK_DOLLAR)) 
	{
		qse_awk_nde_pos_t* nde;
		qse_awk_nde_t* prim;

		if (get_token(awk)) return QSE_NULL;
		
		{
			qse_awk_loc_t ploc = awk->tok.loc;
			prim = parse_primary (awk, &ploc);
		}
		if (prim == QSE_NULL) return QSE_NULL;

		nde = (qse_awk_nde_pos_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_pos_t));
		if (nde == QSE_NULL) 
		{
			qse_awk_clrpt (awk, prim);
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_POS;
		nde->loc = *xloc;
		nde->next = QSE_NULL;
		nde->val = prim;

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOK_LPAREN)) 
	{
		qse_awk_nde_t* nde;
		qse_awk_nde_t* last;

		/* eat up the left parenthesis */
		if (get_token(awk) <= -1) return QSE_NULL;

		/* parse the sub-expression inside the parentheses */
		{
			qse_awk_loc_t eloc = awk->tok.loc;
			nde = parse_expr_dc (awk, &eloc);
		}
		if (nde == QSE_NULL) return QSE_NULL;

		/* parse subsequent expressions separated by a comma, if any */
		last = nde;
		QSE_ASSERT (last->next == QSE_NULL);

		while (MATCH(awk,TOK_COMMA))
		{
			qse_awk_nde_t* tmp;

			do
			{
				if (get_token(awk) <= -1) 
				{
					qse_awk_clrpt (awk, nde);
					return QSE_NULL;
				}	
			}
			while (MATCH(awk,TOK_NEWLINE));

			{
				qse_awk_loc_t eloc = awk->tok.loc;
				tmp = parse_expr_dc (awk, &eloc);
			}
			if (tmp == QSE_NULL) 
			{
				qse_awk_clrpt (awk, nde);
				return QSE_NULL;
			}

			QSE_ASSERT (tmp->next == QSE_NULL);
			last->next = tmp;
			last = tmp;
		} 
		/* ----------------- */

		/* check for the closing parenthesis */
		if (!MATCH(awk,TOK_RPAREN)) 
		{
			qse_awk_clrpt (awk, nde);

			SETERR_TOK (awk, QSE_AWK_ERPAREN);
			return QSE_NULL;
		}

		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, nde);
			return QSE_NULL;
		}

		/* check if it is a chained node */
		if (nde->next != QSE_NULL)
		{
			/* if so, it is a expression group */
			/* (expr1, expr2, expr2) */

			qse_awk_nde_grp_t* tmp;

			if ((awk->parse.id.stmt != TOK_PRINT &&
			     awk->parse.id.stmt != TOK_PRINTF) ||
			    awk->parse.depth.cur.expr != 1)
			{
				if (!MATCH(awk,TOK_IN))
				{
					qse_awk_clrpt (awk, nde);
					SETERR_TOK (awk, QSE_AWK_EKWIN);
					return QSE_NULL;
				}
			}

			tmp = (qse_awk_nde_grp_t*) QSE_AWK_ALLOC (
				awk, QSE_SIZEOF(qse_awk_nde_grp_t));
			if (tmp == QSE_NULL)
			{
				qse_awk_clrpt (awk, nde);
				SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
				return QSE_NULL;
			}	

			tmp->type = QSE_AWK_NDE_GRP;
			tmp->loc = *xloc;
			tmp->next = QSE_NULL;
			tmp->body = nde;		

			nde = (qse_awk_nde_t*)tmp;
		}
		/* ----------------- */

		return nde;
	}
	else if (MATCH(awk,TOK_GETLINE)) 
	{
		qse_awk_nde_getline_t* nde;
		qse_awk_nde_t* var = QSE_NULL;
		qse_awk_nde_t* in = QSE_NULL;

		if (get_token(awk) <= -1) return QSE_NULL;

		if (MATCH(awk,TOK_IDENT))
		{
			/* getline var */
			qse_awk_loc_t gloc = awk->tok.loc;
			var = parse_primary (awk, &gloc);
			if (var == QSE_NULL) return QSE_NULL;
		}

		if (MATCH(awk, TOK_LT))
		{
			/* getline [var] < file */
			if (get_token(awk) <= -1)
			{
				if (var != QSE_NULL) qse_awk_clrpt (awk, var);
				return QSE_NULL;
			}

			{
				qse_awk_loc_t ploc = awk->tok.loc;
				/* TODO: is this correct? */
				/*in = parse_expr_dc (awk, &ploc);*/
				in = parse_primary (awk, &ploc);
			}
			if (in == QSE_NULL)
			{
				if (var != QSE_NULL) qse_awk_clrpt (awk, var);
				return QSE_NULL;
			}
		}

		nde = (qse_awk_nde_getline_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_getline_t));
		if (nde == QSE_NULL)
		{
			if (var != QSE_NULL) qse_awk_clrpt (awk, var);
			if (in != QSE_NULL) qse_awk_clrpt (awk, in);
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_GETLINE;
		nde->loc = *xloc;
		nde->next = QSE_NULL;
		nde->var = var;
		nde->in_type = (in == QSE_NULL)? 
			QSE_AWK_IN_CONSOLE: QSE_AWK_IN_FILE;
		nde->in = in;

		return (qse_awk_nde_t*)nde;
	}

	/* valid expression introducer is expected */
	if (MATCH(awk,TOK_NEWLINE))
	{
		SETERR_ARG_LOC (
			awk, QSE_AWK_EEXPRNR, 
			QSE_STR_PTR(awk->ptok.name), 
			QSE_STR_LEN(awk->ptok.name),
			&awk->ptok.loc
		);
	}
	else 
	{
		SETERR_TOK (awk, QSE_AWK_EEXPRNR);
	}

	return QSE_NULL;
}

static qse_awk_nde_t* parse_primary (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* left;
	qse_awk_nde_getline_t* nde;
	qse_awk_nde_t* var;

	left = parse_primary_nogetline (awk, xloc);

	do
	{
		int intype = -1;

		if (awk->option & QSE_AWK_RIO)
		{
			if (MATCH(awk,TOK_BOR)) 
			{
				intype = QSE_AWK_IN_PIPE;
			}
			else if (MATCH(awk,TOK_LOR)) 
			{
				if (awk->option & QSE_AWK_RWPIPE) 
					intype = QSE_AWK_IN_RWPIPE;
			}
		}
		
		if (intype == -1) break;

		if (preget_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL;
		}
	
		if (awk->ntok.type != TOK_GETLINE) break;

		var = QSE_NULL;

		/* consume ntok */
		get_token (awk);

		/* get the next token */
		if (get_token(awk) <= -1)
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL;
		}

		/* TODO: is this correct? */
		if (MATCH(awk,TOK_IDENT))
		{
			/* command | getline var 
			 * command || getline var */
			qse_awk_loc_t gloc = awk->tok.loc;
			var = parse_primary_ident (awk, &gloc);
			if (var == QSE_NULL) 
			{
				qse_awk_clrpt (awk, left);
				return QSE_NULL;
			}
		}

		nde = (qse_awk_nde_getline_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_getline_t));
		if (nde == QSE_NULL)
		{
			qse_awk_clrpt (awk, left);
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_GETLINE;
		nde->loc = *xloc;
		nde->next = QSE_NULL;
		nde->var = var;
		nde->in_type = intype;
		nde->in = left;

		left = (qse_awk_nde_t*)nde;
	}
	while (1);

	return left;
}

#define FNTYPE_UNKNOWN 0
#define FNTYPE_FNC 1
#define FNTYPE_FUN 2

static QSE_INLINE int isfunname (qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	/* check if it is an awk function being processed currently */
	if (awk->tree.cur_fun.ptr != QSE_NULL)
	{
		if (qse_strxncmp (
			awk->tree.cur_fun.ptr, awk->tree.cur_fun.len,
			name, len) == 0)
		{
			/* the current function begin parsed */
			return FNTYPE_FUN;
		}
	}

	/* check the funtion name in the function table */
	if (qse_htb_search (awk->tree.funs, name, len) != QSE_NULL)
	{
		/* one of the functions defined previously */
		return FNTYPE_FUN;
	}

	/* check if it is a function not resolved so far */
	if (qse_htb_search (awk->parse.funs, name, len) != QSE_NULL) 
	{
		/* one of the function calls not resolved so far. */ 
		return FNTYPE_FUN;
	}

	return FNTYPE_UNKNOWN;
}

static QSE_INLINE int isfnname (qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	if (qse_awk_getfnc (awk, name, len) != QSE_NULL) 
	{
		/* implicit function */
		return FNTYPE_FNC;
	}

	return isfunname (awk, name, len);
}

static qse_awk_nde_t* parse_variable (
	qse_awk_t* awk, const qse_awk_loc_t* xloc, qse_awk_nde_type_t type,
	qse_char_t* nameptr, qse_size_t namelen, qse_size_t idxa)
{
	qse_awk_nde_var_t* nde;

	if ((awk->option & QSE_AWK_EXPLICIT) && 
	    !(awk->option & QSE_AWK_IMPLICIT))
	{
		/* if explicit only, the concatenation operator(.)
		 * must be used. so it is obvious that it is a function
		 * call, which is illegal for a variable. 
		 * if implicit, "var_xxx (1)" may be concatenation of
		 * the value of var_xxx and 1.
		 */
		if (MATCH(awk,TOK_LPAREN))
		{
			/* a variable is not a function */
			SETERR_ARG_LOC (
				awk, QSE_AWK_EFUNNAM,
				nameptr, namelen, xloc);
			return QSE_NULL;
		}
	}

	nde = (qse_awk_nde_var_t*) QSE_AWK_ALLOC (
		awk, QSE_SIZEOF(qse_awk_nde_var_t));
	if (nde == QSE_NULL) 
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	nde->type = type;
	nde->loc = *xloc;
	nde->next = QSE_NULL;
	/*nde->id.name.ptr = QSE_NULL;*/
	nde->id.name.ptr = nameptr;
	nde->id.name.len = namelen;
	nde->id.idxa = idxa;
	nde->idx = QSE_NULL;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_primary_ident (
	qse_awk_t* awk, const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* nde = QSE_NULL;
	qse_char_t* namedup;
	qse_size_t namelen;
	qse_awk_fnc_t* fnc;
	qse_size_t idxa;

	QSE_ASSERT (MATCH(awk,TOK_IDENT));

	namedup = QSE_AWK_STRXDUP (awk,
		QSE_STR_PTR(awk->tok.name),
		QSE_STR_LEN(awk->tok.name));
	if (namedup == QSE_NULL) 
	{
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}
	namelen = QSE_STR_LEN(awk->tok.name);

	if (get_token(awk) <= -1) 
	{
		QSE_AWK_FREE (awk, namedup);
		return QSE_NULL;			
	}

	/* check if namedup is an intrinsic function name */
	fnc = qse_awk_getfnc (awk, namedup, namelen);
	if (fnc != QSE_NULL)
	{
		if (MATCH(awk,TOK_LPAREN))
		{
			nde = parse_fncall (awk, namedup, namelen, fnc, xloc, 0);
		}
		else
		{
			if (fnc->dfl0)
			{
				/* handles a function that assumes () 
				 * when () is missing. i.e. length */
				nde = parse_fncall (
					awk, namedup, namelen, fnc, xloc, 1);
			}
			else
			{
				/* an intrinsic function should be in the form 
		 		 * of the function call */
				SETERR_TOK (awk, QSE_AWK_ELPAREN);
			}
		}
	}
	/* now we know that namedup is a normal identifier. */
	else if (MATCH(awk,TOK_LBRACK)) 
	{
		nde = parse_hashidx (awk, namedup, namelen, xloc);
	}
	else if ((idxa = qse_lda_rsearch (awk->parse.lcls, QSE_LDA_SIZE(awk->parse.lcls), namedup, namelen)) != QSE_LDA_NIL)
	{
		/* local variable */
		nde = parse_variable (
			awk, xloc, QSE_AWK_NDE_LCL,
			namedup, namelen, idxa);
	}
	else if ((idxa = qse_lda_search (awk->parse.params, 0, namedup, namelen)) != QSE_LDA_NIL)
	{
		/* parameter */
		nde = parse_variable (
			awk, xloc, QSE_AWK_NDE_ARG,
			namedup, namelen, idxa);
	}
	else if ((idxa = get_global (awk, namedup, namelen)) != QSE_LDA_NIL)
	{
		/* global variable */
		nde = parse_variable (
			awk, xloc, QSE_AWK_NDE_GBL,
			namedup, namelen, idxa);
	}
	else
	{
		int fntype = isfunname (awk, namedup, namelen);

		if (fntype)
		{
			QSE_ASSERT (fntype == FNTYPE_FUN);

			if (MATCH(awk,TOK_LPAREN))
			{
				/* must be a function name */
				QSE_ASSERT (qse_htb_search (
					awk->parse.named, namedup, namelen) == QSE_NULL);

				nde = parse_fncall (
					awk, namedup, namelen, QSE_NULL, xloc,  0);
			}
			else
			{
				/* function name appeared without () */
				SETERR_ARG_LOC (
					awk, QSE_AWK_EFUNRED, 
					namedup, namelen, xloc
				);
			}
		}
		else if (awk->option & QSE_AWK_IMPLICIT) 
		{
			/* if the name is followed by ( without no spaces 
			 * in the implicit mode, the name is considered a function
			 * name though it has not been seen/resolved */

			if (MATCH(awk,TOK_LPAREN) &&
			    awk->tok.loc.line == xloc->line &&
			    awk->tok.loc.colm == xloc->colm + namelen)
			{
				/* a function call to a yet undefined function */

				if (qse_htb_search (
					awk->parse.named, namedup, namelen) != QSE_NULL)
				{
					/* a function call conflicts with a named variable */
					SETERR_ARG_LOC (
						awk, QSE_AWK_EVARRED,
						namedup, namelen, xloc
					);
				}
				else
				{
					nde = parse_fncall (
						awk, namedup, namelen, QSE_NULL, xloc, 0);
				}
			}
			else
			{
				qse_awk_nde_var_t* tmp;

				/* named variable */
				tmp = (qse_awk_nde_var_t*) QSE_AWK_ALLOC (
					awk, QSE_SIZEOF(qse_awk_nde_var_t));
				if (tmp == QSE_NULL) 
				{
					SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
				}
				else
				{
					/* collect unique instances of a named variable 
					 * for reference */
					if (qse_htb_upsert (
						awk->parse.named, 
						namedup, namelen, QSE_NULL, 0) == QSE_NULL)
					{
						SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
						QSE_AWK_FREE (awk, tmp);
					}
					else
					{
						tmp->type = QSE_AWK_NDE_NAMED;
						tmp->loc = *xloc;
						tmp->next = QSE_NULL;
						tmp->id.name.ptr = namedup;
						tmp->id.name.len = namelen;
						tmp->id.idxa = (qse_size_t)-1;
						tmp->idx = QSE_NULL;

						nde = (qse_awk_nde_t*)tmp;
					}
				}
			}
		}
		else
		{
			if (MATCH(awk,TOK_LPAREN))
			{
				/* it is a function call as the name is followed 
				 * by ( and implicit variables are disabled. */
				nde = parse_fncall (
					awk, namedup, namelen, QSE_NULL, xloc,  0);
			}
			else
			{
				/* undefined variable */
				SETERR_ARG_LOC (	
					awk, QSE_AWK_EUNDEF, namedup, namelen, xloc
				);
			}
		}
	}

	if (nde == QSE_NULL) QSE_AWK_FREE (awk, namedup);
	return nde;
}

static qse_awk_nde_t* parse_hashidx (
	qse_awk_t* awk, qse_char_t* name, qse_size_t namelen, 
	const qse_awk_loc_t* xloc)
{
	qse_awk_nde_t* idx, * tmp, * last;
	qse_awk_nde_var_t* nde;
	qse_size_t idxa;

	idx = QSE_NULL;
	last = QSE_NULL;

	do
	{
		if (get_token(awk) <= -1) 
		{
			if (idx != QSE_NULL) qse_awk_clrpt (awk, idx);
			return QSE_NULL;
		}

		{
			qse_awk_loc_t eloc = awk->tok.loc;
			tmp = parse_expr_dc (awk, &eloc);
		}
		if (tmp == QSE_NULL) 
		{
			if (idx != QSE_NULL) qse_awk_clrpt (awk, idx);
			return QSE_NULL;
		}

		if (idx == QSE_NULL)
		{
			QSE_ASSERT (last == QSE_NULL);
			idx = tmp; last = tmp;
		}
		else
		{
			last->next = tmp;
			last = tmp;
		}
	}
	while (MATCH(awk,TOK_COMMA));

	QSE_ASSERT (idx != QSE_NULL);

	if (!MATCH(awk,TOK_RBRACK)) 
	{
		qse_awk_clrpt (awk, idx);
		SETERR_TOK (awk, QSE_AWK_ERBRACK);
		return QSE_NULL;
	}

	if (get_token(awk) <= -1) 
	{
		qse_awk_clrpt (awk, idx);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_var_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_var_t));
	if (nde == QSE_NULL) 
	{
		qse_awk_clrpt (awk, idx);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	/* search the local variable list */
	idxa = qse_lda_rsearch (
		awk->parse.lcls, 
		QSE_LDA_SIZE(awk->parse.lcls),
		name,
		namelen
	);
	if (idxa != QSE_LDA_NIL)
	{
		nde->type = QSE_AWK_NDE_LCLIDX;
		nde->loc = *xloc;
		nde->next = QSE_NULL;
		/*nde->id.name = QSE_NULL; */
		nde->id.name.ptr = name;
		nde->id.name.len = namelen;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (qse_awk_nde_t*)nde;
	}

	/* search the parameter name list */
	idxa = qse_lda_search (awk->parse.params, 0, name, namelen);
	if (idxa != QSE_LDA_NIL)
	{
		nde->type = QSE_AWK_NDE_ARGIDX;
		nde->loc = *xloc;
		nde->next = QSE_NULL;
		/*nde->id.name = QSE_NULL; */
		nde->id.name.ptr = name;
		nde->id.name.len = namelen;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (qse_awk_nde_t*)nde;
	}

	/* gets the global variable index */
	idxa = get_global (awk, name, namelen);
	if (idxa != QSE_LDA_NIL)
	{
		nde->type = QSE_AWK_NDE_GBLIDX;
		nde->loc = *xloc;
		nde->next = QSE_NULL;
		/*nde->id.name = QSE_NULL;*/
		nde->id.name.ptr = name;
		nde->id.name.len = namelen;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (qse_awk_nde_t*)nde;
	}

	if (awk->option & QSE_AWK_IMPLICIT) 
	{
		int fnname = isfnname (awk, name, namelen);
		switch (fnname)
		{
			case FNTYPE_FNC:
				SETERR_ARG_LOC (	
					awk, QSE_AWK_EFNCRED, name, namelen, xloc);
				goto exit_func;

			case FNTYPE_FUN:
				SETERR_ARG_LOC (	
					awk, QSE_AWK_EFUNRED, name, namelen, xloc);
				goto exit_func;
		}

		QSE_ASSERT (fnname == 0);

		nde->type = QSE_AWK_NDE_NAMEDIDX;
		nde->loc = *xloc;
		nde->next = QSE_NULL;
		nde->id.name.ptr = name;
		nde->id.name.len = namelen;
		nde->id.idxa = (qse_size_t)-1;
		nde->idx = idx;

		return (qse_awk_nde_t*)nde;
	}

	/* undefined variable */
	SETERR_ARG_LOC (awk, QSE_AWK_EUNDEF, name, namelen, xloc);

exit_func:
	qse_awk_clrpt (awk, idx);
	QSE_AWK_FREE (awk, nde);

	return QSE_NULL;
}

static qse_awk_nde_t* parse_fncall (
	qse_awk_t* awk, qse_char_t* name, qse_size_t namelen, 
	qse_awk_fnc_t* fnc, const qse_awk_loc_t* xloc, int noarg)
{
	qse_awk_nde_t* head, * curr, * nde;
	qse_awk_nde_fncall_t* call;
	qse_size_t nargs;

	head = curr = QSE_NULL;
	nargs = 0;

	if (noarg) goto make_node;
	if (get_token(awk) <= -1) return QSE_NULL;

	if (MATCH(awk,TOK_RPAREN)) 
	{
		/* no parameters to the function call */
		if (get_token(awk) <= -1) return QSE_NULL;
	}
	else 
	{
		/* parse function parameters */

		while (1) 
		{
			{
				qse_awk_loc_t eloc = awk->tok.loc;
				nde = parse_expr_dc (awk, &eloc);
			}
			if (nde == QSE_NULL) 
			{
				if (head != QSE_NULL) qse_awk_clrpt (awk, head);
				return QSE_NULL;
			}
	
			if (head == QSE_NULL) head = nde;
			else curr->next = nde;
			curr = nde;

			nargs++;

			if (MATCH(awk,TOK_RPAREN)) 
			{
				if (get_token(awk) <= -1) 
				{
					if (head != QSE_NULL) 
						qse_awk_clrpt (awk, head);
					return QSE_NULL;
				}
				break;
			}

			if (!MATCH(awk,TOK_COMMA)) 
			{
				if (head != QSE_NULL)
					qse_awk_clrpt (awk, head);
				SETERR_TOK (awk, QSE_AWK_ECOMMA);
				return QSE_NULL;
			}

			do
			{
				if (get_token(awk) <= -1) 
				{
					if (head != QSE_NULL)
						qse_awk_clrpt (awk, head);
					return QSE_NULL;
				}
			}
			while (MATCH(awk,TOK_NEWLINE));
		}

	}

make_node:
	call = (qse_awk_nde_fncall_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_fncall_t));
	if (call == QSE_NULL) 
	{
		if (head != QSE_NULL) qse_awk_clrpt (awk, head);
		SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
		return QSE_NULL;
	}

	if (fnc != QSE_NULL)
	{
		call->type = QSE_AWK_NDE_FNC;
		call->loc = *xloc;
		call->next = QSE_NULL;

		/*call->u.fnc = fnc; */
		call->u.fnc.name.ptr = name;
		call->u.fnc.name.len = namelen;
		call->u.fnc.arg.min = fnc->arg.min;
		call->u.fnc.arg.max = fnc->arg.max;
		call->u.fnc.arg.spec = fnc->arg.spec;
		call->u.fnc.handler = fnc->handler;

		call->args = head;
		call->nargs = nargs;
	}
	else
	{
		call->type = QSE_AWK_NDE_FUN;
		call->loc = *xloc;
		call->next = QSE_NULL;
		call->u.fun.name.ptr = name; 
		call->u.fun.name.len = namelen;
		call->args = head;
		call->nargs = nargs;

		/* store a non-builtin function call into the awk->parse.funs 
		 * table */
		if (qse_htb_upsert (
			awk->parse.funs, name, namelen, call, 0) == QSE_NULL)
		{
			QSE_AWK_FREE (awk, call);
			if (head != QSE_NULL) qse_awk_clrpt (awk, head);
			SETERR_LOC (awk, QSE_AWK_ENOMEM, xloc);
			return QSE_NULL;
		}
	}

	return (qse_awk_nde_t*)call;
}

static int get_number (qse_awk_t* awk, qse_awk_tok_t* tok)
{
	qse_cint_t c;

	QSE_ASSERT (QSE_STR_LEN(tok->name) == 0);
	SET_TOKEN_TYPE (awk, tok, TOK_INT);

	c = awk->sio.last.c;

	if (c == QSE_T('0'))
	{
		ADD_TOKEN_CHAR (awk, tok, c);
		GET_CHAR_TO (awk, c);

		if (c == QSE_T('x') || c == QSE_T('X'))
		{
			/* hexadecimal number */
			do 
			{
				ADD_TOKEN_CHAR (awk, tok, c);
				GET_CHAR_TO (awk, c);
			} 
			while (QSE_AWK_ISXDIGIT (awk, c));

			return 0;
		}
		else if (c == QSE_T('b') || c == QSE_T('B'))
		{
			/* binary number */
			do
			{
				ADD_TOKEN_CHAR (awk, tok, c);
				GET_CHAR_TO (awk, c);
			} 
			while (c == QSE_T('0') || c == QSE_T('1'));

			return 0;
		}
		else if (c != '.')
		{
			/* octal number */
			while (c >= QSE_T('0') && c <= QSE_T('7'))
			{
				ADD_TOKEN_CHAR (awk, tok, c);
				GET_CHAR_TO (awk, c);
			}

			if (c == QSE_T('8') || c == QSE_T('9'))
			{
				qse_char_t cc = (qse_char_t)c;
				SETERR_ARG_LOC (
					awk, QSE_AWK_ELXDIG, 
					&cc, 1, &awk->tok.loc);
				return -1;
			}

			return 0;
		}
	}

	while (QSE_AWK_ISDIGIT (awk, c)) 
	{
		ADD_TOKEN_CHAR (awk, tok, c);
		GET_CHAR_TO (awk, c);
	} 

	if (c == QSE_T('.'))
	{
		/* floating-point number */
		SET_TOKEN_TYPE (awk, tok, TOK_FLT);

		ADD_TOKEN_CHAR (awk, tok, c);
		GET_CHAR_TO (awk, c);

		while (QSE_AWK_ISDIGIT (awk, c))
		{
			ADD_TOKEN_CHAR (awk, tok, c);
			GET_CHAR_TO (awk, c);
		}
	}

	if (c == QSE_T('E') || c == QSE_T('e'))
	{
		SET_TOKEN_TYPE (awk, tok, TOK_FLT);

		ADD_TOKEN_CHAR (awk, tok, c);
		GET_CHAR_TO (awk, c);

		if (c == QSE_T('+') || c == QSE_T('-'))
		{
			ADD_TOKEN_CHAR (awk, tok, c);
			GET_CHAR_TO (awk, c);
		}

		while (QSE_AWK_ISDIGIT (awk, c))
		{
			ADD_TOKEN_CHAR (awk, tok, c);
			GET_CHAR_TO (awk, c);
		}
	}

	return 0;
}

static int get_string (
	qse_awk_t* awk, qse_char_t end_char, 
	qse_char_t esc_char, qse_bool_t keep_esc_char,
	int preescaped, qse_awk_tok_t* tok)
{
	qse_cint_t c;
	int escaped = preescaped;
	int digit_count = 0;
	qse_cint_t c_acc = 0;

	while (1)
	{
		GET_CHAR_TO (awk, c);

		if (c == QSE_CHAR_EOF)
		{
			SETERR_TOK (awk, QSE_AWK_ESTRNC);
			return -1;
		}

		if (escaped == 3)
		{
			if (c >= QSE_T('0') && c <= QSE_T('7'))
			{
				c_acc = c_acc * 8 + c - QSE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, tok, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				ADD_TOKEN_CHAR (awk, tok, c_acc);
				escaped = 0;
			}
		}
		else if (escaped == 2 || escaped == 4 || escaped == 8)
		{
			if (c >= QSE_T('0') && c <= QSE_T('9'))
			{
				c_acc = c_acc * 16 + c - QSE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, tok, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= QSE_T('A') && c <= QSE_T('F'))
			{
				c_acc = c_acc * 16 + c - QSE_T('A') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, tok, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= QSE_T('a') && c <= QSE_T('f'))
			{
				c_acc = c_acc * 16 + c - QSE_T('a') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, tok, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				qse_char_t rc;

				rc = (escaped == 2)? QSE_T('x'):
				     (escaped == 4)? QSE_T('u'): QSE_T('U');

				if (digit_count == 0) 
					ADD_TOKEN_CHAR (awk, tok, rc);
				else ADD_TOKEN_CHAR (awk, tok, c_acc);

				escaped = 0;
			}
		}

		if (escaped == 0 && c == end_char)
		{
			/* terminating quote */
			/*GET_CHAR_TO (awk, c);*/
			GET_CHAR (awk);
			break;
		}

		if (escaped == 0 && c == esc_char)
		{
			escaped = 1;
			continue;
		}

		if (escaped == 1)
		{
			if (c == QSE_T('n')) c = QSE_T('\n');
			else if (c == QSE_T('r')) c = QSE_T('\r');
			else if (c == QSE_T('t')) c = QSE_T('\t');
			else if (c == QSE_T('f')) c = QSE_T('\f');
			else if (c == QSE_T('b')) c = QSE_T('\b');
			else if (c == QSE_T('v')) c = QSE_T('\v');
			else if (c == QSE_T('a')) c = QSE_T('\a');
			else if (c >= QSE_T('0') && c <= QSE_T('7')) 
			{
				escaped = 3;
				digit_count = 1;
				c_acc = c - QSE_T('0');
				continue;
			}
			else if (c == QSE_T('x')) 
			{
				escaped = 2;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#ifdef QSE_CHAR_IS_WCHAR
			else if (c == QSE_T('u') && QSE_SIZEOF(qse_char_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == QSE_T('U') && QSE_SIZEOF(qse_char_t) >= 4) 
			{
				escaped = 8;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#endif
			else if (keep_esc_char) 
			{
				ADD_TOKEN_CHAR (awk, tok, esc_char);
			}

			escaped = 0;
		}

		ADD_TOKEN_CHAR (awk, tok, c);
	}

	return 0;
}

static int get_charstr (qse_awk_t* awk, qse_awk_tok_t* tok)
{
	if (awk->sio.last.c != QSE_T('\"')) 
	{
		/* the starting quote has been consumed before this function
		 * has been called */
		ADD_TOKEN_CHAR (awk, tok, awk->sio.last.c);
	}
	return get_string (awk, QSE_T('\"'), QSE_T('\\'), QSE_FALSE, 0, tok);
}

static int get_rexstr (qse_awk_t* awk, qse_awk_tok_t* tok)
{
	if (awk->sio.last.c == QSE_T('/')) 
	{
		/* this part of the function is different from get_charstr
		 * because of the way this function is called. 
		 * this condition is met when the input is //.
		 * the first / has been tokenized to TOK_DIV already.
		 * if TOK_DIV is seen as a primary, this function is called.
		 * as the token buffer has been cleared by the caller and
		 * the token type is set to TOK_REX, this function can
		 * just return after reading the next character */
		GET_CHAR (awk);
		return 0;
	}
	else 
	{
		int escaped = 0;
		if (awk->sio.last.c == QSE_T('\\')) 
		{		
			/* for input like /\//, this condition is met. 
			 * the initial escape character is added when the
			 * second charater is handled in get_string() */
			escaped = 1;
		}
		else 
		{
			/* add other initial characters here as get_string()
			 * begins with reading the next character */
			ADD_TOKEN_CHAR (awk, tok, awk->sio.last.c);
		}
		return get_string (
			awk, QSE_T('/'), QSE_T('\\'), QSE_TRUE, escaped, tok);
	}
}

static int skip_spaces (qse_awk_t* awk)
{
	qse_cint_t c = awk->sio.last.c;

	if (awk->option & QSE_AWK_NEWLINE)
	{
		do 
		{
			while (c != QSE_T('\n') && QSE_AWK_ISSPACE(awk,c))
				GET_CHAR_TO (awk, c);

			if (c == QSE_T('\\'))
			{
				qse_awk_sio_lxc_t bs;
				qse_awk_sio_lxc_t cr;
				int hascr = 0;

				bs = awk->sio.last;
				GET_CHAR_TO (awk, c);
				if (c == QSE_T('\r')) 
				{
					hascr = 1;
					cr = awk->sio.last;
					GET_CHAR_TO (awk, c);
				}

				if (c == QSE_T('\n'))
				{
					GET_CHAR_TO (awk, c);
					continue;
				}
				else
				{
					/* push back the last character */
					unget_char (awk, &awk->sio.last);
					/* push CR if any */
					if (hascr) unget_char (awk, &cr);
					/* restore the orginal backslash */
					awk->sio.last = bs;
				}
			}

			break;
		}
		while (1);
	}
	else
	{
		while (QSE_AWK_ISSPACE (awk, c)) GET_CHAR_TO (awk, c);
	}

	return 0;
}

static int skip_comment (qse_awk_t* awk)
{
	qse_cint_t c = awk->sio.last.c;
	qse_awk_sio_lxc_t lc;

	if (c == QSE_T('#'))
	{
		/* skip up to \n */
		do { GET_CHAR_TO (awk, c); }
		while (c != QSE_T('\n') && c != QSE_CHAR_EOF);

		if (!(awk->option & QSE_AWK_NEWLINE)) GET_CHAR (awk);
		return 1; /* comment by # */
	}

	/* handle c-style comment */
	if (c != QSE_T('/')) return 0; /* not a comment */

	/* save the last character */
	lc = awk->sio.last;
	/* read a new character */
	GET_CHAR_TO (awk, c);

	if (c == QSE_T('*')) 
	{
		do 
		{
			GET_CHAR_TO (awk, c);
			if (c == QSE_CHAR_EOF)
			{
				qse_awk_loc_t loc;
				loc.line = awk->sio.inp->line;
				loc.colm = awk->sio.inp->colm;
				loc.file = awk->sio.inp->name;
				SETERR_LOC (awk, QSE_AWK_ECMTNC, &loc);
				return -1;
			}

			if (c == QSE_T('*')) 
			{
				GET_CHAR_TO (awk, c);
				if (c == QSE_CHAR_EOF)
				{
					qse_awk_loc_t loc;
					loc.line = awk->sio.inp->line;
					loc.colm = awk->sio.inp->colm;
					loc.file = awk->sio.inp->name;
					SETERR_LOC (awk, QSE_AWK_ECMTNC, &loc);
					return -1;
				}

				if (c == QSE_T('/')) 
				{
					/*GET_CHAR_TO (awk, c);*/
					GET_CHAR (awk);
					break;
				}
			}
		} 
		while (1);

		return 1; /* c-style comment */
	}

	/* unget '*' */
	unget_char (awk, &awk->sio.last);
	/* restore the previous state */
	awk->sio.last = lc;

	return 0;
}

static int get_symbols (qse_awk_t* awk, qse_cint_t c, qse_awk_tok_t* tok)
{
	struct ops_t
	{
		const qse_char_t* str;
		qse_size_t len;
		int tid;
		int opt;
	};

	static struct ops_t ops[] = 
	{
		{ QSE_T("=="),  2, TOK_EQ,           0 },
		{ QSE_T("="),   1, TOK_ASSN,         0 },
		{ QSE_T("!="),  2, TOK_NE,           0 },
		{ QSE_T("!~"),  2, TOK_NM,           0 },
		{ QSE_T("!"),   1, TOK_LNOT,         0 },
		{ QSE_T(">>="), 3, TOK_RS_ASSN,      QSE_AWK_EXTRAOPS },
		{ QSE_T(">>"),  2, TOK_RS,           0 },
		{ QSE_T(">="),  2, TOK_GE,           0 },
		{ QSE_T(">"),   1, TOK_GT,           0 },
		{ QSE_T("<<="), 3, TOK_LS_ASSN,      QSE_AWK_EXTRAOPS },
		{ QSE_T("<<"),  2, TOK_LS,           0 },
		{ QSE_T("<="),  2, TOK_LE,           0 },
		{ QSE_T("<"),   1, TOK_LT,           0 },
		{ QSE_T("||"),  2, TOK_LOR,          0 },
		{ QSE_T("|="),  2, TOK_BOR_ASSN,     0 },
		{ QSE_T("|"),   1, TOK_BOR,          0 },
		{ QSE_T("&&"),  2, TOK_LAND,         0 },
		{ QSE_T("&="),  2, TOK_BAND_ASSN,    0 },
		{ QSE_T("&"),   1, TOK_BAND,         0 },
		{ QSE_T("^^="), 3, TOK_BXOR_ASSN,    QSE_AWK_EXTRAOPS },
		{ QSE_T("^^"),  2, TOK_BXOR,         QSE_AWK_EXTRAOPS },
		{ QSE_T("^="),  2, TOK_EXP_ASSN,     0 },
		{ QSE_T("^"),   1, TOK_EXP,          0 },
		{ QSE_T("++"),  2, TOK_PLUSPLUS,     0 },
		{ QSE_T("+="),  2, TOK_PLUS_ASSN,    0 },
		{ QSE_T("+"),   1, TOK_PLUS,         0 },
		{ QSE_T("--"),  2, TOK_MINUSMINUS,   0 },
		{ QSE_T("-="),  2, TOK_MINUS_ASSN,   0 },
		{ QSE_T("-"),   1, TOK_MINUS,        0 },
		{ QSE_T("**="), 3, TOK_EXP_ASSN,     QSE_AWK_EXTRAOPS },
		{ QSE_T("**"),  2, TOK_EXP,          QSE_AWK_EXTRAOPS },
		{ QSE_T("*="),  2, TOK_MUL_ASSN,     0 },
		{ QSE_T("*"),   1, TOK_MUL,          0 },
		{ QSE_T("/="),  2, TOK_DIV_ASSN,     0 },
		{ QSE_T("/"),   1, TOK_DIV,          0 },
		{ QSE_T("\\="), 2, TOK_IDIV_ASSN,    QSE_AWK_EXTRAOPS },
		{ QSE_T("\\"),  1, TOK_IDIV,         QSE_AWK_EXTRAOPS },
		{ QSE_T("%%="), 3, TOK_CONCAT_ASSN,  QSE_AWK_EXPLICIT },
		{ QSE_T("%%"),  2, TOK_CONCAT,       QSE_AWK_EXPLICIT },
		{ QSE_T("%="),  2, TOK_MOD_ASSN,     0 },
		{ QSE_T("%"),   1, TOK_MOD,          0 },
		{ QSE_T("~"),   1, TOK_TILDE,        0 },
		{ QSE_T("("),   1, TOK_LPAREN,       0 },
		{ QSE_T(")"),   1, TOK_RPAREN,       0 },
		{ QSE_T("{"),   1, TOK_LBRACE,       0 },
		{ QSE_T("}"),   1, TOK_RBRACE,       0 },
		{ QSE_T("["),   1, TOK_LBRACK,       0 },
		{ QSE_T("]"),   1, TOK_RBRACK,       0 },
		{ QSE_T("$"),   1, TOK_DOLLAR,       0 },
		{ QSE_T(","),   1, TOK_COMMA,        0 },
		{ QSE_T(";"),   1, TOK_SEMICOLON,    0 },
		{ QSE_T(":"),   1, TOK_COLON,        0 },
		{ QSE_T("?"),   1, TOK_QUEST,        0 },
		{ QSE_T("@"),   1, TOK_ATSIGN,       0 },
		{ QSE_NULL,     0, 0,                0 }
	};

	struct ops_t* p;
	int idx = 0;

	/* note that the loop below is not generaic enough.
	 * you must keep the operators strings in a particular order */

	for (p = ops; p->str != QSE_NULL; )
	{
		if (p->opt == 0 || (awk->option & p->opt))
		{
			if (p->str[idx] == QSE_T('\0'))
			{
				ADD_TOKEN_STR (awk, tok, p->str, p->len);
				SET_TOKEN_TYPE (awk, tok, p->tid);
				return 1;
			}

			if (c == p->str[idx])
			{
				idx++;
				GET_CHAR_TO (awk, c);
				continue;
			}
		}

		p++;
	}

	return 0;
}

static int get_token_into (qse_awk_t* awk, qse_awk_tok_t* tok)
{
	qse_cint_t c;
	int n;

retry:
	do 
	{
		if (skip_spaces(awk) <= -1) return -1;
		if ((n = skip_comment(awk)) <= -1) return -1;
	} 
	while (n >= 1);

	qse_str_clear (tok->name);
	tok->loc.file = awk->sio.last.file;
	tok->loc.line = awk->sio.last.line;
	tok->loc.colm = awk->sio.last.colm;

	c = awk->sio.last.c;

	if (c == QSE_CHAR_EOF) 
	{
		n = end_include (awk);
		if (n <= -1) return -1;
		if (n >= 1) 
		{
			awk->sio.last = awk->sio.inp->last;
			goto retry;
		}

		ADD_TOKEN_STR (awk, tok, QSE_T("<EOF>"), 5);
		SET_TOKEN_TYPE (awk, tok, TOK_EOF);
	}	
	else if (c == QSE_T('\n')) 
	{
		/*ADD_TOKEN_CHAR (awk, tok, QSE_T('\n'));*/
		ADD_TOKEN_STR (awk, tok, QSE_T("<NL>"), 4);
		SET_TOKEN_TYPE (awk, tok, TOK_NEWLINE);
		GET_CHAR (awk);
	}
	else if (QSE_AWK_ISDIGIT (awk, c)/*|| c == QSE_T('.')*/)
	{
		if (get_number (awk, tok) <= -1) return -1;
	}
	else if (c == QSE_T('.'))
	{
		qse_awk_sio_lxc_t lc;

		lc = awk->sio.last;
		GET_CHAR_TO (awk, c);

		unget_char (awk, &awk->sio.last);	
		awk->sio.last = lc;

		if (QSE_AWK_ISDIGIT (awk, c))
		{
			/* for a token such as .123 */
			if (get_number (awk, tok) <= -1) return -1;
		}
		else 
		{
			c = QSE_T('.');
			goto try_get_symbols;
		}
	}
	else if (c == QSE_T('_') || QSE_AWK_ISALPHA (awk, c))
	{
		int type;

		/* identifier */
		do 
		{
			ADD_TOKEN_CHAR (awk, tok, c);
			GET_CHAR_TO (awk, c);
		} 
		while (c == QSE_T('_') || 
		       QSE_AWK_ISALPHA (awk, c) || 
		       QSE_AWK_ISDIGIT (awk, c));

		type = classify_ident (awk, 
			QSE_STR_PTR(tok->name), 
			QSE_STR_LEN(tok->name));
		SET_TOKEN_TYPE (awk, tok, type);
	}
	else if (c == QSE_T('\"')) 
	{
		SET_TOKEN_TYPE (awk, tok, TOK_STR);
		if (get_charstr(awk, tok) <= -1) return -1;
	}
	else
	{
	try_get_symbols:
		n = get_symbols (awk, c, tok);
		if (n <= -1) return -1;
		if (n == 0)
		{
			/* not handled yet */
			if (c == QSE_T('\0'))
			{
				SETERR_ARG_LOC (
					awk, QSE_AWK_ELXCHR,
					QSE_T("<NUL>"), 5, &tok->loc);
			}
			else
			{
				qse_char_t cc = (qse_char_t)c;
				SETERR_ARG_LOC (
					awk, QSE_AWK_ELXCHR, &cc, 1, &tok->loc);
			}
			return -1;
		}
	}

	return 0;
}

static int get_token (qse_awk_t* awk)
{
	awk->ptok.type = awk->tok.type;
	awk->ptok.loc.file = awk->tok.loc.file;
	awk->ptok.loc.line = awk->tok.loc.line;
	awk->ptok.loc.colm = awk->tok.loc.colm;
	qse_str_swap (awk->ptok.name, awk->tok.name);

	if (QSE_STR_LEN(awk->ntok.name) > 0)
	{
		awk->tok.type = awk->ntok.type;
		awk->tok.loc.file = awk->ntok.loc.file;
		awk->tok.loc.line = awk->ntok.loc.line;
		awk->tok.loc.colm = awk->ntok.loc.colm;	

		qse_str_swap (awk->tok.name, awk->ntok.name);
		qse_str_clear (awk->ntok.name);

		return 0;
	}

	return get_token_into (awk, &awk->tok);
}

static int preget_token (qse_awk_t* awk)
{
	return get_token_into (awk, &awk->ntok);
}

static int classify_ident (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	/* perform binary search */

	/* declaring left, right, mid to be of int is ok
	 * because we know kwtab is small enough. */
	int left = 0, right = QSE_COUNTOF(kwtab) - 1, mid;

	while (left <= right)
	{
		int n;
		kwent_t* kwp;

		mid = (left + right) / 2;	
		kwp = &kwtab[mid];

		n = qse_strxncmp (kwp->name.ptr, kwp->name.len, name, len);
		if (n > 0) 
		{
			/* if left, right, mid were of qse_size_t,
			 * you would need the following line. 
			if (mid == 0) break;
			 */
			right = mid - 1;
		}
		else if (n < 0) left = mid + 1;
		else
		{
			if (kwp->valid != 0 && 
			    (awk->option & kwp->valid) != kwp->valid)
				break;

			return kwp->type;
		}
	}

	return TOK_IDENT;
}

struct deparse_func_t 
{
	qse_awk_t* awk;
	qse_char_t* tmp;
	qse_size_t tmp_len;
	int ret;
};

static int deparse (qse_awk_t* awk)
{
	qse_awk_nde_t* nde;
	qse_awk_chain_t* chain;
	qse_char_t tmp[QSE_SIZEOF(qse_size_t)*8 + 32];
	struct deparse_func_t df;
	int n = 0; 
	qse_ssize_t op;
	qse_cstr_t kw;

	QSE_ASSERT (awk->sio.outf != QSE_NULL);

	awk->sio.arg.name = QSE_NULL;
	awk->sio.arg.handle = QSE_NULL;
	awk->sio.arg.b.len = 0;
	awk->sio.arg.b.pos = 0;

	CLRERR (awk);
	op = awk->sio.outf (
		awk, QSE_AWK_SIO_OPEN, &awk->sio.arg, QSE_NULL, 0);
	if (op <= -1)
	{
		if (ISNOERR(awk)) 
			SETERR_ARG (awk, QSE_AWK_EOPEN, QSE_T("<SOUT>"), 6);
		return -1;
	}

	if (op == 0)
	{
		/* the result of the open operation indicates that the 
		 * file has been open but reached the end. so it has to 
		 * skip the entire deparsing procedure as it can't write 
		 * any single characters on such an io handler. but note 
		 * that this is not really an error for the parse and deparser. 
		 *
		 * in fact, there are two ways to skip deparsing.
		 *    1. set awk->sio.inf to NULL.
		 *    2. set awk->sio.inf to a normal handler but
		 *       make it return 0 on the OPEN request.
		 */
		n = 0;
		goto exit_deparse;
	}

#define EXIT_DEPARSE() do { n = -1; goto exit_deparse; } while(0)

	if (awk->tree.ngbls > awk->tree.ngbls_base) 
	{
		qse_size_t i, len;

		QSE_ASSERT (awk->tree.ngbls > 0);

		qse_awk_getkwname (awk, QSE_AWK_KWID_GLOBAL, &kw);
		if (qse_awk_putsrcstrn(awk,kw.ptr,kw.len) <= -1)
		{
			EXIT_DEPARSE ();
		}
		if (qse_awk_putsrcstr (awk, QSE_T(" ")) <= -1)
		{
			EXIT_DEPARSE ();
		}

		for (i = awk->tree.ngbls_base; i < awk->tree.ngbls - 1; i++) 
		{
			if ((awk->option & QSE_AWK_EXPLICIT) && 
			    !(awk->option & QSE_AWK_IMPLICIT))
			{
				/* use the actual name if no named variable 
				 * is allowed */
				if (qse_awk_putsrcstrn (awk, 
					QSE_LDA_DPTR(awk->parse.gbls,i),
					QSE_LDA_DLEN(awk->parse.gbls,i)) <= -1)
				{
					EXIT_DEPARSE ();
				}
			}
			else
			{
				len = qse_awk_longtostr (
					awk, (qse_long_t)i, 
					10, QSE_T("__g"), tmp, QSE_COUNTOF(tmp));
				QSE_ASSERT (len != (qse_size_t)-1);
				if (qse_awk_putsrcstrn (awk, tmp, len) <= -1)
				{
					EXIT_DEPARSE ();
				}
			}

			if (qse_awk_putsrcstr (awk, QSE_T(", ")) <= -1)
				EXIT_DEPARSE ();
		}

		if ((awk->option & QSE_AWK_EXPLICIT) && 
		    !(awk->option & QSE_AWK_IMPLICIT))
		{
			if (qse_awk_putsrcstrn (awk, 
				QSE_LDA_DPTR(awk->parse.gbls,i),
				QSE_LDA_DLEN(awk->parse.gbls,i)) <= -1)
			{
				EXIT_DEPARSE ();
			}
		}
		else
		{
			len = qse_awk_longtostr (	
				awk, (qse_long_t)i, 
				10, QSE_T("__g"), tmp, QSE_COUNTOF(tmp));
			QSE_ASSERT (len != (qse_size_t)-1);
			if (qse_awk_putsrcstrn (awk, tmp, len) <= -1)
			{
				EXIT_DEPARSE ();
			}
		}

		if (awk->option & QSE_AWK_CRLF)
		{
			if (qse_awk_putsrcstr (awk, QSE_T(";\r\n\r\n")) <= -1)
			{
				EXIT_DEPARSE ();
			}
		}
		else
		{
			if (qse_awk_putsrcstr (awk, QSE_T(";\n\n")) <= -1)
			{
				EXIT_DEPARSE ();
			}
		}
	}

	df.awk = awk;
	df.tmp = tmp;
	df.tmp_len = QSE_COUNTOF(tmp);
	df.ret = 0;

	qse_htb_walk (awk->tree.funs, deparse_func, &df);
	if (df.ret <= -1)
	{
		EXIT_DEPARSE ();
	}

	for (nde = awk->tree.begin; nde != QSE_NULL; nde = nde->next)
	{
		qse_cstr_t kw;

		qse_awk_getkwname (awk, QSE_AWK_KWID_BEGIN, &kw);

		if (qse_awk_putsrcstrn (awk, kw.ptr, kw.len) <= -1) EXIT_DEPARSE ();
		if (qse_awk_putsrcstr (awk, QSE_T(" ")) <= -1) EXIT_DEPARSE ();
		if (qse_awk_prnnde (awk, nde) <= -1) EXIT_DEPARSE ();

		if (awk->option & QSE_AWK_CRLF)
		{
			if (put_char (awk, QSE_T('\r')) <= -1) EXIT_DEPARSE ();
		}

		if (put_char (awk, QSE_T('\n')) <= -1) EXIT_DEPARSE ();
	}

	chain = awk->tree.chain;
	while (chain != QSE_NULL) 
	{
		if (chain->pattern != QSE_NULL) 
		{
			if (qse_awk_prnptnpt (awk, chain->pattern) <= -1)
				EXIT_DEPARSE ();
		}

		if (chain->action == QSE_NULL) 
		{
			/* blockless pattern */
			if (awk->option & QSE_AWK_CRLF)
			{
				if (put_char (awk, QSE_T('\r')) <= -1)
					EXIT_DEPARSE ();
			}

			if (put_char (awk, QSE_T('\n')) <= -1)
				EXIT_DEPARSE ();
		}
		else 
		{
			if (chain->pattern != QSE_NULL)
			{
				if (put_char (awk, QSE_T(' ')) <= -1)
					EXIT_DEPARSE ();
			}
			if (qse_awk_prnpt (awk, chain->action) <= -1)
				EXIT_DEPARSE ();
		}

		if (awk->option & QSE_AWK_CRLF)
		{
			if (put_char (awk, QSE_T('\r')) <= -1)
				EXIT_DEPARSE ();
		}

		if (put_char (awk, QSE_T('\n')) <= -1)
			EXIT_DEPARSE ();

		chain = chain->next;	
	}

	for (nde = awk->tree.end; nde != QSE_NULL; nde = nde->next)
	{
		qse_cstr_t kw;

		qse_awk_getkwname (awk, QSE_AWK_KWID_END, &kw);

		if (qse_awk_putsrcstrn (awk, kw.ptr, kw.len) <= -1) EXIT_DEPARSE ();
		if (qse_awk_putsrcstr (awk, QSE_T(" ")) <= -1) EXIT_DEPARSE ();
		if (qse_awk_prnnde (awk, nde) <= -1) EXIT_DEPARSE ();
		
		/*
		if (awk->option & QSE_AWK_CRLF)
		{
			if (put_char (awk, QSE_T('\r')) <= -1) EXIT_DEPARSE ();
		}

		if (put_char (awk, QSE_T('\n')) <= -1) EXIT_DEPARSE ();
		*/
	}

	if (flush_out (awk) <= -1) EXIT_DEPARSE ();

exit_deparse:
	if (n == 0) CLRERR (awk);
	if (awk->sio.outf (
		awk, QSE_AWK_SIO_CLOSE, &awk->sio.arg, QSE_NULL, 0) != 0)
	{
		if (n == 0)
		{
			if (ISNOERR(awk)) 
				SETERR_ARG (awk, QSE_AWK_ECLOSE, QSE_T("<SOUT>"), 6);
			n = -1;
		}
	}

	return n;
}

static qse_htb_walk_t deparse_func (
	qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	struct deparse_func_t* df = (struct deparse_func_t*)arg;
	qse_awk_fun_t* fun = (qse_awk_fun_t*)QSE_HTB_VPTR(pair);
	qse_size_t i, n;
	qse_cstr_t kw;

	QSE_ASSERT (qse_strxncmp (QSE_HTB_KPTR(pair), QSE_HTB_KLEN(pair), fun->name.ptr, fun->name.len) == 0);

#define PUT_C(x,c) \
	if (put_char(x->awk,c)==-1) { \
		x->ret = -1; return QSE_HTB_WALK_STOP; \
	}

#define PUT_S(x,str) \
	if (qse_awk_putsrcstr(x->awk,str) <= -1) { \
		x->ret = -1; return QSE_HTB_WALK_STOP; \
	}

#define PUT_SX(x,str,len) \
	if (qse_awk_putsrcstrn (x->awk, str, len) <= -1) { \
		x->ret = -1; return QSE_HTB_WALK_STOP; \
	}

	qse_awk_getkwname (df->awk, QSE_AWK_KWID_FUNCTION, &kw);
	PUT_SX (df, kw.ptr, kw.len);

	PUT_C (df, QSE_T(' '));
	PUT_SX (df, fun->name.ptr, fun->name.len);
	PUT_S (df, QSE_T(" ("));

	for (i = 0; i < fun->nargs; ) 
	{
		n = qse_awk_longtostr (
			df->awk, i++, 10, 
			QSE_T("__p"), df->tmp, df->tmp_len);
		QSE_ASSERT (n != (qse_size_t)-1);
		PUT_SX (df, df->tmp, n);

		if (i >= fun->nargs) break;
		PUT_S (df, QSE_T(", "));
	}

	PUT_S (df, QSE_T(")"));
	if (df->awk->option & QSE_AWK_CRLF) PUT_C (df, QSE_T('\r'));

	PUT_C (df, QSE_T('\n'));

	if (qse_awk_prnpt (df->awk, fun->body) <= -1) return -1;
	if (df->awk->option & QSE_AWK_CRLF)
	{
		PUT_C (df, QSE_T('\r'));
	}
	PUT_C (df, QSE_T('\n'));

	return QSE_HTB_WALK_FORWARD;

#undef PUT_C
#undef PUT_S
#undef PUT_SX
}

static int put_char (qse_awk_t* awk, qse_char_t c)
{
	awk->sio.arg.b.buf[awk->sio.arg.b.len++] = c;
	if (awk->sio.arg.b.len >= QSE_COUNTOF(awk->sio.arg.b.buf))
	{
		if (flush_out (awk) <= -1) return -1;
	}
	return 0;
}

static int flush_out (qse_awk_t* awk)
{
	qse_ssize_t n;

	while (awk->sio.arg.b.pos < awk->sio.arg.b.len)
	{
		CLRERR (awk);
		n = awk->sio.outf (
			awk, QSE_AWK_SIO_WRITE, &awk->sio.arg,
			&awk->sio.arg.b.buf[awk->sio.arg.b.pos], 
			awk->sio.arg.b.len - awk->sio.arg.b.pos
		);
		if (n <= 0) 
		{
			if (ISNOERR(awk)) 
				SETERR_ARG (awk, QSE_AWK_EWRITE, QSE_T("<SOUT>"), 6);
			return -1;
		}

		awk->sio.arg.b.pos += n;
	}

	awk->sio.arg.b.pos = 0;
	awk->sio.arg.b.len = 0;
	return 0;
}

int qse_awk_putsrcstr (qse_awk_t* awk, const qse_char_t* str)
{
	while (*str != QSE_T('\0'))
	{
		if (put_char (awk, *str) <= -1) return -1;
		str++;
	}

	return 0;
}

int qse_awk_putsrcstrn (
	qse_awk_t* awk, const qse_char_t* str, qse_size_t len)
{
	const qse_char_t* end = str + len;

	while (str < end)
	{
		if (put_char (awk, *str) <= -1) return -1;
		str++;
	}

	return 0;
}
