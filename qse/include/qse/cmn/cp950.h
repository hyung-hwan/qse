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

#ifndef _QSE_CMN_CP950_H_
#define _QSE_CMN_CP950_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides functions, types, macros for cp950 conversion.
 */

/**
 * The QSE_CP950LEN_MAX macro defines the maximum number of bytes
 * needed to form a single unicode character.
 */
#define QSE_CP950LEN_MAX 2

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * The qse_uctocp950() function converts a unicode character to a cp950 sequence.
 * @return 
 * - 0 is returned if @a uc is invalid. 
 * - An integer greater than @a size is returned if the @a cp950 sequence buffer 
 *   is not #QSE_NULL and not large enough. This integer is actually the number 
 *   of bytes needed.
 * - If @a cp950 is #QSE_NULL, the number of bytes that would have been stored 
 *   into @a cp950 if it had not been #QSE_NULL is returned.
 * - An integer between 1 and size inclusive is returned in all other cases.
 * @note
 * This function doesn't check invalid unicode code points and performs
 * conversion compuationally.
 */
qse_size_t qse_uctocp950 (
	qse_wchar_t  uc,
	qse_mchar_t* cp950,
	qse_size_t   size
);

/**
 * The qse_cp950touc() function converts a cp950 sequence to a unicode character.
 * @return
 * - 0 is returned if the @a cp950 sequence is invalid.
 * - An integer greater than @a size is returned if the @a cp950 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */
qse_size_t qse_cp950touc (
	const qse_mchar_t* cp950,
	qse_size_t         size, 
	qse_wchar_t*       uc
);

/**
 * The qse_cp950lenmax() function scans at most @a size bytes from the @a cp950 
 * sequence and returns the number of bytes needed to form a single unicode
 * character.
 * @return
 * - 0 is returned if the @a cp950 sequence is invalid.
 * - An integer greater than @a size is returned if the @a cp950 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */ 
qse_size_t qse_cp950len (
	const qse_mchar_t* cp950,
	qse_size_t         size
);

/**
 * The qse_cp950lenmax() function returns the maximum number of bytes needed 
 * to form a single unicode character. Use #QSE_CP950LEN_MAX if you need a 
 * compile-time constant.
 */
qse_size_t qse_cp950lenmax (
	void
);

#ifdef __cplusplus
}
#endif

#endif