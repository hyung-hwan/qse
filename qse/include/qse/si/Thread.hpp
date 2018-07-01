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

QSE_BEGIN_NAMESPACE(QSE)

class Thread: public Uncopyable, public Mmged
{
public:
	// native thread handle type
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

	static Handle getCurrentHandle () QSE_CPP_NOEXCEPT { return qse_get_thr_hnd(); }

	Thread (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
	virtual ~Thread () QSE_CPP_NOEXCEPT;

	Handle getHandle () const QSE_CPP_NOEXCEPT { return this->thr.__handle; }
	int getState () const QSE_CPP_NOEXCEPT { return this->thr.__state; }
	int getReturnCode () const QSE_CPP_NOEXCEPT { return this->thr.__return_code; }

	qse_size_t getStackSize () const QSE_CPP_NOEXCEPT { return this->thr.__stacksize; }
	void setStackSize (qse_size_t num) QSE_CPP_NOEXCEPT { qse_thr_setstacksize(&this->thr, num); }

	// execute the main method defined in this class in a thread.
	virtual int start (int flags = 0) QSE_CPP_NOEXCEPT;

	virtual int stop () QSE_CPP_NOEXCEPT;

	virtual int main () { return 0; } // to be overridden by a child class.

	// return the context pointer value
	const void* getContext () const QSE_CPP_NOEXCEPT { return this->__exctx; }
	void* getContext () QSE_CPP_NOEXCEPT { return this->__exctx; }

	// change the context pointer value
	void setContext (void* ctx) QSE_CPP_NOEXCEPT { this->__exctx = ctx; }

	int join () QSE_CPP_NOEXCEPT { return qse_thr_join(&this->thr); }
	int detach () QSE_CPP_NOEXCEPT { return qse_thr_detach(&this->thr); }

	int kill (int sig) QSE_CPP_NOEXCEPT { return qse_thr_kill(&this->thr, sig); }
	int blockSignal (int sig) QSE_CPP_NOEXCEPT { return qse_thr_blocksig(&this->thr, sig); }
	int unblockSignal (int sig) QSE_CPP_NOEXCEPT { return qse_thr_unblocksig(&this->thr, sig); }
	int blockAllSignals () QSE_CPP_NOEXCEPT { return qse_thr_blockallsigs(&this->thr); }
	int unblockAllSignals () QSE_CPP_NOEXCEPT { return qse_thr_unblockallsigs(&this->thr); }

protected:
	qse_thr_t thr;
	void* __exctx;
	static Handle INVALID_HANDLE;
};

class ThreadR: public Thread
{
public:
	ThreadR (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: Thread(mmgr) {}
	typedef int (*ThreadRoutine) (Thread* thr);

	// execute the given function in a thread.
	virtual int start (ThreadRoutine rtn, int flags = 0) QSE_CPP_NOEXCEPT;

protected:
	ThreadRoutine __tmprtn;
	static int thr_func_call_rtn (qse_thr_t* rtn, void* ctx);
};

template <typename F>
class ThreadF: public Thread
{
public:
	ThreadF (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: Thread(mmgr) {}
	ThreadF (const F& f, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: Thread(mmgr), __lfunc(f) {}
#if defined(QSE_CPP_ENABLE_CPP11_MOVE)
	ThreadF (F&& f, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: Thread(mmgr), __lfunc(QSE_CPP_RVREF(f)) {}
#endif

	static int call_func (qse_thr_t* thr, void* ctx)
	{
		ThreadF<F>* t = (ThreadF<F>*)ctx;
		return t->__lfunc (t);
	}

	int start (int flags = 0) QSE_CPP_NOEXCEPT
	{
		return qse_thr_start (&this->thr, (qse_thr_rtn_t)ThreadF<F>::call_func, this, flags);
	}

protected:
	F __lfunc;
};

#if defined(QSE_LANG_CPP11)

#if 0
// i don't want to use std::function. 
class ThreadF: public Thread
{
public:
	static int call_func (qse_thr_t* thr, void* ctx)
	{
		ThreadF* t = (ThreadF*)ctx;
		return t->__lfunc (t);
	}

	template <typename X>
	int start (X&& f, int flags) QSE_CPP_NOEXCEPT
	{
		this->__lfunc = QSE_CPP_RVREF(f);
		return qse_thr_start(&this->thr, (qse_thr_rtn_t)ThreadF::call_func, this, flags);
	}

protected:
	std::function<int(ThreadF*)> __lfunc; 
};

#else

template <typename T>
class ThreadL;

template <typename RT, typename... ARGS>
class ThreadL<RT(ARGS...)>: public Thread
{
public:
	ThreadL (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: Thread(mmgr), __lfunc(nullptr) {}
	~ThreadL () QSE_CPP_NOEXCEPT 
	{ 
		if (this->__lfunc) this->getMmgr()->dispose(this->__lfunc); //delete this->__lfunc; 
	}

	static int call_func (qse_thr_t* thr, void* ctx)
	{
		ThreadL* t = (ThreadL*)ctx;
		return t->__lfunc->invoke(t);
	}

	template <typename T>
	int start (T&& f, int flags) QSE_CPP_NOEXCEPT
	//int start (T f, int flags) QSE_CPP_NOEXCEPT
	{
		if (this->__state == QSE_THR_RUNNING) return -1;
		if (this->__lfunc) this->getMmgr()->dispose (this->__lfunc); //delete this->__lfunc;
		try
		{
			// TODO: are there any ways to achieve this without memory allocation?
			this->__lfunc = new(this->getMmgr()) TCallable<T> (QSE_CPP_RVREF(f));
		}
		catch (...)
		{
			this->__lfunc = nullptr;
			return -1;
		}
		return qse_thr_start(&this->thr, (qse_thr_rtn_t)ThreadL::call_func, this, flags);
	}

protected:
	class Callable
	{
	public:
		virtual ~Callable () QSE_CPP_NOEXCEPT {};
		virtual RT invoke (ARGS... args) = 0;
	};

	template <typename T>
	class TCallable: public Callable
	{
	public:
		TCallable (const T& t) QSE_CPP_NOEXCEPT: t(t) { }
		~TCallable () QSE_CPP_NOEXCEPT {}
		RT invoke (ARGS... args) { return this->t(args ...); }

	private:
		T t;
	};

	Callable* __lfunc;
};
#endif

#endif // QSE_LANG_CPP11


QSE_END_NAMESPACE(QSE)


#endif
