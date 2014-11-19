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

qse_size_t qse_mbsincl (qse_mchar_t* str, const qse_mchar_t* cs)
{
	qse_mchar_t* p1, * p2;

	p1 = p2 = str;
	while (*p1 != QSE_MT('\0')) 
	{
		if (qse_mbschr(cs,*p1) != QSE_NULL) *p2++ = *p1;
		p1++;
	}
	*p2 = QSE_MT('\0');
	return p2 - str;
}

qse_size_t qse_mbsxincl (
	qse_mchar_t* str, qse_size_t len, const qse_mchar_t* cs)
{
	qse_mchar_t* p1, * p2, * end;

	p1 = p2 = str;
	end = p1 + len;
	while (p1 < end) 
	{
		if (qse_mbschr(cs,*p1) != QSE_NULL) *p2++ = *p1;
		p1++;
	}
	return p2 - str;
}

qse_size_t qse_wcsincl (qse_wchar_t* str, const qse_wchar_t* cs)
{
	qse_wchar_t* p1, * p2;

	p1 = p2 = str;
	while (*p1 != QSE_WT('\0')) 
	{
		if (qse_wcschr(cs,*p1) != QSE_NULL) *p2++ = *p1;
		p1++;
	}
	*p2 = QSE_WT('\0');
	return p2 - str;
}

qse_size_t qse_wcsxincl (
	qse_wchar_t* str, qse_size_t len, const qse_wchar_t* cs)
{
	qse_wchar_t* p1, * p2, * end;

	p1 = p2 = str;
	end = p1 + len;
	while (p1 < end) 
	{
		if (qse_wcschr(cs,*p1) != QSE_NULL) *p2++ = *p1;
		p1++;
	}
	return p2 - str;
}

