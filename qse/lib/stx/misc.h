/*
 * $Id: misc.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_MISC_H_
#define _QSE_STX_MISC_H_

#include <qse/stx/stx.h>

/* TODO: remove this header later */
#include <qse/bas/stdio.h>

#ifdef _DOS
	#include <stdlib.h>
	#include <assert.h>
	#include <stdarg.h>
	#include <stdio.h>
	#include <ctype.h>
	#include <stdlib.h>

	#define qse_assert   assert
	#define qse_malloc   malloc
	#define qse_realloc  realloc
	#define qse_free     free
	#define qse_va_list  va_list
	#define qse_va_start va_start
	#define qse_va_end   va_end  
	#define qse_va_arg   va_arg
	#define qse_isspace  isspace
	#define qse_isdigit  isdigit
	#define qse_isalpha  isalpha
	#define qse_isalnum  isalnum
#else
	#include <qse/bas/memory.h>
	#include <qse/bas/assert.h>
	#include <qse/bas/stdarg.h>
	#include <qse/bas/ctype.h>
	#include <qse/bas/string.h>
	#include <qse/bas/stdlib.h>
#endif

#if defined(__BORLANDC__) || defined(_MSC_VER)
	#define INLINE 
#else
	#define INLINE inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_hash (const void* data, qse_word_t len);
qse_word_t qse_stx_strhash (const qse_char_t* str);
qse_word_t qse_stx_strxhash (const qse_char_t* str, qse_word_t len);

qse_char_t* qse_stx_strword (
	const qse_char_t* str, const qse_char_t* word, qse_word_t* word_index);

#ifdef __cplusplus
}
#endif

#endif
