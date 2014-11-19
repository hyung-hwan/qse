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

#if !defined(char_t) && !defined(strjoin) && !defined(strxjoin)
#	error Never include this file
#endif


qse_size_t strxjoinv (char_t* buf, qse_size_t size, va_list ap)
{
	const char_t* p;
	char_t* ptr = buf;
	qse_size_t left = size, n;

	while (left > 0) 
	{
		p = va_arg (ap, const char_t*);
		if (p == QSE_NULL) break;

		n = strxcpy (ptr, left, p);
		left -= n; ptr += n;
	}

	return size - left;
}


qse_size_t strxjoin (char_t* buf, qse_size_t size, ...)
{
	va_list ap;
	qse_size_t n;

	va_start (ap, size);
	n = strxjoinv (buf, size, ap);
	va_end (ap);

	return n;
}

qse_size_t strjoinv (char_t* buf, va_list ap)
{
	const char_t* p;
	char_t* ptr = buf;
	qse_size_t n;

	while (1)
	{
		p = va_arg (ap, const char_t*);
		if (p == QSE_NULL) break;

		n = strcpy (ptr, p);
		ptr += n;
	}

	return ptr - buf;
}


qse_size_t strjoin (char_t* buf, ...)
{
	va_list ap;
	qse_size_t n;

	va_start (ap, buf);
	n = strjoinv (buf, ap);
	va_end (ap);

	return n;
}
