/*
 * $Id: misc.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

qse_long_t qse_lsp_strxtolong (
	qse_lsp_t* lsp, const qse_char_t* str, qse_size_t len,
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
	
	/* strip off leading spaces */
	/*while (QSE_LSP_ISSPACE(lsp,*p)) p++;*/

	/* check for a sign */
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
	/*while (*p != QSE_T('\0'))*/
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

	if (endptr != QSE_NULL) *endptr = p;
	return (negative)? -n: n;
}


/*
 * qse_lsp_strtoreal is almost a replica of strtod.
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

qse_real_t qse_lsp_strtoreal (qse_lsp_t* lsp, const qse_char_t* str)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static qse_real_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	qse_real_t fraction, dbl_exp, * d;
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

	/* strip off leading blanks */ 
	/*while (QSE_LSP_ISSPACE(lsp,*p)) p++;*/

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
		if (!QSE_LSP_ISDIGIT (lsp, c)) 
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
		if (!QSE_LSP_ISDIGIT (lsp, *p)) 
		{
			/* p = pexp; */
			/* goto done; */
			goto no_exp;
		}
		while (QSE_LSP_ISDIGIT (lsp, *p)) 
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

qse_real_t qse_lsp_strxtoreal (
	qse_lsp_t* lsp, const qse_char_t* str, qse_size_t len, 
	const qse_char_t** endptr)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static qse_real_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	qse_real_t fraction, dbl_exp, * d;
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
	/*while (QSE_LSP_ISSPACE(lsp,*p)) p++;*/

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
		if (!QSE_LSP_ISDIGIT (lsp, c)) 
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

	if (mant_size > 18)  /* TODO: is 18 correct for qse_real_t??? */
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

		if (!(p < end && QSE_LSP_ISDIGIT (lsp, *p))) 
		{
			/*p = pexp;*/
			/*goto done;*/
			goto no_exp;
		}

		while (p < end && QSE_LSP_ISDIGIT (lsp, *p)) 
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

qse_size_t qse_lsp_longtostr (
	qse_long_t value, int radix, const qse_char_t* prefix, 
	qse_char_t* buf, qse_size_t size)
{
	qse_long_t t, rem;
	qse_size_t len, ret, i;
	qse_size_t prefix_len;

	prefix_len = (prefix != QSE_NULL)? qse_strlen(prefix): 0;

	t = value;
	if (t == 0)
	{
		/* zero */
		if (buf == QSE_NULL) return prefix_len + 1;

		if (size < prefix_len+1) 
		{
			/* buffer too small */
			return (qse_size_t)-1;
		}

		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
		buf[prefix_len] = QSE_T('0');
		if (size > prefix_len+1) buf[prefix_len+1] = QSE_T('\0');
		return 1;
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

