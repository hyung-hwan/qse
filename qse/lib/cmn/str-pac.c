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

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>

qse_size_t qse_mbspac (qse_mchar_t* str)
{
	qse_mchar_t* p = str, * q = str;

	while (QSE_ISMSPACE(*p)) p++;
	while (*p != QSE_MT('\0')) 
	{
		if (QSE_ISMSPACE(*p)) 
		{
			*q++ = *p++;
			while (QSE_ISMSPACE(*p)) p++;
		}
		else *q++ = *p++;
	}

	if (q > str && QSE_ISMSPACE(q[-1])) q--;
	*q = QSE_MT('\0');

	return q - str;
}

qse_size_t qse_mbsxpac (qse_mchar_t* str, qse_size_t len)
{
	qse_mchar_t* p = str, * q = str, * end = str + len;
	int followed_by_space = 0;
	int state = 0;

	while (p < end) 
	{
		if (state == 0) 
		{
			if (!QSE_ISMSPACE(*p)) 
			{
				*q++ = *p;
				state = 1;
			}
		}
		else if (state == 1) 
		{
			if (QSE_ISMSPACE(*p)) 
			{
				if (!followed_by_space) 
				{
					followed_by_space = 1;
					*q++ = *p;
				}
			}
			else 
			{
				followed_by_space = 0;
				*q++ = *p;	
			}
		}

		p++;
	}

	return (followed_by_space) ? (q - str -1): (q - str);
}

qse_size_t qse_wcspac (qse_wchar_t* str)
{
	qse_wchar_t* p = str, * q = str;

	while (QSE_ISWSPACE(*p)) p++;
	while (*p != QSE_WT('\0')) 
	{
		if (QSE_ISWSPACE(*p)) 
		{
			*q++ = *p++;
			while (QSE_ISWSPACE(*p)) p++;
		}
		else *q++ = *p++;
	}

	if (q > str && QSE_ISWSPACE(q[-1])) q--;
	*q = QSE_WT('\0');

	return q - str;
}

qse_size_t qse_wcsxpac (qse_wchar_t* str, qse_size_t len)
{
	qse_wchar_t* p = str, * q = str, * end = str + len;
	int followed_by_space = 0;
	int state = 0;

	while (p < end) 
	{
		if (state == 0) 
		{
			if (!QSE_ISWSPACE(*p)) 
			{
				*q++ = *p;
				state = 1;
			}
		}
		else if (state == 1) 
		{
			if (QSE_ISWSPACE(*p)) 
			{
				if (!followed_by_space) 
				{
					followed_by_space = 1;
					*q++ = *p;
				}
			}
			else 
			{
				followed_by_space = 0;
				*q++ = *p;	
			}
		}

		p++;
	}

	return (followed_by_space) ? (q - str -1): (q - str);
}
