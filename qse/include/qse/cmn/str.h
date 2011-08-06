/*
 * $Id: str.h 536 2011-08-06 03:25:08Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
 */

/** string pointer and length as a aggregate */
#define QSE_MBS_XSTR(s)     (&((s)->val))  
/** constant string pointer and length as a aggregate */
#define QSE_MBS_CSTR(s)     ((qse_mcstr_t*)&((s)->val))
/** string length */
#define QSE_MBS_LEN(s)      ((s)->val.len)
/** string pointer */
#define QSE_MBS_PTR(s)      ((s)->val.ptr)
/** string capacity */
#define QSE_MBS_CAPA(s)     ((s)->capa)
/** character at the given position */
#define QSE_MBS_CHAR(s,idx) ((s)->val.ptr[idx]) 
/**< last character. unsafe if length <= 0 */
#define QSE_MBS_LASTCHAR(s) ((s)->val.ptr[(s)->val.len-1])

/** string pointer and length as a aggregate */
#define QSE_WCS_XSTR(s)     (&((s)->val))  
/** constant string pointer and length as a aggregate */
#define QSE_WCS_CSTR(s)     ((qse_wcstr_t*)&((s)->val))
/** string length */
#define QSE_WCS_LEN(s)      ((s)->val.len)
/** string pointer */
#define QSE_WCS_PTR(s)      ((s)->val.ptr)
/** string capacity */
#define QSE_WCS_CAPA(s)     ((s)->capa)
/** character at the given position */
#define QSE_WCS_CHAR(s,idx) ((s)->val.ptr[idx]) 
/**< last character. unsafe if length <= 0 */
#define QSE_WCS_LASTCHAR(s) ((s)->val.ptr[(s)->val.len-1])

typedef struct qse_mbs_t qse_mbs_t;
typedef struct qse_wcs_t qse_wcs_t;

typedef qse_size_t (*qse_mbs_sizer_t) (
	qse_mbs_t* data,
	qse_size_t hint
);

typedef qse_size_t (*qse_wcs_sizer_t) (
	qse_wcs_t* data,
	qse_size_t hint
);

#ifdef QSE_CHAR_IS_MCHAR
#	define QSE_STR_XSTR(s)     ((qse_xstr_t*)QSE_MBS_XSTR(s))
#	define QSE_STR_CSTR(s)     ((qse_cstr_t*)QSE_MBS_XSTR(s))
#	define QSE_STR_LEN(s)      QSE_MBS_LEN(s)
#	define QSE_STR_PTR(s)      QSE_MBS_PTR(s)
#	define QSE_STR_CAPA(s)     QSE_MBS_CAPA(s)
#	define QSE_STR_CHAR(s,idx) QSE_MBS_CHAR(s,idx)
#	define QSE_STR_LASTCHAR(s) QSE_MBS_LASTCHAR(s)
#	define qse_str_t           qse_mbs_t
#	define qse_str_sizer_t     qse_mbs_sizer_t
#else
#	define QSE_STR_XSTR(s)     ((qse_xstr_t*)QSE_WCS_XSTR(s))
#	define QSE_STR_CSTR(s)     ((qse_cstr_t*)QSE_WCS_XSTR(s))
#	define QSE_STR_LEN(s)      QSE_WCS_LEN(s)
#	define QSE_STR_PTR(s)      QSE_WCS_PTR(s)
#	define QSE_STR_CAPA(s)     QSE_WCS_CAPA(s)
#	define QSE_STR_CHAR(s,idx) QSE_WCS_CHAR(s,idx)
#	define QSE_STR_LASTCHAR(s) QSE_WCS_LASTCHAR(s)
#	define qse_str_t           qse_wcs_t
#	define qse_str_sizer_t     qse_wcs_sizer_t
#endif


/**
 * The qse_mbs_t type defines a dynamically resizable multi-byte string.
 */
struct qse_mbs_t
{
	QSE_DEFINE_COMMON_FIELDS (mbs)
	qse_mbs_sizer_t sizer; /**< buffer resizer function */
	qse_mxstr_t     val;   /**< buffer/string pointer and lengh */
	qse_size_t      capa;  /**< buffer capacity */
};

/**
 * The qse_wcs_t type defines a dynamically resizable wide-character string.
 */
struct qse_wcs_t
{
	QSE_DEFINE_COMMON_FIELDS (wcs)
	qse_wcs_sizer_t sizer; /**< buffer resizer function */
	qse_wxstr_t     val;   /**< buffer/string pointer and lengh */
	qse_size_t      capa;  /**< buffer capacity */
};

/**
 * The qse_mbsxsubst_subst_t type defines a callback function
 * for qse_mbsxsubst() to substitue a new value for an identifier @a ident.
 */
typedef qse_mchar_t* (*qse_mbsxsubst_subst_t) (
	qse_mchar_t*       buf, 
	qse_size_t         bsz, 
	const qse_mcstr_t* ident, 
	void*              ctx
);

/**
 * The qse_wcsxsubst_subst_t type defines a callback function
 * for qse_wcsxsubst() to substitue a new value for an identifier @a ident.
 */
typedef qse_wchar_t* (*qse_wcsxsubst_subst_t) (
	qse_wchar_t*       buf, 
	qse_size_t         bsz, 
	const qse_wcstr_t* ident, 
	void*              ctx
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strxsubst_subst_t qse_mbsxsubst_subst_t
#else
#	define qse_strxsubst_subst_t qse_wcsxsubst_subst_t
#endif

/* int qse_chartonum (qse_char_t c, int base) */
#define QSE_CHARTONUM(c,base) \
	((c>=QSE_T('0') && c<=QSE_T('9'))? ((c-QSE_T('0')<base)? (c-QSE_T('0')): base): \
	 (c>=QSE_T('A') && c<=QSE_T('Z'))? ((c-QSE_T('A')+10<base)? (c-QSE_T('A')+10): base): \
	 (c>=QSE_T('a') && c<=QSE_T('z'))? ((c-QSE_T('a')+10<base)? (c-QSE_T('a')+10): base): base)

/* qse_strtonum (const qse_char_t* nptr, qse_char_t** endptr, int base) */
#define QSE_STRTONUM(value,nptr,endptr,base) do {\
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
	if (endptr) *((const qse_char_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
} while(0)

/* qse_strxtonum (const qse_char_t* nptr, qse_size_t len, qse_char_char** endptr, int base) */
#define QSE_STRXTONUM(value,nptr,len,endptr,base) do {\
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
	if (endptr) *((const qse_char_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
} while(0)

/* int qse_mchartonum (qse_mchar_t c, int base) */
#define QSE_MCHARTONUM(c,base) \
	((c>=QSE_MT('0') && c<=QSE_MT('9'))? ((c-QSE_MT('0')<base)? (c-QSE_MT('0')): base): \
	 (c>=QSE_MT('A') && c<=QSE_MT('Z'))? ((c-QSE_MT('A')+10<base)? (c-QSE_MT('A')+10): base): \
	 (c>=QSE_MT('a') && c<=QSE_MT('z'))? ((c-QSE_MT('a')+10<base)? (c-QSE_MT('a')+10): base): base)

/* qse_strtonum (const qse_mchar_t* nptr, qse_mchar_t** endptr, int base) */
#define QSE_MSTRTONUM(value,nptr,endptr,base) do {\
	int __ston_f = 0, __ston_v; \
	const qse_mchar_t* __ston_ptr = nptr; \
	for (;;) { \
		qse_mchar_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_MT(' ') || \
		    __ston_c == QSE_MT('\t')) { __ston_ptr++; continue; } \
		if (__ston_c == QSE_MT('-')) { __ston_f++; __ston_ptr++; } \
		if (__ston_c == QSE_MT('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; (__ston_v = QSE_MCHARTONUM(*__ston_ptr, base)) < base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr) *((const qse_mchar_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
} while(0)

/* qse_strxtonum (const qse_mchar_t* nptr, qse_size_t len, qse_mchar_t** endptr, int base) */
#define QSE_MSTRXTONUM(value,nptr,len,endptr,base) do {\
	int __ston_f = 0, __ston_v; \
	const qse_mchar_t* __ston_ptr = nptr; \
	const qse_mchar_t* __ston_end = __ston_ptr + len; \
	value = 0; \
	while (__ston_ptr < __ston_end) { \
		qse_mchar_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_MT(' ') || __ston_c == QSE_MT('\t')) { \
			__ston_ptr++; continue; \
		} \
		if (__ston_c == QSE_MT('-')) { __ston_f++; __ston_ptr++; } \
		if (__ston_c == QSE_MT('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; __ston_ptr < __ston_end && \
	               (__ston_v = QSE_MCHARTONUM(*__ston_ptr, base)) != base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr) *((const qse_mchar_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
} while(0)

/* int qse_wchartonum (qse_wchar_t c, int base) */
#define QSE_WCHARTONUM(c,base) \
	((c>=QSE_WT('0') && c<=QSE_WT('9'))? ((c-QSE_WT('0')<base)? (c-QSE_WT('0')): base): \
	 (c>=QSE_WT('A') && c<=QSE_WT('Z'))? ((c-QSE_WT('A')+10<base)? (c-QSE_WT('A')+10): base): \
	 (c>=QSE_WT('a') && c<=QSE_WT('z'))? ((c-QSE_WT('a')+10<base)? (c-QSE_WT('a')+10): base): base)

/* qse_strtonum (const qse_wchar_t* nptr, qse_wchar_t** endptr, int base) */
#define QSE_WSTRTONUM(value,nptr,endptr,base) do {\
	int __ston_f = 0, __ston_v; \
	const qse_wchar_t* __ston_ptr = nptr; \
	for (;;) { \
		qse_wchar_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_WT(' ') || \
		    __ston_c == QSE_WT('\t')) { __ston_ptr++; continue; } \
		if (__ston_c == QSE_WT('-')) { __ston_f++; __ston_ptr++; } \
		if (__ston_c == QSE_WT('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; (__ston_v = QSE_WCHARTONUM(*__ston_ptr, base)) < base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr) *((const qse_wchar_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
} while(0)

/* qse_strxtonum (const qse_wchar_t* nptr, qse_size_t len, qse_wchar_t** endptr, int base) */
#define QSE_WSTRXTONUM(value,nptr,len,endptr,base) do {\
	int __ston_f = 0, __ston_v; \
	const qse_wchar_t* __ston_ptr = nptr; \
	const qse_wchar_t* __ston_end = __ston_ptr + len; \
	value = 0; \
	while (__ston_ptr < __ston_end) { \
		qse_wchar_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_WT(' ') || __ston_c == QSE_WT('\t')) { \
			__ston_ptr++; continue; \
		} \
		if (__ston_c == QSE_WT('-')) { __ston_f++; __ston_ptr++; } \
		if (__ston_c == QSE_WT('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; __ston_ptr < __ston_end && \
	               (__ston_v = QSE_WCHARTONUM(*__ston_ptr, base)) != base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr) *((const qse_wchar_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
} while(0)

/**
 * The qse_mbstrmx_op_t defines a string trimming operation. 
 */
enum qse_mbstrmx_op_t
{
	QSE_MBSTRMX_LEFT  = (1 << 0), /**< trim leading spaces */
	QSE_MBSTRMX_RIGHT = (1 << 1)  /**< trim trailing spaces */
};

/**
 * The qse_wcstrmx_op_t defines a string trimming operation. 
 */
enum qse_wcstrmx_op_t
{
	QSE_WCSTRMX_LEFT  = (1 << 0), /**< trim leading spaces */
	QSE_WCSTRMX_RIGHT = (1 << 1)  /**< trim trailing spaces */
};

#ifdef QSE_CHAR_IS_MCHAR
#	define QSE_STRTRMX_LEFT QSE_MBSTRMX_LEFT
#	define QSE_STRTRMX_RIGHT QSE_MBSTRMX_RIGHT
#else
#	define QSE_STRTRMX_LEFT QSE_WCSTRMX_LEFT
#	define QSE_STRTRMX_RIGHT QSE_WCSTRMX_RIGHT
#endif

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

/**
 * The qse_mbsbytes() function returns the number of bytes a null-terminated
 * string is holding excluding a terminating null.
 */
qse_size_t qse_mbsbytes (
	const qse_mchar_t* str
);

/**
 * The qse_wcsbytes() function returns the number of bytes a null-terminated
 * string is holding excluding a terminating null.
 */
qse_size_t qse_wcsbytes (
	const qse_wchar_t* str
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strlen(str)   qse_mbslen(str)
#	define qse_strbytes(str) qse_mbsbytes(str)
#else
#	define qse_strlen(str)   qse_wcslen(str)
#	define qse_strbytes(str) qse_wcsbytes(str)
#endif

qse_size_t qse_mbscpy (
	qse_mchar_t*       buf,
	const qse_mchar_t* str
);

qse_size_t qse_wcscpy (
	qse_wchar_t*       buf,
	const qse_wchar_t* str
);

qse_size_t qse_mbsxcpy (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* str
);

qse_size_t qse_wcsxcpy (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* str
);

/**
 * The qse_mbsncpy() function copies a length-bounded string into
 * a buffer with unknown size. 
 */
qse_size_t qse_mbsncpy (
	qse_mchar_t*       buf, /**< buffer with unknown length */
	const qse_mchar_t* str, /**< length-bounded string */
	qse_size_t         len  /**< string length */
);

/**
 * The qse_wcsncpy() function copies a length-bounded string into
 * a buffer with unknown size. 
 */
qse_size_t qse_wcsncpy (
	qse_wchar_t*       buf, /**< buffer with unknown length */
	const qse_wchar_t* str, /**< length-bounded string */
	qse_size_t         len  /**< string length */
);

/**
 * The qse_mbsxncpy() function copies a length-bounded string into
 * a length-bounded buffer.
 */
qse_size_t qse_mbsxncpy (
	qse_mchar_t*       buf, /**< length-bounded buffer */
	qse_size_t         bsz, /**< buffer length */
	const qse_mchar_t* str, /**< length-bounded string */
	qse_size_t         len  /**< string length */
);

/**
 * The qse_wcsxncpy() function copies a length-bounded string into
 * a length-bounded buffer.
 */
qse_size_t qse_wcsxncpy (
	qse_wchar_t*       buf, /**< length-bounded buffer */
	qse_size_t         bsz, /**< buffer length */
	const qse_wchar_t* str, /**< length-bounded string */
	qse_size_t         len  /**< string length */
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strcpy(buf,str)           qse_mbscpy(buf,str)
#	define qse_strxcpy(buf,bsz,str)      qse_mbsxcpy(buf,bsz,str)
#	define qse_strncpy(buf,str,len)      qse_mbsncpy(buf,str,len)
#	define qse_strxncpy(buf,bsz,str,len) qse_mbsxncpy(buf,bsz,str,len)
#else
#	define qse_strcpy(buf,str)           qse_wcscpy(buf,str)
#	define qse_strxcpy(buf,bsz,str)      qse_wcsxcpy(buf,bsz,str)
#	define qse_strncpy(buf,str,len)      qse_wcsncpy(buf,str,len)
#	define qse_strxncpy(buf,bsz,str,len) qse_wcsxncpy(buf,bsz,str,len)
#endif

qse_size_t qse_mbsput (
	qse_mchar_t*       buf, 
	const qse_mchar_t* str
);

qse_size_t qse_wcsput (
	qse_wchar_t*       buf, 
	const qse_wchar_t* str
);

/**
 * The qse_mbsxput() function copies the string @a str into the buffer @a buf
 * of the size @a bsz. Unlike qse_strxcpy(), it does not null-terminate the
 * buffer.
 */
qse_size_t qse_mbsxput (
	qse_mchar_t*       buf, 
	qse_size_t         bsz,
	const qse_mchar_t* str
);

/**
 * The qse_wcsxput() function copies the string @a str into the buffer @a buf
 * of the size @a bsz. Unlike qse_strxcpy(), it does not null-terminate the
 * buffer.
 */
qse_size_t qse_wcsxput (
	qse_wchar_t*       buf, 
	qse_size_t         bsz,
	const qse_wchar_t* str
);

qse_size_t qse_mbsxnput (
	qse_mchar_t*       buf,
	qse_size_t        bsz,
	const qse_mchar_t* str,
	qse_size_t        len
);

qse_size_t qse_wcsxnput (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* str,
	qse_size_t         len
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strput(buf,str)           qse_mbsput(buf,str)
#	define qse_strxput(buf,bsz,str)      qse_mbsxput(buf,bsz,str)
#	define qse_strxnput(buf,bsz,str,len) qse_mbsxnput(buf,bsz,str,len)
#else
#	define qse_strput(buf,str)           qse_wcsput(buf,str)
#	define qse_strxput(buf,bsz,str)      qse_wcsxput(buf,bsz,str)
#	define qse_strxnput(buf,bsz,str,len) qse_wcsxnput(buf,bsz,str,len)
#endif

/**
 * The qse_mbsfcpy() function formats a string by position.
 * The position specifier is a number enclosed in ${ and }.
 * When ${ is preceeded by a backslash, it is treated literally. 
 * See the example below:
 * @code
 *  qse_char_t buf[256]
 *  qse_char_t* colors[] = { QSE_MT("blue"), QSE_MT("green"), QSE_MT("red") };
 *  qse_mbsfcpy(buf, QSE_MT("RGB: ${2}, ${1}, ${0}"), colors);
 * @endcode
 * @sa qse_mbsfncpy, qse_mbsxfcpy, qse_mbsxfncpy
 */
qse_size_t qse_mbsfcpy (
	qse_mchar_t*       buf,
	const qse_mchar_t* fmt,
	const qse_mchar_t* str[]
);

/**
 * The qse_wcsfcpy() function formats a string by position.
 * The position specifier is a number enclosed in ${ and }.
 * When ${ is preceeded by a backslash, it is treated literally. 
 * See the example below:
 * @code
 *  qse_char_t buf[256]
 *  qse_char_t* colors[] = { QSE_WT("blue"), QSE_WT("green"), QSE_WT("red") };
 *  qse_wcsfcpy(buf, QSE_WT("RGB: ${2}, ${1}, ${0}"), colors);
 * @endcode
 * @sa qse_wcsfncpy, qse_wcsxfcpy, qse_wcsxfncpy
 */
qse_size_t qse_wcsfcpy (
	qse_wchar_t*       buf,
	const qse_wchar_t* fmt,
	const qse_wchar_t* str[]
);

/**
 * The qse_mbsfncpy() function formats a string by position.
 * It differs from qse_mbsfcpy() in that @a str is an array of the 
 * #qse_mcstr_t type.
 * @sa qse_mbsfcpy, qse_mbsxfcpy, qse_mbsxfncpy
 */
qse_size_t qse_mbsfncpy (
	qse_mchar_t*       buf,
	const qse_mchar_t* fmt,
	const qse_mcstr_t  str[]
);

/**
 * The qse_wcsfncpy() function formats a string by position.
 * It differs from qse_wcsfcpy() in that @a str is an array of the 
 * #qse_wcstr_t type.
 * @sa qse_wcsfcpy, qse_wcsxfcpy, qse_wcsxfncpy
 */
qse_size_t qse_wcsfncpy (
	qse_wchar_t*       buf,
	const qse_wchar_t* fmt,
	const qse_wcstr_t  str[]
);

/**
 * The qse_mbsxfcpy() function formats a string by position.
 * It differs from qse_strfcpy() in that @a buf is length-bounded of @a bsz
 * characters.
 * @code
 *  qse_mchar_t buf[256]
 *  qse_mchar_t* colors[] = { QSE_MT("blue"), QSE_MT("green"), QSE_MT("red") };
 *  qse_mbsxfcpy(buf, QSE_COUNTOF(buf), QSE_MT("RGB: ${2}, ${1}, ${0}"), colors);
 * @endcode
 * @sa qse_mbsfcpy, qse_mbsfncpy, qse_mbsxfncpy
 */
qse_size_t qse_mbsxfcpy (
	qse_mchar_t*       buf,
	qse_size_t         bsz, 
	const qse_mchar_t* fmt,
	const qse_mchar_t* str[]
);

/**
 * The qse_wcsxfcpy() function formats a string by position.
 * It differs from qse_wcsfcpy() in that @a buf is length-bounded of @a bsz
 * characters.
 * @code
 *  qse_char_t buf[256]
 *  qse_char_t* colors[] = { QSE_WT("blue"), QSE_WT("green"), QSE_WT("red") };
 *  qse_wcsxfcpy(buf, QSE_COUNTOF(buf), QSE_WT("RGB: ${2}, ${1}, ${0}"), colors);
 * @endcode
 * @sa qse_wcsfcpy, qse_wcsfncpy, qse_wcsxfncpy
 */
qse_size_t qse_wcsxfcpy (
	qse_wchar_t*       buf,
	qse_size_t         bsz, 
	const qse_wchar_t* fmt,
	const qse_wchar_t* str[]
);

/**
 * The qse_mbsxfncpy() function formats a string by position.
 * It differs from qse_strfcpy() in that @a buf is length-bounded of @a bsz
 * characters and @a str is an array of the #qse_mcstr_t type.
 * @sa qse_mbsfcpy, qse_mbsfncpy, qse_mbsxfcpy
 */
qse_size_t qse_mbsxfncpy (
	qse_mchar_t*       buf,
	qse_size_t         bsz, 
	const qse_mchar_t* fmt,
	const qse_mcstr_t  str[]
);

/**
 * The qse_wcsxfncpy() function formats a string by position.
 * It differs from qse_strfcpy() in that @a buf is length-bounded of @a bsz
 * characters and @a str is an array of the #qse_wcstr_t type.
 * @sa qse_wcsfcpy, qse_wcsfncpy, qse_wcsxfcpy
 */
qse_size_t qse_wcsxfncpy (
	qse_wchar_t*       buf,
	qse_size_t         bsz, 
	const qse_wchar_t* fmt,
	const qse_wcstr_t  str[]
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strfcpy(buf,fmt,str)        qse_mbsfcpy(buf,fmt,str)
#	define qse_strfncpy(buf,fmt,str)       qse_mbsfncpy(buf,fmt,str)
#	define qse_strxfcpy(buf,bsz,fmt,str)   qse_mbsxfcpy(buf,bsz,fmt,str)
#	define qse_strxfncpy(buf,bsz,fmt,str)  qse_mbsxfncpy(buf,bsz,fmt,str)
#else
#	define qse_strfcpy(buf,fmt,str)        qse_wcsfcpy(buf,fmt,str)
#	define qse_strfncpy(buf,fmt,str)       qse_wcsfncpy(buf,fmt,str)
#	define qse_strxfcpy(buf,bsz,fmt,str)   qse_wcsxfcpy(buf,bsz,fmt,str)
#	define qse_strxfncpy(buf,bsz,fmt,str)  qse_wcsxfncpy(buf,bsz,fmt,str)
#endif

/**
 * The qse_mbsxsubst() function expands @a fmt into a buffer @a buf of the size
 * @a bsz by substituting new values for ${} segments within it. The actual
 * substitution is made by invoking the callback function @a subst. 
 * @code
 * qse_mchar_t* subst (qse_mchar_t* buf, qse_size_t bsz, const qse_mcstr_t* ident, void* ctx)
 * { 
 *   if (qse_mbsxcmp (ident->ptr, ident->len, QSE_MT("USER")) == 0)
 *     return buf + qse_mbsxput (buf, bsz, QSE_MT("sam"));	
 *   else if (qse_mbsxcmp (ident->ptr, ident->len, QSE_MT("GROUP")) == 0)
 *     return buf + qse_mbsxput (buf, bsz, QSE_MT("coders"));	
 *   return buf; 
 * }
 * 
 * qse_mchar_t buf[25];
 * qse_mbsxsubst (buf, i, QSE_MT("user=${USER},group=${GROUP}"), subst, QSE_NULL);
 * @endcode
 */
qse_size_t qse_mbsxsubst (
	qse_mchar_t*           buf,
	qse_size_t             bsz,
	const qse_mchar_t*     fmt,
	qse_mbsxsubst_subst_t  subst,
	void*                  ctx
);

/**
 * The qse_wcsxsubst() function expands @a fmt into a buffer @a buf of the size
 * @a bsz by substituting new values for ${} segments within it. The actual
 * substitution is made by invoking the callback function @a subst. 
 * @code
 * qse_wchar_t* subst (qse_wchar_t* buf, qse_size_t bsz, const qse_wcstr_t* ident, void* ctx)
 * { 
 *   if (qse_wcsxcmp (ident->ptr, ident->len, QSE_WT("USER")) == 0)
 *     return buf + qse_wcsxput (buf, bsz, QSE_WT("sam"));	
 *   else if (qse_wcsxcmp (ident->ptr, ident->len, QSE_WT("GROUP")) == 0)
 *     return buf + qse_wcsxput (buf, bsz, QSE_WT("coders"));	
 *   return buf; 
 * }
 * 
 * qse_wchar_t buf[25];
 * qse_wcsxsubst (buf, i, QSE_WT("user=${USER},group=${GROUP}"), subst, QSE_NULL);
 * @endcode
 */
qse_size_t qse_wcsxsubst (
	qse_wchar_t*           buf,
	qse_size_t             bsz,
	const qse_wchar_t*     fmt,
	qse_wcsxsubst_subst_t  subst,
	void*                  ctx
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strxsubst(buf,bsz,fmt,subst,ctx) qse_mbsxsubst(buf,bsz,fmt,subst,ctx)
#else
#	define qse_strxsubst(buf,bsz,fmt,subst,ctx) qse_wcsxsubst(buf,bsz,fmt,subst,ctx)
#endif

qse_size_t qse_mbscat (
	qse_mchar_t*       buf,
	const qse_mchar_t* str
);

qse_size_t qse_mbsncat (
	qse_mchar_t*       buf,
	const qse_mchar_t* str,
	qse_size_t         len
);

qse_size_t qse_mbscatn (
	qse_mchar_t*       buf,
	const qse_mchar_t* str,
	qse_size_t         n
);

qse_size_t qse_mbsxcat (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* str
);

qse_size_t qse_mbsxncat (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* str,
	qse_size_t         len
);

qse_size_t qse_wcscat (
	qse_wchar_t*       buf,
	const qse_wchar_t* str
);

qse_size_t qse_wcsncat (
	qse_wchar_t*       buf,
	const qse_wchar_t* str,
	qse_size_t         len
);

qse_size_t qse_wcscatn (
	qse_wchar_t*       buf,
	const qse_wchar_t* str,
	qse_size_t         n
);

qse_size_t qse_wcsxcat (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* str
);

qse_size_t qse_wcsxncat (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* str,
	qse_size_t         len
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strcat(buf,str)           qse_mbscat(buf,str)
#	define qse_strncat(buf,str,len)      qse_mbsncat(buf,str,len)
#	define qse_strcatn(buf,str,n)        qse_mbscatn(buf,str,n)
#	define qse_strxcat(buf,bsz,str)      qse_mbsxcat(buf,bsz,str);
#	define qse_strxncat(buf,bsz,str,len) qse_mbsxncat(buf,bsz,str,len)
#else
#	define qse_strcat(buf,str)           qse_wcscat(buf,str)
#	define qse_strncat(buf,str,len)      qse_wcsncat(buf,str,len)
#	define qse_strcatn(buf,str,n)        qse_wcscatn(buf,str,n)
#	define qse_strxcat(buf,bsz,str)      qse_wcsxcat(buf,bsz,str);
#	define qse_strxncat(buf,bsz,str,len) qse_wcsxncat(buf,bsz,str,len)
#endif

int qse_mbscmp (
	const qse_mchar_t* s1,
	const qse_mchar_t* s2
);

int qse_wcscmp (
	const qse_wchar_t* s1,
	const qse_wchar_t* s2
);

int qse_mbsxcmp (
	const qse_mchar_t* s1,
	qse_size_t         ln1,
	const qse_mchar_t* s2
);

int qse_wcsxcmp (
	const qse_wchar_t* s1,
	qse_size_t         ln1,
	const qse_wchar_t* s2
);

int qse_mbsxncmp (
	const qse_mchar_t* s1,
	qse_size_t         ln1, 
	const qse_mchar_t* s2,
	qse_size_t         ln2
);

int qse_wcsxncmp (
	const qse_wchar_t* s1,
	qse_size_t         ln1, 
	const qse_wchar_t* s2,
	qse_size_t         ln2
);

int qse_mbscasecmp (
	const qse_mchar_t* s1,
	const qse_mchar_t* s2
);

int qse_wcscasecmp (
	const qse_wchar_t* s1,
	const qse_wchar_t* s2
);

int qse_mbsxcasecmp (
	const qse_mchar_t* s1,
	qse_size_t         ln,
	const qse_mchar_t* s2
);

int qse_wcsxcasecmp (
	const qse_wchar_t* s1,
	qse_size_t         ln,
	const qse_wchar_t* s2
);

/**
 * The qse_mbsxncasecmp() function compares characters at the same position 
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
 * qse_mbsxncasecmp (QSE_MT("foo"), 3, QSE_MT("FoO"), 3);
 * @endcode
 *
 * @return 0 if two strings are equal, 
 *         a positive number if the first string is larger, 
 *         -1 if the second string is larger.
 *
 */
int qse_mbsxncasecmp (
	const qse_mchar_t* s1,  /**< pointer to the first string */
	qse_size_t         ln1, /**< length of the first string */ 
	const qse_mchar_t* s2,  /**< pointer to the second string */
	qse_size_t         ln2  /**< length of the second string */
);

/**
 * The qse_wcsxncasecmp() function compares characters at the same position 
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
 * qse_wcsxncasecmp (QSE_WT("foo"), 3, QSE_WT("FoO"), 3);
 * @endcode
 *
 * @return 0 if two strings are equal, 
 *         a positive number if the first string is larger, 
 *         -1 if the second string is larger.
 *
 */
int qse_wcsxncasecmp (
	const qse_wchar_t* s1,  /**< pointer to the first string */
	qse_size_t         ln1, /**< length of the first string */ 
	const qse_wchar_t* s2,  /**< pointer to the second string */
	qse_size_t         ln2  /**< length of the second string */
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strcmp(s1,s2)               qse_mbscmp(s1,s2)
#	define qse_strxcmp(s1,ln1,s2)          qse_mbsxcmp(s1,ln1,s2)
#	define qse_strxncmp(s1,ln1,s2,ln2)     qse_mbsxncmp(s1,ln1,s2,ln2)
#	define qse_strcasecmp(s1,s2)           qse_mbscasecmp(s1,s2)
#	define qse_strxcasecmp(s1,ln1,s2)      qse_mbsxcasecmp(s1,ln1,s2)
#	define qse_strxncasecmp(s1,ln1,s2,ln2) qse_mbsxncasecmp(s1,ln1,s2,ln2)
#else
#	define qse_strcmp(s1,s2)               qse_wcscmp(s1,s2)
#	define qse_strxcmp(s1,ln1,s2)          qse_wcsxcmp(s1,ln1,s2)
#	define qse_strxncmp(s1,ln1,s2,ln2)     qse_wcsxncmp(s1,ln1,s2,ln2)
#	define qse_strcasecmp(s1,s2)           qse_wcscasecmp(s1,s2)
#	define qse_strxcasecmp(s1,ln1,s2)      qse_wcsxcasecmp(s1,ln1,s2)
#	define qse_strxncasecmp(s1,ln1,s2,ln2) qse_wcsxncasecmp(s1,ln1,s2,ln2)
#endif

qse_mchar_t* qse_mbsdup (
	const qse_mchar_t* str,
	qse_mmgr_t*        mmgr
);

qse_mchar_t* qse_mbsdup2 (
	const qse_mchar_t* str1,
	const qse_mchar_t* str2,
	qse_mmgr_t*        mmgr
);

qse_mchar_t* qse_mbsxdup (
	const qse_mchar_t* str,
	qse_size_t         len, 
	qse_mmgr_t*        mmgr
);

qse_mchar_t* qse_mbsxdup2 (
	const qse_mchar_t* str1,
	qse_size_t         len1,
	const qse_mchar_t* str2,
	qse_size_t         len2,
	qse_mmgr_t*        mmgr
);

qse_wchar_t* qse_wcsdup (
	const qse_wchar_t* str,
	qse_mmgr_t*        mmgr
);

qse_wchar_t* qse_wcsdup2 (
	const qse_wchar_t* str1,
	const qse_wchar_t* str2,
	qse_mmgr_t*        mmgr
);

qse_wchar_t* qse_wcsxdup (
	const qse_wchar_t* str,
	qse_size_t         len, 
	qse_mmgr_t*        mmgr
);

qse_wchar_t* qse_wcsxdup2 (
	const qse_wchar_t* str1,
	qse_size_t         len1,
	const qse_wchar_t* str2,
	qse_size_t         len2,
	qse_mmgr_t*        mmgr
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strdup(s,mmgr)             qse_mbsdup(s,mmgr)
#	define qse_strdup2(s1,s2,mmgr)        qse_mbsdup2(s1,s2,mmgr)
#	define qse_strxdup(s,l,mmgr)          qse_mbsxdup(s,l,mmgr)
#	define qse_strxdup2(s1,l1,s2,l2,mmgr) qse_mbsxdup(s1,l1,s2,l2,mmgr)
#else
#	define qse_strdup(s,mmgr)             qse_wcsdup(s,mmgr)
#	define qse_strdup2(s1,s2,mmgr)        qse_wcsdup2(s1,s2,mmgr)
#	define qse_strxdup(s,l,mmgr)          qse_wcsxdup(s,l,mmgr)
#	define qse_strxdup2(s1,l1,s2,l2,mmgr) qse_wcsxdup(s1,l1,s2,l2,mmgr)
#endif

/**
 * The qse_mbsstr() function searchs a string @a str for the first occurrence 
 * of a substring @a sub.
 * @return pointer to the first occurrence in @a str if @a sub is found, 
 *         #QSE_NULL if not.
 */
qse_mchar_t* qse_mbsstr (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

/**
 * The qse_wcsstr() function searchs a string @a str for the first occurrence 
 * of a substring @a sub.
 * @return pointer to the first occurrence in @a str if @a sub is found, 
 *         #QSE_NULL if not.
 */
qse_wchar_t* qse_wcsstr (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxstr (
	const qse_mchar_t* str,
	qse_size_t         size,
	const qse_mchar_t* sub
);

qse_wchar_t* qse_wcsxstr (
	const qse_wchar_t* str,
	qse_size_t         size,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxnstr (
	const qse_mchar_t* str,
	qse_size_t         strsz, 
	const qse_mchar_t* sub,
	qse_size_t         subsz
);

qse_wchar_t* qse_wcsxnstr (
	const qse_wchar_t* str,
	qse_size_t         strsz, 
	const qse_wchar_t* sub,
	qse_size_t         subsz
);

qse_mchar_t* qse_mbscasestr (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

qse_wchar_t* qse_wcscasestr (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxcasestr (
	const qse_mchar_t* str,
	qse_size_t         size,
	const qse_mchar_t* sub
);

qse_wchar_t* qse_wcsxcasestr (
	const qse_wchar_t* str,
	qse_size_t         size,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxncasestr (
	const qse_mchar_t* str,
	qse_size_t         strsz, 
	const qse_mchar_t* sub,
	qse_size_t         subsz
);

qse_wchar_t* qse_wcsxncasestr (
	const qse_wchar_t* str,
	qse_size_t         strsz, 
	const qse_wchar_t* sub,
	qse_size_t         subsz
);

/**
 * The qse_mbsrstr() function searchs a string @a str for the last occurrence 
 * of a substring @a sub.
 * @return pointer to the last occurrence in @a str if @a sub is found, 
 *         #QSE_NULL if not.
 */
qse_mchar_t* qse_mbsrstr (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

/**
 * The qse_wcsrstr() function searchs a string @a str for the last occurrence 
 * of a substring @a sub.
 * @return pointer to the last occurrence in @a str if @a sub is found, 
 *         #QSE_NULL if not.
 */
qse_wchar_t* qse_wcsrstr (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxrstr (
	const qse_mchar_t* str,
	qse_size_t         size,
	const qse_mchar_t* sub
);

qse_wchar_t* qse_wcsxrstr (
	const qse_wchar_t* str,
	qse_size_t         size,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxnrstr (
	const qse_mchar_t* str,
	qse_size_t         strsz, 
	const qse_mchar_t* sub,
	qse_size_t         subsz
);

qse_wchar_t* qse_wcsxnrstr (
	const qse_wchar_t* str,
	qse_size_t         strsz, 
	const qse_wchar_t* sub,
	qse_size_t         subsz
);

qse_mchar_t* qse_mbsrcasestr (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

qse_wchar_t* qse_wcsrcasestr (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxrcasestr (
	const qse_mchar_t* str,
	qse_size_t         size,
	const qse_mchar_t* sub
);

qse_wchar_t* qse_wcsxrcasestr (
	const qse_wchar_t* str,
	qse_size_t         size,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxnrcasestr (
	const qse_mchar_t* str,
	qse_size_t         strsz, 
	const qse_mchar_t* sub,
	qse_size_t         subsz
);

qse_wchar_t* qse_wcsxnrcasestr (
	const qse_wchar_t* str,
	qse_size_t         strsz, 
	const qse_wchar_t* sub,
	qse_size_t         subsz
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strstr(str,sub)                    qse_mbsstr(str,sub)
#	define qse_strxstr(str,size,sub)              qse_mbsxstr(str,size,sub)
#	define qse_strxnstr(str,strsz,sub,subsz)      qse_mbsxnstr(str,strsz,sub,subsz)
#	define qse_strcasestr(str,sub)                qse_mbscasestr(str,sub)
#	define qse_strxcasestr(str,size,sub)          qse_mbsxcasestr(str,size,sub)
#	define qse_strxncasestr(str,strsz,sub,subsz)  qse_mbsxncasestr(str,strsz,sub,subsz)
#	define qse_strrstr(str,sub)                   qse_mbsrstr(str,sub)
#	define qse_strxrstr(str,size,sub)             qse_mbsxrstr(str,size,sub)
#	define qse_strxnrstr(str,strsz,sub,subsz)     qse_mbsxnrstr(str,strsz,sub,subsz)
#	define qse_strrcasestr(str,sub)               qse_mbsrcasestr(str,sub)
#	define qse_strxrcasestr(str,size,sub)         qse_mbsxrcasestr(str,size,sub)
#	define qse_strxnrcasestr(str,strsz,sub,subsz) qse_mbsxnrcasestr(str,strsz,sub,subsz)
#else
#	define qse_strstr(str,sub)                    qse_wcsstr(str,sub)
#	define qse_strxstr(str,size,sub)              qse_wcsxstr(str,size,sub)
#	define qse_strxnstr(str,strsz,sub,subsz)      qse_wcsxnstr(str,strsz,sub,subsz)
#	define qse_strcasestr(str,sub)                qse_wcscasestr(str,sub)
#	define qse_strxcasestr(str,size,sub)          qse_wcsxcasestr(str,size,sub)
#	define qse_strxncasestr(str,strsz,sub,subsz)  qse_wcsxncasestr(str,strsz,sub,subsz)
#	define qse_strrstr(str,sub)                   qse_wcsrstr(str,sub)
#	define qse_strxrstr(str,size,sub)             qse_wcsxrstr(str,size,sub)
#	define qse_strxnrstr(str,strsz,sub,subsz)     qse_wcsxnrstr(str,strsz,sub,subsz)
#	define qse_strrcasestr(str,sub)               qse_wcsrcasestr(str,sub)
#	define qse_strxrcasestr(str,size,sub)         qse_wcsxrcasestr(str,size,sub)
#	define qse_strxnrcasestr(str,strsz,sub,subsz) qse_wcsxnrcasestr(str,strsz,sub,subsz)
#endif

const qse_mchar_t* qse_mbsword (
	const qse_mchar_t* str,
	const qse_mchar_t* word
);

const qse_wchar_t* qse_wcsword (
	const qse_wchar_t* str,
	const qse_wchar_t* word
);

/**
 * The qse_mbsxword() function finds a whole word in a string.
 */
const qse_mchar_t* qse_mbsxword (
	const qse_mchar_t* str,
	qse_size_t         len,
	const qse_mchar_t* word
);

/**
 * The qse_wcsxword() function finds a whole word in a string.
 */
const qse_wchar_t* qse_wcsxword (
	const qse_wchar_t* str,
	qse_size_t         len,
	const qse_wchar_t* word
);

const qse_mchar_t* qse_mbscaseword (
	const qse_mchar_t* str,
	const qse_mchar_t* word
);

const qse_wchar_t* qse_wcscaseword (
	const qse_wchar_t* str,
	const qse_wchar_t* word
);

/**
 * The qse_mbsxcaseword() function finds a whole word in a string 
 * case-insensitively.
 */
const qse_mchar_t* qse_mbsxcaseword (
	const qse_mchar_t* str,
	qse_size_t         len,
	const qse_mchar_t* word
);

/**
 * The qse_wcsxcaseword() function finds a whole word in a string 
 * case-insensitively.
 */
const qse_wchar_t* qse_wcsxcaseword (
	const qse_wchar_t* str,
	qse_size_t         len,
	const qse_wchar_t* word
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strword(str,word)          qse_mbsword(str,word)
#	define qse_strxword(str,len,word)     qse_mbsxword(str,len,word)
#	define qse_strcaseword(str,word)      qse_mbscaseword(str,word)
#	define qse_strxcaseword(str,len,word) qse_mbsxcaseword(str,len,word)
#else
#	define qse_strword(str,word)          qse_wcsword(str,word)
#	define qse_strxword(str,len,word)     qse_wcsxword(str,len,word)
#	define qse_strcaseword(str,word)      qse_wcscaseword(str,word)
#	define qse_strxcaseword(str,len,word) qse_wcsxcaseword(str,len,word)
#endif

/**
 * The qse_mbschr() function finds a chracter in a string. 
 */
qse_mchar_t* qse_mbschr (
	const qse_mchar_t* str,
	qse_mcint_t        c
);

/**
 * The qse_wcschr() function finds a chracter in a string. 
 */
qse_wchar_t* qse_wcschr (
	const qse_wchar_t* str,
	qse_wcint_t        c
);

qse_mchar_t* qse_mbsxchr (
	const qse_mchar_t* str,
	qse_size_t         len,
	qse_mcint_t        c
);

qse_wchar_t* qse_wcsxchr (
	const qse_wchar_t* str,
	qse_size_t         len,
	qse_wcint_t        c
);

qse_wchar_t* qse_wcsrchr (
	const qse_wchar_t* str,
	qse_wcint_t        c
);

qse_mchar_t* qse_mbsrchr (
	const qse_mchar_t* str,
	qse_mcint_t        c
);

qse_mchar_t* qse_mbsxrchr (
	const qse_mchar_t* str,
	qse_size_t         len,
	qse_mcint_t        c
);

qse_wchar_t* qse_wcsxrchr (
	const qse_wchar_t* str,
	qse_size_t         len,
	qse_wcint_t        c
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strchr(str,c)        qse_mbschr(str,c)
#	define qse_strxchr(str,len,c)   qse_mbsxchr(str,len,c)
#	define qse_strrchr(str,c)       qse_mbsrchr(str,c)
#	define qse_strxrchr(str,len,c)  qse_mbsxrchr(str,len,c)
#else
#	define qse_strchr(str,c)        qse_wcschr(str,c)
#	define qse_strxchr(str,len,c)   qse_wcsxchr(str,len,c)
#	define qse_strrchr(str,c)       qse_wcsrchr(str,c)
#	define qse_strxrrchr(str,len,c) qse_wcsxrchr(str,len,c)
#endif

/**
 * The qse_mbsbeg() function checks if a string begins with a substring.
 * @return pointer to the beginning of a matching beginning, 
 *         #SE_NULL if no match is found.
 */
qse_mchar_t* qse_mbsbeg (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

/**
 * The qse_wcsbeg() function checks if a string begins with a substring.
 * @return pointer to the beginning of a matching beginning, 
 *         #QSE_NULL if no match is found.
 */
qse_wchar_t* qse_wcsbeg (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxbeg (
	const qse_mchar_t* str,
	qse_size_t         len,
	const qse_mchar_t* sub
);

qse_wchar_t* qse_wcsxbeg (
	const qse_wchar_t* str,
	qse_size_t         len,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsnbeg (
	const qse_mchar_t* str,
	const qse_mchar_t* sub,
	qse_size_t         len
);

qse_wchar_t* qse_wcsnbeg (
	const qse_wchar_t* str,
	const qse_wchar_t* sub,
	qse_size_t         len
);

qse_mchar_t* qse_mbsxnbeg (
	const qse_mchar_t* str,
	qse_size_t         len1, 
	const qse_mchar_t* sub,
	qse_size_t         len2
);

qse_wchar_t* qse_wcsxnbeg (
	const qse_wchar_t* str,
	qse_size_t         len1, 
	const qse_wchar_t* sub,
	qse_size_t         len2
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strbeg(str,sub)             qse_mbsbeg(str,sub)
#	define qse_strxbeg(str,len,sub)        qse_mbsxbeg(str,len,sub)
#	define qse_strnbeg(str,sub,len)        qse_mbsnbeg(str,sub,len)
#	define qse_strxnbeg(str,len1,sub,len2) qse_mbsxnbeg(str,len1,sub,len2)
#else
#	define qse_strbeg(str,sub)             qse_wcsbeg(str,sub)
#	define qse_strxbeg(str,len,sub)        qse_wcsxbeg(str,len,sub)
#	define qse_strnbeg(str,sub,len)        qse_wcsnbeg(str,sub,len)
#	define qse_strxnbeg(str,len1,sub,len2) qse_wcsxnbeg(str,len1,sub,len2)
#endif

/**
 * The qse_mbsend() function checks if a string ends with a substring.
 * @return pointer to the beginning of a matching ending, 
 *         #SE_NULL if no match is found.
 */
qse_mchar_t* qse_mbsend (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

/**
 * The qse_wcsend() function checks if a string ends with a substring.
 * @return pointer to the beginning of a matching ending, 
 *         #QSE_NULL if no match is found.
 */
qse_wchar_t* qse_wcsend (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsxend (
	const qse_mchar_t* str,
	qse_size_t         len,
	const qse_mchar_t* sub
);

qse_wchar_t* qse_wcsxend (
	const qse_wchar_t* str,
	qse_size_t         len,
	const qse_wchar_t* sub
);

qse_mchar_t* qse_mbsnend (
	const qse_mchar_t* str,
	const qse_mchar_t* sub,
	qse_size_t         len
);

qse_wchar_t* qse_wcsnend (
	const qse_wchar_t* str,
	const qse_wchar_t* sub,
	qse_size_t         len
);

qse_mchar_t* qse_mbsxnend (
	const qse_mchar_t* str,
	qse_size_t         len1, 
	const qse_mchar_t* sub,
	qse_size_t         len2
);

qse_wchar_t* qse_wcsxnend (
	const qse_wchar_t* str,
	qse_size_t         len1, 
	const qse_wchar_t* sub,
	qse_size_t         len2
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strend(str,sub)             qse_mbsxend(str,sub)
#	define qse_strxend(str,len,sub)        qse_mbsxend(str,len,sub)
#	define qse_strnend(str,sub,len)        qse_mbsnend(str,sub,len)
#	define qse_strxnend(str,len1,sub,len2) qse_mbsxnend(str,len1,sub,len2)
#else
#	define qse_strend(str,sub)             qse_wcsxend(str,sub)
#	define qse_strxend(str,len,sub)        qse_wcsxend(str,len,sub)
#	define qse_strnend(str,sub,len)        qse_wcsnend(str,sub,len)
#	define qse_strxnend(str,len1,sub,len2) qse_wcsxnend(str,len1,sub,len2)
#endif

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
 * The qse_mbspbrk() function searches @a str1 for the first occurrence of 
 * a character in @a str2.
 * @return pointer to the first occurrence in @a str1 if one is found.
 *         QSE_NULL if none is found.
 */
qse_mchar_t* qse_mbspbrk (
	const qse_mchar_t* str1,
	const qse_mchar_t* str2
);

/*
 * The qse_wcspbrk() function searches @a str1 for the first occurrence of 
 * a character in @a str2.
 * @return pointer to the first occurrence in @a str1 if one is found.
 *         QSE_NULL if none is found.
 */
qse_wchar_t* qse_wcspbrk (
	const qse_wchar_t* str1,
	const qse_wchar_t* str2
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strpbrk(str1,str2) qse_mbspbrk(str1,str2)
#else
#	define qse_strpbrk(str1,str2) qse_wcspbrk(str1,str2)
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



qse_size_t qse_mbsdel (
	qse_mchar_t* str, 
	qse_size_t   pos,
	qse_size_t   n
);

qse_size_t qse_wcsdel (
	qse_wchar_t* str, 
	qse_size_t   pos,
	qse_size_t   n
);

qse_size_t qse_mbsxdel (
	qse_mchar_t* str, 
	qse_size_t   len,
	qse_size_t   pos,
	qse_size_t   n
);

qse_size_t qse_wcsxdel (
	qse_wchar_t* str, 
	qse_size_t   len,
	qse_size_t   pos,
	qse_size_t   n
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strdel(str,pos,n)      qse_mbsdel(str,pos,n)
#	define qse_strxdel(str,len,pos,n) qse_mbsxdel(str,len,pos,n)
#else
#	define qse_strdel(str,pos,n)      qse_wcsdel(str,pos,n)
#	define qse_strxdel(str,len,pos,n) qse_wcsxdel(str,len,pos,n)
#endif


qse_size_t qse_mbsexcl (
	qse_mchar_t*       str,
	const qse_mchar_t* cs
);

qse_size_t qse_mbsxexcl (
	qse_mchar_t*       str,
	qse_size_t         len,
	const qse_mchar_t* cs
);

qse_size_t qse_wcsexcl (
	qse_wchar_t*       str,
	const qse_wchar_t* cs
);

qse_size_t qse_wcsxexcl (
	qse_wchar_t*       str,
	qse_size_t         len,
	const qse_wchar_t* cs
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strexcl(str,cs)      qse_mbsexcl(str,cs)
#	define qse_strxexcl(str,len,cs) qse_mbsxexcl(str,len,cs)
#else
#	define qse_strexcl(str,cs)      qse_wcsexcl(str,cs)
#	define qse_strxexcl(str,len,cs) qse_wcsxexcl(str,len,cs)
#endif

qse_size_t qse_mbsincl (
	qse_mchar_t*       str,
	const qse_mchar_t* cs
);

qse_size_t qse_mbsxincl (
	qse_mchar_t*       str,
	qse_size_t         len,
	const qse_mchar_t* cs
);

qse_size_t qse_wcsincl (
	qse_wchar_t*       str,
	const qse_wchar_t* cs
);

qse_size_t qse_wcsxincl (
	qse_wchar_t*       str,
	qse_size_t         len,
	const qse_wchar_t* cs
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strincl(str,cs)      qse_mbsincl(str,cs)
#	define qse_strxincl(str,len,cs) qse_mbsxincl(str,len,cs)
#else
#	define qse_strincl(str,cs)      qse_wcsincl(str,cs)
#	define qse_strxincl(str,len,cs) qse_wcsxincl(str,len,cs)
#endif

qse_size_t qse_mbsset (
	qse_mchar_t* buf,
	qse_mchar_t  c,
	qse_size_t   n
);

qse_size_t qse_wcsset (
	qse_wchar_t* buf,
	qse_wchar_t  c,
	qse_size_t   n
);

qse_size_t qse_mbsxset (
	qse_mchar_t* buf,
	qse_size_t   bsz,
	qse_mchar_t  c,
	qse_size_t   n
);


qse_size_t qse_wcsxset (
	qse_wchar_t* buf,
	qse_size_t   bsz,
	qse_wchar_t  c,
	qse_size_t   n
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strset(buf,c,n) qse_mbsset(buf,c,n)
#	define qse_strxset(buf,bsz,c,n) qse_mbsxset(buf,bsz,c,n)
#else
#	define qse_strset(buf,c,n) qse_wcsset(buf,c,n)
#	define qse_strxset(buf,bsz,c,n) qse_wcsxset(buf,bsz,c,n)
#endif

/* case conversion */

qse_size_t qse_mbslwr (
	qse_mchar_t* str
);

qse_size_t qse_wcslwr (
	qse_wchar_t* str
);

qse_size_t qse_mbsupr (
	qse_mchar_t* str
);

qse_size_t qse_wcsupr (
	qse_wchar_t* str
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strlwr(str) qse_mbslwr(str);
#	define qse_strupr(str) qse_mbsupr(str);
#else
#	define qse_strlwr(str) qse_wcslwr(str);
#	define qse_strupr(str) qse_wcsupr(str);
#endif


qse_size_t qse_mbsrev (
	qse_mchar_t* str
);

qse_size_t qse_wcsrev (
	qse_wchar_t* str
);

qse_size_t qse_mbsxrev (	
	qse_mchar_t* str,
	qse_size_t   len
);

qse_size_t qse_wcsxrev (	
	qse_wchar_t* str,
	qse_size_t   len
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strrev(str)      qse_mbsrev(str)
#	define qse_strxrev(str,len) qse_mbsxrev(str,len)
#else
#	define qse_strrev(str)      qse_wcsrev(str)
#	define qse_strxrev(str,len) qse_wcsxrev(str,len)
#endif

qse_size_t qse_mbsrot (
	qse_mchar_t* str,
	int          dir,
	qse_size_t   n
);

qse_size_t qse_wcsrot (
	qse_wchar_t* str,
	int          dir,
	qse_size_t   n
);

qse_size_t qse_mbsxrot (
	qse_mchar_t* str,
	qse_size_t   len,
	int          dir,
	qse_size_t   n
);

qse_size_t qse_wcsxrot (
	qse_wchar_t* str,
	qse_size_t   len,
	int          dir,
	qse_size_t   n
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strrot(str,dir,n)      qse_mbsrot(str,dir,n)
#	define qse_strxrot(str,len,dir,n) qse_mbsrot(str,len,dir,n)
#else
#	define qse_strrot(str,dir,n)      qse_wcsrot(str,dir,n)
#	define qse_strxrot(str,len,dir,n) qse_wcsrot(str,len,dir,n)
#endif

/**
 * The qse_mbsspl() function splits a string into fields.
 */
int qse_mbsspl (
	qse_mchar_t*       str,
	const qse_mchar_t* delim,
	qse_mchar_t        lquote,
	qse_mchar_t        rquote,
	qse_mchar_t        escape
);

/**
 * The qse_wcsspl() function splits a string into fields.
 */
int qse_wcsspl (
	qse_wchar_t*       str,
	const qse_wchar_t* delim,
	qse_wchar_t        lquote,
	qse_wchar_t        rquote,
	qse_wchar_t        escape
);

/**
 * The qse_mbsspltrn() function splits a string translating special 
 * escape sequences.
 * The argument @a trset is a translation character set which is composed
 * of multiple character pairs. An escape character followed by the 
 * first character in a pair is translated into the second character
 * in the pair. If trset is #QSE_NULL, no translation is performed. 
 *
 * Let's translate a sequence of '\n' and '\r' to a new line and a carriage
 * return respectively.
 * @code
 *   nfields = qse_mbsspltrn (
 *       str, QSE_MT(':'), QSE_MT('['), QSE_MT(']'), 
 *       QSE_MT('\\'), QSE_MT("n\nr\r")
 *   );
 * @endcode
 * Given [xxx]:[\rabc\ndef]:[] as an input, the example breaks the second 
 * fields to <CR>abc<NL>def where <CR> is a carriage return and <NL> is a 
 * new line.
 *
 * If you don't need translation, you may call qse_mbsspl() instead.
 * @return number of resulting fields on success, -1 on failure
 */
int qse_mbsspltrn (
	qse_mchar_t*       str,
	const qse_mchar_t* delim,
	qse_mchar_t        lquote,
	qse_mchar_t        rquote,
	qse_mchar_t        escape,
	const qse_mchar_t* trset
);

/**
 * The qse_wcsspltrn() function splits a string translating special 
 * escape sequences.
 * The argument @a trset is a translation character set which is composed
 * of multiple character pairs. An escape character followed by the 
 * first character in a pair is translated into the second character
 * in the pair. If trset is #QSE_NULL, no translation is performed. 
 *
 * Let's translate a sequence of '\n' and '\r' to a new line and a carriage
 * return respectively.
 * @code
 *   nfields = qse_wcsspltrn (
 *       str, QSE_WT(':'), QSE_WT('['), QSE_WT(']'), 
 *       QSE_WT('\\'), QSE_WT("n\nr\r")
 *   );
 * @endcode
 * Given [xxx]:[\rabc\ndef]:[] as an input, the example breaks the second 
 * fields to <CR>abc<NL>def where <CR> is a carriage return and <NL> is a 
 * new line.
 *
 * If you don't need translation, you may call qse_wcsspl() instead.
 * @return number of resulting fields on success, -1 on failure
 */
int qse_wcsspltrn (
	qse_wchar_t*       str,
	const qse_wchar_t* delim,
	qse_wchar_t        lquote,
	qse_wchar_t        rquote,
	qse_wchar_t        escape,
	const qse_wchar_t* trset
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strspl(str,delim,lquote,rquote,escape) qse_mbsspl(str,delim,lquote,rquote,escape)
#	define qse_strspltrn(str,delim,lquote,rquote,escape,trset) qse_mbsspltrn(str,delim,lquote,rquote,escape,trset)
#else
#	define qse_strspl(str,delim,lquote,rquote,escape) qse_wcsspl(str,delim,lquote,rquote,escape)
#	define qse_strspltrn(str,delim,lquote,rquote,escape,trset) qse_wcsspltrn(str,delim,lquote,rquote,escape,trset)
#endif


qse_mchar_t* qse_mbstok (
	const qse_mchar_t* s,
	const qse_mchar_t* delim, 
	qse_mcstr_t*       tok
);

qse_mchar_t* qse_mbsxtok (
	const qse_mchar_t* s,
	qse_size_t         len,
	const qse_mchar_t* delim,
	qse_mcstr_t*       tok
);

qse_mchar_t* qse_mbsxntok (
	const qse_mchar_t* s,
	qse_size_t         len,
	const qse_mchar_t* delim,
	qse_size_t         delim_len, 
	qse_mcstr_t*       tok
);

qse_wchar_t* qse_wcstok (
	const qse_wchar_t* s,
	const qse_wchar_t* delim, 
	qse_wcstr_t*       tok
);

qse_wchar_t* qse_wcsxtok (
	const qse_wchar_t* s,
	qse_size_t         len,
	const qse_wchar_t* delim,
	qse_wcstr_t*       tok
);

qse_wchar_t* qse_wcsxntok (
	const qse_wchar_t* s,
	qse_size_t         len,
	const qse_wchar_t* delim,
	qse_size_t         delim_len, 
	qse_wcstr_t*       tok
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strtok(s,d,t)          qse_mbstok(s,d,t)
#	define qse_strxtok(s,len,d,t)     qse_mbsxtok(s,len,d,t)
#	define qse_strxntok(s,len,d,dl,t) qse_mbsxntok(s,len,d,dl,t)
#else
#	define qse_strtok(s,d,t)          qse_wcstok(s,d,t)
#	define qse_strxtok(s,len,d,t)     qse_wcsxtok(s,len,d,t)
#	define qse_strxntok(s,len,d,dl,t) qse_wcsxntok(s,len,d,dl,t)
#endif

/**
 * The qse_mbstrmx() function strips leading spaces and/or trailing
 * spaces off a string depending on the opt parameter. You can form
 * the op parameter by bitwise-OR'ing one or more of the following
 * values:
 *
 * - QSE_MBSTRMX_LEFT - trim leading spaces
 * - QSE_MBSTRMX_RIGHT - trim trailing spaces
 *
 * Should it remove leading spaces, it just returns the pointer to
 * the first non-space character in the string. Should it remove trailing
 * spaces, it inserts a QSE_MT('\0') character after the last non-space
 * characters. Take note of this behavior.
 *
 * @code
 * qse_mchar_t a[] = QSE_MT("   this is a test string   ");
 * qse_mbstrmx (a, QSE_MBSTRMX_LEFT|QSE_MBSTRMX_RIGHT);
 * @endcode
 *
 * @return pointer to a trimmed string.
 */
qse_mchar_t* qse_mbstrmx (
	qse_mchar_t* str, /**< string */
	int          opt  /**< option OR'ed of #qse_mbstrmx_op_t values */
);

/**
 * The qse_wcstrmx() function strips leading spaces and/or trailing
 * spaces off a string depending on the opt parameter. You can form
 * the op parameter by bitwise-OR'ing one or more of the following
 * values:
 *
 * - QSE_WCSTRMX_LEFT - trim leading spaces
 * - QSE_WCSTRMX_RIGHT - trim trailing spaces
 *
 * Should it remove leading spaces, it just returns the pointer to
 * the first non-space character in the string. Should it remove trailing
 * spaces, it inserts a QSE_WT('\0') character after the last non-space
 * characters. Take note of this behavior.
 *
 * @code
 * qse_wchar_t a[] = QSE_WT("   this is a test string   ");
 * qse_wcstrmx (a, QSE_STRTRMX_LEFT|QSE_STRTRMX_RIGHT);
 * @endcode
 *
 * @return pointer to a trimmed string.
 */
qse_wchar_t* qse_wcstrmx (
	qse_wchar_t* str, /**< a string */
	int          opt   /**< option OR'ed of #qse_wcstrmx_op_t values */
);

/**
 * The qse_mbstrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by @a str; QSE_MT('\0') is inserted after the last non-space character.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_mbstrm (
	qse_mchar_t* str /**< string */
);

/**
 * The qse_wcstrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by @a str; QSE_WT('\0') is inserted after the last non-space character.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_wcstrm (
	qse_wchar_t* str /**< string */
);

/**
 * The qse_mbsxtrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by @a str; QSE_MT('\0') is inserted after the last non-space character.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_mbsxtrm (
	qse_mchar_t* str, /**< string */
	qse_size_t   len  /**< length */
);

/**
 * The qse_wcsxtrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by @a str; QSE_WT('\0') is inserted after the last non-space character.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_wcsxtrm (
	qse_wchar_t* str, /**< string */
	qse_size_t   len  /**< length */
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strtrmx(str,opt)  qse_mbstrmx(str,opt)
#	define qse_strtrm(str)       qse_mbstrm(str)
#	define qse_strxtrm(str,len)  qse_mbsxtrm(str,len)
#else
#	define qse_strtrmx(str,opt)  qse_wcstrmx(str,opt)
#	define qse_strtrm(str)       qse_wcstrm(str)
#	define qse_strxtrm(str,len)  qse_wcsxtrm(str,len)
#endif

/**
 * The qse_mbspac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_mbspac (
	qse_mchar_t* str /**< string */
);

/**
 * The qse_wcspac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_wcspac (
	qse_wchar_t* str /**< string */
);


/**
 * The qse_mbsxpac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_mbsxpac (
	qse_mchar_t* str, /**< string */
	qse_size_t   len  /**< length */
);

/**
 * The qse_wcsxpac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * @return length of the string without leading and trailing spaces.
 */
qse_size_t qse_wcsxpac (
	qse_wchar_t* str, /**< string */
	qse_size_t   len  /**< length */
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_strpac(str)      qse_mbspac(str)
#	define qse_strxpac(str,len) qse_mbsxpac(str,len)
#else
#	define qse_strpac(str)      qse_wcspac(str)
#	define qse_strxpac(str,len) qse_wcsxpac(str,len)
#endif

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


qse_wchar_t* qse_mbstowcsdup (
	const qse_mchar_t* mbs,
	qse_mmgr_t* mmgr
);

qse_mchar_t* qse_wcstombsdup (
	const qse_wchar_t* wcs,
	qse_mmgr_t* mmgr
);

QSE_DEFINE_COMMON_FUNCTIONS (mbs)

qse_mbs_t* qse_mbs_open (
	qse_mmgr_t* mmgr,
	qse_size_t  ext,
	qse_size_t  capa
);

void qse_mbs_close (
	qse_mbs_t* mbs
);

/**
 * The qse_mbs_init() function initializes a dynamically resizable string
 * If the parameter capa is 0, it doesn't allocate the internal buffer 
 * in advance and always succeeds.
 * @return @a mbs on success, #QSE_NULL on failure.
 */
qse_mbs_t* qse_mbs_init (
	qse_mbs_t*  mbs,
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);

/**
 * The qse_mbs_fini() function finalizes a dynamically resizable string.
 */
void qse_mbs_fini (
	qse_mbs_t* mbs
);

/**
 * The qse_mbs_yield() function assigns the buffer to an variable of the
 * #qse_mxstr_t type and recreate a new buffer of the @a new_capa capacity.
 * The function fails if it fails to allocate a new buffer.
 * @return 0 on success, and -1 on failure.
 */
int qse_mbs_yield (
	qse_mbs_t*   str,    /**< string */
	qse_mxstr_t* buf,    /**< buffer pointer */
	qse_size_t   newcapa /**< new capacity */
);

qse_mchar_t* qse_mbs_yieldptr (
	qse_mbs_t*   str,    /**< string */
	qse_size_t   newcapa /**< new capacity */
);

/**
 * The qse_mbs_getsizer() function gets the sizer.
 * @return sizer function set or QSE_NULL if no sizer is set.
 */
qse_mbs_sizer_t qse_mbs_getsizer (
	qse_mbs_t* str
);

/**
 * The qse_mbs_setsizer() function specify a new sizer for a dynamic string.
 * With no sizer specified, the dynamic string doubles the current buffer
 * when it needs to increase its size. The sizer function is passed a dynamic
 * string and the minimum capacity required to hold data after resizing.
 * The string is truncated if the sizer function returns a smaller number
 * than the hint passed.
 */
void qse_mbs_setsizer (
	qse_mbs_t*      str,
	qse_mbs_sizer_t sizer
);

/**
 * The qse_mbs_getcapa() function returns the current capacity.
 * You may use QSE_STR_CAPA(str) macro for performance sake.
 * @return current capacity in number of characters.
 */
qse_size_t qse_mbs_getcapa (
	qse_mbs_t* str
);

/**
 * The qse_mbs_setcapa() function sets the new capacity. If the new capacity
 * is smaller than the old, the overflowing characters are removed from
 * from the buffer.
 * @return (qse_size_t)-1 on failure, new capacity on success 
 */
qse_size_t qse_mbs_setcapa (
	qse_mbs_t* str,
	qse_size_t capa
);

/**
 * The qse_mbs_getlen() function return the string length.
 */
qse_size_t qse_mbs_getlen (
	qse_mbs_t* str
);

/**
 * The qse_mbs_setlen() function changes the string length.
 * @return (qse_size_t)-1 on failure, new length on success 
 */
qse_size_t qse_mbs_setlen (
	qse_mbs_t* str,
	qse_size_t len
);

/**
 * The qse_mbs_clear() funtion deletes all characters in a string and sets
 * the length to 0. It doesn't resize the internal buffer.
 */
void qse_mbs_clear (
	qse_mbs_t* str
);

/**
 * The qse_mbs_swap() function exchanges the pointers to a buffer between
 * two strings. It updates the length and the capacity accordingly.
 */
void qse_mbs_swap (
	qse_mbs_t* str1,
	qse_mbs_t* str2
);

qse_size_t qse_mbs_cpy (
	qse_mbs_t*         str,
	const qse_mchar_t* s
);

qse_size_t qse_mbs_ncpy (
	qse_mbs_t*         str,
	const qse_mchar_t* s,
	qse_size_t         len
);

qse_size_t qse_mbs_cat (
	qse_mbs_t*         str,
	const qse_mchar_t* s
);

qse_size_t qse_mbs_ncat (
	qse_mbs_t*         str,
	const qse_mchar_t* s,
	qse_size_t         len
);

qse_size_t qse_mbs_ccat (
	qse_mbs_t*  str,
	qse_mchar_t c
);

qse_size_t qse_mbs_nccat (
	qse_mbs_t*  str,
	qse_mchar_t c,
	qse_size_t  len
);

qse_size_t qse_mbs_del (
	qse_mbs_t* str,
	qse_size_t index,
	qse_size_t size
);

qse_size_t qse_mbs_trm (
	qse_mbs_t* str
);

qse_size_t qse_mbs_pac (
	qse_mbs_t* str
);

QSE_DEFINE_COMMON_FUNCTIONS (wcs)

qse_wcs_t* qse_wcs_open (
	qse_mmgr_t* mmgr,
	qse_size_t  ext,
	qse_size_t  capa
);

void qse_wcs_close (
	qse_wcs_t* wcs
);

/**
 * The qse_wcs_init() function initializes a dynamically resizable string
 * If the parameter capa is 0, it doesn't allocate the internal buffer 
 * in advance and always succeeds.
 * @return @a wcs on success, #QSE_NULL on failure.
 */
qse_wcs_t* qse_wcs_init (
	qse_wcs_t*  wcs,
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);

/**
 * The qse_wcs_fini() function finalizes a dynamically resizable string.
 */
void qse_wcs_fini (
	qse_wcs_t* wcs
);

/**
 * The qse_wcs_yield() function assigns the buffer to an variable of the
 * #qse_wxstr_t type and recreate a new buffer of the @a new_capa capacity.
 * The function fails if it fails to allocate a new buffer.
 * @return 0 on success, and -1 on failure.
 */
int qse_wcs_yield (
	qse_wcs_t*   str,     /**< string */
	qse_wxstr_t* buf,     /**< buffer pointer */
	qse_size_t   new_capa /**< new capacity */
);

qse_wchar_t* qse_wcs_yieldptr (
	qse_wcs_t*   str,    /**< string */
	qse_size_t   newcapa /**< new capacity */
);

/**
 * The qse_wcs_getsizer() function gets the sizer.
 * @return sizer function set or QSE_NULL if no sizer is set.
 */
qse_wcs_sizer_t qse_wcs_getsizer (
	qse_wcs_t* str
);

/**
 * The qse_wcs_setsizer() function specify a new sizer for a dynamic string.
 * With no sizer specified, the dynamic string doubles the current buffer
 * when it needs to increase its size. The sizer function is passed a dynamic
 * string and the minimum capacity required to hold data after resizing.
 * The string is truncated if the sizer function returns a smaller number
 * than the hint passed.
 */
void qse_wcs_setsizer (
	qse_wcs_t*      str,
	qse_wcs_sizer_t sizer
);

/**
 * The qse_wcs_getcapa() function returns the current capacity.
 * You may use QSE_STR_CAPA(str) macro for performance sake.
 * @return current capacity in number of characters.
 */
qse_size_t qse_wcs_getcapa (
	qse_wcs_t* str
);

/**
 * The qse_wcs_setcapa() function sets the new capacity. If the new capacity
 * is smaller than the old, the overflowing characters are removed from
 * from the buffer.
 * @return (qse_size_t)-1 on failure, new capacity on success 
 */
qse_size_t qse_wcs_setcapa (
	qse_wcs_t* str,
	qse_size_t capa
);

/**
 * The qse_wcs_getlen() function return the string length.
 */
qse_size_t qse_wcs_getlen (
	qse_wcs_t* str
);

/**
 * The qse_wcs_setlen() function changes the string length.
 * @return (qse_size_t)-1 on failure, new length on success 
 */
qse_size_t qse_wcs_setlen (
	qse_wcs_t* str,
	qse_size_t len
);

/**
 * The qse_wcs_clear() funtion deletes all characters in a string and sets
 * the length to 0. It doesn't resize the internal buffer.
 */
void qse_wcs_clear (
	qse_wcs_t* str
);

/**
 * The qse_wcs_swap() function exchanges the pointers to a buffer between
 * two strings. It updates the length and the capacity accordingly.
 */
void qse_wcs_swap (
	qse_wcs_t* str1,
	qse_wcs_t* str2
);

qse_size_t qse_wcs_cpy (
	qse_wcs_t*         str,
	const qse_wchar_t* s
);

qse_size_t qse_wcs_ncpy (
	qse_wcs_t*         str,
	const qse_wchar_t* s,
	qse_size_t         len
);

qse_size_t qse_wcs_cat (
	qse_wcs_t*         str,
	const qse_wchar_t* s
);

qse_size_t qse_wcs_ncat (
	qse_wcs_t*         str,
	const qse_wchar_t* s,
	qse_size_t         len
);

qse_size_t qse_wcs_ccat (
	qse_wcs_t*  str,
	qse_wchar_t c
);

qse_size_t qse_wcs_nccat (
	qse_wcs_t*  str,
	qse_wchar_t c,
	qse_size_t  len
);

qse_size_t qse_wcs_del (
	qse_wcs_t* str,
	qse_size_t index,
	qse_size_t size
);

qse_size_t qse_wcs_trm (
	qse_wcs_t* str
);

qse_size_t qse_wcs_pac (
	qse_wcs_t* str
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_str_setmmgr(str,mmgr)    qse_mbs_wetmmgr(str,mmgr)
#	define qse_str_getmmgr(str)         qse_mbs_getmmgr(str)
#	define qse_str_open(mmgr,ext,capa)  qse_mbs_open(mmgr,ext,capa)
#	define qse_str_close(str)           qse_mbs_close(str)
#	define qse_str_init(str,mmgr,capa)  qse_mbs_init(str,mmgr,capa)
#	define qse_str_fini(str)            qse_mbs_fini(str)
#	define qse_str_yield(str,buf,ncapa) qse_mbs_yield(str,buf,ncapa)
#	define qse_str_yieldptr(str,ncapa)  qse_mbs_yieldptr(str,ncapa)
#	define qse_str_getsizer(str)        qse_mbs_getsizer(str)
#	define qse_str_setsizer(str,sizer)  qse_mbs_setsizer(str,sizer)
#	define qse_str_getcapa(str)         qse_mbs_getcapa(str)
#	define qse_str_setcapa(str,capa)    qse_mbs_setcapa(str,capa)
#	define qse_str_getlen(str)          qse_mbs_getlen(str)
#	define qse_str_setlen(str,len)      qse_mbs_setlen(str,len)
#	define qse_str_clear(str)           qse_mbs_clear(str)
#	define qse_str_swap(str1,str2)      qse_mbs_swap(str1,str2)
#	define qse_str_cpy(str,s)           qse_mbs_cpy(str,s)
#	define qse_str_ncpy(str,s,len)      qse_mbs_ncpy(str,s,len)
#	define qse_str_cat(str,s)           qse_mbs_cat(str,s)
#	define qse_str_ncat(str,s,len)      qse_mbs_ncat(str,s,len)
#	define qse_str_ccat(str,c)          qse_mbs_ccat(str,c)
#	define qse_str_nccat(str,c,len)     qse_mbs_nccat(str,c,len)
#	define qse_str_del(str,index,size)  qse_mbs_del(str,index,size)
#	define qse_str_trm(str)             qse_mbs_trm(str)
#	define qse_str_pac(str)             qse_mbs_pac(str)
#else
#	define qse_str_setmmgr(str,mmgr)    qse_wcs_wetmmgr(str,mmgr)
#	define qse_str_getmmgr(str)         qse_wcs_getmmgr(str)
#	define qse_str_open(mmgr,ext,capa)  qse_wcs_open(mmgr,ext,capa)
#	define qse_str_close(str)           qse_wcs_close(str)
#	define qse_str_init(str,mmgr,capa)  qse_wcs_init(str,mmgr,capa)
#	define qse_str_fini(str)            qse_wcs_fini(str)
#	define qse_str_yield(str,buf,ncapa) qse_wcs_yield(str,buf,ncapa)
#	define qse_str_yieldptr(str,ncapa)  qse_wcs_yieldptr(str,ncapa)
#	define qse_str_getsizer(str)        qse_wcs_getsizer(str)
#	define qse_str_setsizer(str,sizer)  qse_wcs_setsizer(str,sizer)
#	define qse_str_getcapa(str)         qse_wcs_getcapa(str)
#	define qse_str_setcapa(str,capa)    qse_wcs_setcapa(str,capa)
#	define qse_str_getlen(str)          qse_wcs_getlen(str)
#	define qse_str_setlen(str,len)      qse_wcs_setlen(str,len)
#	define qse_str_clear(str)           qse_wcs_clear(str)
#	define qse_str_swap(str1,str2)      qse_wcs_swap(str1,str2)
#	define qse_str_cpy(str,s)           qse_wcs_cpy(str,s)
#	define qse_str_ncpy(str,s,len)      qse_wcs_ncpy(str,s,len)
#	define qse_str_cat(str,s)           qse_wcs_cat(str,s)
#	define qse_str_ncat(str,s,len)      qse_wcs_ncat(str,s,len)
#	define qse_str_ccat(str,c)          qse_wcs_ccat(str,c)
#	define qse_str_nccat(str,c,len)     qse_wcs_nccat(str,c,len)
#	define qse_str_del(str,index,size)  qse_wcs_del(str,index,size)
#	define qse_str_trm(str)             qse_wcs_trm(str)
#	define qse_str_pac(str)             qse_wcs_pac(str)
#endif


#ifdef __cplusplus
}
#endif

#endif
