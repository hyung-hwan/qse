/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/utf8.h>

/*
 * from RFC 2279 UTF-8, a transformation format of ISO 10646
 *
 *     UCS-4 range (hex.)  UTF-8 octet sequence (binary)
 * 1:2 00000000-0000007F  0xxxxxxx
 * 2:2 00000080-000007FF  110xxxxx 10xxxxxx
 * 3:2 00000800-0000FFFF  1110xxxx 10xxxxxx 10xxxxxx
 * 4:4 00010000-001FFFFF  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 * inv 00200000-03FFFFFF  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 * inv 04000000-7FFFFFFF  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */

struct __utf8_t
{
	qse_uint32_t  lower;
	qse_uint32_t  upper;
	qse_uint8_t   fbyte;  /* mask to the first utf8 byte */
	qse_uint8_t   mask;
	qse_uint8_t   fmask;
	int           length; /* number of bytes */
};

typedef struct __utf8_t __utf8_t;

static __utf8_t utf8_table[] = 
{
	{0x00000000ul, 0x0000007Ful, 0x00, 0x80, 0x7F, 1},
	{0x00000080ul, 0x000007FFul, 0xC0, 0xE0, 0x1F, 2},
	{0x00000800ul, 0x0000FFFFul, 0xE0, 0xF0, 0x0F, 3},
	{0x00010000ul, 0x001FFFFFul, 0xF0, 0xF8, 0x07, 4},
	{0x00200000ul, 0x03FFFFFFul, 0xF8, 0xFC, 0x03, 5},
	{0x04000000ul, 0x7FFFFFFFul, 0xFC, 0xFE, 0x01, 6}
};

static QSE_INLINE __utf8_t* get_utf8_slot (qse_wchar_t uc)
{
	__utf8_t* cur, * end;

	QSE_ASSERT (QSE_SIZEOF(qse_mchar_t) == 1);
	QSE_ASSERT (QSE_SIZEOF(qse_wchar_t) >= 2);

	end = utf8_table + QSE_COUNTOF(utf8_table);
	cur = utf8_table;

	while (cur < end) 
	{
		if (uc >= cur->lower && uc <= cur->upper) return cur;
		cur++;
	}

	return QSE_NULL; /* invalid character */
}

qse_size_t qse_uctoutf8len (qse_wchar_t uc)
{
	__utf8_t* cur = get_utf8_slot (uc);
	return (cur == QSE_NULL)? 0: (qse_size_t)cur->length;
}

/* wctomb for utf8/unicode */
qse_size_t qse_uctoutf8 (qse_wchar_t uc, qse_mchar_t* utf8, qse_size_t size)
{
	__utf8_t* cur = get_utf8_slot (uc);
	int index;

	if (cur == QSE_NULL) return 0; /* illegal character */

	if (cur->length > size)
	{
		/* buffer not big enough. index indicates the buffer 
		 * size needed */
		return size + 1;
	}

	index = cur->length;
	while (index > 1) 
	{
		/*
		 * 0x3F: 00111111
		 * 0x80: 10000000
		 */
		utf8[--index] = (uc & 0x3F) | 0x80;
		uc >>= 6;
	}

	utf8[0] = uc | cur->fbyte;
	return (qse_size_t)cur->length;
}

/* mbtowc for utf8/unicode */
qse_size_t qse_utf8touc (
	const qse_mchar_t* utf8, qse_size_t size, qse_wchar_t* uc)
{
	__utf8_t* cur, * end;
#if 0
	qse_mchar_t c, t;
	int count = 0;
#endif

	QSE_ASSERT (utf8 != QSE_NULL);
	QSE_ASSERT (QSE_SIZEOF(qse_mchar_t) == 1);
	QSE_ASSERT (QSE_SIZEOF(qse_wchar_t) >= 2);

	end = utf8_table + QSE_COUNTOF(utf8_table);
	cur = utf8_table;

	while (cur < end) 
	{
		if ((utf8[0] & cur->mask) == cur->fbyte) 
		{
			int i;
			qse_wchar_t w;

			if (size < cur->length) return size + 1; 

			w = utf8[0] & cur->fmask;
			for (i = 1; i < cur->length; i++)
			{
				if (!(utf8[i] & 0x80)) return 0; 
				w = (w << 6) | (utf8[i] & 0x3F);
			}

			*uc = w;
			return (qse_size_t)cur->length;
		}
		cur++;
	}

	return 0; /* error - invalid sequence */
	
#if 0
	c = *utf8;
	w = c;

	while (cur < end) 
	{
		count++;

		if ((c & cur->mask) == cur->fbyte) 
		{
			w &= cur->upper;
			if (w < cur->lower) break; /* wrong value */
			*uc = w;
			return (qse_size_t)count;
		}

		if (size <= count) break; /* insufficient input */
		utf8++; /* advance to the next character in the sequence */

		t = (*utf8 ^ 0x80) & 0xFF;
		if (t & 0xC0) break;
		w = (w << 6) | t;

		cur++;
	}

	return 0; /* error - invalid sequence */
#endif
}

/* mblen for utf8 */
qse_size_t qse_utf8len (const qse_mchar_t* utf8, qse_size_t len)
{
	__utf8_t* cur, * end;

	end = utf8_table + QSE_COUNTOF(utf8_table);
	cur = utf8_table;

	while (cur < end) 
	{
		if ((utf8[0] & cur->mask) == cur->fbyte) 
		{
			int i;

			if (len < cur->length) 
			{
				/* the input is not as large as 
				 * the expected number of bytes */
				return len + 1; /* incomplete sequence */
			}

			for (i = 1; i < cur->length; i++)
			{
				/* in utf8, trailing bytes are all
				 * set with 0x80. if not, invalid */
				if (!(utf8[i] & 0x80)) return 0; 
			}
			return (qse_size_t)cur->length;
		}
		cur++;
	}

	return 0; /* error - invalid sequence */
}

qse_size_t qse_utf8lenmax (void)
{
	return QSE_UTF8LEN_MAX;
}
