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


