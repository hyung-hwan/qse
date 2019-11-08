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

#ifndef _QSE_SI_APP_H_
#define _QSE_SI_APP_H_

#include <qse/Types.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/cmn/Mmged.hpp>
#include <qse/cmn/Bitset.hpp>
#include <qse/cmn/time.h>
#include <qse/si/Mutex.hpp>
#include <qse/si/os.h>
#include <stdarg.h>

#include <qse/si/log.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

#define QSE_APP_LOG_ENABLED(app, mask) (((app)->getLogMask() & (mask)) == (mask))
#define QSE_APP_LOG0(app, mask, fmt)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt); } while(0)
#define QSE_APP_LOG1(app, mask, fmt, a1)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt, a1); } while(0)
#define QSE_APP_LOG2(app, mask, fmt, a1, a2)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt, a1, a2); } while(0)
#define QSE_APP_LOG3(app, mask, fmt, a1, a2, a3)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt, a1, a2, a3); } while(0)
#define QSE_APP_LOG4(app, mask, fmt, a1, a2, a3, a4)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt, a1, a2, a3, a4); } while(0)
#define QSE_APP_LOG5(app, mask, fmt, a1, a2, a3, a4, a5)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt, a1, a2, a3, a4, a5); } while(0)
#define QSE_APP_LOG6(app, mask, fmt, a1, a2, a3, a4, a5, a6)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt, a1, a2, a3, a4, a5, a6); } while(0)
#define QSE_APP_LOG7(app, mask, fmt, a1, a2, a3, a4, a5, a6, a7)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt, a1, a2, a3, a4, a5, a6, a7); } while(0)
#define QSE_APP_LOG8(app, mask, fmt, a1, a2, a3, a4, a5, a6, a7, a8)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt, a1, a2, a3, a4, a5, a6, a7, a8); } while(0)
#define QSE_APP_LOG9(app, mask, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9)  do { if (QSE_APP_LOG_ENABLED(app, (mask))) (app)->logfmt((mask), fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9); } while(0)

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

	// The neglectSignal() function restored the signal handler
	// to the previous signal handler remembered. If no signal 
	// handler was set up before this application object has been
	// initialized, no signal handler is established for the given
	// signal. the ignore_if_unhandled is true, this function
	// sets up signal handle to ignore the handler instead.
	int neglectSignal (int sig, bool ignore_if_unhandled = false)
	{
		return this->setSignalSubscription(sig, SIGNAL_NEGLECTED, ignore_if_unhandled);
	}

	typedef void (*SignalHandler) (int sig);
	static qse_size_t _sighrs[2][QSE_NSIGS];

	// You may set a global signal handler with setSignalHandler().
	// If an application is subscribing to a signal with subscribeToSignal(),
	// this function is doomed to fail. If a successful call to 
	// setSignalHandler() has been made without unsetSingalHandler() called
	// yet, a subsequent call to subscribeToSignal() is doomed to fail too.
	// These two different interfaces are mutually exclusive.
	static int setSignalHandler (int sig, SignalHandler sighr);
	static int unsetSignalHandler (int sig, bool ignore = false);

	bool isGuardian () const
	{
		return this->_guarded_child_pid >= 0;
	}

	int guardProcess (const SignalSet& signals, qse_mtime_t guard_pause_ms = 0, const qse_mchar_t* proc_name = QSE_NULL);


	// =============================================================
	// LOGGING SUPPORT
	// =============================================================

	enum log_mask_t
	{
		LOG_DEBUG      = (1 << 0),
		LOG_INFO       = (1 << 1),
		LOG_WARN       = (1 << 2),
		LOG_ERROR      = (1 << 3),
		LOG_FATAL      = (1 << 4),

		LOG_TYPE_0     = (1 << 6),
		LOG_TYPE_1     = (1 << 7),
		LOG_TYPE_2     = (1 << 8),
		LOG_TYPE_3     = (1 << 9),
		LOG_TYPE_4     = (1 << 10),
		LOG_TYPE_5     = (1 << 11),
		LOG_TYPE_6     = (1 << 12),
		LOG_TYPE_7     = (1 << 13),
		LOG_TYPE_8     = (1 << 14),
		LOG_TYPE_9     = (1 << 15),

		LOG_ALL_LEVELS = (LOG_DEBUG  | LOG_INFO | LOG_WARN | LOG_ERROR | LOG_FATAL),
		LOG_ALL_TYPES = (LOG_TYPE_0 | LOG_TYPE_1 | LOG_TYPE_2 | LOG_TYPE_3 | LOG_TYPE_4 | LOG_TYPE_5 | LOG_TYPE_6 | LOG_TYPE_7 | LOG_TYPE_8 | LOG_TYPE_9)
	};

	void setLogMask (int mask) { this->_log.mask = mask; }
	int getLogMask () const { return this->_log.mask; }

/*
	void setLogTarget (int flags, const qse_log_target_t& target)
	{
		if (this->_log.logger) qse_log_settarget (this->_log.logger, flags, &target);
	}

	void setLogOption (int oflags)
	{
		qse_log_setoption (&this->_log.logger, oflags);
	}
*/

	
	void logfmt (int mask, const qse_char_t* fmt, ...)
	{
		va_list ap;
		va_start (ap, fmt);
		logfmtv (mask, fmt, ap);
		va_end (ap);
	}

	void logfmtv (int mask, const qse_char_t* fmt, va_list ap);

protected:
	// subclasses may override this if the defaulg logging output is not desired.
	virtual void log_write (int mask, const qse_char_t* msg, qse_size_t len);

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

	qse_cmgr_t* _cmgr;
	struct log_t
	{
		log_t (App* app): mask(0), last_mask(0), len(0), mtx(app->getMmgr())
		{
		}

		int mask, last_mask;
		qse_size_t len;
		qse_char_t buf[256];
		QSE::Mutex mtx;
		qse_log_t logger;
	} _log;

	static int set_signal_handler_no_mutex (int sig, SignalHandler sighr);
	static int unset_signal_handler_no_mutex (int sig, int ignore);
	int set_signal_subscription_no_mutex (int sig, SignalState reqstate, bool ignore_if_unhandled);

	void on_guard_signal (int sig);
	static void handle_signal (int sig);

	static int put_char_to_log_buf (qse_char_t c, void* ctx);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
