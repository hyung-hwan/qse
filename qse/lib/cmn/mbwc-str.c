/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#include <qse/cmn/mbwc.h>
#include "mem.h"

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
	n = mbsn_to_wcsn_with_cmgr (mbs, &mlen, wcs, &wlen, cmgr, all);
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
	return mbs_to_wcs_with_cmgr (mbs, mbslen, wcs, wcslen, cmgr, 0);
}

int qse_mbstowcsallwithcmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_cmgr_t* cmgr)
{
	return mbs_to_wcs_with_cmgr (mbs, mbslen, wcs, wcslen, cmgr, 1);
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

			n = cmgr->mbtowc (p, mlen, q);
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

			n = cmgr->mbtowc (p, mlen, &w);
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
	return mbsn_to_wcsn_with_cmgr (mbs, mbslen, wcs, wcslen, cmgr, 0);
}

int qse_mbsntowcsnallwithcmgr (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_cmgr_t* cmgr)
{
	return mbsn_to_wcsn_with_cmgr (mbs, mbslen, wcs, wcslen, cmgr, 1);
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

static qse_wchar_t* mbs_to_wcs_dup_with_cmgr (
	const qse_mchar_t* mbs, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr, int all)
{
	qse_size_t mbslen, wcslen;
	qse_wchar_t* wcs;

	if (mbs_to_wcs_with_cmgr (
		mbs, &mbslen, QSE_NULL, &wcslen, cmgr, all) <= -1) return QSE_NULL;

	wcslen++; /* for terminating null */
	wcs = QSE_MMGR_ALLOC (mmgr, wcslen * QSE_SIZEOF(*wcs));	
	if (wcs == QSE_NULL) return QSE_NULL;

	mbs_to_wcs_with_cmgr (mbs, &mbslen, wcs, &wcslen, cmgr, all);
	return wcs;
}

qse_wchar_t* qse_mbstowcsdupwithcmgr (
	const qse_mchar_t* mbs, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbs_to_wcs_dup_with_cmgr (mbs, mmgr, cmgr, 0);
}

qse_wchar_t* qse_mbstowcsalldupwithcmgr (
	const qse_mchar_t* mbs, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbs_to_wcs_dup_with_cmgr (mbs, mmgr, cmgr, 1);
}

static qse_wchar_t* mbsa_to_wcs_dup_with_cmgr (
	const qse_mchar_t* mbs[], qse_mmgr_t* mmgr, qse_cmgr_t* cmgr, int all)
{
	qse_wchar_t* buf, * ptr;
	qse_size_t i;
	qse_size_t capa = 0;
	qse_size_t wl, ml;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (i = 0; mbs[i]; i++) 
	{
		if (mbs_to_wcs_with_cmgr (mbs[i], &ml, QSE_NULL, &wl, cmgr, all) <= -1) 
			return QSE_NULL;
		capa += wl;
	}

	buf = (qse_wchar_t*) QSE_MMGR_ALLOC (
		mmgr, (capa + 1) * QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	ptr = buf;
	for (i = 0; mbs[i]; i++) 
	{
		wl = capa + 1;
		mbs_to_wcs_with_cmgr (mbs[i], &ml, ptr, &wl, cmgr, all);
		ptr += wl;
		capa -= wl;
	}

	return buf;
}

qse_wchar_t* qse_mbsatowcsdupwithcmgr (
	const qse_mchar_t* mbs[], qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbsa_to_wcs_dup_with_cmgr (mbs, mmgr, cmgr, 0);
}

qse_wchar_t* qse_mbsatowcsalldupwithcmgr (
	const qse_mchar_t* mbs[], qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	return mbsa_to_wcs_dup_with_cmgr (mbs, mmgr, cmgr, 1);
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

			n = cmgr->wctomb (*p, mbsbuf, QSE_COUNTOF(mbsbuf));
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

qse_mchar_t* qse_wcstombsdupwithcmgr (const qse_wchar_t* wcs, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	qse_size_t wcslen, mbslen;
	qse_mchar_t* mbs;

	if (qse_wcstombswithcmgr (wcs, &wcslen, QSE_NULL, &mbslen, cmgr) <= -1) return QSE_NULL;

	mbslen++; /* for the terminating null character */

	mbs = QSE_MMGR_ALLOC (mmgr, mbslen * QSE_SIZEOF(*mbs));	
	if (mbs == QSE_NULL) return QSE_NULL;

	qse_wcstombswithcmgr (wcs, &wcslen, mbs, &mbslen, cmgr);
	return mbs;
}

qse_mchar_t* qse_wcsntombsdupwithcmgr (const qse_wchar_t* wcs, qse_size_t len, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	qse_size_t mbslen;
	qse_mchar_t* mbs;

	if (qse_wcsntombsnwithcmgr (wcs, &len, QSE_NULL, &mbslen, cmgr) <= -1) return QSE_NULL;

	mbs = QSE_MMGR_ALLOC (mmgr, (mbslen + 1) * QSE_SIZEOF(*mbs));	
	if (mbs == QSE_NULL) return QSE_NULL;

	qse_wcsntombsnwithcmgr (wcs, &len, mbs, &mbslen, cmgr);
	mbs[mbslen] = QSE_MT('\0');
	return mbs;
}

qse_mchar_t* qse_wcsatombsdupwithcmgr (const qse_wchar_t* wcs[], qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	qse_mchar_t* buf, * ptr;
	qse_size_t i;
	qse_size_t wl, ml;
	qse_size_t capa = 0;

	QSE_ASSERT (mmgr != QSE_NULL);

	for (i = 0; wcs[i]; i++) 
	{
		if (qse_wcstombswithcmgr (wcs[i], &wl, QSE_NULL, &ml, cmgr) <= -1) return QSE_NULL;
		capa += ml;
	}

	buf = (qse_mchar_t*) QSE_MMGR_ALLOC (
		mmgr, (capa + 1) * QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	ptr = buf;
	for (i = 0; wcs[i]; i++) 
	{
		ml = capa + 1;
		qse_wcstombswithcmgr (wcs[i], &wl, ptr, &ml, cmgr);
		ptr += ml;
		capa -= ml;
	}

	return buf;
}
