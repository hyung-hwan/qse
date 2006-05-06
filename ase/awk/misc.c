/*
 * $Id: misc.c,v 1.7 2006-05-06 12:52:36 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/ctype.h>
#include <xp/bas/assert.h>
#endif

xp_long_t xp_awk_strtolong (
	const xp_char_t* str, int base, const xp_char_t** endptr)
{
	xp_long_t n = 0;
	const xp_char_t* p;
	int digit, negative = 0;

	xp_assert (base < 37); 

	p = str; while (xp_isspace(*p)) p++;

	while (*p != XP_T('\0')) 
	{
		if (*p == XP_T('-')) 
		{
			negative = ~negative;
			p++;
		}
		else if (*p == XP_T('+')) p++;
		else break;
	}

	if (base == 0) 
	{
		if (*p == XP_T('0')) 
		{
			p++;
			if (*p == XP_T('x') || *p == XP_T('X'))
			{
				p++; base = 16;
			} 
			else if (*p == XP_T('b') || *p == XP_T('B'))
			{
				p++; base = 2;
			}
			else base = 8;
		}
		else base = 10;
	} 
	else if (base == 16) 
	{
		if (*p == XP_T('0') && 
		    (*(p+1) == XP_T('x') || *(p+1) == XP_T('X'))) p += 2; 
	}
	else if (base == 2)
	{
		if (*p == XP_T('0') && 
		    (*(p+1) == XP_T('b') || *(p+1) == XP_T('B'))) p += 2; 
	}

	while (*p != XP_T('\0'))
	{
		if (*p >= XP_T('0') && *p <= XP_T('9'))
			digit = *p - XP_T('0');
		else if (*p >= XP_T('A') && *p <= XP_T('Z'))
			digit = *p - XP_T('A') + 10;
		else if (*p >= XP_T('a') && *p <= XP_T('z'))
			digit = *p - XP_T('a') + 10;
		else break;

		if (digit >= base) break;
		n = n * base + digit;

		p++;
	}

	if (endptr != XP_NULL) *endptr = p;
	return (negative)? -n: n;
}


/*
 * xp_awk_strtoreal is almost a replica of strtod.
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

xp_real_t xp_awk_strtoreal (const xp_char_t* str)
{
	/* 
	 * Table giving binary powers of 10. Entry is 10^2^i.  
	 * Used to convert decimal exponents into floating-point numbers.
	 */ 
	static xp_real_t powersOf10[] = {
		10.,    100.,   1.0e4,   1.0e8,   1.0e16,
		1.0e32, 1.0e64, 1.0e128, 1.0e256
	};

	xp_real_t fraction, dblExp, * d;
	const xp_char_t* p;
	xp_cint_t c;
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
	int mantSize; /* Number of digits in mantissa. */
	int decPt;    /* Number of mantissa digits BEFORE decimal point */
	const xp_char_t *pExp;  /* Temporarily holds location of exponent in string */
	int sign = 0, expSign = 0;

	p = str;

	/* Strip off leading blanks and check for a sign */
	while (xp_isspace(*p)) p++;

	while (*p != XP_T('\0')) 
	{
		if (*p == XP_T('-')) 
		{
			sign = ~sign;
			p++;
		}
		else if (*p == XP_T('+')) p++;
		else break;
	}

	/* Count the number of digits in the mantissa (including the decimal
	 * point), and also locate the decimal point. */
	decPt = -1;
	for (mantSize = 0; ; mantSize++) {
		c = *p;
		if (!xp_isdigit(c)) {
			if ((c != XP_T('.')) || (decPt >= 0)) break;
			decPt = mantSize;
		}
		p++;
	}

	/*
	 * Now suck up the digits in the mantissa.  Use two integers to
	 * collect 9 digits each (this is faster than using floating-point).
	 * If the mantissa has more than 18 digits, ignore the extras, since
	 * they can't affect the value anyway.
	 */
	pExp  = p;
	p -= mantSize;
	if (decPt < 0) 
	{
		decPt = mantSize;
	} 
	else 
	{
		mantSize -= 1;	/* One of the digits was the point */
	}

	if (mantSize > 18) 
	{
		frac_exp = decPt - 18;
		mantSize = 18;
	} 
	else 
	{
		frac_exp = decPt - mantSize;
	}

	if (mantSize == 0) 
	{
		fraction = 0.0;
		/*p = str;*/
		goto done;
	} 
	else 
	{
		int frac1, frac2;
		frac1 = 0;
		for ( ; mantSize > 9; mantSize -= 1) 
		{
			c = *p;
			p++;
			if (c == XP_T('.')) 
			{
				c = *p;
				p++;
			}
			frac1 = 10 * frac1 + (c - XP_T('0'));
		}
		frac2 = 0;
		for (; mantSize > 0; mantSize -= 1) {
			c = *p;
			p++;
			if (c == XP_T('.')) 
			{
				c = *p;
				p++;
			}
			frac2 = 10*frac2 + (c - XP_T('0'));
		}
		fraction = (1.0e9 * frac1) + frac2;
	}

	/* Skim off the exponent */
	p = pExp;
	if ((*p == XP_T('E')) || (*p == XP_T('e'))) 
	{
		p++;
		if (*p == XP_T('-')) 
		{
			expSign = 1;
			p++;
		} 
		else 
		{
			if (*p == XP_T('+')) p++;
			expSign = 0;
		}
		if (!xp_isdigit(*p)) 
		{
			/* p = pExp; */
			goto done;
		}
		while (xp_isdigit(*p)) 
		{
			exp = exp * 10 + (*p - XP_T('0'));
			p++;
		}
	}

	if (expSign) exp = frac_exp - exp;
	else exp = frac_exp + exp;

	/*
	 * Generate a floating-point number that represents the exponent.
	 * Do this by processing the exponent one bit at a time to combine
	 * many powers of 2 of 10. Then combine the exponent with the
	 * fraction.
	 */
	if (exp < 0) 
	{
		expSign = 1;
		exp = -exp;
	} 
	else expSign = 0;

	if (exp > MAX_EXPONENT) exp = MAX_EXPONENT;

	dblExp = 1.0;

	for (d = powersOf10; exp != 0; exp >>= 1, d++) 
	{
		if (exp & 01) dblExp *= *d;
	}

	if (expSign) fraction /= dblExp;
	else fraction *= dblExp;

done:
	return (sign)? -fraction: fraction;
}

