/*
 * $Id: str-len.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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

#include <qse/cmn/str.h>

qse_size_t qse_mbslen (const qse_mchar_t* mbs)
{
	const qse_mchar_t* p = mbs;
	while (*p != QSE_MT('\0')) p++;
	return p - mbs;
}

qse_size_t qse_wcslen (const qse_wchar_t* wcs)
{
	const qse_wchar_t* p = wcs;
	while (*p != QSE_WT('\0')) p++;
	return p - wcs;
}

qse_size_t qse_mbsbytes (const qse_mchar_t* str)
{
	const qse_mchar_t* p = str;
	while (*p != QSE_MT('\0')) p++;
	return (p - str) * QSE_SIZEOF(qse_mchar_t);
}

qse_size_t qse_wcsbytes (const qse_wchar_t* str)
{
	const qse_wchar_t* p = str;
	while (*p != QSE_WT('\0')) p++;
	return (p - str) * QSE_SIZEOF(qse_wchar_t);
}
