/*
 * $Id: parse.c,v 1.10 2006-01-09 12:51:47 bacon Exp $
 */

#include <xp/awk/awk.h>
#include <xp/awk/tree.h>
#include <xp/awk/tree.h>
#include <xp/bas/memory.h>
#include <xp/bas/ctype.h>
#include <xp/bas/string.h>

enum
{
	TOKEN_EOF,

	TOKEN_ASSIGN,
	TOKEN_EQ,
	TOKEN_NE,
	TOKEN_NOT,
	TOKEN_PLUS,
	TOKEN_PLUS_PLUS,
	TOKEN_PLUS_ASSIGN,
	TOKEN_MINUS,
	TOKEN_MINUS_MINUS,
	TOKEN_MINUS_ASSIGN,

	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,

	TOKEN_STRING,
	TOKEN_REGEX,

	TOKEN_IDENT,
	TOKEN_BEGIN,
	TOKEN_END,
	TOKEN_FUNCTION,
	TOKEN_IF,
	TOKEN_WHILE,
	TOKEN_FOR,
	TOKEN_DO,
	TOKEN_BREAK,
	TOKEN_CONTINUE,
	TOKEN_RETURN,
	TOKEN_EXIT,
	TOKEN_DELETE,
	TOKEN_NEXT,
	TOKEN_NEXTFILE
};

static int __parse (xp_awk_t* awk);

static xp_awk_node_t* __parse_program (xp_awk_t* awk);
static xp_awk_node_t* __parse_block (xp_awk_t* awk);
static xp_awk_node_t* __parse_if (xp_awk_t* awk);
static xp_awk_node_t* __parse_while (xp_awk_t* awk);
static xp_awk_node_t* __parse_for (xp_awk_t* awk);
static xp_awk_node_t* __parse_do (xp_awk_t* awk);
static xp_awk_node_t* __parse_break (xp_awk_t* awk);
static xp_awk_node_t* __parse_continue (xp_awk_t* awk);

static int __get_token (xp_awk_t* awk);
static int __get_char (xp_awk_t* awk);
static int __unget_char (xp_awk_t* awk, xp_cint_t c);
static int __skip_spaces (xp_awk_t* awk);
static int __skip_comment (xp_awk_t* awk);
static int __classfy_ident (const xp_char_t* ident);

struct __kwent 
{ 
	const xp_char_t* name; 
	int type; 
};

static struct __kwent __kwtab[] = 
{
	{ XP_TEXT("BEGIN"),    TOKEN_BEGIN },
	{ XP_TEXT("END"),      TOKEN_END },
	{ XP_TEXT("function"), TOKEN_FUNCTION },
	{ XP_TEXT("if"),       TOKEN_IF },
	{ XP_TEXT("while"),    TOKEN_WHILE },
	{ XP_TEXT("for"),      TOKEN_FOR },
	{ XP_TEXT("do"),       TOKEN_DO },
	{ XP_TEXT("break"),    TOKEN_BREAK },
	{ XP_TEXT("continue"), TOKEN_CONTINUE },
	{ XP_TEXT("return"),   TOKEN_RETURN },
	{ XP_TEXT("exit"),     TOKEN_EXIT },
	{ XP_TEXT("delete"),   TOKEN_DELETE },
	{ XP_TEXT("next"),     TOKEN_NEXT },
	{ XP_TEXT("nextfile"), TOKEN_NEXTFILE },
	{ XP_NULL,             0 },
};

#define GET_CHAR(awk) \
	do { if (__get_char(awk) == -1) return -1; } while(0)

#define GET_CHAR_TO(awk,c) do { \
	if (__get_char(awk) == -1) return -1; \
	c = (awk)->lex.curc; \
} while(0)

#define SET_TOKEN_TYPE(awk,code) ((awk)->token.type = code)

#define ADD_TOKEN_CHAR(awk,c) do { \
	if (xp_str_ccat(&(awk)->token.name,(c)) == -1) { \
		(awk)->errnum = XP_AWK_ENOMEM; return -1; \
	} \
} while (0)

#define ADD_TOKEN_STR(awk,str) do { \
	if (xp_str_cat(&(awk)->token.name,(str)) == -1) { \
		(awk)->errnum = XP_AWK_ENOMEM; return -1; \
	} \
} while (0)

#define GET_TOKEN(awk) \
	do { if (__get_token(awk) == -1) return -1; } while(0)

#define MATCH(awk,token_type) ((awk)->token.type == (token_type))
#define CONSUME(awk) \
	do { if (__get_token(awk) == -1) return XP_NULL; } while(0)

#define PANIC(awk,code) do { (awk)->errnum = (code);  return XP_NULL; } while (0);

int xp_awk_parse (xp_awk_t* awk)
{
	xp_awk_node_t* node;

	GET_CHAR (awk);
	GET_TOKEN (awk);

	node = __parse_program(awk);
	if (node == XP_NULL) return -1;

	awk->tree = node;
	return 0;
}

static xp_awk_node_t* __parse_program (xp_awk_t* awk)
{
	/*
	pattern { action }
	function name (parameter-list) { statement }
	*/

	xp_awk_node_t* node;

	/*
	while (1) {
		if (awk->token.type == TOKEN_FUNCTION) {
			if (__parse_function_declaration(awk) == -1) return -1;
		}
		else {
			if (__parse_pattern_action(awk) == -1) return -1;
		}
	}
	*/

	return node;
}


static xp_awk_node_t* __parse_function_declaration (xp_awk_t* awk)
{
	return XP_NULL;
}

static xp_awk_node_t* __parse_pattern_action (xp_awk_t* awk)
{
	/* 
	BEGIN
	END
	expressions 
	/regular expression/
	pattern && pattern
	pattern || pattern
	!pattern
	(pattern)
	pattern, pattern
	*/

	xp_awk_node_t* node;

	if (MATCH(awk,TOKEN_BEGIN)) {
		CONSUME (awk);
	}
	else if (MATCH(awk,TOKEN_END)) {
		CONSUME (awk);
	}
	/* patterns ...
	 * etc */

	if (!MATCH(awk,TOKEN_LBRACE))  {
		PANIC (awk, XP_AWK_ELBRACE);
	}

	CONSUME (awk);
	node = __parse_block(awk);
	if (node == XP_NULL) return XP_NULL;

	return node;
}


/* TODO: what is the best name for the parsing routine for the outermost block? */
static xp_awk_node_t* __parse_block (xp_awk_t* awk) 
{
	xp_awk_node_block_t* node;

	node = (xp_awk_node_block_t*) xp_malloc (xp_sizeof(xp_awk_node_block_t));
	if (node == XP_NULL) {
		/* TODO: do some clean-up */
		PANIC (awk, XP_AWK_ENOMEM);
	}

	node->type = XP_AWK_NODE_BLOCK;
	node->sbls = XP_NULL;
	node->body = XP_NULL;

	while (1) {
		if (MATCH(awk,TOKEN_RBRACE)) {
			/* TODO: should finalize a block */
			CONSUME (awk);
			break;
		}

		if (MATCH(awk,TOKEN_LBRACE)) {
			/* nested block */
			CONSUME (awk);
			if (__parse_block(awk) == XP_NULL) return XP_NULL;
		}
		else if (MATCH(awk,TOKEN_IF)) {
			CONSUME (awk);
			if (__parse_if(awk) == XP_NULL) return XP_NULL;
		}
		else if (MATCH(awk,TOKEN_WHILE)) {
			CONSUME (awk);
			if (__parse_while(awk) == XP_NULL) return XP_NULL;
		}
		else if (MATCH(awk,TOKEN_FOR)) {
			CONSUME (awk);
			if (__parse_for(awk) == XP_NULL) return XP_NULL;
		}
		else if (MATCH(awk,TOKEN_DO)) {
			CONSUME (awk);
			if (__parse_do(awk) == XP_NULL) return XP_NULL;
		}
		else if (MATCH(awk,TOKEN_BREAK)) {
			CONSUME (awk);
			node->body = __parse_break(awk);
		}
		else if (MATCH(awk,TOKEN_CONTINUE)) {
			CONSUME (awk);
			node->body = __parse_continue(awk);
		}
		else if (MATCH(awk,TOKEN_RETURN)) {
			CONSUME (awk);
			/* TOOD: */
		}
		else if (MATCH(awk, TOKEN_EXIT)) {
			CONSUME (awk);
			/* TOOD: */
		}
		else if (MATCH(awk, TOKEN_DELETE)) {
			CONSUME (awk);
			/* TOOD: */
		}
		else if (MATCH(awk, TOKEN_NEXT)) {
			CONSUME (awk);
			/* TOOD: */
		}
		else if (MATCH(awk, TOKEN_NEXTFILE)) {
			CONSUME (awk);
			/* TOOD: */
		}
	}

	return 0;
}

static xp_awk_node_t* __parse_if (xp_awk_t* awk)
{
	return XP_NULL;
}

static xp_awk_node_t* __parse_while (xp_awk_t* awk)
{
	return XP_NULL;
}

static xp_awk_node_t* __parse_for (xp_awk_t* awk)
{
	return XP_NULL;
}

static xp_awk_node_t* __parse_do (xp_awk_t* awk)
{
	return XP_NULL;
}

static xp_awk_node_t* __parse_break (xp_awk_t* awk)
{
	xp_awk_node_t* node;

	node = (xp_awk_node_t*) xp_malloc (xp_sizeof(xp_awk_node_t));
	if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	node->type = XP_AWK_NODE_BREAK;
	
	/* TODO: do i have to consume a semicolon here???? */
	return node;
}

static xp_awk_node_t* __parse_continue (xp_awk_t* awk)
{
	xp_awk_node_t* node;

	node = (xp_awk_node_t*) xp_malloc (xp_sizeof(xp_awk_node_t));
	if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	node->type = XP_AWK_NODE_CONTINUE;
	
	/* TODO: do i have to consume a semicolon here???? */
	return node;
}

static int __get_token (xp_awk_t* awk)
{
	xp_cint_t c;
	int n;

	do {
		if (__skip_spaces(awk) == -1) return -1;
		if ((n = __skip_comment(awk)) == -1) return -1;
	} while (n == 1);

	xp_str_clear (&awk->token.name);
	c = awk->lex.curc;

	if (c == XP_CHAR_EOF) {
		SET_TOKEN_TYPE (awk, TOKEN_EOF);
	}	
	else if (xp_isdigit(c)) {
		/* number */
	}
	else if (xp_isalpha(c) || c == XP_CHAR('_')) {
		/* identifier */
		do {
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		} while (xp_isalpha(c) || c == XP_CHAR('_') || xp_isdigit(c));

		SET_TOKEN_TYPE (awk, __classfy_ident(XP_STR_BUF(&awk->token.name)));
	}
	else if (c == XP_CHAR('\"')) {
		/* string */
	}
	else if (c == XP_CHAR('/')) {
		/* regular expression */
	}
	else if (c == XP_CHAR('=')) {
		GET_CHAR_TO (awk, c);
		if (c == XP_CHAR('=')) {
			SET_TOKEN_TYPE (awk, TOKEN_EQ);
			ADD_TOKEN_STR (awk, XP_TEXT("=="));
			GET_CHAR_TO (awk, c);
		}
		else {
			SET_TOKEN_TYPE (awk, TOKEN_ASSIGN);
			ADD_TOKEN_STR (awk, XP_TEXT("="));
		}
	}
	else if (c == XP_CHAR('!')) {
		GET_CHAR_TO (awk, c);
		if (c == XP_CHAR('=')) {
			SET_TOKEN_TYPE (awk, TOKEN_NE);
			ADD_TOKEN_STR (awk, XP_TEXT("!="));
			GET_CHAR_TO (awk, c);
		}
		else {
			SET_TOKEN_TYPE (awk, TOKEN_NOT);
			ADD_TOKEN_STR (awk, XP_TEXT("!"));
		}
	}
	else if (c == XP_CHAR('+')) {
		GET_CHAR_TO (awk, c);
		if (c == XP_CHAR('+')) {
			SET_TOKEN_TYPE (awk, TOKEN_PLUS_PLUS);
			ADD_TOKEN_STR (awk, XP_TEXT("++"));
			GET_CHAR_TO (awk, c);
		}
		else if (c == XP_CHAR('=')) {
			SET_TOKEN_TYPE (awk, TOKEN_PLUS_ASSIGN);
			ADD_TOKEN_STR (awk, XP_TEXT("+="));
			GET_CHAR_TO (awk, c);
		}
		else if (xp_isdigit(c)) {
		//	read_number (XP_CHAR('+'));
		}
		else {
			SET_TOKEN_TYPE (awk, TOKEN_PLUS);
			ADD_TOKEN_STR (awk, XP_TEXT("+"));
		}
	}
	else if (c == XP_CHAR('-')) {
		GET_CHAR_TO (awk, c);
		if (c == XP_CHAR('-')) {
			SET_TOKEN_TYPE (awk, TOKEN_MINUS_MINUS);
			ADD_TOKEN_STR (awk, XP_TEXT("--"));
			GET_CHAR_TO (awk, c);
		}
		else if (c == XP_CHAR('=')) {
			SET_TOKEN_TYPE (awk, TOKEN_MINUS_ASSIGN);
			ADD_TOKEN_STR (awk, XP_TEXT("-="));
			GET_CHAR_TO (awk, c);
		}
		else if (xp_isdigit(c)) {
		//	read_number (XP_CHAR('-'));
		}
		else {
			SET_TOKEN_TYPE (awk, TOKEN_MINUS);
			ADD_TOKEN_STR (awk, XP_TEXT("-"));
		}
	}
	else if (c == XP_CHAR('(')) {
		SET_TOKEN_TYPE (awk, TOKEN_LPAREN);
		ADD_TOKEN_CHAR (awk, c);
	}
	else if (c == XP_CHAR(')')) {
		SET_TOKEN_TYPE (awk, TOKEN_RPAREN);
		ADD_TOKEN_CHAR (awk, c);
	}
	else if (c == XP_CHAR('{')) {
		SET_TOKEN_TYPE (awk, TOKEN_LBRACE);
		ADD_TOKEN_CHAR (awk, c);
	}
	else if (c == XP_CHAR('}')) {
		SET_TOKEN_TYPE (awk, TOKEN_RBRACE);
		ADD_TOKEN_CHAR (awk, c);
	}
	else if (c == XP_CHAR('[')) {
		SET_TOKEN_TYPE (awk, TOKEN_LBRACKET);
		ADD_TOKEN_CHAR (awk, c);
	}
	else if (c == XP_CHAR(']')) {
		SET_TOKEN_TYPE (awk, TOKEN_RBRACKET);
		ADD_TOKEN_CHAR (awk, c);
	}
	else {
		awk->errnum = XP_AWK_ELXCHR;
		return -1;
	}

	return 0;
}

static int __get_char (xp_awk_t* awk)
{
	if (awk->lex.ungotc_count > 0) {
		awk->lex.curc = awk->lex.ungotc[--awk->lex.ungotc_count];
		return 0;
	}

	if (awk->src_func(XP_AWK_IO_DATA, 
		awk->src_arg, &awk->lex.curc, 1) == -1) {
		awk->errnum = XP_AWK_ESRCDT;
		return -1;
	}

	return 0;
}

static int __unget_char (xp_awk_t* awk, xp_cint_t c)
{
	if (awk->lex.ungotc_count >= xp_countof(awk->lex.ungotc)) {
		awk->errnum = XP_AWK_ELXUNG;
		return -1;
	}

	awk->lex.ungotc[awk->lex.ungotc_count++] = c;
	return 0;
}

static int __skip_spaces (xp_awk_t* awk)
{
	xp_cint_t c = awk->lex.curc;
	while (xp_isspace(c)) GET_CHAR_TO (awk, c);
	return 0;
}

static int __skip_comment (xp_awk_t* awk)
{
	xp_cint_t c = awk->lex.curc;

	if (c != XP_CHAR('/')) return 0;
	GET_CHAR_TO (awk, c);

	if (c == XP_CHAR('/')) {
		do { 
			GET_CHAR_TO (awk, c);
		} while (c != '\n' && c != XP_CHAR_EOF);
		GET_CHAR (awk);
		return 1;
	}
	else if (c == XP_CHAR('*')) {
		do {
			GET_CHAR_TO (awk, c);
			if (c == XP_CHAR('*')) {
				GET_CHAR_TO (awk, c);
				if (c == XP_CHAR('/')) {
					GET_CHAR_TO (awk, c);
					break;
				}
			}
		} while (0);
		return 1;
	}

	if (__unget_char(awk, c) == -1) return -1;
	return 0;
}

static int __classfy_ident (const xp_char_t* ident)
{
	struct __kwent* p = __kwtab;

	while (p->name != XP_NULL) {
		if (xp_strcmp(p->name, ident) == 0) return p->type;
	}

	return TOKEN_IDENT;
}
