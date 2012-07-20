/*
 * $Id: str-cpy.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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
#include "mem.h"

qse_size_t qse_mbscpy (qse_mchar_t* buf, const qse_mchar_t* str)
{
	qse_mchar_t* org = buf;
	while ((*buf++ = *str++) != QSE_MT('\0'));
	return buf - org - 1;
}

qse_size_t qse_mbsxcpy (
	qse_mchar_t* buf, qse_size_t bsz, const qse_mchar_t* str)
{
	qse_mchar_t* p, * p2;

	p = buf; p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == QSE_MT('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_MT('\0');
	return p - buf;
}

qse_size_t qse_mbsncpy (
	qse_mchar_t* buf, const qse_mchar_t* str, qse_size_t len)
{
	/*
	const qse_mchar_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = QSE_MT('\0');
	return len;
	*/

	if (len > 0)
	{
		qse_size_t n = (len-1) >> 3; /* (len-1) / 8 */

		switch (len & 7) /* len % 8 */
		{
		repeat:
			case 0: *buf++ = *str++;
			case 7: *buf++ = *str++;
			case 6: *buf++ = *str++;
			case 5: *buf++ = *str++;
			case 4: *buf++ = *str++;
			case 3: *buf++ = *str++;
			case 2: *buf++ = *str++;
			case 1: *buf++ = *str++;
			        if (n <= 0) break;
			        n--;
			        goto repeat;
		}
	}

	*buf = QSE_MT('\0');
	return len;
}

qse_size_t qse_mbsxncpy (
	qse_mchar_t* buf, qse_size_t bsz, 
	const qse_mchar_t* str, qse_size_t len)
{
	qse_size_t n;

	if (bsz <= 0) return 0;
	if ((n = bsz - 1) > len) n = len;
	QSE_MEMCPY (buf, str, n * QSE_SIZEOF(qse_mchar_t));
	buf[n] = QSE_MT('\0');

	return n;
}

qse_size_t qse_wcscpy (qse_wchar_t* buf, const qse_wchar_t* str)
{
	qse_wchar_t* org = buf;
	while ((*buf++ = *str++) != QSE_WT('\0'));
	return buf - org - 1;
}

qse_size_t qse_wcsxcpy (
	qse_wchar_t* buf, qse_size_t bsz, const qse_wchar_t* str)
{
	qse_wchar_t* p, * p2;

	p = buf; p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == QSE_WT('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_WT('\0');
	return p - buf;
}

qse_size_t qse_wcsncpy (
	qse_wchar_t* buf, const qse_wchar_t* str, qse_size_t len)
{
	/*
	const qse_wchar_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = QSE_WT('\0');
	return len;
	*/

	if (len > 0)
	{
		qse_size_t n = (len-1) >> 3; /* (len-1) / 8 */

		switch (len & 7) /* len % 8 */
		{
		repeat:
			case 0: *buf++ = *str++;
			case 7: *buf++ = *str++;
			case 6: *buf++ = *str++;
			case 5: *buf++ = *str++;
			case 4: *buf++ = *str++;
			case 3: *buf++ = *str++;
			case 2: *buf++ = *str++;
			case 1: *buf++ = *str++;
			        if (n <= 0) break;
			        n--;
			        goto repeat;
		}
	}

	*buf = QSE_WT('\0');
	return len;
}

qse_size_t qse_wcsxncpy (
	qse_wchar_t* buf, qse_size_t bsz, 
	const qse_wchar_t* str, qse_size_t len)
{
	qse_size_t n;

	if (bsz <= 0) return 0;
	if ((n = bsz - 1) > len) n = len;
	QSE_MEMCPY (buf, str, n * QSE_SIZEOF(qse_wchar_t));
	buf[n] = QSE_WT('\0');

	return n;
}






