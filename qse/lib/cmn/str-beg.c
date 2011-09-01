/*
 * $Id: str-beg.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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
#include <qse/cmn/chr.h>

qse_mchar_t* qse_mbsbeg (const qse_mchar_t* str, const qse_mchar_t* sub)
{
	while (*sub != QSE_MT('\0'))
	{
		if (*str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_mchar_t*)str;
}

qse_mchar_t* qse_mbsxbeg (
	const qse_mchar_t* str, qse_size_t len, const qse_mchar_t* sub)
{
	const qse_mchar_t* end = str + len;

	while (*sub != QSE_MT('\0'))
	{
		if (str >= end || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_mchar_t*)str;
}

qse_mchar_t* qse_mbsnbeg (
	const qse_mchar_t* str, const qse_mchar_t* sub, qse_size_t len)
{
	const qse_mchar_t* end = sub + len;
		
	while (sub < end)
	{
		if (*str == QSE_MT('\0') || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_mchar_t*)str;
}

qse_mchar_t* qse_mbsxnbeg (
	const qse_mchar_t* str, qse_size_t len1, 
	const qse_mchar_t* sub, qse_size_t len2)
{
	const qse_mchar_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (sub < end2)
	{
		if (str >= end1 || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_mchar_t*)str;
}

qse_mchar_t* qse_mbscasebeg (const qse_mchar_t* str, const qse_mchar_t* sub)
{
	while (*sub != QSE_MT('\0'))
	{
		if (QSE_TOMUPPER(*str) != QSE_TOMUPPER(*sub)) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_mchar_t*)str;
}

qse_wchar_t* qse_wcsbeg (const qse_wchar_t* str, const qse_wchar_t* sub)
{
	while (*sub != QSE_WT('\0'))
	{
		if (*str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_wchar_t*)str;
}

qse_wchar_t* qse_wcsxbeg (
	const qse_wchar_t* str, qse_size_t len, const qse_wchar_t* sub)
{
	const qse_wchar_t* end = str + len;

	while (*sub != QSE_WT('\0'))
	{
		if (str >= end || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_wchar_t*)str;
}

qse_wchar_t* qse_wcsnbeg (
	const qse_wchar_t* str, const qse_wchar_t* sub, qse_size_t len)
{
	const qse_wchar_t* end = sub + len;
		
	while (sub < end)
	{
		if (*str == QSE_WT('\0') || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_wchar_t*)str;
}

qse_wchar_t* qse_wcsxnbeg (
	const qse_wchar_t* str, qse_size_t len1, 
	const qse_wchar_t* sub, qse_size_t len2)
{
	const qse_wchar_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (sub < end2)
	{
		if (str >= end1 || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_wchar_t*)str;
}

qse_wchar_t* qse_wcscasebeg (const qse_wchar_t* str, const qse_wchar_t* sub)
{
	while (*sub != QSE_WT('\0'))
	{
		if (QSE_TOWUPPER(*str) != QSE_TOWUPPER(*sub)) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_wchar_t*)str;
}
