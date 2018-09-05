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

App::App (Mmgr* mmgr) QSE_CPP_NOEXCEPT: Mmged(mmgr), _root_only(false), _prev_app(QSE_NULL), _next_app(QSE_NULL)
{
	ScopedMutexLocker sml(g_app_mutex);
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
	ScopedMutexLocker sml(g_app_mutex);
	this->unsubscribe_from_all_signals_no_mutex ();
	if (this->_next_app) this->_next_app->_prev_app = this->_prev_app;
	if (this->_prev_app) this->_prev_app->_next_app = this->_next_app;
	if (this == g_app_top) 
	{
		QSE_ASSERT (this->_prev_app == QSE_NULL);
		g_app_top = this->_next_app;
	}
}

int App::daemonize (bool chdir_to_root, int fork_count) QSE_CPP_NOEXCEPT
{
	if (this->_root_only && QSE_GETEUID() != 0) return -1;

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

		if (qse_close_open_fds_using_proc (keep, QSE_COUNTOF(keep)) <= -1)
		{
			for (int i = qse_get_highest_fd(); i >= 3; i--) QSE_CLOSE (i);
		}

		int fd = QSE_OPEN ("/dev/null", O_RDWR, 0);
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
	ScopedMutexLocker sml(g_app_mutex);
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
		sigemptyset (&sa.sa_mask);
		sa.sa_flags |= SA_SIGINFO;
	}
	else
	{
		sa.sa_handler = dispatch_signal;
		sigemptyset (&sa.sa_mask);
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

int App::unsetSignalHandler (int sig)
{
	ScopedMutexLocker sml(g_app_mutex);
	return App::unset_signal_handler_no_mutex(sig);
}

int App::unset_signal_handler_no_mutex(int sig)
{
	if (!App::_sighrs[0][sig]) return -1;

	struct sigaction sa;

	sa.sa_mask = g_app_oldsi[sig].sa_mask;
	sa.sa_flags = g_app_oldsi[sig].sa_flags;
	if (sa.sa_flags & SA_SIGINFO)
		sa.sa_sigaction = (void(*)(int,siginfo_t*,void*))App::_sighrs[1][sig];
	else
		sa.sa_handler = (SignalHandler)App::_sighrs[1][sig];

	if (::sigaction (sig, &sa, QSE_NULL) <= -1) return -1;

	App::_sighrs[0][sig] = 0;
	App::_sighrs[1][sig] = 0;
	return 0;
}

static void handle_signal (int sig)
{
	ScopedMutexLocker sml(g_app_mutex);
	App* app = g_app_sig[sig];
	while (app)
	{
		App::_SigLink& sl = app->_sig[sig];
		App* next = sl._next;
		if (sl._state == App::_SigLink::ACCEPTED) 
		{
			// the actual signal handler is called with the mutex locked.
			// it must not call subscribeToSingal() or unsubscribeFromSingal()
			// from within the handler.
			app->on_signal (sig);
		}
		app = next;
	}
}

int App::subscribeToSignal (int sig, bool accept)
{
	QSE_ASSERT (sig >= 0 && sig < QSE_NSIGS);

	int reqstate = accept? _SigLink::ACCEPTED: _SigLink::IGNORED;
	ScopedMutexLocker sml(g_app_mutex);

	_SigLink& sl = this->_sig[sig];
	if (QSE_LIKELY(sl._state == _SigLink::UNHANDLED))
	{
		QSE_ASSERT (sl._prev == QSE_NULL && sl._next == QSE_NULL);

		App* xapp = g_app_sig[sig];
		App* xapp_xprev = QSE_NULL;

		g_app_sig[sig] = this;
		sl._state = _SigLink::ACCEPTED;
		sl._next = xapp;
		if (xapp) 
		{
			xapp_xprev = xapp->_sig[sig]._prev;
			xapp->_sig[sig]._prev = this;
		}
		else
		{
			// no application is set to accept this signal.
			// this is the first time to 
			if (App::set_signal_handler_no_mutex(sig, handle_signal) <= -1)
			{
				// roll back 
				g_app_sig[sig] = xapp;
				if (xapp) xapp->_sig[sig]._prev = xapp_xprev;
				sl._state = _SigLink::UNHANDLED;
				sl._next = QSE_NULL;
				QSE_ASSERT (sl._prev == QSE_NULL);
				return -1;
			}
		}

		QSE_ASSERT (sl._prev == QSE_NULL);
	}
	else 
	{
		// already configured to receive the signal. change the state only
		QSE_ASSERT (g_app_sig[sig] != QSE_NULL);
		sl._state = reqstate;
	}

	return 0;
}

void App::unsubscribeFromSignal (int sig)
{
	QSE_ASSERT (sig >= 0 && sig < QSE_NSIGS);
	ScopedMutexLocker sml(g_app_mutex);
	this->unsubscribe_from_signal_no_mutex (sig);
}

void App::unsubscribe_from_signal_no_mutex (int sig)
{
	_SigLink& sl = this->_sig[sig];
	if (QSE_UNLIKELY(sl._state == _SigLink::UNHANDLED))
	{
		QSE_ASSERT (g_app_sig[sig] != this);
		QSE_ASSERT (sl._prev == QSE_NULL && sl._next == QSE_NULL);
		// nothing to do
	}
	else
	{
		QSE_ASSERT (g_app_sig[sig] != QSE_NULL);

		if (g_app_sig[sig] == this) 
		{
			QSE_ASSERT (sl._prev == QSE_NULL);
			if (!sl._next) App::unset_signal_handler_no_mutex (sig);
			g_app_sig[sig] = sl._next;
		}

		if (sl._next) sl._next->_sig[sig]._prev = sl._prev;
		if (sl._prev) sl._prev->_sig[sig]._next = sl._next;
		sl._prev = QSE_NULL;
		sl._next = QSE_NULL;
		sl._state = _SigLink::UNHANDLED;
	}
}

void App::unsubscribe_from_all_signals_no_mutex()
{
	for (int i = 0; i < QSE_NSIGS; i++)
	{
		this->unsubscribe_from_signal_no_mutex (i);
	}
}

int App::guardProcess (const qse_mchar_t* proc_name)
{
// TODO: enhance it
	while (1)
	{
		pid_t pid = ::fork();
		if (pid == -1) return -1;
		if (pid == 0) break; // child

		int status;
		while (::waitpid(pid, &status, 0) != pid);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) 
		{
			return 0;
		}
	}

	// TODO: if (proc_name) qse_set_proc_name (proc_name);
	return 1; // the caller must execute the actual work.
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
