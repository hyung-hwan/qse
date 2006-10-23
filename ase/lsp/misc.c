/*
 * $Id: misc.c,v 1.1 2006-10-23 14:42:38 bacon Exp $
 */

#include <sse/lsp/lsp_i.h>

void* sse_lsp_memcpy  (void* dst, const void* src, sse_size_t n)
{
	void* p = dst;
	void* e = (sse_byte_t*)dst + n;

	while (dst < e) 
	{
		*(sse_byte_t*)dst = *(sse_byte_t*)src;
		dst = (sse_byte_t*)dst + 1;
		src = (sse_byte_t*)src + 1;
	}

	return p;
}

void* sse_lsp_memset (void* dst, int val, sse_size_t n)
{
	void* p = dst;
	void* e = (sse_byte_t*)p + n;

	while (p < e) 
	{
		*(sse_byte_t*)p = (sse_byte_t)val;
		p = (sse_byte_t*)p + 1;
	}

	return dst;
}

sse_long_t sse_lsp_strxtolong (
	sse_lsp_t* lsp, const sse_char_t* str, sse_size_t len,
	int base, const sse_char_t** endptr)
{
	sse_long_t n = 0;
	const sse_char_t* p;
	const sse_char_t* end;
	sse_size_t rem;
	int digit, negative = 0;

	sse_lsp_assert (lsp, base < 37); 

	p = str; 
	end = str + len;
	
	/* strip off leading spaces */
	/*while (SSE_LSP_ISSPACE(lsp,*p)) p++;*/

	/* check for a sign */
	/*while (*p != SSE_T('\0')) */
	while (p < end)
	{
		if (*p == SSE_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == SSE_T('+')) p++;
		else break;
	}

	/* check for a binary/octal/hexadecimal notation */
	rem = end - p;
	if (base == 0) 
	{
		if (rem >= 1 && *p == SSE_T('0')) 
		{
			p++;

			if (rem == 1) base = 8;
			else if (*p == SSE_T('x') || *p == SSE_T('X'))
			{
				p++; base = 16;
			} 
			else if (*p == SSE_T('b') || *p == SSE_T('B'))
			{
				p++; base = 2;
			}
			else base = 8;
		}
		else base = 10;
	} 
	else if (rem >= 2 && base == 16)
	{
		if (*p == SSE_T('0') && 
		    (*(p+1) == SSE_T('x') || *(p+1) == SSE_T('X'))) p += 2; 
	}
	else if (rem >= 2 && base == 2)
	{
		if (*p == SSE_T('0') && 
		    (*(p+1) == SSE_T('b') || *(p+1) == SSE_T('B'))) p += 2; 
	}

	/* process the digits */
	/*while (*p != SSE_T('\0'))*/
	while (p < end)
	{
		if (*p >= SSE_T('0') && *p <= SSE_T('9'))
			digit = *p - SSE_T('0');
		else if (*p >= SSE_T('A') && *p <= SSE_T('Z'))
			digit = *p - SSE_T('A') + 10;
		else if (*p >= SSE_T('a') && *p <= SSE_T('z'))
			digit = *p - SSE_T('a') + 10;
		else break;

		if (digit >= base) break;
		n = n * base + digit;

		p++;
	}

	if (endptr != SSE_NULL) *endptr = p;
	return (negative)? -n: n;
}


/*
 * sse_lsp_strtoreal is almost a replica of strtod.
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

sse_real_t sse_lsp_strtoreal (sse_lsp_t* lsp, const sse_char_t* str)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static sse_real_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	sse_real_t fraction, dbl_exp, * d;
	const sse_char_t* p;
	sse_cint_t c;
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
	const sse_char_t *pexp;  /* Temporarily holds location of exponent in string */
	int negative = 0, exp_negative = 0;

	p = str;

	/* strip off leading blanks */ 
	/*while (SSE_LSP_ISSPACE(lsp,*p)) p++;*/

	/* check for a sign */
	while (*p != SSE_T('\0')) 
	{
		if (*p == SSE_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == SSE_T('+')) p++;
		else break;
	}

	/* Count the number of digits in the mantissa (including the decimal
	 * point), and also locate the decimal point. */
	dec_pt = -1;
	for (mant_size = 0; ; mant_size++) 
	{
		c = *p;
		if (!SSE_LSP_ISDIGIT (lsp, c)) 
		{
			if ((c != SSE_T('.')) || (dec_pt >= 0)) break;
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
			if (c == SSE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac1 = 10 * frac1 + (c - SSE_T('0'));
		}
		frac2 = 0;
		for (; mant_size > 0; mant_size--) {
			c = *p;
			p++;
			if (c == SSE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac2 = 10*frac2 + (c - SSE_T('0'));
		}
		fraction = (1.0e9 * frac1) + frac2;
	}

	/* Skim off the exponent */
	p = pexp;
	if ((*p == SSE_T('E')) || (*p == SSE_T('e'))) 
	{
		p++;
		if (*p == SSE_T('-')) 
		{
			exp_negative = 1;
			p++;
		} 
		else 
		{
			if (*p == SSE_T('+')) p++;
			exp_negative = 0;
		}
		if (!SSE_LSP_ISDIGIT (lsp, *p)) 
		{
			/* p = pexp; */
			/* goto done; */
			goto no_exp;
		}
		while (SSE_LSP_ISDIGIT (lsp, *p)) 
		{
			exp = exp * 10 + (*p - SSE_T('0'));
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

sse_real_t sse_lsp_strxtoreal (
	sse_lsp_t* lsp, const sse_char_t* str, sse_size_t len, 
	const sse_char_t** endptr)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static sse_real_t powers_of_10[] = 
	{
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	sse_real_t fraction, dbl_exp, * d;
	const sse_char_t* p, * end;
	sse_cint_t c;
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
	const sse_char_t *pexp;  /* Temporarily holds location of exponent in string */
	int negative = 0, exp_negative = 0;

	p = str;
	end = str + len;

	/* Strip off leading blanks and check for a sign */
	/*while (SSE_LSP_ISSPACE(lsp,*p)) p++;*/

	/*while (*p != SSE_T('\0')) */
	while (p < end)
	{
		if (*p == SSE_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == SSE_T('+')) p++;
		else break;
	}

	/* Count the number of digits in the mantissa (including the decimal
	 * point), and also locate the decimal point. */
	dec_pt = -1;
	/*for (mant_size = 0; ; mant_size++) */
	for (mant_size = 0; p < end; mant_size++) 
	{
		c = *p;
		if (!SSE_LSP_ISDIGIT (lsp, c)) 
		{
			if (c != SSE_T('.') || dec_pt >= 0) break;
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

	if (mant_size > 18)  /* TODO: is 18 correct for sse_real_t??? */
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
			if (c == SSE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac1 = 10 * frac1 + (c - SSE_T('0'));
		}

		frac2 = 0;
		for (; mant_size > 0; mant_size--) {
			c = *p++;
			if (c == SSE_T('.')) 
			{
				c = *p;
				p++;
			}
			frac2 = 10 * frac2 + (c - SSE_T('0'));
		}
		fraction = (1.0e9 * frac1) + frac2;
	}

	/* Skim off the exponent */
	p = pexp;
	if (p < end && (*p == SSE_T('E') || *p == SSE_T('e'))) 
	{
		p++;

		if (p < end) 
		{
			if (*p == SSE_T('-')) 
			{
				exp_negative = 1;
				p++;
			} 
			else 
			{
				if (*p == SSE_T('+')) p++;
				exp_negative = 0;
			}
		}
		else exp_negative = 0;

		if (!(p < end && SSE_LSP_ISDIGIT (lsp, *p))) 
		{
			/*p = pexp;*/
			/*goto done;*/
			goto no_exp;
		}

		while (p < end && SSE_LSP_ISDIGIT (lsp, *p)) 
		{
			exp = exp * 10 + (*p - SSE_T('0'));
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
	if (endptr != SSE_NULL) *endptr = p;
	return (negative)? -fraction: fraction;
}

sse_size_t sse_lsp_longtostr (
	sse_long_t value, int radix, const sse_char_t* prefix, 
	sse_char_t* buf, sse_size_t size)
{
	sse_long_t t, rem;
	sse_size_t len, ret, i;
	sse_size_t prefix_len;

	prefix_len = (prefix != SSE_NULL)? sse_lsp_strlen(prefix): 0;

	t = value;
	if (t == 0)
	{
		/* zero */
		if (buf == SSE_NULL) return prefix_len + 1;

		if (size < prefix_len+1) 
		{
			/* buffer too small */
			return (sse_size_t)-1;
		}

		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
		buf[prefix_len] = SSE_T('0');
		if (size > prefix_len+1) buf[prefix_len+1] = SSE_T('\0');
		return 1;
	}

	/* non-zero values */
	len = prefix_len;
	if (t < 0) { t = -t; len++; }
	while (t > 0) { len++; t /= radix; }

	if (buf == SSE_NULL)
	{
		/* if buf is not given, return the number of bytes required */
		return len;
	}

	if (size < len) return (sse_size_t)-1; /* buffer too small */
	if (size > len) buf[len] = SSE_T('\0');
	ret = len;

	t = value;
	if (t < 0) t = -t;

	while (t > 0) 
	{
		rem = t % radix;
		if (rem >= 10)
			buf[--len] = (sse_char_t)rem + SSE_T('a') - 10;
		else
			buf[--len] = (sse_char_t)rem + SSE_T('0');
		t /= radix;
	}

	if (value < 0) 
	{
		for (i = 1; i <= prefix_len; i++) 
		{
			buf[i] = prefix[i-1];
			len--;
		}
		buf[--len] = SSE_T('-');
	}
	else
	{
		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
	}

	return ret;
}

sse_char_t* sse_lsp_strdup (sse_lsp_t* lsp, const sse_char_t* str)
{
	sse_char_t* tmp;

	tmp = (sse_char_t*) SSE_LSP_MALLOC (
		lsp, (sse_lsp_strlen(str) + 1) * sse_sizeof(sse_char_t));
	if (tmp == SSE_NULL) return SSE_NULL;

	sse_lsp_strcpy (tmp, str);
	return tmp;
}

sse_char_t* sse_lsp_strxdup (sse_lsp_t* lsp, const sse_char_t* str, sse_size_t len)
{
	sse_char_t* tmp;

	tmp = (sse_char_t*) SSE_LSP_MALLOC (
		lsp, (len + 1) * sse_sizeof(sse_char_t));
	if (tmp == SSE_NULL) return SSE_NULL;

	sse_lsp_strncpy (tmp, str, len);
	return tmp;
}

sse_char_t* sse_lsp_strxdup2 (
	sse_lsp_t* lsp,
	const sse_char_t* str1, sse_size_t len1,
	const sse_char_t* str2, sse_size_t len2)
{
	sse_char_t* tmp;

	tmp = (sse_char_t*) SSE_LSP_MALLOC (
		lsp, (len1 + len2 + 1) * sse_sizeof(sse_char_t));
	if (tmp == SSE_NULL) return SSE_NULL;

	sse_lsp_strncpy (tmp, str1, len1);
	sse_lsp_strncpy (tmp + len1, str2, len2);
	return tmp;
}

sse_size_t sse_lsp_strlen (const sse_char_t* str)
{
	const sse_char_t* p = str;
	while (*p != SSE_T('\0')) p++;
	return p - str;
}

sse_size_t sse_lsp_strcpy (sse_char_t* buf, const sse_char_t* str)
{
	sse_char_t* org = buf;
	while ((*buf++ = *str++) != SSE_T('\0'));
	return buf - org - 1;
}

sse_size_t sse_lsp_strncpy (sse_char_t* buf, const sse_char_t* str, sse_size_t len)
{
	const sse_char_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = SSE_T('\0');
	return len;
}

int sse_lsp_strcmp (const sse_char_t* s1, const sse_char_t* s2)
{
	while (*s1 == *s2) 
	{
		if (*s1 == SSE_C('\0')) return 0;
		s1++, s2++;
	}

	return (*s1 > *s2)? 1: -1;
}

int sse_lsp_strxncmp (
	const sse_char_t* s1, sse_size_t len1, 
	const sse_char_t* s2, sse_size_t len2)
{
	sse_char_t c1, c2;
	const sse_char_t* end1 = s1 + len1;
	const sse_char_t* end2 = s2 + len2;

	while (s1 < end1)
	{
		c1 = *s1;
		if (s2 < end2) 
		{
			c2 = *s2;
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

int sse_lsp_strxncasecmp (
	sse_lsp_t* lsp,
	const sse_char_t* s1, sse_size_t len1, 
	const sse_char_t* s2, sse_size_t len2)
{
	sse_char_t c1, c2;
	const sse_char_t* end1 = s1 + len1;
	const sse_char_t* end2 = s2 + len2;

	while (s1 < end1)
	{
		c1 = SSE_LSP_TOUPPER (lsp, *s1); 
		if (s2 < end2) 
		{
			c2 = SSE_LSP_TOUPPER (lsp, *s2);
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

sse_char_t* sse_lsp_strxnstr (
	const sse_char_t* str, sse_size_t strsz, 
	const sse_char_t* sub, sse_size_t subsz)
{
	const sse_char_t* end, * subp;

	if (subsz == 0) return (sse_char_t*)str;
	if (strsz < subsz) return SSE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) {
		const sse_char_t* x = str;
		const sse_char_t* y = sub;

		while (sse_true) {
			if (y >= subp) return (sse_char_t*)str;
			if (*x != *y) break;
			x++; y++;
		}	

		str++;
	}
		
	return SSE_NULL;
}

int sse_lsp_abort (sse_lsp_t* lsp, 
	const sse_char_t* expr, const sse_char_t* file, int line)
{
	lsp->syscas.dprintf (
		SSE_T("ASSERTION FAILURE AT FILE %s, LINE %d\n%s\n"),
		file, line, expr);
	lsp->syscas.abort ();
	return 0;
}
