/*
 * $Id: parser.h,v 1.12 2005-06-08 03:16:34 bacon Exp $
 */

#ifndef _XP_STX_PARSER_H_
#define _XP_STX_PARSER_H_

#include <xp/stx/stx.h>
#include <xp/stx/token.h>

enum
{
	XP_STX_PARSER_ERROR_NONE,

	/* system errors */
	XP_STX_PARSER_ERROR_INVALID,
	XP_STX_PARSER_ERROR_MEMORY,
	XP_STX_PARSER_ERROR_INPUT,

	/* lexical errors */
	XP_STX_PARSER_ERROR_CHAR,
	XP_STX_PARSER_ERROR_CHARLIT,
	XP_STX_PARSER_ERROR_STRLIT,

	/* syntatic error */
	XP_STX_PARSER_ERROR_MESSAGE_SELECTOR
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
	int error_code;
	xp_stx_token_t token;

	xp_stx_cint_t curc;
	xp_stx_cint_t ungotc[5];
	xp_size_t ungotc_count;

	void* input_owner;
	int (*input_func) (int cmd, void* owner, void* arg);

	xp_bool_t __malloced;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser);
void xp_stx_parser_close (xp_stx_parser_t* parser);

int xp_stx_parser_parse_method (
	xp_stx_parser_t* parser, xp_stx_word_t method_class, void* input);

#ifdef __cplusplus
}
#endif

#endif
