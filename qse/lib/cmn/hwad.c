/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redimbsibution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redimbsibutions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redimbsibutions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the dimbsibution.

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

#include <qse/cmn/hwad.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/fmt.h>

int qse_mbstoethwad (const qse_mchar_t* mbs, qse_ethwad_t* hwaddr)
{
	qse_size_t idx = 0;
	const qse_mchar_t* p = mbs;
	int tmp;

	/*while (QSE_ISMSPACE(*p)) p++;*/

	while (*p != QSE_MT('\0'))
	{
		tmp = QSE_MXDIGITTONUM (*p);
		if (tmp <= -1) return -1;
		hwaddr->value[idx] = hwaddr->value[idx] * 16 + tmp;
		p++;

		tmp = QSE_MXDIGITTONUM (*p);
		if (tmp <= -1) return -1;
		hwaddr->value[idx] = hwaddr->value[idx] * 16 + tmp;
		p++;

		if (idx < QSE_ETHWAD_LEN - 1)
		{
			if (*p != QSE_MT(':')) return -1;
			p++; idx++;
		 }
		 else
		 {
			return (*p == QSE_MT('\0'))? 0: -1;
		 }
	}

	return -1;
}

int qse_mbsxtoethwaddr (const qse_mchar_t* mbs, qse_size_t len, qse_ethwad_t* hwaddr)
{
	qse_size_t idx = 0;
	const qse_mchar_t* p = mbs, * end = mbs + len;
	int tmp;

	/*while (QSE_ISWSPACE(*p)) p++;*/

	while (p < end)
	{
		tmp = QSE_MXDIGITTONUM (*p);
		if (tmp <= -1) return -1;
		hwaddr->value[idx] = hwaddr->value[idx] * 16 + tmp;
		p++;

		if (p >= end) return -1;
		tmp = QSE_MXDIGITTONUM (*p);
		if (tmp <= -1) return -1;
		hwaddr->value[idx] = hwaddr->value[idx] * 16 + tmp;
		p++;

		if (idx < QSE_ETHWAD_LEN - 1)
		{
			if (p >= end || *p != QSE_MT(':')) return -1;
			p++; idx++;
		 }
		 else
		 {
			return (p >= end)? 0: -1;
		 }

	}

	return -1;
}

qse_size_t qse_ethwadtombs (const qse_ethwad_t* hwaddr, qse_mchar_t* buf, qse_size_t size)
{
	qse_size_t i;
	qse_mchar_t tmp[QSE_ETHWAD_LEN][3];

	if (size <= 0) return 0;

	QSE_ASSERT (QSE_COUNTOF(hwaddr->value) == QSE_ETHWAD_LEN);

	for (i = 0; i < QSE_COUNTOF(hwaddr->value); i++)
	{
		qse_fmtuintmaxtombs (tmp[i], QSE_COUNTOF(tmp[i]), hwaddr->value[i], 16 | QSE_FMTUINTMAXTOMBS_UPPERCASE, 2, QSE_MT('\0'), QSE_NULL);
	}

	return qse_mbsxjoin (buf, size,
		 tmp[0], QSE_MT(":"), tmp[1], QSE_MT(":"),
		 tmp[2], QSE_MT(":"), tmp[3], QSE_MT(":"),
		 tmp[4], QSE_MT(":"), tmp[5], QSE_NULL);
}

/* -------------------------------------------------------------------------- */

int qse_wcstoethwad (const qse_wchar_t* wcs, qse_ethwad_t* hwaddr)
{
	qse_size_t idx = 0;
	const qse_wchar_t* p = wcs;
	int tmp;

	/*while (qse_ismspace(*p)) p++;*/

	while (*p != QSE_WT('\0'))
	{
		tmp = QSE_WXDIGITTONUM (*p);
		if (tmp <= -1) return -1;
		hwaddr->value[idx] = hwaddr->value[idx] * 16 + tmp;
		p++;

		tmp = QSE_WXDIGITTONUM (*p);
		if (tmp <= -1) return -1;
		hwaddr->value[idx] = hwaddr->value[idx] * 16 + tmp;
		p++;

		if (idx < QSE_ETHWAD_LEN - 1)
		{
			if (*p != QSE_WT(':')) return -1;
			p++; idx++;
		 }
		 else
		 {
			return (*p == QSE_WT('\0'))? 0: -1;
		 }
	}

	return -1;
}

int qse_wcsxtoethwaddr (const qse_wchar_t* wcs, qse_size_t len, qse_ethwad_t* hwaddr)
{
	qse_size_t idx = 0;
	const qse_wchar_t* p = wcs, * end = wcs + len;
	int tmp;

	/*while (qse_ismspace(*p)) p++;*/

	while (p < end)
	{
		tmp = QSE_WXDIGITTONUM (*p);
		if (tmp <= -1) return -1;
		hwaddr->value[idx] = hwaddr->value[idx] * 16 + tmp;
		p++;

		if (p >= end) return -1;
		tmp = QSE_WXDIGITTONUM (*p);
		if (tmp <= -1) return -1;
		hwaddr->value[idx] = hwaddr->value[idx] * 16 + tmp;
		p++;

		if (idx < QSE_ETHWAD_LEN - 1)
		{
			if (p >= end || *p != QSE_WT(':')) return -1;
			p++; idx++;
		 }
		 else
		 {
			return (p >= end)? 0: -1;
		 }

	}

	return -1;
}

qse_size_t qse_ethwadtowcs (const qse_ethwad_t* hwaddr, qse_wchar_t* buf, qse_size_t size)
{
	qse_size_t i;
	qse_wchar_t tmp[QSE_ETHWAD_LEN][3];

	if (size <= 0) return 0;

	QSE_ASSERT (QSE_COUNTOF(hwaddr->value) == QSE_ETHWAD_LEN);

	for (i = 0; i < QSE_COUNTOF(hwaddr->value); i++)
	{
		qse_fmtuintmaxtowcs (tmp[i], QSE_COUNTOF(tmp[i]), hwaddr->value[i], 16 | QSE_FMTUINTMAXTOWCS_UPPERCASE, 2, QSE_WT('\0'), QSE_NULL);
	}

	return qse_wcsxjoin (buf, size,
		 tmp[0], QSE_WT(":"), tmp[1], QSE_WT(":"),
		 tmp[2], QSE_WT(":"), tmp[3], QSE_WT(":"),
		 tmp[4], QSE_WT(":"), tmp[5], QSE_NULL);
}
