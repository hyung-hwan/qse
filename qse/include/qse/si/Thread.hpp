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

#ifndef _QSE_SI_THREAD_CLASS_
#define _QSE_SI_THREAD_CLASS_

#include <qse/si/thr.h>
#include <qse/Uncopyable.hpp>
#include <qse/cmn/Mmged.hpp>

//#include <functional>

QSE_BEGIN_NAMESPACE(QSE)


class Thread: protected qse_thr_t, public Uncopyable
{
public:
	// native thread hadnle type
	typedef qse_thr_hnd_t Handle;

	enum State
	{
		INCUBATING = QSE_THR_INCUBATING,
		RUNNING    = QSE_THR_RUNNING,
		TERMINATED = QSE_THR_TERMINATED,
		ABORTED    = QSE_THR_ABORTED
	};

	enum
	{
		DETACHED        = QSE_THR_DETACHED,
		SIGNALS_BLOCKED = QSE_THR_SIGNALS_BLOCKED
	};

	static Handle getCurrentHandle () QSE_CPP_NOEXCEPT { return qse_get_thr_hnd ();	}

	Thread () QSE_CPP_NOEXCEPT;
	virtual ~Thread () QSE_CPP_NOEXCEPT;

	Handle getHandle () const QSE_CPP_NOEXCEPT { return this->__handle; }
	int getState () const QSE_CPP_NOEXCEPT { return this->__state; }
	int getReturnCode () const QSE_CPP_NOEXCEPT { return this->__return_code; }

	qse_size_t getStackSize () const QSE_CPP_NOEXCEPT { return this->__stacksize; }
	void setStackSize (qse_size_t num) QSE_CPP_NOEXCEPT { qse_thr_setstacksize(this, num); }


#if 0
	static int call_lambda (QSE::Thread* thr)
	{
		return thr->x_func (thr);
	}

	template <typename F>
	int start (F&& f, int flags) 
	{ 
		this->x_func = std::bind(f);
		return qse_thr_start (this, (qse_thr_rtn_t)Thread::call_lambda, flags);
	}
	std::function<int(QSE::Thread*)> x_func;
#endif

	virtual int start (int flags = 0) QSE_CPP_NOEXCEPT;
	virtual int stop () QSE_CPP_NOEXCEPT;

	virtual int main () { return 0; }

	int join () QSE_CPP_NOEXCEPT { return qse_thr_join(this); }
	int detach () QSE_CPP_NOEXCEPT { return qse_thr_detach(this); }

/*
	void sleep (qse_time_t msecs)
	{
		qse_sleep (msecs);
	}
*/
	int kill (int sig) QSE_CPP_NOEXCEPT { return qse_thr_kill(this, sig); }

	int blockSignal (int sig) QSE_CPP_NOEXCEPT { return qse_thr_blocksig(this, sig); }
	int unblockSignal (int sig) QSE_CPP_NOEXCEPT { return qse_thr_unblocksig(this, sig); }

	int blockAllSignals () QSE_CPP_NOEXCEPT { return qse_thr_blockallsigs (this); }
	int unblockAllSignals () QSE_CPP_NOEXCEPT { return qse_thr_unblockallsigs (this); }

protected:
	static Handle INVALID_HANDLE;
};


QSE_END_NAMESPACE(QSE)


#endif
