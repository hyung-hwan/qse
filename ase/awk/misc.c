/*
 * $Id: misc.c,v 1.36 2006-10-31 10:13:14 bacon Exp $
 */

#include <ase/awk/awk_i.h>

void* ase_awk_memcpy  (void* dst, const void* src, ase_size_t n)
{
	void* p = dst;
	void* e = (ase_byte_t*)dst + n;

	while (dst < e) 
	{
		*(ase_byte_t*)dst = *(ase_byte_t*)src;
		dst = (ase_byte_t*)dst + 1;
		src = (ase_byte_t*)src + 1;
	}

	return p;
}

void* ase_awk_memset (void* dst, int val, ase_size_t n)
{
	void* p = dst;
	void* e = (ase_byte_t*)p + n;

	while (p < e) 
	{
		*(ase_byte_t*)p = (ase_byte_t)val;
		p = (ase_byte_t*)p + 1;
	}

	return dst;
}

ase_long_t ase_awk_strxtolong (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len,
	int base, const ase_char_t** endptr)
{
	ase_long_t n = 0;
	const ase_char_t* p;
	const ase_char_t* end;
	ase_size_t rem;
	int digit, negative = 0;

	ASE_AWK_ASSERT (awk, base < 37); 

	p = str; 
	end = str + len;
	
	/* strip off leading spaces */
	/*while (ASE_AWK_ISSPACE(awk,*p)) p++;*/

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
 * ase_awk_strtoreal is almost a replica of strtod.
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

ase_real_t ase_awk_strtoreal (ase_awk_t* awk, const ase_char_t* str)
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
	/*while (ASE_AWK_ISSPACE(awk,*p)) p++;*/

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
		if (!ASE_AWK_ISDIGIT (awk, c)) 
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
		if (!ASE_AWK_ISDIGIT (awk, *p)) 
		{
			/* p = pexp; */
			/* goto done; */
			goto no_exp;
		}
		while (ASE_AWK_ISDIGIT (awk, *p)) 
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

ase_real_t ase_awk_strxtoreal (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len, 
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
	/*while (ASE_AWK_ISSPACE(awk,*p)) p++;*/

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
		if (!ASE_AWK_ISDIGIT (awk, c)) 
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

		if (!(p < end && ASE_AWK_ISDIGIT (awk, *p))) 
		{
			/*p = pexp;*/
			/*goto done;*/
			goto no_exp;
		}

		while (p < end && ASE_AWK_ISDIGIT (awk, *p)) 
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

ase_size_t ase_awk_longtostr (
	ase_long_t value, int radix, const ase_char_t* prefix, 
	ase_char_t* buf, ase_size_t size)
{
	ase_long_t t, rem;
	ase_size_t len, ret, i;
	ase_size_t prefix_len;

	prefix_len = (prefix != ASE_NULL)? ase_awk_strlen(prefix): 0;

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

ase_char_t* ase_awk_strdup (ase_awk_t* awk, const ase_char_t* str)
{
	ase_char_t* tmp;

	tmp = (ase_char_t*) ASE_AWK_MALLOC (
		awk, (ase_awk_strlen(str) + 1) * ase_sizeof(ase_char_t));
	if (tmp == ASE_NULL) return ASE_NULL;

	ase_awk_strcpy (tmp, str);
	return tmp;
}

ase_char_t* ase_awk_strxdup (ase_awk_t* awk, const ase_char_t* str, ase_size_t len)
{
	ase_char_t* tmp;

	tmp = (ase_char_t*) ASE_AWK_MALLOC (
		awk, (len + 1) * ase_sizeof(ase_char_t));
	if (tmp == ASE_NULL) return ASE_NULL;

	ase_awk_strncpy (tmp, str, len);
	return tmp;
}

ase_char_t* ase_awk_strxdup2 (
	ase_awk_t* awk,
	const ase_char_t* str1, ase_size_t len1,
	const ase_char_t* str2, ase_size_t len2)
{
	ase_char_t* tmp;

	tmp = (ase_char_t*) ASE_AWK_MALLOC (
		awk, (len1 + len2 + 1) * ase_sizeof(ase_char_t));
	if (tmp == ASE_NULL) return ASE_NULL;

	ase_awk_strncpy (tmp, str1, len1);
	ase_awk_strncpy (tmp + len1, str2, len2);
	return tmp;
}

ase_size_t ase_awk_strlen (const ase_char_t* str)
{
	const ase_char_t* p = str;
	while (*p != ASE_T('\0')) p++;
	return p - str;
}

ase_size_t ase_awk_strcpy (ase_char_t* buf, const ase_char_t* str)
{
	ase_char_t* org = buf;
	while ((*buf++ = *str++) != ASE_T('\0'));
	return buf - org - 1;
}

ase_size_t ase_awk_strncpy (ase_char_t* buf, const ase_char_t* str, ase_size_t len)
{
	const ase_char_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = ASE_T('\0');
	return len;
}

int ase_awk_strcmp (const ase_char_t* s1, const ase_char_t* s2)
{
	while (*s1 == *s2) 
	{
		if (*s1 == ASE_C('\0')) return 0;
		s1++, s2++;
	}

	return (*s1 > *s2)? 1: -1;
}

int ase_awk_strxncmp (
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2)
{
	ase_char_t c1, c2;
	const ase_char_t* end1 = s1 + len1;
	const ase_char_t* end2 = s2 + len2;

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

int ase_awk_strxncasecmp (
	ase_awk_t* awk,
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2)
{
	ase_char_t c1, c2;
	const ase_char_t* end1 = s1 + len1;
	const ase_char_t* end2 = s2 + len2;

	while (s1 < end1)
	{
		c1 = ASE_AWK_TOUPPER (awk, *s1); 
		if (s2 < end2) 
		{
			c2 = ASE_AWK_TOUPPER (awk, *s2);
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

ase_char_t* ase_awk_strxnstr (
	const ase_char_t* str, ase_size_t strsz, 
	const ase_char_t* sub, ase_size_t subsz)
{
	const ase_char_t* end, * subp;

	if (subsz == 0) return (ase_char_t*)str;
	if (strsz < subsz) return ASE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) {
		const ase_char_t* x = str;
		const ase_char_t* y = sub;

		while (ase_true) {
			if (y >= subp) return (ase_char_t*)str;
			if (*x != *y) break;
			x++; y++;
		}	

		str++;
	}
		
	return ASE_NULL;
}

ase_char_t* ase_awk_strtok (
	ase_awk_run_t* run, const ase_char_t* s, 
	const ase_char_t* delim, ase_char_t** tok, ase_size_t* tok_len)
{
	return ase_awk_strxntok (
		run, s, ase_awk_strlen(s), 
		delim, ase_awk_strlen(delim), tok, tok_len);
}

ase_char_t* ase_awk_strxtok (
	ase_awk_run_t* run, const ase_char_t* s, ase_size_t len,
	const ase_char_t* delim, ase_char_t** tok, ase_size_t* tok_len)
{
	return ase_awk_strxntok (
		run, s, len, 
		delim, ase_awk_strlen(delim), tok, tok_len);
}

ase_char_t* ase_awk_strntok (
	ase_awk_run_t* run, const ase_char_t* s, 
	const ase_char_t* delim, ase_size_t delim_len,
	ase_char_t** tok, ase_size_t* tok_len)
{
	return ase_awk_strxntok (
		run, s, ase_awk_strlen(s), 
		delim, delim_len, tok, tok_len);
}

ase_char_t* ase_awk_strxntok (
	ase_awk_run_t* run, const ase_char_t* s, ase_size_t len,
	const ase_char_t* delim, ase_size_t delim_len, 
	ase_char_t** tok, ase_size_t* tok_len)
{
	const ase_char_t* p = s, *d;
	const ase_char_t* end = s + len;	
	const ase_char_t* sp = ASE_NULL, * ep = ASE_NULL;
	const ase_char_t* delim_end = delim + delim_len;
	ase_char_t c; 
	int delim_mode;

#define __DELIM_NULL      0
#define __DELIM_EMPTY     1
#define __DELIM_SPACES    2
#define __DELIM_NOSPACES  3
#define __DELIM_COMPOSITE 4
	if (delim == ASE_NULL) delim_mode = __DELIM_NULL;
	else 
	{
		delim_mode = __DELIM_EMPTY;

		for (d = delim; d < delim_end; d++) 
		{
			if (ASE_AWK_ISSPACE(run->awk,*d)) 
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
	}		
	
	if (delim_mode == __DELIM_NULL) 
	{ 
		/* when ASE_NULL is given as "delim", it trims off the 
		 * leading and trailing spaces characters off the source
		 * string "s" eventually. */

		while (p < end && ASE_AWK_ISSPACE(run->awk,*p)) p++;
		while (p < end) 
		{
			c = *p;

			if (!ASE_AWK_ISSPACE(run->awk,c)) 
			{
				if (sp == ASE_NULL) sp = p;
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

		while (p < end && ASE_AWK_ISSPACE(run->awk,*p)) p++;
		while (p < end) 
		{
			c = *p;
			if (ASE_AWK_ISSPACE(run->awk,c)) break;
			if (sp == ASE_NULL) sp = p;
			ep = p++;
		}
		while (p < end && ASE_AWK_ISSPACE(run->awk,*p)) p++;
	}
	else if (delim_mode == __DELIM_NOSPACES)
	{
		/* each token is delimited by one of charaters 
		 * in the delimeter set "delim". */
		if (run->global.ignorecase)
		{
			while (p < end) 
			{
				c = ASE_AWK_TOUPPER(run->awk, *p);
				for (d = delim; d < delim_end; d++) 
				{
					if (c == ASE_AWK_TOUPPER(run->awk,*d)) goto exit_loop;
				}

				if (sp == ASE_NULL) sp = p;
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

				if (sp == ASE_NULL) sp = p;
				ep = p++;
			}
		}
	}
	else /* if (delim_mode == __DELIM_COMPOSITE) */ 
	{
		/* each token is delimited by one of non-space charaters
		 * in the delimeter set "delim". however, all space characters
		 * surrounding the token are removed */
		while (p < end && ASE_AWK_ISSPACE(run->awk,*p)) p++;
		if (run->global.ignorecase)
		{
			while (p < end) 
			{
				c = ASE_AWK_TOUPPER(run->awk, *p);
				if (ASE_AWK_ISSPACE(run->awk,c)) 
				{
					p++;
					continue;
				}
				for (d = delim; d < delim_end; d++) 
				{
					if (c == ASE_AWK_TOUPPER(run->awk,*d)) goto exit_loop;
				}
				if (sp == ASE_NULL) sp = p;
				ep = p++;
			}
		}
		else
		{
			while (p < end) 
			{
				c = *p;
				if (ASE_AWK_ISSPACE(run->awk,c)) 
				{
					p++;
					continue;
				}
				for (d = delim; d < delim_end; d++) 
				{
					if (c == *d) goto exit_loop;
				}
				if (sp == ASE_NULL) sp = p;
				ep = p++;
			}
		}
	}

exit_loop:
	if (sp == ASE_NULL) 
	{
		*tok = ASE_NULL;
		*tok_len = (ase_size_t)0;
	}
	else 
	{
		*tok = (ase_char_t*)sp;
		*tok_len = ep - sp + 1;
	}

	/* if ASE_NULL is returned, this function should not be called anymore */
	if (p >= end) return ASE_NULL;
	if (delim_mode == __DELIM_EMPTY || 
	    delim_mode == __DELIM_SPACES) return (ase_char_t*)p;
	return (ase_char_t*)++p;
}

ase_char_t* ase_awk_strxntokbyrex (
	ase_awk_run_t* run, const ase_char_t* s, ase_size_t len,
	void* rex, ase_char_t** tok, ase_size_t* tok_len, int* errnum)
{
	int n;
	ase_char_t* match_ptr;
	ase_size_t match_len, i;
	ase_size_t left = len;
	const ase_char_t* ptr = s;
	const ase_char_t* str_ptr = s;
	ase_size_t str_len = len;

	while (len > 0)
	{
		n = ase_awk_matchrex (
			run->awk, rex, 
			((run->global.ignorecase)? ASE_AWK_REX_IGNORECASE: 0),
			ptr, left, (const ase_char_t**)&match_ptr, &match_len, 
			errnum);
		if (n == -1) return ASE_NULL;
		if (n == 0)
		{
			/* no match has been found. 
			 * return the entire string as a token */
			*tok = (ase_char_t*)str_ptr;
			*tok_len = str_len;
			*errnum = ASE_AWK_ENOERR;
			return ASE_NULL; 
		}

		ASE_AWK_ASSERT (run->awk, n == 1);

		if (match_len == 0)
		{
			ptr++;
			left--;
		}
		else if (run->awk->option & ASE_AWK_STRIPSPACES)
		{
			/* match at the beginning of the input string */
			if (match_ptr == s) 
			{
				for (i = 0; i < match_len; i++)
				{
					if (!ASE_AWK_ISSPACE(run->awk, match_ptr[i]))
						goto exit_loop;
				}

				/* the match that are all spaces at the 
				 * beginning of the input string is skipped */
				ptr += match_len;
				left -= match_len;
				str_ptr = s + match_len;
				str_len -= match_len;
			}
			else  break;
		}
		else break;
	}

exit_loop:
	if (len == 0)
	{
		*tok = (ase_char_t*)str_ptr;
		*tok_len = str_len;
		*errnum = ASE_AWK_ENOERR;
		return ASE_NULL; 
	}

	*tok = (ase_char_t*)str_ptr;
	*tok_len = match_ptr - str_ptr;

	for (i = 0; i < match_len; i++)
	{
		if (!ASE_AWK_ISSPACE(run->awk, match_ptr[i]))
		{
			*errnum = ASE_AWK_ENOERR;
			return match_ptr+match_len;
		}
	}

	*errnum = ASE_AWK_ENOERR;

	if (run->awk->option & ASE_AWK_STRIPSPACES)
	{
		return (match_ptr+match_len >= s+len)? 
			ASE_NULL: (match_ptr+match_len);
	}
	else
	{
		return (match_ptr+match_len > s+len)? 
			ASE_NULL: (match_ptr+match_len);
	}
}

int ase_awk_sprintf (ase_awk_t* awk,
	ase_char_t* buf, ase_size_t len, const ase_char_t* fmt, ...)
{
	/* TODO: */
	return -1;
}

int ase_awk_assertfail (ase_awk_t* awk, 
	const ase_char_t* expr, const ase_char_t* desc, 
	const ase_char_t* file, int line)
{
	if (desc == ASE_NULL)
	{
		awk->syscas.aprintf (
			ASE_T("ASSERTION FAILURE AT FILE %s LINE %d\n%s\n"),
			file, line, expr);
	}
	else
	{
		awk->syscas.aprintf (
			ASE_T("ASSERTION FAILURE AT FILE %s LINE %d\n%s\n\nDESCRIPTION:\n%s\n"),
			file, line, expr, desc);

	}
	awk->syscas.abort ();
	return 0;
}
