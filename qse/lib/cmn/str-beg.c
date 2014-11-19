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
