/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include <qse/cmn/fmt.h>

/* ==================== multibyte ===================================== */
qse_size_t qse_fmtulongtombs (
	qse_mchar_t* buf, qse_size_t size, 
	qse_long_t value, int base_and_flags, qse_mchar_t fillchar)
{
	qse_mchar_t tmp[(QSE_SIZEOF(qse_ulong_t) * 8)];
	qse_mchar_t* p, * bp, * be;
	int base;
	qse_mchar_t xbasechar;

	base = base_and_flags & 0xFF;
	if (base < 2 || base > 36 || size <= 0) return 0;

	p = tmp; 
	bp = buf;
	be = buf + size - 1;

	xbasechar = (base_and_flags & QSE_FMTULONGTOMBS_UPPERCASE)? QSE_MT('A'): QSE_MT('a');

	/* store the resulting numeric string into 'tmp' first */
	do
	{
		int digit = value % base;
		if (digit < 10) *p++ = digit + QSE_MT('0');
		else *p++ = digit + xbasechar - 10;
		value /= base;
	}
	while (value > 0);

	/* fill space */
	if (fillchar != QSE_MT('\0'))
	{
		qse_size_t tmplen = p - tmp;

		if (base_and_flags & QSE_FMTULONGTOMBS_FILLRIGHT)
		{
			/* copy the numeric string to the destination buffer */
			while (p > tmp && bp < be) *bp++ = *--p;

			/* fill the right side */
			while (size - 1 > tmplen)
			{
				*bp++ = fillchar;
				size--;
			}
		}
		else
		{
			/* fill the left side */
			while (size - 1 > tmplen)
			{
				*bp++ = fillchar;
				size--;
			}

			/* copy the numeric string to the destination buffer */
			while (p > tmp && bp < be) *bp++ = *--p;
		}
	}
	else
	{
		/* copy the numeric string to the destination buffer */
		while (p > tmp && bp < be) *bp++ = *--p;
	}

	*bp = QSE_MT('\0');
	return bp - buf;
}

/* ==================== wide-char ===================================== */
qse_size_t qse_fmtulongtowcs (
	qse_wchar_t* buf, qse_size_t size, 
	qse_long_t value, int base_and_flags, qse_wchar_t fillchar)
{
	qse_wchar_t tmp[(QSE_SIZEOF(qse_ulong_t) * 8)];
	qse_wchar_t* p, * bp, * be;
	int base;
	qse_wchar_t xbasechar;

	base = base_and_flags & 0xFF;
	if (base < 2 || base > 36 || size <= 0) return 0;

	p = tmp; 
	bp = buf;
	be = buf + size - 1;

	xbasechar = (base_and_flags & QSE_FMTULONGTOWCS_UPPERCASE)? QSE_WT('A'): QSE_WT('a');

	/* store the resulting numeric string into 'tmp' first */
	do
	{
		int digit = value % base;
		if (digit < 10) *p++ = digit + QSE_WT('0');
		else *p++ = digit + xbasechar - 10;
		value /= base;
	}
	while (value > 0);

	/* fill space */
	if (fillchar != QSE_WT('\0'))
	{
		qse_size_t tmplen = p - tmp;

		if (base_and_flags & QSE_FMTULONGTOWCS_FILLRIGHT)
		{
			/* copy the numeric string to the destination buffer */
			while (p > tmp && bp < be) *bp++ = *--p;

			/* fill the right side */
			while (size - 1 > tmplen)
			{
				*bp++ = fillchar;
				size--;
			}
		}
		else
		{
			/* fill the left side */
			while (size - 1 > tmplen)
			{
				*bp++ = fillchar;
				size--;
			}

			/* copy the numeric string to the destination buffer */
			while (p > tmp && bp < be) *bp++ = *--p;
		}
	}
	else
	{
		/* copy the numeric string to the destination buffer */
		while (p > tmp && bp < be) *bp++ = *--p;
	}

	*bp = QSE_WT('\0');
	return bp - buf;
}


