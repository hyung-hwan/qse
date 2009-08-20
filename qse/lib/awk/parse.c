/*
 * $Id: parse.c 258 2009-08-19 14:04:15Z hyunghwan.chung $
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

#include "awk.h"

enum token_t
{
	TOKEN_EOF,
	TOKEN_NEWLINE,

	/* special token to direct the parser to include a file specified */
	TOKEN_INCLUDE,

	/* TOKEN_XXX_ASSIGNs should in sync 
	 * with assop in assign_to_opcode */
	TOKEN_ASSIGN,
	TOKEN_PLUS_ASSIGN,
	TOKEN_MINUS_ASSIGN,
	TOKEN_MUL_ASSIGN,
	TOKEN_DIV_ASSIGN,
	TOKEN_IDIV_ASSIGN,
	TOKEN_MOD_ASSIGN,
	TOKEN_EXP_ASSIGN,
	TOKEN_RS_ASSIGN,
	TOKEN_LS_ASSIGN,
	TOKEN_BAND_ASSIGN,
	TOKEN_BXOR_ASSIGN,
	TOKEN_BOR_ASSIGN,

	TOKEN_EQ,
	TOKEN_NE,
	TOKEN_LE,
	TOKEN_LT,
	TOKEN_GE,
	TOKEN_GT,
	TOKEN_NM,   /* not match */
	TOKEN_LNOT, /* logical negation ! */
	TOKEN_PLUS,
	TOKEN_PLUSPLUS,
	TOKEN_MINUS,
	TOKEN_MINUSMINUS,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_IDIV,
	TOKEN_MOD,
	TOKEN_LOR,
	TOKEN_LAND,
	TOKEN_BOR,
	TOKEN_BXOR,
	TOKEN_BAND,
	TOKEN_TILDE, /* used for unary bitwise-not and regex match */
	TOKEN_RS,
	TOKEN_LS,
	TOKEN_IN,
	TOKEN_EXP,

	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_LBRACK,
	TOKEN_RBRACK,

	TOKEN_DOLLAR,
	TOKEN_COMMA,
	TOKEN_PERIOD,
	TOKEN_SEMICOLON,
	TOKEN_COLON,
	TOKEN_QUEST,
	TOKEN_ATSIGN,

	TOKEN_BEGIN,
	TOKEN_END,
	TOKEN_FUNCTION,

	TOKEN_LOCAL,
	TOKEN_GLOBAL,

	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_WHILE,
	TOKEN_FOR,
	TOKEN_DO,
	TOKEN_BREAK,
	TOKEN_CONTINUE,
	TOKEN_RETURN,
	TOKEN_EXIT,
	TOKEN_NEXT,
	TOKEN_NEXTFILE,
	TOKEN_NEXTOFILE,
	TOKEN_DELETE,
	TOKEN_RESET,
	TOKEN_PRINT,
	TOKEN_PRINTF,

	TOKEN_GETLINE,
	TOKEN_IDENT,
	TOKEN_INT,
	TOKEN_REAL,
	TOKEN_STR,
	TOKEN_REX,

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
static qse_awk_chain_t* parse_pattern_block (
	qse_awk_t* awk, qse_awk_nde_t* ptn, qse_bool_t blockless);

static qse_awk_nde_t* parse_block_dc (qse_awk_t* awk, qse_size_t line, qse_bool_t istop);
static qse_awk_nde_t* parse_statement (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_statement_nb (qse_awk_t* awk, qse_size_t line);

static qse_awk_nde_t* parse_expression (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_expression0 (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_basic_expr (qse_awk_t* awk, qse_size_t line);

static qse_awk_nde_t* parse_binary_expr (
	qse_awk_t* awk, qse_size_t line, int skipnl, const binmap_t* binmap,
	qse_awk_nde_t*(*next_level_func)(qse_awk_t*,qse_size_t));

static qse_awk_nde_t* parse_logical_or (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_logical_and (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_in (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_regex_match (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_bitwise_or (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_bitwise_xor (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_bitwise_and (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_equality (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_relational (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_shift (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_concat (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_additive (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_multiplicative (qse_awk_t* awk, qse_size_t line);

static qse_awk_nde_t* parse_unary (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_exponent (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_unary_exp (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_increment (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_primary (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_primary_ident (qse_awk_t* awk, qse_size_t line);

static qse_awk_nde_t* parse_hashidx (
	qse_awk_t* awk, qse_char_t* name, qse_size_t name_len, 
	qse_size_t line);
static qse_awk_nde_t* parse_fncall (
	qse_awk_t* awk, qse_char_t* name, qse_size_t name_len, 
	qse_awk_fnc_t* fnc, qse_size_t line, int noarg);
static qse_awk_nde_t* parse_if (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_while (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_for (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_dowhile (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_break (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_continue (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_return (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_exit (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_next (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_nextfile (qse_awk_t* awk, qse_size_t line, int out);
static qse_awk_nde_t* parse_delete (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_reset (qse_awk_t* awk, qse_size_t line);
static qse_awk_nde_t* parse_print (qse_awk_t* awk, qse_size_t line, int type);

static int get_token (qse_awk_t* awk);
static int preget_token (qse_awk_t* awk);
static int get_rexstr (qse_awk_t* awk, qse_awk_token_t* token);

static int skip_spaces (qse_awk_t* awk);
static int skip_comment (qse_awk_t* awk);
static int classify_ident (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len);
static int is_plain_var (qse_awk_nde_t* nde);
static int is_var (qse_awk_nde_t* nde);

static int deparse (qse_awk_t* awk);
static qse_map_walk_t deparse_func (qse_map_t* map, qse_map_pair_t* pair, void* arg);
static int put_char (qse_awk_t* awk, qse_char_t c);
static int flush_out (qse_awk_t* awk);

typedef struct kwent_t kwent_t;

struct kwent_t 
{ 
	const qse_char_t* name; 
	qse_size_t name_len;
	int type; 
	int valid; /* the entry is valid when this option is set */
};

static kwent_t kwtab[] = 
{
	/* keep this table in sync with the kw_t enums in <parse.h>.
	 * also keep it sorted by the first field for binary search */
	{ QSE_T("BEGIN"),        5, TOKEN_BEGIN,       QSE_AWK_PABLOCK },
	{ QSE_T("END"),          3, TOKEN_END,         QSE_AWK_PABLOCK },
	{ QSE_T("break"),        5, TOKEN_BREAK,       0 },
	{ QSE_T("continue"),     8, TOKEN_CONTINUE,    0 },
	{ QSE_T("delete"),       6, TOKEN_DELETE,      0 },
	{ QSE_T("do"),           2, TOKEN_DO,          0 },
	{ QSE_T("else"),         4, TOKEN_ELSE,        0 },
	{ QSE_T("exit"),         4, TOKEN_EXIT,        0 },
	{ QSE_T("for"),          3, TOKEN_FOR,         0 },
	{ QSE_T("function"),     8, TOKEN_FUNCTION,    0 },
	{ QSE_T("getline"),      7, TOKEN_GETLINE,     QSE_AWK_RIO },
	{ QSE_T("global"),       6, TOKEN_GLOBAL,      QSE_AWK_EXPLICIT },
	{ QSE_T("if"),           2, TOKEN_IF,          0 },
	{ QSE_T("in"),           2, TOKEN_IN,          0 },
	{ QSE_T("include"),      7, TOKEN_INCLUDE,     QSE_AWK_INCLUDE },
	{ QSE_T("local"),        5, TOKEN_LOCAL,       QSE_AWK_EXPLICIT },
	{ QSE_T("next"),         4, TOKEN_NEXT,        QSE_AWK_PABLOCK },
	{ QSE_T("nextfile"),     8, TOKEN_NEXTFILE,    QSE_AWK_PABLOCK },
	{ QSE_T("nextofile"),    9, TOKEN_NEXTOFILE,   QSE_AWK_PABLOCK | QSE_AWK_NEXTOFILE },
	{ QSE_T("print"),        5, TOKEN_PRINT,       QSE_AWK_RIO },
	{ QSE_T("printf"),       6, TOKEN_PRINTF,      QSE_AWK_RIO },
	{ QSE_T("reset"),        5, TOKEN_RESET,       QSE_AWK_RESET },
	{ QSE_T("return"),       6, TOKEN_RETURN,      0 },
	{ QSE_T("while"),        5, TOKEN_WHILE,       0 }
};

typedef struct global_t global_t;

struct global_t
{
	const qse_char_t* name;
	qse_size_t name_len;
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

#define SET_TOKEN_TYPE(awk,token,code) \
	do { (token)->type = (code); } while (0)

#define ADD_TOKEN_CHAR(awk,token,c) \
	do { \
		if (qse_str_ccat((token)->name,(c)) == (qse_size_t)-1) \
		{ \
			qse_awk_seterror (awk, QSE_AWK_ENOMEM, QSE_NULL, (token)->lin); \
			return -1; \
		} \
	} while (0)

#define ADD_TOKEN_STR(awk,token,s,l) \
	do { \
		if (qse_str_ncat((token)->name,(s),(l)) == (qse_size_t)-1) \
		{ \
			qse_awk_seterror (awk, QSE_AWK_ENOMEM, QSE_NULL, (token)->lin); \
			return -1; \
		} \
	} while (0)

#define MATCH(awk,token_type) ((awk)->token.type == (token_type))

#define MATCH_TERMINATOR_NORMAL(awk) \
	(MATCH((awk),TOKEN_SEMICOLON) || MATCH((awk),TOKEN_NEWLINE))

#define MATCH_TERMINATOR_RBRACE(awk) \
	((awk->option & QSE_AWK_NEWLINE) && MATCH((awk),TOKEN_RBRACE))

#define MATCH_TERMINATOR(awk) \
	(MATCH_TERMINATOR_NORMAL(awk) || MATCH_TERMINATOR_RBRACE(awk))

#define ISNOERR(awk) ((awk)->errinf.num == QSE_AWK_ENOERR)

#define CLRERR(awk) \
	qse_awk_seterror (awk, QSE_AWK_ENOERR, QSE_NULL, 0)

#define SETERR_TOK(awk,code) \
	do { \
		qse_cstr_t __ea; \
		__ea.len = QSE_STR_LEN((awk)->token.name); \
		__ea.ptr = QSE_STR_PTR((awk)->token.name); \
		qse_awk_seterror (awk, code, &__ea, (awk)->token.lin); \
	} while (0)


#define SETERR_CODE(awk,code) \
	qse_awk_seterror (awk, code, QSE_NULL, 0)

#define SETERR_LIN(awk,code,line) \
	qse_awk_seterror (awk, code, QSE_NULL, line)

#define SETERR_ARG(awk,code,arg,leng) \
	do { \
		qse_cstr_t __ea; \
		__ea.len = (leng); __ea.ptr = (arg); \
		qse_awk_seterror ((awk), (code), &__ea, 0); \
	} while (0)

#define SETERR_ARG_LOC(awk,code,arg,leng,line) \
	do { \
		qse_cstr_t __ea; \
		__ea.len = (leng); __ea.ptr = (arg); \
		qse_awk_seterror ((awk), (code), &__ea, (line)); \
	} while (0)

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
			awk->sio.last.lin = awk->sio.inp->lin;
			awk->sio.last.col = awk->sio.inp->col;
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
		 * TOKEN_EOF to be the same line as the last newline. */
		awk->sio.inp->lin++;
		awk->sio.inp->col = 1;
	}
	
	awk->sio.inp->last.c = awk->sio.inp->b.buf[awk->sio.inp->b.pos++];
	awk->sio.inp->last.lin = awk->sio.inp->lin;
	awk->sio.inp->last.col = awk->sio.inp->col++;
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

qse_cstr_t* qse_awk_getkw (qse_awk_t* awk, int id, qse_cstr_t* s)
{
	qse_map_pair_t* p;

	s->ptr = kwtab[id].name;
	s->len = kwtab[id].name_len;

	p = qse_map_search (awk->wtab, s->ptr, s->len);
	if (p != QSE_NULL) 
	{
		s->ptr = QSE_MAP_VPTR(p);
		s->len = QSE_MAP_VLEN(p);
	}

	return s;
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
			while (MATCH(awk,TOKEN_NEWLINE)) 
			{
				if (get_token(awk) <= -1) goto oops;
			}
			if (MATCH(awk,TOKEN_EOF)) break;

			if (parse_progunit(awk) == QSE_NULL) goto oops;
		}

		if ((awk->option & QSE_AWK_EXPLICIT) &&
		    !(awk->option & QSE_AWK_IMPLICIT))
		{
			qse_map_pair_t* p;
			qse_size_t buckno;

			p = qse_map_getfirstpair (awk->parse.funs, &buckno);
			while (p != QSE_NULL)
			{
				if (qse_map_search (awk->tree.funs, 
					QSE_MAP_KPTR(p), QSE_MAP_KLEN(p)) == QSE_NULL)
				{
					/* TODO: set better error no & line */
					SETERR_ARG_LOC (
						awk, 
						QSE_AWK_EFUNNF, 
						QSE_MAP_KPTR(p),
						QSE_MAP_KLEN(p),
						*(qse_size_t*)QSE_MAP_VPTR(p)
					);
					goto oops;
				}

				p = qse_map_getnextpair (awk->parse.funs, p, &buckno);
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
		SETERR_CODE (awk, QSE_AWK_EINVAL);
		return -1;
	}

	QSE_ASSERT (awk->parse.depth.cur.loop == 0);
	QSE_ASSERT (awk->parse.depth.cur.expr == 0);

	qse_awk_clear (awk);
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
	qse_map_pair_t* pair = QSE_NULL;

	if (qse_strlen(awk->token.name->ptr) != QSE_STR_LEN(awk->token.name))
	{
		SETERR_ARG_LOC (
			awk, 
			QSE_AWK_EIONMNL,
			QSE_STR_PTR(awk->token.name),
			qse_strlen(QSE_STR_PTR(awk->token.name)),
			awk->token.lin
		);
		return -1;
	}

	/* store the file name to awk->sio.names */
	pair = qse_map_ensert (
		awk->sio.names, 
		QSE_STR_PTR(awk->token.name),
		QSE_STR_LEN(awk->token.name) + 1, /* to include '\0' */
		QSE_NULL, 0
	);
	if (pair == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, awk->ptoken.lin);
		goto oops;
	}

	/*QSE_MAP_VPTR(pair) = QSE_MAP_KPTR(pair);
	QSE_MAP_VLEN(pair) = QSE_MAP_KLEN(pair);*/

	arg = (qse_awk_sio_arg_t*) QSE_MMGR_ALLOC (awk->mmgr, QSE_SIZEOF(*arg));
	if (arg == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, awk->ptoken.lin);
		goto oops;
	}

	QSE_MEMSET (arg, 0, QSE_SIZEOF(*arg));
	arg->name = QSE_MAP_KPTR(pair);

	CLRERR (awk);
	op = awk->sio.inf (awk, QSE_AWK_SIO_OPEN, arg, QSE_NULL, 0);
	if (op <= -1)
	{
		if (ISNOERR(awk)) SETERR_TOK (awk, QSE_AWK_EOPEN);
		else awk->errinf.lin = awk->token.lin; /* adjust error line */
		goto oops;
	}

	if (op == 0)
	{
		CLRERR (awk);
		op = awk->sio.inf (awk, QSE_AWK_SIO_CLOSE, arg, QSE_NULL, 0);
		if (op != 0)
		{
			if (ISNOERR(awk)) SETERR_TOK (awk, QSE_AWK_ECLOSE);
			else awk->errinf.lin = awk->token.lin;
			goto oops;
		}
	}

	arg->next = awk->sio.inp;
	awk->sio.inp = arg;
	awk->parse.depth.cur.incl++;

	awk->sio.inp->lin = 1;
	awk->sio.inp->col = 1;

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
	if ((awk->option & QSE_AWK_EXPLICIT) && MATCH(awk,TOKEN_GLOBAL)) 
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
	else if (MATCH(awk,TOKEN_ATSIGN))
	{
		if (get_token(awk) <= -1) return QSE_NULL;

		if (MATCH(awk,TOKEN_INCLUDE))
		{
			if (awk->parse.depth.max.incl > 0 &&
			    awk->parse.depth.cur.incl >=  awk->parse.depth.max.incl)
			{
				SETERR_LIN (awk, QSE_AWK_EINCLTD, awk->ptoken.lin);
				return QSE_NULL;
			}

			if (get_token(awk) <= -1) return QSE_NULL;

			if (!MATCH(awk,TOKEN_STR))
			{
				SETERR_LIN (awk, QSE_AWK_EINCLSTR, awk->ptoken.lin);
				return QSE_NULL;
			}

			if (begin_include (awk) <= -1) return QSE_NULL;
			
			/* read the first meaningful token from the included file 
			 * and recheck it by jumping to retry: */
			do
			{
				if (get_token(awk) <= -1) return QSE_NULL; 
			}
			while (MATCH(awk,TOKEN_NEWLINE));

			goto retry;
		}
		else
		{
			SETERR_TOK (awk, QSE_AWK_EDIRECNR);
			return QSE_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_FUNCTION)) 
	{
		awk->parse.id.block = PARSE_FUNCTION;
		if (parse_function (awk) == QSE_NULL) return QSE_NULL;
	}
	else if (MATCH(awk,TOKEN_BEGIN)) 
	{
		if ((awk->option & QSE_AWK_PABLOCK) == 0)
		{
			SETERR_TOK (awk, QSE_AWK_EKWFNC);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_BEGIN;
		if (get_token(awk) <= -1) return QSE_NULL; 

		if (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF))
		{
			/* when QSE_AWK_NEWLINE is set,
	   		 * BEGIN and { should be located on the same line */
			SETERR_LIN (awk, QSE_AWK_EBLKBEG, awk->ptoken.lin);
			return QSE_NULL;
		}

		if (!MATCH(awk,TOKEN_LBRACE)) 
		{
			SETERR_TOK (awk, QSE_AWK_ELBRACE);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_BEGIN_BLOCK;
		if (parse_begin (awk) == QSE_NULL) return QSE_NULL;
	}
	else if (MATCH(awk,TOKEN_END)) 
	{
		if ((awk->option & QSE_AWK_PABLOCK) == 0)
		{
			SETERR_TOK (awk, QSE_AWK_EKWFNC);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_END;
		if (get_token(awk) <= -1) return QSE_NULL; 

		if (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF))
		{
			/* when QSE_AWK_NEWLINE is set,
	   		 * END and { should be located on the same line */
			SETERR_LIN (awk, QSE_AWK_EBLKEND, awk->ptoken.lin);
			return QSE_NULL;
		}

		if (!MATCH(awk,TOKEN_LBRACE)) 
		{
			SETERR_TOK (awk, QSE_AWK_ELBRACE);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_END_BLOCK;
		if (parse_end (awk) == QSE_NULL) return QSE_NULL;
	}
	else if (MATCH(awk,TOKEN_LBRACE))
	{
		/* patternless block */
		if ((awk->option & QSE_AWK_PABLOCK) == 0)
		{
			SETERR_TOK (awk, QSE_AWK_EKWFNC);
			return QSE_NULL;
		}

		awk->parse.id.block = PARSE_ACTION_BLOCK;
		if (parse_pattern_block (
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

		ptn = parse_expression (awk, awk->token.lin);
		if (ptn == QSE_NULL) return QSE_NULL;

		QSE_ASSERT (ptn->next == QSE_NULL);

		if (MATCH(awk,TOKEN_COMMA))
		{
			if (get_token (awk) <= -1) 
			{
				qse_awk_clrpt (awk, ptn);
				return QSE_NULL;
			}	

			ptn->next = parse_expression (awk, awk->token.lin);
			if (ptn->next == QSE_NULL) 
			{
				qse_awk_clrpt (awk, ptn);
				return QSE_NULL;
			}
		}

		if (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF))
		{
			/* blockless pattern */
			qse_bool_t newline = MATCH(awk,TOKEN_NEWLINE);
			qse_size_t tline = awk->ptoken.lin;

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (parse_pattern_block(awk,ptn,QSE_TRUE) == QSE_NULL) 
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
				SETERR_LIN (awk, QSE_AWK_ENOSUP, tline);
				return QSE_NULL;
			}
		}
		else
		{
			/* parse the action block */
			if (!MATCH(awk,TOKEN_LBRACE))
			{
				qse_awk_clrpt (awk, ptn);
				SETERR_TOK (awk, QSE_AWK_ELBRACE);
				return QSE_NULL;
			}

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (parse_pattern_block (
				awk, ptn, QSE_FALSE) == QSE_NULL) 
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
	qse_char_t* name_dup;
	qse_size_t name_len;
	qse_awk_nde_t* body;
	qse_awk_fun_t* fun;
	qse_size_t nargs, g;
	qse_map_pair_t* pair;

	/* eat up the keyword 'function' and get the next token */
	QSE_ASSERT (MATCH(awk,TOKEN_FUNCTION));
	if (get_token(awk) <= -1) return QSE_NULL;  

	/* match a function name */
	if (!MATCH(awk,TOKEN_IDENT)) 
	{
		/* cannot find a valid identifier for a function name */
		SETERR_TOK (awk, QSE_AWK_EFUNNAME);
		return QSE_NULL;
	}

	name = QSE_STR_PTR(awk->token.name);
	name_len = QSE_STR_LEN(awk->token.name);

	/* check if it is a builtin function */
	if (qse_awk_getfnc (awk, name, name_len) != QSE_NULL)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EFNCRED, name, name_len, awk->token.lin);
		return QSE_NULL;
	}

	/* check if it has already been defined as a function */
	if (qse_map_search (awk->tree.funs, name, name_len) != QSE_NULL)
	{
		/* the function is defined previously */
		SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, name, name_len, awk->token.lin);
		return QSE_NULL;
	}

	/* check if it conflicts with a named variable */
	if (qse_map_search (awk->parse.named, name, name_len) != QSE_NULL)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EVARRED, name, name_len, awk->token.lin);
		return QSE_NULL;
	}

	/* check if it coincides to be a global variable name */
	g = find_global (awk, name, name_len);
	if (g != QSE_LDA_NIL)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EGBLRED, name, name_len, awk->token.lin);
		return QSE_NULL;
	}

	/* clone the function name before it is overwritten */
	name_dup = QSE_AWK_STRXDUP (awk, name, name_len);
	if (name_dup == QSE_NULL) 
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, awk->token.lin);
		return QSE_NULL;
	}

	/* get the next token */
	if (get_token(awk) <= -1) 
	{
		QSE_AWK_FREE (awk, name_dup);
		return QSE_NULL;  
	}

	/* match a left parenthesis */
	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		/* a function name is not followed by a left parenthesis */
		QSE_AWK_FREE (awk, name_dup);
		SETERR_TOK (awk, QSE_AWK_ELPAREN);
		return QSE_NULL;
	}	

	/* get the next token */
	if (get_token(awk) <= -1) 
	{
		QSE_AWK_FREE (awk, name_dup);
		return QSE_NULL;
	}

	/* make sure that parameter table is empty */
	QSE_ASSERT (QSE_LDA_SIZE(awk->parse.params) == 0);

	/* read parameter list */
	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no function parameter found. get the next token */
		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, name_dup);
			return QSE_NULL;
		}
	}
	else 
	{
		while (1) 
		{
			qse_char_t* param;
			qse_size_t param_len;

			if (!MATCH(awk,TOKEN_IDENT)) 
			{
				QSE_AWK_FREE (awk, name_dup);
				qse_lda_clear (awk->parse.params);
				SETERR_TOK (awk, QSE_AWK_EBADPAR);
				return QSE_NULL;
			}

			param = QSE_STR_PTR(awk->token.name);
			param_len = QSE_STR_LEN(awk->token.name);

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
				param, param_len, name_dup, name_len) == 0) ||
			    qse_lda_search (awk->parse.params, 
				0, param, param_len) != QSE_LDA_NIL)
			{
				QSE_AWK_FREE (awk, name_dup);
				qse_lda_clear (awk->parse.params);

				SETERR_ARG_LOC (awk, QSE_AWK_EDUPPAR, param, param_len, awk->token.lin);
				return QSE_NULL;
			}

			/* push the parameter to the parameter list */
			if (QSE_LDA_SIZE(awk->parse.params) >= QSE_AWK_MAX_PARAMS)
			{
				QSE_AWK_FREE (awk, name_dup);
				qse_lda_clear (awk->parse.params);
				SETERR_LIN (awk, QSE_AWK_EPARTM, awk->token.lin);
				return QSE_NULL;
			}

			if (qse_lda_insert (
				awk->parse.params, 
				QSE_LDA_SIZE(awk->parse.params), 
				param, param_len) == QSE_LDA_NIL)
			{
				QSE_AWK_FREE (awk, name_dup);
				qse_lda_clear (awk->parse.params);

				SETERR_LIN (awk, QSE_AWK_ENOMEM, awk->token.lin);
				return QSE_NULL;
			}	

			if (get_token (awk) <= -1) 
			{
				QSE_AWK_FREE (awk, name_dup);
				qse_lda_clear (awk->parse.params);
				return QSE_NULL;
			}	

			if (MATCH(awk,TOKEN_RPAREN)) break;

			if (!MATCH(awk,TOKEN_COMMA)) 
			{
				QSE_AWK_FREE (awk, name_dup);
				qse_lda_clear (awk->parse.params);

				SETERR_TOK (awk, QSE_AWK_ECOMMA);
				return QSE_NULL;
			}

			do
			{
				if (get_token(awk) <= -1) 
				{
					QSE_AWK_FREE (awk, name_dup);
					qse_lda_clear (awk->parse.params);
					return QSE_NULL;
				}
			}
			while (MATCH(awk,TOKEN_NEWLINE));
		}

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, name_dup);
			qse_lda_clear (awk->parse.params);
			return QSE_NULL;
		}
	}

	/* function body can be placed on a different line 
	 * from a function name and the parameters even if
	 * QSE_AWK_NEWLINE is set. note TOKEN_NEWLINE is
	 * available only when the option is set. */
	while (MATCH(awk,TOKEN_NEWLINE))
	{
		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, name_dup);
			qse_lda_clear (awk->parse.params);
			return QSE_NULL;
		}
	}

	/* check if the function body starts with a left brace */
	if (!MATCH(awk,TOKEN_LBRACE)) 
	{
		QSE_AWK_FREE (awk, name_dup);
		qse_lda_clear (awk->parse.params);

		SETERR_TOK (awk, QSE_AWK_ELBRACE);
		return QSE_NULL;
	}
	if (get_token(awk) <= -1) 
	{
		QSE_AWK_FREE (awk, name_dup);
		qse_lda_clear (awk->parse.params);
		return QSE_NULL; 
	}

	/* remember the current function name so that the body parser
	 * can know the name of the current function being parsed */
	awk->tree.cur_fun.ptr = name_dup;
	awk->tree.cur_fun.len = name_len;

	/* actual function body */
	body = parse_block_dc (awk, awk->ptoken.lin, QSE_TRUE);

	/* clear the current function name remembered */
	awk->tree.cur_fun.ptr = QSE_NULL;
	awk->tree.cur_fun.len = 0;

	if (body == QSE_NULL) 
	{
		QSE_AWK_FREE (awk, name_dup);
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
		QSE_AWK_FREE (awk, name_dup);
		qse_awk_clrpt (awk, body);

		SETERR_LIN (awk, QSE_AWK_ENOMEM, awk->token.lin);
		return QSE_NULL;
	}

	fun->name.ptr = QSE_NULL; /* function name is set below */
	fun->name.len = 0;
	fun->nargs = nargs;
	fun->body = body;

	pair = qse_map_insert (awk->tree.funs, name_dup, name_len, fun, 0);
	if (pair == QSE_NULL)
	{
		/* if qse_map_insert() fails for other reasons than memory 
		 * shortage, there should be implementaion errors as duplicate
		 * functions are detected earlier in this function */
		QSE_AWK_FREE (awk, name_dup);
		qse_awk_clrpt (awk, body);
		QSE_AWK_FREE (awk, fun);

		SETERR_LIN (awk, QSE_AWK_ENOMEM, awk->token.lin);
		return QSE_NULL;
	}

	/* do some trick to save a string. make it back-point at the key part 
	 * of the pair */
	fun->name.ptr = QSE_MAP_KPTR(pair); 
	fun->name.len = QSE_MAP_KLEN(pair);
	QSE_AWK_FREE (awk, name_dup);

	/* remove an undefined function call entry from the parse.fun table */
	qse_map_delete (awk->parse.funs, fun->name.ptr, name_len);
	return body;
}

static qse_awk_nde_t* parse_begin (qse_awk_t* awk)
{
	qse_awk_nde_t* nde;

	QSE_ASSERT (MATCH(awk,TOKEN_LBRACE));

	if (get_token(awk) <= -1) return QSE_NULL; 
	nde = parse_block_dc (awk, awk->ptoken.lin, QSE_TRUE);
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

	QSE_ASSERT (MATCH(awk,TOKEN_LBRACE));

	if (get_token(awk) <= -1) return QSE_NULL; 
	nde = parse_block_dc (awk, awk->ptoken.lin, QSE_TRUE);
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

static qse_awk_chain_t* parse_pattern_block (
	qse_awk_t* awk, qse_awk_nde_t* ptn, qse_bool_t blockless)
{
	qse_awk_nde_t* nde;
	qse_awk_chain_t* chain;
	qse_size_t line = awk->token.lin;

	if (blockless) nde = QSE_NULL;
	else
	{
		QSE_ASSERT (MATCH(awk,TOKEN_LBRACE));
		if (get_token(awk) <= -1) return QSE_NULL; 
		nde = parse_block_dc (awk, line, QSE_TRUE);
		if (nde == QSE_NULL) return QSE_NULL;
	}

	chain = (qse_awk_chain_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_chain_t));
	if (chain == QSE_NULL) 
	{
		qse_awk_clrpt (awk, nde);

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
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
	qse_awk_t* awk, qse_size_t line, qse_bool_t istop) 
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
			while (MATCH(awk,TOKEN_NEWLINE))
			{
				if (get_token(awk) <= -1) return QSE_NULL;
			}

			if (!MATCH(awk,TOKEN_LOCAL)) break;

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
		while (MATCH(awk,TOKEN_NEWLINE))
		{
			if (get_token(awk) <= -1) return QSE_NULL;
		}

		/* if EOF is met before the right brace, this is an error */
		if (MATCH(awk,TOKEN_EOF)) 
		{
			qse_lda_delete (
				awk->parse.lcls, nlcls, 
				QSE_LDA_SIZE(awk->parse.lcls) - nlcls);
			if (head != QSE_NULL) qse_awk_clrpt (awk, head);

			/*SETERR_LIN (awk, QSE_AWK_EEOF, awk->ptoken.lin);*/
			SETERR_LIN (awk, QSE_AWK_EEOF, awk->token.lin);
			return QSE_NULL;
		}

		/* end the block when the right brace is met */
		if (MATCH(awk,TOKEN_RBRACE)) 
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
		nde = parse_statement (awk, awk->token.lin);
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

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	tmp = QSE_LDA_SIZE(awk->parse.lcls);
	if (tmp > awk->parse.nlcls_max) awk->parse.nlcls_max = tmp;

	/* remove all lcls to move it up to the top level */
	qse_lda_delete (awk->parse.lcls, nlcls, tmp - nlcls);

	/* adjust the number of lcls for a block without any statements */
	/* if (head == QSE_NULL) tmp = 0; */

	block->type = QSE_AWK_NDE_BLK;
	block->line = line;
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
	qse_awk_t* awk, qse_size_t line, qse_bool_t istop) 
{
	qse_awk_nde_t* nde;
		
	if (awk->parse.depth.max.block > 0 &&
	    awk->parse.depth.cur.block >= awk->parse.depth.max.block)
	{
		SETERR_LIN (awk, QSE_AWK_EBLKNST, awk->ptoken.lin);
		return QSE_NULL;
	}

	awk->parse.depth.cur.block++;
	nde = parse_block (awk, line, istop);
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
			gtab[id].name_len);
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
			QSE_LDA_DLEN(awk->parse.gbls,id) = gtab[id].name_len;
		}
	}
}

typedef struct check_global_t check_global_t;

struct check_global_t
{
	qse_cstr_t name;
	qse_size_t index;
	qse_lda_walk_t walk;
};

static qse_lda_walk_t check_global (qse_lda_t* lda, qse_size_t index, void* arg)
{
	qse_cstr_t tmp;
	qse_awk_t* awk = *(qse_awk_t**)QSE_XTN(lda);
	check_global_t* cg = (check_global_t*)arg;

	tmp.ptr = QSE_LDA_DPTR(lda,index);
	tmp.len = QSE_LDA_DLEN(lda,index);

	if (index < awk->tree.ngbls_base)
	{
		qse_map_pair_t* pair;

		pair = qse_map_search (awk->wtab, tmp.ptr, tmp.len);
		if (pair != QSE_NULL)
		{
			tmp.ptr = ((qse_cstr_t*)(pair->vptr))->ptr;
			tmp.len = ((qse_cstr_t*)(pair->vptr))->len;
		}
	}

	if (qse_strxncmp(tmp.ptr, tmp.len, cg->name.ptr, cg->name.len) == 0) 
	{
		cg->index = index;
		return QSE_LDA_WALK_STOP;
	}

	return cg->walk;
}

static qse_size_t get_global (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	check_global_t cg;

	cg.name.ptr = name;
	cg.name.len = len;
	cg.index = QSE_LDA_NIL;
	cg.walk = QSE_LDA_WALK_BACKWARD;

	qse_lda_rwalk (awk->parse.gbls, check_global, &cg);
	return cg.index;
}

static qse_size_t find_global (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	check_global_t cg;

	cg.name.ptr = name;
	cg.name.len = len;
	cg.index = QSE_LDA_NIL;
	cg.walk = QSE_LDA_WALK_FORWARD;

	qse_lda_walk (awk->parse.gbls, check_global, &cg);
	return cg.index;
}

static int add_global (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len, 
	qse_size_t line, int disabled)
{
	qse_size_t ngbls;

	/* check if it is a keyword */
	if (classify_ident (awk, name, len) != TOKEN_IDENT)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EKWRED, name, len, awk->token.lin);
		return -1;
	}

	/* check if it conflict with a builtin function name */
	if (qse_awk_getfnc (awk, name, len) != QSE_NULL)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EFNCRED, name, len, awk->token.lin);
		return -1;
	}

	/* check if it conflict with a function name */
	if (qse_map_search (awk->tree.funs, name, len) != QSE_NULL) 
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, name, len, line);
		return -1;
	}

	/* check if it conflict with a function name 
	 * caught in the function call table */
	if (qse_map_search (awk->parse.funs, name, len) != QSE_NULL)
	{
		SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, name, len, line);
		return -1;
	}

	/* check if it conflicts with other global variable names */
	if (find_global (awk, name, len) != QSE_LDA_NIL)
	{ 
		SETERR_ARG_LOC (awk, QSE_AWK_EDUPGBL, name, len, line);
		return -1;
	}

	ngbls = QSE_LDA_SIZE (awk->parse.gbls);
	if (ngbls >= QSE_AWK_MAX_GBLS)
	{
		SETERR_LIN (awk, QSE_AWK_EGBLTM, line);
		return -1;
	}

	if (qse_lda_insert (awk->parse.gbls, 
		QSE_LDA_SIZE(awk->parse.gbls), 
		(qse_char_t*)name, len) == QSE_LDA_NIL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
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
		SETERR_CODE (awk, QSE_AWK_EINVAL);
		return -1;
	}

	if (awk->tree.ngbls > awk->tree.ngbls_base) 
	{
		/* this function is not allowed after qse_awk_parse is called */
		SETERR_CODE (awk, QSE_AWK_ENOPER);
		return -1;
	}

	n = add_global (awk, name, len, 0, 0);

	/* update the count of the static globals. 
	 * the total global count has been updated inside add_global. */
	if (n >= 0) awk->tree.ngbls_base++; 

	return n;
}

int qse_awk_delgbl (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	qse_size_t n;
	
#define QSE_AWK_NUM_STATIC_GBLS \
	(QSE_AWK_MAX_GBL_ID-QSE_AWK_MIN_GBL_ID+1)

	if (awk->tree.ngbls > awk->tree.ngbls_base) 
	{
		/* this function is not allow after qse_awk_parse is called */
		SETERR_CODE (awk, QSE_AWK_ENOPER);
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

static qse_awk_t* collect_globals (qse_awk_t* awk)
{
	if (MATCH(awk,TOKEN_NEWLINE))
	{
		/* special check if the first name is on the 
		 * same line when QSE_AWK_NEWLINE is on */
		SETERR_CODE (awk, QSE_AWK_EVARMS);
		return QSE_NULL;
	}

	while (1) 
	{
		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			SETERR_TOK (awk, QSE_AWK_EBADVAR);
			return QSE_NULL;
		}

		if (add_global (
			awk,
			QSE_STR_PTR(awk->token.name),
			QSE_STR_LEN(awk->token.name),
			awk->token.lin, 0) <= -1) return QSE_NULL;

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

		if (!MATCH(awk,TOKEN_COMMA)) 
		{
			SETERR_TOK (awk, QSE_AWK_ECOMMA);
			return QSE_NULL;
		}

		do
		{
			if (get_token(awk) <= -1) return QSE_NULL;
		} 
		while (MATCH(awk,TOKEN_NEWLINE));
	}


	return awk;
}

static qse_awk_t* collect_locals (
	qse_awk_t* awk, qse_size_t nlcls, qse_bool_t istop)
{
	if (MATCH(awk,TOKEN_NEWLINE))
	{
		/* special check if the first name is on the 
		 * same line when QSE_AWK_NEWLINE is on */
		SETERR_CODE (awk, QSE_AWK_EVARMS);
		return QSE_NULL;
	}

	while (1) 
	{
		qse_xstr_t lcl;
		qse_size_t n;

		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			SETERR_TOK (awk, QSE_AWK_EBADVAR);
			return QSE_NULL;
		}

		lcl.ptr = QSE_STR_PTR(awk->token.name);
		lcl.len = QSE_STR_LEN(awk->token.name);

		/* check if it conflict with a builtin function name 
		 * function f() { local length; } */
		if (qse_awk_getfnc (awk, lcl.ptr, lcl.len) != QSE_NULL)
		{
			SETERR_ARG_LOC (awk, QSE_AWK_EFNCRED, lcl.ptr, lcl.len, awk->token.lin);
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
				SETERR_ARG_LOC (awk, QSE_AWK_EPARRED, lcl.ptr, lcl.len, awk->token.lin);
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
					SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, lcl.ptr, lcl.len, awk->token.lin);
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
			SETERR_ARG_LOC (awk, QSE_AWK_EDUPLCL, lcl.ptr, lcl.len, awk->token.lin);
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
				SETERR_ARG_LOC (awk, QSE_AWK_EDUPLCL, lcl.ptr, lcl.len, awk->token.lin);
				return QSE_NULL;
			}
		}

		if (QSE_LDA_SIZE(awk->parse.lcls) >= QSE_AWK_MAX_LCLS)
		{
			SETERR_LIN (awk, QSE_AWK_ELCLTM, awk->token.lin);
			return QSE_NULL;
		}

		if (qse_lda_insert (
			awk->parse.lcls,
			QSE_LDA_SIZE(awk->parse.lcls),
			lcl.ptr, lcl.len) == QSE_LDA_NIL)
		{
			SETERR_LIN (awk, QSE_AWK_ENOMEM, awk->token.lin);
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

		if (!MATCH(awk,TOKEN_COMMA))
		{
			SETERR_TOK (awk, QSE_AWK_ECOMMA);
			return QSE_NULL;
		}

		do
		{
			if (get_token(awk) <= -1) return QSE_NULL;
		}
		while (MATCH(awk,TOKEN_NEWLINE));
	}

	return awk;
}

static qse_awk_nde_t* parse_statement (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* nde;

	/* skip new lines before a statement */
	while (MATCH(awk,TOKEN_NEWLINE))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
	}

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* null statement */	
		nde = (qse_awk_nde_t*) 
			QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_t));
		if (nde == QSE_NULL) 
		{
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_NULL;
		nde->line = line;
		nde->next = QSE_NULL;

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_LBRACE)) 
	{
		/* a block statemnt { ... } */
		if (get_token(awk) <= -1) return QSE_NULL; 
		nde = parse_block_dc (
			awk, awk->ptoken.lin, QSE_FALSE);
	}
	else 
	{
		/* the statement id held in awk->parse.id.stmnt denotes
		 * the token id of the statement currently being parsed.
		 * the current statement id is saved here because the 
		 * statement id can be changed in parse_statement_nb.
		 * it will, in turn, call parse_statement which will
		 * eventually change the statement id. */
		int old_id = awk->parse.id.stmnt;

		/* set the current statement id */
		awk->parse.id.stmnt = awk->token.type;

		/* proceed parsing the statement */
		nde = parse_statement_nb (awk, line);

		/* restore the statement id saved previously */
		awk->parse.id.stmnt = old_id;
	}

	return nde;
}

/* parse a non-block statement */
static qse_awk_nde_t* parse_statement_nb (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* nde;

	/* keywords that don't require any terminating semicolon */
	if (MATCH(awk,TOKEN_IF)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		return parse_if (awk, line);
	}
	else if (MATCH(awk,TOKEN_WHILE)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		
		awk->parse.depth.cur.loop++;
		nde = parse_while (awk, line);
		awk->parse.depth.cur.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_FOR)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;

		awk->parse.depth.cur.loop++;
		nde = parse_for (awk, line);
		awk->parse.depth.cur.loop--;

		return nde;
	}

	/* keywords that require a terminating semicolon */
	if (MATCH(awk,TOKEN_DO)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;

		awk->parse.depth.cur.loop++;
		nde = parse_dowhile (awk, line);
		awk->parse.depth.cur.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_BREAK)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_break (awk, line);
	}
	else if (MATCH(awk,TOKEN_CONTINUE)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_continue (awk, line);
	}
	else if (MATCH(awk,TOKEN_RETURN)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_return (awk, line);
	}
	else if (MATCH(awk,TOKEN_EXIT)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_exit (awk, line);
	}
	else if (MATCH(awk,TOKEN_NEXT)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_next (awk, line);
	}
	else if (MATCH(awk,TOKEN_NEXTFILE)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_nextfile (awk, line, 0);
	}
	else if (MATCH(awk,TOKEN_NEXTOFILE))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_nextfile (awk, line, 1);
	}
	else if (MATCH(awk,TOKEN_DELETE)) 
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_delete (awk, line);
	}
	else if (MATCH(awk,TOKEN_RESET))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_reset (awk, line);
	}
	else if (MATCH(awk,TOKEN_PRINT))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_print (awk, line, QSE_AWK_NDE_PRINT);
	}
	else if (MATCH(awk,TOKEN_PRINTF))
	{
		if (get_token(awk) <= -1) return QSE_NULL;
		nde = parse_print (awk, line, QSE_AWK_NDE_PRINTF);
	}
	else 
	{
		nde = parse_expression (awk, line);
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
		SETERR_LIN (awk, QSE_AWK_ESTMEND, awk->ptoken.lin);
		return QSE_NULL;
	}

	return nde;
}

static qse_awk_nde_t* parse_expression (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* nde;

	if (awk->parse.depth.max.expr > 0 &&
	    awk->parse.depth.cur.expr >= awk->parse.depth.max.expr)
	{
		SETERR_LIN (awk, QSE_AWK_EEXPRNST, line);
		return QSE_NULL;
	}

	awk->parse.depth.cur.expr++;
	nde = parse_expression0 (awk, line);
	awk->parse.depth.cur.expr--;

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
		QSE_AWK_ASSOP_RS,
		QSE_AWK_ASSOP_LS,
		QSE_AWK_ASSOP_BAND,
		QSE_AWK_ASSOP_BXOR,
		QSE_AWK_ASSOP_BOR
	};

	if (awk->token.type >= TOKEN_ASSIGN &&
	    awk->token.type <= TOKEN_BOR_ASSIGN)
	{
		return assop[awk->token.type - TOKEN_ASSIGN];
	}

	return -1;
}


static qse_awk_nde_t* parse_expression0 (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* x, * y;
	qse_awk_nde_ass_t* nde;
	int opcode;

	x = parse_basic_expr (awk, line);
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
		SETERR_LIN (awk, QSE_AWK_EASSIGN, line);
		return QSE_NULL;
	}

	if (get_token(awk) <= -1) 
	{
		qse_awk_clrpt (awk, x);
		return QSE_NULL;
	}

	/*y = parse_basic_expr (awk);*/
	y = parse_expression (awk, awk->token.lin);
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

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_ASS;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->opcode = opcode;
	nde->left = x;
	nde->right = y;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_basic_expr (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* nde, * n1, * n2;
	
	nde = parse_logical_or (awk, line);
	if (nde == QSE_NULL) return QSE_NULL;

	if (MATCH(awk,TOKEN_QUEST))
	{ 
		qse_awk_nde_cnd_t* tmp;

		if (get_token(awk) <= -1) return QSE_NULL;

		/*n1 = parse_basic_expr (awk, awk->token.lin);*/
		n1 = parse_expression (awk, awk->token.lin);
		if (n1 == QSE_NULL) 
		{
			qse_awk_clrpt (awk, nde);
			return QSE_NULL;
		}

		if (!MATCH(awk,TOKEN_COLON)) 
		{
			SETERR_TOK (awk, QSE_AWK_ECOLON);
			return QSE_NULL;
		}
		if (get_token(awk) <= -1) return QSE_NULL;

		/*n2 = parse_basic_expr (awk, awk->token.lin);*/
		n2 = parse_expression (awk, awk->token.lin);
		if (n2 == QSE_NULL)
		{
			qse_awk_clrpt (awk, nde);
			qse_awk_clrpt (awk, n1);
			return QSE_NULL;
		}

		tmp = (qse_awk_nde_cnd_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_cnd_t));
		if (tmp == QSE_NULL)
		{
			qse_awk_clrpt (awk, nde);
			qse_awk_clrpt (awk, n1);
			qse_awk_clrpt (awk, n2);

			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		tmp->type = QSE_AWK_NDE_CND;
		tmp->line = line;
		tmp->next = QSE_NULL;
		tmp->test = nde;
		tmp->left = n1;
		tmp->right = n2;

		nde = (qse_awk_nde_t*)tmp;
	}

	return nde;
}

static qse_awk_nde_t* parse_binary_expr (
	qse_awk_t* awk, qse_size_t line, int skipnl, const binmap_t* binmap,
	qse_awk_nde_t*(*next_level_func)(qse_awk_t*,qse_size_t))
{
	qse_awk_nde_exp_t* nde;
	qse_awk_nde_t* left, * right;
	int opcode;

	left = next_level_func (awk, line);
	if (left == QSE_NULL) return QSE_NULL;
	
	while (1) 
	{
		const binmap_t* p = binmap;
		qse_bool_t matched = QSE_FALSE;

		while (p->token != TOKEN_EOF)
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
			if (get_token(awk) <= -1) 
			{
				qse_awk_clrpt (awk, left);
				return QSE_NULL; 
			}
		}
		while (skipnl && MATCH(awk,TOKEN_NEWLINE));

		right = next_level_func (awk, awk->token.lin);
		if (right == QSE_NULL) 
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL;
		}

		nde = (qse_awk_nde_exp_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_exp_t));
		if (nde == QSE_NULL) 
		{
			qse_awk_clrpt (awk, right);
			qse_awk_clrpt (awk, left);

			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_EXP_BIN;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->opcode = opcode; 
		nde->left = left;
		nde->right = right;

		left = (qse_awk_nde_t*)nde;
	}

	return left;
}

static qse_awk_nde_t* parse_logical_or (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_LOR, QSE_AWK_BINOP_LOR },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 1, map, parse_logical_and);
}

static qse_awk_nde_t* parse_logical_and (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_LAND, QSE_AWK_BINOP_LAND },
		{ TOKEN_EOF,  0 }
	};

	return parse_binary_expr (awk, line, 1, map, parse_in);
}

static qse_awk_nde_t* parse_in (qse_awk_t* awk, qse_size_t line)
{
	/* 
	static binmap_t map[] =
	{
		{ TOKEN_IN, QSE_AWK_BINOP_IN },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_regex_match);
	*/

	qse_awk_nde_exp_t* nde;
	qse_awk_nde_t* left, * right;
	qse_size_t line2;

	left = parse_regex_match (awk, line);
	if (left == QSE_NULL) return QSE_NULL;

	while (1)
	{
		if (!MATCH(awk,TOKEN_IN)) break;

		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL; 
		}

		line2 = awk->token.lin;

		right = parse_regex_match (awk, line2);
		if (right == QSE_NULL) 
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL;
		}

		if (!is_plain_var(right))
		{
			qse_awk_clrpt (awk, right);
			qse_awk_clrpt (awk, left);

			SETERR_LIN (awk, QSE_AWK_ENOTVAR, line2);
			return QSE_NULL;
		}

		nde = (qse_awk_nde_exp_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_exp_t));
		if (nde == QSE_NULL) 
		{
			qse_awk_clrpt (awk, right);
			qse_awk_clrpt (awk, left);

			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_EXP_BIN;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->opcode = QSE_AWK_BINOP_IN; 
		nde->left = left;
		nde->right = right;

		left = (qse_awk_nde_t*)nde;
	}

	return left;
}

static qse_awk_nde_t* parse_regex_match (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] =
	{
		{ TOKEN_TILDE, QSE_AWK_BINOP_MA },
		{ TOKEN_NM,    QSE_AWK_BINOP_NM },
		{ TOKEN_EOF,   0 },
	};

	return parse_binary_expr (awk, line, 0, map, parse_bitwise_or);
}

static qse_awk_nde_t* parse_bitwise_or (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_BOR, QSE_AWK_BINOP_BOR },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (
		awk, line, 0, map, parse_bitwise_xor);
}

static qse_awk_nde_t* parse_bitwise_xor (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_BXOR, QSE_AWK_BINOP_BXOR },
		{ TOKEN_EOF,  0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_bitwise_and);
}

static qse_awk_nde_t* parse_bitwise_and (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_BAND, QSE_AWK_BINOP_BAND },
		{ TOKEN_EOF,  0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_equality);
}

static qse_awk_nde_t* parse_equality (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_EQ, QSE_AWK_BINOP_EQ },
		{ TOKEN_NE, QSE_AWK_BINOP_NE },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_relational);
}

static qse_awk_nde_t* parse_relational (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_GT, QSE_AWK_BINOP_GT },
		{ TOKEN_GE, QSE_AWK_BINOP_GE },
		{ TOKEN_LT, QSE_AWK_BINOP_LT },
		{ TOKEN_LE, QSE_AWK_BINOP_LE },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, 
		((awk->option & QSE_AWK_EXTRAOPS)? parse_shift: parse_concat));
}

static qse_awk_nde_t* parse_shift (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_LS, QSE_AWK_BINOP_LS },
		{ TOKEN_RS, QSE_AWK_BINOP_RS },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_concat);
}

static qse_awk_nde_t* parse_concat (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_exp_t* nde;
	qse_awk_nde_t* left, * right;

	left = parse_additive (awk, line);
	if (left == QSE_NULL) return QSE_NULL;

	while (1)
	{
		if (MATCH(awk,TOKEN_PERIOD))
		{
			if (!(awk->option & QSE_AWK_EXPLICIT)) break;
			if (get_token(awk) <= -1) return QSE_NULL;
		}
		else if (MATCH(awk,TOKEN_LPAREN) ||
	                 MATCH(awk,TOKEN_DOLLAR) ||
			 MATCH(awk,TOKEN_PLUS) ||
			 MATCH(awk,TOKEN_MINUS) ||
			 MATCH(awk,TOKEN_PLUSPLUS) ||
			 MATCH(awk,TOKEN_MINUSMINUS) ||
			 MATCH(awk,TOKEN_LNOT) ||
			 awk->token.type >= TOKEN_GETLINE)
		{
			/* TODO: is the check above sufficient? */
			if (!(awk->option & QSE_AWK_IMPLICIT)) break;
		}
		else break;

		right = parse_additive (awk, awk->token.lin);
		if (right == QSE_NULL) 
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL;
		}

		nde = (qse_awk_nde_exp_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_exp_t));
		if (nde == QSE_NULL)
		{
			qse_awk_clrpt (awk, left);
			qse_awk_clrpt (awk, right);

			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_EXP_BIN;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->opcode = QSE_AWK_BINOP_CONCAT;
		nde->left = left;
		nde->right = right;
		
		left = (qse_awk_nde_t*)nde;
	}

	return left;
}

static qse_awk_nde_t* parse_additive (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_PLUS, QSE_AWK_BINOP_PLUS },
		{ TOKEN_MINUS, QSE_AWK_BINOP_MINUS },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_multiplicative);
}

static qse_awk_nde_t* parse_multiplicative (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_MUL,  QSE_AWK_BINOP_MUL },
		{ TOKEN_DIV,  QSE_AWK_BINOP_DIV },
		{ TOKEN_IDIV, QSE_AWK_BINOP_IDIV },
		{ TOKEN_MOD,  QSE_AWK_BINOP_MOD },
		/* { TOKEN_EXP, QSE_AWK_BINOP_EXP }, */
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_unary);
}

static qse_awk_nde_t* parse_unary (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_exp_t* nde; 
	qse_awk_nde_t* left;
	int opcode;

	opcode = (MATCH(awk,TOKEN_PLUS))?  QSE_AWK_UNROP_PLUS:
	         (MATCH(awk,TOKEN_MINUS))? QSE_AWK_UNROP_MINUS:
	         (MATCH(awk,TOKEN_LNOT))?  QSE_AWK_UNROP_LNOT:
	         ((awk->option & QSE_AWK_EXTRAOPS) && MATCH(awk,TOKEN_TILDE))? 
	                                   QSE_AWK_UNROP_BNOT: -1;

	/*if (opcode <= -1) return parse_increment (awk);*/
	if (opcode <= -1) return parse_exponent (awk, line);

	if (get_token(awk) <= -1) return QSE_NULL;

	if (awk->parse.depth.max.expr > 0 &&
	    awk->parse.depth.cur.expr >= awk->parse.depth.max.expr)
	{
		SETERR_LIN (awk, QSE_AWK_EEXPRNST, awk->token.lin);
		return QSE_NULL;
	}
	awk->parse.depth.cur.expr++;
	left = parse_unary (awk, awk->token.lin);
	awk->parse.depth.cur.expr--;
	if (left == QSE_NULL) return QSE_NULL;

	nde = (qse_awk_nde_exp_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_exp_t));
	if (nde == QSE_NULL)
	{
		qse_awk_clrpt (awk, left);

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_EXP_UNR;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = QSE_NULL;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_exponent (qse_awk_t* awk, qse_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_EXP, QSE_AWK_BINOP_EXP },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_unary_exp);
}

static qse_awk_nde_t* parse_unary_exp (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_exp_t* nde; 
	qse_awk_nde_t* left;
	int opcode;

	opcode = (MATCH(awk,TOKEN_PLUS))?  QSE_AWK_UNROP_PLUS:
	         (MATCH(awk,TOKEN_MINUS))? QSE_AWK_UNROP_MINUS:
	         (MATCH(awk,TOKEN_LNOT))?  QSE_AWK_UNROP_LNOT:
	         ((awk->option & QSE_AWK_EXTRAOPS) && MATCH(awk,TOKEN_TILDE))? 
	                                   QSE_AWK_UNROP_BNOT: -1;

	if (opcode <= -1) return parse_increment (awk, line);

	if (get_token(awk) <= -1) return QSE_NULL;

	if (awk->parse.depth.max.expr > 0 &&
	    awk->parse.depth.cur.expr >= awk->parse.depth.max.expr)
	{
		SETERR_LIN (awk, QSE_AWK_EEXPRNST, awk->token.lin);
		return QSE_NULL;
	}
	awk->parse.depth.cur.expr++;
	left = parse_unary (awk, awk->token.lin);
	awk->parse.depth.cur.expr--;
	if (left == QSE_NULL) return QSE_NULL;

	nde = (qse_awk_nde_exp_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_exp_t));
	if (nde == QSE_NULL)
	{
		qse_awk_clrpt (awk, left);

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_EXP_UNR;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = QSE_NULL;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_increment (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_exp_t* nde;
	qse_awk_nde_t* left;
	int type, opcode, opcode1, opcode2;

	/* check for prefix increment operator */
	opcode1 = MATCH(awk,TOKEN_PLUSPLUS)? QSE_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? QSE_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1)
	{
		if (get_token(awk) <= -1) return QSE_NULL;
	}

	left = parse_primary (awk, line);
	if (left == QSE_NULL) return QSE_NULL;

	/* check for postfix increment operator */
	opcode2 = MATCH(awk,TOKEN_PLUSPLUS)? QSE_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? QSE_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1 && opcode2 != -1)
	{
		/* both prefix and postfix increment operator. 
		 * not allowed */
		qse_awk_clrpt (awk, left);

		SETERR_LIN (awk, QSE_AWK_EPREPST, line);
		return QSE_NULL;
	}
	else if (opcode1 == -1 && opcode2 == -1)
	{
		/* no increment operators */
		return left;
	}
	else if (opcode1 != -1) 
	{
		/* prefix increment operator */
		type = QSE_AWK_NDE_EXP_INCPRE;
		opcode = opcode1;
	}
	else if (opcode2 != -1) 
	{
		/* postfix increment operator */
		type = QSE_AWK_NDE_EXP_INCPST;
		opcode = opcode2;

		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL;
		}
	}

	if (!is_var(left) && left->type != QSE_AWK_NDE_POS)
	{
		qse_awk_clrpt (awk, left);
		SETERR_LIN (awk, QSE_AWK_EINCDECOPR, line);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_exp_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_exp_t));
	if (nde == QSE_NULL)
	{
		qse_awk_clrpt (awk, left);
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = type;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = QSE_NULL;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_primary_nogetline (qse_awk_t* awk, qse_size_t line)
{
	if (MATCH(awk,TOKEN_IDENT))  
	{
		return parse_primary_ident (awk, line);
	}
	else if (MATCH(awk,TOKEN_INT)) 
	{
		qse_awk_nde_int_t* nde;

		nde = (qse_awk_nde_int_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_int_t));
		if (nde == QSE_NULL)
		{
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_INT;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->val = qse_awk_strxtolong (awk, 
			QSE_STR_PTR(awk->token.name), 
			QSE_STR_LEN(awk->token.name), 0, QSE_NULL);
		nde->str = QSE_AWK_STRXDUP (awk,
			QSE_STR_PTR(awk->token.name),
			QSE_STR_LEN(awk->token.name));
		if (nde->str == QSE_NULL)
		{
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;			
		}
		nde->len = QSE_STR_LEN(awk->token.name);

		QSE_ASSERT (
			QSE_STR_LEN(awk->token.name) ==
			qse_strlen(QSE_STR_PTR(awk->token.name)));

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, nde->str);
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;			
		}

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_REAL)) 
	{
		qse_awk_nde_real_t* nde;

		nde = (qse_awk_nde_real_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_real_t));
		if (nde == QSE_NULL)
		{
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_REAL;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->val = qse_awk_strxtoreal (awk, 
			QSE_STR_PTR(awk->token.name), 
			QSE_STR_LEN(awk->token.name), QSE_NULL);
		nde->str = QSE_AWK_STRXDUP (awk,
			QSE_STR_PTR(awk->token.name),
			QSE_STR_LEN(awk->token.name));
		if (nde->str == QSE_NULL)
		{
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;			
		}
		nde->len = QSE_STR_LEN(awk->token.name);

		QSE_ASSERT (
			QSE_STR_LEN(awk->token.name) ==
			qse_strlen(QSE_STR_PTR(awk->token.name)));

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, nde->str);
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;			
		}

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_STR))  
	{
		qse_awk_nde_str_t* nde;

		nde = (qse_awk_nde_str_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_str_t));
		if (nde == QSE_NULL)
		{
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_STR;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->len = QSE_STR_LEN(awk->token.name);
		nde->ptr = QSE_AWK_STRXDUP (awk,
			QSE_STR_PTR(awk->token.name), nde->len);
		if (nde->ptr == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, nde);
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
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
	else if (MATCH(awk,TOKEN_DIV))
	{
		qse_awk_nde_rex_t* nde;
		qse_awk_errnum_t errnum;

		/* the regular expression is tokenized here because 
		 * of the context-sensitivity of the slash symbol.
		 * if TOKEN_DIV is seen as a primary, it tries to compile
		 * it as a regular expression */
		SET_TOKEN_TYPE (awk, &awk->token, TOKEN_REX);
		qse_str_clear (awk->token.name);
		if (get_rexstr (awk, &awk->token) <= -1) return QSE_NULL;

		QSE_ASSERT (MATCH(awk,TOKEN_REX));

		nde = (qse_awk_nde_rex_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_rex_t));
		if (nde == QSE_NULL)
		{
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_REX;
		nde->line = line;
		nde->next = QSE_NULL;

		nde->len = QSE_STR_LEN(awk->token.name);
		nde->ptr = QSE_AWK_STRXDUP (awk,
			QSE_STR_PTR(awk->token.name),
			QSE_STR_LEN(awk->token.name));
		if (nde->ptr == QSE_NULL)
		{
			QSE_AWK_FREE (awk, nde);
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->code = QSE_AWK_BUILDREX (awk,
			QSE_STR_PTR(awk->token.name), 
			QSE_STR_LEN(awk->token.name), 
			&errnum);
		if (nde->code == QSE_NULL)
		{
			QSE_AWK_FREE (awk, nde->ptr);
			QSE_AWK_FREE (awk, nde);

			SETERR_LIN (awk, errnum, line);
			return QSE_NULL;
		}

		if (get_token(awk) <= -1) 
		{
			QSE_AWK_FREE (awk, nde->ptr);
			QSE_AWK_FREE (awk, nde->code);
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;			
		}

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_DOLLAR)) 
	{
		qse_awk_nde_pos_t* nde;
		qse_awk_nde_t* prim;

		if (get_token(awk)) return QSE_NULL;
		
		prim = parse_primary (awk, awk->token.lin);
		if (prim == QSE_NULL) return QSE_NULL;

		nde = (qse_awk_nde_pos_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_pos_t));
		if (nde == QSE_NULL) 
		{
			qse_awk_clrpt (awk, prim);
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_POS;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->val = prim;

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		qse_awk_nde_t* nde;
		qse_awk_nde_t* last;

		/* eat up the left parenthesis */
		if (get_token(awk) <= -1) return QSE_NULL;

		/* parse the sub-expression inside the parentheses */
		nde = parse_expression (awk, awk->token.lin);
		if (nde == QSE_NULL) return QSE_NULL;

		/* parse subsequent expressions separated by a comma, if any */
		last = nde;
		QSE_ASSERT (last->next == QSE_NULL);

		while (MATCH(awk,TOKEN_COMMA))
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
			while (MATCH(awk,TOKEN_NEWLINE));

			tmp = parse_expression (awk, awk->token.lin);
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
		if (!MATCH(awk,TOKEN_RPAREN)) 
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

			if ((awk->parse.id.stmnt != TOKEN_PRINT &&
			     awk->parse.id.stmnt != TOKEN_PRINTF) ||
			    awk->parse.depth.cur.expr != 1)
			{
				if (!MATCH(awk,TOKEN_IN))
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
				SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
				return QSE_NULL;
			}	

			tmp->type = QSE_AWK_NDE_GRP;
			tmp->line = line;
			tmp->next = QSE_NULL;
			tmp->body = nde;		

			nde = (qse_awk_nde_t*)tmp;
		}
		/* ----------------- */

		return nde;
	}
	else if (MATCH(awk,TOKEN_GETLINE)) 
	{
		qse_awk_nde_getline_t* nde;
		qse_awk_nde_t* var = QSE_NULL;
		qse_awk_nde_t* in = QSE_NULL;

		if (get_token(awk) <= -1) return QSE_NULL;

		if (MATCH(awk,TOKEN_IDENT))
		{
			/* getline var */
			var = parse_primary (awk, awk->token.lin);
			if (var == QSE_NULL) return QSE_NULL;
		}

		if (MATCH(awk, TOKEN_LT))
		{
			/* getline [var] < file */
			if (get_token(awk) <= -1)
			{
				if (var != QSE_NULL) qse_awk_clrpt (awk, var);
				return QSE_NULL;
			}

			/* TODO: is this correct? */
			/*in = parse_expression (awk);*/
			in = parse_primary (awk, awk->token.lin);
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
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_GETLINE;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->var = var;
		nde->in_type = (in == QSE_NULL)? 
			QSE_AWK_IN_CONSOLE: QSE_AWK_IN_FILE;
		nde->in = in;

		return (qse_awk_nde_t*)nde;
	}

	/* valid expression introducer is expected */
	if (MATCH(awk,TOKEN_NEWLINE))
	{
		SETERR_ARG_LOC (
			awk, QSE_AWK_EEXPRNR, 
			QSE_STR_PTR(awk->ptoken.name), 
			QSE_STR_LEN(awk->ptoken.name),
			awk->ptoken.lin
		);
	}
	else SETERR_TOK (awk, QSE_AWK_EEXPRNR);

	return QSE_NULL;
}

static qse_awk_nde_t* parse_primary (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* left;
	qse_awk_nde_getline_t* nde;
	qse_awk_nde_t* var;

	left = parse_primary_nogetline (awk, line);

	do
	{
		int intype = -1;

		if (awk->option & QSE_AWK_RIO)
		{
			if (MATCH(awk,TOKEN_BOR)) 
			{
				intype = QSE_AWK_IN_PIPE;
			}
			else if (MATCH(awk,TOKEN_LOR)) 
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
	
		if (awk->ntoken.type != TOKEN_GETLINE) break;

		var = QSE_NULL;

		/* consume ntoken */
		get_token (awk);

		/* get the next token */
		if (get_token(awk) <= -1)
		{
			qse_awk_clrpt (awk, left);
			return QSE_NULL;
		}

		/* TODO: is this correct? */
		if (MATCH(awk,TOKEN_IDENT))
		{
			/* command | getline var 
			 * command || getline var */

			var = parse_primary_ident (awk, awk->token.lin);
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

			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_GETLINE;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->var = var;
		nde->in_type = intype;
		nde->in = left;

		left = (qse_awk_nde_t*)nde;
	}
	while (1);

	return left;
}

static qse_awk_nde_t* parse_primary_ident (qse_awk_t* awk, qse_size_t line)
{
	qse_char_t* name_dup;
	qse_size_t name_len;
	qse_awk_fnc_t* fnc;
	qse_size_t idxa;

	QSE_ASSERT (MATCH(awk,TOKEN_IDENT));

	name_dup = QSE_AWK_STRXDUP (awk,
		QSE_STR_PTR(awk->token.name),
		QSE_STR_LEN(awk->token.name));
	if (name_dup == QSE_NULL) 
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}
	name_len = QSE_STR_LEN(awk->token.name);

	if (get_token(awk) <= -1) 
	{
		QSE_AWK_FREE (awk, name_dup);
		return QSE_NULL;			
	}

	/* check if name_dup is an intrinsic function name */
	fnc = qse_awk_getfnc (awk, name_dup, name_len);
	if (fnc != QSE_NULL)
	{
		qse_awk_nde_t* nde;

		if (!MATCH(awk,TOKEN_LPAREN))
		{
			if (fnc->dfl0)
			{
				/* handles a function that assumes () 
				 * when () is missing */
				nde = parse_fncall (
					awk, name_dup, name_len, fnc, line, 1);
				if (nde == QSE_NULL) 
					QSE_AWK_FREE (awk, name_dup);
				return (qse_awk_nde_t*)nde;
			}

			/* an intrinsic function should be in the form 
		 	 * of the function call */
			QSE_AWK_FREE (awk, name_dup);
			SETERR_TOK (awk, QSE_AWK_ELPAREN);
			return QSE_NULL;
		}

		nde = parse_fncall (awk, name_dup, name_len, fnc, line, 0);
		if (nde == QSE_NULL) QSE_AWK_FREE (awk, name_dup);
		return (qse_awk_nde_t*)nde;
	}

	/* now we know that name_dup is a normal identifier. */
	if (MATCH(awk,TOKEN_LBRACK)) 
	{
		qse_awk_nde_t* nde;
		nde = parse_hashidx (awk, name_dup, name_len, line);
		if (nde == QSE_NULL) QSE_AWK_FREE (awk, name_dup);
		return (qse_awk_nde_t*)nde;
	}
	else if ((idxa = qse_lda_rsearch (awk->parse.lcls, QSE_LDA_SIZE(awk->parse.lcls), name_dup, name_len)) != QSE_LDA_NIL)
	{
		/* local variable */

		qse_awk_nde_var_t* nde;

		if (MATCH(awk,TOKEN_LPAREN))
		{
			/* a local variable is not a function */
			SETERR_ARG_LOC (awk, QSE_AWK_EFUNNAME, name_dup, name_len, line);
			QSE_AWK_FREE (awk, name_dup);
			return QSE_NULL;
		}

		nde = (qse_awk_nde_var_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_var_t));
		if (nde == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, name_dup);
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_LCL;
		nde->line = line;
		nde->next = QSE_NULL;
		/*nde->id.name.ptr = QSE_NULL;*/
		nde->id.name.ptr = name_dup;
		nde->id.name.len = name_len;
		nde->id.idxa = idxa;
		nde->idx = QSE_NULL;

		return (qse_awk_nde_t*)nde;
	}
	else if ((idxa = qse_lda_search (awk->parse.params, 0, name_dup, name_len)) != QSE_LDA_NIL)
	{
		/* parameter */

		qse_awk_nde_var_t* nde;

		if (MATCH(awk,TOKEN_LPAREN))
		{
			/* a parameter is not a function */
			SETERR_ARG_LOC (awk, QSE_AWK_EFUNNAME, name_dup, name_len, line);
			QSE_AWK_FREE (awk, name_dup);
			return QSE_NULL;
		}

		nde = (qse_awk_nde_var_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_var_t));
		if (nde == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, name_dup);
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_ARG;
		nde->line = line;
		nde->next = QSE_NULL;
		/*nde->id.name = QSE_NULL;*/
		nde->id.name.ptr = name_dup;
		nde->id.name.len = name_len;
		nde->id.idxa = idxa;
		nde->idx = QSE_NULL;

		return (qse_awk_nde_t*)nde;
	}
	else if ((idxa = get_global (awk, name_dup, name_len)) != QSE_LDA_NIL)
	{
		/* global variable */

		qse_awk_nde_var_t* nde;

		if (MATCH(awk,TOKEN_LPAREN))
		{
			/* a global variable is not a function */
			SETERR_ARG_LOC (awk, QSE_AWK_EFUNNAME, name_dup, name_len, line);
			QSE_AWK_FREE (awk, name_dup);
			return QSE_NULL;
		}

		nde = (qse_awk_nde_var_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_var_t));
		if (nde == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, name_dup);
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		nde->type = QSE_AWK_NDE_GBL;
		nde->line = line;
		nde->next = QSE_NULL;
		/*nde->id.name = QSE_NULL;*/
		nde->id.name.ptr = name_dup;
		nde->id.name.len = name_len;
		nde->id.idxa = idxa;
		nde->idx = QSE_NULL;

		return (qse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		/* function call */
		qse_awk_nde_t* nde;

		if (awk->option & QSE_AWK_IMPLICIT)
		{
			if (qse_map_search (awk->parse.named, 
				name_dup, name_len) != QSE_NULL)
			{
				/* a function call conflicts with a named variable */
				SETERR_ARG_LOC (awk, QSE_AWK_EVARRED, name_dup, name_len, line);
				QSE_AWK_FREE (awk, name_dup);
				return QSE_NULL;
			}
		}

		nde = parse_fncall (awk, name_dup, name_len, QSE_NULL, line,  0);
		if (nde == QSE_NULL) QSE_AWK_FREE (awk, name_dup);
		return (qse_awk_nde_t*)nde;
	}	
	else 
	{
		/* named variable */
		qse_awk_nde_var_t* nde;

		nde = (qse_awk_nde_var_t*) QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(qse_awk_nde_var_t));
		if (nde == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, name_dup);
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}

		if (awk->option & QSE_AWK_IMPLICIT) 
		{
			qse_bool_t iscur = QSE_FALSE;

			/* the name should not conflict with a function name */
			/* check if it is a builtin function */
			if (qse_awk_getfnc (awk, name_dup, name_len) != QSE_NULL)
			{
				SETERR_ARG_LOC (awk, QSE_AWK_EFNCRED, name_dup, name_len, line);
				goto exit_func;
			}

			/* check if it is an AWK function */
			if (awk->tree.cur_fun.ptr != QSE_NULL)
			{
				iscur = (qse_strxncmp (
					awk->tree.cur_fun.ptr, awk->tree.cur_fun.len, 
					name_dup, name_len) == 0);
			}

			if (iscur || qse_map_search (awk->tree.funs, name_dup, name_len) != QSE_NULL) 
			{
				/* the function is defined previously */
				SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, name_dup, name_len, line);
				goto exit_func;
			}

			if (qse_map_search (awk->parse.funs, 
				name_dup, name_len) != QSE_NULL)
			{
				/* is it one of the function calls found so far? */
				SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, name_dup, name_len, line);
				goto exit_func;
			}

			nde->type = QSE_AWK_NDE_NAMED;
			nde->line = line;
			nde->next = QSE_NULL;
			nde->id.name.ptr = name_dup;
			nde->id.name.len = name_len;
			nde->id.idxa = (qse_size_t)-1;
			nde->idx = QSE_NULL;

			/* collect unique instances of a named variables for reference */
			if (qse_map_upsert (awk->parse.named,
				name_dup, name_len, 
				&line, QSE_SIZEOF(line)) == QSE_NULL)
			{
				  SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
				  goto exit_func;
			}

			return (qse_awk_nde_t*)nde;
		}

		/* undefined variable */
		SETERR_ARG_LOC (awk, QSE_AWK_EUNDEF, name_dup, name_len, line);

	exit_func:
		QSE_AWK_FREE (awk, name_dup);
		QSE_AWK_FREE (awk, nde);

		return QSE_NULL;
	}
}

static qse_awk_nde_t* parse_hashidx (
	qse_awk_t* awk, qse_char_t* name, qse_size_t name_len, qse_size_t line)
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

		tmp = parse_expression (awk, awk->token.lin);
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
	while (MATCH(awk,TOKEN_COMMA));

	QSE_ASSERT (idx != QSE_NULL);

	if (!MATCH(awk,TOKEN_RBRACK)) 
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
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	/* search the local variable list */
	idxa = qse_lda_rsearch (
		awk->parse.lcls, 
		QSE_LDA_SIZE(awk->parse.lcls),
		name,
		name_len
	);
	if (idxa != QSE_LDA_NIL)
	{
		nde->type = QSE_AWK_NDE_LCLIDX;
		nde->line = line;
		nde->next = QSE_NULL;
		/*nde->id.name = QSE_NULL; */
		nde->id.name.ptr = name;
		nde->id.name.len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (qse_awk_nde_t*)nde;
	}

	/* search the parameter name list */
	idxa = qse_lda_search (awk->parse.params, 0, name, name_len);
	if (idxa != QSE_LDA_NIL)
	{
		nde->type = QSE_AWK_NDE_ARGIDX;
		nde->line = line;
		nde->next = QSE_NULL;
		/*nde->id.name = QSE_NULL; */
		nde->id.name.ptr = name;
		nde->id.name.len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (qse_awk_nde_t*)nde;
	}

	/* gets the global variable index */
	idxa = get_global (awk, name, name_len);
	if (idxa != QSE_LDA_NIL)
	{
		nde->type = QSE_AWK_NDE_GBLIDX;
		nde->line = line;
		nde->next = QSE_NULL;
		/*nde->id.name = QSE_NULL;*/
		nde->id.name.ptr = name;
		nde->id.name.len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (qse_awk_nde_t*)nde;
	}

	if (awk->option & QSE_AWK_IMPLICIT) 
	{
		qse_bool_t iscur = QSE_FALSE;

		/* check if it is a builtin function */
		if (qse_awk_getfnc (awk, name, name_len) != QSE_NULL)
		{
			SETERR_ARG_LOC (awk, QSE_AWK_EFNCRED, name, name_len, line);
			goto exit_func;
		}

		/* check if it is an AWK function */
		if (awk->tree.cur_fun.ptr != QSE_NULL)
		{
			iscur = (qse_strxncmp (
				awk->tree.cur_fun.ptr, awk->tree.cur_fun.len, 
				name, name_len) == 0);
		}

		if (iscur || qse_map_search (awk->tree.funs, name, name_len) != QSE_NULL) 
		{
			/* the function is defined previously */
			SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, name, name_len, line);
			goto exit_func;
		}

		if (qse_map_search (
			awk->parse.funs, name, name_len) != QSE_NULL)
		{
			/* is it one of the function calls found so far? */
			SETERR_ARG_LOC (awk, QSE_AWK_EFUNRED, name, name_len, line);
			goto exit_func;
		}

		nde->type = QSE_AWK_NDE_NAMEDIDX;
		nde->line = line;
		nde->next = QSE_NULL;
		nde->id.name.ptr = name;
		nde->id.name.len = name_len;
		nde->id.idxa = (qse_size_t)-1;
		nde->idx = idx;

		return (qse_awk_nde_t*)nde;
	}

	/* undefined variable */
	SETERR_ARG_LOC (awk, QSE_AWK_EUNDEF, name, name_len, line);


exit_func:
	qse_awk_clrpt (awk, idx);
	QSE_AWK_FREE (awk, nde);

	return QSE_NULL;
}

static qse_awk_nde_t* parse_fncall (
	qse_awk_t* awk, qse_char_t* name, qse_size_t name_len, 
	qse_awk_fnc_t* fnc, qse_size_t line, int noarg)
{
	qse_awk_nde_t* head, * curr, * nde;
	qse_awk_nde_call_t* call;
	qse_size_t nargs;

	head = curr = QSE_NULL;
	nargs = 0;

	if (noarg) goto make_node;
	if (get_token(awk) <= -1) return QSE_NULL;

	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no parameters to the function call */
		if (get_token(awk) <= -1) return QSE_NULL;
	}
	else 
	{
		/* parse function parameters */

		while (1) 
		{
			nde = parse_expression (awk, awk->token.lin);
			if (nde == QSE_NULL) 
			{
				if (head != QSE_NULL) qse_awk_clrpt (awk, head);
				return QSE_NULL;
			}
	
			if (head == QSE_NULL) head = nde;
			else curr->next = nde;
			curr = nde;

			nargs++;

			if (MATCH(awk,TOKEN_RPAREN)) 
			{
				if (get_token(awk) <= -1) 
				{
					if (head != QSE_NULL) 
						qse_awk_clrpt (awk, head);
					return QSE_NULL;
				}
				break;
			}

			if (!MATCH(awk,TOKEN_COMMA)) 
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
			while (MATCH(awk,TOKEN_NEWLINE));
		}

	}

make_node:
	call = (qse_awk_nde_call_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_call_t));
	if (call == QSE_NULL) 
	{
		if (head != QSE_NULL) qse_awk_clrpt (awk, head);

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	if (fnc != QSE_NULL)
	{
		call->type = QSE_AWK_NDE_FNC;
		call->line = line;
		call->next = QSE_NULL;

		/*call->what.fnc = fnc; */
		call->what.fnc.name.ptr = name;
		call->what.fnc.name.len = name_len;

		/* NOTE: oname is the original as in the fnc table.
		 *       it would not duplicated here and not freed in
		 *       qse_awk_clrpt either. so qse_awk_delfnc between
		 *       qse_awk_parse and qse_awk_run may cause the program 
		 *       to fail. */
		call->what.fnc.oname.ptr = fnc->name.ptr;
		call->what.fnc.oname.len = fnc->name.len;

		call->what.fnc.arg.min = fnc->arg.min;
		call->what.fnc.arg.max = fnc->arg.max;
		call->what.fnc.arg.spec = fnc->arg.spec;
		call->what.fnc.handler = fnc->handler;

		call->args = head;
		call->nargs = nargs;
	}
	else
	{
		call->type = QSE_AWK_NDE_FUN;
		call->line = line;
		call->next = QSE_NULL;
		call->what.fun.name.ptr = name; 
		call->what.fun.name.len = name_len;
		call->args = head;
		call->nargs = nargs;

		/* store a non-builtin function call into the parse.funs table */
		if (qse_map_upsert (
			awk->parse.funs, name, name_len,
			&line, QSE_SIZEOF(line)) == QSE_NULL)
		{
			QSE_AWK_FREE (awk, call);
			if (head != QSE_NULL) qse_awk_clrpt (awk, head);
			SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
			return QSE_NULL;
		}
	}

	return (qse_awk_nde_t*)call;
}

static qse_awk_nde_t* parse_if (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* test;
	qse_awk_nde_t* then_part;
	qse_awk_nde_t* else_part;
	qse_awk_nde_if_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		SETERR_TOK (awk, QSE_AWK_ELPAREN);
		return QSE_NULL;

	}
	if (get_token(awk) <= -1) return QSE_NULL;

	test = parse_expression (awk, awk->token.lin);
	if (test == QSE_NULL) return QSE_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
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

	then_part = parse_statement (awk, awk->token.lin);
	if (then_part == QSE_NULL) 
	{
		qse_awk_clrpt (awk, test);
		return QSE_NULL;
	}

	/* skip any new lines before the else block */
	while (MATCH(awk,TOKEN_NEWLINE))
	{
		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, then_part);
			qse_awk_clrpt (awk, test);
			return QSE_NULL;
		}
	} 

	if (MATCH(awk,TOKEN_ELSE)) 
	{
		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, then_part);
			qse_awk_clrpt (awk, test);
			return QSE_NULL;
		}

		else_part = parse_statement (awk, awk->ptoken.lin);
		if (else_part == QSE_NULL) 
		{
			qse_awk_clrpt (awk, then_part);
			qse_awk_clrpt (awk, test);
			return QSE_NULL;
		}
	}
	else else_part = QSE_NULL;

	nde = (qse_awk_nde_if_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_if_t));
	if (nde == QSE_NULL) 
	{
		qse_awk_clrpt (awk, else_part);
		qse_awk_clrpt (awk, then_part);
		qse_awk_clrpt (awk, test);

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_IF;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->test = test;
	nde->then_part = then_part;
	nde->else_part = else_part;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_while (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* test, * body;
	qse_awk_nde_while_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		SETERR_TOK (awk, QSE_AWK_ELPAREN);
		return QSE_NULL;
	}
	if (get_token(awk) <= -1) return QSE_NULL;

	test = parse_expression (awk, awk->token.lin);
	if (test == QSE_NULL) return QSE_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
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

	body = parse_statement (awk, awk->token.lin);
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

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_WHILE;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->test = test;
	nde->body = body;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_for (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* init, * test, * incr, * body;
	qse_awk_nde_for_t* nde; 
	qse_awk_nde_foreach_t* nde2;

	if (!MATCH(awk,TOKEN_LPAREN))
	{
		SETERR_TOK (awk, QSE_AWK_ELPAREN);
		return QSE_NULL;
	}
	if (get_token(awk) <= -1) return QSE_NULL;
		
	if (MATCH(awk,TOKEN_SEMICOLON)) init = QSE_NULL;
	else 
	{
		/* this line is very ugly. it checks the entire next 
		 * expression or the first element in the expression
		 * is wrapped by a parenthesis */
		int no_foreach = MATCH(awk,TOKEN_LPAREN);

		init = parse_expression (awk, awk->token.lin);
		if (init == QSE_NULL) return QSE_NULL;

		if (!no_foreach && init->type == QSE_AWK_NDE_EXP_BIN &&
		    ((qse_awk_nde_exp_t*)init)->opcode == QSE_AWK_BINOP_IN &&
		    is_plain_var(((qse_awk_nde_exp_t*)init)->left))
		{	
			/* switch to foreach */
			
			if (!MATCH(awk,TOKEN_RPAREN))
			{
				qse_awk_clrpt (awk, init);
				SETERR_TOK (awk, QSE_AWK_ERPAREN);
				return QSE_NULL;
			}

			if (get_token(awk) <= -1) 
			{
				qse_awk_clrpt (awk, init);
				return QSE_NULL;
			}	
			
			body = parse_statement (awk, awk->token.lin);
			if (body == QSE_NULL) 
			{
				qse_awk_clrpt (awk, init);
				return QSE_NULL;
			}

			nde2 = (qse_awk_nde_foreach_t*) QSE_AWK_ALLOC (
				awk, QSE_SIZEOF(qse_awk_nde_foreach_t));
			if (nde2 == QSE_NULL)
			{
				qse_awk_clrpt (awk, init);
				qse_awk_clrpt (awk, body);

				SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
				return QSE_NULL;
			}

			nde2->type = QSE_AWK_NDE_FOREACH;
			nde2->line = line;
			nde2->next = QSE_NULL;
			nde2->test = init;
			nde2->body = body;

			return (qse_awk_nde_t*)nde2;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			qse_awk_clrpt (awk, init);
			SETERR_TOK (awk, QSE_AWK_ESCOLON);
			return QSE_NULL;
		}
	}

	do
	{
		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, init);
			return QSE_NULL;
		}

		/* skip new lines after the first semicolon */
	} 
	while (MATCH(awk,TOKEN_NEWLINE));

	if (MATCH(awk,TOKEN_SEMICOLON)) test = QSE_NULL;
	else 
	{
		test = parse_expression (awk, awk->token.lin);
		if (test == QSE_NULL) 
		{
			qse_awk_clrpt (awk, init);
			return QSE_NULL;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			qse_awk_clrpt (awk, init);
			qse_awk_clrpt (awk, test);

			SETERR_TOK (awk, QSE_AWK_ESCOLON);
			return QSE_NULL;
		}
	}

	do
	{
		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, init);
			qse_awk_clrpt (awk, test);
			return QSE_NULL;
		}

		/* skip new lines after the second semicolon */
	}
	while (MATCH(awk,TOKEN_NEWLINE));
	
	if (MATCH(awk,TOKEN_RPAREN)) incr = QSE_NULL;
	else 
	{
		incr = parse_expression (awk, awk->token.lin);
		if (incr == QSE_NULL) 
		{
			qse_awk_clrpt (awk, init);
			qse_awk_clrpt (awk, test);
			return QSE_NULL;
		}

		if (!MATCH(awk,TOKEN_RPAREN)) 
		{
			qse_awk_clrpt (awk, init);
			qse_awk_clrpt (awk, test);
			qse_awk_clrpt (awk, incr);

			SETERR_TOK (awk, QSE_AWK_ERPAREN);
			return QSE_NULL;
		}
	}

	if (get_token(awk) <= -1) 
	{
		qse_awk_clrpt (awk, init);
		qse_awk_clrpt (awk, test);
		qse_awk_clrpt (awk, incr);
		return QSE_NULL;
	}

	body = parse_statement (awk, awk->token.lin);
	if (body == QSE_NULL) 
	{
		qse_awk_clrpt (awk, init);
		qse_awk_clrpt (awk, test);
		qse_awk_clrpt (awk, incr);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_for_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_for_t));
	if (nde == QSE_NULL) 
	{
		qse_awk_clrpt (awk, init);
		qse_awk_clrpt (awk, test);
		qse_awk_clrpt (awk, incr);
		qse_awk_clrpt (awk, body);

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_FOR;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->init = init;
	nde->test = test;
	nde->incr = incr;
	nde->body = body;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_dowhile (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_t* test, * body;
	qse_awk_nde_while_t* nde;

	QSE_ASSERT (awk->ptoken.type == TOKEN_DO);

	body = parse_statement (awk, awk->token.lin);
	if (body == QSE_NULL) return QSE_NULL;

	while (MATCH(awk,TOKEN_NEWLINE))
	{
		if (get_token(awk) <= -1) 
		{
			qse_awk_clrpt (awk, body);
			return QSE_NULL;
		}
	}

	if (!MATCH(awk,TOKEN_WHILE)) 
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

	if (!MATCH(awk,TOKEN_LPAREN)) 
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

	test = parse_expression (awk, awk->token.lin);
	if (test == QSE_NULL) 
	{
		qse_awk_clrpt (awk, body);
		return QSE_NULL;
	}

	if (!MATCH(awk,TOKEN_RPAREN)) 
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

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_DOWHILE;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->test = test;
	nde->body = body;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_break (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_break_t* nde;

	QSE_ASSERT (awk->ptoken.type == TOKEN_BREAK);
	if (awk->parse.depth.cur.loop <= 0) 
	{
		SETERR_LIN (awk, QSE_AWK_EBREAK, line);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_break_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_break_t));
	if (nde == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_BREAK;
	nde->line = line;
	nde->next = QSE_NULL;
	
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_continue (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_continue_t* nde;

	QSE_ASSERT (awk->ptoken.type == TOKEN_CONTINUE);
	if (awk->parse.depth.cur.loop <= 0) 
	{
		SETERR_LIN (awk, QSE_AWK_ECONTINUE, line);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_continue_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_continue_t));
	if (nde == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_CONTINUE;
	nde->line = line;
	nde->next = QSE_NULL;
	
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_return (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_return_t* nde;
	qse_awk_nde_t* val;

	QSE_ASSERT (awk->ptoken.type == TOKEN_RETURN);

	nde = (qse_awk_nde_return_t*) QSE_AWK_ALLOC (
		awk, QSE_SIZEOF(qse_awk_nde_return_t));
	if (nde == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_RETURN;
	nde->line = line;
	nde->next = QSE_NULL;

	if (MATCH_TERMINATOR(awk))
	{
		/* no return value */
		val = QSE_NULL;
	}
	else 
	{
		val = parse_expression (awk, awk->token.lin);
		if (val == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;
		}
	}

	nde->val = val;
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_exit (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_exit_t* nde;
	qse_awk_nde_t* val;

	QSE_ASSERT (awk->ptoken.type == TOKEN_EXIT);

	nde = (qse_awk_nde_exit_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_exit_t));
	if (nde == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_EXIT;
	nde->line = line;
	nde->next = QSE_NULL;

	if (MATCH_TERMINATOR(awk)) 
	{
		/* no exit code */
		val = QSE_NULL;
	}
	else 
	{
		val = parse_expression (awk, awk->token.lin);
		if (val == QSE_NULL) 
		{
			QSE_AWK_FREE (awk, nde);
			return QSE_NULL;
		}
	}

	nde->val = val;
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_next (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_next_t* nde;

	QSE_ASSERT (awk->ptoken.type == TOKEN_NEXT);

	if (awk->parse.id.block == PARSE_BEGIN_BLOCK)
	{
		SETERR_LIN (awk, QSE_AWK_ENEXTBEG, line);
		return QSE_NULL;
	}
	if (awk->parse.id.block == PARSE_END_BLOCK)
	{
		SETERR_LIN (awk, QSE_AWK_ENEXTEND, line);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_next_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_next_t));
	if (nde == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}
	nde->type = QSE_AWK_NDE_NEXT;
	nde->line = line;
	nde->next = QSE_NULL;
	
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_nextfile (qse_awk_t* awk, qse_size_t line, int out)
{
	qse_awk_nde_nextfile_t* nde;

	if (!out && awk->parse.id.block == PARSE_BEGIN_BLOCK)
	{
		SETERR_LIN (awk, QSE_AWK_ENEXTFBEG, line);
		return QSE_NULL;
	}
	if (!out && awk->parse.id.block == PARSE_END_BLOCK)
	{
		SETERR_LIN (awk, QSE_AWK_ENEXTFEND, line);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_nextfile_t*) 
		QSE_AWK_ALLOC (awk, QSE_SIZEOF(qse_awk_nde_nextfile_t));
	if (nde == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_NEXTFILE;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->out = out;
	
	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_delete (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_delete_t* nde;
	qse_awk_nde_t* var;

	QSE_ASSERT (awk->ptoken.type == TOKEN_DELETE);
	if (!MATCH(awk,TOKEN_IDENT)) 
	{
		SETERR_TOK (awk, QSE_AWK_EIDENT);
		return QSE_NULL;
	}

	var = parse_primary_ident (awk, awk->token.lin);
	if (var == QSE_NULL) return QSE_NULL;

	if (!is_var (var))
	{
		/* a normal identifier is expected */
		qse_awk_clrpt (awk, var);
		SETERR_LIN (awk, QSE_AWK_EDELETE, line);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_delete_t*) QSE_AWK_ALLOC (
		awk, QSE_SIZEOF(qse_awk_nde_delete_t));
	if (nde == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_DELETE;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->var = var;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_reset (qse_awk_t* awk, qse_size_t line)
{
	qse_awk_nde_reset_t* nde;
	qse_awk_nde_t* var;

	QSE_ASSERT (awk->ptoken.type == TOKEN_RESET);
	if (!MATCH(awk,TOKEN_IDENT)) 
	{
		SETERR_TOK (awk, QSE_AWK_EIDENT);
		return QSE_NULL;
	}

	var = parse_primary_ident (awk, awk->token.lin);
	if (var == QSE_NULL) return QSE_NULL;

	/* unlike delete, it must be followed by a plain variable only */
	if (!is_plain_var (var))
	{
		/* a normal identifier is expected */
		qse_awk_clrpt (awk, var);
		SETERR_LIN (awk, QSE_AWK_ERESET, line);
		return QSE_NULL;
	}

	nde = (qse_awk_nde_reset_t*) QSE_AWK_ALLOC (
		awk, QSE_SIZEOF(qse_awk_nde_reset_t));
	if (nde == QSE_NULL)
	{
		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	nde->type = QSE_AWK_NDE_RESET;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->var = var;

	return (qse_awk_nde_t*)nde;
}

static qse_awk_nde_t* parse_print (qse_awk_t* awk, qse_size_t line, int type)
{
	qse_awk_nde_print_t* nde;
	qse_awk_nde_t* args = QSE_NULL; 
	qse_awk_nde_t* out = QSE_NULL;
	int out_type;

	if (!MATCH_TERMINATOR(awk) &&
	    !MATCH(awk,TOKEN_GT) &&
	    !MATCH(awk,TOKEN_RS) &&
	    !MATCH(awk,TOKEN_BOR) &&
	    !MATCH(awk,TOKEN_LOR)) 
	{
		qse_awk_nde_t* args_tail;
		qse_awk_nde_t* tail_prev;

		args = parse_expression (awk, awk->token.lin);
		if (args == QSE_NULL) return QSE_NULL;

		args_tail = args;
		tail_prev = QSE_NULL;

		if (args->type != QSE_AWK_NDE_GRP)
		{
			/* args->type == QSE_AWK_NDE_GRP when print (a, b, c) 
			 * args->type != QSE_AWK_NDE_GRP when print a, b, c */
			
			while (MATCH(awk,TOKEN_COMMA))
			{
				do {
					if (get_token(awk) <= -1)
					{
						qse_awk_clrpt (awk, args);
						return QSE_NULL;
					}
				}
				while (MATCH(awk,TOKEN_NEWLINE));

				args_tail->next = parse_expression (awk, awk->token.lin);
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
		if (awk->ptoken.type != TOKEN_RPAREN &&
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
		out_type = MATCH(awk,TOKEN_GT)?       QSE_AWK_OUT_FILE:
		           MATCH(awk,TOKEN_RS)?   QSE_AWK_OUT_APFILE:
		           MATCH(awk,TOKEN_BOR)?      QSE_AWK_OUT_PIPE:
		           ((awk->option & QSE_AWK_RWPIPE) &&
			    MATCH(awk,TOKEN_LOR))?    QSE_AWK_OUT_RWPIPE:
		                                      QSE_AWK_OUT_CONSOLE;

		if (out_type != QSE_AWK_OUT_CONSOLE)
		{
			if (get_token(awk) <= -1)
			{
				if (args != QSE_NULL) qse_awk_clrpt (awk, args);
				return QSE_NULL;
			}

			out = parse_expression (awk, awk->token.lin);
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

		SETERR_LIN (awk, QSE_AWK_ENOMEM, line);
		return QSE_NULL;
	}

	QSE_ASSERTX (
		type == QSE_AWK_NDE_PRINT || type == QSE_AWK_NDE_PRINTF, 
		"the node type should be either QSE_AWK_NDE_PRINT or QSE_AWK_NDE_PRINTF");

	if (type == QSE_AWK_NDE_PRINTF && args == QSE_NULL)
	{
		if (out != QSE_NULL) qse_awk_clrpt (awk, out);
		SETERR_LIN (awk, QSE_AWK_EPRINTFARG, line);
		return QSE_NULL;
	}

	nde->type = type;
	nde->line = line;
	nde->next = QSE_NULL;
	nde->args = args;
	nde->out_type = out_type;
	nde->out = out;

	return (qse_awk_nde_t*)nde;
}

static int get_number (qse_awk_t* awk, qse_awk_token_t* token)
{
	qse_cint_t c;

	QSE_ASSERT (QSE_STR_LEN(token->name) == 0);
	SET_TOKEN_TYPE (awk, token, TOKEN_INT);

	c = awk->sio.last.c;

	if (c == QSE_T('0'))
	{
		ADD_TOKEN_CHAR (awk, token, c);
		GET_CHAR_TO (awk, c);

		if (c == QSE_T('x') || c == QSE_T('X'))
		{
			/* hexadecimal number */
			do 
			{
				ADD_TOKEN_CHAR (awk, token, c);
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
				ADD_TOKEN_CHAR (awk, token, c);
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
				ADD_TOKEN_CHAR (awk, token, c);
				GET_CHAR_TO (awk, c);
			}

			if (c == QSE_T('8') || c == QSE_T('9'))
			{
				qse_char_t cc = (qse_char_t)c;
				SETERR_ARG_LOC (awk, QSE_AWK_ELXDIG, &cc, 1, awk->token.lin);
				return -1;
			}

			return 0;
		}
	}

	while (QSE_AWK_ISDIGIT (awk, c)) 
	{
		ADD_TOKEN_CHAR (awk, token, c);
		GET_CHAR_TO (awk, c);
	} 

	if (c == QSE_T('.'))
	{
		/* floating-point number */
		SET_TOKEN_TYPE (awk, token, TOKEN_REAL);

		ADD_TOKEN_CHAR (awk, token, c);
		GET_CHAR_TO (awk, c);

		while (QSE_AWK_ISDIGIT (awk, c))
		{
			ADD_TOKEN_CHAR (awk, token, c);
			GET_CHAR_TO (awk, c);
		}
	}

	if (c == QSE_T('E') || c == QSE_T('e'))
	{
		SET_TOKEN_TYPE (awk, token, TOKEN_REAL);

		ADD_TOKEN_CHAR (awk, token, c);
		GET_CHAR_TO (awk, c);

		if (c == QSE_T('+') || c == QSE_T('-'))
		{
			ADD_TOKEN_CHAR (awk, token, c);
			GET_CHAR_TO (awk, c);
		}

		while (QSE_AWK_ISDIGIT (awk, c))
		{
			ADD_TOKEN_CHAR (awk, token, c);
			GET_CHAR_TO (awk, c);
		}
	}

	return 0;
}

static int get_string (
	qse_awk_t* awk, qse_char_t end_char, 
	qse_char_t esc_char, qse_bool_t keep_esc_char,
	int preescaped, qse_awk_token_t* token)
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
					ADD_TOKEN_CHAR (awk, token, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				ADD_TOKEN_CHAR (awk, token, c_acc);
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
					ADD_TOKEN_CHAR (awk, token, c_acc);
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
					ADD_TOKEN_CHAR (awk, token, c_acc);
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
					ADD_TOKEN_CHAR (awk, token, c_acc);
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
					ADD_TOKEN_CHAR (awk, token, rc);
				else ADD_TOKEN_CHAR (awk, token, c_acc);

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
				ADD_TOKEN_CHAR (awk, token, esc_char);
			}

			escaped = 0;
		}

		ADD_TOKEN_CHAR (awk, token, c);
	}

	return 0;
}

static int get_charstr (qse_awk_t* awk, qse_awk_token_t* token)
{
	if (awk->sio.last.c != QSE_T('\"')) 
	{
		/* the starting quote has been consumed before this function
		 * has been called */
		ADD_TOKEN_CHAR (awk, token, awk->sio.last.c);
	}
	return get_string (awk, QSE_T('\"'), QSE_T('\\'), QSE_FALSE, 0, token);
}

static int get_rexstr (qse_awk_t* awk, qse_awk_token_t* token)
{
	if (awk->sio.last.c == QSE_T('/')) 
	{
		/* this part of the function is different from get_charstr
		 * because of the way this function is called. 
		 * this condition is met when the input is //.
		 * the first / has been tokenized to TOKEN_DIV already.
		 * if TOKEN_DIV is seen as a primary, this function is called.
		 * as the token buffer has been cleared by the caller and
		 * the token type is set to TOKEN_REX, this function can
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
			ADD_TOKEN_CHAR (awk, token, awk->sio.last.c);
		}
		return get_string (
			awk, QSE_T('/'), QSE_T('\\'), QSE_TRUE, escaped, token);
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
				SETERR_LIN (awk, QSE_AWK_ECMTNC, awk->sio.inp->lin);
				return -1;
			}

			if (c == QSE_T('*')) 
			{
				GET_CHAR_TO (awk, c);
				if (c == QSE_CHAR_EOF)
				{
					SETERR_LIN (awk, QSE_AWK_ECMTNC, awk->sio.inp->lin);
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

static int get_symbols (qse_awk_t* awk, qse_cint_t c, qse_awk_token_t* token)
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
		{ QSE_T("=="),  2, TOKEN_EQ,           0 },
		{ QSE_T("="),   1, TOKEN_ASSIGN,       0 },
		{ QSE_T("!="),  2, TOKEN_NE,           0 },
		{ QSE_T("!~"),  2, TOKEN_NM,           0 },
		{ QSE_T("!"),   1, TOKEN_LNOT,         0 },
		{ QSE_T(">>="), 3, TOKEN_RS_ASSIGN,    QSE_AWK_EXTRAOPS },
		{ QSE_T(">>"),  2, TOKEN_RS,           0 },
		{ QSE_T(">="),  2, TOKEN_GE,           0 },
		{ QSE_T(">"),   1, TOKEN_GT,           0 },
		{ QSE_T("<<="), 3, TOKEN_LS_ASSIGN,    QSE_AWK_EXTRAOPS },
		{ QSE_T("<<"),  2, TOKEN_LS,           0 },
		{ QSE_T("<="),  2, TOKEN_LE,           0 },
		{ QSE_T("<"),   1, TOKEN_LT,           0 },
		{ QSE_T("||"),  2, TOKEN_LOR,          0 },
		{ QSE_T("|="),  2, TOKEN_BOR_ASSIGN,   0 },
		{ QSE_T("|"),   1, TOKEN_BOR,          0 },
		{ QSE_T("&&"),  2, TOKEN_LAND,         0 },
		{ QSE_T("&="),  2, TOKEN_BAND_ASSIGN,  0 },
		{ QSE_T("&"),   1, TOKEN_BAND,         0 },
		{ QSE_T("^^="), 3, TOKEN_BXOR_ASSIGN,  QSE_AWK_EXTRAOPS },
		{ QSE_T("^^"),  2, TOKEN_BXOR,         QSE_AWK_EXTRAOPS },
		{ QSE_T("^="),  2, TOKEN_EXP_ASSIGN,   0 },
		{ QSE_T("^"),   1, TOKEN_EXP,          0 },
		{ QSE_T("++"),  2, TOKEN_PLUSPLUS,     0 },
		{ QSE_T("+="),  2, TOKEN_PLUS_ASSIGN,  0 },
		{ QSE_T("+"),   1, TOKEN_PLUS,         0 },
		{ QSE_T("--"),  2, TOKEN_MINUSMINUS,   0 },
		{ QSE_T("-="),  2, TOKEN_MINUS_ASSIGN, 0 },
		{ QSE_T("-"),   1, TOKEN_MINUS,        0 },
		{ QSE_T("**="), 3, TOKEN_EXP_ASSIGN,   QSE_AWK_EXTRAOPS },
		{ QSE_T("**"),  2, TOKEN_EXP,          QSE_AWK_EXTRAOPS },
		{ QSE_T("*="),  2, TOKEN_MUL_ASSIGN,   0 },
		{ QSE_T("*"),   1, TOKEN_MUL,          0 },
		{ QSE_T("//="), 3, TOKEN_IDIV_ASSIGN,  0 },
		{ QSE_T("//"),  2, TOKEN_IDIV,         QSE_AWK_EXTRAOPS },
		{ QSE_T("/="),  2, TOKEN_DIV_ASSIGN,   QSE_AWK_EXTRAOPS },
		{ QSE_T("/"),   1, TOKEN_DIV,          0 },
		{ QSE_T("%="),  2, TOKEN_MOD_ASSIGN,   0 },
		{ QSE_T("%"),   1, TOKEN_MOD,          0 },
		{ QSE_T("~"),   1, TOKEN_TILDE,        0 },
		{ QSE_T("("),   1, TOKEN_LPAREN,       0 },
		{ QSE_T(")"),   1, TOKEN_RPAREN,       0 },
		{ QSE_T("{"),   1, TOKEN_LBRACE,       0 },
		{ QSE_T("}"),   1, TOKEN_RBRACE,       0 },
		{ QSE_T("["),   1, TOKEN_LBRACK,       0 },
		{ QSE_T("]"),   1, TOKEN_RBRACK,       0 },
		{ QSE_T("$"),   1, TOKEN_DOLLAR,       0 },
		{ QSE_T(","),   1, TOKEN_COMMA,        0 },
		{ QSE_T(";"),   1, TOKEN_SEMICOLON,    0 },
		{ QSE_T(":"),   1, TOKEN_COLON,        0 },
		{ QSE_T("?"),   1, TOKEN_QUEST,        0 },
		{ QSE_T("@"),   1, TOKEN_ATSIGN,       0 },
		{ QSE_NULL,     0, 0,                  0 }
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
				ADD_TOKEN_STR (awk, token, p->str, p->len);
				SET_TOKEN_TYPE (awk, token, p->tid);
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

static int get_token_into (qse_awk_t* awk, qse_awk_token_t* token)
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

	qse_str_clear (token->name);
	token->file = awk->sio.last.file;
	token->lin = awk->sio.last.lin;
	token->col = awk->sio.last.col;

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

		ADD_TOKEN_STR (awk, token, QSE_T("<EOF>"), 5);
		SET_TOKEN_TYPE (awk, token, TOKEN_EOF);
	}	
	else if (c == QSE_T('\n')) 
	{
		/*ADD_TOKEN_CHAR (awk, token, QSE_T('\n'));*/
		ADD_TOKEN_STR (awk, token, QSE_T("<NL>"), 4);
		SET_TOKEN_TYPE (awk, token, TOKEN_NEWLINE);
		GET_CHAR (awk);
	}
	else if (QSE_AWK_ISDIGIT (awk, c)/*|| c == QSE_T('.')*/)
	{
		if (get_number (awk, token) <= -1) return -1;
	}
	else if (c == QSE_T('.'))
	{
		qse_awk_sio_lxc_t lc;

		lc = awk->sio.last;
		GET_CHAR_TO (awk, c);

		if (!(awk->option & QSE_AWK_EXPLICIT) && 
		    QSE_AWK_ISDIGIT (awk, c))
		{
			/* for a token such as .123 */
			unget_char (awk, &awk->sio.last);	
			awk->sio.last = lc;
			if (get_number (awk, token) <= -1) return -1;

		}
		else
		{
			SET_TOKEN_TYPE (awk, token, TOKEN_PERIOD);
			ADD_TOKEN_CHAR (awk, token, QSE_T('.'));
		}
	}
	else if (c == QSE_T('_') || QSE_AWK_ISALPHA (awk, c))
	{
		int type;

		/* identifier */
		do 
		{
			ADD_TOKEN_CHAR (awk, token, c);
			GET_CHAR_TO (awk, c);
		} 
		while (c == QSE_T('_') || 
		       QSE_AWK_ISALPHA (awk, c) || 
		       QSE_AWK_ISDIGIT (awk, c));

		type = classify_ident (awk, 
			QSE_STR_PTR(token->name), 
			QSE_STR_LEN(token->name));
		SET_TOKEN_TYPE (awk, token, type);
	}
	else if (c == QSE_T('\"')) 
	{
		SET_TOKEN_TYPE (awk, token, TOKEN_STR);
		if (get_charstr(awk, token) <= -1) return -1;
	}
	else
	{
		n = get_symbols (awk, c, token);
		if (n <= -1) return -1;
		if (n == 0)
		{
			/* not handled yet */
			if (c == QSE_T('\0'))
				SETERR_ARG_LOC (awk, QSE_AWK_ELXCHR, QSE_T("<NUL>"), 5, token->lin);
			else
			{
				qse_char_t cc = (qse_char_t)c;
				SETERR_ARG_LOC (awk, QSE_AWK_ELXCHR, &cc, 1, token->lin);
			}
			return -1;
		}
	}

	return 0;
}

static int get_token (qse_awk_t* awk)
{
	awk->ptoken.type = awk->token.type;
	awk->ptoken.file = awk->token.file;
	awk->ptoken.lin = awk->token.lin;
	awk->ptoken.col = awk->token.col;
	qse_str_swap (awk->ptoken.name, awk->token.name);

	if (QSE_STR_LEN(awk->ntoken.name) > 0)
	{
		awk->token.type = awk->ntoken.type;
		awk->token.file = awk->ntoken.file;
		awk->token.lin = awk->ntoken.lin;
		awk->token.col = awk->token.col;	

		qse_str_swap (awk->token.name, awk->ntoken.name);
		qse_str_clear (awk->ntoken.name);

		return 0;
	}

	return get_token_into (awk, &awk->token);
}

static int preget_token (qse_awk_t* awk)
{
	return get_token_into (awk, &awk->ntoken);
}

static int classify_ident (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	if (QSE_MAP_SIZE(awk->wtab) <= 0)
	{
		/* perform binary search if no custom words are specified */

		/* declaring left, right, mid to be of int is ok
		 * because we know kwtab is small enough. */
		int left = 0, right = QSE_COUNTOF(kwtab) - 1, mid;

		while (left <= right)
		{
			int n;
			kwent_t* kwp;

			mid = (left + right) / 2;	
			kwp = &kwtab[mid];
			n = qse_strxncmp (kwp->name, kwp->name_len, name, len);
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
	}
	else
	{
		/* perform linear search if there are any custom words set */
		kwent_t* kwp, * end;
		qse_map_pair_t* pair;

		end = kwtab + QSE_COUNTOF(kwtab);
		for (kwp = kwtab; kwp < end; kwp++) 
		{
			const qse_char_t* k;
			qse_size_t l;

			if (kwp->valid != 0 && 
			    (awk->option & kwp->valid) != kwp->valid) continue;

			pair = qse_map_search (awk->wtab, kwp->name, kwp->name_len);
			if (pair != QSE_NULL)
			{
				k = ((qse_cstr_t*)(pair->vptr))->ptr;
				l = ((qse_cstr_t*)(pair->vptr))->len;
			}
			else
			{
				k = kwp->name;
				l = kwp->name_len;
			}

			if (qse_strxncmp (k, l, name, len) == 0) 
			{
				return kwp->type;
			}
		}

	}	

	return TOKEN_IDENT;
}

static int is_plain_var (qse_awk_nde_t* nde)
{
	return nde->type == QSE_AWK_NDE_GBL ||
	       nde->type == QSE_AWK_NDE_LCL ||
	       nde->type == QSE_AWK_NDE_ARG ||
	       nde->type == QSE_AWK_NDE_NAMED;
}

static int is_var (qse_awk_nde_t* nde)
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

		qse_awk_getkw (awk, KW_GLOBAL, &kw);
		if (qse_awk_putsrcstrx(awk,kw.ptr,kw.len) <= -1)
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
				if (qse_awk_putsrcstrx (awk, 
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
				if (qse_awk_putsrcstrx (awk, tmp, len) <= -1)
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
			if (qse_awk_putsrcstrx (awk, 
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
			if (qse_awk_putsrcstrx (awk, tmp, len) <= -1)
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

	qse_map_walk (awk->tree.funs, deparse_func, &df);
	if (df.ret <= -1)
	{
		EXIT_DEPARSE ();
	}

	for (nde = awk->tree.begin; nde != QSE_NULL; nde = nde->next)
	{
		qse_cstr_t kw;

		qse_awk_getkw (awk, KW_BEGIN, &kw);

		if (qse_awk_putsrcstrx (awk, kw.ptr, kw.len) <= -1) EXIT_DEPARSE ();
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

		qse_awk_getkw (awk, KW_END, &kw);

		if (qse_awk_putsrcstrx (awk, kw.ptr, kw.len) <= -1) EXIT_DEPARSE ();
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

static qse_map_walk_t deparse_func (
	qse_map_t* map, qse_map_pair_t* pair, void* arg)
{
	struct deparse_func_t* df = (struct deparse_func_t*)arg;
/* CHECK: */
	qse_awk_fun_t* fun = (qse_awk_fun_t*)QSE_MAP_VPTR(pair);
	qse_size_t i, n;
	qse_cstr_t kw;

	QSE_ASSERT (qse_strxncmp (QSE_MAP_KPTR(pair), QSE_MAP_KLEN(pair), fun->name.ptr, fun->name.len) == 0);

#define PUT_C(x,c) \
	if (put_char(x->awk,c)==-1) { \
		x->ret = -1; return QSE_MAP_WALK_STOP; \
	}

#define PUT_S(x,str) \
	if (qse_awk_putsrcstr(x->awk,str) <= -1) { \
		x->ret = -1; return QSE_MAP_WALK_STOP; \
	}

#define PUT_SX(x,str,len) \
	if (qse_awk_putsrcstrx (x->awk, str, len) <= -1) { \
		x->ret = -1; return QSE_MAP_WALK_STOP; \
	}

	qse_awk_getkw (df->awk, KW_FUNCTION, &kw);
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

	return QSE_MAP_WALK_FORWARD;

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

int qse_awk_putsrcstrx (
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
