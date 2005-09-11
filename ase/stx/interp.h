/*
 * $Id: interp.h,v 1.5 2005-09-11 15:15:35 bacon Exp $
 */

#ifndef _XP_STX_INTERP_H_
#define _XP_STX_INTERP_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_new_context (xp_stx_t* stx, xp_word_t receiver, xp_word_t method);
int xp_stx_interp (xp_stx_t* stx, xp_word_t context);

#ifdef __cplusplus
}
#endif

#endif
