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

#include <qse/si/Thread.hpp>
#include "thr-prv.h"

QSE_BEGIN_NAMESPACE(QSE)


Thread::Handle Thread::INVALID_HANDLE = QSE_THR_HND_INVALID;

Thread::Thread(Mmgr* mmgr) QSE_CPP_NOEXCEPT: Mmged(mmgr), __exctx(QSE_NULL)
{
	qse_thr_init (&this->thr, this->getMmgr());
}

Thread::~Thread () QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->thr.__state != QSE_THR_RUNNING);
	// it is subclasses' responsibility to stop the thread gracefully.
	// so stop is not called here.
	// this->stop ();

	/*if (this->thr.__joinable)*/ this->join ();
	qse_thr_fini (&this->thr);
}


static int thr_func_call_main (qse_thr_t* thr, void* ctx)
{
	Thread* t = (Thread*)ctx;
	return t->main ();
}

int Thread::start (int flags) QSE_CPP_NOEXCEPT
{
	return qse_thr_start(&this->thr, thr_func_call_main, this, flags);
}

int Thread::stop () QSE_CPP_NOEXCEPT
{
	// NOTICE:
	// make sure that subclasses override "stop" and call it
	// properly so that the thread can be terminated gracefully.
	// "stop" here just aborts the running thread.
	return qse_thr_stop(&this->thr);
}


int ThreadR::thr_func_call_rtn (qse_thr_t* thr, void* ctx)
{
	// 'thr' may not be point to the actual Thread 
	// for the reason stated in Thread::start(). 
	// utilize the ctx pointer passed in Thread::start().
	ThreadR* t = (ThreadR*)ctx;
	return t->__tmprtn(t);
}

int ThreadR::start (ThreadRoutine rtn, int flags) QSE_CPP_NOEXCEPT
{
	if (this->thr.__state == QSE_THR_RUNNING) return -1;

	// this != (qse_thr_t*)this may not be equal if this class
	// has some internal added data fields. e.g. it contains
	// a virtual function. direct invocation without the extra ctx pointer
	// like this has some implications when attempting to convert
	// qse_thr_t* to Thread*.
	// 	qse_thr_start (this, (qse_thr_rtn_t)rtn, QSE_NULL, flags);
	// so i pass a void pointer 'this' as the third argument.
	this->__tmprtn = rtn;
	return qse_thr_start(&this->thr, thr_func_call_rtn, this, flags);
}

QSE_END_NAMESPACE(QSE)
