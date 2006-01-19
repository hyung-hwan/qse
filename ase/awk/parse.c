/*
 * $Id: parse.c,v 1.25 2006-01-19 13:28:29 bacon Exp $
 */

#include <xp/awk/awk.h>

#ifndef __STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/ctype.h>
#include <xp/bas/string.h>
#include <xp/bas/assert.h>
#endif

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
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_MOD,

	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,

	TOKEN_COMMA,
	TOKEN_SEMICOLON,

	TOKEN_INTEGER,
	TOKEN_STRING,
	TOKEN_REGEX,

	TOKEN_IDENT,
	TOKEN_BEGIN,
	TOKEN_END,
	TOKEN_FUNCTION,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_WHILE,
	TOKEN_FOR,
	TOKEN_DO,
	TOKEN_BREAK,
	TOKEN_CONTINUE,
	TOKEN_RETURN,
	TOKEN_EXIT,
	TOKEN_DELETE,
	TOKEN_NEXT,
	TOKEN_NEXTFILE,

	__TOKEN_COUNT__
};

enum {
	BINOP_PLUS,
	BINOP_MINUS,
	BINOP_MUL,
	BINOP_DIV,
	BINOP_MOD
};

static xp_awk_node_t* __parse_program (xp_awk_t* awk);
static xp_awk_node_t* __parse_funcdcl (xp_awk_t* awk);
static xp_awk_node_t* __parse_patnact (xp_awk_t* awk);
static xp_awk_node_t* __parse_block (xp_awk_t* awk);
static xp_awk_node_t* __parse_statement (xp_awk_t* awk);
static xp_awk_node_t* __parse_statement_nb (xp_awk_t* awk);
static xp_awk_node_t* __parse_expression (xp_awk_t* awk);
static xp_awk_node_t* __parse_assignment (xp_awk_t* awk, xp_char_t* ident);
static xp_awk_node_t* __parse_basic_expr (xp_awk_t* awk, xp_char_t* ident);
static xp_awk_node_t* __parse_additive (xp_awk_t* awk, xp_char_t* ident);
static xp_awk_node_t* __parse_multiplicative (xp_awk_t* awk, xp_char_t* ident);
static xp_awk_node_t* __parse_unary (xp_awk_t* awk, xp_char_t* ident);
static xp_awk_node_t* __parse_primary (xp_awk_t* awk, xp_char_t* ident);
static xp_awk_node_t* __parse_funcall (xp_awk_t* awk, xp_char_t* name);
static xp_awk_node_t* __parse_if (xp_awk_t* awk);
static xp_awk_node_t* __parse_while (xp_awk_t* awk);
static xp_awk_node_t* __parse_for (xp_awk_t* awk);
static xp_awk_node_t* __parse_dowhile (xp_awk_t* awk);
static xp_awk_node_t* __parse_break (xp_awk_t* awk);
static xp_awk_node_t* __parse_continue (xp_awk_t* awk);
static xp_awk_node_t* __parse_return (xp_awk_t* awk);
static xp_awk_node_t* __parse_exit (xp_awk_t* awk);

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
	{ XP_TEXT("else"),     TOKEN_ELSE },
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
	if (xp_str_ccat(&(awk)->token.name,(c)) == (xp_size_t)-1) { \
		(awk)->errnum = XP_AWK_ENOMEM; return -1; \
	} \
} while (0)

#define ADD_TOKEN_STR(awk,str) do { \
	if (xp_str_cat(&(awk)->token.name,(str)) == (xp_size_t)-1) { \
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
	if (node == XP_NULL) {
xp_printf (XP_TEXT("error - %d\n"), awk->errnum);
		return -1;
	}

xp_printf (XP_TEXT("end - %d\n"), awk->errnum);
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

	while (1) {
		if (MATCH(awk,TOKEN_FUNCTION)) {
			node = __parse_funcdcl(awk);
		}
		else {
			node = __parse_patnact(awk);
		}

		if (node == XP_NULL) return XP_NULL;
xp_printf (XP_TEXT(">>>>> breaking ... for testing ...\n"));
break;
	}

	return node;
}

static xp_awk_node_t* __parse_funcdcl (xp_awk_t* awk)
{
	return XP_NULL;
}

static xp_awk_node_t* __parse_patnact (xp_awk_t* awk)
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
		if (__get_token(awk) == -1) return XP_NULL; 
	}
	else if (MATCH(awk,TOKEN_END)) {
		if (__get_token(awk) == -1) return XP_NULL; 
	}
	/* patterns ...
	 * etc */

	if (!MATCH(awk,TOKEN_LBRACE)) PANIC (awk, XP_AWK_ELBRACE);
	if (__get_token(awk) == -1) return XP_NULL; 

	node = __parse_block(awk);
	if (node == XP_NULL) return XP_NULL;

	return node;
}

/* TODO: what is the best name for the parsing routine for the outermost block? */
static xp_awk_node_t* __parse_block (xp_awk_t* awk) 
{
	xp_awk_node_t* head, * curr, * node;
	xp_awk_node_block_t* block;

	head = XP_NULL; curr = XP_NULL;

	while (1) {
		if (MATCH(awk,TOKEN_EOF)) {
			if (head != XP_NULL) xp_awk_clrpt (head);
			PANIC (awk, XP_AWK_EENDSRC);
		}

		if (MATCH(awk,TOKEN_RBRACE)) {
			if (__get_token(awk) == -1) {
				if (head != XP_NULL) xp_awk_clrpt (head);
				return XP_NULL; 
			}
			break;
		}

/* if you want to remove top-level null statement... get it here... */
/*
		if (MATCH(awk,TOKEN_SEMICOLON)) {
			if (__get_token(awk) == -1) {
				if (head != XP_NULL) xp_awk_clrpt (head);
				return XP_NULL;
			}
			continue;
		}
*/

		node = __parse_statement (awk);
		if (node == XP_NULL) {
			if (head != XP_NULL) xp_awk_clrpt (head);
			return XP_NULL;
		}
		
		if (curr == XP_NULL) head = node;
		else curr->next = node;	
		curr = node;
	}

	block = (xp_awk_node_block_t*) xp_malloc (xp_sizeof(xp_awk_node_block_t));
	if (block == XP_NULL) {
		xp_awk_clrpt (head);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	block->type = XP_AWK_NODE_BLOCK;
	block->next = XP_NULL;
	block->body = head;

	return (xp_awk_node_t*)block;
}

static xp_awk_node_t* __parse_statement (xp_awk_t* awk)
{
	xp_awk_node_t* node;

	if (MATCH(awk,TOKEN_SEMICOLON)) {
		/* null statement */	
		node = (xp_awk_node_t*) xp_malloc (xp_sizeof(xp_awk_node_t));
		if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		node->type = XP_AWK_NODE_NULL;
		node->next = XP_NULL;

		if (__get_token(awk) == -1) {
			xp_free (node);
			return XP_NULL;
		}
	}
	else if (MATCH(awk,TOKEN_LBRACE)) {
		if (__get_token(awk) == -1) return XP_NULL; 
		node = __parse_block (awk);
	}
	else node = __parse_statement_nb (awk);

	return node;
}

static xp_awk_node_t* __parse_statement_nb (xp_awk_t* awk)
{
	xp_awk_node_t* node;

	/* 
	 * keywords that don't require any terminating semicolon 
	 */
	if (MATCH(awk,TOKEN_IF)) {
		if (__get_token(awk) == -1) return XP_NULL;
		return __parse_if (awk);
	}
	else if (MATCH(awk,TOKEN_WHILE)) {
		if (__get_token(awk) == -1) return XP_NULL;
		return __parse_while (awk);
	}
	else if (MATCH(awk,TOKEN_FOR)) {
		if (__get_token(awk) == -1) return XP_NULL;
		return __parse_for (awk);
	}

	/* 
	 * keywords that require a terminating semicolon 
	 */
	if (MATCH(awk,TOKEN_DO)) {
		if (__get_token(awk) == -1) return XP_NULL;
		node = __parse_dowhile (awk);
	}
	else if (MATCH(awk,TOKEN_BREAK)) {
		if (__get_token(awk) == -1) return XP_NULL;
		node = __parse_break(awk);
	}
	else if (MATCH(awk,TOKEN_CONTINUE)) {
		if (__get_token(awk) == -1) return XP_NULL;
		node = __parse_continue(awk);
	}
	else if (MATCH(awk,TOKEN_RETURN)) {
		if (__get_token(awk) == -1) return XP_NULL;
		node = __parse_return(awk);
	}
	else if (MATCH(awk,TOKEN_EXIT)) {
		if (__get_token(awk) == -1) return XP_NULL;
		node = __parse_exit(awk);
	}
/*
TODO:
	else if (MATCH(awk,TOKEN_DELETE)) {
		if (__get_token(awk) == -1) return XP_NULL;
		node = __parse_delete(awk);
	}
	else if (MATCH(awk,TOKEN_NEXT)) {
		if (__get_token(awk) == -1) return XP_NULL;
		node = __parse_next(awk);
	}
	else if (MATCH(awk,TOKEN_NEXTFILE)) {
		if (__get_token(awk) == -1) return XP_NULL;
		node = __parse_nextfile(awk);
	}
*/
	else {
		node = __parse_expression(awk);
	}

	if (node == XP_NULL) return XP_NULL;

	/* check if a statement ends with a semicolon */
	if (!MATCH(awk,TOKEN_SEMICOLON)) {
		if (node != XP_NULL) xp_awk_clrpt (node);
		PANIC (awk, XP_AWK_ESEMICOLON);
	}

	/* eat up the semicolon and read in the next token */
	if (__get_token(awk) == -1) {
		if (node != XP_NULL) xp_awk_clrpt (node);
		return XP_NULL;
	}

	return node;
}

static xp_awk_node_t* __parse_expression (xp_awk_t* awk)
{
	/*
	 * <expression> ::= <assignment> | <basic expression>
	 * <assignment> ::= <assignment target> assignmentOperator <basic expression>
	 * assignmentOperator ::= '='
	 * <basic expression> ::= 
	 */

	xp_awk_node_t* x;
	 
	if (MATCH(awk,TOKEN_IDENT)) {
		/* the expression starts with an identifier */

		xp_char_t* ident = xp_strdup (XP_STR_BUF(&awk->token.name));
		if (ident == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		if (__get_token(awk) == -1) {
			xp_free (ident);
			return XP_NULL;
		}
		if (MATCH(awk,TOKEN_ASSIGN)) {
			if (__get_token(awk) == -1) {
				xp_free (ident);
				return XP_NULL;
			}
			x = __parse_assignment (awk, ident);
		}
		else x = __parse_basic_expr (awk, ident);

		xp_free (ident);
	}
	else {
		/* the expression starts with a non-identifier */

// TODO: maybe this should be an error ->>> just an expression without assignment */
		x = __parse_basic_expr (awk, XP_NULL);
	}

	return x;
}

static xp_awk_node_t* __parse_assignment (xp_awk_t* awk, xp_char_t* ident)
{
	xp_awk_node_assign_t* node;
	xp_awk_node_t* value;
	xp_char_t* idtdup;

	xp_assert (ident != XP_NULL);

	node = (xp_awk_node_assign_t*)xp_malloc (xp_sizeof(xp_awk_node_assign_t));
	if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

	idtdup = xp_strdup (ident);
	if (idtdup == XP_NULL) {
		xp_free (node);
		PANIC (awk, XP_AWK_ENOMEM);
	}	

	value = __parse_basic_expr (awk, XP_NULL);
	if (value == XP_NULL) {
		xp_free (idtdup);
		xp_free (node);
		return XP_NULL;
	}

	node->type = XP_AWK_NODE_ASSIGN;
	node->next = XP_NULL;
	node->left = idtdup;	
	node->right = value;

	return (xp_awk_node_t*)node;
}

static xp_awk_node_t* __parse_basic_expr (xp_awk_t* awk, xp_char_t* ident)
{
	/* <basic expression> ::= <additive> 
	 * <additive> ::= <multiplicative> [additiveOp <multiplicative>]*
	 * <multiplicative> ::= <unary> [multiplicativeOp <unary>]*
	 * <unary> ::= [unaryOp]* <term>
	 * <term> ::= <function call> | variable | literals
	 * <function call> ::= <identifier> <lparen> <basic expression list>* <rparen>
	 * <basic expression list> ::= <basic expression> [comma <basic expression>]*
	 */
	
	return __parse_additive (awk, ident);
}


static xp_awk_node_t* __parse_additive (xp_awk_t* awk, xp_char_t* ident)
{
	xp_awk_node_expr_t* node;
	xp_awk_node_t* left, * right;
	int opcode;

	left = __parse_multiplicative (awk, ident);
	if (left == XP_NULL) return XP_NULL;
	
	while (1) {
		if (MATCH(awk,TOKEN_PLUS)) opcode = BINOP_PLUS;
		else if (MATCH(awk,TOKEN_MINUS)) opcode = BINOP_MINUS;
		else break;

		if (__get_token(awk) == -1) {
			xp_awk_clrpt (left);
			return XP_NULL; 
		}

		right = __parse_multiplicative (awk, XP_NULL);
		if (right == XP_NULL) {
			xp_awk_clrpt (left);
			return XP_NULL;
		}

		node = (xp_awk_node_expr_t*)xp_malloc(xp_sizeof(xp_awk_node_expr_t));
		if (node == XP_NULL) {
			xp_awk_clrpt (right);
			xp_awk_clrpt (left);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		node->type = XP_AWK_NODE_BINARY;
		node->next = XP_NULL;
		node->opcode = opcode; 
		node->left = left;
		node->right = right;

		left = (xp_awk_node_t*)node;
	}

	return left;
}

static xp_awk_node_t* __parse_multiplicative (xp_awk_t* awk, xp_char_t* ident)
{
	xp_awk_node_expr_t* node;
	xp_awk_node_t* left, * right;
	int opcode;

	left = __parse_unary (awk, ident);
	if (left == XP_NULL) return XP_NULL;
	
	while (1) {
		if (MATCH(awk,TOKEN_MUL)) opcode = BINOP_MUL;
		else if (MATCH(awk,TOKEN_DIV)) opcode = BINOP_DIV;
		else if (MATCH(awk,TOKEN_MOD)) opcode = BINOP_MOD;
		else break;

		if (__get_token(awk) == -1) {
			xp_awk_clrpt (left);
			return XP_NULL; 
		}

		right = __parse_unary (awk, XP_NULL);
		if (right == XP_NULL) {
			xp_awk_clrpt (left);	
			return XP_NULL;
		}

		node = (xp_awk_node_expr_t*)xp_malloc(xp_sizeof(xp_awk_node_expr_t));
		if (node == XP_NULL) {
			xp_awk_clrpt (right);	
			xp_awk_clrpt (left);	
			PANIC (awk, XP_AWK_ENOMEM);
		}

		node->type = XP_AWK_NODE_BINARY;
		node->next = XP_NULL;
		node->opcode = opcode;
		node->left = left;
		node->right = right;

		left = (xp_awk_node_t*)node;
	}

	return left;
}

static xp_awk_node_t* __parse_unary (xp_awk_t* awk, xp_char_t* ident)
{
	return __parse_primary (awk, ident);
}

static xp_awk_node_t* __parse_primary (xp_awk_t* awk, xp_char_t* ident)
{
	if (ident != XP_NULL || MATCH(awk,TOKEN_IDENT))  {
		xp_char_t* name;

		if (ident != XP_NULL) {
			name = (xp_char_t*)xp_strdup(ident);
			if (name == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
		}
		else {
			name = (xp_char_t*)xp_strdup(XP_STR_BUF(&awk->token.name));
			if (name == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

			if (__get_token(awk) == -1) {
				xp_free (name);	
				return XP_NULL;			
			}
		}

		if (MATCH(awk,TOKEN_LPAREN)) {
			/* function call */
			xp_awk_node_t* node;
			node = __parse_funcall (awk, name);
			if (node == XP_NULL) xp_free (name);
			return (xp_awk_node_t*)node;
		}	
		else {
			/* normal variable */
			xp_awk_node_term_t* node;
	
			node = (xp_awk_node_term_t*)xp_malloc(xp_sizeof(xp_awk_node_term_t));
			if (node == XP_NULL) {
				xp_free (name);
				PANIC (awk, XP_AWK_ENOMEM);
			}

			node->type = XP_AWK_NODE_VAR;
			node->next = XP_NULL;
			node->value = name;

			return (xp_awk_node_t*)node;
		}
	}
	else if (MATCH(awk,TOKEN_INTEGER)) {
		xp_awk_node_term_t* node;

		node = (xp_awk_node_term_t*)xp_malloc(xp_sizeof(xp_awk_node_term_t));
		if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		node->type = XP_AWK_NODE_NUM;
		node->next = XP_NULL;
		node->value = xp_strdup(XP_STR_BUF(&awk->token.name));
		if (node->value == XP_NULL) {
			xp_free (node);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		if (__get_token(awk) == -1) {
			xp_free (node->value);
			xp_free (node);
			return XP_NULL;			
		}

		return (xp_awk_node_t*)node;
	}
	else if (MATCH(awk,TOKEN_STRING))  {
		xp_awk_node_term_t* node;

		node = (xp_awk_node_term_t*)xp_malloc(xp_sizeof(xp_awk_node_term_t));
		if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		node->type = XP_AWK_NODE_STR;
		node->next = XP_NULL;
		node->value = xp_strdup(XP_STR_BUF(&awk->token.name));
		if (node->value == XP_NULL) {
			xp_free (node);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		if (__get_token(awk) == -1) {
			xp_free (node->value);
			xp_free (node);
			return XP_NULL;			
		}

		return (xp_awk_node_t*)node;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) {
		xp_awk_node_t* node;

		/* eat up the left parenthesis */
		if (__get_token(awk) == -1) return XP_NULL;

		/* parse the sub-expression inside the parentheses */
		node = __parse_expression (awk);
		if (node == XP_NULL) return XP_NULL;

		/* check for the closing parenthesis */
		if (!MATCH(awk,TOKEN_RPAREN)) {
			xp_awk_clrpt (node);
			PANIC (awk, XP_AWK_ERPAREN);
		}

		if (__get_token(awk) == -1) {
			xp_awk_clrpt (node);
			return XP_NULL;
		}

		return node;
	}

	/* valid expression introducer is expected */
	PANIC (awk, XP_AWK_EEXPR);
}

static xp_awk_node_t* __parse_funcall (xp_awk_t* awk, xp_char_t* name)
{
	xp_awk_node_t* head, * curr, * node;
	xp_awk_node_call_t* call;

	if (__get_token(awk) == -1) return XP_NULL;
	
	head = curr = XP_NULL;

	if (MATCH(awk,TOKEN_RPAREN)) {
		/* no parameters to the function call */
		if (__get_token(awk) == -1) return XP_NULL;
	}
	else {
		while (1) {
			node = __parse_expression (awk);
			if (node == XP_NULL) {
				if (head != XP_NULL) xp_awk_clrpt (head);
				return XP_NULL;
			}
	
			if (head == XP_NULL) head = node;
			else curr->next = node;
			curr = node;

			if (MATCH(awk,TOKEN_RPAREN)) {
				if (__get_token(awk) == -1) {
					if (head != XP_NULL) xp_awk_clrpt (head);
					return XP_NULL;
				}
				break;
			}

			if (!MATCH(awk,TOKEN_COMMA)) {
				if (head != XP_NULL) xp_awk_clrpt (head);
				PANIC (awk, XP_AWK_ECOMMA);	
			}

			if (__get_token(awk) == -1) {
				if (head != XP_NULL) xp_awk_clrpt (head);
				return XP_NULL;
			}
		}

	}

	call = (xp_awk_node_call_t*)xp_malloc (xp_sizeof(xp_awk_node_call_t));
	if (call == XP_NULL) {
		if (head != XP_NULL) xp_awk_clrpt (head);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	call->type = XP_AWK_NODE_CALL;
	call->next = XP_NULL;
	call->name = name;
	call->args = head;

	return (xp_awk_node_t*)call;
}

static xp_awk_node_t* __parse_if (xp_awk_t* awk)
{
	xp_awk_node_t* test;
	xp_awk_node_t* then_part;
	xp_awk_node_t* else_part;
	xp_awk_node_if_t* node;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, XP_AWK_ELPAREN);
	if (__get_token(awk) == -1) return XP_NULL;

	test = __parse_expression (awk);
	if (test == XP_NULL) return XP_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) {
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) {
		xp_awk_clrpt (test);
		return XP_NULL;
	}

	then_part = __parse_statement (awk);
	if (then_part == XP_NULL) {
		xp_awk_clrpt (test);
		return XP_NULL;
	}

	if (MATCH(awk,TOKEN_ELSE)) {
		if (__get_token(awk) == -1) {
			xp_awk_clrpt (then_part);
			xp_awk_clrpt (test);
			return XP_NULL;
		}

		else_part = __parse_statement (awk);
		if (else_part == XP_NULL) {
			xp_awk_clrpt (then_part);
			xp_awk_clrpt (test);
			return XP_NULL;
		}
	}
	else else_part = XP_NULL;

	node = (xp_awk_node_if_t*) xp_malloc (xp_sizeof(xp_awk_node_if_t));
	if (node == XP_NULL) {
		xp_awk_clrpt (else_part);
		xp_awk_clrpt (then_part);
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	node->type = XP_AWK_NODE_IF;
	node->next = XP_NULL;
	node->test = test;
	node->then_part = then_part;
	node->else_part = else_part;

	return (xp_awk_node_t*)node;
}

static xp_awk_node_t* __parse_while (xp_awk_t* awk)
{
	xp_awk_node_t* test, * body;
	xp_awk_node_while_t* node;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, XP_AWK_ELPAREN);
	if (__get_token(awk) == -1) return XP_NULL;

	test = __parse_expression (awk);
	if (test == XP_NULL) return XP_NULL;

	if (!MATCH(awk,TOKEN_RPAREN)) {
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) {
		xp_awk_clrpt (test);
		return XP_NULL;
	}

	body = __parse_statement (awk);
	if (body == XP_NULL) {
		xp_awk_clrpt (test);
		return XP_NULL;
	}

	node = (xp_awk_node_while_t*) xp_malloc (xp_sizeof(xp_awk_node_while_t));
	if (node == XP_NULL) {
		xp_awk_clrpt (body);
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	node->type = XP_AWK_NODE_WHILE;
	node->next = XP_NULL;
	node->test = test;
	node->body = body;

	return (xp_awk_node_t*)node;
}

static xp_awk_node_t* __parse_for (xp_awk_t* awk)
{
	xp_awk_node_t* init, * test, * incr, * body;
	xp_awk_node_for_t* node;

	if (!MATCH(awk,TOKEN_LPAREN)) PANIC (awk, XP_AWK_ELPAREN);
	if (__get_token(awk) == -1) return XP_NULL;
		
	if (MATCH(awk,TOKEN_SEMICOLON)) init = XP_NULL;
	else {
		init = __parse_expression (awk);
		if (init == XP_NULL) return XP_NULL;

		if (!MATCH(awk,TOKEN_SEMICOLON)) {
			xp_awk_clrpt (init);
			PANIC (awk, XP_AWK_ESEMICOLON);
		}
	}

	if (__get_token(awk) == -1) {
		xp_awk_clrpt (init);
		return XP_NULL;
	}

	if (MATCH(awk,TOKEN_SEMICOLON)) test = XP_NULL;
	else {
		test = __parse_expression (awk);
		if (test == XP_NULL) {
			xp_awk_clrpt (init);
			return XP_NULL;
		}

		if (!MATCH(awk,TOKEN_SEMICOLON)) {
			xp_awk_clrpt (init);
			xp_awk_clrpt (test);
			PANIC (awk, XP_AWK_ESEMICOLON);
		}
	}

	if (__get_token(awk) == -1) {
		xp_awk_clrpt (init);
		xp_awk_clrpt (test);
		return XP_NULL;
	}
	
	if (MATCH(awk,TOKEN_RPAREN)) incr = XP_NULL;
	else {
		incr = __parse_expression (awk);
		if (incr == XP_NULL) {
			xp_awk_clrpt (init);
			xp_awk_clrpt (test);
			return XP_NULL;
		}

		if (!MATCH(awk,TOKEN_RPAREN)) {
			xp_awk_clrpt (init);
			xp_awk_clrpt (test);
			xp_awk_clrpt (incr);
			PANIC (awk, XP_AWK_ERPAREN);
		}
	}

	if (__get_token(awk) == -1) {
		xp_awk_clrpt (init);
		xp_awk_clrpt (test);
		xp_awk_clrpt (incr);
		return XP_NULL;
	}

	body = __parse_statement (awk);
	if (body == XP_NULL) {
		xp_awk_clrpt (init);
		xp_awk_clrpt (test);
		xp_awk_clrpt (incr);
		return XP_NULL;
	}

	node = (xp_awk_node_for_t*) xp_malloc (xp_sizeof(xp_awk_node_for_t));
	if (node == XP_NULL) {
		xp_awk_clrpt (init);
		xp_awk_clrpt (test);
		xp_awk_clrpt (incr);
		xp_awk_clrpt (body);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	node->type = XP_AWK_NODE_FOR;
	node->next = XP_NULL;
	node->init = init;
	node->test = test;
	node->incr = incr;
	node->body = body;

	return (xp_awk_node_t*)node;
}

static xp_awk_node_t* __parse_dowhile (xp_awk_t* awk)
{
	xp_awk_node_t* test, * body;
	xp_awk_node_while_t* node;

	body = __parse_statement (awk);
	if (body == XP_NULL) return XP_NULL;

	if (!MATCH(awk,TOKEN_WHILE)) {
		xp_awk_clrpt (body);
		PANIC (awk, XP_AWK_EWHILE);
	}

	if (__get_token(awk) == -1) {
		xp_awk_clrpt (body);
		return XP_NULL;
	}

	if (!MATCH(awk,TOKEN_LPAREN)) {
		xp_awk_clrpt (body);
		PANIC (awk, XP_AWK_ELPAREN);
	}

	if (__get_token(awk) == -1) {
		xp_awk_clrpt (body);
		return XP_NULL;
	}

	test = __parse_expression (awk);
	if (test == XP_NULL) {
		xp_awk_clrpt (body);
		return XP_NULL;
	}

	if (!MATCH(awk,TOKEN_RPAREN)) {
		xp_awk_clrpt (body);
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ERPAREN);
	}

	if (__get_token(awk) == -1) {
		xp_awk_clrpt (body);
		xp_awk_clrpt (test);
		return XP_NULL;
	}
	
	node = (xp_awk_node_while_t*) xp_malloc (xp_sizeof(xp_awk_node_while_t));
	if (node == XP_NULL) {
		xp_awk_clrpt (body);
		xp_awk_clrpt (test);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	node->type = XP_AWK_NODE_DOWHILE;
	node->next = XP_NULL;
	node->test = test;
	node->body = body;

	return (xp_awk_node_t*)node;
}

static xp_awk_node_t* __parse_break (xp_awk_t* awk)
{
	xp_awk_node_t* node;

	node = (xp_awk_node_t*) xp_malloc (xp_sizeof(xp_awk_node_t));
	if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	node->type = XP_AWK_NODE_BREAK;
	node->next = XP_NULL;
	
	return node;
}

static xp_awk_node_t* __parse_continue (xp_awk_t* awk)
{
	xp_awk_node_t* node;

	node = (xp_awk_node_t*) xp_malloc (xp_sizeof(xp_awk_node_t));
	if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	node->type = XP_AWK_NODE_CONTINUE;
	node->next = XP_NULL;
	
	return node;
}

static xp_awk_node_t* __parse_return (xp_awk_t* awk)
{
	xp_awk_node_block_t* node;
	xp_awk_node_t* val;

	node = (xp_awk_node_block_t*) xp_malloc (xp_sizeof(xp_awk_node_block_t));
	if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	node->type = XP_AWK_NODE_RETURN;
	node->next = XP_NULL;

	val = __parse_expression (awk);
	if (val == XP_NULL) {
		xp_free (node);
		return XP_NULL;
	}

	node->body = val;
	return (xp_awk_node_t*)node;
}

static xp_awk_node_t* __parse_exit (xp_awk_t* awk)
{
	xp_awk_node_block_t* node;
	xp_awk_node_t* val;

	node = (xp_awk_node_block_t*) xp_malloc (xp_sizeof(xp_awk_node_block_t));
	if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	node->type = XP_AWK_NODE_EXIT;
	node->next = XP_NULL;

	val = __parse_expression (awk);
	if (val == XP_NULL) {
		xp_free (node);
		return XP_NULL;
	}

	node->body = val;
	return (xp_awk_node_t*)node;
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
		do {
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		} while (xp_isdigit(c));

		SET_TOKEN_TYPE (awk, TOKEN_INTEGER);
// TODO: enhance nubmer handling
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
		GET_CHAR_TO (awk, c);
		do {
			ADD_TOKEN_CHAR (awk, c);
			GET_CHAR_TO (awk, c);
		} while (c != XP_CHAR('\"'));

		SET_TOKEN_TYPE (awk, TOKEN_STRING);
		GET_CHAR_TO (awk, c); 
// TODO: enhance string handling including escaping
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
	else if (c == XP_CHAR('*')) {
		SET_TOKEN_TYPE (awk, TOKEN_MUL);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR('/')) {
// TODO: handle regular expression here... /^pattern$/
		SET_TOKEN_TYPE (awk, TOKEN_DIV);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR('%')) {
		SET_TOKEN_TYPE (awk, TOKEN_MOD);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR('(')) {
		SET_TOKEN_TYPE (awk, TOKEN_LPAREN);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR(')')) {
		SET_TOKEN_TYPE (awk, TOKEN_RPAREN);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR('{')) {
		SET_TOKEN_TYPE (awk, TOKEN_LBRACE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR('}')) {
		SET_TOKEN_TYPE (awk, TOKEN_RBRACE);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR('[')) {
		SET_TOKEN_TYPE (awk, TOKEN_LBRACKET);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR(']')) {
		SET_TOKEN_TYPE (awk, TOKEN_RBRACKET);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR(',')) {
		SET_TOKEN_TYPE (awk, TOKEN_COMMA);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else if (c == XP_CHAR(';')) {
		SET_TOKEN_TYPE (awk, TOKEN_SEMICOLON);
		ADD_TOKEN_CHAR (awk, c);
		GET_CHAR_TO (awk, c);
	}
	else {
		awk->errnum = XP_AWK_ELXCHR;
		return -1;
	}

	return 0;
}

static int __get_char (xp_awk_t* awk)
{
	xp_ssize_t n;

	if (awk->lex.ungotc_count > 0) {
		awk->lex.curc = awk->lex.ungotc[--awk->lex.ungotc_count];
		return 0;
	}

	n = awk->src_func(XP_AWK_IO_DATA, awk->src_arg, &awk->lex.curc, 1);
	if (n == -1) {
		awk->errnum = XP_AWK_ESRCDT;
		return -1;
	}
	if (n == 0) awk->lex.curc = XP_CHAR_EOF;
	
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
	awk->lex.curc = XP_CHAR('/');

	return 0;
}

static int __classfy_ident (const xp_char_t* ident)
{
	struct __kwent* p = __kwtab;

	while (p->name != XP_NULL) {
		if (xp_strcmp(p->name, ident) == 0) return p->type;
		p++;
	}

	return TOKEN_IDENT;
}
