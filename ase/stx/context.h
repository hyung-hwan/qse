/* 
 * $Id: context.h,v 1.4 2005-08-18 15:16:39 bacon Exp $
 */

#ifndef _XP_STX_CONTEXT_H_
#define _XP_STX_CONTEXT_H_

#include <xp/stx/stxi.h>

#define PUSH_OBJECT 0xA0
#define SEND_UNARY_MESSAGE 0xB0
#define HALT 0xFF

#define XP_STX_CONTEXT_SIZE        4
#define XP_STX_CONTEXT_IP          0
#define XP_STX_CONTEXT_METHOD      1
#define XP_STX_CONTEXT_ARGUMENTS   2
#define XP_STX_CONTEXT_TEMPORARIES 3

struct xp_stx_context_t
{
	xp_stx_objhdr_t header;
	xp_word_t ip;
	xp_word_t method;
	xp_word_t arguments;
	xp_word_t temporaries;
};

typedef struct xp_stx_context_t xp_stx_context_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_new_context (xp_stx_t* stx, 
	xp_word_t method, xp_word_t args, xp_word_t temp);
int xp_stx_run_context (xp_stx_t* stx, xp_word_t context);

#ifdef __cplusplus
}
#endif

#endif
