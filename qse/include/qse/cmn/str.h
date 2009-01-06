/*
 * $Id: str.h 496 2008-12-15 09:56:48Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_CMN_STR_H_
#define _QSE_CMN_STR_H_

#include <qse/types.h>
#include <qse/macros.h>

/****o* qse.cmn.str/string
 * DESCRIPTION
 *  <qse/cmn/str.h> defines various functions, types, macros to manipulate
 *  strings.
 *
 *  The qse_cstr_t type and the qse_xstr_t defined in ase/types.h helps you
 *  dealing with a string pointer and length.
 *
 *  #include <qse/cmn/str.h>
 *
 * EXAMPLES
 *  void f (void)
 *  {
 *  }
 ******
 */


#define QSE_STR_LEN(s)       ((s)->len)
#define QSE_STR_PTR(s)       ((s)->ptr)
#define QSE_STR_CAPA(s)      ((s)->capa)
#define QSE_STR_CHAR(s,idx)  ((s)->ptr[idx])

#define QSE_STR_MMGR(s)      ((s)->mmgr)
#define QSE_STR_XTN(s)       ((void*)(((qse_str_t*)s) + 1))
#define QSE_STR_SIZER(s)     ((s)->sizer)

typedef struct qse_str_t qse_str_t;
typedef qse_size_t (*qse_str_sizer_t) (qse_str_t* data, qse_size_t hint);

struct qse_str_t
{
	qse_mmgr_t* mmgr;
	qse_str_sizer_t sizer;

	qse_char_t* ptr;
	qse_size_t  len;
	qse_size_t  capa;
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

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * basic string functions
 */
qse_size_t qse_strlen (const qse_char_t* str);
qse_size_t qse_strbytes (const qse_char_t* str);

qse_size_t qse_strcpy (
	qse_char_t* buf, const qse_char_t* str);
qse_size_t qse_strxcpy (
	qse_char_t* buf, qse_size_t bsz, const qse_char_t* str);
qse_size_t qse_strncpy (
	qse_char_t* buf, const qse_char_t* str, qse_size_t len);
qse_size_t qse_strxncpy (
    qse_char_t* buf, qse_size_t bsz, const qse_char_t* str, qse_size_t len);

qse_size_t qse_strxcat (
    qse_char_t* buf, qse_size_t bsz, const qse_char_t* str);
qse_size_t qse_strxncat (
    qse_char_t* buf, qse_size_t bsz, const qse_char_t* str, qse_size_t len);

int qse_strcmp (const qse_char_t* s1, const qse_char_t* s2);
int qse_strxcmp (const qse_char_t* s1, qse_size_t len1, const qse_char_t* s2);
int qse_strxncmp (
	const qse_char_t* s1, qse_size_t len1, 
	const qse_char_t* s2, qse_size_t len2);

int qse_strcasecmp (
	const qse_char_t* s1, const qse_char_t* s2, qse_ccls_t* ccls);

/****f* qse.cmn.str/qse_strxncasecmp
 * NAME
 *  qse_strxncasecmp - compare strings ignoring case
 *
 * DESCRIPTION
 *  The qse_strxncasecmp() function compares characters at the same position 
 *  in each string after converting them to the same case temporarily. 
 *  It accepts two strings and a character class handler. A string is 
 *  represented by its beginning pointer and length. You can write your own
 *  character class handler or use QSE_CCLS_GETDFL() to get the default 
 *  character class handler.
 *
 *  For two strings to be equal, they need to have the same length and all
 *  characters in the first string should be equal to their counterpart in the
 *  second string.
 *
 * RETURN
 *  The qse_strxncasecmp() returns 0 if two strings are equal, a positive
 *  number if the first string is larger, -1 if the second string is larger.
 *  
 * EXAMPLES
 *   qse_strxncasecmp (QSE_T("foo"), 3, QSE_T("FoO"), 3, QSE_CCLS_GETDFL());
 *
 * SYNOPSIS
 */
int qse_strxncasecmp (
	const qse_char_t* s1  /* the pointer to the first string */,
	qse_size_t len1       /* the length of the first string */, 
	const qse_char_t* s2  /* the pointer to the second string */,
	qse_size_t len2       /* the length of the second string */,
	qse_ccls_t* ccls      /* character class handler */
);
/******/

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

/* Checks if a string ends with a substring */
qse_char_t* qse_strend (const qse_char_t* str, const qse_char_t* sub);
qse_char_t* qse_strxend (
	const qse_char_t* str, qse_size_t len, const qse_char_t* sub);
qse_char_t* qse_strnend (
	const qse_char_t* str, const qse_char_t* sub, qse_size_t len);
qse_char_t* qse_strxnend (
	const qse_char_t* str, qse_size_t len1, 
	const qse_char_t* sub, qse_size_t len2);

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

qse_str_t* qse_str_open (
	qse_mmgr_t* mmgr,
	qse_size_t ext,
	qse_size_t capa
);

void qse_str_close (
	qse_str_t* str
);

/* 
 * If capa is 0, it doesn't allocate the buffer in advance. 
 */
qse_str_t* qse_str_init (
	qse_str_t* str,
	qse_mmgr_t* mmgr,
	qse_size_t capa
);

void qse_str_fini (
	qse_str_t* str
);

/****f* qse.cmn.str/qse_str_yield
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
	qse_str_t* str  /* a dynamic string */,
	qse_xstr_t* buf /* the pointer to a qse_xstr_t variable */,
	int new_capa    /* new capacity in number of characters */
);
/******/

void* qse_str_getxtn (
	qse_str_t* str
);

qse_mmgr_t* qse_str_getmmgr (
	qse_str_t* str
);

void qse_str_setmmgr (
	qse_str_t* str,
	qse_mmgr_t* mmgr
);

/*
 * NAME: get the sizer
 *
 * DESCRIPTION:
 *  The qse_str_getsizer() function returns the sizer specified.
 *
 * RETURNS: a sizer function set or QSE_NULL if no sizer is set.
 */
qse_str_sizer_t qse_str_getsizer (
	qse_str_t* str  /* a dynamic string */
);

/*
 * NAME: specify a sizer
 *
 * DESCRIPTION:
 *  The qse_str_setsizer() function specify a new sizer for a dynamic string.
 *  With no sizer specified, the dynamic string doubles the current buffer
 *  when it needs to increase its size. The sizer function is passed a dynamic
 *  string and the minimum capacity required to hold data after resizing.
 *  The string is truncated if the sizer function returns a smaller number
 *  than the hint passed.
 */
void qse_str_setsizer (
	qse_str_t* str         /* a dynamic string */,
	qse_str_sizer_t sizer /* a sizer function */
);

/*
 * NAME: get capacity
 *
 * DESCRIPTION:
 *  The qse_str_getcapa() function returns the current capacity.
 *  You may use QSE_STR_CAPA(str) macro for performance sake.
 *
 * RETURNS: the current capacity in number of characters.
 */
qse_size_t qse_str_getcapa (
	qse_str_t* str /* a dynamic string */
);

/*
 * NAME: set new capacity
 *
 * DESCRIPTION:
 *  The qse_str_setcapa() function set new capacity. If the new capacity
 *  is smaller than the old, the overflowing characters are removed from
 *  from the buffer.
 * 
 * RETURNS: -1 on failure, a new capacity on success 
 */
qse_size_t qse_str_setcapa (
	qse_str_t* str  /* a dynamic string */,
	qse_size_t capa /* a new capacity */
);

void qse_str_clear (
	qse_str_t* str
);

void qse_str_swap (
	qse_str_t* str,
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


qse_size_t qse_mbstowcs (
        const qse_mchar_t* mbs,
        qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);

/****f* qse.cmn.str/qse_mbsntowcsn
 * NAME
 *  qse_mbsntowcsn - convert a multibyte string to a wide character string
 * 
 * RETURN
 *  The qse_mbstowcs() function returns the number of bytes handled.
 *
 * SYNOPSIS
 */
qse_size_t qse_mbsntowcsn (
        const qse_mchar_t* mbs,
	qse_size_t         mbslen,
        qse_wchar_t*       wcs,
	qse_size_t*        wcslen
);
/******/

/****f* qse.cmn.str/qse_wcstombslen
 * NAME
 *  qse_wcstombslen - get the length 
 *
 * DESCRIPTION
 *  The qse_wcstombslen() function scans a null-terminated wide character 
 *  string to get the total number of multibyte characters that it can be
 *  converted to. The resulting number of characters is stored into memory
 *  pointed to by mbslen.
 * 
 * RETURN
 *  The qse_wcstombslen() function returns the number of wide characters 
 *  handled.
 *
 * SYNOPSIS
 */
qse_size_t qse_wcstombslen (
	const qse_wchar_t* wcs,
	qse_size_t*        mbslen
);
/******/

/****f* qse.cmn.str/qse_wcstombs
 * NAME
 *  qse_wcstombs - convert a wide character string to a multibyte string.
 *
 * DESCRIPTION
 *  The qse_wcstombs() function converts a null-terminated wide character 
 *  string to a multibyte string and stores it into the buffer pointed to
 *  by mbs. The pointer to a variable holding the buffer length should be
 *  passed to the function as the third parameter. After conversion, it holds 
 *  the length of the multibyte string excluding the terminating-null.
 *  It may not null-terminate the resulting multibyte string if the buffer
 *  is not large enough. You can check if the resulting mbslen is equal to 
 *  the input mbslen to know it.
 *
 * RETURN
 *  The qse_wcstombs() function returns the number of wide characters handled.
 *
 * SYNOPSIS
 */
qse_size_t qse_wcstombs (
        const qse_wchar_t* wcs,
        qse_mchar_t*       mbs,
	qse_size_t*        mbslen
);

/****f* qse.cmn.str/qse_wcsntombsn
 * NAME
 *  qse_wcstombs - convert a wide character string to a multibyte string
 *
 * RETURN
 *  The qse_wcstombs() function returns the number of wide characters handled.
 *
 * SYNOPSIS
 */
qse_size_t qse_wcsntombsn (
        const qse_wchar_t* wcs,
	qse_size_t         wcslen,
        qse_mchar_t*       mbs,
	qse_size_t*        mbslen
);
/******/

/****f* qse.cmn.str/qse_wcstombs_strict
 * NAME
 *  qse_wcstombs_strict - convert a wide character string to a multibyte string.
 *
 * DESCRIPTION
 *  The qse_wcstombs_strict() function performs the same as the qse_wcsmbs() 
 *  function except that it returns an error if it can't fully convert the
 *  input string and/or the buffer is not large enough.
 *
 * RETURN
 *  The qse_wcstombs_strict() function returns 0 on success and -1 on failure.
 * SYNOPSIS
 */
int qse_wcstombs_strict (
        const qse_wchar_t* wcs,
        qse_mchar_t*       mbs,
	qse_size_t         mbslen
);
/******/

#ifdef __cplusplus
}
#endif

#endif
