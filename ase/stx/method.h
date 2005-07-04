/*
 * $Id: method.h,v 1.1 2005-07-04 11:32:41 bacon Exp $
 */

#ifndef _XP_STX_METHOD_H_
#define _XP_STX_METHOD_H_

#include <xp/stx/stx.h>

#define XP_STX_METHOD_SIZE           6
#define XP_STX_METHOD_TEXT           0
#define XP_STX_METHOD_MESSAGE        1
#define XP_STX_METHOD_BYTECODES      2
#define XP_STX_METHOD_LITERALS       3
#define XP_STX_METHOD_STACK_SIZE     4
#define XP_STX_METHOD_TEMPORARY_SIZE 5

#ifdef __cplusplus
extern "C"  {
#endif

int xp_stx_new_method (xp_stx_t* stx, xp_word_t size);

#ifdef __cplusplus
}
#endif

#endif
