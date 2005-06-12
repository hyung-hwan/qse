/*
 * $Id: name.h,v 1.1 2005-06-12 16:22:03 bacon Exp $
 */

#ifndef _XP_STX_NAME_H_
#define _XP_STX_NAME_H_

#include <xp/stx/stx.h>

struct xp_stx_name_t 
{
	xp_word_t   capacity;
	xp_word_t   size;
	xp_char_t*  buffer;
	xp_char_t   static_buffer[128];
	xp_bool_t __malloced;
};

typedef struct xp_stx_name_t xp_stx_name_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_name_t* xp_stx_name_open (
	xp_stx_name_t* name, xp_word_t capacity);
void xp_stx_name_close (xp_stx_name_t* name);

int xp_stx_name_addc (xp_stx_name_t* name, xp_cint_t c);
int xp_stx_name_adds (xp_stx_name_t* name, const xp_char_t* s);
void xp_stx_name_clear (xp_stx_name_t* name);
xp_char_t* xp_stx_name_yield (xp_stx_name_t* name, xp_word_t capacity);
int xp_stx_name_compare  (xp_stx_name_t* name, const xp_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
