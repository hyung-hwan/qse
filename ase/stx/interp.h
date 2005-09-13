/*
 * $Id: interp.h,v 1.6 2005-09-13 11:15:41 bacon Exp $
 */

#ifndef _XP_STX_INTERP_H_
#define _XP_STX_INTERP_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

int xp_stx_interp (xp_stx_t* stx, xp_word_t receiver, xp_word_t method);

#ifdef __cplusplus
}
#endif

#endif
