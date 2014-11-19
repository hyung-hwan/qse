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
