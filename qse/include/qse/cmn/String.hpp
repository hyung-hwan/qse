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

#ifndef _QSE_CMN_STRING_HPP_
#define _QSE_CMN_STRING_HPP_

#include <qse/cmn/StrBase.hpp>
#include <qse/cmn/str.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

struct WcStringOpset
{
	qse_size_t copy (qse_wchar_t* dst, const qse_wchar_t* src, qse_size_t ssz)
	{
		return qse_wcsncpy(dst, src, ssz);
	}

	// compare two strings of the same length
	int compare (const qse_wchar_t* str1, const qse_wchar_t* str2, qse_size_t len)
	{
		return qse_wcsxncmp(str1, len, str2, len);
	}

	// compare a length-bound string with a null-terminated string.
	int compare (const qse_wchar_t* str1, qse_size_t len, const qse_wchar_t* str2)
	{
		return qse_wcsxcmp(str1, len, str2);
	}

	int length (const qse_wchar_t* str)
	{
		return qse_strlen(str);
	}
};

//struct MbStringOpset
//{
//};

typedef StrBase<qse_wchar_t, QSE_WT('\0'), WcStringOpset > WcString;
//typedef StrBase<qse_mchar_t, QSE_MT('\0'), MbStringOpset > MbString;

#if defined(QSE_CHAR_IS_MCHAR)
	//typedef MbString String;
#else
	typedef WcString String;
#endif


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
