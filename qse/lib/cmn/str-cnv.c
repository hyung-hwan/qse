/*
 * $Id: str-cnv.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include "mem.h"

int qse_strtoi (const qse_char_t* str)
{
	int v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

long qse_strtol (const qse_char_t* str)
{
	long v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

unsigned int qse_strtoui (const qse_char_t* str)
{
	unsigned int v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

unsigned long qse_strtoul (const qse_char_t* str)
{
	unsigned long v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

int qse_strxtoi (const qse_char_t* str, qse_size_t len)
{
	int v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

long qse_strxtol (const qse_char_t* str, qse_size_t len)
{
	long v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

unsigned int qse_strxtoui (const qse_char_t* str, qse_size_t len)
{
	unsigned int v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

unsigned long qse_strxtoul (const qse_char_t* str, qse_size_t len)
{
	unsigned long v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

qse_int_t qse_strtoint (const qse_char_t* str)
{
	qse_int_t v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

qse_long_t qse_strtolong (const qse_char_t* str)
{
	qse_long_t v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

qse_uint_t qse_strtouint (const qse_char_t* str)
{
	qse_uint_t v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

qse_ulong_t qse_strtoulong (const qse_char_t* str)
{
	qse_ulong_t v;
	QSE_STRTONUM (v, str, QSE_NULL, 10);
	return v;
}

qse_int_t qse_strxtoint (const qse_char_t* str, qse_size_t len)
{
	qse_int_t v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

qse_long_t qse_strxtolong (const qse_char_t* str, qse_size_t len)
{
	qse_long_t v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

qse_uint_t qse_strxtouint (const qse_char_t* str, qse_size_t len)
{
	qse_uint_t v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}

qse_ulong_t qse_strxtoulong (const qse_char_t* str, qse_size_t len)
{
	qse_ulong_t v;
	QSE_STRXTONUM (v, str, len, QSE_NULL, 10);
	return v;
}


/*
 * TODO: fix wrong mbstate handling 
 */

qse_size_t qse_mbstowcslen (const qse_mchar_t* mcs, qse_size_t* wcslen)
{
	qse_wchar_t wc;
	qse_size_t n, ml, wl = 0;
	const qse_mchar_t* p = mcs;
	qse_mbstate_t state = {{ 0, }};
	
	while (*p != '\0') p++;
	ml = p - mcs;
	
	for (p = mcs; ml > 0; p += n, ml -= n) 
	{
		n = qse_mbrtowc (p, ml, &wc, &state);
		/* insufficient input or wrong sequence */
		if (n == 0 || n > ml) break;
		wl++;
	}

	if (wcslen) *wcslen = wl;
	return p - mcs;
}

qse_size_t qse_mbsntowcsnlen (
	const qse_mchar_t* mcs, qse_size_t mcslen, qse_size_t* wcslen)
{
	qse_wchar_t wc;
	qse_size_t n, ml = mcslen, wl = 0;
	const qse_mchar_t* p = mcs;
	qse_mbstate_t state = {{ 0, }};
	
	for (p = mcs; ml > 0; p += n, ml -= n) 
	{
		n = qse_mbrtowc (p, ml, &wc, &state);
		/* insufficient or invalid sequence */
		if (n == 0 || n > ml) break;
		wl++;
	}

	if (wcslen) *wcslen = wl;
	return mcslen - ml;
}

qse_size_t qse_mbstowcs (
	const qse_mchar_t* mbs, qse_wchar_t* wcs, qse_size_t* wcslen)
{
	qse_size_t wlen, mlen;
	const qse_mchar_t* mp;

	/* get the length of mbs and pass it to qse_mbsntowcsn as 
	 * qse_mbtowc called by qse_mbsntowcsn needs it. */
	wlen = *wcslen;
	if (wlen <= 0)
	{
		/* buffer too small. also cannot null-terminate it */
		*wcslen = 0;
		return 0; /* 0 byte processed */
	}

	for (mp = mbs; *mp != QSE_MT('\0'); mp++);
	mlen = qse_mbsntowcsn (mbs, mp - mbs, wcs, &wlen);
	if (wlen < *wcslen) 
	{
		/* null-terminate wcs if it is large enough. */
		wcs[wlen] = QSE_WT('\0');
	}

	/* if null-terminated properly, the input wcslen must be less than
	 * the output wcslen. (input length includs the terminating null
	 * while the output length excludes the terminating null) */
	*wcslen = wlen; 

	return mlen;
}

qse_size_t qse_mbsntowcsn (
	const qse_mchar_t* mbs, qse_size_t mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	qse_size_t mlen = mbslen, n;
	const qse_mchar_t* p;
	qse_wchar_t* q, * qend ;
	qse_mbstate_t state = {{ 0, }};

	qend = wcs + *wcslen;

	for (p = mbs, q = wcs; mlen > 0 && q < qend; p += n, mlen -= n) 
	{
		n = qse_mbrtowc (p, mlen, q, &state);
		if (n == 0 || n > mlen)
		{
			/* wrong sequence or insufficient input */
			break;
		}

		q++;
	}

	*wcslen = q - wcs;
	return p - mbs; /* returns the number of bytes processed */
}

qse_size_t qse_wcstombslen (const qse_wchar_t* wcs, qse_size_t* mbslen)
{
	const qse_wchar_t* p = wcs;
	qse_mchar_t mbs[32];
	qse_size_t mlen = 0;
	qse_mbstate_t state = {{ 0, }};

	while (*p != QSE_WT('\0'))
	{
		qse_size_t n = qse_wcrtomb (*p, mbs, QSE_COUNTOF(mbs), &state);
		if (n == 0) break; /* illegal character */

		/* it assumes that mbs is large enough to hold a character */
		QSE_ASSERT (n <= QSE_COUNTOF(mbs));

		p++; mlen += n;
	}

	/* this length holds the number of resulting multi-byte characters 
	 * excluding the terminating null character */
	*mbslen = mlen;

	/* returns the number of characters handled. 
	 * if the function has encountered an illegal character in
	 * the while loop above, wcs[p-wcs] will not be a null character */
	return p - wcs;  
}

qse_size_t qse_wcsntombsnlen (
	const qse_wchar_t* wcs, qse_size_t wcslen, qse_size_t* mbslen)
{
	const qse_wchar_t* p = wcs;
	const qse_wchar_t* end = wcs + wcslen;
	qse_mchar_t mbs[32];
	qse_size_t mlen = 0;
	qse_mbstate_t state = {{ 0, }};

	while (p < end)
	{
		qse_size_t n = qse_wcrtomb (*p, mbs, QSE_COUNTOF(mbs), &state);
		if (n == 0) break; /* illegal character */

		/* it assumes that mbs is large enough to hold a character */
		QSE_ASSERT (n <= QSE_COUNTOF(mbs));

		p++; mlen += n;
	}

	/* this length excludes the terminating null character. */
	*mbslen = mlen;

	/* returns the number of characters handled. 
	 * if the function has encountered an illegal character in
	 * the while loop above, wcs[p-wcs] will not be a null character */
	return p - wcs;  
}

qse_size_t qse_wcstombs (
	const qse_wchar_t* wcs, qse_mchar_t* mbs, qse_size_t* mbslen)
{
	const qse_wchar_t* p = wcs;
	qse_size_t rem = *mbslen;
	qse_mbstate_t state = {{ 0, }};

	while (*p != QSE_WT('\0') && rem > 1) 
	{
		qse_size_t n = qse_wcrtomb (*p, mbs, rem, &state);
		if (n == 0 || n > rem)
		{
			/* illegal character or buffer not enough */
			break;
		}

		if (rem == n) 
		{
			/* the buffer is full without the space for a 
			 * terminating null. should stop processing further
			 * excluding this last character emitted. */
			break;
		}

		mbs += n; rem -= n; p++;
	}

	/* update mbslen to the length of the mbs string converted excluding
	 * terminating null */
	*mbslen -= rem; 

	/* null-terminate the multibyte sequence if it has sufficient space */
	if (rem > 0) *mbs = QSE_MT('\0');

	/* returns the number of characters handled. */
	return p - wcs; 
}

qse_size_t qse_wcsntombsn (
	const qse_wchar_t* wcs, qse_size_t wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen)
{
	const qse_wchar_t* p = wcs;
	const qse_wchar_t* end = wcs + wcslen;
	qse_size_t len = *mbslen;
	qse_mbstate_t state = {{ 0, }};

	while (p < end && len > 0) 
	{
		qse_size_t n = qse_wcrtomb (*p, mbs, len, &state);
		if (n == 0 || n > len)
		{
			/* illegal character or buffer not enough */
			break;
		}
		mbs += n; len -= n; p++;
	}

	*mbslen -= len; 

	/* returns the number of characters handled.
	 * the caller can check if the return value is as large is wcslen
	 * for an error. */
	return p - wcs; 
}

int qse_mbstowcs_strict (
	const qse_mchar_t* mbs, qse_wchar_t* wcs, qse_size_t wcslen)
{
	qse_size_t n;
	qse_size_t wn = wcslen;

	n = qse_mbstowcs (mbs, wcs, &wn);
	if (wn >= wcslen)
	{
		/* wcs not big enough to be null-terminated.
		 * if it has been null-terminated properly, 
		 * wn should be less than wcslen. */
		return -1;
	}
	if (mbs[n] != QSE_MT('\0'))
	{
		/* incomplete sequence or invalid sequence */
		return -1;
	}

	return 0;
}

int qse_wcstombs_strict (
	const qse_wchar_t* wcs, qse_mchar_t* mbs, qse_size_t mbslen)
{
	qse_size_t n;
	qse_size_t mn = mbslen;

	n = qse_wcstombs (wcs, mbs, &mn);
	if (mn >= mbslen) 
	{
		/* mbs not big enough to be null-terminated.
		 * if it has been null-terminated properly, 
		 * mn should be less than mbslen. */
		return -1; 
	}
	if (wcs[n] != QSE_WT('\0')) 
	{
		/* if qse_wcstombs() processed all wide characters,
		 * the character at position 'n' should be a null character
		 * as 'n' is the number of wide characters processed. */
		return -1;
	}

	return 0;
}

qse_wchar_t* qse_mbstowcsdup (
	const qse_mchar_t* mbs, qse_mmgr_t* mmgr)
{
	qse_size_t n, req;
	qse_wchar_t* wcs;

	n = qse_mbstowcslen (mbs, &req);
	if (mbs[n] != QSE_WT('\0')) return QSE_NULL;

	req++;

	wcs = QSE_MMGR_ALLOC (mmgr, req * QSE_SIZEOF(*wcs));	
	if (wcs == QSE_NULL) return QSE_NULL;

	qse_mbstowcs (mbs, wcs, &req);
	return wcs;
}

qse_mchar_t* qse_wcstombsdup (
	const qse_wchar_t* wcs, qse_mmgr_t* mmgr)
{
	qse_size_t n, req;
	qse_mchar_t* mbs;

	n = qse_wcstombslen (wcs, &req);
	if (wcs[n] != QSE_WT('\0')) return QSE_NULL;

	req++;

	mbs = QSE_MMGR_ALLOC (mmgr, req * QSE_SIZEOF(*mbs));	
	if (mbs == QSE_NULL) return QSE_NULL;

	qse_wcstombs (wcs, mbs, &req);
	return mbs;
}

/* case conversion */

qse_size_t qse_mbslwr (qse_mchar_t* str)
{
	qse_mchar_t* p = str;
	for (p = str; *p != QSE_MT('\0'); p++) *p = QSE_TOMLOWER (*p);
	return p - str;
}

qse_size_t qse_mbsupr (qse_mchar_t* str)
{
	qse_mchar_t* p = str;
	for (p = str; *p != QSE_MT('\0'); p++) *p = QSE_TOMUPPER (*p);
	return p - str;
}

qse_size_t qse_wcslwr (qse_wchar_t* str)
{
	qse_wchar_t* p = str;
	for (p = str; *p != QSE_WT('\0'); p++) *p = QSE_TOWLOWER (*p);
	return p - str;
}

qse_size_t qse_wcsupr (qse_wchar_t* str)
{
	qse_wchar_t* p = str;
	for (p = str; *p != QSE_WT('\0'); p++) *p = QSE_TOWUPPER (*p);
	return p - str;
}
