/*
 * $Id: object.h,v 1.11 2005-05-15 18:37:00 bacon Exp $
 */

#ifndef _XP_STX_OBJECT_H_
#define _XP_STX_OBJECT_H_

#include <xp/stx/stx.h>

#define XP_STX_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define XP_STX_TO_SMALLINT(x)   (((x) << 1) | 0x01)
#define XP_STX_FROM_SMALLINT(x) ((x) >> 1)

/* definitions for common objects */
#define XP_STX_CLASS_DIMENSION   8
#define XP_STX_CLASS_NAME        0
#define XP_STX_CLASS_SIZE        1
#define XP_STX_CLASS_METHODS     2
#define XP_STX_CLASS_SUPERCLASS  3
#define XP_STX_CLASS_VARIABLES   4
#define XP_STX_CLASS_CLASSVARS   5
#define XP_STX_CLASS_POOLDICT    6
#define XP_STX_CLASS_CATEGORY    7

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_word_t xp_stx_alloc_object (xp_stx_t* stx, xp_stx_word_t n);
xp_stx_word_t xp_stx_alloc_byte_object (xp_stx_t* stx, xp_stx_word_t n);
xp_stx_word_t xp_stx_alloc_string_object (
	xp_stx_t* stx, const xp_stx_char_t* str);
xp_stx_word_t xp_stx_allocn_string_object (xp_stx_t* stx, ...);

xp_stx_word_t xp_stx_hash_string_object (xp_stx_t* stx, xp_stx_word_t idx);
xp_stx_word_t xp_stx_new_class (
	xp_stx_t* stx, const xp_stx_char_t* name);

int xp_stx_lookup_global (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t* value);

#ifdef __cplusplus
}
#endif

#endif
