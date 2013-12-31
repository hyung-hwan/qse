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

#include <qse/cmn/str.h>

qse_size_t qse_mbsincl (qse_mchar_t* str, const qse_mchar_t* cs)
{
	qse_mchar_t* p1, * p2;

	p1 = p2 = str;
	while (*p1 != QSE_MT('\0')) 
	{
		if (qse_mbschr(cs,*p1) != QSE_NULL) *p2++ = *p1;
		p1++;
	}
	*p2 = QSE_MT('\0');
	return p2 - str;
}

qse_size_t qse_mbsxincl (
	qse_mchar_t* str, qse_size_t len, const qse_mchar_t* cs)
{
	qse_mchar_t* p1, * p2, * end;

	p1 = p2 = str;
	end = p1 + len;
	while (p1 < end) 
	{
		if (qse_mbschr(cs,*p1) != QSE_NULL) *p2++ = *p1;
		p1++;
	}
	return p2 - str;
}

qse_size_t qse_wcsincl (qse_wchar_t* str, const qse_wchar_t* cs)
{
	qse_wchar_t* p1, * p2;

	p1 = p2 = str;
	while (*p1 != QSE_WT('\0')) 
	{
		if (qse_wcschr(cs,*p1) != QSE_NULL) *p2++ = *p1;
		p1++;
	}
	*p2 = QSE_WT('\0');
	return p2 - str;
}

qse_size_t qse_wcsxincl (
	qse_wchar_t* str, qse_size_t len, const qse_wchar_t* cs)
{
	qse_wchar_t* p1, * p2, * end;

	p1 = p2 = str;
	end = p1 + len;
	while (p1 < end) 
	{
		if (qse_wcschr(cs,*p1) != QSE_NULL) *p2++ = *p1;
		p1++;
	}
	return p2 - str;
}

