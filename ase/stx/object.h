/*
 * $Id: object.h,v 1.28 2005-07-19 15:52:19 bacon Exp $
 */

#ifndef _XP_STX_OBJECT_H_
#define _XP_STX_OBJECT_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_alloc_word_object (
	xp_stx_t* stx, const xp_word_t* data, xp_word_t nfields,
	const xp_word_t* variable_data, xp_word_t variable_nfields);

xp_word_t xp_stx_alloc_byte_object (
	xp_stx_t* stx, const xp_byte_t* data, xp_word_t n);

xp_word_t xp_stx_alloc_char_object (
	xp_stx_t* stx, const xp_char_t* str);
xp_word_t xp_stx_alloc_char_objectx (
	xp_stx_t* stx, const xp_char_t* str, xp_word_t n);
xp_word_t xp_stx_allocn_char_object (xp_stx_t* stx, ...);

xp_word_t xp_stx_hash_object (xp_stx_t* stx, xp_word_t object);

xp_word_t xp_stx_instantiate (
	xp_stx_t* stx, xp_word_t class_index, const void* data,
	const void* variable_data, xp_word_t variable_nfields);
xp_word_t xp_stx_classof (xp_stx_t* stx, xp_word_t obj);
xp_word_t xp_stx_sizeof (xp_stx_t* stx, xp_word_t obj);


#ifdef __cplusplus
}
#endif

#endif
