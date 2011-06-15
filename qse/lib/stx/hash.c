/*
 * $Id$
 */

#include "stx.h"
#include <qse/cmn/str.h>

qse_word_t qse_stx_hashbytes (qse_stx_t* stx, const void* data, qse_word_t len)
{
	qse_word_t h = 0;
	qse_byte_t* bp, * be;

	bp = (qse_byte_t*)data; be = bp + len;
	while (bp < be) h = h * 31 + *bp++;

	return h;
}

qse_word_t qse_stx_hashstr (qse_stx_t* stx, const qse_char_t* str)
{
	qse_word_t h = 0;
	qse_byte_t* bp, * be;
	const qse_char_t* p = str;

	while (*p != QSE_T('\0')) 
	{
		bp = (qse_byte_t*)p;
		be = bp + QSE_SIZEOF(qse_char_t);
		while (bp < be) h = h * 31 + *bp++;
		p++;
	}

	return h;
}

qse_word_t qse_stx_hashstrx (qse_stx_t* stx, const qse_char_t* str, qse_word_t len)
{
	return qse_stx_hashbytes (stx, str, len * QSE_SIZEOF(*str));
}

qse_word_t qse_stx_hashobj (qse_stx_t* stx, qse_word_t ref)
{
	qse_word_t hv;

	if (REFISINT(stx, ref)) 
	{
		qse_word_t tmp = REFTOINT(stx, ref);
		hv = qse_stx_hashbytes (stx, &tmp, QSE_SIZEOF(tmp));
	}
	else
	{
		switch (OBJTYPE(stx,ref))
		{
			case BYTEOBJ:
				hv = qse_stx_hashbytes (
					stx,
					&BYTEAT(stx,ref,0),
					OBJSIZE(stx,ref)
				);
				break;

			case CHAROBJ:
				/* the additional null is not taken into account */
				hv = qse_stx_hashbytes (
					stx,
					&CHARAT(stx,ref,0),
					OBJSIZE(stx,ref) * QSE_SIZEOF(qse_char_t)
				);
				break;

			case WORDOBJ:
				hv = qse_stx_hashbytes (
					stx, 
					&WORDAT(stx,ref,0),
					OBJSIZE(stx,ref) * QSE_SIZEOF(qse_word_t)
				);
				break;

			default:		
				QSE_ASSERT (
					!"This should never happen"
				);
				break;
		}
	}

	return hv;
}

#if 0
qse_char_t* qse_stx_strword (
	const qse_char_t* str, const qse_char_t* word, qse_word_t* word_index)
{
	qse_char_t* p = (qse_char_t*)str;
	qse_char_t* tok;
	qse_size_t len;
	qse_word_t index = 0;

	while (p != QSE_NULL) 
	{
		p = qse_strtok (p, QSE_T(""), &tok, &len);
		if (qse_strxcmp (tok, len, word) == 0) 
		{
			*word_index = index;
			return tok;
		}

		index++;
	}

	*word_index = index;
	return QSE_NULL;
}
#endif
