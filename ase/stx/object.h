/*
 * $Id: object.h,v 1.6 2005-05-10 12:00:43 bacon Exp $
 */

#ifndef _XP_STX_OBJECT_H_
#define _XP_STX_OBJECT_H_

#include <xp/stx/stx.h>

#define XP_STX_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define XP_STX_TO_SMALLINT(x)   ((x) << 1) | 0x01)
#define XP_STX_FROM_SMALLINT(x) ((x) >> 1)

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_word_t xp_stx_alloc_object (xp_stx_t* stx, xp_stx_word_t n);
xp_stx_word_t xp_stx_alloc_byte_object (xp_stx_t* stx, xp_stx_word_t n);
xp_stx_word_t xp_stx_alloc_string_object (
	xp_stx_t* stx, const xp_stx_char_t* str);

xp_stx_word_t xp_stx_hash_string_object (xp_stx_t* stx, xp_stx_word_t idx);

xp_stx_word_t xp_stx_new_string_object (
	xp_stx_t* stx, const xp_stx_char_t* name, xp_stx_word_t class);


#ifdef __cplusplus
}
#endif

#endif
