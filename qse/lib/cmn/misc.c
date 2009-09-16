/*
 * $Id: misc.c 287 2009-09-15 10:01:02Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/cmn/misc.h>

const qse_char_t* qse_basename (const qse_char_t* path)
{
	const qse_char_t* p, * last = QSE_NULL;

	for (p = path; *p != QSE_T('\0'); p++)
	{
		if (*p == QSE_T('/')) last = p;
	#ifdef _WIN32
		else if (*p == QSE_T('\\')) last = p;
	#endif
	}

	return (last == QSE_NULL)? path: (last + 1);
}
