/*
 * $Id: object.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_OBJECT_H_
#define _QSE_STX_OBJECT_H_

#include <qse/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_alloc_word_object (
	qse_stx_t* stx, const qse_word_t* data, qse_word_t nfields,
	const qse_word_t* variable_data, qse_word_t variable_nfields);

qse_word_t qse_stx_alloc_byte_object (
	qse_stx_t* stx, const qse_byte_t* data, qse_word_t n);

qse_word_t qse_stx_alloc_char_object (
	qse_stx_t* stx, const qse_char_t* str);
qse_word_t qse_stx_alloc_char_objectx (
	qse_stx_t* stx, const qse_char_t* str, qse_word_t n);
qse_word_t qse_stx_allocn_char_object (qse_stx_t* stx, ...);

qse_word_t qse_stx_hash_object (qse_stx_t* stx, qse_word_t object);

qse_word_t qse_stx_instantiate (
	qse_stx_t* stx, qse_word_t class_index, const void* data,
	const void* variable_data, qse_word_t variable_nfields);
qse_word_t qse_stx_classof (qse_stx_t* stx, qse_word_t obj);
qse_word_t qse_stx_sizeof (qse_stx_t* stx, qse_word_t obj);


#ifdef __cplusplus
}
#endif

#endif
