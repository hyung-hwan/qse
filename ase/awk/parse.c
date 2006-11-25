/*
 * $Id: parse.c,v 1.209 2006-11-25 15:51:30 bacon Exp $
 */

#include <ase/awk/awk_i.h>

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
	TOKEN_NEXTINFILE,
	TOKEN_NEXTOFILE,
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

static int __parse (ase_awk_t* awk);

static ase_awk_t* __parse_progunit (ase_awk_t* awk);
static ase_awk_t* __collect_globals (ase_awk_t* awk);
static ase_awk_t* __add_builtin_globals (ase_awk_t* awk);
static ase_awk_t* __add_global (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len, int force);
static ase_awk_t* __collect_locals (ase_awk_t* awk, ase_size_t nlocals);

static ase_awk_nde_t* __parse_function (ase_awk_t* awk);
static ase_awk_nde_t* __parse_begin (ase_awk_t* awk);
static ase_awk_nde_t* __parse_end (ase_awk_t* awk);
static ase_awk_chain_t* __parse_pattern_block (
	ase_awk_t* awk, ase_awk_nde_t* ptn, ase_bool_t blockless);

static ase_awk_nde_t* __parse_block (ase_awk_t* awk, ase_bool_t is_top);
static ase_awk_nde_t* __parse_block_dc (ase_awk_t* awk, ase_bool_t is_top);
static ase_awk_nde_t* __parse_statement (ase_awk_t* awk);
static ase_awk_nde_t* __parse_statement_nb (ase_awk_t* awk);

static ase_awk_nde_t* __parse_expression (ase_awk_t* awk);
static ase_awk_nde_t* __parse_expression0 (ase_awk_t* awk);
static ase_awk_nde_t* __parse_basic_expr (ase_awk_t* awk);

static ase_awk_nde_t* __parse_binary_expr (
	ase_awk_t* awk, const __binmap_t* binmap,
	ase_awk_nde_t*(*next_level_func)(ase_awk_t*));

static ase_awk_nde_t* __parse_logical_or (ase_awk_t* awk);
static ase_awk_nde_t* __parse_logical_and (ase_awk_t* awk);
static ase_awk_nde_t* __parse_in (ase_awk_t* awk);
static ase_awk_nde_t* __parse_regex_match (ase_awk_t* awk);
static ase_awk_nde_t* __parse_bitwise_or (ase_awk_t* awk);
static ase_awk_nde_t* __parse_bitwise_or_with_extio (ase_awk_t* awk);
static ase_awk_nde_t* __parse_bitwise_xor (ase_awk_t* awk);
static ase_awk_nde_t* __parse_bitwise_and (ase_awk_t* awk);
static ase_awk_nde_t* __parse_equality (ase_awk_t* awk);
static ase_awk_nde_t* __parse_relational (ase_awk_t* awk);
static ase_awk_nde_t* __parse_shift (ase_awk_t* awk);
static ase_awk_nde_t* __parse_concat (ase_awk_t* awk);
static ase_awk_nde_t* __parse_additive (ase_awk_t* awk);
static ase_awk_nde_t* __parse_multiplicative (ase_awk_t* awk);

static ase_awk_nde_t* __parse_unary (ase_awk_t* awk);
static ase_awk_nde_t* __parse_exponent (ase_awk_t* awk);
static ase_awk_nde_t* __parse_unary_exp (ase_awk_t* awk);
static ase_awk_nde_t* __parse_increment (ase_awk_t* awk);
static ase_awk_nde_t* __parse_primary (ase_awk_t* awk);
static ase_awk_nde_t* __parse_primary_ident (ase_awk_t* awk);

static ase_awk_nde_t* __parse_hashidx (
	ase_awk_t* awk, ase_char_t* name, ase_size_t name_len);
static ase_awk_nde_t* __parse_fncall (
	ase_awk_t* awk, ase_char_t* name, ase_size_t name_len, ase_awk_bfn_t* bfn);
static ase_awk_nde_t* __parse_if (ase_awk_t* awk);
static ase_awk_nde_t* __parse_while (ase_awk_t* awk);
static ase_awk_nde_t* __parse_for (ase_awk_t* awk);
static ase_awk_nde_t* __parse_dowhile (ase_awk_t* awk);
static ase_awk_nde_t* __parse_break (ase_awk_t* awk);
static ase_awk_nde_t* __parse_continue (ase_awk_t* awk);
static ase_awk_nde_t* __parse_return (ase_awk_t* awk);
static ase_awk_nde_t* __parse_exit (ase_awk_t* awk);
static ase_awk_nde_t* __parse_next (ase_awk_t* awk);
static ase_awk_nde_t* __parse_nextfile (ase_awk_t* awk, int out);
static ase_awk_nde_t* __parse_delete (ase_awk_t* awk);
static ase_awk_nde_t* __parse_print (ase_awk_t* awk, int type);

static int __get_token (ase_awk_t* awk);
static int __get_number (ase_awk_t* awk);
static int __get_charstr (ase_awk_t* awk);
static int __get_rexstr (ase_awk_t* awk);
static int __get_string (
	ase_awk_t* awk, ase_char_t end_char,
	ase_char_t esc_char, ase_bool_t keep_esc_char);
static int __get_char (ase_awk_t* awk);
static int __unget_char (ase_awk_t* awk, ase_cint_t c);
static int __skip_spaces (ase_awk_t* awk);
static int __skip_comment (ase_awk_t* awk);
static int __classify_ident (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len);
static int __assign_to_opcode (ase_awk_t* awk);
static int __is_plain_var (ase_awk_nde_t* nde);
static int __is_var (ase_awk_nde_t* nde);

static int __deparse (ase_awk_t* awk);
static int __deparse_func (ase_awk_pair_t* pair, void* arg);
static int __put_char (ase_awk_t* awk, ase_char_t c);
static int __flush (ase_awk_t* awk);

struct __kwent 
{ 
	const ase_char_t* name; 
	ase_size_t name_len;
	int type; 
	int valid; /* the entry is valid when this option is set */
};

static struct __kwent __kwtab[] = 
{
	/* operators */
	{ ASE_T("in"),           2, TOKEN_IN,          0 },

	/* top-level block starters */
	{ ASE_T("BEGIN"),        5, TOKEN_BEGIN,       0 },
	{ ASE_T("END"),          3, TOKEN_END,         0 },
	{ ASE_T("function"),     8, TOKEN_FUNCTION,    0 },
	{ ASE_T("func"),         4, TOKEN_FUNCTION,    0 },

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
	{ ASE_T("next"),         4, TOKEN_NEXT,        0 },
	{ ASE_T("nextfile"),     8, TOKEN_NEXTFILE,    0 },
	{ ASE_T("nextofile"),    9, TOKEN_NEXTOFILE,   ASE_AWK_NEXTOFILE },
	{ ASE_T("delete"),       6, TOKEN_DELETE,      0 },
	{ ASE_T("print"),        5, TOKEN_PRINT,       ASE_AWK_EXTIO },
	{ ASE_T("printf"),       6, TOKEN_PRINTF,      ASE_AWK_EXTIO },

	/* keywords that can start an expression */
	{ ASE_T("getline"),      7, TOKEN_GETLINE,     ASE_AWK_EXTIO },

	{ ASE_NULL,              0, 0,                 0 }
};

struct __bvent
{
	const ase_char_t* name;
	ase_size_t name_len;
	int valid;
};

static struct __bvent __bvtab[] =
{
	{ ASE_T("ARGC"),         4,  0 },
	{ ASE_T("ARGV"),         4,  0 },
	{ ASE_T("CONVFMT"),      7,  0 },
	{ ASE_T("ENVIRON"),      7,  0 },
	{ ASE_T("ERRNO"),        5,  0 },
	{ ASE_T("FILENAME"),     8,  0 },
	{ ASE_T("FNR"),          3,  0 },
	{ ASE_T("FS"),           2,  0 },
	{ ASE_T("IGNORECASE"),  10,  0 },
	{ ASE_T("NF"),           2,  0 },
	{ ASE_T("NR"),           2,  0 },
	{ ASE_T("OFILENAME"),    9,  ASE_AWK_NEXTOFILE },
	{ ASE_T("OFMT"),         4,  0 },
	{ ASE_T("OFS"),          3,  0 },
	{ ASE_T("ORS"),          3,  0 },
	{ ASE_T("RLENGTH"),      7,  0 },
	{ ASE_T("RS"),           2,  0 },
	{ ASE_T("RSTART"),       6,  0 },
	{ ASE_T("SUBSEP"),       6,  0 },
	{ ASE_NULL,              0,  0 }
};

#define GET_CHAR(awk) \
	do { if (__get_char (awk) == -1) return -1; } while(0)

#define GET_CHAR_TO(awk,c) \
	do { \
		if (__get_char (awk) == -1) return -1; \
		c = (awk)->src.lex.curc; \
	} while(0)

#define SET_TOKEN_TYPE(awk,code) \
	do { \
		(awk)->token.prev = (awk)->token.type; \
		(awk)->token.type = (code); \
	} while (0);

#define ADD_TOKEN_CHAR(awk,c) \
	do { \
		if (ase_awk_str_ccat(&(awk)->token.name,(c)) == (ase_size_t)-1) { \
			(awk)->errnum = ASE_AWK_ENOMEM; return -1; \
		} \
	} while (0)

#define ADD_TOKEN_STR(awk,str) \
	do { \
		if (ase_awk_str_cat(&(awk)->token.name,(str)) == (ase_size_t)-1) { \
			(awk)->errnum = ASE_AWK_ENOMEM; return -1; \
		} \
	} while (0)

#define MATCH(awk,token_type) ((awk)->token.type == (token_type))

#define PANIC(awk,code) \
	do { (awk)->errnum = (code); return ASE_NULL; } while (0)

void ase_awk_setmaxparsedepth (ase_awk_t* awk, int types, ase_size_t depth)
{
	if (types & ASE_AWK_DEPTH_BLOCK)
	{
		awk->parse.depth.max.block = depth;
		if (depth <= 0)
			awk->parse.parse_block = __parse_block;
		else
			awk->parse.parse_block = __parse_block_dc;
	}

	if (types & ASE_AWK_DEPTH_EXPR)
	{
		awk->parse.depth.max.expr = depth;
	}
}

int ase_awk_parse (ase_awk_t* awk, ase_awk_srcios_t* srcios)
{
	int n;

	ASE_AWK_ASSERT (awk, srcios != ASE_NULL && srcios->in != ASE_NULL);
	ASE_AWK_ASSERT (awk, awk->parse.depth.cur.loop == 0);
	ASE_AWK_ASSERT (awk, awk->parse.depth.cur.expr == 0);

	ase_awk_clear (awk);
	ASE_AWK_MEMCPY (awk, &awk->src.ios, srcios, ase_sizeof(awk->src.ios));

	n = __parse (awk);

	ASE_AWK_ASSERT (awk, awk->parse.depth.cur.loop == 0);
	ASE_AWK_ASSERT (awk, awk->parse.depth.cur.expr == 0);

	return n;
}

static int __parse (ase_awk_t* awk)
{
	int n = 0, op;

	ASE_AWK_ASSERT (awk, awk->src.ios.in != ASE_NULL);

	op = awk->src.ios.in (
		ASE_AWK_IO_OPEN, awk->src.ios.custom_data, ASE_NULL, 0);
	if (op == -1)
	{
		/* cannot open the source file.
		 * it doesn't even have to call CLOSE */
		awk->errnum = ASE_AWK_ESRCINOPEN;
		return -1;
	}

	if (__add_builtin_globals (awk) == ASE_NULL) 
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

			if (__parse_progunit (awk) == ASE_NULL) 
			{
				n = -1;
				goto exit_parse;
			}
		}
	}

	awk->tree.nglobals = ase_awk_tab_getsize(&awk->parse.globals);

	if (awk->src.ios.out != ASE_NULL) 
	{
		if (__deparse (awk) == -1) 
		{
			n = -1;
			goto exit_parse;
		}
	}

exit_parse:
	if (awk->src.ios.in (
		ASE_AWK_IO_CLOSE, awk->src.ios.custom_data, ASE_NULL, 0) == -1)
	{
		if (n != -1)
		{
			/* this is to keep the earlier error above
			 * that might be more critical than this */
			awk->errnum = ASE_AWK_ESRCINCLOSE;
			n = -1;
		}
	}

	if (n == -1) ase_awk_clear (awk);
	return n;
}

static ase_awk_t* __parse_progunit (ase_awk_t* awk)
{
	/*
	pattern { action }
	function name (parameter-list) { statement }
	*/

	ASE_AWK_ASSERT (awk, awk->parse.depth.cur.loop == 0);

	if ((awk->option & ASE_AWK_EXPLICIT) && MATCH(awk,TOKEN_GLOBAL)) 
	{
		ase_size_t nglobals;

		awk->parse.id.block = PARSE_GLOBAL;

		if (__get_token(awk) == -1) return ASE_NULL;

		nglobals = ase_awk_tab_getsize(&awk->parse.globals);
		if (__collect_globals (awk) == ASE_NULL) 
		{
			ase_awk_tab_remove (
				&awk->parse.globals, nglobals, 
				ase_awk_tab_getsize(&awk->parse.globals) - nglobals);
			return ASE_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_FUNCTION)) 
	{
		awk->parse.id.block = PARSE_FUNCTION;
		if (__parse_function (awk) == ASE_NULL) return ASE_NULL;
	}
	else if (MATCH(awk,TOKEN_BEGIN)) 
	{
		awk->parse.id.block = PARSE_BEGIN;
		if (__get_token(awk) == -1) return ASE_NULL; 

		if ((awk->option & ASE_AWK_BLOCKLESS) &&
		    (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF)))
		{
			/* when the blockless pattern is supported
	   		 * BEGIN and { should be located on the same line */
			PANIC (awk, ASE_AWK_EBEGINBLOCK);
		}

		if (!MATCH(awk,TOKEN_LBRACE)) PANIC (awk, ASE_AWK_ELBRACE);

		awk->parse.id.block = PARSE_BEGIN_BLOCK;
		if (__parse_begin (awk) == ASE_NULL) return ASE_NULL;
	}
	else if (MATCH(awk,TOKEN_END)) 
	{
		awk->parse.id.block = PARSE_END;
		if (__get_token(awk) == -1) return ASE_NULL; 

		if ((awk->option & ASE_AWK_BLOCKLESS) &&
		    (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF)))
		{
			/* when the blockless pattern is supported
	   		 * END and { should be located on the same line */
			PANIC (awk, ASE_AWK_EENDBLOCK);
		}

		if (!MATCH(awk,TOKEN_LBRACE)) PANIC (awk, ASE_AWK_ELBRACE);

		awk->parse.id.block = PARSE_END_BLOCK;
		if (__parse_end (awk) == ASE_NULL) return ASE_NULL;
	}
	else if (MATCH(awk,TOKEN_LBRACE))
	{
		/* patternless block */
		awk->parse.id.block = PARSE_ACTION_BLOCK;
		if (__parse_pattern_block (
			awk, ASE_NULL, ase_false) == ASE_NULL) return ASE_NULL;
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

		awk->parse.id.block = PARSE_PATTERN;

		ptn = __parse_expression (awk);
		if (ptn == ASE_NULL) return ASE_NULL;

		ASE_AWK_ASSERT (awk, ptn->next == ASE_NULL);

		if (MATCH(awk,TOKEN_COMMA))
		{
			if (__get_token (awk) == -1) 
			{
				ase_awk_clrpt (awk, ptn);
				return ASE_NULL;
			}	

			ptn->next = __parse_expression (awk);
			if (ptn->next == ASE_NULL) 
			{
				ase_awk_clrpt (awk, ptn);
				return ASE_NULL;
			}
		}

		if ((awk->option & ASE_AWK_BLOCKLESS) &&
		    (MATCH(awk,TOKEN_NEWLINE) || MATCH(awk,TOKEN_EOF)))
		{
			/* blockless pattern */
			ase_bool_t newline = MATCH(awk,TOKEN_NEWLINE);

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (__parse_pattern_block (
				awk, ptn, ase_true) == ASE_NULL) 
			{
				ase_awk_clrpt (awk, ptn);
				return ASE_NULL;	
			}

			if (newline)
			{
				if (__get_token (awk) == -1) 
				{
					ase_awk_clrpt (awk, ptn);
					return ASE_NULL;
				}	
			}
		}
		else
		{
			/* parse the action block */
			if (!MATCH(awk,TOKEN_LBRACE))
			{
				ase_awk_clrpt (awk, ptn);
				PANIC (awk, ASE_AWK_ELBRACE);
			}

			awk->parse.id.block = PARSE_ACTION_BLOCK;
			if (__parse_pattern_block (
				awk, ptn, ase_false) == ASE_NULL) 
			{
				ase_awk_clrpt (awk, ptn);
				return ASE_NULL;	
			}
		}
	}

	return awk;
}

static ase_awk_nde_t* __parse_function (ase_awk_t* awk)
{
	ase_char_t* name;
	ase_char_t* name_dup;
	ase_size_t name_len;
	ase_awk_nde_t* body;
	ase_awk_afn_t* afn;
	ase_size_t nargs;
	ase_awk_pair_t* pair;
	int n;

	/* eat up the keyword 'function' and get the next token */
	ASE_AWK_ASSERT (awk, MATCH(awk,TOKEN_FUNCTION));
	if (__get_token(awk) == -1) return ASE_NULL;  

	/* match a function name */
	if (!MATCH(awk,TOKEN_IDENT)) 
	{
		/* cannot find a valid identifier for a function name */
		PANIC (awk, ASE_AWK_EIDENT);
	}

	name = ASE_AWK_STR_BUF(&awk->token.name);
	name_len = ASE_AWK_STR_LEN(&awk->token.name);
	if (ase_awk_map_get(&awk->tree.afns, name, name_len) != ASE_NULL) 
	{
		/* the function is defined previously */
		PANIC (awk, ASE_AWK_EDUPFUNC);
	}

	if (awk->option & ASE_AWK_UNIQUE) 
	{
		/* check if it coincides to be a global variable name */
		if (ase_awk_tab_find (
			&awk->parse.globals, 0, name, name_len) != (ase_size_t)-1) 
		{
			PANIC (awk, ASE_AWK_EDUPNAME);
		}
	}

	/* clone the function name before it is overwritten */
	name_dup = ase_awk_strxdup (awk, name, name_len);
	if (name_dup == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);

	/* get the next token */
	if (__get_token(awk) == -1) 
	{
		ASE_AWK_FREE (awk, name_dup);
		return ASE_NULL;  
	}

	/* match a left parenthesis */
	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		/* a function name is not followed by a left parenthesis */
		ASE_AWK_FREE (awk, name_dup);
		PANIC (awk, ASE_AWK_ELPAREN);
	}	

	/* get the next token */
	if (__get_token(awk) == -1) 
	{
		ASE_AWK_FREE (awk, name_dup);
		return ASE_NULL;
	}

	/* make sure that parameter table is empty */
	ASE_AWK_ASSERT (awk, ase_awk_tab_getsize(&awk->parse.params) == 0);

	/* read parameter list */
	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no function parameter found. get the next token */
		if (__get_token(awk) == -1) 
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
				PANIC (awk, ASE_AWK_EIDENT);
			}

			param = ASE_AWK_STR_BUF(&awk->token.name);
			param_len = ASE_AWK_STR_LEN(&awk->token.name);

			if (awk->option & ASE_AWK_UNIQUE) 
			{
				/* check if a parameter conflicts with a function */
				if (ase_awk_strxncmp (name_dup, name_len, param, param_len) == 0 ||
				    ase_awk_map_get (&awk->tree.afns, param, param_len) != ASE_NULL) 
				{
					ASE_AWK_FREE (awk, name_dup);
					ase_awk_tab_clear (&awk->parse.params);
					PANIC (awk, ASE_AWK_EDUPNAME);
				}

				/* NOTE: the following is not a conflict
				 *  global x; 
				 *  function f (x) { print x; } 
				 *  x in print x is a parameter
				 */
			}

			/* check if a parameter conflicts with other parameters */
			if (ase_awk_tab_find (
				&awk->parse.params, 
				0, param, param_len) != (ase_size_t)-1) 
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);
				PANIC (awk, ASE_AWK_EDUPPARAM);
			}

			/* push the parameter to the parameter list */
			if (ase_awk_tab_getsize (
				&awk->parse.params) >= ASE_AWK_MAX_PARAMS)
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);
				PANIC (awk, ASE_AWK_ETOOMANYPARAMS);
			}

			if (ase_awk_tab_add (
				&awk->parse.params, 
				param, param_len) == (ase_size_t)-1) 
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);
				PANIC (awk, ASE_AWK_ENOMEM);
			}	

			if (__get_token (awk) == -1) 
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
				PANIC (awk, ASE_AWK_ECOMMA);
			}

			if (__get_token(awk) == -1) 
			{
				ASE_AWK_FREE (awk, name_dup);
				ase_awk_tab_clear (&awk->parse.params);
				return ASE_NULL;
			}
		}

		if (__get_token(awk) == -1) 
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
		PANIC (awk, ASE_AWK_ELBRACE);
	}
	if (__get_token(awk) == -1) 
	{
		ASE_AWK_FREE (awk, name_dup);
		ase_awk_tab_clear (&awk->parse.params);
		return ASE_NULL; 
	}

	/* actual function body */
	body = awk->parse.parse_block (awk, ase_true);
	if (body == ASE_NULL) 
	{
		ASE_AWK_FREE (awk, name_dup);
		ase_awk_tab_clear (&awk->parse.params);
		return ASE_NULL;
	}

	/* TODO: study furthur if the parameter names should be saved 
	 *       for some reasons */
	nargs = ase_awk_tab_getsize (&awk->parse.params);
	/* parameter names are not required anymore. clear them */
	ase_awk_tab_clear (&awk->parse.params);

	afn = (ase_awk_afn_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_afn_t));
	if (afn == ASE_NULL) 
	{
		ASE_AWK_FREE (awk, name_dup);
		ase_awk_clrpt (awk, body);
		return ASE_NULL;
	}

	afn->name = ASE_NULL; /* function name set below */
	afn->name_len = 0;
	afn->nargs = nargs;
	afn->body = body;

	n = ase_awk_map_putx (&awk->tree.afns, name_dup, name_len, afn, &pair);
	if (n < 0)
	{
		ASE_AWK_FREE (awk, name_dup);
		ase_awk_clrpt (awk, body);
		ASE_AWK_FREE (awk, afn);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	/* duplicate functions should have been detected previously */
	ASE_AWK_ASSERT (awk, n != 0); 

	afn->name = pair->key; /* do some trick to save a string.  */
	afn->name_len = pair->key_len;
	ASE_AWK_FREE (awk, name_dup);

	return body;
}

static ase_awk_nde_t* __parse_begin (ase_awk_t* awk)
{
	ase_awk_nde_t* nde;

	ASE_AWK_ASSERT (awk, MATCH(awk,TOKEN_LBRACE));

	if (__get_token(awk) == -1) return ASE_NULL; 
	nde = awk->parse.parse_block (awk, ase_true);
	if (nde == ASE_NULL) return ASE_NULL;

	awk->tree.begin = nde;
	return nde;
}

static ase_awk_nde_t* __parse_end (ase_awk_t* awk)
{
	ase_awk_nde_t* nde;

	ASE_AWK_ASSERT (awk, MATCH(awk,TOKEN_LBRACE));

	if (__get_token(awk) == -1) return ASE_NULL; 
	nde = awk->parse.parse_block (awk, ase_true);
	if (nde == ASE_NULL) return ASE_NULL;

	awk->tree.end = nde;
	return nde;
}

static ase_awk_chain_t* __parse_pattern_block (
	ase_awk_t* awk, ase_awk_nde_t* ptn, ase_bool_t blockless)
{
	ase_awk_nde_t* nde;
	ase_awk_chain_t* chain;

	if (blockless) nde = ASE_NULL;
	else
	{
		ASE_AWK_ASSERT (awk, MATCH(awk,TOKEN_LBRACE));
		if (__get_token(awk) == -1) return ASE_NULL; 
		nde = awk->parse.parse_block (awk, ase_true);
		if (nde == ASE_NULL) return ASE_NULL;
	}

	chain = (ase_awk_chain_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_chain_t));
	if (chain == ASE_NULL) 
	{
		ase_awk_clrpt (awk, nde);
		PANIC (awk, ASE_AWK_ENOMEM);
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

static ase_awk_nde_t* __parse_block (ase_awk_t* awk, ase_bool_t is_top) 
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

			if (__get_token(awk) == -1) 
			{
				ase_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					ase_awk_tab_getsize(&awk->parse.locals) - nlocals);
				return ASE_NULL;
			}

			if (__collect_locals(awk, nlocals) == ASE_NULL)
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
		if (MATCH(awk,TOKEN_EOF)) 
		{
			ase_awk_tab_remove (
				&awk->parse.locals, nlocals, 
				ase_awk_tab_getsize(&awk->parse.locals) - nlocals);
			if (head != ASE_NULL) ase_awk_clrpt (awk, head);
			PANIC (awk, ASE_AWK_EENDSRC);
		}

		if (MATCH(awk,TOKEN_RBRACE)) 
		{
			if (__get_token(awk) == -1) 
			{
				ase_awk_tab_remove (
					&awk->parse.locals, nlocals, 
					ase_awk_tab_getsize(&awk->parse.locals) - nlocals);
				if (head != ASE_NULL) ase_awk_clrpt (awk, head);
				return ASE_NULL; 
			}
			break;
		}

		nde = __parse_statement (awk);
		if (nde == ASE_NULL) 
		{
			ase_awk_tab_remove (
				&awk->parse.locals, nlocals, 
				ase_awk_tab_getsize(&awk->parse.locals) - nlocals);
			if (head != ASE_NULL) ase_awk_clrpt (awk, head);
			return ASE_NULL;
		}

		/* remove unnecessary statements */
		if (nde->type == ASE_AWK_NDE_NULL ||
		    (nde->type == ASE_AWK_NDE_BLK && 
		     ((ase_awk_nde_blk_t*)nde)->body == ASE_NULL)) continue;
			
		if (curr == ASE_NULL) head = nde;
		else curr->next = nde;	
		curr = nde;
	}

	block = (ase_awk_nde_blk_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_blk_t));
	if (block == ASE_NULL) 
	{
		ase_awk_tab_remove (
			&awk->parse.locals, nlocals, 
			ase_awk_tab_getsize(&awk->parse.locals) - nlocals);
		ase_awk_clrpt (awk, head);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	tmp = ase_awk_tab_getsize(&awk->parse.locals);
	if (tmp > awk->parse.nlocals_max) awk->parse.nlocals_max = tmp;

	ase_awk_tab_remove (
		&awk->parse.locals, nlocals, tmp - nlocals);

	/* adjust the number of locals for a block without any statements */
	/* if (head == ASE_NULL) tmp = 0; */

	block->type = ASE_AWK_NDE_BLK;
	block->next = ASE_NULL;
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

	return (ase_awk_nde_t*)block;
}

static ase_awk_nde_t* __parse_block_dc (ase_awk_t* awk, ase_bool_t is_top) 
{
	ase_awk_nde_t* nde;
		
	ASE_AWK_ASSERT (awk, awk->parse.depth.max.block > 0);

	if (awk->parse.depth.cur.block >= awk->parse.depth.max.block)
	{
		awk->errnum = ASE_AWK_ERECURSION;
		return ASE_NULL;
	}

	awk->parse.depth.cur.block++;
	nde = __parse_block (awk, is_top);
	awk->parse.depth.cur.block--;

	return nde;
}

static ase_awk_t* __add_builtin_globals (ase_awk_t* awk)
{
	struct __bvent* p = __bvtab;
	ase_awk_t* tmp;

	awk->tree.nbglobals = 0;
	while (p->name != ASE_NULL)
	{

		if (p->valid != 0 && (awk->option & p->valid) == 0)
		{
			/* an invalid global variable are still added
			 * to the global variable table with an empty name.
			 * this is to prevent the run-time from looking up
			 * the variable */
			tmp =__add_global (awk, ASE_T(""), 0, 1);
		}
		else 
		{
			tmp =__add_global (awk, p->name, p->name_len, 0);
		}
		if (tmp == ASE_NULL) return ASE_NULL;

		awk->tree.nbglobals++;
		p++;
	}

	return awk;
}

static ase_awk_t* __add_global (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len, int force)
{
	if (!force)
	{
		if (awk->option & ASE_AWK_UNIQUE) 
		{
			/* check if it conflict with a function name */
			if (ase_awk_map_get (
				&awk->tree.afns, name, len) != ASE_NULL) 
			{
				PANIC (awk, ASE_AWK_EDUPNAME);
			}
		}

		/* check if it conflicts with other global variable names */
		if (ase_awk_tab_find (
			&awk->parse.globals, 0, name, len) != (ase_size_t)-1) 
		{ 
			PANIC (awk, ASE_AWK_EDUPVAR);
		}
	}

	if (ase_awk_tab_getsize(&awk->parse.globals) >= ASE_AWK_MAX_GLOBALS)
	{
		PANIC (awk, ASE_AWK_ETOOMANYGLOBALS);
	}

	if (ase_awk_tab_add (&awk->parse.globals, name, len) == (ase_size_t)-1) 
	{
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	return awk;
}

static ase_awk_t* __collect_globals (ase_awk_t* awk)
{
	while (1) 
	{
		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			PANIC (awk, ASE_AWK_EIDENT);
		}

		if (__add_global (
			awk,
			ASE_AWK_STR_BUF(&awk->token.name),
			ASE_AWK_STR_LEN(&awk->token.name),
			0) == ASE_NULL) return ASE_NULL;

		if (__get_token(awk) == -1) return ASE_NULL;

		if (MATCH(awk,TOKEN_SEMICOLON)) break;

		if (!MATCH(awk,TOKEN_COMMA)) 
		{
			PANIC (awk, ASE_AWK_ECOMMA);
		}

		if (__get_token(awk) == -1) return ASE_NULL;
	}

	/* skip a semicolon */
	if (__get_token(awk) == -1) return ASE_NULL;

	return awk;
}

static ase_awk_t* __collect_locals (ase_awk_t* awk, ase_size_t nlocals)
{
	ase_char_t* local;
	ase_size_t local_len;

	while (1) 
	{
		if (!MATCH(awk,TOKEN_IDENT)) 
		{
			PANIC (awk, ASE_AWK_EIDENT);
		}

		local = ASE_AWK_STR_BUF(&awk->token.name);
		local_len = ASE_AWK_STR_LEN(&awk->token.name);

		/* NOTE: it is not checked againt globals names */

		if (awk->option & ASE_AWK_UNIQUE) 
		{
			/* check if it conflict with a function name */
			if (ase_awk_map_get (
				&awk->tree.afns, local, local_len) != ASE_NULL) 
			{
				PANIC (awk, ASE_AWK_EDUPNAME);
			}
		}

		/* check if it conflicts with a paremeter name */
		if (ase_awk_tab_find (&awk->parse.params,
			0, local, local_len) != (ase_size_t)-1) 
		{
			PANIC (awk, ASE_AWK_EDUPNAME);
		}

		/* check if it conflicts with other local variable names */
		if (ase_awk_tab_find (&awk->parse.locals, 
			((awk->option & ASE_AWK_SHADING)? nlocals: 0),
			local, local_len) != (ase_size_t)-1)
		{
			PANIC (awk, ASE_AWK_EDUPVAR);	
		}

		if (ase_awk_tab_getsize(&awk->parse.locals) >= ASE_AWK_MAX_LOCALS)
		{
			PANIC (awk, ASE_AWK_ETOOMANYLOCALS);
		}

		if (ase_awk_tab_add (
			&awk->parse.locals, local, local_len) == (ase_size_t)-1) 
		{
			PANIC (awk, ASE_AWK_ENOMEM);
		}

		if (__get_token(awk) == -1) return ASE_NULL;

		if (MATCH(awk,TOKEN_SEMICOLON)) break;

		if (!MATCH(awk,TOKEN_COMMA)) PANIC (awk, ASE_AWK_ECOMMA);

		if (__get_token(awk) == -1) return ASE_NULL;
	}

	/* skip a semicolon */
	if (__get_token(awk) == -1) return ASE_NULL;

	return awk;
}

static ase_awk_nde_t* __parse_statement (ase_awk_t* awk)
{
	ase_awk_nde_t* nde;

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* null statement */	
		nde = (ase_awk_nde_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_t));
		if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);

		nde->type = ASE_AWK_NDE_NULL;
		nde->next = ASE_NULL;

		if (__get_token(awk) == -1) 
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_LBRACE)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL; 
		nde = awk->parse.parse_block (awk, ase_false);
	}
	else 
	{
		int old_id = awk->parse.id.stmnt;
		awk->parse.id.stmnt = awk->token.type;
		nde = __parse_statement_nb (awk);
		awk->parse.id.stmnt = old_id;
awk->parse.nl_semicolon = 0;
	}

	return nde;
}

static ase_awk_nde_t* __parse_statement_nb (ase_awk_t* awk)
{
	ase_awk_nde_t* nde;

	/* keywords that don't require any terminating semicolon */
	if (MATCH(awk,TOKEN_IF)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		return __parse_if (awk);
	}
	else if (MATCH(awk,TOKEN_WHILE)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		
		awk->parse.depth.cur.loop++;
		nde = __parse_while (awk);
		awk->parse.depth.cur.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_FOR)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;

		awk->parse.depth.cur.loop++;
		nde = __parse_for (awk);
		awk->parse.depth.cur.loop--;

		return nde;
	}

awk->parse.nl_semicolon = 1;
	/* keywords that require a terminating semicolon */
	if (MATCH(awk,TOKEN_DO)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;

		awk->parse.depth.cur.loop++;
		nde = __parse_dowhile (awk);
		awk->parse.depth.cur.loop--;

		return nde;
	}
	else if (MATCH(awk,TOKEN_BREAK)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_break (awk);
	}
	else if (MATCH(awk,TOKEN_CONTINUE)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_continue (awk);
	}
	else if (MATCH(awk,TOKEN_RETURN)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_return (awk);
	}
	else if (MATCH(awk,TOKEN_EXIT)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_exit (awk);
	}
	else if (MATCH(awk,TOKEN_NEXT)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_next (awk);
	}
	else if (MATCH(awk,TOKEN_NEXTFILE)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_nextfile (awk, 0);
	}
	else if (MATCH(awk,TOKEN_NEXTOFILE))
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_nextfile (awk, 1);
	}
	else if (MATCH(awk,TOKEN_DELETE)) 
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_delete (awk);
	}
	else if (MATCH(awk,TOKEN_PRINT))
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_print (awk, ASE_AWK_NDE_PRINT);
	}
	else if (MATCH(awk,TOKEN_PRINTF))
	{
		if (__get_token(awk) == -1) return ASE_NULL;
		nde = __parse_print (awk, ASE_AWK_NDE_PRINTF);
	}
	else 
	{
		nde = __parse_expression(awk);
	}

/* TODO: newline ... */
awk->parse.nl_semicolon = 0;
	if (nde == ASE_NULL) return ASE_NULL;

	/* check if a statement ends with a semicolon */
	if (!MATCH(awk,TOKEN_SEMICOLON)) 
	{
		if (nde != ASE_NULL) ase_awk_clrpt (awk, nde);
		PANIC (awk, ASE_AWK_ESEMICOLON);
	}

	/* eat up the semicolon and read in the next token */
	if (__get_token(awk) == -1) 
	{
		if (nde != ASE_NULL) ase_awk_clrpt (awk, nde);
		return ASE_NULL;
	}

	return nde;
}

static ase_awk_nde_t* __parse_expression (ase_awk_t* awk)
{
	ase_awk_nde_t* nde;

	awk->parse.depth.cur.expr++;
	nde = __parse_expression0 (awk);
	awk->parse.depth.cur.expr--;

	return nde;
}

static ase_awk_nde_t* __parse_expression0 (ase_awk_t* awk)
{
	ase_awk_nde_t* x, * y;
	ase_awk_nde_ass_t* nde;
	int opcode;

	x = __parse_basic_expr (awk);
	if (x == ASE_NULL) return ASE_NULL;

	opcode = __assign_to_opcode (awk);
	if (opcode == -1) 
	{
		/* no assignment operator found. */
		return x;
	}

	ASE_AWK_ASSERT (awk, x->next == ASE_NULL);
	if (!__is_var(x) && x->type != ASE_AWK_NDE_POS) 
	{
		ase_awk_clrpt (awk, x);
		PANIC (awk, ASE_AWK_EASSIGNMENT);
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, x);
		return ASE_NULL;
	}

	y = __parse_basic_expr (awk);
	if (y == ASE_NULL) 
	{
		ase_awk_clrpt (awk, x);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_ass_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_ass_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, x);
		ase_awk_clrpt (awk, y);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	nde->type = ASE_AWK_NDE_ASS;
	nde->next = ASE_NULL;
	nde->opcode = opcode;
	nde->left = x;
	nde->right = y;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_basic_expr (ase_awk_t* awk)
{
	ase_awk_nde_t* nde, * n1, * n2;
	
	nde = __parse_logical_or (awk);
	if (nde == ASE_NULL) return ASE_NULL;

	if (MATCH(awk,TOKEN_QUEST))
	{ 
		ase_awk_nde_cnd_t* tmp;

		if (__get_token(awk) == -1) return ASE_NULL;

		n1 = __parse_basic_expr (awk);
		if (n1 == ASE_NULL) 
		{
			ase_awk_clrpt (awk, nde);
			return ASE_NULL;
		}

		if (!MATCH(awk,TOKEN_COLON)) PANIC (awk, ASE_AWK_ECOLON);
		if (__get_token(awk) == -1) return ASE_NULL;

		n2 = __parse_basic_expr (awk);
		if (n2 == ASE_NULL)
		{
			ase_awk_clrpt (awk, nde);
			ase_awk_clrpt (awk, n1);
			return ASE_NULL;
		}

		tmp = (ase_awk_nde_cnd_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_nde_cnd_t));
		if (tmp == ASE_NULL)
		{
			ase_awk_clrpt (awk, nde);
			ase_awk_clrpt (awk, n1);
			ase_awk_clrpt (awk, n2);
			return ASE_NULL;
		}

		tmp->type = ASE_AWK_NDE_CND;
		tmp->next = ASE_NULL;
		tmp->test = nde;
		tmp->left = n1;
		tmp->right = n2;

		nde = (ase_awk_nde_t*)tmp;
	}

	return nde;
}

static ase_awk_nde_t* __parse_binary_expr (
	ase_awk_t* awk, const __binmap_t* binmap,
	ase_awk_nde_t*(*next_level_func)(ase_awk_t*))
{
	ase_awk_nde_exp_t* nde;
	ase_awk_nde_t* left, * right;
	int opcode;

	left = next_level_func (awk);
	if (left == ASE_NULL) return ASE_NULL;
	
	while (1) 
	{
		const __binmap_t* p = binmap;
		ase_bool_t matched = ase_false;

		while (p->token != TOKEN_EOF)
		{
			if (MATCH(awk,p->token)) 
			{
				opcode = p->binop;
				matched = ase_true;
				break;
			}
			p++;
		}
		if (!matched) break;

		if (__get_token(awk) == -1) 
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL; 
		}

		right = next_level_func (awk);
		if (right == ASE_NULL) 
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL;
		}

#if 0
		/* TODO: enhance constant folding. do it in a better way */
		/* TODO: differentiate different types of numbers ... */
		if (left->type == ASE_AWK_NDE_INT && 
		    right->type == ASE_AWK_NDE_INT) 
		{
			ase_long_t l, r;

			l = ((ase_awk_nde_int_t*)left)->val; 
			r = ((ase_awk_nde_int_t*)right)->val; 

			/* TODO: more operators */
			if (opcode == ASE_AWK_BINOP_PLUS) l += r;
			else if (opcode == ASE_AWK_BINOP_MINUS) l -= r;
			else if (opcode == ASE_AWK_BINOP_MUL) l *= r;
			else if (opcode == ASE_AWK_BINOP_DIV && r != 0) l /= r;
			else if (opcode == ASE_AWK_BINOP_MOD && r != 0) l %= r;
			else goto skip_constant_folding;

			ase_awk_clrpt (awk, right);
			((ase_awk_nde_int_t*)left)->val = l;

			if (((ase_awk_nde_int_t*)left)->str != ASE_NULL)
			{
				ASE_AWK_FREE (awk, ((ase_awk_nde_int_t*)left)->str);
				((ase_awk_nde_int_t*)left)->str = ASE_NULL;
				((ase_awk_nde_int_t*)left)->len = 0;
			}

			continue;
		} 
		else if (left->type == ASE_AWK_NDE_REAL && 
		         right->type == ASE_AWK_NDE_REAL) 
		{
			ase_real_t l, r;

			l = ((ase_awk_nde_real_t*)left)->val; 
			r = ((ase_awk_nde_real_t*)right)->val; 

			/* TODO: more operators */
			if (opcode == ASE_AWK_BINOP_PLUS) l += r;
			else if (opcode == ASE_AWK_BINOP_MINUS) l -= r;
			else if (opcode == ASE_AWK_BINOP_MUL) l *= r;
			else if (opcode == ASE_AWK_BINOP_DIV) l /= r;
			else goto skip_constant_folding;

			ase_awk_clrpt (awk, right);
			((ase_awk_nde_real_t*)left)->val = l;

			if (((ase_awk_nde_real_t*)left)->str != ASE_NULL)
			{
				ASE_AWK_FREE (awk, ((ase_awk_nde_real_t*)left)->str);
				((ase_awk_nde_real_t*)left)->str = ASE_NULL;
				((ase_awk_nde_real_t*)left)->len = 0;
			}

			continue;
		}
		/* TODO: enhance constant folding more... */

	skip_constant_folding:
#endif
		nde = (ase_awk_nde_exp_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_nde_exp_t));
		if (nde == ASE_NULL) 
		{
			ase_awk_clrpt (awk, right);
			ase_awk_clrpt (awk, left);
			PANIC (awk, ASE_AWK_ENOMEM);
		}

		nde->type = ASE_AWK_NDE_EXP_BIN;
		nde->next = ASE_NULL;
		nde->opcode = opcode; 
		nde->left = left;
		nde->right = right;

		left = (ase_awk_nde_t*)nde;
	}

	return left;
}

static ase_awk_nde_t* __parse_logical_or (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_LOR, ASE_AWK_BINOP_LOR },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_logical_and);
}

static ase_awk_nde_t* __parse_logical_and (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_LAND, ASE_AWK_BINOP_LAND },
		{ TOKEN_EOF,  0 }
	};

	return __parse_binary_expr (awk, map, __parse_in);
}

static ase_awk_nde_t* __parse_in (ase_awk_t* awk)
{
	/* 
	static __binmap_t map[] =
	{
		{ TOKEN_IN, ASE_AWK_BINOP_IN },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_regex_match);
	*/

	ase_awk_nde_exp_t* nde;
	ase_awk_nde_t* left, * right;

	left = __parse_regex_match (awk);
	if (left == ASE_NULL) return ASE_NULL;

	while (1)
	{
		if (!MATCH(awk,TOKEN_IN)) break;

		if (__get_token(awk) == -1) 
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL; 
		}

		right = __parse_regex_match (awk);
		if (right == ASE_NULL) 
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL;
		}

		if (!__is_plain_var(right))
		{
			ase_awk_clrpt (awk, right);
			ase_awk_clrpt (awk, left);
			PANIC (awk, ASE_AWK_ENOTVAR);
		}

		nde = (ase_awk_nde_exp_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_nde_exp_t));
		if (nde == ASE_NULL) 
		{
			ase_awk_clrpt (awk, right);
			ase_awk_clrpt (awk, left);
			PANIC (awk, ASE_AWK_ENOMEM);
		}

		nde->type = ASE_AWK_NDE_EXP_BIN;
		nde->next = ASE_NULL;
		nde->opcode = ASE_AWK_BINOP_IN; 
		nde->left = left;
		nde->right = right;

		left = (ase_awk_nde_t*)nde;
	}

	return left;
}

static ase_awk_nde_t* __parse_regex_match (ase_awk_t* awk)
{
	static __binmap_t map[] =
	{
		{ TOKEN_TILDE, ASE_AWK_BINOP_MA },
		{ TOKEN_NM,    ASE_AWK_BINOP_NM },
		{ TOKEN_EOF,   0 },
	};

	return __parse_binary_expr (awk, map, __parse_bitwise_or);
}

static ase_awk_nde_t* __parse_bitwise_or (ase_awk_t* awk)
{
	if (awk->option & ASE_AWK_EXTIO)
	{
		return __parse_bitwise_or_with_extio (awk);
	}
	else
	{
		static __binmap_t map[] = 
		{
			{ TOKEN_BOR, ASE_AWK_BINOP_BOR },
			{ TOKEN_EOF, 0 }
		};

		return __parse_binary_expr (awk, map, __parse_bitwise_xor);
	}
}

static ase_awk_nde_t* __parse_bitwise_or_with_extio (ase_awk_t* awk)
{
	ase_awk_nde_t* left, * right;

	left = __parse_bitwise_xor (awk);
	if (left == ASE_NULL) return ASE_NULL;

	while (1)
	{
		int in_type;

		if (MATCH(awk,TOKEN_BOR)) 
			in_type = ASE_AWK_IN_PIPE;
		else if (MATCH(awk,TOKEN_BORAND)) 
			in_type = ASE_AWK_IN_COPROC;
		else break;
		
		if (__get_token(awk) == -1)
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL;
		}

		if (MATCH(awk,TOKEN_GETLINE))
		{
			ase_awk_nde_getline_t* nde;
			ase_awk_nde_t* var = ASE_NULL;

			/* piped getline */
			if (__get_token(awk) == -1)
			{
				ase_awk_clrpt (awk, left);
				return ASE_NULL;
			}

			/* TODO: is this correct? */

			if (MATCH(awk,TOKEN_IDENT))
			{
				/* command | getline var */

				var = __parse_primary (awk);
				if (var == ASE_NULL) 
				{
					ase_awk_clrpt (awk, left);
					return ASE_NULL;
				}
			}

			nde = (ase_awk_nde_getline_t*) ASE_AWK_MALLOC (
				awk, ase_sizeof(ase_awk_nde_getline_t));
			if (nde == ASE_NULL)
			{
				ase_awk_clrpt (awk, left);
				PANIC (awk, ASE_AWK_ENOMEM);
			}

			nde->type = ASE_AWK_NDE_GETLINE;
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
				PANIC (awk, ASE_AWK_EGETLINE);
			}

			right = __parse_bitwise_xor (awk);
			if (right == ASE_NULL)
			{
				ase_awk_clrpt (awk, left);
				return ASE_NULL;
			}

			/* TODO: do constant folding */

			nde = (ase_awk_nde_exp_t*) ASE_AWK_MALLOC (
				awk, ase_sizeof(ase_awk_nde_exp_t));
			if (nde == ASE_NULL)
			{
				ase_awk_clrpt (awk, right);
				ase_awk_clrpt (awk, left);
				PANIC (awk, ASE_AWK_ENOMEM);
			}

			nde->type = ASE_AWK_NDE_EXP_BIN;
			nde->next = ASE_NULL;
			nde->opcode = ASE_AWK_BINOP_BOR;
			nde->left = left;
			nde->right = right;

			left = (ase_awk_nde_t*)nde;
		}
	}

	return left;
}

static ase_awk_nde_t* __parse_bitwise_xor (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_BXOR, ASE_AWK_BINOP_BXOR },
		{ TOKEN_EOF,  0 }
	};

	return __parse_binary_expr (awk, map, __parse_bitwise_and);
}

static ase_awk_nde_t* __parse_bitwise_and (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_BAND, ASE_AWK_BINOP_BAND },
		{ TOKEN_EOF,  0 }
	};

	return __parse_binary_expr (awk, map, __parse_equality);
}

static ase_awk_nde_t* __parse_equality (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_EQ, ASE_AWK_BINOP_EQ },
		{ TOKEN_NE, ASE_AWK_BINOP_NE },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_relational);
}

static ase_awk_nde_t* __parse_relational (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_GT, ASE_AWK_BINOP_GT },
		{ TOKEN_GE, ASE_AWK_BINOP_GE },
		{ TOKEN_LT, ASE_AWK_BINOP_LT },
		{ TOKEN_LE, ASE_AWK_BINOP_LE },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_shift);
}

static ase_awk_nde_t* __parse_shift (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_LSHIFT, ASE_AWK_BINOP_LSHIFT },
		{ TOKEN_RSHIFT, ASE_AWK_BINOP_RSHIFT },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_concat);
}

static ase_awk_nde_t* __parse_concat (ase_awk_t* awk)
{
	ase_awk_nde_exp_t* nde;
	ase_awk_nde_t* left, * right;

	left = __parse_additive (awk);
	if (left == ASE_NULL) return ASE_NULL;

	/* TODO: write a better code to do this.... 
	 *       first of all, is the following check sufficient? */
	while (MATCH(awk,TOKEN_LPAREN) || 
	       MATCH(awk,TOKEN_DOLLAR) ||
	       awk->token.type >= TOKEN_GETLINE)
	{
		right = __parse_additive (awk);
		if (right == ASE_NULL) 
		{
			ase_awk_clrpt (awk, left);
			return ASE_NULL;
		}

		nde = (ase_awk_nde_exp_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_nde_exp_t));
		if (nde == ASE_NULL)
		{
			ase_awk_clrpt (awk, left);
			ase_awk_clrpt (awk, right);
			PANIC (awk, ASE_AWK_ENOMEM);
		}

		nde->type = ASE_AWK_NDE_EXP_BIN;
		nde->next = ASE_NULL;
		nde->opcode = ASE_AWK_BINOP_CONCAT;
		nde->left = left;
		nde->right = right;
		
		left = (ase_awk_nde_t*)nde;
	}

	return left;
}

static ase_awk_nde_t* __parse_additive (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_PLUS, ASE_AWK_BINOP_PLUS },
		{ TOKEN_MINUS, ASE_AWK_BINOP_MINUS },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_multiplicative);
}

static ase_awk_nde_t* __parse_multiplicative (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_MUL, ASE_AWK_BINOP_MUL },
		{ TOKEN_DIV, ASE_AWK_BINOP_DIV },
		{ TOKEN_MOD, ASE_AWK_BINOP_MOD },
		/* { TOKEN_EXP, ASE_AWK_BINOP_EXP }, */
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_unary);
}

static ase_awk_nde_t* __parse_unary (ase_awk_t* awk)
{
	ase_awk_nde_exp_t* nde; 
	ase_awk_nde_t* left;
	int opcode;

	opcode = (MATCH(awk,TOKEN_PLUS))?  ASE_AWK_UNROP_PLUS:
	         (MATCH(awk,TOKEN_MINUS))? ASE_AWK_UNROP_MINUS:
	         (MATCH(awk,TOKEN_NOT))?   ASE_AWK_UNROP_NOT:
	         (MATCH(awk,TOKEN_TILDE))? ASE_AWK_UNROP_BNOT: -1;

	/*if (opcode == -1) return __parse_increment (awk);*/
	if (opcode == -1) return __parse_exponent (awk);

	if (__get_token(awk) == -1) return ASE_NULL;

	left = __parse_unary (awk);
	if (left == ASE_NULL) return ASE_NULL;

	nde = (ase_awk_nde_exp_t*) 
		ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_exp_t));
	if (nde == ASE_NULL)
	{
		ase_awk_clrpt (awk, left);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	nde->type = ASE_AWK_NDE_EXP_UNR;
	nde->next = ASE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = ASE_NULL;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_exponent (ase_awk_t* awk)
{
	static __binmap_t map[] = 
	{
		{ TOKEN_EXP, ASE_AWK_BINOP_EXP },
		{ TOKEN_EOF, 0 }
	};

	return __parse_binary_expr (awk, map, __parse_unary_exp);
}

static ase_awk_nde_t* __parse_unary_exp (ase_awk_t* awk)
{
	ase_awk_nde_exp_t* nde; 
	ase_awk_nde_t* left;
	int opcode;

	opcode = (MATCH(awk,TOKEN_PLUS))?  ASE_AWK_UNROP_PLUS:
	         (MATCH(awk,TOKEN_MINUS))? ASE_AWK_UNROP_MINUS:
	         (MATCH(awk,TOKEN_NOT))?   ASE_AWK_UNROP_NOT:
	         (MATCH(awk,TOKEN_TILDE))? ASE_AWK_UNROP_BNOT: -1;

	if (opcode == -1) return __parse_increment (awk);

	if (__get_token(awk) == -1) return ASE_NULL;

	left = __parse_unary (awk);
	if (left == ASE_NULL) return ASE_NULL;

	nde = (ase_awk_nde_exp_t*) 
		ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_exp_t));
	if (nde == ASE_NULL)
	{
		ase_awk_clrpt (awk, left);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	nde->type = ASE_AWK_NDE_EXP_UNR;
	nde->next = ASE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = ASE_NULL;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_increment (ase_awk_t* awk)
{
	ase_awk_nde_exp_t* nde;
	ase_awk_nde_t* left;
	int type, opcode, opcode1, opcode2;

	opcode1 = MATCH(awk,TOKEN_PLUSPLUS)? ASE_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? ASE_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1)
	{
		if (__get_token(awk) == -1) return ASE_NULL;
	}

	left = __parse_primary (awk);
	if (left == ASE_NULL) return ASE_NULL;

	opcode2 = MATCH(awk,TOKEN_PLUSPLUS)? ASE_AWK_INCOP_PLUS:
	          MATCH(awk,TOKEN_MINUSMINUS)? ASE_AWK_INCOP_MINUS: -1;

	if (opcode1 != -1 && opcode2 != -1)
	{
		ase_awk_clrpt (awk, left);
		PANIC (awk, ASE_AWK_ELVALUE);
	}
	else if (opcode1 == -1 && opcode2 == -1)
	{
		return left;
	}
	else if (opcode1 != -1) 
	{
		type = ASE_AWK_NDE_EXP_INCPRE;
		opcode = opcode1;
	}
	else if (opcode2 != -1) 
	{
		type = ASE_AWK_NDE_EXP_INCPST;
		opcode = opcode2;

		if (__get_token(awk) == -1) return ASE_NULL;
	}

	nde = (ase_awk_nde_exp_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_exp_t));
	if (nde == ASE_NULL)
	{
		ase_awk_clrpt (awk, left);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	nde->type = type;
	nde->next = ASE_NULL;
	nde->opcode = opcode;
	nde->left = left;
	nde->right = ASE_NULL;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_primary (ase_awk_t* awk)
{
	if (MATCH(awk,TOKEN_IDENT))  
	{
		return __parse_primary_ident (awk);
	}
	else if (MATCH(awk,TOKEN_INT)) 
	{
		ase_awk_nde_int_t* nde;

		nde = (ase_awk_nde_int_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_nde_int_t));
		if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);

		nde->type = ASE_AWK_NDE_INT;
		nde->next = ASE_NULL;
		nde->val = ase_awk_strxtolong (awk, 
			ASE_AWK_STR_BUF(&awk->token.name), 
			ASE_AWK_STR_LEN(&awk->token.name), 0, ASE_NULL);
		nde->str = ase_awk_strxdup (awk,
			ASE_AWK_STR_BUF(&awk->token.name),
			ASE_AWK_STR_LEN(&awk->token.name));
		if (nde->str == ASE_NULL)
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;			
		}
		nde->len = ASE_AWK_STR_LEN(&awk->token.name);

		ASE_AWK_ASSERT (awk, 
			ASE_AWK_STR_LEN(&awk->token.name) ==
			ase_awk_strlen(ASE_AWK_STR_BUF(&awk->token.name)));

		if (__get_token(awk) == -1) 
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
			awk, ase_sizeof(ase_awk_nde_real_t));
		if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);

		nde->type = ASE_AWK_NDE_REAL;
		nde->next = ASE_NULL;
		nde->val = ase_awk_strxtoreal (awk, 
			ASE_AWK_STR_BUF(&awk->token.name), 
			ASE_AWK_STR_LEN(&awk->token.name), ASE_NULL);
		nde->str = ase_awk_strxdup (awk,
			ASE_AWK_STR_BUF(&awk->token.name),
			ASE_AWK_STR_LEN(&awk->token.name));
		if (nde->str == ASE_NULL)
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;			
		}
		nde->len = ASE_AWK_STR_LEN(&awk->token.name);

		ASE_AWK_ASSERT (awk, 
			ASE_AWK_STR_LEN(&awk->token.name) ==
			ase_awk_strlen(ASE_AWK_STR_BUF(&awk->token.name)));

		if (__get_token(awk) == -1) 
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
			awk, ase_sizeof(ase_awk_nde_str_t));
		if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);

		nde->type = ASE_AWK_NDE_STR;
		nde->next = ASE_NULL;
		nde->len = ASE_AWK_STR_LEN(&awk->token.name);
		nde->buf = ase_awk_strxdup (
			awk, ASE_AWK_STR_BUF(&awk->token.name), nde->len);
		if (nde->buf == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, nde);
			PANIC (awk, ASE_AWK_ENOMEM);
		}

		if (__get_token(awk) == -1) 
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
		ase_awk_str_clear (&awk->token.name);
		if (__get_rexstr (awk) == -1) return ASE_NULL;
		ASE_AWK_ASSERT (awk, MATCH(awk,TOKEN_REX));

		nde = (ase_awk_nde_rex_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_nde_rex_t));
		if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);

		nde->type = ASE_AWK_NDE_REX;
		nde->next = ASE_NULL;

		nde->len = ASE_AWK_STR_LEN(&awk->token.name);
		nde->buf = ase_awk_strxdup (
			awk,
			ASE_AWK_STR_BUF(&awk->token.name),
			ASE_AWK_STR_LEN(&awk->token.name));
		if (nde->buf == ASE_NULL)
		{
			ASE_AWK_FREE (awk, nde);
			PANIC (awk, ASE_AWK_ENOMEM);
		}

		nde->code = ase_awk_buildrex (
			awk,
			ASE_AWK_STR_BUF(&awk->token.name), 
			ASE_AWK_STR_LEN(&awk->token.name), 
			&errnum);
		if (nde->code == ASE_NULL)
		{
			ASE_AWK_FREE (awk, nde->buf);
			ASE_AWK_FREE (awk, nde);
			PANIC (awk, errnum);
		}

		if (__get_token(awk) == -1) 
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

		if (__get_token(awk)) return ASE_NULL;
		
		prim = __parse_primary (awk);
		if (prim == ASE_NULL) return ASE_NULL;

		nde = (ase_awk_nde_pos_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_nde_pos_t));
		if (nde == ASE_NULL) 
		{
			ase_awk_clrpt (awk, prim);
			PANIC (awk, ASE_AWK_ENOMEM);
		}

		nde->type = ASE_AWK_NDE_POS;
		nde->next = ASE_NULL;
		nde->val = prim;

		return (ase_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		ase_awk_nde_t* nde;
		ase_awk_nde_t* last;

		/* eat up the left parenthesis */
		if (__get_token(awk) == -1) return ASE_NULL;

		/* parse the sub-expression inside the parentheses */
		nde = __parse_expression (awk);
		if (nde == ASE_NULL) return ASE_NULL;

		/* parse subsequent expressions separated by a comma, if any */
		last = nde;
		ASE_AWK_ASSERT (awk, last->next == ASE_NULL);

		while (MATCH(awk,TOKEN_COMMA))
		{
			ase_awk_nde_t* tmp;

			if (__get_token(awk) == -1) 
			{
				ase_awk_clrpt (awk, nde);
				return ASE_NULL;
			}	

			tmp = __parse_expression (awk);
			if (tmp == ASE_NULL) 
			{
				ase_awk_clrpt (awk, nde);
				return ASE_NULL;
			}

			ASE_AWK_ASSERT (awk, tmp->next == ASE_NULL);
			last->next = tmp;
			last = tmp;
		} 
		/* ----------------- */

		/* check for the closing parenthesis */
		if (!MATCH(awk,TOKEN_RPAREN)) 
		{
			ase_awk_clrpt (awk, nde);
			PANIC (awk, ASE_AWK_ERPAREN);
		}

		if (__get_token(awk) == -1) 
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
					PANIC (awk, ASE_AWK_EIN);
				}
			}

			tmp = (ase_awk_nde_grp_t*) ASE_AWK_MALLOC (
				awk, ase_sizeof(ase_awk_nde_grp_t));
			if (tmp == ASE_NULL)
			{
				ase_awk_clrpt (awk, nde);
				PANIC (awk, ASE_AWK_ENOMEM);
			}	

			tmp->type = ASE_AWK_NDE_GRP;
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

		if (__get_token(awk) == -1) return ASE_NULL;

		if (MATCH(awk,TOKEN_IDENT))
		{
			/* getline var */
			var = __parse_primary (awk);
			if (var == ASE_NULL) return ASE_NULL;
		}

		if (MATCH(awk, TOKEN_LT))
		{
			/* getline [var] < file */
			if (__get_token(awk) == -1)
			{
				if (var != ASE_NULL) ase_awk_clrpt (awk, var);
				return ASE_NULL;
			}

			/* TODO: is this correct? */
			/*in = __parse_expression (awk);*/
			in = __parse_primary (awk);
			if (in == ASE_NULL)
			{
				if (var != ASE_NULL) ase_awk_clrpt (awk, var);
				return ASE_NULL;
			}
		}

		nde = (ase_awk_nde_getline_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_nde_getline_t));
		if (nde == ASE_NULL)
		{
			if (var != ASE_NULL) ase_awk_clrpt (awk, var);
			if (in != ASE_NULL) ase_awk_clrpt (awk, in);
			PANIC (awk, ASE_AWK_ENOMEM);
		}

		nde->type = ASE_AWK_NDE_GETLINE;
		nde->next = ASE_NULL;
		nde->var = var;
		nde->in_type = (in == ASE_NULL)? 
			ASE_AWK_IN_CONSOLE: ASE_AWK_IN_FILE;
		nde->in = in;

		return (ase_awk_nde_t*)nde;
	}

	/* valid expression introducer is expected */
	awk->errnum = ASE_AWK_EEXPRESSION;
	return ASE_NULL;
}

static ase_awk_nde_t* __parse_primary_ident (ase_awk_t* awk)
{
	ase_char_t* name_dup;
	ase_size_t name_len;
	ase_awk_bfn_t* bfn;

	ASE_AWK_ASSERT (awk, MATCH(awk,TOKEN_IDENT));

	name_dup = ase_awk_strxdup (
		awk, 
		ASE_AWK_STR_BUF(&awk->token.name), 
		ASE_AWK_STR_LEN(&awk->token.name));
	if (name_dup == ASE_NULL) 
	{
		awk->errnum = ASE_AWK_ENOMEM;
		return ASE_NULL;
	}
	name_len = ASE_AWK_STR_LEN(&awk->token.name);

	if (__get_token(awk) == -1) 
	{
		ASE_AWK_FREE (awk, name_dup);
		return ASE_NULL;			
	}

	/* check if name_dup is a built-in function name */
	bfn = ase_awk_getbfn (awk, name_dup, name_len);
	if (bfn != ASE_NULL)
	{
		ase_awk_nde_t* nde;

		ASE_AWK_FREE (awk, name_dup);
		if (!MATCH(awk,TOKEN_LPAREN))
		{
			/* built-in function should be in the form 
		 	 * of the function call */
			awk->errnum = ASE_AWK_ELPAREN;
			return ASE_NULL;
		}

		nde = __parse_fncall (awk, ASE_NULL, 0, bfn);
		return (ase_awk_nde_t*)nde;
	}

	/* now we know that name_dup is a normal identifier. */
	if (MATCH(awk,TOKEN_LBRACK)) 
	{
		ase_awk_nde_t* nde;
		nde = __parse_hashidx (awk, name_dup, name_len);
		if (nde == ASE_NULL) ASE_AWK_FREE (awk, name_dup);
		return (ase_awk_nde_t*)nde;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) 
	{
		/* function call */
		ase_awk_nde_t* nde;
		nde = __parse_fncall (awk, name_dup, name_len, ASE_NULL);
		if (nde == ASE_NULL) ASE_AWK_FREE (awk, name_dup);
		return (ase_awk_nde_t*)nde;
	}	
	else 
	{
		/* normal variable */
		ase_awk_nde_var_t* nde;
		ase_size_t idxa;

		nde = (ase_awk_nde_var_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_nde_var_t));
		if (nde == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, name_dup);
			awk->errnum = ASE_AWK_ENOMEM;
			return ASE_NULL;
		}

		/* search the parameter name list */
		idxa = ase_awk_tab_find (
			&awk->parse.params, 0, name_dup, name_len);
		if (idxa != (ase_size_t)-1) 
		{
			nde->type = ASE_AWK_NDE_ARG;
			nde->next = ASE_NULL;
			/*nde->id.name = ASE_NULL;*/
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = idxa;
			nde->idx = ASE_NULL;

			return (ase_awk_nde_t*)nde;
		}

		/* search the local variable list */
		idxa = ase_awk_tab_rrfind (
			&awk->parse.locals, 0, name_dup, name_len);
		if (idxa != (ase_size_t)-1) 
		{
			nde->type = ASE_AWK_NDE_LOCAL;
			nde->next = ASE_NULL;
			/*nde->id.name = ASE_NULL;*/
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = idxa;
			nde->idx = ASE_NULL;

			return (ase_awk_nde_t*)nde;
		}

		/* search the global variable list */
		idxa = ase_awk_tab_rrfind (
			&awk->parse.globals, 0, name_dup, name_len);
		if (idxa != (ase_size_t)-1) 
		{
			nde->type = ASE_AWK_NDE_GLOBAL;
			nde->next = ASE_NULL;
			/*nde->id.name = ASE_NULL;*/
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = idxa;
			nde->idx = ASE_NULL;

			return (ase_awk_nde_t*)nde;
		}

		if (awk->option & ASE_AWK_IMPLICIT) 
		{
			nde->type = ASE_AWK_NDE_NAMED;
			nde->next = ASE_NULL;
			nde->id.name = name_dup;
			nde->id.name_len = name_len;
			nde->id.idxa = (ase_size_t)-1;
			nde->idx = ASE_NULL;

			return (ase_awk_nde_t*)nde;
		}

		/* undefined variable */
		ASE_AWK_FREE (awk, name_dup);
		ASE_AWK_FREE (awk, nde);

		awk->errnum = ASE_AWK_EUNDEF;
		return ASE_NULL;
	}
}

static ase_awk_nde_t* __parse_hashidx (
	ase_awk_t* awk, ase_char_t* name, ase_size_t name_len)
{
	ase_awk_nde_t* idx, * tmp, * last;
	ase_awk_nde_var_t* nde;
	ase_size_t idxa;

	idx = ASE_NULL;
	last = ASE_NULL;

	do
	{
		if (__get_token(awk) == -1) 
		{
			if (idx != ASE_NULL) ase_awk_clrpt (awk, idx);
			return ASE_NULL;
		}

		tmp = __parse_expression (awk);
		if (tmp == ASE_NULL) 
		{
			if (idx != ASE_NULL) ase_awk_clrpt (awk, idx);
			return ASE_NULL;
		}

		if (idx == ASE_NULL)
		{
			ASE_AWK_ASSERT (awk, last == ASE_NULL);
			idx = tmp; last = tmp;
		}
		else
		{
			last->next = tmp;
			last = tmp;
		}
	}
	while (MATCH(awk,TOKEN_COMMA));

	ASE_AWK_ASSERT (awk, idx != ASE_NULL);

	if (!MATCH(awk,TOKEN_RBRACK)) 
	{
		ase_awk_clrpt (awk, idx);
		PANIC (awk, ASE_AWK_ERBRACK);
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, idx);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_var_t*) 
		ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_var_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, idx);
		awk->errnum = ASE_AWK_ENOMEM;
		return ASE_NULL;
	}

	/* search the parameter name list */
	idxa = ase_awk_tab_find (&awk->parse.params, 0, name, name_len);
	if (idxa != (ase_size_t)-1) 
	{
		nde->type = ASE_AWK_NDE_ARGIDX;
		nde->next = ASE_NULL;
		/*nde->id.name = ASE_NULL; */
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (ase_awk_nde_t*)nde;
	}

	/* search the local variable list */
	idxa = ase_awk_tab_rrfind(&awk->parse.locals, 0, name, name_len);
	if (idxa != (ase_size_t)-1) 
	{
		nde->type = ASE_AWK_NDE_LOCALIDX;
		nde->next = ASE_NULL;
		/*nde->id.name = ASE_NULL; */
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = idxa;
		nde->idx = idx;

		return (ase_awk_nde_t*)nde;
	}

	/* search the global variable list */
	idxa = ase_awk_tab_rrfind(&awk->parse.globals, 0, name, name_len);
	if (idxa != (ase_size_t)-1) 
	{
		nde->type = ASE_AWK_NDE_GLOBALIDX;
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
		nde->type = ASE_AWK_NDE_NAMEDIDX;
		nde->next = ASE_NULL;
		nde->id.name = name;
		nde->id.name_len = name_len;
		nde->id.idxa = (ase_size_t)-1;
		nde->idx = idx;

		return (ase_awk_nde_t*)nde;
	}

	/* undefined variable */
	ase_awk_clrpt (awk, idx);
	ASE_AWK_FREE (awk, nde);

	awk->errnum = ASE_AWK_EUNDEF;
	return ASE_NULL;
}

static ase_awk_nde_t* __parse_fncall (
	ase_awk_t* awk, ase_char_t* name, ase_size_t name_len, ase_awk_bfn_t* bfn)
{
	ase_awk_nde_t* head, * curr, * nde;
	ase_awk_nde_call_t* call;
	ase_size_t nargs;

	if (__get_token(awk) == -1) return ASE_NULL;
	
	head = curr = ASE_NULL;
	nargs = 0;

	if (MATCH(awk,TOKEN_RPAREN)) 
	{
		/* no parameters to the function call */
		if (__get_token(awk) == -1) return ASE_NULL;
	}
	else 
	{
		/* parse function parameters */

		while (1) 
		{
			nde = __parse_expression (awk);
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
				if (__get_token(awk) == -1) 
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
				PANIC (awk, ASE_AWK_ECOMMA);	
			}

			if (__get_token(awk) == -1) 
			{
				if (head != ASE_NULL) ase_awk_clrpt (awk, head);
				return ASE_NULL;
			}
		}

	}

	call = (ase_awk_nde_call_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_call_t));
	if (call == ASE_NULL) 
	{
		if (head != ASE_NULL) ase_awk_clrpt (awk, head);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	if (bfn != ASE_NULL)
	{
		call->type = ASE_AWK_NDE_BFN;
		call->next = ASE_NULL;

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
		call->type = ASE_AWK_NDE_AFN;
		call->next = ASE_NULL;
		call->what.afn.name = name; 
		call->what.afn.name_len = name_len;
		call->args = head;
		call->nargs = nargs;
	}

	return (ase_awk_nde_t*)call;
}

static ase_awk_nde_t* __parse_if (ase_awk_t* awk)
{
	ase_awk_nde_t* test;
	ase_awk_nde_t* then_part;
	ase_awk_nde_t* else_part;
	ase_awk_nde_if_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, ASE_AWK_ELPAREN);
	if (__get_token(awk) == -1) return ASE_NULL;

	test = __parse_expression (awk);
	if (test == ASE_NULL) return ASE_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		ase_awk_clrpt (awk, test);
		PANIC (awk, ASE_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}

	then_part = __parse_statement (awk);
	if (then_part == ASE_NULL) 
	{
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}

	if (MATCH(awk,TOKEN_ELSE)) 
	{
		if (__get_token(awk) == -1) 
		{
			ase_awk_clrpt (awk, then_part);
			ase_awk_clrpt (awk, test);
			return ASE_NULL;
		}

		else_part = __parse_statement (awk);
		if (else_part == ASE_NULL) 
		{
			ase_awk_clrpt (awk, then_part);
			ase_awk_clrpt (awk, test);
			return ASE_NULL;
		}
	}
	else else_part = ASE_NULL;

	nde = (ase_awk_nde_if_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_if_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, else_part);
		ase_awk_clrpt (awk, then_part);
		ase_awk_clrpt (awk, test);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	nde->type = ASE_AWK_NDE_IF;
	nde->next = ASE_NULL;
	nde->test = test;
	nde->then_part = then_part;
	nde->else_part = else_part;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_while (ase_awk_t* awk)
{
	ase_awk_nde_t* test, * body;
	ase_awk_nde_while_t* nde;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, ASE_AWK_ELPAREN);
	if (__get_token(awk) == -1) return ASE_NULL;

	test = __parse_expression (awk);
	if (test == ASE_NULL) return ASE_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		ase_awk_clrpt (awk, test);
		PANIC (awk, ASE_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}

	body = __parse_statement (awk);
	if (body == ASE_NULL) 
	{
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_while_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_while_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, body);
		ase_awk_clrpt (awk, test);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	nde->type = ASE_AWK_NDE_WHILE;
	nde->next = ASE_NULL;
	nde->test = test;
	nde->body = body;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_for (ase_awk_t* awk)
{
	ase_awk_nde_t* init, * test, * incr, * body;
	ase_awk_nde_for_t* nde; 
	ase_awk_nde_foreach_t* nde2;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, ASE_AWK_ELPAREN);
	if (__get_token(awk) == -1) return ASE_NULL;
		
	if (MATCH(awk,TOKEN_SEMICOLON)) init = ASE_NULL;
	else 
	{
		/* this line is very ugly. it checks the entire next 
		 * expression or the first element in the expression
		 * is wrapped by a parenthesis */
		int no_foreach = MATCH(awk,TOKEN_LPAREN);

		init = __parse_expression (awk);
		if (init == ASE_NULL) return ASE_NULL;

		if (!no_foreach && init->type == ASE_AWK_NDE_EXP_BIN &&
		    ((ase_awk_nde_exp_t*)init)->opcode == ASE_AWK_BINOP_IN &&
		    __is_plain_var(((ase_awk_nde_exp_t*)init)->left))
		{	
			/* switch to foreach */
			
			if (!MATCH(awk,TOKEN_RPAREN))
			{
				ase_awk_clrpt (awk, init);
				PANIC (awk, ASE_AWK_ERPAREN);
			}

			if (__get_token(awk) == -1) 
			{
				ase_awk_clrpt (awk, init);
				return ASE_NULL;

			}	
			
			body = __parse_statement (awk);
			if (body == ASE_NULL) 
			{
				ase_awk_clrpt (awk, init);
				return ASE_NULL;
			}

			nde2 = (ase_awk_nde_foreach_t*) ASE_AWK_MALLOC (
				awk, ase_sizeof(ase_awk_nde_foreach_t));
			if (nde2 == ASE_NULL)
			{
				ase_awk_clrpt (awk, init);
				ase_awk_clrpt (awk, body);
				PANIC (awk, ASE_AWK_ENOMEM);
			}

			nde2->type = ASE_AWK_NDE_FOREACH;
			nde2->next = ASE_NULL;
			nde2->test = init;
			nde2->body = body;

			return (ase_awk_nde_t*)nde2;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			ase_awk_clrpt (awk, init);
			PANIC (awk, ASE_AWK_ESEMICOLON);
		}
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, init);
		return ASE_NULL;
	}

	if (MATCH(awk,TOKEN_SEMICOLON)) test = ASE_NULL;
	else 
	{
		test = __parse_expression (awk);
		if (test == ASE_NULL) 
		{
			ase_awk_clrpt (awk, init);
			return ASE_NULL;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) 
		{
			ase_awk_clrpt (awk, init);
			ase_awk_clrpt (awk, test);
			PANIC (awk, ASE_AWK_ESEMICOLON);
		}
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, init);
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}
	
	if (MATCH(awk,TOKEN_RPAREN)) incr = ASE_NULL;
	else 
	{
		incr = __parse_expression (awk);
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
			PANIC (awk, ASE_AWK_ERPAREN);
		}
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, init);
		ase_awk_clrpt (awk, test);
		ase_awk_clrpt (awk, incr);
		return ASE_NULL;
	}

	body = __parse_statement (awk);
	if (body == ASE_NULL) 
	{
		ase_awk_clrpt (awk, init);
		ase_awk_clrpt (awk, test);
		ase_awk_clrpt (awk, incr);
		return ASE_NULL;
	}

	nde = (ase_awk_nde_for_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_for_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, init);
		ase_awk_clrpt (awk, test);
		ase_awk_clrpt (awk, incr);
		ase_awk_clrpt (awk, body);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	nde->type = ASE_AWK_NDE_FOR;
	nde->next = ASE_NULL;
	nde->init = init;
	nde->test = test;
	nde->incr = incr;
	nde->body = body;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_dowhile (ase_awk_t* awk)
{
	ase_awk_nde_t* test, * body;
	ase_awk_nde_while_t* nde;

	body = __parse_statement (awk);
	if (body == ASE_NULL) return ASE_NULL;

	if (!MATCH(awk,TOKEN_WHILE)) 
	{
		ase_awk_clrpt (awk, body);
		PANIC (awk, ASE_AWK_EWHILE);
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, body);
		return ASE_NULL;
	}

	if (!MATCH(awk,TOKEN_LPAREN)) 
	{
		ase_awk_clrpt (awk, body);
		PANIC (awk, ASE_AWK_ELPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, body);
		return ASE_NULL;
	}

	test = __parse_expression (awk);
	if (test == ASE_NULL) 
	{
		ase_awk_clrpt (awk, body);
		return ASE_NULL;
	}

	if (!MATCH(awk,TOKEN_RPAREN)) 
	{
		ase_awk_clrpt (awk, body);
		ase_awk_clrpt (awk, test);
		PANIC (awk, ASE_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) 
	{
		ase_awk_clrpt (awk, body);
		ase_awk_clrpt (awk, test);
		return ASE_NULL;
	}
	
	nde = (ase_awk_nde_while_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_while_t));
	if (nde == ASE_NULL) 
	{
		ase_awk_clrpt (awk, body);
		ase_awk_clrpt (awk, test);
		PANIC (awk, ASE_AWK_ENOMEM);
	}

	nde->type = ASE_AWK_NDE_DOWHILE;
	nde->next = ASE_NULL;
	nde->test = test;
	nde->body = body;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_break (ase_awk_t* awk)
{
	ase_awk_nde_break_t* nde;

	if (awk->parse.depth.cur.loop <= 0) PANIC (awk, ASE_AWK_EBREAK);

	nde = (ase_awk_nde_break_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_break_t));
	if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);
	nde->type = ASE_AWK_NDE_BREAK;
	nde->next = ASE_NULL;
	
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_continue (ase_awk_t* awk)
{
	ase_awk_nde_continue_t* nde;

	if (awk->parse.depth.cur.loop <= 0) PANIC (awk, ASE_AWK_ECONTINUE);

	nde = (ase_awk_nde_continue_t*) ASE_AWK_MALLOC (
		awk, ase_sizeof(ase_awk_nde_continue_t));
	if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);
	nde->type = ASE_AWK_NDE_CONTINUE;
	nde->next = ASE_NULL;
	
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_return (ase_awk_t* awk)
{
	ase_awk_nde_return_t* nde;
	ase_awk_nde_t* val;

	nde = (ase_awk_nde_return_t*) ASE_AWK_MALLOC (
		awk, ase_sizeof(ase_awk_nde_return_t));
	if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);
	nde->type = ASE_AWK_NDE_RETURN;
	nde->next = ASE_NULL;

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* no return value */
		val = ASE_NULL;
	}
	else 
	{
		val = __parse_expression (awk);
		if (val == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;
		}
	}

	nde->val = val;
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_exit (ase_awk_t* awk)
{
	ase_awk_nde_exit_t* nde;
	ase_awk_nde_t* val;

	nde = (ase_awk_nde_exit_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_exit_t));
	if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);
	nde->type = ASE_AWK_NDE_EXIT;
	nde->next = ASE_NULL;

	if (MATCH(awk,TOKEN_SEMICOLON)) 
	{
		/* no exit code */
		val = ASE_NULL;
	}
	else 
	{
		val = __parse_expression (awk);
		if (val == ASE_NULL) 
		{
			ASE_AWK_FREE (awk, nde);
			return ASE_NULL;
		}
	}

	nde->val = val;
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_delete (ase_awk_t* awk)
{
	ase_awk_nde_delete_t* nde;
	ase_awk_nde_t* var;

	if (!MATCH(awk,TOKEN_IDENT)) PANIC (awk, ASE_AWK_EIDENT);

	var = __parse_primary_ident (awk);
	if (var == ASE_NULL) return ASE_NULL;

	if (!__is_var (var))
	{
		/* a normal identifier is expected */
		ase_awk_clrpt (awk, var);
		PANIC (awk, ASE_AWK_EIDENT);
	}

	nde = (ase_awk_nde_delete_t*) ASE_AWK_MALLOC (
		awk, ase_sizeof(ase_awk_nde_delete_t));
	if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);

	nde->type = ASE_AWK_NDE_DELETE;
	nde->next = ASE_NULL;
	nde->var = var;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_print (ase_awk_t* awk, int type)
{
	ase_awk_nde_print_t* nde;
	ase_awk_nde_t* args = ASE_NULL; 
	ase_awk_nde_t* out = ASE_NULL;
	int out_type;

	if (!MATCH(awk,TOKEN_SEMICOLON) &&
	    !MATCH(awk,TOKEN_GT) &&
	    !MATCH(awk,TOKEN_RSHIFT) &&
	    !MATCH(awk,TOKEN_BOR) &&
	    !MATCH(awk,TOKEN_BORAND)) 
	{
		ase_awk_nde_t* args_tail;
		ase_awk_nde_t* tail_prev;

		args = __parse_expression (awk);
		if (args == ASE_NULL) return ASE_NULL;

		args_tail = args;
		tail_prev = ASE_NULL;

		if (args->type != ASE_AWK_NDE_GRP)
		{
			/* args->type == ASE_AWK_NDE_GRP when print (a, b, c) 
			 * args->type != ASE_AWK_NDE_GRP when print a, b, c */
			
			while (MATCH(awk,TOKEN_COMMA))
			{
				if (__get_token(awk) == -1)
				{
					ase_awk_clrpt (awk, args);
					return ASE_NULL;
				}

				args_tail->next = __parse_expression (awk);
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
		 * print (1 > 2) would print (1 > 2) in the console */
		if (awk->token.prev != TOKEN_RPAREN &&
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
			if (__get_token(awk) == -1)
			{
				if (args != ASE_NULL) ase_awk_clrpt (awk, args);
				return ASE_NULL;
			}

			out = __parse_expression(awk);
			if (out == ASE_NULL)
			{
				if (args != ASE_NULL) ase_awk_clrpt (awk, args);
				return ASE_NULL;
			}
		}
	}

	nde = (ase_awk_nde_print_t*) 
		ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_print_t));
	if (nde == ASE_NULL) 
	{
		if (args != ASE_NULL) ase_awk_clrpt (awk, args);
		if (out != ASE_NULL) ase_awk_clrpt (awk, out);

		awk->errnum = ASE_AWK_ENOMEM;
		return ASE_NULL;
	}

	ASE_AWK_ASSERTX (awk, 
		type == ASE_AWK_NDE_PRINT || type == ASE_AWK_NDE_PRINTF, 
		"the node type should be either ASE_AWK_NDE_PRINT or ASE_AWK_NDE_PRINTF");

	if (type == ASE_AWK_NDE_PRINTF && args == ASE_NULL)
	{
		if (out != ASE_NULL) ase_awk_clrpt (awk, out);
		awk->errnum = ASE_AWK_EPRINTFARG;
		return ASE_NULL;
	}

	nde->type = type;
	nde->next = ASE_NULL;
	nde->args = args;
	nde->out_type = out_type;
	nde->out = out;

	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_next (ase_awk_t* awk)
{
	ase_awk_nde_next_t* nde;

	if (awk->parse.id.block == PARSE_BEGIN_BLOCK ||
	    awk->parse.id.block == PARSE_END_BLOCK)
	{
		PANIC (awk, ASE_AWK_ENEXT);
	}

	nde = (ase_awk_nde_next_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_nde_next_t));
	if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);
	nde->type = ASE_AWK_NDE_NEXT;
	nde->next = ASE_NULL;
	
	return (ase_awk_nde_t*)nde;
}

static ase_awk_nde_t* __parse_nextfile (ase_awk_t* awk, int out)
{
	ase_awk_nde_nextfile_t* nde;

	if (awk->parse.id.block == PARSE_BEGIN_BLOCK ||
	    awk->parse.id.block == PARSE_END_BLOCK)
	{
		PANIC (awk, ASE_AWK_ENEXTFILE);
	}

	nde = (ase_awk_nde_nextfile_t*) ASE_AWK_MALLOC (
		awk, ase_sizeof(ase_awk_nde_nextfile_t));
	if (nde == ASE_NULL) PANIC (awk, ASE_AWK_ENOMEM);
	nde->type = ASE_AWK_NDE_NEXTFILE;
	nde->next = ASE_NULL;
	nde->out = out;
	
	return (ase_awk_nde_t*)nde;
}

static int __get_token (ase_awk_t* awk)
{
	ase_cint_t c;
	ase_size_t line;
	int n;

	line = awk->token.line;
	do 
	{
		if (__skip_spaces(awk) == -1) return -1;
		if ((n = __skip_comment(awk)) == -1) return -1;
	} 
	while (n == 1);

	ase_awk_str_clear (&awk->token.name);
	awk->token.line = awk->src.lex.line;
	awk->token.column = awk->src.lex.column;

	if (line != 0 && (awk->option & ASE_AWK_BLOCKLESS) &&
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

	if (c == ASE_CHAR_EOF) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_EOF);
	}	
	else if (ASE_AWK_ISDIGIT (awk, c)/*|| c == ASE_T('.')*/)
	{
		if (__get_number (awk) == -1) return -1;
	}
	else if (c == ASE_T('.'))
	{
		if (__get_char (awk) == -1) return -1;
		c = awk->src.lex.curc;

		if (ASE_AWK_ISDIGIT (awk, c))
		{
			if (__unget_char (awk, c) == -1) return -1;
			if (__get_number (awk) == -1) return -1;
		}
		else
		{
			awk->errnum = ASE_AWK_ELXCHR;
			return -1;
		}
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

		type = __classify_ident (awk, 
			ASE_AWK_STR_BUF(&awk->token.name), 
			ASE_AWK_STR_LEN(&awk->token.name));
		SET_TOKEN_TYPE (awk, type);
	}
	else if (c == ASE_T('\"')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_STR);

		if (__get_charstr(awk) == -1) return -1;

		while (awk->option & ASE_AWK_STRCONCAT) 
		{
			do 
			{
				if (__skip_spaces(awk) == -1) return -1;
				if ((n = __skip_comment(awk)) == -1) return -1;
			} 
			while (n == 1);

			c = awk->src.lex.curc;
			if (c != ASE_T('\"')) break;

			if (__get_charstr(awk) == -1) return -1;
		}

	}
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
			SET_TOKEN_TYPE (awk, TOKEN_NOT);
		}
	}
	else if (c == ASE_T('>')) 
	{
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
		if ((awk->option & ASE_AWK_SHIFT) && c == ASE_T('>')) 
		{
			SET_TOKEN_TYPE (awk, TOKEN_RSHIFT);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
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
			SET_TOKEN_TYPE (awk, TOKEN_LSHIFT);
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR (awk);
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
		else if (c == ASE_T('&'))
		{
			SET_TOKEN_TYPE (awk, TOKEN_BORAND);
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
		else
		{
			SET_TOKEN_TYPE (awk, TOKEN_BAND);
		}
	}
	else if (c == ASE_T('~'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_TILDE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T('^'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_BXOR);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
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
	else if (c == ASE_T('(')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_LPAREN);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T(')')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_RPAREN);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T('{')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_LBRACE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T('}')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_RBRACE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T('[')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_LBRACK);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T(']')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_RBRACK);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T('$')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_DOLLAR);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T(',')) 
	{
		SET_TOKEN_TYPE (awk, TOKEN_COMMA);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T('.'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_PERIOD);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T(';') || 
	         (c == ASE_T('\n') && (awk->option & ASE_AWK_NEWLINE))) 
	{
	/* TODO: more check on the newline terminator... */
		SET_TOKEN_TYPE (awk, TOKEN_SEMICOLON);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T(':'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_COLON);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else if (c == ASE_T('?'))
	{
		SET_TOKEN_TYPE (awk, TOKEN_QUEST);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR (awk);
	}
	else 
	{
		awk->errnum = ASE_AWK_ELXCHR;
		return -1;
	}

	return 0;
}

static int __get_number (ase_awk_t* awk)
{
	ase_cint_t c;

	ASE_AWK_ASSERT (awk, ASE_AWK_STR_LEN(&awk->token.name) == 0);
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

static int __get_charstr (ase_awk_t* awk)
{
	if (awk->src.lex.curc != ASE_T('\"')) 
	{
		/* the starting quote has been consumed before this function
		 * has been called */
		ADD_TOKEN_CHAR (awk, awk->src.lex.curc);
	}
	return __get_string (awk, ASE_T('\"'), ASE_T('\\'), ase_false);
}

static int __get_rexstr (ase_awk_t* awk)
{
	if (awk->src.lex.curc == ASE_T('/')) 
	{
		/* this part of the function is different from __get_charstr
		 * because of the way this function is called */
		GET_CHAR (awk);
		return 0;
	}
	else 
	{
		ADD_TOKEN_CHAR (awk, awk->src.lex.curc);
		return __get_string (awk, ASE_T('/'), ASE_T('\\'), ase_true);
	}
}

static int __get_string (
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
			awk->errnum = ASE_AWK_EENDSTR;
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
			else if (c == ASE_T('u') && ase_sizeof(ase_char_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == ASE_T('U') && ase_sizeof(ase_char_t) >= 4) 
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

static int __get_char (ase_awk_t* awk)
{
	ase_ssize_t n;
	/*ase_char_t c;*/

	if (awk->src.lex.ungotc_count > 0) 
	{
		awk->src.lex.curc = awk->src.lex.ungotc[--awk->src.lex.ungotc_count];
		return 0;
	}

	if (awk->src.shared.buf_pos >= awk->src.shared.buf_len)
	{
		n = awk->src.ios.in (
			ASE_AWK_IO_READ, awk->src.ios.custom_data,
			awk->src.shared.buf, ase_countof(awk->src.shared.buf));
		if (n == -1)
		{
			awk->errnum = ASE_AWK_ESRCINREAD;
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

static int __unget_char (ase_awk_t* awk, ase_cint_t c)
{
	if (awk->src.lex.ungotc_count >= ase_countof(awk->src.lex.ungotc)) 
	{
		awk->errnum = ASE_AWK_ELXUNG;
		return -1;
	}

	awk->src.lex.ungotc[awk->src.lex.ungotc_count++] = c;
	return 0;
}

static int __skip_spaces (ase_awk_t* awk)
{
	ase_cint_t c = awk->src.lex.curc;

	if (awk->option & ASE_AWK_NEWLINE && awk->parse.nl_semicolon)
	{
		while (c != ASE_T('\n') &&
		       ASE_AWK_ISSPACE (awk, c)) GET_CHAR_TO (awk, c);
	}
	else
	{
		while (ASE_AWK_ISSPACE (awk, c)) GET_CHAR_TO (awk, c);
	}
	return 0;
}

static int __skip_comment (ase_awk_t* awk)
{
	ase_cint_t c = awk->src.lex.curc;

	if ((awk->option & ASE_AWK_HASHSIGN) && c == ASE_T('#'))
	{
		do 
		{ 
			GET_CHAR_TO (awk, c);
		} 
		while (c != ASE_T('\n') && c != ASE_CHAR_EOF);

		GET_CHAR (awk);
		return 1; /* comment by # */
	}

	if (c != ASE_T('/')) return 0; /* not a comment */
	GET_CHAR_TO (awk, c);

	if ((awk->option & ASE_AWK_DBLSLASHES) && c == ASE_T('/')) 
	{
		do 
		{ 
			GET_CHAR_TO (awk, c);
		} 
		while (c != ASE_T('\n') && c != ASE_CHAR_EOF);

		GET_CHAR (awk);
		return 1; /* comment by // */
	}
	else if (c == ASE_T('*')) 
	{
		do 
		{
			GET_CHAR_TO (awk, c);
			if (c == ASE_CHAR_EOF)
			{
				awk->errnum = ASE_AWK_EENDCOMMENT;
				return -1;
			}

			if (c == ASE_T('*')) 
			{
				GET_CHAR_TO (awk, c);
				if (c == ASE_CHAR_EOF)
				{
					awk->errnum = ASE_AWK_EENDCOMMENT;
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

	if (__unget_char (awk, c) == -1) return -1; /* error */
	awk->src.lex.curc = ASE_T('/');

	return 0;
}

static int __classify_ident (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len)
{
	struct __kwent* kwp;

	for (kwp = __kwtab; kwp->name != ASE_NULL; kwp++) 
	{
		if (kwp->valid != 0 && 
		    (awk->option & kwp->valid) == 0) continue;

		if (ase_awk_strxncmp (kwp->name, kwp->name_len, name, len) == 0) 
		{
			return kwp->type;
		}
	}

	return TOKEN_IDENT;
}

static int __assign_to_opcode (ase_awk_t* awk)
{
	static int __assop[] =
	{
		ASE_AWK_ASSOP_NONE,
		ASE_AWK_ASSOP_PLUS,
		ASE_AWK_ASSOP_MINUS,
		ASE_AWK_ASSOP_MUL,
		ASE_AWK_ASSOP_DIV,
		ASE_AWK_ASSOP_MOD,
		ASE_AWK_ASSOP_EXP
	};

	if (awk->token.type >= TOKEN_ASSIGN &&
	    awk->token.type <= TOKEN_EXP_ASSIGN)
	{
		return __assop[awk->token.type - TOKEN_ASSIGN];
	}

	return -1;
}

static int __is_plain_var (ase_awk_nde_t* nde)
{
	return nde->type == ASE_AWK_NDE_GLOBAL ||
	       nde->type == ASE_AWK_NDE_LOCAL ||
	       nde->type == ASE_AWK_NDE_ARG ||
	       nde->type == ASE_AWK_NDE_NAMED;
}

static int __is_var (ase_awk_nde_t* nde)
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

struct __deparse_func_t 
{
	ase_awk_t* awk;
	ase_char_t* tmp;
	ase_size_t tmp_len;
};

static int __deparse (ase_awk_t* awk)
{
	ase_awk_chain_t* chain;
	ase_char_t tmp[ase_sizeof(ase_size_t)*8 + 32];
	struct __deparse_func_t df;
	int n = 0, op;

	ASE_AWK_ASSERT (awk, awk->src.ios.out != ASE_NULL);

	awk->src.shared.buf_len = 0;
	awk->src.shared.buf_pos = 0;

	op = awk->src.ios.out (
		ASE_AWK_IO_OPEN, awk->src.ios.custom_data, ASE_NULL, 0);
	if (op == -1)
	{
		awk->errnum = ASE_AWK_ESRCOUTOPEN;
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
		ase_size_t i, len;

		ASE_AWK_ASSERT (awk, awk->tree.nglobals > 0);
		if (ase_awk_putsrcstr (awk, ASE_T("global ")) == -1)
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);

		for (i = awk->tree.nbglobals; i < awk->tree.nglobals - 1; i++) 
		{
			len = ase_awk_longtostr ((ase_long_t)i, 
				10, ASE_T("__global"), tmp, ase_countof(tmp));
			ASE_AWK_ASSERT (awk, len != (ase_size_t)-1);
			if (ase_awk_putsrcstrx (awk, tmp, len) == -1)
				EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
			/*
			if (ase_awk_putsrcstrx (awk, 
				awk->parse.globals.buf[i].name, 
				awk->parse.globals.buf[i].name_len) == -1)
			{
				EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
			}
			*/

			if (ase_awk_putsrcstr (awk, ASE_T(", ")) == -1)
				EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
		}

		len = ase_awk_longtostr ((ase_long_t)i, 
			10, ASE_T("__global"), tmp, ase_countof(tmp));
		ASE_AWK_ASSERT (awk, len != (ase_size_t)-1);
		if (ase_awk_putsrcstrx (awk, tmp, len) == -1)
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
		/*
		if (ase_awk_putsrcstrx (awk, 
			awk->parse.globals.buf[i].name, 
			awk->parse.globals.buf[i].name_len) == -1)
		{
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
		}
		*/

		if (ase_awk_putsrcstr (awk, ASE_T(";\n\n")) == -1)
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
	}

	df.awk = awk;
	df.tmp = tmp;
	df.tmp_len = ase_countof(tmp);

	if (ase_awk_map_walk (&awk->tree.afns, __deparse_func, &df) == -1) 
	{
		EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
	}

	if (awk->tree.begin != ASE_NULL) 
	{
		if (ase_awk_putsrcstr (awk, ASE_T("BEGIN ")) == -1)
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);

		if (ase_awk_prnpt (awk, awk->tree.begin) == -1)
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);

		if (__put_char (awk, ASE_T('\n')) == -1)
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
	}

	chain = awk->tree.chain;
	while (chain != ASE_NULL) 
	{
		if (chain->pattern != ASE_NULL) 
		{
			if (ase_awk_prnptnpt (awk, chain->pattern) == -1)
				EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
		}

		if (chain->action == ASE_NULL) 
		{
			/* blockless pattern */
			if (__put_char (awk, ASE_T('\n')) == -1)
				EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
		}
		else 
		{
			if (chain->pattern != ASE_NULL)
			{
				if (__put_char (awk, ASE_T(' ')) == -1)
					EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
			}
			if (ase_awk_prnpt (awk, chain->action) == -1)
				EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
		}

		if (__put_char (awk, ASE_T('\n')) == -1)
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);

		chain = chain->next;	
	}

	if (awk->tree.end != ASE_NULL) 
	{
		if (ase_awk_putsrcstr (awk, ASE_T("END ")) == -1)
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
		if (ase_awk_prnpt (awk, awk->tree.end) == -1)
			EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);
	}

	if (__flush (awk) == -1) EXIT_DEPARSE (ASE_AWK_ESRCOUTWRITE);

exit_deparse:
	if (awk->src.ios.out (
		ASE_AWK_IO_CLOSE, awk->src.ios.custom_data, ASE_NULL, 0) == -1)
	{
		if (n != -1)
		{
			awk->errnum = ASE_AWK_ESRCOUTCLOSE;
			n = -1;
		}
	}

	return n;
}

static int __deparse_func (ase_awk_pair_t* pair, void* arg)
{
	struct __deparse_func_t* df = (struct __deparse_func_t*)arg;
	ase_awk_afn_t* afn = (ase_awk_afn_t*)pair->val;
	ase_size_t i, n;

	ASE_AWK_ASSERT (df->awk, ase_awk_strxncmp (
		pair->key, pair->key_len, afn->name, afn->name_len) == 0);

	if (ase_awk_putsrcstr (df->awk, ASE_T("function ")) == -1) return -1;
	if (ase_awk_putsrcstr (df->awk, afn->name) == -1) return -1;
	if (ase_awk_putsrcstr (df->awk, ASE_T(" (")) == -1) return -1;

	for (i = 0; i < afn->nargs; ) 
	{
		n = ase_awk_longtostr (i++, 10, 
			ASE_T("__param"), df->tmp, df->tmp_len);
		ASE_AWK_ASSERT (df->awk, n != (ase_size_t)-1);
		if (ase_awk_putsrcstrx (df->awk, df->tmp, n) == -1) return -1;
		if (i >= afn->nargs) break;
		if (ase_awk_putsrcstr (df->awk, ASE_T(", ")) == -1) return -1;
	}

	if (ase_awk_putsrcstr (df->awk, ASE_T(")\n")) == -1) return -1;

	if (ase_awk_prnpt (df->awk, afn->body) == -1) return -1;
	if (ase_awk_putsrcstr (df->awk, ASE_T("\n")) == -1) return -1;

	return 0;
}

static int __put_char (ase_awk_t* awk, ase_char_t c)
{
	awk->src.shared.buf[awk->src.shared.buf_len++] = c;
	if (awk->src.shared.buf_len >= ase_countof(awk->src.shared.buf))
	{
		if (__flush (awk) == -1) return -1;
	}
	return 0;
}

static int __flush (ase_awk_t* awk)
{
	ase_ssize_t n;

	ASE_AWK_ASSERT (awk, awk->src.ios.out != ASE_NULL);

	while (awk->src.shared.buf_pos < awk->src.shared.buf_len)
	{
		n = awk->src.ios.out (
			ASE_AWK_IO_WRITE, awk->src.ios.custom_data,
			&awk->src.shared.buf[awk->src.shared.buf_pos], 
			awk->src.shared.buf_len - awk->src.shared.buf_pos);
		if (n <= 0) return -1;

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
		if (__put_char (awk, *str) == -1) return -1;
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
		if (__put_char (awk, *str) == -1) return -1;
		str++;
	}

	return 0;
}

