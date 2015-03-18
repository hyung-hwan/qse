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

/// \file
/// Provides the String, WcString, McString classes.
///
/// \includelineno str02.cpp
///

#include <qse/cmn/StrBase.hpp>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <stdarg.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

struct WcStringOpset
{
	qse_size_t copy (qse_wchar_t* dst, const qse_wchar_t* src, qse_size_t ssz) const
	{
		return qse_wcsncpy(dst, src, ssz);
	}

	qse_size_t move (qse_wchar_t* dst, const qse_wchar_t* src, qse_size_t ssz) const
	{
		// this one doesn't insert terminating null
		qse_memmove (dst, src, ssz * QSE_SIZEOF(*dst));
		return ssz;
	}

	// compare two strings of the same length
	int compare (const qse_wchar_t* str1, const qse_wchar_t* str2, qse_size_t len) const
	{
		return qse_wcsxncmp(str1, len, str2, len);
	}

	// compare a length-bound string with a null-terminated string.
	int compare (const qse_wchar_t* str1, qse_size_t len, const qse_wchar_t* str2) const
	{
		return qse_wcsxcmp(str1, len, str2);
	}

	qse_size_t getLength (const qse_wchar_t* str) const
	{
		return qse_wcslen(str);
	}

	bool beginsWith (const qse_wchar_t* str, qse_size_t len, const qse_wchar_t* sub) const
	{
		return qse_wcsxbeg (str, len, sub) != QSE_NULL;
	}

	bool beginsWith (const qse_wchar_t* str, qse_size_t len, const qse_wchar_t* sub, qse_size_t sublen) const
	{
		return qse_wcsxnbeg (str, len, sub, sublen) != QSE_NULL;
	}

	bool endsWith (const qse_wchar_t* str, qse_size_t len, const qse_wchar_t* sub) const
	{
		return qse_wcsxend (str, len, sub) != QSE_NULL;
	}

	bool endsWith (const qse_wchar_t* str, qse_size_t len, const qse_wchar_t* sub, qse_size_t sublen) const
	{
		return qse_wcsxnend (str, len, sub, sublen) != QSE_NULL;
	}

	qse_size_t trim (qse_wchar_t* str, qse_size_t len, bool left, bool right) const
	{
		qse_wchar_t* ptr;
		qse_size_t xlen = len;
		int flags = 0;

		if (left) flags |= QSE_WCSTRMX_LEFT;
		if (right) flags |= QSE_WCSTRMX_RIGHT;
		ptr = qse_wcsxtrmx (str, &xlen, flags);
		this->move (str, ptr, xlen);
		str[xlen] = QSE_WT('\0');

		return xlen;
	}
};

struct MbStringOpset
{
	qse_size_t copy (qse_mchar_t* dst, const qse_mchar_t* src, qse_size_t ssz) const
	{
		return qse_mbsncpy(dst, src, ssz);
	}

	qse_size_t move (qse_mchar_t* dst, const qse_mchar_t* src, qse_size_t ssz) const
	{
		// this one doesn't insert terminating null
		qse_memmove (dst, src, ssz * QSE_SIZEOF(*dst));
		return ssz;
	}

	// compare two strings of the same length
	int compare (const qse_mchar_t* str1, const qse_mchar_t* str2, qse_size_t len) const
	{
		return qse_mbsxncmp(str1, len, str2, len);
	}

	// compare a length-bound string with a null-terminated string.
	int compare (const qse_mchar_t* str1, qse_size_t len, const qse_mchar_t* str2) const
	{
		return qse_mbsxcmp(str1, len, str2);
	}

	qse_size_t getLength (const qse_mchar_t* str) const
	{
		return qse_mbslen(str);
	}

	bool beginsWith (const qse_mchar_t* str, qse_size_t len, const qse_mchar_t* sub) const
	{
		return qse_mbsxbeg (str, len, sub) != QSE_NULL;
	}

	bool beginsWith (const qse_mchar_t* str, qse_size_t len, const qse_mchar_t* sub, qse_size_t sublen) const
	{
		return qse_mbsxnbeg (str, len, sub, sublen) != QSE_NULL;
	}

	bool endsWith (const qse_mchar_t* str, qse_size_t len, const qse_mchar_t* sub) const
	{
		return qse_mbsxend (str, len, sub) != QSE_NULL;
	}

	bool endsWith (const qse_mchar_t* str, qse_size_t len, const qse_mchar_t* sub, qse_size_t sublen) const
	{
		return qse_mbsxnend (str, len, sub, sublen) != QSE_NULL;
	}

	qse_size_t trim (qse_mchar_t* str, qse_size_t len, bool left, bool right) const
	{
		qse_mchar_t* ptr;
		qse_size_t xlen = len;
		int flags = 0;

		if (left) flags |= QSE_MBSTRMX_LEFT;
		if (right) flags |= QSE_MBSTRMX_RIGHT;
		ptr = qse_mbsxtrmx (str, &xlen, flags);
		this->move (str, ptr, xlen);
		str[xlen] = QSE_MT('\0');

		return xlen;
	}
};

// It's a pain to inherit StrBase<>. i do this not to have various va_xxx calls
// in the header file  of StrBase.

class WcString: public StrBase<qse_wchar_t, QSE_WT('\0'), WcStringOpset> 
{
public:
	typedef StrBase<qse_wchar_t, QSE_WT('\0'), WcStringOpset> ParentType;

	WcString (): ParentType() {}
	WcString (Mmgr* mmgr): ParentType(mmgr) {}
	WcString (qse_size_t capacity): ParentType(capacity) {}
	WcString (Mmgr* mmgr, qse_size_t capacity): ParentType(mmgr, capacity) {}
	WcString (const qse_wchar_t* str): ParentType(str) {}
	WcString (Mmgr* mmgr, const qse_wchar_t* str): ParentType(mmgr, str) {}
	WcString (const qse_wchar_t* str, qse_size_t size): ParentType(str, size) {}
	WcString (Mmgr* mmgr, const qse_wchar_t* str, qse_size_t size): ParentType(mmgr, str, size) {}
	WcString (qse_wchar_t c, qse_size_t size): ParentType(c, size) {}
	WcString (Mmgr* mmgr, qse_wchar_t c, qse_size_t size): ParentType(mmgr, c, size) {}

	WcString (const WcString& str): ParentType(str) {}

	WcString& operator= (const WcString& str) { ParentType::operator=(str); return *this; }
	WcString& operator= (const qse_wchar_t* str) { ParentType::operator=(str); return *this; }
	WcString& operator= (const qse_wchar_t c) { ParentType::operator=(c); return *this; }

	int format (const qse_wchar_t* fmt, ...);
	int formatv (const qse_wchar_t* fmt, va_list ap);
};

class MbString: public StrBase<qse_mchar_t, QSE_MT('\0'), MbStringOpset>
{
public:
	typedef StrBase<qse_mchar_t, QSE_MT('\0'), MbStringOpset> ParentType;

	MbString (): ParentType() {}
	MbString (Mmgr* mmgr): ParentType(mmgr) {}
	MbString (qse_size_t capacity): ParentType(capacity) {}
	MbString (Mmgr* mmgr, qse_size_t capacity): ParentType(mmgr, capacity) {}
	MbString (const qse_mchar_t* str): ParentType(str) {}
	MbString (Mmgr* mmgr, const qse_mchar_t* str): ParentType(mmgr, str) {}
	MbString (const qse_mchar_t* str, qse_size_t size): ParentType(str, size) {}
	MbString (Mmgr* mmgr, const qse_mchar_t* str, qse_size_t size): ParentType(mmgr, str, size) {}
	MbString (qse_mchar_t c, qse_size_t size): ParentType(c, size) {}
	MbString (Mmgr* mmgr, qse_mchar_t c, qse_size_t size): ParentType(mmgr, c, size) {}

	MbString (const MbString& str): ParentType(str) {}

	MbString& operator= (const MbString& str) { ParentType::operator=(str); return *this; }
	MbString& operator= (const qse_mchar_t* str) { ParentType::operator=(str); return *this; }
	MbString& operator= (const qse_mchar_t c) { ParentType::operator=(c); return *this; }

	int format (const qse_mchar_t* fmt, ...);
	int formatv (const qse_mchar_t* fmt, va_list ap);
};

#if defined(QSE_CHAR_IS_MCHAR)
	typedef MbString String;
#else
	typedef WcString String;
#endif

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
