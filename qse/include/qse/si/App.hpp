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

#ifndef _QSE_SI_APP_H_
#define _QSE_SI_APP_H_

#include <qse/Types.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/cmn/Mmged.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class App: public Uncopyable, public Types, public Mmged
{
public:
	App (Mmgr* mmgr) QSE_CPP_NOEXCEPT;
	virtual ~App () QSE_CPP_NOEXCEPT;

	int daemonize (bool chdir_to_root = true, int fork_count = 1) QSE_CPP_NOEXCEPT;

	int chroot (const qse_mchar_t* mpath) QSE_CPP_NOEXCEPT;
	int chroot (const qse_wchar_t* wpath) QSE_CPP_NOEXCEPT;

#if 0
	int switchUser (uid_t uid, gid_t gid, bool permanently) QSE_CPP_NOEXCEPT;
	int restoreUser () QSE_CPP_NOEXCEPT;
#endif

	virtual void on_signal (int sig) { }

protected:
	bool _root_only;

private:
	App* _prev_app;
	App* _next_app;

public:
	struct _SigLink
	{
		enum 
		{
			UNHANDLED,
			ACCEPTED,
			IGNORED // handled but ignored
		};

		_SigLink(): _prev(QSE_NULL), _next(QSE_NULL), _state(UNHANDLED) {}
		App* _prev;
		App* _next;
		int  _state;
	};

	_SigLink _sig[QSE_NSIGS]; 

public:

	typedef void (*SignalHandler) (int sig);
	static int setSignalHandler (int sig, SignalHandler sighr);
	static int unsetSignalHandler (int sig);
	static qse_size_t _sighrs[2][QSE_NSIGS];

	void handleSignal (int sig, bool accept);
	void unhandleSignal (int sig);
};

// functor as a template parameter
template <typename F>
class QSE_EXPORT AppF: public App
{
public:
	AppF (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: App(mmgr) {}
	AppF (const F& f, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: App(mmgr), __lfunc(f) {}
#if defined(QSE_CPP_ENABLE_CPP11_MOVE)
	AppF (F&& f, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: App(mmgr), __lfunc(QSE_CPP_RVREF(f)) {}
#endif

protected:
	F __lfunc;

	void on_signal (int sig)
	{
		this->__lfunc(this, sig);
	}
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
