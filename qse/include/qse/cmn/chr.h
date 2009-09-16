/*
 * $Id: chr.h 287 2009-09-15 10:01:02Z hyunghwan.chung $
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

#ifndef _QSE_CMN_CHR_H_
#define _QSE_CMN_CHR_H_

#include <qse/types.h>
#include <qse/macros.h>

/****t* Common/qse_ccls_id_t
 * NAME
 *  qse_ccls_id_t - define character class types
 * SYNOPSIS
 */
enum qse_ccls_id_t
{
        QSE_CCLS_UPPER,
        QSE_CCLS_LOWER,
        QSE_CCLS_ALPHA,
        QSE_CCLS_DIGIT,
        QSE_CCLS_XDIGIT,
        QSE_CCLS_ALNUM,
        QSE_CCLS_SPACE,
        QSE_CCLS_PRINT,
        QSE_CCLS_GRAPH,
        QSE_CCLS_CNTRL,
        QSE_CCLS_PUNCT
};
typedef enum qse_ccls_id_t qse_ccls_id_t;
/******/

#ifdef USE_STDC
#	if defined(QSE_CHAR_IS_MCHAR)
#		include <ctype.h>
#		define QSE_ISUPPER(c) isupper(c)
#		define QSE_ISLOWER(c) islower(c)
#		define QSE_ISALPHA(c) isalpha(c)
#		define QSE_ISDIGIT(c) isdigit(c)
#		define QSE_ISXDIGIT(c) isxdigit(c)
#		define QSE_ISALNUM(c) isalnum(c)
#		define QSE_ISSPACE(c) isspace(c)
#		define QSE_ISPRINT(c) isprint(c)
#		define QSE_ISGRAPH(c) isgraph(c)
#		define QSE_ISCNTRL(c) iscntrl(c)
#		define QSE_ISPUNCT(c) ispunct(c)
#		define QSE_TOUPPER(c) toupper(c)
#		define QSE_TOLOWER(c) tolower(c)
#	elif defined(QSE_CHAR_IS_WCHAR)
#		include <wctype.h>
#		define QSE_ISUPPER(c) iswupper(c)
#		define QSE_ISLOWER(c) iswlower(c)
#		define QSE_ISALPHA(c) iswalpha(c)
#		define QSE_ISDIGIT(c) iswdigit(c)
#		define QSE_ISXDIGIT(c) iswxdigit(c)
#		define QSE_ISALNUM(c) iswalnum(c)
#		define QSE_ISSPACE(c) iswspace(c)
#		define QSE_ISPRINT(c) iswprint(c)
#		define QSE_ISGRAPH(c) iswgraph(c)
#		define QSE_ISCNTRL(c) iswcntrl(c)
#		define QSE_ISPUNCT(c) iswpunct(c)
#		define QSE_TOUPPER(c) towupper(c)
#		define QSE_TOLOWER(c) towlower(c)
#	else
#		error Unsupported character type
#	endif
#else
#	define QSE_ISUPPER(c) (qse_ccls_is(c,QSE_CCLS_UPPER))
#	define QSE_ISLOWER(c) (qse_ccls_is(c,QSE_CCLS_LOWER))
#	define QSE_ISALPHA(c) (qse_ccls_is(c,QSE_CCLS_ALPHA))
#	define QSE_ISDIGIT(c) (qse_ccls_is(c,QSE_CCLS_DIGIT))
#	define QSE_ISXDIGIT(c) (qse_ccls_is(c,QSE_CCLS_XDIGIT))
#	define QSE_ISALNUM(c) (qse_ccls_is(c,QSE_CCLS_ALNUM))
#	define QSE_ISSPACE(c) (qse_ccls_is(c,QSE_CCLS_SPACE))
#	define QSE_ISPRINT(c) (qse_ccls_is(c,QSE_CCLS_PRINT))
#	define QSE_ISGRAPH(c) (qse_ccls_is(c,QSE_CCLS_GRAPH))
#	define QSE_ISCNTRL(c) (qse_ccls_is(c,QSE_CCLS_CNTRL))
#	define QSE_ISPUNCT(c) (qse_ccls_is(c,QSE_CCLS_PUNCT))
#	define QSE_TOUPPER(c) (qse_ccls_to(c,QSE_CCLS_UPPER))
#	define QSE_TOLOWER(c) (qse_ccls_to(c,QSE_CCLS_LOWER))
#endif

#ifdef __cplusplus
extern "C" {
#endif

qse_bool_t qse_ccls_is (
	qse_cint_t      c,
	qse_ccls_id_t type
);

qse_cint_t qse_ccls_to (
	qse_cint_t      c,
	qse_ccls_id_t type
);

qse_size_t qse_mblen (
	const qse_mchar_t* mb,
	qse_size_t mblen
);

/****f* Common/qse_mbtowc
 * NAME
 *  qse_mbtowc - convert a multibyte sequence to a wide character.
 * RETURN
 *  The qse_mbtowc() function returns 0 if an invalid multibyte sequence is
 *  detected, mblen + 1 if the sequence is incomplete. It returns the number
 *  of bytes processed to form a wide character.
 * SYNOPSIS
 */
qse_size_t qse_mbtowc (
	const qse_mchar_t* mb,
	qse_size_t         mblen,
	qse_wchar_t*       wc
);
/******/

/****f* Common/qse_wctomb
 * NAME
 *  qse_wctomb - convert a wide character to a multibyte sequence
 * RETURN
 *  The qse_wctomb() functions returns 0 if the wide character is illegal, 
 *  mblen + 1 if mblen is not large enough to hold the multibyte sequence.
 *  On successful conversion, it returns the number of bytes in the sequence.
 * SYNOPSIS
 */
qse_size_t qse_wctomb (
	qse_wchar_t        wc,
	qse_mchar_t*       mb,
	qse_size_t         mblen
);
/******/

#ifdef __cplusplus
}
#endif

#endif
