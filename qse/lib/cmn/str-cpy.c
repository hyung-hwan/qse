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
#include "mem.h"

qse_size_t qse_mbscpy (qse_mchar_t* buf, const qse_mchar_t* str)
{
	qse_mchar_t* org = buf;
	while ((*buf++ = *str++) != QSE_MT('\0'));
	return buf - org - 1;
}

qse_size_t qse_mbsxcpy (
	qse_mchar_t* buf, qse_size_t bsz, const qse_mchar_t* str)
{
	qse_mchar_t* p, * p2;

	p = buf; p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == QSE_MT('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_MT('\0');
	return p - buf;
}

qse_size_t qse_mbsncpy (
	qse_mchar_t* buf, const qse_mchar_t* str, qse_size_t len)
{
	/*
	const qse_mchar_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = QSE_MT('\0');
	return len;
	*/

	if (len > 0)
	{
		qse_size_t n = (len-1) >> 3; /* (len-1) / 8 */

		switch (len & 7) /* len % 8 */
		{
		repeat:
			case 0: *buf++ = *str++;
			case 7: *buf++ = *str++;
			case 6: *buf++ = *str++;
			case 5: *buf++ = *str++;
			case 4: *buf++ = *str++;
			case 3: *buf++ = *str++;
			case 2: *buf++ = *str++;
			case 1: *buf++ = *str++;
			        if (n <= 0) break;
			        n--;
			        goto repeat;
		}
	}

	*buf = QSE_MT('\0');
	return len;
}

qse_size_t qse_mbsxncpy (
	qse_mchar_t* buf, qse_size_t bsz, 
	const qse_mchar_t* str, qse_size_t len)
{
	qse_size_t n;

	if (bsz <= 0) return 0;
	if ((n = bsz - 1) > len) n = len;
	QSE_MEMCPY (buf, str, n * QSE_SIZEOF(qse_mchar_t));
	buf[n] = QSE_MT('\0');

	return n;
}

qse_size_t qse_wcscpy (qse_wchar_t* buf, const qse_wchar_t* str)
{
	qse_wchar_t* org = buf;
	while ((*buf++ = *str++) != QSE_WT('\0'));
	return buf - org - 1;
}

qse_size_t qse_wcsxcpy (
	qse_wchar_t* buf, qse_size_t bsz, const qse_wchar_t* str)
{
	qse_wchar_t* p, * p2;

	p = buf; p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == QSE_WT('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = QSE_WT('\0');
	return p - buf;
}

qse_size_t qse_wcsncpy (
	qse_wchar_t* buf, const qse_wchar_t* str, qse_size_t len)
{
	/*
	const qse_wchar_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = QSE_WT('\0');
	return len;
	*/

	if (len > 0)
	{
		qse_size_t n = (len-1) >> 3; /* (len-1) / 8 */

		switch (len & 7) /* len % 8 */
		{
		repeat:
			case 0: *buf++ = *str++;
			case 7: *buf++ = *str++;
			case 6: *buf++ = *str++;
			case 5: *buf++ = *str++;
			case 4: *buf++ = *str++;
			case 3: *buf++ = *str++;
			case 2: *buf++ = *str++;
			case 1: *buf++ = *str++;
			        if (n <= 0) break;
			        n--;
			        goto repeat;
		}
	}

	*buf = QSE_WT('\0');
	return len;
}

qse_size_t qse_wcsxncpy (
	qse_wchar_t* buf, qse_size_t bsz, 
	const qse_wchar_t* str, qse_size_t len)
{
	qse_size_t n;

	if (bsz <= 0) return 0;
	if ((n = bsz - 1) > len) n = len;
	QSE_MEMCPY (buf, str, n * QSE_SIZEOF(qse_wchar_t));
	buf[n] = QSE_WT('\0');

	return n;
}






