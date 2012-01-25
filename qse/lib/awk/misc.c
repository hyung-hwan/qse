/*
 * $Id: misc.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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

#include "awk.h"

void* qse_awk_allocmem (qse_awk_t* awk, qse_size_t size)
{
	void* ptr = QSE_AWK_ALLOC (awk, size);
	if (ptr == QSE_NULL)  qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

void* qse_awk_reallocmem (qse_awk_t* awk, void* ptr, qse_size_t size)
{
	void* nptr = QSE_AWK_REALLOC (awk, ptr, size);
	if (nptr == QSE_NULL) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return nptr;
}

void qse_awk_freemem (qse_awk_t* awk, void* ptr)
{
	QSE_AWK_FREE (awk, ptr);
}

qse_char_t* qse_awk_strdup (qse_awk_t* awk, const qse_char_t* s)
{
	qse_char_t* ptr = QSE_AWK_STRDUP (awk, s);
	if (ptr == QSE_NULL) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

qse_char_t* qse_awk_strxdup (qse_awk_t* awk, const qse_char_t* s, qse_size_t l)
{
	qse_char_t* ptr = QSE_AWK_STRXDUP (awk, s, l);
	if (ptr == QSE_NULL) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

qse_long_t qse_awk_strxtolong (
	qse_awk_t* awk, const qse_char_t* str, qse_size_t len,
	int base, const qse_char_t** endptr)
{
	qse_long_t n = 0;
	const qse_char_t* p;
	const qse_char_t* end;
	qse_size_t rem;
	int digit, negative = 0;

	QSE_ASSERT (base < 37); 

	p = str; 
	end = str + len;
	
	if (awk->option & QSE_AWK_STRIPSTRSPC)
	{
		/* strip off leading spaces */
		while (p < end && QSE_AWK_ISSPACE(awk,*p)) p++;
	}

	/* check for a sign */
	while (p < end)
	{
		if (*p == QSE_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == QSE_T('+')) p++;
		else break;
	}

	/* check for a binary/octal/hexadecimal notation */
	rem = end - p;
	if (base == 0) 
	{
		if (rem >= 1 && *p == QSE_T('0')) 
		{
			p++;

			if (rem == 1) base = 8;
			else if (*p == QSE_T('x') || *p == QSE_T('X'))
			{
				p++; base = 16;
			} 
			else if (*p == QSE_T('b') || *p == QSE_T('B'))
			{
				p++; base = 2;
			}
			else base = 8;
		}
		else base = 10;
	} 
	else if (rem >= 2 && base == 16)
	{
		if (*p == QSE_T('0') && 
		    (*(p+1) == QSE_T('x') || *(p+1) == QSE_T('X'))) p += 2; 
	}
	else if (rem >= 2 && base == 2)
	{
		if (*p == QSE_T('0') && 
		    (*(p+1) == QSE_T('b') || *(p+1) == QSE_T('B'))) p += 2; 
	}

	/* process the digits */
	while (p < end)
	{
		if (*p >= QSE_T('0') && *p <= QSE_T('9'))
			digit = *p - QSE_T('0');
		else if (*p >= QSE_T('A') && *p <= QSE_T('Z'))
			digit = *p - QSE_T('A') + 10;
		else if (*p >= QSE_T('a') && *p <= QSE_T('z'))
			digit = *p - QSE_T('a') + 10;
		else break;

		if (digit >= base) break;
		n = n * base + digit;

		p++;
	}

	if (endptr) *endptr = p;
	return (negative)? -n: n;
}


/*
 * qse_awk_strtoreal is almost a replica of strtod.
 *
 * strtod.c --
 *
 *      Source code for the "strtod" library procedure.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#define MAX_EXPONENT 511

qse_flt_t qse_awk_strtoreal (qse_awk_t* awk, const qse_char_t* str)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static qse_flt_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	qse_flt_t fraction, dbl_exp, * d;
	const qse_char_t* p;
	qse_cint_t c;
	int exp = 0;		/* Esseonent read from "EX" field */

	/* 
	 * Esseonent that derives from the fractional part.  Under normal 
	 * circumstatnces, it is the negative of the number of digits in F.
	 * However, if I is very long, the last digits of I get dropped 
	 * (otherwise a long I with a large negative exponent could cause an
	 * unnecessary overflow on I alone).  In this case, frac_exp is 
	 * incremented one for each dropped digit. 
	 */

	int frac_exp;
	int mant_size; /* Number of digits in mantissa. */
	int dec_pt;    /* Number of mantissa digits BEFORE decimal point */
	const qse_char_t *pexp;  /* Temporarily holds location of exponent in string */
	int negative = 0, exp_negative = 0;

	p = str;

	if (awk->option & QSE_AWK_STRIPSTRSPC)
	{
		/* strip off leading spaces */ 
		while (QSE_AWK_ISSPACE(awk,*p)) p++;
	}

	/* check for a sign */
	while (*p != QSE_T('\0')) 
	{
		if (*p == QSE_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == QSE_T('+')) p++;
		else break;
	}

	/* Count the number of digits in the mantissa (including the decimal
	 * point), and also locate the decimal point. */
	dec_pt = -1;
	for (mant_size = 0; ; mant_size++) 
	{
		c = *p;
		if (!QSE_AWK_ISDIGIT (awk, c)) 
		{
			if ((c != QSE_T('.')) || (dec_pt >= 0)) break;
			dec_pt = mant_size;
		}
		p++;
	}

	/*
	 * Now suck up the digits in the mantissa.  Use two integers to
	 * collect 9 digits each (this is faster than using floating-point).
	 * If the mantissa has more than 18 digits, ignore the extras, since
	 * they can't affect the value anyway.
	 */
	pexp = p;
	p -= mant_size;
	if (dec_pt < 0) 
	{
		dec_pt = mant_size;
	} 
	else 
	{
		mant_size--;	/* One of the digits was the point */
	}

	if (mant_size > 18) 
	{
		frac_exp = dec_pt - 18;
		mant_size = 18;
	} 
	else 
	{
		frac_exp = dec_pt - mant_size;
	}

	if (mant_size == 0) 
	{
		fraction = 0.0;
		/*p = str;*/
		p = pexp;
		goto done;
	} 
	else 
	{
		int frac1, frac2;
		frac1 = 0;
		for ( ; mant_size > 9; mant_size--) 
		{
			c = *p;
			p++;
			if (c == QSE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac1 = 10 * frac1 + (c - QSE_T('0'));
		}
		frac2 = 0;
		for (; mant_size > 0; mant_size--) {
			c = *p;
			p++;
			if (c == QSE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac2 = 10*frac2 + (c - QSE_T('0'));
		}
		fraction = (1.0e9 * frac1) + frac2;
	}

	/* Skim off the exponent */
	p = pexp;
	if ((*p == QSE_T('E')) || (*p == QSE_T('e'))) 
	{
		p++;
		if (*p == QSE_T('-')) 
		{
			exp_negative = 1;
			p++;
		} 
		else 
		{
			if (*p == QSE_T('+')) p++;
			exp_negative = 0;
		}
		if (!QSE_AWK_ISDIGIT (awk, *p)) 
		{
			/* p = pexp; */
			/* goto done; */
			goto no_exp;
		}
		while (QSE_AWK_ISDIGIT (awk, *p)) 
		{
			exp = exp * 10 + (*p - QSE_T('0'));
			p++;
		}
	}

no_exp:
	if (exp_negative) exp = frac_exp - exp;
	else exp = frac_exp + exp;

	/*
	 * Generate a floating-point number that represents the exponent.
	 * Do this by processing the exponent one bit at a time to combine
	 * many powers of 2 of 10. Then combine the exponent with the
	 * fraction.
	 */
	if (exp < 0) 
	{
		exp_negative = 1;
		exp = -exp;
	} 
	else exp_negative = 0;

	if (exp > MAX_EXPONENT) exp = MAX_EXPONENT;

	dbl_exp = 1.0;

	for (d = powers_of_10; exp != 0; exp >>= 1, d++) 
	{
		if (exp & 01) dbl_exp *= *d;
	}

	if (exp_negative) fraction /= dbl_exp;
	else fraction *= dbl_exp;

done:
	return (negative)? -fraction: fraction;
}

qse_flt_t qse_awk_strxtoflt (
	qse_awk_t* awk, const qse_char_t* str, qse_size_t len, 
	const qse_char_t** endptr)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static qse_flt_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	qse_flt_t fraction, dbl_exp, * d;
	const qse_char_t* p, * end;
	qse_cint_t c;
	int exp = 0; /* Esseonent read from "EX" field */

	/* 
	 * Esseonent that derives from the fractional part.  Under normal 
	 * circumstatnces, it is the negative of the number of digits in F.
	 * However, if I is very long, the last digits of I get dropped 
	 * (otherwise a long I with a large negative exponent could cause an
	 * unnecessary overflow on I alone).  In this case, frac_exp is 
	 * incremented one for each dropped digit. 
	 */

	int frac_exp;
	int mant_size; /* Number of digits in mantissa. */
	int dec_pt;    /* Number of mantissa digits BEFORE decimal point */
	const qse_char_t *pexp;  /* Temporarily holds location of exponent in string */
	int negative = 0, exp_negative = 0;

	p = str;
	end = str + len;

	/* Strip off leading blanks and check for a sign */
	/*while (QSE_AWK_ISSPACE(awk,*p)) p++;*/

	/*while (*p != QSE_T('\0')) */
	while (p < end)
	{
		if (*p == QSE_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == QSE_T('+')) p++;
		else break;
	}

	/* Count the number of digits in the mantissa (including the decimal
	 * point), and also locate the decimal point. */
	dec_pt = -1;
	/*for (mant_size = 0; ; mant_size++) */
	for (mant_size = 0; p < end; mant_size++) 
	{
		c = *p;
		if (!QSE_AWK_ISDIGIT (awk, c)) 
		{
			if (c != QSE_T('.') || dec_pt >= 0) break;
			dec_pt = mant_size;
		}
		p++;
	}

	/*
	 * Now suck up the digits in the mantissa.  Use two integers to
	 * collect 9 digits each (this is faster than using floating-point).
	 * If the mantissa has more than 18 digits, ignore the extras, since
	 * they can't affect the value anyway.
	 */
	pexp = p;
	p -= mant_size;
	if (dec_pt < 0) 
	{
		dec_pt = mant_size;
	} 
	else 
	{
		mant_size--;	/* One of the digits was the point */
	}

	if (mant_size > 18)  /* TODO: is 18 correct for qse_flt_t??? */
	{
		frac_exp = dec_pt - 18;
		mant_size = 18;
	} 
	else 
	{
		frac_exp = dec_pt - mant_size;
	}

	if (mant_size == 0) 
	{
		fraction = 0.0;
		/*p = str;*/
		p = pexp;
		goto done;
	} 
	else 
	{
		int frac1, frac2;

		frac1 = 0;
		for ( ; mant_size > 9; mant_size--) 
		{
			c = *p;
			p++;
			if (c == QSE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac1 = 10 * frac1 + (c - QSE_T('0'));
		}

		frac2 = 0;
		for (; mant_size > 0; mant_size--) {
			c = *p++;
			if (c == QSE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac2 = 10 * frac2 + (c - QSE_T('0'));
		}
		fraction = (1.0e9 * frac1) + frac2;
	}

	/* Skim off the exponent */
	p = pexp;
	if (p < end && (*p == QSE_T('E') || *p == QSE_T('e'))) 
	{
		p++;

		if (p < end) 
		{
			if (*p == QSE_T('-')) 
			{
				exp_negative = 1;
				p++;
			} 
			else 
			{
				if (*p == QSE_T('+')) p++;
				exp_negative = 0;
			}
		}
		else exp_negative = 0;

		if (!(p < end && QSE_AWK_ISDIGIT (awk, *p))) 
		{
			/*p = pexp;*/
			/*goto done;*/
			goto no_exp;
		}

		while (p < end && QSE_AWK_ISDIGIT (awk, *p)) 
		{
			exp = exp * 10 + (*p - QSE_T('0'));
			p++;
		}
	}

no_exp:
	if (exp_negative) exp = frac_exp - exp;
	else exp = frac_exp + exp;

	/*
	 * Generate a floating-point number that represents the exponent.
	 * Do this by processing the exponent one bit at a time to combine
	 * many powers of 2 of 10. Then combine the exponent with the
	 * fraction.
	 */
	if (exp < 0) 
	{
		exp_negative = 1;
		exp = -exp;
	} 
	else exp_negative = 0;

	if (exp > MAX_EXPONENT) exp = MAX_EXPONENT;

	dbl_exp = 1.0;

	for (d = powers_of_10; exp != 0; exp >>= 1, d++) 
	{
		if (exp & 01) dbl_exp *= *d;
	}

	if (exp_negative) fraction /= dbl_exp;
	else fraction *= dbl_exp;

done:
	if (endptr != QSE_NULL) *endptr = p;
	return (negative)? -fraction: fraction;
}

qse_size_t qse_awk_longtostr (
	qse_awk_t* awk, qse_long_t value, 
	int radix, const qse_char_t* prefix, qse_char_t* buf, qse_size_t size)
{
	qse_long_t t, rem;
	qse_size_t len, ret, i;
	qse_size_t prefix_len;

	prefix_len = (prefix != QSE_NULL)? qse_strlen(prefix): 0;

	t = value;
	if (t == 0)
	{
		/* zero */
		if (buf == QSE_NULL) 
		{
			/* if buf is not given, 
			 * return the number of bytes required */
			return prefix_len + 1;
		}

		if (size < prefix_len+1) 
		{
			/* buffer too small */
			return (qse_size_t)-1;
		}

		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
		buf[prefix_len] = QSE_T('0');
		if (size > prefix_len+1) buf[prefix_len+1] = QSE_T('\0');
		return prefix_len+1;
	}

	/* non-zero values */
	len = prefix_len;
	if (t < 0) { t = -t; len++; }
	while (t > 0) { len++; t /= radix; }

	if (buf == QSE_NULL)
	{
		/* if buf is not given, return the number of bytes required */
		return len;
	}

	if (size < len) return (qse_size_t)-1; /* buffer too small */
	if (size > len) buf[len] = QSE_T('\0');
	ret = len;

	t = value;
	if (t < 0) t = -t;

	while (t > 0) 
	{
		rem = t % radix;
		if (rem >= 10)
			buf[--len] = (qse_char_t)rem + QSE_T('a') - 10;
		else
			buf[--len] = (qse_char_t)rem + QSE_T('0');
		t /= radix;
	}

	if (value < 0) 
	{
		for (i = 1; i <= prefix_len; i++) 
		{
			buf[i] = prefix[i-1];
			len--;
		}
		buf[--len] = QSE_T('-');
	}
	else
	{
		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
	}

	return ret;
}

qse_char_t* qse_awk_rtx_strtok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, 
	const qse_char_t* delim, qse_cstr_t* tok)
{
	return qse_awk_rtx_strxntok (
		rtx, s, qse_strlen(s), delim, qse_strlen(delim), tok);
}

qse_char_t* qse_awk_rtx_strxtok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, qse_size_t len,
	const qse_char_t* delim, qse_cstr_t* tok)
{
	return qse_awk_rtx_strxntok (
		rtx, s, len, delim, qse_strlen(delim), tok);
}

qse_char_t* qse_awk_rtx_strntok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, 
	const qse_char_t* delim, qse_size_t delim_len,
	qse_cstr_t* tok)
{
	return qse_awk_rtx_strxntok (
		rtx, s, qse_strlen(s), delim, delim_len, tok);
}

qse_char_t* qse_awk_rtx_strxntok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, qse_size_t len,
	const qse_char_t* delim, qse_size_t delim_len, qse_cstr_t* tok)
{
	const qse_char_t* p = s, *d;
	const qse_char_t* end = s + len;	
	const qse_char_t* sp = QSE_NULL, * ep = QSE_NULL;
	const qse_char_t* delim_end = delim + delim_len;
	qse_char_t c; 
	int delim_mode;

#define __DELIM_NULL      0
#define __DELIM_EMPTY     1
#define __DELIM_SPACES    2
#define __DELIM_NOSPACES  3
#define __DELIM_COMPOSITE 4
	if (delim == QSE_NULL) delim_mode = __DELIM_NULL;
	else 
	{
		delim_mode = __DELIM_EMPTY;

		for (d = delim; d < delim_end; d++) 
		{
			if (QSE_AWK_ISSPACE(rtx->awk,*d)) 
			{
				if (delim_mode == __DELIM_EMPTY)
					delim_mode = __DELIM_SPACES;
				else if (delim_mode == __DELIM_NOSPACES)
				{
					delim_mode = __DELIM_COMPOSITE;
					break;
				}
			}
			else
			{
				if (delim_mode == __DELIM_EMPTY)
					delim_mode = __DELIM_NOSPACES;
				else if (delim_mode == __DELIM_SPACES)
				{
					delim_mode = __DELIM_COMPOSITE;
					break;
				}
			}
		}

		/* TODO: verify the following statement... */
		if (delim_mode == __DELIM_SPACES && 
		    delim_len == 1 && 
		    delim[0] != QSE_T(' ')) delim_mode = __DELIM_NOSPACES;
	}		
	
	if (delim_mode == __DELIM_NULL) 
	{ 
		/* when QSE_NULL is given as "delim", it trims off the 
		 * leading and trailing spaces characters off the source
		 * string "s" eventually. */

		while (p < end && QSE_AWK_ISSPACE(rtx->awk,*p)) p++;
		while (p < end) 
		{
			c = *p;

			if (!QSE_AWK_ISSPACE(rtx->awk,c)) 
			{
				if (sp == QSE_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == __DELIM_EMPTY)
	{
		/* each character in the source string "s" becomes a token. */
		if (p < end)
		{
			c = *p;
			sp = p;
			ep = p++;
		}
	}
	else if (delim_mode == __DELIM_SPACES) 
	{
		/* each token is delimited by space characters. all leading
		 * and trailing spaces are removed. */

		while (p < end && QSE_AWK_ISSPACE(rtx->awk,*p)) p++;
		while (p < end) 
		{
			c = *p;
			if (QSE_AWK_ISSPACE(rtx->awk,c)) break;
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
		while (p < end && QSE_AWK_ISSPACE(rtx->awk,*p)) p++;
	}
	else if (delim_mode == __DELIM_NOSPACES)
	{
		/* each token is delimited by one of charaters 
		 * in the delimeter set "delim". */
		if (rtx->gbl.ignorecase)
		{
			while (p < end) 
			{
				c = QSE_AWK_TOUPPER(rtx->awk, *p);
				for (d = delim; d < delim_end; d++) 
				{
					if (c == QSE_AWK_TOUPPER(rtx->awk,*d)) goto exit_loop;
				}

				if (sp == QSE_NULL) sp = p;
				ep = p++;
			}
		}
		else
		{
			while (p < end) 
			{
				c = *p;
				for (d = delim; d < delim_end; d++) 
				{
					if (c == *d) goto exit_loop;
				}

				if (sp == QSE_NULL) sp = p;
				ep = p++;
			}
		}
	}
	else /* if (delim_mode == __DELIM_COMPOSITE) */ 
	{
		/* each token is delimited by one of non-space charaters
		 * in the delimeter set "delim". however, all space characters
		 * surrounding the token are removed */
		while (p < end && QSE_AWK_ISSPACE(rtx->awk,*p)) p++;
		if (rtx->gbl.ignorecase)
		{
			while (p < end) 
			{
				c = QSE_AWK_TOUPPER(rtx->awk, *p);
				if (QSE_AWK_ISSPACE(rtx->awk,c)) 
				{
					p++;
					continue;
				}
				for (d = delim; d < delim_end; d++) 
				{
					if (c == QSE_AWK_TOUPPER(rtx->awk,*d))
						goto exit_loop;
				}
				if (sp == QSE_NULL) sp = p;
				ep = p++;
			}
		}
		else
		{
			while (p < end) 
			{
				c = *p;
				if (QSE_AWK_ISSPACE(rtx->awk,c)) 
				{
					p++;
					continue;
				}
				for (d = delim; d < delim_end; d++) 
				{
					if (c == *d) goto exit_loop;
				}
				if (sp == QSE_NULL) sp = p;
				ep = p++;
			}
		}
	}

exit_loop:
	if (sp == QSE_NULL) 
	{
		tok->ptr = QSE_NULL;
		tok->len = (qse_size_t)0;
	}
	else 
	{
		tok->ptr = sp;
		tok->len = ep - sp + 1;
	}

	/* if QSE_NULL is returned, this function should not be called again */
	if (p >= end) return QSE_NULL;
	if (delim_mode == __DELIM_EMPTY || 
	    delim_mode == __DELIM_SPACES) return (qse_char_t*)p;
	return (qse_char_t*)++p;
}

qse_char_t* qse_awk_rtx_strxntokbyrex (
	qse_awk_rtx_t* rtx, 
	const qse_char_t* str, qse_size_t len,
	const qse_char_t* substr, qse_size_t sublen,
	void* rex, qse_cstr_t* tok,
	qse_awk_errnum_t* errnum)
{
	int n;
	qse_size_t i;
	qse_cstr_t match, s, cursub, realsub;

	s.ptr = str;
	s.len = len;

	cursub.ptr = substr;
	cursub.len = sublen;

	realsub.ptr = substr;
	realsub.len = sublen;

	while (cursub.len > 0)
	{
		n = QSE_AWK_MATCHREX (
			rtx->awk, rex, 
			((rtx->gbl.ignorecase)? QSE_REX_IGNORECASE: 0),
			&s, &cursub, &match, errnum);
		if (n == -1) return QSE_NULL;
		if (n == 0)
		{
			/* no match has been found. 
			 * return the entire string as a token */
			tok->ptr = realsub.ptr;
			tok->len = realsub.len;
			*errnum = QSE_AWK_ENOERR;
			return QSE_NULL; 
		}

		QSE_ASSERT (n == 1);

		if (match.len == 0)
		{
			/* the match length is zero. */
			cursub.ptr++;
			cursub.len--;
		}
		else if (rtx->awk->option & QSE_AWK_STRIPRECSPC)
		{
			/* match at the beginning of the input string */
			if (match.ptr == substr) 
			{
				for (i = 0; i < match.len; i++)
				{
					if (!QSE_AWK_ISSPACE(rtx->awk, match.ptr[i]))
						goto exit_loop;
				}

				/* the match that is all spaces at the 
				 * beginning of the input string is skipped */
				cursub.ptr += match.len;
				cursub.len -= match.len;

				/* adjust the substring by skipping the leading
				 * spaces and retry matching */
				realsub.ptr = substr + match.len;
				realsub.len -= match.len;
			}
			else break;
		}
		else break;
	}

exit_loop:
	if (cursub.len <= 0)
	{
		tok->ptr = realsub.ptr;
		tok->len = realsub.len;
		*errnum = QSE_AWK_ENOERR;
		return QSE_NULL; 
	}

	tok->ptr = realsub.ptr;
	tok->len = match.ptr - realsub.ptr;

	for (i = 0; i < match.len; i++)
	{
		if (!QSE_AWK_ISSPACE(rtx->awk, match.ptr[i]))
		{
			/* the match contains a non-space character. */
			*errnum = QSE_AWK_ENOERR;
			return (qse_char_t*)match.ptr+match.len;
		}
	}

	/* the match is all spaces */
	*errnum = QSE_AWK_ENOERR;
	if (rtx->awk->option & QSE_AWK_STRIPRECSPC)
	{
		/* if the match reached the last character in the input string,
		 * it returns QSE_NULL to terminate tokenization. */
		return (match.ptr+match.len >= substr+sublen)? 
			QSE_NULL: ((qse_char_t*)match.ptr+match.len);
	}
	else
	{
		/* if the match went beyond the the last character in the input 
		 * string, it returns QSE_NULL to terminate tokenization. */
		return (match.ptr+match.len > substr+sublen)? 
			QSE_NULL: ((qse_char_t*)match.ptr+match.len);
	}
}

qse_char_t* qse_awk_rtx_strxnfld (
	qse_awk_rtx_t* rtx, qse_char_t* str, qse_size_t len,
	qse_char_t fs, qse_char_t ec, qse_char_t lq, qse_char_t rq,
	qse_cstr_t* tok)
{
	qse_char_t* p = str;
	qse_char_t* end = str + len;
	int escaped = 0, quoted = 0;
	qse_char_t* ts; /* token start */
	qse_char_t* tp; /* points to one char past the last token char */
	qse_char_t* xp; /* points to one char past the last effective char */

	/* skip leading spaces */
	while (p < end && QSE_ISSPACE(*p)) p++;

	/* initialize token pointers */
	ts = tp = xp = p; 

	while (p < end)
	{
		char c = *p;

		if (escaped)
		{
			*tp++ = c; xp = tp; p++;
			escaped = 0;
		}
		else
		{
			if (c == ec)
			{
				escaped = 1;
				p++;
			}
			else if (quoted)
			{
				if (c == rq)
				{
					quoted = 0;
					p++;
				}
				else
				{
					*tp++ = c; xp = tp; p++;
				}
			}
			else 
			{
				if (c == fs)
				{
					tok->ptr = ts;
					tok->len = xp - ts;
					p++;

					if (QSE_ISSPACE(fs))
					{
						while (p < end && *p == fs) p++;
						if (p >= end) return QSE_NULL;
					}

					return p;
				}
		
				if (c == lq)
				{
					quoted = 1;
					p++;
				}
				else
				{
					*tp++ = c; p++;
					if (!QSE_ISSPACE(c)) xp = tp; 
				}
			}
		}
	}

	if (escaped) 
	{
		/* if it is still escaped, the last character must be 
		 * the escaper itself. treat it as a normal character */
		*xp++ = ec;
	}
	
	tok->ptr = ts;
	tok->len = xp - ts;
	return QSE_NULL;
}

#define QSE_AWK_REXERRTOERR(err) \
	((err == QSE_REX_ENOERR)?      QSE_AWK_ENOERR: \
	 (err == QSE_REX_ENOMEM)?      QSE_AWK_ENOMEM: \
	 (err == QSE_REX_ENOCOMP)?     QSE_AWK_EREXNOCOMP: \
	 (err == QSE_REX_ERECUR)?      QSE_AWK_EREXRECUR: \
	 (err == QSE_REX_ERPAREN)?     QSE_AWK_EREXRPAREN: \
	 (err == QSE_REX_ERBRACK)?     QSE_AWK_EREXRBRACK: \
	 (err == QSE_REX_ERBRACE)?     QSE_AWK_EREXRBRACE: \
	 (err == QSE_REX_ECOLON)?      QSE_AWK_EREXCOLON: \
	 (err == QSE_REX_ECRANGE)?     QSE_AWK_EREXCRANGE: \
	 (err == QSE_REX_ECCLASS)?     QSE_AWK_EREXCCLASS: \
	 (err == QSE_REX_EBOUND)?      QSE_AWK_EREXBOUND: \
	 (err == QSE_REX_ESPCAWP)?     QSE_AWK_EREXSPCAWP: \
	 (err == QSE_REX_EPREEND)?     QSE_AWK_EREXPREEND: \
	                               QSE_AWK_EINTERN)

void* qse_awk_buildrex (
	qse_awk_t* awk, const qse_char_t* ptn, 
	qse_size_t len, qse_awk_errnum_t* errnum)
{
	qse_rex_errnum_t err;
	void* p;

	p = qse_buildrex (
		awk->mmgr, awk->rex.depth.max.build, 
		((awk->option&QSE_AWK_REXBOUND)? 0: QSE_REX_NOBOUND),
		ptn, len, &err
	);
	if (p == QSE_NULL) *errnum = QSE_AWK_REXERRTOERR(err);
	return p;
}

int qse_awk_matchrex (
	qse_awk_t* awk, void* code, int option,
	const qse_cstr_t* str, const qse_cstr_t* substr,
	qse_cstr_t* match, qse_awk_errnum_t* errnum)
{
	int x;
	qse_rex_errnum_t err;

	x = qse_matchrex (
		awk->mmgr, awk->rex.depth.max.match,
		code, option, str, substr, match, &err);
	if (x <= -1) *errnum = QSE_AWK_REXERRTOERR(err);
	return x;
}

void* qse_awk_rtx_allocmem (qse_awk_rtx_t* rtx, qse_size_t size)
{
	return qse_awk_allocmem (rtx->awk, size);
}

void* qse_awk_rtx_reallocmem (qse_awk_rtx_t* rtx, void* ptr, qse_size_t size)
{
	return qse_awk_reallocmem (rtx->awk, ptr, size);
}

void qse_awk_rtx_freemem (qse_awk_rtx_t* rtx, void* ptr)
{
	qse_awk_freemem (rtx->awk, ptr);
}

int qse_awk_sprintflt (
	qse_awk_t* awk, qse_char_t* buf, qse_size_t len, qse_flt_t num)
{
	return awk->prm.sprintf (
		awk, buf, len,
	#if QSE_SIZEOF_LONG_DOUBLE > 0
		QSE_T("%Lf"), (long double)num
	#elif QSE_SIZEOF_DOUBLE > 0
		QSE_T("%f"), (double)num
	#else
		#error unsupported size	
	#endif
	);
}
