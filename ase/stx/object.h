/*
 * $Id: object.h,v 1.19 2005-07-04 11:32:41 bacon Exp $
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
int xp_stx_lookup_global (
	xp_stx_t* stx, xp_word_t key, xp_word_t* value);

#ifdef __cplusplus
}
#endif

#endif
