/*
 * $Id: bootstrp.h,v 1.8 2005-08-18 15:28:18 bacon Exp $
 */

#ifndef _XP_STX_BOOTSTRP_H_
#define _XP_STX_BOOTSTRP_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_new_array (xp_stx_t* stx, xp_word_t size);
int xp_stx_bootstrap (xp_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
