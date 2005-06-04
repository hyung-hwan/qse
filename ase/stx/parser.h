/*
 * $Id: parser.h,v 1.8 2005-06-04 07:01:57 bacon Exp $
 */

#ifndef _XP_STX_PARSER_H_
#define _XP_STX_PARSER_H_

#include <xp/stx/stx.h>
#include <xp/stx/token.h>

struct xp_stx_parser_t
{
	int error_code;
	xp_stx_token_t token;

	/* lexer data */
	const xp_char_t* text;
	const xp_cint_t  curc;
	const xp_char_t* curp;

	xp_bool_t __malloced;
};

typedef struct xp_stx_parser_t xp_stx_parser_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser);
void xp_stx_parser_close (xp_stx_parser_t* parser);

int xp_stx_parse_method (xp_stx_parser_t* parser, 
	xp_stx_word_t method_class, xp_stx_char_t* method_text);

#ifdef __cplusplus
}
#endif

#endif
