/*
 * $Id$
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

#ifndef _QSE_CMN_UTF8_H_
#define _QSE_CMN_UTF8_H_

#include <qse/types.h>
#include <qse/macros.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * The qse_uctoutf8len() function returns the number bytes in the utf8 sequence
 * that would result from the original unicode character.
 * @return 
 * - 0 is returned if @a uc is invalid. 
 * - A positive integer is returned in all other cases.
 */
int qse_uctoutf8len (
	qse_wchar_t uc
);

/** 
 * The qse_uctoutf8() function converts a unicode character to a utf8 sequence.
 * @return 
 * - 0 is returned if @a uc is invalid. 
 * - A negative integer is returned if the utf8 sequence buffer is not 
 *   large enough. It is the negated buffer size required.
 * - A positive integer is returned in all other cases.
 */
int qse_uctoutf8 (
	qse_wchar_t  uc,
	qse_mchar_t* utf8,
	int          size
);

int qse_utf8touc (
	const qse_mchar_t* utf8,
	int                size, 
	qse_wchar_t*       uc
);

#ifdef __cplusplus
}
#endif

#endif
