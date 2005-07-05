/*
 * $Id: misc.c,v 1.7 2005-07-05 11:15:51 bacon Exp $
 */

#include <xp/stx/misc.h>

xp_word_t xp_stx_hash (const void* data, xp_word_t len)
{
	xp_word_t h = 0;
	xp_byte_t* bp, * be;

	bp = (xp_byte_t*)data; be = bp + len;
	while (bp < be) h = h * 31 + *bp++;

	return h;
}

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

xp_char_t* xp_stx_strword (
	const xp_char_t* str, const xp_char_t* word, xp_word_t* word_index)
{
	xp_char_t* p = (xp_char_t*)str;
	xp_char_t* tok;
	xp_size_t len;
	xp_word_t index = 0;

	while (p != XP_NULL) {
		p = xp_strtok (p, XP_TEXT(""), &tok, &len);
		if (xp_strxcmp (tok, len, word) == 0) {
			*word_index = index;
			return tok;
		}

		index++;
	}

	*word_index = index;
	return XP_NULL;
}
