/*
 * $Id: parser.h,v 1.36 2006-01-30 16:44:03 bacon Exp $
 */

#ifndef _XP_STX_PARSER_H_
#define _XP_STX_PARSER_H_

#include <xp/stx/stx.h>
#include <xp/stx/name.h>
#include <xp/stx/token.h>
#include <xp/bas/arr.h>

enum
{
	XP_STX_PARSER_ERROR_NONE,

	/* system errors */
	XP_STX_PARSER_ERROR_INPUT_FUNC,
	XP_STX_PARSER_ERROR_INPUT,
	XP_STX_PARSER_ERROR_MEMORY,

	/* lexical errors */
	XP_STX_PARSER_ERROR_CHAR,
	XP_STX_PARSER_ERROR_CHARLIT,
	XP_STX_PARSER_ERROR_STRLIT,
	XP_STX_PARSER_ERROR_LITERAL,

	/* syntatic error */
	XP_STX_PARSER_ERROR_MESSAGE_SELECTOR,
	XP_STX_PARSER_ERROR_ARGUMENT_NAME,
	XP_STX_PARSER_ERROR_TOO_MANY_ARGUMENTS,

	XP_STX_PARSER_ERROR_PRIMITIVE_KEYWORD,
	XP_STX_PARSER_ERROR_PRIMITIVE_NUMBER,
	XP_STX_PARSER_ERROR_PRIMITIVE_NUMBER_RANGE,
	XP_STX_PARSER_ERROR_PRIMITIVE_NOT_CLOSED,

	XP_STX_PARSER_ERROR_TEMPORARIES_NOT_CLOSED,
	XP_STX_PARSER_ERROR_TOO_MANY_TEMPORARIES,
	XP_STX_PARSER_ERROR_PSEUDO_VARIABLE,
	XP_STX_PARSER_ERROR_PRIMARY,

	XP_STX_PARSER_ERROR_NO_PERIOD,
	XP_STX_PARSER_ERROR_NO_RPAREN,
	XP_STX_PARSER_ERROR_BLOCK_ARGUMENT_NAME,
	XP_STX_PARSER_ERROR_BLOCK_ARGUMENT_LIST,
	XP_STX_PARSER_ERROR_BLOCK_NOT_CLOSED,

	XP_STX_PARSER_ERROR_UNDECLARED_NAME,
	XP_STX_PARSER_ERROR_TOO_MANY_LITERALS
};

enum
{
	/* input_func cmd */
	XP_STX_PARSER_INPUT_OPEN,
	XP_STX_PARSER_INPUT_CLOSE,
	XP_STX_PARSER_INPUT_CONSUME,
	XP_STX_PARSER_INPUT_REWIND
};

typedef struct xp_stx_parser_t xp_stx_parser_t;

struct xp_stx_parser_t
{
	xp_stx_t* stx;
	int error_code;

	xp_word_t method_class;
	xp_stx_name_t method_name;

	xp_char_t* temporaries[256]; /* TODO: different size? or dynamic? */
	xp_word_t argument_count;
	xp_word_t temporary_count;

	xp_word_t literals[256]; /* TODO: make it a dynamic array */
	xp_word_t literal_count;

	xp_arr_t bytecode;

	xp_stx_token_t token;
	xp_cint_t curc;
	xp_cint_t ungotc[5];
	xp_size_t ungotc_count;

	void* input_owner;
	int (*input_func) (int cmd, void* owner, void* arg);

	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser, xp_stx_t* stx);
void xp_stx_parser_close (xp_stx_parser_t* parser);

const xp_char_t* xp_stx_parser_error_string (xp_stx_parser_t* parser);
int xp_stx_parser_parse_method (
	xp_stx_parser_t* parser, xp_word_t method_class, void* input);

#ifdef __cplusplus
}
#endif

#endif
