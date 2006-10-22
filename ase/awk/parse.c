/*
 * $Id: parse.c,v 1.192 2006-10-22 11:34:53 bacon Exp $
 */

#include <sse/awk/awk_i.h>

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
	TOKEN_ESSE_ASSIGN,

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
	TOKEN_ESSE,

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

static sse_awk_t* __parse_progunit (sse_awk_t* awk);
static sse_awk_t* __collect_globals (sse_awk_t* awk);
static sse_awk_t* __add_builtin_globals (sse_awk_t* awk);
static sse_awk_t* __add_global (
	sse_awk_t* awk, const sse_char_t* name, sse_size_t name_len);
static sse_awk_t* __collect_locals (sse_awk_t* awk, sse_size_t nlocals);

static sse_awk_nde_t* __parse_function (sse_awk_t* awk);
static sse_awk_nde_t* __parse_begin (sse_awk_t* awk);
static sse_awk_nde_t* __parse_end (sse_awk_t* awk);
static sse_awk_chain_t* __parse_pattern_block (
	sse_awk_t* awk, sse_awk_nde_t* ptn, sse_bool_t blockless);

static sse_awk_nde_t* __parse_block (sse_awk_t* awk, sse_bool_t is_top);
static sse_awk_nde_t* __parse_statement (sse_awk_t* awk);
static sse_awk_nde_t* __parse_statement_nb (sse_awk_t* awk);
static sse_awk_nde_t* __parse_esseression (sse_awk_t* awk);

static sse_awk_nde_t* __parse_basic_esser (sse_awk_t* awk);

static sse_awk_nde_t* __parse_binary_esser (
	sse_awk_t* awk, const __binmap_t* binmap,
	sse_awk_nde_t*(*next_level_func)(sse_awk_t*));

static sse_awk_nde_t* __parse_logical_or (sse_awk_t* awk);
static sse_awk_nde_t* __parse_logical_and (sse_awk_t* awk);
static sse_awk_nde_t* __parse_in (sse_awk_t* awk);
static sse_awk_nde_t* __parse_regex_match (sse_awk_t* awk);
static sse_awk_nde_t* __parse_bitwise_or (sse_awk_t* awk);
static sse_awk_nde_t* __parse_bitwise_or_with_extio (sse_awk_t* awk);
static sse_awk_nde_t* __parse_bitwise_xor (sse_awk_t* awk);
static sse_awk_nde_t* __parse_bitwise_and (sse_awk_t* awk);
static sse_awk_nde_t* __parse_equality (sse_awk_t* awk);
static sse_awk_nde_t* __parse_relational (sse_awk_t* awk);
static sse_awk_nde_t* __parse_shift (sse_awk_t* awk);
static sse_awk_nde_t* __parse_concat (sse_awk_t* awk);
static sse_awk_nde_t* __parse_additive (sse_awk_t* awk);
static sse_awk_nde_t* __parse_multiplicative (sse_awk_t* awk);

static sse_awk_nde_t* __parse_unary (sse_awk_t* awk);
static sse_awk_nde_t* __parse_increment (sse_awk_t* awk);
static sse_awk_nde_t* __parse_primary (sse_awk_t* awk);
static sse_awk_nde_t* __parse_primary_ident (sse_awk_t* awk);

static sse_awk_nde_t* __parse_hashidx (
	sse_awk_t* awk, sse_char_t* name, sse_size_t name_len);
static sse_awk_nde_t* __parse_fncall (
	sse_awk_t* awk, sse_char_t* name, sse_size_t name_len, sse_awk_bfn_t* bfn);
static sse_awk_nde_t* __parse_if (sse_awk_t* awk);
static sse_awk_nde_t* __parse_while (sse_awk_t* awk);
static sse_awk_nde_t* __parse_for (sse_awk_t* awk);
static sse_awk_nde_t* __parse_dowhile (sse_awk_t* awk);
static sse_awk_nde_t* __parse_break (sse_awk_t* awk);
static sse_awk_nde_t* __parse_continue (sse_awk_t* awk);
static sse_awk_nde_t* __parse_return (sse_awk_t* awk);
static sse_awk_nde_t* __parse_exit (sse_awk_t* awk);
static sse_awk_nde_t* __parse_next (sse_awk_t* awk);
static sse_awk_nde_t* __parse_nextfile (sse_awk_t* awk);
static sse_awk_nde_t* __parse_delete (sse_awk_t* awk);
static sse_awk_nde_t* __parse_print (sse_awk_t* awk);
static sse_awk_nde_t* __parse_printf (sse_awk_t* awk);

static int __get_token (sse_awk_t* awk);
static int __get_number (sse_awk_t* awk);
static int __get_charstr (sse_awk_t* awk);
static int __get_rexstr (sse_awk_t* awk);
static int __get_string (
	sse_awk_t* awk, sse_char_t end_char,
	sse_char_t esc_char, sse_bool_t keep_esc_char);
static int __get_char (sse_awk_t* awk);
static int __unget_char (sse_awk_t* awk, sse_cint_t c);
static int __skip_spaces (sse_awk_t* awk);
static int __skip_comment (sse_awk_t* awk);
static int __classify_ident (
	sse_awk_t* awk, const sse_char_t* name, sse_size_t len);
static int __assign_to_opcode (sse_awk_t* awk);
static int __is_plain_var (sse_awk_nde_t* nde);
static int __is_var (sse_awk_nde_t* nde);

static int __deparse (sse_awk_t* awk);
static int __deparse_func (sse_awk_pair_t* pair, void* arg);
static int __put_char (sse_awk_t* awk, sse_char_t c);
static int __flush (sse_awk_t* awk);

struct __kwent 
{ 
	const sse_char_t* name; 
	sse_size_t name_len;
	int type; 
	int valid; /* the entry is valid when this option is set */
};

static struct __kwent __kwtab[] = 
{
	/* operators */
	{ SSE_T("in"),       2, TOKEN_IN,       0 },

	/* top-level block starters */
	{ SSE_T("BEGIN"),    5, TOKEN_BEGIN,    0 },
	{ SSE_T("END"),      3, TOKEN_END,      0 },
	{ SSE_T("function"), 8, TOKEN_FUNCTION, 0 },
	{ SSE_T("func"),     4, TOKEN_FUNCTION, 0 },

	/* keywords for variable declaration */
	{ SSE_T("local"),    5, TOKEN_LOCAL,    SSE_AWK_ESSELICIT },
	{ SSE_T("global"),   6, TOKEN_GLOBAL,   SSE_AWK_ESSELICIT },

	/* keywords that start statements excluding esseression statements */
	{ SSE_T("if"),       2, TOKEN_IF,       0 },
	{ SSE_T("else"),     4, TOKEN_ELSE,     0 },
	{ SSE_T("while"),    5, TOKEN_WHILE,    0 },
	{ SSE_T("for"),      3, TOKEN_FOR,      0 },
	{ SSE_T("do"),       2, TOKEN_DO,       0 },
	{ SSE_T("break"),    5, TOKEN_BREAK,    0 },
	{ SSE_T("continue"), 8, TOKEN_CONTINUE, 0 },
	{ SSE_T("return"),   6, TOKEN_RETURN,   0 },
	{ SSE_T("exit"),     4, TOKEN_EXIT,     0 },
	{ SSE_T("next"),     4, TOKEN_NEXT,     0 },
	{ SSE_T("nextfile"), 8, TOKEN_NEXTFILE, 0 },
	{ SSE_T("delete"),   6, TOKEN_DELETE,   0 },
	{ SSE_T("print"),    5, TOKEN_PRINT,    SSE_AWK_EXTIO },
	{ SSE_T("printf"),   6, TOKEN_PRINTF,   SSE_AWK_EXTIO },

	/* keywords that can start an esseression */
	{ SSE_T("getline"),  7, TOKEN_GETLINE,  SSE_AWK_EXTIO },

	{ SSE_NULL,          0,              0 }
};

struct __bvent
{
	const sse_char_t* name;
	sse_size_t name_len;
	int valid;
};

static struct __bvent __bvtab[] =
{
	{ SSE_T("ARGC"),         4, 0 },
	{ SSE_T("ARGV"),         4, 0 },
	{ SSE_T("CONVFMT"),      7, 0 },
	{ SSE_T("ENVIRON"),      7, 0 },
	{ SSE_T("ERRNO"),        5, 0 },
	{ SSE_T("FILENAME"),     8, 0 },
	{ SSE_T("FNR"),          3, 0 },
	{ SSE_T("FS"),           2, 0 },
	{ SSE_T("IGNORECASE"),  10, 0 },
	{ SSE_T("NF"),           2, 0 },
	{ SSE_T("NR"),           2, 0 },
	{ SSE_T("OFMT"),         4, 0 },
	{ SSE_T("OFS"),          3, 0 },
	{ SSE_T("ORS"),          3, 0 },
	{ SSE_T("RS"),           2, 0 },
	{ SSE_T("RT"),           2, 0 },
	{ SSE_T("RSTART"),       6, 0 },
	{ SSE_T("RLENGTH"),      7, 0 },
	{ SSE_T("SUBSEP"),       6, 0 },
	{ SSE_NULL,              0, 0 }
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
		if (sse_awk_str_ccat(&(awk)->token.name,(c)) == (sse_size_t)-1) { \
			(awk)->errnum = SSE_AWK_ENOMEM; return -1; \
		} \
	} while (0)

#define ADD_TOKEN_STR(awk,str) \
	do { \
		if (sse_awk_str_cat(&(awk)->token.name,(str)) == (sse_size_t)-1) { \
			(awk)->errnum = SSE_AWK_ENOMEM; return -1; \
		} \
	} while (0)

#define MATCH(awk,token_type) ((awk)->token.type == (token_type))

#define PANIC(awk,code) \
	do { (awk)->errnum = (code); return SSE_NULL; } while (0)

int sse_awk_parse (sse_awk_t* awk, sse_awk_srcios_t* srcios)
{
	int n = 0, op;

	sse_awk_assert (awk, srcios != SSE_NULL && srcios->in != SSE_NULL);

	sse_awk_clear (awk);
	awk->src.ios = srcios;

	op = awk->src.ios->in (
		SSE_AWK_IO_OPEN, awk->src.ios->custom_data, SSE_NULL, 0);
	if (op == -1)
	{
		/* cannot open the source file.
		 * it doesn't even have to call CLOSE */
		awk->errnum = SSE_AWK_ESRCINOPEN;
		return -1;
	}

	if (__add_builtin_globals (awk) == SSE_NULL) 
	{
		n = -1;
		goto exit_parse;
	}

	/* the user io handler for the source code input returns 0 when
	 * it doesn't have any files to open. this is the same condition
	 * as the source code file is empty. so it will perform the parsing
	 * when op is positive, which means there are something to parse */
	if (op > 0)
	{
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

			if (__parse_progunit (awk) == SSE_NULL) 
			{
				n = -1;
				goto exit_parse;
			}
		}
	}

	awk->tree.nglobals = sse_awk_tab_getsize(&awk->parse.globals);

	if (awk->src.ios->out != SSE_NULL) 
	{
		if (__deparse (awk) == -1) 
		{
			n = -1;
			goto exit_parse;
		}
	}

exit_parse:
	if (awk->src.ios->in (
		SSE_AWK_IO_CLOSE, awk->src.ios->custom_data, SSE_NULL, 0) == -1)
	{
		if (n != -1)
		{
			/* this is to keep the earlier error above
			 * that might be more critical than this */
			awk->errnum = SSE_AWK_ESRCINCLOSE;
			n = -1;
		}
	}

	if (n == -1) sse_awk_clear (awk);
	return n;
}

static sse_awk_t* __parse_progunit (sse_awk_t* awk)
{
	/*
	pattern { action }
	function name (parameter-list) { statement }
	*/

	sse_awk_assert (awk, awk->parse.depth.loop == 0);

	if ((awk->option & SSE_AWK_ESSELICIT) && MATCH(awk,TOKEN_GLOBAL)) 
	{
		sse_size_t nglobals;

		awk->parse.id.block = PARSE_GLOBAL;

		if (__get_token(awk) == -1) return SSE_NULL;

		nglobals = sse_awk_tab_getsize(&awk->parse.globals);
		if (__collect_globals (awk) == SSE_NULL) 
		{
			sse_awk_tab_remove (
				&awk->parse.globals, nglobals, 
				sse_awk_tab_getsize(&awk->parse.globals) - nglobals);
			return SSE_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_FUNCTION)) 
	{
		awk->parse.id.block = PARSE_FUNCTION;
		if (__parse_function (awk) == SSE_NULL) return SSE_NULL;
	}
	else if (MATCH(awk,TOKEN_BEGIN)) 
	{
		awk->parse.id.block = PARSE_BEGIN;
		if (__get_token(awk) == -1) return SSE_NULL; 

		if ((awk->option & SSE_AWK_BLOCKLESS) &&
		    (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF)))
		{
			/* when the blockless pattern is supported
	   		 * BEGIN and { should be located on the same line */
			PANIC (awk, SSE_AWK_EBEGINBLOCK);
		}

		if (!MATCH(awk,TOKEN_LBRACE)) PANIC (awk, SSE_AWK_ELBRACE);

		awk->parse.id.block = PARSE_BEGIN_BLOCK;
		if (__parse_begin (awk) == SSE_NULL) return SSE_NULL;
	}
	else if (MATCH(awk,TOKEN_END)) 
	{
		awk->parse.id.block = PARSE_END;
		if (__get_token(awk) == -1) return SSE_NULL; 

		if ((awk->option & SSE_AWK_BLOCKLESS) &&
		    (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF)))
		{
			/* when the blockless pattern is supported
	   		 * END and { should be located on the same line */
			PANIC (awk, SSE_AWK_EENDBLOCK);
		}

		if (!MATCH(awk,TOKEN_LBRACE)) PANIC (awk, SSE_AWK_ELBRACE);

		awk->parse.id.block = PARSE_END_BLOCK;
		if (__parse_end (awk) == SSE_NULL) return SSE_NULL;
	}
	else if (MATCH(awk,TOKEN_LBRACE))
	{
		/* patternless block */
		awk->parse.id.block = PARSE_ACTION_BLOCK;
		if (__parse_pattern_block (
			awk, SSE_NULL, sse_false) == SSE_NULL) return SSE_NULL;
	}
	else
	{
		/* 
		esseressions 
		/regular esseression/
		pattern && pattern
		pattern || pattern
		!pattern
		(pattern)
		pattern, pattern
		*/
		sse_awk_nde_t* ptn;

		awk->parse.id.block = PARSE_PATTERN;

		ptn = __parse_esseression (awk);
		if (ptn == SSE_NULL) return SSE_NULL;

		sse_awk_assert (awk, ptn->next == SSE_NULL);

		if (MATCH(awk,TOKEN_COMMA))
		{
			if (__get_token (awk) == -1) 
			{
				sse_awk_clrpt (awk, ptn);
				return SSE_NULL;
			}	

			ptn->next = __parse_esseression (awk);
			if (ptn->next == SSE_NULL) 
			{
				sse_awk_clrpt (awk, ptn);
				return SSE_NULL;
			}
		}

		if ((awk->option & SSE_AWK_BLOCKLESS) &&
		    (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF)))
		{
			/* blockless pattern */
			sse_bool_t newline = MATCH(awk,TOKEN_NEWLINE);

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (__parse_pattern_block (
				awk, ptn, sse_true) == SSE_NULL) 
			{
				sse_awk_clrpt (awk, ptn);
				return SSE_NULL;	
			}

			if (newline)
			{
				if (__get_token (awk) == -1) 
				{
					sse_awk_clrpt (awk, ptn);
					return SSE_NULL;
				}	
			}
		}
		else
		{
			/* parse the action block */
			if (!MATCH(awk,TOKEN_LBRACE))
			{
				sse_awk_clrpt (awk, ptn);
				PANIC (awk, SSE_AWK_ELBRACE);
			}

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (__parse_pattern_block (
				awk, ptn, sse_false) == SSE_NULL) 
			{
				sse_awk_clrpt (awk, ptn);
				return SSE_NULL;	
			}
		}
	}

	return awk;
}

static sse_awk_nde_t* __parse_function (sse_awk_t* awk)
{
	sse_char_t* name;
	sse_char_t* name_dup;
	sse_size_t name_len;
	sse_awk_nde_t* body;
	sse_awk_afn_t* afn;
	sse_size_t nargs;
	sse_awk_pair_t* pair;
	int n;

	/* eat up the keyword 'function' and get the next token */
	sse_awk_assert (awk, MATCH(awk,TOKEN_FUNCTION));
	if (__get_token(awk) == -1) return SSE_NULL;  

	/* match a function name */
	if (!MATCH(awk,TOKEN_IDENT)) 
	{
		/* cannot find a valid identifier for a function name */
		PANIC (awk, SSE_AWK_EIDENT);
	}

	name = SSE_AWK_STR_BUF(&awk->token.name);
	name_len = SSE_AWK_STR_LEN(&awk->token.name);
	if (sse_awk_map_get(&awk->tree.afns, name, name_len) != SSE_NULL) 
	{
		/* the function is defined previously */
		PANIC (awk, SSE_AWK_EDUPFUNC);
	}

	if (awk->option & SSE_AWK_UNIQUE) 
	{
		/* check if it coincides to be a global variable name */
		if (sse_awk_tab_find (
			&awk->parse.globals, 0, name, name_len) != (sse_size_t)-1) 
		{
			PANIC (awk, SSE_AWK_EDUPNAME);
		}
	}

	/* clone the function name before it is overwritten */
	name_dup = sse_awk_strxdup (awk, name, name_len);
	if (name_dup == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);

	/* get the next token */
	if (__get_token(awk) == -1) 
	{
		SSE_AWK_FREE (awk, name_dup);
		return SSE_NULL;  
	}

	/* match a left parenthesis */
	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		/* a function name is not followed by a left parenthesis */
		SSE_AWK_FREE (awk, name_dup);
		PANIC (awk, SSE_AWK_ELPAREN);
	}	

	/* get the next token */
	if (__get_token(awk) == -1) 
	{
		SSE_AWK_FREE (awk, name_dup);
		return SSE_NULL;
	}

	/* make sure that parameter table is empty */
	sse_awk_assert (awk, sse_awk_tab_getsize(&awk->parse.params) == 0);

	/* read parameter list */
	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no function parameter found. get the next token */
		if (__get_token(awk) == -1) 
		{
			SSE_AWK_FREE (awk, name_dup);
			return SSE_NULL;
		}
	}
	else 
	{
		while (1) 
		{
			sse_char_t* param;
			sse_size_t param_len;

			if (!MATCH(awk,TOKEN_IDENT)) 
			{
				SSE_AWK_FREE (awk, name_dup);
				sse_awk_tab_clear (&awk->parse.params);
				PANIC (awk, SSE_AWK_EIDENT);
			}

			param = SSE_AWK_STR_BUF(&awk->token.name);
			param_len = SSE_AWK_STR_LEN(&awk->token.name);

			if (awk->option & SSE_AWK_UNIQUE) 
			{
				/* check if a parameter conflicts with a function */
				if (sse_awk_strxncmp (name_dup, name_len, param, param_len) == 0 ||
				    sse_awk_map_get (&awk->tree.afns, param, param_len) != SSE_NULL) 
				{
					SSE_AWK_FREE (awk, name_dup);
					sse_awk_tab_clear (&awk->parse.params);
					PANIC (awk, SSE_AWK_EDUPNAME);
				}

				/* NOTE: the following is not a conflict
				 *  global x; 
				 *  function f (x) { print x; } 
				 *  x in print x is a parameter
				 */
			}

			/* check if a parameter conflicts with other parameters */
			if (sse_awk_tab_find (
				&awk->parse.params, 
				0, param, param_len) != (sse_size_t)-1) 
			{
				SSE_AWK_FREE (awk, name_dup);
				sse_awk_tab_clear (&awk->parse.params);
				PANIC (awk, SSE_AWK_EDUPPARAM);
			}

			/* push the parameter to the parameter list */
			if (sse_awk_tab_getsize (
				&awk->parse.params) >= SSE_AWK_MAX_PARAMS)
			{
				SSE_AWK_FREE (awk, name_dup);
				sse_awk_tab_clear (&awk->parse.params);
				PANIC (awk, SSE_AWK_ETOOMANYPARAMS);
			}

			if (sse_awk_tab_add (
				&awk->parse.params, 
				param, param_len) == (sse_size_t)-1) 
			{
				SSE_AWK_FREE (awk, name_dup);
				sse_awk_tab_clear (&awk->parse.params);
				PANIC (awk, SSE_AWK_ENOMEM);
			}	

			if (__get_token (awk) == -1) 
			{
				SSE_AWK_FREE (awk, name_dup);
				sse_awk_tab_clear (&awk->parse.params);
				return SSE_NULL;
			}	

			if (MATCH(awk,TOKEN_RPAREN)) break;

			if (!MATCH(awk,TOKEN_COMMA)) 
			{
				SSE_AWK_FREE (awk, name_dup);
				sse_awk_tab_clear (&awk->parse.params);
				PANIC (awk, SSE_AWK_ECOMMA);
			}

			if (__get_token(awk) == -1) 
			{
				SSE_AWK_FREE (awk, name_dup);
				sse_awk_tab_clear (&awk->parse.params);
				return SSE_NULL;
			}
		}

		if (__get_token(awk) == -1) 
		{
			SSE_AWK_FREE (awk, name_dup);
			sse_awk_tab_clear (&awk->parse.params);
			return SSE_NULL;
		}
	}

	/* check if the function body starts with a left brace */
	if (!MATCH(awk,TOKEN_LBRACE)) 
	{
		SSE_AWK_FREE (awk, name_dup);
		sse_awk_tab_clear (&awk->parse.params);
		PANIC (awk, SSE_AWK_ELBRACE);
	}
	if (__get_token(awk) == -1) 
	{
		SSE_AWK_FREE (awk, name_dup);
		sse_awk_tab_clear (&awk->parse.params);
		return SSE_NULL; 
	}

	/* actual function body */
	body = __parse_block (awk, sse_true);
	if (body == SSE_NULL) 
	{
		SSE_AWK_FREE (awk, name_dup);
		sse_awk_tab_clear (&awk->parse.params);
		return SSE_NULL;
	}

	/* TODO: study furthur if the parameter names should be saved 
	 *       for some reasons */
	nargs = sse_awk_tab_getsize (&awk->parse.params);
	/* parameter names are not required anymore. clear them */
	sse_awk_tab_clear (&awk->parse.params);

	afn = (sse_awk_afn_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_afn_t));
	if (afn == SSE_NULL) 
	{
		SSE_AWK_FREE (awk, name_dup);
		sse_awk_clrpt (awk, body);
		return SSE_NULL;
	}

	afn->name = SSE_NULL; /* function name set below */
	afn->name_len = 0;
	afn->nargs = nargs;
	afn->body  = body;

	n = sse_awk_map_putx (&awk->tree.afns, name_dup, name_len, afn, &pair);
	if (n < 0)
	{
		SSE_AWK_FREE (awk, name_dup);
		sse_awk_clrpt (awk, body);
		SSE_AWK_FREE (awk, afn);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	/* duplicate functions should have been detected previously */
	sse_awk_assert (awk, n != 0); 

	afn->name = pair->key; /* do some trick to save a string.  */
	afn->name_len = pair->key_len;
	SSE_AWK_FREE (awk, name_dup);

	return body;
}

static sse_awk_nde_t* __parse_begin (sse_awk_t* awk)
{
	sse_awk_nde_t* nde;

	sse_awk_assert (awk, MATCH(awk,TOKEN_LBRACE));

	if (__get_token(awk) == -1) return SSE_NULL; 
	nde = __parse_block(awk, sse_true);
	if (nde == SSE_NULL) return SSE_NULL;

	awk->tree.begin = nde;
	return nde;
}

static sse_awk_nde_t* __parse_end (sse_awk_t* awk)
{
	sse_awk_nde_t* nde;

	sse_awk_assert (awk, MATCH(awk,TOKEN_LBRACE));

	if (__get_token(awk) == -1) return SSE_NULL; 
	nde = __parse_block(awk, sse_true);
	if (nde == SSE_NULL) return SSE_NULL;

	awk->tree.end = nde;
	return nde;
}

static sse_awk_chain_t* __parse_pattern_block (
	sse_awk_t* awk, sse_awk_nde_t* ptn, sse_bool_t blockless)
{
	sse_awk_nde_t* nde;
	sse_awk_chain_t* chain;

	if (blockless) nde = SSE_NULL;
	else
	{
		sse_awk_assert (awk, MATCH(awk,TOKEN_LBRACE));
		if (__get_token(awk) == -1) return SSE_NULL; 
		nde = __parse_block(awk, sse_true);
		if (nde == SSE_NULL) return SSE_NULL;
	}

	chain = (sse_awk_chain_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_chain_t));
	if (chain == SSE_NULL) 
	{
		sse_awk_clrpt (awk, nde);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	chain->pattern = ptn;
	chain->action = nde;
	chain->next = SSE_NULL;

	if (awk->tree.chain == SSE_NULL) 
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

static sse_awk_nde_t* __parse_block (sse_awk_t* awk, sse_bool_t is_top) 
{
	sse_awk_nde_t* head, * curr, * nde;
	sse_awk_nde_blk_t* block;
	sse_size_t nlocals, nlocals_max, tmp;

	nlocals = sse_awk_tab_getsize(&awk->parse.locals);
	nlocals_max = awk->parse.nlocals_max;

	/* local variable declarations */
	if (awk->option & SSE_AWK_ESSELICIT) 
	{
		while (1) 
		{
			if (!MATCH(awk,TOKEN_LOCAL)) break;

			if (__get_token(awk) == -1) 
			{
				sse_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					sse_awk_tab_getsize(&awk->parse.locals) - nlocals);
				return SSE_NULL;
			}

			if (__collect_locals(awk, nlocals) == SSE_NULL)
			{
				sse_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					sse_awk_tab_getsize(&awk->parse.locals) - nlocals);
				return SSE_NULL;
			}
		}
	}

	/* block body */
	head = SSE_NULL; curr = SSE_NULL;

	while (1) 
	{
		if (MATCH(awk,TOKEN_EOF)) 
		{
			sse_awk_tab_remove (
				&awk->parse.locals, nlocals, 
				sse_awk_tab_getsize(&awk->parse.locals) - nlocals);
			if (head != SSE_NULL) sse_awk_clrpt (awk, head);
			PANIC (awk, SSE_AWK_EENDSRC);
		}

		if (MATCH(awk,TOKEN_RBRACE)) 
		{
			if (__get_token(awk) == -1) 
			{
				sse_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					sse_awk_tab_getsize(&awk->parse.locals) - nlocals);
				if (head != SSE_NULL) sse_awk_clrpt (awk, head);
				return SSE_NULL; 
			}
			break;
		}

		nde = __parse_statement (awk);
		if (nde == SSE_NULL) 
		{
			sse_awk_tab_remove (
				&awk->parse.locals, nlocals, 
				sse_awk_tab_getsize(&awk->parse.locals) - nlocals);
			if (head != SSE_NULL) sse_awk_clrpt (awk, head);
			return SSE_NULL;
		}

		/* remove unnecessary statements */
		if (nde->type == SSE_AWK_NDE_NULL ||
		    (nde->type == SSE_AWK_NDE_BLK && 
		     ((sse_awk_nde_blk_t*)nde)->body == SSE_NULL)) continue;
			
		if (curr == SSE_NULL) head = nde;
		else curr->next = nde;	
		curr = nde;
	}

	block = (sse_awk_nde_blk_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_blk_t));
	if (block == SSE_NULL) 
	{
		sse_awk_tab_remove (
			&awk->parse.locals, nlocals, 
			sse_awk_tab_getsize(&awk->parse.locals) - nlocals);
		sse_awk_clrpt (awk, head);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	tmp = sse_awk_tab_getsize(&awk->parse.locals);
	if (tmp > awk->parse.nlocals_max) awk->parse.nlocals_max = tmp;

	sse_awk_tab_remove (
		&awk->parse.locals, nlocals, tmp - nlocals);

	/* adjust the number of locals for a block without any statements */
	/* if (head == SSE_NULL) tmp = 0; */

	block->type = SSE_AWK_NDE_BLK;
	block->next = SSE_NULL;
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

	return (sse_awk_nde_t*)block;
}

static sse_awk_t* __add_builtin_globals (sse_awk_t* awk)
{
	struct __bvent* p = __bvtab;

	awk->tree.nbglobals = 0;
	while (p->name != SSE_NULL)
	{
		if (__add_global (awk, 
			p->name, p->name_len) == SSE_NULL) return SSE_NULL;
		awk->tree.nbglobals++;
		p++;
	}

	return awk;
}

static sse_awk_t* __add_global (
	sse_awk_t* awk, const sse_char_t* name, sse_size_t len)
{
	if (awk->option & SSE_AWK_UNIQUE) 
	{
		/* check if it conflict with a function name */
		if (sse_awk_map_get(&awk->tree.afns, name, len) != SSE_NULL) 
		{
			PANIC (awk, SSE_AWK_EDUPNAME);
		}
	}

	/* check if it conflicts with other global variable names */
	if (sse_awk_tab_find (&awk->parse.globals, 0, name, len) != (sse_size_t)-1) 
	{ 
		PANIC (awk, SSE_AWK_EDUPVAR);
	}

	if (sse_awk_tab_getsize(&awk->parse.globals) >= SSE_AWK_MAX_GLOBALS)
	{
		PANIC (awk, SSE_AWK_ETOOMANYGLOBALS);
	}

	if (sse_awk_tab_add (&awk->parse.globals, name, len) == (sse_size_t)-1) 
	{
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	return awk;
}

static sse_awk_t* __collect_globals (sse_awk_t* awk)
{
	while (1) 
	{
		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			PANIC (awk, SSE_AWK_EIDENT);
		}

		if (__add_global (awk,
			SSE_AWK_STR_BUF(&awk->token.name),
			SSE_AWK_STR_LEN(&awk->token.name)) == SSE_NULL) return SSE_NULL;

		if (__get_token(awk) == -1) return SSE_NULL;

		if (MATCH(awk,TOKEN_SEMICOLON)) break;

		if (!MATCH(awk,TOKEN_COMMA)) 
		{
			PANIC (awk, SSE_AWK_ECOMMA);
		}

		if (__get_token(awk) == -1) return SSE_NULL;
	}

	/* skip a semicolon */
	if (__get_token(awk) == -1) return SSE_NULL;

	return awk;
}

static sse_awk_t* __collect_locals (sse_awk_t* awk, sse_size_t nlocals)
{
	sse_char_t* local;
	sse_size_t local_len;

	while (1) 
	{
		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			PANIC (awk, SSE_AWK_EIDENT);
		}

		local = SSE_AWK_STR_BUF(&awk->token.name);
		local_len = SSE_AWK_STR_LEN(&awk->token.name);

		/* NOTE: it is not checked againt globals names */

		if (awk->option & SSE_AWK_UNIQUE) 
		{
			/* check if it conflict with a function name */
			if (sse_awk_map_get (
				&awk->tree.afns, local, local_len) != SSE_NULL) 
			{
				PANIC (awk, SSE_AWK_EDUPNAME);
			}
		}

		/* check if it conflicts with a paremeter name */
		if (sse_awk_tab_find (&awk->parse.params,
			0, local, local_len) != (sse_size_t)-1) 
		{
			PANIC (awk, SSE_AWK_EDUPNAME);
		}

		/* check if it conflicts with other local variable names */
		if (sse_awk_tab_find (&awk->parse.locals, 
			((awk->option & SSE_AWK_SHADING)? nlocals: 0),
			local, local_len) != (sse_size_t)-1)
		{
			PANIC (awk, SSE_AWK_EDUPVAR);	
		}

		if (sse_awk_tab_getsize(&awk->parse.locals) >= SSE_AWK_MAX_LOCALS)
		{
			PANIC (awk, SSE_AWK_ETOOMANYLOCALS);
		}

		if (sse_awk_tab_add (
			&awk->parse.locals, local, local_len) == (sse_size_t)-1) 
		{
			PANIC (awk, SSE_AWK_ENOMEM);
		}

		if (__get_token(awk) == -1) return SSE_NULL;

		if (MATCH(awk,TOKEN_SEMICOLON)) break;

		if (!MATCH(awk,TOKEN_COMMA)) PANIC (awk, SSE_AWK_ECOMMA);

		if (__get_token(awk) == -1) return SSE_NULL;
	}

	/* skip a semicolon */
	if (__get_token(awk) == -1) return SSE_NULL;

	return awk;
}

static sse_awk_nde_t* __parse_statement (sse_awk_t* awk)
{
	sse_awk_nde_t* nde;

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* null statement */	
		nde = (sse_awk_nde_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_t));
		if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);

		nde->type = SSE_AWK_NDE_NULL;
		nde->next = SSE_NULL;

		if (__get_token(awk) == -1) 
		{
			SSE_AWK_FREE (awk, nde);
			return SSE_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_LBRACE)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL; 
		nde = __parse_block (awk, sse_false);
	}
	else 
	{
		nde = __parse_statement_nb (awk);
awk->parse.nl_semicolon = 0;
	}

	return nde;
}

static sse_awk_nde_t* __parse_statement_nb (sse_awk_t* awk)
{
	sse_awk_nde_t* nde;

	/* 
	 * keywords that don't require any terminating semicolon 
	 */
	if (MATCH(awk,TOKEN_IF)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		return __parse_if (awk);
	}
	else if (MATCH(awk,TOKEN_WHILE)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		
		awk->parse.depth.loop++;
		nde = __parse_while (awk);
		awk->parse.depth.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_FOR)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;

		awk->parse.depth.loop++;
		nde = __parse_for (awk);
		awk->parse.depth.loop--;

		return nde;
	}

awk->parse.nl_semicolon = 1;
	/* 
	 * keywords that require a terminating semicolon 
	 */
	if (MATCH(awk,TOKEN_DO)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;

		awk->parse.depth.loop++;
		nde = __parse_dowhile (awk);
		awk->parse.depth.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_BREAK)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		nde = __parse_break (awk);
	}
	else if (MATCH(awk,TOKEN_CONTINUE)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		nde = __parse_continue (awk);
	}
	else if (MATCH(awk,TOKEN_RETURN)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		nde = __parse_return (awk);
	}
	else if (MATCH(awk,TOKEN_EXIT)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		nde = __parse_exit (awk);
	}
	else if (MATCH(awk,TOKEN_NEXT)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		nde = __parse_next (awk);
	}
	else if (MATCH(awk,TOKEN_NEXTFILE)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		nde = __parse_nextfile (awk);
	}
	else if (MATCH(awk,TOKEN_DELETE)) 
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		nde = __parse_delete (awk);
	}
	else if (MATCH(awk,TOKEN_PRINT))
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		nde = __parse_print (awk);
	}
	else if (MATCH(awk,TOKEN_PRINTF))
	{
		if (__get_token(awk) == -1) return SSE_NULL;
		nde = __parse_printf (awk);
	}
	else 
	{
		nde = __parse_esseression(awk);
	}

awk->parse.nl_semicolon = 0;
	if (nde == SSE_NULL) return SSE_NULL;

	/* check if a statement ends with a semicolon */
	if (!MATCH(awk,TOKEN_SEMICOLON)) 
	{
		if (nde != SSE_NULL) sse_awk_clrpt (awk, nde);
		PANIC (awk, SSE_AWK_ESEMICOLON);
	}

	/* eat up the semicolon and read in the next token */
	if (__get_token(awk) == -1) 
	{
		if (nde != SSE_NULL) sse_awk_clrpt (awk, nde);
		return SSE_NULL;
	}

	return nde;
}

static sse_awk_nde_t* __parse_esseression (sse_awk_t* awk)
{
	sse_awk_nde_t* x, * y;
	sse_awk_nde_ass_t* nde;
	int opcode;

	x = __parse_basic_esser (awk);
	if (x == SSE_NULL) return SSE_NULL;

	opcode = __assign_to_opcode (awk);
	if (opcode == -1) 
	{
		/* no assignment operator found. */
		return x;
	}

	sse_awk_assert (awk, x->next == SSE_NULL);
	if (!__is_var(x) && x->type != SSE_AWK_NDE_POS) 
	{
		sse_awk_clrpt (awk, x);
		PANIC (awk, SSE_AWK_EASSIGNMENT);
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, x);
		return SSE_NULL;
	}

	y = __parse_basic_esser (awk);
	if (y == SSE_NULL) 
	{
		sse_awk_clrpt (awk, x);
		return SSE_NULL;
	}

	nde = (sse_awk_nde_ass_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_ass_t));
	if (nde == SSE_NULL) 
	{
		sse_awk_clrpt (awk, x);
		sse_awk_clrpt (awk, y);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	nde->type = SSE_AWK_NDE_ASS;
	nde->next = SSE_NULL;
	nde->opcode = opcode;
	nde->left = x;
	nde->right = y;

	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_basic_esser (sse_awk_t* awk)
{
	sse_awk_nde_t* nde, * n1, * n2;
	
	nde = __parse_logical_or (awk);
	if (nde == SSE_NULL) return SSE_NULL;

	if (MATCH(awk,TOKEN_QUEST))
	{ 
		sse_awk_nde_cnd_t* tmp;

		if (__get_token(awk) == -1) return SSE_NULL;

		n1 = __parse_basic_esser (awk);
		if (n1 == SSE_NULL) 
		{
			sse_awk_clrpt (awk, nde);
			return SSE_NULL;
		}

		if (!MATCH(awk,TOKEN_COLON)) PANIC (awk, SSE_AWK_ECOLON);
		if (__get_token(awk) == -1) return SSE_NULL;

		n2 = __parse_basic_esser (awk);
		if (n2 == SSE_NULL)
		{
			sse_awk_clrpt (awk, nde);
			sse_awk_clrpt (awk, n1);
			return SSE_NULL;
		}

		tmp = (sse_awk_nde_cnd_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_cnd_t));
		if (tmp == SSE_NULL)
		{
			sse_awk_clrpt (awk, nde);
			sse_awk_clrpt (awk, n1);
			sse_awk_clrpt (awk, n2);
			return SSE_NULL;
		}

		tmp->type = SSE_AWK_NDE_CND;
		tmp->next = SSE_NULL;
		tmp->test = nde;
		tmp->left = n1;
		tmp->right = n2;

		nde = (sse_awk_nde_t*)tmp;
	}

	return nde;
}

static sse_awk_nde_t* __parse_binary_esser (
	sse_awk_t* awk, const __binmap_t* binmap,
	sse_awk_nde_t*(*next_level_func)(sse_awk_t*))
{
	sse_awk_nde_esse_t* nde;
	sse_awk_nde_t* left, * right;
	int opcode;

	left = next_level_func (awk);
	if (left == SSE_NULL) return SSE_NULL;
	
	while (1) 
	{
		const __binmap_t* p = binmap;
		sse_bool_t matched = sse_false;

		while (p->token != TOKEN_EOF)
		{
			if (MATCH(awk,p->token)) 
			{
				opcode = p->binop;
				matched = sse_true;
				break;
			}
			p++;
		}
		if (!matched) break;

		if (__get_token(awk) == -1) 
		{
			sse_awk_clrpt (awk, left);
			return SSE_NULL; 
		}

		right = next_level_func (awk);
		if (right == SSE_NULL) 
		{
			sse_awk_clrpt (awk, left);
			return SSE_NULL;
		}

#if 0
		/* TODO: enhance constant folding. do it in a better way */
		/* TODO: differentiate different types of numbers ... */
		if (left->type == SSE_AWK_NDE_INT && 
		    right->type == SSE_AWK_NDE_INT) 
		{
			sse_long_t l, r;

			l = ((sse_awk_nde_int_t*)left)->val; 
			r = ((sse_awk_nde_int_t*)right)->val; 

			/* TODO: more operators */
			if (opcode == SSE_AWK_BINOP_PLUS) l += r;
			else if (opcode == SSE_AWK_BINOP_MINUS) l -= r;
			else if (opcode == SSE_AWK_BINOP_MUL) l *= r;
			else if (opcode == SSE_AWK_BINOP_DIV && r != 0) l /= r;
			else if (opcode == SSE_AWK_BINOP_MOD && r != 0) l %= r;
			else goto skip_constant_folding;

			sse_awk_clrpt (awk, right);
			((sse_awk_nde_int_t*)left)->val = l;

			if (((sse_awk_nde_int_t*)left)->str != SSE_NULL)
			{
				SSE_AWK_FREE (awk, ((sse_awk_nde_int_t*)left)->str);
				((sse_awk_nde_int_t*)left)->str = SSE_NULL;
				((sse_awk_nde_int_t*)left)->len = 0;
			}

			continue;
		} 
		else if (left->type == SSE_AWK_NDE_REAL && 
		         right->type == SSE_AWK_NDE_REAL) 
		{
			sse_real_t l, r;

			l = ((sse_awk_nde_real_t*)left)->val; 
			r = ((sse_awk_nde_real_t*)right)->val; 

			/* TODO: more operators */
			if (opcode == SSE_AWK_BINOP_PLUS) l += r;
			else if (opcode == SSE_AWK_BINOP_MINUS) l -= r;
			else if (opcode == SSE_AWK_BINOP_MUL) l *= r;
			else if (opcode == SSE_AWK_BINOP_DIV) l /= r;
			else goto skip_constant_folding;

			sse_awk_clrpt (awk, right);
			((sse_awk_nde_real_t*)left)->val = l;

			if (((sse_awk_nde_real_t*)left)->str != SSE_NULL)
			{
				SSE_AWK_FREE (awk, ((sse_awk_nde_real_t*)left)->str);
				((sse_awk_nde_real_t*)left)->str = SSE_NULL;
				((sse_awk_nde_real_t*)left)->len = 0;
			}

			continue;
		}
		/* TODO: enhance constant folding more... */

	skip_constant_folding:
#endif
		nde = (sse_awk_nde_esse_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_esse_t));
		if (nde == SSE_NULL) 
		{
			sse_awk_clrpt (awk, right);
			sse_awk_clrpt (awk, left);
			PANIC (awk, SSE_AWK_ENOMEM);
		}

		nde->type = SSE_AWK_NDE_ESSE_BIN;
		nde->next = SSE_NULL;
		nde->opcode = opcode; 
		nde->left = left;
		nde->right = right;

		left = (sse_awk_nde_t*)nde;
	}

	return left;
}

static sse_awk_nde_t* __parse_logical_or (sse_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_LOR, SSE_AWK_BINOP_LOR },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_esser (awk, map, __parse_logical_and);
}

static sse_awk_nde_t* __parse_logical_and (sse_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_LAND, SSE_AWK_BINOP_LAND },
		{ TOKEN_EOF,  0 }
	};

	return __parse_binary_esser (awk, map, __parse_in);
}

static sse_awk_nde_t* __parse_in (sse_awk_t* awk)
{
	/* 
	static __binmap_t map[] =
	{
		{ TOKEN_IN, SSE_AWK_BINOP_IN },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_esser (awk, map, __parse_regex_match);
	*/

	sse_awk_nde_esse_t* nde;
	sse_awk_nde_t* left, * right;

	left = __parse_regex_match (awk);
	if (left == SSE_NULL) return SSE_NULL;

	while (1)
	{
		if (!MATCH(awk,TOKEN_IN)) break;

		if (__get_token(awk) == -1) 
		{
			sse_awk_clrpt (awk, left);
			return SSE_NULL; 
		}

		right = __parse_regex_match (awk);
		if (right == SSE_NULL) 
		{
			sse_awk_clrpt (awk, left);
			return SSE_NULL;
		}

		if (!__is_plain_var(right))
		{
			sse_awk_clrpt (awk, right);
			sse_awk_clrpt (awk, left);
			PANIC (awk, SSE_AWK_ENOTVAR);
		}

		nde = (sse_awk_nde_esse_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_esse_t));
		if (nde == SSE_NULL) 
		{
			sse_awk_clrpt (awk, right);
			sse_awk_clrpt (awk, left);
			PANIC (awk, SSE_AWK_ENOMEM);
		}

		nde->type = SSE_AWK_NDE_ESSE_BIN;
		nde->next = SSE_NULL;
		nde->opcode = SSE_AWK_BINOP_IN; 
		nde->left = left;
		nde->right = right;

		left = (sse_awk_nde_t*)nde;
	}

	return left;
}

static sse_awk_nde_t* __parse_regex_match (sse_awk_t* awk)
{
	static __binmap_t map[] =
	{
		{ TOKEN_TILDE, SSE_AWK_BINOP_MA },
		{ TOKEN_NM,    SSE_AWK_BINOP_NM },
		{ TOKEN_EOF,   0 },
	};

	return __parse_binary_esser (awk, map, __parse_bitwise_or);
}

static sse_awk_nde_t* __parse_bitwise_or (sse_awk_t* awk)
{
	if (awk->option & SSE_AWK_EXTIO)
	{
		return __parse_bitwise_or_with_extio (awk);
	}
	else
	{
		static __binmap_t map[] = 
		{
			{ TOKEN_BOR, SSE_AWK_BINOP_BOR },
			{ TOKEN_EOF, 0 }
		};

		return __parse_binary_esser (awk, map, __parse_bitwise_xor);
	}
}

static sse_awk_nde_t* __parse_bitwise_or_with_extio (sse_awk_t* awk)
{
	sse_awk_nde_t* left, * right;

	left = __parse_bitwise_xor (awk);
	if (left == SSE_NULL) return SSE_NULL;

	while (1)
	{
		int in_type;

		if (MATCH(awk,TOKEN_BOR)) 
			in_type = SSE_AWK_IN_PIPE;
		else if (MATCH(awk,TOKEN_BORAND)) 
			in_type = SSE_AWK_IN_COPROC;
		else break;
		
		if (__get_token(awk) == -1)
		{
			sse_awk_clrpt (awk, left);
			return SSE_NULL;
		}

		if (MATCH(awk,TOKEN_GETLINE))
		{
			sse_awk_nde_getline_t* nde;
			sse_awk_nde_t* var = SSE_NULL;

			/* piped getline */
			if (__get_token(awk) == -1)
			{
				sse_awk_clrpt (awk, left);
				return SSE_NULL;
			}

			/* TODO: is this correct? */

			if (MATCH(awk,TOKEN_IDENT))
			{
				/* command | getline var */

				var = __parse_primary (awk);
				if (var == SSE_NULL) 
				{
					sse_awk_clrpt (awk, left);
					return SSE_NULL;
				}
			}

			nde = (sse_awk_nde_getline_t*) SSE_AWK_MALLOC (
				awk, sse_sizeof(sse_awk_nde_getline_t));
			if (nde == SSE_NULL)
			{
				sse_awk_clrpt (awk, left);
				PANIC (awk, SSE_AWK_ENOMEM);
			}

			nde->type = SSE_AWK_NDE_GETLINE;
			nde->next = SSE_NULL;
			nde->var = var;
			nde->in_type = in_type;
			nde->in = left;

			left = (sse_awk_nde_t*)nde;
		}
		else
		{
			sse_awk_nde_esse_t* nde;

			if (in_type == SSE_AWK_IN_COPROC)
			{
				sse_awk_clrpt (awk, left);
				PANIC (awk, SSE_AWK_EGETLINE);
			}

			right = __parse_bitwise_xor (awk);
			if (right == SSE_NULL)
			{
				sse_awk_clrpt (awk, left);
				return SSE_NULL;
			}

			/* TODO: do constant folding */

			nde = (sse_awk_nde_esse_t*) SSE_AWK_MALLOC (
				awk, sse_sizeof(sse_awk_nde_esse_t));
			if (nde == SSE_NULL)
			{
				sse_awk_clrpt (awk, right);
				sse_awk_clrpt (awk, left);
				PANIC (awk, SSE_AWK_ENOMEM);
			}

			nde->type = SSE_AWK_NDE_ESSE_BIN;
			nde->next = SSE_NULL;
			nde->opcode = SSE_AWK_BINOP_BOR;
			nde->left = left;
			nde->right = right;

			left = (sse_awk_nde_t*)nde;
		}
	}

	return left;
}

static sse_awk_nde_t* __parse_bitwise_xor (sse_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_BXOR, SSE_AWK_BINOP_BXOR },
		{ TOKEN_EOF,  0 }
	};

	return __parse_binary_esser (awk, map, __parse_bitwise_and);
}

static sse_awk_nde_t* __parse_bitwise_and (sse_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_BAND, SSE_AWK_BINOP_BAND },
		{ TOKEN_EOF,  0 }
	};

	return __parse_binary_esser (awk, map, __parse_equality);
}

static sse_awk_nde_t* __parse_equality (sse_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_EQ, SSE_AWK_BINOP_EQ },
		{ TOKEN_NE, SSE_AWK_BINOP_NE },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_esser (awk, map, __parse_relational);
}

static sse_awk_nde_t* __parse_relational (sse_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_GT, SSE_AWK_BINOP_GT },
		{ TOKEN_GE, SSE_AWK_BINOP_GE },
		{ TOKEN_LT, SSE_AWK_BINOP_LT },
		{ TOKEN_LE, SSE_AWK_BINOP_LE },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_esser (awk, map, __parse_shift);
}

static sse_awk_nde_t* __parse_shift (sse_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_LSHIFT, SSE_AWK_BINOP_LSHIFT },
		{ TOKEN_RSHIFT, SSE_AWK_BINOP_RSHIFT },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_esser (awk, map, __parse_concat);
}

static sse_awk_nde_t* __parse_concat (sse_awk_t* awk)
{
	sse_awk_nde_esse_t* nde;
	sse_awk_nde_t* left, * right;

	left = __parse_additive (awk);
	if (left == SSE_NULL) return SSE_NULL;

	/* TODO: write a better code to do this.... 
	 *       first of all, is the following check sufficient? */
	while (MATCH(awk,TOKEN_LPAREN) || 
	       MATCH(awk,TOKEN_DOLLAR) ||
	       awk->token.type >= TOKEN_GETLINE)
	{
		right = __parse_additive (awk);
		if (right == SSE_NULL) 
		{
			sse_awk_clrpt (awk, left);
			return SSE_NULL;
		}

		nde = (sse_awk_nde_esse_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_esse_t));
		if (nde == SSE_NULL)
		{
			sse_awk_clrpt (awk, left);
			sse_awk_clrpt (awk, right);
			PANIC (awk, SSE_AWK_ENOMEM);
		}

		nde->type = SSE_AWK_NDE_ESSE_BIN;
		nde->next = SSE_NULL;
		nde->opcode = SSE_AWK_BINOP_CONCAT;
		nde->left = left;
		nde->right = right;
		
		left = (sse_awk_nde_t*)nde;
	}

	return left;
}

static sse_awk_nde_t* __parse_additive (sse_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_PLUS, SSE_AWK_BINOP_PLUS },
		{ TOKEN_MINUS, SSE_AWK_BINOP_MINUS },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_esser (awk, map, __parse_multiplicative);
}

static sse_awk_nde_t* __parse_multiplicative (sse_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_MUL, SSE_AWK_BINOP_MUL },
		{ TOKEN_DIV, SSE_AWK_BINOP_DIV },
		{ TOKEN_MOD, SSE_AWK_BINOP_MOD },
		{ TOKEN_ESSE, SSE_AWK_BINOP_ESSE },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_esser (awk, map, __parse_unary);
}

static sse_awk_nde_t* __parse_unary (sse_awk_t* awk)
{
	sse_awk_nde_esse_t* nde; 
	sse_awk_nde_t* left;
	int opcode;

	opcode = (MATCH(awk,TOKEN_PLUS))?  SSE_AWK_UNROP_PLUS:
	         (MATCH(awk,TOKEN_MINUS))? SSE_AWK_UNROP_MINUS:
	         (MATCH(awk,TOKEN_NOT))?   SSE_AWK_UNROP_NOT:
	         (MATCH(awk,TOKEN_TILDE))? SSE_AWK_UNROP_BNOT: -1;

	if (opcode == -1) return __parse_increment (awk);

	if (__get_token(awk) == -1) return SSE_NULL;

	left = __parse_unary (awk);
	if (left == SSE_NULL) return SSE_NULL;

	nde = (sse_awk_nde_esse_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_esse_t));
	if (nde == SSE_NULL)
	{
		sse_awk_clrpt (awk, left);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	nde->type = SSE_AWK_NDE_ESSE_UNR;
	nde->next = SSE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = SSE_NULL;

	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_increment (sse_awk_t* awk)
{
	sse_awk_nde_esse_t* nde;
	sse_awk_nde_t* left;
	int type, opcode, opcode1, opcode2;

	opcode1 = MATCH(awk,TOKEN_PLUSPLUS)? SSE_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? SSE_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1)
	{
		if (__get_token(awk) == -1) return SSE_NULL;
	}

	left = __parse_primary (awk);
	if (left == SSE_NULL) return SSE_NULL;

	opcode2 = MATCH(awk,TOKEN_PLUSPLUS)? SSE_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? SSE_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1 && opcode2 != -1)
	{
		sse_awk_clrpt (awk, left);
		PANIC (awk, SSE_AWK_ELVALUE);
	}
	else if (opcode1 == -1 && opcode2 == -1)
	{
		return left;
	}
	else if (opcode1 != -1) 
	{
		type = SSE_AWK_NDE_ESSE_INCPRE;
		opcode = opcode1;
	}
	else if (opcode2 != -1) 
	{
		type = SSE_AWK_NDE_ESSE_INCPST;
		opcode = opcode2;

		if (__get_token(awk) == -1) return SSE_NULL;
	}

	nde = (sse_awk_nde_esse_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_esse_t));
	if (nde == SSE_NULL)
	{
		sse_awk_clrpt (awk, left);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	nde->type = type;
	nde->next = SSE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = SSE_NULL;

	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_primary (sse_awk_t* awk)
{
	if (MATCH(awk,TOKEN_IDENT))  
	{
		return __parse_primary_ident (awk);
	}
	else if (MATCH(awk,TOKEN_INT)) 
	{
		sse_awk_nde_int_t* nde;

		nde = (sse_awk_nde_int_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_int_t));
		if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);

		nde->type = SSE_AWK_NDE_INT;
		nde->next = SSE_NULL;
		nde->val = sse_awk_strxtolong (awk, 
			SSE_AWK_STR_BUF(&awk->token.name), 
			SSE_AWK_STR_LEN(&awk->token.name), 0, SSE_NULL);
		nde->str = sse_awk_strxdup (awk,
			SSE_AWK_STR_BUF(&awk->token.name),
			SSE_AWK_STR_LEN(&awk->token.name));
		if (nde->str == SSE_NULL)
		{
			SSE_AWK_FREE (awk, nde);
			return SSE_NULL;			
		}
		nde->len = SSE_AWK_STR_LEN(&awk->token.name);

		sse_awk_assert (awk, 
			SSE_AWK_STR_LEN(&awk->token.name) ==
			sse_awk_strlen(SSE_AWK_STR_BUF(&awk->token.name)));

		if (__get_token(awk) == -1) 
		{
			SSE_AWK_FREE (awk, nde->str);
			SSE_AWK_FREE (awk, nde);
			return SSE_NULL;			
		}

		return (sse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_REAL)) 
	{
		sse_awk_nde_real_t* nde;

		nde = (sse_awk_nde_real_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_real_t));
		if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);

		nde->type = SSE_AWK_NDE_REAL;
		nde->next = SSE_NULL;
		nde->val = sse_awk_strxtoreal (awk, 
			SSE_AWK_STR_BUF(&awk->token.name), 
			SSE_AWK_STR_LEN(&awk->token.name), SSE_NULL);
		nde->str = sse_awk_strxdup (awk,
			SSE_AWK_STR_BUF(&awk->token.name),
			SSE_AWK_STR_LEN(&awk->token.name));
		if (nde->str == SSE_NULL)
		{
			SSE_AWK_FREE (awk, nde);
			return SSE_NULL;			
		}
		nde->len = SSE_AWK_STR_LEN(&awk->token.name);

		sse_awk_assert (awk, 
			SSE_AWK_STR_LEN(&awk->token.name) ==
			sse_awk_strlen(SSE_AWK_STR_BUF(&awk->token.name)));

		if (__get_token(awk) == -1) 
		{
			SSE_AWK_FREE (awk, nde->str);
			SSE_AWK_FREE (awk, nde);
			return SSE_NULL;			
		}

		return (sse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_STR))  
	{
		sse_awk_nde_str_t* nde;

		nde = (sse_awk_nde_str_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_str_t));
		if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);

		nde->type = SSE_AWK_NDE_STR;
		nde->next = SSE_NULL;
		nde->len = SSE_AWK_STR_LEN(&awk->token.name);
		nde->buf = sse_awk_strxdup (
			awk, SSE_AWK_STR_BUF(&awk->token.name), nde->len);
		if (nde->buf == SSE_NULL) 
		{
			SSE_AWK_FREE (awk, nde);
			PANIC (awk, SSE_AWK_ENOMEM);
		}

		if (__get_token(awk) == -1) 
		{
			SSE_AWK_FREE (awk, nde->buf);
			SSE_AWK_FREE (awk, nde);
			return SSE_NULL;			
		}

		return (sse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_DIV))
	{
		sse_awk_nde_rex_t* nde;
		int errnum;

		/* the regular esseression is tokenized here because 
		 * of the context-sensitivity of the slash symbol */
		SET_TOKEN_TYPE (awk, TOKEN_REX);
		sse_awk_str_clear (&awk->token.name);
		if (__get_rexstr (awk) == -1) return SSE_NULL;
		sse_awk_assert (awk, MATCH(awk,TOKEN_REX));

		nde = (sse_awk_nde_rex_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_rex_t));
		if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);

		nde->type = SSE_AWK_NDE_REX;
		nde->next = SSE_NULL;

		nde->len = SSE_AWK_STR_LEN(&awk->token.name);
		nde->buf = sse_awk_strxdup (
			awk,
			SSE_AWK_STR_BUF(&awk->token.name),
			SSE_AWK_STR_LEN(&awk->token.name));
		if (nde->buf == SSE_NULL)
		{
			SSE_AWK_FREE (awk, nde);
			PANIC (awk, SSE_AWK_ENOMEM);
		}

		nde->code = sse_awk_buildrex (
			awk,
			SSE_AWK_STR_BUF(&awk->token.name), 
			SSE_AWK_STR_LEN(&awk->token.name), 
			&errnum);
		if (nde->code == SSE_NULL)
		{
			SSE_AWK_FREE (awk, nde->buf);
			SSE_AWK_FREE (awk, nde);
			PANIC (awk, errnum);
		}

		if (__get_token(awk) == -1) 
		{
			SSE_AWK_FREE (awk, nde->buf);
			SSE_AWK_FREE (awk, nde->code);
			SSE_AWK_FREE (awk, nde);
			return SSE_NULL;			
		}

		return (sse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_DOLLAR)) 
	{
		sse_awk_nde_pos_t* nde;
		sse_awk_nde_t* prim;

		if (__get_token(awk)) return SSE_NULL;
		
		prim = __parse_primary (awk);
		if (prim == SSE_NULL) return SSE_NULL;

		nde = (sse_awk_nde_pos_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_pos_t));
		if (nde == SSE_NULL) 
		{
			sse_awk_clrpt (awk, prim);
			PANIC (awk, SSE_AWK_ENOMEM);
		}

		nde->type = SSE_AWK_NDE_POS;
		nde->next = SSE_NULL;
		nde->val = prim;

		return (sse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		sse_awk_nde_t* nde;
		sse_awk_nde_t* last;

		/* eat up the left parenthesis */
		if (__get_token(awk) == -1) return SSE_NULL;

		/* parse the sub-esseression inside the parentheses */
		nde = __parse_esseression (awk);
		if (nde == SSE_NULL) return SSE_NULL;

		/* parse subsequent esseressions separated by a comma, if any */
		last = nde;
		sse_awk_assert (awk, last->next == SSE_NULL);

		while (MATCH(awk,TOKEN_COMMA))
		{
			sse_awk_nde_t* tmp;

			if (__get_token(awk) == -1) 
			{
				sse_awk_clrpt (awk, nde);
				return SSE_NULL;
			}	

			tmp = __parse_esseression (awk);
			if (tmp == SSE_NULL) 
			{
				sse_awk_clrpt (awk, nde);
				return SSE_NULL;
			}

			sse_awk_assert (awk, tmp->next == SSE_NULL);
			last->next = tmp;
			last = tmp;
		} 
		/* ----------------- */

		/* check for the closing parenthesis */
		if (!MATCH(awk,TOKEN_RPAREN)) 
		{
			sse_awk_clrpt (awk, nde);
			PANIC (awk, SSE_AWK_ERPAREN);
		}

		if (__get_token(awk) == -1) 
		{
			sse_awk_clrpt (awk, nde);
			return SSE_NULL;
		}

		/* check if it is a chained node */
		if (nde->next != SSE_NULL)
		{
			/* if so, it is a esseression group */
			/* (esser1, esser2, esser2) */

			sse_awk_nde_grp_t* tmp;

			if (!MATCH(awk,TOKEN_IN))
			{
				sse_awk_clrpt (awk, nde);
				PANIC (awk, SSE_AWK_EIN);
			}

			tmp = (sse_awk_nde_grp_t*) SSE_AWK_MALLOC (
				awk, sse_sizeof(sse_awk_nde_grp_t));
			if (tmp == SSE_NULL)
			{
				sse_awk_clrpt (awk, nde);
				PANIC (awk, SSE_AWK_ENOMEM);
			}	

			tmp->type = SSE_AWK_NDE_GRP;
			tmp->next = SSE_NULL;
			tmp->body = nde;		

			nde = (sse_awk_nde_t*)tmp;
		}
		/* ----------------- */

		return nde;
	}
	else if (MATCH(awk,TOKEN_GETLINE)) 
	{
		sse_awk_nde_getline_t* nde;
		sse_awk_nde_t* var = SSE_NULL;
		sse_awk_nde_t* in = SSE_NULL;

		if (__get_token(awk) == -1) return SSE_NULL;

		if (MATCH(awk,TOKEN_IDENT))
		{
			/* getline var */
			
			var = __parse_primary (awk);
			if (var == SSE_NULL) return SSE_NULL;
		}

		if (MATCH(awk, TOKEN_LT))
		{
			/* getline [var] < file */
			if (__get_token(awk) == -1)
			{
				if (var != SSE_NULL) sse_awk_clrpt (awk, var);
				return SSE_NULL;
			}

			/* TODO: is this correct? */
			/*in = __parse_esseression (awk);*/
			in = __parse_primary (awk);
			if (in == SSE_NULL)
			{
				if (var != SSE_NULL) sse_awk_clrpt (awk, var);
				return SSE_NULL;
			}
		}

		nde = (sse_awk_nde_getline_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_getline_t));
		if (nde == SSE_NULL)
		{
			if (var != SSE_NULL) sse_awk_clrpt (awk, var);
			if (in != SSE_NULL) sse_awk_clrpt (awk, in);
			PANIC (awk, SSE_AWK_ENOMEM);
		}

		nde->type = SSE_AWK_NDE_GETLINE;
		nde->next = SSE_NULL;
		nde->var = var;
		nde->in_type = (in == SSE_NULL)? 
			SSE_AWK_IN_CONSOLE: SSE_AWK_IN_FILE;
		nde->in = in;

		return (sse_awk_nde_t*)nde;
	}

	/* valid esseression introducer is esseected */
	PANIC (awk, SSE_AWK_EESSERESSION);
}

static sse_awk_nde_t* __parse_primary_ident (sse_awk_t* awk)
{
	sse_char_t* name_dup;
	sse_size_t name_len;
	sse_awk_bfn_t* bfn;

	sse_awk_assert (awk, MATCH(awk,TOKEN_IDENT));

	name_dup = sse_awk_strxdup (
		awk, 
		SSE_AWK_STR_BUF(&awk->token.name), 
		SSE_AWK_STR_LEN(&awk->token.name));
	if (name_dup == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);
	name_len = SSE_AWK_STR_LEN(&awk->token.name);

	if (__get_token(awk) == -1) 
	{
		SSE_AWK_FREE (awk, name_dup);
		return SSE_NULL;			
	}

	/* check if name_dup is a built-in function name */
	bfn = sse_awk_getbfn (awk, name_dup, name_len);
	if (bfn != SSE_NULL)
	{
		sse_awk_nde_t* nde;

		SSE_AWK_FREE (awk, name_dup);
		if (!MATCH(awk,TOKEN_LPAREN))
		{
			/* built-in function should be in the form 
		 	 * of the function call */
			PANIC (awk, SSE_AWK_ELPAREN);
		}

		nde = __parse_fncall (awk, SSE_NULL, 0, bfn);
		return (sse_awk_nde_t*)nde;
	}

	/* now we know that name_dup is a normal identifier. */
	if (MATCH(awk,TOKEN_LBRACK)) 
	{
		sse_awk_nde_t* nde;
		nde = __parse_hashidx (awk, name_dup, name_len);
		if (nde == SSE_NULL) SSE_AWK_FREE (awk, name_dup);
		return (sse_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		/* function call */
		sse_awk_nde_t* nde;
		nde = __parse_fncall (awk, name_dup, name_len, SSE_NULL);
		if (nde == SSE_NULL) SSE_AWK_FREE (awk, name_dup);
		return (sse_awk_nde_t*)nde;
	}	
	else 
	{
		/* normal variable */
		sse_awk_nde_var_t* nde;
		sse_size_t idxa;

		nde = (sse_awk_nde_var_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_nde_var_t));
		if (nde == SSE_NULL) 
		{
			SSE_AWK_FREE (awk, name_dup);
			PANIC (awk, SSE_AWK_ENOMEM);
		}

		/* search the parameter name list */
		idxa = sse_awk_tab_find (
			&awk->parse.params, 0, name_dup, name_len);
		if (idxa != (sse_size_t)-1) 
		{
			nde->type = SSE_AWK_NDE_ARG;
			nde->next = SSE_NULL;
			/*nde->id.name = SSE_NULL;*/
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = idxa;
			nde->idx = SSE_NULL;

			return (sse_awk_nde_t*)nde;
		}

		/* search the local variable list */
		idxa = sse_awk_tab_rrfind (
			&awk->parse.locals, 0, name_dup, name_len);
		if (idxa != (sse_size_t)-1) 
		{
			nde->type = SSE_AWK_NDE_LOCAL;
			nde->next = SSE_NULL;
			/*nde->id.name = SSE_NULL;*/
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = idxa;
			nde->idx = SSE_NULL;

			return (sse_awk_nde_t*)nde;
		}

		/* search the global variable list */
		idxa = sse_awk_tab_rrfind (
			&awk->parse.globals, 0, name_dup, name_len);
		if (idxa != (sse_size_t)-1) 
		{
			nde->type = SSE_AWK_NDE_GLOBAL;
			nde->next = SSE_NULL;
			/*nde->id.name = SSE_NULL;*/
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = idxa;
			nde->idx = SSE_NULL;

			return (sse_awk_nde_t*)nde;
		}

		if (awk->option & SSE_AWK_IMPLICIT) 
		{
			nde->type = SSE_AWK_NDE_NAMED;
			nde->next = SSE_NULL;
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = (sse_size_t)-1;
			nde->idx = SSE_NULL;

			return (sse_awk_nde_t*)nde;
		}

		/* undefined variable */
		SSE_AWK_FREE (awk, name_dup);
		SSE_AWK_FREE (awk, nde);
		PANIC (awk, SSE_AWK_EUNDEF);
	}
}

static sse_awk_nde_t* __parse_hashidx (
	sse_awk_t* awk, sse_char_t* name, sse_size_t name_len)
{
	sse_awk_nde_t* idx, * tmp, * last;
	sse_awk_nde_var_t* nde;
	sse_size_t idxa;

	idx = SSE_NULL;
	last = SSE_NULL;

	do
	{
		if (__get_token(awk) == -1) 
		{
			if (idx != SSE_NULL) sse_awk_clrpt (awk, idx);
			return SSE_NULL;
		}

		tmp = __parse_esseression (awk);
		if (tmp == SSE_NULL) 
		{
			if (idx != SSE_NULL) sse_awk_clrpt (awk, idx);
			return SSE_NULL;
		}

		if (idx == SSE_NULL)
		{
			sse_awk_assert (awk, last == SSE_NULL);
			idx = tmp; last = tmp;
		}
		else
		{
			last->next = tmp;
			last = tmp;
		}
	}
	while (MATCH(awk,TOKEN_COMMA));

	sse_awk_assert (awk, idx != SSE_NULL);

	if (!MATCH(awk,TOKEN_RBRACK)) 
	{
		sse_awk_clrpt (awk, idx);
		PANIC (awk, SSE_AWK_ERBRACK);
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, idx);
		return SSE_NULL;
	}

	nde = (sse_awk_nde_var_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_var_t));
	if (nde == SSE_NULL) 
	{
		sse_awk_clrpt (awk, idx);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	/* search the parameter name list */
	idxa = sse_awk_tab_find (&awk->parse.params, 0, name, name_len);
	if (idxa != (sse_size_t)-1) 
	{
		nde->type = SSE_AWK_NDE_ARGIDX;
		nde->next = SSE_NULL;
		/*nde->id.name = SSE_NULL; */
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (sse_awk_nde_t*)nde;
	}

	/* search the local variable list */
	idxa = sse_awk_tab_rrfind(&awk->parse.locals, 0, name, name_len);
	if (idxa != (sse_size_t)-1) 
	{
		nde->type = SSE_AWK_NDE_LOCALIDX;
		nde->next = SSE_NULL;
		/*nde->id.name = SSE_NULL; */
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (sse_awk_nde_t*)nde;
	}

	/* search the global variable list */
	idxa = sse_awk_tab_rrfind(&awk->parse.globals, 0, name, name_len);
	if (idxa != (sse_size_t)-1) 
	{
		nde->type = SSE_AWK_NDE_GLOBALIDX;
		nde->next = SSE_NULL;
		/*nde->id.name = SSE_NULL;*/
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (sse_awk_nde_t*)nde;
	}

	if (awk->option & SSE_AWK_IMPLICIT) 
	{
		nde->type = SSE_AWK_NDE_NAMEDIDX;
		nde->next = SSE_NULL;
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = (sse_size_t)-1;
		nde->idx = idx;

		return (sse_awk_nde_t*)nde;
	}

	/* undefined variable */
	sse_awk_clrpt (awk, idx);
	SSE_AWK_FREE (awk, nde);
	PANIC (awk, SSE_AWK_EUNDEF);
}

static sse_awk_nde_t* __parse_fncall (
	sse_awk_t* awk, sse_char_t* name, sse_size_t name_len, sse_awk_bfn_t* bfn)
{
	sse_awk_nde_t* head, * curr, * nde;
	sse_awk_nde_call_t* call;
	sse_size_t nargs;

	if (__get_token(awk) == -1) return SSE_NULL;
	
	head = curr = SSE_NULL;
	nargs = 0;

	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no parameters to the function call */
		if (__get_token(awk) == -1) return SSE_NULL;
	}
	else 
	{
		/* parse function parameters */

		while (1) 
		{
			nde = __parse_esseression (awk);
			if (nde == SSE_NULL) 
			{
				if (head != SSE_NULL) sse_awk_clrpt (awk, head);
				return SSE_NULL;
			}
	
			if (head == SSE_NULL) head = nde;
			else curr->next = nde;
			curr = nde;

			nargs++;

			if (MATCH(awk,TOKEN_RPAREN)) 
			{
				if (__get_token(awk) == -1) 
				{
					if (head != SSE_NULL) 
						sse_awk_clrpt (awk, head);
					return SSE_NULL;
				}
				break;
			}

			if (!MATCH(awk,TOKEN_COMMA)) 
			{
				if (head != SSE_NULL) sse_awk_clrpt (awk, head);
				PANIC (awk, SSE_AWK_ECOMMA);	
			}

			if (__get_token(awk) == -1) 
			{
				if (head != SSE_NULL) sse_awk_clrpt (awk, head);
				return SSE_NULL;
			}
		}

	}

	call = (sse_awk_nde_call_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_call_t));
	if (call == SSE_NULL) 
	{
		if (head != SSE_NULL) sse_awk_clrpt (awk, head);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	if (bfn != SSE_NULL)
	{
		call->type = SSE_AWK_NDE_BFN;
		call->next = SSE_NULL;

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
		call->type = SSE_AWK_NDE_AFN;
		call->next = SSE_NULL;
		call->what.afn.name = name; 
		call->what.afn.name_len = name_len;
		call->args = head;
		call->nargs = nargs;
	}

	return (sse_awk_nde_t*)call;
}

static sse_awk_nde_t* __parse_if (sse_awk_t* awk)
{
	sse_awk_nde_t* test;
	sse_awk_nde_t* then_part;
	sse_awk_nde_t* else_part;
	sse_awk_nde_if_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, SSE_AWK_ELPAREN);
	if (__get_token(awk) == -1) return SSE_NULL;

	test = __parse_esseression (awk);
	if (test == SSE_NULL) return SSE_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		sse_awk_clrpt (awk, test);
		PANIC (awk, SSE_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, test);
		return SSE_NULL;
	}

	then_part = __parse_statement (awk);
	if (then_part == SSE_NULL) 
	{
		sse_awk_clrpt (awk, test);
		return SSE_NULL;
	}

	if (MATCH(awk,TOKEN_ELSE)) 
	{
		if (__get_token(awk) == -1) 
		{
			sse_awk_clrpt (awk, then_part);
			sse_awk_clrpt (awk, test);
			return SSE_NULL;
		}

		else_part = __parse_statement (awk);
		if (else_part == SSE_NULL) 
		{
			sse_awk_clrpt (awk, then_part);
			sse_awk_clrpt (awk, test);
			return SSE_NULL;
		}
	}
	else else_part = SSE_NULL;

	nde = (sse_awk_nde_if_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_if_t));
	if (nde == SSE_NULL) 
	{
		sse_awk_clrpt (awk, else_part);
		sse_awk_clrpt (awk, then_part);
		sse_awk_clrpt (awk, test);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	nde->type = SSE_AWK_NDE_IF;
	nde->next = SSE_NULL;
	nde->test = test;
	nde->then_part = then_part;
	nde->else_part = else_part;

	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_while (sse_awk_t* awk)
{
	sse_awk_nde_t* test, * body;
	sse_awk_nde_while_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, SSE_AWK_ELPAREN);
	if (__get_token(awk) == -1) return SSE_NULL;

	test = __parse_esseression (awk);
	if (test == SSE_NULL) return SSE_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		sse_awk_clrpt (awk, test);
		PANIC (awk, SSE_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, test);
		return SSE_NULL;
	}

	body = __parse_statement (awk);
	if (body == SSE_NULL) 
	{
		sse_awk_clrpt (awk, test);
		return SSE_NULL;
	}

	nde = (sse_awk_nde_while_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_while_t));
	if (nde == SSE_NULL) 
	{
		sse_awk_clrpt (awk, body);
		sse_awk_clrpt (awk, test);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	nde->type = SSE_AWK_NDE_WHILE;
	nde->next = SSE_NULL;
	nde->test = test;
	nde->body = body;

	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_for (sse_awk_t* awk)
{
	sse_awk_nde_t* init, * test, * incr, * body;
	sse_awk_nde_for_t* nde; 
	sse_awk_nde_foreach_t* nde2;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, SSE_AWK_ELPAREN);
	if (__get_token(awk) == -1) return SSE_NULL;
		
	if (MATCH(awk,TOKEN_SEMICOLON)) init = SSE_NULL;
	else 
	{
		/* this line is very ugly. it checks the entire next 
		 * esseression or the first element in the esseression
		 * is wrapped by a parenthesis */
		int no_foreach = MATCH(awk,TOKEN_LPAREN);

		init = __parse_esseression (awk);
		if (init == SSE_NULL) return SSE_NULL;

		if (!no_foreach && init->type == SSE_AWK_NDE_ESSE_BIN &&
		    ((sse_awk_nde_esse_t*)init)->opcode == SSE_AWK_BINOP_IN &&
		    __is_plain_var(((sse_awk_nde_esse_t*)init)->left))
		{	
			/* switch to foreach */
			
			if (!MATCH(awk,TOKEN_RPAREN))
			{
				sse_awk_clrpt (awk, init);
				PANIC (awk, SSE_AWK_ERPAREN);
			}

			if (__get_token(awk) == -1) 
			{
				sse_awk_clrpt (awk, init);
				return SSE_NULL;

			}	
			
			body = __parse_statement (awk);
			if (body == SSE_NULL) 
			{
				sse_awk_clrpt (awk, init);
				return SSE_NULL;
			}

			nde2 = (sse_awk_nde_foreach_t*) SSE_AWK_MALLOC (
				awk, sse_sizeof(sse_awk_nde_foreach_t));
			if (nde2 == SSE_NULL)
			{
				sse_awk_clrpt (awk, init);
				sse_awk_clrpt (awk, body);
				PANIC (awk, SSE_AWK_ENOMEM);
			}

			nde2->type = SSE_AWK_NDE_FOREACH;
			nde2->next = SSE_NULL;
			nde2->test = init;
			nde2->body = body;

			return (sse_awk_nde_t*)nde2;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			sse_awk_clrpt (awk, init);
			PANIC (awk, SSE_AWK_ESEMICOLON);
		}
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, init);
		return SSE_NULL;
	}

	if (MATCH(awk,TOKEN_SEMICOLON)) test = SSE_NULL;
	else 
	{
		test = __parse_esseression (awk);
		if (test == SSE_NULL) 
		{
			sse_awk_clrpt (awk, init);
			return SSE_NULL;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			sse_awk_clrpt (awk, init);
			sse_awk_clrpt (awk, test);
			PANIC (awk, SSE_AWK_ESEMICOLON);
		}
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, init);
		sse_awk_clrpt (awk, test);
		return SSE_NULL;
	}
	
	if (MATCH(awk,TOKEN_RPAREN)) incr = SSE_NULL;
	else 
	{
		incr = __parse_esseression (awk);
		if (incr == SSE_NULL) 
		{
			sse_awk_clrpt (awk, init);
			sse_awk_clrpt (awk, test);
			return SSE_NULL;
		}

		if (!MATCH(awk,TOKEN_RPAREN)) 
		{
			sse_awk_clrpt (awk, init);
			sse_awk_clrpt (awk, test);
			sse_awk_clrpt (awk, incr);
			PANIC (awk, SSE_AWK_ERPAREN);
		}
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, init);
		sse_awk_clrpt (awk, test);
		sse_awk_clrpt (awk, incr);
		return SSE_NULL;
	}

	body = __parse_statement (awk);
	if (body == SSE_NULL) 
	{
		sse_awk_clrpt (awk, init);
		sse_awk_clrpt (awk, test);
		sse_awk_clrpt (awk, incr);
		return SSE_NULL;
	}

	nde = (sse_awk_nde_for_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_for_t));
	if (nde == SSE_NULL) 
	{
		sse_awk_clrpt (awk, init);
		sse_awk_clrpt (awk, test);
		sse_awk_clrpt (awk, incr);
		sse_awk_clrpt (awk, body);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	nde->type = SSE_AWK_NDE_FOR;
	nde->next = SSE_NULL;
	nde->init = init;
	nde->test = test;
	nde->incr = incr;
	nde->body = body;

	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_dowhile (sse_awk_t* awk)
{
	sse_awk_nde_t* test, * body;
	sse_awk_nde_while_t* nde;

	body = __parse_statement (awk);
	if (body == SSE_NULL) return SSE_NULL;

	if (!MATCH(awk,TOKEN_WHILE)) 
	{
		sse_awk_clrpt (awk, body);
		PANIC (awk, SSE_AWK_EWHILE);
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, body);
		return SSE_NULL;
	}

	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		sse_awk_clrpt (awk, body);
		PANIC (awk, SSE_AWK_ELPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, body);
		return SSE_NULL;
	}

	test = __parse_esseression (awk);
	if (test == SSE_NULL) 
	{
		sse_awk_clrpt (awk, body);
		return SSE_NULL;
	}

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		sse_awk_clrpt (awk, body);
		sse_awk_clrpt (awk, test);
		PANIC (awk, SSE_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		sse_awk_clrpt (awk, body);
		sse_awk_clrpt (awk, test);
		return SSE_NULL;
	}
	
	nde = (sse_awk_nde_while_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_while_t));
	if (nde == SSE_NULL) 
	{
		sse_awk_clrpt (awk, body);
		sse_awk_clrpt (awk, test);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	nde->type = SSE_AWK_NDE_DOWHILE;
	nde->next = SSE_NULL;
	nde->test = test;
	nde->body = body;

	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_break (sse_awk_t* awk)
{
	sse_awk_nde_break_t* nde;

	if (awk->parse.depth.loop <= 0) PANIC (awk, SSE_AWK_EBREAK);

	nde = (sse_awk_nde_break_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_break_t));
	if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);
	nde->type = SSE_AWK_NDE_BREAK;
	nde->next = SSE_NULL;
	
	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_continue (sse_awk_t* awk)
{
	sse_awk_nde_continue_t* nde;

	if (awk->parse.depth.loop <= 0) PANIC (awk, SSE_AWK_ECONTINUE);

	nde = (sse_awk_nde_continue_t*) SSE_AWK_MALLOC (
		awk, sse_sizeof(sse_awk_nde_continue_t));
	if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);
	nde->type = SSE_AWK_NDE_CONTINUE;
	nde->next = SSE_NULL;
	
	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_return (sse_awk_t* awk)
{
	sse_awk_nde_return_t* nde;
	sse_awk_nde_t* val;

	nde = (sse_awk_nde_return_t*) SSE_AWK_MALLOC (
		awk, sse_sizeof(sse_awk_nde_return_t));
	if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);
	nde->type = SSE_AWK_NDE_RETURN;
	nde->next = SSE_NULL;

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* no return value */
		val = SSE_NULL;
	}
	else 
	{
		val = __parse_esseression (awk);
		if (val == SSE_NULL) 
		{
			SSE_AWK_FREE (awk, nde);
			return SSE_NULL;
		}
	}

	nde->val = val;
	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_exit (sse_awk_t* awk)
{
	sse_awk_nde_exit_t* nde;
	sse_awk_nde_t* val;

	nde = (sse_awk_nde_exit_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_exit_t));
	if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);
	nde->type = SSE_AWK_NDE_EXIT;
	nde->next = SSE_NULL;

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* no exit code */
		val = SSE_NULL;
	}
	else 
	{
		val = __parse_esseression (awk);
		if (val == SSE_NULL) 
		{
			SSE_AWK_FREE (awk, nde);
			return SSE_NULL;
		}
	}

	nde->val = val;
	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_delete (sse_awk_t* awk)
{
	sse_awk_nde_delete_t* nde;
	sse_awk_nde_t* var;

	if (!MATCH(awk,TOKEN_IDENT)) PANIC (awk, SSE_AWK_EIDENT);

	var = __parse_primary_ident (awk);
	if (var == SSE_NULL) return SSE_NULL;

	if (!__is_var (var))
	{
		/* a normal identifier is esseected */
		sse_awk_clrpt (awk, var);
		PANIC (awk, SSE_AWK_EIDENT);
	}

	nde = (sse_awk_nde_delete_t*) SSE_AWK_MALLOC (
		awk, sse_sizeof(sse_awk_nde_delete_t));
	if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);

	nde->type = SSE_AWK_NDE_DELETE;
	nde->next = SSE_NULL;
	nde->var = var;

	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_print (sse_awk_t* awk)
{
	sse_awk_nde_print_t* nde;
	sse_awk_nde_t* args = SSE_NULL; 
	sse_awk_nde_t* args_tail = SSE_NULL;
	sse_awk_nde_t* tail_prev = SSE_NULL;
	sse_awk_nde_t* out = SSE_NULL;
	int out_type;

	if (!MATCH(awk,TOKEN_SEMICOLON) &&
	    !MATCH(awk,TOKEN_GT) &&
	    !MATCH(awk,TOKEN_RSHIFT) &&
	    !MATCH(awk,TOKEN_BOR) &&
	    !MATCH(awk,TOKEN_BORAND)) 
	{
		args = __parse_esseression (awk);
		if (args == SSE_NULL) return SSE_NULL;

		args_tail = args;
		tail_prev = SSE_NULL;

		while (MATCH(awk,TOKEN_COMMA))
		{
			if (__get_token(awk) == -1)
			{
				sse_awk_clrpt (awk, args);
				return SSE_NULL;
			}

			args_tail->next = __parse_esseression (awk);
			if (args_tail->next == SSE_NULL)
			{
				sse_awk_clrpt (awk, args);
				return SSE_NULL;
			}

			tail_prev = args_tail;
			args_tail = args_tail->next;
		}

		/* print 1 > 2 would print 1 to the file named 2. 
		 * print (1 > 2) would print (1 > 2) in the console */
		if (awk->token.prev != TOKEN_RPAREN &&
		    args_tail->type == SSE_AWK_NDE_ESSE_BIN)
		{
			sse_awk_nde_esse_t* ep = (sse_awk_nde_esse_t*)args_tail;
			if (ep->opcode == SSE_AWK_BINOP_GT)
			{
				sse_awk_nde_t* tmp = args_tail;

				if (tail_prev != SSE_NULL) 
					tail_prev->next = ep->left;
				else args = ep->left;

				out = ep->right;
				out_type = SSE_AWK_OUT_FILE;

				SSE_AWK_FREE (awk, tmp);
			}
			else if (ep->opcode == SSE_AWK_BINOP_RSHIFT)
			{
				sse_awk_nde_t* tmp = args_tail;

				if (tail_prev != SSE_NULL) 
					tail_prev->next = ep->left;
				else args = ep->left;

				out = ep->right;
				out_type = SSE_AWK_OUT_FILE_APPEND;

				SSE_AWK_FREE (awk, tmp);
			}
			else if (ep->opcode == SSE_AWK_BINOP_BOR)
			{
				sse_awk_nde_t* tmp = args_tail;

				if (tail_prev != SSE_NULL) 
					tail_prev->next = ep->left;
				else args = ep->left;

				out = ep->right;
				out_type = SSE_AWK_OUT_PIPE;

				SSE_AWK_FREE (awk, tmp);
			}
		}
	}

	if (out == SSE_NULL)
	{
		out_type = MATCH(awk,TOKEN_GT)?     SSE_AWK_OUT_FILE:
		           MATCH(awk,TOKEN_RSHIFT)? SSE_AWK_OUT_FILE_APPEND:
		           MATCH(awk,TOKEN_BOR)?    SSE_AWK_OUT_PIPE:
		           MATCH(awk,TOKEN_BORAND)? SSE_AWK_OUT_COPROC:
		                                    SSE_AWK_OUT_CONSOLE;

		if (out_type != SSE_AWK_OUT_CONSOLE)
		{
			if (__get_token(awk) == -1)
			{
				if (args != SSE_NULL) sse_awk_clrpt (awk, args);
				return SSE_NULL;
			}

			out = __parse_esseression(awk);
			if (out == SSE_NULL)
			{
				if (args != SSE_NULL) sse_awk_clrpt (awk, args);
				return SSE_NULL;
			}
		}
	}

	nde = (sse_awk_nde_print_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_print_t));
	if (nde == SSE_NULL) 
	{
		if (args != SSE_NULL) sse_awk_clrpt (awk, args);
		if (out != SSE_NULL) sse_awk_clrpt (awk, out);
		PANIC (awk, SSE_AWK_ENOMEM);
	}

	nde->type = SSE_AWK_NDE_PRINT;
	nde->next = SSE_NULL;
	nde->args = args;
	nde->out_type = out_type;
	nde->out = out;

	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_printf (sse_awk_t* awk)
{
	/* TODO: implement this... */
	return SSE_NULL;
}

static sse_awk_nde_t* __parse_next (sse_awk_t* awk)
{
	sse_awk_nde_next_t* nde;

	if (awk->parse.id.block == PARSE_BEGIN_BLOCK ||
	    awk->parse.id.block == PARSE_END_BLOCK)
	{
		PANIC (awk, SSE_AWK_ENEXT);
	}

	nde = (sse_awk_nde_next_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_nde_next_t));
	if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);
	nde->type = SSE_AWK_NDE_NEXT;
	nde->next = SSE_NULL;
	
	return (sse_awk_nde_t*)nde;
}

static sse_awk_nde_t* __parse_nextfile (sse_awk_t* awk)
{
	sse_awk_nde_nextfile_t* nde;

	if (awk->parse.id.block == PARSE_BEGIN_BLOCK ||
	    awk->parse.id.block == PARSE_END_BLOCK)
	{
		PANIC (awk, SSE_AWK_ENEXTFILE);
	}

	nde = (sse_awk_nde_nextfile_t*) SSE_AWK_MALLOC (
		awk, sse_sizeof(sse_awk_nde_nextfile_t));
	if (nde == SSE_NULL) PANIC (awk, SSE_AWK_ENOMEM);
	nde->type = SSE_AWK_NDE_NEXTFILE;
	nde->next = SSE_NULL;
	
	return (sse_awk_nde_t*)nde;
}

static int __get_token (sse_awk_t* awk)
{
	sse_cint_t c;
	sse_size_t line;
	int n;

	line = awk->token.line;
	do 
	{
		if (__skip_spaces(awk) == -1) return -1;
		if ((n = __skip_comment(awk)) == -1) return -1;
	} 
	while (n == 1);

	sse_awk_str_clear (&awk->token.name);
	awk->token.line = awk->src.lex.line;
	awk->token.column = awk->src.lex.column;

	if (line != 0 && (awk->option & SSE_AWK_BLOCKLESS) &&
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

	if (c == SSE_CHAR_EOF) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_EOF);
	}	
	else if (SSE_AWK_ISDIGIT (awk, c) || c == SSE_T('.'))
	{
		if (__get_number (awk) == -1) return -1;
	}
	else if (SSE_AWK_ISALPHA (awk, c) || c == SSE_T('_')) 
	{
		int type;

		/* identifier */
		do 
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		} 
		while (SSE_AWK_ISALPHA (awk, c) || 
		       c == SSE_T('_') || SSE_AWK_ISDIGIT(awk,c));

		type = __classify_ident (awk, 
			SSE_AWK_STR_BUF(&awk->token.name), 
			SSE_AWK_STR_LEN(&awk->token.name));
		SET_TOKEN_TYPE (awk, type);
	}
	else if (c == SSE_T('\"')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_STR);

		if (__get_charstr(awk) == -1) return -1;

		while (awk->option & SSE_AWK_STRCONCAT) 
		{
			do 
			{
				if (__skip_spaces(awk) == -1) return -1;
				if ((n = __skip_comment(awk)) == -1) return -1;
			} 
			while (n == 1);

			c = awk->src.lex.curc;
			if (c != SSE_T('\"')) break;

			if (__get_charstr(awk) == -1) return -1;
		}

	}
	else if (c == SSE_T('=')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == SSE_T('=')) 
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
	else if (c == SSE_T('!')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == SSE_T('=')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_NE);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == SSE_T('~'))
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
	else if (c == SSE_T('>')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if ((awk->option & SSE_AWK_SHIFT) && c == SSE_T('>')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_RSHIFT);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == SSE_T('=')) 
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
	else if (c == SSE_T('<')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if ((awk->option & SSE_AWK_SHIFT) && c == SSE_T('<')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_LSHIFT);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == SSE_T('=')) 
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
	else if (c == SSE_T('|'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == SSE_T('|'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_LOR);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == SSE_T('&'))
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
	else if (c == SSE_T('&'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == SSE_T('&'))
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
	else if (c == SSE_T('~'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_TILDE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T('^'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_BXOR);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T('+')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == SSE_T('+')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_PLUSPLUS);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == SSE_T('=')) 
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
	else if (c == SSE_T('-')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if (c == SSE_T('-')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_MINUSMINUS);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == SSE_T('=')) 
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
	else if (c == SSE_T('*')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == SSE_T('='))
		{
			SET_TOKEN_TYPE (awk, TOKEN_MUL_ASSIGN);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
		else if (c == SSE_T('*'))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
			if (c == SSE_T('='))
			{
				SET_TOKEN_TYPE (awk, TOKEN_ESSE_ASSIGN);
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			}
			else 
			{
				SET_TOKEN_TYPE (awk, TOKEN_ESSE);
			}
		}
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_MUL);
		}
	}
	else if (c == SSE_T('/')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == SSE_T('='))
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
	else if (c == SSE_T('%')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == SSE_T('='))
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
	else if (c == SSE_T('(')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_LPAREN);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T(')')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_RPAREN);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T('{')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_LBRACE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T('}')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_RBRACE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T('[')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_LBRACK);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T(']')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_RBRACK);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T('$')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_DOLLAR);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T(',')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_COMMA);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T('.'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_PERIOD);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T(';') || 
	         (c == SSE_T('\n') && (awk->option & SSE_AWK_NEWLINE))) 
	{
	/* TODO: more check on the newline terminator... */
		SET_TOKEN_TYPE (awk, TOKEN_SEMICOLON);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T(':'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_COLON);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == SSE_T('?'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_QUEST);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else 
	{
		awk->errnum = SSE_AWK_ELXCHR;
		return -1;
	}

/*sse_printf (SSE_T("token -> [%s]\n"), SSE_AWK_STR_BUF(&awk->token.name));*/
	return 0;
}

static int __get_number (sse_awk_t* awk)
{
	sse_cint_t c;

	sse_awk_assert (awk, SSE_AWK_STR_LEN(&awk->token.name) == 0);
	SET_TOKEN_TYPE (awk, TOKEN_INT);

	c = awk->src.lex.curc;

	if (c == SSE_T('0'))
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == SSE_T('x') || c == SSE_T('X'))
		{
			/* hexadecimal number */
			do 
			{
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			} 
			while (SSE_AWK_ISXDIGIT (awk, c));

			return 0;
		}
		else if (c == SSE_T('b') || c == SSE_T('B'))
		{
			/* binary number */
			do
			{
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			} 
			while (c == SSE_T('0') || c == SSE_T('1'));

			return 0;
		}
		else if (c != '.')
		{
			/* octal number */
			while (c >= SSE_T('0') && c <= SSE_T('7'))
			{
				ADD_TOKEN_CHAR (awk, c);
				GET_CHAR_TO (awk, c);
			}

			return 0;
		}
	}

	while (SSE_AWK_ISDIGIT (awk, c)) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	} 

	if (c == SSE_T('.'))
	{
		/* floating-point number */
		SET_TOKEN_TYPE (awk, TOKEN_REAL);

		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		while (SSE_AWK_ISDIGIT (awk, c))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
	}

	if (c == SSE_T('E') || c == SSE_T('e'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_REAL);

		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);

		if (c == SSE_T('+') || c == SSE_T('-'))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}

		while (SSE_AWK_ISDIGIT (awk, c))
		{
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		}
	}

	return 0;
}

static int __get_charstr (sse_awk_t* awk)
{
	if (awk->src.lex.curc != SSE_T('\"')) 
	{
		/* the starting quote has been consumed before this function
		 * has been called */
		ADD_TOKEN_CHAR (awk, awk->src.lex.curc);
	}
	return __get_string (awk, SSE_T('\"'), SSE_T('\\'), sse_false);
}

static int __get_rexstr (sse_awk_t* awk)
{
	if (awk->src.lex.curc == SSE_T('/')) 
	{
		/* this part of the function is different from __get_charstr
		 * because of the way this function is called */
		GET_CHAR (awk);
		return 0;
	}
	else 
	{
		ADD_TOKEN_CHAR (awk, awk->src.lex.curc);
		return __get_string (awk, SSE_T('/'), SSE_T('\\'), sse_true);
	}
}

static int __get_string (
	sse_awk_t* awk, sse_char_t end_char, 
	sse_char_t esc_char, sse_bool_t keep_esc_char)
{
	sse_cint_t c;
	int escaped = 0;
	int digit_count = 0;
	sse_cint_t c_acc;

	while (1)
	{
		GET_CHAR_TO (awk, c);

		if (c == SSE_CHAR_EOF)
		{
			awk->errnum = SSE_AWK_EENDSTR;
			return -1;
		}

		if (escaped == 3)
		{
			if (c >= SSE_T('0') && c <= SSE_T('7'))
			{
				c_acc = c_acc * 8 + c - SSE_T('0');
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
			if (c >= SSE_T('0') && c <= SSE_T('9'))
			{
				c_acc = c_acc * 16 + c - SSE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= SSE_T('A') && c <= SSE_T('F'))
			{
				c_acc = c_acc * 16 + c - SSE_T('A') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (awk, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= SSE_T('a') && c <= SSE_T('f'))
			{
				c_acc = c_acc * 16 + c - SSE_T('a') + 10;
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
				sse_char_t rc;

				rc = (escaped == 2)? SSE_T('x'):
				     (escaped == 4)? SSE_T('u'): SSE_T('U');

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
			if (c == SSE_T('n')) c = SSE_T('\n');
			else if (c == SSE_T('r')) c = SSE_T('\r');
			else if (c == SSE_T('t')) c = SSE_T('\t');
			else if (c == SSE_T('f')) c = SSE_T('\f');
			else if (c == SSE_T('b')) c = SSE_T('\b');
			else if (c == SSE_T('v')) c = SSE_T('\v');
			else if (c == SSE_T('a')) c = SSE_T('\a');
			else if (c >= SSE_T('0') && c <= SSE_T('7')) 
			{
				escaped = 3;
				digit_count = 1;
				c_acc = c - SSE_T('0');
				continue;
			}
			else if (c == SSE_T('x')) 
			{
				escaped = 2;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#ifdef SSE_CHAR_IS_WCHAR
			else if (c == SSE_T('u') && sse_sizeof(sse_char_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == SSE_T('U') && sse_sizeof(sse_char_t) >= 4) 
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

static int __get_char (sse_awk_t* awk)
{
	sse_ssize_t n;
	/*sse_char_t c;*/

	if (awk->src.lex.ungotc_count > 0) 
	{
		awk->src.lex.curc = awk->src.lex.ungotc[--awk->src.lex.ungotc_count];
		return 0;
	}

	if (awk->src.shared.buf_pos >= awk->src.shared.buf_len)
	{
		n = awk->src.ios->in (
			SSE_AWK_IO_READ, awk->src.ios->custom_data,
			awk->src.shared.buf, sse_countof(awk->src.shared.buf));
		if (n == -1)
		{
			awk->errnum = SSE_AWK_ESRCINREAD;
			return -1;
		}

		if (n == 0) 
		{
			awk->src.lex.curc = SSE_CHAR_EOF;
			return 0;
		}

		awk->src.shared.buf_pos = 0;
		awk->src.shared.buf_len = n;	
	}

	awk->src.lex.curc = awk->src.shared.buf[awk->src.shared.buf_pos++];

	if (awk->src.lex.curc == SSE_T('\n'))
	{
		awk->src.lex.line++;
		awk->src.lex.column = 1;
	}
	else awk->src.lex.column++;

	return 0;
}

static int __unget_char (sse_awk_t* awk, sse_cint_t c)
{
	if (awk->src.lex.ungotc_count >= sse_countof(awk->src.lex.ungotc)) 
	{
		awk->errnum = SSE_AWK_ELXUNG;
		return -1;
	}

	awk->src.lex.ungotc[awk->src.lex.ungotc_count++] = c;
	return 0;
}

static int __skip_spaces (sse_awk_t* awk)
{
	sse_cint_t c = awk->src.lex.curc;

	if (awk->option & SSE_AWK_NEWLINE && awk->parse.nl_semicolon)
	{
		while (c != SSE_T('\n') &&
		       SSE_AWK_ISSPACE (awk, c)) GET_CHAR_TO (awk, c);
	}
	else
	{
		while (SSE_AWK_ISSPACE (awk, c)) GET_CHAR_TO (awk, c);
	}
	return 0;
}

static int __skip_comment (sse_awk_t* awk)
{
	sse_cint_t c = awk->src.lex.curc;

	if ((awk->option & SSE_AWK_HASHSIGN) && c == SSE_T('#'))
	{
		do 
		{ 
			GET_CHAR_TO (awk, c);
		} 
		while (c != SSE_T('\n') && c != SSE_CHAR_EOF);

		GET_CHAR (awk);
		return 1; /* comment by # */
	}

	if (c != SSE_T('/')) return 0; /* not a comment */
	GET_CHAR_TO (awk, c);

	if ((awk->option & SSE_AWK_DBLSLASHES) && c == SSE_T('/')) 
	{
		do 
		{ 
			GET_CHAR_TO (awk, c);
		} 
		while (c != SSE_T('\n') && c != SSE_CHAR_EOF);

		GET_CHAR (awk);
		return 1; /* comment by // */
	}
	else if (c == SSE_T('*')) 
	{
		do 
		{
			GET_CHAR_TO (awk, c);
			if (c == SSE_CHAR_EOF)
			{
				awk->errnum = SSE_AWK_EENDCOMMENT;
				return -1;
			}

			if (c == SSE_T('*')) 
			{
				GET_CHAR_TO (awk, c);
				if (c == SSE_CHAR_EOF)
				{
					awk->errnum = SSE_AWK_EENDCOMMENT;
					return -1;
				}

				if (c == SSE_T('/')) 
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
	awk->src.lex.curc = SSE_T('/');

	return 0;
}

static int __classify_ident (
	sse_awk_t* awk, const sse_char_t* name, sse_size_t len)
{
	struct __kwent* kwp;

	for (kwp = __kwtab; kwp->name != SSE_NULL; kwp++) 
	{
		if (kwp->valid != 0 && 
		    (awk->option & kwp->valid) == 0) continue;

		if (sse_awk_strxncmp (kwp->name, kwp->name_len, name, len) == 0) 
		{
			return kwp->type;
		}
	}

	return TOKEN_IDENT;
}

static int __assign_to_opcode (sse_awk_t* awk)
{
	static int __assop[] =
	{
		SSE_AWK_ASSOP_NONE,
		SSE_AWK_ASSOP_PLUS,
		SSE_AWK_ASSOP_MINUS,
		SSE_AWK_ASSOP_MUL,
		SSE_AWK_ASSOP_DIV,
		SSE_AWK_ASSOP_MOD,
		SSE_AWK_ASSOP_ESSE
	};

	if (awk->token.type >= TOKEN_ASSIGN &&
	    awk->token.type <= TOKEN_ESSE_ASSIGN)
	{
		return __assop[awk->token.type - TOKEN_ASSIGN];
	}

	return -1;
}

static int __is_plain_var (sse_awk_nde_t* nde)
{
	return nde->type == SSE_AWK_NDE_GLOBAL ||
	       nde->type == SSE_AWK_NDE_LOCAL ||
	       nde->type == SSE_AWK_NDE_ARG ||
	       nde->type == SSE_AWK_NDE_NAMED;
}

static int __is_var (sse_awk_nde_t* nde)
{
	return nde->type == SSE_AWK_NDE_GLOBAL ||
	       nde->type == SSE_AWK_NDE_LOCAL ||
	       nde->type == SSE_AWK_NDE_ARG ||
	       nde->type == SSE_AWK_NDE_NAMED ||
	       nde->type == SSE_AWK_NDE_GLOBALIDX ||
	       nde->type == SSE_AWK_NDE_LOCALIDX ||
	       nde->type == SSE_AWK_NDE_ARGIDX ||
	       nde->type == SSE_AWK_NDE_NAMEDIDX;
}

struct __deparse_func_t 
{
	sse_awk_t* awk;
	sse_char_t* tmp;
	sse_size_t tmp_len;
};

static int __deparse (sse_awk_t* awk)
{
	sse_awk_chain_t* chain;
	sse_char_t tmp[sse_sizeof(sse_size_t)*8 + 32];
	struct __deparse_func_t df;
	int n = 0, op;

	sse_awk_assert (awk, awk->src.ios->out != SSE_NULL);

	awk->src.shared.buf_len = 0;
	awk->src.shared.buf_pos = 0;

	op = awk->src.ios->out (
		SSE_AWK_IO_OPEN, awk->src.ios->custom_data, SSE_NULL, 0);
	if (op == -1)
	{
		awk->errnum = SSE_AWK_ESRCOUTOPEN;
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

#define EXIT_DEPARSE(num) \
	do { n = -1; awk->errnum = num ; goto exit_deparse; } while(0)

	if (awk->tree.nglobals > awk->tree.nbglobals) 
	{
		sse_size_t i, len;

		sse_awk_assert (awk, awk->tree.nglobals > 0);
		if (sse_awk_putsrcstr (awk, SSE_T("global ")) == -1)
			EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);

		for (i = awk->tree.nbglobals; i < awk->tree.nglobals - 1; i++) 
		{
			len = sse_awk_longtostr ((sse_long_t)i, 
				10, SSE_T("__global"), tmp, sse_countof(tmp));
			sse_awk_assert (awk, len != (sse_size_t)-1);
			if (sse_awk_putsrcstrx (awk, tmp, len) == -1)
				EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
			if (sse_awk_putsrcstr (awk, SSE_T(", ")) == -1)
				EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
		}

		len = sse_awk_longtostr ((sse_long_t)i, 
			10, SSE_T("__global"), tmp, sse_countof(tmp));
		sse_awk_assert (awk, len != (sse_size_t)-1);
		if (sse_awk_putsrcstrx (awk, tmp, len) == -1)
			EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
		if (sse_awk_putsrcstr (awk, SSE_T(";\n\n")) == -1)
			EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
	}

	df.awk = awk;
	df.tmp = tmp;
	df.tmp_len = sse_countof(tmp);

	if (sse_awk_map_walk (&awk->tree.afns, __deparse_func, &df) == -1) 
	{
		EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
	}

	if (awk->tree.begin != SSE_NULL) 
	{
		if (sse_awk_putsrcstr (awk, SSE_T("BEGIN ")) == -1)
			EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);

		if (sse_awk_prnpt (awk, awk->tree.begin) == -1)
			EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);

		if (__put_char (awk, SSE_T('\n')) == -1)
			EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
	}

	chain = awk->tree.chain;
	while (chain != SSE_NULL) 
	{
		if (chain->pattern != SSE_NULL) 
		{
			if (sse_awk_prnptnpt (awk, chain->pattern) == -1)
				EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
		}

		if (chain->action == SSE_NULL) 
		{
			/* blockless pattern */
			if (__put_char (awk, SSE_T('\n')) == -1)
				EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
		}
		else 
		{
			if (sse_awk_prnpt (awk, chain->action) == -1)
				EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
		}

		if (__put_char (awk, SSE_T('\n')) == -1)
			EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);

		chain = chain->next;	
	}

	if (awk->tree.end != SSE_NULL) 
	{
		if (sse_awk_putsrcstr (awk, SSE_T("END ")) == -1)
			EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
		if (sse_awk_prnpt (awk, awk->tree.end) == -1)
			EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);
	}

	if (__flush (awk) == -1) EXIT_DEPARSE (SSE_AWK_ESRCOUTWRITE);

exit_deparse:
	if (awk->src.ios->out (
		SSE_AWK_IO_CLOSE, awk->src.ios->custom_data, SSE_NULL, 0) == -1)
	{
		if (n != -1)
		{
			awk->errnum = SSE_AWK_ESRCOUTCLOSE;
			n = -1;
		}
	}

	return 0;
}

static int __deparse_func (sse_awk_pair_t* pair, void* arg)
{
	struct __deparse_func_t* df = (struct __deparse_func_t*)arg;
	sse_awk_afn_t* afn = (sse_awk_afn_t*)pair->val;
	sse_size_t i, n;

	sse_awk_assert (df->awk, sse_awk_strxncmp (
		pair->key, pair->key_len, afn->name, afn->name_len) == 0);

	if (sse_awk_putsrcstr (df->awk, SSE_T("function ")) == -1) return -1;
	if (sse_awk_putsrcstr (df->awk, afn->name) == -1) return -1;
	if (sse_awk_putsrcstr (df->awk, SSE_T(" (")) == -1) return -1;

	for (i = 0; i < afn->nargs; ) 
	{
		n = sse_awk_longtostr (i++, 10, 
			SSE_T("__param"), df->tmp, df->tmp_len);
		sse_awk_assert (df->awk, n != (sse_size_t)-1);
		if (sse_awk_putsrcstrx (df->awk, df->tmp, n) == -1) return -1;
		if (i >= afn->nargs) break;
		if (sse_awk_putsrcstr (df->awk, SSE_T(", ")) == -1) return -1;
	}

	if (sse_awk_putsrcstr (df->awk, SSE_T(")\n")) == -1) return -1;

	if (sse_awk_prnpt (df->awk, afn->body) == -1) return -1;
	if (sse_awk_putsrcstr (df->awk, SSE_T("\n")) == -1) return -1;

	return 0;
}

static int __put_char (sse_awk_t* awk, sse_char_t c)
{
	awk->src.shared.buf[awk->src.shared.buf_len++] = c;
	if (awk->src.shared.buf_len >= sse_countof(awk->src.shared.buf))
	{
		if (__flush (awk) == -1) return -1;
	}
	return 0;
}

static int __flush (sse_awk_t* awk)
{
	sse_ssize_t n;

	sse_awk_assert (awk, awk->src.ios->out != SSE_NULL);

	while (awk->src.shared.buf_pos < awk->src.shared.buf_len)
	{
		n = awk->src.ios->out (
			SSE_AWK_IO_WRITE, awk->src.ios->custom_data,
			&awk->src.shared.buf[awk->src.shared.buf_pos], 
			awk->src.shared.buf_len - awk->src.shared.buf_pos);
		if (n <= 0) return -1;

		awk->src.shared.buf_pos += n;
	}

	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;
	return 0;
}

int sse_awk_putsrcstr (sse_awk_t* awk, const sse_char_t* str)
{
	while (*str != SSE_T('\0'))
	{
		if (__put_char (awk, *str) == -1) return -1;
		str++;
	}

	return 0;
}

int sse_awk_putsrcstrx (
	sse_awk_t* awk, const sse_char_t* str, sse_size_t len)
{
	const sse_char_t* end = str + len;

	while (str < end)
	{
		if (__put_char (awk, *str) == -1) return -1;
		str++;
	}

	return 0;
}

