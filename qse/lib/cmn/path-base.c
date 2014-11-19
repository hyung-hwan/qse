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

#include <qse/cmn/path.h>

#define IS_MSEP(c) QSE_ISPATHMBSEP(c)
#define IS_WSEP(c) QSE_ISPATHWCSEP(c)

const qse_mchar_t* qse_mbsbasename (const qse_mchar_t* path)
{
	const qse_mchar_t* p, * last = QSE_NULL;

	for (p = path; *p != QSE_MT('\0'); p++)
	{
		if (IS_MSEP(*p)) last = p;
	}

	return (last == QSE_NULL)? path: (last + 1);
}

const qse_wchar_t* qse_wcsbasename (const qse_wchar_t* path)
{
	const qse_wchar_t* p, * last = QSE_NULL;

	for (p = path; *p != QSE_WT('\0'); p++)
	{
		if (IS_WSEP(*p)) last = p;
	}

	return (last == QSE_NULL)? path: (last + 1);
}
