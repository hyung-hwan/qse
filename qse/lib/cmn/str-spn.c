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

qse_size_t qse_mbsspn (const qse_mchar_t* str1, const qse_mchar_t* str2)
{
	const qse_mchar_t* p1, * p2;
	qse_size_t n = 0;

	for (p1 = str1; *p1 != QSE_MT('\0'); p1++)
	{
		for (p2 = str2; *p2 != QSE_MT('\0'); p2++)
		{
			if (*p2 == *p1) goto matched;
		}

		/* didn't match anything  */
		break;

	matched:
		n++;
	}

	return n;
}

qse_size_t qse_wcsspn (const qse_wchar_t* str1, const qse_wchar_t* str2)
{
	const qse_wchar_t* p1, * p2;
	qse_size_t n = 0;

	for (p1 = str1; *p1 != QSE_WT('\0'); p1++)
	{
		for (p2 = str2; *p2 != QSE_WT('\0'); p2++)
		{
			if (*p2 == *p1) goto matched;
		}

		/* didn't match anything  */
		break;

	matched:
		n++;
	}

	return n;
}

qse_size_t qse_mbscspn (const qse_mchar_t* str1, const qse_mchar_t* str2)
{
	const qse_mchar_t* p1, * p2;
	qse_size_t n = 0;

	for (p1 = str1; *p1 != QSE_WT('\0'); p1++)
	{
		for (p2 = str2; *p2 != QSE_WT('\0'); p2++)
		{
			if (*p2 == *p1) goto done;
		}

		/* didn't match anything. increment the length */
		n++;
	}

done:
	return n;
}

qse_size_t qse_wcscspn (const qse_wchar_t* str1, const qse_wchar_t* str2)
{
	const qse_wchar_t* p1, * p2;
	qse_size_t n = 0;

	for (p1 = str1; *p1 != QSE_WT('\0'); p1++)
	{
		for (p2 = str2; *p2 != QSE_WT('\0'); p2++)
		{
			if (*p2 == *p1) goto done;
		}

		/* didn't match anything. increment the length */
		n++;
	}

done:
	return n;
}
