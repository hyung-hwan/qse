/*
 * $Id: str.h 407 2011-03-23 02:21:14Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_CMN_STR_H_
#define _QSE_CMN_STR_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides various functions, types, macros for string manipulation.
 *
 * The #qse_cstr_t type and the #qse_xstr_t defined in <qse/types.h> help you
 * deal with a string pointer and length in a structure.
 *
 */

#define QSE_STR_LEN(s)       ((const qse_size_t)(s)->len) /**< string length */
#define QSE_STR_PTR(s)       ((qse_char_t* const)(s)->ptr) /**< string/buffer pointer */
#define QSE_STR_CAPA(s)      ((qse_size_t)(s)->capa) /**< buffer capacity */
#define QSE_STR_CHAR(s,idx)  ((s)->ptr[idx]) /**< character at given position */

typedef struct qse_str_t qse_str_t;

typedef qse_size_t (*qse_str_sizer_t) (
	qse_str_t* data,
	qse_size_t hint
);

/**
 * The qse_str_t type defines a dynamically resizable string.
 */
struct qse_str_t
{
	QSE_DEFINE_COMMON_FIELDS (str)
	qse_str_sizer_t sizer; /**< buffer resizer function */
	qse_char_t*     ptr;   /**< buffer/string pointer */
	qse_size_t      len;   /**< string length */
	qse_size_t      capa;  /**< buffer capacity */
};

/**
 * The qse_strxsubst_subst_t type defines a callback function
 * for qse_strxsubst() to substitue a new value for an identifier @a ident.
 */
typedef qse_char_t* (*qse_strxsubst_subst_t) (
	qse_char_t*       buf, 
	qse_size_t        bsz, 
	const qse_cstr_t* ident, 
	void*             ctx
);


/* int qse_chartonum (qse_char_t c, int base) */
#define QSE_CHARTONUM(c,base) \
	((c>=QSE_T('0') && c<=QSE_T('9'))? ((c-QSE_T('0')<base)? (c-QSE_T('0')): base): \
	 (c>=QSE_T('A') && c<=QSE_T('Z'))? ((c-QSE_T('A')+10<base)? (c-QSE_T('A')+10): base): \
	 (c>=QSE_T('a') && c<=QSE_T('z'))? ((c-QSE_T('a')+10<base)? (c-QSE_T('a')+10): base): base)

/* qse_strtonum (const qse_char_t* nptr, qse_char_t** endptr, int base) */
#define QSE_STRTONUM(value,nptr,endptr,base) \
{ \
	int __ston_f = 0, __ston_v; \
	const qse_char_t* __ston_ptr = nptr; \
	for (;;) { \
		qse_char_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_T(' ') || \
		    __ston_c == QSE_T('\t')) { __ston_ptr++; continue; } \
		if (__ston_c == QSE_T('-')) { __ston_f++; __ston_ptr++; } \
		if (__ston_c == QSE_T('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; (__ston_v = QSE_CHARTONUM(*__ston_ptr, base)) < base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr != QSE_NULL) *((const qse_char_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
}

/* qse_strxtonum (const qse_char_t* nptr, qse_size_t len, qse_char_char** endptr, int base) */
#define QSE_STRXTONUM(value,nptr,len,endptr,base) \
{ \
	int __ston_f = 0, __ston_v; \
	const qse_char_t* __ston_ptr = nptr; \
	const qse_char_t* __ston_end = __ston_ptr + len; \
	value = 0; \
	while (__ston_ptr < __ston_end) { \
		qse_char_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_T(' ') || __ston_c == QSE_T('\t')) { \
			__ston_ptr++; continue; \
		} \
		if (__ston_c == QSE_T('-')) { __ston_f++; __ston_ptr++; } \
		if (__ston_c == QSE_T('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; __ston_ptr < __ston_end && \
	               (__ston_v = QSE_CHARTONUM(*__ston_ptr, base)) != base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr != QSE_NULL) *((const qse_char_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
}

/**
 * The qse_strtrmx_op_t defines a string trimming operation. 
 */
enum qse_strtrmx_op_t
{
	QSE_STRTRMX_LEFT  = (1 << 0), /**< trim leading spaces */
	QSE_STRTRMX_RIGHT = (1 << 1)  /**< trim trailing spaces */
};

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * basic string functions
 */

/**
 * The qse_mbslen() function returns the number of characters in a 
 * multibyte null-terminated string. The length returned excludes a 
 * terminating null.
 */
qse_size_t qse_mbslen (
	const qse_mchar_t* mbs
);

/**
 * The qse_wcslen() function returns the number of characters in a 
 * wide-character null-terminated string. The length returned excludes 
 * a terminating null.
 */
qse_size_t qse_wcslen (
	const qse_wchar_t* wcs
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strlen(str) qse_mbslen(str)
#else
#	define qse_strlen(str) qse_wcslen(str)
#endif

/**
 * The qse_strbytes() function returns the number of bytes a null-terminated
 * string is holding excluding a terminating null.
 */
qse_size_t qse_strbytes (
	const qse_char_t* str
);

qse_size_t qse_mbscpy (
	qse_mchar_t*       buf,
	const qse_mchar_t* str
);

qse_size_t qse_wcscpy (
	qse_wchar_t*       buf,
	const qse_wchar_t* str
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strcpy(buf,str) qse_mbscpy(buf,str)
#else
#	define qse_strcpy(buf,str) qse_wcscpy(buf,str)
#endif

qse_size_t qse_strxcpy (
	qse_char_t*       buf,
	qse_size_t        bsz,
	const qse_char_t* str
);

/**
 * The qse_strycpy() function copies a length-bounded string into
 * a buffer with unknown size. 
 */
qse_size_t qse_strncpy (
	qse_char_t*       buf, /**< buffer with unknown length */
	const qse_char_t* str, /**< length-bounded string */
	qse_size_t        len  /**< string length */
);

/**
 * The qse_strycpy() function copies a length-bounded string into
 * a length-bounded buffer.
 */
qse_size_t qse_strxncpy (
	qse_char_t*       buf, /**< length-bounded buffer */
	qse_size_t        bsz, /**< buffer length */
	const qse_char_t* str, /**< length-bounded string */
	qse_size_t        len  /**< string length */
);

/**
 * The qse_strxput() function copies the string @a str into the buffer @a buf
 * of the size @a bsz. Unlike qse_strxcpy(), it does not null-terminate the
 * buffer.
 */
qse_size_t qse_strxput (
	qse_char_t*       buf, 
	qse_size_t        bsz,
	const qse_char_t* str
);

qse_size_t qse_strxnput (
	qse_char_t*       buf,
	qse_size_t        bsz,
	const qse_char_t* str,
	qse_size_t        len
);

/**
 * The qse_strfcpy() function formats a string by position.
 * The position specifier is a number enclosed in ${ and }.
 * When ${ is preceeded by a backslash, it is treated literally. 
 * See the example below:
 * @code
 *  qse_char_t buf[256]
 *  qse_char_t* colors[] = { QSE_T("blue"), QSE_T("green"), QSE_T("red") };
 *  qse_strfcpy(buf, QSE_T("RGB: ${2}, ${1}, ${0}"), colors);
 * @endcode
 * @sa qse_strfncpy, qse_strxfcpy, qse_strxfncpy
 */
qse_size_t qse_strfcpy (
	qse_char_t*       buf,
	const qse_char_t* fmt,
	const qse_char_t* str[]
);

/**
 * The qse_strfncpy() function formats a string by position.
 * It differs from qse_strfcpy() in that @a str is an array of the 
 * #qse_cstr_t type.
 * @sa qse_strfcpy, qse_strxfcpy, qse_strxfncpy
 */
qse_size_t qse_strfncpy (
	qse_char_t*       buf,
	const qse_char_t* fmt, 
	const qse_cstr_t  str[]
);

/**
 * The qse_strxfcpy() function formats a string by position.
 * It differs from qse_strfcpy() in that @a buf is length-bounded of @a bsz
 * characters.
 * @code
 *  qse_char_t buf[256]
 *  qse_char_t* colors[] = { QSE_T("blue"), QSE_T("green"), QSE_T("red") };
 *  qse_strxfcpy(buf, QSE_COUNTOF(buf), QSE_T("RGB: ${2}, ${1}, ${0}"), colors);
 * @endcode
 * @sa qse_strfcpy, qse_strfncpy, qse_strxfncpy
 */
qse_size_t qse_strxfcpy (
	qse_char_t*       buf,
	qse_size_t        bsz,
	const qse_char_t* fmt,
	const qse_char_t* str[]
);

/**
 * The qse_strxfncpy() function formats a string by position.
 * It differs from qse_strfcpy() in that @a buf is length-bounded of @a bsz
 * characters and @a str is an array of the #qse_cstr_t type.
 * @sa qse_strfcpy, qse_strfncpy, qse_strxfcpy
 */
qse_size_t qse_strxfncpy (
	qse_char_t*       buf,
	qse_size_t        bsz,
	const qse_char_t* fmt,
	const qse_cstr_t  str[]
);

/**
 * The qse_strxsubst() function expands @a fmt into a buffer @a buf of the size
 * @a bsz by substituting new values for ${} segments within it. The actual
 * substitution is made by invoking the callback function @a subst. 
 * @code
 * qse_char_t* subst (qse_char_t* buf, qse_size_t bsz, const qse_cstr_t* ident, void* ctx)
 * { 
 *   if (qse_strxcmp (ident->ptr, ident->len, QSE_T("USER")) == 0)
 *     return buf + qse_strxput (buf, bsz, QSE_T("sam"));	
 *   else if (qse_strxcmp (ident->ptr, ident->len, QSE_T("GROUP")) == 0)
 *     return buf + qse_strxput (buf, bsz, QSE_T("coders"));	
 *   return buf; 
 * }
 * 
 * qse_char_t buf[25];
 * qse_strxsubst (buf, i, QSE_T("user=${USER},group=${GROUP}"), subst, QSE_NULL);
 * @endcode
 */
qse_size_t qse_strxsubst (
	qse_char_t*            buf,
	qse_size_t             bsz,
	const qse_char_t*      fmt,
	qse_strxsubst_subst_t  subst,
	void*                  ctx
);

qse_size_t qse_strxcat (
	qse_char_t*       buf,
	qse_size_t        bsz, 
	const qse_char_t* str
);

qse_size_t qse_strxncat (
	qse_char_t*       buf,
	qse_size_t        bsz,
	const qse_char_t* str,
	qse_size_t        len
);

int qse_strcmp (
	const qse_char_t* s1,
	const qse_char_t* s2
);

int qse_strxcmp (
	const qse_char_t* s1,
	qse_size_t        len1,
	const qse_char_t* s2
);

int qse_strxncmp (
	const qse_char_t* s1,
	qse_size_t        len1, 
	const qse_char_t* s2,
	qse_size_t        len2
);

int qse_strcasecmp (
	const qse_char_t* s1,
	const qse_char_t* s2
);

int qse_strxcasecmp (
	const qse_char_t* s1,
	qse_size_t        len,
	const qse_char_t* s2
);

/**
 * The qse_strxncasecmp() function compares characters at the same position 
 * in each string after converting them to the same case temporarily. 
 * It accepts two strings and a character class handler. A string is 
 * represented by its beginning pointer and length. 
 *
 * For two strings to be equal, they need to have the same length and all
 * characters in the first string must be equal to their counterpart in the
 * second string.
 *
 * The following code snippet compares "foo" and "FoO" case-insenstively.
 * @code
 * qse_strxncasecmp (QSE_T("foo"), 3, QSE_T("FoO"), 3);
 * @endcode
 *
 * @return 0 if two strings are equal, 
 *         a positive number if the first string is larger, 
 *         -1 if the second string is larger.
 *
 */
int qse_strxncasecmp (
	const qse_char_t* s1,   /**< pointer to the first string */
	qse_size_t        len1, /**< length of the first string */ 
	const qse_char_t* s2,   /**< pointer to the second string */
	qse_size_t        len2  /**< length of the second string */
);

qse_char_t* qse_strdup (
	const qse_char_t* str,
	qse_mmgr_t*       mmgr
);

qse_char_t* qse_strdup2 (
	const qse_char_t* str1,
	const qse_char_t* str2,
	qse_mmgr_t*       mmgr
);

qse_char_t* qse_strxdup (
	const qse_char_t* str,
	qse_size_t        len, 
	qse_mmgr_t*       mmgr
);

qse_char_t* qse_strxdup2 (
	const qse_char_t* str1,
	qse_size_t        len1,
	const qse_char_t* str2,
	qse_size_t        len2,
	qse_mmgr_t*       mmgr
);

/**
 * The qse_mbsstr() function searchs a string @a str for the first occurrence 
 * of a substring @a sub.
 * @return pointer to the first occurrence in @a str if @a sub is found, 
 *         QSE_NULL if not.
 */
qse_mchar_t* qse_mbsstr (
	const qse_mchar_t* str, 
	const qse_mchar_t* sub
);

/**
 * The qse_wcsstr() function searchs a string @a str for the first occurrence 
 * of a substring @a sub.
 * @return pointer to the first occurrence in @a str if @a sub is found, 
 *         QSE_NULL if not.
 */
qse_wchar_t* qse_wcsstr (
	const qse_wchar_t* str, 
	const qse_wchar_t* sub
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strstr(str,sub) qse_mbsstr(str,sub)
#else
#	define qse_strstr(str,sub) qse_wcsstr(str,sub)
#endif

qse_char_t* qse_strxstr (
	const qse_char_t* str,
	qse_size_t        size,
	const qse_char_t* sub
);

qse_char_t* qse_strxnstr (
	const qse_char_t* str,
	qse_size_t        strsz, 
	const qse_char_t* sub,
	qse_size_t        subsz
);

qse_char_t* qse_strcasestr (
	const qse_char_t* str, 
	const qse_char_t* sub
);

qse_char_t* qse_strxcasestr (
	const qse_char_t* str,
	qse_size_t        size,
	const qse_char_t* sub
);

qse_char_t* qse_strxncasestr (
	const qse_char_t* str,
	qse_size_t        strsz, 
	const qse_char_t* sub,
	qse_size_t        subsz
);

/**
 * The qse_strrstr() function searchs a string @a str for the last occurrence 
 * of a substring @a sub.
 * @return pointer to the last occurrence in @a str if @a sub is found, 
 *         QSE_NULL if not.
 */
qse_char_t* qse_strrstr (
	const qse_char_t* str,
	const qse_char_t* sub
);

qse_char_t* qse_strxrstr (
	const qse_char_t* str,
	qse_size_t        size,
	const qse_char_t* sub
);

qse_char_t* qse_strxnrstr (
	const qse_char_t* str,
	qse_size_t        strsz, 
	const qse_char_t* sub,
	qse_size_t        subsz
);

qse_mchar_t* qse_mbschr (
	const qse_mchar_t* str,
	qse_mcint_t        c
);

qse_wchar_t* qse_wcschr (
	const qse_wchar_t* str,
	qse_wcint_t        c
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strchr(str,c) qse_mbschr(str,c)
#else
#	define qse_strchr(str,c) qse_wcschr(str,c)
#endif

qse_char_t* qse_strxchr (
	const qse_char_t* str,
	qse_size_t len,
	qse_cint_t c
);

qse_char_t* qse_strrchr (
	const qse_char_t* str,
	qse_cint_t c
);

qse_char_t* qse_strxrchr (
	const qse_char_t* str,
	qse_size_t len,
	qse_cint_t c
);

/**
 * The qse_strxword() function finds a whole word in a string.
 */
const qse_char_t* qse_strxword (
     const qse_char_t* str,
	qse_size_t        len,
	const qse_char_t* word
);

/**
 * The qse_strxcaseword() function finds a whole word in a string 
 * case-insensitively.
 */
const qse_char_t* qse_strxcaseword (
     const qse_char_t* str,
	qse_size_t        len,
	const qse_char_t* word
);

/**
 * The qse_strbeg() function checks if the a string begins with a substring.
 * @return the pointer to a beginning of a matching beginning, 
 *         QSE_NULL if no match is found.
 */
qse_char_t* qse_strbeg (
	const qse_char_t* str,
	const qse_char_t* sub
);

qse_char_t* qse_strxbeg (
	const qse_char_t* str,
	qse_size_t len,
	const qse_char_t* sub)
;

/*
 * The qse_strbeg() function checks if the a string begins with a substring.
 * @return @a str on match, QSE_NULL on no match
 */
qse_char_t* qse_strnbeg (
	const qse_char_t* str,
	const qse_char_t* sub,
	qse_size_t        len
);

/*
 * The qse_strbeg() function checks if the a string begins with a substring.
 * @return @a str on match, QSE_NULL on no match
 */
qse_char_t* qse_strxnbeg (
	const qse_char_t* str,
	qse_size_t        len1, 
	const qse_char_t* sub,
	qse_size_t        len2
);

/**
 * The qse_strend() function checks if the a string ends with a substring.
 * @return the pointer to a beginning of a matching end, 
 *         QSE_NULL if no match is found.
 */
qse_char_t* qse_strend (
	const qse_char_t* str, /**< a string */
	const qse_char_t* sub  /**< a substring */
);

/**
 * The qse_strxend function checks if the a string ends with a substring.
 * @return the pointer to a beginning of a matching end, 
 *         QSE_NULL if no match is found.
 */
qse_char_t* qse_strxend (
	const qse_char_t* str,
	qse_size_t        len,
	const qse_char_t* sub
);

/**
 * The qse_strnend() function checks if the a string ends with a substring.
 * @return the pointer to a beginning of a matching end, 
 *         QSE_NULL if no match is found.
 */
qse_char_t* qse_strnend (
	const qse_char_t* str,
	const qse_char_t* sub,
	qse_size_t len
);

/**
 * The qse_strxnend() function checks if the a string ends with a substring.
 * @return the pointer to a beginning of a matching end, 
 *         QSE_NULL if no match is found.
 */
qse_char_t* qse_strxnend (
	const qse_char_t* str,
	qse_size_t        len1, 
	const qse_char_t* sub,
	qse_size_t        len2
);

qse_size_t qse_mbsspn (
	const qse_mchar_t* str1,
	const qse_mchar_t* str2
);

qse_size_t qse_wcsspn (
	const qse_wchar_t* str1,
	const qse_wchar_t* str2
);

qse_size_t qse_mbscspn (
	const qse_mchar_t* str1,
	const qse_mchar_t* str2
);

qse_size_t qse_wcscspn (
	const qse_wchar_t* str1,
	const qse_wchar_t* str2
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strspn(str1,str2) qse_mbsspn(str1,str2)
#	define qse_strcspn(str1,str2) qse_mbscspn(str1,str2)
#else
#	define qse_strspn(str1,str2) qse_wcsspn(str1,str2)
#	define qse_strcspn(str1,str2) qse_wcscspn(str1,str2)
#endif

/* 
 * string conversion
 */
int qse_strtoi (
	const qse_char_t* str
);

long qse_strtol (
	const qse_char_t* str
);

unsigned int qse_strtoui (
	const qse_char_t* str
);

unsigned long qse_strtoul (
	const qse_char_t* str
);

int qse_strxtoi (
	const qse_char_t* str,
	qse_size_t        len
);

long qse_strxtol (
	const qse_char_t* str,
	qse_size_t        len
);

unsigned int qse_strxtoui (
	const qse_char_t* str,
	qse_size_t        len
);

unsigned long qse_strxtoul (
	const qse_char_t* str,
	qse_size_t        len
);

qse_int_t qse_strtoint (
	const qse_char_t* str
);

qse_long_t qse_strtolong (
	const qse_char_t* str
);

qse_uint_t qse_strtouint (
	const qse_char_t* str
);

qse_ulong_t qse_strtoulong (
	const qse_char_t* str
);

qse_int_t qse_strxtoint (
	const qse_char_t* str, qse_size_t len
);

qse_long_t qse_strxtolong (
	const qse_char_t* str,
	qse_size_t        len
);

qse_uint_t qse_strxtouint (
	const qse_char_t* str,
	qse_size_t        len
);

qse_ulong_t qse_strxtoulong (
	const qse_char_t* str,
	qse_size_t        len
);

/**
 * The qse_strspl() function splits a string into fields.
 */
int qse_strspl (
	qse_char_t*       str,
	const qse_char_t* delim,
	qse_char_t        lquote,
	qse_char_t        rquote,
	qse_char_t        escape
);

/**
 * The qse_strspltrn() function splits a string translating special 
 * escape sequences.
 * The argument @a trset is a translation character set which is composed
 * of multiple character pairs. An escape character followed by the 
 * first character in a pair is translated into the second character
 * in the pair. If trset is QSE_NULL, no translation is performed. 
 *
 * Let's translate a sequence of '\n' and '\r' to a new line and a carriage
 * return respectively.
 * @code
 *   qse_strspltrn (str, QSE_T(':'), QSE_T('['), QSE_T(']'), QSE_T('\\'), QSE_T("n\nr\r"), &nfields);
 * @endcode
 * Given [xxx]:[\rabc\ndef]:[] as an input, the example breaks the second 
 * fields to <CR>abc<NL>def where <CR> is a carriage return and <NL> is a 
 * new line.
 *
 * If you don't need any translation, you may call qse_strspl() alternatively.
 */
int qse_strspltrn (
	qse_char_t*       str,
	const qse_char_t* delim,
	qse_char_t        lquote,
	qse_char_t        rquote,
	qse_char_t        escape,
	const qse_char_t* trset
);

/**
 * The qse_strtrmx() function strips leading spaces and/or trailing
 * spaces off a string depending on the opt parameter. You can form
 * the op parameter by bitwise-OR'ing one or more of the following
 * values:
 *
 * - QSE_STRTRMX_LEFT - trim leading spaces
 * - QSE_STRTRMX_RIGHT - trim trailing spaces
 *
 * Should it remove leading spaces, it just returns the pointer to
 * the first non-space character in the string. Should it remove trailing
 * spaces, it inserts a QSE_T('\0') character after the last non-space
 * characters. Take note of this behavior.
 *
 * @code
 * qse_char_t a[] = QSE_T("   this is a test string   ");
 * qse_printf (QSE_T("[%s]\n"), qse_strtrmx(a,QSE_STRTRMX_LEFT|QSE_STRTRMX_RIGHT));
 * @endcode
 *
 * @return pointer to a trimmed string.
 */
qse_char_t* qse_strtrmx (
	qse_char_t* str, /**< a string */
	int         op   /**< operation code XOR'ed of qse_strtrmx_op_t values */
);

/**
 * The qse_strtrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by @a str; QSE_T('\0') is inserted after the last non-space character.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_strtrm (
	qse_char_t* str /**< string */
);

/**
 * The qse_strxtrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by @a str; QSE_T('\0') is inserted after the last non-space character.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_strxtrm (
	qse_char_t* str, /**< string */
	qse_size_t  len  /**< length */
);

/**
 * The qse_strpac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_strpac (
	qse_char_t* str /**< string */
);

/**
 * The qse_strxpac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_strxpac (
	qse_char_t* str, /**< string */
	qse_size_t  len  /**< length */
);

/**
 * The qse_mbstowcslen() function scans a null-terminated multibyte string
 * to calculate the number of wide characters it can be converted to.
 * The number of wide characters is returned via @a wcslen if it is not 
 * #QSE_NULL. The function may be aborted if it has encountered invalid
 * or incomplete multibyte sequences. The return value, in this case, 
 * is less than qse_strlen(mcs).
 * @return number of bytes scanned
 */
qse_size_t qse_mbstowcslen (
	const qse_mchar_t* mcs,
	qse_size_t*        wcslen
);

/**
 * The qse_mbsntowcsnlen() function scans a multibyte string of @a mcslen bytes
 * to get the number of wide characters it can be converted to.
 * The number of wide characters is returned via @a wcslen if it is not 
 * #QSE_NULL. The function may be aborted if it has encountered invalid
 * or incomplete multibyte sequences. The return value, in this case, 
 * is less than @a mcslen.
 * @return number of bytes scanned
 */
qse_size_t qse_mbsntowcsnlen (
	const qse_mchar_t* mcs,
	qse_size_t         mcslen,
	qse_size_t*        wcslen
);

/**
 * The qse_mbstowcs() function converts a multibyte string to a wide 
 * character string.
 *
 * @code
 *  const qse_mchar_t* mbs = "a multibyte string";
 *  qse_wchar_t buf[100];
 *  qse_size_t bufsz = QSE_COUNTOF(buf), n;
 *  n = qse_mbstowcs (mbs, buf, bufsz);
 *  if (bufsz >= QSE_COUNTOF(buf)) { buffer too small }
 *  if (mbs[n] != '\0') { incomplete processing  }
 *  //if (n != strlen(mbs)) { incomplete processing  }
 * @endcode
 *
 * @return number of multibyte characters processed.
 */
qse_size_t qse_mbstowcs (
	const qse_mchar_t* mbs,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);

/**
 * The qse_mbsntowcsn() function converts a multibyte string to a 
 * wide character string.
 * @return number of multibyte characters processed.
 */
qse_size_t qse_mbsntowcsn (
	const qse_mchar_t* mbs,
	qse_size_t         mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);

/**
 * The qse_wcstombslen() function scans a null-terminated wide character 
 * string @a wcs to get the total number of multibyte characters that it 
 * can be converted to. The resulting number of characters is stored into 
 * memory pointed to by @a mbslen.
 * Complete scanning is indicated by the following condition:
 * @code
 *  qse_wcstombslen(wcs,&xx) == qse_strlen(wcs)
 * @endcode
 * @return number of wide characters handled
 */
qse_size_t qse_wcstombslen (
	const qse_wchar_t* wcs,
	qse_size_t*        mbslen
);

/**
 * The qse_wcsntombsnlen() function scans a wide character wcs as long as
 * @a wcslen characters to get the total number of multibyte characters 
 * that it can be converted to. The resulting number of characters is stored 
 * into memory pointed to by @a mbslen.
 * Complete scanning is indicated by the following condition:
 * @code
 *  qse_wcstombslen(wcs,&xx) == wcslen
 * @endcode
 * @return number of wide characters handled
 */
qse_size_t qse_wcsntombsnlen (
	const qse_wchar_t* wcs,
	qse_size_t         wcslen,
	qse_size_t*        mbslen
);

/**
 * The qse_wcstombs() function converts a null-terminated wide character 
 * string to a multibyte string and stores it into the buffer pointed to
 * by mbs. The pointer to a variable holding the buffer length should be
 * passed to the function as the third parameter. After conversion, it holds 
 * the length of the multibyte string excluding the terminating-null character.
 * It may not null-terminate the resulting multibyte string if the buffer
 * is not large enough. You can check if the resulting mbslen is equal to 
 * the input mbslen to know it.
 * @return number of wide characters processed
 */
qse_size_t qse_wcstombs (
	const qse_wchar_t* wcs,   /**< wide-character string to convert */
	qse_mchar_t*       mbs,   /**< multibyte string buffer */
	qse_size_t*        mbslen /**< [IN] buffer size, [OUT] string length */
);

/**
 * The qse_wcsntombsn() function converts a wide character string to a
 * multibyte string.
 * @return the number of wide characters
 */
qse_size_t qse_wcsntombsn (
	const qse_wchar_t* wcs,   /**< wide string */
	qse_size_t         wcslen,/**< wide string length */
	qse_mchar_t*       mbs,   /**< multibyte string buffer */
	qse_size_t*        mbslen /**< [IN] buffer size, [OUT] string length */
);

/**
 * The qse_mbstowcs_strict() function performs the same as the qse_mbstowcs() 
 * function except that it returns an error if it can't fully convert the
 * input string and/or the buffer is not large enough.
 * @return 0 on success, -1 on failure.
 */
int qse_mbstowcs_strict (
	const qse_mchar_t* mbs,
	qse_wchar_t*       wcs,
	qse_size_t         wcslen
);

/**
 * The qse_wcstombs_strict() function performs the same as the qse_wcstombs() 
 * function except that it returns an error if it can't fully convert the
 * input string and/or the buffer is not large enough.
 * @return 0 on success, -1 on failure.
 */
int qse_wcstombs_strict (
	const qse_wchar_t* wcs,
	qse_mchar_t*       mbs,
	qse_size_t         mbslen
);

QSE_DEFINE_COMMON_FUNCTIONS (str)

qse_str_t* qse_str_open (
	qse_mmgr_t* mmgr,
	qse_size_t ext,
	qse_size_t capa
);

void qse_str_close (
	qse_str_t* str
);

/**
 * The qse_str_init() function initializes a dynamically resizable string
 * If the parameter capa is 0, it doesn't allocate the internal buffer 
 * in advance.
 */
qse_str_t* qse_str_init (
	qse_str_t*  str,
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);

/**
 * The qse_str_fini() function finalizes a dynamically resizable string.
 */
void qse_str_fini (
	qse_str_t* str
);

/**
 * The qse_str_yield() function assigns the buffer to an variable of the
 * qse_xstr_t type and recreate a new buffer of the @a new_capa capacity.
 * The function fails if it fails to allocate a new buffer.
 * @return 0 on success, and -1 on failure.
 */
int qse_str_yield (
	qse_str_t*  str,     /**< string */
	qse_xstr_t* buf,     /**< buffer pointer */
	int         new_capa /**< new capacity */
);

/**
 * The qse_str_getsizer() function gets the sizer.
 * @return sizer function set or QSE_NULL if no sizer is set.
 */
qse_str_sizer_t qse_str_getsizer (
	qse_str_t* str
);

/**
 * The qse_str_setsizer() function specify a new sizer for a dynamic string.
 * With no sizer specified, the dynamic string doubles the current buffer
 * when it needs to increase its size. The sizer function is passed a dynamic
 * string and the minimum capacity required to hold data after resizing.
 * The string is truncated if the sizer function returns a smaller number
 * than the hint passed.
 */
void qse_str_setsizer (
	qse_str_t*      str,
	qse_str_sizer_t sizer
);
/******/

/**
 * The qse_str_getcapa() function returns the current capacity.
 * You may use QSE_STR_CAPA(str) macro for performance sake.
 * @return current capacity in number of characters.
 */
qse_size_t qse_str_getcapa (
	qse_str_t* str
);

/**
 * The qse_str_setcapa() function sets the new capacity. If the new capacity
 * is smaller than the old, the overflowing characters are removed from
 * from the buffer.
 * @return (qse_size_t)-1 on failure, new capacity on success 
 */
qse_size_t qse_str_setcapa (
	qse_str_t* str,
	qse_size_t capa
);

/**
 * The qse_str_getlen() function return the string length.
 */
qse_size_t qse_str_getlen (
	qse_str_t* str
);

/**
 * The qse_str_setlen() function changes the string length.
 * @return (qse_size_t)-1 on failure, new length on success 
 */
qse_size_t qse_str_setlen (
	qse_str_t* str,
	qse_size_t len
);

/**
 * The qse_str_clear() funtion deletes all characters in a string and sets
 * the length to 0. It doesn't resize the internal buffer.
 */
void qse_str_clear (
	qse_str_t* str
);

/**
 * The qse_str_swap() function exchanges the pointers to a buffer between
 * two strings. It updates the length and the capacity accordingly.
 */
void qse_str_swap (
	qse_str_t* str1,
	qse_str_t* str2
);

qse_size_t qse_str_cpy (
	qse_str_t*        str,
	const qse_char_t* s
);

qse_size_t qse_str_ncpy (
	qse_str_t*        str,
	const qse_char_t* s,
	qse_size_t        len
);

qse_size_t qse_str_cat (
	qse_str_t*        str,
	const qse_char_t* s
);

qse_size_t qse_str_ncat (
	qse_str_t*        str,
	const qse_char_t* s,
	qse_size_t        len
);

qse_size_t qse_str_ccat (
	qse_str_t* str,
	qse_char_t c
);

qse_size_t qse_str_nccat (
	qse_str_t* str,
	qse_char_t c,
	qse_size_t len
);

qse_size_t qse_str_del (
	qse_str_t* str,
	qse_size_t index,
	qse_size_t size
);

qse_size_t qse_str_trm (
	qse_str_t* str
);

qse_size_t qse_str_pac (
	qse_str_t* str
);

#ifdef __cplusplus
}
#endif

#endif
