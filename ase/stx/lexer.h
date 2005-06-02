/*
 * $Id: lexer.h,v 1.2 2005-06-02 16:14:58 bacon Exp $
 */

#ifndef _XP_STX_LEXER_H_
#define _XP_STX_LEXER_H_

#include <xp/stx/stx.h>
#include <xp/stx/token.h>

struct xp_stx_lexer_t
{
	xp_stx_token_t token;
	const xp_stx_char_t* text;
	xp_bool_t __malloced;
};

typedef struct xp_stx_lexer_t xp_stx_lexer_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_lexer_t* xp_stx_lexer_open (
	xp_stx_lexer_t* lexer, const xp_stx_char_t* text);
void xp_stx_lexer_close (xp_stx_lexer_t* lexer);

void xp_stx_lexer_reset (
	xp_stx_lexer_t* lexer, const xp_stx_char_t* text);
xp_stx_token_t* xp_stx_lexer_consume (xp_stx_lexer_t* lexer);

#ifdef __cplusplus
}
#endif

#endif
