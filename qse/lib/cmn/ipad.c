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

/* Copyright (c) 1996-1999 by Internet Software Consortium
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <qse/cmn/ipad.h>
#include <qse/cmn/hton.h>
#include <qse/cmn/str.h>
#include "mem.h"

#if 0
const qse_ipad4_t qse_ipad4_any =
{
	0 /* 0.0.0.0 */
};

const qse_ipad4_t qse_ipad4_loopback =
{
#if defined(QSE_ENDIAN_BIG)
	0x7F000001u /* 127.0.0.1 */
#elif defined(QSE_ENDIAN_LITTLE)
	0x0100007Fu
#else
#	error Unknown endian
#endif
};

const qse_ipad6_t qse_ipad6_any =
{
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } /* :: */
};

const qse_ipad6_t qse_ipad6_loopback =
{
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } /* ::1 */
};
#endif

static int str_to_ipad4 (int mbs, const void* str, qse_size_t len, qse_ipad4_t* ipad)
{
	const void* end;
	int dots = 0, digits = 0;
	qse_uint32_t acc = 0, addr = 0;	
	qse_wchar_t c;

	end = (mbs? (const void*)((const qse_mchar_t*)str + len):
	            (const void*)((const qse_wchar_t*)str + len));

	do
	{
		if (str >= end)
		{
			if (dots < 3 || digits == 0) return -1;
			addr = (addr << 8) | acc;
			break;
		}

		if (mbs)
		{
			c = *(const qse_mchar_t*)str;
			str = (const qse_mchar_t*)str + 1;
		}
		else
		{
			c = *(const qse_wchar_t*)str;
			str = (const qse_wchar_t*)str + 1;
		}

		if (c >= QSE_WT('0') && c <= QSE_WT('9')) 
		{
			if (digits > 0 && acc == 0) return -1;
			acc = acc * 10 + (c - QSE_T('0'));
			if (acc > 255) return -1;
			digits++;
		}
		else if (c == QSE_WT('.')) 
		{
			if (dots >= 3 || digits == 0) return -1;
			addr = (addr << 8) | acc;
			dots++; acc = 0; digits = 0;
		}
		else return -1;
	}
	while (1);

	if (ipad != QSE_NULL) ipad->value = qse_hton32(addr);
	return 0;
}

int qse_mbstoipad4 (const qse_mchar_t* str, qse_ipad4_t* ipad)
{
	return str_to_ipad4 (1, str, qse_mbslen(str), ipad);
}

int qse_wcstoipad4 (const qse_wchar_t* str, qse_ipad4_t* ipad)
{
	return str_to_ipad4 (0, str, qse_wcslen(str), ipad);
}

int qse_mbsntoipad4 (const qse_mchar_t* str, qse_size_t len, qse_ipad4_t* ipad)
{
	return str_to_ipad4 (1, str, len, ipad);
}

int qse_wcsntoipad4 (const qse_wchar_t* str, qse_size_t len, qse_ipad4_t* ipad)
{
	return str_to_ipad4 (0, str, len, ipad);
}

#define __BTOA(type_t,b,p,end) \
	do { \
		type_t* sp = p; \
		do {  \
			if (p >= end) { \
				if (p == sp) break; \
				if (p - sp > 1) p[-2] = p[-1]; \
				p[-1] = (b % 10) + '0'; \
			} \
			else *p++ = (b % 10) + '0'; \
			b /= 10; \
		} while (b > 0); \
		if (p - sp > 1) { \
			type_t t = sp[0]; \
			sp[0] = p[-1]; \
			p[-1] = t; \
		} \
	} while (0);

#define __ADDDOT(p, end) \
	do { \
		if (p >= end) break; \
		*p++ = '.'; \
	} while (0)

qse_size_t qse_ipad4tombs (
	const qse_ipad4_t* ipad, qse_mchar_t* buf, qse_size_t size)
{
	qse_byte_t b;
	qse_mchar_t* p, * end;
	qse_uint32_t ip;

	if (size <= 0) return 0;

	ip = ipad->value;

	p = buf;
	end = buf + size - 1;

#if defined(QSE_ENDIAN_BIG)
	b = (ip >> 24) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 16) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  8) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  0) & 0xFF; __BTOA (qse_mchar_t, b, p, end);
#elif defined(QSE_ENDIAN_LITTLE)
	b = (ip >>  0) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  8) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 16) & 0xFF; __BTOA (qse_mchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 24) & 0xFF; __BTOA (qse_mchar_t, b, p, end);
#else
#	error Unknown Endian
#endif

	*p = QSE_MT('\0');
	return p - buf;
}

qse_size_t qse_ipad4towcs (
	const qse_ipad4_t* ipad, qse_wchar_t* buf, qse_size_t size)
{
	qse_byte_t b;
	qse_wchar_t* p, * end;
	qse_uint32_t ip;

	if (size <= 0) return 0;

	ip = ipad->value;

	p = buf;
	end = buf + size - 1;

#if defined(QSE_ENDIAN_BIG)
	b = (ip >> 24) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 16) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  8) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  0) & 0xFF; __BTOA (qse_wchar_t, b, p, end);
#elif defined(QSE_ENDIAN_LITTLE)
	b = (ip >>  0) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >>  8) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 16) & 0xFF; __BTOA (qse_wchar_t, b, p, end); __ADDDOT (p, end);
	b = (ip >> 24) & 0xFF; __BTOA (qse_wchar_t, b, p, end);
#else
#	error Unknown Endian
#endif

	*p = QSE_WT('\0');
	return p - buf;
}

#if 0
int qse_strtoipad6 (const qse_char_t* src, qse_ipad6_t* ipad)
{
#if 0
	static const qse_char_t xdigits_l[] = QSE_T("0123456789abcdef"),
		                  xdigits_u[] = QSE_T("0123456789ABCDEF");
	const qse_char_t* xdigits;
#endif

	qse_ipad6_t tmp;
	qse_byte_t* tp, * endp, * colonp;
	const qse_char_t* curtok;
	qse_char_t ch;
	int saw_xdigit;
	unsigned int val;

	QSE_MEMSET (&tmp, 0, QSE_SIZEOF(tmp));
	tp = &tmp.value[0];
	endp = &tmp.value[QSE_COUNTOF(tmp.value)];
	colonp = QSE_NULL;

	/* Leading :: requires some special handling. */
	if (*src == QSE_T(':'))
	{
		 if (*++src != QSE_T(':')) return -1;
	}

	curtok = src;
	saw_xdigit = 0;
	val = 0;
	while ((ch = *src++) != QSE_T('\0')) 
	{
	#if 0
		const char *pch;
		if ((pch = qse_strchr((xdigits = xdigits_l), ch)) == QSE_NULL)
			pch = qse_strchr((xdigits = xdigits_u), ch);
		if (pch != QSE_NULL) 
		{
			val <<= 4;
			val |= (pch - xdigits);
			if (val > 0xffff) return -1;
			saw_xdigit = 1;
			continue;
		}
	#endif
		int v1;
		if (ch >= QSE_T('0') && ch <= QSE_T('9'))
			v1 = ch - QSE_T('0');
		else if (ch >= QSE_T('A') && ch <= QSE_T('F'))
			v1 = ch - QSE_T('A') + 10;
		else if (ch >= QSE_T('a') && ch <= QSE_T('f'))
			v1 = ch - QSE_T('a') + 10;
		else v1 = -1;
		if (v1 >= 0)
		{
			val <<= 4;
			val |= v1;
			if (val > 0xffff) return -1;
			saw_xdigit = 1;
			continue;
		}

		if (ch == QSE_T(':')) 
		{
			curtok = src;
			if (!saw_xdigit) 
			{
				if (colonp) return -1;
				colonp = tp;
				continue;
			}
			else if (*src == QSE_T('\0')) 
			{
				/* a colon can't be the last character */
				return -1;
			}

			if (tp + QSE_SIZEOF(qse_uint16_t) > endp) return -1;
			*tp++ = (qse_byte_t) (val >> 8) & 0xff;
			*tp++ = (qse_byte_t) val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}
	#if 0
		if (ch == QSE_T('.') && ((tp + NS_INADDRSZ) <= endp) &&
		    inet_pton4(curtok, tp) > 0) 
		{
			tp += NS_INADDRSZ;
			saw_xdigit = 0;
			break;  /* '\0' was seen by inet_pton4(). */
		}
	#endif
		if (ch == QSE_T('.') && ((tp + QSE_SIZEOF(qse_ipad4_t)) <= endp) &&
		    qse_strtoipad4(curtok, (qse_ipad4_t*)tp) == 0) 
		{
			tp += QSE_SIZEOF(qse_ipad4_t);
			saw_xdigit = 0;
			break; 
		}

		return -1;
	}

	if (saw_xdigit) 
	{
		if (tp + QSE_SIZEOF(qse_uint16_t) > endp) return -1;
		*tp++ = (qse_byte_t) (val >> 8) & 0xff;
		*tp++ = (qse_byte_t) val & 0xff;
	}
	if (colonp != QSE_NULL) 
	{
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		qse_size_t n = tp - colonp;
		qse_size_t i;
 
		for (i = 1; i <= n; i++) 
		{
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}

	if (tp != endp) return -1;

	*ipad = tmp;
	return 0;
}

int qse_strxtoipad6 (const qse_char_t* src, qse_size_t len, qse_ipad6_t* ipad)
{
	qse_ipad6_t tmp;
	qse_byte_t* tp, * endp, * colonp;
	const qse_char_t* curtok;
	qse_char_t ch;
	int saw_xdigit;
	unsigned int val;
	const qse_char_t* src_end;

	src_end = src + len;

	QSE_MEMSET (&tmp, 0, QSE_SIZEOF(tmp));
	tp = &tmp.value[0];
	endp = &tmp.value[QSE_COUNTOF(tmp.value)];
	colonp = QSE_NULL;

	/* Leading :: requires some special handling. */
	if (src < src_end && *src == QSE_T(':'))
	{
		src++;
		if (src >= src_end || *src != QSE_T(':')) return -1;
	}

	curtok = src;
	saw_xdigit = 0;
	val = 0;

	while (src < src_end)
	{
		int v1;

		ch = *src++;

		if (ch >= QSE_T('0') && ch <= QSE_T('9'))
			v1 = ch - QSE_T('0');
		else if (ch >= QSE_T('A') && ch <= QSE_T('F'))
			v1 = ch - QSE_T('A') + 10;
		else if (ch >= QSE_T('a') && ch <= QSE_T('f'))
			v1 = ch - QSE_T('a') + 10;
		else v1 = -1;
		if (v1 >= 0)
		{
			val <<= 4;
			val |= v1;
			if (val > 0xffff) return -1;
			saw_xdigit = 1;
			continue;
		}

		if (ch == QSE_T(':')) 
		{
			curtok = src;
			if (!saw_xdigit) 
			{
				if (colonp) return -1;
				colonp = tp;
				continue;
			}
			else if (src >= src_end)
			{
				/* a colon can't be the last character */
				return -1;
			}

			*tp++ = (qse_byte_t) (val >> 8) & 0xff;
			*tp++ = (qse_byte_t) val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}

		if (ch == QSE_T('.') && ((tp + QSE_SIZEOF(qse_ipad4_t)) <= endp) &&
		    qse_strxtoipad4(curtok, src_end - curtok, (qse_ipad4_t*)tp) == 0) 
		{
			tp += QSE_SIZEOF(qse_ipad4_t);
			saw_xdigit = 0;
			break; 
		}

		return -1;
	}

	if (saw_xdigit) 
	{
		if (tp + QSE_SIZEOF(qse_uint16_t) > endp) return -1;
		*tp++ = (qse_byte_t) (val >> 8) & 0xff;
		*tp++ = (qse_byte_t) val & 0xff;
	}
	if (colonp != QSE_NULL) 
	{
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		qse_size_t n = tp - colonp;
		qse_size_t i;
 
		for (i = 1; i <= n; i++) 
		{
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}

	if (tp != endp) return -1;

	*ipad = tmp;
	return 0;
}

qse_size_t qse_ipad6tostrx (
	const qse_ipad6_t* ipad, qse_char_t* buf, qse_size_t size)
{
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */

#define IP6ADDR_NWORDS (QSE_SIZEOF(ipad->value) / QSE_SIZEOF(qse_uint16_t))

	qse_char_t tmp[QSE_COUNTOF(QSE_T("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"))], *tp;
	struct { int base, len; } best, cur;
	qse_uint16_t words[IP6ADDR_NWORDS];
	int i;

	if (size <= 0) return 0;

	/*
	 * Preprocess:
	 *	Copy the input (bytewise) array into a wordwise array.
	 *	Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	QSE_MEMSET (words, 0, QSE_SIZEOF(words));
	for (i = 0; i < QSE_SIZEOF(ipad->value); i++)
		words[i / 2] |= (ipad->value[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	cur.base = -1;

	for (i = 0; i < IP6ADDR_NWORDS; i++) 
	{
		if (words[i] == 0) 
		{
			if (cur.base == -1)
			{
				cur.base = i;
				cur.len = 1;
			}
			else
			{
				cur.len++;
			}
		}
		else 
		{
			if (cur.base != -1) 
			{
				if (best.base == -1 || cur.len > best.len) best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) 
	{
		if (best.base == -1 || cur.len > best.len) best = cur;
	}
	if (best.base != -1 && best.len < 2) best.base = -1;

	/*
	 * Format the result.
	 */
	tp = tmp;
	for (i = 0; i < IP6ADDR_NWORDS; i++) 
	{
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
		    i < (best.base + best.len)) 
		{
			if (i == best.base) *tp++ = QSE_T(':');
			continue;
		}

		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0) *tp++ = QSE_T(':');

		/* Is this address an encapsulated IPv4? ipv4-compatible or ipv4-mapped */
		if (i == 6 && best.base == 0 &&
		    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) 
		{
			qse_ipad4_t ipad4;
			QSE_MEMCPY (&ipad4.value, ipad->value+12, QSE_SIZEOF(ipad4.value));
			tp += qse_ipad4tostrx (&ipad4, tp, QSE_SIZEOF(tmp) - (tp - tmp));
			break;
		}

		tp += qse_uint16tostr_lower (words[i], tp, QSE_SIZEOF(tmp) - (tp - tmp), 16, QSE_T('\0'));
	}

	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && 
	    (best.base + best.len) == IP6ADDR_NWORDS) *tp++ = QSE_T(':');
	*tp++ = QSE_T('\0');

	return qse_strxcpy (buf, size, tmp);

#undef IP6ADDR_NWORDS
}

#endif
