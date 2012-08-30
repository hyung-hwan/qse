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

#include <qse/cmn/path.h>

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	define IS_MSEP(c) ((c) == QSE_MT('/') || (c) == QSE_MT('\\'))
#	define IS_WSEP(c) ((c) == QSE_WT('/') || (c) == QSE_WT('\\'))
#else
#	define IS_MSEP(c) ((c) == QSE_MT('/'))
#	define IS_WSEP(c) ((c) == QSE_WT('/'))
#endif

const qse_mchar_t* qse_mbsbasename (const qse_mchar_t* path)
{
	const qse_mchar_t* p, * last = QSE_NULL;

	for (p = path; *p != QSE_MT('\0'); p++)
	{
		if (IS_MSEP(*p)) last = p;
	}

	return (last == QSE_NULL)? path: (last + 1);
}

const qse_wchar_t* qse_wcsbasename (const qse_wchar_t* path)
{
	const qse_wchar_t* p, * last = QSE_NULL;

	for (p = path; *p != QSE_WT('\0'); p++)
	{
		if (IS_WSEP(*p)) last = p;
	}

	return (last == QSE_NULL)? path: (last + 1);
}
