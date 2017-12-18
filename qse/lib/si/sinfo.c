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

#include <qse/si/sinfo.h>
#include <qse/cmn/str.h>
#include "../cmn/syscall.h"

int qse_get_highest_fd (void)
{
#if defined(HAVE_GETRLIMIT)
	struct rlimit rlim;
#endif
	int fd = -1;
	QSE_DIR* d;

#if defined(F_MAXFD)
	fd = QSE_FCNTL (0, F_MAXFD, 0);
	if (fd >= 0) return fd;
#endif

	/* will getting the highest file descriptor be faster than
	 * attempting to close any files descriptors less than the 
	 * system limit? */	

	d = QSE_OPENDIR (QSE_MT("/proc/self/fd"));
	if (!d)
	{
		qse_mchar_t buf[64];
		qse_mbsxfmt (buf, QSE_COUNTOF(buf), QSE_MT("/proc/%d/fd"), QSE_GETPID());
		d = QSE_OPENDIR (buf);
		if (!d) d = QSE_OPENDIR (QSE_MT("/dev/fd")); /* Darwin, FreeBSD */
	}

	if (d)
	{
		int maxfd = -1;
		qse_dirent_t* de;
		while ((de = QSE_READDIR (d)))
		{
			qse_long_t l;
			const qse_mchar_t* endptr;

			if (de->d_name[0] == QSE_MT('.')) continue;

			/*QSE_MBSTONUM (l, de->d_name, &endptr, 10);*/
			l = qse_mbstolong (de->d_name, 10, &endptr);
			if (*endptr == QSE_MT('\0'))
			{
				fd = (int)l;
				if ((qse_long_t)fd == l && fd != QSE_DIRFD(d))
				{
					if (fd > maxfd) maxfd = fd;
				}
			}
		}

		QSE_CLOSEDIR (d);
		return maxfd;
	}

/* TODO: should i also use getdtablesize() if available? */

#if defined(HAVE_GETRLIMIT)
	if (QSE_GETRLIMIT (RLIMIT_NOFILE, &rlim) <= -1 ||
	    rlim.rlim_max == RLIM_INFINITY) 
	{
	#if defined(HAVE_SYSCONF)
		fd = sysconf (_SC_OPEN_MAX);
	#endif
	}
	else fd = rlim.rlim_max;
#elif defined(HAVE_SYSCONF)
	fd = sysconf (_SC_OPEN_MAX);
#endif
	if (fd <= -1) fd = 1024; /* fallback */

	/* F_MAXFD is the highest fd. but RLIMIT_NOFILE and 
	 * _SC_OPEN_MAX returnes the maximum number of file 
	 * descriptors. make adjustment */
	if (fd > 0) fd--; 

	return fd;
}

int qse_close_open_fds_using_proc (int* excepts, qse_size_t count)
{
	QSE_DIR* d;

	/* will getting the highest file descriptor be faster than
	 * attempting to close any files descriptors less than the 
	 * system limit? */	

	d = QSE_OPENDIR (QSE_MT("/proc/self/fd"));
	if (!d)
	{
		qse_mchar_t buf[64];
		qse_mbsxfmt (buf, QSE_COUNTOF(buf), QSE_MT("/proc/%d/fd"), QSE_GETPID());
		d = QSE_OPENDIR (buf);
	#if !defined(_SCO_DS)
		/* on SCO OpenServer, a range of file descriptors starting from 0 are
		 * listed under /dev/fd regardless of opening state. And some high 
		 * numbered descriptors are not listed. not reliable */

		if (!d) d = QSE_OPENDIR (QSE_MT("/dev/fd")); /* Darwin, FreeBSD */
	#endif
	}

	if (d)
	{
		qse_dirent_t* de;
		while ((de = QSE_READDIR (d)))
		{
			qse_long_t l;
			const qse_mchar_t* endptr;

			if (de->d_name[0] == QSE_MT('.')) continue;

			/*QSE_MBSTONUM (l, de->d_name, &endptr, 10);*/
			l = qse_mbstolong (de->d_name, 10, &endptr);
			if (*endptr == QSE_MT('\0'))
			{
				int fd = (int)l;
				if ((qse_long_t)fd == l && fd != QSE_DIRFD(d) && fd > 2)
				{
					qse_size_t i;

					for (i = 0; i < count; i++)
					{
						if (fd == excepts[i]) goto skip_close;
					}

					QSE_CLOSE (fd);

				skip_close:
					;
				}
			}
		}

		QSE_CLOSEDIR (d);
		return 0;
	}

	return -1;
}
