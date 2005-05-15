/* 
 * $Id: context.h,v 1.1 2005-05-15 18:37:00 bacon Exp $
 */

#ifndef _XP_STX_CONTEXT_H_
#define _XP_STX_CONTEXT_H_

#include <xp/stx/stx.h>

#define PUSH_OBJECT 0xA0
#define SEND_UNARY_MESSAGE 0xB0
#define HALT 0xFF

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_word_t xp_stx_new_context (xp_stx_t* stx, 
	xp_stx_word_t method, xp_stx_word_t args, xp_stx_word_t temp);
int xp_stx_run_context (xp_stx_t* stx, xp_stx_word_t context);

#ifdef __cplusplus
}
#endif

#endif
