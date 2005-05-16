/*
 * $Id: misc.c,v 1.2 2005-05-16 14:14:34 bacon Exp $
 */

#include <xp/stx/misc.h>

xp_stx_word_t xp_stx_strlen (const xp_stx_char_t* str)
{
	const xp_stx_char_t* p = str;
	while (*p != XP_STX_CHAR('\0')) p++;
	return p - str;
}

int xp_stx_strcmp (const xp_stx_char_t* s1, const xp_stx_char_t* s2)
{
	while (*s1 == *s2 && *s2 != XP_STX_CHAR('\0')) s1++, s2++;
	if (*s1 > *s2) return 1;
	else if (*s1 < *s2) return -1;
	return 0;
}

int xp_stx_strxcmp (
	const xp_stx_char_t* s1, xp_stx_word_t len, const xp_stx_char_t* s2)
{
	const xp_stx_char_t* end = s1 + len;
	while (s1 < end && *s2 != XP_STX_CHAR('\0') && *s1 == *s2) {
		s1++; s2++;
	}
	if (s1 == end && *s2 == XP_STX_CHAR('\0')) return 0;
	if (*s1 == *s2) return (s1 < end)? 1: -1;
	return (*s1 > *s2)? 1: -1;
}

xp_stx_word_t xp_stx_strhash (const xp_stx_char_t* str)
{
	xp_stx_word_t h = 0;
	xp_stx_byte_t* bp, * be;
	const xp_stx_char_t* p = str;

	while (*p != XP_STX_CHAR('\0')) {
		bp = (xp_stx_byte_t*)p;
		be = bp + xp_sizeof(xp_stx_char_t);
		while (bp < be) h = h * 31 + *bp++;
		p++;
	}

	return h;
}

xp_stx_word_t xp_stx_strxhash (const xp_stx_char_t* str, xp_stx_word_t len)
{
	xp_stx_word_t h = 0;
	xp_stx_byte_t* bp, * be;
	const xp_stx_char_t* p = str, * end = str + len;

	while (p < end) {
		bp = (xp_stx_byte_t*)p;
		be = bp + xp_sizeof(xp_stx_char_t);
		while (bp < be) h = h * 31 + *bp++;
		p++;
	}

	return h;
}

