/*
 * $Id: bytecode.h,v 1.4 2005-07-07 16:52:48 bacon Exp $
 */

#ifndef _XP_STX_BYTECODE_H_
#define _XP_STX_BYTECODE_H_

#include <xp/stx/stx.h>

#define PUSH_VARIABLE           0x0
#define PUSH_TEMPORARY          0x1
#define PUSH_LITERAL_CONSTANT   0x2
#define PUSH_LITERAL_VARIABLE   0x3
#define STORE_VARIABLE          0x4
#define STORE_TEMPORARY         0x5
#define DO_SPECIAL              0x6
#define DO_PRIMITIVE            0x7
#define PUSH_VARIABLE_EXTENDED  0xA
#define PUSH_TEMPORARY_EXTENDED 0xB
#define STORE_TEMPORARY_EXTENDED 0xC
#define DO_PRIMITIVE_EXTENDED   0xF

#ifdef __cplusplus
extern "C" {
#endif

int xp_stx_decode (xp_stx_t* stx, xp_word_t class_idx);

#ifdef __cplusplus
}
#endif

#endif
