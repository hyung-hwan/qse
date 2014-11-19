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


qse_size_t strvfmt (char_t* buf, const char_t* fmt, va_list ap)
{
	buf_t b;
	fmtout_t fo;
	int x;

	b.ptr = buf;
	b.len = 0;
	b.capa = QSE_TYPE_MAX(qse_ssize_t);

	fo.limit = QSE_TYPE_MAX(qse_size_t) - 1;
	fo.ctx = &b;
	fo.put = b.ptr? put_char: put_char_null;
	fo.conv = conv_char;

	/* no I/O error must occurred by fmtout but there can be
	 * encoding conversion error by fmtout */
	x = fmtout (fmt, &fo, ap);

	/* fmtout must produce no I/O error but it can produce
	 * an encoding conversion error. if you didn't use a conversion
	 * specifier that requires encoding conversion (%S, %C, etc), 
	 * you don't need to worry about an error. */

	/* null-terminate regardless of error */
	if (b.ptr) 
	{
		QSE_ASSERT (fo.count == b.len);
		b.ptr[b.len] = T('\0');
	}

	if (x <= -1) return QSE_TYPE_MAX(qse_size_t);
	return fo.count;
}

qse_size_t strfmt (char_t* buf, const char_t* fmt, ...)
{
	qse_size_t x;
	va_list ap;

	va_start (ap, fmt);
	x = strvfmt (buf, fmt, ap);
	va_end (ap);

	return x;
}

qse_size_t strxvfmt (char_t* buf, qse_size_t len, const char_t* fmt, va_list ap)
{
	buf_t b;
	fmtout_t fo;
	int x;

	b.ptr = buf;
	b.len = 0;

	if (len > QSE_TYPE_MAX(qse_ssize_t))
		b.capa = QSE_TYPE_MAX(qse_ssize_t);
	else if (len > 0)
		b.capa = len - 1;
	else 
		b.capa = 0;

	fo.limit = QSE_TYPE_MAX(qse_size_t) - 1;
	fo.ctx = &b;
	fo.put = b.ptr? put_char: put_char_null;
	fo.conv = conv_char;

	x = fmtout (fmt, &fo, ap);

	/* fmtout must produce no I/O error but it can produce
	 * an encoding conversion error. if you didn't use a conversion
	 * specifier that requires encoding conversion (%S, %C, etc), 
	 * you don't need to worry about an error. */

	/* null-terminate regardless of error */
	if (b.ptr)
	{
		QSE_ASSERT (fo.count == b.len);
		if (len > 0) b.ptr[b.len] = T('\0'); 
	}

	if (x <= -1) return QSE_TYPE_MAX(qse_size_t);
	return fo.count;
}

qse_size_t strxfmt (char_t* buf, qse_size_t len, const char_t* fmt, ...)
{
	qse_size_t x;
	va_list ap;

	va_start (ap, fmt);
	x = strxvfmt (buf, len, fmt, ap);
	va_end (ap);

	return x;
}
