/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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
