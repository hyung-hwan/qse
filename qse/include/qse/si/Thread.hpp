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

#include <functional>

QSE_BEGIN_NAMESPACE(QSE)

class Thread: protected qse_thr_t, public Uncopyable
{
public:
	// native thread hadnle type
	typedef qse_thr_hnd_t Handle;

	typedef int (*ThreadRoutine) (Thread* thr);

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


#if (__cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1900) //C++11 or later
	using ThreadLambda = std::function<int(QSE::Thread*)>;

	static int call_lambda (qse_thr_t* thr, void* ctx)
	{
		Thread* t = (Thread*)ctx;
		return t->__tmplam (t);
	}

	int startl (ThreadLambda&& f, int flags)
	{
		this->__tmplam = QSE_CPP_RVREF(f);
		return qse_thr_start (this, (qse_thr_rtn_t)Thread::call_lambda, this, flags);
	}

#if 0
	static int call_lambda_lx (qse_thr_t* thr, void* ctx)
	{
		Thread* t = (Thread*)ctx;
		//return ([]int(QSE::Thread*))t->__tmpvoid (t);
	}

	template <typename F>
	int startlx (F&& f, int flags)
	{
		this->__tmplam = QSE_CPP_RVREF(f);
		auto xx = QSE_CPP_RVREF(f);
		this->__tmpvoid = (void*)&xx;
		return qse_thr_start (this, (qse_thr_rtn_t)Thread::call_lambda_lx, this, flags);
	}
#endif

#endif


	// execute the given function in a thread.
	virtual int start (ThreadRoutine rtn, int flags = 0) QSE_CPP_NOEXCEPT;

	// execute the main method defined in this class in a thread.
	virtual int start (int flags = 0) QSE_CPP_NOEXCEPT;

	virtual int stop () QSE_CPP_NOEXCEPT;

	virtual int main () { return 0; }

	// return the context pointer value
	const void* getContext () const { return this->__exctx; }
	void* getContext () { return this->__exctx; }

	// change the context pointer value
	void setContext (void* ctx) { this->__exctx = ctx; }

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
	void* __exctx;
	ThreadRoutine __tmprtn;
#if (__cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1900) //C++11 or later
	ThreadLambda __tmplam;
//	void* __tmpvoid;
#endif
	static Handle INVALID_HANDLE;


	static int thr_func_call_rtn (qse_thr_t* rtn, void* ctx);
};


QSE_END_NAMESPACE(QSE)


#endif
