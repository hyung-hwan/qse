/*
 * $Id: parser.c,v 1.21 2005-06-08 03:16:34 bacon Exp $
 */

#include <xp/stx/parser.h>
#include <xp/stx/token.h>
#include <xp/stx/misc.h>

static int __parse_method (
	xp_stx_parser_t* parser, 
	xp_stx_word_t method_class, void* input);
static int __parse_message_pattern (xp_stx_parser_t* parser);
static int __parse_temporaries (xp_stx_parser_t* parser);
static int __parse_statements (xp_stx_parser_t* parser);

static int __get_token (xp_stx_parser_t* parser);
static int __get_ident (xp_stx_parser_t* parser);
static int __get_numlit (xp_stx_parser_t* parser, xp_bool_t negated);
static int __get_charlit (xp_stx_parser_t* parser);
static int __get_strlit (xp_stx_parser_t* parser);
static int __skip_spaces (xp_stx_parser_t* parser);
static int __skip_comment (xp_stx_parser_t* parser);
static int __get_char (xp_stx_parser_t* parser);
static int __unget_char (xp_stx_parser_t* parser, xp_cint_t c);
static int __open_input (xp_stx_parser_t* parser, void* input);
static int __close_input (xp_stx_parser_t* parser);

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser)
{
	if (parser == XP_NULL) {
		parser = (xp_stx_parser_t*)
			xp_stx_malloc (xp_sizeof(xp_stx_parser_t));		
		if (parser == XP_NULL) return XP_NULL;
		parser->__malloced = xp_true;
	}
	else parser->__malloced = xp_false;

	if (xp_stx_token_open (&parser->token, 256) == XP_NULL) {
		if (parser->__malloced) xp_stx_free (parser);
		return XP_NULL;
	}

	parser->error_code = XP_STX_PARSER_ERROR_NONE;
	parser->curc = XP_STX_CHAR_EOF;
	parser->ungotc_count = 0;

	parser->input_owner = XP_NULL;
	parser->input_func = XP_NULL;
	return parser;
}

void xp_stx_parser_close (xp_stx_parser_t* parser)
{
	xp_stx_token_close (&parser->token);
	if (parser->__malloced) xp_stx_free (parser);
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
	
int xp_stx_parser_parse_method (
	xp_stx_parser_t* parser, xp_stx_word_t method_class, void* input)
{
	int n;

	if (parser->input_func == XP_NULL) { 
		parser->error_code = XP_STX_PARSER_ERROR_INVALID;
		return -1;
	}

	if (__open_input(parser,input) == -1) return -1;
	n = __parse_method (parser, method_class, input);
	if (__close_input(parser) == -1) return -1;

	return n;
}

static int __parse_method (
	xp_stx_parser_t* parser, xp_stx_word_t method_class, void* input)
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

	if (parser->token.type == XP_STX_TOKEN_IDENT) { 
		/* unary pattern */
	}
	else if (parser->token.type == XP_STX_TOKEN_BINARY) { 
		/* binary pattern */
	}
	else if (parser->token.type == XP_STX_TOKEN_KEYWORD) { 
		/* keyword pattern */
		xp_stx_char_t* selector;
	}
	else {
		parser->error_code = XP_STX_PARSER_ERROR_MESSAGE_SELECTOR;
		return -1;
	}
/*
	while (parser->token.type != XP_STX_TOKEN_END) {
		xp_printf (XP_TEXT("token: [%s] %d\n"), 
			parser->token.buffer, parser->token.type);
		GET_TOKEN (parser);
	}
*/
	return 0;
}

static int __parse_temporaries (xp_stx_parser_t* parser)
{
	return -1;
}

static int __parse_statements (xp_stx_parser_t* parser)
{
	return -1;
}

static int __get_token (xp_stx_parser_t* parser)
{
	xp_cint_t c;

	do {
		if (__skip_spaces(parser) == -1) return -1;
		if (parser->curc == XP_STX_CHAR('"')) {
			GET_CHAR (parser);
			if (__skip_comment(parser) == -1) return -1;
		}
		else break;
	} while (1);

	c = parser->curc;
	xp_stx_token_clear (&parser->token);

	if (c == XP_STX_CHAR_EOF) {
		parser->token.type = XP_STX_TOKEN_END;
	}
	else if (xp_stx_isalpha(c)) {
		if (__get_ident(parser) == -1) return -1;
	}
	else if (xp_stx_isdigit(c)) {
		if (__get_numlit(parser, xp_false) == -1) return -1;
	}
	else if (c == XP_STX_CHAR('-')) {
		parser->token.type = XP_STX_TOKEN_MINUS;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
		
		c = parser->curc;
		if (xp_stx_isdigit(c)) {
			if (__get_numlit(parser,xp_true) == -1) return -1;	
		}
	}
	else if (c == XP_STX_CHAR('$')) {
		GET_CHAR (parser);
		if (__get_charlit(parser) == -1) return -1;
	}
	else if (c == XP_STX_CHAR('\'')) {
		GET_CHAR (parser);
		if (__get_strlit(parser) == -1) return -1;
	}
	else if (c == XP_STX_CHAR(':')) {
		parser->token.type = XP_STX_TOKEN_COLON;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);

		c = parser->curc;
		if (c == XP_STX_CHAR('=')) {
			parser->token.type = XP_STX_TOKEN_ASSIGN;
			ADD_TOKEN_CHAR(parser, c);
			GET_CHAR (parser);
		}
	}
	else if (c == XP_STX_CHAR('^')) {
		parser->token.type = XP_STX_TOKEN_RETURN;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_STX_CHAR('|')) {
		parser->token.type = XP_STX_TOKEN_BAR;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_STX_CHAR('[')) {
		parser->token.type = XP_STX_TOKEN_LBRACKET;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_STX_CHAR(']')) {
		parser->token.type = XP_STX_TOKEN_RBRACKET;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else if (c == XP_STX_CHAR('.')) {
		parser->token.type = XP_STX_TOKEN_PERIOD;
		ADD_TOKEN_CHAR(parser, c);
		GET_CHAR (parser);
	}
	else {
		parser->error_code = XP_STX_PARSER_ERROR_CHAR;
		return -1;
	}

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
	} while (xp_stx_isalnum(c));

	if (c == XP_STX_CHAR(':')) {
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
	} while (xp_stx_isalnum(c));

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
	if (c == XP_STX_CHAR_EOF) {
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

			if (c == XP_STX_CHAR_EOF) {
				parser->error_code = XP_STX_PARSER_ERROR_STRLIT;
				return -1;
			}
		} while (c != XP_STX_CHAR('\''));

		GET_CHAR (parser);
		c = parser->curc;
	} while (c == XP_STX_CHAR('\''));

	return 0;
}

static int __skip_spaces (xp_stx_parser_t* parser)
{
	while (xp_stx_isspace(parser->curc)) GET_CHAR (parser);
	return 0;
}

static int __skip_comment (xp_stx_parser_t* parser)
{
	while (parser->curc != XP_STX_CHAR('"')) GET_CHAR (parser);
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
	parser->curc = XP_STX_CHAR_EOF;
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
