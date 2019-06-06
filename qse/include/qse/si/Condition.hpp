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

#ifndef _QSE_SI_CONDITION_CLASS_
#define _QSE_SI_CONDITION_CLASS_

#include <qse/si/cnd.h>
#include <qse/si/Mutex.hpp>

QSE_BEGIN_NAMESPACE(QSE)

class Condition: public Uncopyable
{
public:
	Condition() QSE_CPP_NOEXCEPT 
	{
		qse_cnd_init (&this->cnd, QSE_NULL);
	}
	~Condition() QSE_CPP_NOEXCEPT
	{
		qse_cnd_fini (&this->cnd);
	}

	void signal ()
	{
		qse_cnd_signal (&this->cnd);
	}

	void broadcast ()
	{
		qse_cnd_broadcast (&this->cnd);
	}

	void wait (Mutex& mtx, const qse_ntime_t* timeout = QSE_NULL)
	{
		qse_cnd_wait (&this->cnd, &mtx.mtx, timeout);
	}

protected:
	qse_cnd_t cnd;
};

QSE_END_NAMESPACE(QSE)

#endif
