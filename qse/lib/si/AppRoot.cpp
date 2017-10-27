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

#include <qse/si/AppRoot.hpp>
#include <qse/si/sinfo.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>
#include "../cmn/syscall.h"

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

int AppRoot::daemonize (bool chdir_to_root, int fork_count) QSE_CPP_NOEXCEPT
{
	if (this->_root_only && QSE_GETEUID() != 0) return -1;

	if (fork_count >= 1)
	{
		if (fork_count >= 2)
		{
			struct sigaction sa;
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

#if 0
int AppRoot::switchUser () QSE_CPP_NOEXCEPT
{
	struct passwd* pw;

	pw = getpwnam (username);
	if (!pw)


	if (QSE_SETGID(pw->pw_gid) == -1)
	{
	}

	QSE_SETEGID(gid);
	QSE_SETUID(uid);
	QSE_SETEUID(uid);

}
#endif


int AppRoot::chroot (const qse_mchar_t* mpath) QSE_CPP_NOEXCEPT
{
	int orgdirfd;

	orgdirfd = QSE_OPEN (".", O_RDONLY, 0);
	if (orgdirfd == -1) return -1;

	if (QSE_CHDIR(mpath) == -1) return -1;
	if (QSE_CHROOT(mpath) == -1)
	{
		QSE_FCHDIR (orgdirfd);
		QSE_CLOSE (orgdirfd);
		return -1;
	}

	QSE_CLOSE (orgdirfd);
	QSE_CHROOT ("/");

	return 0;
}

int AppRoot::chroot (const qse_wchar_t* wpath) QSE_CPP_NOEXCEPT
{
	qse_mchar_t* mpath;

	mpath = qse_wcstombsdup (wpath, QSE_NULL, QSE_MMGR_GETDFL());
	if (!mpath) return -1;

	int n = AppRoot::chroot (mpath);
	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), mpath);
	return n;
}

void AppRoot::on_signal () QSE_CPP_NOEXCEPT
{
}


#if 0
int main ()
{
	AppRoot app;

	app.daemonize();
	app.switchUser ("nobody", "nobody");
	app.switchUser (10, 20);
	app.catchSignal (SIGINT, xxxxx);
	app.catchSignal (SIGTERM, xxx);

	return 0;
}
#endif

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
