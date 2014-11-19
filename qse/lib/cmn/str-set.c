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


qse_size_t qse_mbsset (qse_mchar_t* buf, qse_mchar_t c, qse_size_t n)
{
	qse_size_t len = n;
	buf[n] = QSE_MT('\0');
	while (n > 0) buf[--n] = c;
	return len;
}

qse_size_t qse_mbsxset (
	qse_mchar_t* buf, qse_size_t bsz, qse_mchar_t c, qse_size_t n)
{
	qse_size_t len;

	if (bsz <= 0) return 0;

	if (n >= bsz) n = bsz - 1;
	len = n;

	buf[n] = QSE_MT('\0');
	while (n > 0) buf[--n] = c;
	return len;
}

qse_size_t qse_wcsset (qse_wchar_t* buf, qse_wchar_t c, qse_size_t n)
{
	qse_size_t len = n;
	buf[n] = QSE_WT('\0');
	while (n > 0) buf[--n] = c;
	return len;
}

qse_size_t qse_wcsxset (
	qse_wchar_t* buf, qse_size_t bsz, qse_wchar_t c, qse_size_t n)
{
	qse_size_t len;

	if (bsz <= 0) return 0;

	if (n >= bsz) n = bsz - 1;
	len = n;

	buf[n] = QSE_WT('\0');
	while (n > 0) buf[--n] = c;
	return len;
}
