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

#ifndef _QSE_CMN_MB8_H_
#define _QSE_CMN_MB8_H_

#include <qse/types.h>
#include <qse/macros.h>

/** \file
 * This file provides functions, types, macros for mb8 conversion.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/** 
 * The qse_wctomb8() function converts a wide character to a mb8 sequence.
 * \return 
 * - 0 is returned if \a wc is invalid. 
 * - An integer greater than \a size is returned if the \a mb8 sequence buffer 
 *   is not #QSE_NULL and not large enough. This integer is actually the number 
 *   of bytes needed.
 * - If \a mb8 is #QSE_NULL, the number of bytes that would have been stored 
 *   into \a mb8 if it had not been #QSE_NULL is returned.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */
qse_size_t qse_wctomb8 (
	qse_wchar_t  wc,
	qse_mchar_t* mb8,
	qse_size_t   size
);

/**
 * The qse_mb8towc() function converts a mb8 sequence to a wide character.
 * \return
 * - 0 is returned if the \a mb8 sequence is invalid.
 * - An integer greater than \a size is returned if the \a mb8 sequence is 
 *   not complete.
 * - An integer between 1 and size inclusive is returned in all other cases.
 */
qse_size_t qse_mb8towc (
	const qse_mchar_t* mb8,
	qse_size_t         size, 
	qse_wchar_t*       wc
);

#if defined(__cplusplus)
}
#endif

#endif
