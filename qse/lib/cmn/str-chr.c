/*
 * $Id: str-chr.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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

qse_mchar_t* qse_mbschr (const qse_mchar_t* str, qse_mcint_t c)
{
	while (*str != QSE_MT('\0')) 
	{
		if (*str == c) return (qse_mchar_t*)str;
		str++;
	}
	return QSE_NULL;
}

qse_mchar_t* qse_mbsxchr (const qse_mchar_t* str, qse_size_t len, qse_mcint_t c)
{
	const qse_mchar_t* end = str + len;

	while (str < end) 
	{
		if (*str == c) return (qse_mchar_t*)str;
		str++;
	}

	return QSE_NULL;
}

qse_mchar_t* qse_mbsrchr (const qse_mchar_t* str, qse_mcint_t c)
{
	const qse_mchar_t* end = str;

	while (*end != QSE_MT('\0')) end++;

	while (end > str) 
	{
		if (*--end == c) return (qse_mchar_t*)end;
	}

	return QSE_NULL;
}

qse_mchar_t* qse_mbsxrchr (const qse_mchar_t* str, qse_size_t len, qse_mcint_t c)
{
	const qse_mchar_t* end = str + len;

	while (end > str) 
	{
		if (*--end == c) return (qse_mchar_t*)end;
	}

	return QSE_NULL;
}

qse_wchar_t* qse_wcschr (const qse_wchar_t* str, qse_wcint_t c)
{
	while (*str != QSE_WT('\0')) 
	{
		if (*str == c) return (qse_wchar_t*)str;
		str++;
	}
	return QSE_NULL;
}

qse_wchar_t* qse_wcsxchr (const qse_wchar_t* str, qse_size_t len, qse_wcint_t c)
{
	const qse_wchar_t* end = str + len;

	while (str < end) 
	{
		if (*str == c) return (qse_wchar_t*)str;
		str++;
	}

	return QSE_NULL;
}

qse_wchar_t* qse_wcsrchr (const qse_wchar_t* str, qse_wcint_t c)
{
	const qse_wchar_t* end = str;

	while (*end != QSE_MT('\0')) end++;

	while (end > str) 
	{
		if (*--end == c) return (qse_wchar_t*)end;
	}

	return QSE_NULL;
}

qse_wchar_t* qse_wcsxrchr (const qse_wchar_t* str, qse_size_t len, qse_wcint_t c)
{
	const qse_wchar_t* end = str + len;

	while (end > str) 
	{
		if (*--end == c) return (qse_wchar_t*)end;
	}

	return QSE_NULL;
}


