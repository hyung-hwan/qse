/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */


qse_size_t strfmt (char_t* buf, const char_t* fmt, ...)
{
	buf_t b;
	va_list ap;
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
	va_start (ap, fmt);
	x = fmtout (fmt, &fo, ap);
	va_end (ap);

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

qse_size_t strxfmt (char_t* buf, qse_size_t len, const char_t* fmt, ...)
{
	buf_t b;
	va_list ap;
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

	va_start (ap, fmt);
	x = fmtout (fmt, &fo, ap);
	va_end (ap);

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

