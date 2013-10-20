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

#ifndef _QSE_LIB_CMN_FMT_H_
#define _QSE_LIB_CMN_FMT_H_

#include <qse/cmn/fmt.h>
#include <stdarg.h>

#ifdef __cplusplus
extern {
#endif

qse_ssize_t qse_mxprintf (
	const qse_mchar_t* fmt,
	int              (*put_mchar) (qse_mchar_t, void*),
	int              (*put_wchar) (qse_wchar_t, void*),
	void*              arg,
	va_list            ap
);

qse_ssize_t qse_wxprintf (
	const qse_wchar_t* fmt,
	int              (*put_wchar) (qse_wchar_t, void*),
	int              (*put_mchar) (qse_mchar_t, void*),
	void*              arg,
	va_list            ap
);

#ifdef __cplusplus
}
#endif

#endif
