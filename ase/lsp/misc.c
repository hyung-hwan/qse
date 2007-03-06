/*
 * $Id: misc.c,v 1.11 2007-03-06 14:58:00 bacon Exp $
 *
 * {License}
 */

#include <ase/lsp/lsp_i.h>

ase_long_t ase_lsp_strxtolong (
	ase_lsp_t* lsp, const ase_char_t* str, ase_size_t len,
	int base, const ase_char_t** endptr)
{
	ase_long_t n = 0;
	const ase_char_t* p;
	const ase_char_t* end;
	ase_size_t rem;
	int digit, negative = 0;

	ASE_LSP_ASSERT (lsp, base < 37); 

	p = str; 
	end = str + len;
	
	/* strip off leading spaces */
	/*while (ASE_LSP_ISSPACE(lsp,*p)) p++;*/

	/* check for a sign */
	/*while (*p != ASE_T('\0')) */
	while (p < end)
	{
		if (*p == ASE_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == ASE_T('+')) p++;
		else break;
	}

	/* check for a binary/octal/hexadecimal notation */
	rem = end - p;
	if (base == 0) 
	{
		if (rem >= 1 && *p == ASE_T('0')) 
		{
			p++;

			if (rem == 1) base = 8;
			else if (*p == ASE_T('x') || *p == ASE_T('X'))
			{
				p++; base = 16;
			} 
			else if (*p == ASE_T('b') || *p == ASE_T('B'))
			{
				p++; base = 2;
			}
			else base = 8;
		}
		else base = 10;
	} 
	else if (rem >= 2 && base == 16)
	{
		if (*p == ASE_T('0') && 
		    (*(p+1) == ASE_T('x') || *(p+1) == ASE_T('X'))) p += 2; 
	}
	else if (rem >= 2 && base == 2)
	{
		if (*p == ASE_T('0') && 
		    (*(p+1) == ASE_T('b') || *(p+1) == ASE_T('B'))) p += 2; 
	}

	/* process the digits */
	/*while (*p != ASE_T('\0'))*/
	while (p < end)
	{
		if (*p >= ASE_T('0') && *p <= ASE_T('9'))
			digit = *p - ASE_T('0');
		else if (*p >= ASE_T('A') && *p <= ASE_T('Z'))
			digit = *p - ASE_T('A') + 10;
		else if (*p >= ASE_T('a') && *p <= ASE_T('z'))
			digit = *p - ASE_T('a') + 10;
		else break;

		if (digit >= base) break;
		n = n * base + digit;

		p++;
	}

	if (endptr != ASE_NULL) *endptr = p;
	return (negative)? -n: n;
}


/*
 * ase_lsp_strtoreal is almost a replica of strtod.
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

ase_real_t ase_lsp_strtoreal (ase_lsp_t* lsp, const ase_char_t* str)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static ase_real_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	ase_real_t fraction, dbl_exp, * d;
	const ase_char_t* p;
	ase_cint_t c;
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
	const ase_char_t *pexp;  /* Temporarily holds location of exponent in string */
	int negative = 0, exp_negative = 0;

	p = str;

	/* strip off leading blanks */ 
	/*while (ASE_LSP_ISSPACE(lsp,*p)) p++;*/

	/* check for a sign */
	while (*p != ASE_T('\0')) 
	{
		if (*p == ASE_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == ASE_T('+')) p++;
		else break;
	}

	/* Count the number of digits in the mantissa (including the decimal
	 * point), and also locate the decimal point. */
	dec_pt = -1;
	for (mant_size = 0; ; mant_size++) 
	{
		c = *p;
		if (!ASE_LSP_ISDIGIT (lsp, c)) 
		{
			if ((c != ASE_T('.')) || (dec_pt >= 0)) break;
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
			if (c == ASE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac1 = 10 * frac1 + (c - ASE_T('0'));
		}
		frac2 = 0;
		for (; mant_size > 0; mant_size--) {
			c = *p;
			p++;
			if (c == ASE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac2 = 10*frac2 + (c - ASE_T('0'));
		}
		fraction = (1.0e9 * frac1) + frac2;
	}

	/* Skim off the exponent */
	p = pexp;
	if ((*p == ASE_T('E')) || (*p == ASE_T('e'))) 
	{
		p++;
		if (*p == ASE_T('-')) 
		{
			exp_negative = 1;
			p++;
		} 
		else 
		{
			if (*p == ASE_T('+')) p++;
			exp_negative = 0;
		}
		if (!ASE_LSP_ISDIGIT (lsp, *p)) 
		{
			/* p = pexp; */
			/* goto done; */
			goto no_exp;
		}
		while (ASE_LSP_ISDIGIT (lsp, *p)) 
		{
			exp = exp * 10 + (*p - ASE_T('0'));
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

ase_real_t ase_lsp_strxtoreal (
	ase_lsp_t* lsp, const ase_char_t* str, ase_size_t len, 
	const ase_char_t** endptr)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static ase_real_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	ase_real_t fraction, dbl_exp, * d;
	const ase_char_t* p, * end;
	ase_cint_t c;
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
	const ase_char_t *pexp;  /* Temporarily holds location of exponent in string */
	int negative = 0, exp_negative = 0;

	p = str;
	end = str + len;

	/* Strip off leading blanks and check for a sign */
	/*while (ASE_LSP_ISSPACE(lsp,*p)) p++;*/

	/*while (*p != ASE_T('\0')) */
	while (p < end)
	{
		if (*p == ASE_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == ASE_T('+')) p++;
		else break;
	}

	/* Count the number of digits in the mantissa (including the decimal
	 * point), and also locate the decimal point. */
	dec_pt = -1;
	/*for (mant_size = 0; ; mant_size++) */
	for (mant_size = 0; p < end; mant_size++) 
	{
		c = *p;
		if (!ASE_LSP_ISDIGIT (lsp, c)) 
		{
			if (c != ASE_T('.') || dec_pt >= 0) break;
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

	if (mant_size > 18)  /* TODO: is 18 correct for ase_real_t??? */
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
			if (c == ASE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac1 = 10 * frac1 + (c - ASE_T('0'));
		}

		frac2 = 0;
		for (; mant_size > 0; mant_size--) {
			c = *p++;
			if (c == ASE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac2 = 10 * frac2 + (c - ASE_T('0'));
		}
		fraction = (1.0e9 * frac1) + frac2;
	}

	/* Skim off the exponent */
	p = pexp;
	if (p < end && (*p == ASE_T('E') || *p == ASE_T('e'))) 
	{
		p++;

		if (p < end) 
		{
			if (*p == ASE_T('-')) 
			{
				exp_negative = 1;
				p++;
			} 
			else 
			{
				if (*p == ASE_T('+')) p++;
				exp_negative = 0;
			}
		}
		else exp_negative = 0;

		if (!(p < end && ASE_LSP_ISDIGIT (lsp, *p))) 
		{
			/*p = pexp;*/
			/*goto done;*/
			goto no_exp;
		}

		while (p < end && ASE_LSP_ISDIGIT (lsp, *p)) 
		{
			exp = exp * 10 + (*p - ASE_T('0'));
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
	if (endptr != ASE_NULL) *endptr = p;
	return (negative)? -fraction: fraction;
}

ase_size_t ase_lsp_longtostr (
	ase_long_t value, int radix, const ase_char_t* prefix, 
	ase_char_t* buf, ase_size_t size)
{
	ase_long_t t, rem;
	ase_size_t len, ret, i;
	ase_size_t prefix_len;

	prefix_len = (prefix != ASE_NULL)? ase_strlen(prefix): 0;

	t = value;
	if (t == 0)
	{
		/* zero */
		if (buf == ASE_NULL) return prefix_len + 1;

		if (size < prefix_len+1) 
		{
			/* buffer too small */
			return (ase_size_t)-1;
		}

		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
		buf[prefix_len] = ASE_T('0');
		if (size > prefix_len+1) buf[prefix_len+1] = ASE_T('\0');
		return 1;
	}

	/* non-zero values */
	len = prefix_len;
	if (t < 0) { t = -t; len++; }
	while (t > 0) { len++; t /= radix; }

	if (buf == ASE_NULL)
	{
		/* if buf is not given, return the number of bytes required */
		return len;
	}

	if (size < len) return (ase_size_t)-1; /* buffer too small */
	if (size > len) buf[len] = ASE_T('\0');
	ret = len;

	t = value;
	if (t < 0) t = -t;

	while (t > 0) 
	{
		rem = t % radix;
		if (rem >= 10)
			buf[--len] = (ase_char_t)rem + ASE_T('a') - 10;
		else
			buf[--len] = (ase_char_t)rem + ASE_T('0');
		t /= radix;
	}

	if (value < 0) 
	{
		for (i = 1; i <= prefix_len; i++) 
		{
			buf[i] = prefix[i-1];
			len--;
		}
		buf[--len] = ASE_T('-');
	}
	else
	{
		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
	}

	return ret;
}

