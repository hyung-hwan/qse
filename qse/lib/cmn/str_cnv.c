/*
 * $Id: str_cnv.c 455 2008-11-26 09:05:00Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/cmn/str.h>

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

qse_size_t qse_mbstowcs (
	const qse_mchar_t* mbs, qse_wchar_t* wcs, qse_size_t* wcslen)
{
	qse_size_t wlen, mlen;
	const qse_mchar_t* mp;

	/* get the length of mbs and pass it to qse_mbsntowcsn as 
	 * qse_mbtowc called by qse_mbsntowcsn needs it. */
	for (mp = mbs; *mp != '\0'; mp++);

	if (*wcslen <= 0) 
	{
		/* buffer too small. cannot null-terminate it */
		return 0;
	}
	if (*wcslen == 1) 
	{
		wcs[0] = L'\0';
		return 0;
	}

	wlen = *wcslen - 1;
	mlen = qse_mbsntowcsn (mbs, mp - mbs, wcs, &wlen);

	wcs[wlen] = L'\0';
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

	qend = wcs + *wcslen;

	for (p = mbs, q = wcs; mlen > 0 && q < qend; p += n, mlen -= n) 
	{
		n = qse_mbtowc (p, mlen, q);
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

	while (*p != QSE_WT('\0'))
	{
		qse_size_t n = qse_wctomb (*p, mbs, QSE_COUNTOF(mbs));
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

qse_size_t qse_wcsntombsnlen (
	const qse_wchar_t* wcs, qse_size_t wcslen, qse_size_t* mbslen)
{
	const qse_wchar_t* p = wcs;
	const qse_wchar_t* end = wcs + wcslen;
	qse_mchar_t mbs[32];
	qse_size_t mlen = 0;

	while (p < end)
	{
		qse_size_t n = qse_wctomb (*p, mbs, QSE_COUNTOF(mbs));
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

	while (*p != QSE_WT('\0') && rem > 1) 
	{
		qse_size_t n = qse_wctomb (*p, mbs, rem);
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

	while (p < end && len > 0) 
	{
		qse_size_t n = qse_wctomb (*p, mbs, len);
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

int qse_wcstombs_strict (
	const qse_wchar_t* wcs, qse_mchar_t* mbs, qse_size_t mbslen)
{
	qse_size_t n;
	qse_size_t mn = mbslen;

	n = qse_wcstombs (wcs, mbs, &mn);
	if (wcs[n] != QSE_WT('\0')) 
	{
		/* if qse_wcstombs() processed all wide characters,
		 * the character at position 'n' should be a null character
		 * as 'n' is the number of wide characters processed. */
		return -1;
	}
	if (mn >= mbslen) 
	{
		/* mbs not big enough to be null-terminated.
		 * if it has been null-terminated properly, 
		 * mn should be less than mbslen. */
		return -1; 
	}

	return 0;
}
