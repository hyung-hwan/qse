/*
 * $Id: object.h,v 1.16 2005-05-22 15:03:20 bacon Exp $
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

xp_stx_word_t xp_stx_alloc_word_object (xp_stx_t* stx, xp_stx_word_t n);
xp_stx_word_t xp_stx_alloc_byte_object (xp_stx_t* stx, xp_stx_word_t n);
xp_stx_word_t xp_stx_alloc_char_object (
	xp_stx_t* stx, const xp_stx_char_t* str);
xp_stx_word_t xp_stx_allocn_char_object (xp_stx_t* stx, ...);

xp_stx_word_t xp_stx_hash_char_object (xp_stx_t* stx, xp_stx_word_t idx);
int xp_stx_lookup_global (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t* value);

#ifdef __cplusplus
}
#endif

#endif
