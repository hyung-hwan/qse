/*
 * $Id: sa.h,v 1.32 2006-08-30 07:15:14 bacon Exp $
 */

#ifndef _XP_AWK_SA_H_
#define _XP_AWK_SA_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

#ifdef XP_AWK_STAND_ALONE 

#if !defined(XP_CHAR_IS_MCHAR) && !defined(XP_CHAR_IS_WCHAR)
#error Neither XP_CHAR_IS_MCHAR nor XP_CHAR_IS_WCHAR is defined.
#endif

#ifdef XP_AWK_NTDDK
	#include <ntddk.h>
	#include <stdarg.h>

	#define xp_assert ASSERT

	#define xp_malloc(size) ExAllocatePool(PagedPool,size)
	#define xp_free(ptr) ExFreePool(ptr)

	#define xp_memset(dst,fill,len) RtlFillMemory(dst,len,fill)
	#define xp_memcpy(dst,src,len) RtlCopyMemory(dst,src,len)
	#define xp_memcmp(src1,src2,len) RtlCompareMemory(src1,src2,len);
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <stdarg.h>
	#include <assert.h>

	#ifdef XP_CHAR_IS_MCHAR
		#include <ctype.h>
	#else
		#include <ctype.h>
		#include <wchar.h>
		#if !defined(__BEOS__)
			#include <wctype.h>
		#endif
	#endif

	#define xp_assert assert

	#define xp_malloc malloc
	#define xp_calloc calloc
	#define xp_realloc realloc
	#define xp_free free

	#define xp_memset(dst,fill,len)  memset(dst,fill,len)
	#define xp_memcpy(dst,src,len)   memcpy(dst,src,len)
	#define xp_memcmp(src1,src2,len) memcmp(src1,src2,len);

	#ifdef XP_CHAR_IS_MCHAR
		#define xp_isdigit isdigit
		#define xp_isxdigit isxdigit
		#define xp_isalpha isalpha
		#define xp_isalnum isalnum
		#define xp_isspace isspace
		#define xp_iscntrl iscntrl
		#define xp_isgraph isgraph
		#define xp_islower islower
		#define xp_isupper isupper
		#define xp_isprint isprint
		#define xp_ispunct ispunct
		#define xp_toupper toupper
		#define xp_tolower tolower
	#else
		#define xp_isdigit iswdigit
		#define xp_isxdigit iswxdigit
		#define xp_isalpha iswalpha
		#define xp_isalnum iswalnum
		#define xp_isspace iswspace
		#define xp_iscntrl iswcntrl
		#define xp_isgraph iswgraph
		#define xp_islower iswlower
		#define xp_isupper iswupper
		#define xp_isprint iswprint
		#define xp_ispunct iswpunct
		#define xp_toupper towupper
		#define xp_tolower towlower
	#endif

#endif

#define xp_va_start(pvar,param) va_start(pvar,param)
#define xp_va_list va_list
#define xp_va_end(pvar)      va_end(pvar)
#define xp_va_arg(pvar,type) va_arg(pvar,type)

#define XP_STR_LEN(x)  ((x)->size)
#define XP_STR_SIZE(x) ((x)->size + 1)
#define XP_STR_CAPA(x) ((x)->capa)
#define XP_STR_BUF(x)  ((x)->buf)
#define XP_STR_CHAR(x,idx) ((x)->buf[idx])

typedef struct xp_str_t xp_str_t;

struct xp_str_t
{
	xp_char_t* buf;
	xp_size_t size;
	xp_size_t capa;
	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

#define xp_strlen xp_awk_strlen
xp_size_t xp_strlen (const xp_char_t* str);

#define xp_strdup xp_awk_strdup
xp_char_t* xp_strdup (const xp_char_t* str);

#define xp_strxdup xp_awk_strxdup
xp_char_t* xp_strxdup (const xp_char_t* str, xp_size_t len);

#define xp_strxdup2 xp_awk_strxdup2
xp_char_t* xp_strxdup2 (
	const xp_char_t* str1, xp_size_t len1,
	const xp_char_t* str2, xp_size_t len2);

#define xp_strcpy xp_awk_strcpy
xp_size_t xp_strcpy (xp_char_t* buf, const xp_char_t* str);

#define xp_strncpy xp_awk_strncpy
xp_size_t xp_strncpy (xp_char_t* buf, const xp_char_t* str, xp_size_t len);

#define xp_strcmp xp_awk_strcmp
int xp_strcmp (const xp_char_t* s1, const xp_char_t* s2);

#define xp_strxncmp xp_awk_strxncmp
int xp_strxncmp (
	const xp_char_t* s1, xp_size_t len1, 
	const xp_char_t* s2, xp_size_t len2);

#define xp_strxnstr xp_awk_strxnstr
xp_char_t* xp_strxnstr (
	const xp_char_t* str, xp_size_t strsz, 
	const xp_char_t* sub, xp_size_t subsz);

#define xp_strtok xp_awk_strtok
xp_char_t* xp_strtok (const xp_char_t* s, 
	const xp_char_t* delim, xp_char_t** tok, xp_size_t* tok_len);

#define xp_strxtok xp_awk_strxtok
xp_char_t* xp_strxtok (const xp_char_t* s, xp_size_t len,
	const xp_char_t* delim, xp_char_t** tok, xp_size_t* tok_len);

#define xp_printf xp_awk_printf
int xp_printf (const xp_char_t* fmt, ...);

#define xp_vprintf xp_awk_vprintf
int xp_vprintf (const xp_char_t* fmt, xp_va_list ap);

#define xp_sprintf xp_awk_sprintf
int xp_sprintf (
	xp_char_t* buf, xp_size_t size, const xp_char_t* fmt, ...);

#define xp_vsprintf xp_awk_vsprintf
int xp_vsprintf (
	xp_char_t* buf, xp_size_t size, const xp_char_t* fmt, xp_va_list ap);

#define xp_str_open xp_awk_str_open
xp_str_t* xp_str_open (xp_str_t* str, xp_size_t capa);

#define xp_str_close xp_awk_str_close
void xp_str_close (xp_str_t* str);

#define xp_str_forfeit xp_awk_str_forfeit
void xp_str_forfeit (xp_str_t* str);

#define xp_str_cpy xp_awk_str_cpy
xp_size_t xp_str_cpy (xp_str_t* str, const xp_char_t* s);

#define xp_str_ncpy xp_awk_str_ncpy
xp_size_t xp_str_ncpy (xp_str_t* str, const xp_char_t* s, xp_size_t len);

#define xp_str_cat xp_awk_str_cat
xp_size_t xp_str_cat (xp_str_t* str, const xp_char_t* s);

#define xp_str_ncat xp_awk_str_ncat
xp_size_t xp_str_ncat (xp_str_t* str, const xp_char_t* s, xp_size_t len);

#define xp_str_ccat xp_awk_str_ccat
xp_size_t xp_str_ccat (xp_str_t* str, xp_char_t c);

#define xp_str_nccat xp_awk_str_nccat
xp_size_t xp_str_nccat (xp_str_t* str, xp_char_t c, xp_size_t len);

#define xp_str_clear xp_awk_str_clear
void xp_str_clear (xp_str_t* str);

#ifdef __cplusplus
}
#endif

#endif

#endif
