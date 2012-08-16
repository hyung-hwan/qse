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

qse_size_t qse_mbsput (qse_mchar_t* buf, const qse_mchar_t* str)
{
	qse_mchar_t* org = buf;
	while (*str != QSE_MT('\0')) *buf++ = *str++;
	return buf - org;
}

qse_size_t qse_wcsput (qse_wchar_t* buf, const qse_wchar_t* str)
{
	qse_wchar_t* org = buf;
	while (*str != QSE_WT('\0')) *buf++ = *str++;
	return buf - org;
}

qse_size_t qse_mbsxput (
	qse_mchar_t* buf, qse_size_t bsz, const qse_mchar_t* str)
{
	qse_mchar_t* p, * p2;

	p = buf; p2 = buf + bsz;

	while (p < p2) 
	{
		if (*str == QSE_MT('\0')) break;
		*p++ = *str++;
	}

	return p - buf;
}

qse_size_t qse_mbsxnput (
	qse_mchar_t* buf, qse_size_t bsz, const qse_mchar_t* str, qse_size_t len)
{
	qse_mchar_t* p, * p2; 
	const qse_mchar_t* end;

	p = buf; p2 = buf + bsz; end = str + len;

	while (p < p2) 
	{
		if (str >= end) break;
		*p++ = *str++;
	}

	return p - buf;
}

qse_size_t qse_wcsxput (
	qse_wchar_t* buf, qse_size_t bsz, const qse_wchar_t* str)
{
	qse_wchar_t* p, * p2;

	p = buf; p2 = buf + bsz;

	while (p < p2) 
	{
		if (*str == QSE_WT('\0')) break;
		*p++ = *str++;
	}

	return p - buf;
}

qse_size_t qse_wcsxnput (
	qse_wchar_t* buf, qse_size_t bsz, const qse_wchar_t* str, qse_size_t len)
{
	qse_wchar_t* p, * p2; 
	const qse_wchar_t* end;

	p = buf; p2 = buf + bsz; end = str + len;

	while (p < p2) 
	{
		if (str >= end) break;
		*p++ = *str++;
	}

	return p - buf;
}
