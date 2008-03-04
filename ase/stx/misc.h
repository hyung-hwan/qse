/*
 * $Id: misc.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _ASE_STX_MISC_H_
#define _ASE_STX_MISC_H_

#include <ase/stx/stx.h>

/* TODO: remove this header later */
#include <ase/bas/stdio.h>

#ifdef _DOS
	#include <stdlib.h>
	#include <assert.h>
	#include <stdarg.h>
	#include <stdio.h>
	#include <ctype.h>
	#include <stdlib.h>

	#define ase_assert   assert
	#define ase_malloc   malloc
	#define ase_realloc  realloc
	#define ase_free     free
	#define ase_va_list  va_list
	#define ase_va_start va_start
	#define ase_va_end   va_end  
	#define ase_va_arg   va_arg
	#define ase_isspace  isspace
	#define ase_isdigit  isdigit
	#define ase_isalpha  isalpha
	#define ase_isalnum  isalnum
#else
	#include <ase/bas/memory.h>
	#include <ase/bas/assert.h>
	#include <ase/bas/stdarg.h>
	#include <ase/bas/ctype.h>
	#include <ase/bas/string.h>
	#include <ase/bas/stdlib.h>
#endif

#if defined(__BORLANDC__) || defined(_MSC_VER)
	#define INLINE 
#else
	#define INLINE inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

ase_word_t ase_stx_hash (const void* data, ase_word_t len);
ase_word_t ase_stx_strhash (const ase_char_t* str);
ase_word_t ase_stx_strxhash (const ase_char_t* str, ase_word_t len);

ase_char_t* ase_stx_strword (
	const ase_char_t* str, const ase_char_t* word, ase_word_t* word_index);

#ifdef __cplusplus
}
#endif

#endif
