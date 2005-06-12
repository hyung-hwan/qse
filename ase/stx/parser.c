/*
 * $Id: parser.c,v 1.27 2005-06-12 14:40:35 bacon Exp $
 */

#include <xp/stx/parser.h>
#include <xp/stx/token.h>
#include <xp/stx/misc.h>

static int __parse_method (
	xp_stx_parser_t* parser, 
	xp_word_t method_class, void* input);
static int __parse_message_pattern (xp_stx_parser_t* parser);
static int __parse_temporaries (xp_stx_parser_t* parser);
static int __parse_statements (xp_stx_parser_t* parser);
static int __parse_statements_2 (xp_stx_parser_t* parser);
static int __parse_expression (xp_stx_parser_t* parser);

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

	if (xp_stx_token_open (&parser->token, 256) == XP_NULL) {
		if (parser->__malloced) xp_free (parser);
		return XP_NULL;
	}

	parser->stx = stx;
	parser->error_code = XP_STX_PARSER_ERROR_NONE;

	parser->argument_count = 0;

	parser->curc = XP_CHAR_EOF;
	parser->ungotc_count = 0;

	parser->input_owner = XP_NULL;
	parser->input_func = XP_NULL;
	return parser;
}

void xp_stx_parser_close (xp_stx_parser_t* parser)
{
	xp_stx_token_close (&parser->token);
	if (parser->__malloced) xp_free (parser);
}

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
		XP_TEXT("temporary list not closed"),
		XP_TEXT("invalid argument name"),
		XP_TEXT("too many arguments"),
		XP_TEXT("invalid expression start"),
		XP_TEXT("no period at end of statement")
	};

	if (parser->error_code >= 0 && 
	    parser->error_code < xp_countof(msg)) return msg[parser->error_code];

	return XP_TEXT("unknown error");
}

int xp_stx_parser_parse_method (
	xp_stx_parser_t* parser, xp_word_t method_class, void* input)
{
	int n;

	if (parser->input_func == XP_NULL) { 
		parser->error_code = XP_STX_PARSER_ERROR_INPUT_FUNC;
		return -1;
	}

	if (__open_input(parser,input) == -1) return -1;
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

	parser->argument_count = 0;

	if (parser->token.type == XP_STX_TOKEN_IDENT) { 
		/* unary pattern */
		GET_TOKEN (parser);
	}
	else if (parser->token.type == XP_STX_TOKEN_BINARY) { 
		/* binary pattern */
		GET_TOKEN (parser);
		if (parser->token.type != XP_STX_TOKEN_IDENT) {
			parser->error_code = XP_STX_PARSER_ERROR_ARGUMENT_NAME;
			return -1;
		}

		/* TODO: decide whether to use symbol */
		//parser->arguments[parser->argument_count++] = 
		GET_TOKEN (parser);
	}
	else if (parser->token.type == XP_STX_TOKEN_KEYWORD) { 
		/* keyword pattern */
		do {
			GET_TOKEN (parser);
			if (parser->token.type != XP_STX_TOKEN_IDENT) {
				parser->error_code = XP_STX_PARSER_ERROR_ARGUMENT_NAME;
				return -1;
			}
			GET_TOKEN (parser);
		} while (parser->token.type == XP_STX_TOKEN_KEYWORD);
	}
	else {
		parser->error_code = XP_STX_PARSER_ERROR_MESSAGE_SELECTOR;
		return -1;
	}

	return 0;
}

static inline xp_bool_t __is_vbar_token (const xp_stx_token_t* token)
{
	return 
		token->type == XP_STX_TOKEN_BINARY &&
		token->size == 1 &&
		token->buffer[0] == XP_CHAR('|');
}

static int __parse_temporaries (xp_stx_parser_t* parser)
{
	if (!__is_vbar_token(&parser->token)) return 0;

	GET_TOKEN (parser);
	while (parser->token.type == XP_STX_TOKEN_IDENT) {
xp_printf (XP_TEXT("temporary: %s\n"), parser->token.buffer);
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

	if (__parse_statements_2 (parser) == -1) return -1;
	if (parser->token.type != XP_STX_TOKEN_END) {
		parser->error_code = XP_STX_PARSER_ERROR_NO_PERIOD;
		return -1;
	}
	return 0;
}

static int __parse_statements_2 (xp_stx_parser_t* parser)
{
	if (parser->token.type == XP_STX_TOKEN_END) return 0;

	if (parser->token.type == XP_STX_TOKEN_RETURN) {
		GET_TOKEN (parser);
		if (__parse_expression (parser) == -1) return -1;
		/* TODO */
	}
	else {
		if (__parse_expression (parser) == -1) return -1;
	}

	if (parser->token.type == XP_STX_TOKEN_PERIOD) {
		GET_TOKEN (parser);
		if (__parse_statements_2 (parser) == -1) return -1;
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
xp_printf (XP_TEXT("identifier......[%s]\n"), parser->token.buffer);
		GET_TOKEN (parser);
	}
	else if (parser->token.type == XP_STX_TOKEN_CHARLIT ||
	         parser->token.type == XP_STX_TOKEN_STRLIT ||
	         parser->token.type == XP_STX_TOKEN_NUMLIT) {
		/* more literals - array symbol #xxx #(1 2 3) */
xp_printf (XP_TEXT("literal......[%s]\n"), parser->token.buffer);
		GET_TOKEN (parser);
	}
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
	else if (__is_binary_char(c)) {
		if (__get_binary(parser) == -1) return -1;
	}
	else {
		parser->error_code = XP_STX_PARSER_ERROR_CHAR;
		return -1;
	}

xp_printf (XP_TEXT("TOKEN: [%s]\n"), parser->token.buffer);
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
