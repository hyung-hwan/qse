/*
 * $Id: bytecode.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_BYTECODE_H_
#define _QSE_STX_BYTECODE_H_

#include <qse/stx/stx.h>

#define PUSH_RECEIVER_VARIABLE            0x00
#define PUSH_TEMPORARY_LOCATION           0x10
#define PUSH_LITERAL_CONSTANT             0x20
#define PUSH_LITERAL_VARIABLE             0x30
#define STORE_RECEIVER_VARIABLE           0x40
#define STORE_TEMPORARY_LOCATION          0x50

#define PUSH_RECEIVER_VARIABLE_EXTENDED   0x60
#define PUSH_TEMPORARY_LOCATION_EXTENDED  0x61
#define PUSH_LITERAL_CONSTANT_EXTENDED    0x62
#define PUSH_LITERAL_VARIABLE_EXTENDED    0x63
#define STORE_RECEIVER_VARIABLE_EXTENDED  0x64
#define STORE_TEMPORARY_LOCATION_EXTENDED 0x65

#define POP_STACK_TOP                     0x67
#define DUPLICATE_POP_STACK_TOP           0x68
#define PUSH_ACTIVE_CONTEXT               0x69
#define PUSH_NIL                          0x6A
#define PUSH_TRUE                         0x6B
#define PUSH_FALSE                        0x6C
#define PUSH_RECEIVER                     0x6D

#define SEND_TO_SELF                      0x70
#define SEND_TO_SUPER                     0x71
#define SEND_TO_SELF_EXTENDED             0x72
#define SEND_TO_SUPER_EXTENDED            0x73

#define RETURN_RECEIVER                   0x78
#define RETURN_TRUE                       0x79
#define RETURN_FALSE                      0x7A
#define RETURN_NIL                        0x7B
#define RETURN_FROM_MESSAGE               0x7C
#define RETURN_FROM_BLOCK                 0x7D

#define DO_PRIMITIVE                      0xF0

#ifdef __cplusplus
extern "C" {
#endif

int qse_stx_decode (qse_stx_t* stx, qse_word_t class_idx);

#ifdef __cplusplus
}
#endif

#endif
