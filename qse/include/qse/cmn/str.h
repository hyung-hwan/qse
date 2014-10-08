/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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
#include <stdarg.h>

/** \file
 * This file provides various functions, types, macros for string manipulation.
 *
 * The #qse_cstr_t type and the #qse_cstr_t defined in <qse/types.h> help you
 * deal with a string pointer and length in a structure.
 */

/** string pointer and length as a aggregate */
#define QSE_MBS_XSTR(s)     (&((s)->val))  
/** string length */
#define QSE_MBS_LEN(s)      ((s)->val.len)
/** string pointer */
#define QSE_MBS_PTR(s)      ((s)->val.ptr)
/** pointer to a particular position */
#define QSE_MBS_CPTR(s,idx) (&(s)->val.ptr[idx])
/** string capacity */
#define QSE_MBS_CAPA(s)     ((s)->capa)
/** character at the given position */
#define QSE_MBS_CHAR(s,idx) ((s)->val.ptr[idx]) 
/**< last character. unsafe if length <= 0 */
#define QSE_MBS_LASTCHAR(s) ((s)->val.ptr[(s)->val.len-1])

/** string pointer and length as a aggregate */
#define QSE_WCS_XSTR(s)     (&((s)->val))  
/** string length */
#define QSE_WCS_LEN(s)      ((s)->val.len)
/** string pointer */
#define QSE_WCS_PTR(s)      ((s)->val.ptr)
/** pointer to a particular position */
#define QSE_WCS_CPTR(s,idx) (&(s)->val.ptr[idx])
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

#if defined(QSE_CHAR_IS_MCHAR)
#	define QSE_STR_XSTR(s)     ((qse_cstr_t*)QSE_MBS_XSTR(s))
#	define QSE_STR_LEN(s)      QSE_MBS_LEN(s)
#	define QSE_STR_PTR(s)      QSE_MBS_PTR(s)
#	define QSE_STR_CPTR(s,idx) QSE_MBS_CPTR(s,idx)
#	define QSE_STR_CAPA(s)     QSE_MBS_CAPA(s)
#	define QSE_STR_CHAR(s,idx) QSE_MBS_CHAR(s,idx)
#	define QSE_STR_LASTCHAR(s) QSE_MBS_LASTCHAR(s)
#	define qse_str_t           qse_mbs_t
#	define qse_str_sizer_t     qse_mbs_sizer_t
#else
#	define QSE_STR_XSTR(s)     ((qse_cstr_t*)QSE_WCS_XSTR(s))
#	define QSE_STR_LEN(s)      QSE_WCS_LEN(s)
#	define QSE_STR_PTR(s)      QSE_WCS_PTR(s)
#	define QSE_STR_CPTR(s,idx) QSE_WCS_CPTR(s,idx)
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
	qse_mmgr_t*     mmgr;
	qse_mbs_sizer_t sizer; /**< buffer resizer function */
	qse_mcstr_t     val;   /**< buffer/string pointer and lengh */
	qse_size_t      capa;  /**< buffer capacity */
};

/**
 * The qse_wcs_t type defines a dynamically resizable wide-character string.
 */
struct qse_wcs_t
{
	qse_mmgr_t*     mmgr;
	qse_wcs_sizer_t sizer; /**< buffer resizer function */
	qse_wcstr_t     val;   /**< buffer/string pointer and lengh */
	qse_size_t      capa;  /**< buffer capacity */
};


#define QSE_MBSSUBST_NOBUF ((qse_mchar_t*)1)
#define QSE_WCSSUBST_NOBUF ((qse_wchar_t*)1)

/**
 * The qse_mbssubst_t type defines a callback function for qse_mbsxsubst()
 * and qse_mbsxnsubst() to substitue a new value for an identifier \a ident.
 */
typedef qse_mchar_t* (*qse_mbssubst_t) (
	qse_mchar_t*       buf, 
	qse_size_t         bsz, 
	const qse_mcstr_t* ident, 
	void*              ctx
);

/**
 * The qse_wcssubst_t type defines a callback function for qse_wcsxsubst()
 * and qse_wcsxnsubst() to substitue a new value for an identifier \a ident.
 */
typedef qse_wchar_t* (*qse_wcssubst_t) (
	qse_wchar_t*       buf, 
	qse_size_t         bsz, 
	const qse_wcstr_t* ident, 
	void*              ctx
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define QSE_STRSUBST_NOBUF QSE_MBSSUBST_NOBUF
#	define qse_strsubst_t qse_mbssubst_t
#else
#	define QSE_STRSUBST_NOBUF QSE_WCSSUBST_NOBUF
#	define qse_strsubst_t qse_wcssubst_t
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
		else if (__ston_c == QSE_T('-')) { __ston_f++; __ston_ptr++; } \
		else if (__ston_c == QSE_T('+')) { __ston_ptr++; } \
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
		else if (__ston_c == QSE_T('-')) { __ston_f++; __ston_ptr++; } \
		else if (__ston_c == QSE_T('+')) { __ston_ptr++; } \
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
#define QSE_MBSTONUM(value,nptr,endptr,base) do {\
	int __ston_f = 0, __ston_v; \
	const qse_mchar_t* __ston_ptr = nptr; \
	for (;;) { \
		qse_mchar_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_MT(' ') || \
		    __ston_c == QSE_MT('\t')) { __ston_ptr++; continue; } \
		else if (__ston_c == QSE_MT('-')) { __ston_f++; __ston_ptr++; } \
		else if (__ston_c == QSE_MT('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; (__ston_v = QSE_MCHARTONUM(*__ston_ptr, base)) < base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr) *((const qse_mchar_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
} while(0)

/* qse_strxtonum (const qse_mchar_t* nptr, qse_size_t len, qse_mchar_t** endptr, int base) */
#define QSE_MBSXTONUM(value,nptr,len,endptr,base) do {\
	int __ston_f = 0, __ston_v; \
	const qse_mchar_t* __ston_ptr = nptr; \
	const qse_mchar_t* __ston_end = __ston_ptr + len; \
	value = 0; \
	while (__ston_ptr < __ston_end) { \
		qse_mchar_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_MT(' ') || __ston_c == QSE_MT('\t')) { \
			__ston_ptr++; continue; \
		} \
		else if (__ston_c == QSE_MT('-')) { __ston_f++; __ston_ptr++; } \
		else if (__ston_c == QSE_MT('+')) { __ston_ptr++; } \
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
#define QSE_WCSTONUM(value,nptr,endptr,base) do {\
	int __ston_f = 0, __ston_v; \
	const qse_wchar_t* __ston_ptr = nptr; \
	for (;;) { \
		qse_wchar_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_WT(' ') || \
		    __ston_c == QSE_WT('\t')) { __ston_ptr++; continue; } \
		else if (__ston_c == QSE_WT('-')) { __ston_f++; __ston_ptr++; } \
		else if (__ston_c == QSE_WT('+')) { __ston_ptr++; } \
		break; \
	} \
	for (value = 0; (__ston_v = QSE_WCHARTONUM(*__ston_ptr, base)) < base; __ston_ptr++) { \
		value = value * base + __ston_v; \
	} \
	if (endptr) *((const qse_wchar_t**)endptr) = __ston_ptr; \
	if (__ston_f > 0) value *= -1; \
} while(0)

/* qse_strxtonum (const qse_wchar_t* nptr, qse_size_t len, qse_wchar_t** endptr, int base) */
#define QSE_WCSXTONUM(value,nptr,len,endptr,base) do {\
	int __ston_f = 0, __ston_v; \
	const qse_wchar_t* __ston_ptr = nptr; \
	const qse_wchar_t* __ston_end = __ston_ptr + len; \
	value = 0; \
	while (__ston_ptr < __ston_end) { \
		qse_wchar_t __ston_c = *__ston_ptr; \
		if (__ston_c == QSE_WT(' ') || __ston_c == QSE_WT('\t')) { \
			__ston_ptr++; continue; \
		} \
		else if (__ston_c == QSE_WT('-')) { __ston_f++; __ston_ptr++; } \
		else if (__ston_c == QSE_WT('+')) { __ston_ptr++; } \
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
 * The qse_mbstrmx_flag_t defines a string trimming operation. 
 */
enum qse_mbstrmx_flag_t
{
	QSE_MBSTRMX_LEFT  = (1 << 0), /**< trim leading spaces */
#define QSE_MBSTRMX_LEFT QSE_MBSTRMX_LEFT
	QSE_MBSTRMX_RIGHT = (1 << 1)  /**< trim trailing spaces */
#define QSE_MBSTRMX_RIGHT QSE_MBSTRMX_RIGHT
};

/**
 * The qse_wcstrmx_flag_t defines a string trimming operation. 
 */
enum qse_wcstrmx_flag_t
{
	QSE_WCSTRMX_LEFT  = (1 << 0), /**< trim leading spaces */
#define QSE_WCSTRMX_LEFT QSE_WCSTRMX_LEFT
	QSE_WCSTRMX_RIGHT = (1 << 1)  /**< trim trailing spaces */
#define QSE_WCSTRMX_RIGHT QSE_WCSTRMX_RIGHT
};

#if defined(QSE_CHAR_IS_MCHAR)
#	define QSE_STRTRMX_LEFT QSE_MBSTRMX_LEFT
#	define QSE_STRTRMX_RIGHT QSE_MBSTRMX_RIGHT
#else
#	define QSE_STRTRMX_LEFT QSE_WCSTRMX_LEFT
#	define QSE_STRTRMX_RIGHT QSE_WCSTRMX_RIGHT
#endif

enum qse_mbsfnmat_flag_t
{
	QSE_MBSFNMAT_PATHNAME   = (1 << 0),
#define QSE_MBSFNMAT_PATHNAME   QSE_MBSFNMAT_PATHNAME
	QSE_MBSFNMAT_NOESCAPE   = (1 << 1),
#define QSE_MBSFNMAT_NOESCAPE   QSE_MBSFNMAT_NOESCAPE
	QSE_MBSFNMAT_PERIOD     = (1 << 2),
#define QSE_MBSFNMAT_PERIOD     QSE_MBSFNMAT_PERIOD
	QSE_MBSFNMAT_IGNORECASE = (1 << 3)
#define QSE_MBSFNMAT_IGNORECASE QSE_MBSFNMAT_IGNORECASE
};

enum qse_wcsfnmat_flag_t
{
	QSE_WCSFNMAT_PATHNAME   = (1 << 0),
#define QSE_WCSFNMAT_PATHNAME   QSE_WCSFNMAT_PATHNAME
	QSE_WCSFNMAT_NOESCAPE   = (1 << 1),
#define QSE_WCSFNMAT_NOESCAPE   QSE_WCSFNMAT_NOESCAPE
	QSE_WCSFNMAT_PERIOD     = (1 << 2),
#define QSE_WCSFNMAT_PERIOD     QSE_WCSFNMAT_PERIOD
	QSE_WCSFNMAT_IGNORECASE = (1 << 3)
#define QSE_WCSFNMAT_IGNORECASE QSE_WCSFNMAT_IGNORECASE
};

#if defined(QSE_CHAR_IS_MCHAR)
#	define QSE_STRFNMAT_PATHNAME   QSE_MBSFNMAT_PATHNAME
#	define QSE_STRFNMAT_NOESCAPE   QSE_MBSFNMAT_NOESCAPE
#	define QSE_STRFNMAT_PERIOD     QSE_MBSFNMAT_PERIOD
#	define QSE_STRFNMAT_IGNORECASE QSE_MBSFNMAT_IGNORECASE
#else
#	define QSE_STRFNMAT_PATHNAME   QSE_WCSFNMAT_PATHNAME
#	define QSE_STRFNMAT_NOESCAPE   QSE_WCSFNMAT_NOESCAPE
#	define QSE_STRFNMAT_PERIOD     QSE_WCSFNMAT_PERIOD
#	define QSE_STRFNMAT_IGNORECASE QSE_WCSFNMAT_IGNORECASE
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
QSE_EXPORT qse_size_t qse_mbslen (
	const qse_mchar_t* mbs
);

/**
 * The qse_wcslen() function returns the number of characters in a 
 * wide-character null-terminated string. The length returned excludes 
 * a terminating null.
 */
QSE_EXPORT qse_size_t qse_wcslen (
	const qse_wchar_t* wcs
);

/**
 * The qse_mbsbytes() function returns the number of bytes a null-terminated
 * string is holding excluding a terminating null.
 */
QSE_EXPORT qse_size_t qse_mbsbytes (
	const qse_mchar_t* str
);

/**
 * The qse_wcsbytes() function returns the number of bytes a null-terminated
 * string is holding excluding a terminating null.
 */
QSE_EXPORT qse_size_t qse_wcsbytes (
	const qse_wchar_t* str
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strlen(str)   qse_mbslen(str)
#	define qse_strbytes(str) qse_mbsbytes(str)
#else
#	define qse_strlen(str)   qse_wcslen(str)
#	define qse_strbytes(str) qse_wcsbytes(str)
#endif

QSE_EXPORT qse_size_t qse_mbscpy (
	qse_mchar_t*       buf,
	const qse_mchar_t* str
);

QSE_EXPORT qse_size_t qse_wcscpy (
	qse_wchar_t*       buf,
	const qse_wchar_t* str
);

QSE_EXPORT qse_size_t qse_mbsxcpy (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* str
);

QSE_EXPORT qse_size_t qse_wcsxcpy (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* str
);

/**
 * The qse_mbsncpy() function copies a length-bounded string into
 * a buffer with unknown size. 
 */
QSE_EXPORT qse_size_t qse_mbsncpy (
	qse_mchar_t*       buf, /**< buffer with unknown length */
	const qse_mchar_t* str, /**< length-bounded string */
	qse_size_t         len  /**< string length */
);

/**
 * The qse_wcsncpy() function copies a length-bounded string into
 * a buffer with unknown size. 
 */
QSE_EXPORT qse_size_t qse_wcsncpy (
	qse_wchar_t*       buf, /**< buffer with unknown length */
	const qse_wchar_t* str, /**< length-bounded string */
	qse_size_t         len  /**< string length */
);

/**
 * The qse_mbsxncpy() function copies a length-bounded string into
 * a length-bounded buffer.
 */
QSE_EXPORT qse_size_t qse_mbsxncpy (
	qse_mchar_t*       buf, /**< length-bounded buffer */
	qse_size_t         bsz, /**< buffer length */
	const qse_mchar_t* str, /**< length-bounded string */
	qse_size_t         len  /**< string length */
);

/**
 * The qse_wcsxncpy() function copies a length-bounded string into
 * a length-bounded buffer.
 */
QSE_EXPORT qse_size_t qse_wcsxncpy (
	qse_wchar_t*       buf, /**< length-bounded buffer */
	qse_size_t         bsz, /**< buffer length */
	const qse_wchar_t* str, /**< length-bounded string */
	qse_size_t         len  /**< string length */
);

#if defined(QSE_CHAR_IS_MCHAR)
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

QSE_EXPORT qse_size_t qse_mbsput (
	qse_mchar_t*       buf, 
	const qse_mchar_t* str
);

QSE_EXPORT qse_size_t qse_wcsput (
	qse_wchar_t*       buf, 
	const qse_wchar_t* str
);

/**
 * The qse_mbsxput() function copies the string \a str into the buffer \a buf
 * of the size \a bsz. Unlike qse_strxcpy(), it does not null-terminate the
 * buffer.
 */
QSE_EXPORT qse_size_t qse_mbsxput (
	qse_mchar_t*       buf, 
	qse_size_t         bsz,
	const qse_mchar_t* str
);

/**
 * The qse_wcsxput() function copies the string \a str into the buffer \a buf
 * of the size \a bsz. Unlike qse_strxcpy(), it does not null-terminate the
 * buffer.
 */
QSE_EXPORT qse_size_t qse_wcsxput (
	qse_wchar_t*       buf, 
	qse_size_t         bsz,
	const qse_wchar_t* str
);

QSE_EXPORT qse_size_t qse_mbsxnput (
	qse_mchar_t*       buf,
	qse_size_t        bsz,
	const qse_mchar_t* str,
	qse_size_t        len
);

QSE_EXPORT qse_size_t qse_wcsxnput (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* str,
	qse_size_t         len
);

#if defined(QSE_CHAR_IS_MCHAR)
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
 * \code
 *  qse_char_t buf[256]
 *  qse_char_t* colors[] = { QSE_MT("blue"), QSE_MT("green"), QSE_MT("red") };
 *  qse_mbsfcpy(buf, QSE_MT("RGB: ${2}, ${1}, ${0}"), colors);
 * \endcode
 * \sa qse_mbsfncpy, qse_mbsxfcpy, qse_mbsxfncpy
 */
QSE_EXPORT qse_size_t qse_mbsfcpy (
	qse_mchar_t*       buf,
	const qse_mchar_t* fmt,
	const qse_mchar_t* str[]
);

/**
 * The qse_wcsfcpy() function formats a string by position.
 * The position specifier is a number enclosed in ${ and }.
 * When ${ is preceeded by a backslash, it is treated literally. 
 * See the example below:
 * \code
 *  qse_char_t buf[256]
 *  qse_char_t* colors[] = { QSE_WT("blue"), QSE_WT("green"), QSE_WT("red") };
 *  qse_wcsfcpy(buf, QSE_WT("RGB: ${2}, ${1}, ${0}"), colors);
 * \endcode
 * \sa qse_wcsfncpy, qse_wcsxfcpy, qse_wcsxfncpy
 */
QSE_EXPORT qse_size_t qse_wcsfcpy (
	qse_wchar_t*       buf,
	const qse_wchar_t* fmt,
	const qse_wchar_t* str[]
);

/**
 * The qse_mbsfncpy() function formats a string by position.
 * It differs from qse_mbsfcpy() in that \a str is an array of the 
 * #qse_mcstr_t type.
 * \sa qse_mbsfcpy, qse_mbsxfcpy, qse_mbsxfncpy
 */
QSE_EXPORT qse_size_t qse_mbsfncpy (
	qse_mchar_t*       buf,
	const qse_mchar_t* fmt,
	const qse_mcstr_t  str[]
);

/**
 * The qse_wcsfncpy() function formats a string by position.
 * It differs from qse_wcsfcpy() in that \a str is an array of the 
 * #qse_wcstr_t type.
 * \sa qse_wcsfcpy, qse_wcsxfcpy, qse_wcsxfncpy
 */
QSE_EXPORT qse_size_t qse_wcsfncpy (
	qse_wchar_t*       buf,
	const qse_wchar_t* fmt,
	const qse_wcstr_t  str[]
);

/**
 * The qse_mbsxfcpy() function formats a string by position.
 * It differs from qse_strfcpy() in that \a buf is length-bounded of \a bsz
 * characters.
 * \code
 *  qse_mchar_t buf[256]
 *  qse_mchar_t* colors[] = { QSE_MT("blue"), QSE_MT("green"), QSE_MT("red") };
 *  qse_mbsxfcpy(buf, QSE_COUNTOF(buf), QSE_MT("RGB: ${2}, ${1}, ${0}"), colors);
 * \endcode
 * \sa qse_mbsfcpy, qse_mbsfncpy, qse_mbsxfncpy
 */
QSE_EXPORT qse_size_t qse_mbsxfcpy (
	qse_mchar_t*       buf,
	qse_size_t         bsz, 
	const qse_mchar_t* fmt,
	const qse_mchar_t* str[]
);

/**
 * The qse_wcsxfcpy() function formats a string by position.
 * It differs from qse_wcsfcpy() in that \a buf is length-bounded of \a bsz
 * characters.
 * \code
 *  qse_char_t buf[256]
 *  qse_char_t* colors[] = { QSE_WT("blue"), QSE_WT("green"), QSE_WT("red") };
 *  qse_wcsxfcpy(buf, QSE_COUNTOF(buf), QSE_WT("RGB: ${2}, ${1}, ${0}"), colors);
 * \endcode
 * \sa qse_wcsfcpy, qse_wcsfncpy, qse_wcsxfncpy
 */
QSE_EXPORT qse_size_t qse_wcsxfcpy (
	qse_wchar_t*       buf,
	qse_size_t         bsz, 
	const qse_wchar_t* fmt,
	const qse_wchar_t* str[]
);

/**
 * The qse_mbsxfncpy() function formats a string by position.
 * It differs from qse_strfcpy() in that \a buf is length-bounded of \a bsz
 * characters and \a str is an array of the #qse_mcstr_t type.
 * \sa qse_mbsfcpy, qse_mbsfncpy, qse_mbsxfcpy
 */
QSE_EXPORT qse_size_t qse_mbsxfncpy (
	qse_mchar_t*       buf,
	qse_size_t         bsz, 
	const qse_mchar_t* fmt,
	const qse_mcstr_t  str[]
);

/**
 * The qse_wcsxfncpy() function formats a string by position.
 * It differs from qse_strfcpy() in that \a buf is length-bounded of \a bsz
 * characters and \a str is an array of the #qse_wcstr_t type.
 * \sa qse_wcsfcpy, qse_wcsfncpy, qse_wcsxfcpy
 */
QSE_EXPORT qse_size_t qse_wcsxfncpy (
	qse_wchar_t*       buf,
	qse_size_t         bsz, 
	const qse_wchar_t* fmt,
	const qse_wcstr_t  str[]
);

#if defined(QSE_CHAR_IS_MCHAR)
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


QSE_EXPORT qse_size_t qse_mbsfmt (
	qse_mchar_t*       buf,
	const qse_mchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_mbsvfmt (
	qse_mchar_t*       buf,
	const qse_mchar_t* fmt,
	va_list            ap
);

/**
 * The qse_mbsxfmt() function writes a formatted string into the buffer \a buf
 * of the size \a bsz using the format \a fmt and the variable arguments. It
 * doesn't write more than bsz characters including the terminating null 
 * chracter. If \a buf is #QSE_NULL, no characters are written to the buffer.
 * If conversion from a wide character string fails, the formatting is aborted
 * and the output buffer gets null-terminated.
 * 
 * \return 
 *   It returns the number of characters written to the buffer excluding the 
 *   terminating null. When \a buf is #QSE_NULL, it returns the number of 
 *   characters that would have been written excluding the terminating null
 *   if \a buf has not been #QSE_NULL. If conversion from a wide-character 
 *   string fails, it returns (qse_size_t)-1.
 */
QSE_EXPORT qse_size_t qse_mbsxfmt (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_mbsxvfmt (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* fmt,
	va_list            ap
);

QSE_EXPORT qse_size_t qse_mbsfmts (
	qse_mchar_t*       buf,
	const qse_mchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_mbsvfmts (
	qse_mchar_t*       buf,
	const qse_mchar_t* fmt,
	va_list            ap
);

/**
 * The qse_mbsxfmt() function behaves the same as qse_mbsxfmt() except
 * that it returns (qse_size_t)-1 if the buffer is not large enough. 
 */
QSE_EXPORT qse_size_t qse_mbsxfmts (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_mbsxvfmts (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* fmt,
	va_list            ap
);

QSE_EXPORT qse_size_t qse_wcsfmt (
	qse_wchar_t*       buf,
	const qse_wchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_wcsvfmt (
	qse_wchar_t*       buf,
	const qse_wchar_t* fmt,
	va_list            ap
);

/**
 * The qse_wcsxfmt() function writes a formatted string into the buffer \a buf
 * of the size \a bsz using the format \a fmt and the variable arguments. It
 * doesn't write more than bsz characters including the terminating null 
 * chracter. If \a buf is #QSE_NULL, no characters are written to the buffer.
 * If conversion from a multi-byte string fails, the formatting is aborted and
 * the output buffer gets null-terminated.
 * 
 * \return 
 *   It returns the number of characters written to the buffer excluding the 
 *   terminating null. When \a buf is #QSE_NULL, it returns the number of 
 *   characters that would have been written excluding the terminating null
 *   if \a buf has not been #QSE_NULL. If conversion from a multi-byte string
 *   fails, it returns (qse_size_t)-1.
 */
QSE_EXPORT qse_size_t qse_wcsxfmt (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_wcsxvfmt (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* fmt,
	va_list            ap
);

QSE_EXPORT qse_size_t qse_wcsfmts (
	qse_wchar_t*       buf,
	const qse_wchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_wcsvfmts (
	qse_wchar_t*       buf,
	const qse_wchar_t* fmt,
	va_list            ap
);

/**
 * The qse_wcsxfmts() function behaves the same as qse_wcsxfmt() except
 * that it returns (qse_size_t)-1 if the buffer is not large enough.
 */
QSE_EXPORT qse_size_t qse_wcsxfmts (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_wcsxvfmts (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* fmt,
	va_list            ap
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strfmt qse_mbsfmt
#	define qse_strvfmt qse_mbsvfmt
#	define qse_strxfmt qse_mbsxfmt
#	define qse_strxvfmt qse_mbsxvfmt

#	define qse_strfmts qse_mbsfmts
#	define qse_strvfmts qse_mbsvfmts
#	define qse_strxfmts qse_mbsxfmts
#	define qse_strxvfmts qse_mbsxvfmts
#else
#	define qse_strfmt qse_wcsfmt
#	define qse_strvfmt qse_wcsvfmt
#	define qse_strxfmt qse_wcsxfmt
#	define qse_strxvfmt qse_wcsxvfmt

#	define qse_strfmts qse_wcsfmts
#	define qse_strvfmts qse_wcsvfmts
#	define qse_strxfmts qse_wcsxfmts
#	define qse_strxvfmts qse_wcsxvfmts
#endif

/**
 * The qse_mbsxsubst() function expands \a fmt into a buffer \a buf of the size
 * \a bsz by substituting new values for ${} segments within it. The actual
 * substitution is made by invoking the callback function \a subst. 
 * \code
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
 * \endcode
 * 
 * If \a buf is #QSE_MBSSUBST_NOBUF, the function returns the length of the 
 * resulting string that a buffer large enough would be able to hold. You can 
 * pass #QSE_MBSSUBST_NOBUF to find out how large a buffer you need to perform 
 * substitution without truncation. The substitution callback function should 
 * handle this special case for buffer size prediction. The sample callback shown
 * above can be extended to return the buffer pointer without actual copying 
 * if the buffer points to #QSE_MBSSUBST_NOBUF as shown belown.
 *
 * \code
 * qse_mchar_t* subst (qse_mchar_t* buf, qse_size_t bsz, const qse_mcstr_t* ident, void* ctx)
 * { 
 *   if (qse_mcsxcmp (ident->ptr, ident->len, QSE_MT("USER")) == 0)
 *     return buf + (buf == QSE_MBSSUBST_NOBUF? 3: qse_mcsxput (buf, bsz, QSE_MT("sam")));
 *   else if (qse_mcsxcmp (ident->ptr, ident->len, QSE_MT("GROUP")) == 0)
 *     return buf + (buf == QSE_MBSSUBST_NOBUF? 6: qse_mcsxput (buf, bsz, QSE_MT("coders")));	
 *   return buf; 
 * }
 * 
 * len = qse_mbsxsubst (QSE_MBSSUBST_NOBUF, 0, QSE_MT("user=${USER},group=${GROUP}"), subst, QSE_NULL);
 * buf = malloc (QSE_SIZEOF(qse_mbs_t) * (len + 1));
 * qse_mbsxsubst (buf, len + 1, QSE_MT("user=${USER},group=${GROUP}"), subst, QSE_NULL);
 * \endcode
 *
 * A ${} segment may contain a default value. For example, the segment 
 * ${USER:=Unknown} translates to Unknown if the callback function returns 
 * #QSE_NULL for USER. The default value part may contain ${} segments 
 * recursively.
 *
 * \code
 * qse_mchar_t* subst (qse_mchar_t* buf, qse_size_t bsz, const qse_mcstr_t* ident, void* ctx)
 * { 
 *   if (qse_mbsxcmp (ident->ptr, ident->len, QSE_MT("USER")) == 0)
 *     return QSE_NULL;
 *   else if (qse_mbsxcmp (ident->ptr, ident->len, QSE_MT("GROUP")) == 0)
 *     return buf + (buf == QSE_MBSSUBST_NOBUF? 6: qse_wcsxput (buf, bsz, QSE_MT("coders")));	
 *   return buf; 
 * }
 * \endcode
 */
QSE_EXPORT qse_size_t qse_mbsxsubst (
	qse_mchar_t*           buf,
	qse_size_t             bsz,
	const qse_mchar_t*     fmt,
	qse_mbssubst_t         subst,
	void*                  ctx
);

QSE_EXPORT qse_size_t qse_mbsxnsubst (
	qse_mchar_t*           buf,
	qse_size_t             bsz,
	const qse_mchar_t*     fmt,
	qse_size_t             fsz,
	qse_mbssubst_t         subst,
	void*                  ctx
);

/**
 * The qse_wcsxsubst() function expands \a fmt into a buffer \a buf of the size
 * \a bsz by substituting new values for ${} segments within it. The actual
 * substitution is made by invoking the callback function \a subst. 
 * \code
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
 * \endcode
 *
 * If \a buf is #QSE_WCSSUBST_NOBUF, the function returns the length of the 
 * resulting string that a buffer large enough would be able to hold. You can 
 * pass #QSE_WCSSUBST_NOBUF to find out how large a buffer you need to perform 
 * substitution without truncation. The substitution callback function should 
 * handle this special case for buffer size prediction. The sample callback shown
 * above can be extended to return the buffer pointer without actual copying 
 *
 * \code
 * qse_mchar_t* subst (qse_mchar_t* buf, qse_size_t bsz, const qse_wcstr_t* ident, void* ctx)
 * { 
 *   if (qse_wcsxcmp (ident->ptr, ident->len, QSE_WT("USER")) == 0)
 *     return buf + (buf == QSE_WCSSUBST_NOBUF? 3: qse_wcsxput (buf, bsz, QSE_WT("sam")));
 *   else if (qse_wcsxcmp (ident->ptr, ident->len, QSE_WT("GROUP")) == 0)
 *     return buf + (buf == QSE_WCSSUBST_NOBUF? 6: qse_wcsxput (buf, bsz, QSE_WT("coders")));	
 *   return buf; 
 * }
 * 
 * len = qse_wcsxsubst (QSE_WCSSUBST_NOBUF, 0, QSE_WT("user=${USER},group=${GROUP}"), subst, QSE_NULL);
 * buf = malloc (QSE_SIZEOF(qse_wcs_t) * (len + 1));
 * qse_wcsxsubst (buf, len + 1, QSE_WT("user=${USER},group=${GROUP}"), subst, QSE_NULL);
 * \endcode
 *
 * A ${} segment may contain a default value. For example, the segment 
 * ${USER:=Unknown} translates to Unknown if the callback function returns 
 * #QSE_NULL for USER. The default value part may contain ${} segments 
 * recursively.
 *
 * \code
 * qse_mchar_t* subst (qse_mchar_t* buf, qse_size_t bsz, const qse_wcstr_t* ident, void* ctx)
 * { 
 *   if (qse_wcsxcmp (ident->ptr, ident->len, QSE_WT("USER")) == 0)
 *     return QSE_NULL;
 *   else if (qse_wcsxcmp (ident->ptr, ident->len, QSE_WT("GROUP")) == 0)
 *     return buf + (buf == QSE_WCSSUBST_NOBUF? 6: qse_wcsxput (buf, bsz, QSE_WT("coders")));	
 *   return buf; 
 * }
 * \endcode
 */
QSE_EXPORT qse_size_t qse_wcsxsubst (
	qse_wchar_t*           buf,
	qse_size_t             bsz,
	const qse_wchar_t*     fmt,
	qse_wcssubst_t         subst,
	void*                  ctx
);

QSE_EXPORT qse_size_t qse_wcsxnsubst (
	qse_wchar_t*           buf,
	qse_size_t             bsz,
	const qse_wchar_t*     fmt,
	qse_size_t             fsz,
	qse_wcssubst_t         subst,
	void*                  ctx
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strxsubst(buf,bsz,fmt,subst,ctx) qse_mbsxsubst(buf,bsz,fmt,subst,ctx)
#	define qse_strxnsubst(buf,bsz,fmt,fsz,subst,ctx) qse_mbsxnsubst(buf,bsz,fmt,fsz,subst,ctx)
#else
#	define qse_strxsubst(buf,bsz,fmt,subst,ctx) qse_wcsxsubst(buf,bsz,fmt,subst,ctx)
#	define qse_strxnsubst(buf,bsz,fmt,fsz,subst,ctx) qse_wcsxnsubst(buf,bsz,fmt,fsz,subst,ctx)
#endif

QSE_EXPORT qse_size_t qse_mbscat (
	qse_mchar_t*       buf,
	const qse_mchar_t* str
);

QSE_EXPORT qse_size_t qse_mbsncat (
	qse_mchar_t*       buf,
	const qse_mchar_t* str,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_mbscatn (
	qse_mchar_t*       buf,
	const qse_mchar_t* str,
	qse_size_t         n
);

QSE_EXPORT qse_size_t qse_mbsxcat (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* str
);

QSE_EXPORT qse_size_t qse_mbsxncat (
	qse_mchar_t*       buf,
	qse_size_t         bsz,
	const qse_mchar_t* str,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_wcscat (
	qse_wchar_t*       buf,
	const qse_wchar_t* str
);

QSE_EXPORT qse_size_t qse_wcsncat (
	qse_wchar_t*       buf,
	const qse_wchar_t* str,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_wcscatn (
	qse_wchar_t*       buf,
	const qse_wchar_t* str,
	qse_size_t         n
);

QSE_EXPORT qse_size_t qse_wcsxcat (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* str
);

QSE_EXPORT qse_size_t qse_wcsxncat (
	qse_wchar_t*       buf,
	qse_size_t         bsz,
	const qse_wchar_t* str,
	qse_size_t         len
);

#if defined(QSE_CHAR_IS_MCHAR)
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


/* ---------------------------------------------------- */

QSE_EXPORT qse_size_t qse_mbsjoin (
	qse_mchar_t* buf, 
	...
);

QSE_EXPORT qse_size_t qse_mbsxjoin (
	qse_mchar_t* buf,
	qse_size_t   size,
	...
);

/*
 * The qse_wcsjoin() function joins a list of wide-charcter strings into 
 * a buffer. The list of strings is terminated by QSE_NULL.
 *
 * \code
 *  qse_wcsjoin (x, QSE_T("hello"), QSE_T("world"), QSE_NULL);
 * \endcode
 *
 * \return the number of character in the joined string excluding 
 *         the terminating null.
 */
QSE_EXPORT qse_size_t qse_wcsjoin (
	qse_wchar_t* buf, 
	...
);

QSE_EXPORT qse_size_t qse_wcsxjoin (
	qse_wchar_t* buf,
	qse_size_t   size,
	...
);

QSE_EXPORT qse_size_t qse_strjoin (
	qse_char_t* buf, 
	...
);

QSE_EXPORT qse_size_t qse_strxjoin (
	qse_char_t* buf,
	qse_size_t  size,
	...
);

/* ---------------------------------------------------- */


QSE_EXPORT int qse_mbscmp (
	const qse_mchar_t* s1,
	const qse_mchar_t* s2
);

QSE_EXPORT int qse_wcscmp (
	const qse_wchar_t* s1,
	const qse_wchar_t* s2
);

QSE_EXPORT int qse_mbsxcmp (
	const qse_mchar_t* s1,
	qse_size_t         ln1,
	const qse_mchar_t* s2
);

QSE_EXPORT int qse_wcsxcmp (
	const qse_wchar_t* s1,
	qse_size_t         ln1,
	const qse_wchar_t* s2
);

QSE_EXPORT int qse_mbsxncmp (
	const qse_mchar_t* s1,
	qse_size_t         ln1, 
	const qse_mchar_t* s2,
	qse_size_t         ln2
);

QSE_EXPORT int qse_wcsxncmp (
	const qse_wchar_t* s1,
	qse_size_t         ln1, 
	const qse_wchar_t* s2,
	qse_size_t         ln2
);

QSE_EXPORT int qse_mbscasecmp (
	const qse_mchar_t* s1,
	const qse_mchar_t* s2
);

QSE_EXPORT int qse_wcscasecmp (
	const qse_wchar_t* s1,
	const qse_wchar_t* s2
);

QSE_EXPORT int qse_mbsxcasecmp (
	const qse_mchar_t* s1,
	qse_size_t         ln,
	const qse_mchar_t* s2
);

QSE_EXPORT int qse_wcsxcasecmp (
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
 * \code
 * qse_mbsxncasecmp (QSE_MT("foo"), 3, QSE_MT("FoO"), 3);
 * \endcode
 *
 * \return 0 if two strings are equal, 
 *         a positive number if the first string is larger, 
 *         -1 if the second string is larger.
 *
 */
QSE_EXPORT int qse_mbsxncasecmp (
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
 * \code
 * qse_wcsxncasecmp (QSE_WT("foo"), 3, QSE_WT("FoO"), 3);
 * \endcode
 *
 * \return 0 if two strings are equal, 
 *         a positive number if the first string is larger, 
 *         -1 if the second string is larger.
 *
 */
QSE_EXPORT int qse_wcsxncasecmp (
	const qse_wchar_t* s1,  /**< pointer to the first string */
	qse_size_t         ln1, /**< length of the first string */ 
	const qse_wchar_t* s2,  /**< pointer to the second string */
	qse_size_t         ln2  /**< length of the second string */
);

QSE_EXPORT int qse_mbszcmp (
	const qse_mchar_t* s1,
	const qse_mchar_t* s2,
	qse_size_t         n
);

QSE_EXPORT int qse_mbszcasecmp (
	const qse_mchar_t* s1,
	const qse_mchar_t* s2,
	qse_size_t         n
);

QSE_EXPORT int qse_wcszcmp (
	const qse_wchar_t* s1,
	const qse_wchar_t* s2,
	qse_size_t         n
);

QSE_EXPORT int qse_wcszcasecmp (
	const qse_wchar_t* s1,
	const qse_wchar_t* s2,
	qse_size_t         n
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strcmp(s1,s2)               qse_mbscmp(s1,s2)
#	define qse_strxcmp(s1,ln1,s2)          qse_mbsxcmp(s1,ln1,s2)
#	define qse_strxncmp(s1,ln1,s2,ln2)     qse_mbsxncmp(s1,ln1,s2,ln2)
#	define qse_strcasecmp(s1,s2)           qse_mbscasecmp(s1,s2)
#	define qse_strxcasecmp(s1,ln1,s2)      qse_mbsxcasecmp(s1,ln1,s2)
#	define qse_strxncasecmp(s1,ln1,s2,ln2) qse_mbsxncasecmp(s1,ln1,s2,ln2)
#	define qse_strzcmp(s1,s2,n)            qse_mbszcmp(s1,s2,n)
#	define qse_strzcasecmp(s1,s2,n)        qse_mbszcasecmp(s1,s2,n)
#else
#	define qse_strcmp(s1,s2)               qse_wcscmp(s1,s2)
#	define qse_strxcmp(s1,ln1,s2)          qse_wcsxcmp(s1,ln1,s2)
#	define qse_strxncmp(s1,ln1,s2,ln2)     qse_wcsxncmp(s1,ln1,s2,ln2)
#	define qse_strcasecmp(s1,s2)           qse_wcscasecmp(s1,s2)
#	define qse_strxcasecmp(s1,ln1,s2)      qse_wcsxcasecmp(s1,ln1,s2)
#	define qse_strxncasecmp(s1,ln1,s2,ln2) qse_wcsxncasecmp(s1,ln1,s2,ln2)
#	define qse_strzcmp(s1,s2,n)            qse_wcszcmp(s1,s2,n)
#	define qse_strzcasecmp(s1,s2,n)        qse_wcszcasecmp(s1,s2,n)
#endif

QSE_EXPORT qse_mchar_t* qse_mbsdup (
	const qse_mchar_t* str,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_mchar_t* qse_mbsdup2 (
	const qse_mchar_t* str1,
	const qse_mchar_t* str2,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_mchar_t* qse_mbsxdup (
	const qse_mchar_t* str,
	qse_size_t         len, 
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_mchar_t* qse_mbsxdup2 (
	const qse_mchar_t* str1,
	qse_size_t         len1,
	const qse_mchar_t* str2,
	qse_size_t         len2,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_mchar_t* qse_mbsadup (
	const qse_mchar_t* str[],
	qse_size_t*        len,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_wcsdup (
	const qse_wchar_t* str,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_wcsdup2 (
	const qse_wchar_t* str1,
	const qse_wchar_t* str2,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_wcsxdup (
	const qse_wchar_t* str,
	qse_size_t         len, 
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_wcsxdup2 (
	const qse_wchar_t* str1,
	qse_size_t         len1,
	const qse_wchar_t* str2,
	qse_size_t         len2,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_wcsadup (
	const qse_wchar_t* str[],
	qse_size_t*        len,
	qse_mmgr_t*        mmgr
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strdup(s,mmgr)             qse_mbsdup(s,mmgr)
#	define qse_strdup2(s1,s2,mmgr)        qse_mbsdup2(s1,s2,mmgr)
#	define qse_strxdup(s,l,mmgr)          qse_mbsxdup(s,l,mmgr)
#	define qse_strxdup2(s1,l1,s2,l2,mmgr) qse_mbsxdup(s1,l1,s2,l2,mmgr)
#	define qse_stradup(sa,len,mmgr)       qse_mbsadup(sa,len,mmgr)
#else
#	define qse_strdup(s,mmgr)             qse_wcsdup(s,mmgr)
#	define qse_strdup2(s1,s2,mmgr)        qse_wcsdup2(s1,s2,mmgr)
#	define qse_strxdup(s,l,mmgr)          qse_wcsxdup(s,l,mmgr)
#	define qse_strxdup2(s1,l1,s2,l2,mmgr) qse_wcsxdup(s1,l1,s2,l2,mmgr)
#	define qse_stradup(sa,len,mmgr)       qse_wcsadup(sa,len,mmgr)
#endif

QSE_EXPORT qse_mchar_t* qse_mcstrdup (
	const qse_mcstr_t* str,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_mchar_t* qse_mcstradup (
	const qse_mcstr_t str[],
	qse_size_t*       len,
	qse_mmgr_t*       mmgr
);

QSE_EXPORT qse_wchar_t* qse_wcstrdup (
	const qse_wcstr_t* str,
	qse_mmgr_t*        mmgr
);

QSE_EXPORT qse_wchar_t* qse_wcstradup (
	const qse_wcstr_t str[],
	qse_size_t*       len,
	qse_mmgr_t*       mmgr
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_cstrdup(sa,mmgr)           qse_mcstrdup(sa,mmgr)
#	define qse_cstradup(sa,len,mmgr)      qse_mcstradup(sa,len,mmgr)
#else
#	define qse_cstrdup(sa,mmgr)           qse_wcstrdup(sa,mmgr)
#	define qse_cstradup(sa,len,mmgr)      qse_wcstradup(sa,len,mmgr)
#endif

/**
 * The qse_mbsstr() function searchs a string \a str for the first occurrence 
 * of a substring \a sub.
 * \return pointer to the first occurrence in \a str if \a sub is found, 
 *         #QSE_NULL if not.
 */
QSE_EXPORT qse_mchar_t* qse_mbsstr (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

/**
 * The qse_wcsstr() function searchs a string \a str for the first occurrence 
 * of a substring \a sub.
 * \return pointer to the first occurrence in \a str if \a sub is found, 
 *         #QSE_NULL if not.
 */
QSE_EXPORT qse_wchar_t* qse_wcsstr (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxstr (
	const qse_mchar_t* str,
	qse_size_t         size,
	const qse_mchar_t* sub
);

QSE_EXPORT qse_wchar_t* qse_wcsxstr (
	const qse_wchar_t* str,
	qse_size_t         size,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxnstr (
	const qse_mchar_t* str,
	qse_size_t         strsz, 
	const qse_mchar_t* sub,
	qse_size_t         subsz
);

QSE_EXPORT qse_wchar_t* qse_wcsxnstr (
	const qse_wchar_t* str,
	qse_size_t         strsz, 
	const qse_wchar_t* sub,
	qse_size_t         subsz
);

QSE_EXPORT qse_mchar_t* qse_mbscasestr (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

QSE_EXPORT qse_wchar_t* qse_wcscasestr (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxcasestr (
	const qse_mchar_t* str,
	qse_size_t         size,
	const qse_mchar_t* sub
);

QSE_EXPORT qse_wchar_t* qse_wcsxcasestr (
	const qse_wchar_t* str,
	qse_size_t         size,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxncasestr (
	const qse_mchar_t* str,
	qse_size_t         strsz, 
	const qse_mchar_t* sub,
	qse_size_t         subsz
);

QSE_EXPORT qse_wchar_t* qse_wcsxncasestr (
	const qse_wchar_t* str,
	qse_size_t         strsz, 
	const qse_wchar_t* sub,
	qse_size_t         subsz
);

/**
 * The qse_mbsrstr() function searchs a string \a str for the last occurrence 
 * of a substring \a sub.
 * \return pointer to the last occurrence in \a str if \a sub is found, 
 *         #QSE_NULL if not.
 */
QSE_EXPORT qse_mchar_t* qse_mbsrstr (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

/**
 * The qse_wcsrstr() function searchs a string \a str for the last occurrence 
 * of a substring \a sub.
 * \return pointer to the last occurrence in \a str if \a sub is found, 
 *         #QSE_NULL if not.
 */
QSE_EXPORT qse_wchar_t* qse_wcsrstr (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxrstr (
	const qse_mchar_t* str,
	qse_size_t         size,
	const qse_mchar_t* sub
);

QSE_EXPORT qse_wchar_t* qse_wcsxrstr (
	const qse_wchar_t* str,
	qse_size_t         size,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxnrstr (
	const qse_mchar_t* str,
	qse_size_t         strsz, 
	const qse_mchar_t* sub,
	qse_size_t         subsz
);

QSE_EXPORT qse_wchar_t* qse_wcsxnrstr (
	const qse_wchar_t* str,
	qse_size_t         strsz, 
	const qse_wchar_t* sub,
	qse_size_t         subsz
);

QSE_EXPORT qse_mchar_t* qse_mbsrcasestr (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

QSE_EXPORT qse_wchar_t* qse_wcsrcasestr (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxrcasestr (
	const qse_mchar_t* str,
	qse_size_t         size,
	const qse_mchar_t* sub
);

QSE_EXPORT qse_wchar_t* qse_wcsxrcasestr (
	const qse_wchar_t* str,
	qse_size_t         size,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxnrcasestr (
	const qse_mchar_t* str,
	qse_size_t         strsz, 
	const qse_mchar_t* sub,
	qse_size_t         subsz
);

QSE_EXPORT qse_wchar_t* qse_wcsxnrcasestr (
	const qse_wchar_t* str,
	qse_size_t         strsz, 
	const qse_wchar_t* sub,
	qse_size_t         subsz
);

#if defined(QSE_CHAR_IS_MCHAR)
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
	const qse_mchar_t* word,
	qse_mchar_t        extra_delim
);

const qse_wchar_t* qse_wcsword (
	const qse_wchar_t* str,
	const qse_wchar_t* word,
	qse_wchar_t        extra_delim
);

/**
 * The qse_mbsxword() function finds a whole word in a string.
 * The word can be delimited by white spaces or an extra delimiter
 * \a extra_delim. Pass QSE_MT('\0') if no extra delimiter is
 * needed.
 */
const qse_mchar_t* qse_mbsxword (
	const qse_mchar_t* str,
	qse_size_t         len,
	const qse_mchar_t* word,
	qse_mchar_t        extra_delim
);

/**
 * The qse_wcsxword() function finds a whole word in a string.
 * The word can be delimited by white spaces or an extra delimiter
 * \a extra_delim. Pass QSE_WT('\0') if no extra delimiter is
 * needed.
 */
const qse_wchar_t* qse_wcsxword (
	const qse_wchar_t* str,
	qse_size_t         len,
	const qse_wchar_t* word,
	qse_wchar_t        extra_delim
);

const qse_mchar_t* qse_mbscaseword (
	const qse_mchar_t* str,
	const qse_mchar_t* word,
	qse_mchar_t        extra_delim
);

const qse_wchar_t* qse_wcscaseword (
	const qse_wchar_t* str,
	const qse_wchar_t* word,
	qse_wchar_t        extra_delim
);

/**
 * The qse_mbsxcaseword() function finds a whole word in a string 
 * case-insensitively.
 */
const qse_mchar_t* qse_mbsxcaseword (
	const qse_mchar_t* str,
	qse_size_t         len,
	const qse_mchar_t* word,
	qse_mchar_t        extra_delim
);

/**
 * The qse_wcsxcaseword() function finds a whole word in a string 
 * case-insensitively.
 */
const qse_wchar_t* qse_wcsxcaseword (
	const qse_wchar_t* str,
	qse_size_t         len,
	const qse_wchar_t* word,
	qse_wchar_t        extra_delim
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strword(str,word,edelim)          qse_mbsword(str,word,edelim)
#	define qse_strxword(str,len,word,edelim)     qse_mbsxword(str,len,word,edelim)
#	define qse_strcaseword(str,word,edelim)      qse_mbscaseword(str,word,edelim)
#	define qse_strxcaseword(str,len,word,edelim) qse_mbsxcaseword(str,len,word,edelim)
#else
#	define qse_strword(str,word,edelim)          qse_wcsword(str,word,edelim)
#	define qse_strxword(str,len,word,edelim)     qse_wcsxword(str,len,word,edelim)
#	define qse_strcaseword(str,word,edelim)      qse_wcscaseword(str,word,edelim)
#	define qse_strxcaseword(str,len,word,edelim) qse_wcsxcaseword(str,len,word,edelim)
#endif

/**
 * The qse_mbschr() function finds a chracter in a string. 
 */
QSE_EXPORT qse_mchar_t* qse_mbschr (
	const qse_mchar_t* str,
	qse_mcint_t        c
);

/**
 * The qse_wcschr() function finds a chracter in a string. 
 */
QSE_EXPORT qse_wchar_t* qse_wcschr (
	const qse_wchar_t* str,
	qse_wcint_t        c
);

QSE_EXPORT qse_mchar_t* qse_mbsxchr (
	const qse_mchar_t* str,
	qse_size_t         len,
	qse_mcint_t        c
);

QSE_EXPORT qse_wchar_t* qse_wcsxchr (
	const qse_wchar_t* str,
	qse_size_t         len,
	qse_wcint_t        c
);

QSE_EXPORT qse_wchar_t* qse_wcsrchr (
	const qse_wchar_t* str,
	qse_wcint_t        c
);

QSE_EXPORT qse_mchar_t* qse_mbsrchr (
	const qse_mchar_t* str,
	qse_mcint_t        c
);

QSE_EXPORT qse_mchar_t* qse_mbsxrchr (
	const qse_mchar_t* str,
	qse_size_t         len,
	qse_mcint_t        c
);

QSE_EXPORT qse_wchar_t* qse_wcsxrchr (
	const qse_wchar_t* str,
	qse_size_t         len,
	qse_wcint_t        c
);

#if defined(QSE_CHAR_IS_MCHAR)
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
 * \return pointer to the beginning of a matching beginning, 
 *         #SE_NULL if no match is found.
 */
QSE_EXPORT qse_mchar_t* qse_mbsbeg (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

/**
 * The qse_wcsbeg() function checks if a string begins with a substring.
 * \return pointer to the beginning of a matching beginning, 
 *         #QSE_NULL if no match is found.
 */
QSE_EXPORT qse_wchar_t* qse_wcsbeg (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxbeg (
	const qse_mchar_t* str,
	qse_size_t         len,
	const qse_mchar_t* sub
);

QSE_EXPORT qse_wchar_t* qse_wcsxbeg (
	const qse_wchar_t* str,
	qse_size_t         len,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsnbeg (
	const qse_mchar_t* str,
	const qse_mchar_t* sub,
	qse_size_t         len
);

QSE_EXPORT qse_wchar_t* qse_wcsnbeg (
	const qse_wchar_t* str,
	const qse_wchar_t* sub,
	qse_size_t         len
);

QSE_EXPORT qse_mchar_t* qse_mbsxnbeg (
	const qse_mchar_t* str,
	qse_size_t         len1, 
	const qse_mchar_t* sub,
	qse_size_t         len2
);

QSE_EXPORT qse_wchar_t* qse_wcsxnbeg (
	const qse_wchar_t* str,
	qse_size_t         len1, 
	const qse_wchar_t* sub,
	qse_size_t         len2
);

QSE_EXPORT qse_mchar_t* qse_mbscasebeg (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

QSE_EXPORT qse_wchar_t* qse_wcscasebeg (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strbeg(str,sub)             qse_mbsbeg(str,sub)
#	define qse_strxbeg(str,len,sub)        qse_mbsxbeg(str,len,sub)
#	define qse_strnbeg(str,sub,len)        qse_mbsnbeg(str,sub,len)
#	define qse_strxnbeg(str,len1,sub,len2) qse_mbsxnbeg(str,len1,sub,len2)
#	define qse_strcasebeg(str,sub)         qse_mbscasebeg(str,sub)
#else
#	define qse_strbeg(str,sub)             qse_wcsbeg(str,sub)
#	define qse_strxbeg(str,len,sub)        qse_wcsxbeg(str,len,sub)
#	define qse_strnbeg(str,sub,len)        qse_wcsnbeg(str,sub,len)
#	define qse_strxnbeg(str,len1,sub,len2) qse_wcsxnbeg(str,len1,sub,len2)
#	define qse_strcasebeg(str,sub)         qse_wcscasebeg(str,sub)
#endif

/**
 * The qse_mbsend() function checks if a string ends with a substring.
 * \return pointer to the beginning of a matching ending, 
 *         #SE_NULL if no match is found.
 */
QSE_EXPORT qse_mchar_t* qse_mbsend (
	const qse_mchar_t* str,
	const qse_mchar_t* sub
);

/**
 * The qse_wcsend() function checks if a string ends with a substring.
 * \return pointer to the beginning of a matching ending, 
 *         #QSE_NULL if no match is found.
 */
QSE_EXPORT qse_wchar_t* qse_wcsend (
	const qse_wchar_t* str,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsxend (
	const qse_mchar_t* str,
	qse_size_t         len,
	const qse_mchar_t* sub
);

QSE_EXPORT qse_wchar_t* qse_wcsxend (
	const qse_wchar_t* str,
	qse_size_t         len,
	const qse_wchar_t* sub
);

QSE_EXPORT qse_mchar_t* qse_mbsnend (
	const qse_mchar_t* str,
	const qse_mchar_t* sub,
	qse_size_t         len
);

QSE_EXPORT qse_wchar_t* qse_wcsnend (
	const qse_wchar_t* str,
	const qse_wchar_t* sub,
	qse_size_t         len
);

QSE_EXPORT qse_mchar_t* qse_mbsxnend (
	const qse_mchar_t* str,
	qse_size_t         len1, 
	const qse_mchar_t* sub,
	qse_size_t         len2
);

QSE_EXPORT qse_wchar_t* qse_wcsxnend (
	const qse_wchar_t* str,
	qse_size_t         len1, 
	const qse_wchar_t* sub,
	qse_size_t         len2
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strend(str,sub)             qse_mbsend(str,sub)
#	define qse_strxend(str,len,sub)        qse_mbsxend(str,len,sub)
#	define qse_strnend(str,sub,len)        qse_mbsnend(str,sub,len)
#	define qse_strxnend(str,len1,sub,len2) qse_mbsxnend(str,len1,sub,len2)
#else
#	define qse_strend(str,sub)             qse_wcsend(str,sub)
#	define qse_strxend(str,len,sub)        qse_wcsxend(str,len,sub)
#	define qse_strnend(str,sub,len)        qse_wcsnend(str,sub,len)
#	define qse_strxnend(str,len1,sub,len2) qse_wcsxnend(str,len1,sub,len2)
#endif

QSE_EXPORT qse_size_t qse_mbsspn (
	const qse_mchar_t* str1,
	const qse_mchar_t* str2
);

QSE_EXPORT qse_size_t qse_wcsspn (
	const qse_wchar_t* str1,
	const qse_wchar_t* str2
);

QSE_EXPORT qse_size_t qse_mbscspn (
	const qse_mchar_t* str1,
	const qse_mchar_t* str2
);

QSE_EXPORT qse_size_t qse_wcscspn (
	const qse_wchar_t* str1,
	const qse_wchar_t* str2
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strspn(str1,str2) qse_mbsspn(str1,str2)
#	define qse_strcspn(str1,str2) qse_mbscspn(str1,str2)
#else
#	define qse_strspn(str1,str2) qse_wcsspn(str1,str2)
#	define qse_strcspn(str1,str2) qse_wcscspn(str1,str2)
#endif

/*
 * The qse_mbspbrk() function searches \a str1 for the first occurrence of 
 * a character in \a str2.
 * \return pointer to the first occurrence in \a str1 if one is found.
 *         QSE_NULL if none is found.
 */
QSE_EXPORT qse_mchar_t* qse_mbspbrk (
	const qse_mchar_t* str1,
	const qse_mchar_t* str2
);

QSE_EXPORT qse_mchar_t* qse_mbsxpbrk (
	const qse_mchar_t* str1,
	qse_size_t         len,
	const qse_mchar_t* str2
);

QSE_EXPORT qse_mchar_t* qse_mbsrpbrk (
	const qse_mchar_t* str1,
	const qse_mchar_t* str2
);

QSE_EXPORT qse_mchar_t* qse_mbsxrpbrk (
	const qse_mchar_t* str1,
	qse_size_t         len,
	const qse_mchar_t* str2
);

/*
 * The qse_wcspbrk() function searches \a str1 for the first occurrence of 
 * a character in \a str2.
 * \return pointer to the first occurrence in \a str1 if one is found.
 *         QSE_NULL if none is found.
 */
QSE_EXPORT qse_wchar_t* qse_wcspbrk (
	const qse_wchar_t* str1,
	const qse_wchar_t* str2
);

QSE_EXPORT qse_wchar_t* qse_wcsxpbrk (
	const qse_wchar_t* str1,
	qse_size_t         len,
	const qse_wchar_t* str2
);

QSE_EXPORT qse_wchar_t* qse_wcsrpbrk (
	const qse_wchar_t* str1,
	const qse_wchar_t* str2
);

QSE_EXPORT qse_wchar_t* qse_wcsxrpbrk (
	const qse_wchar_t* str1,
	qse_size_t         len,
	const qse_wchar_t* str2
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strpbrk(str1,str2)       qse_mbspbrk(str1,str2)
#	define qse_strxpbrk(str1,len,str2)  qse_mbsxpbrk(str1,len,str2)
#	define qse_strrpbrk(str1,str2)      qse_mbsrpbrk(str1,str2)
#	define qse_strxrpbrk(str1,len,str2) qse_mbsxrpbrk(str1,len,str2)
#else
#	define qse_strpbrk(str1,str2)       qse_wcspbrk(str1,str2)
#	define qse_strxpbrk(str1,len,str2)  qse_wcsxpbrk(str1,len,str2)
#	define qse_strrpbrk(str1,str2)      qse_wcsrpbrk(str1,str2)
#	define qse_strxrpbrk(str1,len,str2) qse_wcsxrpbrk(str1,len,str2)
#endif

/* 
 * string conversion
 */
QSE_EXPORT int qse_mbstoi (
	const qse_mchar_t* str,
	int               base
);

QSE_EXPORT long qse_mbstol (
	const qse_mchar_t* str,
	int               base
);

QSE_EXPORT unsigned int qse_mbstoui (
	const qse_mchar_t* str,
	int               base
);

QSE_EXPORT unsigned long qse_mbstoul (
	const qse_mchar_t* str,
	int               base
);

QSE_EXPORT int qse_mbsxtoi (
	const qse_mchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT long qse_mbsxtol (
	const qse_mchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT unsigned int qse_mbsxtoui (
	const qse_mchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT unsigned long qse_mbsxtoul (
	const qse_mchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT qse_int_t qse_mbstoint (
	const qse_mchar_t* str,
	int               base
);

QSE_EXPORT qse_long_t qse_mbstolong (
	const qse_mchar_t* str,
	int               base
);

QSE_EXPORT qse_uint_t qse_mbstouint (
	const qse_mchar_t* str,
	int               base
);

QSE_EXPORT qse_ulong_t qse_mbstoulong (
	const qse_mchar_t* str,
	int               base
);

QSE_EXPORT qse_int_t qse_mbsxtoint (
	const qse_mchar_t* str, 
	qse_size_t        len,
	int               base
);

QSE_EXPORT qse_long_t qse_mbsxtolong (
	const qse_mchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT qse_uint_t qse_mbsxtouint (
	const qse_mchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT qse_ulong_t qse_mbsxtoulong (
	const qse_mchar_t* str,
	qse_size_t        len,
	int               base
);



QSE_EXPORT int qse_wcstoi (
	const qse_wchar_t* str,
	int               base
);

QSE_EXPORT long qse_wcstol (
	const qse_wchar_t* str,
	int               base
);

QSE_EXPORT unsigned int qse_wcstoui (
	const qse_wchar_t* str,
	int               base
);

QSE_EXPORT unsigned long qse_wcstoul (
	const qse_wchar_t* str,
	int               base
);

QSE_EXPORT int qse_wcsxtoi (
	const qse_wchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT long qse_wcsxtol (
	const qse_wchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT unsigned int qse_wcsxtoui (
	const qse_wchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT unsigned long qse_wcsxtoul (
	const qse_wchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT qse_int_t qse_wcstoint (
	const qse_wchar_t* str,
	int               base
);

QSE_EXPORT qse_long_t qse_wcstolong (
	const qse_wchar_t* str,
	int               base
);

QSE_EXPORT qse_uint_t qse_wcstouint (
	const qse_wchar_t* str,
	int               base
);

QSE_EXPORT qse_ulong_t qse_wcstoulong (
	const qse_wchar_t* str,
	int               base
);

QSE_EXPORT qse_int_t qse_wcsxtoint (
	const qse_wchar_t* str, 
	qse_size_t        len,
	int               base
);

QSE_EXPORT qse_long_t qse_wcsxtolong (
	const qse_wchar_t* str,
	qse_size_t        len,
	int               base
);

QSE_EXPORT qse_uint_t qse_wcsxtouint (
	const qse_wchar_t* str,
	qse_size_t         len,
	int                base
);

QSE_EXPORT qse_ulong_t qse_wcsxtoulong (
	const qse_wchar_t* str,
	qse_size_t         len,
	int                base
);

#if defined(QSE_CHAR_IS_MCHAR)

#	define qse_strtoi(str,base) qse_mbstoi(str,base)
#	define qse_strtol(str,base) qse_mbstol(str,base)
#	define qse_strtoui(str,base) qse_mbstoui(str,base)
#	define qse_strtoul(str,base) qse_mbstoul(str,base)

#	define qse_strxtoi(str,len,base) qse_mbsxtoi(str,len,base)
#	define qse_strxtol(str,len,base) qse_mbsxtol(str,len,base)
#	define qse_strxtoui(str,len,base) qse_mbsxtoui(str,len,base)
#	define qse_strxtoul(str,len,base) qse_mbsxtoul(str,len,base)

#	define qse_strtoint(str,base) qse_mbstoint(str,base)
#	define qse_strtolong(str,base) qse_mbstolong(str,base)
#	define qse_strtouint(str,base) qse_mbstouint(str,base)
#	define qse_strtoulong(str,base) qse_mbstoulong(str,base)

#	define qse_strxtoint(str,len,base) qse_mbsxtoint(str,len,base)
#	define qse_strxtolong(str,len,base) qse_mbsxtolong(str,len,base)
#	define qse_strxtouint(str,len,base) qse_mbsxtouint(str,len,base)
#	define qse_strxtoulong(str,len,base) qse_mbsxtoulong(str,len,base)

#else

#	define qse_strtoi(str,base) qse_wcstoi(str,base)
#	define qse_strtol(str,base) qse_wcstol(str,base)
#	define qse_strtoui(str,base) qse_wcstoui(str,base)
#	define qse_strtoul(str,base) qse_wcstoul(str,base)

#	define qse_strxtoi(str,len,base) qse_wcsxtoi(str,len,base)
#	define qse_strxtol(str,len,base) qse_wcsxtol(str,len,base)
#	define qse_strxtoui(str,len,base) qse_wcsxtoui(str,len,base)
#	define qse_strxtoul(str,len,base) qse_wcsxtoul(str,len,base)

#	define qse_strtoint(str,base) qse_wcstoint(str,base)
#	define qse_strtolong(str,base) qse_wcstolong(str,base)
#	define qse_strtouint(str,base) qse_wcstouint(str,base)
#	define qse_strtoulong(str,base) qse_wcstoulong(str,base)

#	define qse_strxtoint(str,len,base) qse_wcsxtoint(str,len,base)
#	define qse_strxtolong(str,len,base) qse_wcsxtolong(str,len,base)
#	define qse_strxtouint(str,len,base) qse_wcsxtouint(str,len,base)
#	define qse_strxtoulong(str,len,base) qse_wcsxtoulong(str,len,base)

#endif


QSE_EXPORT int qse_mbshextobin (
	const qse_mchar_t* hex,
	qse_size_t         hexlen,
	qse_uint8_t*       buf,
	qse_size_t         buflen
);

QSE_EXPORT int qse_wcshextobin (
	const qse_wchar_t* hex,
	qse_size_t         hexlen,
	qse_uint8_t*       buf,
	qse_size_t         buflen
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strhextobin(hex,hexlen,buf,buflen) qse_mbshextobin(hex,hexlen,buf,buflen)
#else
#	define qse_strhextobin(hex,hexlen,buf,buflen) qse_wcshextobin(hex,hexlen,buf,buflen)
#endif

QSE_EXPORT qse_size_t qse_mbsdel (
	qse_mchar_t* str, 
	qse_size_t   pos,
	qse_size_t   n
);

QSE_EXPORT qse_size_t qse_wcsdel (
	qse_wchar_t* str, 
	qse_size_t   pos,
	qse_size_t   n
);

QSE_EXPORT qse_size_t qse_mbsxdel (
	qse_mchar_t* str, 
	qse_size_t   len,
	qse_size_t   pos,
	qse_size_t   n
);

QSE_EXPORT qse_size_t qse_wcsxdel (
	qse_wchar_t* str, 
	qse_size_t   len,
	qse_size_t   pos,
	qse_size_t   n
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strdel(str,pos,n)      qse_mbsdel(str,pos,n)
#	define qse_strxdel(str,len,pos,n) qse_mbsxdel(str,len,pos,n)
#else
#	define qse_strdel(str,pos,n)      qse_wcsdel(str,pos,n)
#	define qse_strxdel(str,len,pos,n) qse_wcsxdel(str,len,pos,n)
#endif


QSE_EXPORT qse_size_t qse_mbsexcl (
	qse_mchar_t*       str,
	const qse_mchar_t* cs
);

QSE_EXPORT qse_size_t qse_mbsxexcl (
	qse_mchar_t*       str,
	qse_size_t         len,
	const qse_mchar_t* cs
);

QSE_EXPORT qse_size_t qse_wcsexcl (
	qse_wchar_t*       str,
	const qse_wchar_t* cs
);

QSE_EXPORT qse_size_t qse_wcsxexcl (
	qse_wchar_t*       str,
	qse_size_t         len,
	const qse_wchar_t* cs
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strexcl(str,cs)      qse_mbsexcl(str,cs)
#	define qse_strxexcl(str,len,cs) qse_mbsxexcl(str,len,cs)
#else
#	define qse_strexcl(str,cs)      qse_wcsexcl(str,cs)
#	define qse_strxexcl(str,len,cs) qse_wcsxexcl(str,len,cs)
#endif

QSE_EXPORT qse_size_t qse_mbsincl (
	qse_mchar_t*       str,
	const qse_mchar_t* cs
);

QSE_EXPORT qse_size_t qse_mbsxincl (
	qse_mchar_t*       str,
	qse_size_t         len,
	const qse_mchar_t* cs
);

QSE_EXPORT qse_size_t qse_wcsincl (
	qse_wchar_t*       str,
	const qse_wchar_t* cs
);

QSE_EXPORT qse_size_t qse_wcsxincl (
	qse_wchar_t*       str,
	qse_size_t         len,
	const qse_wchar_t* cs
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strincl(str,cs)      qse_mbsincl(str,cs)
#	define qse_strxincl(str,len,cs) qse_mbsxincl(str,len,cs)
#else
#	define qse_strincl(str,cs)      qse_wcsincl(str,cs)
#	define qse_strxincl(str,len,cs) qse_wcsxincl(str,len,cs)
#endif

QSE_EXPORT qse_size_t qse_mbsset (
	qse_mchar_t* buf,
	qse_mchar_t  c,
	qse_size_t   n
);

QSE_EXPORT qse_size_t qse_wcsset (
	qse_wchar_t* buf,
	qse_wchar_t  c,
	qse_size_t   n
);

QSE_EXPORT qse_size_t qse_mbsxset (
	qse_mchar_t* buf,
	qse_size_t   bsz,
	qse_mchar_t  c,
	qse_size_t   n
);


QSE_EXPORT qse_size_t qse_wcsxset (
	qse_wchar_t* buf,
	qse_size_t   bsz,
	qse_wchar_t  c,
	qse_size_t   n
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strset(buf,c,n) qse_mbsset(buf,c,n)
#	define qse_strxset(buf,bsz,c,n) qse_mbsxset(buf,bsz,c,n)
#else
#	define qse_strset(buf,c,n) qse_wcsset(buf,c,n)
#	define qse_strxset(buf,bsz,c,n) qse_wcsxset(buf,bsz,c,n)
#endif

/* case conversion */

QSE_EXPORT qse_size_t qse_mbslwr (
	qse_mchar_t* str
);

QSE_EXPORT qse_size_t qse_wcslwr (
	qse_wchar_t* str
);

QSE_EXPORT qse_size_t qse_mbsupr (
	qse_mchar_t* str
);

QSE_EXPORT qse_size_t qse_wcsupr (
	qse_wchar_t* str
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strlwr(str) qse_mbslwr(str);
#	define qse_strupr(str) qse_mbsupr(str);
#else
#	define qse_strlwr(str) qse_wcslwr(str);
#	define qse_strupr(str) qse_wcsupr(str);
#endif


QSE_EXPORT qse_size_t qse_mbsrev (
	qse_mchar_t* str
);

QSE_EXPORT qse_size_t qse_wcsrev (
	qse_wchar_t* str
);

QSE_EXPORT qse_size_t qse_mbsxrev (	
	qse_mchar_t* str,
	qse_size_t   len
);

QSE_EXPORT qse_size_t qse_wcsxrev (	
	qse_wchar_t* str,
	qse_size_t   len
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strrev(str)      qse_mbsrev(str)
#	define qse_strxrev(str,len) qse_mbsxrev(str,len)
#else
#	define qse_strrev(str)      qse_wcsrev(str)
#	define qse_strxrev(str,len) qse_wcsxrev(str,len)
#endif

QSE_EXPORT qse_size_t qse_mbsrot (
	qse_mchar_t* str,
	int          dir,
	qse_size_t   n
);

QSE_EXPORT qse_size_t qse_wcsrot (
	qse_wchar_t* str,
	int          dir,
	qse_size_t   n
);

QSE_EXPORT qse_size_t qse_mbsxrot (
	qse_mchar_t* str,
	qse_size_t   len,
	int          dir,
	qse_size_t   n
);

QSE_EXPORT qse_size_t qse_wcsxrot (
	qse_wchar_t* str,
	qse_size_t   len,
	int          dir,
	qse_size_t   n
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strrot(str,dir,n)      qse_mbsrot(str,dir,n)
#	define qse_strxrot(str,len,dir,n) qse_mbsrot(str,len,dir,n)
#else
#	define qse_strrot(str,dir,n)      qse_wcsrot(str,dir,n)
#	define qse_strxrot(str,len,dir,n) qse_wcsrot(str,len,dir,n)
#endif

/**
 * The qse_mbsspl() function splits a string into fields.
 */
QSE_EXPORT int qse_mbsspl (
	qse_mchar_t*       str,
	const qse_mchar_t* delim,
	qse_mchar_t        lquote,
	qse_mchar_t        rquote,
	qse_mchar_t        escape
);

/**
 * The qse_wcsspl() function splits a string into fields.
 */
QSE_EXPORT int qse_wcsspl (
	qse_wchar_t*       str,
	const qse_wchar_t* delim,
	qse_wchar_t        lquote,
	qse_wchar_t        rquote,
	qse_wchar_t        escape
);

/**
 * The qse_mbsspltrn() function splits a string translating special 
 * escape sequences.
 * The argument \a trset is a translation character set which is composed
 * of multiple character pairs. An escape character followed by the 
 * first character in a pair is translated into the second character
 * in the pair. If trset is #QSE_NULL, no translation is performed. 
 *
 * Let's translate a sequence of '\n' and '\r' to a new line and a carriage
 * return respectively.
 * \code
 *   nfields = qse_mbsspltrn (
 *       str, QSE_MT(':'), QSE_MT('['), QSE_MT(']'), 
 *       QSE_MT('\\'), QSE_MT("n\nr\r")
 *   );
 * \endcode
 * Given [xxx]:[\rabc\ndef]:[] as an input, the example breaks the second 
 * fields to <CR>abc<NL>def where <CR> is a carriage return and <NL> is a 
 * new line.
 *
 * If you don't need translation, you may call qse_mbsspl() instead.
 * \return number of resulting fields on success, -1 on failure
 */
QSE_EXPORT int qse_mbsspltrn (
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
 * The argument \a trset is a translation character set which is composed
 * of multiple character pairs. An escape character followed by the 
 * first character in a pair is translated into the second character
 * in the pair. If trset is #QSE_NULL, no translation is performed. 
 *
 * Let's translate a sequence of '\n' and '\r' to a new line and a carriage
 * return respectively.
 * \code
 *   nfields = qse_wcsspltrn (
 *       str, QSE_WT(':'), QSE_WT('['), QSE_WT(']'), 
 *       QSE_WT('\\'), QSE_WT("n\nr\r")
 *   );
 * \endcode
 * Given [xxx]:[\rabc\ndef]:[] as an input, the example breaks the second 
 * fields to <CR>abc<NL>def where <CR> is a carriage return and <NL> is a 
 * new line.
 *
 * If you don't need translation, you may call qse_wcsspl() instead.
 * \return number of resulting fields on success, -1 on failure
 */
QSE_EXPORT int qse_wcsspltrn (
	qse_wchar_t*       str,
	const qse_wchar_t* delim,
	qse_wchar_t        lquote,
	qse_wchar_t        rquote,
	qse_wchar_t        escape,
	const qse_wchar_t* trset
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strspl(str,delim,lquote,rquote,escape) qse_mbsspl(str,delim,lquote,rquote,escape)
#	define qse_strspltrn(str,delim,lquote,rquote,escape,trset) qse_mbsspltrn(str,delim,lquote,rquote,escape,trset)
#else
#	define qse_strspl(str,delim,lquote,rquote,escape) qse_wcsspl(str,delim,lquote,rquote,escape)
#	define qse_strspltrn(str,delim,lquote,rquote,escape,trset) qse_wcsspltrn(str,delim,lquote,rquote,escape,trset)
#endif


QSE_EXPORT qse_mchar_t* qse_mbstok (
	const qse_mchar_t* s,
	const qse_mchar_t* delim, 
	qse_mcstr_t*       tok
);

QSE_EXPORT qse_mchar_t* qse_mbsxtok (
	const qse_mchar_t* s,
	qse_size_t         len,
	const qse_mchar_t* delim,
	qse_mcstr_t*       tok
);

QSE_EXPORT qse_mchar_t* qse_mbsxntok (
	const qse_mchar_t* s,
	qse_size_t         len,
	const qse_mchar_t* delim,
	qse_size_t         delim_len, 
	qse_mcstr_t*       tok
);

QSE_EXPORT qse_wchar_t* qse_wcstok (
	const qse_wchar_t* s,
	const qse_wchar_t* delim, 
	qse_wcstr_t*       tok
);

QSE_EXPORT qse_wchar_t* qse_wcsxtok (
	const qse_wchar_t* s,
	qse_size_t         len,
	const qse_wchar_t* delim,
	qse_wcstr_t*       tok
);

QSE_EXPORT qse_wchar_t* qse_wcsxntok (
	const qse_wchar_t* s,
	qse_size_t         len,
	const qse_wchar_t* delim,
	qse_size_t         delim_len, 
	qse_wcstr_t*       tok
);

#if defined(QSE_CHAR_IS_MCHAR)
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
 * \code
 * qse_mchar_t a[] = QSE_MT("   this is a test string   ");
 * qse_mbstrmx (a, QSE_MBSTRMX_LEFT|QSE_MBSTRMX_RIGHT);
 * \endcode
 *
 * \return pointer to a trimmed string.
 */
QSE_EXPORT qse_mchar_t* qse_mbstrmx (
	qse_mchar_t* str,   /**< string */
	int          flags  /**< option OR'ed of #qse_mbstrmx_flag_t values */
);

QSE_EXPORT qse_mchar_t* qse_mbsxtrmx (
	qse_mchar_t* str,   /**< string */
	qse_size_t*  len,   /**< [IN/OUT] length */
	int          flags  /**< option OR'ed of #qse_mbstrmx_flag_t values */
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
 * \code
 * qse_wchar_t a[] = QSE_WT("   this is a test string   ");
 * qse_wcstrmx (a, QSE_STRTRMX_LEFT|QSE_STRTRMX_RIGHT);
 * \endcode
 *
 * \return pointer to a trimmed string.
 */
QSE_EXPORT qse_wchar_t* qse_wcstrmx (
	qse_wchar_t* str,   /**< string */
	int          flags  /**< option OR'ed of #qse_wcstrmx_flag_t values */
);

QSE_EXPORT qse_wchar_t* qse_wcsxtrmx (
	qse_wchar_t* str,   /**< string */
	qse_size_t*  len,   /**< [IN/OUT] length */
	int          flags  /**< option OR'ed of #qse_wcstrmx_flag_t values */
);

/**
 * The qse_mbstrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by \a str; QSE_MT('\0') is inserted after the last non-space character.
 * \return length of the string without leading and trailing spaces.
 */
QSE_EXPORT qse_size_t qse_mbstrm (
	qse_mchar_t* str /**< string */
);

/**
 * The qse_wcstrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by \a str; QSE_WT('\0') is inserted after the last non-space character.
 * \return length of the string without leading and trailing spaces.
 */
QSE_EXPORT qse_size_t qse_wcstrm (
	qse_wchar_t* str /**< string */
);

/**
 * The qse_mbsxtrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by \a str; QSE_MT('\0') is inserted after the last non-space character.
 * \return length of the string without leading and trailing spaces.
 */
QSE_EXPORT qse_size_t qse_mbsxtrm (
	qse_mchar_t* str, /**< string */
	qse_size_t   len  /**< length */
);

/**
 * The qse_wcsxtrm() function strips leading spaces and/or trailing
 * spaces off a string. All characters between the first and the last non-space
 * character inclusive are relocated to the beginning of memory pointed to 
 * by \a str; QSE_WT('\0') is inserted after the last non-space character.
 * \return length of the string without leading and trailing spaces.
 */
QSE_EXPORT qse_size_t qse_wcsxtrm (
	qse_wchar_t* str, /**< string */
	qse_size_t   len  /**< length */
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strtrmx(str,opt)      qse_mbstrmx(str,opt)
#	define qse_strxtrmx(str,len,opt) qse_mbsxtrmx(str,len,opt)
#	define qse_strtrm(str)           qse_mbstrm(str)
#	define qse_strxtrm(str,len)      qse_mbsxtrm(str,len)
#else
#	define qse_strtrmx(str,opt)       qse_wcstrmx(str,opt)
#	define qse_strxtrmx(str,len,opt)  qse_wcsxtrmx(str,len,opt)
#	define qse_strtrm(str)            qse_wcstrm(str)
#	define qse_strxtrm(str,len)       qse_wcsxtrm(str,len)
#endif

/**
 * The qse_mbspac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * \return length of the string without leading and trailing spaces.
 */
QSE_EXPORT qse_size_t qse_mbspac (
	qse_mchar_t* str /**< string */
);

/**
 * The qse_wcspac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * \return length of the string without leading and trailing spaces.
 */
QSE_EXPORT qse_size_t qse_wcspac (
	qse_wchar_t* str /**< string */
);


/**
 * The qse_mbsxpac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * \return length of the string without leading and trailing spaces.
 */
QSE_EXPORT qse_size_t qse_mbsxpac (
	qse_mchar_t* str, /**< string */
	qse_size_t   len  /**< length */
);

/**
 * The qse_wcsxpac() function folds repeated whitespaces into one as well
 * as stripping leading whitespaces and trailing whitespaces.
 * \return length of the string without leading and trailing spaces.
 */
QSE_EXPORT qse_size_t qse_wcsxpac (
	qse_wchar_t* str, /**< string */
	qse_size_t   len  /**< length */
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strpac(str)      qse_mbspac(str)
#	define qse_strxpac(str,len) qse_mbsxpac(str,len)
#else
#	define qse_strpac(str)      qse_wcspac(str)
#	define qse_strxpac(str,len) qse_wcsxpac(str,len)
#endif

QSE_EXPORT int qse_mbsfnmat (
	const qse_mchar_t* str,
	const qse_mchar_t* ptn,
	int                flags
);

QSE_EXPORT int qse_mbsxfnmat  (
     const qse_mchar_t* str,
	qse_size_t         slen,
	const qse_mchar_t* ptn,
	int                flags
);

QSE_EXPORT int qse_mbsnfnmat  (
     const qse_mchar_t* str,
	const qse_mchar_t* ptn,
	qse_size_t         plen,
	int                flags
);

QSE_EXPORT int qse_mbsxnfnmat (
	const qse_mchar_t* str,
	qse_size_t         slen,
	const qse_mchar_t* ptn,
	qse_size_t         plen,
	int                flags
);

QSE_EXPORT int qse_wcsfnmat (
	const qse_wchar_t* str,
	const qse_wchar_t* ptn,
	int                flags
);

QSE_EXPORT int qse_wcsxfnmat  (
     const qse_wchar_t* str,
	qse_size_t         slen,
	const qse_wchar_t* ptn,
	int                flags
);

QSE_EXPORT int qse_wcsnfnmat  (
     const qse_wchar_t* str,
	const qse_wchar_t* ptn,
	qse_size_t         plen,
	int                flags
);

QSE_EXPORT int qse_wcsxnfnmat (
	const qse_wchar_t* str,
	qse_size_t         slen,
	const qse_wchar_t* ptn,
	qse_size_t         plen,
	int                flags
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strfnmat(str,ptn,flags)             qse_mbsfnmat(str,ptn,flags)
#	define qse_strxfnmat(str,slen,ptn,flags)       qse_mbsxfnmat(str,slen,ptn,flags)
#	define qse_strnfnmat(str,ptn,plen,flags)       qse_mbsnfnmat(str,ptn,plen,flags)
#	define qse_strxnfnmat(str,slen,ptn,plen,flags) qse_mbsxnfnmat(str,slen,ptn,plen,flags)
#else
#	define qse_strfnmat(str,ptn,flags)             qse_wcsfnmat(str,ptn,flags)
#	define qse_strxfnmat(str,slen,ptn,flags)       qse_wcsxfnmat(str,slen,ptn,flags)
#	define qse_strnfnmat(str,ptn,plen,flags)       qse_wcsnfnmat(str,ptn,plen,flags)
#	define qse_strxnfnmat(str,slen,ptn,plen,flags) qse_wcsxnfnmat(str,slen,ptn,plen,flags)
#endif


/**
 * The qse_mbs_open() function creates a dynamically resizable multibyte string.
 */
QSE_EXPORT qse_mbs_t* qse_mbs_open (
	qse_mmgr_t* mmgr,
	qse_size_t  ext,
	qse_size_t  capa
);

QSE_EXPORT void qse_mbs_close (
	qse_mbs_t* mbs
);

/**
 * The qse_mbs_init() function initializes a dynamically resizable string
 * If the parameter capa is 0, it doesn't allocate the internal buffer 
 * in advance and always succeeds.
 * \return 0 on success, -1 on failure.
 */
QSE_EXPORT int qse_mbs_init (
	qse_mbs_t*  mbs,
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);

/**
 * The qse_mbs_fini() function finalizes a dynamically resizable string.
 */
QSE_EXPORT void qse_mbs_fini (
	qse_mbs_t* mbs
);

QSE_EXPORT qse_mmgr_t* qse_mbs_getmmgr (
	qse_mbs_t* mbs
);

QSE_EXPORT void* qse_mbs_getxtn (
	qse_mbs_t* mbs
);

/**
 * The qse_mbs_yield() function assigns the buffer to an variable of the
 * #qse_mcstr_t type and recreate a new buffer of the \a new_capa capacity.
 * The function fails if it fails to allocate a new buffer.
 * \return 0 on success, and -1 on failure.
 */
QSE_EXPORT int qse_mbs_yield (
	qse_mbs_t*   str,    /**< string */
	qse_mcstr_t* buf,    /**< buffer pointer */
	qse_size_t   newcapa /**< new capacity */
);

QSE_EXPORT qse_mchar_t* qse_mbs_yieldptr (
	qse_mbs_t*   str,    /**< string */
	qse_size_t   newcapa /**< new capacity */
);

/**
 * The qse_mbs_getsizer() function gets the sizer.
 * \return sizer function set or QSE_NULL if no sizer is set.
 */
QSE_EXPORT qse_mbs_sizer_t qse_mbs_getsizer (
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
QSE_EXPORT void qse_mbs_setsizer (
	qse_mbs_t*      str,
	qse_mbs_sizer_t sizer
);

/**
 * The qse_mbs_getcapa() function returns the current capacity.
 * You may use QSE_STR_CAPA(str) macro for performance sake.
 * \return current capacity in number of characters.
 */
QSE_EXPORT qse_size_t qse_mbs_getcapa (
	qse_mbs_t* str
);

/**
 * The qse_mbs_setcapa() function sets the new capacity. If the new capacity
 * is smaller than the old, the overflowing characters are removed from
 * from the buffer.
 * \return (qse_size_t)-1 on failure, new capacity on success 
 */
QSE_EXPORT qse_size_t qse_mbs_setcapa (
	qse_mbs_t* str,
	qse_size_t capa
);

/**
 * The qse_mbs_getlen() function return the string length.
 */
QSE_EXPORT qse_size_t qse_mbs_getlen (
	qse_mbs_t* str
);

/**
 * The qse_mbs_setlen() function changes the string length.
 * \return (qse_size_t)-1 on failure, new length on success 
 */
QSE_EXPORT qse_size_t qse_mbs_setlen (
	qse_mbs_t* str,
	qse_size_t len
);

/**
 * The qse_mbs_clear() funtion deletes all characters in a string and sets
 * the length to 0. It doesn't resize the internal buffer.
 */
QSE_EXPORT void qse_mbs_clear (
	qse_mbs_t* str
);

/**
 * The qse_mbs_swap() function exchanges the pointers to a buffer between
 * two strings. It updates the length and the capacity accordingly.
 */
QSE_EXPORT void qse_mbs_swap (
	qse_mbs_t* str1,
	qse_mbs_t* str2
);

QSE_EXPORT qse_size_t qse_mbs_cpy (
	qse_mbs_t*         str,
	const qse_mchar_t* s
);

QSE_EXPORT qse_size_t qse_mbs_ncpy (
	qse_mbs_t*         str,
	const qse_mchar_t* s,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_mbs_cat (
	qse_mbs_t*         str,
	const qse_mchar_t* s
);

QSE_EXPORT qse_size_t qse_mbs_ncat (
	qse_mbs_t*         str,
	const qse_mchar_t* s,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_mbs_nrcat (
	qse_mbs_t*         str,
	const qse_mchar_t* s,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_mbs_ccat (
	qse_mbs_t*  str,
	qse_mchar_t c
);

QSE_EXPORT qse_size_t qse_mbs_nccat (
	qse_mbs_t*  str,
	qse_mchar_t c,
	qse_size_t  len
);

QSE_EXPORT qse_size_t qse_mbs_del (
	qse_mbs_t* str,
	qse_size_t index,
	qse_size_t size
);

QSE_EXPORT qse_size_t qse_mbs_amend (
	qse_mbs_t*         str,
	qse_size_t         index,
	qse_size_t         size,
	const qse_mchar_t* repl
);

QSE_EXPORT qse_size_t qse_mbs_trm (
	qse_mbs_t* str
);

QSE_EXPORT qse_size_t qse_mbs_pac (
	qse_mbs_t* str
);

QSE_EXPORT qse_size_t qse_mbs_fcat (
	qse_mbs_t*         str,
	const qse_mchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_mbs_vfcat (
	qse_mbs_t*         str,
	const qse_mchar_t* fmt,
	va_list            ap
);

QSE_EXPORT qse_size_t qse_mbs_fmt (
	qse_mbs_t*         str,
	const qse_mchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_mbs_vfmt (
	qse_mbs_t*         str,
	const qse_mchar_t* fmt,
	va_list            ap
);

/**
 * The qse_wcs_open() function creates a dynamically resizable wide-character
 * string.
 */
QSE_EXPORT qse_wcs_t* qse_wcs_open (
	qse_mmgr_t* mmgr,
	qse_size_t  ext,
	qse_size_t  capa
);

QSE_EXPORT void qse_wcs_close (
	qse_wcs_t* wcs
);

/**
 * The qse_wcs_init() function initializes a dynamically resizable string
 * If the parameter capa is 0, it doesn't allocate the internal buffer 
 * in advance and always succeeds.
 * \return 0 on success, -1 on failure.
 */
QSE_EXPORT int qse_wcs_init (
	qse_wcs_t*  wcs,
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);

/**
 * The qse_wcs_fini() function finalizes a dynamically resizable string.
 */
QSE_EXPORT void qse_wcs_fini (
	qse_wcs_t* wcs
);

QSE_EXPORT qse_mmgr_t* qse_wcs_getmmgr (
	qse_wcs_t* wcs
);

QSE_EXPORT void* qse_wcs_getxtn (
	qse_wcs_t* wcs
);

/**
 * The qse_wcs_yield() function assigns the buffer to an variable of the
 * #qse_wcstr_t type and recreate a new buffer of the \a new_capa capacity.
 * The function fails if it fails to allocate a new buffer.
 * \return 0 on success, and -1 on failure.
 */
QSE_EXPORT int qse_wcs_yield (
	qse_wcs_t*   str,     /**< string */
	qse_wcstr_t* buf,     /**< buffer pointer */
	qse_size_t   new_capa /**< new capacity */
);

QSE_EXPORT qse_wchar_t* qse_wcs_yieldptr (
	qse_wcs_t*   str,    /**< string */
	qse_size_t   newcapa /**< new capacity */
);

/**
 * The qse_wcs_getsizer() function gets the sizer.
 * \return sizer function set or QSE_NULL if no sizer is set.
 */
QSE_EXPORT qse_wcs_sizer_t qse_wcs_getsizer (
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
QSE_EXPORT void qse_wcs_setsizer (
	qse_wcs_t*      str,
	qse_wcs_sizer_t sizer
);

/**
 * The qse_wcs_getcapa() function returns the current capacity.
 * You may use QSE_STR_CAPA(str) macro for performance sake.
 * \return current capacity in number of characters.
 */
QSE_EXPORT qse_size_t qse_wcs_getcapa (
	qse_wcs_t* str
);

/**
 * The qse_wcs_setcapa() function sets the new capacity. If the new capacity
 * is smaller than the old, the overflowing characters are removed from
 * from the buffer.
 * \return (qse_size_t)-1 on failure, new capacity on success 
 */
QSE_EXPORT qse_size_t qse_wcs_setcapa (
	qse_wcs_t* str,
	qse_size_t capa
);

/**
 * The qse_wcs_getlen() function return the string length.
 */
QSE_EXPORT qse_size_t qse_wcs_getlen (
	qse_wcs_t* str
);

/**
 * The qse_wcs_setlen() function changes the string length.
 * \return (qse_size_t)-1 on failure, new length on success 
 */
QSE_EXPORT qse_size_t qse_wcs_setlen (
	qse_wcs_t* str,
	qse_size_t len
);

/**
 * The qse_wcs_clear() funtion deletes all characters in a string and sets
 * the length to 0. It doesn't resize the internal buffer.
 */
QSE_EXPORT void qse_wcs_clear (
	qse_wcs_t* str
);

/**
 * The qse_wcs_swap() function exchanges the pointers to a buffer between
 * two strings. It updates the length and the capacity accordingly.
 */
QSE_EXPORT void qse_wcs_swap (
	qse_wcs_t* str1,
	qse_wcs_t* str2
);

QSE_EXPORT qse_size_t qse_wcs_cpy (
	qse_wcs_t*         str,
	const qse_wchar_t* s
);

QSE_EXPORT qse_size_t qse_wcs_ncpy (
	qse_wcs_t*         str,
	const qse_wchar_t* s,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_wcs_cat (
	qse_wcs_t*         str,
	const qse_wchar_t* s
);

QSE_EXPORT qse_size_t qse_wcs_ncat (
	qse_wcs_t*         str,
	const qse_wchar_t* s,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_wcs_nrcat (
	qse_wcs_t*         str,
	const qse_wchar_t* s,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_wcs_ccat (
	qse_wcs_t*  str,
	qse_wchar_t c
);

QSE_EXPORT qse_size_t qse_wcs_nccat (
	qse_wcs_t*  str,
	qse_wchar_t c,
	qse_size_t  len
);

QSE_EXPORT qse_size_t qse_wcs_del (
	qse_wcs_t* str,
	qse_size_t index,
	qse_size_t size
);

/**
 * The qse_wcs_amend() function is a versatile string editing function.
 * It selects a string segment as long as \a size characters starting from
 * the \a index position and changes it to the replacement string \a repl. 
 * \return (qse_size_t)-1 on failure, string length on success.
 */
QSE_EXPORT qse_size_t qse_wcs_amend (
	qse_wcs_t*         str,
	qse_size_t         index,
	qse_size_t         size,
	const qse_wchar_t* repl
);

QSE_EXPORT qse_size_t qse_wcs_trm (
	qse_wcs_t* str
);

QSE_EXPORT qse_size_t qse_wcs_pac (
	qse_wcs_t* str
);

QSE_EXPORT qse_size_t qse_wcs_fcat (
	qse_wcs_t*         str,
	const qse_wchar_t* fmt,
	...
);

QSE_EXPORT qse_size_t qse_wcs_fmt (
	qse_wcs_t*         str,
	const qse_wchar_t* fmt,
	...
);

#if defined(QSE_CHAR_IS_MCHAR)
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
#	define qse_str_nrcat(str,s,len)     qse_mbs_nrcat(str,s,len)
#	define qse_str_ccat(str,c)          qse_mbs_ccat(str,c)
#	define qse_str_nccat(str,c,len)     qse_mbs_nccat(str,c,len)
#	define qse_str_del(str,index,size)  qse_mbs_del(str,index,size)
#	define qse_str_amend(str,index,size,repl)  qse_mbs_amend(str,index,size,repl)
#	define qse_str_trm(str)             qse_mbs_trm(str)
#	define qse_str_pac(str)             qse_mbs_pac(str)
#	define qse_str_fcat                 qse_mbs_fcat
#	define qse_str_vfcat                qse_mbs_vfcat
#	define qse_str_fmt                  qse_mbs_fmt
#	define qse_str_vfmt                 qse_mbs_vfmt
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
#	define qse_str_nrcat(str,s,len)     qse_wcs_nrcat(str,s,len)
#	define qse_str_ccat(str,c)          qse_wcs_ccat(str,c)
#	define qse_str_nccat(str,c,len)     qse_wcs_nccat(str,c,len)
#	define qse_str_del(str,index,size)  qse_wcs_del(str,index,size)
#	define qse_str_amend(str,index,size,repl)  qse_wcs_amend(str,index,size,repl)
#	define qse_str_trm(str)             qse_wcs_trm(str)
#	define qse_str_pac(str)             qse_wcs_pac(str)
#	define qse_str_fcat                 qse_wcs_fcat
#	define qse_str_vfcat                qse_wcs_vfcat
#	define qse_str_fmt                  qse_wcs_fmt
#	define qse_str_vfmt                 qse_wcs_vfmt
#endif


#ifdef __cplusplus
}
#endif

#endif
