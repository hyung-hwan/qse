/*
 * $Id: bytecode.h,v 1.3 2005-07-07 16:32:37 bacon Exp $
 */

#ifndef _XP_STX_BYTECODE_H_
#define _XP_STX_BYTECODE_H_

#include <xp/stx/stx.h>

#define PUSH_VARIABLE           0x0
#define PUSH_TEMPORARY          0x1
#define PUSH_LITERAL            0x2
#define DO_SPECIAL              0x3
#define DO_PRIMITIVE            0x4
#define PUSH_VARIABLE_EXTENDED  0xA
#define PUSH_TEMPORARY_EXTENDED 0xB
#define DO_PRIMITIVE_EXTENDED   0xF

#ifdef __cplusplus
extern "C" {
#endif

int xp_stx_decode (xp_stx_t* stx, xp_word_t class_idx);

#ifdef __cplusplus
}
#endif

#endif
