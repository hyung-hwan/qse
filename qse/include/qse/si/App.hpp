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
#include <qse/cmn/Bitset.hpp>
#include <qse/cmn/time.h>
#include <qse/si/os.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class App: public Uncopyable, public Types, public Mmged
{
public:
	typedef QSE::Bitset<QSE_NSIGS> SignalSet;

	enum SignalState
	{
		SIGNAL_NEGLECTED, // signal is unhandled at the system level.
		SIGNAL_ACCEPTED,  // on_signal callback is triggered
		SIGNAL_DISCARDED  // handled but doesn't trigger the on_signal callback
	};

	App (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
	virtual ~App () QSE_CPP_NOEXCEPT;

	int daemonize (bool chdir_to_root = true, int fork_count = 1, bool root_only = false) QSE_CPP_NOEXCEPT;

	int chroot (const qse_mchar_t* mpath) QSE_CPP_NOEXCEPT;
	int chroot (const qse_wchar_t* wpath) QSE_CPP_NOEXCEPT;

	void sleep (const qse_ntime_t& duration) QSE_CPP_NOEXCEPT 
	{ 
		qse_sleep (&duration); 
	}

	void sleep (qse_long_t secs, qse_int32_t nsecs = 0) QSE_CPP_NOEXCEPT 
	{
		qse_ntime_t duration = { secs, nsecs };
		qse_sleep (&duration);
	}

	void msleep (qse_mtime_t duration) QSE_CPP_NOEXCEPT 
	{
		qse_msleep (duration); 
	}

#if 0
	int switchUser (qse_uid_t uid, qse_gid_t gid, bool permanently) QSE_CPP_NOEXCEPT;
	int restoreUser () QSE_CPP_NOEXCEPT;
#endif

	virtual void on_signal (int sig) { }

	SignalState getSignalSubscription (int sig) const;
	int setSignalSubscription (int sig, SignalState ss, bool ignore_if_unhandled = false);

	int acceptSignal (int sig)
	{
		return this->setSignalSubscription(sig, SIGNAL_ACCEPTED);
	}

	int discardSignal (int sig)
	{
		return this->setSignalSubscription(sig, SIGNAL_DISCARDED);
	}

	int neglectSignal (int sig)
	{
		return this->setSignalSubscription(sig, SIGNAL_NEGLECTED);
	}

	typedef void (*SignalHandler) (int sig);
	static qse_size_t _sighrs[2][QSE_NSIGS];

	// You may set a global signal handler with setSignalHandler().
	// If an application is subscribing to a single with subscribeToSignal(),
	// this function is doomed to fail. If a successful call to 
	// setSignalHandler() has been made withoutut unsetSingalHandler() called
	// yet, a subsequence call to subscribeToSignal() is doomed to fail too.
	// These two different interfaces are mutually exclusive.
	static int setSignalHandler (int sig, SignalHandler sighr);
	static int unsetSignalHandler (int sig, bool ignore = false);

	bool isGuardian () const
	{
		return this->_guarded_child_pid >= 0;
	}

	int guardProcess (const SignalSet& signals, const qse_mchar_t* proc_name = QSE_NULL);

private:
	App* _prev_app;
	App* _next_app;

	struct _SigLink
	{
		_SigLink(): _prev(QSE_NULL), _next(QSE_NULL), _state(SIGNAL_NEGLECTED) {}
		App* _prev;
		App* _next;
		SignalState _state;
	};

	_SigLink _sig[QSE_NSIGS]; 
	long int _guarded_child_pid;

	static int set_signal_handler_no_mutex (int sig, SignalHandler sighr);
	static int unset_signal_handler_no_mutex (int sig, int ignore);
	int set_signal_subscription_no_mutex (int sig, SignalState reqstate, bool ignore_if_unhandled);

	void on_guard_signal (int sig);
	static void handle_signal (int sig);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
