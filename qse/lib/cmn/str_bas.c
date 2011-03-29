/*
 * $Id: str_bas.c 419 2011-03-28 16:07:37Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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
#include "mem.h"

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

qse_size_t qse_strbytes (const qse_char_t* str)
{
	const qse_char_t* p = str;
	while (*p != QSE_T('\0')) p++;
	return (p - str) * QSE_SIZEOF(qse_char_t);
}

qse_char_t* qse_strdup (const qse_char_t* str, qse_mmgr_t* mmgr)
{
	qse_char_t* tmp;

	tmp = (qse_char_t*) QSE_MMGR_ALLOC (
		mmgr, (qse_strlen(str)+1)*QSE_SIZEOF(qse_char_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_strcpy (tmp, str);
	return tmp;
}

qse_char_t* qse_strdup2 (
	const qse_char_t* str1, const qse_char_t* str2, qse_mmgr_t* mmgr)
{
	return qse_strxdup2 (
		str1, qse_strlen(str1), str2, qse_strlen(str2), mmgr);
}

qse_char_t* qse_strxdup (
	const qse_char_t* str, qse_size_t len, qse_mmgr_t* mmgr)
{
	qse_char_t* tmp;

	tmp = (qse_char_t*) QSE_MMGR_ALLOC (
		mmgr, (len+1)*QSE_SIZEOF(qse_char_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_strncpy (tmp, str, len);
	return tmp;
}

qse_char_t* qse_strxdup2 (
	const qse_char_t* str1, qse_size_t len1,
	const qse_char_t* str2, qse_size_t len2, qse_mmgr_t* mmgr)
{
	qse_char_t* tmp;

	tmp = (qse_char_t*) QSE_MMGR_ALLOC (
		mmgr, (len1+len2+1) * QSE_SIZEOF(qse_char_t));
	if (tmp == QSE_NULL) return QSE_NULL;

	qse_strncpy (tmp, str1, len1);
	qse_strncpy (tmp + len1, str2, len2);
	return tmp;
}

qse_mchar_t* qse_mbsstr (const qse_mchar_t* str, const qse_mchar_t* sub)
{
	const qse_mchar_t* x, * y;

	y = sub;
	if (*y == QSE_MT('\0')) return (qse_mchar_t*)str;

	while (*str != QSE_MT('\0')) 
	{
		if (*str != *y) 
		{
			str++;
			continue;
		}

		x = str;
		do
		{
			if (*y == QSE_MT('\0')) return (qse_mchar_t*)str;
			if (*x++ != *y++) break;
		}
		while (1);

		y = sub;
		str++;
	}

	return QSE_NULL;
}

qse_wchar_t* qse_wcsstr (const qse_wchar_t* str, const qse_wchar_t* sub)
{
	const qse_wchar_t* x, * y;

	y = sub;
	if (*y == QSE_WT('\0')) return (qse_wchar_t*)str;

	while (*str != QSE_WT('\0')) 
	{
		if (*str != *y) 
		{
			str++;
			continue;
		}

		x = str;
		do
		{
			if (*y == QSE_WT('\0')) return (qse_wchar_t*)str;
			if (*x++ != *y++) break;
		}
		while (1);

		y = sub;
		str++;
	}

	return QSE_NULL;
}

qse_char_t* qse_strxstr (
	const qse_char_t* str, qse_size_t size, const qse_char_t* sub)
{
	return qse_strxnstr (str, size, sub, qse_strlen(sub));
}

qse_char_t* qse_strxnstr (
	const qse_char_t* str, qse_size_t strsz, 
	const qse_char_t* sub, qse_size_t subsz)
{
	const qse_char_t* end, * subp;

	if (subsz == 0) return (qse_char_t*)str;
	if (strsz < subsz) return QSE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) 
	{
		const qse_char_t* x = str;
		const qse_char_t* y = sub;

		while (1)
		{
			if (y >= subp) return (qse_char_t*)str;
			if (*x != *y) break;
			x++; y++;
		}	

		str++;
	}
		
	return QSE_NULL;
}

qse_char_t* qse_strcasestr (const qse_char_t* str, const qse_char_t* sub)
{
	const qse_char_t* x, * y;

	y = sub;
	if (*y == QSE_T('\0')) return (qse_char_t*)str;

	while (*str != QSE_T('\0')) 
	{
		if (QSE_TOUPPER(*str) != QSE_TOUPPER(*y)) 
		{
			str++;
			continue;
		}

		x = str;
		while (1)
		{
			if (*y == QSE_T('\0')) return (qse_char_t*)str;
			if (QSE_TOUPPER(*x) != QSE_TOUPPER(*y)) break;
			x++; y++;
		}

		y = sub;
		str++;
	}

	return QSE_NULL;
}

qse_char_t* qse_strxcasestr (
	const qse_char_t* str, qse_size_t size, const qse_char_t* sub)
{
	return qse_strxncasestr (str, size, sub, qse_strlen(sub));
}

qse_char_t* qse_strxncasestr (
	const qse_char_t* str, qse_size_t strsz, 
	const qse_char_t* sub, qse_size_t subsz)
{
	const qse_char_t* end, * subp;

	if (subsz == 0) return (qse_char_t*)str;
	if (strsz < subsz) return QSE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) 
	{
		const qse_char_t* x = str;
		const qse_char_t* y = sub;

		while (1)
		{
			if (y >= subp) return (qse_char_t*)str;
			if (QSE_TOUPPER(*x) != QSE_TOUPPER(*y)) break;
			x++; y++;
		}	

		str++;
	}
		
	return QSE_NULL;
}

qse_char_t* qse_strrstr (const qse_char_t* str, const qse_char_t* sub)
{
	return qse_strxnrstr (str, qse_strlen(str), sub, qse_strlen(sub));
}

qse_char_t* qse_strxrstr (
	const qse_char_t* str, qse_size_t size, const qse_char_t* sub)
{
	return qse_strxnrstr (str, size, sub, qse_strlen(sub));
}

qse_char_t* qse_strxnrstr (
	const qse_char_t* str, qse_size_t strsz, 
	const qse_char_t* sub, qse_size_t subsz)
{
	const qse_char_t* p = str + strsz;
	const qse_char_t* subp = sub + subsz;

	if (subsz == 0) return (qse_char_t*)p;
	if (strsz < subsz) return QSE_NULL;

	p = p - subsz;

	while (p >= str) 
	{
		const qse_char_t* x = p;
		const qse_char_t* y = sub;

		while (1) 
		{
			if (y >= subp) return (qse_char_t*)p;
			if (*x != *y) break;
			x++; y++;
		}	

		p--;
	}

	return QSE_NULL;
}

const qse_char_t* qse_strxword (
	const qse_char_t* str, qse_size_t len, const qse_char_t* word)
{
	/* find a full word in a string */

	const qse_char_t* ptr = str;
	const qse_char_t* end = str + len;
	const qse_char_t* s;

	do
	{
		while (ptr < end && QSE_ISSPACE(*ptr)) ptr++;
		if (ptr >= end) return QSE_NULL;

		s = ptr;
		while (ptr < end && !QSE_ISSPACE(*ptr)) ptr++;

		if (qse_strxcmp (s, ptr-s, word) == 0) return s;
	}
	while (ptr < end);

	return QSE_NULL;
}

const qse_char_t* qse_strxcaseword (
	const qse_char_t* str, qse_size_t len, const qse_char_t* word)
{
	const qse_char_t* ptr = str;
	const qse_char_t* end = str + len;
	const qse_char_t* s;

	do
	{
		while (ptr < end && QSE_ISSPACE(*ptr)) ptr++;
		if (ptr >= end) return QSE_NULL;

		s = ptr;
		while (ptr < end && !QSE_ISSPACE(*ptr)) ptr++;

		if (qse_strxcasecmp (s, ptr-s, word) == 0) return s;
	}
	while (ptr < end);

	return QSE_NULL;
}


qse_char_t* qse_strbeg (const qse_char_t* str, const qse_char_t* sub)
{
	while (*sub != QSE_T('\0'))
	{
		if (*str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_char_t*)str;
}

qse_char_t* qse_strxbeg (
	const qse_char_t* str, qse_size_t len, const qse_char_t* sub)
{
	const qse_char_t* end = str + len;

	while (*sub != QSE_T('\0'))
	{
		if (str >= end || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_char_t*)str;
}

qse_char_t* qse_strnbeg (
	const qse_char_t* str, const qse_char_t* sub, qse_size_t len)
{
	const qse_char_t* end = sub + len;
		
	while (sub < end)
	{
		if (*str == QSE_T('\0') || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_char_t*)str;
}

qse_char_t* qse_strxnbeg (
	const qse_char_t* str, qse_size_t len1, 
	const qse_char_t* sub, qse_size_t len2)
{
	const qse_char_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (sub < end2)
	{
		if (str >= end1 || *str != *sub) return QSE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (qse_char_t*)str;
}

qse_char_t* qse_strend (const qse_char_t* str, const qse_char_t* sub)
{
	return qse_strxnend (str, qse_strlen(str), sub, qse_strlen(sub));
}

qse_char_t* qse_strxend (
	const qse_char_t* str, qse_size_t len, const qse_char_t* sub)
{
	return qse_strxnend (str, len, sub, qse_strlen(sub));
}

qse_char_t* qse_strnend (
	const qse_char_t* str, const qse_char_t* sub, qse_size_t len)
{
	return qse_strxnend (str, qse_strlen(str), sub, len);
}

qse_char_t* qse_strxnend (
	const qse_char_t* str, qse_size_t len1, 
	const qse_char_t* sub, qse_size_t len2)
{
	const qse_char_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (end2 > sub)
	{
		if (end1 <= str) return QSE_NULL;
		if (*(--end1) != *(--end2)) return QSE_NULL;
	}
	
	/* returns the pointer to the match start */
	return (qse_char_t*)end1;
}
