/*
 * $Id: misc.h,v 1.10 2005-07-03 16:37:01 bacon Exp $
 */

#ifndef _XP_STX_MISC_H_
#define _XP_STX_MISC_H_

#include <xp/stx/stx.h>

#ifdef _DOS
	#include <stdlib.h>
	#include <assert.h>
	#include <stdarg.h>
	#include <stdio.h>
	#include <ctype.h>

	#define xp_assert   assert
	#define xp_malloc   malloc
	#define xp_realloc  realloc
	#define xp_free     free
	#define xp_va_list  va_list
	#define xp_va_start va_start
	#define xp_va_end   va_end  
	#define xp_va_arg   va_arg
	#define xp_isspace  isspace
	#define xp_isdigit  isdigit
	#define xp_isalpha  isalpha
	#define xp_isalnum  isalnum
#else
	#include <xp/bas/memory.h>
	#include <xp/bas/assert.h>
	#include <xp/bas/stdarg.h>
	#include <xp/bas/ctype.h>
	#include <xp/bas/string.h>
#endif

#if defined(__BORLANDC__) || defined(_MSC_VER)
	#define INLINE 
#else
	#define INLINE inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_strhash (const xp_char_t* str);
xp_word_t xp_stx_strxhash (const xp_char_t* str, xp_word_t len);

xp_char_t* xp_stx_strword (
	const xp_char_t* str, const xp_char_t* word, xp_word_t* word_index);

#ifdef __cplusplus
}
#endif

#endif
