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

#ifndef _QSE_SI_APP_HPP_
#define _QSE_SI_APP_HPP_

#include <qse/Types.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/cmn/Mmged.hpp>
#include <qse/cmn/Named.hpp>
#include <qse/cmn/Bitset.hpp>
#include <qse/cmn/time.h>
#include <qse/si/Mutex.hpp>
#include <qse/si/os.h>
#include <qse/si/log.h>
#include <stdarg.h>


/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

#define QSE_APP_LOG_ENABLED(app, pri) (((app)->getLogPriorityMask() & (pri)) == (pri))
#define QSE_APP_LOG0(app, pri, fmt)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt); } while(0)
#define QSE_APP_LOG1(app, pri, fmt, a1)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt, a1); } while(0)
#define QSE_APP_LOG2(app, pri, fmt, a1, a2)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt, a1, a2); } while(0)
#define QSE_APP_LOG3(app, pri, fmt, a1, a2, a3)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt, a1, a2, a3); } while(0)
#define QSE_APP_LOG4(app, pri, fmt, a1, a2, a3, a4)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt, a1, a2, a3, a4); } while(0)
#define QSE_APP_LOG5(app, pri, fmt, a1, a2, a3, a4, a5)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt, a1, a2, a3, a4, a5); } while(0)
#define QSE_APP_LOG6(app, pri, fmt, a1, a2, a3, a4, a5, a6)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt, a1, a2, a3, a4, a5, a6); } while(0)
#define QSE_APP_LOG7(app, pri, fmt, a1, a2, a3, a4, a5, a6, a7)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt, a1, a2, a3, a4, a5, a6, a7); } while(0)
#define QSE_APP_LOG8(app, pri, fmt, a1, a2, a3, a4, a5, a6, a7, a8)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt, a1, a2, a3, a4, a5, a6, a7, a8); } while(0)
#define QSE_APP_LOG9(app, pri, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9)  do { if (QSE_APP_LOG_ENABLED(app, (pri))) (app)->logfmt((pri), fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9); } while(0)

class App: public Uncopyable, public Types, public Mmged, public Named<32>
{
public:
	typedef QSE::Bitset<QSE_NSIGS> SignalSet;
	typedef long int child_pid_t;

	enum SignalState
	{
		SIGNAL_NEGLECTED, // signal is unhandled at the system level.
		SIGNAL_ACCEPTED,  // on_signal callback is triggered
		SIGNAL_DISCARDED  // handled but doesn't trigger the on_signal callback
	};

	App (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
	virtual ~App () QSE_CPP_NOEXCEPT;

	void setName (const qse_char_t* name) QSE_CPP_NOEXCEPT;

	void setCmgr (qse_cmgr_t* cmgr) QSE_CPP_NOEXCEPT
	{
		this->_cmgr = cmgr;
	}
	qse_cmgr_t* getCmgr () QSE_CPP_NOEXCEPT
	{
		return this->_cmgr;
	}
	const qse_cmgr_t* getCmgr () const QSE_CPP_NOEXCEPT
	{
		return this->_cmgr;
	}

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

	SignalState getSignalSubscription (int sig) const QSE_CPP_NOEXCEPT;
	int setSignalSubscription (int sig, SignalState ss, bool ignore_if_unhandled = false) QSE_CPP_NOEXCEPT;

	int acceptSignal (int sig) QSE_CPP_NOEXCEPT
	{
		return this->setSignalSubscription(sig, SIGNAL_ACCEPTED);
	}

	int discardSignal (int sig) QSE_CPP_NOEXCEPT
	{
		return this->setSignalSubscription(sig, SIGNAL_DISCARDED);
	}

	// The neglectSignal() function restored the signal handler
	// to the previous signal handler remembered. If no signal 
	// handler was set up before this application object has been
	// initialized, no signal handler is established for the given
	// signal. the ignore_if_unhandled is true, this function
	// sets up signal handle to ignore the handler instead.
	int neglectSignal (int sig, bool ignore_if_unhandled = false) QSE_CPP_NOEXCEPT
	{
		return this->setSignalSubscription(sig, SIGNAL_NEGLECTED, ignore_if_unhandled);
	}

	int acceptSignals (const SignalSet& signals) QSE_CPP_NOEXCEPT;
	int discardSignals (const SignalSet& signals) QSE_CPP_NOEXCEPT;
	int neglectSignals (const SignalSet& signals, bool ignore_if_unhandled = false) QSE_CPP_NOEXCEPT;


	typedef void (*SignalHandler) (int sig);
	static qse_size_t _sighrs[2][QSE_NSIGS];

	// You may set a global signal handler with setSignalHandler().
	// If an application is subscribing to a signal with subscribeToSignal(),
	// this function is doomed to fail. If a successful call to 
	// setSignalHandler() has been made without unsetSingalHandler() called
	// yet, a subsequent call to subscribeToSignal() is doomed to fail too.
	// These two different interfaces are mutually exclusive.
	static int setSignalHandler (int sig, SignalHandler sighr) QSE_CPP_NOEXCEPT;
	static int unsetSignalHandler (int sig, bool ignore = false) QSE_CPP_NOEXCEPT;

	bool isGuardian () const QSE_CPP_NOEXCEPT
	{
		return this->_guarded_child_pid >= 0;
	}

	int guardProcess (const SignalSet& signals, bool _guard = true, qse_mtime_t guard_pause_ms = 0, const qse_mchar_t* proc_name = QSE_NULL) QSE_CPP_NOEXCEPT;


	// =============================================================
	// LOGGING SUPPORT
	// =============================================================
	void setSyslogFacility (qse_log_facility_t fac) QSE_CPP_NOEXCEPT /* useful for syslog is set as a target */
	{
		qse_log_setsyslogfacility (&this->_log.logger, fac);
	}

	void setLogPriorityMask (int pri_mask) QSE_CPP_NOEXCEPT { this->_log.pri_mask = pri_mask;  }
	int getLogPriorityMask () const QSE_CPP_NOEXCEPT { return this->_log.pri_mask; }

	void setLogOption (int option_flags) QSE_CPP_NOEXCEPT /* 0 or bitwise-OR'ed of qse_log_option_flag_t bits */
	{
		qse_log_setoption (&this->_log.logger, option_flags);
	}

	int getLogOption () const QSE_CPP_NOEXCEPT
	{
		return qse_log_getoption(&this->_log.logger);
	}

	void setLogTarget (int target_flags, const qse_log_target_data_t& target) QSE_CPP_NOEXCEPT
	{
		qse_log_settarget (&this->_log.logger, target_flags, &target);
	}

	int getLogTarget (qse_log_target_data_t& target) const QSE_CPP_NOEXCEPT
	{
		return qse_log_gettarget(&this->_log.logger, &target);
	}

	void logfmt (qse_log_priority_flag_t pri, const qse_char_t* fmt, ...) QSE_CPP_NOEXCEPT
	{
		va_list ap;
		va_start (ap, fmt);
		logfmtv (pri, fmt, ap);
		va_end (ap);
	}

	void logfmtv (qse_log_priority_flag_t pri, const qse_char_t* fmt, va_list ap) QSE_CPP_NOEXCEPT;

protected:
	// subclasses may override this if the defaulg logging output is not desired.
	virtual void log_write (qse_log_priority_flag_t mask, const qse_char_t* msg, qse_size_t len) QSE_CPP_NOEXCEPT;

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
	child_pid_t _guarded_child_pid;

	qse_cmgr_t* _cmgr;
	struct log_t
	{
		log_t (App* app): pri_mask(0), last_pri(QSE_LOG_PANIC), len(0), mtx(app->getMmgr())
		{
		}

		int pri_mask; /* what priorities to log */
		qse_log_priority_flag_t last_pri;
		qse_size_t len;
		qse_char_t buf[256];
		QSE::Mutex mtx;
		mutable qse_log_t logger;
	} _log;

	static int set_signal_handler_no_mutex (int sig, SignalHandler sighr) QSE_CPP_NOEXCEPT;
	static int unset_signal_handler_no_mutex (int sig, int ignore) QSE_CPP_NOEXCEPT;
	int set_signal_subscription_no_mutex (int sig, SignalState reqstate, bool ignore_if_unhandled) QSE_CPP_NOEXCEPT;

	void on_guard_signal (int sig) QSE_CPP_NOEXCEPT;
	static void handle_signal (int sig) QSE_CPP_NOEXCEPT;

	static int put_char_to_log_buf (qse_char_t c, void* ctx) QSE_CPP_NOEXCEPT;

protected:
	enum guarded_child_state_t
	{
		GUARDED_CHILD_STARTED,
		GUARDED_CHILD_EXITED
	};

	void on_guarded_child (child_pid_t pid, guarded_child_state_t state, int status) QSE_CPP_NOEXCEPT {};
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
