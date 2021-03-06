/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#include <qse/cmn/mbwc.h>
#include "mem-prv.h"

static int mbsn_to_wcsn_with_cmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_cmgr_t* cmgr, int all);

static int mbs_to_wcs_with_cmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_cmgr_t* cmgr, int all)
{
	const qse_mchar_t* mp;
	qse_size_t mlen, wlen;
	int n;

	for (mp = mbs; *mp != QSE_MT('\0'); mp++);

	mlen = mp - mbs; wlen = *wcslen;
	n = mbsn_to_wcsn_with_cmgr(mbs, &mlen, wcs, &wlen, cmgr, all);
	if (wcs)
	{
		if (wlen < *wcslen) wcs[wlen] = QSE_WT('\0');
		else n = -2; /* buffer too small */
	}
	*mbslen = mlen; *wcslen = wlen;

	return n;
}

int qse_mbstowcswithcmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_cmgr_t* cmgr)
{
	return mbs_to_wcs_with_cmgr(mbs, mbslen, wcs, wcslen, cmgr, 0);
}

int qse_mbstowcsallwithcmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_cmgr_t* cmgr)
{
	return mbs_to_wcs_with_cmgr(mbs, mbslen, wcs, wcslen, cmgr, 1);
}

static int mbsn_to_wcsn_with_cmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_cmgr_t* cmgr, int all)
{
	const qse_mchar_t* p;
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

			n = cmgr->mbtowc(p, mlen, q);
			if (n == 0)
			{
				/* invalid sequence */
				if (all)
				{
					n = 1;
					*q = QSE_WT('?');	
				}
				else
				{
					ret = -1;
					break;
				}
			}
			if (n > mlen)
			{
				/* incomplete sequence */
				if (all)
				{
					n = 1;
					*q = QSE_WT('?');	
				}
				else
				{
					ret = -3;
					break;
				}
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

			n = cmgr->mbtowc(p, mlen, &w);
			if (n == 0)
			{
				/* invalid sequence */
				if (all) n = 1;
				else
				{
					ret = -1;
					break;
				}
			}
			if (n > mlen)
			{
				/* incomplete sequence */
				if (all) n = 1;
				else
				{
					ret = -3;
					break;
				}
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

int qse_mbsntowcsnwithcmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_cmgr_t* cmgr)
{
	return mbsn_to_wcsn_with_cmgr(mbs, mbslen, wcs, wcslen, cmgr, 0);
}

int qse_mbsntowcsnallwithcmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_cmgr_t* cmgr)
{
	return mbsn_to_wcsn_with_cmgr(mbs, mbslen, wcs, wcslen, cmgr, 1);
}

int qse_mbsntowcsnuptowithcmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_wchar_t stopper, qse_cmgr_t* cmgr)
{
	const qse_mchar_t* p;
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

		n = cmgr->mbtowc (p, mlen, &w);
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

static qse_wchar_t* mbsn_to_wcs_dup_with_cmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen, qse_size_t* wcslen,
	qse_mmgr_t* mmgr, qse_cmgr_t* cmgr, int all)
{
	qse_size_t ml, wl;
	qse_wchar_t* wcs;

	ml = *mbslen;
	if (mbsn_to_wcsn_with_cmgr(mbs, &ml, QSE_NULL, &wl, cmgr, all) <= -1) return QSE_NULL;

	wl++; /* for terminating null */
	wcs = (qse_wchar_t*)QSE_MMGR_ALLOC(mmgr, wl * QSE_SIZEOF(*wcs));	
	if (!wcs) return QSE_NULL;

	mbsn_to_wcsn_with_cmgr (mbs, mbslen, wcs, &wl, cmgr, all);
	wcs[wl] = QSE_WT('\0');

	if (wcslen) *wcslen = wl;
	return wcs;
}

qse_wchar_t* qse_mbsntowcsdupwithcmgr (const qse_mchar_t* mbs, qse_size_t* mbslen, qse_size_t* wcslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbsn_to_wcs_dup_with_cmgr (mbs, mbslen, wcslen, mmgr, cmgr, 0);
}

qse_wchar_t* qse_mbsntowcsalldupwithcmgr (const qse_mchar_t* mbs, qse_size_t* mbslen, qse_size_t* wcslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbsn_to_wcs_dup_with_cmgr (mbs, mbslen, wcslen, mmgr, cmgr, 1);
}

static qse_wchar_t* mbs_to_wcs_dup_with_cmgr (const qse_mchar_t* mbs, qse_size_t* wcslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr, int all)
{
	qse_size_t ml, wl;
	qse_wchar_t* wcs;

	if (mbs_to_wcs_with_cmgr(mbs, &ml, QSE_NULL, &wl, cmgr, all) <= -1) return QSE_NULL;

	wl++; /* for terminating null */
	wcs = (qse_wchar_t*)QSE_MMGR_ALLOC(mmgr, wl * QSE_SIZEOF(*wcs));
	if (!wcs) return QSE_NULL;

	mbs_to_wcs_with_cmgr (mbs, &ml, wcs, &wl, cmgr, all);

	if (wcslen) *wcslen = wl;
	return wcs;
}

qse_wchar_t* qse_mbstowcsdupwithcmgr (const qse_mchar_t* mbs, qse_size_t* wcslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbs_to_wcs_dup_with_cmgr(mbs, wcslen, mmgr, cmgr, 0);
}

qse_wchar_t* qse_mbstowcsalldupwithcmgr (const qse_mchar_t* mbs, qse_size_t* wcslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbs_to_wcs_dup_with_cmgr(mbs, wcslen, mmgr, cmgr, 1);
}

static qse_wchar_t* mbsa_to_wcs_dup_with_cmgr (const qse_mchar_t* mbs[], qse_size_t* wcslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr, int all)
{
	qse_wchar_t* buf;
	qse_size_t i;
	qse_size_t capa, pos;
	qse_size_t wl, ml;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (capa = 0, i = 0; mbs[i]; i++) 
	{
		if (mbs_to_wcs_with_cmgr (mbs[i], &ml, QSE_NULL, &wl, cmgr, all) <= -1) 
			return QSE_NULL;
		capa += wl;
	}

	buf = (qse_wchar_t*) QSE_MMGR_ALLOC (
		mmgr, (capa + 1) * QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	for (pos = 0, i = 0; mbs[i]; i++) 
	{
		wl = capa - pos + 1;
		mbs_to_wcs_with_cmgr (mbs[i], &ml, &buf[pos], &wl, cmgr, all);
		pos += wl;
	}

	QSE_ASSERT (pos == capa);

	if (wcslen) *wcslen = capa;
	return buf;
}

qse_wchar_t* qse_mbsatowcsdupwithcmgr (const qse_mchar_t* mbs[], qse_size_t* wcslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbsa_to_wcs_dup_with_cmgr(mbs, wcslen, mmgr, cmgr, 0);
}

qse_wchar_t* qse_mbsatowcsalldupwithcmgr (const qse_mchar_t* mbs[], qse_size_t* wcslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbsa_to_wcs_dup_with_cmgr(mbs, wcslen, mmgr, cmgr, 1);
}

/* ======================================================================== */

int qse_wcstombswithcmgr (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen, qse_cmgr_t* cmgr)
{
	const qse_wchar_t* p = wcs;
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
			
			n = cmgr->wctomb (*p, mbs, rem);
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

			n = cmgr->wctomb(*p, mbsbuf, QSE_COUNTOF(mbsbuf));
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}

			/* it assumes that mbsbuf is large enough to hold a character.
			 * since mbsbuf is of the QSE_MBLEN_MAX size, the return value
			 * must not exceed the size of mbsbuf. */
			QSE_ASSERT (n <= QSE_COUNTOF(mbsbuf));

			p++; mlen += n;
		}

		/* this length holds the number of resulting multi-byte characters 
		 * excluding the terminating null character */
		*mbslen = mlen;
	}

	*wcslen = p - wcs;  /* the number of wide characters handled. */

	return ret;	
}

int qse_wcsntombsnwithcmgr (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen, qse_cmgr_t* cmgr)
{
	const qse_wchar_t* p = wcs;
	const qse_wchar_t* end = wcs + *wcslen;
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

			n = cmgr->wctomb (*p, mbs, rem);
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

			n = cmgr->wctomb (*p, mbsbuf, QSE_COUNTOF(mbsbuf));
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

qse_mchar_t* qse_wcstombsdupwithcmgr (
	const qse_wchar_t* wcs, qse_size_t* mbslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	qse_size_t wl, ml;
	qse_mchar_t* mbs;

	if (qse_wcstombswithcmgr (wcs, &wl, QSE_NULL, &ml, cmgr) <= -1) return QSE_NULL;

	ml++; /* for the terminating null character */

	mbs = QSE_MMGR_ALLOC(mmgr, ml * QSE_SIZEOF(*mbs));	
	if (mbs == QSE_NULL) return QSE_NULL;

	qse_wcstombswithcmgr (wcs, &wl, mbs, &ml, cmgr);

	if (mbslen) *mbslen = ml;
	return mbs;
}

qse_mchar_t* qse_wcsntombsdupwithcmgr (
	const qse_wchar_t* wcs, qse_size_t wcslen,
	qse_size_t* mbslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	qse_size_t wl, ml;
	qse_mchar_t* mbs;

	wl = wcslen;
	if (qse_wcsntombsnwithcmgr(wcs, &wl, QSE_NULL, &ml, cmgr) <= -1) return QSE_NULL;

	mbs = QSE_MMGR_ALLOC (mmgr, (ml + 1) * QSE_SIZEOF(*mbs));	
	if (mbs == QSE_NULL) return QSE_NULL;

	wl = wcslen;
	qse_wcsntombsnwithcmgr (wcs, &wl, mbs, &ml, cmgr);
	mbs[ml] = QSE_MT('\0');

	if (mbslen) *mbslen = ml;
	return mbs;
}

qse_mchar_t* qse_wcsatombsdupwithcmgr (
	const qse_wchar_t* wcs[], qse_size_t* mbslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	qse_size_t wl, ml, capa, pos, i;
	qse_mchar_t* mbs;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (capa = 0, i = 0; wcs[i]; i++) 
	{
		if (qse_wcstombswithcmgr (wcs[i], &wl, QSE_NULL, &ml, cmgr) <= -1) return QSE_NULL;
		capa += ml;
	}

	mbs = (qse_mchar_t*) QSE_MMGR_ALLOC (mmgr, (capa + 1) * QSE_SIZEOF(*mbs));
	if (mbs == QSE_NULL) return QSE_NULL;

	for (pos = 0, i = 0; wcs[i]; i++) 
	{
		ml = capa - pos + 1;
		qse_wcstombswithcmgr (wcs[i], &wl, &mbs[pos], &ml, cmgr);
		pos += ml;
	}

	if (mbslen) *mbslen = capa;
	return mbs;
}

qse_mchar_t* qse_wcsnatombsdupwithcmgr (
	const qse_wcstr_t wcs[], qse_size_t* mbslen, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	qse_size_t wl, ml, capa, pos, i;
	qse_mchar_t* mbs;

	for (capa = 0, i = 0; wcs[i].ptr; i++)
	{
		wl = wcs[i].len;
		if (qse_wcsntombsnwithcmgr (wcs[i].ptr, &wl, QSE_NULL, &ml, cmgr) <= -1) return QSE_NULL;
		capa += ml;
	}

	mbs = QSE_MMGR_ALLOC (mmgr, (capa + 1) * QSE_SIZEOF(*mbs));	
	if (mbs == QSE_NULL) return QSE_NULL;

	for (pos = 0, i = 0; wcs[i].ptr; i++)
	{
		wl = wcs[i].len;
		ml = capa - pos + 1;
		qse_wcsntombsnwithcmgr (wcs[i].ptr, &wl, &mbs[pos], &ml, cmgr);
		pos += ml;
	}
	mbs[pos] = QSE_MT('\0');

	QSE_ASSERT (pos == capa);

	if (mbslen) *mbslen = capa;
	return mbs;
}
