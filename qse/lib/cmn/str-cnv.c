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


int qse_mbstowcs (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	const qse_mchar_t* mp;
	qse_size_t mlen, wlen;
	int n;

	for (mp = mbs; *mp != QSE_MT('\0'); mp++);

	mlen = mp - mbs; wlen = *wcslen;
	n = qse_mbsntowcsn (mbs, &mlen, wcs, &wlen);
	if (wcs)
	{
		if (wlen < *wcslen) wcs[wlen] = QSE_WT('\0');
		else n = -2; /* buffer too small */
	}
	*mbslen = mlen; *wcslen = wlen;

	return n;
}

int qse_mbsntowcsn (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	const qse_mchar_t* p;
	qse_mbstate_t state = {{ 0, }};
	int ret = 0;
	qse_size_t mlen;

	if (wcs)
	{
		qse_wchar_t* q, * qend;

		p = mbs;
		q = wcs;
		qend = wcs + *wcslen;
		mlen = *mbslen;

		while (mlen > 0)
		{
			qse_size_t n;

			if (q >= qend)
			{
				/* buffer too small */
				ret = -2;
				break;
			}

			n = qse_mbrtowc (p, mlen, q, &state);
			if (n == 0)
			{
				/* invalid sequence */
				ret = -1;
				break;
			}
			if (n > mlen)
			{
				/* incomplete sequence */
				ret = -3;
				break;
			}

			q++;
			p += n;
			mlen -= n;
		}

		*wcslen = q - wcs;
		*mbslen = p - mbs;
	}
	else
	{
		qse_wchar_t w;
		qse_size_t wlen = 0;

		p = mbs;
		mlen = *mbslen;

		while (mlen > 0)
		{
			qse_size_t n;

			n = qse_mbrtowc (p, mlen, &w, &state);
			if (n == 0)
			{
				/* invalid sequence */
				ret = -1;
				break;
			}
			if (n > mlen)
			{
				/* incomplete sequence */
				ret = -3;
				break;
			}

			p += n;
			mlen -= n;
			wlen += 1;
		}

		*wcslen = wlen;
		*mbslen = p - mbs;
	}

	return ret;
}

int qse_mbsntowcsnupto (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_wchar_t stopper)
{
	const qse_mchar_t* p;
	qse_mbstate_t state = {{ 0, }};
	int ret = 0;
	qse_size_t mlen;

	qse_wchar_t w;
	qse_size_t wlen = 0;
	qse_wchar_t* wend;

	p = mbs;
	mlen = *mbslen;

	if (wcs) wend = wcs + *wcslen;

	/* since it needs to break when a stopper is met,
	 * i can't perform bulky conversion using the buffer
	 * provided. so conversion is conducted character by
	 * character */
	while (mlen > 0)
	{
		qse_size_t n;

		n = qse_mbrtowc (p, mlen, &w, &state);
		if (n == 0)
		{
			/* invalid sequence */
			ret = -1;
			break;
		}
		if (n > mlen)
		{
			/* incomplete sequence */
			ret = -3;
			break;
		}

		if (wcs) 
		{
			if (wcs >= wend) break;
			*wcs++ = w;
		}

		p += n;
		mlen -= n;
		wlen += 1;

		if (w == stopper) break;
	}

	*wcslen = wlen;
	*mbslen = p - mbs;

	return ret;
}

qse_wchar_t* qse_mbstowcsdup (const qse_mchar_t* mbs, qse_mmgr_t* mmgr)
{
	qse_size_t mbslen, wcslen;
	qse_wchar_t* wcs;

	if (qse_mbstowcs (mbs, &mbslen, QSE_NULL, &wcslen) <= -1) return QSE_NULL;

	wcslen++; /* for terminating null */
	wcs = QSE_MMGR_ALLOC (mmgr, wcslen * QSE_SIZEOF(*wcs));	
	if (wcs == QSE_NULL) return QSE_NULL;

	qse_mbstowcs (mbs, &mbslen, wcs, &wcslen);
	return wcs;
}

qse_wchar_t* qse_mbsatowcsdup (const qse_mchar_t* mbs[], qse_mmgr_t* mmgr)
{
	qse_wchar_t* buf, * ptr;
	qse_size_t i;
	qse_size_t capa = 0;
	qse_size_t wl, ml;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (i = 0; mbs[i]; i++) 
	{
		if (qse_mbstowcs(mbs[i], &ml, QSE_NULL, &wl) <= -1) return QSE_NULL;
		capa += wl;
	}

	buf = (qse_wchar_t*) QSE_MMGR_ALLOC (
		mmgr, (capa + 1) * QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	ptr = buf;
	for (i = 0; mbs[i]; i++) 
	{
		wl = capa + 1;
		qse_mbstowcs (mbs[i], &ml, ptr, &wl);
		ptr += wl;
		capa -= wl;
	}

	return buf;
}

int qse_wcstombs (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen)
{
	const qse_wchar_t* p = wcs;
	qse_mbstate_t state = {{ 0, }};
	int ret = 0;

	if (mbs)
	{
		qse_size_t rem = *mbslen;

		while (*p != QSE_WT('\0'))
		{
			qse_size_t n;

			if (rem <= 0)
			{
				ret = -2;
				break;
			}
			
			n = qse_wcrtomb (*p, mbs, rem, &state);
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}
			if (n > rem) 
			{
				ret = -2;
				break; /* buffer too small */
			}

			mbs += n; rem -= n; p++;
		}

		/* update mbslen to the length of the mbs string converted excluding
		 * terminating null */
		*mbslen -= rem; 

		/* null-terminate the multibyte sequence if it has sufficient space */
		if (rem > 0) *mbs = QSE_MT('\0');
		else 
		{
			/* if ret is -2 and wcs[wcslen] == QSE_WT('\0'), 
			 * this means that the mbs buffer was lacking one
			 * slot for the terminating null */
			ret = -2; /* buffer too small */
		}
	}
	else
	{
		qse_mchar_t mbsbuf[QSE_MBLEN_MAX];
		qse_size_t mlen = 0;

		while (*p != QSE_WT('\0'))
		{
			qse_size_t n;

			n = qse_wcrtomb (*p, mbsbuf, QSE_COUNTOF(mbsbuf), &state);
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}

			/* it assumes that mbs is large enough to hold a character */
			QSE_ASSERT (n <= QSE_COUNTOF(mbs));

			p++; mlen += n;
		}

		/* this length holds the number of resulting multi-byte characters 
		 * excluding the terminating null character */
		*mbslen = mlen;
	}

	*wcslen = p - wcs;  /* the number of wide characters handled. */

	return ret;	
}

int qse_wcsntombsn (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen)
{
	const qse_wchar_t* p = wcs;
	const qse_wchar_t* end = wcs + *wcslen;
	qse_mbstate_t state = {{ 0, }};
	int ret = 0; 

	if (mbs)
	{
		qse_size_t rem = *mbslen;

		while (p < end) 
		{
			qse_size_t n;

			if (rem <= 0)
			{
				ret = -2; /* buffer too small */
				break;
			}

			n = qse_wcrtomb (*p, mbs, rem, &state);
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}
			if (n > rem) 
			{
				ret = -2; /* buffer too small */
				break;
			}
			mbs += n; rem -= n; p++;
		}

		*mbslen -= rem; 
	}
	else
	{
		qse_mchar_t mbsbuf[QSE_MBLEN_MAX];
		qse_size_t mlen = 0;

		while (p < end)
		{
			qse_size_t n;

			n = qse_wcrtomb (*p, mbsbuf, QSE_COUNTOF(mbsbuf), &state);
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}

			/* it assumes that mbs is large enough to hold a character */
			QSE_ASSERT (n <= QSE_COUNTOF(mbsbuf));

			p++; mlen += n;
		}

		/* this length excludes the terminating null character. 
		 * this function doesn't event null-terminate the result. */
		*mbslen = mlen;
	}

	*wcslen = p - wcs;

	return ret;
}

qse_mchar_t* qse_wcstombsdup (const qse_wchar_t* wcs, qse_mmgr_t* mmgr)
{
	qse_size_t wcslen, mbslen;
	qse_mchar_t* mbs;

	if (qse_wcstombs (wcs, &wcslen, QSE_NULL, &mbslen) <= -1) return QSE_NULL;

	mbslen++; /* for the terminating null character */

	mbs = QSE_MMGR_ALLOC (mmgr, mbslen * QSE_SIZEOF(*mbs));	
	if (mbs == QSE_NULL) return QSE_NULL;

	qse_wcstombs (wcs, &wcslen, mbs, &mbslen);
	return mbs;
}

qse_mchar_t* qse_wcsatombsdup (const qse_wchar_t* wcs[], qse_mmgr_t* mmgr)
{
	qse_mchar_t* buf, * ptr;
	qse_size_t i;
	qse_size_t wl, ml;
	qse_size_t capa = 0;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (i = 0; wcs[i]; i++) 
	{
		if (qse_wcstombs (wcs[i], &wl, QSE_NULL, &ml) <= -1) return QSE_NULL;
		capa += ml;
	}

	buf = (qse_mchar_t*) QSE_MMGR_ALLOC (
		mmgr, (capa + 1) * QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	ptr = buf;
	for (i = 0; wcs[i]; i++) 
	{
		ml = capa + 1;
		qse_wcstombs (wcs[i], &wl, ptr, &ml);
		ptr += ml;
		capa -= ml;
	}

	return buf;
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
