/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_CMN_PATH_H_
#define _QSE_CMN_PATH_H_

/** \file
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

#if defined(__cplusplus)
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
 * \return 1 if absolute, 0 if not.
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
 * The qse_ismbsdriveabspath() function determines if a path name is in the form
 * of a drive letter followed by a colon like A: and a path separator.
 */
QSE_EXPORT int qse_ismbsdriveabspath (
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
 * The qse_getmbspathroot() function returns the core part of \a path 
 * excluding a special prefix.
 */
QSE_EXPORT qse_mchar_t* qse_getmbspathcore (
	const qse_mchar_t* path
);

/**
 * The qse_canonmbspath() function canonicalizes a path name \a path by deleting 
 * unnecessary path segments from it and stores the result to a memory buffer 
 * pointed to by \a canon. Canonicalization is purely performed on the path
 * name without refering to actual file systems. It null-terminates the 
 * canonical path in \a canon and returns the number of characters excluding
 * the terminating null. 
 *
 * \code
 * qse_mchar_t buf[64];
 * qse_canonmbspath (QSE_MT("/usr/local/../bin/sh"), buf);
 * qse_printf (QSE_T("%hs\n")); // prints /usr/bin/sh
 * \endcode
 *
 * If #QSE_CANONPATH_EMPTYSINGLEDOT is clear in the \a flags, a single dot 
 * is produced if the input \a path resolves to the current directory logically.
 * For example, dir/.. is canonicalized to a single period; If it is set, 
 * an empty string is produced. Even a single period as an input produces
 * an empty string if it is set.
 *
 * The output is empty returning 0 regardless of \a flags if the input 
 * \a path is empty.
 * 
 * The caller must ensure that it is large enough to hold the resulting 
 * canonical path before calling because this function does not check the
 * size of the memory buffer. Since the canonical path cannot be larger 
 * than the original path, you can simply ensure this by providing a memory
 * buffer as long as the number of characters and a terminating null in 
 * the original path.
 *
 * \return number of characters in the resulting canonical path excluding
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
 * \return 1 if absolute, 0 if not.
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
 * The qse_iswcsdriveabspath() function determines if a path name is in the form
 * of a drive letter followed by a colon like A: and a path separtor.
 */
QSE_EXPORT int qse_iswcsdriveabspath (
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
 * The qse_getwcspathroot() function returns the core part of \a path 
 * excluding a special prefix.
 */
QSE_EXPORT qse_wchar_t* qse_getwcspathcore (
	const qse_wchar_t* path
);

/**
 * The qse_canonwcspath() function canonicalizes a path name \a path by deleting 
 * unnecessary path segments from it and stores the result to a memory buffer 
 * pointed to by \a canon. Canonicalization is purely performed on the path
 * name without refering to actual file systems. It null-terminates the 
 * canonical path in \a canon and returns the number of characters excluding
 * the terminating null. 
 *
 * \code
 * qse_wchar_t buf[64];
 * qse_canonwcspath (QSE_WT("/usr/local/../bin/sh"), buf);
 * qse_printf (QSE_T("%ls\n")); // prints /usr/bin/sh
 * \endcode
 *
 * If #QSE_CANONPATH_EMPTYSINGLEDOT is clear in the \a flags, a single dot 
 * is produced if the input \a path resolves to the current directory logically.
 * For example, dir/.. is canonicalized to a single period; If it is set, 
 * an empty string is produced. Even a single period as an input produces
 * an empty string if it is set.
 *
 * The output is empty returning 0 regardless of \a flags if the input 
 * \a path is empty.
 * 
 * The caller must ensure that it is large enough to hold the resulting 
 * canonical path before calling because this function does not check the
 * size of the memory buffer. Since the canonical path cannot be larger 
 * than the original path, you can simply ensure this by providing a memory
 * buffer as long as the number of characters and a terminating null in 
 * the original path.
 *
 * \return number of characters in the resulting canonical path excluding
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
#	define qse_isdriveabspath(p) qse_ismbsdriveabspath(p)
#	define qse_isdrivecurpath(p) qse_ismbsdrivecurpath(p)
#	define qse_getpathcore(p)    qse_getmbspathcore(p)
#	define qse_canonpath(p,c,f)  qse_canonmbspath(p,c,f)
#else
#	define qse_isabspath(p)      qse_iswcsabspath(p)
#	define qse_isdrivepath(p)    qse_iswcsdrivepath(p)
#	define qse_isdriveabspath(p) qse_iswcsdriveabspath(p)
#	define qse_isdrivecurpath(p) qse_iswcsdrivecurpath(p)
#	define qse_getpathcore(p)    qse_getwcspathcore(p)
#	define qse_canonpath(p,c,f)  qse_canonwcspath(p,c,f)
#endif

#if defined(__cplusplus)
}
#endif

#endif
