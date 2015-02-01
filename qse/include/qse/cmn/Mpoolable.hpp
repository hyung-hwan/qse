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

#ifndef _QSE_CMN_MPOOLABLE_HPP_
#define _QSE_CMN_MPOOLABLE_HPP_

#include <qse/cmn/Mpool.hpp>

// use size_t as some compilers complain about qse_size_t used in new().
#include <stddef.h> 

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class QSE_EXPORT Mpoolable
{
public:

/*
	inline void* operator new (mp_size_t size)
	{
		return ::operator new (size);
	}

	inline void operator delete (void* ptr)
	{
		::operator delete (ptr);
	}
*/

	inline void* operator new (size_t size, Mpool* mp)
	{
		return mp->isEnabled()? mp->allocate (): ::operator new (size);
	}

#if defined(_MSC_VER)
	void operator delete (void* ptr, Mpool* mp)
	{
		if (mp->isEnabled()) mp->dispose (ptr);
		else ::operator delete (mp);
	}
#else
	inline void dispose (void* ptr, Mpool* mp)
	{
		if (mp->isEnabled()) mp->dispose (ptr);
		else ::operator delete (mp);
	}
#endif
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif