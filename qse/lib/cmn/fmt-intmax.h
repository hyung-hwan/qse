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

static int fmt_uintmax (
	char_t* buf, int size, 
	qse_uintmax_t value, int base_and_flags, int prec,
	char_t fillchar, char_t signchar, const char_t* prefix)
{
	char_t tmp[(QSE_SIZEOF(qse_uintmax_t) * 8)];
	int reslen, base, fillsize, reqlen, pflen, preczero;
	char_t* p, * bp, * be;
	const char_t* xbasestr;

	base = base_and_flags & 0x3F;
	if (base < 2 || base > 36) return -1;

	xbasestr = (base_and_flags & QSE_FMTINTMAX_UPPERCASE)?
		T("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
		T("0123456789abcdefghijklmnopqrstuvwxyz");

	if ((base_and_flags & QSE_FMTINTMAX_NOZERO) && value == 0) 
	{
		p = tmp; 
		if (base_and_flags & QSE_FMTINTMAX_ZEROLEAD) 
		{
			/* NOZERO emits no digit, ZEROLEAD emits 1 digit.
			 * so it emits '0' */
			reslen = 1;
			preczero = 1;
		}
		else
		{
			/* since the value is zero, emit no digits */
			reslen = 0;
			preczero = 0;
		}
	}
	else
	{
		qse_uintmax_t v = value;

		/* store the resulting numeric string into 'tmp' first */
		p = tmp; 
		do
		{
			*p++ = xbasestr[v % base];
			v /= base;
		}
		while (v > 0);

		/* reslen is the length of the resulting string without padding. */
		reslen = (int)(p - tmp);
	
		/* precision specified the minum number of digits to produce.
		 * so if the precision is larger that the digits produced, 
		 * reslen should be adjusted to precision */
		if (prec > reslen) 
		{
			/* if the precision is greater than the actual digits
			 * made from the value, 0 is inserted in front.
			 * ZEROLEAD doesn't have to be handled explicitly
			 * since it's achieved effortlessly */
			preczero = prec - reslen;
			reslen = prec;
		}
		else 
		{
			preczero = 0;
			if ((base_and_flags & QSE_FMTINTMAX_ZEROLEAD) && value != 0) 
			{
				/* if value is zero, 0 is emitted from it. 
				 * so ZEROLEAD don't need to add another 0. */
				preczero++;
				reslen++;
			}
		}
	}

	if (signchar) reslen++; /* increment reslen for the sign character */
	if (prefix)
	{
		/* since the length can be truncated for different type sizes,
		 * don't pass in a very long prefix. */
		pflen = (int) strlen(prefix);
		reslen += pflen;
	}
	else pflen = 0;

	/* get the required buffer size for lossless formatting */
	reqlen = (base_and_flags & QSE_FMTINTMAX_NONULL)? reslen: (reslen + 1);

	if (size <= 0 ||
	    ((base_and_flags & QSE_FMTINTMAX_NOTRUNC) && size < reqlen))
	{
		return -reqlen;
	}

	/* get the size to fill with fill characters */
	fillsize = (base_and_flags & QSE_FMTINTMAX_NONULL)? size: (size - 1);
	bp = buf;
	be = buf + fillsize;

	/* fill space */
	if (fillchar != T('\0'))
	{
		if (base_and_flags & QSE_FMTINTMAX_FILLRIGHT)
		{
			/* emit sign */
			if (signchar && bp < be) *bp++ = signchar;

			/* copy prefix if necessary */
			if (prefix) while (*prefix && bp < be) *bp++ = *prefix++;

			/* add 0s for precision */
			while (preczero > 0 && bp < be) 
			{ 
				*bp++ = T('0');
				preczero--; 
			}

			/* copy the numeric string to the destination buffer */
			while (p > tmp && bp < be) *bp++ = *--p;

			/* fill the right side */
			while (fillsize > reslen)
			{
				*bp++ = fillchar;
				fillsize--;
			}
		}
		else if (base_and_flags & QSE_FMTINTMAX_FILLCENTER)
		{
			/* emit sign */
			if (signchar && bp < be) *bp++ = signchar;

			/* fill the left side */
			while (fillsize > reslen)
			{
				*bp++ = fillchar;
				fillsize--;
			}

			/* copy prefix if necessary */
			if (prefix) while (*prefix && bp < be) *bp++ = *prefix++;

			/* add 0s for precision */
			while (preczero > 0 && bp < be) 
			{ 
				*bp++ = T('0');
				preczero--; 
			}

			/* copy the numeric string to the destination buffer */
			while (p > tmp && bp < be) *bp++ = *--p;
		}
		else
		{
			/* fill the left side */
			while (fillsize > reslen)
			{
				*bp++ = fillchar;
				fillsize--;
			}

			/* emit sign */
			if (signchar && bp < be) *bp++ = signchar;

			/* copy prefix if necessary */
			if (prefix) while (*prefix && bp < be) *bp++ = *prefix++;

			/* add 0s for precision */
			while (preczero > 0 && bp < be) 
			{ 
				*bp++ = T('0');
				preczero--; 
			}

			/* copy the numeric string to the destination buffer */
			while (p > tmp && bp < be) *bp++ = *--p;
		}
	}
	else
	{
		/* emit sign */
		if (signchar && bp < be) *bp++ = signchar;

		/* copy prefix if necessary */
		if (prefix) while (*prefix && bp < be) *bp++ = *prefix++;

		/* add 0s for precision */
		while (preczero > 0 && bp < be) 
		{ 
			*bp++ = T('0');
			preczero--; 
		}

		/* copy the numeric string to the destination buffer */
		while (p > tmp && bp < be) *bp++ = *--p;
	}

	if (!(base_and_flags & QSE_FMTINTMAX_NONULL)) *bp = T('\0');
	return bp - buf;
}
