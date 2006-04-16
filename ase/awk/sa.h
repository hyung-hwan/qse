/*
 * $Id: sa.h,v 1.22 2006-04-16 04:31:38 bacon Exp $
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

#define xp_malloc malloc
#define xp_realloc realloc
#define xp_free free
#define xp_assert assert

#ifdef XP_CHAR_IS_MCHAR
#define xp_isdigit isdigit
#define xp_isxdigit isxdigit
#define xp_isalpha isalpha
#define xp_isalnum isalnum
#define xp_isspace isspace
#define xp_toupper toupper
#define xp_tolower tolower
#else
#define xp_isdigit iswdigit
#define xp_isxdigit iswxdigit
#define xp_isalpha iswalpha
#define xp_isalnum iswalnum
#define xp_isspace iswspace
#define xp_toupper towupper
#define xp_tolower towlower
#endif

#define xp_memcpy memcpy
#define xp_memcmp memcmp

#define xp_va_start(pvar,param) va_start(pvar,param)
#define xp_va_list va_list
#define xp_va_end(pvar)      va_end(pvar)
#define xp_va_arg(pvar,type) va_arg(pvar,type)

#define XP_STR_LEN(x)  ((x)->size)
#define XP_STR_SIZE(x) ((x)->size + 1)
#define XP_STR_CAPA(x) ((x)->capa)
#define XP_STR_BUF(x)  ((x)->buf)

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

#define xp_str_cat xp_awk_str_cat
xp_size_t xp_str_cat (xp_str_t* str, const xp_char_t* s);

#define xp_str_ncat xp_awk_str_ncat
xp_size_t xp_str_ncat (xp_str_t* str, const xp_char_t* s, xp_size_t len);

#define xp_str_ccat xp_awk_str_ccat
xp_size_t xp_str_ccat (xp_str_t* str, xp_char_t c);

#define xp_str_clear xp_awk_str_clear
void xp_str_clear (xp_str_t* str);

#ifdef __cplusplus
}
#endif

#endif

#endif
