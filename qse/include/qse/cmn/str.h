/*
 * $Id: str.h 287 2009-09-15 10:01:02Z hyunghwan.chung $
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
 *  <qse/cmn/str.h> defines various functions, types, macros to manipulate
 *  a string.
 *
 *  The qse_cstr_t type and the qse_xstr_t defined in <qse/types.h> helps you
 *  dealing with a string pointer and length.
 */

#define QSE_STR_LEN(s)       ((s)->len)
#define QSE_STR_PTR(s)       ((s)->ptr)
#define QSE_STR_CAPA(s)      ((s)->capa)
#define QSE_STR_CHAR(s,idx)  ((s)->ptr[idx])
#define QSE_STR_SIZER(s)     ((s)->sizer)

typedef struct qse_str_t qse_str_t;
typedef qse_size_t (*qse_str_sizer_t) (qse_str_t* data, qse_size_t hint);

/**
 * The qse_str_t type defines a dynamically resizable string.
 */
struct qse_str_t
{
	QSE_DEFINE_COMMON_FIELDS (str)
	qse_str_sizer_t sizer;
	qse_char_t*     ptr;
	qse_size_t      len;
	qse_size_t      capa;
};

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
 * The qse_strtrm_op_t defines a string trimming operation. 
 */
enum qse_strtrm_op_t
{
	QSE_STRTRM_LEFT  = (1 << 0), /**< trim leading spaces */
	QSE_STRTRM_RIGHT = (1 << 1)  /**< trim trailing spaces */
};

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * basic string functions
 */

/****f* Common/qse_strlen
 * NAME
 *  qse_strlen - get the number of characters
 * DESCRIPTION
 *  The qse_strlen() function returns the number of characters in a 
 *  null-terminated string. The length returned excludes a terminating null.
 * SYNOPSIS
 */
qse_size_t qse_strlen (
	const qse_char_t* str
);
/******/

/****f* Common/qse_strbytes
 * NAME
 *  qse_strbytes - get the length of a string in bytes
 * DESCRIPTOIN
 *  The qse_strbytes() function returns the number of bytes a null-terminated
 *  string is holding excluding a terminating null.
 * SYNOPSIS
 */
qse_size_t qse_strbytes (
	const qse_char_t* str
);
/******/

qse_size_t qse_strcpy (
	qse_char_t*       buf,
	const qse_char_t* str
);

qse_size_t qse_strxcpy (
	qse_char_t*       buf,
	qse_size_t        bsz,
	const qse_char_t* str
);

qse_size_t qse_strncpy (
	qse_char_t*       buf, 
	const qse_char_t* str,
	qse_size_t        len
);

qse_size_t qse_strxncpy (
	qse_char_t*       buf,
	qse_size_t        bsz,
	const qse_char_t* str,
	qse_size_t        len
);

/****f* Common/qse_strfcpy
 * NAME
 *  qse_strfcpy - copy a string
 * DESCRIPTION
 *  "format ${1} ${3} ${2} \\${1} string"
 * SYNOPSIS
 */
qse_size_t qse_strfcpy (
	qse_char_t*       buf,
	const qse_char_t* fmt,
	const qse_char_t* str[]
);
/******/

/****f* Common/qse_strfncpy
 * NAME
 *  qse_strfncpy - copy a string 
 * SYNOPSIS
 */
qse_size_t qse_strfncpy (
        qse_char_t*       buf,
        const qse_char_t* fmt, 
	const qse_cstr_t  str[]
);
/******/

/****f* Common/qse_strxfcpy
 * NAME
 *  qse_strxfcpy - copy a string
 * SYNOPSIS
 */
qse_size_t qse_strxfcpy (
	qse_char_t*       buf,
	qse_size_t        bsz,
	const qse_char_t* fmt,
	const qse_char_t* str[]
);
/******/

/****f* Common/qse_strxfncpy
 * NAME
 *  qse_strxfncpy - copy a string
 * SYNOPSIS
 */
qse_size_t qse_strxfncpy (
        qse_char_t*       buf,
	qse_size_t        bsz,
        const qse_char_t* fmt,
	const qse_cstr_t  str[]
);
/******/

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

int qse_strcmp (const qse_char_t* s1, const qse_char_t* s2);
int qse_strxcmp (const qse_char_t* s1, qse_size_t len1, const qse_char_t* s2);
int qse_strxncmp (
	const qse_char_t* s1, qse_size_t len1, 
	const qse_char_t* s2, qse_size_t len2);

int qse_strcasecmp (const qse_char_t* s1, const qse_char_t* s2);

/**
 * The qse_strxncasecmp() function compares characters at the same position 
 * in each string after converting them to the same case temporarily. 
 * It accepts two strings and a character class handler. A string is 
 * represented by its beginning pointer and length. 
 *
 * For two strings to be equal, they need to have the same length and all
 * characters in the first string should be equal to their counterpart in the
 * second string.
 *
 * The following code snippet compares "foo" and "FoO" case-insenstively.
 * @code
 * qse_strxncasecmp (QSE_T("foo"), 3, QSE_T("FoO"), 3);
 * @endcode
 *
 * @return
 * The qse_strxncasecmp() returns 0 if two strings are equal, a positive
 * number if the first string is larger, -1 if the second string is larger.
 *
 */
int qse_strxncasecmp (
	const qse_char_t* s1   /* the pointer to the first string */,
	qse_size_t        len1 /* the length of the first string */, 
	const qse_char_t* s2   /* the pointer to the second string */,
	qse_size_t        len2 /* the length of the second string */
);

qse_char_t* qse_strdup (const qse_char_t* str, qse_mmgr_t* mmgr);
qse_char_t* qse_strxdup (
	const qse_char_t* str, qse_size_t len, qse_mmgr_t* mmgr);
qse_char_t* qse_strxdup2 (
	const qse_char_t* str1, qse_size_t len1,
	const qse_char_t* str2, qse_size_t len2, qse_mmgr_t* mmgr);

qse_char_t* qse_strstr (const qse_char_t* str, const qse_char_t* sub);
qse_char_t* qse_strxstr (
	const qse_char_t* str, qse_size_t size, const qse_char_t* sub);
qse_char_t* qse_strxnstr (
	const qse_char_t* str, qse_size_t strsz, 
	const qse_char_t* sub, qse_size_t subsz);

qse_char_t* qse_strchr (const qse_char_t* str, qse_cint_t c);
qse_char_t* qse_strxchr (const qse_char_t* str, qse_size_t len, qse_cint_t c);
qse_char_t* qse_strrchr (const qse_char_t* str, qse_cint_t c);
qse_char_t* qse_strxrchr (const qse_char_t* str, qse_size_t len, qse_cint_t c);

/* Checks if a string begins with a substring */
qse_char_t* qse_strbeg (const qse_char_t* str, const qse_char_t* sub);
qse_char_t* qse_strxbeg (
	const qse_char_t* str, qse_size_t len, const qse_char_t* sub);
qse_char_t* qse_strnbeg (
	const qse_char_t* str, const qse_char_t* sub, qse_size_t len);
qse_char_t* qse_strxnbeg (
	const qse_char_t* str, qse_size_t len1, 
	const qse_char_t* sub, qse_size_t len2);

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

/* 
 * string conversion
 */
int qse_strtoi (const qse_char_t* str);
long qse_strtol (const qse_char_t* str);
unsigned int qse_strtoui (const qse_char_t* str);
unsigned long qse_strtoul (const qse_char_t* str);

int qse_strxtoi (const qse_char_t* str, qse_size_t len);
long qse_strxtol (const qse_char_t* str, qse_size_t len);
unsigned int qse_strxtoui (const qse_char_t* str, qse_size_t len);
unsigned long qse_strxtoul (const qse_char_t* str, qse_size_t len);

qse_int_t qse_strtoint (const qse_char_t* str);
qse_long_t qse_strtolong (const qse_char_t* str);
qse_uint_t qse_strtouint (const qse_char_t* str);
qse_ulong_t qse_strtoulong (const qse_char_t* str);

qse_int_t qse_strxtoint (const qse_char_t* str, qse_size_t len);
qse_long_t qse_strxtolong (const qse_char_t* str, qse_size_t len);
qse_uint_t qse_strxtouint (const qse_char_t* str, qse_size_t len);
qse_ulong_t qse_strxtoulong (const qse_char_t* str, qse_size_t len);

/****f* Common/qse_strspl
 * NAME
 *  qse_strspl - split a string into fields
 * SEE ALSO
 *  qse_strspltrn
 * SYNOPSIS
 */
int qse_strspl (
	qse_char_t*       str,
	const qse_char_t* delim,
	qse_char_t        lquote,
	qse_char_t        rquote,
	qse_char_t        escape
);
/******/

/****f* Common/qse_strspltrn
 * NAME
 *  qse_strspltrn - split a string translating special escape sequences 
 * DESCRIPTION
 *  The argument trset is a translation character set which is composed
 *  of multiple character pairs. An escape character followed by the 
 *  first character in a pair is translated into the second character
 *  in the pair. If trset is QSE_NULL, no translation is performed. 
 * EXAMPLES
 *  Let's translate a sequence of '\n' and '\r' to a new line and a carriage
 *  return respectively.
 *   qse_strspltrn (str, QSE_T(':'), QSE_T('['), QSE_T(']'), QSE_T('\\'), QSE_T("n\nr\r"), &nfields);
 *  Given [xxx]:[\rabc\ndef]:[] as an input, the example breaks the second 
 *  fields to <CR>abc<NL>def where <CR> is a carriage return and <NL> is a 
 *  new line.
 * SEE ALSO
 *  If you don't need any translation, you may call qse_strspl() alternatively.
 * SYNOPSIS
 */
int qse_strspltrn (
	qse_char_t*       str,
	const qse_char_t* delim,
	qse_char_t        lquote,
	qse_char_t        rquote,
	qse_char_t        escape,
	const qse_char_t* trset
);
/******/

/**
 * The qse_strtrm() function removes leading spaces and/or trailing
 * spaces from a string depending on the opt parameter. You can form
 * the op parameter by bitwise-OR'ing one or more of the following
 * values:
 *
 * - QSE_STRTRM_LEFT - trim leading spaces
 * - QSE_STRTRM_RIGHT - trim trailing spaces
 *
 * Should it remove leading spaces, it just returns the pointer to
 * the first non-space character in the string. Should it remove trailing
 * spaces, it inserts a QSE_T('\0') character after the last non-space
 * characters. Take note of this behavior.
 *
 * @code
 * qse_char_t a[] = QSE_T("   this is a test string   ");
 * qse_printf (QSE_T("[%s]\n"), qse_strtrm(a,QSE_STRTRM_LEFT|QSE_STRTRM_RIGHT));
 * @endcode
 *
 * @return the pointer to a trimmed string.
 */
qse_char_t* qse_strtrm (
	qse_char_t* str, /**< a string */
	int         op   /**< operation code XOR'ed of qse_strtrm_op_t values */
);


/****f* Common/qse_mbstowcs
 * NAME
 *  qse_mbstowcs - convert a multibyte string to a wide character string
 * SYNOPSIS
 */
qse_size_t qse_mbstowcs (
	const qse_mchar_t* mbs,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);
/******/

/****f* Common/qse_mbsntowcsn
 * NAME
 *  qse_mbsntowcsn - convert a multibyte string to a wide character string
 * RETURN
 *  The qse_mbstowcs() function returns the number of bytes handled.
 * SYNOPSIS
 */
qse_size_t qse_mbsntowcsn (
	const qse_mchar_t* mbs,
	qse_size_t         mbslen,
	qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);
/******/

/****f* Common/qse_wcstombslen
 * NAME
 *  qse_wcstombslen - get the length 
 * DESCRIPTION
 *  The qse_wcstombslen() function scans a null-terminated wide character 
 *  string to get the total number of multibyte characters that it can be
 *  converted to. The resulting number of characters is stored into memory
 *  pointed to by mbslen.
 * RETURN
 *  The qse_wcstombslen() function returns the number of wide characters 
 *  handled.
 * SYNOPSIS
 */
qse_size_t qse_wcstombslen (
	const qse_wchar_t* wcs,
	qse_size_t*        mbslen
);
/******/

/****f* Common/qse_wcsntombsnlen
 * NAME
 *  qse_wcsntombsnlen - get the length 
 * DESCRIPTION
 *  The qse_wcsntombsnlen() function scans a wide character wcs as long as
 *  wcslen characters to get the get the total number of multibyte characters 
 *  that it can be converted to. The resulting number of characters is stored 
 *  into memory pointed to by mbslen.
 * RETURN
 *  The qse_wcsntombsnlen() function returns the number of wide characters 
 *  handled.
 * SYNOPSIS
 */
qse_size_t qse_wcsntombsnlen (
	const qse_wchar_t* wcs,
	qse_size_t         wcslen,
	qse_size_t*        mbslen
);
/******/

/****f* Common/qse_wcstombs
 * NAME
 *  qse_wcstombs - convert a wide character string to a multibyte string.
 * DESCRIPTION
 *  The qse_wcstombs() function converts a null-terminated wide character 
 *  string to a multibyte string and stores it into the buffer pointed to
 *  by mbs. The pointer to a variable holding the buffer length should be
 *  passed to the function as the third parameter. After conversion, it holds 
 *  the length of the multibyte string excluding the terminating-null.
 *  It may not null-terminate the resulting multibyte string if the buffer
 *  is not large enough. You can check if the resulting mbslen is equal to 
 *  the input mbslen to know it.
 * RETURN
 *  The qse_wcstombs() function returns the number of wide characters handled.
 * SYNOPSIS
 */
qse_size_t qse_wcstombs (
	const qse_wchar_t* wcs,
	qse_mchar_t*       mbs,
	qse_size_t*        mbslen
);
/******/

/**
 * The qse_wcsntombsn() function converts a wide character string to a
 * multibyte string.
 * @return the number of wide characters
 */
qse_size_t qse_wcsntombsn (
	const qse_wchar_t* wcs,    /**< a wide string */
	qse_size_t         wcslen, /**< wide string length */
	qse_mchar_t*       mbs,    /**< a multibyte string buffer */
	qse_size_t*        mbslen  /**< the buffer size */
);

/**
 * The qse_wcstombs_strict() function performs the same as the qse_wcsmbs() 
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

/****f* Common/qse_str_init
 * NAME
 *  qse_str_init - initialize a dynamically resizable string
 * NOTE
 *  If the parameter capa is 0, it doesn't allocate the internal buffer 
 *  in advance.
 * SYNOPSIS
 */
qse_str_t* qse_str_init (
	qse_str_t*  str,
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);
/******/

/****f* Common/qse_str_fini
 * NAME
 *  qse_str_fini - finialize a dynamically resizable string
 * SYNOPSIS
 */
void qse_str_fini (
	qse_str_t* str
);
/******/

/****f* Common/qse_str_yield
 * NAME 
 *  qse_str_yield - yield the buffer 
 * 
 * DESCRIPTION
 *  The qse_str_yield() function assigns the buffer to an variable of the
 *  qse_xstr_t type and recreate a new buffer of the new_capa capacity.
 *  The function fails if it fails to allocate a new buffer.
 *
 * RETURN
 *  The qse_str_yield() function returns 0 on success, and -1 on failure.
 *
 * SYNOPSIS
 */
int qse_str_yield (
	qse_str_t*  str      /* a dynamic string */,
	qse_xstr_t* buf      /* the pointer to a qse_xstr_t variable */,
	int         new_capa /* new capacity in number of characters */
);
/******/

/****f* Common/qse_str_getsizer
 * NAME
 *  qse_str_getsizer - get the sizer
 * RETURN
 *  a sizer function set or QSE_NULL if no sizer is set.
 * SYNOPSIS
 */
qse_str_sizer_t qse_str_getsizer (
	qse_str_t* str
);
/******/

/****f* Common/qse_str_setsizer
 * NAME
 *  qse_str_setsizer - specify a sizer
 * DESCRIPTION
 *  The qse_str_setsizer() function specify a new sizer for a dynamic string.
 *  With no sizer specified, the dynamic string doubles the current buffer
 *  when it needs to increase its size. The sizer function is passed a dynamic
 *  string and the minimum capacity required to hold data after resizing.
 *  The string is truncated if the sizer function returns a smaller number
 *  than the hint passed.
 * SYNOPSIS
 */
void qse_str_setsizer (
	qse_str_t*      str,
	qse_str_sizer_t sizer
);
/******/

/****f* Common/qse_str_getcapa
 * NAME
 *  qse_str_getcapa - get capacity
 * DESCRIPTION
 *  The qse_str_getcapa() function returns the current capacity.
 *  You may use QSE_STR_CAPA(str) macro for performance sake.
 * RETURNS
 *  current capacity in number of characters.
 * SYNOPSIS
 */
qse_size_t qse_str_getcapa (
	qse_str_t* str
);
/******/

/****f* Common/qse_str_setcapa
 * NAME
 *  qse_str_setcapa - set new capacity
 * DESCRIPTION
 *  The qse_str_setcapa() function sets the new capacity. If the new capacity
 *  is smaller than the old, the overflowing characters are removed from
 *  from the buffer.
 * RETURNS
 *  (qse_size_t)-1 on failure, new capacity on success 
 * SYNOPSIS
 */
qse_size_t qse_str_setcapa (
	qse_str_t* str,
	qse_size_t capa
);
/******/

/****f* Common/qse_str_getlen
 * NAME
 *  qse_str_getlen - get length
 * SYNOPSIS
 */
qse_size_t qse_str_getlen (
	qse_str_t* str
);
/******/

/****f* Common/qse_str_setlen
 * NAME
 *  qse_str_setlen - change length
 * RETURNS
 *  (qse_size_t)-1 on failure, new length on success 
 * SYNOPSIS
 */
qse_size_t qse_str_setlen (
	qse_str_t* str,
	qse_size_t len
);
/******/

/****f* Common/qse_str_clear
 * NAME
 *  qse_str_clear - clear a string
 * DESCRIPTION
 *  The qse_str_clear() funtion deletes all characters in a string and sets
 *  the length to 0. It doesn't resize the internal buffer.
 * SYNOPSIS
 */
void qse_str_clear (
	qse_str_t* str
);
/******/

/****f* Common/qse_str_swap
 * NAME
 *  qse_str_swap - swap buffers of two dynamic string
 * DESCRIPTION
 *  The qse_str_swap() function exchanges the pointers to a buffer between
 *  two strings. It updates the length and the capacity accordingly.
 * SYNOPSIS
 */
void qse_str_swap (
	qse_str_t* str1,
	qse_str_t* str2
);
/******/

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


#ifdef __cplusplus
}
#endif

#endif
