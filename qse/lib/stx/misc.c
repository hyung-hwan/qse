/*
 * $Id: misc.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/misc.h>

qse_word_t qse_stx_hash (const void* data, qse_word_t len)
{
	qse_word_t h = 0;
	qse_byte_t* bp, * be;

	bp = (qse_byte_t*)data; be = bp + len;
	while (bp < be) h = h * 31 + *bp++;

	return h;
}

qse_word_t qse_stx_strhash (const qse_char_t* str)
{
	qse_word_t h = 0;
	qse_byte_t* bp, * be;
	const qse_char_t* p = str;

	while (*p != QSE_T('\0')) {
		bp = (qse_byte_t*)p;
		be = bp + qse_sizeof(qse_char_t);
		while (bp < be) h = h * 31 + *bp++;
		p++;
	}

	return h;
}

qse_word_t qse_stx_strxhash (const qse_char_t* str, qse_word_t len)
{
	qse_word_t h = 0;
	qse_byte_t* bp, * be;
	const qse_char_t* p = str, * end = str + len;

	while (p < end) {
		bp = (qse_byte_t*)p;
		be = bp + qse_sizeof(qse_char_t);
		while (bp < be) h = h * 31 + *bp++;
		p++;
	}

	return h;
}

qse_char_t* qse_stx_strword (
	const qse_char_t* str, const qse_char_t* word, qse_word_t* word_index)
{
	qse_char_t* p = (qse_char_t*)str;
	qse_char_t* tok;
	qse_size_t len;
	qse_word_t index = 0;

	while (p != QSE_NULL) {
		p = qse_strtok (p, QSE_T(""), &tok, &len);
		if (qse_strxcmp (tok, len, word) == 0) {
			*word_index = index;
			return tok;
		}

		index++;
	}

	*word_index = index;
	return QSE_NULL;
}
