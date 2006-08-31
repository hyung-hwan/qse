/*
 * $Id: parse.c,v 1.172 2006-08-31 13:56:46 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/ctype.h>
#include <xp/bas/string.h>
#include <xp/bas/stdlib.h>
#include <xp/bas/assert.h>
#include <xp/bas/stdio.h>
#endif

enum
{
	TOKEN_EOF,
	TOKEN_NEWLINE,

	/* TOKEN_XXX_ASSIGNs should in sync 
	 * with __assop in __assign_to_opcode */
	TOKEN_ASSIGN,
	TOKEN_PLUS_ASSIGN,
	TOKEN_MINUS_ASSIGN,
	TOKEN_MUL_ASSIGN,
	TOKEN_DIV_ASSIGN,
	TOKEN_MOD_ASSIGN,
	TOKEN_EXP_ASSIGN,

	TOKEN_EQ,
	TOKEN_NE,
	TOKEN_LE,
	TOKEN_LT,
	TOKEN_GE,
	TOKEN_GT,
	TOKEN_NM,   /* not match */
	TOKEN_NOT,
	TOKEN_PLUS,
	TOKEN_PLUSPLUS,
	TOKEN_MINUS,
	TOKEN_MINUSMINUS,
	TOKEN_MUL,
	TOKEN_DIV,
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
	TOKEN_DELETE,
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

typedef struct __binmap_t __binmap_t;

struct __binmap_t
{
	int token;
	int binop;
};

static xp_awk_t* __parse_progunit (xp_awk_t* awk);
static xp_awk_t* __collect_globals (xp_awk_t* awk);
static xp_awk_t* __add_builtin_globals (xp_awk_t* awk);
static xp_awk_t* __add_global (
	xp_awk_t* awk, const xp_char_t* name, xp_size_t name_len);
static xp_awk_t* __collect_locals (xp_awk_t* awk, xp_size_t nlocals);

static xp_awk_nde_t* __parse_function (xp_awk_t* awk);
static xp_awk_nde_t* __parse_begin (xp_awk_t* awk);
static xp_awk_nde_t* __parse_end (xp_awk_t* awk);
static xp_awk_chain_t* __parse_pattern_block (
	xp_awk_t* awk, xp_awk_nde_t* ptn, xp_bool_t blockless);

static xp_awk_nde_t* __parse_block (xp_awk_t* awk, xp_bool_t is_top);
static xp_awk_nde_t* __parse_statement (xp_awk_t* awk);
static xp_awk_nde_t* __parse_statement_nb (xp_awk_t* awk);
static xp_awk_nde_t* __parse_expression (xp_awk_t* awk);

static xp_awk_nde_t* __parse_basic_expr (xp_awk_t* awk);

static xp_awk_nde_t* __parse_binary_expr (
	xp_awk_t* awk, const __binmap_t* binmap,
	xp_awk_nde_t*(*next_level_func)(xp_awk_t*));

static xp_awk_nde_t* __parse_logical_or (xp_awk_t* awk);
static xp_awk_nde_t* __parse_logical_and (xp_awk_t* awk);
static xp_awk_nde_t* __parse_in (xp_awk_t* awk);
static xp_awk_nde_t* __parse_regex_match (xp_awk_t* awk);
static xp_awk_nde_t* __parse_bitwise_or (xp_awk_t* awk);
static xp_awk_nde_t* __parse_bitwise_or_with_extio (xp_awk_t* awk);
static xp_awk_nde_t* __parse_bitwise_xor (xp_awk_t* awk);
static xp_awk_nde_t* __parse_bitwise_and (xp_awk_t* awk);
static xp_awk_nde_t* __parse_equality (xp_awk_t* awk);
static xp_awk_nde_t* __parse_relational (xp_awk_t* awk);
static xp_awk_nde_t* __parse_shift (xp_awk_t* awk);
static xp_awk_nde_t* __parse_concat (xp_awk_t* awk);
static xp_awk_nde_t* __parse_additive (xp_awk_t* awk);
static xp_awk_nde_t* __parse_multiplicative (xp_awk_t* awk);

static xp_awk_nde_t* __parse_unary (xp_awk_t* awk);
static xp_awk_nde_t* __parse_increment (xp_awk_t* awk);
static xp_awk_nde_t* __parse_primary (xp_awk_t* awk);
static xp_awk_nde_t* __parse_primary_ident (xp_awk_t* awk);

static xp_awk_nde_t* __parse_hashidx (
	xp_awk_t* awk, xp_char_t* name, xp_size_t name_len);
static xp_awk_nde_t* __parse_fncall (
	xp_awk_t* awk, xp_char_t* name, xp_size_t name_len, xp_awk_bfn_t* bfn);
static xp_awk_nde_t* __parse_if (xp_awk_t* awk);
static xp_awk_nde_t* __parse_while (xp_awk_t* awk);
static xp_awk_nde_t* __parse_for (xp_awk_t* awk);
static xp_awk_nde_t* __parse_dowhile (xp_awk_t* awk);
static xp_awk_nde_t* __parse_break (xp_awk_t* awk);
static xp_awk_nde_t* __parse_continue (xp_awk_t* awk);
static xp_awk_nde_t* __parse_return (xp_awk_t* awk);
static xp_awk_nde_t* __parse_exit (xp_awk_t* awk);
static xp_awk_nde_t* __parse_next (xp_awk_t* awk);
static xp_awk_nde_t* __parse_nextfile (xp_awk_t* awk);
static xp_awk_nde_t* __parse_delete (xp_awk_t* awk);
static xp_awk_nde_t* __parse_print (xp_awk_t* awk);
static xp_awk_nde_t* __parse_printf (xp_awk_t* awk);

static int __get_token (xp_awk_t* awk);
static int __get_number (xp_awk_t* awk);
static int __get_charstr (xp_awk_t* awk);
static int __get_rexstr (xp_awk_t* awk);
static int __get_string (
	xp_awk_t* awk, xp_char_t end_char,
	xp_char_t esc_char, xp_bool_t keep_esc_char);
static int __get_char (xp_awk_t* awk);
static int __unget_char (xp_awk_t* awk, xp_cint_t c);
static int __skip_spaces (xp_awk_t* awk);
static int __skip_comment (xp_awk_t* awk);
static int __classify_ident (
	xp_awk_t* awk, const xp_char_t* name, xp_size_t len);
static int __assign_to_opcode (xp_awk_t* awk);
static int __is_plain_var (xp_awk_nde_t* nde);
static int __is_var (xp_awk_nde_t* nde);

static int __deparse (xp_awk_t* awk);
static int __deparse_func (xp_awk_pair_t* pair, void* arg);
static int __put_char (xp_awk_t* awk, xp_char_t c);
static int __flush (xp_awk_t* awk);

struct __kwent 
{ 
	const xp_char_t* name; 
	xp_size_t name_len;
	int type; 
	int valid; /* the entry is valid when this option is set */
};

static struct __kwent __kwtab[] = 
{
	/* operators */
	{ XP_T("in"),       2, TOKEN_IN,       0 },

	/* top-level block starters */
	{ XP_T("BEGIN"),    5, TOKEN_BEGIN,    0 },
	{ XP_T("END"),      3, TOKEN_END,      0 },
	{ XP_T("function"), 8, TOKEN_FUNCTION, 0 },
	{ XP_T("func"),     4, TOKEN_FUNCTION, 0 },

	/* keywords for variable declaration */
	{ XP_T("local"),    5, TOKEN_LOCAL,    XP_AWK_EXPLICIT },
	{ XP_T("global"),   6, TOKEN_GLOBAL,   XP_AWK_EXPLICIT },

	/* keywords that start statements excluding expression statements */
	{ XP_T("if"),       2, TOKEN_IF,       0 },
	{ XP_T("else"),     4, TOKEN_ELSE,     0 },
	{ XP_T("while"),    5, TOKEN_WHILE,    0 },
	{ XP_T("for"),      3, TOKEN_FOR,      0 },
	{ XP_T("do"),       2, TOKEN_DO,       0 },
	{ XP_T("break"),    5, TOKEN_BREAK,    0 },
	{ XP_T("continue"), 8, TOKEN_CONTINUE, 0 },
	{ XP_T("return"),   6, TOKEN_RETURN,   0 },
	{ XP_T("exit"),     4, TOKEN_EXIT,     0 },
	{ XP_T("next"),     4, TOKEN_NEXT,     0 },
	{ XP_T("nextfile"), 8, TOKEN_NEXTFILE, 0 },
	{ XP_T("delete"),   6, TOKEN_DELETE,   0 },
	{ XP_T("print"),    5, TOKEN_PRINT,    XP_AWK_EXTIO },
	{ XP_T("printf"),   6, TOKEN_PRINTF,   XP_AWK_EXTIO },

	/* keywords that can start an expression */
	{ XP_T("getline"),  7, TOKEN_GETLINE,  XP_AWK_EXTIO },

	{ XP_NULL,          0,              0 }
};

struct __bvent
{
	const xp_char_t* name;
	xp_size_t name_len;
	int valid;
};

static struct __bvent __bvtab[] =
{
	{ XP_T("ARGC"),         4, 0 },
	{ XP_T("ARGIND"),       6, 0 },
	{ XP_T("ARGV"),         4, 0 },
	{ XP_T("CONVFMT"),      7, 0 },
	{ XP_T("FIELDWIDTHS"), 11, 0 },
	{ XP_T("ENVIRON"),      7, 0 },
	{ XP_T("ERRNO"),        5, 0 },
	{ XP_T("FILENAME"),     8, 0 },
	{ XP_T("FNR"),          3, 0 },
	{ XP_T("FS"),           2, 0 },
	{ XP_T("IGNORECASE"),  10, 0 },
	{ XP_T("NF"),           2, 0 },
	{ XP_T("NR"),           2, 0 },
	{ XP_T("OFMT"),         4, 0 },
	{ XP_T("OFS"),          3, 0 },
	{ XP_T("ORS"),          3, 0 },
	{ XP_T("RS"),           2, 0 },
	{ XP_T("RT"),           2, 0 },
	{ XP_T("RSTART"),       6, 0 },
	{ XP_T("RLENGTH"),      7, 0 },
	{ XP_T("SUBSEP"),       6, 0 },
	{ XP_NULL,              0, 0 }
};

#define GET_CHAR(awk) \
	do { if (__get_char(awk) == -1) return -1; } while(0)

#define GET_CHAR_TO(awk,c) \
	do { \
		if (__get_char(awk) == -1) return -1; \
		c = (awk)->src.lex.curc; \
	} while(0)

#define SET_TOKEN_TYPE(awk,code) \
	do { \
		(awk)->token.prev = (awk)->token.type; \
		(awk)->token.type = (code); \
	} while (0);

#define ADD_TOKEN_CHAR(awk,c) \
	do { \
		if (xp_str_ccat(&(awk)->token.name,(c)) == (xp_size_t)-1) { \
			(awk)->errnum = XP_AWK_ENOMEM; return -1; \
		} \
	} while (0)

#define ADD_TOKEN_STR(awk,str) \
	do { \
		if (xp_str_cat(&(awk)->token.name,(str)) == (xp_size_t)-1) { \
			(awk)->errnum = XP_AWK_ENOMEM; return -1; \
		} \
	} while (0)

#define MATCH(awk,token_type) ((awk)->token.type == (token_type))

#define PANIC(awk,code) \
	do { (awk)->errnum = (code); return XP_NULL; } while (0)

#define MALLOC(awk,size) \
	(awk)->syscas->malloc (size, (awk)->syscas->custom_data)
#define FREE(awk,ptr) \
	(awk)->syscas->free (ptr, (awk)->syscas->custom_data)

int xp_awk_parse (xp_awk_t* awk, xp_awk_srcios_t* srcios)
{
	int n = 0;

	xp_assert (srcios != XP_NULL && srcios->in != XP_NULL);

	xp_awk_clear (awk);
	awk->src.ios = srcios;

	if (awk->src.ios->in (
		XP_AWK_IO_OPEN, awk->src.ios->custom_data, XP_NULL, 0) == -1)
	{
		awk->errnum = XP_AWK_ESRCINOPEN;
		return -1;
	}

	if (__add_builtin_globals (awk) == XP_NULL) 
	{
		n = -1;
		goto exit_parse;
	}

	/* get the first character */
	if (__get_char(awk) == -1) 
	{
		n = -1;
		goto exit_parse;
	}

	/* get the first token */
	if (__get_token(awk) == -1) 
	{
		n = -1;
		goto exit_parse;
	}

	while (1) 
	{
		if (MATCH(awk,TOKEN_EOF)) break;
		if (MATCH(awk,TOKEN_NEWLINE)) continue;

		if (__parse_progunit (awk) == XP_NULL) 
		{
			n = -1;
			goto exit_parse;
		}
	}

	awk->tree.nglobals = xp_awk_tab_getsize(&awk->parse.globals);

	if (awk->src.ios->out != XP_NULL) 
	{
		if (__deparse (awk) == -1) 
		{
			n = -1;
			goto exit_parse;
		}
	}

exit_parse:
	if (awk->src.ios->in (
		XP_AWK_IO_CLOSE, awk->src.ios->custom_data, XP_NULL, 0) == -1)
	{
		if (n == 0)
		{
			/* this is to keep the earlier error above
			 * that might be more critical than this */
			awk->errnum = XP_AWK_ESRCINCLOSE;
			n = -1;
		}
	}

	if (n == -1) xp_awk_clear (awk);
	return n;
}

static xp_awk_t* __parse_progunit (xp_awk_t* awk)
{
	/*
	pattern { action }
	function name (parameter-list) { statement }
	*/

	xp_assert (awk->parse.depth.loop == 0);

	if ((awk->option & XP_AWK_EXPLICIT) && MATCH(awk,TOKEN_GLOBAL)) 
	{
		xp_size_t nglobals;

		awk->parse.id.block = PARSE_GLOBAL;

		if (__get_token(awk) == -1) return XP_NULL;

		nglobals = xp_awk_tab_getsize(&awk->parse.globals);
		if (__collect_globals (awk) == XP_NULL) 
		{
			xp_awk_tab_remove (
				&awk->parse.globals, nglobals, 
				xp_awk_tab_getsize(&awk->parse.globals) - nglobals);
			return XP_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_FUNCTION)) 
	{
		awk->parse.id.block = PARSE_FUNCTION;
		if (__parse_function (awk) == XP_NULL) return XP_NULL;
	}
	else if (MATCH(awk,TOKEN_BEGIN)) 
	{
		awk->parse.id.block = PARSE_BEGIN;
		if (__get_token(awk) == -1) return XP_NULL; 

		if ((awk->option & XP_AWK_BLOCKLESS) &&
		    (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF)))
		{
			/* when the blockless pattern is supported
	   		 * BEGIN and { should be located on the same line */
			PANIC (awk, XP_AWK_EBEGINBLOCK);
		}

		if (!MATCH(awk,TOKEN_LBRACE)) PANIC (awk, XP_AWK_ELBRACE);

		awk->parse.id.block = PARSE_BEGIN_BLOCK;
		if (__parse_begin (awk) == XP_NULL) return XP_NULL;
	}
	else if (MATCH(awk,TOKEN_END)) 
	{
		awk->parse.id.block = PARSE_END;
		if (__get_token(awk) == -1) return XP_NULL; 

		if ((awk->option & XP_AWK_BLOCKLESS) &&
		    (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF)))
		{
			/* when the blockless pattern is supported
	   		 * END and { should be located on the same line */
			PANIC (awk, XP_AWK_EENDBLOCK);
		}

		if (!MATCH(awk,TOKEN_LBRACE)) PANIC (awk, XP_AWK_ELBRACE);

		awk->parse.id.block = PARSE_END_BLOCK;
		if (__parse_end (awk) == XP_NULL) return XP_NULL;
	}
	else if (MATCH(awk,TOKEN_LBRACE))
	{
		/* patternless block */
		awk->parse.id.block = PARSE_ACTION_BLOCK;
		if (__parse_pattern_block (
			awk, XP_NULL, xp_false) == XP_NULL) return XP_NULL;
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
		xp_awk_nde_t* ptn;

		awk->parse.id.block = PARSE_PATTERN;

		ptn = __parse_expression (awk);
		if (ptn == XP_NULL) return XP_NULL;

		xp_assert (ptn->next == XP_NULL);

		if (MATCH(awk,TOKEN_COMMA))
		{
			if (__get_token (awk) == -1) 
			{
				xp_awk_clrpt (ptn);
				return XP_NULL;
			}	

			ptn->next = __parse_expression (awk);
			if (ptn->next == XP_NULL) 
			{
				xp_awk_clrpt (ptn);
				return XP_NULL;
			}
		}

		if ((awk->option & XP_AWK_BLOCKLESS) &&
		    (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF)))
		{
			/* blockless pattern */
			xp_bool_t newline = MATCH(awk,TOKEN_NEWLINE);

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (__parse_pattern_block (
				awk, ptn, xp_true) == XP_NULL) 
			{
				xp_awk_clrpt (ptn);
				return XP_NULL;	
			}

			if (newline)
			{
				if (__get_token (awk) == -1) 
				{
					xp_awk_clrpt (ptn);
					return XP_NULL;
				}	
			}
		}
		else
		{
			/* parse the action block */
			if (!MATCH(awk,TOKEN_LBRACE))
			{
				xp_awk_clrpt (ptn);
				PANIC (awk, XP_AWK_ELBRACE);
			}

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (__parse_pattern_block (
				awk, ptn, xp_false) == XP_NULL) 
			{
				xp_awk_clrpt (ptn);
				return XP_NULL;	
			}
		}
	}

	return awk;
}

static xp_awk_nde_t* __parse_function (xp_awk_t* awk)
{
	xp_char_t* name;
	xp_char_t* name_dup;
	xp_size_t name_len;
	xp_awk_nde_t* body;
	xp_awk_afn_t* afn;
	xp_size_t nargs;
	xp_awk_pair_t* pair;
	int n;

	/* eat up the keyword 'function' and get the next token */
	xp_assert (MATCH(awk,TOKEN_FUNCTION));
	if (__get_token(awk) == -1) return XP_NULL;  

	/* match a function name */
	if (!MATCH(awk,TOKEN_IDENT)) 
	{
		/* cannot find a valid identifier for a function name */
		PANIC (awk, XP_AWK_EIDENT);
	}

	name = XP_STR_BUF(&awk->token.name);
	name_len = XP_STR_LEN(&awk->token.name);
	if (xp_awk_map_get(&awk->tree.afns, name, name_len) != XP_NULL) 
	{
		/* the function is defined previously */
		PANIC (awk, XP_AWK_EDUPFUNC);
	}

	if (awk->option & XP_AWK_UNIQUE) 
	{
		/* check if it coincides to be a global variable name */
		if (xp_awk_tab_find (
			&awk->parse.globals, 0, name, name_len) != (xp_size_t)-1) 
		{
			PANIC (awk, XP_AWK_EDUPNAME);
		}
	}

	/* clone the function name before it is overwritten */
	name_dup = xp_strxdup (name, name_len);
	if (name_dup == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

	/* get the next token */
	if (__get_token(awk) == -1) 
	{
		FREE (awk, name_dup);
		return XP_NULL;  
	}

	/* match a left parenthesis */
	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		/* a function name is not followed by a left parenthesis */
		FREE (awk, name_dup);
		PANIC (awk, XP_AWK_ELPAREN);
	}	

	/* get the next token */
	if (__get_token(awk) == -1) 
	{
		FREE (awk, name_dup);
		return XP_NULL;
	}

	/* make sure that parameter table is empty */
	xp_assert (xp_awk_tab_getsize(&awk->parse.params) == 0);

	/* read parameter list */
	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no function parameter found. get the next token */
		if (__get_token(awk) == -1) 
		{
			FREE (awk, name_dup);
			return XP_NULL;
		}
	}
	else 
	{
		while (1) 
		{
			xp_char_t* param;
			xp_size_t param_len;

			if (!MATCH(awk,TOKEN_IDENT)) 
			{
				FREE (awk, name_dup);
				xp_awk_tab_clear (&awk->parse.params);
				PANIC (awk, XP_AWK_EIDENT);
			}

			param = XP_STR_BUF(&awk->token.name);
			param_len = XP_STR_LEN(&awk->token.name);

			if (awk->option & XP_AWK_UNIQUE) 
			{
				/* check if a parameter conflicts with a function */
				if (xp_strxncmp (name_dup, name_len, param, param_len) == 0 ||
				    xp_awk_map_get (&awk->tree.afns, param, param_len) != XP_NULL) 
				{
					FREE (awk, name_dup);
					xp_awk_tab_clear (&awk->parse.params);
					PANIC (awk, XP_AWK_EDUPNAME);
				}

				/* NOTE: the following is not a conflict
				 *  global x; 
				 *  function f (x) { print x; } 
				 *  x in print x is a parameter
				 */
			}

			/* check if a parameter conflicts with other parameters */
			if (xp_awk_tab_find (
				&awk->parse.params, 
				0, param, param_len) != (xp_size_t)-1) 
			{
				FREE (awk, name_dup);
				xp_awk_tab_clear (&awk->parse.params);
				PANIC (awk, XP_AWK_EDUPPARAM);
			}

			/* push the parameter to the parameter list */
			if (xp_awk_tab_getsize (
				&awk->parse.params) >= XP_AWK_MAX_PARAMS)
			{
				FREE (awk, name_dup);
				xp_awk_tab_clear (&awk->parse.params);
				PANIC (awk, XP_AWK_ETOOMANYPARAMS);
			}

			if (xp_awk_tab_add (
				&awk->parse.params, 
				param, param_len) == (xp_size_t)-1) 
			{
				FREE (awk, name_dup);
				xp_awk_tab_clear (&awk->parse.params);
				PANIC (awk, XP_AWK_ENOMEM);
			}	

			if (__get_token (awk) == -1) 
			{
				FREE (awk, name_dup);
				xp_awk_tab_clear (&awk->parse.params);
				return XP_NULL;
			}	

			if (MATCH(awk,TOKEN_RPAREN)) break;

			if (!MATCH(awk,TOKEN_COMMA)) 
			{
				FREE (awk, name_dup);
				xp_awk_tab_clear (&awk->parse.params);
				PANIC (awk, XP_AWK_ECOMMA);
			}

			if (__get_token(awk) == -1) 
			{
				FREE (awk, name_dup);
				xp_awk_tab_clear (&awk->parse.params);
				return XP_NULL;
			}
		}

		if (__get_token(awk) == -1) 
		{
			FREE (awk, name_dup);
			xp_awk_tab_clear (&awk->parse.params);
			return XP_NULL;
		}
	}

	/* check if the function body starts with a left brace */
	if (!MATCH(awk,TOKEN_LBRACE)) 
	{
		FREE (awk, name_dup);
		xp_awk_tab_clear (&awk->parse.params);
		PANIC (awk, XP_AWK_ELBRACE);
	}
	if (__get_token(awk) == -1) 
	{
		FREE (awk, name_dup);
		xp_awk_tab_clear (&awk->parse.params);
		return XP_NULL; 
	}

	/* actual function body */
	body = __parse_block (awk, xp_true);
	if (body == XP_NULL) 
	{
		FREE (awk, name_dup);
		xp_awk_tab_clear (&awk->parse.params);
		return XP_NULL;
	}

	/* TODO: study furthur if the parameter names should be saved 
	 *       for some reasons */
	nargs = xp_awk_tab_getsize (&awk->parse.params);
	/* parameter names are not required anymore. clear them */
	xp_awk_tab_clear (&awk->parse.params);

	afn = (xp_awk_afn_t*) MALLOC (awk, xp_sizeof(xp_awk_afn_t));
	if (afn == XP_NULL) 
	{
		FREE (awk, name_dup);
		xp_awk_clrpt (body);
		return XP_NULL;
	}

	afn->name = XP_NULL; /* function name set below */
	afn->name_len = 0;
	afn->nargs = nargs;
	afn->body  = body;

	n = xp_awk_map_putx (&awk->tree.afns, name_dup, name_len, afn, &pair);
	if (n < 0)
	{
		FREE (awk, name_dup);
		xp_awk_clrpt (body);
		FREE (awk, afn);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	/* duplicate functions should have been detected previously */
	xp_assert (n != 0); 

	afn->name = pair->key; /* do some trick to save a string.  */
	afn->name_len = pair->key_len;
	FREE (awk, name_dup);

	return body;
}

static xp_awk_nde_t* __parse_begin (xp_awk_t* awk)
{
	xp_awk_nde_t* nde;

	xp_assert (MATCH(awk,TOKEN_LBRACE));

	if (__get_token(awk) == -1) return XP_NULL; 
	nde = __parse_block(awk, xp_true);
	if (nde == XP_NULL) return XP_NULL;

	awk->tree.begin = nde;
	return nde;
}

static xp_awk_nde_t* __parse_end (xp_awk_t* awk)
{
	xp_awk_nde_t* nde;

	xp_assert (MATCH(awk,TOKEN_LBRACE));

	if (__get_token(awk) == -1) return XP_NULL; 
	nde = __parse_block(awk, xp_true);
	if (nde == XP_NULL) return XP_NULL;

	awk->tree.end = nde;
	return nde;
}

static xp_awk_chain_t* __parse_pattern_block (
	xp_awk_t* awk, xp_awk_nde_t* ptn, xp_bool_t blockless)
{
	xp_awk_nde_t* nde;
	xp_awk_chain_t* chain;

	if (blockless) nde = XP_NULL;
	else
	{
		xp_assert (MATCH(awk,TOKEN_LBRACE));
		if (__get_token(awk) == -1) return XP_NULL; 
		nde = __parse_block(awk, xp_true);
		if (nde == XP_NULL) return XP_NULL;
	}

	chain = (xp_awk_chain_t*) MALLOC (awk, xp_sizeof(xp_awk_chain_t));
	if (chain == XP_NULL) 
	{
		xp_awk_clrpt (nde);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	chain->pattern = ptn;
	chain->action = nde;
	chain->next = XP_NULL;

	if (awk->tree.chain == XP_NULL) 
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

static xp_awk_nde_t* __parse_block (xp_awk_t* awk, xp_bool_t is_top) 
{
	xp_awk_nde_t* head, * curr, * nde;
	xp_awk_nde_blk_t* block;
	xp_size_t nlocals, nlocals_max, tmp;

	nlocals = xp_awk_tab_getsize(&awk->parse.locals);
	nlocals_max = awk->parse.nlocals_max;

	/* local variable declarations */
	if (awk->option & XP_AWK_EXPLICIT) 
	{
		while (1) 
		{
			if (!MATCH(awk,TOKEN_LOCAL)) break;

			if (__get_token(awk) == -1) 
			{
				xp_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					xp_awk_tab_getsize(&awk->parse.locals) - nlocals);
				return XP_NULL;
			}

			if (__collect_locals(awk, nlocals) == XP_NULL)
			{
				xp_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					xp_awk_tab_getsize(&awk->parse.locals) - nlocals);
				return XP_NULL;
			}
		}
	}

	/* block body */
	head = XP_NULL; curr = XP_NULL;

	while (1) 
	{
		if (MATCH(awk,TOKEN_EOF)) 
		{
			xp_awk_tab_remove (
				&awk->parse.locals, nlocals, 
				xp_awk_tab_getsize(&awk->parse.locals) - nlocals);
			if (head != XP_NULL) xp_awk_clrpt (head);
			PANIC (awk, XP_AWK_EENDSRC);
		}

		if (MATCH(awk,TOKEN_RBRACE)) 
		{
			if (__get_token(awk) == -1) 
			{
				xp_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					xp_awk_tab_getsize(&awk->parse.locals) - nlocals);
				if (head != XP_NULL) xp_awk_clrpt (head);
				return XP_NULL; 
			}
			break;
		}

		nde = __parse_statement (awk);
		if (nde == XP_NULL) 
		{
			xp_awk_tab_remove (
				&awk->parse.locals, nlocals, 
				xp_awk_tab_getsize(&awk->parse.locals) - nlocals);
			if (head != XP_NULL) xp_awk_clrpt (head);
			return XP_NULL;
		}

		/* remove unnecessary statements */
		if (nde->type == XP_AWK_NDE_NULL ||
		    (nde->type == XP_AWK_NDE_BLK && 
		     ((xp_awk_nde_blk_t*)nde)->body == XP_NULL)) continue;
			
		if (curr == XP_NULL) head = nde;
		else curr->next = nde;	
		curr = nde;
	}

	block = (xp_awk_nde_blk_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_blk_t));
	if (block == XP_NULL) 
	{
		xp_awk_tab_remove (
			&awk->parse.locals, nlocals, 
			xp_awk_tab_getsize(&awk->parse.locals) - nlocals);
		xp_awk_clrpt (head);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	tmp = xp_awk_tab_getsize(&awk->parse.locals);
	if (tmp > awk->parse.nlocals_max) awk->parse.nlocals_max = tmp;

	xp_awk_tab_remove (
		&awk->parse.locals, nlocals, tmp - nlocals);

	/* adjust the number of locals for a block without any statements */
	/* if (head == XP_NULL) tmp = 0; */

	block->type = XP_AWK_NDE_BLK;
	block->next = XP_NULL;
	block->body = head;

/* TODO: not only local variables but also nested blocks, 
unless it is part of other constructs such as if, can be promoted 
and merged to top-level block */

	/* migrate all block-local variables to a top-level block */
	if (is_top) 
	{
		block->nlocals = awk->parse.nlocals_max - nlocals;
		awk->parse.nlocals_max = nlocals_max;
	}
	else 
	{
		/*block->nlocals = tmp - nlocals;*/
		block->nlocals = 0;
	}

	return (xp_awk_nde_t*)block;
}

static xp_awk_t* __add_builtin_globals (xp_awk_t* awk)
{
	struct __bvent* p = __bvtab;

	awk->tree.nbglobals = 0;
	while (p->name != XP_NULL)
	{
		if (__add_global (awk, 
			p->name, p->name_len) == XP_NULL) return XP_NULL;
		awk->tree.nbglobals++;
		p++;
	}

	return awk;
}

static xp_awk_t* __add_global (
	xp_awk_t* awk, const xp_char_t* name, xp_size_t len)
{
	if (awk->option & XP_AWK_UNIQUE) 
	{
		/* check if it conflict with a function name */
		if (xp_awk_map_get(&awk->tree.afns, name, len) != XP_NULL) 
		{
			PANIC (awk, XP_AWK_EDUPNAME);
		}
	}

	/* check if it conflicts with other global variable names */
	if (xp_awk_tab_find (&awk->parse.globals, 0, name, len) != (xp_size_t)-1) 
	{ 
		PANIC (awk, XP_AWK_EDUPVAR);
	}

	if (xp_awk_tab_getsize(&awk->parse.globals) >= XP_AWK_MAX_GLOBALS)
	{
		PANIC (awk, XP_AWK_ETOOMANYGLOBALS);
	}

	if (xp_awk_tab_add (&awk->parse.globals, name, len) == (xp_size_t)-1) 
	{
		PANIC (awk, XP_AWK_ENOMEM);
	}

	return awk;
}

static xp_awk_t* __collect_globals (xp_awk_t* awk)
{
	while (1) 
	{
		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			PANIC (awk, XP_AWK_EIDENT);
		}

		if (__add_global (awk,
			XP_STR_BUF(&awk->token.name),
			XP_STR_LEN(&awk->token.name)) == XP_NULL) return XP_NULL;

		if (__get_token(awk) == -1) return XP_NULL;

		if (MATCH(awk,TOKEN_SEMICOLON)) break;

		if (!MATCH(awk,TOKEN_COMMA)) 
		{
			PANIC (awk, XP_AWK_ECOMMA);
		}

		if (__get_token(awk) == -1) return XP_NULL;
	}

	/* skip a semicolon */
	if (__get_token(awk) == -1) return XP_NULL;

	return awk;
}

static xp_awk_t* __collect_locals (xp_awk_t* awk, xp_size_t nlocals)
{
	xp_char_t* local;
	xp_size_t local_len;

	while (1) 
	{
		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			PANIC (awk, XP_AWK_EIDENT);
		}

		local = XP_STR_BUF(&awk->token.name);
		local_len = XP_STR_LEN(&awk->token.name);

		/* NOTE: it is not checked againt globals names */

		if (awk->option & XP_AWK_UNIQUE) 
		{
			/* check if it conflict with a function name */
			if (xp_awk_map_get (
				&awk->tree.afns, local, local_len) != XP_NULL) 
			{
				PANIC (awk, XP_AWK_EDUPNAME);
			}
		}

		/* check if it conflicts with a paremeter name */
		if (xp_awk_tab_find (&awk->parse.params,
			0, local, local_len) != (xp_size_t)-1) 
		{
			PANIC (awk, XP_AWK_EDUPNAME);
		}

		/* check if it conflicts with other local variable names */
		if (xp_awk_tab_find (&awk->parse.locals, 
			((awk->option & XP_AWK_SHADING)? nlocals: 0),
			local, local_len) != (xp_size_t)-1)
		{
			PANIC (awk, XP_AWK_EDUPVAR);	
		}

		if (xp_awk_tab_getsize(&awk->parse.locals) >= XP_AWK_MAX_LOCALS)
		{
			PANIC (awk, XP_AWK_ETOOMANYLOCALS);
		}

		if (xp_awk_tab_add (
			&awk->parse.locals, local, local_len) == (xp_size_t)-1) 
		{
			PANIC (awk, XP_AWK_ENOMEM);
		}

		if (__get_token(awk) == -1) return XP_NULL;

		if (MATCH(awk,TOKEN_SEMICOLON)) break;

		if (!MATCH(awk,TOKEN_COMMA)) PANIC (awk, XP_AWK_ECOMMA);

		if (__get_token(awk) == -1) return XP_NULL;
	}

	/* skip a semicolon */
	if (__get_token(awk) == -1) return XP_NULL;

	return awk;
}

static xp_awk_nde_t* __parse_statement (xp_awk_t* awk)
{
	xp_awk_nde_t* nde;

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* null statement */	
		nde = (xp_awk_nde_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_t));
		if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		nde->type = XP_AWK_NDE_NULL;
		nde->next = XP_NULL;

		if (__get_token(awk) == -1) 
		{
			FREE (awk, nde);
			return XP_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_LBRACE)) 
	{
		if (__get_token(awk) == -1) return XP_NULL; 
		nde = __parse_block (awk, xp_false);
	}
	else nde = __parse_statement_nb (awk);

	return nde;
}

static xp_awk_nde_t* __parse_statement_nb (xp_awk_t* awk)
{
	xp_awk_nde_t* nde;

	/* 
	 * keywords that don't require any terminating semicolon 
	 */
	if (MATCH(awk,TOKEN_IF)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;
		return __parse_if (awk);
	}
	else if (MATCH(awk,TOKEN_WHILE)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;
		
		awk->parse.depth.loop++;
		nde = __parse_while (awk);
		awk->parse.depth.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_FOR)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;

		awk->parse.depth.loop++;
		nde = __parse_for (awk);
		awk->parse.depth.loop--;

		return nde;
	}

	/* 
	 * keywords that require a terminating semicolon 
	 */
	if (MATCH(awk,TOKEN_DO)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;

		awk->parse.depth.loop++;
		nde = __parse_dowhile (awk);
		awk->parse.depth.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_BREAK)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;
		nde = __parse_break (awk);
	}
	else if (MATCH(awk,TOKEN_CONTINUE)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;
		nde = __parse_continue (awk);
	}
	else if (MATCH(awk,TOKEN_RETURN)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;
		nde = __parse_return (awk);
	}
	else if (MATCH(awk,TOKEN_EXIT)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;
		nde = __parse_exit (awk);
	}
	else if (MATCH(awk,TOKEN_NEXT)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;
		nde = __parse_next (awk);
	}
	else if (MATCH(awk,TOKEN_NEXTFILE)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;
		nde = __parse_nextfile (awk);
	}
	else if (MATCH(awk,TOKEN_DELETE)) 
	{
		if (__get_token(awk) == -1) return XP_NULL;
		nde = __parse_delete (awk);
	}
	else if (MATCH(awk,TOKEN_PRINT))
	{
		if (__get_token(awk) == -1) return XP_NULL;
		nde = __parse_print (awk);
	}
	else if (MATCH(awk,TOKEN_PRINTF))
	{
		if (__get_token(awk) == -1) return XP_NULL;
		nde = __parse_printf (awk);
	}
	else 
	{
		nde = __parse_expression(awk);
	}

	if (nde == XP_NULL) return XP_NULL;

	/* check if a statement ends with a semicolon */
	if (!MATCH(awk,TOKEN_SEMICOLON)) 
	{
		if (nde != XP_NULL) xp_awk_clrpt (nde);
		PANIC (awk, XP_AWK_ESEMICOLON);
	}

	/* eat up the semicolon and read in the next token */
	if (__get_token(awk) == -1) 
	{
		if (nde != XP_NULL) xp_awk_clrpt (nde);
		return XP_NULL;
	}

	return nde;
}

static xp_awk_nde_t* __parse_expression (xp_awk_t* awk)
{
	xp_awk_nde_t* x, * y;
	xp_awk_nde_ass_t* nde;
	int opcode;

	x = __parse_basic_expr (awk);
	if (x == XP_NULL) return XP_NULL;

	opcode = __assign_to_opcode (awk);
	if (opcode == -1) 
	{
		/* no assignment operator found. */
		return x;
	}

	xp_assert (x->next == XP_NULL);
	if (!__is_var(x) && x->type != XP_AWK_NDE_POS) 
	{
		xp_awk_clrpt (x);
		PANIC (awk, XP_AWK_EASSIGNMENT);
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (x);
		return XP_NULL;
	}

	y = __parse_basic_expr (awk);
	if (y == XP_NULL) 
	{
		xp_awk_clrpt (x);
		return XP_NULL;
	}

	nde = (xp_awk_nde_ass_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_ass_t));
	if (nde == XP_NULL) 
	{
		xp_awk_clrpt (x);
		xp_awk_clrpt (y);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	nde->type = XP_AWK_NDE_ASS;
	nde->next = XP_NULL;
	nde->opcode = opcode;
	nde->left = x;
	nde->right = y;

	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_basic_expr (xp_awk_t* awk)
{
	xp_awk_nde_t* nde, * n1, * n2;
	
	nde = __parse_logical_or (awk);
	if (nde == XP_NULL) return XP_NULL;

	if (MATCH(awk,TOKEN_QUEST))
	{ 
		xp_awk_nde_cnd_t* tmp;

		if (__get_token(awk) == -1) return XP_NULL;

		n1 = __parse_basic_expr (awk);
		if (n1 == XP_NULL) 
		{
			xp_awk_clrpt (nde);
			return XP_NULL;
		}

		if (!MATCH(awk,TOKEN_COLON)) PANIC (awk, XP_AWK_ECOLON);
		if (__get_token(awk) == -1) return XP_NULL;

		n2 = __parse_basic_expr (awk);
		if (n2 == XP_NULL)
		{
			xp_awk_clrpt (nde);
			xp_awk_clrpt (n1);
			return XP_NULL;
		}

		tmp = (xp_awk_nde_cnd_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_cnd_t));
		if (tmp == XP_NULL)
		{
			xp_awk_clrpt (nde);
			xp_awk_clrpt (n1);
			xp_awk_clrpt (n2);
			return XP_NULL;
		}

		tmp->type = XP_AWK_NDE_CND;
		tmp->next = XP_NULL;
		tmp->test = nde;
		tmp->left = n1;
		tmp->right = n2;

		nde = (xp_awk_nde_t*)tmp;
	}

	return nde;
}

static xp_awk_nde_t* __parse_binary_expr (
	xp_awk_t* awk, const __binmap_t* binmap,
	xp_awk_nde_t*(*next_level_func)(xp_awk_t*))
{
	xp_awk_nde_exp_t* nde;
	xp_awk_nde_t* left, * right;
	int opcode;

	left = next_level_func (awk);
	if (left == XP_NULL) return XP_NULL;
	
	while (1) 
	{
		const __binmap_t* p = binmap;
		xp_bool_t matched = xp_false;

		while (p->token != TOKEN_EOF)
		{
			if (MATCH(awk,p->token)) 
			{
				opcode = p->binop;
				matched = xp_true;
				break;
			}
			p++;
		}
		if (!matched) break;

		if (__get_token(awk) == -1) 
		{
			xp_awk_clrpt (left);
			return XP_NULL; 
		}

		right = next_level_func (awk);
		if (right == XP_NULL) 
		{
			xp_awk_clrpt (left);
			return XP_NULL;
		}

		/* TODO: enhance constant folding. do it in a better way */
		/* TODO: differentiate different types of numbers ... */
		if (left->type == XP_AWK_NDE_INT && 
		    right->type == XP_AWK_NDE_INT) 
		{
			xp_long_t l, r;

			l = ((xp_awk_nde_int_t*)left)->val; 
			r = ((xp_awk_nde_int_t*)right)->val; 

			/* TODO: more operators */
			if (opcode == XP_AWK_BINOP_PLUS) l += r;
			else if (opcode == XP_AWK_BINOP_MINUS) l -= r;
			else if (opcode == XP_AWK_BINOP_MUL) l *= r;
			else if (opcode == XP_AWK_BINOP_DIV && r != 0) l /= r;
			else if (opcode == XP_AWK_BINOP_MOD && r != 0) l %= r;
			else goto skip_constant_folding;

			xp_awk_clrpt (right);
			((xp_awk_nde_int_t*)left)->val = l;
			continue;
		} 
		else if (left->type == XP_AWK_NDE_REAL && 
		         right->type == XP_AWK_NDE_REAL) 
		{
			xp_real_t l, r;

			l = ((xp_awk_nde_real_t*)left)->val; 
			r = ((xp_awk_nde_real_t*)right)->val; 

			/* TODO: more operators */
			if (opcode == XP_AWK_BINOP_PLUS) l += r;
			else if (opcode == XP_AWK_BINOP_MINUS) l -= r;
			else if (opcode == XP_AWK_BINOP_MUL) l *= r;
			else if (opcode == XP_AWK_BINOP_DIV) l /= r;
			else goto skip_constant_folding;

			xp_awk_clrpt (right);
			((xp_awk_nde_real_t*)left)->val = l;
			continue;
		}
		/* TODO: enhance constant folding more... */

	skip_constant_folding:
		nde = (xp_awk_nde_exp_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_exp_t));
		if (nde == XP_NULL) 
		{
			xp_awk_clrpt (right);
			xp_awk_clrpt (left);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		nde->type = XP_AWK_NDE_EXP_BIN;
		nde->next = XP_NULL;
		nde->opcode = opcode; 
		nde->left = left;
		nde->right = right;

		left = (xp_awk_nde_t*)nde;
	}

	return left;
}

static xp_awk_nde_t* __parse_logical_or (xp_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_LOR, XP_AWK_BINOP_LOR },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_logical_and);
}

static xp_awk_nde_t* __parse_logical_and (xp_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_LAND, XP_AWK_BINOP_LAND },
		{ TOKEN_EOF,  0 }
	};

	return __parse_binary_expr (awk, map, __parse_in);
}

static xp_awk_nde_t* __parse_in (xp_awk_t* awk)
{
	/* 
	static __binmap_t map[] =
	{
		{ TOKEN_IN, XP_AWK_BINOP_IN },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_regex_match);
	*/

	xp_awk_nde_exp_t* nde;
	xp_awk_nde_t* left, * right;

	left = __parse_regex_match (awk);
	if (left == XP_NULL) return XP_NULL;

	while (1)
	{
		if (!MATCH(awk,TOKEN_IN)) break;

		if (__get_token(awk) == -1) 
		{
			xp_awk_clrpt (left);
			return XP_NULL; 
		}

		right = __parse_regex_match (awk);
		if (right == XP_NULL) 
		{
			xp_awk_clrpt (left);
			return XP_NULL;
		}

		if (!__is_plain_var(right))
		{
			xp_awk_clrpt (right);
			xp_awk_clrpt (left);
			PANIC (awk, XP_AWK_ENOTVAR);
		}

		nde = (xp_awk_nde_exp_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_exp_t));
		if (nde == XP_NULL) 
		{
			xp_awk_clrpt (right);
			xp_awk_clrpt (left);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		nde->type = XP_AWK_NDE_EXP_BIN;
		nde->next = XP_NULL;
		nde->opcode = XP_AWK_BINOP_IN; 
		nde->left = left;
		nde->right = right;

		left = (xp_awk_nde_t*)nde;
	}

	return left;
}

static xp_awk_nde_t* __parse_regex_match (xp_awk_t* awk)
{
	static __binmap_t map[] =
	{
		{ TOKEN_TILDE, XP_AWK_BINOP_MA },
		{ TOKEN_NM,    XP_AWK_BINOP_NM },
		{ TOKEN_EOF,   0 },
	};

	return __parse_binary_expr (awk, map, __parse_bitwise_or);
}

static xp_awk_nde_t* __parse_bitwise_or (xp_awk_t* awk)
{
	if (awk->option & XP_AWK_EXTIO)
	{
		return __parse_bitwise_or_with_extio (awk);
	}
	else
	{
		static __binmap_t map[] = 
		{
			{ TOKEN_BOR, XP_AWK_BINOP_BOR },
			{ TOKEN_EOF, 0 }
		};

		return __parse_binary_expr (awk, map, __parse_bitwise_xor);
	}
}

static xp_awk_nde_t* __parse_bitwise_or_with_extio (xp_awk_t* awk)
{
	xp_awk_nde_t* left, * right;

	left = __parse_bitwise_xor (awk);
	if (left == XP_NULL) return XP_NULL;

	while (1)
	{
		int in_type;

		if (MATCH(awk,TOKEN_BOR)) 
			in_type = XP_AWK_IN_PIPE;
		else if (MATCH(awk,TOKEN_BORAND)) 
			in_type = XP_AWK_IN_COPROC;
		else break;
		
		if (__get_token(awk) == -1)
		{
			xp_awk_clrpt (left);
			return XP_NULL;
		}

		if (MATCH(awk,TOKEN_GETLINE))
		{
			xp_awk_nde_getline_t* nde;
			xp_awk_nde_t* var = XP_NULL;

			/* piped getline */
			if (__get_token(awk) == -1)
			{
				xp_awk_clrpt (left);
				return XP_NULL;
			}

			/* TODO: is this correct? */

			if (MATCH(awk,TOKEN_IDENT))
			{
				/* command | getline var */

				var = __parse_primary (awk);
				if (var == XP_NULL) 
				{
					xp_awk_clrpt (left);
					return XP_NULL;
				}
			}

			nde = (xp_awk_nde_getline_t*) MALLOC (
				awk, xp_sizeof(xp_awk_nde_getline_t));
			if (nde == XP_NULL)
			{
				xp_awk_clrpt (left);
				PANIC (awk, XP_AWK_ENOMEM);
			}

			nde->type = XP_AWK_NDE_GETLINE;
			nde->next = XP_NULL;
			nde->var = var;
			nde->in_type = in_type;
			nde->in = left;

			left = (xp_awk_nde_t*)nde;
		}
		else
		{
			xp_awk_nde_exp_t* nde;

			if (in_type == XP_AWK_IN_COPROC)
			{
				xp_awk_clrpt (left);
				PANIC (awk, XP_AWK_EGETLINE);
			}

			right = __parse_bitwise_xor (awk);
			if (right == XP_NULL)
			{
				xp_awk_clrpt (left);
				return XP_NULL;
			}

			/* TODO: do constant folding */

			nde = (xp_awk_nde_exp_t*) MALLOC (
				awk, xp_sizeof(xp_awk_nde_exp_t));
			if (nde == XP_NULL)
			{
				xp_awk_clrpt (right);
				xp_awk_clrpt (left);
				PANIC (awk, XP_AWK_ENOMEM);
			}

			nde->type = XP_AWK_NDE_EXP_BIN;
			nde->next = XP_NULL;
			nde->opcode = XP_AWK_BINOP_BOR;
			nde->left = left;
			nde->right = right;

			left = (xp_awk_nde_t*)nde;
		}
	}

	return left;
}

static xp_awk_nde_t* __parse_bitwise_xor (xp_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_BXOR, XP_AWK_BINOP_BXOR },
		{ TOKEN_EOF,  0 }
	};

	return __parse_binary_expr (awk, map, __parse_bitwise_and);
}

static xp_awk_nde_t* __parse_bitwise_and (xp_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_BAND, XP_AWK_BINOP_BAND },
		{ TOKEN_EOF,  0 }
	};

	return __parse_binary_expr (awk, map, __parse_equality);
}

static xp_awk_nde_t* __parse_equality (xp_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_EQ, XP_AWK_BINOP_EQ },
		{ TOKEN_NE, XP_AWK_BINOP_NE },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_relational);
}

static xp_awk_nde_t* __parse_relational (xp_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_GT, XP_AWK_BINOP_GT },
		{ TOKEN_GE, XP_AWK_BINOP_GE },
		{ TOKEN_LT, XP_AWK_BINOP_LT },
		{ TOKEN_LE, XP_AWK_BINOP_LE },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_shift);
}

static xp_awk_nde_t* __parse_shift (xp_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_LSHIFT, XP_AWK_BINOP_LSHIFT },
		{ TOKEN_RSHIFT, XP_AWK_BINOP_RSHIFT },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_concat);
}

static xp_awk_nde_t* __parse_concat (xp_awk_t* awk)
{
	xp_awk_nde_exp_t* nde;
	xp_awk_nde_t* left, * right;

	left = __parse_additive (awk);
	if (left == XP_NULL) return XP_NULL;

	/* TODO: write a better code to do this.... 
	 *       first of all, is the following check sufficient? */
	while (MATCH(awk,TOKEN_LPAREN) || 
	       MATCH(awk,TOKEN_DOLLAR) ||
	       awk->token.type >= TOKEN_GETLINE)
	{
		right = __parse_additive (awk);
		if (right == XP_NULL) 
		{
			xp_awk_clrpt (left);
			return XP_NULL;
		}

		nde = (xp_awk_nde_exp_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_exp_t));
		if (nde == XP_NULL)
		{
			xp_awk_clrpt (left);
			xp_awk_clrpt (right);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		nde->type = XP_AWK_NDE_EXP_BIN;
		nde->next = XP_NULL;
		nde->opcode = XP_AWK_BINOP_CONCAT;
		nde->left = left;
		nde->right = right;
		
		left = (xp_awk_nde_t*)nde;
	}

	return left;
}

static xp_awk_nde_t* __parse_additive (xp_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_PLUS, XP_AWK_BINOP_PLUS },
		{ TOKEN_MINUS, XP_AWK_BINOP_MINUS },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_multiplicative);
}

static xp_awk_nde_t* __parse_multiplicative (xp_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_MUL, XP_AWK_BINOP_MUL },
		{ TOKEN_DIV, XP_AWK_BINOP_DIV },
		{ TOKEN_MOD, XP_AWK_BINOP_MOD },
		{ TOKEN_EXP, XP_AWK_BINOP_EXP },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_unary);
}

static xp_awk_nde_t* __parse_unary (xp_awk_t* awk)
{
	xp_awk_nde_exp_t* nde; 
	xp_awk_nde_t* left;
	int opcode;

	opcode = (MATCH(awk,TOKEN_PLUS))?  XP_AWK_UNROP_PLUS:
	         (MATCH(awk,TOKEN_MINUS))? XP_AWK_UNROP_MINUS:
	         (MATCH(awk,TOKEN_NOT))?   XP_AWK_UNROP_NOT:
	         (MATCH(awk,TOKEN_TILDE))? XP_AWK_UNROP_BNOT: -1;

	if (opcode == -1) return __parse_increment (awk);

	if (__get_token(awk) == -1) return XP_NULL;

	left = __parse_unary (awk);
	if (left == XP_NULL) return XP_NULL;

	nde = (xp_awk_nde_exp_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_exp_t));
	if (nde == XP_NULL)
	{
		xp_awk_clrpt (left);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	nde->type = XP_AWK_NDE_EXP_UNR;
	nde->next = XP_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = XP_NULL;

	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_increment (xp_awk_t* awk)
{
	xp_awk_nde_exp_t* nde;
	xp_awk_nde_t* left;
	int type, opcode, opcode1, opcode2;

	opcode1 = MATCH(awk,TOKEN_PLUSPLUS)? XP_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? XP_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1)
	{
		if (__get_token(awk) == -1) return XP_NULL;
	}

	left = __parse_primary (awk);
	if (left == XP_NULL) return XP_NULL;

	opcode2 = MATCH(awk,TOKEN_PLUSPLUS)? XP_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? XP_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1 && opcode2 != -1)
	{
		xp_awk_clrpt (left);
		PANIC (awk, XP_AWK_ELVALUE);
	}
	else if (opcode1 == -1 && opcode2 == -1)
	{
		return left;
	}
	else if (opcode1 != -1) 
	{
		type = XP_AWK_NDE_EXP_INCPRE;
		opcode = opcode1;
	}
	else if (opcode2 != -1) 
	{
		type = XP_AWK_NDE_EXP_INCPST;
		opcode = opcode2;

		if (__get_token(awk) == -1) return XP_NULL;
	}

	nde = (xp_awk_nde_exp_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_exp_t));
	if (nde == XP_NULL)
	{
		xp_awk_clrpt (left);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	nde->type = type;
	nde->next = XP_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = XP_NULL;

	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_primary (xp_awk_t* awk)
{
	if (MATCH(awk,TOKEN_IDENT))  
	{
		return __parse_primary_ident (awk);
	}
	else if (MATCH(awk,TOKEN_INT)) 
	{
		xp_awk_nde_int_t* nde;

		nde = (xp_awk_nde_int_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_int_t));
		if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		nde->type = XP_AWK_NDE_INT;
		nde->next = XP_NULL;
		nde->val = xp_awk_strtolong (
			XP_STR_BUF(&awk->token.name), 0, XP_NULL);

		xp_assert (
			XP_STR_LEN(&awk->token.name) ==
			xp_strlen(XP_STR_BUF(&awk->token.name)));

		if (__get_token(awk) == -1) 
		{
			FREE (awk, nde);
			return XP_NULL;			
		}

		return (xp_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_REAL)) 
	{
		xp_awk_nde_real_t* nde;

		nde = (xp_awk_nde_real_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_real_t));
		if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		nde->type = XP_AWK_NDE_REAL;
		nde->next = XP_NULL;
		nde->val = xp_awk_strtoreal (XP_STR_BUF(&awk->token.name));

		xp_assert (
			XP_STR_LEN(&awk->token.name) ==
			xp_strlen(XP_STR_BUF(&awk->token.name)));

		if (__get_token(awk) == -1) 
		{
			FREE (awk, nde);
			return XP_NULL;			
		}

		return (xp_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_STR))  
	{
		xp_awk_nde_str_t* nde;

		nde = (xp_awk_nde_str_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_str_t));
		if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		nde->type = XP_AWK_NDE_STR;
		nde->next = XP_NULL;
		nde->len = XP_STR_LEN(&awk->token.name);
		nde->buf = xp_strxdup(XP_STR_BUF(&awk->token.name), nde->len);
		if (nde->buf == XP_NULL) 
		{
			FREE (awk, nde);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		if (__get_token(awk) == -1) 
		{
			FREE (awk, nde->buf);
			FREE (awk, nde);
			return XP_NULL;			
		}

		return (xp_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_DIV))
	{
		xp_awk_nde_rex_t* nde;
		int errnum;

		/* the regular expression is tokenized here because 
		 * of the context-sensitivity of the slash symbol */
		SET_TOKEN_TYPE (awk, TOKEN_REX);
		xp_str_clear (&awk->token.name);
		if (__get_rexstr (awk) == -1) return XP_NULL;
		xp_assert (MATCH(awk,TOKEN_REX));

		nde = (xp_awk_nde_rex_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_rex_t));
		if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		nde->type = XP_AWK_NDE_REX;
		nde->next = XP_NULL;

		nde->len = XP_STR_LEN(&awk->token.name);
		nde->buf = xp_strxdup (
			XP_STR_BUF(&awk->token.name),
			XP_STR_LEN(&awk->token.name));
		if (nde->buf == XP_NULL)
		{
			FREE (awk, nde);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		nde->code = xp_awk_buildrex (
			XP_STR_BUF(&awk->token.name), 
			XP_STR_LEN(&awk->token.name), 
			&errnum);
		if (nde->code == XP_NULL)
		{
			FREE (awk, nde->buf);
			FREE (awk, nde);
			PANIC (awk, errnum);
		}

		if (__get_token(awk) == -1) 
		{
			FREE (awk, nde->buf);
			FREE (awk, nde->code);
			FREE (awk, nde);
			return XP_NULL;			
		}

		return (xp_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_DOLLAR)) 
	{
		xp_awk_nde_pos_t* nde;
		xp_awk_nde_t* prim;

		if (__get_token(awk)) return XP_NULL;
		
		prim = __parse_primary (awk);
		if (prim == XP_NULL) return XP_NULL;

		nde = (xp_awk_nde_pos_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_pos_t));
		if (nde == XP_NULL) 
		{
			xp_awk_clrpt (prim);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		nde->type = XP_AWK_NDE_POS;
		nde->next = XP_NULL;
		nde->val = prim;

		return (xp_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		xp_awk_nde_t* nde;
		xp_awk_nde_t* last;

		/* eat up the left parenthesis */
		if (__get_token(awk) == -1) return XP_NULL;

		/* parse the sub-expression inside the parentheses */
		nde = __parse_expression (awk);
		if (nde == XP_NULL) return XP_NULL;

		/* parse subsequent expressions separated by a comma, if any */
		last = nde;
		xp_assert (last->next == XP_NULL);

		while (MATCH(awk,TOKEN_COMMA))
		{
			xp_awk_nde_t* tmp;

			if (__get_token(awk) == -1) 
			{
				xp_awk_clrpt (nde);
				return XP_NULL;
			}	

			tmp = __parse_expression (awk);
			if (tmp == XP_NULL) 
			{
				xp_awk_clrpt (nde);
				return XP_NULL;
			}

			xp_assert (tmp->next == XP_NULL);
			last->next = tmp;
			last = tmp;
		} 
		/* ----------------- */

		/* check for the closing parenthesis */
		if (!MATCH(awk,TOKEN_RPAREN)) 
		{
			xp_awk_clrpt (nde);
			PANIC (awk, XP_AWK_ERPAREN);
		}

		if (__get_token(awk) == -1) 
		{
			xp_awk_clrpt (nde);
			return XP_NULL;
		}

		/* check if it is a chained node */
		if (nde->next != XP_NULL)
		{
			/* if so, it is a expression group */
			/* (expr1, expr2, expr2) */

			xp_awk_nde_grp_t* tmp;

			if (!MATCH(awk,TOKEN_IN))
			{
				xp_awk_clrpt (nde);
				PANIC (awk, XP_AWK_EIN);
			}

			tmp = (xp_awk_nde_grp_t*) MALLOC (
				awk, xp_sizeof(xp_awk_nde_grp_t));
			if (tmp == XP_NULL)
			{
				xp_awk_clrpt (nde);
				PANIC (awk, XP_AWK_ENOMEM);
			}	

			tmp->type = XP_AWK_NDE_GRP;
			tmp->next = XP_NULL;
			tmp->body = nde;		

			nde = (xp_awk_nde_t*)tmp;
		}
		/* ----------------- */

		return nde;
	}
	else if (MATCH(awk,TOKEN_GETLINE)) 
	{
		xp_awk_nde_getline_t* nde;
		xp_awk_nde_t* var = XP_NULL;
		xp_awk_nde_t* in = XP_NULL;

		if (__get_token(awk) == -1) return XP_NULL;

		if (MATCH(awk,TOKEN_IDENT))
		{
			/* getline var */
			
			var = __parse_primary (awk);
			if (var == XP_NULL) return XP_NULL;
		}

		if (MATCH(awk, TOKEN_LT))
		{
			/* getline [var] < file */
			if (__get_token(awk) == -1)
			{
				if (var != XP_NULL) xp_awk_clrpt (var);
				return XP_NULL;
			}

			/* TODO: is this correct? */
			/*in = __parse_expression (awk);*/
			in = __parse_primary (awk);
			if (in == XP_NULL)
			{
				if (var != XP_NULL) xp_awk_clrpt (var);
				return XP_NULL;
			}
		}

		nde = (xp_awk_nde_getline_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_getline_t));
		if (nde == XP_NULL)
		{
			if (var != XP_NULL) xp_awk_clrpt (var);
			if (in != XP_NULL) xp_awk_clrpt (in);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		nde->type = XP_AWK_NDE_GETLINE;
		nde->next = XP_NULL;
		nde->var = var;
		nde->in_type = (in == XP_NULL)? 
			XP_AWK_IN_CONSOLE: XP_AWK_IN_FILE;
		nde->in = in;

		return (xp_awk_nde_t*)nde;
	}

	/* valid expression introducer is expected */
	PANIC (awk, XP_AWK_EEXPRESSION);
}

static xp_awk_nde_t* __parse_primary_ident (xp_awk_t* awk)
{
	xp_char_t* name_dup;
	xp_size_t name_len;
	xp_awk_bfn_t* bfn;

	xp_assert (MATCH(awk,TOKEN_IDENT));

	name_dup = xp_strxdup (
		XP_STR_BUF(&awk->token.name), XP_STR_LEN(&awk->token.name));
	if (name_dup == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	name_len = XP_STR_LEN(&awk->token.name);

	if (__get_token(awk) == -1) 
	{
		FREE (awk, name_dup);
		return XP_NULL;			
	}

	/* check if name_dup is a built-in function name */
	bfn = xp_awk_getbfn (awk, name_dup);
	if (bfn != XP_NULL)
	{
		xp_awk_nde_t* nde;

		FREE (awk, name_dup);
		if (!MATCH(awk,TOKEN_LPAREN))
		{
			/* built-in function should be in the form 
		 	 * of the function call */
			PANIC (awk, XP_AWK_ELPAREN);
		}

		nde = __parse_fncall (awk, XP_NULL, 0, bfn);
		return (xp_awk_nde_t*)nde;
	}

	/* now we know that name_dup is a normal identifier. */
	if (MATCH(awk,TOKEN_LBRACK)) 
	{
		xp_awk_nde_t* nde;
		nde = __parse_hashidx (awk, name_dup, name_len);
		if (nde == XP_NULL) FREE (awk, name_dup);
		return (xp_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		/* function call */
		xp_awk_nde_t* nde;
		nde = __parse_fncall (awk, name_dup, name_len, XP_NULL);
		if (nde == XP_NULL) FREE (awk, name_dup);
		return (xp_awk_nde_t*)nde;
	}	
	else 
	{
		/* normal variable */
		xp_awk_nde_var_t* nde;
		xp_size_t idxa;

		nde = (xp_awk_nde_var_t*) MALLOC (
			awk, xp_sizeof(xp_awk_nde_var_t));
		if (nde == XP_NULL) 
		{
			FREE (awk, name_dup);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		/* search the parameter name list */
		idxa = xp_awk_tab_find (
			&awk->parse.params, 0, name_dup, name_len);
		if (idxa != (xp_size_t)-1) 
		{
			nde->type = XP_AWK_NDE_ARG;
			nde->next = XP_NULL;
			/*nde->id.name = XP_NULL;*/
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = idxa;
			nde->idx = XP_NULL;

			return (xp_awk_nde_t*)nde;
		}

		/* search the local variable list */
		idxa = xp_awk_tab_rrfind (
			&awk->parse.locals, 0, name_dup, name_len);
		if (idxa != (xp_size_t)-1) 
		{
			nde->type = XP_AWK_NDE_LOCAL;
			nde->next = XP_NULL;
			/*nde->id.name = XP_NULL;*/
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = idxa;
			nde->idx = XP_NULL;

			return (xp_awk_nde_t*)nde;
		}

		/* search the global variable list */
		idxa = xp_awk_tab_rrfind (
			&awk->parse.globals, 0, name_dup, name_len);
		if (idxa != (xp_size_t)-1) 
		{
			nde->type = XP_AWK_NDE_GLOBAL;
			nde->next = XP_NULL;
			/*nde->id.name = XP_NULL;*/
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = idxa;
			nde->idx = XP_NULL;

			return (xp_awk_nde_t*)nde;
		}

		if (awk->option & XP_AWK_IMPLICIT) 
		{
			nde->type = XP_AWK_NDE_NAMED;
			nde->next = XP_NULL;
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = (xp_size_t)-1;
			nde->idx = XP_NULL;

			return (xp_awk_nde_t*)nde;
		}

		/* undefined variable */
		FREE (awk, name_dup);
		FREE (awk, nde);
		PANIC (awk, XP_AWK_EUNDEF);
	}
}

static xp_awk_nde_t* __parse_hashidx (
	xp_awk_t* awk, xp_char_t* name, xp_size_t name_len)
{
	xp_awk_nde_t* idx, * tmp, * last;
	xp_awk_nde_var_t* nde;
	xp_size_t idxa;

	idx = XP_NULL;
	last = XP_NULL;

	do
	{
		if (__get_token(awk) == -1) 
		{
			if (idx != XP_NULL) xp_awk_clrpt (idx);
			return XP_NULL;
		}

		tmp = __parse_expression (awk);
		if (tmp == XP_NULL) 
		{
			if (idx != XP_NULL) xp_awk_clrpt (idx);
			return XP_NULL;
		}

		if (idx == XP_NULL)
		{
			xp_assert (last == XP_NULL);
			idx = tmp; last = tmp;
		}
		else
		{
			last->next = tmp;
			last = tmp;
		}
	}
	while (MATCH(awk,TOKEN_COMMA));

	xp_assert (idx != XP_NULL);

	if (!MATCH(awk,TOKEN_RBRACK)) 
	{
		xp_awk_clrpt (idx);
		PANIC (awk, XP_AWK_ERBRACK);
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (idx);
		return XP_NULL;
	}

	nde = (xp_awk_nde_var_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_var_t));
	if (nde == XP_NULL) 
	{
		xp_awk_clrpt (idx);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	/* search the parameter name list */
	idxa = xp_awk_tab_find (&awk->parse.params, 0, name, name_len);
	if (idxa != (xp_size_t)-1) 
	{
		nde->type = XP_AWK_NDE_ARGIDX;
		nde->next = XP_NULL;
		/*nde->id.name = XP_NULL; */
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (xp_awk_nde_t*)nde;
	}

	/* search the local variable list */
	idxa = xp_awk_tab_rrfind(&awk->parse.locals, 0, name, name_len);
	if (idxa != (xp_size_t)-1) 
	{
		nde->type = XP_AWK_NDE_LOCALIDX;
		nde->next = XP_NULL;
		/*nde->id.name = XP_NULL; */
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (xp_awk_nde_t*)nde;
	}

	/* search the global variable list */
	idxa = xp_awk_tab_rrfind(&awk->parse.globals, 0, name, name_len);
	if (idxa != (xp_size_t)-1) 
	{
		nde->type = XP_AWK_NDE_GLOBALIDX;
		nde->next = XP_NULL;
		/*nde->id.name = XP_NULL;*/
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (xp_awk_nde_t*)nde;
	}

	if (awk->option & XP_AWK_IMPLICIT) 
	{
		nde->type = XP_AWK_NDE_NAMEDIDX;
		nde->next = XP_NULL;
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = (xp_size_t)-1;
		nde->idx = idx;

		return (xp_awk_nde_t*)nde;
	}

	/* undefined variable */
	xp_awk_clrpt (idx);
	FREE (awk, nde);
	PANIC (awk, XP_AWK_EUNDEF);
}

static xp_awk_nde_t* __parse_fncall (
	xp_awk_t* awk, xp_char_t* name, xp_size_t name_len, xp_awk_bfn_t* bfn)
{
	xp_awk_nde_t* head, * curr, * nde;
	xp_awk_nde_call_t* call;
	xp_size_t nargs;

	if (__get_token(awk) == -1) return XP_NULL;
	
	head = curr = XP_NULL;
	nargs = 0;

	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no parameters to the function call */
		if (__get_token(awk) == -1) return XP_NULL;
	}
	else 
	{
		/* parse function parameters */

		while (1) 
		{
			nde = __parse_expression (awk);
			if (nde == XP_NULL) 
			{
				if (head != XP_NULL) xp_awk_clrpt (head);
				return XP_NULL;
			}
	
			if (head == XP_NULL) head = nde;
			else curr->next = nde;
			curr = nde;

			nargs++;

			if (MATCH(awk,TOKEN_RPAREN)) 
			{
				if (__get_token(awk) == -1) 
				{
					if (head != XP_NULL) 
						xp_awk_clrpt (head);
					return XP_NULL;
				}
				break;
			}

			if (!MATCH(awk,TOKEN_COMMA)) 
			{
				if (head != XP_NULL) xp_awk_clrpt (head);
				PANIC (awk, XP_AWK_ECOMMA);	
			}

			if (__get_token(awk) == -1) 
			{
				if (head != XP_NULL) xp_awk_clrpt (head);
				return XP_NULL;
			}
		}

	}

	call = (xp_awk_nde_call_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_call_t));
	if (call == XP_NULL) 
	{
		if (head != XP_NULL) xp_awk_clrpt (head);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	if (bfn != XP_NULL)
	{
		call->type = XP_AWK_NDE_BFN;
		call->next = XP_NULL;

		/*call->what.bfn = bfn; */
		call->what.bfn.name     = bfn->name;
		call->what.bfn.name_len = bfn->name_len;
		call->what.bfn.min_args = bfn->min_args;
		call->what.bfn.max_args = bfn->max_args;
		call->what.bfn.arg_spec = bfn->arg_spec;
		call->what.bfn.handler  = bfn->handler;

		call->args = head;
		call->nargs = nargs;
	}
	else
	{
		call->type = XP_AWK_NDE_AFN;
		call->next = XP_NULL;
		call->what.afn.name = name; 
		call->what.afn.name_len = name_len;
		call->args = head;
		call->nargs = nargs;
	}

	return (xp_awk_nde_t*)call;
}

static xp_awk_nde_t* __parse_if (xp_awk_t* awk)
{
	xp_awk_nde_t* test;
	xp_awk_nde_t* then_part;
	xp_awk_nde_t* else_part;
	xp_awk_nde_if_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, XP_AWK_ELPAREN);
	if (__get_token(awk) == -1) return XP_NULL;

	test = __parse_expression (awk);
	if (test == XP_NULL) return XP_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (test);
		return XP_NULL;
	}

	then_part = __parse_statement (awk);
	if (then_part == XP_NULL) 
	{
		xp_awk_clrpt (test);
		return XP_NULL;
	}

	if (MATCH(awk,TOKEN_ELSE)) 
	{
		if (__get_token(awk) == -1) 
		{
			xp_awk_clrpt (then_part);
			xp_awk_clrpt (test);
			return XP_NULL;
		}

		else_part = __parse_statement (awk);
		if (else_part == XP_NULL) 
		{
			xp_awk_clrpt (then_part);
			xp_awk_clrpt (test);
			return XP_NULL;
		}
	}
	else else_part = XP_NULL;

	nde = (xp_awk_nde_if_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_if_t));
	if (nde == XP_NULL) 
	{
		xp_awk_clrpt (else_part);
		xp_awk_clrpt (then_part);
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	nde->type = XP_AWK_NDE_IF;
	nde->next = XP_NULL;
	nde->test = test;
	nde->then_part = then_part;
	nde->else_part = else_part;

	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_while (xp_awk_t* awk)
{
	xp_awk_nde_t* test, * body;
	xp_awk_nde_while_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, XP_AWK_ELPAREN);
	if (__get_token(awk) == -1) return XP_NULL;

	test = __parse_expression (awk);
	if (test == XP_NULL) return XP_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (test);
		return XP_NULL;
	}

	body = __parse_statement (awk);
	if (body == XP_NULL) 
	{
		xp_awk_clrpt (test);
		return XP_NULL;
	}

	nde = (xp_awk_nde_while_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_while_t));
	if (nde == XP_NULL) 
	{
		xp_awk_clrpt (body);
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	nde->type = XP_AWK_NDE_WHILE;
	nde->next = XP_NULL;
	nde->test = test;
	nde->body = body;

	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_for (xp_awk_t* awk)
{
	xp_awk_nde_t* init, * test, * incr, * body;
	xp_awk_nde_for_t* nde; 
	xp_awk_nde_foreach_t* nde2;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, XP_AWK_ELPAREN);
	if (__get_token(awk) == -1) return XP_NULL;
		
	if (MATCH(awk,TOKEN_SEMICOLON)) init = XP_NULL;
	else 
	{
		/* this line is very ugly. it checks the entire next 
		 * expression or the first element in the expression
		 * is wrapped by a parenthesis */
		int no_foreach = MATCH(awk,TOKEN_LPAREN);

		init = __parse_expression (awk);
		if (init == XP_NULL) return XP_NULL;

		if (!no_foreach && init->type == XP_AWK_NDE_EXP_BIN &&
		    ((xp_awk_nde_exp_t*)init)->opcode == XP_AWK_BINOP_IN &&
		    __is_plain_var(((xp_awk_nde_exp_t*)init)->left))
		{	
			/* switch to foreach */
			
			if (!MATCH(awk,TOKEN_RPAREN))
			{
				xp_awk_clrpt (init);
				PANIC (awk, XP_AWK_ERPAREN);
			}

			if (__get_token(awk) == -1) 
			{
				xp_awk_clrpt (init);
				return XP_NULL;

			}	
			
			body = __parse_statement (awk);
			if (body == XP_NULL) 
			{
				xp_awk_clrpt (init);
				return XP_NULL;
			}

			nde2 = (xp_awk_nde_foreach_t*) MALLOC (
				awk, xp_sizeof(xp_awk_nde_foreach_t));
			if (nde2 == XP_NULL)
			{
				xp_awk_clrpt (init);
				xp_awk_clrpt (body);
				PANIC (awk, XP_AWK_ENOMEM);
			}

			nde2->type = XP_AWK_NDE_FOREACH;
			nde2->next = XP_NULL;
			nde2->test = init;
			nde2->body = body;

			return (xp_awk_nde_t*)nde2;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			xp_awk_clrpt (init);
			PANIC (awk, XP_AWK_ESEMICOLON);
		}
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (init);
		return XP_NULL;
	}

	if (MATCH(awk,TOKEN_SEMICOLON)) test = XP_NULL;
	else 
	{
		test = __parse_expression (awk);
		if (test == XP_NULL) 
		{
			xp_awk_clrpt (init);
			return XP_NULL;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			xp_awk_clrpt (init);
			xp_awk_clrpt (test);
			PANIC (awk, XP_AWK_ESEMICOLON);
		}
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (init);
		xp_awk_clrpt (test);
		return XP_NULL;
	}
	
	if (MATCH(awk,TOKEN_RPAREN)) incr = XP_NULL;
	else 
	{
		incr = __parse_expression (awk);
		if (incr == XP_NULL) 
		{
			xp_awk_clrpt (init);
			xp_awk_clrpt (test);
			return XP_NULL;
		}

		if (!MATCH(awk,TOKEN_RPAREN)) 
		{
			xp_awk_clrpt (init);
			xp_awk_clrpt (test);
			xp_awk_clrpt (incr);
			PANIC (awk, XP_AWK_ERPAREN);
		}
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (init);
		xp_awk_clrpt (test);
		xp_awk_clrpt (incr);
		return XP_NULL;
	}

	body = __parse_statement (awk);
	if (body == XP_NULL) 
	{
		xp_awk_clrpt (init);
		xp_awk_clrpt (test);
		xp_awk_clrpt (incr);
		return XP_NULL;
	}

	nde = (xp_awk_nde_for_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_for_t));
	if (nde == XP_NULL) 
	{
		xp_awk_clrpt (init);
		xp_awk_clrpt (test);
		xp_awk_clrpt (incr);
		xp_awk_clrpt (body);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	nde->type = XP_AWK_NDE_FOR;
	nde->next = XP_NULL;
	nde->init = init;
	nde->test = test;
	nde->incr = incr;
	nde->body = body;

	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_dowhile (xp_awk_t* awk)
{
	xp_awk_nde_t* test, * body;
	xp_awk_nde_while_t* nde;

	body = __parse_statement (awk);
	if (body == XP_NULL) return XP_NULL;

	if (!MATCH(awk,TOKEN_WHILE)) 
	{
		xp_awk_clrpt (body);
		PANIC (awk, XP_AWK_EWHILE);
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (body);
		return XP_NULL;
	}

	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		xp_awk_clrpt (body);
		PANIC (awk, XP_AWK_ELPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (body);
		return XP_NULL;
	}

	test = __parse_expression (awk);
	if (test == XP_NULL) 
	{
		xp_awk_clrpt (body);
		return XP_NULL;
	}

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		xp_awk_clrpt (body);
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		xp_awk_clrpt (body);
		xp_awk_clrpt (test);
		return XP_NULL;
	}
	
	nde = (xp_awk_nde_while_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_while_t));
	if (nde == XP_NULL) 
	{
		xp_awk_clrpt (body);
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	nde->type = XP_AWK_NDE_DOWHILE;
	nde->next = XP_NULL;
	nde->test = test;
	nde->body = body;

	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_break (xp_awk_t* awk)
{
	xp_awk_nde_break_t* nde;

	if (awk->parse.depth.loop <= 0) PANIC (awk, XP_AWK_EBREAK);

	nde = (xp_awk_nde_break_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_break_t));
	if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	nde->type = XP_AWK_NDE_BREAK;
	nde->next = XP_NULL;
	
	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_continue (xp_awk_t* awk)
{
	xp_awk_nde_continue_t* nde;

	if (awk->parse.depth.loop <= 0) PANIC (awk, XP_AWK_ECONTINUE);

	nde = (xp_awk_nde_continue_t*) MALLOC (
		awk, xp_sizeof(xp_awk_nde_continue_t));
	if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	nde->type = XP_AWK_NDE_CONTINUE;
	nde->next = XP_NULL;
	
	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_return (xp_awk_t* awk)
{
	xp_awk_nde_return_t* nde;
	xp_awk_nde_t* val;

	nde = (xp_awk_nde_return_t*) MALLOC (
		awk, xp_sizeof(xp_awk_nde_return_t));
	if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	nde->type = XP_AWK_NDE_RETURN;
	nde->next = XP_NULL;

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* no return value */
		val = XP_NULL;
	}
	else 
	{
		val = __parse_expression (awk);
		if (val == XP_NULL) 
		{
			FREE (awk, nde);
			return XP_NULL;
		}
	}

	nde->val = val;
	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_exit (xp_awk_t* awk)
{
	xp_awk_nde_exit_t* nde;
	xp_awk_nde_t* val;

	nde = (xp_awk_nde_exit_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_exit_t));
	if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	nde->type = XP_AWK_NDE_EXIT;
	nde->next = XP_NULL;

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* no exit code */
		val = XP_NULL;
	}
	else 
	{
		val = __parse_expression (awk);
		if (val == XP_NULL) 
		{
			FREE (awk, nde);
			return XP_NULL;
		}
	}

	nde->val = val;
	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_delete (xp_awk_t* awk)
{
	xp_awk_nde_delete_t* nde;
	xp_awk_nde_t* var;

	if (!MATCH(awk,TOKEN_IDENT)) PANIC (awk, XP_AWK_EIDENT);

	var = __parse_primary_ident (awk);
	if (var == XP_NULL) return XP_NULL;

	if (!__is_var (var))
	{
		/* a normal identifier is expected */
		xp_awk_clrpt (var);
		PANIC (awk, XP_AWK_EIDENT);
	}

	nde = (xp_awk_nde_delete_t*) MALLOC (
		awk, xp_sizeof(xp_awk_nde_delete_t));
	if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

	nde->type = XP_AWK_NDE_DELETE;
	nde->next = XP_NULL;
	nde->var = var;

	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_print (xp_awk_t* awk)
{
	xp_awk_nde_print_t* nde;
	xp_awk_nde_t* args = XP_NULL; 
	xp_awk_nde_t* args_tail = XP_NULL;
	xp_awk_nde_t* tail_prev = XP_NULL;
	xp_awk_nde_t* out = XP_NULL;
	int out_type;

	if (!MATCH(awk,TOKEN_SEMICOLON) &&
	    !MATCH(awk,TOKEN_GT) &&
	    !MATCH(awk,TOKEN_RSHIFT) &&
	    !MATCH(awk,TOKEN_BOR) &&
	    !MATCH(awk,TOKEN_BORAND)) 
	{
		args = __parse_expression (awk);
		if (args == XP_NULL) return XP_NULL;

		args_tail = args;
		tail_prev = XP_NULL;

		while (MATCH(awk,TOKEN_COMMA))
		{
			if (__get_token(awk) == -1)
			{
				xp_awk_clrpt (args);
				return XP_NULL;
			}

			args_tail->next = __parse_expression (awk);
			if (args_tail->next == XP_NULL)
			{
				xp_awk_clrpt (args);
				return XP_NULL;
			}

			tail_prev = args_tail;
			args_tail = args_tail->next;
		}

		/* print 1 > 2 would print 1 to the file named 2. 
		 * print (1 > 2) would print (1 > 2) in the console */
		if (awk->token.prev != TOKEN_RPAREN &&
		    args_tail->type == XP_AWK_NDE_EXP_BIN)
		{
			xp_awk_nde_exp_t* ep = (xp_awk_nde_exp_t*)args_tail;
			if (ep->opcode == XP_AWK_BINOP_GT)
			{
				xp_awk_nde_t* tmp = args_tail;

				if (tail_prev != XP_NULL) 
					tail_prev->next = ep->left;
				else args = ep->left;

				out = ep->right;
				out_type = XP_AWK_OUT_FILE;

				FREE (awk, tmp);
			}
			else if (ep->opcode == XP_AWK_BINOP_RSHIFT)
			{
				xp_awk_nde_t* tmp = args_tail;

				if (tail_prev != XP_NULL) 
					tail_prev->next = ep->left;
				else args = ep->left;

				out = ep->right;
				out_type = XP_AWK_OUT_FILE_APPEND;

				FREE (awk, tmp);
			}
			else if (ep->opcode == XP_AWK_BINOP_BOR)
			{
				xp_awk_nde_t* tmp = args_tail;

				if (tail_prev != XP_NULL) 
					tail_prev->next = ep->left;
				else args = ep->left;

				out = ep->right;
				out_type = XP_AWK_OUT_PIPE;

				FREE (awk, tmp);
			}
		}
	}

	if (out == XP_NULL)
	{
		out_type = MATCH(awk,TOKEN_GT)?     XP_AWK_OUT_FILE:
		           MATCH(awk,TOKEN_RSHIFT)? XP_AWK_OUT_FILE_APPEND:
		           MATCH(awk,TOKEN_BOR)?    XP_AWK_OUT_PIPE:
		           MATCH(awk,TOKEN_BORAND)? XP_AWK_OUT_COPROC:
		                                    XP_AWK_OUT_CONSOLE;

		if (out_type != XP_AWK_OUT_CONSOLE)
		{
			if (__get_token(awk) == -1)
			{
				if (args != XP_NULL) xp_awk_clrpt (args);
				return XP_NULL;
			}

			out = __parse_expression(awk);
			if (out == XP_NULL)
			{
				if (args != XP_NULL) xp_awk_clrpt (args);
				return XP_NULL;
			}
		}
	}

	nde = (xp_awk_nde_print_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_print_t));
	if (nde == XP_NULL) 
	{
		if (args != XP_NULL) xp_awk_clrpt (args);
		if (out != XP_NULL) xp_awk_clrpt (out);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	nde->type = XP_AWK_NDE_PRINT;
	nde->next = XP_NULL;
	nde->args = args;
	nde->out_type = out_type;
	nde->out = out;

	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_printf (xp_awk_t* awk)
{
	/* TODO: implement this... */
	return XP_NULL;
}

static xp_awk_nde_t* __parse_next (xp_awk_t* awk)
{
	xp_awk_nde_next_t* nde;

	if (awk->parse.id.block == PARSE_BEGIN_BLOCK ||
	    awk->parse.id.block == PARSE_END_BLOCK)
	{
		PANIC (awk, XP_AWK_ENEXT);
	}

	nde = (xp_awk_nde_next_t*) MALLOC (awk, xp_sizeof(xp_awk_nde_next_t));
	if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	nde->type = XP_AWK_NDE_NEXT;
	nde->next = XP_NULL;
	
	return (xp_awk_nde_t*)nde;
}

static xp_awk_nde_t* __parse_nextfile (xp_awk_t* awk)
{
	xp_awk_nde_nextfile_t* nde;

	if (awk->parse.id.block == PARSE_BEGIN_BLOCK ||
	    awk->parse.id.block == PARSE_END_BLOCK)
	{
		PANIC (awk, XP_AWK_ENEXTFILE);
	}

	nde = (xp_awk_nde_nextfile_t*) MALLOC (
		awk, xp_sizeof(xp_awk_nde_nextfile_t));
	if (nde == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	nde->type = XP_AWK_NDE_NEXTFILE;
	nde->next = XP_NULL;
	
	return (xp_awk_nde_t*)nde;
}

static int __get_token (xp_awk_t* awk)
{
	xp_cint_t c;
	xp_size_t line;
	int n;

	line = awk->token.line;
	do 
	{
		if (__skip_spaces(awk) == -1) return -1;
		if ((n = __skip_comment(awk)) == -1) return -1;
	} 
	while (n == 1);

	xp_str_clear (&awk->token.name);
	awk->token.line = awk->src.lex.line;
	awk->token.column = awk->src.lex.column;

	if (line != 0 && (awk->option & XP_AWK_BLOCKLESS) &&
	    (awk->parse.id.block == PARSE_PATTERN ||
	     awk->parse.id.block == PARSE_BEGIN ||
	     awk->parse.id.block == PARSE_END))
	{
		if (awk->token.line != line)
		{
			SET_TOKEN_TYPE (awk, TOKEN_NEWLINE);
			return 0;
		}
	}

	c = awk->src.lex.curc;

	if (c == XP_CHAR_EOF) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_EOF);
	}	
	else if (xp_isdigit(c)) 
	{
		if (__get_number(awk) == -1) return -1;
	}
	else if (xp_isalpha(c) || c == XP_T('_')) 
	{
		int type;

		/* identifier */
		do 
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		} 
		while (xp_isalpha(c) || c == XP_T('_') || xp_isdigit(c));

		type = __classify_ident (awk, 
			XP_STR_BUF(&awk->token.name), 
			XP_STR_LEN(&awk->token.name));
		SET_TOKEN_TYPE (awk, type);
	}
	else if (c == XP_T('\"')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_STR);

		if (__get_charstr(awk) == -1) return -1;

		while (awk->option & XP_AWK_STRCONCAT) 
		{
			do 
			{
				if (__skip_spaces(awk) == -1) return -1;
				if ((n = __skip_comment(awk)) == -1) return -1;
			} 
			while (n == 1);

			c = awk->src.lex.curc;
			if (c != XP_T('\"')) break;

			if (__get_charstr(awk) == -1) return -1;
		}

	}
	else if (c == XP_T('=')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == XP_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_EQ);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_ASSIGN);
		}
	}
	else if (c == XP_T('!')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == XP_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_NE);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == XP_T('~'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_NM);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_NOT);
		}
	}
	else if (c == XP_T('>')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if ((awk->option & XP_AWK_SHIFT) && c == XP_T('>')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_RSHIFT);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == XP_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_GE);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_GT);
		}
	}
	else if (c == XP_T('<')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if ((awk->option & XP_AWK_SHIFT) && c == XP_T('<')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_LSHIFT);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == XP_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_LE);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_LT);
		}
	}
	else if (c == XP_T('|'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == XP_T('|'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_LOR);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == XP_T('&'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_BORAND);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_BOR);
		}
	}
	else if (c == XP_T('&'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == XP_T('&'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_LAND);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_BAND);
		}
	}
	else if (c == XP_T('~'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_TILDE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T('^'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_BXOR);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T('+')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == XP_T('+')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_PLUSPLUS);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == XP_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_PLUS_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_PLUS);
		}
	}
	else if (c == XP_T('-')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == XP_T('-')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_MINUSMINUS);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == XP_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_MINUS_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else 
		{
			SET_TOKEN_TYPE (awk, TOKEN_MINUS);
		}
	}
	else if (c == XP_T('*')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == XP_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_MUL_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == XP_T('*'))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
			if (c == XP_T('='))
			{
				SET_TOKEN_TYPE (awk, TOKEN_EXP_ASSIGN);
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
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
	else if (c == XP_T('/')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == XP_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_DIV_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_DIV);
		}
	}
	else if (c == XP_T('%')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == XP_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_MOD_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_MOD);
		}
	}
	else if (c == XP_T('(')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_LPAREN);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T(')')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_RPAREN);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T('{')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_LBRACE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T('}')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_RBRACE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T('[')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_LBRACK);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T(']')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_RBRACK);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T('$')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_DOLLAR);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T(',')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_COMMA);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T('.'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_PERIOD);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T(';')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_SEMICOLON);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T(':'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_COLON);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_T('?'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_QUEST);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else 
	{
		awk->errnum = XP_AWK_ELXCHR;
		return -1;
	}

/*xp_printf (XP_T("token -> [%s]\n"), XP_STR_BUF(&awk->token.name));*/
	return 0;
}

static int __get_number (xp_awk_t* awk)
{
	xp_cint_t c;

	xp_assert (XP_STR_LEN(&awk->token.name) == 0);
	SET_TOKEN_TYPE (awk, TOKEN_INT);

	c = awk->src.lex.curc;

	if (c == XP_T('0'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == XP_T('x') || c == XP_T('X'))
		{
			/* hexadecimal number */
			do 
			{
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			} 
			while (xp_isxdigit(c));

			return 0;
		}
		else if (c == XP_T('b') || c == XP_T('B'))
		{
			/* binary number */
			do
			{
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			} 
			while (c == XP_T('0') || c == XP_T('1'));

			return 0;
		}
		else if (c != '.')
		{
			/* octal number */
			while (c >= XP_T('0') && c <= XP_T('7'))
			{
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			}

			return 0;
		}
	}

	while (xp_isdigit(c)) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	} 

	if (c == XP_T('.'))
	{
		/* floating-point number */
		SET_TOKEN_TYPE (awk, TOKEN_REAL);

		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		while (xp_isdigit(c))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
	}

	if (c == XP_T('E') || c == XP_T('e'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_REAL);

		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == XP_T('+') || c == XP_T('-'))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}

		while (xp_isdigit(c))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
	}

	return 0;
}

static int __get_charstr (xp_awk_t* awk)
{
	if (awk->src.lex.curc != XP_T('\"')) 
	{
		/* the starting quote has been consumed before this function
		 * has been called */
		ADD_TOKEN_CHAR (awk, awk->src.lex.curc);
	}
	return __get_string (awk, XP_T('\"'), XP_T('\\'), xp_false);
}

static int __get_rexstr (xp_awk_t* awk)
{
	if (awk->src.lex.curc == XP_T('/')) 
	{
		/* this part of the function is different from __get_charstr
		 * because of the way this function is called */
		GET_CHAR (awk);
		return 0;
	}
	else 
	{
		ADD_TOKEN_CHAR (awk, awk->src.lex.curc);
		return __get_string (awk, XP_T('/'), XP_T('\\'), xp_true);
	}
}

static int __get_string (
	xp_awk_t* awk, xp_char_t end_char, 
	xp_char_t esc_char, xp_bool_t keep_esc_char)
{
	xp_cint_t c;
	int escaped = 0;
	int digit_count = 0;
	xp_cint_t c_acc;

	while (1)
	{
		GET_CHAR_TO (awk, c);

		if (c == XP_CHAR_EOF)
		{
			awk->errnum = XP_AWK_EENDSTR;
			return -1;
		}

		if (escaped == 3)
		{
			if (c >= XP_T('0') && c <= XP_T('7'))
			{
				c_acc = c_acc * 8 + c - XP_T('0');
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
			if (c >= XP_T('0') && c <= XP_T('9'))
			{
				c_acc = c_acc * 16 + c - XP_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= XP_T('A') && c <= XP_T('F'))
			{
				c_acc = c_acc * 16 + c - XP_T('A') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= XP_T('a') && c <= XP_T('f'))
			{
				c_acc = c_acc * 16 + c - XP_T('a') + 10;
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
				xp_char_t rc;

				rc = (escaped == 2)? XP_T('x'):
				     (escaped == 4)? XP_T('u'): XP_T('U');

				if (digit_count == 0) ADD_TOKEN_CHAR (awk, rc);
				else ADD_TOKEN_CHAR (awk, c_acc);

				escaped = 0;
			}
		}

		if (escaped == 0 && c == end_char)
		{
			/* terminating quote */
			GET_CHAR_TO (awk, c);
			break;
		}

		if (escaped == 0 && c == esc_char)
		{
			escaped = 1;
			continue;
		}

		if (escaped == 1)
		{
			if (c == XP_T('n')) c = XP_T('\n');
			else if (c == XP_T('r')) c = XP_T('\r');
			else if (c == XP_T('t')) c = XP_T('\t');
			else if (c == XP_T('f')) c = XP_T('\f');
			else if (c == XP_T('b')) c = XP_T('\b');
			else if (c == XP_T('v')) c = XP_T('\v');
			else if (c == XP_T('a')) c = XP_T('\a');
			else if (c >= XP_T('0') && c <= XP_T('7')) 
			{
				escaped = 3;
				digit_count = 1;
				c_acc = c - XP_T('0');
				continue;
			}
			else if (c == XP_T('x')) 
			{
				escaped = 2;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#ifdef XP_CHAR_IS_WCHAR
			else if (c == XP_T('u') && xp_sizeof(xp_char_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == XP_T('U') && xp_sizeof(xp_char_t) >= 4) 
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

static int __get_char (xp_awk_t* awk)
{
	xp_ssize_t n;
	/*xp_char_t c;*/

	if (awk->src.lex.ungotc_count > 0) 
	{
		awk->src.lex.curc = awk->src.lex.ungotc[--awk->src.lex.ungotc_count];
		return 0;
	}

	if (awk->src.shared.buf_pos >= awk->src.shared.buf_len)
	{
		n = awk->src.ios->in (
			XP_AWK_IO_READ, awk->src.ios->custom_data,
			awk->src.shared.buf, xp_countof(awk->src.shared.buf));
		if (n == -1)
		{
			awk->errnum = XP_AWK_ESRCINREAD;
			return -1;
		}

		if (n == 0) 
		{
			awk->src.lex.curc = XP_CHAR_EOF;
			return 0;
		}

		awk->src.shared.buf_pos = 0;
		awk->src.shared.buf_len = n;	
	}

	awk->src.lex.curc = awk->src.shared.buf[awk->src.shared.buf_pos++];

	if (awk->src.lex.curc == XP_T('\n'))
	{
		awk->src.lex.line++;
		awk->src.lex.column = 1;
	}
	else awk->src.lex.column++;

	return 0;
}

static int __unget_char (xp_awk_t* awk, xp_cint_t c)
{
	if (awk->src.lex.ungotc_count >= xp_countof(awk->src.lex.ungotc)) 
	{
		awk->errnum = XP_AWK_ELXUNG;
		return -1;
	}

	awk->src.lex.ungotc[awk->src.lex.ungotc_count++] = c;
	return 0;
}

static int __skip_spaces (xp_awk_t* awk)
{
	xp_cint_t c = awk->src.lex.curc;

	while (xp_isspace(c)) GET_CHAR_TO (awk, c);
	return 0;
}

static int __skip_comment (xp_awk_t* awk)
{
	xp_cint_t c = awk->src.lex.curc;

	if ((awk->option & XP_AWK_HASHSIGN) && c == XP_T('#'))
	{
		do 
		{ 
			GET_CHAR_TO (awk, c);
		} 
		while (c != XP_T('\n') && c != XP_CHAR_EOF);

		GET_CHAR (awk);
		return 1; /* comment by # */
	}

	if (c != XP_T('/')) return 0; /* not a comment */
	GET_CHAR_TO (awk, c);

	if ((awk->option & XP_AWK_DBLSLASHES) && c == XP_T('/')) 
	{
		do 
		{ 
			GET_CHAR_TO (awk, c);
		} 
		while (c != XP_T('\n') && c != XP_CHAR_EOF);

		GET_CHAR (awk);
		return 1; /* comment by // */
	}
	else if (c == XP_T('*')) 
	{
		do 
		{
			GET_CHAR_TO (awk, c);
			if (c == XP_CHAR_EOF)
			{
				awk->errnum = XP_AWK_EENDCOMMENT;
				return -1;
			}

			if (c == XP_T('*')) 
			{
				GET_CHAR_TO (awk, c);
				if (c == XP_CHAR_EOF)
				{
					awk->errnum = XP_AWK_EENDCOMMENT;
					return -1;
				}

				if (c == XP_T('/')) 
				{
					GET_CHAR_TO (awk, c);
					break;
				}
			}
		} 
		while (1);

		return 1; /* c-style comment */
	}

	if (__unget_char(awk,c) == -1) return -1; /* error */
	awk->src.lex.curc = XP_T('/');

	return 0;
}

static int __classify_ident (
	xp_awk_t* awk, const xp_char_t* name, xp_size_t len)
{
	struct __kwent* kwp;

	for (kwp = __kwtab; kwp->name != XP_NULL; kwp++) 
	{
		if (kwp->valid != 0 && 
		    (awk->option & kwp->valid) == 0) continue;

		if (xp_strxncmp (kwp->name, kwp->name_len, name, len) == 0) 
		{
			return kwp->type;
		}
	}

	return TOKEN_IDENT;
}

static int __assign_to_opcode (xp_awk_t* awk)
{
	static int __assop[] =
	{
		XP_AWK_ASSOP_NONE,
		XP_AWK_ASSOP_PLUS,
		XP_AWK_ASSOP_MINUS,
		XP_AWK_ASSOP_MUL,
		XP_AWK_ASSOP_DIV,
		XP_AWK_ASSOP_MOD,
		XP_AWK_ASSOP_EXP
	};

	if (awk->token.type >= TOKEN_ASSIGN &&
	    awk->token.type <= TOKEN_EXP_ASSIGN)
	{
		return __assop[awk->token.type - TOKEN_ASSIGN];
	}

	return -1;
}

static int __is_plain_var (xp_awk_nde_t* nde)
{
	return nde->type == XP_AWK_NDE_GLOBAL ||
	       nde->type == XP_AWK_NDE_LOCAL ||
	       nde->type == XP_AWK_NDE_ARG ||
	       nde->type == XP_AWK_NDE_NAMED;
}

static int __is_var (xp_awk_nde_t* nde)
{
	return nde->type == XP_AWK_NDE_GLOBAL ||
	       nde->type == XP_AWK_NDE_LOCAL ||
	       nde->type == XP_AWK_NDE_ARG ||
	       nde->type == XP_AWK_NDE_NAMED ||
	       nde->type == XP_AWK_NDE_GLOBALIDX ||
	       nde->type == XP_AWK_NDE_LOCALIDX ||
	       nde->type == XP_AWK_NDE_ARGIDX ||
	       nde->type == XP_AWK_NDE_NAMEDIDX;
}

struct __deparse_func_t 
{
	xp_awk_t* awk;
	xp_char_t* tmp;
	xp_size_t tmp_len;
};

static int __deparse (xp_awk_t* awk)
{
	xp_awk_chain_t* chain;
	xp_char_t tmp[64];
	struct __deparse_func_t df;
	int n;

	xp_assert (awk->src.ios->out != XP_NULL);

	awk->src.shared.buf_len = 0;
	awk->src.shared.buf_pos = 0;

/* TODO: more error handling */
	if (awk->src.ios->out (
		XP_AWK_IO_OPEN, awk->src.ios->custom_data, XP_NULL, 0) == -1)
	{
		awk->errnum = XP_AWK_ESRCOUTOPEN;
		return -1;
	}

#define EXIT_DEPARSE(num) \
	do { n = -1; awk->errnum = num ; goto exit_deparse; } while(0)

	if (awk->tree.nglobals > awk->tree.nbglobals) 
	{
		xp_size_t i;

		xp_assert (awk->tree.nglobals > 0);
		if (xp_awk_putsrcstr (awk, XP_T("global ")) == -1)
			EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);

		for (i = awk->tree.nbglobals; i < awk->tree.nglobals - 1; i++) 
		{
			xp_sprintf (tmp, xp_countof(tmp), 
				XP_T("__global%lu, "), (unsigned long)i);
			if (xp_awk_putsrcstr (awk, tmp) == -1)
				EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);
		}

		xp_sprintf (tmp, xp_countof(tmp),
			XP_T("__global%lu;\n\n"), (unsigned long)i);
		if (xp_awk_putsrcstr (awk, tmp) == -1)
			EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);
	}

	df.awk = awk;
	df.tmp = tmp;
	df.tmp_len = xp_countof(tmp);

	if (xp_awk_map_walk (&awk->tree.afns, __deparse_func, &df) == -1) 
	{
		EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);
	}

	if (awk->tree.begin != XP_NULL) 
	{
		if (xp_awk_putsrcstr (awk, XP_T("BEGIN ")) == -1)
			EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);

		if (xp_awk_prnpt (awk, awk->tree.begin) == -1)
			EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);

		if (__put_char (awk, XP_T('\n')) == -1)
			EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);
	}

	chain = awk->tree.chain;
	while (chain != XP_NULL) 
	{
		if (chain->pattern != XP_NULL) 
		{
			if (xp_awk_prnptnpt (awk, chain->pattern) == -1)
				EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);
		}

		if (chain->action == XP_NULL) 
		{
			/* blockless pattern */
			if (__put_char (awk, XP_T('\n')) == -1)
				EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);
		}
		else 
		{
			if (xp_awk_prnpt (awk, chain->action) == -1)
				EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);
		}

		if (__put_char (awk, XP_T('\n')) == -1)
			EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);

		chain = chain->next;	
	}

	if (awk->tree.end != XP_NULL) 
	{
		if (xp_awk_putsrcstr (awk, XP_T("END ")) == -1)
			EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);
		if (xp_awk_prnpt (awk, awk->tree.end) == -1)
			EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);
	}

	if (__flush (awk) == -1) EXIT_DEPARSE (XP_AWK_ESRCOUTWRITE);

exit_deparse:
	if (awk->src.ios->out (
		XP_AWK_IO_CLOSE, awk->src.ios->custom_data, XP_NULL, 0) == -1)
	{
		if (n != -1)
		{
			awk->errnum = XP_AWK_ESRCOUTOPEN;
			n = -1;
		}
	}

	return 0;
}

static int __deparse_func (xp_awk_pair_t* pair, void* arg)
{
	struct __deparse_func_t* df = (struct __deparse_func_t*)arg;
	xp_awk_afn_t* afn = (xp_awk_afn_t*)pair->val;
	xp_size_t i;

	xp_assert (xp_strxncmp (
		pair->key, pair->key_len, afn->name, afn->name_len) == 0);

	if (xp_awk_putsrcstr (df->awk, XP_T("function ")) == -1) return -1;
	if (xp_awk_putsrcstr (df->awk, afn->name) == -1) return -1;
	if (xp_awk_putsrcstr (df->awk, XP_T(" (")) == -1) return -1;

	for (i = 0; i < afn->nargs; ) 
	{
		xp_sprintf (df->tmp, df->tmp_len, 
			XP_T("__param%lu"), (unsigned long)i++);
		if (xp_awk_putsrcstr (df->awk, df->tmp) == -1) return -1;
		if (i >= afn->nargs) break;
		if (xp_awk_putsrcstr (df->awk, XP_T(", ")) == -1) return -1;
	}

	if (xp_awk_putsrcstr (df->awk, XP_T(")\n")) == -1) return -1;

	if (xp_awk_prnpt (df->awk, afn->body) == -1) return -1;
	if (xp_awk_putsrcstr (df->awk, XP_T("\n")) == -1) return -1;

	return 0;
}

static int __put_char (xp_awk_t* awk, xp_char_t c)
{
	awk->src.shared.buf[awk->src.shared.buf_len++] = c;
	if (awk->src.shared.buf_len >= xp_countof(awk->src.shared.buf))
	{
		if (__flush (awk) == -1) return -1;
	}
	return 0;
}

static int __flush (xp_awk_t* awk)
{
	xp_ssize_t n;

	xp_assert (awk->src.ios->out != XP_NULL);

	while (awk->src.shared.buf_pos < awk->src.shared.buf_len)
	{
		n = awk->src.ios->out (
			XP_AWK_IO_WRITE, awk->src.ios->custom_data,
			&awk->src.shared.buf[awk->src.shared.buf_pos], 
			awk->src.shared.buf_len - awk->src.shared.buf_pos);
		if (n <= 0) return -1;

		awk->src.shared.buf_pos += n;
	}

	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;
	return 0;
}

int xp_awk_putsrcstr (xp_awk_t* awk, const xp_char_t* str)
{
	while (*str != XP_T('\0'))
	{
		if (__put_char (awk, *str) == -1) return -1;
		str++;
	}

	return 0;
}

int xp_awk_putsrcstrx (
	xp_awk_t* awk, const xp_char_t* str, xp_size_t len)
{
	const xp_char_t* end = str + len;

	while (str < end)
	{
		if (__put_char (awk, *str) == -1) return -1;
		str++;
	}

	return 0;
}

