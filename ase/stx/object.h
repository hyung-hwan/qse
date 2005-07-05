/*
 * $Id: object.h,v 1.20 2005-07-05 04:29:31 bacon Exp $
 */

#ifndef _XP_STX_OBJECT_H_
#define _XP_STX_OBJECT_H_

#include <xp/stx/stx.h>

#define XP_STX_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define XP_STX_TO_SMALLINT(x)   (((x) << 1) | 0x01)
#define XP_STX_FROM_SMALLINT(x) ((x) >> 1)

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_alloc_word_object (xp_stx_t* stx, xp_word_t n);

xp_word_t xp_stx_alloc_byte_object (
	xp_stx_t* stx, const xp_byte_t* data, xp_word_t n);

xp_word_t xp_stx_alloc_char_object (
	xp_stx_t* stx, const xp_char_t* str);
xp_word_t xp_stx_alloc_char_objectx (
	xp_stx_t* stx, const xp_char_t* str, xp_word_t n);
xp_word_t xp_stx_allocn_char_object (xp_stx_t* stx, ...);

xp_word_t xp_stx_hash_char_object (xp_stx_t* stx, xp_word_t idx);

xp_word_t xp_stx_instantiate (
        xp_stx_t* stx, xp_word_t class_index, xp_word_t size);

#ifdef __cplusplus
}
#endif

#endif
