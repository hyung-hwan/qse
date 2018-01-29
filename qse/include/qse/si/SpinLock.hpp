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

#ifndef _QSE_SI_SPINLOCK_CLASS_
#define _QSE_SI_SPINLOCK_CLASS_

#include <qse/Types.hpp>
#include <qse/Uncopyable.hpp>

QSE_BEGIN_NAMESPACE(QSE)


class SpinLock
{
public:
	bool tryock()
	{
		return !__sync_lock_test_and_set(&this->flag, 1);
	}

	void lock ()
	{
		//while (!this->tryLock());
		while (__sync_lock_test_and_set(&this->flag, 1));
	}

	void unlock ()
	{
		__sync_lock_release (&this->flag);
	}

protected:
	volatile int flag;
};

class ScopedSpinLock: public Uncopyable
{
	ScopedSpinLock (SpinLock& spl): spl(spl)
	{
		this->spl.lock ();
	}

	~ScopedSpinLock ()
	{
		this->spl.unlock ();
	}

protected:
	SpinLock& spl;
};

QSE_END_NAMESPACE(QSE)

#endif
