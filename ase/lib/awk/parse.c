/*
 * $Id: parse.c 255 2008-07-18 10:42:24Z baconevi $
 *
 * {License}
 */

#include "awk_i.h"

enum
{
	TOKEN_EOF,
	TOKEN_NEWLINE,

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
	TOKEN_RSHIFT_ASSIGN,
	TOKEN_LSHIFT_ASSIGN,
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
	TOKEN_BORAND,
	TOKEN_TILDE, /* used for unary bitwise-not and regex match */
	TOKEN_RSHIFT,
	TOKEN_LSHIFT,
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
	PARSE_GLOBAL,
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

static int parse (ase_awk_t* awk);

static ase_awk_t* parse_progunit (ase_awk_t* awk);
static ase_awk_t* collect_globals (ase_awk_t* awk);
static void adjust_static_globals (ase_awk_t* awk);

static ase_size_t get_global (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len);
static ase_size_t find_global (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len);
static int add_global (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len, 
	ase_size_t line, int force);
static ase_awk_t* collect_locals (
	ase_awk_t* awk, ase_size_t nlocals, ase_bool_t istop);

static ase_awk_nde_t* parse_function (ase_awk_t* awk);
static ase_awk_nde_t* parse_begin (ase_awk_t* awk);
static ase_awk_nde_t* parse_end (ase_awk_t* awk);
static ase_awk_chain_t* parse_pattern_block (
	ase_awk_t* awk, ase_awk_nde_t* ptn, ase_bool_t blockless);

static ase_awk_nde_t* parse_block (ase_awk_t* awk, ase_size_t line, ase_bool_t istop);
static ase_awk_nde_t* parse_block_dc (ase_awk_t* awk, ase_size_t line, ase_bool_t istop);
static ase_awk_nde_t* parse_statement (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_statement_nb (ase_awk_t* awk, ase_size_t line);

static ase_awk_nde_t* parse_expression (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_expression0 (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_basic_expr (ase_awk_t* awk, ase_size_t line);

static ase_awk_nde_t* parse_binary_expr (
	ase_awk_t* awk, ase_size_t line, int skipnl, const binmap_t* binmap,
	ase_awk_nde_t*(*next_level_func)(ase_awk_t*,ase_size_t));

static ase_awk_nde_t* parse_logical_or (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_logical_and (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_in (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_regex_match (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_bitwise_or (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_bitwise_or_with_extio (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_bitwise_xor (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_bitwise_and (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_equality (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_relational (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_shift (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_concat (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_additive (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_multiplicative (ase_awk_t* awk, ase_size_t line);

static ase_awk_nde_t* parse_unary (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_exponent (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_unary_exp (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_increment (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_primary (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_primary_ident (ase_awk_t* awk, ase_size_t line);

static ase_awk_nde_t* parse_hashidx (
	ase_awk_t* awk, ase_char_t* name, ase_size_t name_len, 
	ase_size_t line);
static ase_awk_nde_t* parse_fncall (
	ase_awk_t* awk, ase_char_t* name, ase_size_t name_len, 
	ase_awk_bfn_t* bfn, ase_size_t line);
static ase_awk_nde_t* parse_if (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_while (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_for (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_dowhile (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_break (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_continue (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_return (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_exit (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_next (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_nextfile (ase_awk_t* awk, ase_size_t line, int out);
static ase_awk_nde_t* parse_delete (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_reset (ase_awk_t* awk, ase_size_t line);
static ase_awk_nde_t* parse_print (ase_awk_t* awk, ase_size_t line, int type);

static int get_token (ase_awk_t* awk);
static int get_number (ase_awk_t* awk);
static int get_charstr (ase_awk_t* awk);
static int get_rexstr (ase_awk_t* awk);
static int get_string (
	ase_awk_t* awk, ase_char_t end_char,
	ase_char_t esc_char, ase_bool_t keep_esc_char);
static int get_symbol (ase_awk_t* awk, ase_cint_t fc);
static int get_char (ase_awk_t* awk);
static int unget_char (ase_awk_t* awk, ase_cint_t c);
static int skip_spaces (ase_awk_t* awk);
static int skip_comment (ase_awk_t* awk);
static int classify_ident (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len);
static int assign_to_opcode (ase_awk_t* awk);
static int is_plain_var (ase_awk_nde_t* nde);
static int is_var (ase_awk_nde_t* nde);

static int deparse (ase_awk_t* awk);
static int deparse_func (ase_pair_t* pair, void* arg);
static int put_char (ase_awk_t* awk, ase_char_t c);
static int flush_out (ase_awk_t* awk);

typedef struct kwent_t kwent_t;

struct kwent_t 
{ 
	const ase_char_t* name; 
	ase_size_t name_len;
	int type; 
	int valid; /* the entry is valid when this option is set */
};

static kwent_t kwtab[] = 
{
	/* operators */
	{ ASE_T("in"),           2, TOKEN_IN,          0 },

	/* top-level block starters */
	{ ASE_T("BEGIN"),        5, TOKEN_BEGIN,       ASE_AWK_PABLOCK },
	{ ASE_T("END"),          3, TOKEN_END,         ASE_AWK_PABLOCK },
	{ ASE_T("function"),     8, TOKEN_FUNCTION,    0 },

	/* keywords for variable declaration */
	{ ASE_T("local"),        5, TOKEN_LOCAL,       ASE_AWK_EXPLICIT },
	{ ASE_T("global"),       6, TOKEN_GLOBAL,      ASE_AWK_EXPLICIT },

	/* keywords that start statements excluding expression statements */
	{ ASE_T("if"),           2, TOKEN_IF,          0 },
	{ ASE_T("else"),         4, TOKEN_ELSE,        0 },
	{ ASE_T("while"),        5, TOKEN_WHILE,       0 },
	{ ASE_T("for"),          3, TOKEN_FOR,         0 },
	{ ASE_T("do"),           2, TOKEN_DO,          0 },
	{ ASE_T("break"),        5, TOKEN_BREAK,       0 },
	{ ASE_T("continue"),     8, TOKEN_CONTINUE,    0 },
	{ ASE_T("return"),       6, TOKEN_RETURN,      0 },
	{ ASE_T("exit"),         4, TOKEN_EXIT,        0 },
	{ ASE_T("next"),         4, TOKEN_NEXT,        ASE_AWK_PABLOCK },
	{ ASE_T("nextfile"),     8, TOKEN_NEXTFILE,    ASE_AWK_PABLOCK },
	{ ASE_T("nextofile"),    9, TOKEN_NEXTOFILE,   ASE_AWK_PABLOCK | ASE_AWK_NEXTOFILE },
	{ ASE_T("delete"),       6, TOKEN_DELETE,      0 },
	{ ASE_T("reset"),        5, TOKEN_RESET,       ASE_AWK_RESET },
	{ ASE_T("print"),        5, TOKEN_PRINT,       ASE_AWK_EXTIO },
	{ ASE_T("printf"),       6, TOKEN_PRINTF,      ASE_AWK_EXTIO },

	/* keywords that can start an expression */
	{ ASE_T("getline"),      7, TOKEN_GETLINE,     ASE_AWK_EXTIO },

	{ ASE_NULL,              0, 0,                 0 }
};

typedef struct global_t global_t;

struct global_t
{
	const ase_char_t* name;
	ase_size_t name_len;
	int valid;
};

static global_t gtab[] =
{
	{ ASE_T("ARGC"),         4,  0 },
	{ ASE_T("ARGV"),         4,  0 },

	/* output real-to-str conversion format for other cases than 'print' */
	{ ASE_T("CONVFMT"),      7,  0 },

	/* current input file name */
	{ ASE_T("FILENAME"),     8,  ASE_AWK_PABLOCK },

	/* input record number in current file */
	{ ASE_T("FNR"),          3,  ASE_AWK_PABLOCK },

	/* input field separator */
	{ ASE_T("FS"),           2,  0 },

	/* ignore case in string comparison */
	{ ASE_T("IGNORECASE"),  10,  0 },

	/* number of fields in current input record 
	 * NF is also updated if you assign a value to $0. so it is not
	 * associated with ASE_AWK_PABLOCK */
	{ ASE_T("NF"),           2,  0 },

	/* input record number */
	{ ASE_T("NR"),           2,  ASE_AWK_PABLOCK },

	/* current output file name */
	{ ASE_T("OFILENAME"),    9,  ASE_AWK_PABLOCK | ASE_AWK_NEXTOFILE },

	/* output real-to-str conversion format for 'print' */
	{ ASE_T("OFMT"),         4,  ASE_AWK_EXTIO}, 

	/* output field separator for 'print' */
	{ ASE_T("OFS"),          3,  ASE_AWK_EXTIO },

	/* output record separator. used for 'print' and blockless output */
	{ ASE_T("ORS"),          3,  ASE_AWK_EXTIO },

	{ ASE_T("RLENGTH"),      7,  0 },
	{ ASE_T("RS"),           2,  0 },
	{ ASE_T("RSTART"),       6,  0 },
	{ ASE_T("SUBSEP"),       6,  0 }
};

#define GET_CHAR(awk) \
	do { if (get_char(awk) == -1) return -1; } while(0)

#define GET_CHAR_TO(awk,c) \
	do { \
		if (get_char(awk) == -1) return -1; \
		c = (awk)->src.lex.curc; \
	} while(0)

#define UNGET_CHAR(awk,c) \
	do { if (unget_char (awk, c) == -1) return -1; } while(0)

#define SET_TOKEN_TYPE(awk,code) do { (awk)->token.type = (code); } while (0)

#define ADD_TOKEN_CHAR(awk,c) \
	do { \
		if (ase_str_ccat(&(awk)->token.name,(c)) == (ase_size_t)-1) \
		{ \
			ase_awk_seterror (awk, ASE_AWK_ENOMEM, (awk)->token.line, ASE_NULL, 0); \
			return -1; \
		} \
	} while (0)

#define ADD_TOKEN_STR(awk,s,l) \
	do { \
		if (ase_str_ncat(&(awk)->token.name,(s),(l)) == (ase_size_t)-1) \
		{ \
			ase_awk_seterror (awk, ASE_AWK_ENOMEM, (awk)->token.line, ASE_NULL, 0); \
			return -1; \
		} \
	} while (0)

#define MATCH(awk,token_type) ((awk)->token.type == (token_type))

#define CLRERR(awk) ase_awk_seterrnum(awk,ASE_AWK_ENOERR)
#define ISNOERR(awk) ((awk)->errnum == ASE_AWK_ENOERR)
#define SETERR(awk,code) ase_awk_seterrnum(awk,code)
#define SETERRLIN(awk,code,line) ase_awk_seterror(awk,code,line,ASE_NULL,0)
#define SETERRTOK(awk,code) \
	do { \
		ase_cstr_t errarg; \
		errarg.len = ASE_STR_LEN(&(awk)->token.name); \
		errarg.ptr = ASE_STR_BUF(&(awk)->token.name); \
		if (MATCH(awk,TOKEN_EOF)) \
			ase_awk_seterror (awk, code, (awk)->token.prev.line, &errarg, 1); \
		else \
			ase_awk_seterror (awk, code, (awk)->token.line, &errarg, 1); \
	} while (0)

#define SETERRARG(awk,code,line,arg,leng) \
	do { \
		ase_cstr_t errarg; \
		errarg.len = (leng); \
		errarg.ptr = (arg); \
		ase_awk_seterror ((awk), (code), (line), &errarg, 1); \
	} while (0)

#define MATCH_TERMINATOR(awk) \
	(MATCH((awk),TOKEN_SEMICOLON) || \
	 MATCH((awk),TOKEN_NEWLINE) || \
	 ((awk->option & ASE_AWK_NEWLINE) && MATCH((awk),TOKEN_RBRACE)))

ase_size_t ase_awk_getmaxdepth (ase_awk_t* awk, int type)
{
	return (type == ASE_AWK_DEPTH_BLOCK_PARSE)? awk->parse.depth.max.block:
	       (type == ASE_AWK_DEPTH_BLOCK_RUN)? awk->run.depth.max.block:
	       (type == ASE_AWK_DEPTH_EXPR_PARSE)? awk->parse.depth.max.expr:
	       (type == ASE_AWK_DEPTH_EXPR_RUN)? awk->run.depth.max.expr:
	       (type == ASE_AWK_DEPTH_REX_BUILD)? awk->rex.depth.max.build:
	       (type == ASE_AWK_DEPTH_REX_MATCH)? awk->rex.depth.max.match: 0;
}

void ase_awk_setmaxdepth (ase_awk_t* awk, int types, ase_size_t depth)
{
	if (types & ASE_AWK_DEPTH_BLOCK_PARSE)
	{
		awk->parse.depth.max.block = depth;
		if (depth <= 0)
			awk->parse.parse_block = parse_block;
		else
			awk->parse.parse_block = parse_block_dc;
	}

	if (types & ASE_AWK_DEPTH_EXPR_PARSE)
	{
		awk->parse.depth.max.expr = depth;
	}

	if (types & ASE_AWK_DEPTH_BLOCK_RUN)
	{
		awk->run.depth.max.block = depth;
	}

	if (types & ASE_AWK_DEPTH_EXPR_RUN)
	{
		awk->run.depth.max.expr = depth;
	}

	if (types & ASE_AWK_DEPTH_REX_BUILD)
	{
		awk->rex.depth.max.build = depth;
	}

	if (types & ASE_AWK_DEPTH_REX_MATCH)
	{
		awk->rex.depth.max.match = depth;
	}
}

const ase_char_t* ase_awk_getglobalname (
	ase_awk_t* awk, ase_size_t idx, ase_size_t* len)
{
	/*
	*len = gtab[idx].name_len;
	return gtab[idx].name;
	*/

	ASE_ASSERT (idx < ase_awk_tab_getsize(&awk->parse.globals));

	*len = awk->parse.globals.buf[idx].name.len;
	return awk->parse.globals.buf[idx].name.ptr;
}

const ase_char_t* ase_awk_getkw (ase_awk_t* awk, const ase_char_t*  kw)
{
	ase_pair_t* p;

	ASE_ASSERT (kw != ASE_NULL);

	p = ase_map_get (awk->wtab, kw, ase_strlen(kw));
	if (p != ASE_NULL) return ((ase_cstr_t*)p->val)->ptr;

	return kw;
}

int ase_awk_parse (ase_awk_t* awk, ase_awk_srcios_t* srcios)
{
	int n;

	ASE_ASSERTX (
		srcios != ASE_NULL && srcios->in != ASE_NULL,
		"the source code input stream must be provided at least");
	ASE_ASSERT (awk->parse.depth.cur.loop == 0);
	ASE_ASSERT (awk->parse.depth.cur.expr == 0);

	ase_awk_clear (awk);
	ase_memcpy (&awk->src.ios, srcios, ASE_SIZEOF(awk->src.ios));

	n = parse (awk);

	ASE_ASSERT (awk->parse.depth.cur.loop == 0);
	ASE_ASSERT (awk->parse.depth.cur.expr == 0);

	return n;
}

static int parse (ase_awk_t* awk)
{
	int n = 0; 
	ase_ssize_t op;

	ASE_ASSERT (awk->src.ios.in != ASE_NULL);

	CLRERR (awk);
	op = awk->src.ios.in (
		ASE_AWK_IO_OPEN, awk->src.ios.custom_data, ASE_NULL, 0);
	if (op <= -1)
	{
		/* cannot open the source file.
		 * it doesn't even have to call CLOSE */
		if (ISNOERR(awk)) SETERR (awk, ASE_AWK_ESINOP);
		return -1;
	}

	adjust_static_globals (awk);

#define EXIT_PARSE(v) do { n = (v); goto exit_parse; } while(0)

	/* the user io handler for the source code input returns 0 when
	 * it doesn't have any files to open. this is the same condition
	 * as the source code file is empty. so it will perform the parsing
	 * when op is positive, which means there are something to parse */
	if (op > 0)
	{
		/* get the first character */
		if (get_char(awk) == -1) EXIT_PARSE(-1);
		/* get the first token */
		if (get_token(awk) == -1) EXIT_PARSE(-1);

		while (1) 
		{
			while (MATCH(awk,TOKEN_NEWLINE)) 
			{
				if (get_token(awk) == -1) EXIT_PARSE(-1);
			}
			if (MATCH(awk,TOKEN_EOF)) break;

			if (parse_progunit(awk) == ASE_NULL) EXIT_PARSE(-1);
		}

		if ((awk->option & ASE_AWK_EXPLICIT) &&
		    !(awk->option & ASE_AWK_IMPLICIT))
		{
			ase_pair_t* p;
			ase_size_t buckno;

			p = ase_map_getfirstpair (awk->parse.afns, &buckno);
			while (p != ASE_NULL)
			{
				if (ase_map_get (awk->tree.afns, 
					p->key.ptr, p->key.len) == ASE_NULL)
				{
					/* TODO: set better error no & line */
					/* this line number might be truncated as 
			 		 * sizeof(line) could be > sizeof(void*) */
					SETERRARG (awk, ASE_AWK_EFNNONE, 
						(ase_size_t)p->val, 
						p->key.ptr, p->key.len);
					EXIT_PARSE(-1);
				}

				p = ase_map_getnextpair (awk->parse.afns, p, &buckno);
			}

		}
	}

	ASE_ASSERT (awk->tree.nglobals == awk->parse.globals.size);

	if (awk->src.ios.out != ASE_NULL) 
	{
		if (deparse (awk) == -1) EXIT_PARSE(-1);
	}

#undef EXIT_PARSE
exit_parse:
	if (n == 0) CLRERR (awk);
	if (awk->src.ios.in (
		ASE_AWK_IO_CLOSE, awk->src.ios.custom_data, ASE_NULL, 0) != 0)
	{
		if (n == 0)
		{
			/* this is to keep the earlier error above
			 * that might be more critical than this */
			if (ISNOERR(awk)) SETERR (awk, ASE_AWK_ESINCL);
			n = -1;
		}
	}

	if (n == -1) ase_awk_clear (awk);
	else awk->tree.ok = 1;

	return n;
}

static ase_awk_t* parse_progunit (ase_awk_t* awk)
{
	/*
	global xxx, xxxx;
	BEGIN { action }
	END { action }
	pattern { action }
	function name (parameter-list) { statement }
	*/

	ASE_ASSERT (awk->parse.depth.cur.loop == 0);

	if ((awk->option & ASE_AWK_EXPLICIT) && MATCH(awk,TOKEN_GLOBAL)) 
	{
		ase_size_t nglobals;

		awk->parse.id.block = PARSE_GLOBAL;

		if (get_token(awk) == -1) return ASE_NULL;

		ASE_ASSERT (awk->tree.nglobals == ase_awk_tab_getsize(&awk->parse.globals));
		nglobals = awk->tree.nglobals;
		if (collect_globals (awk) == ASE_NULL) 
		{
			ase_awk_tab_remove (
				&awk->parse.globals, nglobals, 
				ase_awk_tab_getsize(&awk->parse.globals) - nglobals);
			awk->tree.nglobals = nglobals;
			return ASE_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_FUNCTION)) 
	{
		awk->parse.id.block = PARSE_FUNCTION;
		if (parse_function (awk) == ASE_NULL) return ASE_NULL;
	}
	else if (MATCH(awk,TOKEN_BEGIN)) 
	{
		if ((awk->option & ASE_AWK_PABLOCK) == 0)
		{
			SETERRTOK (awk, ASE_AWK_EFUNC);
			return ASE_NULL;
		}

		/*
		if (awk->tree.begin != ASE_NULL)
		{
			SETERRLIN (awk, ASE_AWK_EDUPBEG, awk->token.prev.line);
			return ASE_NULL;
		}
		*/

		awk->parse.id.block = PARSE_BEGIN;
		if (get_token(awk) == -1) return ASE_NULL; 

		if (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF))
		{
			/* when ASE_AWK_NEWLINE is set,
	   		 * BEGIN and { should be located on the same line */
			SETERRLIN (awk, ASE_AWK_EBLKBEG, awk->token.prev.line);
			return ASE_NULL;
		}

		if (!MATCH(awk,TOKEN_LBRACE)) 
		{
			SETERRTOK (awk, ASE_AWK_ELBRACE);
			return ASE_NULL;
		}

		awk->parse.id.block = PARSE_BEGIN_BLOCK;
		if (parse_begin (awk) == ASE_NULL) return ASE_NULL;
	}
	else if (MATCH(awk,TOKEN_END)) 
	{
		if ((awk->option & ASE_AWK_PABLOCK) == 0)
		{
			SETERRTOK (awk, ASE_AWK_EFUNC);
			return ASE_NULL;
		}

		/*
		if (awk->tree.end != ASE_NULL)
		{
			SETERRLIN (awk, ASE_AWK_EDUPEND, awk->token.prev.line);
			return ASE_NULL;
		}
		*/

		awk->parse.id.block = PARSE_END;
		if (get_token(awk) == -1) return ASE_NULL; 

		if (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF))
		{
			/* when ASE_AWK_NEWLINE is set,
	   		 * END and { should be located on the same line */
			SETERRLIN (awk, ASE_AWK_EBLKEND, awk->token.prev.line);
			return ASE_NULL;
		}

		if (!MATCH(awk,TOKEN_LBRACE)) 
		{
			SETERRTOK (awk, ASE_AWK_ELBRACE);
			return ASE_NULL;
		}

		awk->parse.id.block = PARSE_END_BLOCK;
		if (parse_end (awk) == ASE_NULL) return ASE_NULL;
	}
	else if (MATCH(awk,TOKEN_LBRACE))
	{
		/* patternless block */
		if ((awk->option & ASE_AWK_PABLOCK) == 0)
		{
			SETERRTOK (awk, ASE_AWK_EFUNC);
			return ASE_NULL;
		}

		awk->parse.id.block = PARSE_ACTION_BLOCK;
		if (parse_pattern_block (
			awk, ASE_NULL, ASE_FALSE) == ASE_NULL) return ASE_NULL;
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
		ase_awk_nde_t* ptn;

		if ((awk->option & ASE_AWK_PABLOCK) == 0)
		{
			SETERRTOK (awk, ASE_AWK_EFUNC);
			return ASE_NULL;
		}

		awk->parse.id.block = PARSE_PATTERN;

		ptn = parse_expression (awk, awk->token.line);
		if (ptn == ASE_NULL) return ASE_NULL;

		ASE_ASSERT (ptn->next == ASE_NULL);

		if (MATCH(awk,TOKEN_COMMA))
		{
			if (get_token (awk) == -1) 
			{
				ase_awk_clrpt (awk, ptn);
				return ASE_NULL;
			}	

			ptn->next = parse_expression (awk, awk->token.line);
			if (ptn->next == ASE_NULL) 
			{
				ase_awk_clrpt (awk, ptn);
				return ASE_NULL;
			}
		}

		if (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF))
		{
			/* blockless pattern */
			ase_bool_t newline = MATCH(awk,TOKEN_NEWLINE);
			ase_size_t tline = awk->token.prev.line;

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (parse_pattern_block(awk,ptn,ASE_TRUE) == ASE_NULL) 
			{
				ase_awk_clrpt (awk, ptn);
				return ASE_NULL;	
			}

			if (newline)
			{
				if (get_token(awk) == -1) 
				{
					/* ptn has been added to the chain. 
					 * it doesn't have to be cleared here
					 * as ase_awk_clear does it */
					/*ase_awk_clrpt (awk, ptn);*/
					return ASE_NULL;
				}	
			}

			if ((awk->option & ASE_AWK_EXTIO) != ASE_AWK_EXTIO)
			{
				/* blockless pattern requires ASE_AWK_EXTIO
				 * to be ON because the implicit block is
				 * "print $0" */
				SETERRLIN (awk, ASE_AWK_ENOSUP, tline);
				return ASE_NULL;
			}
		}
		else
		{
			/* parse the action block */
			if (!MATCH(awk,TOKEN_LBRACE))
			{
				ase_awk_clrpt (awk, ptn);
				SETERRTOK (awk, ASE_AWK_ELBRACE);
				return ASE_NULL;
			}

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (parse_pattern_block (
				awk, ptn, ASE_FALSE) == ASE_NULL) 
			{
				ase_awk_clrpt (awk, ptn);
				return ASE_NULL;	
			}
		}
	}

	return awk;
}

static ase_awk_nde_t* parse_function (ase_awk_t* awk)
{
	ase_char_t* name;
	ase_char_t* name_dup;
	ase_size_t name_len;
	ase_awk_nde_t* body;
	ase_awk_afn_t* afn;
	ase_size_t nargs, g;
	ase_pair_t* pair;
	int n;

	/* eat up the keyword 'function' and get the next token */
	ASE_ASSERT (MATCH(awk,TOKEN_FUNCTION));
	if (get_token(awk) == -1) return ASE_NULL;  

	/* match a function name */
	if (!MATCH(awk,TOKEN_IDENT)) 
	{
		/* cannot find a valid identifier for a function name */
		SETERRTOK (awk, ASE_AWK_EFNNAME);
		return ASE_NULL;
	}

	name = ASE_STR_BUF(&awk->token.name);
	name_len = ASE_STR_LEN(&awk->token.name);

	/* check if it is a builtin function */
	if (ase_awk_getbfn (awk, name, name_len) != ASE_NULL)
	{
		SETERRARG (awk, ASE_AWK_EBFNRED, awk->token.line, name, name_len);
		return ASE_NULL;
	}

	if (ase_map_get(awk->tree.afns, name, name_len) != ASE_NULL) 
	{
		/* the function is defined previously */
		SETERRARG (
			awk, ASE_AWK_EAFNRED, awk->token.line, 
			name, name_len);
		return ASE_NULL;
	}

	#if 0
	if (awk->option & ASE_AWK_UNIQUEFN) 
	{
	#endif
		/* check if it coincides to be a global variable name */
		g = find_global (awk, name, name_len);
		if (g != (ase_size_t)-1) 
		{
			SETERRARG (
				awk, ASE_AWK_EGBLRED, awk->token.line, 
				name, name_len);
			return ASE_NULL;
		}
	#if 0
	}
	#endif

	/* clone the function name before it is overwritten */
	name_dup = ase_awk_strxdup (awk, name, name_len);
	if (name_dup == ASE_NULL) 
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, awk->token.line);
		return ASE_NULL;
	}

	/* get the next token */
	if (get_token(awk) == -1) 
	{
		ASE_AWK_FREE (awk, name_dup);
		return ASE_NULL;  
	}

	/* match a left parenthesis */
	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		/* a function name is not followed by a left parenthesis */
		ASE_AWK_FREE (awk, name_dup);

		SETERRTOK (awk, ASE_AWK_ELPAREN);
		return ASE_NULL;
	}	

	/* get the next token */
	if (get_token(awk) == -1) 
	{
		ASE_AWK_FREE (awk, name_dup);
		return ASE_NULL;
	}

	/* make sure that parameter table is empty */
	ASE_ASSERT (ase_awk_tab_getsize(&awk->parse.params) == 0);

	/* read parameter list */
	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no function parameter found. get the next token */
		if (get_token(awk) == -1) 
		{
			ASE_AWK_FREE (awk, name_dup);
			return ASE_NULL;
		}
	}
	else 
	{
		while (1) 
		{
			ase_char_t* param;
			ase_size_t param_len;

			if (!MATCH(awk,TOKEN_IDENT)) 
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);
				SETERRTOK (awk, ASE_AWK_EBADPAR);
				return ASE_NULL;
			}

			param = ASE_STR_BUF(&awk->token.name);
			param_len = ASE_STR_LEN(&awk->token.name);

			#if 0
			if (awk->option & ASE_AWK_UNIQUEFN) 
			{
				/* check if a parameter conflicts with a function 
				 *  function f (f) { print f; } */
				if (ase_strxncmp (name_dup, name_len, param, param_len) == 0 ||
				    ase_map_get (awk->tree.afns, param, param_len) != ASE_NULL) 
				{
					ASE_AWK_FREE (awk, name_dup);
					ase_awk_tab_clear (&awk->parse.params);
					
					SETERRARG (
						awk, ASE_AWK_EAFNRED, awk->token.line,
						param, param_len);

					return ASE_NULL;
				}

				/* NOTE: the following is not a conflict
				 *  global x; 
				 *  function f (x) { print x; } 
				 *  x in print x is a parameter
				 */
			}
			#endif

			/* check if a parameter conflicts with other parameters */
			if (ase_awk_tab_find (
				&awk->parse.params, 
				0, param, param_len) != (ase_size_t)-1) 
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);

				SETERRARG (
					awk, ASE_AWK_EDUPPAR, awk->token.line,
					param, param_len);

				return ASE_NULL;
			}

			/* push the parameter to the parameter list */
			if (ase_awk_tab_getsize (
				&awk->parse.params) >= ASE_AWK_MAX_PARAMS)
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);

				SETERRTOK (awk, ASE_AWK_EPARTM);
				return ASE_NULL;
			}

			if (ase_awk_tab_add (
				&awk->parse.params, 
				param, param_len) == (ase_size_t)-1) 
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);

				SETERRLIN (awk, ASE_AWK_ENOMEM, awk->token.line);
				return ASE_NULL;
			}	

			if (get_token (awk) == -1) 
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);
				return ASE_NULL;
			}	

			if (MATCH(awk,TOKEN_RPAREN)) break;

			if (!MATCH(awk,TOKEN_COMMA)) 
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);

				SETERRTOK (awk, ASE_AWK_ECOMMA);
				return ASE_NULL;
			}

			if (get_token(awk) == -1) 
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);
				return ASE_NULL;
			}
		}

		if (get_token(awk) == -1) 
		{
			ASE_AWK_FREE (awk, name_dup);
			ase_awk_tab_clear (&awk->parse.params);
			return ASE_NULL;
		}
	}

	/* function body can be placed on a different line 
	 * from a function name and the parameters even if
	 * ASE_AWK_NEWLINE is set. note TOKEN_NEWLINE is
	 * available only when the option is set. */
	while (MATCH(awk,TOKEN_NEWLINE))
	{
		if (get_token(awk) == -1) 
		{
			ASE_AWK_FREE (awk, name_dup);
			ase_awk_tab_clear (&awk->parse.params);
			return ASE_NULL;
		}
	}

	/* check if the function body starts with a left brace */
	if (!MATCH(awk,TOKEN_LBRACE)) 
	{
		ASE_AWK_FREE (awk, name_dup);
		ase_awk_tab_clear (&awk->parse.params);

		SETERRTOK (awk, ASE_AWK_ELBRACE);
		return ASE_NULL;
	}
	if (get_token(awk) == -1) 
	{
		ASE_AWK_FREE (awk, name_dup);
		ase_awk_tab_clear (&awk->parse.params);
		return ASE_NULL; 
	}

	/* remember the current function name so that the body parser
	 * can know the name of the current function being parsed */
	awk->tree.cur_afn.ptr = name_dup;
	awk->tree.cur_afn.len = name_len;

	/* actual function body */
	body = awk->parse.parse_block (awk, awk->token.prev.line, ASE_TRUE);

	/* clear the current function name remembered */
	awk->tree.cur_afn.ptr = ASE_NULL;
	awk->tree.cur_afn.len = 0;

	if (body == ASE_NULL) 
	{
		ASE_AWK_FREE (awk, name_dup);
		ase_awk_tab_clear (&awk->parse.params);
		return ASE_NULL;
	}

	/* TODO: study furthur if the parameter names should be saved 
	 *       for some reasons - might be needed for deparsing output */
	nargs = ase_awk_tab_getsize (&awk->parse.params);
	/* parameter names are not required anymore. clear them */
	ase_awk_tab_clear (&awk->parse.params);

	afn = (ase_awk_afn_t*) ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_afn_t));
	if (afn == ASE_NULL) 
	{
		ASE_AWK_FREE (awk, name_dup);
		ase_awk_clrpt (awk, body);

		SETERRLIN (awk, ASE_AWK_ENOMEM, awk->token.line);
		return ASE_NULL;
	}

	afn->name = ASE_NULL; /* function name set below */
	afn->name_len = 0;
	afn->nargs = nargs;
	afn->body = body;

	n = ase_map_putx (awk->tree.afns, name_dup, name_len, afn, &pair);
	if (n < 0)
	{
		ASE_AWK_FREE (awk, name_dup);
		ase_awk_clrpt (awk, body);
		ASE_AWK_FREE (awk, afn);

		SETERRLIN (awk, ASE_AWK_ENOMEM, awk->token.line);
		return ASE_NULL;
	}

	/* duplicate functions should have been detected previously */
	ASE_ASSERT (n != 0); 

	afn->name = ASE_PAIR_KEYPTR(pair); /* do some trick to save a string. */
	afn->name_len = ASE_PAIR_KEYLEN(pair);
	ASE_AWK_FREE (awk, name_dup);

	/* remove the undefined function call entries from parse.afn table */
	ase_map_remove (awk->parse.afns, afn->name, name_len);
	return body;
}

static ase_awk_nde_t* parse_begin (ase_awk_t* awk)
{
	ase_awk_nde_t* nde;

	ASE_ASSERT (MATCH(awk,TOKEN_LBRACE));

	if (get_token(awk) == -1) return ASE_NULL; 
	nde = awk->parse.parse_block (awk, awk->token.prev.line, ASE_TRUE);
	if (nde == ASE_NULL) return ASE_NULL;

	if (awk->tree.begin == ASE_NULL)
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

static ase_awk_nde_t* parse_end (ase_awk_t* awk)
{
	ase_awk_nde_t* nde;

	ASE_ASSERT (MATCH(awk,TOKEN_LBRACE));

	if (get_token(awk) == -1) return ASE_NULL; 
	nde = awk->parse.parse_block (awk, awk->token.prev.line, ASE_TRUE);
	if (nde == ASE_NULL) return ASE_NULL;

	if (awk->tree.end == ASE_NULL)
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

static ase_awk_chain_t* parse_pattern_block (
	ase_awk_t* awk, ase_awk_nde_t* ptn, ase_bool_t blockless)
{
	ase_awk_nde_t* nde;
	ase_awk_chain_t* chain;
	ase_size_t line = awk->token.line;

	if (blockless) nde = ASE_NULL;
	else
	{
		ASE_ASSERT (MATCH(awk,TOKEN_LBRACE));
		if (get_token(awk) == -1) return ASE_NULL; 
		nde = awk->parse.parse_block (awk, line, ASE_TRUE);
		if (nde == ASE_NULL) return ASE_NULL;
	}

	chain = (ase_awk_chain_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_chain_t));
	if (chain == ASE_NULL) 
	{
		ase_awk_clrpt (awk, nde);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	chain->pattern = ptn;
	chain->action = nde;
	chain->next = ASE_NULL;

	if (awk->tree.chain == ASE_NULL) 
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

static ase_awk_nde_t* parse_block (
	ase_awk_t* awk, ase_size_t line, ase_bool_t istop) 
{
	ase_awk_nde_t* head, * curr, * nde;
	ase_awk_nde_blk_t* block;
	ase_size_t nlocals, nlocals_max, tmp;

	nlocals = ase_awk_tab_getsize(&awk->parse.locals);
	nlocals_max = awk->parse.nlocals_max;

	/* local variable declarations */
	if (awk->option & ASE_AWK_EXPLICIT) 
	{
		while (1) 
		{
			if (!MATCH(awk,TOKEN_LOCAL)) break;

			if (get_token(awk) == -1) 
			{
				ase_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					ase_awk_tab_getsize(&awk->parse.locals) - nlocals);
				return ASE_NULL;
			}

			if (collect_locals (awk, nlocals, istop) == ASE_NULL)
			{
				ase_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					ase_awk_tab_getsize(&awk->parse.locals) - nlocals);
				return ASE_NULL;
			}
		}
	}

	/* block body */
	head = ASE_NULL; curr = ASE_NULL;

	while (1) 
	{
		/* skip new lines within a block */
		while (MATCH(awk,TOKEN_NEWLINE))
		{
			if (get_token(awk) == -1) return ASE_NULL;
		}

		/* if EOF is met before the right brace, this is an error */
		if (MATCH(awk,TOKEN_EOF)) 
		{
			ase_awk_tab_remove (
				&awk->parse.locals, nlocals, 
				ase_awk_tab_getsize(&awk->parse.locals) - nlocals);
			if (head != ASE_NULL) ase_awk_clrpt (awk, head);

			SETERRLIN (awk, ASE_AWK_EENDSRC, awk->token.prev.line);
			return ASE_NULL;
		}

		/* end the block when the right brace is met */
		if (MATCH(awk,TOKEN_RBRACE)) 
		{
			if (get_token(awk) == -1) 
			{
				ase_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					ase_awk_tab_getsize(&awk->parse.locals)-nlocals);
				if (head != ASE_NULL) ase_awk_clrpt (awk, head);
				return ASE_NULL; 
			}

			break;
		}

		/* parse an actual statement in a block */
		nde = parse_statement (awk, awk->token.line);
		if (nde == ASE_NULL) 
		{
			ase_awk_tab_remove (
				&awk->parse.locals, nlocals, 
				ase_awk_tab_getsize(&awk->parse.locals)-nlocals);
			if (head != ASE_NULL) ase_awk_clrpt (awk, head);
			return ASE_NULL;
		}

		/* remove unnecessary statements such as adjacent 
		 * null statements */
		if (nde->type == ASE_AWK_NDE_NULL) 
		{
			ase_awk_clrpt (awk, nde);
			continue;
		}
		if (nde->type == ASE_AWK_NDE_BLK && 
		    ((ase_awk_nde_blk_t*)nde)->body == ASE_NULL) 
		{
			ase_awk_clrpt (awk, nde);
			continue;
		}
			
		if (curr == ASE_NULL) head = nde;
		else curr->next = nde;	
		curr = nde;
	}

	block = (ase_awk_nde_blk_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_blk_t));
	if (block == ASE_NULL) 
	{
		ase_awk_tab_remove (
			&awk->parse.locals, nlocals, 
			ase_awk_tab_getsize(&awk->parse.locals)-nlocals);
		ase_awk_clrpt (awk, head);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	tmp = ase_awk_tab_getsize(&awk->parse.locals);
	if (tmp > awk->parse.nlocals_max) awk->parse.nlocals_max = tmp;

	/* remove all locals to move it up to the top level */
	ase_awk_tab_remove (&awk->parse.locals, nlocals, tmp - nlocals);

	/* adjust the number of locals for a block without any statements */
	/* if (head == ASE_NULL) tmp = 0; */

	block->type = ASE_AWK_NDE_BLK;
	block->line = line;
	block->next = ASE_NULL;
	block->body = head;

	/* TODO: not only local variables but also nested blocks, 
	unless it is part of other constructs such as if, can be promoted 
	and merged to top-level block */

	/* migrate all block-local variables to a top-level block */
	if (istop) 
	{
		block->nlocals = awk->parse.nlocals_max - nlocals;
		awk->parse.nlocals_max = nlocals_max;
	}
	else 
	{
		/*block->nlocals = tmp - nlocals;*/
		block->nlocals = 0;
	}

	return (ase_awk_nde_t*)block;
}

static ase_awk_nde_t* parse_block_dc (
	ase_awk_t* awk, ase_size_t line, ase_bool_t istop) 
{
	ase_awk_nde_t* nde;
		
	ASE_ASSERT (awk->parse.depth.max.block > 0);

	if (awk->parse.depth.cur.block >= awk->parse.depth.max.block)
	{
		SETERRLIN (awk, ASE_AWK_EBLKNST, awk->token.prev.line);
		return ASE_NULL;
	}

	awk->parse.depth.cur.block++;
	nde = parse_block (awk, line, istop);
	awk->parse.depth.cur.block--;

	return nde;
}

int ase_awk_initglobals (ase_awk_t* awk)
{	
	int id;

	/* ase_awk_initglobals is not generic-purpose. call this from
	 * ase_awk_open only. */
	ASE_ASSERT (awk->tree.nbglobals == 0 && awk->tree.nglobals == 0);

	awk->tree.nbglobals = 0;
	awk->tree.nglobals = 0;

	for (id = ASE_AWK_MIN_GLOBAL_ID; id <= ASE_AWK_MAX_GLOBAL_ID; id++)
	{
		ase_size_t g;

		g = ase_awk_tab_add (&awk->parse.globals, 
			gtab[id].name, gtab[id].name_len);
		if (g == (ase_size_t)-1) return -1;

		ASE_ASSERT ((int)g == id);

		awk->tree.nbglobals++;
		awk->tree.nglobals++;
	}

	ASE_ASSERT (awk->tree.nbglobals == 
		ASE_AWK_MAX_GLOBAL_ID-ASE_AWK_MIN_GLOBAL_ID+1);
	return 0;
}

static void adjust_static_globals (ase_awk_t* awk)
{
	int id;

	ASE_ASSERT (awk->tree.nbglobals >=
		ASE_AWK_MAX_GLOBAL_ID - ASE_AWK_MAX_GLOBAL_ID + 1);

	for (id = ASE_AWK_MIN_GLOBAL_ID; id <= ASE_AWK_MAX_GLOBAL_ID; id++)
	{
		if (gtab[id].valid != 0 && 
		    (awk->option & gtab[id].valid) != gtab[id].valid)
		{
			awk->parse.globals.buf[id].name.len = 0;
		}
		else
		{
			awk->parse.globals.buf[id].name.len = gtab[id].name_len;
		}
	}
}

static void trans_global (
	ase_size_t index, ase_cstr_t* word, void* arg)
{
	ase_awk_tab_t* tab = (ase_awk_tab_t*)arg;
	ase_awk_t* awk = tab->awk;

	/*
	if (index >= ASE_AWK_MIN_GLOBAL_ID && 
	    index <= ASE_AWK_MAX_GLOBAL_ID)
	*/
	if (index < awk->tree.nbglobals)
	{
		ase_pair_t* pair;

		pair = ase_map_get (awk->wtab, word->ptr, word->len);
		if (pair != ASE_NULL)
		{
			word->ptr = ((ase_cstr_t*)(pair->val))->ptr;
			word->len = ((ase_cstr_t*)(pair->val))->len;
		}
	}
}

static ase_size_t get_global (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len)
{
	return ase_awk_tab_rrfindx (
		&awk->parse.globals, 0, name, len, 
		trans_global, &awk->parse.globals);
}

static ase_size_t find_global (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len)
{
	return ase_awk_tab_findx (
		&awk->parse.globals, 0, name, len,
		trans_global, &awk->parse.globals);
}

static int add_global (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len, 
	ase_size_t line, int disabled)
{
	ase_size_t nglobals;

	#if 0
	if (awk->option & ASE_AWK_UNIQUEFN) 
	{
	#endif
		/* check if it conflict with a builtin function name */
		if (ase_awk_getbfn (awk, name, len) != ASE_NULL)
		{
			SETERRARG (
				awk, ASE_AWK_EBFNRED, awk->token.line,
				name, len);
			return -1;
		}

		/* check if it conflict with a function name */
		if (ase_map_get (awk->tree.afns, name, len) != ASE_NULL) 
		{
			SETERRARG (
				awk, ASE_AWK_EAFNRED, line, 
				name, len);
			return -1;
		}

		/* check if it conflict with a function name 
		 * caught in the function call table */
		if (ase_map_get (awk->parse.afns, name, len) != ASE_NULL)
		{
			SETERRARG (
				awk, ASE_AWK_EAFNRED, line, 
				name, len);
			return -1;
		}
	#if 0
	}
	#endif

	/* check if it conflicts with other global variable names */
	if (find_global (awk, name, len) != (ase_size_t)-1)
	{ 
		SETERRARG (awk, ASE_AWK_EDUPGBL, line, name, len);
		return -1;
	}

	nglobals = ase_awk_tab_getsize (&awk->parse.globals);
	if (nglobals >= ASE_AWK_MAX_GLOBALS)
	{
		SETERRLIN (awk, ASE_AWK_EGBLTM, line);
		return -1;
	}

	if (ase_awk_tab_add (&awk->parse.globals, name, len) == (ase_size_t)-1) 
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return -1;
	}

	/* the disabled item is inserted normally but 
	 * the name length is reset to zero. */
	if (disabled) awk->parse.globals.buf[nglobals].name.len = 0;

	awk->tree.nglobals = ase_awk_tab_getsize (&awk->parse.globals);
	ASE_ASSERT (nglobals == awk->tree.nglobals-1);

	/* return the id which is the index to the global table. */
	return (int)nglobals;
}

int ase_awk_addglobal (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len)
{
	int n;

	if (len <= 0)
	{
		SETERR (awk, ASE_AWK_EINVAL);
		return -1;
	}

	if (awk->tree.nglobals > awk->tree.nbglobals) 
	{
		/* this function is not allow after ase_awk_parse is called */
		SETERR (awk, ASE_AWK_ENOPER);
		return -1;
	}
	n = add_global (awk, name, len, 0, 0);

	/* update the count of the static globals. 
	 * the total global count has been updated inside add_global. */
	if (n >= 0) awk->tree.nbglobals++; 

	return n;
}

int ase_awk_delglobal (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len)
{
	ase_size_t n;
	
#define ASE_AWK_NUM_STATIC_GLOBALS \
	(ASE_AWK_MAX_GLOBAL_ID-ASE_AWK_MIN_GLOBAL_ID+1)

	if (awk->tree.nglobals > awk->tree.nbglobals) 
	{
		/* this function is not allow after ase_awk_parse is called */
		SETERR (awk, ASE_AWK_ENOPER);
		return -1;
	}

	n = ase_awk_tab_find (&awk->parse.globals, 
		ASE_AWK_NUM_STATIC_GLOBALS, name, len);
	if (n == (ase_size_t)-1) 
	{
		SETERRARG (awk, ASE_AWK_ENOENT, 0, name, len);
		return -1;
	}

	/* invalidate the name if deletion is requested.
	 * this approach does not delete the entry.
	 * if ase_awk_addglobal is called with the same name
	 * again, the entry will be appended again. 
	 * never call this funciton unless it is really required. */
	awk->parse.globals.buf[n].name.ptr[0] = ASE_T('\0');;
	awk->parse.globals.buf[n].name.len = 0;

	return 0;
}

static ase_awk_t* collect_globals (ase_awk_t* awk)
{
	while (1) 
	{
		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			SETERRTOK (awk, ASE_AWK_EBADVAR);
			return ASE_NULL;
		}

		if (add_global (
			awk,
			ASE_STR_BUF(&awk->token.name),
			ASE_STR_LEN(&awk->token.name),
			awk->token.line, 0) == -1) return ASE_NULL;

		if (get_token(awk) == -1) return ASE_NULL;

		if (MATCH_TERMINATOR(awk)) break;

		if (!MATCH(awk,TOKEN_COMMA)) 
		{
			SETERRTOK (awk, ASE_AWK_ECOMMA);
			return ASE_NULL;
		}

		if (get_token(awk) == -1) return ASE_NULL;
	}

	/* skip a semicolon */
	if (get_token(awk) == -1) return ASE_NULL;

	return awk;
}

static ase_awk_t* collect_locals (
	ase_awk_t* awk, ase_size_t nlocals, ase_bool_t istop)
{
	ase_char_t* local;
	ase_size_t local_len;
	ase_size_t n;

	while (1) 
	{
		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			SETERRTOK (awk, ASE_AWK_EBADVAR);
			return ASE_NULL;
		}

		local = ASE_STR_BUF(&awk->token.name);
		local_len = ASE_STR_LEN(&awk->token.name);

		#if 0
		if (awk->option & ASE_AWK_UNIQUEFN) 
		{
			ase_bool_t iscur = ASE_FALSE;
		#endif

			/* check if it conflict with a builtin function name 
			 * function f() { local length; } */
			if (ase_awk_getbfn (awk, local, local_len) != ASE_NULL)
			{
				SETERRARG (
					awk, ASE_AWK_EBFNRED, awk->token.line,
					local, local_len);
				return ASE_NULL;
			}

		#if 0
			/* check if it conflict with a function name */
			if (awk->tree.cur_afn.ptr != ASE_NULL)
			{
				iscur = (ase_strxncmp (
					awk->tree.cur_afn.ptr, awk->tree.cur_afn.len, 
					local, local_len) == 0);
			}

			if (iscur || ase_map_get (awk->tree.afns, local, local_len) != ASE_NULL) 
			{
				SETERRARG (
					awk, ASE_AWK_EAFNRED, awk->token.line,
					local, local_len);
				return ASE_NULL;
			}

			/* check if it conflict with a function name 
			 * caught in the function call table */
			if (ase_map_get (awk->parse.afns, local, local_len) != ASE_NULL)
			{
				SETERRARG (
					awk, ASE_AWK_EAFNRED, awk->token.line, 
					local, local_len);
				return ASE_NULL;
			}
		}
		#endif

		if (istop)
		{
			/* check if it conflicts with a paremeter name */
			n = ase_awk_tab_find (&awk->parse.params, 0, local, local_len);
			if (n != (ase_size_t)-1)
			{
				SETERRARG (
					awk, ASE_AWK_EPARRED, awk->token.line,
					local, local_len);
				return ASE_NULL;
			}
		}

		/* check if it conflicts with other local variable names */
		n = ase_awk_tab_find (
			&awk->parse.locals, 
			nlocals, /*((awk->option&ASE_AWK_SHADING)? nlocals:0)*/
			local, local_len);
		if (n != (ase_size_t)-1)
		{
			SETERRARG (
				awk, ASE_AWK_EDUPLCL, awk->token.line, 
				local, local_len);
			return ASE_NULL;
		}

		/* check if it conflicts with global variable names */
		n = find_global (awk, local, local_len);
		if (n != (ase_size_t)-1)
		{
 			if (n < awk->tree.nbglobals)
			{
				/* conflicting with a static global variable */
				SETERRARG (
					awk, ASE_AWK_EDUPLCL, awk->token.line, 
					local, local_len);
				return ASE_NULL;
			}

			#if 0
			if (!(awk->option & ASE_AWK_SHADING))
			{
				/* conflicting with a normal global variable */
				SETERRARG (
					awk, ASE_AWK_EDUPLCL, awk->token.line, 
					local, local_len);
				return ASE_NULL;
			}
			#endif
		}

		if (ase_awk_tab_getsize(&awk->parse.locals) >= ASE_AWK_MAX_LOCALS)
		{
			SETERRLIN (awk, ASE_AWK_ELCLTM, awk->token.line);
			return ASE_NULL;
		}

		if (ase_awk_tab_add (
			&awk->parse.locals, local, local_len) == (ase_size_t)-1) 
		{
			SETERRLIN (awk, ASE_AWK_ENOMEM, awk->token.line);
			return ASE_NULL;
		}

		if (get_token(awk) == -1) return ASE_NULL;

		if (MATCH_TERMINATOR(awk)) break;

		if (!MATCH(awk,TOKEN_COMMA))
		{
			SETERRTOK (awk, ASE_AWK_ECOMMA);
			return ASE_NULL;
		}

		if (get_token(awk) == -1) return ASE_NULL;
	}

	/* skip a semicolon */
	if (get_token(awk) == -1) return ASE_NULL;

	return awk;
}

static ase_awk_nde_t* parse_statement (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* nde;

	/* skip new lines before a statement */
	while (MATCH(awk,TOKEN_NEWLINE))
	{
		if (get_token(awk) == -1) return ASE_NULL;
	}

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* null statement */	
		nde = (ase_awk_nde_t*) 
			ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_t));
		if (nde == ASE_NULL) 
		{
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_NULL;
		nde->line = line;
		nde->next = ASE_NULL;

		if (get_token(awk) == -1) 
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_LBRACE)) 
	{
		/* a block statemnt { ... } */
		if (get_token(awk) == -1) return ASE_NULL; 
		nde = awk->parse.parse_block (
			awk, awk->token.prev.line, ASE_FALSE);
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
static ase_awk_nde_t* parse_statement_nb (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* nde;

	/* keywords that don't require any terminating semicolon */
	if (MATCH(awk,TOKEN_IF)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;
		return parse_if (awk, line);
	}
	else if (MATCH(awk,TOKEN_WHILE)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;
		
		awk->parse.depth.cur.loop++;
		nde = parse_while (awk, line);
		awk->parse.depth.cur.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_FOR)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;

		awk->parse.depth.cur.loop++;
		nde = parse_for (awk, line);
		awk->parse.depth.cur.loop--;

		return nde;
	}

	/* keywords that require a terminating semicolon */
	if (MATCH(awk,TOKEN_DO)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;

		awk->parse.depth.cur.loop++;
		nde = parse_dowhile (awk, line);
		awk->parse.depth.cur.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_BREAK)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_break (awk, line);
	}
	else if (MATCH(awk,TOKEN_CONTINUE)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_continue (awk, line);
	}
	else if (MATCH(awk,TOKEN_RETURN)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_return (awk, line);
	}
	else if (MATCH(awk,TOKEN_EXIT)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_exit (awk, line);
	}
	else if (MATCH(awk,TOKEN_NEXT)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_next (awk, line);
	}
	else if (MATCH(awk,TOKEN_NEXTFILE)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_nextfile (awk, line, 0);
	}
	else if (MATCH(awk,TOKEN_NEXTOFILE))
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_nextfile (awk, line, 1);
	}
	else if (MATCH(awk,TOKEN_DELETE)) 
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_delete (awk, line);
	}
	else if (MATCH(awk,TOKEN_RESET))
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_reset (awk, line);
	}
	else if (MATCH(awk,TOKEN_PRINT))
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_print (awk, line, ASE_AWK_NDE_PRINT);
	}
	else if (MATCH(awk,TOKEN_PRINTF))
	{
		if (get_token(awk) == -1) return ASE_NULL;
		nde = parse_print (awk, line, ASE_AWK_NDE_PRINTF);
	}
	else 
	{
		nde = parse_expression (awk, line);
	}

	if (nde == ASE_NULL) return ASE_NULL;

	/* check if a statement ends with a semicolon */
	if (MATCH_TERMINATOR(awk)) 
	{
		/* eat up the semicolon or a new line and read in the next token
		 * when ASE_AWK_NEWLINE is set, a statement may end with }.
		 * it should not be eaten up here. */
		if (!MATCH(awk,TOKEN_RBRACE) && get_token(awk) == -1) 
		{
			if (nde != ASE_NULL) ase_awk_clrpt (awk, nde);
			return ASE_NULL;
		}
	}
	else
	{
		if (nde != ASE_NULL) ase_awk_clrpt (awk, nde);
		SETERRLIN (awk, ASE_AWK_ESTMEND, awk->token.prev.line);
		return ASE_NULL;
	}

	return nde;
}

static ase_awk_nde_t* parse_expression (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* nde;

	if (awk->parse.depth.max.expr > 0 &&
	    awk->parse.depth.cur.expr >= awk->parse.depth.max.expr)
	{
		SETERRLIN (awk, ASE_AWK_EEXPRNST, line);
		return ASE_NULL;
	}

	awk->parse.depth.cur.expr++;
	nde = parse_expression0 (awk, line);
	awk->parse.depth.cur.expr--;

	return nde;
}

static ase_awk_nde_t* parse_expression0 (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* x, * y;
	ase_awk_nde_ass_t* nde;
	int opcode;

	x = parse_basic_expr (awk, line);
	if (x == ASE_NULL) return ASE_NULL;

	opcode = assign_to_opcode (awk);
	if (opcode == -1) 
	{
		/* no assignment operator found. */
		return x;
	}

	ASE_ASSERT (x->next == ASE_NULL);
	if (!is_var(x) && x->type != ASE_AWK_NDE_POS) 
	{
		ase_awk_clrpt (awk, x);
		SETERRLIN (awk, ASE_AWK_EASSIGN, line);
		return ASE_NULL;
	}

	if (get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, x);
		return ASE_NULL;
	}

	/*y = parse_basic_expr (awk);*/
	y = parse_expression (awk, awk->token.line);
	if (y == ASE_NULL) 
	{
		ase_awk_clrpt (awk, x);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_ass_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_ass_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, x);
		ase_awk_clrpt (awk, y);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_ASS;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->opcode = opcode;
	nde->left = x;
	nde->right = y;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_basic_expr (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* nde, * n1, * n2;
	
	nde = parse_logical_or (awk, line);
	if (nde == ASE_NULL) return ASE_NULL;

	if (MATCH(awk,TOKEN_QUEST))
	{ 
		ase_awk_nde_cnd_t* tmp;

		if (get_token(awk) == -1) return ASE_NULL;

		/*n1 = parse_basic_expr (awk, awk->token.line);*/
		n1 = parse_expression (awk, awk->token.line);
		if (n1 == ASE_NULL) 
		{
			ase_awk_clrpt (awk, nde);
			return ASE_NULL;
		}

		if (!MATCH(awk,TOKEN_COLON)) 
		{
			SETERRTOK (awk, ASE_AWK_ECOLON);
			return ASE_NULL;
		}
		if (get_token(awk) == -1) return ASE_NULL;

		/*n2 = parse_basic_expr (awk, awk->token.line);*/
		n2 = parse_expression (awk, awk->token.line);
		if (n2 == ASE_NULL)
		{
			ase_awk_clrpt (awk, nde);
			ase_awk_clrpt (awk, n1);
			return ASE_NULL;
		}

		tmp = (ase_awk_nde_cnd_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_cnd_t));
		if (tmp == ASE_NULL)
		{
			ase_awk_clrpt (awk, nde);
			ase_awk_clrpt (awk, n1);
			ase_awk_clrpt (awk, n2);

			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		tmp->type = ASE_AWK_NDE_CND;
		tmp->line = line;
		tmp->next = ASE_NULL;
		tmp->test = nde;
		tmp->left = n1;
		tmp->right = n2;

		nde = (ase_awk_nde_t*)tmp;
	}

	return nde;
}

static ase_awk_nde_t* parse_binary_expr (
	ase_awk_t* awk, ase_size_t line, int skipnl, const binmap_t* binmap,
	ase_awk_nde_t*(*next_level_func)(ase_awk_t*,ase_size_t))
{
	ase_awk_nde_exp_t* nde;
	ase_awk_nde_t* left, * right;
	int opcode;

	left = next_level_func (awk, line);
	if (left == ASE_NULL) return ASE_NULL;
	
	while (1) 
	{
		const binmap_t* p = binmap;
		ase_bool_t matched = ASE_FALSE;

		while (p->token != TOKEN_EOF)
		{
			if (MATCH(awk,p->token)) 
			{
				opcode = p->binop;
				matched = ASE_TRUE;
				break;
			}
			p++;
		}
		if (!matched) break;

		do 
		{
			if (get_token(awk) == -1) 
			{
				ase_awk_clrpt (awk, left);
				return ASE_NULL; 
			}
		}
		while (skipnl && MATCH(awk,TOKEN_NEWLINE));

		right = next_level_func (awk, awk->token.line);
		if (right == ASE_NULL) 
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL;
		}

		nde = (ase_awk_nde_exp_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_exp_t));
		if (nde == ASE_NULL) 
		{
			ase_awk_clrpt (awk, right);
			ase_awk_clrpt (awk, left);

			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_EXP_BIN;
		nde->line = line;
		nde->next = ASE_NULL;
		nde->opcode = opcode; 
		nde->left = left;
		nde->right = right;

		left = (ase_awk_nde_t*)nde;
	}

	return left;
}

static ase_awk_nde_t* parse_logical_or (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_LOR, ASE_AWK_BINOP_LOR },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 1, map, parse_logical_and);
}

static ase_awk_nde_t* parse_logical_and (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_LAND, ASE_AWK_BINOP_LAND },
		{ TOKEN_EOF,  0 }
	};

	return parse_binary_expr (awk, line, 1, map, parse_in);
}

static ase_awk_nde_t* parse_in (ase_awk_t* awk, ase_size_t line)
{
	/* 
	static binmap_t map[] =
	{
		{ TOKEN_IN, ASE_AWK_BINOP_IN },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_regex_match);
	*/

	ase_awk_nde_exp_t* nde;
	ase_awk_nde_t* left, * right;
	ase_size_t line2;

	left = parse_regex_match (awk, line);
	if (left == ASE_NULL) return ASE_NULL;

	while (1)
	{
		if (!MATCH(awk,TOKEN_IN)) break;

		if (get_token(awk) == -1) 
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL; 
		}

		line2 = awk->token.line;

		right = parse_regex_match (awk, line2);
		if (right == ASE_NULL) 
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL;
		}

		if (!is_plain_var(right))
		{
			ase_awk_clrpt (awk, right);
			ase_awk_clrpt (awk, left);

			SETERRLIN (awk, ASE_AWK_ENOTVAR, line2);
			return ASE_NULL;
		}

		nde = (ase_awk_nde_exp_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_exp_t));
		if (nde == ASE_NULL) 
		{
			ase_awk_clrpt (awk, right);
			ase_awk_clrpt (awk, left);

			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_EXP_BIN;
		nde->line = line;
		nde->next = ASE_NULL;
		nde->opcode = ASE_AWK_BINOP_IN; 
		nde->left = left;
		nde->right = right;

		left = (ase_awk_nde_t*)nde;
	}

	return left;
}

static ase_awk_nde_t* parse_regex_match (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] =
	{
		{ TOKEN_TILDE, ASE_AWK_BINOP_MA },
		{ TOKEN_NM,    ASE_AWK_BINOP_NM },
		{ TOKEN_EOF,   0 },
	};

	return parse_binary_expr (awk, line, 0, map, parse_bitwise_or);
}

static ase_awk_nde_t* parse_bitwise_or (ase_awk_t* awk, ase_size_t line)
{
	if (awk->option & ASE_AWK_EXTIO)
	{
		return parse_bitwise_or_with_extio (awk, line);
	}
	else
	{
		static binmap_t map[] = 
		{
			{ TOKEN_BOR, ASE_AWK_BINOP_BOR },
			{ TOKEN_EOF, 0 }
		};

		return parse_binary_expr (
			awk, line, 0, map, parse_bitwise_xor);
	}
}

static ase_awk_nde_t* parse_bitwise_or_with_extio (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* left, * right;

	left = parse_bitwise_xor (awk, line);
	if (left == ASE_NULL) return ASE_NULL;

	while (1)
	{
		int in_type;

		if (MATCH(awk,TOKEN_BOR)) 
			in_type = ASE_AWK_IN_PIPE;
		else if (MATCH(awk,TOKEN_BORAND)) 
			in_type = ASE_AWK_IN_COPROC;
		else break;
		
		if (get_token(awk) == -1)
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL;
		}

		if (MATCH(awk,TOKEN_GETLINE))
		{
			ase_awk_nde_getline_t* nde;
			ase_awk_nde_t* var = ASE_NULL;

			/* piped getline */
			if (get_token(awk) == -1)
			{
				ase_awk_clrpt (awk, left);
				return ASE_NULL;
			}

			/* TODO: is this correct? */

			if (MATCH(awk,TOKEN_IDENT))
			{
				/* command | getline var */

				var = parse_primary (awk, awk->token.line);
				if (var == ASE_NULL) 
				{
					ase_awk_clrpt (awk, left);
					return ASE_NULL;
				}
			}

			nde = (ase_awk_nde_getline_t*) ASE_AWK_MALLOC (
				awk, ASE_SIZEOF(ase_awk_nde_getline_t));
			if (nde == ASE_NULL)
			{
				ase_awk_clrpt (awk, left);

				SETERRLIN (awk, ASE_AWK_ENOMEM, line);
				return ASE_NULL;
			}

			nde->type = ASE_AWK_NDE_GETLINE;
			nde->line = line;
			nde->next = ASE_NULL;
			nde->var = var;
			nde->in_type = in_type;
			nde->in = left;

			left = (ase_awk_nde_t*)nde;
		}
		else
		{
			ase_awk_nde_exp_t* nde;

			if (in_type == ASE_AWK_IN_COPROC)
			{
				ase_awk_clrpt (awk, left);

				/* TODO: support coprocess */
				SETERRLIN (awk, ASE_AWK_EGLNCPS, line);
				return ASE_NULL;
			}

			right = parse_bitwise_xor (awk, awk->token.line);
			if (right == ASE_NULL)
			{
				ase_awk_clrpt (awk, left);
				return ASE_NULL;
			}

			nde = (ase_awk_nde_exp_t*) ASE_AWK_MALLOC (
				awk, ASE_SIZEOF(ase_awk_nde_exp_t));
			if (nde == ASE_NULL)
			{
				ase_awk_clrpt (awk, right);
				ase_awk_clrpt (awk, left);

				SETERRLIN (awk, ASE_AWK_ENOMEM, line);
				return ASE_NULL;
			}

			nde->type = ASE_AWK_NDE_EXP_BIN;
			nde->line = line;
			nde->next = ASE_NULL;
			nde->opcode = ASE_AWK_BINOP_BOR;
			nde->left = left;
			nde->right = right;

			left = (ase_awk_nde_t*)nde;
		}
	}

	return left;
}

static ase_awk_nde_t* parse_bitwise_xor (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_BXOR, ASE_AWK_BINOP_BXOR },
		{ TOKEN_EOF,  0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_bitwise_and);
}

static ase_awk_nde_t* parse_bitwise_and (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_BAND, ASE_AWK_BINOP_BAND },
		{ TOKEN_EOF,  0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_equality);
}

static ase_awk_nde_t* parse_equality (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_EQ, ASE_AWK_BINOP_EQ },
		{ TOKEN_NE, ASE_AWK_BINOP_NE },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_relational);
}

static ase_awk_nde_t* parse_relational (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_GT, ASE_AWK_BINOP_GT },
		{ TOKEN_GE, ASE_AWK_BINOP_GE },
		{ TOKEN_LT, ASE_AWK_BINOP_LT },
		{ TOKEN_LE, ASE_AWK_BINOP_LE },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, 
		((awk->option & ASE_AWK_SHIFT)? parse_shift: parse_concat));
}

static ase_awk_nde_t* parse_shift (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_LSHIFT, ASE_AWK_BINOP_LSHIFT },
		{ TOKEN_RSHIFT, ASE_AWK_BINOP_RSHIFT },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_concat);
}

static ase_awk_nde_t* parse_concat (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_exp_t* nde;
	ase_awk_nde_t* left, * right;

	left = parse_additive (awk, line);
	if (left == ASE_NULL) return ASE_NULL;

	while (1)
	{
		if (MATCH(awk,TOKEN_PERIOD))
		{
			if (!(awk->option & ASE_AWK_EXPLICIT)) break;
			if (get_token(awk) == -1) return ASE_NULL;
		}
		else if (MATCH(awk,TOKEN_LPAREN) ||
	                 MATCH(awk,TOKEN_DOLLAR) ||
			 MATCH(awk,TOKEN_PLUS) ||
			 MATCH(awk,TOKEN_MINUS) ||
			 MATCH(awk,TOKEN_PLUSPLUS) ||
			 MATCH(awk,TOKEN_MINUSMINUS) ||
			 awk->token.type >= TOKEN_GETLINE)
		{
			/* TODO: is the check above sufficient? */
			if (!(awk->option & ASE_AWK_IMPLICIT)) break;
		}
		else break;

		right = parse_additive (awk, awk->token.line);
		if (right == ASE_NULL) 
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL;
		}

		nde = (ase_awk_nde_exp_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_exp_t));
		if (nde == ASE_NULL)
		{
			ase_awk_clrpt (awk, left);
			ase_awk_clrpt (awk, right);

			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_EXP_BIN;
		nde->line = line;
		nde->next = ASE_NULL;
		nde->opcode = ASE_AWK_BINOP_CONCAT;
		nde->left = left;
		nde->right = right;
		
		left = (ase_awk_nde_t*)nde;
	}

	return left;
}

static ase_awk_nde_t* parse_additive (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_PLUS, ASE_AWK_BINOP_PLUS },
		{ TOKEN_MINUS, ASE_AWK_BINOP_MINUS },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_multiplicative);
}

static ase_awk_nde_t* parse_multiplicative (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_MUL,  ASE_AWK_BINOP_MUL },
		{ TOKEN_DIV,  ASE_AWK_BINOP_DIV },
		{ TOKEN_IDIV, ASE_AWK_BINOP_IDIV },
		{ TOKEN_MOD,  ASE_AWK_BINOP_MOD },
		/* { TOKEN_EXP, ASE_AWK_BINOP_EXP }, */
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_unary);
}

static ase_awk_nde_t* parse_unary (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_exp_t* nde; 
	ase_awk_nde_t* left;
	int opcode;

	opcode = (MATCH(awk,TOKEN_PLUS))?  ASE_AWK_UNROP_PLUS:
	         (MATCH(awk,TOKEN_MINUS))? ASE_AWK_UNROP_MINUS:
	         (MATCH(awk,TOKEN_LNOT))?  ASE_AWK_UNROP_LNOT:
	         (MATCH(awk,TOKEN_TILDE))? ASE_AWK_UNROP_BNOT: -1;

	/*if (opcode == -1) return parse_increment (awk);*/
	if (opcode == -1) return parse_exponent (awk, line);

	if (get_token(awk) == -1) return ASE_NULL;

	if (awk->parse.depth.max.expr > 0 &&
	    awk->parse.depth.cur.expr >= awk->parse.depth.max.expr)
	{
		SETERRLIN (awk, ASE_AWK_EEXPRNST, awk->token.line);
		return ASE_NULL;
	}
	awk->parse.depth.cur.expr++;
	left = parse_unary (awk, awk->token.line);
	awk->parse.depth.cur.expr--;
	if (left == ASE_NULL) return ASE_NULL;

	nde = (ase_awk_nde_exp_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_exp_t));
	if (nde == ASE_NULL)
	{
		ase_awk_clrpt (awk, left);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_EXP_UNR;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = ASE_NULL;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_exponent (ase_awk_t* awk, ase_size_t line)
{
	static binmap_t map[] = 
	{
		{ TOKEN_EXP, ASE_AWK_BINOP_EXP },
		{ TOKEN_EOF, 0 }
	};

	return parse_binary_expr (awk, line, 0, map, parse_unary_exp);
}

static ase_awk_nde_t* parse_unary_exp (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_exp_t* nde; 
	ase_awk_nde_t* left;
	int opcode;

	opcode = (MATCH(awk,TOKEN_PLUS))?  ASE_AWK_UNROP_PLUS:
	         (MATCH(awk,TOKEN_MINUS))? ASE_AWK_UNROP_MINUS:
	         (MATCH(awk,TOKEN_LNOT))?  ASE_AWK_UNROP_LNOT:
	         (MATCH(awk,TOKEN_TILDE))? ASE_AWK_UNROP_BNOT: -1;

	if (opcode == -1) return parse_increment (awk, line);

	if (get_token(awk) == -1) return ASE_NULL;

	if (awk->parse.depth.max.expr > 0 &&
	    awk->parse.depth.cur.expr >= awk->parse.depth.max.expr)
	{
		SETERRLIN (awk, ASE_AWK_EEXPRNST, awk->token.line);
		return ASE_NULL;
	}
	awk->parse.depth.cur.expr++;
	left = parse_unary (awk, awk->token.line);
	awk->parse.depth.cur.expr--;
	if (left == ASE_NULL) return ASE_NULL;

	nde = (ase_awk_nde_exp_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_exp_t));
	if (nde == ASE_NULL)
	{
		ase_awk_clrpt (awk, left);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_EXP_UNR;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = ASE_NULL;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_increment (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_exp_t* nde;
	ase_awk_nde_t* left;
	int type, opcode, opcode1, opcode2;

	/* check for prefix increment operator */
	opcode1 = MATCH(awk,TOKEN_PLUSPLUS)? ASE_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? ASE_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1)
	{
		if (get_token(awk) == -1) return ASE_NULL;
	}

	left = parse_primary (awk, line);
	if (left == ASE_NULL) return ASE_NULL;

	if (awk->option & ASE_AWK_IMPLICIT)
	{
		/* concatenation operation by whitepaces are allowed 
		 * if this option is set. the ++/-- operator following
		 * the primary should be treated specially. 
		 * for example, "abc" ++  10 => "abc" . ++10
		 */
		/*if (!is_var(left)) return left; XXX */
		if (!is_var(left) && left->type != ASE_AWK_NDE_POS) return left;
	}

	/* check for postfix increment operator */
	opcode2 = MATCH(awk,TOKEN_PLUSPLUS)? ASE_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? ASE_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1 && opcode2 != -1)
	{
		/* both prefix and postfix increment operator. 
		 * not allowed */
		ase_awk_clrpt (awk, left);

		SETERRLIN (awk, ASE_AWK_EPREPST, line);
		return ASE_NULL;
	}
	else if (opcode1 == -1 && opcode2 == -1)
	{
		/* no increment operators */
		return left;
	}
	else if (opcode1 != -1) 
	{
		/* prefix increment operator */
		type = ASE_AWK_NDE_EXP_INCPRE;
		opcode = opcode1;
	}
	else if (opcode2 != -1) 
	{
		/* postfix increment operator */
		type = ASE_AWK_NDE_EXP_INCPST;
		opcode = opcode2;

		if (get_token(awk) == -1) return ASE_NULL;
	}

	nde = (ase_awk_nde_exp_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_exp_t));
	if (nde == ASE_NULL)
	{
		ase_awk_clrpt (awk, left);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = type;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = ASE_NULL;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_primary (ase_awk_t* awk, ase_size_t line)
{
	if (MATCH(awk,TOKEN_IDENT))  
	{
		return parse_primary_ident (awk, line);
	}
	else if (MATCH(awk,TOKEN_INT)) 
	{
		ase_awk_nde_int_t* nde;

		nde = (ase_awk_nde_int_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_int_t));
		if (nde == ASE_NULL)
		{
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_INT;
		nde->line = line;
		nde->next = ASE_NULL;
		nde->val = ase_awk_strxtolong (awk, 
			ASE_STR_BUF(&awk->token.name), 
			ASE_STR_LEN(&awk->token.name), 0, ASE_NULL);
		nde->str = ase_awk_strxdup (awk,
			ASE_STR_BUF(&awk->token.name),
			ASE_STR_LEN(&awk->token.name));
		if (nde->str == ASE_NULL)
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;			
		}
		nde->len = ASE_STR_LEN(&awk->token.name);

		ASE_ASSERT (
			ASE_STR_LEN(&awk->token.name) ==
			ase_strlen(ASE_STR_BUF(&awk->token.name)));

		if (get_token(awk) == -1) 
		{
			ASE_AWK_FREE (awk, nde->str);
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;			
		}

		return (ase_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_REAL)) 
	{
		ase_awk_nde_real_t* nde;

		nde = (ase_awk_nde_real_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_real_t));
		if (nde == ASE_NULL)
		{
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_REAL;
		nde->line = line;
		nde->next = ASE_NULL;
		nde->val = ase_awk_strxtoreal (awk, 
			ASE_STR_BUF(&awk->token.name), 
			ASE_STR_LEN(&awk->token.name), ASE_NULL);
		nde->str = ase_awk_strxdup (awk,
			ASE_STR_BUF(&awk->token.name),
			ASE_STR_LEN(&awk->token.name));
		if (nde->str == ASE_NULL)
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;			
		}
		nde->len = ASE_STR_LEN(&awk->token.name);

		ASE_ASSERT (
			ASE_STR_LEN(&awk->token.name) ==
			ase_strlen(ASE_STR_BUF(&awk->token.name)));

		if (get_token(awk) == -1) 
		{
			ASE_AWK_FREE (awk, nde->str);
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;			
		}

		return (ase_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_STR))  
	{
		ase_awk_nde_str_t* nde;

		nde = (ase_awk_nde_str_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_str_t));
		if (nde == ASE_NULL)
		{
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_STR;
		nde->line = line;
		nde->next = ASE_NULL;
		nde->len = ASE_STR_LEN(&awk->token.name);
		nde->buf = ase_awk_strxdup (awk,
			ASE_STR_BUF(&awk->token.name), nde->len);
		if (nde->buf == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, nde);
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		if (get_token(awk) == -1) 
		{
			ASE_AWK_FREE (awk, nde->buf);
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;			
		}

		return (ase_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_DIV))
	{
		ase_awk_nde_rex_t* nde;
		int errnum;

		/* the regular expression is tokenized here because 
		 * of the context-sensitivity of the slash symbol */
		SET_TOKEN_TYPE (awk, TOKEN_REX);

		ase_str_clear (&awk->token.name);
		if (get_rexstr (awk) == -1) return ASE_NULL;
		ASE_ASSERT (MATCH(awk,TOKEN_REX));

		nde = (ase_awk_nde_rex_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_rex_t));
		if (nde == ASE_NULL)
		{
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_REX;
		nde->line = line;
		nde->next = ASE_NULL;

		nde->len = ASE_STR_LEN(&awk->token.name);
		nde->buf = ase_awk_strxdup (awk,
			ASE_STR_BUF(&awk->token.name),
			ASE_STR_LEN(&awk->token.name));
		if (nde->buf == ASE_NULL)
		{
			ASE_AWK_FREE (awk, nde);
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->code = ASE_AWK_BUILDREX (awk,
			ASE_STR_BUF(&awk->token.name), 
			ASE_STR_LEN(&awk->token.name), 
			&errnum);
		if (nde->code == ASE_NULL)
		{
			ASE_AWK_FREE (awk, nde->buf);
			ASE_AWK_FREE (awk, nde);

			SETERRLIN (awk, errnum, line);
			return ASE_NULL;
		}

		if (get_token(awk) == -1) 
		{
			ASE_AWK_FREE (awk, nde->buf);
			ASE_AWK_FREE (awk, nde->code);
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;			
		}

		return (ase_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_DOLLAR)) 
	{
		ase_awk_nde_pos_t* nde;
		ase_awk_nde_t* prim;

		if (get_token(awk)) return ASE_NULL;
		
		prim = parse_primary (awk, awk->token.line);
		if (prim == ASE_NULL) return ASE_NULL;

		nde = (ase_awk_nde_pos_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_pos_t));
		if (nde == ASE_NULL) 
		{
			ase_awk_clrpt (awk, prim);
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_POS;
		nde->line = line;
		nde->next = ASE_NULL;
		nde->val = prim;

		return (ase_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		ase_awk_nde_t* nde;
		ase_awk_nde_t* last;

		/* eat up the left parenthesis */
		if (get_token(awk) == -1) return ASE_NULL;

		/* parse the sub-expression inside the parentheses */
		nde = parse_expression (awk, awk->token.line);
		if (nde == ASE_NULL) return ASE_NULL;

		/* parse subsequent expressions separated by a comma, if any */
		last = nde;
		ASE_ASSERT (last->next == ASE_NULL);

		while (MATCH(awk,TOKEN_COMMA))
		{
			ase_awk_nde_t* tmp;

			if (get_token(awk) == -1) 
			{
				ase_awk_clrpt (awk, nde);
				return ASE_NULL;
			}	

			tmp = parse_expression (awk, awk->token.line);
			if (tmp == ASE_NULL) 
			{
				ase_awk_clrpt (awk, nde);
				return ASE_NULL;
			}

			ASE_ASSERT (tmp->next == ASE_NULL);
			last->next = tmp;
			last = tmp;
		} 
		/* ----------------- */

		/* check for the closing parenthesis */
		if (!MATCH(awk,TOKEN_RPAREN)) 
		{
			ase_awk_clrpt (awk, nde);

			SETERRTOK (awk, ASE_AWK_ERPAREN);
			return ASE_NULL;
		}

		if (get_token(awk) == -1) 
		{
			ase_awk_clrpt (awk, nde);
			return ASE_NULL;
		}

		/* check if it is a chained node */
		if (nde->next != ASE_NULL)
		{
			/* if so, it is a expression group */
			/* (expr1, expr2, expr2) */

			ase_awk_nde_grp_t* tmp;

			if ((awk->parse.id.stmnt != TOKEN_PRINT &&
			     awk->parse.id.stmnt != TOKEN_PRINTF) ||
			    awk->parse.depth.cur.expr != 1)
			{
				if (!MATCH(awk,TOKEN_IN))
				{
					ase_awk_clrpt (awk, nde);

					SETERRTOK (awk, ASE_AWK_EIN);
					return ASE_NULL;
				}
			}

			tmp = (ase_awk_nde_grp_t*) ASE_AWK_MALLOC (
				awk, ASE_SIZEOF(ase_awk_nde_grp_t));
			if (tmp == ASE_NULL)
			{
				ase_awk_clrpt (awk, nde);
				SETERRLIN (awk, ASE_AWK_ENOMEM, line);
				return ASE_NULL;
			}	

			tmp->type = ASE_AWK_NDE_GRP;
			tmp->line = line;
			tmp->next = ASE_NULL;
			tmp->body = nde;		

			nde = (ase_awk_nde_t*)tmp;
		}
		/* ----------------- */

		return nde;
	}
	else if (MATCH(awk,TOKEN_GETLINE)) 
	{
		ase_awk_nde_getline_t* nde;
		ase_awk_nde_t* var = ASE_NULL;
		ase_awk_nde_t* in = ASE_NULL;

		if (get_token(awk) == -1) return ASE_NULL;

		if (MATCH(awk,TOKEN_IDENT))
		{
			/* getline var */
			var = parse_primary (awk, awk->token.line);
			if (var == ASE_NULL) return ASE_NULL;
		}

		if (MATCH(awk, TOKEN_LT))
		{
			/* getline [var] < file */
			if (get_token(awk) == -1)
			{
				if (var != ASE_NULL) ase_awk_clrpt (awk, var);
				return ASE_NULL;
			}

			/* TODO: is this correct? */
			/*in = parse_expression (awk);*/
			in = parse_primary (awk, awk->token.line);
			if (in == ASE_NULL)
			{
				if (var != ASE_NULL) ase_awk_clrpt (awk, var);
				return ASE_NULL;
			}
		}

		nde = (ase_awk_nde_getline_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_getline_t));
		if (nde == ASE_NULL)
		{
			if (var != ASE_NULL) ase_awk_clrpt (awk, var);
			if (in != ASE_NULL) ase_awk_clrpt (awk, in);
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_GETLINE;
		nde->line = line;
		nde->next = ASE_NULL;
		nde->var = var;
		nde->in_type = (in == ASE_NULL)? 
			ASE_AWK_IN_CONSOLE: ASE_AWK_IN_FILE;
		nde->in = in;

		return (ase_awk_nde_t*)nde;
	}

	/* valid expression introducer is expected */
	SETERRLIN (awk, ASE_AWK_EEXPRES, 
		(MATCH(awk,TOKEN_EOF)? awk->token.prev.line: line));
	return ASE_NULL;
}

static ase_awk_nde_t* parse_primary_ident (ase_awk_t* awk, ase_size_t line)
{
	ase_char_t* name_dup;
	ase_size_t name_len;
	ase_awk_bfn_t* bfn;
	ase_size_t idxa;

	ASE_ASSERT (MATCH(awk,TOKEN_IDENT));

	name_dup = ase_awk_strxdup (awk,
		ASE_STR_BUF(&awk->token.name),
		ASE_STR_LEN(&awk->token.name));
	if (name_dup == ASE_NULL) 
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}
	name_len = ASE_STR_LEN(&awk->token.name);

	if (get_token(awk) == -1) 
	{
		ASE_AWK_FREE (awk, name_dup);
		return ASE_NULL;			
	}

	/* check if name_dup is an intrinsic function name */
	bfn = ase_awk_getbfn (awk, name_dup, name_len);
	if (bfn != ASE_NULL)
	{
		ase_awk_nde_t* nde;

		if (!MATCH(awk,TOKEN_LPAREN))
		{
			/* an intrinsic function should be in the form 
		 	 * of the function call */
			ASE_AWK_FREE (awk, name_dup);
			SETERRTOK (awk, ASE_AWK_ELPAREN);
			return ASE_NULL;
		}

		nde = parse_fncall (awk, name_dup, name_len, bfn, line);
		if (nde == ASE_NULL) ASE_AWK_FREE (awk, name_dup);
		return (ase_awk_nde_t*)nde;
	}

	/* now we know that name_dup is a normal identifier. */
	if (MATCH(awk,TOKEN_LBRACK)) 
	{
		ase_awk_nde_t* nde;
		nde = parse_hashidx (awk, name_dup, name_len, line);
		if (nde == ASE_NULL) ASE_AWK_FREE (awk, name_dup);
		return (ase_awk_nde_t*)nde;
	}
	else if ((idxa = ase_awk_tab_rrfind (
		&awk->parse.locals, 0, name_dup, name_len)) != (ase_size_t)-1)
	{
		/* local variable */

		ase_awk_nde_var_t* nde;

		if (MATCH(awk,TOKEN_LPAREN))
		{
			/* a local variable is not a function */
			SETERRARG (awk, ASE_AWK_EFNNAME, line, name_dup, name_len);
			ASE_AWK_FREE (awk, name_dup);
			return ASE_NULL;
		}

		nde = (ase_awk_nde_var_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_var_t));
		if (nde == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, name_dup);
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_LOCAL;
		nde->line = line;
		nde->next = ASE_NULL;
		/*nde->id.name = ASE_NULL;*/
		nde->id.name = name_dup;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = ASE_NULL;

		return (ase_awk_nde_t*)nde;
	}
	else if ((idxa = ase_awk_tab_find (
		&awk->parse.params, 0, name_dup, name_len)) != (ase_size_t)-1)
	{
		/* parameter */

		ase_awk_nde_var_t* nde;

		if (MATCH(awk,TOKEN_LPAREN))
		{
			/* a parameter is not a function */
			SETERRARG (awk, ASE_AWK_EFNNAME, line, name_dup, name_len);
			ASE_AWK_FREE (awk, name_dup);
			return ASE_NULL;
		}

		nde = (ase_awk_nde_var_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_var_t));
		if (nde == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, name_dup);
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_ARG;
		nde->line = line;
		nde->next = ASE_NULL;
		/*nde->id.name = ASE_NULL;*/
		nde->id.name = name_dup;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = ASE_NULL;

		return (ase_awk_nde_t*)nde;
	}
	else if ((idxa = get_global (
		awk, name_dup, name_len)) != (ase_size_t)-1)
	{
		/* global variable */

		ase_awk_nde_var_t* nde;

		if (MATCH(awk,TOKEN_LPAREN))
		{
			/* a global variable is not a function */
			SETERRARG (awk, ASE_AWK_EFNNAME, line, name_dup, name_len);
			ASE_AWK_FREE (awk, name_dup);
			return ASE_NULL;
		}

		nde = (ase_awk_nde_var_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_var_t));
		if (nde == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, name_dup);
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		nde->type = ASE_AWK_NDE_GLOBAL;
		nde->line = line;
		nde->next = ASE_NULL;
		/*nde->id.name = ASE_NULL;*/
		nde->id.name = name_dup;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = ASE_NULL;

		return (ase_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		/* function call */
		ase_awk_nde_t* nde;

		if (awk->option & ASE_AWK_IMPLICIT)
		{
			if (ase_map_get (awk->parse.named, name_dup, name_len) != ASE_NULL)
			{
				/* a function call conflicts with a named variable */
				SETERRARG (awk, ASE_AWK_EVARRED, line, name_dup, name_len);
				ASE_AWK_FREE (awk, name_dup);
				return ASE_NULL;
			}
		}

		nde = parse_fncall (awk, name_dup, name_len, ASE_NULL, line);
		if (nde == ASE_NULL) ASE_AWK_FREE (awk, name_dup);
		return (ase_awk_nde_t*)nde;
	}	
	else 
	{
		/* named variable */
		ase_awk_nde_var_t* nde;

		nde = (ase_awk_nde_var_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_nde_var_t));
		if (nde == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, name_dup);
			SETERRLIN (awk, ASE_AWK_ENOMEM, line);
			return ASE_NULL;
		}

		if (awk->option & ASE_AWK_IMPLICIT) 
		{
			#if 0
			if (awk->option & ASE_AWK_UNIQUEFN)
			{
			#endif
				ase_bool_t iscur = ASE_FALSE;

				/* the name should not conflict with a function name */
				/* check if it is a builtin function */
				if (ase_awk_getbfn (awk, name_dup, name_len) != ASE_NULL)
				{
					SETERRARG (awk, ASE_AWK_EBFNRED, line, name_dup, name_len);
					goto exit_func;
				}

				/* check if it is an AWK function */
				if (awk->tree.cur_afn.ptr != ASE_NULL)
				{
					iscur = (ase_strxncmp (
						awk->tree.cur_afn.ptr, awk->tree.cur_afn.len, 
						name_dup, name_len) == 0);
				}

				if (iscur || ase_map_get (awk->tree.afns, name_dup, name_len) != ASE_NULL) 
				{
					/* the function is defined previously */
					SETERRARG (awk, ASE_AWK_EAFNRED, line, name_dup, name_len);
					goto exit_func;
				}

				if (ase_map_get (awk->parse.afns, name_dup, name_len) != ASE_NULL)
				{
					/* is it one of the function calls found so far? */
					SETERRARG (awk, ASE_AWK_EAFNRED, line, name_dup, name_len);
					goto exit_func;
				}
			#if 0
			}
			#endif

			nde->type = ASE_AWK_NDE_NAMED;
			nde->line = line;
			nde->next = ASE_NULL;
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = (ase_size_t)-1;
			nde->idx = ASE_NULL;

			/* collect unique instances of a named variables for reference */
			if (ase_map_put (awk->parse.named,
				name_dup, name_len, (void*)line) == ASE_NULL)
			{
				  SETERRLIN (awk, ASE_AWK_ENOMEM, line);
				  goto exit_func;
			}

			return (ase_awk_nde_t*)nde;
		}

		/* undefined variable */
		SETERRARG (awk, ASE_AWK_EUNDEF, line, name_dup, name_len);

	exit_func:
		ASE_AWK_FREE (awk, name_dup);
		ASE_AWK_FREE (awk, nde);

		return ASE_NULL;
	}
}

static ase_awk_nde_t* parse_hashidx (
	ase_awk_t* awk, ase_char_t* name, ase_size_t name_len, ase_size_t line)
{
	ase_awk_nde_t* idx, * tmp, * last;
	ase_awk_nde_var_t* nde;
	ase_size_t idxa;

	idx = ASE_NULL;
	last = ASE_NULL;

	do
	{
		if (get_token(awk) == -1) 
		{
			if (idx != ASE_NULL) ase_awk_clrpt (awk, idx);
			return ASE_NULL;
		}

		tmp = parse_expression (awk, awk->token.line);
		if (tmp == ASE_NULL) 
		{
			if (idx != ASE_NULL) ase_awk_clrpt (awk, idx);
			return ASE_NULL;
		}

		if (idx == ASE_NULL)
		{
			ASE_ASSERT (last == ASE_NULL);
			idx = tmp; last = tmp;
		}
		else
		{
			last->next = tmp;
			last = tmp;
		}
	}
	while (MATCH(awk,TOKEN_COMMA));

	ASE_ASSERT (idx != ASE_NULL);

	if (!MATCH(awk,TOKEN_RBRACK)) 
	{
		ase_awk_clrpt (awk, idx);

		SETERRTOK (awk, ASE_AWK_ERBRACK);
		return ASE_NULL;
	}

	if (get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, idx);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_var_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_var_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, idx);
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	/* search the local variable list */
	idxa = ase_awk_tab_rrfind(&awk->parse.locals, 0, name, name_len);
	if (idxa != (ase_size_t)-1) 
	{
		nde->type = ASE_AWK_NDE_LOCALIDX;
		nde->line = line;
		nde->next = ASE_NULL;
		/*nde->id.name = ASE_NULL; */
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (ase_awk_nde_t*)nde;
	}

	/* search the parameter name list */
	idxa = ase_awk_tab_find (&awk->parse.params, 0, name, name_len);
	if (idxa != (ase_size_t)-1) 
	{
		nde->type = ASE_AWK_NDE_ARGIDX;
		nde->line = line;
		nde->next = ASE_NULL;
		/*nde->id.name = ASE_NULL; */
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (ase_awk_nde_t*)nde;
	}

	/* gets the global variable index */
	idxa = get_global (awk, name, name_len);
	if (idxa != (ase_size_t)-1) 
	{
		nde->type = ASE_AWK_NDE_GLOBALIDX;
		nde->line = line;
		nde->next = ASE_NULL;
		/*nde->id.name = ASE_NULL;*/
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (ase_awk_nde_t*)nde;
	}

	if (awk->option & ASE_AWK_IMPLICIT) 
	{
		#if 0
		if (awk->option & ASE_AWK_UNIQUEFN) 
		{
		#endif
			ase_bool_t iscur = ASE_FALSE;

			/* check if it is a builtin function */
			if (ase_awk_getbfn (awk, name, name_len) != ASE_NULL)
			{
				SETERRARG (awk, ASE_AWK_EBFNRED, line, name, name_len);
				goto exit_func;
			}

			/* check if it is an AWK function */
			if (awk->tree.cur_afn.ptr != ASE_NULL)
			{
				iscur = (ase_strxncmp (
					awk->tree.cur_afn.ptr, awk->tree.cur_afn.len, 
					name, name_len) == 0);
			}

			if (iscur || ase_map_get (awk->tree.afns, name, name_len) != ASE_NULL) 
			{
				/* the function is defined previously */
				SETERRARG (awk, ASE_AWK_EAFNRED, line, name, name_len);
				goto exit_func;
			}

			if (ase_map_get (awk->parse.afns, name, name_len) != ASE_NULL)
			{
				/* is it one of the function calls found so far? */
				SETERRARG (awk, ASE_AWK_EAFNRED, line, name, name_len);
				goto exit_func;
			}
		#if 0
		}
		#endif

		nde->type = ASE_AWK_NDE_NAMEDIDX;
		nde->line = line;
		nde->next = ASE_NULL;
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = (ase_size_t)-1;
		nde->idx = idx;

		return (ase_awk_nde_t*)nde;
	}

	/* undefined variable */
	SETERRARG (awk, ASE_AWK_EUNDEF, line, name, name_len);


exit_func:
	ase_awk_clrpt (awk, idx);
	ASE_AWK_FREE (awk, nde);

	return ASE_NULL;
}

static ase_awk_nde_t* parse_fncall (
	ase_awk_t* awk, ase_char_t* name, ase_size_t name_len, 
	ase_awk_bfn_t* bfn, ase_size_t line)
{
	ase_awk_nde_t* head, * curr, * nde;
	ase_awk_nde_call_t* call;
	ase_size_t nargs;

	if (get_token(awk) == -1) return ASE_NULL;
	
	head = curr = ASE_NULL;
	nargs = 0;

	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no parameters to the function call */
		if (get_token(awk) == -1) return ASE_NULL;
	}
	else 
	{
		/* parse function parameters */

		while (1) 
		{
			nde = parse_expression (awk, awk->token.line);
			if (nde == ASE_NULL) 
			{
				if (head != ASE_NULL) ase_awk_clrpt (awk, head);
				return ASE_NULL;
			}
	
			if (head == ASE_NULL) head = nde;
			else curr->next = nde;
			curr = nde;

			nargs++;

			if (MATCH(awk,TOKEN_RPAREN)) 
			{
				if (get_token(awk) == -1) 
				{
					if (head != ASE_NULL) 
						ase_awk_clrpt (awk, head);
					return ASE_NULL;
				}
				break;
			}

			if (!MATCH(awk,TOKEN_COMMA)) 
			{
				if (head != ASE_NULL) ase_awk_clrpt (awk, head);

				SETERRTOK (awk, ASE_AWK_ECOMMA);
				return ASE_NULL;
			}

			if (get_token(awk) == -1) 
			{
				if (head != ASE_NULL) ase_awk_clrpt (awk, head);
				return ASE_NULL;
			}
		}

	}

	call = (ase_awk_nde_call_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_call_t));
	if (call == ASE_NULL) 
	{
		if (head != ASE_NULL) ase_awk_clrpt (awk, head);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	if (bfn != ASE_NULL)
	{
		call->type = ASE_AWK_NDE_BFN;
		call->line = line;
		call->next = ASE_NULL;

		/*call->what.bfn = bfn; */
		call->what.bfn.name.ptr = name;
		call->what.bfn.name.len = name_len;

		/* NOTE: oname is the original as in the bfn table.
		 *       it would not duplicated here and not freed in
		 *       ase_awk_clrpt either. so ase_awk_delfunc between
		 *       ase_awk_parse and ase_awk_run may cause the program 
		 *       to fail. */
		call->what.bfn.oname.ptr = bfn->name.ptr;
		call->what.bfn.oname.len = bfn->name.len;

		call->what.bfn.arg.min = bfn->arg.min;
		call->what.bfn.arg.max = bfn->arg.max;
		call->what.bfn.arg.spec = bfn->arg.spec;
		call->what.bfn.handler = bfn->handler;

		call->args = head;
		call->nargs = nargs;
	}
	else
	{
		call->type = ASE_AWK_NDE_AFN;
		call->line = line;
		call->next = ASE_NULL;
		call->what.afn.name.ptr = name; 
		call->what.afn.name.len = name_len;
		call->args = head;
		call->nargs = nargs;

		#if 0
		if (((awk->option & ASE_AWK_EXPLICIT) && 
		     !(awk->option & ASE_AWK_IMPLICIT)) /*|| 
		    (awk->option & ASE_AWK_UNIQUEFN)*/)
		{
		#endif

			/* this line number might be truncated as 
			 * sizeof(line) could be > sizeof(void*) */
			if (ase_map_put (awk->parse.afns, 
				name, name_len, (void*)line) == ASE_NULL)
			{
				ASE_AWK_FREE (awk, call);
				if (head != ASE_NULL) ase_awk_clrpt (awk, head);
				SETERRLIN (awk, ASE_AWK_ENOMEM, line);
				return ASE_NULL;
			}
		#if 0
		}
		#endif
	}

	return (ase_awk_nde_t*)call;
}

static ase_awk_nde_t* parse_if (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* test;
	ase_awk_nde_t* then_part;
	ase_awk_nde_t* else_part;
	ase_awk_nde_if_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		SETERRTOK (awk, ASE_AWK_ELPAREN);
		return ASE_NULL;

	}
	if (get_token(awk) == -1) return ASE_NULL;

	test = parse_expression (awk, awk->token.line);
	if (test == ASE_NULL) return ASE_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		ase_awk_clrpt (awk, test);

		SETERRTOK (awk, ASE_AWK_ERPAREN);
		return ASE_NULL;
	}

	if (get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}

	then_part = parse_statement (awk, awk->token.line);
	if (then_part == ASE_NULL) 
	{
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}

	if (MATCH(awk,TOKEN_ELSE)) 
	{
		if (get_token(awk) == -1) 
		{
			ase_awk_clrpt (awk, then_part);
			ase_awk_clrpt (awk, test);
			return ASE_NULL;
		}

		else_part = parse_statement (awk, awk->token.prev.line);
		if (else_part == ASE_NULL) 
		{
			ase_awk_clrpt (awk, then_part);
			ase_awk_clrpt (awk, test);
			return ASE_NULL;
		}
	}
	else else_part = ASE_NULL;

	nde = (ase_awk_nde_if_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_if_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, else_part);
		ase_awk_clrpt (awk, then_part);
		ase_awk_clrpt (awk, test);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_IF;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->test = test;
	nde->then_part = then_part;
	nde->else_part = else_part;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_while (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* test, * body;
	ase_awk_nde_while_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		SETERRTOK (awk, ASE_AWK_ELPAREN);
		return ASE_NULL;
	}
	if (get_token(awk) == -1) return ASE_NULL;

	test = parse_expression (awk, awk->token.line);
	if (test == ASE_NULL) return ASE_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		ase_awk_clrpt (awk, test);

		SETERRTOK (awk, ASE_AWK_ERPAREN);
		return ASE_NULL;
	}

	if (get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}

	body = parse_statement (awk, awk->token.line);
	if (body == ASE_NULL) 
	{
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_while_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_while_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, body);
		ase_awk_clrpt (awk, test);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_WHILE;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->test = test;
	nde->body = body;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_for (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* init, * test, * incr, * body;
	ase_awk_nde_for_t* nde; 
	ase_awk_nde_foreach_t* nde2;

	if (!MATCH(awk,TOKEN_LPAREN))
	{
		SETERRTOK (awk, ASE_AWK_ELPAREN);
		return ASE_NULL;
	}
	if (get_token(awk) == -1) return ASE_NULL;
		
	if (MATCH(awk,TOKEN_SEMICOLON)) init = ASE_NULL;
	else 
	{
		/* this line is very ugly. it checks the entire next 
		 * expression or the first element in the expression
		 * is wrapped by a parenthesis */
		int no_foreach = MATCH(awk,TOKEN_LPAREN);

		init = parse_expression (awk, awk->token.line);
		if (init == ASE_NULL) return ASE_NULL;

		if (!no_foreach && init->type == ASE_AWK_NDE_EXP_BIN &&
		    ((ase_awk_nde_exp_t*)init)->opcode == ASE_AWK_BINOP_IN &&
		    is_plain_var(((ase_awk_nde_exp_t*)init)->left))
		{	
			/* switch to foreach */
			
			if (!MATCH(awk,TOKEN_RPAREN))
			{
				ase_awk_clrpt (awk, init);
				SETERRTOK (awk, ASE_AWK_ERPAREN);
				return ASE_NULL;
			}

			if (get_token(awk) == -1) 
			{
				ase_awk_clrpt (awk, init);
				return ASE_NULL;
			}	
			
			body = parse_statement (awk, awk->token.line);
			if (body == ASE_NULL) 
			{
				ase_awk_clrpt (awk, init);
				return ASE_NULL;
			}

			nde2 = (ase_awk_nde_foreach_t*) ASE_AWK_MALLOC (
				awk, ASE_SIZEOF(ase_awk_nde_foreach_t));
			if (nde2 == ASE_NULL)
			{
				ase_awk_clrpt (awk, init);
				ase_awk_clrpt (awk, body);

				SETERRLIN (awk, ASE_AWK_ENOMEM, line);
				return ASE_NULL;
			}

			nde2->type = ASE_AWK_NDE_FOREACH;
			nde2->line = line;
			nde2->next = ASE_NULL;
			nde2->test = init;
			nde2->body = body;

			return (ase_awk_nde_t*)nde2;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			ase_awk_clrpt (awk, init);
			SETERRTOK (awk, ASE_AWK_ESCOLON);
			return ASE_NULL;
		}
	}

	do
	{
		if (get_token(awk) == -1) 
		{
			ase_awk_clrpt (awk, init);
			return ASE_NULL;
		}

		/* skip new lines after the first semicolon */
	} 
	while (MATCH(awk,TOKEN_NEWLINE));

	if (MATCH(awk,TOKEN_SEMICOLON)) test = ASE_NULL;
	else 
	{
		test = parse_expression (awk, awk->token.line);
		if (test == ASE_NULL) 
		{
			ase_awk_clrpt (awk, init);
			return ASE_NULL;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			ase_awk_clrpt (awk, init);
			ase_awk_clrpt (awk, test);

			SETERRTOK (awk, ASE_AWK_ESCOLON);
			return ASE_NULL;
		}
	}

	do
	{
		if (get_token(awk) == -1) 
		{
			ase_awk_clrpt (awk, init);
			ase_awk_clrpt (awk, test);
			return ASE_NULL;
		}

		/* skip new lines after the second semicolon */
	}
	while (MATCH(awk,TOKEN_NEWLINE));
	
	if (MATCH(awk,TOKEN_RPAREN)) incr = ASE_NULL;
	else 
	{
		incr = parse_expression (awk, awk->token.line);
		if (incr == ASE_NULL) 
		{
			ase_awk_clrpt (awk, init);
			ase_awk_clrpt (awk, test);
			return ASE_NULL;
		}

		if (!MATCH(awk,TOKEN_RPAREN)) 
		{
			ase_awk_clrpt (awk, init);
			ase_awk_clrpt (awk, test);
			ase_awk_clrpt (awk, incr);

			SETERRTOK (awk, ASE_AWK_ERPAREN);
			return ASE_NULL;
		}
	}

	if (get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, init);
		ase_awk_clrpt (awk, test);
		ase_awk_clrpt (awk, incr);
		return ASE_NULL;
	}

	body = parse_statement (awk, awk->token.line);
	if (body == ASE_NULL) 
	{
		ase_awk_clrpt (awk, init);
		ase_awk_clrpt (awk, test);
		ase_awk_clrpt (awk, incr);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_for_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_for_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, init);
		ase_awk_clrpt (awk, test);
		ase_awk_clrpt (awk, incr);
		ase_awk_clrpt (awk, body);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_FOR;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->init = init;
	nde->test = test;
	nde->incr = incr;
	nde->body = body;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_dowhile (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_t* test, * body;
	ase_awk_nde_while_t* nde;

	ASE_ASSERT (awk->token.prev.type == TOKEN_DO);

	body = parse_statement (awk, awk->token.line);
	if (body == ASE_NULL) return ASE_NULL;

	while (MATCH(awk,TOKEN_NEWLINE))
	{
		if (get_token(awk) == -1) 
		{
			ase_awk_clrpt (awk, body);
			return ASE_NULL;
		}
	}

	if (!MATCH(awk,TOKEN_WHILE)) 
	{
		ase_awk_clrpt (awk, body);

		SETERRTOK (awk, ASE_AWK_EWHILE);
		return ASE_NULL;
	}

	if (get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, body);
		return ASE_NULL;
	}

	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		ase_awk_clrpt (awk, body);

		SETERRTOK (awk, ASE_AWK_ELPAREN);
		return ASE_NULL;
	}

	if (get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, body);
		return ASE_NULL;
	}

	test = parse_expression (awk, awk->token.line);
	if (test == ASE_NULL) 
	{
		ase_awk_clrpt (awk, body);
		return ASE_NULL;
	}

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		ase_awk_clrpt (awk, body);
		ase_awk_clrpt (awk, test);

		SETERRTOK (awk, ASE_AWK_ERPAREN);
		return ASE_NULL;
	}

	if (get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, body);
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}
	
	nde = (ase_awk_nde_while_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_while_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, body);
		ase_awk_clrpt (awk, test);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_DOWHILE;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->test = test;
	nde->body = body;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_break (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_break_t* nde;

	ASE_ASSERT (awk->token.prev.type == TOKEN_BREAK);
	if (awk->parse.depth.cur.loop <= 0) 
	{
		SETERRLIN (awk, ASE_AWK_EBREAK, line);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_break_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_break_t));
	if (nde == ASE_NULL)
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_BREAK;
	nde->line = line;
	nde->next = ASE_NULL;
	
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_continue (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_continue_t* nde;

	ASE_ASSERT (awk->token.prev.type == TOKEN_CONTINUE);
	if (awk->parse.depth.cur.loop <= 0) 
	{
		SETERRLIN (awk, ASE_AWK_ECONTINUE, line);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_continue_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_continue_t));
	if (nde == ASE_NULL)
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_CONTINUE;
	nde->line = line;
	nde->next = ASE_NULL;
	
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_return (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_return_t* nde;
	ase_awk_nde_t* val;

	ASE_ASSERT (awk->token.prev.type == TOKEN_RETURN);

	nde = (ase_awk_nde_return_t*) ASE_AWK_MALLOC (
		awk, ASE_SIZEOF(ase_awk_nde_return_t));
	if (nde == ASE_NULL)
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_RETURN;
	nde->line = line;
	nde->next = ASE_NULL;

	if (MATCH_TERMINATOR(awk)) 
	{
		/* no return value */
		val = ASE_NULL;
	}
	else 
	{
		val = parse_expression (awk, awk->token.line);
		if (val == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;
		}
	}

	nde->val = val;
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_exit (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_exit_t* nde;
	ase_awk_nde_t* val;

	ASE_ASSERT (awk->token.prev.type == TOKEN_EXIT);

	nde = (ase_awk_nde_exit_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_exit_t));
	if (nde == ASE_NULL)
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_EXIT;
	nde->line = line;
	nde->next = ASE_NULL;

	if (MATCH_TERMINATOR(awk)) 
	{
		/* no exit code */
		val = ASE_NULL;
	}
	else 
	{
		val = parse_expression (awk, awk->token.line);
		if (val == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;
		}
	}

	nde->val = val;
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_next (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_next_t* nde;

	ASE_ASSERT (awk->token.prev.type == TOKEN_NEXT);

	if (awk->parse.id.block == PARSE_BEGIN_BLOCK)
	{
		SETERRLIN (awk, ASE_AWK_ENEXTBEG, line);
		return ASE_NULL;
	}
	if (awk->parse.id.block == PARSE_END_BLOCK)
	{
		SETERRLIN (awk, ASE_AWK_ENEXTEND, line);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_next_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_next_t));
	if (nde == ASE_NULL)
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}
	nde->type = ASE_AWK_NDE_NEXT;
	nde->line = line;
	nde->next = ASE_NULL;
	
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_nextfile (ase_awk_t* awk, ase_size_t line, int out)
{
	ase_awk_nde_nextfile_t* nde;

	if (!out && awk->parse.id.block == PARSE_BEGIN_BLOCK)
	{
		SETERRLIN (awk, ASE_AWK_ENEXTFBEG, line);
		return ASE_NULL;
	}
	if (!out && awk->parse.id.block == PARSE_END_BLOCK)
	{
		SETERRLIN (awk, ASE_AWK_ENEXTFEND, line);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_nextfile_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_nextfile_t));
	if (nde == ASE_NULL)
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_NEXTFILE;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->out = out;
	
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_delete (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_delete_t* nde;
	ase_awk_nde_t* var;

	ASE_ASSERT (awk->token.prev.type == TOKEN_DELETE);
	if (!MATCH(awk,TOKEN_IDENT)) 
	{
		SETERRTOK (awk, ASE_AWK_EIDENT);
		return ASE_NULL;
	}

	var = parse_primary_ident (awk, awk->token.line);
	if (var == ASE_NULL) return ASE_NULL;

	if (!is_var (var))
	{
		/* a normal identifier is expected */
		ase_awk_clrpt (awk, var);
		SETERRLIN (awk, ASE_AWK_EDELETE, line);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_delete_t*) ASE_AWK_MALLOC (
		awk, ASE_SIZEOF(ase_awk_nde_delete_t));
	if (nde == ASE_NULL)
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_DELETE;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->var = var;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_reset (ase_awk_t* awk, ase_size_t line)
{
	ase_awk_nde_reset_t* nde;
	ase_awk_nde_t* var;

	ASE_ASSERT (awk->token.prev.type == TOKEN_RESET);
	if (!MATCH(awk,TOKEN_IDENT)) 
	{
		SETERRTOK (awk, ASE_AWK_EIDENT);
		return ASE_NULL;
	}

	var = parse_primary_ident (awk, awk->token.line);
	if (var == ASE_NULL) return ASE_NULL;

	/* unlike delete, it must be followed by a plain variable only */
	if (!is_plain_var (var))
	{
		/* a normal identifier is expected */
		ase_awk_clrpt (awk, var);
		SETERRLIN (awk, ASE_AWK_ERESET, line);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_reset_t*) ASE_AWK_MALLOC (
		awk, ASE_SIZEOF(ase_awk_nde_reset_t));
	if (nde == ASE_NULL)
	{
		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	nde->type = ASE_AWK_NDE_RESET;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->var = var;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* parse_print (ase_awk_t* awk, ase_size_t line, int type)
{
	ase_awk_nde_print_t* nde;
	ase_awk_nde_t* args = ASE_NULL; 
	ase_awk_nde_t* out = ASE_NULL;
	int out_type;

	if (!MATCH_TERMINATOR(awk) &&
	    !MATCH(awk,TOKEN_GT) &&
	    !MATCH(awk,TOKEN_RSHIFT) &&
	    !MATCH(awk,TOKEN_BOR) &&
	    !MATCH(awk,TOKEN_BORAND)) 
	{
		ase_awk_nde_t* args_tail;
		ase_awk_nde_t* tail_prev;

		args = parse_expression (awk, awk->token.line);
		if (args == ASE_NULL) return ASE_NULL;

		args_tail = args;
		tail_prev = ASE_NULL;

		if (args->type != ASE_AWK_NDE_GRP)
		{
			/* args->type == ASE_AWK_NDE_GRP when print (a, b, c) 
			 * args->type != ASE_AWK_NDE_GRP when print a, b, c */
			
			while (MATCH(awk,TOKEN_COMMA))
			{
				do {
					if (get_token(awk) == -1)
					{
						ase_awk_clrpt (awk, args);
						return ASE_NULL;
					}
				}
				while (MATCH(awk,TOKEN_NEWLINE));

				args_tail->next = parse_expression (awk, awk->token.line);
				if (args_tail->next == ASE_NULL)
				{
					ase_awk_clrpt (awk, args);
					return ASE_NULL;
				}

				tail_prev = args_tail;
				args_tail = args_tail->next;
			}
		}

		/* print 1 > 2 would print 1 to the file named 2. 
		 * print (1 > 2) would print (1 > 2) on the console */
		if (awk->token.prev.type != TOKEN_RPAREN &&
		    args_tail->type == ASE_AWK_NDE_EXP_BIN)
		{
			ase_awk_nde_exp_t* ep = (ase_awk_nde_exp_t*)args_tail;
			if (ep->opcode == ASE_AWK_BINOP_GT)
			{
				ase_awk_nde_t* tmp = args_tail;

				if (tail_prev != ASE_NULL) 
					tail_prev->next = ep->left;
				else args = ep->left;

				out = ep->right;
				out_type = ASE_AWK_OUT_FILE;

				ASE_AWK_FREE (awk, tmp);
			}
			else if (ep->opcode == ASE_AWK_BINOP_RSHIFT)
			{
				ase_awk_nde_t* tmp = args_tail;

				if (tail_prev != ASE_NULL) 
					tail_prev->next = ep->left;
				else args = ep->left;

				out = ep->right;
				out_type = ASE_AWK_OUT_FILE_APPEND;

				ASE_AWK_FREE (awk, tmp);
			}
			else if (ep->opcode == ASE_AWK_BINOP_BOR)
			{
				ase_awk_nde_t* tmp = args_tail;

				if (tail_prev != ASE_NULL) 
					tail_prev->next = ep->left;
				else args = ep->left;

				out = ep->right;
				out_type = ASE_AWK_OUT_PIPE;

				ASE_AWK_FREE (awk, tmp);
			}
		}
	}

	if (out == ASE_NULL)
	{
		out_type = MATCH(awk,TOKEN_GT)?     ASE_AWK_OUT_FILE:
		           MATCH(awk,TOKEN_RSHIFT)? ASE_AWK_OUT_FILE_APPEND:
		           MATCH(awk,TOKEN_BOR)?    ASE_AWK_OUT_PIPE:
		           MATCH(awk,TOKEN_BORAND)? ASE_AWK_OUT_COPROC:
		                                    ASE_AWK_OUT_CONSOLE;

		if (out_type != ASE_AWK_OUT_CONSOLE)
		{
			if (get_token(awk) == -1)
			{
				if (args != ASE_NULL) ase_awk_clrpt (awk, args);
				return ASE_NULL;
			}

			out = parse_expression (awk, awk->token.line);
			if (out == ASE_NULL)
			{
				if (args != ASE_NULL) ase_awk_clrpt (awk, args);
				return ASE_NULL;
			}
		}
	}

	nde = (ase_awk_nde_print_t*) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_nde_print_t));
	if (nde == ASE_NULL) 
	{
		if (args != ASE_NULL) ase_awk_clrpt (awk, args);
		if (out != ASE_NULL) ase_awk_clrpt (awk, out);

		SETERRLIN (awk, ASE_AWK_ENOMEM, line);
		return ASE_NULL;
	}

	ASE_ASSERTX (
		type == ASE_AWK_NDE_PRINT || type == ASE_AWK_NDE_PRINTF, 
		"the node type should be either ASE_AWK_NDE_PRINT or ASE_AWK_NDE_PRINTF");

	if (type == ASE_AWK_NDE_PRINTF && args == ASE_NULL)
	{
		if (out != ASE_NULL) ase_awk_clrpt (awk, out);
		SETERRLIN (awk, ASE_AWK_EPRINTFARG, line);
		return ASE_NULL;
	}

	nde->type = type;
	nde->line = line;
	nde->next = ASE_NULL;
	nde->args = args;
	nde->out_type = out_type;
	nde->out = out;

	return (ase_awk_nde_t*)nde;
}


static int get_token (ase_awk_t* awk)
{
	ase_cint_t c;
	ase_size_t line;
	int n;

	line = awk->token.line;

	awk->token.prev.type = awk->token.type;
	awk->token.prev.line = awk->token.line;
	awk->token.prev.column = awk->token.column;

	do 
	{
		if (skip_spaces(awk) == -1) return -1;
		if ((n = skip_comment(awk)) == -1) return -1;
	} 
	while (n == 1);

	ase_str_clear (&awk->token.name);
	awk->token.line = awk->src.lex.line;
	awk->token.column = awk->src.lex.column;

	c = awk->src.lex.curc;

	if (c == ASE_CHAR_EOF) 
	{
		ADD_TOKEN_STR (awk, ASE_T("<EOF>"), 5);
		SET_TOKEN_TYPE (awk, TOKEN_EOF);
	}	
	else if (c == ASE_T('\n')) 
	{
		ADD_TOKEN_CHAR (awk, ASE_T('\n'));
		SET_TOKEN_TYPE (awk, TOKEN_NEWLINE);
		GET_CHAR (awk);
	}
	else if (ASE_AWK_ISDIGIT (awk, c)/*|| c == ASE_T('.')*/)
	{
		if (get_number (awk) == -1) return -1;
	}
	else if (c == ASE_T('.'))
	{
		GET_CHAR_TO (awk, c);

		if ((awk->option & ASE_AWK_EXPLICIT) == 0 && 
		    ASE_AWK_ISDIGIT (awk, c))
		{
			awk->src.lex.curc = ASE_T('.');
			UNGET_CHAR (awk, c);	
			if (get_number (awk) == -1) return -1;

		}
		else /*if (ASE_AWK_ISSPACE (awk, c) || c == ASE_CHAR_EOF)*/
		{
			SET_TOKEN_TYPE (awk, TOKEN_PERIOD);
			ADD_TOKEN_CHAR (awk, ASE_T('.'));
		}
		/*
		else
		{
			ase_awk_seterror (
				awk, ASE_AWK_ELXCHR, awk->token.line,
				ASE_T("floating point not followed by any valid digits"));
			return -1;
		}
		*/
	}
	else if (ASE_AWK_ISALPHA (awk, c) || c == ASE_T('_')) 
	{
		int type;

		/* identifier */
		do 
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		} 
		while (ASE_AWK_ISALPHA (awk, c) || 
		       c == ASE_T('_') || ASE_AWK_ISDIGIT(awk,c));

		type = classify_ident (awk, 
			ASE_STR_BUF(&awk->token.name), 
			ASE_STR_LEN(&awk->token.name));
		SET_TOKEN_TYPE (awk, type);
	}
	else if (c == ASE_T('\"')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_STR);

		if (get_charstr(awk) == -1) return -1;

		while (awk->option & ASE_AWK_STRCONCAT) 
		{
			do 
			{
				if (skip_spaces(awk) == -1) return -1;
				if ((n = skip_comment(awk)) == -1) return -1;
			} 
			while (n == 1);

			c = awk->src.lex.curc;
			if (c != ASE_T('\"')) break;

			if (get_charstr(awk) == -1) return -1;
		}

	}
#if 0
	else if (c == ASE_T('=')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == ASE_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_EQ);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_ASSIGN);
		}
	}
	else if (c == ASE_T('!')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == ASE_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_NE);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else if (c == ASE_T('~'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_NM);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_LNOT);
		}
	}
	else if (c == ASE_T('>')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == ASE_T('>')) 
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
			if ((awk->option & ASE_AWK_SHIFT) && c == ASE_T('='))
			{
				SET_TOKEN_TYPE (awk, TOKEN_RSHIFT_ASSIGN);
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR (awk);
			}
			else
			{
				SET_TOKEN_TYPE (awk, TOKEN_RSHIFT);
			}
		}
		else if (c == ASE_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_GE);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_GT);
		}
	}
	else if (c == ASE_T('<')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if ((awk->option & ASE_AWK_SHIFT) && c == ASE_T('<')) 
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
			if (c == ASE_T('='))
			{
				SET_TOKEN_TYPE (awk, TOKEN_LSHIFT_ASSIGN);
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR (awk);
			}
			else
			{
				SET_TOKEN_TYPE (awk, TOKEN_LSHIFT);
			}
		}
		else if (c == ASE_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_LE);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_LT);
		}
	}
	else if (c == ASE_T('|'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == ASE_T('|'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_LOR);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else if ((awk->option & ASE_AWK_COPROC) && c == ASE_T('&'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_BORAND);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else if (c == ASE_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_BOR_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_BOR);
		}
	}
	else if (c == ASE_T('&'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == ASE_T('&'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_LAND);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else if (c == ASE_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_BAND_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_BAND);
		}
	}
	else if (c == ASE_T('^'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == ASE_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_BXOR_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_BXOR);
		}
	}
	else if (c == ASE_T('+')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == ASE_T('+')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_PLUSPLUS);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else if (c == ASE_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_PLUS_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_PLUS);
		}
	}
	else if (c == ASE_T('-')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == ASE_T('-')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_MINUSMINUS);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else if (c == ASE_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_MINUS_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_MINUS);
		}
	}
	else if (c == ASE_T('*')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == ASE_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_MUL_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else if (c == ASE_T('*'))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
			if (c == ASE_T('='))
			{
				SET_TOKEN_TYPE (awk, TOKEN_EXP_ASSIGN);
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR (awk);
			}
			else 
			{
				SET_TOKEN_TYPE (awk, TOKEN_EXP);
			}
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_MUL);
		}
	}
	else if (c == ASE_T('/')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == ASE_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_DIV_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else if ((awk->option & ASE_AWK_IDIV) && c == ASE_T('/'))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
			if (c == ASE_T('='))
			{
				SET_TOKEN_TYPE (awk, TOKEN_IDIV_ASSIGN);
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR (awk);
			}
			else 
			{
				SET_TOKEN_TYPE (awk, TOKEN_IDIV);
			}
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_DIV);
		}
	}
	else if (c == ASE_T('%')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == ASE_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_MOD_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_MOD);
		}
	}
#endif
	else
	{
		int n = get_symbol (awk, c);
		if (n == -1) return -1;
		if (n == 0)
		{
			ase_char_t cc = (ase_char_t)c;
			SETERRARG (awk, ASE_AWK_ELXCHR, awk->token.line, &cc, 1);
			return -1;
		}
	}
#if 0
	else 
	{
		int i;
		static struct 
		{
			ase_char_t c;
			int t;
		} tab[] =
		{
			{ ASE_T('~'), TOKEN_TILDE },
			{ ASE_T('('), TOKEN_LPAREN },
			{ ASE_T(')'), TOKEN_RPAREN },
			{ ASE_T('{'), TOKEN_LBRACE },
			{ ASE_T('}'), TOKEN_RBRACE },
			{ ASE_T('['), TOKEN_LBRACK },
			{ ASE_T(']'), TOKEN_RBRACK },
			{ ASE_T('$'), TOKEN_DOLLAR },
			{ ASE_T(','), TOKEN_COMMA },
			{ ASE_T(';'), TOKEN_SEMICOLON },
			{ ASE_T(':'), TOKEN_COLON },
			{ ASE_T('?'), TOKEN_QUEST },
			{ ASE_CHAR_EOF, TOKEN_EOF }
		};

		for (i = 0; tab[i].c != ASE_CHAR_EOF; i++)
		{
			if (c == tab[i].c) 
			{
				SET_TOKEN_TYPE (awk, tab[i].t);
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR (awk);
				goto get_token_ok;
			}
		}

		ase_char_t cc = (ase_char_t)c;
		SETERRARG (awk, ASE_AWK_ELXCHR, awk->token.line, &cc, 1);
		return -1;
	}
#endif

get_token_ok:
wprintf (L"TOKEN=[%S]\n", awk->token.name.buf);
	return 0;
}

static int get_number (ase_awk_t* awk)
{
	ase_cint_t c;

	ASE_ASSERT (ASE_STR_LEN(&awk->token.name) == 0);
	SET_TOKEN_TYPE (awk, TOKEN_INT);

	c = awk->src.lex.curc;

	if (c == ASE_T('0'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == ASE_T('x') || c == ASE_T('X'))
		{
			/* hexadecimal number */
			do 
			{
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			} 
			while (ASE_AWK_ISXDIGIT (awk, c));

			return 0;
		}
		#if 0
		else if (c == ASE_T('b') || c == ASE_T('B'))
		{
			/* binary number */
			do
			{
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			} 
			while (c == ASE_T('0') || c == ASE_T('1'));

			return 0;
		}
		#endif
		else if (c != '.')
		{
			/* octal number */
			while (c >= ASE_T('0') && c <= ASE_T('7'))
			{
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			}

			if (c == ASE_T('8') || c == ASE_T('9'))
			{
				ase_char_t cc = (ase_char_t)c;
				SETERRARG (awk, ASE_AWK_ELXDIG, awk->token.line, &cc, 1);
				return -1;
			}

			return 0;
		}
	}

	while (ASE_AWK_ISDIGIT (awk, c)) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	} 

	if (c == ASE_T('.'))
	{
		/* floating-point number */
		SET_TOKEN_TYPE (awk, TOKEN_REAL);

		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		while (ASE_AWK_ISDIGIT (awk, c))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
	}

	if (c == ASE_T('E') || c == ASE_T('e'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_REAL);

		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == ASE_T('+') || c == ASE_T('-'))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}

		while (ASE_AWK_ISDIGIT (awk, c))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
	}

	return 0;
}

static int get_charstr (ase_awk_t* awk)
{
	if (awk->src.lex.curc != ASE_T('\"')) 
	{
		/* the starting quote has been consumed before this function
		 * has been called */
		ADD_TOKEN_CHAR (awk, awk->src.lex.curc);
	}
	return get_string (awk, ASE_T('\"'), ASE_T('\\'), ASE_FALSE);
}

static int get_rexstr (ase_awk_t* awk)
{
	if (awk->src.lex.curc == ASE_T('/')) 
	{
		/* this part of the function is different from get_charstr
		 * because of the way this function is called */
		GET_CHAR (awk);
		return 0;
	}
	else 
	{
		ADD_TOKEN_CHAR (awk, awk->src.lex.curc);
		return get_string (awk, ASE_T('/'), ASE_T('\\'), ASE_TRUE);
	}
}

static int get_string (
	ase_awk_t* awk, ase_char_t end_char, 
	ase_char_t esc_char, ase_bool_t keep_esc_char)
{
	ase_cint_t c;
	int escaped = 0;
	int digit_count = 0;
	ase_cint_t c_acc = 0;

	while (1)
	{
		GET_CHAR_TO (awk, c);

		if (c == ASE_CHAR_EOF)
		{
			SETERRTOK (awk, ASE_AWK_EENDSTR);
			return -1;
		}

		if (escaped == 3)
		{
			if (c >= ASE_T('0') && c <= ASE_T('7'))
			{
				c_acc = c_acc * 8 + c - ASE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				ADD_TOKEN_CHAR (awk, c_acc);
				escaped = 0;
			}
		}
		else if (escaped == 2 || escaped == 4 || escaped == 8)
		{
			if (c >= ASE_T('0') && c <= ASE_T('9'))
			{
				c_acc = c_acc * 16 + c - ASE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= ASE_T('A') && c <= ASE_T('F'))
			{
				c_acc = c_acc * 16 + c - ASE_T('A') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= ASE_T('a') && c <= ASE_T('f'))
			{
				c_acc = c_acc * 16 + c - ASE_T('a') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				ase_char_t rc;

				rc = (escaped == 2)? ASE_T('x'):
				     (escaped == 4)? ASE_T('u'): ASE_T('U');

				if (digit_count == 0) ADD_TOKEN_CHAR (awk, rc);
				else ADD_TOKEN_CHAR (awk, c_acc);

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
			if (c == ASE_T('n')) c = ASE_T('\n');
			else if (c == ASE_T('r')) c = ASE_T('\r');
			else if (c == ASE_T('t')) c = ASE_T('\t');
			else if (c == ASE_T('f')) c = ASE_T('\f');
			else if (c == ASE_T('b')) c = ASE_T('\b');
			else if (c == ASE_T('v')) c = ASE_T('\v');
			else if (c == ASE_T('a')) c = ASE_T('\a');
			else if (c >= ASE_T('0') && c <= ASE_T('7')) 
			{
				escaped = 3;
				digit_count = 1;
				c_acc = c - ASE_T('0');
				continue;
			}
			else if (c == ASE_T('x')) 
			{
				escaped = 2;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#ifdef ASE_CHAR_IS_WCHAR
			else if (c == ASE_T('u') && ASE_SIZEOF(ase_char_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == ASE_T('U') && ASE_SIZEOF(ase_char_t) >= 4) 
			{
				escaped = 8;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#endif
			else if (keep_esc_char) 
			{
				ADD_TOKEN_CHAR (awk, esc_char);
			}

			escaped = 0;
		}

		ADD_TOKEN_CHAR (awk, c);
	}

	return 0;
}

static int get_symbol (ase_awk_t* awk, ase_cint_t fc)
{
	int i;
	static struct 
	{
		const ase_char_t* s;
		unsigned short l; 
		unsigned short t; 
		int o;
	} tab[] =
	{
		{ ASE_T("=="),  2, TOKEN_EQ,              0 },
		{ ASE_T("="),   1, TOKEN_ASSIGN,          0 },

/*
{ ASE_T("!@#$%"), 6, 1000, 0 },
{ ASE_T("!^"), 2, 1001, 0 },
*/

		{ ASE_T("!="),  2, TOKEN_NE,              0 },
		{ ASE_T("!~"),  2, TOKEN_NM,              0 },
		{ ASE_T("!"),   1, TOKEN_LNOT,            0 },

		{ ASE_T(">>="), 3, TOKEN_RSHIFT_ASSIGN,   ASE_AWK_SHIFT },
		{ ASE_T(">>"),  2, TOKEN_RSHIFT,          0 },
		{ ASE_T(">="),  2, TOKEN_GE,              0 },
		{ ASE_T(">"),   1, TOKEN_GT,              0 },

		{ ASE_T("<<="), 3, TOKEN_LSHIFT_ASSIGN,   ASE_AWK_SHIFT },
		{ ASE_T("<<"),  2, TOKEN_LSHIFT,          ASE_AWK_SHIFT },
		{ ASE_T("<="),  2, TOKEN_LE,              0 },
		{ ASE_T("<"),   1, TOKEN_LT,              0 },

		{ ASE_T("||"),  2, TOKEN_LOR,             0 },
		{ ASE_T("|&"),  2, TOKEN_BORAND,          ASE_AWK_COPROC },
		{ ASE_T("|="),  2, TOKEN_BOR_ASSIGN,      0 },
		{ ASE_T("|"),   1, TOKEN_BOR,             0 },
		
		{ ASE_T("&&"),  2, TOKEN_LAND,            0 },
		{ ASE_T("&="),  2, TOKEN_BAND_ASSIGN,     0 },
		{ ASE_T("&"),   1, TOKEN_BAND,            0 },

		{ ASE_T("^="),  2, TOKEN_BXOR_ASSIGN,     0 },
		{ ASE_T("^"),   1, TOKEN_BXOR,            0 },

		{ ASE_T("++"),  2, TOKEN_PLUSPLUS,        0 },
		{ ASE_T("+="),  2, TOKEN_PLUS_ASSIGN,     0 },
		{ ASE_T("+"),   1, TOKEN_PLUS,            0 },

		{ ASE_T("--"),  2, TOKEN_MINUSMINUS,      0 },
		{ ASE_T("-="),  2, TOKEN_MINUS_ASSIGN,    0 },
		{ ASE_T("-"),   1, TOKEN_MINUS,           0 },

		{ ASE_T("**="), 3, TOKEN_EXP_ASSIGN,      0 },
		{ ASE_T("**"),  2, TOKEN_EXP,             0 },
		{ ASE_T("*="),  2, TOKEN_MUL_ASSIGN,      0 },
		{ ASE_T("*"),   1, TOKEN_MUL,             0 },

		{ ASE_T("//="), 3, TOKEN_IDIV_ASSIGN,     ASE_AWK_IDIV },
		{ ASE_T("//"),  2, TOKEN_IDIV_ASSIGN,     ASE_AWK_IDIV },
		{ ASE_T("/="),  2, TOKEN_DIV_ASSIGN,      0 },
		{ ASE_T("/"),   1, TOKEN_DIV,             0 },

		{ ASE_T("%="),  2, TOKEN_MOD_ASSIGN,      0 },
		{ ASE_T("%"),   1, TOKEN_MOD,             0 },

		{ ASE_T("~"),   1, TOKEN_TILDE,           0 },
		{ ASE_T("("),   1, TOKEN_LPAREN,          0 },
		{ ASE_T(")"),   1, TOKEN_RPAREN,          0 },
		{ ASE_T("{"),   1, TOKEN_LBRACE,          0 },
		{ ASE_T("}"),   1, TOKEN_RBRACE,          0 },
		{ ASE_T("["),   1, TOKEN_LBRACK,          0 },
		{ ASE_T("]"),   1, TOKEN_RBRACK,          0 },
		{ ASE_T("$"),   1, TOKEN_DOLLAR,          0 },
		{ ASE_T(","),   1, TOKEN_COMMA,           0 },
		{ ASE_T(";"),   1, TOKEN_SEMICOLON,       0 },
		{ ASE_T(":"),   1, TOKEN_COLON,           0 },
		{ ASE_T("?"),   1, TOKEN_QUEST,           0 },

		{ ASE_NULL,     0, TOKEN_EOF,             0 }
	};

	/*
	 * INPUT: ABCDEFX
	 *
	 * TOKENS:
	 *	ABCDEFG
	 *	ABX 
	 */
	for (i = 0; tab[i].s != ASE_NULL; )
	{
		const ase_char_t* p = tab[i].s;
		const ase_char_t* e = p + tab[i].l;
		ase_cint_t c = fc;

		ASE_ASSERT (tab[i].l > 0);

		if (c == *p)
		{
			int len, len2;
			const ase_char_t* p2;

		try_again:
//wprintf (L"PROCESSING ENTRY AT %d fc = [%c] tab[i].s = [%S] c = [%c] p = [%S]\n", i, fc, tab[i].s, c, p);
			/* proceed as long as the string matches the stream 
			 * of characters read in */
			while (c == *p)
			{
				if (++p >= e) /* reached the end */
				{
					/* found a matching entry */
					GET_CHAR (awk);
					SET_TOKEN_TYPE (awk, tab[i].t);
					ADD_TOKEN_STR (awk, tab[i].s, tab[i].l);
					return 1;
				}

				GET_CHAR_TO (awk, c);
			} 

			len = p - tab[i++].s /*+ 1*/;

			if (fc != tab[i].s[0]) break;
			len2 = tab[i].l;
			ASE_ASSERT (len2 > 0);

			/* unget as many characters as the length 
			 * difference */
//wprintf (L"UNGET STAGE 1 len=%d len2=%d\n", len, len2);
			UNGET_CHAR (awk, c);
			while (--len > len2) UNGET_CHAR (awk, *--p); 

			/* unget different characters from the back */
//wprintf (L"UNGET STAGE 2 len=[%d], len2=[%d]\n", len, len2);
			p2 = &tab[i].s[len2-1];
			while (len > 0 && *p != *p2) 
			{
				UNGET_CHAR (awk, *p); 
				p--; p2--; len--;
			}
//wprintf (L"UNGET OVER\n");
			/* restore the current character */
			GET_CHAR_TO (awk, c);

			p = p2;
			goto try_again;
		}
		i++;
	}

	/* not found */
	return 0;
}

static int get_char (ase_awk_t* awk)
{
	ase_ssize_t n;
	/*ase_char_t c;*/

	if (awk->src.lex.ungotc_count > 0) 
	{
		awk->src.lex.curc = awk->src.lex.ungotc[--awk->src.lex.ungotc_count];
		awk->src.lex.line = awk->src.lex.ungotc_line[awk->src.lex.ungotc_count];
		awk->src.lex.column = awk->src.lex.ungotc_column[awk->src.lex.ungotc_count];
		return 0;
	}

	if (awk->src.shared.buf_pos >= awk->src.shared.buf_len)
	{
		CLRERR (awk);
		n = awk->src.ios.in (
			ASE_AWK_IO_READ, awk->src.ios.custom_data,
			awk->src.shared.buf, ASE_COUNTOF(awk->src.shared.buf));
		if (n <= -1)
		{
			if (ISNOERR(awk)) SETERR (awk, ASE_AWK_ESINRD);
			return -1;
		}

		if (n == 0) 
		{
			awk->src.lex.curc = ASE_CHAR_EOF;
			return 0;
		}

		awk->src.shared.buf_pos = 0;
		awk->src.shared.buf_len = n;	
	}

	awk->src.lex.curc = awk->src.shared.buf[awk->src.shared.buf_pos++];

	if (awk->src.lex.curc == ASE_T('\n'))
	{
		awk->src.lex.line++;
		awk->src.lex.column = 1;
	}
	else awk->src.lex.column++;

	return 0;
}

static int unget_char (ase_awk_t* awk, ase_cint_t c)
{
	if (awk->src.lex.ungotc_count >= ASE_COUNTOF(awk->src.lex.ungotc)) 
	{
		SETERRLIN (awk, ASE_AWK_ELXUNG, awk->src.lex.line);
		return -1;
	}

	awk->src.lex.ungotc_line[awk->src.lex.ungotc_count] = awk->src.lex.line;
	awk->src.lex.ungotc_column[awk->src.lex.ungotc_count] = awk->src.lex.column;
	awk->src.lex.ungotc[awk->src.lex.ungotc_count++] = c;
	return 0;
}

static int skip_spaces (ase_awk_t* awk)
{
	ase_cint_t c = awk->src.lex.curc;

	if (awk->option & ASE_AWK_NEWLINE)
	{
		do 
		{
			while (c != ASE_T('\n') && ASE_AWK_ISSPACE(awk,c))
				GET_CHAR_TO (awk, c);

			if  (c == ASE_T('\\'))
			{
				int cr = 0;
				GET_CHAR_TO (awk, c);
				if (c == ASE_T('\r')) 
				{
					cr = 1;
					GET_CHAR_TO (awk, c);
				}
				if (c == ASE_T('\n'))
				{
					GET_CHAR_TO (awk, c);
					continue;
				}
				else
				{
					UNGET_CHAR (awk, c);	
					if (cr) UNGET_CHAR(awk, ASE_T('\r'));
				}
			}

			break;
		}
		while (1);
	}
	else
	{
		while (ASE_AWK_ISSPACE (awk, c)) GET_CHAR_TO (awk, c);
	}

	return 0;
}

static int skip_comment (ase_awk_t* awk)
{
	ase_cint_t c = awk->src.lex.curc;
	ase_size_t line, column;

	if (c == ASE_T('#'))
	{
		do 
		{ 
			GET_CHAR_TO (awk, c);
		} 
		while (c != ASE_T('\n') && c != ASE_CHAR_EOF);

		if (!(awk->option & ASE_AWK_NEWLINE)) GET_CHAR (awk);
		return 1; /* comment by # */
	}

	if (c != ASE_T('/')) return 0; /* not a comment */

	line = awk->src.lex.line;
	column = awk->src.lex.column;
	GET_CHAR_TO (awk, c);

	if (c == ASE_T('*')) 
	{
		do 
		{
			GET_CHAR_TO (awk, c);
			if (c == ASE_CHAR_EOF)
			{
				SETERRLIN (awk, ASE_AWK_EENDCMT, awk->src.lex.line);
				return -1;
			}

			if (c == ASE_T('*')) 
			{
				GET_CHAR_TO (awk, c);
				if (c == ASE_CHAR_EOF)
				{
					SETERRLIN (awk, ASE_AWK_EENDCMT, awk->src.lex.line);
					return -1;
				}

				if (c == ASE_T('/')) 
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

	UNGET_CHAR (awk, c);
	awk->src.lex.curc = ASE_T('/');
	awk->src.lex.line = line;
	awk->src.lex.column = column;

	return 0;
}

static int classify_ident (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len)
{
	kwent_t* kwp;
	ase_pair_t* pair;

	for (kwp = kwtab; kwp->name != ASE_NULL; kwp++) 
	{
		const ase_char_t* k;
		ase_size_t l;

		if (kwp->valid != 0 && 
		    (awk->option & kwp->valid) != kwp->valid) continue;

		pair = ase_map_get (awk->wtab, kwp->name, kwp->name_len);
		if (pair != ASE_NULL)
		{
			k = ((ase_cstr_t*)(pair->val))->ptr;
			l = ((ase_cstr_t*)(pair->val))->len;
		}
		else
		{
			k = kwp->name;
			l = kwp->name_len;
		}

		if (ase_strxncmp (k, l, name, len) == 0) 
		{

			return kwp->type;
		}
	}

	return TOKEN_IDENT;
}

static int assign_to_opcode (ase_awk_t* awk)
{
	/* synchronize it with ase_awk_assop_type_t in run.h */
	static int assop[] =
	{
		ASE_AWK_ASSOP_NONE,
		ASE_AWK_ASSOP_PLUS,
		ASE_AWK_ASSOP_MINUS,
		ASE_AWK_ASSOP_MUL,
		ASE_AWK_ASSOP_DIV,
		ASE_AWK_ASSOP_IDIV,
		ASE_AWK_ASSOP_MOD,
		ASE_AWK_ASSOP_EXP,
		ASE_AWK_ASSOP_RSHIFT,
		ASE_AWK_ASSOP_LSHIFT,
		ASE_AWK_ASSOP_BAND,
		ASE_AWK_ASSOP_BXOR,
		ASE_AWK_ASSOP_BOR
	};

	if (awk->token.type >= TOKEN_ASSIGN &&
	    awk->token.type <= TOKEN_BOR_ASSIGN)
	{
		return assop[awk->token.type - TOKEN_ASSIGN];
	}

	return -1;
}

static int is_plain_var (ase_awk_nde_t* nde)
{
	return nde->type == ASE_AWK_NDE_GLOBAL ||
	       nde->type == ASE_AWK_NDE_LOCAL ||
	       nde->type == ASE_AWK_NDE_ARG ||
	       nde->type == ASE_AWK_NDE_NAMED;
}

static int is_var (ase_awk_nde_t* nde)
{
	return nde->type == ASE_AWK_NDE_GLOBAL ||
	       nde->type == ASE_AWK_NDE_LOCAL ||
	       nde->type == ASE_AWK_NDE_ARG ||
	       nde->type == ASE_AWK_NDE_NAMED ||
	       nde->type == ASE_AWK_NDE_GLOBALIDX ||
	       nde->type == ASE_AWK_NDE_LOCALIDX ||
	       nde->type == ASE_AWK_NDE_ARGIDX ||
	       nde->type == ASE_AWK_NDE_NAMEDIDX;
}

struct deparse_func_t 
{
	ase_awk_t* awk;
	ase_char_t* tmp;
	ase_size_t tmp_len;
};

static int deparse (ase_awk_t* awk)
{
	ase_awk_nde_t* nde;
	ase_awk_chain_t* chain;
	ase_char_t tmp[ASE_SIZEOF(ase_size_t)*8 + 32];
	struct deparse_func_t df;
	int n = 0; 
	ase_ssize_t op;

	ASE_ASSERT (awk->src.ios.out != ASE_NULL);

	awk->src.shared.buf_len = 0;
	awk->src.shared.buf_pos = 0;

	CLRERR (awk);
	op = awk->src.ios.out (
		ASE_AWK_IO_OPEN, awk->src.ios.custom_data, ASE_NULL, 0);
	if (op <= -1)
	{
		if (ISNOERR(awk)) SETERR (awk, ASE_AWK_ESOUTOP);
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
		 *    1. set awk->src.ios.out to NULL.
		 *    2. set awk->src.ios.out to a normal handler but
		 *       make it return 0 on the OPEN request.
		 */
		n = 0;
		goto exit_deparse;
	}

#define EXIT_DEPARSE() do { n = -1; goto exit_deparse; } while(0)

	if (awk->tree.nglobals > awk->tree.nbglobals) 
	{
		ase_size_t i, len;

		ASE_ASSERT (awk->tree.nglobals > 0);
		if (ase_awk_putsrcstr(awk,ase_awk_getkw(awk,ASE_T("global"))) == -1)
		{
			EXIT_DEPARSE ();
		}
		if (ase_awk_putsrcstr (awk, ASE_T(" ")) == -1)
		{
			EXIT_DEPARSE ();
		}

		for (i = awk->tree.nbglobals; i < awk->tree.nglobals - 1; i++) 
		{
			if ((awk->option & ASE_AWK_EXPLICIT) && 
			    !(awk->option & ASE_AWK_IMPLICIT))
			{
				/* use the actual name if no named variable 
				 * is allowed */
				if (ase_awk_putsrcstrx (awk, 
					awk->parse.globals.buf[i].name.ptr, 
					awk->parse.globals.buf[i].name.len) == -1)
				{
					EXIT_DEPARSE ();
				}
			}
			else
			{
				len = ase_awk_longtostr ((ase_long_t)i, 
					10, ASE_T("__g"), tmp, ASE_COUNTOF(tmp));
				ASE_ASSERT (len != (ase_size_t)-1);
				if (ase_awk_putsrcstrx (awk, tmp, len) == -1)
				{
					EXIT_DEPARSE ();
				}
			}

			if (ase_awk_putsrcstr (awk, ASE_T(", ")) == -1)
				EXIT_DEPARSE ();
		}

		if ((awk->option & ASE_AWK_EXPLICIT) && 
		    !(awk->option & ASE_AWK_IMPLICIT))
		{
			if (ase_awk_putsrcstrx (awk, 
				awk->parse.globals.buf[i].name.ptr, 
				awk->parse.globals.buf[i].name.len) == -1)
			{
				EXIT_DEPARSE ();
			}
		}
		else
		{
			len = ase_awk_longtostr ((ase_long_t)i, 
				10, ASE_T("__g"), tmp, ASE_COUNTOF(tmp));
			ASE_ASSERT (len != (ase_size_t)-1);
			if (ase_awk_putsrcstrx (awk, tmp, len) == -1)
			{
				EXIT_DEPARSE ();
			}
		}

		if (awk->option & ASE_AWK_CRLF)
		{
			if (ase_awk_putsrcstr (awk, ASE_T(";\r\n\r\n")) == -1)
			{
				EXIT_DEPARSE ();
			}
		}
		else
		{
			if (ase_awk_putsrcstr (awk, ASE_T(";\n\n")) == -1)
			{
				EXIT_DEPARSE ();
			}
		}
	}

	df.awk = awk;
	df.tmp = tmp;
	df.tmp_len = ASE_COUNTOF(tmp);

	if (ase_map_walk (awk->tree.afns, deparse_func, &df) == -1) 
	{
		EXIT_DEPARSE ();
	}

	for (nde = awk->tree.begin; nde != ASE_NULL; nde = nde->next)
	{
		const ase_char_t* kw = ase_awk_getkw(awk,ASE_T("BEGIN"));
		if (ase_awk_putsrcstr(awk,kw) == -1) EXIT_DEPARSE ();
		if (ase_awk_putsrcstr (awk, ASE_T(" ")) == -1) EXIT_DEPARSE ();
		if (ase_awk_prnnde (awk, nde) == -1) EXIT_DEPARSE ();

		if (awk->option & ASE_AWK_CRLF)
		{
			if (put_char (awk, ASE_T('\r')) == -1) EXIT_DEPARSE ();
		}

		if (put_char (awk, ASE_T('\n')) == -1) EXIT_DEPARSE ();
	}

	chain = awk->tree.chain;
	while (chain != ASE_NULL) 
	{
		if (chain->pattern != ASE_NULL) 
		{
			if (ase_awk_prnptnpt (awk, chain->pattern) == -1)
				EXIT_DEPARSE ();
		}

		if (chain->action == ASE_NULL) 
		{
			/* blockless pattern */
			if (awk->option & ASE_AWK_CRLF)
			{
				if (put_char (awk, ASE_T('\r')) == -1)
					EXIT_DEPARSE ();
			}

			if (put_char (awk, ASE_T('\n')) == -1)
				EXIT_DEPARSE ();
		}
		else 
		{
			if (chain->pattern != ASE_NULL)
			{
				if (put_char (awk, ASE_T(' ')) == -1)
					EXIT_DEPARSE ();
			}
			if (ase_awk_prnpt (awk, chain->action) == -1)
				EXIT_DEPARSE ();
		}

		if (awk->option & ASE_AWK_CRLF)
		{
			if (put_char (awk, ASE_T('\r')) == -1)
				EXIT_DEPARSE ();
		}

		if (put_char (awk, ASE_T('\n')) == -1)
			EXIT_DEPARSE ();

		chain = chain->next;	
	}

	for (nde = awk->tree.end; nde != ASE_NULL; nde = nde->next)
	{
		const ase_char_t* kw = ase_awk_getkw(awk,ASE_T("END"));
		if (ase_awk_putsrcstr(awk,kw) == -1) EXIT_DEPARSE ();
		if (ase_awk_putsrcstr (awk, ASE_T(" ")) == -1) EXIT_DEPARSE ();
		if (ase_awk_prnnde (awk, nde) == -1) EXIT_DEPARSE ();
		
		/*
		if (awk->option & ASE_AWK_CRLF)
		{
			if (put_char (awk, ASE_T('\r')) == -1) EXIT_DEPARSE ();
		}

		if (put_char (awk, ASE_T('\n')) == -1) EXIT_DEPARSE ();
		*/
	}

	if (flush_out (awk) == -1) EXIT_DEPARSE ();

exit_deparse:
	if (n == 0) CLRERR (awk);
	if (awk->src.ios.out (
		ASE_AWK_IO_CLOSE, awk->src.ios.custom_data, ASE_NULL, 0) != 0)
	{
		if (n == 0)
		{
			if (ISNOERR(awk)) SETERR (awk, ASE_AWK_ESOUTCL);
			n = -1;
		}
	}

	return n;
}

static int deparse_func (ase_pair_t* pair, void* arg)
{
	struct deparse_func_t* df = (struct deparse_func_t*)arg;
	ase_awk_afn_t* afn = (ase_awk_afn_t*)pair->val;
	ase_size_t i, n;

	ASE_ASSERT (ase_strxncmp (ASE_PAIR_KEYPTR(pair), ASE_PAIR_KEYLEN(pair), afn->name, afn->name_len) == 0);

	if (ase_awk_putsrcstr(df->awk,ase_awk_getkw(df->awk,ASE_T("function"))) == -1) return -1;
	if (ase_awk_putsrcstr (df->awk, ASE_T(" ")) == -1) return -1;
	if (ase_awk_putsrcstr (df->awk, afn->name) == -1) return -1;
	if (ase_awk_putsrcstr (df->awk, ASE_T(" (")) == -1) return -1;

	for (i = 0; i < afn->nargs; ) 
	{
		n = ase_awk_longtostr (i++, 10, 
			ASE_T("__p"), df->tmp, df->tmp_len);
		ASE_ASSERT (n != (ase_size_t)-1);
		if (ase_awk_putsrcstrx (df->awk, df->tmp, n) == -1) return -1;
		if (i >= afn->nargs) break;
		if (ase_awk_putsrcstr (df->awk, ASE_T(", ")) == -1) return -1;
	}

	if (ase_awk_putsrcstr (df->awk, ASE_T(")")) == -1) return -1;
	if (df->awk->option & ASE_AWK_CRLF)
	{
		if (put_char (df->awk, ASE_T('\r')) == -1) return -1;
	}
	if (put_char (df->awk, ASE_T('\n')) == -1) return -1;

	if (ase_awk_prnpt (df->awk, afn->body) == -1) return -1;
	if (df->awk->option & ASE_AWK_CRLF)
	{
		if (put_char (df->awk, ASE_T('\r')) == -1) return -1;
	}
	if (put_char (df->awk, ASE_T('\n')) == -1) return -1;

	return 0;
}

static int put_char (ase_awk_t* awk, ase_char_t c)
{
	awk->src.shared.buf[awk->src.shared.buf_len++] = c;
	if (awk->src.shared.buf_len >= ASE_COUNTOF(awk->src.shared.buf))
	{
		if (flush_out (awk) == -1) return -1;
	}
	return 0;
}

static int flush_out (ase_awk_t* awk)
{
	ase_ssize_t n;

	ASE_ASSERT (awk->src.ios.out != ASE_NULL);

	while (awk->src.shared.buf_pos < awk->src.shared.buf_len)
	{
		CLRERR (awk);

		n = awk->src.ios.out (
			ASE_AWK_IO_WRITE, awk->src.ios.custom_data,
			&awk->src.shared.buf[awk->src.shared.buf_pos], 
			awk->src.shared.buf_len - awk->src.shared.buf_pos);
		if (n <= 0) 
		{
			if (ISNOERR(awk)) SETERR (awk, ASE_AWK_ESOUTWR);
			return -1;
		}

		awk->src.shared.buf_pos += n;
	}

	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;
	return 0;
}

int ase_awk_putsrcstr (ase_awk_t* awk, const ase_char_t* str)
{
	while (*str != ASE_T('\0'))
	{
		if (put_char (awk, *str) == -1) return -1;
		str++;
	}

	return 0;
}

int ase_awk_putsrcstrx (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len)
{
	const ase_char_t* end = str + len;

	while (str < end)
	{
		if (put_char (awk, *str) == -1) return -1;
		str++;
	}

	return 0;
}

