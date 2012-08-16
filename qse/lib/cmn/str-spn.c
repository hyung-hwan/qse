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

#include <qse/cmn/str.h>

qse_size_t qse_mbsspn (const qse_mchar_t* str1, const qse_mchar_t* str2)
{
	const qse_mchar_t* p1, * p2;
	qse_size_t n = 0;

	for (p1 = str1; *p1 != QSE_MT('\0'); p1++)
	{
		for (p2 = str2; *p2 != QSE_MT('\0'); p2++)
		{
			if (*p2 == *p1) goto matched;
		}

		/* didn't match anything  */
		break;

	matched:
		n++;
	}

	return n;
}

qse_size_t qse_wcsspn (const qse_wchar_t* str1, const qse_wchar_t* str2)
{
	const qse_wchar_t* p1, * p2;
	qse_size_t n = 0;

	for (p1 = str1; *p1 != QSE_WT('\0'); p1++)
	{
		for (p2 = str2; *p2 != QSE_WT('\0'); p2++)
		{
			if (*p2 == *p1) goto matched;
		}

		/* didn't match anything  */
		break;

	matched:
		n++;
	}

	return n;
}

qse_size_t qse_mbscspn (const qse_mchar_t* str1, const qse_mchar_t* str2)
{
	const qse_mchar_t* p1, * p2;
	qse_size_t n = 0;

	for (p1 = str1; *p1 != QSE_WT('\0'); p1++)
	{
		for (p2 = str2; *p2 != QSE_WT('\0'); p2++)
		{
			if (*p2 == *p1) goto done;
		}

		/* didn't match anything. increment the length */
		n++;
	}

done:
	return n;
}

qse_size_t qse_wcscspn (const qse_wchar_t* str1, const qse_wchar_t* str2)
{
	const qse_wchar_t* p1, * p2;
	qse_size_t n = 0;

	for (p1 = str1; *p1 != QSE_WT('\0'); p1++)
	{
		for (p2 = str2; *p2 != QSE_WT('\0'); p2++)
		{
			if (*p2 == *p1) goto done;
		}

		/* didn't match anything. increment the length */
		n++;
	}

done:
	return n;
}
