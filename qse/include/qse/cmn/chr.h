/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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
 * The qse_ctype_t type defines character class types.
 */
enum qse_ctype_t
{
        QSE_CTYPE_ALNUM = 1,
        QSE_CTYPE_ALPHA,
        QSE_CTYPE_BLANK,
        QSE_CTYPE_CNTRL,
        QSE_CTYPE_DIGIT,
        QSE_CTYPE_GRAPH,
        QSE_CTYPE_LOWER,
        QSE_CTYPE_PRINT,
        QSE_CTYPE_PUNCT,
        QSE_CTYPE_SPACE,
        QSE_CTYPE_UPPER,
        QSE_CTYPE_XDIGIT
};
typedef enum qse_ctype_t qse_ctype_t;
typedef qse_ctype_t qse_mctype_t;
typedef qse_ctype_t qse_wctype_t;

#define QSE_MCTYPE_UPPER  QSE_CTYPE_UPPER
#define QSE_MCTYPE_LOWER  QSE_CTYPE_LOWER
#define QSE_MCTYPE_ALPHA  QSE_CTYPE_ALPHA
#define QSE_MCTYPE_DIGIT  QSE_CTYPE_DIGIT
#define QSE_MCTYPE_XDIGIT QSE_CTYPE_XDIGIT
#define QSE_MCTYPE_ALNUM  QSE_CTYPE_ALNUM
#define QSE_MCTYPE_SPACE  QSE_CTYPE_SPACE
#define QSE_MCTYPE_PRINT  QSE_CTYPE_PRINT
#define QSE_MCTYPE_GRAPH  QSE_CTYPE_GRAPH
#define QSE_MCTYPE_CNTRL  QSE_CTYPE_CNTRL
#define QSE_MCTYPE_PUNCT  QSE_CTYPE_PUNCT
#define QSE_MCTYPE_BLANK  QSE_CTYPE_BLANK

#define QSE_WCTYPE_UPPER  QSE_CTYPE_UPPER
#define QSE_WCTYPE_LOWER  QSE_CTYPE_LOWER
#define QSE_WCTYPE_ALPHA  QSE_CTYPE_ALPHA
#define QSE_WCTYPE_DIGIT  QSE_CTYPE_DIGIT
#define QSE_WCTYPE_XDIGIT QSE_CTYPE_XDIGIT
#define QSE_WCTYPE_ALNUM  QSE_CTYPE_ALNUM
#define QSE_WCTYPE_SPACE  QSE_CTYPE_SPACE
#define QSE_WCTYPE_PRINT  QSE_CTYPE_PRINT
#define QSE_WCTYPE_GRAPH  QSE_CTYPE_GRAPH
#define QSE_WCTYPE_CNTRL  QSE_CTYPE_CNTRL
#define QSE_WCTYPE_PUNCT  QSE_CTYPE_PUNCT
#define QSE_WCTYPE_BLANK  QSE_CTYPE_BLANK

#define QSE_MCTYPE(name) (qse_getmctype(name))
#define QSE_ISMCTYPE(c,t) (qse_ismctype(c,t))
#define QSE_ISMALNUM(c) (qse_ismctype(c,QSE_CTYPE_ALNUM))
#define QSE_ISMALPHA(c) (qse_ismctype(c,QSE_CTYPE_ALPHA))
#define QSE_ISMBLANK(c) (qse_ismctype(c,QSE_CTYPE_BLANK))
#define QSE_ISMCNTRL(c) (qse_ismctype(c,QSE_CTYPE_CNTRL))
#define QSE_ISMDIGIT(c) (qse_ismctype(c,QSE_CTYPE_DIGIT))
#define QSE_ISMGRAPH(c) (qse_ismctype(c,QSE_CTYPE_GRAPH))
#define QSE_ISMLOWER(c) (qse_ismctype(c,QSE_CTYPE_LOWER))
#define QSE_ISMPRINT(c) (qse_ismctype(c,QSE_CTYPE_PRINT))
#define QSE_ISMPUNCT(c) (qse_ismctype(c,QSE_CTYPE_PUNCT))
#define QSE_ISMSPACE(c) (qse_ismctype(c,QSE_CTYPE_SPACE))
#define QSE_ISMUPPER(c) (qse_ismctype(c,QSE_CTYPE_UPPER))
#define QSE_ISMXDIGIT(c) (qse_ismctype(c,QSE_CTYPE_XDIGIT))
#define QSE_TOMUPPER(c) (qse_tomctype(c,QSE_CTYPE_UPPER))
#define QSE_TOMLOWER(c) (qse_tomctype(c,QSE_CTYPE_LOWER))

#define QSE_WCTYPE(name) (qse_getwctype(name))
#define QSE_ISWCTYPE(c,t) (qse_iswctype(c,t))
#define QSE_ISWALNUM(c) (qse_iswctype(c,QSE_CTYPE_ALNUM))
#define QSE_ISWALPHA(c) (qse_iswctype(c,QSE_CTYPE_ALPHA))
#define QSE_ISWBLANK(c) (qse_iswctype(c,QSE_CTYPE_BLANK))
#define QSE_ISWCNTRL(c) (qse_iswctype(c,QSE_CTYPE_CNTRL))
#define QSE_ISWDIGIT(c) (qse_iswctype(c,QSE_CTYPE_DIGIT))
#define QSE_ISWGRAPH(c) (qse_iswctype(c,QSE_CTYPE_GRAPH))
#define QSE_ISWLOWER(c) (qse_iswctype(c,QSE_CTYPE_LOWER))
#define QSE_ISWPRINT(c) (qse_iswctype(c,QSE_CTYPE_PRINT))
#define QSE_ISWPUNCT(c) (qse_iswctype(c,QSE_CTYPE_PUNCT))
#define QSE_ISWSPACE(c) (qse_iswctype(c,QSE_CTYPE_SPACE))
#define QSE_ISWUPPER(c) (qse_iswctype(c,QSE_CTYPE_UPPER))
#define QSE_ISWXDIGIT(c) (qse_iswctype(c,QSE_CTYPE_XDIGIT))
#define QSE_TOWUPPER(c) (qse_towctype(c,QSE_CTYPE_UPPER))
#define QSE_TOWLOWER(c) (qse_towctype(c,QSE_CTYPE_LOWER))

#ifdef QSE_CHAR_IS_MCHAR
#	define QSE_CTYPE(name) QSE_MCTYPE(name)
#	define QSE_ISCTYPE(c,t) QSE_ISMCTYPE(c,t)
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
#	define QSE_TOUPPER(c)  QSE_TOMUPPER(c)
#	define QSE_TOLOWER(c)  QSE_TOMLOWER(c)
#else
#	define QSE_CTYPE(name) QSE_WCTYPE(name)
#	define QSE_ISCTYPE(c,t) QSE_ISWCTYPE(c,t)
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
#	define QSE_TOUPPER(c)  QSE_TOWUPPER(c)
#	define QSE_TOLOWER(c)  QSE_TOWLOWER(c)
#endif

#define QSE_XDIGITTONUM(c) \
	(((c) >= QSE_T('0') && (c) <= QSE_T('9'))? ((c) - QSE_T('0')): \
	 ((c) >= QSE_T('A') && (c) <= QSE_T('F'))? ((c) - QSE_T('A') + 10): \
	 ((c) >= QSE_T('a') && (c) <= QSE_T('f'))? ((c) - QSE_T('a') + 10): -1)

#define QSE_MXDIGITTONUM(c) \
	(((c) >= QSE_MT('0') && (c) <= QSE_MT('9'))? ((c) - QSE_MT('0')): \
	 ((c) >= QSE_MT('A') && (c) <= QSE_MT('F'))? ((c) - QSE_MT('A') + 10): \
	 ((c) >= QSE_MT('a') && (c) <= QSE_MT('f'))? ((c) - QSE_MT('a') + 10): -1)

#define QSE_WXDIGITTONUM(c) \
	(((c) >= QSE_WT('0') && (c) <= QSE_WT('9'))? ((c) - QSE_WT('0')): \
	 ((c) >= QSE_WT('A') && (c) <= QSE_WT('F'))? ((c) - QSE_WT('A') + 10): \
	 ((c) >= QSE_WT('a') && (c) <= QSE_WT('f'))? ((c) - QSE_WT('a') + 10): -1)

#ifdef __cplusplus
extern "C" {
#endif

int qse_ismctype (
	qse_mcint_t    c,
	qse_mctype_t   type
);

int qse_iswctype (
	qse_wcint_t    c,
	qse_wctype_t   type
);

qse_mcint_t qse_tomctype (
	qse_mcint_t    c,
	qse_mctype_t   type
);

qse_wcint_t qse_towctype (
	qse_wcint_t      c,
	qse_wctype_t   type
);

int qse_mbstoctype (
	const qse_mchar_t* name,
	qse_mctype_t*    id
);

int qse_mbsntoctype (
	const qse_mchar_t* name,
	qse_size_t         len,
	qse_mctype_t*      id
);

int qse_wcstoctype (
	const qse_wchar_t* name,
	qse_wctype_t* id
);

int qse_wcsntoctype (
	const qse_wchar_t* name,
	qse_size_t         len,
	qse_wctype_t*      id
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_isctype(c,type)              qse_ismctype(c,type)
#	define qse_toctype(c,type)              qse_ismctype(c,type)
#	define qse_strtoctype(name,id)          qse_mbstoctype(name,id)
#	define qse_strntoctype(name,len,id)     qse_mbsntoctype(name,len,id)
#else
#	define qse_isctype(c,type)              qse_iswctype(c,type)
#	define qse_toctype(c,type)              qse_towctype(c,type)
#	define qse_strtoctype(name,id)          qse_wcstoctype(name,id)
#	define qse_strntoctype(name,len,id)     qse_wcsntoctype(name,len,id)
#endif

#ifdef __cplusplus
}
#endif

#endif
