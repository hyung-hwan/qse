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

qse_mchar_t* qse_mbsend (const qse_mchar_t* str, const qse_mchar_t* sub)
{
	return qse_mbsxnend (str, qse_mbslen(str), sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsxend (
	const qse_mchar_t* str, qse_size_t len, const qse_mchar_t* sub)
{
	return qse_mbsxnend (str, len, sub, qse_mbslen(sub));
}

qse_mchar_t* qse_mbsnend (
	const qse_mchar_t* str, const qse_mchar_t* sub, qse_size_t len)
{
	return qse_mbsxnend (str, qse_mbslen(str), sub, len);
}

qse_mchar_t* qse_mbsxnend (
	const qse_mchar_t* str, qse_size_t len1, 
	const qse_mchar_t* sub, qse_size_t len2)
{
	const qse_mchar_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (end2 > sub)
	{
		if (end1 <= str) return QSE_NULL;
		if (*(--end1) != *(--end2)) return QSE_NULL;
	}
	
	/* returns the pointer to the match start */
	return (qse_mchar_t*)end1;
}

qse_wchar_t* qse_wcsend (const qse_wchar_t* str, const qse_wchar_t* sub)
{
	return qse_wcsxnend (str, qse_wcslen(str), sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsxend (
	const qse_wchar_t* str, qse_size_t len, const qse_wchar_t* sub)
{
	return qse_wcsxnend (str, len, sub, qse_wcslen(sub));
}

qse_wchar_t* qse_wcsnend (
	const qse_wchar_t* str, const qse_wchar_t* sub, qse_size_t len)
{
	return qse_wcsxnend (str, qse_wcslen(str), sub, len);
}

qse_wchar_t* qse_wcsxnend (
	const qse_wchar_t* str, qse_size_t len1, 
	const qse_wchar_t* sub, qse_size_t len2)
{
	const qse_wchar_t* end1, * end2;

	if (len2 > len1) return QSE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (end2 > sub)
	{
		if (end1 <= str) return QSE_NULL;
		if (*(--end1) != *(--end2)) return QSE_NULL;
	}
	
	/* returns the pointer to the match start */
	return (qse_wchar_t*)end1;
}
