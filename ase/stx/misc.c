/*
 * $Id: misc.c,v 1.4 2005-06-08 16:11:18 bacon Exp $
 */

#include <xp/stx/misc.h>

xp_word_t xp_stx_strhash (const xp_char_t* str)
{
	xp_word_t h = 0;
	xp_byte_t* bp, * be;
	const xp_char_t* p = str;

	while (*p != XP_CHAR('\0')) {
		bp = (xp_byte_t*)p;
		be = bp + xp_sizeof(xp_char_t);
		while (bp < be) h = h * 31 + *bp++;
		p++;
	}

	return h;
}

xp_word_t xp_stx_strxhash (const xp_char_t* str, xp_word_t len)
{
	xp_word_t h = 0;
	xp_byte_t* bp, * be;
	const xp_char_t* p = str, * end = str + len;

	while (p < end) {
		bp = (xp_byte_t*)p;
		be = bp + xp_sizeof(xp_char_t);
		while (bp < be) h = h * 31 + *bp++;
		p++;
	}

	return h;
}

