/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/cmn/str.h>
#include "mem.h"
#include "fmt.h"
#include <stdarg.h>

str_t* str_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_size_t capa)
{
	str_t* str;

	str = (str_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(str_t) + xtnsize);
	if (str)
	{
		if (str_init (str, mmgr, capa) <= -1)
		{
			QSE_MMGR_FREE (mmgr, str);
			str = QSE_NULL;
		}
		else
		{
			QSE_MEMSET (QSE_XTN(str), 0, xtnsize);
		}
	}
	return str;
}

void str_close (str_t* str)
{
	str_fini (str);
	QSE_MMGR_FREE (str->mmgr, str);
}

int str_init (str_t* str, qse_mmgr_t* mmgr, qse_size_t capa)
{
	QSE_MEMSET (str, 0, QSE_SIZEOF(str_t));

	str->mmgr = mmgr;
	str->sizer = QSE_NULL;

	if (capa == 0) str->val.ptr = QSE_NULL;
	else
	{
		str->val.ptr = (char_t*) QSE_MMGR_ALLOC (
			mmgr, QSE_SIZEOF(char_t) * (capa + 1));
		if (str->val.ptr == QSE_NULL) return -1;
		str->val.ptr[0] = T('\0');
	}

	str->val.len = 0;
	str->capa = capa;

	return 0;
}

void str_fini (str_t* str)
{
	if (str->val.ptr) QSE_MMGR_FREE (str->mmgr, str->val.ptr);
}

qse_mmgr_t* str_getmmgr (str_t* mbs)
{
	return mbs->mmgr;
}

void* str_getxtn (str_t* mbs)
{
	return QSE_XTN (mbs);
}

int str_yield (str_t* str, cstr_t* buf, qse_size_t newcapa)
{
	char_t* tmp;

	if (newcapa == 0) tmp = QSE_NULL;
	else
	{
		tmp = (char_t*) QSE_MMGR_ALLOC (
			str->mmgr, QSE_SIZEOF(char_t) * (newcapa + 1));
		if (tmp == QSE_NULL) return -1;
		tmp[0] = T('\0');
	}

	if (buf != QSE_NULL)
	{
		buf->ptr = str->val.ptr;
		buf->len = str->val.len;
	}

	str->val.ptr = tmp;
	str->val.len = 0;
	str->capa = newcapa;

	return 0;
}

char_t* str_yieldptr (str_t* str, qse_size_t newcapa)
{
	cstr_t mx;
	if (str_yield (str, &mx, newcapa) <= -1) return QSE_NULL;
	return mx.ptr;
}

str_sizer_t str_getsizer (str_t* str)
{
	return str->sizer;	
}

void str_setsizer (str_t* str, str_sizer_t sizer)
{
	str->sizer = sizer;
}

qse_size_t str_getcapa (str_t* str)
{
	return str->capa;
}

qse_size_t str_setcapa (str_t* str, qse_size_t capa)
{
	char_t* tmp;

	if (capa == str->capa) return capa;

	if (str->mmgr->realloc != QSE_NULL && str->val.ptr != QSE_NULL)
	{
		tmp = (char_t*) QSE_MMGR_REALLOC (
			str->mmgr, str->val.ptr, 
			QSE_SIZEOF(char_t)*(capa+1));
		if (tmp == QSE_NULL) return (qse_size_t)-1;
	}
	else
	{
		tmp = (char_t*) QSE_MMGR_ALLOC (
			str->mmgr, QSE_SIZEOF(char_t)*(capa+1));
		if (tmp == QSE_NULL) return (qse_size_t)-1;

		if (str->val.ptr)
		{
			qse_size_t ncopy = (str->val.len <= capa)? str->val.len: capa;
			QSE_MEMCPY (tmp, str->val.ptr, 
				QSE_SIZEOF(char_t)*(ncopy+1));
			QSE_MMGR_FREE (str->mmgr, str->val.ptr);
		}
	}

	if (capa < str->val.len)
	{
		str->val.len = capa;
		tmp[capa] = T('\0');
	}

	str->capa = capa;
	str->val.ptr = tmp;

	return str->capa;
}

qse_size_t str_getlen (str_t* str)
{
	return QSE_MBS_LEN (str);
}

qse_size_t str_setlen (str_t* str, qse_size_t len)
{
	if (len == str->val.len) return len;
	if (len < str->val.len) 
	{
		str->val.len = len;
		str->val.ptr[len] = T('\0');
		return len;
	}

	if (len > str->capa)
	{
		if (str_setcapa (str, len) == (qse_size_t)-1) 
			return (qse_size_t)-1;
	}

	while (str->val.len < len) str->val.ptr[str->val.len++] = T(' ');
	str->val.ptr[str->val.len] = T('\0');
	return str->val.len;
}

void str_clear (str_t* str)
{
	str->val.len = 0;
	if (str->val.ptr)
	{
		QSE_ASSERT (str->capa >= 1);
		str->val.ptr[0] = T('\0');
	}
}

void str_swap (str_t* str, str_t* str1)
{
	str_t tmp;

	tmp.val.ptr = str->val.ptr;
	tmp.val.len = str->val.len;
	tmp.capa = str->capa;
	tmp.mmgr = str->mmgr;

	str->val.ptr = str1->val.ptr;
	str->val.len = str1->val.len;
	str->capa = str1->capa;
	str->mmgr = str1->mmgr;

	str1->val.ptr = tmp.val.ptr;
	str1->val.len = tmp.val.len;
	str1->capa = tmp.capa;
	str1->mmgr = tmp.mmgr;
}

qse_size_t str_cpy (str_t* str, const char_t* s)
{
	/* TODO: improve it */
	return str_ncpy (str, s, strlen(s));
}

qse_size_t str_ncpy (str_t* str, const char_t* s, qse_size_t len)
{
	if (len > str->capa || str->capa <= 0)
	{
		qse_size_t tmp;

		/* if the current capacity is 0 and the string len to copy is 0
		 * we can't simply pass 'len' as the new capapcity.
		 * str_setcapa() won't do anything the current capacity of 0
		 * is the same as new capacity required. note that when str->capa 
		 * is 0, str->val.ptr is QSE_NULL. However, this is copying operation.
		 * Copying a zero-length string may indicate that str->val.ptr must
		 * not be QSE_NULL. so I simply pass 1 as the new capacity */
		tmp = str_setcapa (
			str, ((str->capa <= 0 && len <= 0)? 1: len)
		);
		if (tmp == (qse_size_t)-1) return (qse_size_t)-1;
	}

	QSE_MEMCPY (&str->val.ptr[0], s, len*QSE_SIZEOF(*s));
	str->val.ptr[len] = T('\0');
	str->val.len = len;
	return len;
#if 0
	str->val.len = strncpy (str->val.ptr, s, len);
	/*str->val.ptr[str->val.len] = T('\0'); -> mbsncpy does this*/
	return str->val.len;
#endif
}

qse_size_t str_cat (str_t* str, const char_t* s)
{
	/* TODO: improve it */
	return str_ncat (str, s, strlen(s));
}

static int resize_for_ncat (str_t* str, qse_size_t len)
{
	if (len > str->capa - str->val.len) 
	{
		qse_size_t ncapa, mincapa;

		/* let the minimum capacity be as large as 
		 * to fit in the new substring */
		mincapa = str->val.len + len;

		if (str->sizer == QSE_NULL)
		{
			/* increase the capacity by the length to add */
			ncapa = mincapa;
			/* if the new capacity is less than the double,
			 * just double it */
			if (ncapa < str->capa * 2) ncapa = str->capa * 2;
		}
		else
		{
			/* let the user determine the new capacity.
			 * pass the minimum capacity required as a hint */
			ncapa = str->sizer (str, mincapa);
			/* if no change in capacity, return current length */
			if (ncapa == str->capa) return 0;
		}

		/* change the capacity */
		do
		{
			if (str_setcapa (str, ncapa) != (qse_size_t)-1) break;
			if (ncapa <= mincapa) return -1;
			ncapa--;
		}
		while (1);
	}
	else if (str->capa <= 0 && len <= 0)
	{
		QSE_ASSERT (str->val.ptr == QSE_NULL);
		QSE_ASSERT (str->val.len <= 0);
		if (str_setcapa (str, 1) == (qse_size_t)-1) return -1;
	}

	return 1;
}

qse_size_t str_ncat (str_t* str, const char_t* s, qse_size_t len)
{
	int n;
	qse_size_t i, j;

	n = resize_for_ncat (str, len);
	if (n <= -1) return (qse_size_t)-1;
	if (n == 0) return str->val.len;

	if (len > str->capa - str->val.len) 
	{
		/* copy as many characters as the number of cells available.
		 * if the capacity has been decreased, len is adjusted here */
		len = str->capa - str->val.len;
	}

	/*
	QSE_MEMCPY (&str->val.ptr[str->val.len], s, len*QSE_SIZEOF(*s));
	str->val.len += len;
	str->val.ptr[str->val.len] = T('\0');
	*/
	for (i = 0, j = str->val.len ; i < len; j++, i++) str->val.ptr[j] = s[i];
	str->val.ptr[j] = T('\0');
	str->val.len = j;

	return str->val.len;
}

qse_size_t str_nrcat (str_t* str, const char_t* s, qse_size_t len)
{
	int n;
	qse_size_t i, j;

	n = resize_for_ncat (str, len);
	if (n <= -1) return (qse_size_t)-1;
	if (n == 0) return str->val.len;

	if (len > str->capa - str->val.len) len = str->capa - str->val.len;

	for (i = len, j = str->val.len ; i > 0; j++) str->val.ptr[j] = s[--i];	
	str->val.ptr[j] = T('\0');
	str->val.len = j;

	return str->val.len;
}

qse_size_t str_ccat (str_t* str, char_t c)
{
	return str_ncat (str, &c, 1);
}

qse_size_t str_nccat (str_t* str, char_t c, qse_size_t len)
{
	while (len > 0)
	{
		if (str_ncat (str, &c, 1) == (qse_size_t)-1) 
		{
			return (qse_size_t)-1;
		}

		len--;
	}
	return str->val.len;
}

qse_size_t str_del (str_t* str, qse_size_t index, qse_size_t size)
{
	if (str->val.ptr && index < str->val.len && size > 0)
	{
		qse_size_t nidx = index + size;
		if (nidx >= str->val.len)
		{
			str->val.ptr[index] = T('\0');
			str->val.len = index;
		}
		else
		{
			strncpy (
				&str->val.ptr[index], &str->val.ptr[nidx],
				str->val.len - nidx);
			str->val.len -= size;
		}
	}

	return str->val.len;
}

qse_size_t str_amend (str_t* str, qse_size_t pos, qse_size_t len, const char_t* repl)
{
	qse_size_t max_len;
	qse_size_t repl_len = strlen(repl);

	if (pos >= str->val.len) pos = str->val.len;
	max_len = str->val.len - pos;
	if (len > max_len) len = max_len;

	if (len > repl_len)
	{
		str_del (str, pos, len - repl_len);
	}
	else if (len < repl_len)
	{
		qse_size_t old_str_len = str->val.len;
		if (str_setlen (str, str->val.len + repl_len - len) == (qse_size_t)-1) return (qse_size_t)-1;
		QSE_MEMMOVE (&str->val.ptr[pos + repl_len], &str->val.ptr[pos + len], QSE_SIZEOF(*repl) * (old_str_len - (pos + len)));
	}

	if (repl_len > 0) QSE_MEMMOVE (&str->val.ptr[pos], repl, QSE_SIZEOF(*repl) * repl_len); 
	return str->val.len;
}

qse_size_t str_trm (str_t* str)
{
	if (str->val.ptr)
	{
		str->val.len = strxtrm (str->val.ptr, str->val.len);
	}

	return str->val.len;
}

qse_size_t str_pac (str_t* str)
{
	if (str->val.ptr)
	{
		str->val.len = strxpac (str->val.ptr, str->val.len);
	}

	return str->val.len;
}

qse_size_t str_vfcat (str_t* str, const char_t* fmt, va_list ap)
{
	va_list orgap;
	fmtout_t fo;
	int x;
	qse_size_t old_len;

	old_len = str->val.len;

	fo.limit = QSE_TYPE_MAX(qse_size_t) - 1;
	fo.ctx = str;
	fo.put = str->val.ptr? put_char_check: put_char_null;
	fo.conv = conv_char;

	va_copy (orgap, ap);
	x = fmtout (fmt, &fo, ap);

	if (x <= -1)
	{
		str->val.len = old_len;
		return (qse_size_t)-1;
	}
	
	if (str->val.ptr == QSE_NULL || str->val.len - old_len < fo.count)
	{
		str->val.len = old_len;

		/* resizing is required */
		x = resize_for_ncat (str, fo.count);

		if (x <= -1) return (qse_size_t)-1;
		if (x >= 1)
		{
			fo.put = put_char_nocheck;
			x = fmtout (fmt, &fo, orgap);
		}
	}

	str->val.ptr[str->val.len] = T('\0');
	return str->val.len;
}

qse_size_t str_fcat (str_t* str, const char_t* fmt, ...)
{
	qse_size_t x;
	va_list ap;

	va_start (ap, fmt);
	x =  str_vfcat (str, fmt, ap);
	va_end (ap);

	return x;
}

qse_size_t str_vfmt (str_t* str, const char_t* fmt, va_list ap)
{
	int x;
	fmtout_t fo;
	va_list orgap;

	fo.limit = QSE_TYPE_MAX(qse_size_t) - 1;
	fo.ctx = str;
	fo.put = put_char_null;
	fo.conv = conv_char;

	va_copy (orgap, ap);
	if (fmtout (fmt, &fo, ap) <= -1) return (qse_size_t)-1;

	str_clear (str);
	x = resize_for_ncat (str, fo.count);

	if (x <= -1) return (qse_size_t)-1;
	if (x >= 1)
	{
		fo.put = put_char_nocheck;
		x = fmtout (fmt, &fo, orgap);
	}

	str->val.ptr[str->val.len] = T('\0');
	return str->val.len;
}

qse_size_t str_fmt (str_t* str, const char_t* fmt, ...)
{
	qse_size_t x;
	va_list ap;

	va_start (ap, fmt);
	x = str_vfmt (str, fmt, ap);
	va_end (ap);

	return x;
}
