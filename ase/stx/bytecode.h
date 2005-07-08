/*
 * $Id: bytecode.h,v 1.5 2005-07-08 11:32:50 bacon Exp $
 */

#ifndef _XP_STX_BYTECODE_H_
#define _XP_STX_BYTECODE_H_

#include <xp/stx/stx.h>

#define PUSH_RECEIVER_VARIABLE     0x00
#define PUSH_RECEIVER_VARIABLE_BIG 0x10

#define PUSH_VARIABLE            0x0
#define PUSH_TEMPORARY           0x1
#define PUSH_LITERAL_CONSTANT    0x2
#define PUSH_LITERAL_VARIABLE    0x3
#define STORE_VARIABLE           0x4
#define STORE_TEMPORARY          0x5
#define SEND                     0x6
#define JUMP                     0x7
#define DO_SPECIAL               0x8
#define DO_PRIMITIVE             0x9
#define PUSH_VARIABLE_EXTENDED   0xA
#define PUSH_TEMPORARY_EXTENDED  0xB
#define STORE_TEMPORARY_EXTENDED 0xC
#define STORE_VARIABLE_EXTENDED  0xD
#define DO_SPECIAL_EXTENDED      0xE
#define DO_PRIMITIVE_EXTENDED    0xF

#define RETURN_RECEIVER      1
#define RETURN_NIL           2
#define RETURN_TRUE          3
#define RETURN_FALSE         4
#define RETURN_FROM_MESSAGE  5
#define RETURN_FROM_BLOCK    6

#define PUSH_VARIABLE(i)    ((i <= 0x0F)?  (i): 
#define PUSH_VARIABLE(i)     i

#ifdef __cplusplus
extern "C" {
#endif

int xp_stx_decode (xp_stx_t* stx, xp_word_t class_idx);

#ifdef __cplusplus
}
#endif

#endif
