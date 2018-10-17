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

#include <qse/si/App.hpp>
#include <qse/si/Mutex.hpp>
#include <qse/si/sinfo.h>
#include <qse/si/os.h>
#include "../cmn/syscall.h"
#include <qse/cmn/mbwc.h>


/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

static Mutex g_app_mutex;
static App* g_app_top = QSE_NULL; // maintain the chain of application objects
static App* g_app_sig[QSE_NSIGS] = { QSE_NULL, };
static struct
{
	sigset_t sa_mask;
	int      sa_flags;
} g_app_oldsi[QSE_NSIGS] = { { 0, 0 }, };

class SigScopedMutexLocker
{
public:
	SigScopedMutexLocker (Mutex& mutex): mutex(mutex)
	{
		sigset_t sigset;
		::sigfillset (&sigset);
		::sigprocmask (SIG_BLOCK, &sigset, &this->oldsigset);
// TODO: would this work properly if this is called within a thread?
//       do i need to use pthread_sigmask() conditionally?
		this->mutex.lock ();
	}

	~SigScopedMutexLocker ()
	{
		this->mutex.unlock ();
		::sigprocmask (SIG_SETMASK, &this->oldsigset, QSE_NULL);
	}
protected:
	Mutex& mutex;
	sigset_t oldsigset;
};

App::App (Mmgr* mmgr) QSE_CPP_NOEXCEPT: Mmged(mmgr), _prev_app(QSE_NULL), _next_app(QSE_NULL), _guarded_child_pid(-1)
{
	SigScopedMutexLocker sml(g_app_mutex);
	if (!g_app_top)
	{
		g_app_top = this;
	}
	else
	{
		g_app_top->_prev_app = this;
		this->_next_app = g_app_top;
	}
}

App::~App () QSE_CPP_NOEXCEPT 
{
	SigScopedMutexLocker sml(g_app_mutex);
	for (int i = 0; i < QSE_NSIGS; i++)
	{
		// if the signal handler has not been removed before this application
		// is destroyed and this is the last application subscribing to 
		// a signal, i arrange to set the signal handler to SIG_IGN as indicated by
		// the third argument 'true' to set_signal_subscription_no_mutex().
		// if the signal is not ignored, the signal received after destruction
		// of this application object may lead to program crash as the handler
		// is still associated with the destructed object.

		// if the signal handler has been removed, the following function
		// actual does nothing.
		this->set_signal_subscription_no_mutex (i, SIGNAL_NEGLECTED, true);
	}

	if (this->_next_app) this->_next_app->_prev_app = this->_prev_app;
	if (this->_prev_app) this->_prev_app->_next_app = this->_next_app;
	if (this == g_app_top) 
	{
		QSE_ASSERT (this->_prev_app == QSE_NULL);
		g_app_top = this->_next_app;
	}
}

int App::daemonize (bool chdir_to_root, int fork_count, bool root_only) QSE_CPP_NOEXCEPT
{
	if (root_only && QSE_GETEUID() != 0) return -1;

	if (fork_count >= 1)
	{
		if (fork_count >= 2)
		{
			int n = QSE_FORK();
			if (n == -1) return -1;
			if (n != 0) QSE_EXIT(0);

			QSE_SETSID ();
		}

		int n = QSE_FORK();
		if (n == -1) return -1;
		if (n != 0) QSE_EXIT(0); // TODO: should i call exit() in stdlib.h? 
	}

	QSE_UMASK (0);
	if (chdir_to_root) QSE_CHDIR("/"); // don't care about failure

	if (true)
	{
		int keep[] = { 0, 1, 2};

		if (qse_close_open_fds_using_proc(keep, QSE_COUNTOF(keep)) <= -1)
		{
			for (int i = qse_get_highest_fd(); i >= 3; i--) QSE_CLOSE (i);
		}

		int fd = QSE_OPEN("/dev/null", O_RDWR, 0);
		if (fd >= 0)
		{
			if (fd != 0) QSE_DUP2 (fd, 0);
			if (fd != 1) QSE_DUP2 (fd, 1);
			if (fd != 2) QSE_DUP2 (fd, 2);
			if (fd > 2) QSE_CLOSE (fd);
		}
	}

	if (fork_count <= 1) 
	{
		if (fork_count <= 0) 
		{
			// detach from the controlling terminal manually
			// when no fork() is requested though setsid() is supposed to 
			// do this as well. 
			int fd = QSE_OPEN ("/dev/tty", O_RDWR, 0);
			if (fd >= 0)
			{
				QSE_IOCTL(fd, TIOCNOTTY, NULL);
				QSE_CLOSE (fd);
			}

			// since no fork() has been executed, the process that has
			// executed this process will remain as the parent. there
			// will happen no orphaning of this process and take-over
			// by init happen
		}

		QSE_SETSID (); // don't care about failure
	}

	return 0;
}

int App::chroot (const qse_mchar_t* mpath) QSE_CPP_NOEXCEPT
{
	return QSE_CHROOT(mpath);
}

int App::chroot (const qse_wchar_t* wpath) QSE_CPP_NOEXCEPT
{
	qse_mchar_t* mpath;
	mpath = qse_wcstombsdup (wpath, QSE_NULL, this->getMmgr());
	if (!mpath) return -1;
	int n = App::chroot((const qse_mchar_t*)mpath);
	this->getMmgr()->dispose (mpath);
	return n;
}

void App::sleep (const qse_ntime_t* duration) QSE_CPP_NOEXCEPT
{
	qse_sleep (duration);
}

void App::sleep (unsigned int seconds) QSE_CPP_NOEXCEPT
{
	static qse_ntime_t duration = { seconds, 0 };
	qse_sleep (&duration);
}

#if 0
int App::switchPrivilege (int gid, int uid, bool permanently)
{
	gid = QSE_GETGID();
	uid = QSE_GETUID();

	this->saved_egid = QSE_GETEGID();
	this->saved_euid = QSE_GETEUID();
	this->saved_ngroups = getgroups (QSE_COUNTOF(this->saved_groups), this->saved_groups);

	if (this->saved_euid ==  0) setgrops (1, gid);

	setegid (gid);
	//setregid (-1, gid);

	if (uid != this->saved_euid)
	{
		seteuid (uid);
		//setreuid (-1, uid);
	}
}

int App::restorePrivilege ()
{
	if (QSE_GETEUID() != this->saved_euid) seteuid (this->saved_euid);
	if (QSE_GETEGID() != this->saved_egid) setegid (this->saved_egid);
	if (this->saved_euid == 0) setgroups (this->saved_ngroups, this->saved_groups);
	return 0;
}
#endif

qse_size_t App::_sighrs[2][QSE_NSIGS] = { { 0, }, };

static void dispatch_signal (int sig)
{
	((App::SignalHandler)App::_sighrs[0][sig]) (sig);
	if (App::_sighrs[1][sig] && App::_sighrs[1][sig] != (qse_size_t)SIG_IGN && App::_sighrs[1][sig] != (qse_size_t)SIG_DFL) 
	{
		((App::SignalHandler)App::_sighrs[1][sig]) (sig);
	}
}

static void dispatch_siginfo (int sig, siginfo_t* si, void* ctx)
{
	((App::SignalHandler)App::_sighrs[0][sig]) (sig);
	if (App::_sighrs[1][sig] && App::_sighrs[1][sig] != (qse_size_t)SIG_IGN && App::_sighrs[1][sig] != (qse_size_t)SIG_DFL) 
	{
		((void(*)(int, siginfo_t*, void*))App::_sighrs[1][sig]) (sig, si, ctx);
	}
}

int App::setSignalHandler (int sig, SignalHandler sighr)
{
	SigScopedMutexLocker sml(g_app_mutex);
	return App::set_signal_handler_no_mutex (sig, sighr);
}

int App::set_signal_handler_no_mutex (int sig, SignalHandler sighr)
{
	if (App::_sighrs[0][sig]) return -1; // already set

	struct sigaction sa, oldsa;

	if (::sigaction(sig, QSE_NULL, &oldsa) == -1) return -1;

	if (oldsa.sa_flags & SA_SIGINFO)
	{
		sa.sa_sigaction = dispatch_siginfo;
		::sigfillset (&sa.sa_mask); // block all signals while the handler is being executed
		sa.sa_flags |= SA_SIGINFO;
	}
	else
	{
		sa.sa_handler = dispatch_signal;
		::sigfillset (&sa.sa_mask); 
		sa.sa_flags = 0;
		//sa.sa_flags |= SA_INTERUPT;
		//sa.sa_flags |= SA_RESTART;
	}

	if (::sigaction(sig, &sa, QSE_NULL) == -1) return -1;

	App::_sighrs[0][sig] = (qse_size_t)sighr;
	App::_sighrs[1][sig] = (qse_size_t)oldsa.sa_handler;
	g_app_oldsi[sig].sa_mask = oldsa.sa_mask;
	g_app_oldsi[sig].sa_flags = oldsa.sa_flags;

	return 0;
}

int App::unsetSignalHandler (int sig, bool ignore)
{
	SigScopedMutexLocker sml(g_app_mutex);
	return App::unset_signal_handler_no_mutex(sig, ignore);
}

int App::unset_signal_handler_no_mutex(int sig, int ignore)
{
	if (!App::_sighrs[0][sig]) return -1;

	struct sigaction sa;

	sa.sa_mask = g_app_oldsi[sig].sa_mask;
	sa.sa_flags = g_app_oldsi[sig].sa_flags;
	if (ignore)
	{
		sa.sa_handler = SIG_IGN;
		sa.sa_flags &= ~SA_SIGINFO;
	}
	else if (sa.sa_flags & SA_SIGINFO)
	{
		sa.sa_sigaction = (void(*)(int,siginfo_t*,void*))App::_sighrs[1][sig];
	}
	else
	{
		sa.sa_handler = (SignalHandler)App::_sighrs[1][sig];
	}

	if (::sigaction (sig, &sa, QSE_NULL) <= -1) return -1;

	App::_sighrs[0][sig] = 0;
	App::_sighrs[1][sig] = 0;
	return 0;
}

/*static*/ void App::handle_signal (int sig)
{
	// Note: i use ScopedMutexLocker in the signal handler
	//       whereas I use SigScopedMutexLocker in other functions. 
	//       as SigScopedMutexLock blocks all signals, this signal handler
	//       is not called while those other functions are holding on to
	//       this mutex object.
	ScopedMutexLocker sml(g_app_mutex); 

	App* app = g_app_sig[sig];
	while (app)
	{
		App::_SigLink& sl = app->_sig[sig];
		App* next = sl._next;
		if (sl._state == App::SIGNAL_ACCEPTED) 
		{
			// the actual signal handler is called with the mutex locked.
			// it must not call acceptSingal()/discardSignal()neglectSingal()
			// from within the handler.
			if (app->_guarded_child_pid >= 0) app->on_guard_signal (sig);
			app->on_signal (sig);
		}
		app = next;
	}
}

void App::on_guard_signal (int sig)
{
	::kill (this->_guarded_child_pid, sig);
}

App::SignalState App::getSignalSubscription (int sig) const
{
	QSE_ASSERT (sig >= 0 && sig < QSE_NSIGS);
	return this->_sig[sig]._state;
}

int App::setSignalSubscription (int sig, SignalState ss, bool ignore_if_unhandled)
{
	QSE_ASSERT (sig >= 0 && sig < QSE_NSIGS);
	SigScopedMutexLocker sml(g_app_mutex);
	return this->set_signal_subscription_no_mutex(sig, ss, ignore_if_unhandled);
}

int App::set_signal_subscription_no_mutex (int sig, SignalState reqstate, bool ignore_if_unhandled)
{
	_SigLink& sl = this->_sig[sig];

	if (QSE_UNLIKELY(sl._state == reqstate)) return 0; // no change

	if (reqstate == SIGNAL_NEGLECTED)
	{
		// accepted/discarded -> neglected(unhandled)
		QSE_ASSERT (g_app_sig[sig] != QSE_NULL);

		if (g_app_sig[sig] == this) 
		{
			QSE_ASSERT (sl._prev == QSE_NULL);
			if (!sl._next) 
			{
				// this is the last application subscribing to the signal.
				// let's get rid of the signal handler
				if (App::unset_signal_handler_no_mutex(sig, ignore_if_unhandled) <= -1) return -1;
			}
			g_app_sig[sig] = sl._next;
		}

		if (sl._next) sl._next->_sig[sig]._prev = sl._prev;
		if (sl._prev) sl._prev->_sig[sig]._next = sl._next;
		sl._prev = QSE_NULL;
		sl._next = QSE_NULL;
		sl._state = reqstate;
	}
	else if (QSE_LIKELY(sl._state == SIGNAL_NEGLECTED))
	{
		// neglected(unhandled) -> accepted/discarded
		QSE_ASSERT (sl._prev == QSE_NULL && sl._next == QSE_NULL);

		App* xapp = g_app_sig[sig];
		App* xapp_xprev = QSE_NULL;

		g_app_sig[sig] = this;
		sl._state = SIGNAL_ACCEPTED;
		sl._next = xapp;
		if (xapp) 
		{
			xapp_xprev = xapp->_sig[sig]._prev;
			xapp->_sig[sig]._prev = this;
		}
		else
		{
			// no application is set to accept this signal.
			// it is the first time to set the system-level signal handler.
			if (App::set_signal_handler_no_mutex(sig, App::handle_signal) <= -1)
			{
				// roll back 
				g_app_sig[sig] = xapp;
				if (xapp) xapp->_sig[sig]._prev = xapp_xprev;
				sl._state = SIGNAL_NEGLECTED;
				sl._next = QSE_NULL;
				QSE_ASSERT (sl._prev == QSE_NULL);
				return -1;
			}
		}

		QSE_ASSERT (sl._prev == QSE_NULL);
	}
	else 
	{
		// accpeted/ignored -> ignored/accepted
		QSE_ASSERT (g_app_sig[sig] != QSE_NULL);
		sl._state = reqstate;
	}

	return reqstate;
}

int App::guardProcess (const SignalSet& signals, const qse_mchar_t* proc_name)
{
	SignalState old_ss[QSE_NSIGS];
	int seq = 0;

	for (int i = 0; i < QSE_NSIGS; i++)
	{
		if (signals.isSet(i)) 
		{
			old_ss[i] = this->getSignalSubscription(i);
			this->setSignalSubscription (i, SIGNAL_ACCEPTED);
		}
	}

	while (1)
	{
		if (seq < QSE_TYPE_MAX(int)) ++seq;

		pid_t pid = ::fork();
		if (pid == -1) return -1;
		if (pid == 0) 
		{
			// child process
			this->_guarded_child_pid = -1;

			for (int i = 0; i < QSE_NSIGS; i++)
			{
				if (signals.isSet(i)) this->setSignalSubscription (i, old_ss[i]);
			}

			::setpgid (0, 0); // change the process group id. 
			break; 
		}

		// ===============================================
		// the guardian(parent) process
		// ===============================================
		this->_guarded_child_pid = pid;

		int status;
		while (::waitpid(pid, &status, 0) != pid) 
		{
			// if there is a signal sent to this guarding process,
			// the signal should be relayed to the child process
			// via on_guard_signal().
			// ------------------------------------------------------
			// do nothing inside this loop block.
			// ------------------------------------------------------
		}

		if (WIFEXITED(status))
		{
			if (WEXITSTATUS(status) == 0) 
			{
				// the child has terminated normally and successfully.
				for (int i = 0; i < QSE_NSIGS; i++)
				{
					if (signals.isSet(i)) this->setSignalSubscription (i, old_ss[i]);
				}
				return 0; 
			}
		}
		else
		{
			// the child process aborted or crashed.
			// let's kill all other processes in the same process group as the child
			// but is it safe to do this after waitpid() has been called on 'pid'?
			// it should be mostly safe because most OSes doesn't reuse the same pid
			// within a very short time.
			::kill (-pid, SIGKILL); 
		}
	}

	// TODO: if (proc_name) qse_set_proc_name (proc_name);
	return seq; // the caller must execute the actual work.
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
