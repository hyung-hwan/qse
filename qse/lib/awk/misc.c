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

#include "awk.h"

/*#define USE_REX */

#if defined(USE_REX)
#	include <qse/cmn/rex.h>
#else
#	include <qse/cmn/tre.h>
#endif

void* qse_awk_allocmem (qse_awk_t* awk, qse_size_t size)
{
	void* ptr = QSE_AWK_ALLOC (awk, size);
	if (ptr == QSE_NULL) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

void* qse_awk_callocmem (qse_awk_t* awk, qse_size_t size)
{
	void* ptr = QSE_AWK_ALLOC (awk, size);
	if (ptr) QSE_MEMSET (ptr, 0, size);
	else qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
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

qse_char_t* qse_awk_cstrdup (qse_awk_t* awk, const qse_cstr_t* s)
{
	qse_char_t* ptr = qse_cstrdup (s, awk->mmgr);
	if (ptr == QSE_NULL) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

qse_awk_int_t qse_awk_strxtoint (
	qse_awk_t* awk, const qse_char_t* str, qse_size_t len,
	int base, const qse_char_t** endptr)
{
	qse_awk_int_t n = 0;
	const qse_char_t* p;
	const qse_char_t* end;
	qse_size_t rem;
	int digit, negative = 0;

	QSE_ASSERT (base < 37); 

	p = str; 
	end = str + len;
	
	if (awk->opt.trait & QSE_AWK_STRIPSTRSPC)
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
 * qse_awk_strtoflt is almost a replica of strtod.
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

/*
 *                double(64bits)    extended(80-bits)    quadruple(128-bits)
 *  exponent      11 bits           15 bits              15 bits
 *  fraction      52 bits           63 bits              112 bits
 *  sign          1 bit             1 bit                1 bit
 *  integer                         1 bit
 */         
#define MAX_EXPONENT 511

qse_awk_flt_t qse_awk_strtoflt (qse_awk_t* awk, const qse_char_t* str)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static qse_awk_flt_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	qse_awk_flt_t fraction, dbl_exp, * d;
	const qse_char_t* p;
	qse_cint_t c;
	int exp = 0;		/* Exponent read from "EX" field */

	/* 
	 * Exponent that derives from the fractional part.  Under normal 
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

	if (awk->opt.trait & QSE_AWK_STRIPSTRSPC)
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

qse_awk_flt_t qse_awk_strxtoflt (
	qse_awk_t* awk, const qse_char_t* str, qse_size_t len, 
	const qse_char_t** endptr)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static qse_awk_flt_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	qse_awk_flt_t fraction, dbl_exp, * d;
	const qse_char_t* p, * end;
	qse_cint_t c;
	int exp = 0; /* Exponent read from "EX" field */

	/* 
	 * Exponent that derives from the fractional part.  Under normal 
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

	if (mant_size > 18)  /* TODO: is 18 correct for qse_awk_flt_t??? */
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

qse_size_t qse_awk_inttostr (
	qse_awk_t* awk, qse_awk_int_t value, 
	int radix, const qse_char_t* prefix, qse_char_t* buf, qse_size_t size)
{
	qse_awk_int_t t, rem;
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
		n = qse_awk_matchrex (
			rtx->awk, rex, rtx->gbl.ignorecase,
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
		else if (rtx->awk->opt.trait & QSE_AWK_STRIPRECSPC)
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
	if (rtx->awk->opt.trait & QSE_AWK_STRIPRECSPC)
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

static QSE_INLINE int rexerr_to_errnum (int err)
{
	switch (err)
	{
		case QSE_REX_ENOERR:   return QSE_AWK_ENOERR;
		case QSE_REX_ENOMEM:   return QSE_AWK_ENOMEM;
	 	case QSE_REX_ENOCOMP:  return QSE_AWK_EREXBL;
	 	case QSE_REX_ERECUR:   return QSE_AWK_EREXRECUR;
	 	case QSE_REX_ERPAREN:  return QSE_AWK_EREXRPAREN;
	 	case QSE_REX_ERBRACK:  return QSE_AWK_EREXRBRACK;
	 	case QSE_REX_ERBRACE:  return QSE_AWK_EREXRBRACE;
	 	case QSE_REX_ECOLON:   return QSE_AWK_EREXCOLON;
	 	case QSE_REX_ECRANGE:  return QSE_AWK_EREXCRANGE;
	 	case QSE_REX_ECCLASS:  return QSE_AWK_EREXCCLASS;
	 	case QSE_REX_EBOUND:   return QSE_AWK_EREXBOUND;
	 	case QSE_REX_ESPCAWP:  return QSE_AWK_EREXSPCAWP;
	 	case QSE_REX_EPREEND:  return QSE_AWK_EREXPREEND;
		default:               return QSE_AWK_EINTERN;
	}
}

int qse_awk_buildrex (
	qse_awk_t* awk, const qse_char_t* ptn, qse_size_t len, 
	qse_awk_errnum_t* errnum, void** code, void** icode)
{
#if defined(USE_REX)
	qse_rex_errnum_t err;
	void* p;

	if (code || icode)
	{
		p = qse_buildrex (
			awk->mmgr, awk->opt.depth.s.rex_build,
			((awk->opt.trait & QSE_AWK_REXBOUND)? 0: QSE_REX_NOBOUND),
			ptn, len, &err
		);
		if (p == QSE_NULL) 
		{
			*errnum = rexerr_to_errnum(err);
			return -1;
		}
	
		if (code) *code = p;
		if (icode) *icode = p;
	}

	return 0;
#else
	qse_tre_t* tre = QSE_NULL; 
	qse_tre_t* itre = QSE_NULL;
	int opt = QSE_TRE_EXTENDED;

	if (code)
	{
		tre = qse_tre_open (awk->mmgr, 0);
		if (tre == QSE_NULL)
		{
			*errnum = QSE_AWK_ENOMEM;
			return -1;
		}

		if (!(awk->opt.trait & QSE_AWK_REXBOUND)) opt |= QSE_TRE_NOBOUND;

		if (qse_tre_compx (tre, ptn, len, QSE_NULL, opt) <= -1)
		{
#if 0 /* TODO */

			if (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM) *errnum = QSE_AWK_ENOMEM;
			else
				SETERR1 (awk, QSE_AWK_EREXBL, str->ptr, str->len, loc);
#endif
			*errnum = (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)? 
				QSE_AWK_ENOMEM: QSE_AWK_EREXBL;
			qse_tre_close (tre);
			return -1;
		}
	}

	if (icode) 
	{
		itre = qse_tre_open (awk->mmgr, 0);
		if (itre == QSE_NULL)
		{
			if (tre) qse_tre_close (tre);
			*errnum = QSE_AWK_ENOMEM;
			return -1;
		}

		/* ignorecase is a compile option for TRE */
		if (qse_tre_compx (itre, ptn, len, QSE_NULL, opt | QSE_TRE_IGNORECASE) <= -1)
		{
#if 0 /* TODO */

			if (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM) *errnum = QSE_AWK_ENOMEM;
			else
				SETERR1 (awk, QSE_AWK_EREXBL, str->ptr, str->len, loc);
#endif
			*errnum = (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)? 
				QSE_AWK_ENOMEM: QSE_AWK_EREXBL;
			qse_tre_close (itre);
			if (tre) qse_tre_close (tre);
			return -1;
		}
	}

	if (code) *code = tre;
	if (icode) *icode = itre;
	return 0;	
#endif
}


#if !defined(USE_REX)

static int matchtre (
	qse_awk_t* awk, qse_tre_t* tre, int opt, 
	const qse_cstr_t* str, qse_cstr_t* mat, 
	qse_cstr_t submat[9], qse_awk_errnum_t* errnum)
{
	int n;
	qse_tre_match_t match[10] = { { 0, 0 }, };

	n = qse_tre_execx (tre, str->ptr, str->len, match, QSE_COUNTOF(match), opt);
	if (n <= -1)
	{
		if (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMATCH) return 0;

#if 0 /* TODO: */
		*errnum = (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)? 
			QSE_AWK_ENOMEM: QSE_AWK_EREXMA;
		SETERR0 (sed, errnum, loc);
#endif
		*errnum = (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)? 
			QSE_AWK_ENOMEM: QSE_AWK_EREXMA;
		return -1;	
	}

	QSE_ASSERT (match[0].rm_so != -1);
	if (mat)
	{
		mat->ptr = &str->ptr[match[0].rm_so];
		mat->len = match[0].rm_eo - match[0].rm_so;
	}

	if (submat)
	{
		int i;

		/* you must intialize submat before you pass into this 
		 * function because it can abort filling */
		for (i = 1; i < QSE_COUNTOF(match); i++)
		{
			if (match[i].rm_so != -1) 
			{
				submat[i-1].ptr = &str->ptr[match[i].rm_so];
				submat[i-1].len = match[i].rm_eo - match[i].rm_so;
			}
		}
	}
	return 1;
}
#endif

int qse_awk_matchrex (
	qse_awk_t* awk, void* code, int icase,
	const qse_cstr_t* str, const qse_cstr_t* substr,
	qse_cstr_t* match, qse_awk_errnum_t* errnum)
{
#if defined(USE_REX)
	int x;
	qse_rex_errnum_t err;

	x = qse_matchrex (
		awk->mmgr, awk->opt.depth.s.rex_match, code, 
		(icase? QSE_REX_IGNORECASE: 0), str, substr, match, &err);
	if (x <= -1) *errnum = rexerr_to_errnum(err);
	return x;
#else
	int x;
	int opt = QSE_TRE_BACKTRACKING; /* TODO: option... QSE_TRE_BACKTRACKING ??? */

	x = matchtre (
		awk, code,
		((str->ptr == substr->ptr)? opt: (opt | QSE_TRE_NOTBOL)),
		substr, match, QSE_NULL, errnum
	);
	return x;
#endif
}

void qse_awk_freerex (qse_awk_t* awk, void* code, void* icode)
{
	if (code)
	{
#if defined(USE_REX)
		qse_freerex ((awk)->mmgr, code);
#else
		qse_tre_close (code);
#endif
	}

	if (icode && icode != code)
	{
#if defined(USE_REX)
		qse_freerex ((awk)->mmgr, icode);
#else
		qse_tre_close (icode);
#endif
	}
}

int qse_awk_rtx_matchrex (
	qse_awk_rtx_t* rtx, qse_awk_val_t* val,
	const qse_cstr_t* str, const qse_cstr_t* substr, qse_cstr_t* match)
{
	void* code;
	int icase, x;
	qse_awk_errnum_t awkerr;
#if defined(USE_REX)
	qse_rex_errnum_t rexerr;
#endif

	icase = rtx->gbl.ignorecase;

	if (val->type == QSE_AWK_VAL_REX)
	{
		code = ((qse_awk_val_rex_t*)val)->code[icase];
	}
	else if (val->type == QSE_AWK_VAL_STR)
	{
		/* build a regular expression */
		qse_awk_val_str_t* strv = (qse_awk_val_str_t*)val;
		x = icase? qse_awk_buildrex (rtx->awk, strv->val.ptr, strv->val.len, &awkerr, QSE_NULL, &code):
		           qse_awk_buildrex (rtx->awk, strv->val.ptr, strv->val.len, &awkerr, &code, QSE_NULL);
		if (x <= -1)
		{
			qse_awk_rtx_seterrnum (rtx, awkerr, QSE_NULL);
			return -1;
		}
	}
	else 
	{
		/* convert to a string and build a regular expression */

		qse_xstr_t tmp;
		tmp.ptr = qse_awk_rtx_valtostrdup (rtx, val, &tmp.len);
		if (tmp.ptr == QSE_NULL) return -1;

		x = icase? qse_awk_buildrex (rtx->awk, tmp.ptr, tmp.len, &awkerr, QSE_NULL, &code):
		           qse_awk_buildrex (rtx->awk, tmp.ptr, tmp.len, &awkerr, &code, QSE_NULL);
		qse_awk_rtx_freemem (rtx, tmp.ptr);
		if (x <= -1)
		{
			qse_awk_rtx_seterrnum (rtx, awkerr, QSE_NULL);
			return -1;
		}
	}
	
#if defined(USE_REX)
	x = qse_matchrex (
		rtx->awk->mmgr, rtx->awk->opt.depth.s.rex_match,
		code, (icase? QSE_REX_IGNORECASE: 0),
		str, substr, match, &rexerr);
	if (x <= -1) qse_awk_rtx_seterrnum (rtx, rexerr_to_errnum(rexerr), QSE_NULL);
#else
	x = matchtre (
		rtx->awk, code,
		((str->ptr == substr->ptr)? QSE_TRE_BACKTRACKING: (QSE_TRE_BACKTRACKING | QSE_TRE_NOTBOL)),
		substr, match, QSE_NULL, &awkerr
	);
	if (x <= -1) qse_awk_rtx_seterrnum (rtx, awkerr, QSE_NULL);
#endif

	if (val->type == QSE_AWK_VAL_REX) 
	{
		/* nothing to free */
	}
	else
	{
		if (icase) 
			qse_awk_freerex (rtx->awk, QSE_NULL, code);
		else
			qse_awk_freerex (rtx->awk, code, QSE_NULL);
	}

	return x;
}

void* qse_awk_rtx_allocmem (qse_awk_rtx_t* rtx, qse_size_t size)
{
	void* ptr = QSE_AWK_ALLOC (rtx->awk, size);
	if (ptr == QSE_NULL) qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

void* qse_awk_rtx_reallocmem (qse_awk_rtx_t* rtx, void* ptr, qse_size_t size)
{
	void* nptr = QSE_AWK_REALLOC (rtx->awk, ptr, size);
	if (nptr == QSE_NULL) qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
	return nptr;
}

void* qse_awk_rtx_callocmem (qse_awk_rtx_t* rtx, qse_size_t size)
{
	void* ptr = QSE_AWK_ALLOC (rtx->awk, size);
	if (ptr) QSE_MEMSET (ptr, 0, size);
	else qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

void qse_awk_rtx_freemem (qse_awk_rtx_t* rtx, void* ptr)
{
	QSE_AWK_FREE (rtx->awk, ptr);
}

