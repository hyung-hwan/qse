/*
 * $Id: bootstrp.h,v 1.5 2005-06-30 12:07:02 bacon Exp $
 */

#ifndef _XP_STX_BOOTSTRP_H_
#define _XP_STX_BOOTSTRP_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_new_array (xp_stx_t* stx, xp_word_t size);
xp_word_t xp_stx_new_string (xp_stx_t* stx, const xp_char_t* str);
int xp_stx_bootstrap (xp_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
