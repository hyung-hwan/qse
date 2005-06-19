/*
 * $Id: parser.h,v 1.22 2005-06-19 16:16:33 bacon Exp $
 */

#ifndef _XP_STX_PARSER_H_
#define _XP_STX_PARSER_H_

#include <xp/stx/stx.h>
#include <xp/stx/name.h>
#include <xp/stx/token.h>
#include <xp/bas/array.h>

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

	/* syntatic error */
	XP_STX_PARSER_ERROR_MESSAGE_SELECTOR,
	XP_STX_PARSER_ERROR_ARGUMENT_NAME,
	XP_STX_PARSER_ERROR_TOO_MANY_ARGUMENTS,
	XP_STX_PARSER_ERROR_TEMPORARIES_NOT_CLOSED,
	XP_STX_PARSER_ERROR_TOO_MANY_TEMPORARIES,
	XP_STX_PARSER_ERROR_PSEUDO_VARIABLE,
	XP_STX_PARSER_ERROR_EXPRESSION_START,
	XP_STX_PARSER_ERROR_NO_PERIOD
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

	xp_char_t* argument[32];
	xp_size_t argument_count;
	xp_char_t* temporary[32];
	xp_size_t temporary_count;

	xp_array_t byte_code;

	xp_stx_token_t token;
	xp_cint_t curc;
	xp_cint_t ungotc[5];
	xp_size_t ungotc_count;

	void* input_owner;
	int (*input_func) (int cmd, void* owner, void* arg);

	xp_bool_t __malloced;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser, xp_stx_t* stx);
void xp_stx_parser_close (xp_stx_parser_t* parser);

int xp_stx_parser_parse_method (
	xp_stx_parser_t* parser, xp_word_t method_class, void* input);

#ifdef __cplusplus
}
#endif

#endif
