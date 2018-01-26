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

#include <qse/si/Thread.hpp>
#include "thr-prv.h"

#include <stdio.h>
QSE_BEGIN_NAMESPACE(QSE)


Thread::Handle Thread::INVALID_HANDLE = QSE_THR_HND_INVALID;

Thread::Thread() QSE_CPP_NOEXCEPT //: thread_target (QSE_NULL)
{
	//qse_thr_init (this, this->getMmgr());
	qse_thr_init (this, QSE_NULL);
}

Thread::~Thread () QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->__state != Thread::RUNNING);
	// it is subclasses' responsibility to stop the thread gracefully.
	// so stop is not called here.
	// this->stop ();

	/*if (this->__joinable)*/ this->join ();
	qse_thr_fini (this);
}

int thr_func (qse_thr_t* thr)
{
	Thread* t = (Thread*)thr;
	return t->main ();
}

int Thread::start (int flags) QSE_CPP_NOEXCEPT
{
	return qse_thr_start(this, thr_func, flags);
}

int Thread::stop () QSE_CPP_NOEXCEPT
{
	// NOTICE:
	// make sure that subclasses override "stop" and call it
	// properly so that the thread can be terminated gracefully.
	// "stop" here just aborts the running thread.
	return qse_thr_stop(this);
}

QSE_END_NAMESPACE(QSE)
