/*
 * $Id: token.h,v 1.3 2005-06-02 16:14:58 bacon Exp $
 */

#ifndef _XP_STX_TOKEN_H_
#define _XP_STX_TOKEN_H_

#include <xp/stx/stx.h>

enum
{
	XP_STX_TOKEN_END    = 0,
	XP_STX_TOKEN_STRING = 1,
	XP_STX_TOKEN_IDENT  = 2,
	XP_STX_TOKEN_SELF   = 3,
	XP_STX_TOKEN_SUPER  = 4,
	XP_STX_TOKEN_NIL    = 5,
	XP_STX_TOKEN_TRUE   = 6,
	XP_STX_TOKEN_FALSE  = 7
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
