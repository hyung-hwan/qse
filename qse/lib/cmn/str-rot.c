/*
 * $Id: str-rot.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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


qse_size_t qse_mbsrot (qse_mchar_t* str, int dir, qse_size_t n)
{
	return qse_mbsxrot (str, qse_mbslen(str), dir, n);
}

qse_size_t qse_mbsxrot (qse_mchar_t* str, qse_size_t len, int dir, qse_size_t n)
{
	qse_size_t first, last, count, index, nk;
	qse_mchar_t c;

	if (dir == 0 || len == 0) return len;
	if ((n %= len) == 0) return len;

	if (dir > 0) n = len - n;
	first = 0; nk = len - n; count = 0;

	while (count < n) 
	{
		last = first + nk;
		index = first;
		c = str[first];
		do
		{
			count++;
			while (index < nk) 
			{
				str[index] = str[index + n];
				index += n;
			}
			if (index == last) break;
			str[index] = str[index - nk];
			index -= nk;
		}
		while (1);
		str[last] = c; first++;
	}
	return len;
}

qse_size_t qse_wcsrot (qse_wchar_t* str, int dir, qse_size_t n)
{
	return qse_wcsxrot (str, qse_wcslen(str), dir, n);
}

qse_size_t qse_wcsxrot (qse_wchar_t* str, qse_size_t len, int dir, qse_size_t n)
{
	qse_size_t first, last, count, index, nk;
	qse_wchar_t c;

	if (dir == 0 || len == 0) return len;
	if ((n %= len) == 0) return len;

	if (dir > 0) n = len - n;
	first = 0; nk = len - n; count = 0;

	while (count < n) 
	{
		last = first + nk;
		index = first;
		c = str[first];
		do
		{
			count++;
			while (index < nk) 
			{
				str[index] = str[index + n];
				index += n;
			}
			if (index == last) break;
			str[index] = str[index - nk];
			index -= nk;
		}
		while (1);
		str[last] = c; first++;
	}
	return len;
}


