/*
 * $Id: token.h,v 1.1 2005-02-04 15:39:11 bacon Exp $
 */

#ifndef _RBL_TOKEN_H_
#define _RBL_TOKEN_H_

#include "types.h"

struct xp_lisp_token_t 
{
	int        type;

	xp_lisp_int    ivalue;
	xp_lisp_float  fvalue;

	xp_size_t     capacity;
	xp_size_t     size;
	xp_lisp_char*  buffer;
};

typedef struct xp_lisp_token_t xp_lisp_token_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_lisp_token_t* xp_lisp_token_new      (xp_size_t capacity);
void         xp_lisp_token_free     (xp_lisp_token_t* token);
int          xp_lisp_token_addc     (xp_lisp_token_t* token, xp_lisp_cint c);
void         xp_lisp_token_clear    (xp_lisp_token_t* token);
xp_lisp_char*    xp_lisp_token_transfer (xp_lisp_token_t* token, xp_size_t capacity);
int          xp_lisp_token_compare  (xp_lisp_token_t* token, const xp_lisp_char* str);

#ifdef __cplusplus
}
#endif

#endif
