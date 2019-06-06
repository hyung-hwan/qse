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

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EQSERESS OR
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

#ifndef _QSE_CMN_NAMED_CLASS_
#define _QSE_CMN_NAMED_CLASS_

#include <qse/Types.hpp>
#include <qse/cmn/str.h>

QSE_BEGIN_NAMESPACE(QSE)

template <int N>
class Named 
{
public:
	Named (const qse_char_t* name = QSE_T("")) QSE_CPP_NOEXCEPT
	{
		QSE_ASSERT (name != QSE_NULL);
		// WARNING: a long name can be truncated
		qse_strxcpy (this->name_buf, QSE_COUNTOF(this->name_buf), name);
	}
	~Named () QSE_CPP_NOEXCEPT {}

	qse_char_t* getName () QSE_CPP_NOEXCEPT
	{
		return this->name_buf;
	}

	const qse_char_t* getName () const QSE_CPP_NOEXCEPT
	{
		return this->name_buf;
	}

	void setName (const qse_char_t* name) QSE_CPP_NOEXCEPT
	{
		QSE_ASSERT (name != QSE_NULL);
		// WARNING: a long name can be truncated
		qse_strxcpy (this->name_buf, QSE_COUNTOF(this->name_buf), name);
	}

	bool isNamed () const QSE_CPP_NOEXCEPT
	{
		return this->name_buf[0] != QSE_T('\0');
	}

	bool isNamed (const qse_char_t* n) const QSE_CPP_NOEXCEPT
	{
		return qse_strcmp(this->name_buf, n) == 0;
	}

	enum 
	{
		MAX_NAME_LEN = N
	};

protected:
	qse_char_t name_buf[MAX_NAME_LEN + 1]; 
};

QSE_END_NAMESPACE(QSE)

#endif
