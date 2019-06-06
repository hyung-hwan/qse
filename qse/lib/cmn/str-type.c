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

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include "mem-prv.h"

int qse_mbsistype (const qse_mchar_t* str, qse_mctype_t type)
{
	while (*str)
	{
		if (!qse_ismctype(*str, type)) return 0;
		str++;
	}
	return 1;
}

int qse_mbsxistype (const qse_mchar_t* str, qse_size_t len, qse_mctype_t type)
{
	const qse_mchar_t* end = str + len;
	while (str < end)
	{
		if (!qse_ismctype(*str, type)) return 0;
		str++;
	}
	return 1;
}

/* -------------------------------------------------------------------------- */

int qse_wcsistype (const qse_wchar_t* str, qse_wctype_t type)
{
	while (*str)
	{
		if (!qse_iswctype(*str, type)) return 0;
		str++;
	}
	return 1;
}

int qse_wcsxistype (const qse_wchar_t* str, qse_size_t len, qse_wctype_t type)
{
	const qse_wchar_t* end = str + len;
	while (str < end)
	{
		if (!qse_iswctype(*str, type)) return 0;
		str++;
	}
	return 1;
}
