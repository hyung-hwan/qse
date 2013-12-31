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

qse_size_t qse_mbsrev (qse_mchar_t* str)
{
	return qse_mbsxrev (str, qse_mbslen(str));
}

qse_size_t qse_mbsxrev (qse_mchar_t* str, qse_size_t len)
{
	qse_mchar_t ch;
	qse_mchar_t* start = str;
	qse_mchar_t* end = str + len - 1;

	while (start < end) 
	{
		ch = *start;
		*start++ = *end; 
		*end-- = ch;
	}

	return len;
}

qse_size_t qse_wcsrev (qse_wchar_t* str)
{
	return qse_wcsxrev (str, qse_wcslen(str));
}

qse_size_t qse_wcsxrev (qse_wchar_t* str, qse_size_t len)
{
	qse_wchar_t ch;
	qse_wchar_t* start = str;
	qse_wchar_t* end = str + len - 1;

	while (start < end) 
	{
		ch = *start;
		*start++ = *end; 
		*end-- = ch;
	}

	return len;
}

