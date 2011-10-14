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

#ifndef _QSE_CMN_PATH_H_
#define _QSE_CMN_PATH_H_

/** @file
 * This file provides functions for simple path name manipulation.
 */

#include <qse/types.h>
#include <qse/macros.h>

#ifdef QSE_CHAR_IS_MCHAR
#	define	qse_basename(path) qse_mbsbasename(path)
#else
#	define	qse_basename(path) qse_wcsbasename(path)
#endif

#ifdef __cplusplus
extern "C" {
#endif

const qse_mchar_t* qse_mbsbasename (
	const qse_mchar_t* path
);

const qse_wchar_t* qse_wcsbasename (
	const qse_wchar_t* path
);

/*
 * The qse_canonpath() function deletes unnecessary path segments
 * from a path name 'path' and stores it to a memory buffer pointed
 * to by 'canon'. It null-terminates the canonical path in 'canon' and
 * returns the number of characters excluding the terminating null.
 * The caller must ensure that it is large enough before calling this
 * because this function does not check the size of the memory buffer.
 * Since the canonical path cannot be larger than the original path,
 * you can simply ensure this by providing a memory buffer as long as
 * the number of characters and a terminating null in the original path.
 *
 * @return the number of characters in the resulting canonical path.
 */
qse_size_t qse_canonpath (
	const qse_char_t* path,
	qse_char_t*       canon
);

#ifdef __cplusplus
}
#endif

#endif
