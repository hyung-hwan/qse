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

#ifndef _QSE_CSTR_HPP_
#define _QSE_CSTR_HPP_

#include <qse/Types.hpp>
#include <qse/Hashable.hpp>
#include <qse/cmn/str.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/// The Mcstr class encapsulates a multibyte string pointer and length.
class Mcstr: public Types::mcstr_t, public Hashable
{
public:
	Mcstr (qse_mchar_t* ptr, qse_size_t len)
	{
		this->ptr = ptr;
		this->len = len;
	}

	Mcstr (const qse_mchar_t* ptr, qse_size_t len)
	{
		this->ptr = (qse_mchar_t*)ptr;
		this->len = len;
	}

	Mcstr (Types::mcstr_t& str)
	{
		this->ptr = (qse_mchar_t*)str.ptr;
		this->len = str.len;
	}

	qse_size_t getHashCode () const 
	{
		return Hashable::getHashCode (this->ptr, this->len * QSE_SIZEOF(*this->ptr));
	}

	bool operator== (const Mcstr& str) const
	{
		return qse_mbsxncmp (this->ptr, this->len, str.ptr, str.len) == 0;
	}

	bool operator!= (const Mcstr& str) const
	{
		return qse_mbsxncmp (this->ptr, this->len, str.ptr, str.len) != 0;
	}
};

/// The Mcstr class encapsulates a wide-character string pointer and length.
class Wcstr: public Types::wcstr_t, public Hashable
{
public:
	Wcstr (qse_wchar_t* ptr, qse_size_t len)
	{
		this->ptr = ptr;
		this->len = len;
	}

	Wcstr (const qse_wchar_t* ptr, qse_size_t len)
	{
		this->ptr = (qse_wchar_t*)ptr;
		this->len = len;
	}

	Wcstr (Types::wcstr_t& str)
	{
		this->ptr = (qse_wchar_t*)str.ptr;
		this->len = str.len;
	}

	qse_size_t getHashCode () const 
	{
		return Hashable::getHashCode (this->ptr, this->len * QSE_SIZEOF(*this->ptr));
	}

	bool operator== (const Wcstr& str) const
	{
		return qse_wcsxncmp (this->ptr, this->len, str.ptr, str.len) == 0;
	}

	bool operator!= (const Wcstr& str) const
	{
		return qse_wcsxncmp (this->ptr, this->len, str.ptr, str.len) != 0;
	}
};

/// The Mcstr class encapsulates a character string pointer and length.
class Cstr: public Types::cstr_t, public Hashable
{
public:
	Cstr (qse_char_t* ptr, qse_size_t len)
	{
		this->ptr = ptr;
		this->len = len;
	}

	Cstr (const qse_char_t* ptr, qse_size_t len)
	{
		this->ptr = (qse_char_t*)ptr;
		this->len = len;
	}

	Cstr (Types::cstr_t& str)
	{
		this->ptr = (qse_char_t*)str.ptr;
		this->len = str.len;
	}

	qse_size_t getHashCode () const 
	{
		return Hashable::getHashCode (this->ptr, this->len * QSE_SIZEOF(*this->ptr));
	}

	bool operator== (const Cstr& str) const
	{
		return qse_strxncmp (this->ptr, this->len, str.ptr, str.len) == 0;
	}

	bool operator!= (const Cstr& str) const
	{
		return qse_strxncmp (this->ptr, this->len, str.ptr, str.len) != 0;
	}
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
