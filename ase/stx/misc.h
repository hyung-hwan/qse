/*
 * $Id: misc.h,v 1.5 2005-06-08 16:00:51 bacon Exp $
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

	/*
	#define xp_assert   xp_assert
	#define xp_malloc   xp_malloc
	#define xp_realloc  xp_realloc
	#define xp_free     xp_free
	#define xp_va_list  xp_va_list
	#define xp_va_start xp_va_start
	#define xp_va_end   xp_va_end  
	#define xp_va_arg   xp_va_arg
	#define xp_isspace  xp_isspace
	#define xp_isdigit  xp_isdigit
	#define xp_isalpha  xp_isalpha
	#define xp_isalnum  xp_isalnum
	*/
#endif

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_strlen (const xp_char_t* str);

int xp_stx_strcmp (
	const xp_char_t* s1, const xp_char_t* s2);
int xp_stx_strxcmp (
	const xp_char_t* s1, xp_word_t len, const xp_char_t* s2);

xp_word_t xp_stx_strhash (const xp_char_t* str);
xp_word_t xp_stx_strxhash (const xp_char_t* str, xp_word_t len);

#ifdef __cplusplus
}
#endif

#endif
