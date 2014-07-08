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

#ifndef _QSE_CMN_GLOB_H_
#define _QSE_CMN_GLOB_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides functions, types, macros for wildcard expansion
 * in a path name.
 */

typedef int (*qse_glob_cbimpl_t) (
	const qse_cstr_t* path,
	void*             cbctx
);

enum qse_glob_flags_t
{
	/** Don't use the backslash as an escape charcter.
	 *  This option is on in Win32/OS2/DOS. */
	QSE_GLOB_NOESCAPE   = (1 << 0),

	/** Match a leading period explicitly by a literal period in the pattern */
	QSE_GLOB_PERIOD     = (1 << 1),

	/** Perform case-insensitive matching. 
	 *  This option is always on in Win32/OS2/DOS. */
	QSE_GLOB_IGNORECASE = (1 << 2),

	/** Make the function to be more fault-resistent */
	QSE_GLOB_TOLERANT   = (1 << 3)
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_glob() function finds path names matchin the @a pattern.
 * It calls the call-back function @a cbimpl for each path name found.
 * 
 * @return -1 on failure, 0 on no match, 1 if matches are found.
 */
QSE_EXPORT int qse_glob (
	const qse_char_t*  pattern,
	qse_glob_cbimpl_t  cbimpl,
	void*              cbctx,
	int                flags,
	qse_mmgr_t*        mmgr
);

#ifdef __cplusplus
}
#endif

#endif
