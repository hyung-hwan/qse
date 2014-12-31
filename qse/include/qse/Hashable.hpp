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

#ifndef _QSE_HASHABLE_HPP_
#define _QSE_HASHABLE_HPP_

#include <qse/Types.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class QSE_EXPORT Hashable
{
public:
	virtual ~Hashable () {}

	virtual qse_size_t getHashCode () const = 0;

	static qse_size_t getHashCode (qse_size_t init, const qse_char_t* str)
	{
		qse_size_t n = init;

		while (*str != QSE_T('\0'))
		{
			const qse_uint8_t* p = (const qse_uint8_t*)str;
			for (qse_size_t i = 0; i < QSE_SIZEOF(*str); i++)
				n = n * 31 + *p++;
			str++;
		}

		return n;
	}

	static qse_size_t getHashCode (const qse_char_t* str)
	{
		return getHashCode (0, str);
	}

	static qse_size_t getHashCode (qse_size_t init, const void* data, qse_size_t size)
	{
		qse_size_t n = init;
		const qse_uint8_t* p = (const qse_uint8_t*)data;

		/*
		for (qse_size_t i = 0; i < size; i++) {
			n <<= 1;
			n += *p++;
		}
		*/

		for (qse_size_t i = 0; i < size; i++) n = n * 31 + *p++;

		/*
		for (qse_size_t i = 0; i < size; i++) {
			n = (n << 4) + *p++;
			//qse_size_t g = n & 0xF0000000U;
			qse_size_t g = n & ((qse_size_t)0xF << (qse_sizeof(qse_size_t) * 8 - 4));
			n &= ~g;
		}
		*/

		return n;
	}

	static qse_size_t getHashCode (const void* data, qse_size_t size)
	{
		return getHashCode (0, data, size);
	}
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
