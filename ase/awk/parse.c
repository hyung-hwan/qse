/*
 * $Id: parse.c,v 1.18 2006-01-13 14:16:56 bacon Exp $
 */

#include <xp/awk/awk.h>
#include <xp/awk/tree.h>
#include <xp/awk/tree.h>
#include <xp/bas/memory.h>
#include <xp/bas/ctype.h>
#include <xp/bas/string.h>
#include <xp/bas/assert.h>

#include <xp/bas/stdio.h>
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

enum {
	BINOP_PLUS,
	BINOP_MINUS,
	BINOP_MUL,
	BINOP_DIV,
	BINOP_MOD
};

static xp_char_t __binop_char[] =
{
	XP_CHAR('+'),
	XP_CHAR('-'),
	XP_CHAR('*'),
	XP_CHAR('/'),
	XP_CHAR('%')
};

static xp_awk_node_t* __parse_program (xp_awk_t* awk);
static xp_awk_node_t* __parse_funcdcl (xp_awk_t* awk);
static xp_awk_node_t* __parse_patnact (xp_awk_t* awk);
static xp_awk_node_t* __parse_block (xp_awk_t* awk);
static xp_awk_node_t* __parse_stat (xp_awk_t* awk);
static xp_awk_node_t* __parse_expr (xp_awk_t* awk);
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

void __print_tabs (int depth)
{
	int i;
	for (i = 0; i < depth; i++) xp_printf (XP_TEXT("\t"));
}

void __print_parse_tree (xp_awk_node_t* tree, int depth)
{
	xp_awk_node_t* p;

	p = tree;
	while (p != XP_NULL) {
		if (p->type == XP_AWK_NODE_BLOCK) {
			__print_tabs (depth);
			xp_printf (XP_TEXT("{\n"));

			__print_parse_tree (((xp_awk_node_block_t*)p)->body, depth + 1);	

			__print_tabs (depth);
			xp_printf (XP_TEXT("}\n"));
		}
		else if (p->type == XP_AWK_NODE_BREAK) {
			__print_tabs (depth);
			xp_printf (XP_TEXT("break;\n"));
		}
		else if (p->type == XP_AWK_NODE_CONTINUE) {
			__print_tabs (depth);
			xp_printf (XP_TEXT("continue;\n"));
		}
		else if (p->type == XP_AWK_NODE_ASSIGN) {
			__print_tabs (depth);
			xp_printf (XP_TEXT("%s = "), ((xp_awk_node_assign_t*)p)->left);
			__print_parse_tree (((xp_awk_node_assign_t*)p)->right, 0);
			xp_printf (XP_TEXT(";\n"));
		}
		else if (p->type == XP_AWK_NODE_BINARY) {
			xp_printf (XP_TEXT("("));
			__print_parse_tree (((xp_awk_node_expr_t*)p)->left, 0);
			xp_printf (XP_TEXT(" %c "), __binop_char[((xp_awk_node_expr_t*)p)->opcode]);
			__print_parse_tree (((xp_awk_node_expr_t*)p)->right, 0);
			xp_printf (XP_TEXT(")"));
		}
		else if (p->type == XP_AWK_NODE_UNARY) {
xp_printf (XP_TEXT("unary basic expression\n"));
		}
		else if (p->type == XP_AWK_NODE_STR) {
			xp_printf (XP_TEXT("\"%s\""), ((xp_awk_node_term_t*)p)->value);
		}
		else if (p->type == XP_AWK_NODE_NUM) {
			xp_printf (XP_TEXT("%s"), ((xp_awk_node_term_t*)p)->value);
		}
		else if (p->type == XP_AWK_NODE_VAR) {
			xp_printf (XP_TEXT("%s"), ((xp_awk_node_term_t*)p)->value);
		}
		else if (p->type == XP_AWK_NODE_CALL) {
			xp_printf (XP_TEXT("%s ("), ((xp_awk_node_call_t*)p)->name);
			__print_parse_tree (((xp_awk_node_call_t*)p)->args, 0); // TODO: improve this... comman-separated...
			xp_printf (XP_TEXT(")"));
			
		}

		p = p->next;
	}
}

void print_parse_tree (xp_awk_node_t* tree)
{
	__print_parse_tree (tree, 0);
}

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
	xp_awk_node_block_t* blk;
	xp_awk_node_t* node, * prev;

	blk = (xp_awk_node_block_t*) xp_malloc (xp_sizeof(xp_awk_node_block_t));
	if (blk == XP_NULL) {
/* TODO: do some clean-up */
		PANIC (awk, XP_AWK_ENOMEM);
	}

	blk->type = XP_AWK_NODE_BLOCK;
	blk->next = XP_NULL;
	blk->body = prev = XP_NULL;

	while (1) {
		if (MATCH(awk,TOKEN_EOF)) {
/* TODO: do come clean-up */
			PANIC (awk, XP_AWK_EENDSRC);
		}

		if (MATCH(awk,TOKEN_RBRACE)) {
/* TODO: should finalize a block */
			CONSUME (awk);
			break;
		}

		if (MATCH(awk, TOKEN_SEMICOLON)) {
			/* null statement */
			CONSUME (awk);
			continue;
		}

		if (MATCH(awk,TOKEN_LBRACE)) {
			/* nested block */
			CONSUME (awk);
			node = __parse_block(awk);
		}
		else node = __parse_stat (awk);

		if (node == XP_NULL) return XP_NULL;

		if (prev == XP_NULL) blk->body = node;
		else prev->next = node;
		prev = node;
	}

	return (xp_awk_node_t*)blk;
}

static xp_awk_node_t* __parse_stat (xp_awk_t* awk)
{
	xp_awk_node_t* node;

	if (MATCH(awk,TOKEN_IF)) {
		CONSUME (awk);
		node = __parse_if(awk);
	}
	else if (MATCH(awk,TOKEN_WHILE)) {
		CONSUME (awk);
		node = __parse_while(awk);
	}
	else if (MATCH(awk,TOKEN_FOR)) {
		CONSUME (awk);
		node = __parse_for(awk);
	}
	else if (MATCH(awk,TOKEN_DO)) {
		CONSUME (awk);
		node = __parse_do(awk);
	}
	else if (MATCH(awk,TOKEN_BREAK)) {
		CONSUME (awk);
		node = __parse_break(awk);
	}
	else if (MATCH(awk,TOKEN_CONTINUE)) {
		CONSUME (awk);
		node = __parse_continue(awk);
	}
	else if (MATCH(awk,TOKEN_RETURN)) {
		CONSUME (awk);
		/* TOOD: */
		node = XP_NULL;
	}
	else if (MATCH(awk,TOKEN_EXIT)) {
		CONSUME (awk);
		/* TOOD: */
		node = XP_NULL;
	}
	else if (MATCH(awk,TOKEN_DELETE)) {
		CONSUME (awk);
		/* TOOD: */
		node = XP_NULL;
	}
	else if (MATCH(awk,TOKEN_NEXT)) {
		CONSUME (awk);
		/* TOOD: */
		node = XP_NULL;
	}
	else if (MATCH(awk,TOKEN_NEXTFILE)) {
		CONSUME (awk);
		/* TOOD: */
		node = XP_NULL;
	}
	else {
		node = __parse_expr(awk);
	}

	if (node == XP_NULL) {
// TODO: clean-up
		return XP_NULL;
	}

	/* check if a statement ends with a semicolon */
	if (!MATCH(awk,TOKEN_SEMICOLON)) {
/* TODO: do some clean-up */
		PANIC (awk, XP_AWK_ESEMICOLON);
	}

	/* eat up the semicolon and read in the next token */
	CONSUME (awk);

	return node;
}

static xp_awk_node_t* __parse_expr (xp_awk_t* awk)
{
	/*
	 * <expression> ::= <assignment> | <basic expression>
	 * <assignment> ::= <assignment target> assignmentOperator <basic expression>
	 * assignmentOperator ::= '='
	 * <basic expression> ::= 
	 */
	xp_awk_node_t* x;
	 
	if (MATCH(awk,TOKEN_IDENT)) {
// TODO: use a different approach later...
		xp_char_t* ident = xp_strdup (XP_STR_BUF(&awk->token.name));
		if (ident == XP_NULL) {
// TODO: memory clean up
			PANIC (awk, XP_AWK_ENOMEM);
		}

		CONSUME (awk);
		if (MATCH(awk,TOKEN_ASSIGN)) {
			CONSUME (awk);
			x = __parse_assignment(awk, ident);
		}
		else x = __parse_basic_expr (awk, ident);

		xp_free (ident);
		return x; // TODO: anything to clearn when x is XP_NULL????
	}

// TODO: maybe this shoudl be an error ->>> just an expression without assignment */
	return __parse_basic_expr (awk, XP_NULL);
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
// TODO: cleanup left...
			return XP_NULL; 
		}

		right = __parse_multiplicative (awk, XP_NULL);
		if (right == XP_NULL) {
// TOOD: cleanup left if necessary
			return XP_NULL;
		}

		node = (xp_awk_node_expr_t*)xp_malloc(xp_sizeof(xp_awk_node_expr_t));
		if (node == XP_NULL) {
// TODO: cleanup previous tmp (maybe current left) if necessary
// TOOD: cleanup left if necessary
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

		CONSUME (awk);
		right = __parse_unary (awk, XP_NULL);
		if (right == XP_NULL) {
// TOOD: cleanup left if necessary
			return XP_NULL;
		}

		node = (xp_awk_node_expr_t*)xp_malloc(xp_sizeof(xp_awk_node_expr_t));
		if (node == XP_NULL) {
// TODO: cleanup previous tmp (maybe current left) if necessary
// TOOD: cleanup left if necessary
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
			CONSUME (awk); // error handling... replace it by get_token...
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

		CONSUME (awk);
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

		CONSUME (awk);
		return (xp_awk_node_t*)node;
	}
	else if (MATCH(awk,TOKEN_LPAREN)) {
		xp_awk_node_t* node;

		CONSUME (awk);
		node = __parse_expr (awk);
		if (node == XP_NULL) return XP_NULL;

		if (!MATCH(awk,TOKEN_RPAREN)) {
// TODO: free tmp...
			PANIC (awk, XP_AWK_ERPAREN);
		}
		CONSUME (awk);

		return node;
	}

	/* valid expression introducer is expected */
	PANIC (awk, XP_AWK_EEXPR);
}

static xp_awk_node_t* __parse_funcall (xp_awk_t* awk, xp_char_t* name)
{
	xp_awk_node_call_t* node;
	xp_awk_node_t* tmp, * cur;

	node = (xp_awk_node_call_t*)xp_malloc (xp_sizeof(xp_awk_node_call_t));
	if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

	node->type = XP_AWK_NODE_CALL;
	node->next = XP_NULL;
	node->name = name;
	node->args = XP_NULL;

	CONSUME (awk);
	if (MATCH(awk,TOKEN_RPAREN)) {
		CONSUME (awk);
		return (xp_awk_node_t*)node;
	}

	cur = XP_NULL;
	while (1) {
		tmp = __parse_expr (awk);
		if (tmp == XP_NULL) {
// TODO: free all other nodes associated into node->args
			xp_free (node);
			return XP_NULL;
		}

		if (cur == XP_NULL) node->args = tmp;
		else cur->next = tmp;
		cur = tmp;

		if (MATCH(awk,TOKEN_RPAREN)) {
			CONSUME (awk);
			break;
		}

		if (!MATCH(awk,TOKEN_COMMA)) {
// TODO: free all other nodes associated into node->args
			xp_free (node);
			PANIC (awk, XP_AWK_ECOMMA);	
		}

		CONSUME (awk);
	}

	return (xp_awk_node_t*)node;
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
	node->next = XP_NULL;
	
	/* TODO: do i have to consume a semicolon here???? */
	return node;
}

static xp_awk_node_t* __parse_continue (xp_awk_t* awk)
{
	xp_awk_node_t* node;

	node = (xp_awk_node_t*) xp_malloc (xp_sizeof(xp_awk_node_t));
	if (node == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	node->type = XP_AWK_NODE_CONTINUE;
	node->next = XP_NULL;
	
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
