/*
 * $Id: parser.h,v 1.5 2005-05-30 15:24:12 bacon Exp $
 */

#ifndef _XP_STX_PARSER_H_
#define _XP_STX_PARSER_H_

#include <xp/stx/stx.h>
#include <xp/stx/lexer.h>
#include <xp/stx/token.h>

struct xp_stx_parser_t
{
	xp_stx_lexer_t lexer;
	xp_stx_token_t* token;
	int error_code;
	xp_bool_t __malloced;
};

typedef struct xp_stx_parser_t xp_stx_parser_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser);
void xp_stx_parser_close (xp_stx_parser_t* parser);

#ifdef __cplusplus
}
#endif

#endif
