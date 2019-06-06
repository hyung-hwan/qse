/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#if !defined(char_t) && !defined(strcat) && !defined(strxcat)
#	error Never include this file
#endif

qse_size_t strcat (char_t* buf, const char_t* str)
{
	char_t* org = buf;   
	buf += strlen(buf);
	while ((*buf++ = *str++) != T('\0'));
	return buf - org - 1;
}

qse_size_t strncat (char_t* buf, const char_t* str, qse_size_t len)
{
	qse_size_t x;
	const char_t* end = str + len;

	x = strlen(buf); buf += x;
	while (str < end) *buf++ = *str++;
	*buf = T('\0');
	return len + x;
}

qse_size_t strcatn (char_t* buf, const char_t* str, qse_size_t n)
{
	qse_size_t x;
	char_t* org = buf;
	const char_t* end = str + n;

	x = strlen(buf); buf += x;
	while (str < end) 
	{
		/* copies not more than n characters and stop if '\0' is met */
		if ((*buf++ = *str++) == T('\0')) return buf - org - 1;
	}
	return n + x;
}

qse_size_t strxcat (char_t* buf, qse_size_t bsz, const char_t* str)
{
	char_t* p, * p2;
	qse_size_t blen;

	blen = strlen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == T('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = T('\0');
	return p - buf;
}

qse_size_t strxncat (char_t* buf, qse_size_t bsz, const char_t* str, qse_size_t len)
{
	char_t* p, * p2;
	const char_t* end;
	qse_size_t blen;

	blen = strlen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	end = str + len;

	while (p < p2) 
	{
		if (str >= end) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = T('\0');
	return p - buf;
}

