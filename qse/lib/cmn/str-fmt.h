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
	qse_fmtout_t fo;

	b.ptr = buf;
	b.len = 0;
	b.capa = QSE_TYPE_MAX(qse_ssize_t);

	fo.limit = QSE_TYPE_MAX(qse_size_t) - 1;
	fo.ctx = &b;
	fo.put_mchar = output_mchar;
	fo.put_wchar = output_wchar;

	/* no error must be returned by fmtout since
	 * the callback function never fails. */
	va_start (ap, fmt);
	fmtout (fmt, &fo, ap);
	va_end (ap);

	b.ptr[b.len] = T('\0');

	return fo.count;
}

qse_size_t strxfmt (char_t* buf, qse_size_t len, const char_t* fmt, ...)
{
	buf_t b;
	va_list ap;
	qse_fmtout_t fo;

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
	fo.put_mchar = output_mchar;
	fo.put_wchar = output_wchar;

	/* no error must be returned by fmtout since
	 * the callback function never fails. */
	va_start (ap, fmt);
	fmtout (fmt, &fo, ap);
	va_end (ap);

	if (len > 0) b.ptr[b.len] = T('\0');

	return fo.count;
}

