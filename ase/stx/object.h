/*
 * $Id: object.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _ASE_STX_OBJECT_H_
#define _ASE_STX_OBJECT_H_

#include <ase/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

ase_word_t ase_stx_alloc_word_object (
	ase_stx_t* stx, const ase_word_t* data, ase_word_t nfields,
	const ase_word_t* variable_data, ase_word_t variable_nfields);

ase_word_t ase_stx_alloc_byte_object (
	ase_stx_t* stx, const ase_byte_t* data, ase_word_t n);

ase_word_t ase_stx_alloc_char_object (
	ase_stx_t* stx, const ase_char_t* str);
ase_word_t ase_stx_alloc_char_objectx (
	ase_stx_t* stx, const ase_char_t* str, ase_word_t n);
ase_word_t ase_stx_allocn_char_object (ase_stx_t* stx, ...);

ase_word_t ase_stx_hash_object (ase_stx_t* stx, ase_word_t object);

ase_word_t ase_stx_instantiate (
	ase_stx_t* stx, ase_word_t class_index, const void* data,
	const void* variable_data, ase_word_t variable_nfields);
ase_word_t ase_stx_classof (ase_stx_t* stx, ase_word_t obj);
ase_word_t ase_stx_sizeof (ase_stx_t* stx, ase_word_t obj);


#ifdef __cplusplus
}
#endif

#endif
