/*
 * $Id: token.h,v 1.4 2005-06-05 16:44:05 bacon Exp $
 */

#ifndef _XP_STX_TOKEN_H_
#define _XP_STX_TOKEN_H_

#include <xp/stx/stx.h>

enum
{
	XP_STX_TOKEN_END = 0,
	XP_STX_TOKEN_CHARLIT,
	XP_STX_TOKEN_STRLIT,
	XP_STX_TOKEN_IDENT,
	XP_STX_TOKEN_KEYWORD
};

struct xp_stx_token_t 
{
	int type;

	/*
	xp_stx_int_t   ivalue;
	xp_stx_real_t  fvalue;
	*/

	xp_stx_word_t   capacity;
	xp_stx_word_t   size;
	xp_stx_char_t*  buffer;

	xp_bool_t __malloced;
};

typedef struct xp_stx_token_t xp_stx_token_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_token_t* xp_stx_token_open (
	xp_stx_token_t* token, xp_stx_word_t capacity);
void xp_stx_token_close (xp_stx_token_t* token);

int xp_stx_token_addc (xp_stx_token_t* token, xp_stx_cint_t c);
void xp_stx_token_clear (xp_stx_token_t* token);
xp_stx_char_t* xp_stx_token_yield (xp_stx_token_t* token, xp_stx_word_t capacity);
int xp_stx_token_compare  (xp_stx_token_t* token, const xp_stx_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
