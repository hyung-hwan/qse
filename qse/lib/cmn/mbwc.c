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

#include <qse/cmn/mbwc.h>
#include <qse/cmn/slmb.h>
#include <qse/cmn/utf8.h>
#include <qse/cmn/mb8.h>
#include <qse/cmn/cp949.h>
#include <qse/cmn/cp950.h>
#include <qse/cmn/str.h>

/* TODO: there is no guarantee that slwc is a unicode charater or vice versa.
 *       the ctype handling functions should be made wide-character
 *       dependent. 
 */

static qse_cmgr_t builtin_cmgr[] =
{
	/* keep the order aligned with qse_cmgr_id_t values in <qse/cmn/mbwc.h> */
	{ qse_slmbtoslwc, qse_slwctoslmb },
	{ qse_utf8touc,   qse_uctoutf8 },
	{ qse_mb8towc,    qse_wctomb8 }
#if defined(QSE_ENABLE_XCMGRS)
	,
	{ qse_cp949touc,  qse_uctocp949 },
	{ qse_cp950touc,  qse_uctocp950 }
#endif
};

static qse_cmgr_t* dfl_cmgr = &builtin_cmgr[QSE_CMGR_SLMB];
static qse_cmgr_finder_t cmgr_finder = QSE_NULL;

qse_cmgr_t* qse_getdflcmgr (void)
{
	return dfl_cmgr;
}

void qse_setdflcmgr (qse_cmgr_t* cmgr)
{
	dfl_cmgr = (cmgr? cmgr: &builtin_cmgr[QSE_CMGR_SLMB]);
}

void qse_setdflcmgrbyid (qse_cmgr_id_t id)
{
	qse_cmgr_t* cmgr = qse_findcmgrbyid (id);
	dfl_cmgr = (cmgr? cmgr: &builtin_cmgr[QSE_CMGR_SLMB]);
}

qse_cmgr_t* qse_findcmgrbyid (qse_cmgr_id_t id)
{
	if (id < 0 || id >= QSE_COUNTOF(builtin_cmgr)) return QSE_NULL;
	return &builtin_cmgr[id];
}

qse_cmgr_t* qse_findcmgr (const qse_char_t* name)
{
	/* TODO: binary search or something better for performance improvement 
	 *       when there are many entries in the table */
	static struct 
	{
		const qse_char_t* name;
		qse_cmgr_id_t     id;
	} tab[] =
	{
		{ QSE_T("utf8"),   QSE_CMGR_UTF8 },
#if defined(QSE_ENABLE_XCMGRS)
		{ QSE_T("cp949"),  QSE_CMGR_CP949 },
		{ QSE_T("cp950"),  QSE_CMGR_CP950 },
#endif
		{ QSE_T("slmb"),   QSE_CMGR_SLMB },
		{ QSE_T("mb8"),    QSE_CMGR_MB8 }
	};

	if (name)
	{
		qse_size_t i;

		if (cmgr_finder)
		{
			qse_cmgr_t* cmgr;
			cmgr = cmgr_finder (name);
			if (cmgr) return cmgr;
		}

		if (qse_strcasecmp(name, QSE_T("")) == 0) return dfl_cmgr;

		for (i = 0; i < QSE_COUNTOF(tab); i++)
		{
			if (qse_strcasecmp(name, tab[i].name) == 0) 
			{
				return &builtin_cmgr[tab[i].id];
			}
		}
	}

	return QSE_NULL;
}

void qse_setcmgrfinder (qse_cmgr_finder_t finder)
{
	cmgr_finder = finder;
}

qse_cmgr_finder_t qse_getcmgrfinder (void)
{
	return cmgr_finder;
}

/* string conversion function using default character conversion manager */

int qse_mbstowcs (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	return qse_mbstowcswithcmgr (mbs, mbslen, wcs, wcslen, dfl_cmgr);
}

int qse_mbstowcsall (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	return qse_mbstowcsallwithcmgr (mbs, mbslen, wcs, wcslen, dfl_cmgr);
}

int qse_mbsntowcsn (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	return qse_mbsntowcsnwithcmgr (mbs, mbslen, wcs, wcslen, dfl_cmgr);
}

int qse_mbsntowcsnall (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen)
{
	return qse_mbsntowcsnallwithcmgr (mbs, mbslen, wcs, wcslen, dfl_cmgr);
}

int qse_mbsntowcsnupto (
	const qse_mchar_t* mbs, qse_size_t* mbslen,
	qse_wchar_t* wcs, qse_size_t* wcslen, qse_wchar_t stopper)
{
	return qse_mbsntowcsnuptowithcmgr (
		mbs, mbslen, wcs, wcslen, stopper, dfl_cmgr);
}

qse_wchar_t* qse_mbstowcsdup (
	const qse_mchar_t* mbs, qse_size_t* wcslen, qse_mmgr_t* mmgr)
{
	return qse_mbstowcsdupwithcmgr (mbs, wcslen, mmgr, dfl_cmgr);
}

qse_wchar_t* qse_mbstowcsalldup (
	const qse_mchar_t* mbs, qse_size_t* wcslen, qse_mmgr_t* mmgr)
{
	return qse_mbstowcsalldupwithcmgr (mbs, wcslen, mmgr, dfl_cmgr);
}

qse_wchar_t* qse_mbsntowcsdup (
	const qse_mchar_t* mbs, qse_size_t* mbslen, qse_size_t* wcslen, qse_mmgr_t* mmgr)
{
	return qse_mbsntowcsdupwithcmgr (mbs, mbslen, wcslen, mmgr, dfl_cmgr);
}

qse_wchar_t* qse_mbsntowcsalldup (
	const qse_mchar_t* mbs, qse_size_t* mbslen, qse_size_t* wcslen, qse_mmgr_t* mmgr)
{
	return qse_mbsntowcsalldupwithcmgr (mbs, mbslen, wcslen, mmgr, dfl_cmgr);
}

qse_wchar_t* qse_mbsatowcsdup (
	const qse_mchar_t* mbs[], qse_size_t* wcslen, qse_mmgr_t* mmgr)
{
	return qse_mbsatowcsdupwithcmgr (mbs, wcslen, mmgr, dfl_cmgr);
}

qse_wchar_t* qse_mbsatowcsalldup (
	const qse_mchar_t* mbs[], qse_size_t* wcslen, qse_mmgr_t* mmgr)
{
	return qse_mbsatowcsalldupwithcmgr (mbs, wcslen, mmgr, dfl_cmgr);
}

/* -------------------------------------------------------------- */

int qse_wcstombs (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen)
{
	return qse_wcstombswithcmgr (wcs, wcslen, mbs, mbslen, dfl_cmgr);
}

int qse_wcsntombsn (
	const qse_wchar_t* wcs, qse_size_t* wcslen,
	qse_mchar_t* mbs, qse_size_t* mbslen)
{
	return qse_wcsntombsnwithcmgr (wcs, wcslen, mbs, mbslen, dfl_cmgr);
}

qse_mchar_t* qse_wcstombsdup (
	const qse_wchar_t* wcs, qse_size_t* mbslen, qse_mmgr_t* mmgr)
{
	return qse_wcstombsdupwithcmgr (wcs, mbslen, mmgr, dfl_cmgr);
}

qse_mchar_t* qse_wcsntombsdup (
	const qse_wchar_t* wcs, qse_size_t wcslen, 
	qse_size_t* mbslen, qse_mmgr_t* mmgr)
{
	return qse_wcsntombsdupwithcmgr (wcs, wcslen, mbslen, mmgr, dfl_cmgr);
}

qse_mchar_t* qse_wcsatombsdup (
	const qse_wchar_t* wcs[], qse_size_t* mbslen, qse_mmgr_t* mmgr)
{
	return qse_wcsatombsdupwithcmgr (wcs, mbslen, mmgr, dfl_cmgr);
}

qse_mchar_t* qse_wcsnatombsdup (
	const qse_wcstr_t wcs[], qse_size_t* mbslen, qse_mmgr_t* mmgr)
{
	return qse_wcsnatombsdupwithcmgr (wcs, mbslen, mmgr, dfl_cmgr);
}
