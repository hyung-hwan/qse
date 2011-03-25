/*
 * $Id: chr.h 414 2011-03-25 04:52:47Z hyunghwan.chung $
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

/**
 * The qse_ccls_id_t type defines character class types.
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

#ifdef USE_STDC
#	include <ctype.h>
#	include <wctype.h>
#
#	define QSE_ISMUPPER(c) isupper(c)
#	define QSE_ISMLOWER(c) islower(c)
#	define QSE_ISMALPHA(c) isalpha(c)
#	define QSE_ISMDIGIT(c) isdigit(c)
#	define QSE_ISMXDIGIT(c) isxdigit(c)
#	define QSE_ISMALNUM(c) isalnum(c)
#	define QSE_ISMSPACE(c) isspace(c)
#	define QSE_ISMPRINT(c) isprint(c)
#	define QSE_ISMGRAPH(c) isgraph(c)
#	define QSE_ISMCNTRL(c) iscntrl(c)
#	define QSE_ISMPUNCT(c) ispunct(c)
#	define QSE_TOMUPPER(c) toupper(c)
#	define QSE_TOMLOWER(c) tolower(c)
#
#	define QSE_ISWUPPER(c) iswupper(c)
#	define QSE_ISWLOWER(c) iswlower(c)
#	define QSE_ISWALPHA(c) iswalpha(c)
#	define QSE_ISWDIGIT(c) iswdigit(c)
#	define QSE_ISWXDIGIT(c) iswxdigit(c)
#	define QSE_ISWALNUM(c) iswalnum(c)
#	define QSE_ISWSPACE(c) iswspace(c)
#	define QSE_ISWPRINT(c) iswprint(c)
#	define QSE_ISWGRAPH(c) iswgraph(c)
#	define QSE_ISWCNTRL(c) iswcntrl(c)
#	define QSE_ISWPUNCT(c) iswpunct(c)
#	define QSE_TOWUPPER(c) towupper(c)
#	define QSE_TOWLOWER(c) towlower(c)
#else
#	define QSE_ISMUPPER(c) (qse_mccls_is(c,QSE_CCLS_UPPER))
#	define QSE_ISMLOWER(c) (qse_mccls_is(c,QSE_CCLS_LOWER))
#	define QSE_ISMALPHA(c) (qse_mccls_is(c,QSE_CCLS_ALPHA))
#	define QSE_ISMDIGIT(c) (qse_mccls_is(c,QSE_CCLS_DIGIT))
#	define QSE_ISMXDIGIT(c) (qse_mccls_is(c,QSE_CCLS_XDIGIT))
#	define QSE_ISMALNUM(c) (qse_mccls_is(c,QSE_CCLS_ALNUM))
#	define QSE_ISMSPACE(c) (qse_mccls_is(c,QSE_CCLS_SPACE))
#	define QSE_ISMPRINT(c) (qse_mccls_is(c,QSE_CCLS_PRINT))
#	define QSE_ISMGRAPH(c) (qse_mccls_is(c,QSE_CCLS_GRAPH))
#	define QSE_ISMCNTRL(c) (qse_mccls_is(c,QSE_CCLS_CNTRL))
#	define QSE_ISMPUNCT(c) (qse_mccls_is(c,QSE_CCLS_PUNCT))
#	define QSE_TOMUPPER(c) (qse_mccls_to(c,QSE_CCLS_UPPER))
#	define QSE_TOMLOWER(c) (qse_mccls_to(c,QSE_CCLS_LOWER))
#
#	define QSE_ISWUPPER(c) (qse_wccls_is(c,QSE_CCLS_UPPER))
#	define QSE_ISWLOWER(c) (qse_wccls_is(c,QSE_CCLS_LOWER))
#	define QSE_ISWALPHA(c) (qse_wccls_is(c,QSE_CCLS_ALPHA))
#	define QSE_ISWDIGIT(c) (qse_wccls_is(c,QSE_CCLS_DIGIT))
#	define QSE_ISWXDIGIT(c) (qse_wccls_is(c,QSE_CCLS_XDIGIT))
#	define QSE_ISWALNUM(c) (qse_wccls_is(c,QSE_CCLS_ALNUM))
#	define QSE_ISWSPACE(c) (qse_wccls_is(c,QSE_CCLS_SPACE))
#	define QSE_ISWPRINT(c) (qse_wccls_is(c,QSE_CCLS_PRINT))
#	define QSE_ISWGRAPH(c) (qse_wccls_is(c,QSE_CCLS_GRAPH))
#	define QSE_ISWCNTRL(c) (qse_wccls_is(c,QSE_CCLS_CNTRL))
#	define QSE_ISWPUNCT(c) (qse_wccls_is(c,QSE_CCLS_PUNCT))
#	define QSE_TOWUPPER(c) (qse_wccls_to(c,QSE_CCLS_UPPER))
#	define QSE_TOWLOWER(c) (qse_wccls_to(c,QSE_CCLS_LOWER))
#endif

#ifdef QSE_CHAR_IS_MCHAR
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
#	define QSE_TOUPPER(c)  QSE_TOMUPPER(c)
#	define QSE_TOLOWER(c)  QSE_TOMLOWER(c)
#else
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
#	define QSE_TOUPPER(c)  QSE_TOWUPPER(c)
#	define QSE_TOLOWER(c)  QSE_TOWLOWER(c)
#endif

#ifdef __cplusplus
extern "C" {
#endif

qse_bool_t qse_mccls_is (
	qse_mcint_t      c,
	qse_mccls_id_t   type
);

qse_bool_t qse_wccls_is (
	qse_wcint_t      c,
	qse_wccls_id_t   type
);

qse_mcint_t qse_mccls_to (
	qse_mcint_t      c,
	qse_mccls_id_t    type
);

qse_wcint_t qse_wccls_to (
	qse_wcint_t      c,
	qse_wccls_id_t    type
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_ccls_is(c,type) qse_mccls_is(c,type);
#	define qse_ccls_to(c,type) qse_mccls_to(c,type);
#else
#	define qse_ccls_is(c,type) qse_wccls_is(c,type);
#	define qse_ccls_to(c,type) qse_wccls_to(c,type);
#endif

/**
 * The qse_mblen() function scans a multibyte sequence to get the number of 
 * bytes needed to form a wide character. It does not scan more than @a mblen
 * bytes.
 * @return number of bytes processed on success, 
 *         0 for invalid sequences, 
 *         mblen + 1 for incomplete sequences
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
 */
qse_size_t qse_wctomb (
	qse_wchar_t        wc,
	qse_mchar_t*       mb,
	qse_size_t         mblen
);

#ifdef __cplusplus
}
#endif

#endif
