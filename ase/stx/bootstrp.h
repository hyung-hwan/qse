/*
 * $Id: bootstrp.h,v 1.2 2005-05-23 15:51:03 bacon Exp $
 */

#ifndef _XP_STX_BOOTSTRP_H_
#define _XP_STX_BOOTSTRP_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif


xp_stx_word_t xp_stx_new_array (xp_stx_t* stx, xp_stx_word_t size);
int xp_stx_bootstrap (xp_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
