/*
 * $Id: parser.c,v 1.60 2005-07-10 03:16:40 bacon Exp $
 */

#include <xp/stx/parser.h>
#include <xp/stx/object.h>
#include <xp/stx/class.h>
#include <xp/stx/method.h>
#include <xp/stx/symbol.h>
#include <xp/stx/bytecode.h>
#include <xp/stx/hash.h>
#include <xp/stx/misc.h>

static int __parse_method (
	xp_stx_parser_t* parser, 
	xp_word_t method_class, void* input);
static int __finish_method (xp_stx_parser_t* parser);

static int __parse_message_pattern (xp_stx_parser_t* parser);
static int __parse_unary_pattern (xp_stx_parser_t* parser);
static int __parse_binary_pattern (xp_stx_parser_t* parser);
static int __parse_keyword_pattern (xp_stx_parser_t* parser);

static int __parse_temporaries (xp_stx_parser_t* parser);
static int __parse_primitive (xp_stx_parser_t* parser);
static int __parse_statements (xp_stx_parser_t* parser);
static int __parse_block_statements (xp_stx_parser_t* parser);
static int __parse_statement (xp_stx_parser_t* parser);
static int __parse_expression (xp_stx_parser_t* parser);

static int __parse_assignment (
	xp_stx_parser_t* parser, const xp_char_t* target);
static int __parse_basic_expression (
	xp_stx_parser_t* parser, const xp_char_t* ident);
static int __parse_primary (
	xp_stx_parser_t* parser, const xp_char_t* ident);
static int __parse_primary_ident (
	xp_stx_parser_t* parser, const xp_char_t* ident);

static int __parse_block_constructor (xp_stx_parser_t* parser);
static int __parse_message_continuation (xp_stx_parser_t* parser);
static int __parse_keyword_message (xp_stx_parser_t* parser);
static int __parse_binary_message (xp_stx_parser_t* parser);
static int __parse_unary_message (xp_stx_parser_t* parser);

static int __get_token (xp_stx_parser_t* parser);
static int __get_ident (xp_stx_parser_t* parser);
static int __get_numlit (xp_stx_parser_t* parser, xp_bool_t negated);
static int __get_charlit (xp_stx_parser_t* parser);
static int __get_strlit (xp_stx_parser_t* parser);
static int __get_binary (xp_stx_parser_t* parser);
static int __skip_spaces (xp_stx_parser_t* parser);
static int __skip_comment (xp_stx_parser_t* parser);
static int __get_char (xp_stx_parser_t* parser);
static int __unget_char (xp_stx_parser_t* parser, xp_cint_t c);
static int __open_input (xp_stx_parser_t* parser, void* input);
static int __close_input (xp_stx_parser_t* parser);

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser, xp_stx_t* stx)
{
	if (parser == XP_NULL) {
		parser = (xp_stx_parser_t*)
			xp_malloc (xp_sizeof(xp_stx_parser_t));		
		if (parser == XP_NULL) return XP_NULL;
		parser->__malloced = xp_true;
	}
	else parser->__malloced = xp_false;

	if (xp_stx_name_open (&parser->method_name, 0) == XP_NULL) {
		if (parser->__malloced) xp_free (parser);
		return XP_NULL;
	}

	if (xp_stx_token_open (&parser->token, 0) == XP_NULL) {
		xp_stx_name_close (&parser->method_name);
		if (parser->__malloced) xp_free (parser);
		return XP_NULL;
	}

	if (xp_array_open (
		&parser->bytecode, 256, 
		xp_sizeof(xp_byte_t), XP_NULL) == XP_NULL) {
		xp_stx_name_close (&parser->method_name);
		xp_stx_token_close (&parser->token);
		if (parser->__malloced) xp_free (parser);
		return XP_NULL;
	}

	parser->stx = stx;
	parser->error_code = XP_STX_PARSER_ERROR_NONE;

	parser->temporary_count = 0;
	parser->argument_count = 0;
	parser->literal_count = 0;

	parser->curc = XP_CHAR_EOF;
	parser->ungotc_count = 0;

	parser->input_owner = XP_NULL;
	parser->input_func = XP_NULL;
	return parser;
}

void xp_stx_parser_close (xp_stx_parser_t* parser)
{
	while (parser->temporary_count > 0) {
		xp_free (parser->temporaries[--parser->temporary_count]);
	}
	parser->argument_count = 0;

	xp_array_close (&parser->bytecode);
	xp_stx_name_close (&parser->method_name);
	xp_stx_token_close (&parser->token);

	if (parser->__malloced) xp_free (parser);
}

#define EMIT_CODE_TEST(parser,high,low) \
	do { if (__emit_code_test(parser,high,low) == -1) return -1; } while (0)

#define EMIT_CODE(parser,code) \
	do { if (__emit_code(parser,code) == -1) return -1; } while(0)

#define GET_CHAR(parser) \
	do { if (__get_char(parser) == -1) return -1; } while (0)
#define UNGET_CHAR(parser,c) \
	do { if (__unget_char(parser,c) == -1) return -1; } while (0)
#define GET_TOKEN(parser) \
	do { if (__get_token(parser) == -1) return -1; } while (0)
#define ADD_TOKEN_CHAR(parser,c) \
	do {  \
		if (xp_stx_token_addc (&(parser)->token, c) == -1) { \
			(parser)->error_code = XP_STX_PARSER_ERROR_MEMORY; \
			return -1; \
		} \
	} while (0)
	
const xp_char_t* xp_stx_parser_error_string (xp_stx_parser_t* parser)
{
	static const xp_char_t* msg[] =
	{
		XP_TEXT("no error"),

		XP_TEXT("input fucntion not ready"),
		XP_TEXT("input function error"),
		XP_TEXT("out of memory"),

		XP_TEXT("invalid character"),
		XP_TEXT("incomplete character literal"),
		XP_TEXT("incomplete string literal"),
		XP_TEXT("incomplete literal"),

		XP_TEXT("message selector"),
		XP_TEXT("invalid argument name"),
		XP_TEXT("too many arguments"),

		XP_TEXT("invalid primitive type"),
		XP_TEXT("primitive number expected"),
		XP_TEXT("primitive number out of range"),
		XP_TEXT("primitive not closed"),

		XP_TEXT("temporary list not closed"),
		XP_TEXT("too many temporaries"),
		XP_TEXT("cannot redefine pseudo variable"),
		XP_TEXT("invalid primary/expression-start"),

		XP_TEXT("no period at end of statement"),
		XP_TEXT("no closing parenthesis"),
		XP_TEXT("block argument name missing"),
		XP_TEXT("block argument list not closed"),
		XP_TEXT("block not closed"),

		XP_TEXT("undeclared name")
	};

	if (parser->error_code >= 0 && 
	    parser->error_code < xp_countof(msg)) return msg[parser->error_code];

	return XP_TEXT("unknown error");
}

static INLINE xp_bool_t __is_pseudo_variable (const xp_stx_token_t* token)
{
	return token->type == XP_STX_TOKEN_IDENT &&
		(xp_strcmp(token->name.buffer, XP_TEXT("self")) == 0 ||
		 xp_strcmp(token->name.buffer, XP_TEXT("super")) == 0 ||
		 xp_strcmp(token->name.buffer, XP_TEXT("nil")) == 0 ||
		 xp_strcmp(token->name.buffer, XP_TEXT("true")) == 0 ||
		 xp_strcmp(token->name.buffer, XP_TEXT("false")) == 0);
}

static INLINE xp_bool_t __is_vbar_token (const xp_stx_token_t* token)
{
	return 
		token->type == XP_STX_TOKEN_BINARY &&
		token->name.size == 1 &&
		token->name.buffer[0] == XP_CHAR('|');
}

static INLINE xp_bool_t __is_primitive_opener (const xp_stx_token_t* token)
{
	return 
		token->type == XP_STX_TOKEN_BINARY &&
		token->name.size == 1 &&
		token->name.buffer[0] == XP_CHAR('<');
}

static INLINE xp_bool_t __is_primitive_closer (const xp_stx_token_t* token)
{
	return 
		token->type == XP_STX_TOKEN_BINARY &&
		token->name.size == 1 &&
		token->name.buffer[0] == XP_CHAR('>');
}

static INLINE xp_bool_t __is_binary_char (xp_cint_t c)
{
	/*
	 * binaryCharacter ::=
	 * 	'!' | '%' | '&' | '*' | '+' | ',' | 
	 * 	'/' | '<' | '=' | '>' | '?' | '@' | 
	 * 	'\' | '~' | '|' | '-'
	 */

	return
		c == XP_CHAR('!') || c == XP_CHAR('%') ||
		c == XP_CHAR('&') || c == XP_CHAR('*') ||
		c == XP_CHAR('+') || c == XP_CHAR(',') ||
		c == XP_CHAR('/') || c == XP_CHAR('<') ||
		c == XP_CHAR('=') || c == XP_CHAR('>') ||
		c == XP_CHAR('?') || c == XP_CHAR('@') ||
		c == XP_CHAR('\\') || c == XP_CHAR('|') ||
		c == XP_CHAR('~') || c == XP_CHAR('-');
}

static INLINE xp_bool_t __is_closing_char (xp_cint_t c)
{
	return 
		c == XP_CHAR('.') || c == XP_CHAR(']') ||
		c == XP_CHAR(')') || c == XP_CHAR(';') ||
		c == XP_CHAR('\"') || c == XP_CHAR('\'');
}

static INLINE int __emit_code_test (
	xp_stx_parser_t* parser, const xp_char_t* high, const xp_char_t* low)
{
	xp_printf (XP_TEXT("CODE: %s %s\n"), high, low);
	return 0;
}

static INLINE int __emit_code (xp_stx_parser_t* parser, xp_byte_t code)
{
	if (xp_array_add_datum(&parser->bytecode, &code) == XP_NULL) {
		parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
		return -1;
	}

	return 0;
}

static INLINE int __emit_stack_code_positional (
	xp_stx_parser_t* parser, int opcode, int pos)
{
	static int mapping[] = {
		PUSH_RECEIVER_VARIABLE_EXTENDED,
		PUSH_TEMPORARY_LOCATION_EXTENDED,
		PUSH_LITERAL_CONSTANT_EXTENDED,
		PUSH_LITERAL_VARIABLE_EXTENDED,
		STORE_RECEIVER_VARIABLE,
		STORE_TEMPORARY_VARIABLE
	};
}

static INLINE int __emit_push_receiver_variable (xp_stx_parser_t* parser, int pos)
{
	if (pos > 0x0F) {
		EMIT_CODE (parser, PUSH_RECEIVER_VARIABLE | (pos & 0x0F));
	}
	else {
		EMIT_CODE (parser, PUSH_RECEIVER_VARIABLE_EXTENDED);
		EMIT_CODE (parser, pos & 0xFF)
	}

	return 0;
}

static INLINE int __emit_push_temporary_location (xp_stx_parser_t* parser, int pos)
{
	if (pos > 0x0F) {
		EMIT_CODE (parser, PUSH_TEMPORARY_LOCATION | (pos & 0x0F));
	}
	else {
		EMIT_CODE (parser, PUSH_RECEIVER_VARIABLE_EXTENDED);
		EMIT_CODE (parser, pos & 0xFF)
	}

	return 0;
}

/*
push_receiver_variable,
push_temporary_location,
push_literal_constant,
push_literal_variable,
store_receiver_variable,
store_temporary_location,
push_receiver,
push_true,
push_false,
push_nil,
push_minus_one,
push_zero,
push_one,
push_two,
return_receiver,
return_true,
return_false,
return_nil,
return_from_message,
return_from_block,
XP_NULL,
push_receiver_variable,
push_temporary_location,
push_literal_constant,
push_literal_variable,
*/

int xp_stx_parser_parse_method (
	xp_stx_parser_t* parser, xp_word_t method_class, void* input)
{
	int n;

	if (parser->input_func == XP_NULL) { 
		parser->error_code = XP_STX_PARSER_ERROR_INPUT_FUNC;
		return -1;
	}

	parser->method_class = method_class;
	if (__open_input(parser, input) == -1) return -1;
	n = __parse_method (parser, method_class, input);
	if (__close_input(parser) == -1) return -1;

	return n;
}

static int __parse_method (
	xp_stx_parser_t* parser, xp_word_t method_class, void* input)
{
	/*
	 * <method definition> ::= 
	 * 	<message pattern> [<temporaries>] [<primitive>] [<statements>]
	 */

	GET_CHAR (parser);
	GET_TOKEN (parser);

	xp_stx_name_clear (&parser->method_name);
	xp_array_clear (&parser->bytecode);

	while (parser->temporary_count > 0) {
		xp_free (parser->temporaries[--parser->temporary_count]);
	}
	parser->argument_count = 0;
	parser->literal_count = 0;

	if (__parse_message_pattern(parser) == -1) return -1;
	if (__parse_temporaries(parser) == -1) return -1;
	if (__parse_primitive(parser) == -1) return -1;
	if (__parse_statements(parser) == -1) return -1;
	if (__finish_method (parser) == -1) return -1;

	return 0;
}

static int __finish_method (xp_stx_parser_t* parser)
{
	xp_stx_t* stx = parser->stx;
	xp_stx_class_t* class_obj;
	xp_stx_method_t* method_obj;
	xp_word_t method, selector;

	xp_assert (parser->bytecode.size != 0);

	class_obj = (xp_stx_class_t*)
		XP_STX_OBJECT(stx, parser->method_class);

	if (class_obj->methods == stx->nil) {
		/* TODO: reconfigure method dictionary size */
		class_obj->methods = xp_stx_instantiate (
			stx, stx->class_dictionary, XP_NULL, XP_NULL, 64);
	}
	xp_assert (class_obj->methods != stx->nil);

	selector = xp_stx_new_symbolx (
		stx, parser->method_name.buffer, parser->method_name.size);

	method = xp_stx_instantiate(stx, stx->class_method, 
		XP_NULL, parser->literals, parser->literal_count);
	method_obj = (xp_stx_method_t*)XP_STX_OBJECT(stx, method);

	/* TODO: text saving must be optional */
	/*method_obj->text = xp_stx_instantiate (
		stx, stx->class_string, XP_NULL, 
		parser->text, xp_strlen(parser->text));
	*/
	method_obj->selector = selector;
	method_obj->bytecodes = xp_stx_instantiate (
		stx, stx->class_bytearray, XP_NULL, 
		parser->bytecode.buffer, parser->bytecode.size);
	/*
	method_obj->stack_size = XP_STX_TO_SMALLINT(100);
	method_obj->temporary_size = 
		XP_STX_TO_SMALLINT(parser->temporary_count);
	*/

	/* TODO: dictionaryAtPut (), remove hash.h above */
	xp_stx_hash_insert (
		stx, class_obj->methods, 
		xp_stx_hash_object(stx, selector),
		selector, method);
	return 0;
}

static int __parse_message_pattern (xp_stx_parser_t* parser)
{
	/* 
	 * <message pattern> ::= 
	 * 	<unary pattern> | <binary pattern> | <keyword pattern>
	 * <unary pattern> ::= unarySelector
	 * <binary pattern> ::= binarySelector <method argument>
	 * <keyword pattern> ::= (keyword  <method argument>)+
	 */
	int n;

	if (parser->token.type == XP_STX_TOKEN_IDENT) { 
		n = __parse_unary_pattern (parser);
	}
	else if (parser->token.type == XP_STX_TOKEN_BINARY) { 
		n = __parse_binary_pattern (parser);
	}
	else if (parser->token.type == XP_STX_TOKEN_KEYWORD) { 
		n = __parse_keyword_pattern (parser);
	}
	else {
		parser->error_code = XP_STX_PARSER_ERROR_MESSAGE_SELECTOR;
		n = -1;
	}

	parser->temporary_count = parser->argument_count;
	return n;
}

static int __parse_unary_pattern (xp_stx_parser_t* parser)
{
	/* TODO: check if the method name exists */

	if (xp_stx_name_adds(
		&parser->method_name, parser->token.name.buffer) == -1) {
		parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
		return -1;
	}

	GET_TOKEN (parser);
	return 0;
}

static int __parse_binary_pattern (xp_stx_parser_t* parser)
{
	/* TODO: check if the method name exists */

	if (xp_stx_name_adds(
		&parser->method_name, parser->token.name.buffer) == -1) {
		parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
		return -1;
	}

	GET_TOKEN (parser);
	if (parser->token.type != XP_STX_TOKEN_IDENT) {
		parser->error_code = XP_STX_PARSER_ERROR_ARGUMENT_NAME;
		return -1;
	}

	if (parser->argument_count >= xp_countof(parser->temporaries)) {
		parser->error_code = XP_STX_PARSER_ERROR_TOO_MANY_ARGUMENTS;
		return -1;
	}

	/* TODO: check for duplicate entries...in instvars */
	parser->temporaries[parser->argument_count] = 
		xp_stx_token_yield (&parser->token, 0);
	if (parser->temporaries[parser->argument_count] == XP_NULL) {
		parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
		return -1;
	}
	parser->argument_count++;

	GET_TOKEN (parser);
	return 0;
}

static int __parse_keyword_pattern (xp_stx_parser_t* parser)
{
	do {
		if (xp_stx_name_adds(
			&parser->method_name, parser->token.name.buffer) == -1) {
			parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
			return -1;
		}

		GET_TOKEN (parser);
		if (parser->token.type != XP_STX_TOKEN_IDENT) {
			parser->error_code = XP_STX_PARSER_ERROR_ARGUMENT_NAME;
			return -1;
		}

		if (__is_pseudo_variable(&parser->token)) {
			parser->error_code = XP_STX_PARSER_ERROR_PSEUDO_VARIABLE;
			return -1;
		}

		if (parser->argument_count >= xp_countof(parser->temporaries)) {
			parser->error_code = XP_STX_PARSER_ERROR_TOO_MANY_ARGUMENTS;
			return -1;
		}

		parser->temporaries[parser->argument_count] = 
			xp_stx_token_yield (&parser->token, 0);
		if (parser->temporaries[parser->argument_count] == XP_NULL) {
			parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
			return -1;
		}

/* TODO: check for duplicate entries...in instvars/arguments */
		parser->argument_count++;

		GET_TOKEN (parser);
	} while (parser->token.type == XP_STX_TOKEN_KEYWORD);

	/* TODO: check if the method name exists */
	/* if it exists, collapse arguments */
xp_printf (XP_TEXT("METHOD NAME ==> [%s]\n"), parser->method_name.buffer);

	return 0;
}

static int __parse_temporaries (xp_stx_parser_t* parser)
{
	/* 
	 * <temporaries> ::= '|' <temporary variable list> '|'
	 * <temporary variable list> ::= identifier*
	 */

	if (!__is_vbar_token(&parser->token)) return 0;

	GET_TOKEN (parser);
	while (parser->token.type == XP_STX_TOKEN_IDENT) {
		if (parser->temporary_count >= xp_countof(parser->temporaries)) {
			parser->error_code = XP_STX_PARSER_ERROR_TOO_MANY_TEMPORARIES;
			return -1;
		}

		if (__is_pseudo_variable(&parser->token)) {
			parser->error_code = XP_STX_PARSER_ERROR_PSEUDO_VARIABLE;
			return -1;
		}

		parser->temporaries[parser->temporary_count] = 
			xp_stx_token_yield (&parser->token, 0);
		if (parser->temporaries[parser->temporary_count] == XP_NULL) {
			parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
			return -1;
		}

/* TODO: check for duplicate entries...in instvars/arguments/temporaries */
		parser->temporary_count++;

		GET_TOKEN (parser);
	}
	if (!__is_vbar_token(&parser->token)) {
		parser->error_code = XP_STX_PARSER_ERROR_TEMPORARIES_NOT_CLOSED;
		return -1;
	}

	GET_TOKEN (parser);
	return 0;
}

static int __parse_primitive (xp_stx_parser_t* parser)
{
	/* 
	 * <primitive> ::= '<' 'primitive:' number '>'
	 */

	int prim_no;

	if (!__is_primitive_opener(&parser->token)) return 0;
	GET_TOKEN (parser);

	if (parser->token.type != XP_STX_TOKEN_KEYWORD ||
	    xp_strcmp (parser->token.name.buffer, XP_TEXT("primitive:")) != 0) {
		parser->error_code = XP_STX_PARSER_ERROR_PRIMITIVE_KEYWORD;
		return -1;
	}

	GET_TOKEN (parser); /* TODO: only integer */
	if (parser->token.type != XP_STX_TOKEN_NUMLIT) {
		parser->error_code = XP_STX_PARSER_ERROR_PRIMITIVE_NUMBER;
		return -1;
	}

/*TODO: more checks the validity of the primitive number */
	if (!xp_stristype(parser->token.name.buffer, xp_isdigit)) {
		parser->error_code = XP_STX_PARSER_ERROR_PRIMITIVE_NUMBER;
		return -1;
	}

	XP_STRTOI (prim_no, parser->token.name.buffer, XP_NULL, 10);
	if (prim_no < 0 || prim_no > 0x0FFF) {
		parser->error_code = XP_STX_PARSER_ERROR_PRIMITIVE_NUMBER_RANGE;
		return -1;
	}

	if (prim_no <= 0x0F)  {
		EMIT_CODE_TEST (parser, XP_TEXT("DO_PRIMITIVE"), parser->token.name.buffer);
		EMIT_CODE (parser, (DO_PRIMITIVE << 4) | prim_no);
	}
	else {
		EMIT_CODE_TEST (parser, XP_TEXT("DO_PRIMITIVE_EXTENDED"), parser->token.name.buffer);
		EMIT_CODE (parser, (DO_PRIMITIVE_EXTENDED << 4) | (prim_no & 0x0F));
		EMIT_CODE (parser, prim_no >> 4);
	}

	GET_TOKEN (parser);
	if (!__is_primitive_closer(&parser->token)) {
		parser->error_code = XP_STX_PARSER_ERROR_PRIMITIVE_NOT_CLOSED;
		return -1;
	}

	GET_TOKEN (parser);
	return 0;
}

static int __parse_statements (xp_stx_parser_t* parser)
{
	/*
	 * <statements> ::= (ORIGINAL->maybe wrong)
	 * 	(<return statement> ['.'] ) |
	 * 	(<expression> ['.' [<statements>]])
	 * <statements> ::= (REVISED->correct?)
	 * 	<statement> ['. [<statements>]]
	 */

	while (parser->token.type != XP_STX_TOKEN_END) {
		if (__parse_statement (parser) == -1) return -1;

		if (parser->token.type == XP_STX_TOKEN_PERIOD) {
			GET_TOKEN (parser);
			continue;
		}

		if (parser->token.type != XP_STX_TOKEN_END) {
			parser->error_code = XP_STX_PARSER_ERROR_NO_PERIOD;
			return -1;
		}
	}

	return 0;
}

static int __parse_block_statements (xp_stx_parser_t* parser)
{
	while (parser->token.type != XP_STX_TOKEN_RBRACKET && 
	       parser->token.type != XP_STX_TOKEN_END) {

		if (__parse_statement(parser) == -1) return -1;
		if (parser->token.type != XP_STX_TOKEN_PERIOD) break;
		GET_TOKEN (parser);
	}

	return 0;
}

static int __parse_statement (xp_stx_parser_t* parser)
{
	/* 
	 * <statement> ::= <return statement> | <expression>
	 * <return statement> ::= returnOperator <expression> 
	 * returnOperator ::= '^'
	 */

	if (parser->token.type == XP_STX_TOKEN_RETURN) {
		GET_TOKEN (parser);
		if (__parse_expression(parser) == -1) return -1;

		/* TODO */
		if (RETURN_FROM_MESSAGE <= 0x0F)  {
			EMIT_CODE_TEST (parser, XP_TEXT("DO_SPECIAL"), XP_TEXT("RETURN_FROM_MESSAGE"));
			EMIT_CODE (parser, (DO_SPECIAL << 4) | RETURN_FROM_MESSAGE);
		}
		else {
			EMIT_CODE_TEST (parser, XP_TEXT("DO_SPECIAL_EXTENDED"), XP_TEXT("RETURN_FROM_MESSAGE"));
			EMIT_CODE (parser, (DO_SPECIAL_EXTENDED << 4) | (RETURN_FROM_MESSAGE & 0x0F));
			EMIT_CODE (parser, RETURN_FROM_MESSAGE >> 4);
		}
	}
	else {
		if (__parse_expression(parser) == -1) return -1;
	}

	return 0;
}

static int __parse_expression (xp_stx_parser_t* parser)
{
	/*
	 * <expression> ::= <assignment> | <basic expression>
	 * <assignment> ::= <assignment target> assignmentOperator <expression>
	 * <basic expression> ::= <primary> [<messages> <cascaded messages>]
	 * <assignment target> ::= identifier
	 * assignmentOperator ::=  ':='
	 */

	if (parser->token.type == XP_STX_TOKEN_IDENT) {
		xp_char_t* ident = xp_stx_token_yield (&parser->token, 0);
		if (ident == XP_NULL) {
			parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
			return -1;
		}

		GET_TOKEN (parser);
		if (parser->token.type == XP_STX_TOKEN_ASSIGN) {
			GET_TOKEN (parser);
			if (__parse_assignment(parser, ident) == -1) {
				xp_free (ident);
				return -1;
			}
		}
		else {
			if (__parse_basic_expression(parser, ident) == -1) {
				xp_free (ident);
				return -1;
			}
		}

		xp_free (ident);
	}
	else {
		if (__parse_basic_expression(parser, XP_NULL) == -1) return -1;
	}


	return 0;
}

static int __parse_basic_expression (
	xp_stx_parser_t* parser, const xp_char_t* ident)
{
	/*
	 * <basic expression> ::= <primary> [<messages> <cascaded messages>]
	 */

	if (__parse_primary(parser, ident) == -1) return -1;
	if (parser->token.type != XP_STX_TOKEN_END &&
	    parser->token.type != XP_STX_TOKEN_PERIOD) {
		if (__parse_message_continuation(parser) == -1) return -1;
	}
	return 0;
}

static int __parse_assignment (
	xp_stx_parser_t* parser, const xp_char_t* target)
{
	/*
	 * <assignment> ::= <assignment target> assignmentOperator <expression>
	 */

	xp_word_t i;
	xp_stx_t* stx = parser->stx;

	for (i = parser->argument_count; i < parser->temporary_count; i++) {
		if (xp_strcmp (target, parser->temporaries[i]) == 0) {
xp_char_t buf[100];
			if (__parse_expression(parser) == -1) return -1;
xp_sprintf (buf, xp_countof(buf), XP_TEXT("%d"), i);
			if (i <= 0x0F)  {
				EMIT_CODE_TEST (parser, XP_TEXT("STORE_TEMPORARY"), buf);
				EMIT_CODE (parser, (STORE_TEMPORARY << 4) | i);
			}
			else {
				EMIT_CODE_TEST (parser, XP_TEXT("STORE_TEMPORARY_EXTENDED"), buf);
				EMIT_CODE (parser, (STORE_TEMPORARY_EXTENDED << 4) | (i & 0x0F));
				EMIT_CODE (parser, i >> 4);
			}

			return 0;
		}
	}

	if (xp_stx_get_instance_variable_index (
		stx, parser->method_class, target, &i) == 0) {
xp_char_t buf[100];
		if (__parse_expression(parser) == -1) return -1;
xp_sprintf (buf, xp_countof(buf), XP_TEXT("%d"), i);

		/* TODO */
		if (i <= 0x0F)  {
			EMIT_CODE_TEST (parser, XP_TEXT("STORE_VARIABLE"), buf);
			EMIT_CODE (parser, (STORE_VARIABLE << 4) | i);
		}
		else {
			EMIT_CODE_TEST (parser, XP_TEXT("STORE_VARIABLE_EXTENDED"), buf);
			EMIT_CODE (parser, (STORE_VARIABLE_EXTENDED << 4) | (i & 0x0F));
			EMIT_CODE (parser, i >> 4);
		}

		return 0;
	}

	if (xp_stx_lookup_class_variable (
		stx, parser->method_class, target) != stx->nil) {
		if (__parse_expression(parser) == -1) return -1;

		/* TODO */
		EMIT_CODE_TEST (parser, XP_TEXT("ASSIGN_CLASSVAR #"), target);
		return 0;
	}

	/* TODO: IMPLEMENT POOL DICTIONARIES */

	/* TODO: IMPLEMENT GLOBLAS, but i don't like this idea */

	parser->error_code = XP_STX_PARSER_ERROR_UNDECLARED_NAME;
	return -1;
}

static int __parse_primary (xp_stx_parser_t* parser, const xp_char_t* ident)
{
	/*
	 * <primary> ::=
	 * 	identifier | <literal> | 
	 * 	<block constructor> | ( '('<expression>')' )
	 */

	if (ident == XP_NULL) {
		if (parser->token.type == XP_STX_TOKEN_IDENT) {
			if (__parse_primary_ident (parser, parser->token.buffer) == -1) return -1;
			GET_TOKEN (parser);
		}
		else if (parser->token.type == XP_STX_TOKEN_CHARLIT) {
			EMIT_CODE_TEST (parser, XP_TEXT("PushLiteral(CHAR)"), parser->token.name.buffer);
			GET_TOKEN (parser);
		}
		else if (parser->token.type == XP_STX_TOKEN_STRLIT) {
			EMIT_CODE_TEST (parser, XP_TEXT("PushLiteral(STR)"), parser->token.name.buffer);
			GET_TOKEN (parser);
		}
		else if (parser->token.type == XP_STX_TOKEN_NUMLIT) {
			EMIT_CODE_TEST (parser, XP_TEXT("PushLiteral(NUM)"), parser->token.name.buffer);
			GET_TOKEN (parser);
		}
		else if (parser->token.type == XP_STX_TOKEN_SYMLIT) {
			EMIT_CODE_TEST (parser, XP_TEXT("PushLiteral(SYM)"), parser->token.name.buffer);
			GET_TOKEN (parser);
		}
		else if (parser->token.type == XP_STX_TOKEN_LBRACKET) {
			GET_TOKEN (parser);
			if (__parse_block_constructor(parser) == -1) return -1;
		}
		else if (parser->token.type == XP_STX_TOKEN_APAREN) {
			/* TODO: array literal */
		}
		else if (parser->token.type == XP_STX_TOKEN_LPAREN) {
			GET_TOKEN (parser);
			if (__parse_expression(parser) == -1) return -1;
			if (parser->token.type != XP_STX_TOKEN_RPAREN) {
				parser->error_code = XP_STX_PARSER_ERROR_NO_RPAREN;
				return -1;
			}
			GET_TOKEN (parser);
		}
		else {
			parser->error_code = XP_STX_PARSER_ERROR_PRIMARY;
			return -1;
		}
	}
	else {
		if (__parse_primary_ident (parser, parser->token.buffer) == -1) return -1;
	}

	return 0;
}

static int __parse_primary_ident (xp_stx_parser_t* parser, const xp_char_t* ident)
{
	xp_word_t i;

	/* Refer to __parse_assignment for identifier lookup */

	for (i = 0; i < parser->temporary_count; i++) {
		if (xp_strcmp (target, parser->temporaries[i]) == 0) {
			EMIT_CODE_1 (parser, PUSH_TEMPORARY_VARIABLE, i);
			return 0;
		}
	}

	if (xp_stx_get_instance_variable_index (
		stx, parser->method_class, target, &i) == 0) {
		EMIT_CODE_1 (parser, PUSH_RECEIVER_VARIABLE, i);
		return 0;
	}	

	if (xp_stx_lookup_class_variable (
		stx, parser->method_class, target) != stx->nil) {
		//PUSH_CLASS_VARIABLE
		return 0;
	}

	/* TODO: IMPLEMENT POOL DICTIONARIES */

	/* TODO: IMPLEMENT GLOBLAS, but i don't like this idea */

	parser->error_code = XP_STX_PARSER_ERROR_UNDECLARED_NAME;
	return -1;
}

static int __parse_block_constructor (xp_stx_parser_t* parser)
{
	/*
	 * <block constructor> ::= '[' <block body> ']'
	 * <block body> ::= [<block argument>* '|']
	 * 	[<temporaries>] [<statements>]
	 * <block argument> ::= ':'  identifier
	 */

	if (parser->token.type == XP_STX_TOKEN_COLON) {
		do {
			GET_TOKEN (parser);

			if (parser->token.type != XP_STX_TOKEN_IDENT) {
				parser->error_code = XP_STX_PARSER_ERROR_BLOCK_ARGUMENT_NAME;
				return -1;
			}

			/* TODO : store block arguments */
			GET_TOKEN (parser);
		} while (parser->token.type == XP_STX_TOKEN_COLON);
			
		if (!__is_vbar_token(&parser->token)) {
			parser->error_code = XP_STX_PARSER_ERROR_BLOCK_ARGUMENT_LIST;
			return -1;
		}

		GET_TOKEN (parser);
	}

	/* TODO: create a block closure */
	if (__parse_temporaries(parser) == -1) return -1;
	if (__parse_block_statements(parser) == -1) return -1;

	if (parser->token.type != XP_STX_TOKEN_RBRACKET) {
		parser->error_code = XP_STX_PARSER_ERROR_BLOCK_NOT_CLOSED;
		return -1;
	}

	GET_TOKEN (parser);

	/* TODO: do special treatment for block closures */

	return 0;
}

static int __parse_message_continuation (xp_stx_parser_t* parser)
{
	/*
	 * <messages> ::=
	 * 	(<unary message>+ <binary message>* [<keyword message>] ) |
	 * 	(<binary message>+ [<keyword message>] ) |
	 * 	<keyword message>
	 * <cascaded messages> ::= (';' <messages>)*
	 */
	
	if (__parse_keyword_message(parser) == -1) return -1;

	while (parser->token.type == XP_STX_TOKEN_SEMICOLON) {
		EMIT_CODE_TEST (parser, XP_TEXT("DoSpecial(DUP_RECEIVER(CASCADE))"), XP_TEXT(""));
		GET_TOKEN (parser);

		if (__parse_keyword_message (parser) == -1) return -1;
		EMIT_CODE_TEST (parser, XP_TEXT("DoSpecial(POP_TOP)"), XP_TEXT(""));
	}

	return 0;
}

static int __parse_keyword_message (xp_stx_parser_t* parser)
{
	/*
	 * <keyword message> ::= (keyword <keyword argument> )+
	 * <keyword argument> ::= <primary> <unary message>* <binary message>*
	 */

	xp_stx_name_t name;

	if (__parse_binary_message (parser) == -1) return -1;

	if (xp_stx_name_open(&name, 0) == XP_NULL) {
		parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
		return -1;
	}
	
	while (parser->token.type == XP_STX_TOKEN_KEYWORD) {
		if (xp_stx_name_adds(&name, parser->token.name.buffer) == -1) {
			parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
			xp_stx_name_close (&name);
			return -1;
		}

		GET_TOKEN (parser);
		if (__parse_primary (parser, XP_NULL) == -1) {
			xp_stx_name_close (&name);
			return -1;
		}

		if (__parse_binary_message (parser) == -1) {
			xp_stx_name_close (&name);
			return -1;
		}
	}

	EMIT_CODE_TEST (parser, XP_TEXT("SendKeyword"), name.buffer);
	xp_stx_name_close (&name);

	return 0;
}

static int __parse_binary_message (xp_stx_parser_t* parser)
{
	/*
	 * <binary message> ::= binarySelector <binary argument>
	 * <binary argument> ::= <primary> <unary message>*
	 */

	if (__parse_unary_message (parser) == -1) return -1;

	while (parser->token.type == XP_STX_TOKEN_BINARY) {
		xp_char_t* op = xp_stx_token_yield (&parser->token, 0);
		if (op == XP_NULL) {
			parser->error_code = XP_STX_PARSER_ERROR_MEMORY;
			return -1;
		}

		GET_TOKEN (parser);
		if (__parse_primary (parser, XP_NULL) == -1) {
			xp_free (op);
			return -1;
		}

		if (__parse_unary_message (parser) == -1) {
			xp_free (op);
			return -1;
		}

		EMIT_CODE_TEST (parser, XP_TEXT("SendBinary"), op);
		xp_free (op);
	}

	return 0;
}

static int __parse_unary_message (xp_stx_parser_t* parser)
{
	/* <unary message> ::= unarySelector */

	while (parser->token.type == XP_STX_TOKEN_IDENT) {
		EMIT_CODE_TEST (parser, XP_TEXT("SendUnary"), parser->token.name.buffer);		
		GET_TOKEN (parser);
	}

	return 0;
}

static int __get_token (xp_stx_parser_t* parser)
{
	xp_cint_t c;

	do {
		if (__skip_spaces(parser) == -1) return -1;
		if (parser->curc == XP_CHAR('"')) {
			GET_CHAR (parser);
			if (__skip_comment(parser) == -1) return -1;
		}
		else break;
	} while (1);

	c = parser->curc;
	xp_stx_token_clear (&parser->token);

	if (c == XP_CHAR_EOF) {
		parser->token.type = XP_STX_TOKEN_END;
	}
	else if (xp_isalpha(c)) {
		if (__get_ident(parser) == -1) return -1;
	}
	else if (xp_isdigit(c)) {
		if (__get_numlit(parser, xp_false) == -1) return -1;
	}
	else if (c == XP_CHAR('$')) {
		GET_CHAR (parser);
		if (__get_charlit(parser) == -1) return -1;
	}
	else if (c == XP_CHAR('\'')) {
		GET_CHAR (parser);
		if (__get_strlit(parser) == -1) return -1;
	}
	else if (c == XP_CHAR(':')) {
		parser->token.type = XP_STX_TOKEN_COLON;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);

		c = parser->curc;
		if (c == XP_CHAR('=')) {
			parser->token.type = XP_STX_TOKEN_ASSIGN;
			ADD_TOKEN_CHAR(parser, c);
			GET_CHAR (parser);
		}
	}
	else if (c == XP_CHAR('^')) {
		parser->token.type = XP_STX_TOKEN_RETURN;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_CHAR('[')) {
		parser->token.type = XP_STX_TOKEN_LBRACKET;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_CHAR(']')) {
		parser->token.type = XP_STX_TOKEN_RBRACKET;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_CHAR('(')) {
		parser->token.type = XP_STX_TOKEN_LPAREN;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_CHAR(')')) {
		parser->token.type = XP_STX_TOKEN_RPAREN;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_CHAR('#')) {
		/*ADD_TOKEN_CHAR(parser, c);*/
		GET_CHAR (parser);

		c = parser->curc;
		if (c == XP_CHAR_EOF) {
			parser->error_code = XP_STX_PARSER_ERROR_LITERAL;
			return -1;
		}
		else if (c == XP_CHAR('(')) {
			ADD_TOKEN_CHAR(parser, c);
			parser->token.type = XP_STX_TOKEN_APAREN;
			GET_CHAR (parser);
		}
		else if (c == XP_CHAR('\'')) {
			GET_CHAR (parser);
			if (__get_strlit(parser) == -1) return -1;
			parser->token.type = XP_STX_TOKEN_SYMLIT;
		}
		else if (!__is_closing_char(c) && !xp_isspace(c)) {
			do {
				ADD_TOKEN_CHAR(parser, c);
				GET_CHAR (parser);
				c = parser->curc;
			} while (!__is_closing_char(c) && !xp_isspace(c));

			parser->token.type = XP_STX_TOKEN_SYMLIT;
		}
		else {
			parser->error_code = XP_STX_PARSER_ERROR_LITERAL;
			return -1;
		}
	}
	else if (c == XP_CHAR('.')) {
		parser->token.type = XP_STX_TOKEN_PERIOD;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_CHAR(';')) {
		parser->token.type = XP_STX_TOKEN_SEMICOLON;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (__is_binary_char(c)) {
		if (__get_binary(parser) == -1) return -1;
	}
	else {
		parser->error_code = XP_STX_PARSER_ERROR_CHAR;
		return -1;
	}

//xp_printf (XP_TEXT("TOKEN: %s\n"), parser->token.name.buffer);
	return 0;
}

static int __get_ident (xp_stx_parser_t* parser)
{
	/*
	 * identifier ::= letter (letter | digit)*
	 * keyword ::= identifier ':'
	 */

	xp_cint_t c = parser->curc;
	parser->token.type = XP_STX_TOKEN_IDENT;

	do {
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
		c = parser->curc;
	} while (xp_isalnum(c));

	if (c == XP_CHAR(':')) {
		ADD_TOKEN_CHAR (parser, c);
		parser->token.type = XP_STX_TOKEN_KEYWORD;
		GET_CHAR (parser);
	}

	return 0;
}

static int __get_numlit (xp_stx_parser_t* parser, xp_bool_t negated)
{
	/* 
	 * <number literal> ::= ['-'] <number>
	 * <number> ::= integer | float | scaledDecimal
	 * integer ::= decimalInteger  | radixInteger
	 * decimalInteger ::= digits
	 * digits ::= digit+
	 * radixInteger ::= radixSpecifier  'r' radixDigits
	 * radixSpecifier := digits
	 * radixDigits ::= (digit | uppercaseAlphabetic)+
	 * float ::=  mantissa [exponentLetter exponent]
	 * mantissa ::= digits'.' digits
	 * exponent ::= ['-']decimalInteger
	 * exponentLetter ::= 'e' | 'd' | 'q'
	 * scaledDecimal ::= scaledMantissa 's' [fractionalDigits]
	 * scaledMantissa ::= decimalInteger | mantissa
	 * fractionalDigits ::= decimalInteger
	 */

	xp_cint_t c = parser->curc;
	parser->token.type = XP_STX_TOKEN_NUMLIT;

	do {
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
		c = parser->curc;
	} while (xp_isalnum(c));

	/* TODO; more */
	return 0;
}

static int __get_charlit (xp_stx_parser_t* parser)
{
	/* 
	 * character_literal ::= '$' character
	 * character ::= "Any character in the implementation-defined character set"
	 */

	xp_cint_t c = parser->curc; /* even a new-line or white space would be taken */
	if (c == XP_CHAR_EOF) {
		parser->error_code = XP_STX_PARSER_ERROR_CHARLIT;
		return -1;
	}	

	parser->token.type = XP_STX_TOKEN_CHARLIT;
	ADD_TOKEN_CHAR(parser, c);
	GET_CHAR (parser);
	return 0;
}

static int __get_strlit (xp_stx_parser_t* parser)
{
	/* 
	 * string_literal ::= stringDelimiter stringBody stringDelimiter
	 * stringBody ::= (nonStringDelimiter | (stringDelimiter stringDelimiter)*)
	 * stringDelimiter ::= '''    "a single quote"
	 */

	/* TODO: C-like string */

	xp_cint_t c = parser->curc;
	parser->token.type = XP_STX_TOKEN_STRLIT;

	do {
		do {
			ADD_TOKEN_CHAR (parser, c);
			GET_CHAR (parser);
			c = parser->curc;

			if (c == XP_CHAR_EOF) {
				parser->error_code = XP_STX_PARSER_ERROR_STRLIT;
				return -1;
			}
		} while (c != XP_CHAR('\''));

		GET_CHAR (parser);
		c = parser->curc;
	} while (c == XP_CHAR('\''));

	return 0;
}

static int __get_binary (xp_stx_parser_t* parser)
{
	/* 
	 * binarySelector ::= binaryCharacter+
	 */

	xp_cint_t c = parser->curc;
	ADD_TOKEN_CHAR (parser, c);

	if (c == XP_CHAR('-')) {
		GET_CHAR (parser);
		c = parser->curc;
		if (xp_isdigit(c)) return __get_numlit(parser,xp_true);
	}
	else {
		GET_CHAR (parser);
		c = parser->curc;
	}

	/* up to 2 characters only */
	if (__is_binary_char(c)) {
		ADD_TOKEN_CHAR (parser, c);
		GET_CHAR (parser);
		c = parser->curc;
	}

	/* or up to any occurrences */
	/*
	while (__is_binary_char(c)) {
		ADD_TOKEN_CHAR (parser, c);
		GET_CHAR (parser);
		c = parser->curc;
	}
	*/

	parser->token.type = XP_STX_TOKEN_BINARY;
	return 0;
}

static int __skip_spaces (xp_stx_parser_t* parser)
{
	while (xp_isspace(parser->curc)) GET_CHAR (parser);
	return 0;
}

static int __skip_comment (xp_stx_parser_t* parser)
{
	while (parser->curc != XP_CHAR('"')) GET_CHAR (parser);
	GET_CHAR (parser);
	return 0;
}

static int __get_char (xp_stx_parser_t* parser)
{
	xp_cint_t c;

	if (parser->ungotc_count > 0) {
		parser->curc = parser->ungotc[parser->ungotc_count--];
	}
	else {
		if (parser->input_func (
			XP_STX_PARSER_INPUT_CONSUME, 
			parser->input_owner, (void*)&c) == -1) {
			parser->error_code = XP_STX_PARSER_ERROR_INPUT;
			return -1;
		}
		parser->curc = c;
	}
	return 0;
}

static int __unget_char (xp_stx_parser_t* parser, xp_cint_t c)
{
	if (parser->ungotc_count >= xp_countof(parser->ungotc)) return -1;
	parser->ungotc[parser->ungotc_count++] = c;
	return 0;
}

static int __open_input (xp_stx_parser_t* parser, void* input)
{
	if (parser->input_func(
		XP_STX_PARSER_INPUT_OPEN, 
		(void*)&parser->input_owner, input) == -1) {
		parser->error_code = XP_STX_PARSER_ERROR_INPUT;
		return -1;
	}

	parser->error_code = XP_STX_PARSER_ERROR_NONE;
	parser->curc = XP_CHAR_EOF;
	parser->ungotc_count = 0;
	return 0;
}

static int __close_input (xp_stx_parser_t* parser)
{
	if (parser->input_func(
		XP_STX_PARSER_INPUT_CLOSE, 
		parser->input_owner, XP_NULL) == -1) {
		parser->error_code = XP_STX_PARSER_ERROR_INPUT;
		return -1;
	}

	return 0;
}

