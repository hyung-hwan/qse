/*
 * $Id: method.h,v 1.5 2005-08-18 15:28:18 bacon Exp $
 */

#ifndef _XP_STX_METHOD_H_
#define _XP_STX_METHOD_H_

#include <xp/stx/stx.h>

#define XP_STX_METHOD_SIZE           3
#define XP_STX_METHOD_TEXT           0
#define XP_STX_METHOD_SELECTOR       1
#define XP_STX_METHOD_BYTECODES      2

struct xp_stx_method_t
{
	xp_stx_objhdr_t header;
	xp_word_t text;
	xp_word_t selector; /* is this necessary? */
	xp_word_t bytecodes;
	xp_word_t temporaries; /* number of temporaries required */
	xp_word_t literals[1];
};

typedef struct xp_stx_method_t xp_stx_method_t;

#ifdef __cplusplus
extern "C"  {
#endif

#ifdef __cplusplus
}
#endif

#endif
