/*
 * $Id: token.h,v 1.4 2005-02-07 15:10:41 bacon Exp $
 */

#ifndef _XP_LISP_TOKEN_H_
#define _XP_LISP_TOKEN_H_

#include <xp/lisp/types.h>

struct xp_lisp_token_t 
{
	int        type;

	xp_lisp_int    ivalue;
	xp_lisp_float  fvalue;

	xp_size_t     capacity;
	xp_size_t     size;
	xp_char_t*  buffer;
};

typedef struct xp_lisp_token_t xp_lisp_token_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_lisp_token_t* xp_lisp_token_new      (xp_size_t capacity);
void         xp_lisp_token_free     (xp_lisp_token_t* token);
int          xp_lisp_token_addc     (xp_lisp_token_t* token, xp_cint_t c);
void         xp_lisp_token_clear    (xp_lisp_token_t* token);
xp_char_t*    xp_lisp_token_transfer (xp_lisp_token_t* token, xp_size_t capacity);
int          xp_lisp_token_compare  (xp_lisp_token_t* token, const xp_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
