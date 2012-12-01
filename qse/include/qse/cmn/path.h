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
#if defined(QSE_CHAR_IS_MCHAR)
#	define	qse_basename(path) qse_mbsbasename(path)
#else
#	define	qse_basename(path) qse_wcsbasename(path)
#endif

enum qse_canonpath_flag_t
{
	/** if the final output is . logically, return an empty path */
	QSE_CANONPATH_EMPTYSINGLEDOT  = (1 << 0),
	/** keep the .. segment in the path name */
	QSE_CANONPATH_KEEPDOUBLEDOTS  = (1 << 1),
	/** drop a trailing separator even if the source contains one */
	QSE_CANONPATH_DROPTRAILINGSEP = (1 << 2)
};


#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	define QSE_ISPATHSEP(c) ((c) == QSE_T('/') || (c) == QSE_T('\\'))
#	define QSE_ISPATHMBSEP(c) ((c) == QSE_MT('/') || (c) == QSE_MT('\\'))
#	define QSE_ISPATHWCSEP(c) ((c) == QSE_WT('/') || (c) == QSE_WT('\\'))
#else
#	define QSE_ISPATHSEP(c) ((c) == QSE_T('/'))
#	define QSE_ISPATHMBSEP(c) ((c) == QSE_MT('/'))
#	define QSE_ISPATHWCSEP(c) ((c) == QSE_WT('/'))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_mbsbasename() function returns the pointer to the file name 
 * segment in a multibyte path name.
 */
QSE_EXPORT const qse_mchar_t* qse_mbsbasename (
	const qse_mchar_t* path
);

/**
 * The qse_wcsbasename() function returns the pointer to the file name 
 * segment in a wide-character path name.
 */
QSE_EXPORT const qse_wchar_t* qse_wcsbasename (
	const qse_wchar_t* path
);

/**
 * The qse_ismbsabspath() function determines if a path name is absolute.
 * A path name beginning with a segment separator is absolute.
 * On Win32/OS2/DOS, it also returns 1 if a path name begins with a drive 
 * letter followed by a colon.
 * @return 1 if absolute, 0 if not.
 */
QSE_EXPORT int qse_ismbsabspath (
	const qse_mchar_t* path
);

/**
 * The qse_ismbsdrivepath() function determines if a path name begins with
 * a drive letter followed by a colon like A:.
 */
QSE_EXPORT int qse_ismbsdrivepath (
	const qse_mchar_t* path
);

/**
 * The qse_ismbsdrivecurpath() function determines if a path name is in the form
 * of a drive letter followed by a colon like A:, without any trailing path.
 */
QSE_EXPORT int qse_ismbsdrivecurpath (
	const qse_mchar_t* path
);

/**
 * The qse_canonmbspath() function canonicalizes a path name @a path by deleting 
 * unnecessary path segments from it and stores the result to a memory buffer 
 * pointed to by @a canon. Canonicalization is purely performed on the path
 * name without refering to actual file systems. It null-terminates the 
 * canonical path in @a canon and returns the number of characters excluding
 * the terminating null. 
 *
 * @code
 * qse_mchar_t buf[64];
 * qse_canonmbspath (QSE_MT("/usr/local/../bin/sh"), buf);
 * qse_printf (QSE_T("%hs\n")); // prints /usr/bin/sh
 * @endcode
 *
 * If #QSE_CANONPATH_EMPTYSINGLEDOT is clear in the @a flags, a single dot 
 * is produced if the input @path resolves to the current directory logically.
 * For example, dir/.. is canonicalized to a single period; If it is set, 
 * an empty string is produced. Even a single period as an input produces
 * an empty string if it is set.
 *
 * The output is empty returning 0 regardless of @a flags if the input 
 * @a path is empty.
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
QSE_EXPORT qse_size_t qse_canonmbspath (
	const qse_mchar_t* path,
	qse_mchar_t*       canon,
	int                flags
);

/**
 * The qse_iswcsabspath() function determines if a path name is absolute.
 * A path name beginning with a segment separator is absolute.
 * On Win32/OS2/DOS, it also returns 1 if a path name begins with a drive 
 * letter followed by a colon.
 * @return 1 if absolute, 0 if not.
 */
QSE_EXPORT int qse_iswcsabspath (
	const qse_wchar_t* path
);

/**
 * The qse_iswcsdrivepath() function determines if a path name begins with
 * a drive letter followed by a colon like A:.
 */
QSE_EXPORT int qse_iswcsdrivepath (
	const qse_wchar_t* path
);

/**
 * The qse_iswcsdrivecurpath() function determines if a path name is in the form
 * of a drive letter followed by a colon like A:, without any trailing path.
 */
QSE_EXPORT int qse_iswcsdrivecurpath (
	const qse_wchar_t* path
);

/**
 * The qse_canonwcspath() function canonicalizes a path name @a path by deleting 
 * unnecessary path segments from it and stores the result to a memory buffer 
 * pointed to by @a canon. Canonicalization is purely performed on the path
 * name without refering to actual file systems. It null-terminates the 
 * canonical path in @a canon and returns the number of characters excluding
 * the terminating null. 
 *
 * @code
 * qse_wchar_t buf[64];
 * qse_canonwcspath (QSE_WT("/usr/local/../bin/sh"), buf);
 * qse_printf (QSE_T("%ls\n")); // prints /usr/bin/sh
 * @endcode
 *
 * If #QSE_CANONPATH_EMPTYSINGLEDOT is clear in the @a flags, a single dot 
 * is produced if the input @path resolves to the current directory logically.
 * For example, dir/.. is canonicalized to a single period; If it is set, 
 * an empty string is produced. Even a single period as an input produces
 * an empty string if it is set.
 *
 * The output is empty returning 0 regardless of @a flags if the input 
 * @a path is empty.
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
QSE_EXPORT qse_size_t qse_canonwcspath (
	const qse_wchar_t* path,
	qse_wchar_t*       canon,
	int                flags
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_isabspath(p)      qse_ismbsabspath(p)
#	define qse_isdrivepath(p)    qse_ismbsdrivepath(p)
#	define qse_isdrivecurpath(p) qse_ismbsdrivecurpath(p)
#	define qse_canonpath(p,c,f)  qse_canonmbspath(p,c,f)
#else
#	define qse_isabspath(p)      qse_iswcsabspath(p)
#	define qse_isdrivepath(p)    qse_iswcsdrivepath(p)
#	define qse_isdrivecurpath(p) qse_iswcsdrivecurpath(p)
#	define qse_canonpath(p,c,f)  qse_canonwcspath(p,c,f)
#endif

#ifdef __cplusplus
}
#endif

#endif
