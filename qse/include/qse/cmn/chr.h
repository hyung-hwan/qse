/*
 * $Id: chr.h 554 2011-08-22 05:26:26Z hyunghwan.chung $
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

#ifndef _QSE_CMN_CHR_H_
#define _QSE_CMN_CHR_H_


/** @file
 * This file provides functions, types, macros for character handling.
 */

#include <qse/types.h>
#include <qse/macros.h>

/**
 * The qse_ccls_id_t type defines character class types.
 */
enum qse_ccls_id_t
{
        QSE_CCLS_ALNUM = 1,
        QSE_CCLS_ALPHA,
        QSE_CCLS_BLANK,
        QSE_CCLS_CNTRL,
        QSE_CCLS_DIGIT,
        QSE_CCLS_GRAPH,
        QSE_CCLS_LOWER,
        QSE_CCLS_PRINT,
        QSE_CCLS_PUNCT,
        QSE_CCLS_SPACE,
        QSE_CCLS_UPPER,
        QSE_CCLS_XDIGIT
};
typedef enum qse_ccls_id_t qse_ccls_id_t;
typedef qse_ccls_id_t qse_mccls_id_t;
typedef qse_ccls_id_t qse_wccls_id_t;

#define QSE_MCCLS_UPPER  QSE_CCLS_UPPER
#define QSE_MCCLS_LOWER  QSE_CCLS_LOWER
#define QSE_MCCLS_ALPHA  QSE_CCLS_ALPHA
#define QSE_MCCLS_DIGIT  QSE_CCLS_DIGIT
#define QSE_MCCLS_XDIGIT QSE_CCLS_XDIGIT
#define QSE_MCCLS_ALNUM  QSE_CCLS_ALNUM
#define QSE_MCCLS_SPACE  QSE_CCLS_SPACE
#define QSE_MCCLS_PRINT  QSE_CCLS_PRINT
#define QSE_MCCLS_GRAPH  QSE_CCLS_GRAPH
#define QSE_MCCLS_CNTRL  QSE_CCLS_CNTRL
#define QSE_MCCLS_PUNCT  QSE_CCLS_PUNCT
#define QSE_MCCLS_BLANK  QSE_CCLS_BLANK

#define QSE_WCCLS_UPPER  QSE_CCLS_UPPER
#define QSE_WCCLS_LOWER  QSE_CCLS_LOWER
#define QSE_WCCLS_ALPHA  QSE_CCLS_ALPHA
#define QSE_WCCLS_DIGIT  QSE_CCLS_DIGIT
#define QSE_WCCLS_XDIGIT QSE_CCLS_XDIGIT
#define QSE_WCCLS_ALNUM  QSE_CCLS_ALNUM
#define QSE_WCCLS_SPACE  QSE_CCLS_SPACE
#define QSE_WCCLS_PRINT  QSE_CCLS_PRINT
#define QSE_WCCLS_GRAPH  QSE_CCLS_GRAPH
#define QSE_WCCLS_CNTRL  QSE_CCLS_CNTRL
#define QSE_WCCLS_PUNCT  QSE_CCLS_PUNCT
#define QSE_WCCLS_BLANK  QSE_CCLS_BLANK

#define QSE_MCTYPE(name) (qse_getmcclsid(name))
#define QSE_ISMCTYPE(c,t) (qse_ismccls(c,t))
#define QSE_ISMALNUM(c) (qse_ismccls(c,QSE_CCLS_ALNUM))
#define QSE_ISMALPHA(c) (qse_ismccls(c,QSE_CCLS_ALPHA))
#define QSE_ISMBLANK(c) (qse_ismccls(c,QSE_CCLS_BLANK))
#define QSE_ISMCNTRL(c) (qse_ismccls(c,QSE_CCLS_CNTRL))
#define QSE_ISMDIGIT(c) (qse_ismccls(c,QSE_CCLS_DIGIT))
#define QSE_ISMGRAPH(c) (qse_ismccls(c,QSE_CCLS_GRAPH))
#define QSE_ISMLOWER(c) (qse_ismccls(c,QSE_CCLS_LOWER))
#define QSE_ISMPRINT(c) (qse_ismccls(c,QSE_CCLS_PRINT))
#define QSE_ISMPUNCT(c) (qse_ismccls(c,QSE_CCLS_PUNCT))
#define QSE_ISMSPACE(c) (qse_ismccls(c,QSE_CCLS_SPACE))
#define QSE_ISMUPPER(c) (qse_ismccls(c,QSE_CCLS_UPPER))
#define QSE_ISMXDIGIT(c) (qse_ismccls(c,QSE_CCLS_XDIGIT))
#define QSE_TOMUPPER(c) (qse_tomccls(c,QSE_CCLS_UPPER))
#define QSE_TOMLOWER(c) (qse_tomccls(c,QSE_CCLS_LOWER))

#define QSE_WCTYPE(name) (qse_getwcclsid(name))
#define QSE_ISWCTYPE(c,t) (qse_iswccls(c,t))
#define QSE_ISWALNUM(c) (qse_iswccls(c,QSE_CCLS_ALNUM))
#define QSE_ISWALPHA(c) (qse_iswccls(c,QSE_CCLS_ALPHA))
#define QSE_ISWBLANK(c) (qse_iswccls(c,QSE_CCLS_BLANK))
#define QSE_ISWCNTRL(c) (qse_iswccls(c,QSE_CCLS_CNTRL))
#define QSE_ISWDIGIT(c) (qse_iswccls(c,QSE_CCLS_DIGIT))
#define QSE_ISWGRAPH(c) (qse_iswccls(c,QSE_CCLS_GRAPH))
#define QSE_ISWLOWER(c) (qse_iswccls(c,QSE_CCLS_LOWER))
#define QSE_ISWPRINT(c) (qse_iswccls(c,QSE_CCLS_PRINT))
#define QSE_ISWPUNCT(c) (qse_iswccls(c,QSE_CCLS_PUNCT))
#define QSE_ISWSPACE(c) (qse_iswccls(c,QSE_CCLS_SPACE))
#define QSE_ISWUPPER(c) (qse_iswccls(c,QSE_CCLS_UPPER))
#define QSE_ISWXDIGIT(c) (qse_iswccls(c,QSE_CCLS_XDIGIT))
#define QSE_TOWUPPER(c) (qse_towccls(c,QSE_CCLS_UPPER))
#define QSE_TOWLOWER(c) (qse_towccls(c,QSE_CCLS_LOWER))

#ifdef QSE_CHAR_IS_MCHAR
#	define QSE_CTYPE(name) QSE_MCTYPE(name)
#	define QSE_ISCTYPE(c,t) QSE_ISMCTYPE(c,t)
#
#	define QSE_ISUPPER(c)  QSE_ISMUPPER(c)
#	define QSE_ISLOWER(c)  QSE_ISMLOWER(c) 
#	define QSE_ISALPHA(c)  QSE_ISMALPHA(c) 
#	define QSE_ISDIGIT(c)  QSE_ISMDIGIT(c) 
#	define QSE_ISXDIGIT(c) QSE_ISMXDIGIT(c)
#	define QSE_ISALNUM(c)  QSE_ISMALNUM(c)
#	define QSE_ISSPACE(c)  QSE_ISMSPACE(c)
#	define QSE_ISPRINT(c)  QSE_ISMPRINT(c)
#	define QSE_ISGRAPH(c)  QSE_ISMGRAPH(c)
#	define QSE_ISCNTRL(c)  QSE_ISMCNTRL(c)
#	define QSE_ISPUNCT(c)  QSE_ISMPUNCT(c)
#	define QSE_ISBLANK(c)  QSE_ISMBLANK(c)
#
#	define QSE_TOUPPER(c)  QSE_TOMUPPER(c)
#	define QSE_TOLOWER(c)  QSE_TOMLOWER(c)
#else
#	define QSE_CTYPE(name) QSE_WCTYPE(name)
#	define QSE_ISCTYPE(c,t) QSE_ISWCTYPE(c,t)
#
#	define QSE_ISUPPER(c)  QSE_ISWUPPER(c)
#	define QSE_ISLOWER(c)  QSE_ISWLOWER(c) 
#	define QSE_ISALPHA(c)  QSE_ISWALPHA(c) 
#	define QSE_ISDIGIT(c)  QSE_ISWDIGIT(c) 
#	define QSE_ISXDIGIT(c) QSE_ISWXDIGIT(c)
#	define QSE_ISALNUM(c)  QSE_ISWALNUM(c)
#	define QSE_ISSPACE(c)  QSE_ISWSPACE(c)
#	define QSE_ISPRINT(c)  QSE_ISWPRINT(c)
#	define QSE_ISGRAPH(c)  QSE_ISWGRAPH(c)
#	define QSE_ISCNTRL(c)  QSE_ISWCNTRL(c)
#	define QSE_ISPUNCT(c)  QSE_ISWPUNCT(c)
#	define QSE_ISBLANK(c)  QSE_ISWBLANK(c)
#
#	define QSE_TOUPPER(c)  QSE_TOWUPPER(c)
#	define QSE_TOLOWER(c)  QSE_TOWLOWER(c)
#endif

/**
 * The qse_mbstate_t type defines a structure large enough to hold
 * the standard mbstate_t.
 */
typedef struct qse_mbstate_t qse_mbstate_t;
struct qse_mbstate_t
{
#if defined(QSE_SIZEOF_MBSTATE_T) && (QSE_SIZEOF_MBSTATE_T > 0)
	char dummy[QSE_SIZEOF_MBSTATE_T];
#else
	char dummy[1];
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

qse_bool_t qse_ismccls (
	qse_mcint_t      c,
	qse_mccls_id_t   type
);

qse_bool_t qse_iswccls (
	qse_wcint_t      c,
	qse_wccls_id_t   type
);

qse_mcint_t qse_tomccls (
	qse_mcint_t      c,
	qse_mccls_id_t   type
);

qse_wcint_t qse_towccls (
	qse_wcint_t      c,
	qse_wccls_id_t   type
);

int qse_getwcclsidbyname (
	const qse_wchar_t* name,
	qse_wccls_id_t*    id
);

int qse_getwcclsidbyxname (
	const qse_wchar_t* name,
	qse_size_t         len,
	qse_wccls_id_t*    id
);

qse_wccls_id_t qse_getwcclsid (
	const qse_wchar_t* name
);

int qse_getmcclsidbyname (
	const qse_mchar_t* name,
	qse_mccls_id_t*    id
);

int qse_getmcclsidbyxname (
	const qse_mchar_t* name,
	qse_size_t         len,
	qse_mccls_id_t*    id
);

qse_mccls_id_t qse_getmcclsid (
	const qse_mchar_t* name
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_isccls(c,type) qse_ismccls(c,type)
#	define qse_toccls(c,type) qse_tomccls(c,type)
#	define qse_getcclsidbyname(name,id) qse_getmcclsidbyname(name,id)
#	define qse_getcclsidbyxname(name,len,id) qse_getmcclsidbyxname(name,len,id)
#	define qse_getcclsid(name) qse_getmcclsid(name)
#else
#	define qse_isccls(c,type) qse_iswccls(c,type)
#	define qse_toccls(c,type) qse_towccls(c,type)
#	define qse_getcclsidbyname(name,id) qse_getwcclsidbyname(name,id)
#	define qse_getcclsidbyxname(name,len,id) qse_getwcclsidbyxname(name,len,id)
#	define qse_getcclsid(name) qse_getwcclsid(name)
#endif


qse_size_t qse_mbrlen (
	const qse_mchar_t* mb,
	qse_size_t         mblen,
	qse_mbstate_t*     state
);

qse_size_t qse_mbrtowc (
	const qse_mchar_t* mb,
	qse_size_t         mblen,
	qse_wchar_t*       wc,
	qse_mbstate_t*     state
);

qse_size_t qse_wcrtomb (
	qse_wchar_t        wc,
	qse_mchar_t*       mb,
	qse_size_t         mblen,
	qse_mbstate_t*     state
);

/**
 * The qse_mblen() function scans a multibyte sequence to get the number of 
 * bytes needed to form a wide character. It does not scan more than @a mblen
 * bytes.
 * @return number of bytes processed on success, 
 *         0 for invalid sequences, 
 *         mblen + 1 for incomplete sequences
 * @note This function can not handle conversion producing non-initial
 *       states. For each call, it assumes initial state.
 */
qse_size_t qse_mblen (
	const qse_mchar_t* mb,
	qse_size_t         mblen
);

/**
 * The qse_mbtowc() function converts a multibyte sequence to a wide character.
 * It returns 0 if an invalid multibyte sequence is detected, mblen + 1 if the 
 * sequence is incomplete. It returns the number of bytes processed to form a 
 * wide character.
 * @note This function can not handle conversion producing non-initial
 *       states. For each call, it assumes initial state.
 */
qse_size_t qse_mbtowc (
	const qse_mchar_t* mb,
	qse_size_t         mblen,
	qse_wchar_t*       wc
);

/**
 * The qse_wctomb() function converts a wide character to a multibyte sequence.
 * It returns 0 if the wide character is illegal, mblen + 1 if mblen is not 
 * large enough to hold the multibyte sequence. On successful conversion, it 
 * returns the number of bytes in the sequence.
 * @note This function can not handle conversion producing non-initial
 *       states. For each call, it assumes initial state.
 */
qse_size_t qse_wctomb (
	qse_wchar_t        wc,
	qse_mchar_t*       mb,
	qse_size_t         mblen
);

/**
 * The qse_getmbcurmax() function returns the value of MB_CUR_MAX.
 * Note that QSE_MBLEN_MAX defines MB_LEN_MAX.
 */
int qse_getmbcurmax (
	void
);

#ifdef __cplusplus
}
#endif

#endif
