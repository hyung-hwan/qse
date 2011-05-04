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

#ifndef _QSE_CMN_MISC_H_
#define _QSE_CMN_MISC_H_

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

#ifdef __cplusplus
}
#endif

#endif
