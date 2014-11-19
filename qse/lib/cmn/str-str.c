/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>

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

qse_mchar_t* qse_mbsxstr (
	const qse_mchar_t* str, qse_size_t size, const qse_mchar_t* sub)
{
	return qse_mbsxnstr (str, size, sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsxnstr (
	const qse_mchar_t* str, qse_size_t strsz, 
	const qse_mchar_t* sub, qse_size_t subsz)
{
	const qse_mchar_t* end, * subp;

	if (subsz == 0) return (qse_mchar_t*)str;
	if (strsz < subsz) return QSE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) 
	{
		const qse_mchar_t* x = str;
		const qse_mchar_t* y = sub;

		while (1)
		{
			if (y >= subp) return (qse_mchar_t*)str;
			if (*x != *y) break;
			x++; y++;
		}	

		str++;
	}
		
	return QSE_NULL;
}

qse_mchar_t* qse_mbscasestr (const qse_mchar_t* str, const qse_mchar_t* sub)
{
	const qse_mchar_t* x, * y;

	y = sub;
	if (*y == QSE_MT('\0')) return (qse_mchar_t*)str;

	while (*str != QSE_MT('\0')) 
	{
		if (QSE_TOMUPPER(*str) != QSE_TOMUPPER(*y)) 
		{
			str++;
			continue;
		}

		x = str;
		while (1)
		{
			if (*y == QSE_MT('\0')) return (qse_mchar_t*)str;
			if (QSE_TOMUPPER(*x) != QSE_TOMUPPER(*y)) break;
			x++; y++;
		}

		y = sub;
		str++;
	}

	return QSE_NULL;
}

qse_mchar_t* qse_mbsxcasestr (
	const qse_mchar_t* str, qse_size_t size, const qse_mchar_t* sub)
{
	return qse_mbsxncasestr (str, size, sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsxncasestr (
	const qse_mchar_t* str, qse_size_t strsz, 
	const qse_mchar_t* sub, qse_size_t subsz)
{
	const qse_mchar_t* end, * subp;

	if (subsz == 0) return (qse_mchar_t*)str;
	if (strsz < subsz) return QSE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) 
	{
		const qse_mchar_t* x = str;
		const qse_mchar_t* y = sub;

		while (1)
		{
			if (y >= subp) return (qse_mchar_t*)str;
			if (QSE_TOMUPPER(*x) != QSE_TOMUPPER(*y)) break;
			x++; y++;
		}	

		str++;
	}
		
	return QSE_NULL;
}

qse_mchar_t* qse_mbsrstr (const qse_mchar_t* str, const qse_mchar_t* sub)
{
	return qse_mbsxnrstr (str, qse_mbslen(str), sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsxrstr (
	const qse_mchar_t* str, qse_size_t size, const qse_mchar_t* sub)
{
	return qse_mbsxnrstr (str, size, sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsxnrstr (
	const qse_mchar_t* str, qse_size_t strsz, 
	const qse_mchar_t* sub, qse_size_t subsz)
{
	const qse_mchar_t* p = str + strsz;
	const qse_mchar_t* subp = sub + subsz;

	if (subsz == 0) return (qse_mchar_t*)p;
	if (strsz < subsz) return QSE_NULL;

	p = p - subsz;

	while (p >= str) 
	{
		const qse_mchar_t* x = p;
		const qse_mchar_t* y = sub;

		while (1) 
		{
			if (y >= subp) return (qse_mchar_t*)p;
			if (*x != *y) break;
			x++; y++;
		}	

		p--;
	}

	return QSE_NULL;
}

qse_mchar_t* qse_mbsrcasestr (const qse_mchar_t* str, const qse_mchar_t* sub)
{
	return qse_mbsxnrcasestr (str, qse_mbslen(str), sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsxrcasestr (
	const qse_mchar_t* str, qse_size_t size, const qse_mchar_t* sub)
{
	return qse_mbsxnrcasestr (str, size, sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsxnrcasestr (
	const qse_mchar_t* str, qse_size_t strsz, 
	const qse_mchar_t* sub, qse_size_t subsz)
{
	const qse_mchar_t* p = str + strsz;
	const qse_mchar_t* subp = sub + subsz;

	if (subsz == 0) return (qse_mchar_t*)p;
	if (strsz < subsz) return QSE_NULL;

	p = p - subsz;

	while (p >= str) 
	{
		const qse_mchar_t* x = p;
		const qse_mchar_t* y = sub;

		while (1) 
		{
			if (y >= subp) return (qse_mchar_t*)p;
			if (QSE_TOMUPPER(*x) != QSE_TOMUPPER(*y)) break;
			x++; y++;
		}	

		p--;
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

qse_wchar_t* qse_wcsxstr (
	const qse_wchar_t* str, qse_size_t size, const qse_wchar_t* sub)
{
	return qse_wcsxnstr (str, size, sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsxnstr (
	const qse_wchar_t* str, qse_size_t strsz, 
	const qse_wchar_t* sub, qse_size_t subsz)
{
	const qse_wchar_t* end, * subp;

	if (subsz == 0) return (qse_wchar_t*)str;
	if (strsz < subsz) return QSE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) 
	{
		const qse_wchar_t* x = str;
		const qse_wchar_t* y = sub;

		while (1)
		{
			if (y >= subp) return (qse_wchar_t*)str;
			if (*x != *y) break;
			x++; y++;
		}	

		str++;
	}
		
	return QSE_NULL;
}

qse_wchar_t* qse_wcscasestr (const qse_wchar_t* str, const qse_wchar_t* sub)
{
	const qse_wchar_t* x, * y;

	y = sub;
	if (*y == QSE_WT('\0')) return (qse_wchar_t*)str;

	while (*str != QSE_WT('\0')) 
	{
		if (QSE_TOWUPPER(*str) != QSE_TOWUPPER(*y)) 
		{
			str++;
			continue;
		}

		x = str;
		while (1)
		{
			if (*y == QSE_WT('\0')) return (qse_wchar_t*)str;
			if (QSE_TOWUPPER(*x) != QSE_TOWUPPER(*y)) break;
			x++; y++;
		}

		y = sub;
		str++;
	}

	return QSE_NULL;
}

qse_wchar_t* qse_wcsxcasestr (
	const qse_wchar_t* str, qse_size_t size, const qse_wchar_t* sub)
{
	return qse_wcsxncasestr (str, size, sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsxncasestr (
	const qse_wchar_t* str, qse_size_t strsz, 
	const qse_wchar_t* sub, qse_size_t subsz)
{
	const qse_wchar_t* end, * subp;

	if (subsz == 0) return (qse_wchar_t*)str;
	if (strsz < subsz) return QSE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) 
	{
		const qse_wchar_t* x = str;
		const qse_wchar_t* y = sub;

		while (1)
		{
			if (y >= subp) return (qse_wchar_t*)str;
			if (QSE_TOWUPPER(*x) != QSE_TOWUPPER(*y)) break;
			x++; y++;
		}	

		str++;
	}
		
	return QSE_NULL;
}

qse_wchar_t* qse_wcsrstr (const qse_wchar_t* str, const qse_wchar_t* sub)
{
	return qse_wcsxnrstr (str, qse_wcslen(str), sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsxrstr (
	const qse_wchar_t* str, qse_size_t size, const qse_wchar_t* sub)
{
	return qse_wcsxnrstr (str, size, sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsxnrstr (
	const qse_wchar_t* str, qse_size_t strsz, 
	const qse_wchar_t* sub, qse_size_t subsz)
{
	const qse_wchar_t* p = str + strsz;
	const qse_wchar_t* subp = sub + subsz;

	if (subsz == 0) return (qse_wchar_t*)p;
	if (strsz < subsz) return QSE_NULL;

	p = p - subsz;

	while (p >= str) 
	{
		const qse_wchar_t* x = p;
		const qse_wchar_t* y = sub;

		while (1) 
		{
			if (y >= subp) return (qse_wchar_t*)p;
			if (*x != *y) break;
			x++; y++;
		}	

		p--;
	}

	return QSE_NULL;
}

qse_wchar_t* qse_wcsrcasestr (const qse_wchar_t* str, const qse_wchar_t* sub)
{
	return qse_wcsxnrcasestr (str, qse_wcslen(str), sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsxrcasestr (
	const qse_wchar_t* str, qse_size_t size, const qse_wchar_t* sub)
{
	return qse_wcsxnrcasestr (str, size, sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsxnrcasestr (
	const qse_wchar_t* str, qse_size_t strsz, 
	const qse_wchar_t* sub, qse_size_t subsz)
{
	const qse_wchar_t* p = str + strsz;
	const qse_wchar_t* subp = sub + subsz;

	if (subsz == 0) return (qse_wchar_t*)p;
	if (strsz < subsz) return QSE_NULL;

	p = p - subsz;

	while (p >= str) 
	{
		const qse_wchar_t* x = p;
		const qse_wchar_t* y = sub;

		while (1) 
		{
			if (y >= subp) return (qse_wchar_t*)p;
			if (QSE_TOWUPPER(*x) != QSE_TOWUPPER(*y)) break;
			x++; y++;
		}	

		p--;
	}

	return QSE_NULL;
}
