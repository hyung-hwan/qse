/*
 * $Id: misc.c,v 1.1 2006-04-04 16:03:14 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef __STAND_ALONE
#include <xp/bas/ctype.h>
#include <xp/bas/assert.h>
#endif

xp_long_t xp_awk_strtolong (const xp_char_t* str, int base)
{
	xp_long_t n = 0;
	const xp_char_t* p;
	int digit, sign = 0;

	xp_assert (base < 37); 

	p = str; while (xp_isspace(*p)) p++;

	while (*p != XP_CHAR('\0')) 
	{
		if (*p == XP_CHAR('-')) 
		{
			sign = ~sign;
			p++;
		}
		else if (*p == XP_CHAR('+')) p++;
		else break;
	}

	if (base == 0) 
	{
		if (*p == XP_CHAR('0')) 
		{
			p++;
			if (*p == XP_CHAR('x'))
			{
				p++; base = 16;
			} 
			else base = 8;
		}
		else base = 10;
	} 
	else if (base == 16) 
	{
		// skip a leading "0x" from hex numbers.
		if ((*p == XP_CHAR('0')) && (*(p+1) == XP_CHAR('x'))) 
		{ 
			p += 2; 
		}
	}

	while (*p != XP_CHAR('\0'))
	{
		if (*p >= XP_CHAR('0') && *p <= XP_CHAR('9'))
			digit = *p - XP_CHAR('0');
		else if (*p >= XP_CHAR('A') && *p <= XP_CHAR('Z'))
			digit = *p - XP_CHAR('A') + 10;
		else if (*p >= XP_CHAR('a') && *p <= XP_CHAR('z'))
			digit = *p - XP_CHAR('a') + 10;
		else break;

		if (digit >= base) break;
		n = n * base + digit;

		p++;
	}

	return (sign)? -n: n;
}

xp_real_t xp_awk_strtoreal (const xp_char_t* str)
{
	/* TODO: */
	return (xp_real_t)0.0;
}

