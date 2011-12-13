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

qse_word_t qse_stx_hashstrn (
	qse_stx_t* stx, const qse_char_t* str, qse_word_t len)
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
					BYTEPTR(stx,ref),
					BYTELEN(stx,ref)
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
					!"This must never happen"
				);
				break;
		}
	}

	return hv;
}
