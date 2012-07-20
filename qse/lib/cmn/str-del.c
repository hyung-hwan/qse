/*
 * $Id: str-del.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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

qse_size_t qse_mbsdel (qse_mchar_t* str, qse_size_t pos, qse_size_t n)
{
	qse_mchar_t* p = str, * q = str;
	qse_size_t idx = 0;

	while (*p != QSE_MT('\0')) 
	{
		if (idx < pos || idx >= pos + n) *q++ = *p;
		idx++; p++;
	}

	*q = QSE_MT('\0');
	return q - str;
}

qse_size_t qse_mbsxdel (	
	qse_mchar_t* str, qse_size_t len, qse_size_t pos, qse_size_t n)
{
	qse_mchar_t* p = str, * q = str, * end = str + len;
	qse_size_t idx = 0;

	while (p < end) 
	{
		if (idx < pos || idx >= pos + n) *q++ = *p;
		idx++; p++;
	}

	return q - str;
}

qse_size_t qse_wcsdel (qse_wchar_t* str, qse_size_t pos, qse_size_t n)
{
	qse_wchar_t* p = str, * q = str;
	qse_size_t idx = 0;

	while (*p != QSE_WT('\0')) 
	{
		if (idx < pos || idx >= pos + n) *q++ = *p;
		idx++; p++;
	}

	*q = QSE_WT('\0');
	return q - str;
}

qse_size_t qse_wcsxdel (	
	qse_wchar_t* str, qse_size_t len, qse_size_t pos, qse_size_t n)
{
	qse_wchar_t* p = str, * q = str, * end = str + len;
	qse_size_t idx = 0;

	while (p < end) 
	{
		if (idx < pos || idx >= pos + n) *q++ = *p;
		idx++; p++;
	}

	return q - str;
}
