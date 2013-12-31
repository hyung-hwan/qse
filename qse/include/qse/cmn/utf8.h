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

#ifndef _QSE_CMN_UTF8_H_
#define _QSE_CMN_UTF8_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides functions, types, macros for utf8 conversion.
 */

/**
 * The QSE_UTF8LEN_MAX macro defines the maximum number of bytes
 * needed to form a single unicode character.
 */
#if QSE_SIZEOF_WCHAR_T == 2
#	define QSE_UTF8LEN_MAX 3
#elif QSE_SIZEOF_WCHAR_T == 4
#	define QSE_UTF8LEN_MAX 6
#else
#	error Unsupported wide-character size
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * The qse_uctoutf8() function converts a unicode character to a utf8 sequence.
 * @return 
 * - 0 is returned if @a uc is invalid. 
 * - An integer greater than @a size is returned if the @a utf8 sequence buffer 
 *   is not #QSE_NULL and not large enough. This integer is actually the number 
 *   of bytes needed.
 * - If @a utf8 is #QSE_NULL, the number of bytes that would have been stored 
 *   into @a utf8 if it had not been #QSE_NULL is returned.
 * - An integer between 1 and size inclusive is returned in all other cases.
 * @note
 * This function doesn't check invalid unicode code points and performs
 * conversion compuationally.
 */
qse_size_t qse_uctoutf8 (
	qse_wchar_t  uc,
	qse_mchar_t* utf8,
	qse_size_t   size
);

/**
 * The qse_utf8touc() function converts a utf8 sequence to a unicode character.
 * @return
 * - 0 is returned if the @a utf8 sequence is invalid.
 * - An integer greater than @a size is returned if the @a utf8 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */
qse_size_t qse_utf8touc (
	const qse_mchar_t* utf8,
	qse_size_t         size, 
	qse_wchar_t*       uc
);

/**
 * The qse_utf8lenmax() function scans at most @a size bytes from the @a utf8 
 * sequence and returns the number of bytes needed to form a single unicode
 * character.
 * @return
 * - 0 is returned if the @a utf8 sequence is invalid.
 * - An integer greater than @a size is returned if the @a utf8 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */ 
qse_size_t qse_utf8len (
	const qse_mchar_t* utf8,
	qse_size_t         size
);

/**
 * The qse_utf8lenmax() function returns the maximum number of bytes needed 
 * to form a single unicode character. Use #QSE_UTF8LEN_MAX if you need a 
 * compile-time constant.
 */
qse_size_t qse_utf8lenmax (
	void
);

#ifdef __cplusplus
}
#endif

#endif
