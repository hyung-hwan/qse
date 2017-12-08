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
#include "../cmn/syscall.h"
#include <qse/cmn/mbwc.h>

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



int AppRoot::chroot (const qse_mchar_t* mpath) QSE_CPP_NOEXCEPT
{
	return QSE_CHROOT (mpath);
}

int AppRoot::chroot (const qse_wchar_t* wpath) QSE_CPP_NOEXCEPT
{
	qse_mchar_t* mpath;
	mpath = qse_wcstombsdup (wpath, QSE_NULL, this->getMmgr());
	if (!mpath) return -1;
	int n = AppRoot::chroot ((const qse_mchar_t*)mpath);
	this->getMmgr()->dispose (mpath);
	return n;
}

#if 0
int AppRoot::switchPrivilege (int gid, int uid, bool permanently)
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

int AppRoot::restorePrivilege ()
{
	if (QSE_GETEUID() != this->saved_euid) seteuid (this->saved_euid);
	if (QSE_GETEGID() != this->saved_egid) setegid (this->saved_egid);
	if (this->saved_euid == 0) setgroups (this->saved_ngroups, this->saved_groups);
	return 0;
}
#endif

#if 0
int main ()
{
	AppRoot app;

	app.daemonize();
	app.chuser ();
	app.chgroup ();
	app.chroot ();

	app.catchSignal (SIGINT, xxxxx);
	app.catchSignal (SIGTERM, xxx);

	return 0;
}
#endif

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////