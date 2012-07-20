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

#include <qse/cmn/fmt.h>
#include <qse/cmn/str.h>

/* ==================== multibyte ===================================== */

static int fmt_unsigned_to_mbs (
	qse_mchar_t* buf, int size, 
	qse_uintmax_t value, int base_and_flags, int prec,
	qse_mchar_t fillchar, qse_mchar_t signchar, const qse_mchar_t* prefix)
{
	qse_mchar_t tmp[(QSE_SIZEOF(qse_uintmax_t) * 8)];
	int reslen, base, fillsize, reqlen, pflen, preczero;
	qse_mchar_t* p, * bp, * be;
	const qse_mchar_t* xbasestr;

	base = base_and_flags & 0x3F;
	if (base < 2 || base > 36) return -1;

	xbasestr = (base_and_flags & QSE_FMTINTMAXTOMBS_UPPERCASE)?
		QSE_MT("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
		QSE_MT("0123456789abcdefghijklmnopqrstuvwxyz");


	if ((base_and_flags & QSE_FMTINTMAXTOMBS_NOZERO) && value == 0) 
	{
		p = tmp; 
		if (base_and_flags & QSE_FMTINTMAXTOMBS_ZEROLEAD) 
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
			if ((base_and_flags & QSE_FMTINTMAXTOMBS_ZEROLEAD) && value != 0) 
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
		pflen = (int)qse_mbslen(prefix);
		reslen += pflen;
	}
	else pflen = 0;

	/* get the required buffer size for lossless formatting */
	reqlen = (base_and_flags & QSE_FMTINTMAXTOMBS_NONULL)? reslen: (reslen + 1);

	if (size <= 0 ||
	    ((base_and_flags & QSE_FMTINTMAXTOMBS_NOTRUNC) && size < reqlen))
	{
		return -reqlen;
	}

	/* get the size to fill with fill characters */
	fillsize = (base_and_flags & QSE_FMTINTMAXTOMBS_NONULL)? size: (size - 1);
	bp = buf;
	be = buf + fillsize;

	/* fill space */
	if (fillchar != QSE_MT('\0'))
	{
		if (base_and_flags & QSE_FMTINTMAXTOMBS_FILLRIGHT)
		{
			/* emit sign */
			if (signchar && bp < be) *bp++ = signchar;

			/* copy prefix if necessary */
			if (prefix) while (*prefix && bp < be) *bp++ = *prefix++;

			/* add 0s for precision */
			while (preczero > 0 && bp < be) 
			{ 
				*bp++ = QSE_MT('0');
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
		else if (base_and_flags & QSE_FMTINTMAXTOMBS_FILLCENTER)
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
				*bp++ = QSE_MT('0');
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
				*bp++ = QSE_MT('0');
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
			*bp++ = QSE_MT('0');
			preczero--; 
		}

		/* copy the numeric string to the destination buffer */
		while (p > tmp && bp < be) *bp++ = *--p;
	}

	if (!(base_and_flags & QSE_FMTINTMAXTOMBS_NONULL)) *bp = QSE_MT('\0');
	return bp - buf;
}

int qse_fmtintmaxtombs (
	qse_mchar_t* buf, int size, 
	qse_intmax_t value, int base_and_flags, int prec,
	qse_mchar_t fillchar, const qse_mchar_t* prefix)
{
	qse_mchar_t signchar;
	qse_uintmax_t absvalue;

	if (value < 0)
	{
		signchar = QSE_MT('-');
		absvalue = -value;
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_PLUSSIGN)
	{
		signchar = QSE_MT('+');
		absvalue = value;
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_EMPTYSIGN)
	{
		signchar = QSE_MT(' ');
		absvalue = value;
	}
	else
	{
		signchar = QSE_MT('\0');
		absvalue = value;
	}

	return fmt_unsigned_to_mbs (
		buf, size, absvalue, base_and_flags, prec, fillchar, signchar, prefix);
}

int qse_fmtuintmaxtombs (
	qse_mchar_t* buf, int size, 
	qse_uintmax_t value, int base_and_flags, int prec,
	qse_mchar_t fillchar, const qse_mchar_t* prefix)
{
	qse_mchar_t signchar;

	/* determine if a sign character is needed */
	if (base_and_flags & QSE_FMTINTMAXTOMBS_PLUSSIGN)
	{
		signchar = QSE_MT('+');
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_EMPTYSIGN)
	{
		signchar = QSE_MT(' ');
	}
	else
	{
		signchar = QSE_MT('\0');
	}

	return fmt_unsigned_to_mbs (
		buf, size, value, base_and_flags, prec, fillchar, signchar, prefix);
}


/* ==================== wide-char ===================================== */

static int fmt_unsigned_to_wcs (
	qse_wchar_t* buf, int size, 
	qse_uintmax_t value, int base_and_flags, int prec,
	qse_wchar_t fillchar, qse_wchar_t signchar, const qse_wchar_t* prefix)
{
	qse_wchar_t tmp[(QSE_SIZEOF(qse_uintmax_t) * 8)];
	int reslen, base, fillsize, reqlen, pflen, preczero;
	qse_wchar_t* p, * bp, * be;
	const qse_wchar_t* xbasestr;

	base = base_and_flags & 0x3F;
	if (base < 2 || base > 36) return -1;

	xbasestr = (base_and_flags & QSE_FMTINTMAXTOWCS_UPPERCASE)?
		QSE_WT("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
		QSE_WT("0123456789abcdefghijklmnopqrstuvwxyz");


	if ((base_and_flags & QSE_FMTINTMAXTOWCS_NOZERO) && value == 0) 
	{
		p = tmp; 
		if (base_and_flags & QSE_FMTINTMAXTOWCS_ZEROLEAD) 
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
			if ((base_and_flags & QSE_FMTINTMAXTOWCS_ZEROLEAD) && value != 0) 
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
		pflen = (int)qse_wcslen(prefix);
		reslen += pflen;
	}
	else pflen = 0;

	/* get the required buffer size for lossless formatting */
	reqlen = (base_and_flags & QSE_FMTINTMAXTOWCS_NONULL)? reslen: (reslen + 1);

	if (size <= 0 ||
	    ((base_and_flags & QSE_FMTINTMAXTOWCS_NOTRUNC) && size < reqlen))
	{
		return -reqlen;
	}

	/* get the size to fill with fill characters */
	fillsize = (base_and_flags & QSE_FMTINTMAXTOWCS_NONULL)? size: (size - 1);
	bp = buf;
	be = buf + fillsize;

	/* fill space */
	if (fillchar != QSE_WT('\0'))
	{
		if (base_and_flags & QSE_FMTINTMAXTOWCS_FILLRIGHT)
		{
			/* emit sign */
			if (signchar && bp < be) *bp++ = signchar;

			/* copy prefix if necessary */
			if (prefix) while (*prefix && bp < be) *bp++ = *prefix++;

			/* add 0s for precision */
			while (preczero > 0 && bp < be) 
			{ 
				*bp++ = QSE_WT('0');
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
		else if (base_and_flags & QSE_FMTINTMAXTOWCS_FILLCENTER)
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
				*bp++ = QSE_WT('0');
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
				*bp++ = QSE_WT('0');
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
			*bp++ = QSE_WT('0');
			preczero--; 
		}

		/* copy the numeric string to the destination buffer */
		while (p > tmp && bp < be) *bp++ = *--p;
	}

	if (!(base_and_flags & QSE_FMTINTMAXTOWCS_NONULL)) *bp = QSE_WT('\0');
	return bp - buf;
}

int qse_fmtintmaxtowcs (
	qse_wchar_t* buf, int size, 
	qse_intmax_t value, int base_and_flags, int prec,
	qse_wchar_t fillchar, const qse_wchar_t* prefix)
{
	qse_wchar_t signchar;
	qse_uintmax_t absvalue;

	if (value < 0)
	{
		signchar = QSE_WT('-');
		absvalue = -value;
	}
	else if (base_and_flags & QSE_FMTINTMAXTOWCS_PLUSSIGN)
	{
		signchar = QSE_WT('+');
		absvalue = value;
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_EMPTYSIGN)
	{
		signchar = QSE_WT(' ');
		absvalue = value;
	}
	else
	{
		signchar = QSE_WT('\0');
		absvalue = value;
	}

	return fmt_unsigned_to_wcs (
		buf, size, absvalue, base_and_flags, prec, fillchar, signchar, prefix);
}

int qse_fmtuintmaxtowcs (
	qse_wchar_t* buf, int size, 
	qse_uintmax_t value, int base_and_flags, int prec,
	qse_wchar_t fillchar, const qse_wchar_t* prefix)
{
	qse_wchar_t signchar;

	/* determine if a sign character is needed */
	if (base_and_flags & QSE_FMTINTMAXTOWCS_PLUSSIGN)
	{
		signchar = QSE_WT('+');
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_EMPTYSIGN)
	{
		signchar = QSE_WT(' ');
	}
	else
	{
		signchar = QSE_WT('\0');
	}

	return fmt_unsigned_to_wcs (
		buf, size, value, base_and_flags, prec, fillchar, signchar, prefix);
}

