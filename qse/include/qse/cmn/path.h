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
 * This file provides functions for path name manipulation.
 */

#include <qse/types.h>
#include <qse/macros.h>

/**
 * The qse_basename() macro returns the pointer to the file name 
 * segment in a path name. It maps to qse_mbsbasename() if #QSE_CHAR_IS_MCHAR
 * is defined; it maps to qse_wcsbasename() if #QSE_CHAR_IS_WCHAR is defined.
 */
#ifdef QSE_CHAR_IS_MCHAR
#	define	qse_basename(path) qse_mbsbasename(path)
#else
#	define	qse_basename(path) qse_wcsbasename(path)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_mbsbasename() function returns the pointer to the file name 
 * segment in a multibyte path name.
 */
const qse_mchar_t* qse_mbsbasename (
	const qse_mchar_t* path
);

/**
 * The qse_wcsbasename() function returns the pointer to the file name 
 * segment in a wide-character path name.
 */
const qse_wchar_t* qse_wcsbasename (
	const qse_wchar_t* path
);

/**
 * The qse_isabspath() function determines if a path name is absolute.
 * A path name beginning with a segment separator is absolute.
 * On Win32/OS2/DOS, it also returns 1 if a path name begins with a drive 
 * letter followed by a colon.
 * @return 1 if absolute, 0 if not.
 */
int qse_isabspath (
	const qse_char_t* path
);

/**
 * The qse_canonpath() function canonicalizes a path name @a path by deleting 
 * unnecessary path segments from it and stores the result to a memory buffer 
 * pointed to by @a canon. Canonicalization is purely performed on the path
 * name without refering to actual file systems. It null-terminates the 
 * canonical path in @a canon and returns the number of characters excluding
 * the terminating null. 
 *
 * @code
 * qse_char_t buf[64];
 * qse_canonpath ("/usr/local/../bin/sh", buf);
 * qse_printf (QSE_T("%s\n")); // prints /usr/bin/sh
 * @endcode
 *
 * The caller must ensure that it is large enough to hold the resulting 
 * canonical path before calling because this function does not check the
 * size of the memory buffer. Since the canonical path cannot be larger 
 * than the original path, you can simply ensure this by providing a memory
 * buffer as long as the number of characters and a terminating null in 
 * the original path.
 *
 * @return number of characters in the resulting canonical path excluding
 *         the terminating null.
 */
qse_size_t qse_canonpath (
	const qse_char_t* path,
	qse_char_t*       canon
);

#ifdef __cplusplus
}
#endif

#endif
