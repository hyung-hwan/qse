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

#ifndef _QSE_SI_MUTEX_CLASS_
#define _QSE_SI_MUTEX_CLASS_

#include <qse/Types.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/si/mtx.h>

QSE_BEGIN_NAMESPACE(QSE)

class Mutex: public Uncopyable
{
public:
	Mutex() QSE_CPP_NOEXCEPT 
	{
		qse_mtx_init (&this->mtx, QSE_NULL);
	}
	~Mutex() QSE_CPP_NOEXCEPT
	{
		qse_mtx_fini (&this->mtx);
	}

#if 0
	bool tryock() QSE_CPP_NOEXCEPT
	{
	}
#endif

	void lock () QSE_CPP_NOEXCEPT
	{
		qse_mtx_lock (&this->mtx, QSE_NULL);
	}

	void unlock () QSE_CPP_NOEXCEPT
	{
		qse_mtx_unlock (&this->mtx);
	}

protected:
	qse_mtx_t mtx;
};

class ScopedMutexLocker: public Uncopyable
{
public:
	ScopedMutexLocker (Mutex& mtx) QSE_CPP_NOEXCEPT: mtx(mtx)
	{
		this->mtx.lock ();
	}

	~ScopedMutexLocker () QSE_CPP_NOEXCEPT
	{
		this->mtx.unlock ();
	}

protected:
	Mutex& mtx;
};
QSE_END_NAMESPACE(QSE)

#endif
