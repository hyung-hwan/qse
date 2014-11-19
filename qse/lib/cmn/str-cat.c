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

