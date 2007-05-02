/*
 * $Id: misc.c,v 1.3 2007/04/30 08:32:41 bacon Exp $
 */

#include <ase/stx/misc.h>

ase_word_t ase_stx_hash (const void* data, ase_word_t len)
{
	ase_word_t h = 0;
	ase_byte_t* bp, * be;

	bp = (ase_byte_t*)data; be = bp + len;
	while (bp < be) h = h * 31 + *bp++;

	return h;
}

ase_word_t ase_stx_strhash (const ase_char_t* str)
{
	ase_word_t h = 0;
	ase_byte_t* bp, * be;
	const ase_char_t* p = str;

	while (*p != ASE_T('\0')) {
		bp = (ase_byte_t*)p;
		be = bp + ase_sizeof(ase_char_t);
		while (bp < be) h = h * 31 + *bp++;
		p++;
	}

	return h;
}

ase_word_t ase_stx_strxhash (const ase_char_t* str, ase_word_t len)
{
	ase_word_t h = 0;
	ase_byte_t* bp, * be;
	const ase_char_t* p = str, * end = str + len;

	while (p < end) {
		bp = (ase_byte_t*)p;
		be = bp + ase_sizeof(ase_char_t);
		while (bp < be) h = h * 31 + *bp++;
		p++;
	}

	return h;
}

ase_char_t* ase_stx_strword (
	const ase_char_t* str, const ase_char_t* word, ase_word_t* word_index)
{
	ase_char_t* p = (ase_char_t*)str;
	ase_char_t* tok;
	ase_size_t len;
	ase_word_t index = 0;

	while (p != ASE_NULL) {
		p = ase_strtok (p, ASE_T(""), &tok, &len);
		if (ase_strxcmp (tok, len, word) == 0) {
			*word_index = index;
			return tok;
		}

		index++;
	}

	*word_index = index;
	return ASE_NULL;
}
