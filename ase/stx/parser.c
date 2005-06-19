/*
 * $Id: parser.c,v 1.36 2005-06-19 16:16:33 bacon Exp $
 */

#include <xp/stx/parser.h>
#include <xp/stx/misc.h>

static int __parse_method (
	xp_stx_parser_t* parser, 
	xp_word_t method_class, void* input);

static int __parse_message_pattern (xp_stx_parser_t* parser);
static int __parse_unary_pattern (xp_stx_parser_t* parser);
static int __parse_binary_pattern (xp_stx_parser_t* parser);
static int __parse_keyword_pattern (xp_stx_parser_t* parser);

static int __parse_temporaries (xp_stx_parser_t* parser);
static int __parse_statements (xp_stx_parser_t* parser);
static int __parse_statements_2 (xp_stx_parser_t* parser);
static int __parse_expression (xp_stx_parser_t* parser);

static int __parse_assignment (
	xp_stx_parser_t* parser, const xp_char_t* target);
static int __parse_message_continuation (xp_stx_parser_t* parser);
static int __parse_keyword_continuation (xp_stx_parser_t* parser);
static int __parse_binary_continuation (xp_stx_parser_t* parser);
static int __parse_unary_continuation (xp_stx_parser_t* parser);

static int __emit_code (
	xp_stx_parser_t* parser, const xp_char_t* high, int low); 

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
		&parser->byte_code, 256, 
		xp_sizeof(xp_byte_t), XP_NULL) == XP_NULL) {
		xp_stx_name_close (&parser->method_name);
		xp_stx_token_close (&parser->token);
		if (parser->__malloced) xp_free (parser);
		return XP_NULL;
	}

	parser->stx = stx;
	parser->error_code = XP_STX_PARSER_ERROR_NONE;

	parser->argument_count = 0;
	parser->temporary_count = 0;

	parser->curc = XP_CHAR_EOF;
	parser->ungotc_count = 0;

	parser->input_owner = XP_NULL;
	parser->input_func = XP_NULL;
	return parser;
}

void xp_stx_parser_close (xp_stx_parser_t* parser)
{
	while (parser->argument_count > 0) {
		xp_free (parser->argument[--parser->argument_count]);
	}
	while (parser->temporary_count > 0) {
		xp_free (parser->temporary[--parser->temporary_count]);
	}

	xp_array_close (&parser->byte_code);
	xp_stx_name_close (&parser->method_name);
	xp_stx_token_close (&parser->token);

	if (parser->__malloced) xp_free (parser);
}

#define EMIT_CODE(parser,high,low) \
	do { if (__emit_code(parser,high,low) == -1) return -1; } while (0)

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

		XP_TEXT("message selector"),
		XP_TEXT("invalid argument name"),
		XP_TEXT("too many arguments"),
		XP_TEXT("temporary list not closed"),
		XP_TEXT("too many temporaries"),
		XP_TEXT("cannot redefine pseudo variable"),
		XP_TEXT("invalid expression start"),

		XP_TEXT("no period at end of statement")
	};

	if (parser->error_code >= 0 && 
	    parser->error_code < xp_countof(msg)) return msg[parser->error_code];

	return XP_TEXT("unknown error");
}

static inline xp_bool_t __is_pseudo_variable (const xp_stx_token_t* token)
{
	return token->type == XP_STX_TOKEN_IDENT &&
		(xp_strcmp(token->name.buffer, XP_TEXT("self")) == 0 ||
		 xp_strcmp(token->name.buffer, XP_TEXT("super")) == 0 ||
		 xp_strcmp(token->name.buffer, XP_TEXT("nil")) == 0 ||
		 xp_strcmp(token->name.buffer, XP_TEXT("true")) == 0 ||
		 xp_strcmp(token->name.buffer, XP_TEXT("false")) == 0);
}

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
	 * 	<message pattern> [<temporaries> ] [<statements>]
	 */

	GET_CHAR (parser);
	GET_TOKEN (parser);

	xp_stx_name_clear (&parser->method_name);

	while (parser->argument_count > 0) {
		xp_free (parser->argument[--parser->argument_count]);
	}
	while (parser->temporary_count > 0) {
		xp_free (parser->temporary[--parser->temporary_count]);
	}

	if (__parse_message_pattern (parser) == -1) return -1;
	if (__parse_temporaries (parser) == -1) return -1;
	if (__parse_statements (parser) == -1) return -1;
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

	if (parser->argument_count >= xp_countof(parser->argument)) {
		parser->error_code = XP_STX_PARSER_ERROR_TOO_MANY_ARGUMENTS;
		return -1;
	}

	/* TODO: check for duplicate entries...in instvars */
	parser->argument[parser->argument_count] = 
		xp_stx_token_yield (&parser->token, 0);
	if (parser->argument[parser->argument_count] == XP_NULL) {
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

		if (parser->argument_count >= xp_countof(parser->argument)) {
			parser->error_code = XP_STX_PARSER_ERROR_TOO_MANY_ARGUMENTS;
			return -1;
		}

		parser->argument[parser->argument_count] = 
			xp_stx_token_yield (&parser->token, 0);
		if (parser->argument[parser->argument_count] == XP_NULL) {
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

static inline xp_bool_t __is_vbar_token (const xp_stx_token_t* token)
{
	return 
		token->type == XP_STX_TOKEN_BINARY &&
		token->name.size == 1 &&
		token->name.buffer[0] == XP_CHAR('|');
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
		if (parser->temporary_count >= xp_countof(parser->temporary)) {
			parser->error_code = XP_STX_PARSER_ERROR_TOO_MANY_TEMPORARIES;
			return -1;
		}

		if (__is_pseudo_variable(&parser->token)) {
			parser->error_code = XP_STX_PARSER_ERROR_PSEUDO_VARIABLE;
			return -1;
		}

		parser->temporary[parser->temporary_count] = 
			xp_stx_token_yield (&parser->token, 0);
		if (parser->temporary[parser->temporary_count] == XP_NULL) {
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

static int __parse_statements (xp_stx_parser_t* parser)
{
	/*
	 * <statements> ::= (TODO: this definition seems to be wrong)
	 * 	(<return statement> ['.'] ) |
	 * 	(<expression> ['.' [<statements>]])
	 * <return statement> ::= returnOperator  <expression>
	 * returnOperator ::= '^'
	 */

	while (parser->token.type != XP_STX_TOKEN_END) {
		if (__parse_statements_2 (parser) == -1) return -1;

		if (parser->token.type == XP_STX_TOKEN_PERIOD) {
			GET_TOKEN (parser);
		}
		else if (parser->token.type != XP_STX_TOKEN_END) {
			parser->error_code = XP_STX_PARSER_ERROR_NO_PERIOD;
			return -1;
		}
	}

	return 0;
}

static int __parse_statements_2 (xp_stx_parser_t* parser)
{
	if (parser->token.type == XP_STX_TOKEN_RETURN) {
		GET_TOKEN (parser);
		if (__parse_expression (parser) == -1) return -1;
		/* TODO */
	}
	else {
		if (__parse_expression (parser) == -1) return -1;
	}

	return 0;
}

static int __parse_expression (xp_stx_parser_t* parser)
{
	/*
	 * <expression> ::= <assignment> | <basic expression>
	 * <assignment> ::= <assignment target> assignmentOperator  <expression>
	 * <basic expression> ::= <primary> [<messages> <cascaded messages>]
	 * <assignment target> := identifier
	 * assignmentOperator ::=  ':='
	 * <primary> ::=
	 * 	identifier | <literal> | 
	 * 	<block constructor> | ( '('<expression>')' )
	 */

	if (parser->token.type == XP_STX_TOKEN_IDENT) {
		xp_char_t* ident = xp_stx_token_yield (
			&parser->token, parser->token.name.capacity);
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
			if (__parse_message_continuation(parser) == -1) {
				return -1;
				xp_free (ident);
			}
		}

		xp_free (ident);
	}
	else if (parser->token.type == XP_STX_TOKEN_CHARLIT) {
		EMIT_CODE (parser, XP_TEXT("PushLiteral(CHAR)"), 0xFFFF);
		GET_TOKEN (parser);
		if (__parse_message_continuation(parser) == -1) return -1;
	}
	else if (parser->token.type == XP_STX_TOKEN_STRLIT) {
		EMIT_CODE (parser, XP_TEXT("PushLiteral(STR)"), 0xFFFF);
		GET_TOKEN (parser);
		if (__parse_message_continuation(parser) == -1) return -1;
	}
	else if (parser->token.type == XP_STX_TOKEN_NUMLIT) {
		EMIT_CODE (parser, XP_TEXT("PushLiteral(NUM)"), 0xFFFF);
		GET_TOKEN (parser);
		if (__parse_message_continuation(parser) == -1) return -1;
	}
	/* TODO: more literals - array symbol #xxx #(1 2 3) */
	else if (parser->token.type == XP_STX_TOKEN_LBRACKET) {
	}
	else if (parser->token.type == XP_STX_TOKEN_LPAREN) {
	}
	else {
		parser->error_code = XP_STX_PARSER_ERROR_EXPRESSION_START;
		return -1;
	}

	return 0;
}
/*
   &unsupportedByte,      //--- 00
   &bytePushInstance,     //--- 01
   &bytePushArgument,     //--- 02
   &bytePushTemporary,    //--- 03
   &bytePushLiteral,      //--- 04
   &bytePushConstant,     //--- 05
   &byteAssignInstance,   //--- 06
   &byteAssignTemporary,  //--- 07
   &byteMarkArguments,    //--- 08
   &byteSendMessage,      //--- 09
   &byteSendUnary,        //--- 10
   &byteSendBinary,       //--- 11
   &unsupportedByte,      //--- 12
   &byteDoPrimitive,      //--- 13
   &unsupportedByte,      //--- 14
   &byteDoSpecial         //--- 15


* Directly access by byte codes
> the receiver and arguments of the invoking message 
> the values of the receiver's instance variables 
> the values of any temporary variables required by the method 
> seven special constants (true, false, nil, -1, 0, 1, and 2) 
> 32 special message selectors 

* contained in literal frame.
> shared variables (global, class, and pool) 
> most literal constants (numbers, characters, strings, arrays, and symbols) 
> most message selectors (those that are not special) 

PushInstance
PushArgument -> normal arguments plus self/super(0)
PushTemporary
PushConstant -> nil, true, false, etc....
PushLiteral -> global variables, literal constants... -> access literal frame

AssignInstance
AssignTemporary
*/

static int __identify_ident (xp_stx_parser_t* parser, const xp_char_t* ident)
{
	xp_size_t i;

	if (xp_strcmp(ident, XP_TEXT("self")) == 0) {
	}

	if (xp_strcmp(ident, XP_TEXT("super")) == 0) {
	}

	for (i = 0; i < parser->temporary_count; i++) {
		if (xp_strcmp (parser->temporary[i], ident) == 0) {
		}
	}

	for (i = 0; i < parser->argument_count; i++) {
		if (xp_strcmp (parser->argument[i], ident) == 0) {
		}
	}

	/* TODO; find it in intance variables names */

	/* TODO; find it in class variables names */

	/* TODO; find it in global variables */

	/* TODO: dynamic global */
	
	return 0;
}

static int __parse_assignment (
	xp_stx_parser_t* parser, const xp_char_t* target)
{
	/*
	 * <assignment> ::= <assignment target> assignmentOperator <expression>
	 */

	xp_size_t i;

	for (i = 0; i < parser->temporary_count; i++) {
		if (xp_strcmp (target, parser->temporary[i]) == 0) {
			if (__parse_expression(parser) == -1) return -1;
			EMIT_CODE (parser, XP_TEXT("AssignTemporary"), i);
			return 0;
		}
	}

	/* TODO: check it in instance variable */

	/* TODO: check it in class variables */

	/* TODO: global, but i don't like this idea */

	return -1;
}

static int __parse_message_continuation (xp_stx_parser_t* parser)
{
	if (__parse_keyword_continuation (parser) == -1) return -1;

	while (parser->token.type == XP_STX_TOKEN_SEMICOLON) {
		EMIT_CODE (parser, XP_TEXT("DoSpecial(DUP_RECEIVER)"), 0);
		GET_TOKEN (parser);

		if (__parse_keyword_continuation (parser) == -1) return -1;
		EMIT_CODE (parser, XP_TEXT("DoSpecial(POP_TOP)"), 0);
	}

	return 0;
}

static int __parse_keyword_continuation (xp_stx_parser_t* parser)
{
	if (__parser_binary_continuation (parser) == -1) return -1;
	return -1;
}

static int __parse_binary_continuation (xp_stx_parser_t* parser)
{
	if (__parser_unary_continuation (parser) == -1) return -1;
	return -1;
}

static int __parse_unary_continuation (xp_stx_parser_t* parser)
{
	while (parser->token.type == XP_STX_TOKEN_IDENT) {

	}

	return 0;
}

static int __emit_code (
	xp_stx_parser_t* parser, const xp_char_t* high, int low)
{
	xp_printf (XP_TEXT("CODE: %s %d\n"), high, low);
	return 0;
}


static inline xp_bool_t __is_binary_char (xp_cint_t c)
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
		ADD_TOKEN_CHAR(parser, c);
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
