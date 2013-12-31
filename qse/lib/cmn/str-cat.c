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

qse_size_t qse_mbscat (qse_mchar_t* buf, const qse_mchar_t* str)
{
	qse_mchar_t* org = buf;   
	buf += qse_mbslen(buf);
	while ((*buf++ = *str++) != QSE_MT('\0'));
	return buf - org - 1;
}

qse_size_t qse_mbsncat (qse_mchar_t* buf, const qse_mchar_t* str, qse_size_t len)
{
	qse_size_t x;
	const qse_mchar_t* end = str + len;

	x = qse_mbslen(buf); buf += x;
	while (str < end) *buf++ = *str++;
	*buf = QSE_MT('\0');
	return len + x;
}

qse_size_t qse_mbscatn (qse_mchar_t* buf, const qse_mchar_t* str, qse_size_t n)
{
	qse_size_t x;
	qse_mchar_t* org = buf;
	const qse_mchar_t* end = str + n;

	x = qse_mbslen(buf); buf += x;
	while (str < end) 
	{
		if ((*buf++ = *str++) == QSE_MT('\0')) return buf - org - 1;
	}
	return n + x;
}


qse_size_t qse_mbsxcat (qse_mchar_t* buf, qse_size_t bsz, const qse_mchar_t* str)
{
	qse_mchar_t* p, * p2;
	qse_size_t blen;

	blen = qse_mbslen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == QSE_MT('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_MT('\0');
	return p - buf;
}

qse_size_t qse_mbsxncat (
	qse_mchar_t* buf, qse_size_t bsz, const qse_mchar_t* str, qse_size_t len)
{
	qse_mchar_t* p, * p2;
	const qse_mchar_t* end;
	qse_size_t blen;

	blen = qse_mbslen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	end = str + len;

	while (p < p2) 
	{
		if (str >= end) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_MT('\0');
	return p - buf;
}

qse_size_t qse_wcscat (qse_wchar_t* buf, const qse_wchar_t* str)
{
	qse_wchar_t* org = buf;   
	buf += qse_wcslen(buf);
	while ((*buf++ = *str++) != QSE_WT('\0'));
	return buf - org - 1;
}

qse_size_t qse_wcsncat (qse_wchar_t* buf, const qse_wchar_t* str, qse_size_t len)
{
	qse_size_t x;
	const qse_wchar_t* end = str + len;

	x = qse_wcslen(buf); buf += x;
	while (str < end) *buf++ = *str++;
	*buf = QSE_WT('\0');
	return len + x;
}

qse_size_t qse_wcscatn (qse_wchar_t* buf, const qse_wchar_t* str, qse_size_t n)
{
	qse_size_t x;
	qse_wchar_t* org = buf;
	const qse_wchar_t* end = str + n;

	x = qse_wcslen(buf); buf += x;
	while (str < end) 
	{
		if ((*buf++ = *str++) == QSE_WT('\0')) return buf - org - 1;
	}
	return n + x;
}

qse_size_t qse_wcsxcat (qse_wchar_t* buf, qse_size_t bsz, const qse_wchar_t* str)
{
	qse_wchar_t* p, * p2;
	qse_size_t blen;

	blen = qse_wcslen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == QSE_WT('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_WT('\0');
	return p - buf;
}

qse_size_t qse_wcsxncat (
	qse_wchar_t* buf, qse_size_t bsz, const qse_wchar_t* str, qse_size_t len)
{
	qse_wchar_t* p, * p2;
	const qse_wchar_t* end;
	qse_size_t blen;

	blen = qse_wcslen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	end = str + len;

	while (p < p2) 
	{
		if (str >= end) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_WT('\0');
	return p - buf;
}

