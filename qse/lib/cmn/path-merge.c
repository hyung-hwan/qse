/*
 * $Id
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

#include <qse/cmn/path.h>
#include <qse/cmn/str.h>

qse_mchar_t* qse_mergembspathdup (const qse_mchar_t* dir, const qse_mchar_t* file, qse_mmgr_t* mmgr)
{
	const qse_mchar_t* seg[4];
	qse_mchar_t tmp[2];
	int idx = 0;

	if (*dir != QSE_MT('\0'))
	{
		seg[idx++] = dir;

	#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		while (!QSE_ISPATHMBSEPORNIL(*dir)) dir++;
		if (QSE_ISPATHMBSEP(*dir)) tmp[0] = *dir;
		else
		{
			while (!QSE_ISPATHMBSEPORNIL(*file)) file++;
			if (QSE_ISPATHMBSEP(*file)) tmp[0] = *file;
			else tmp[0] = QSE_MT('\\');
		}
	#else
		tmp[0] = QSE_MT('/');
		tmp[1] = QSE_MT('\0');
	#endif

		while (*dir != QSE_MT('\0')) dir++;

		if (!QSE_ISPATHMBSEP(*(dir - 1)) && !QSE_ISPATHMBSEP(*file))
		{
			seg[idx++] = tmp;
		}
	}

	seg[idx++] = file;
	seg[idx++] = QSE_NULL;

	return qse_mbsadup (seg, QSE_NULL, mmgr);
}

qse_wchar_t* qse_mergewcspathdup (const qse_wchar_t* dir, const qse_wchar_t* file, qse_mmgr_t* mmgr)
{
	const qse_wchar_t* seg[4];
	qse_wchar_t tmp[2];
	int idx = 0;

	if (*dir != QSE_WT('\0'))
	{
		seg[idx++] = dir;

	#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		while (!QSE_ISPATHWCSEPORNIL(*dir)) dir++;
		if (QSE_ISPATHWCSEP(*dir)) tmp[0] = *dir;
		else
		{
			while (!QSE_ISPATHWCSEPORNIL(*file)) file++;
			if (QSE_ISPATHWCSEP(*file)) tmp[0] = *file;
			else tmp[0] = QSE_WT('\\');
		}
	#else
		tmp[0] = QSE_WT('/');
		tmp[1] = QSE_WT('\0');
	#endif

		while (*dir != QSE_WT('\0')) dir++;

		if (!QSE_ISPATHWCSEP(*(dir - 1)) && !QSE_ISPATHWCSEP(*file))
		{
			seg[idx++] = tmp;
		}
	}

	seg[idx++] = file;
	seg[idx++] = QSE_NULL;

	return qse_wcsadup (seg, QSE_NULL, mmgr);
}
