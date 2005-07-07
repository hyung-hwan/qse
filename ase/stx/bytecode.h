/*
 * $Id: bytecode.h,v 1.2 2005-07-07 07:45:05 bacon Exp $
 */

#ifndef _XP_STX_BYTECODE_H_
#define _XP_STX_BYTECODE_H_

#include <xp/stx/stx.h>

#define PUSH_VARIABLE         0x0
#define PUSH_ARGUMENT         0x1
#define PUSH_TEMPORARY        0x2
#define PUSH_LITERAL          0x3
#define DO_SPECIAL            0x5
#define DO_PRIMITIVE          0x6
#define DO_PRIMITIVE_EXTENDED 0xF

#ifdef __cplusplus
extern "C" {
#endif

int xp_stx_decode (xp_stx_t* stx, xp_word_t class_idx);

#ifdef __cplusplus
}
#endif

#endif
