/*
 * $Id: rex.c,v 1.1 2006-07-17 06:21:39 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#endif

xp_awk_rex_t* xp_awk_rex_open (xp_awk_rex_t* rex)
{
	if (rex == XP_NULL)
	{
		rex = (xp_awk_rex_t*) xp_malloc (xp_sizeof(xp_awk_rex_t));
		if (rex == XP_NULL) return XP_NULL;
		rex->__dynamic = xp_true;
	}
	else rex->__dynamic = xp_false;

	return rex;
}

void xp_awk_rex_close (xp_awk_rex_t* rex)
{
	if (rex->__dynamic) xp_free (rex);
}

int xp_awk_rex_compile (const xp_awk_rex_t* rex, const xp_char_t* ptn)
{
	const xp_char_t* p = ptn;
	xp_char_t c;

	while (*p != XP_T('\0'))
	{
		c = *p++; // TODO: backspace escaping...

		if (c == XP_T('|'))
		{
		}

	}

	return -1;
}
