/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/pio.h>
#include <qse/cmn/mbwc.h>
#include "mem.h"

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#elif defined(__OS2__)
#	define INCL_DOSQUEUES
#	define INCL_DOSPROCESS
#	define INCL_DOSERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <io.h>
#	include <errno.h>
#else
#	include "syscall.h"
#	if defined(HAVE_SPAWN_H)
#		include <spawn.h>
#	endif
#endif

static qse_ssize_t pio_input (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size);
static qse_ssize_t pio_output (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size);

#if defined(_WIN32)
static qse_pio_errnum_t syserr_to_errnum (DWORD e)
{
	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			return QSE_PIO_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_PIO_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_PIO_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_PIO_ENOENT;

		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			return QSE_PIO_EEXIST;

		case ERROR_BROKEN_PIPE:
			return QSE_PIO_EPIPE;

		default:
			return QSE_PIO_ESYSERR;
	}
}
#elif defined(__OS2__)
static qse_pio_errnum_t syserr_to_errnum (APIRET e)
{
	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
			return QSE_PIO_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_PIO_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_PIO_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_PIO_ENOENT;

		case ERROR_ALREADY_EXISTS:
			return QSE_PIO_EEXIST;

		case ERROR_BROKEN_PIPE:
			return QSE_PIO_EPIPE;

		default:
			return QSE_PIO_ESYSERR;
	}
}
#elif defined(__DOS__)
static qse_pio_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_PIO_ENOMEM;

		case EINVAL:
			return QSE_PIO_EINVAL;

		case EACCES:
			return QSE_PIO_EACCES;

		case ENOENT:
			return QSE_PIO_ENOENT;

		case EEXIST:
			return QSE_PIO_EEXIST;
	
		default:
			return QSE_PIO_ESYSERR;
	}
}
#else
static qse_pio_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_PIO_ENOMEM;

		case EINVAL:
			return QSE_PIO_EINVAL;

		case EACCES:
			return QSE_PIO_EACCES;

		case ENOENT:
			return QSE_PIO_ENOENT;

		case EEXIST:
			return QSE_PIO_EEXIST;
	
		case EINTR:
			return QSE_PIO_EINTR;

		case EPIPE:
			return QSE_PIO_EPIPE;

		default:
			return QSE_PIO_ESYSERR;
	}
}
#endif

static qse_pio_errnum_t tio_errnum_to_pio_errnum (qse_tio_t* tio)
{
	/* i only translate error codes that's relevant
	 * to pio. all other errors becom QSE_PIO_EOTHER */
	switch (tio->errnum)
	{
		case QSE_TIO_ENOMEM:
			return QSE_PIO_ENOMEM;
		case QSE_TIO_EINVAL:
			return QSE_PIO_EINVAL;
		case QSE_TIO_ENOENT:
			return QSE_PIO_ENOENT;
		case QSE_TIO_EACCES:
			return QSE_PIO_EACCES;
		case QSE_TIO_EILSEQ:
			return QSE_PIO_EILSEQ;
		case QSE_TIO_EICSEQ:
			return QSE_PIO_EICSEQ;
		case QSE_TIO_EILCHR:
			return QSE_PIO_EILCHR;
		default:
			return QSE_PIO_EOTHER;
	}
}

qse_pio_t* qse_pio_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, 
	const qse_char_t* cmd, qse_env_t* env, int flags)
{
	qse_pio_t* pio;

	pio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_pio_t) + xtnsize);
	if (pio)
	{
		if (qse_pio_init (pio, mmgr, cmd, env, flags) <= -1)
		{
			QSE_MMGR_FREE (mmgr, pio);
			pio = QSE_NULL;
		}
		else
		{
			QSE_MEMSET (pio + 1, 0, xtnsize);
		}
	}

	return pio;
}

void qse_pio_close (qse_pio_t* pio)
{
	qse_pio_fini (pio);
	QSE_MMGR_FREE (pio->mmgr, pio);
}

#if !defined(_WIN32) && !defined(__OS2__) && !defined(__DOS__)
struct param_t
{
	qse_mchar_t* mcmd;
	qse_mchar_t* fixed_argv[4];
	qse_mchar_t** argv;

#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing extra */
#else
	qse_mchar_t fixed_mbuf[64];
#endif
};
typedef struct param_t param_t;

static void free_param (qse_pio_t* pio, param_t* param)
{
	if (param->argv && param->argv != param->fixed_argv) 
		QSE_MMGR_FREE (pio->mmgr, param->argv);
	if (param->mcmd) QSE_MMGR_FREE (pio->mmgr, param->mcmd);
}

static int make_param (
	qse_pio_t* pio, const qse_char_t* cmd, int flags, param_t* param)
{
#if defined(QSE_CHAR_IS_MCHAR)
	qse_mchar_t* mcmd = QSE_NULL;
#else
	qse_mchar_t* mcmd = QSE_NULL;
	qse_char_t* wcmd = QSE_NULL;
#endif
	int fcnt = 0;

	QSE_MEMSET (param, 0, QSE_SIZEOF(*param));

#if defined(QSE_CHAR_IS_MCHAR)
	if (flags & QSE_PIO_SHELL) mcmd = (qse_char_t*)cmd;
	else
	{
		mcmd = qse_strdup (cmd, pio->mmgr);
		if (mcmd == QSE_NULL) 
		{
			pio->errnum = QSE_PIO_ENOMEM;
			goto oops;
		}

		fcnt = qse_strspl (mcmd, QSE_T(""), 
			QSE_T('\"'), QSE_T('\"'), QSE_T('\\')); 
		if (fcnt <= 0) 
		{
			/* no field or an error */
			pio->errnum = QSE_PIO_EINVAL;
			goto oops; 
		}
	}
#else	
	if (flags & QSE_PIO_MBSCMD) 
	{
		/* the cmd is flagged to be of qse_mchar_t 
		 * while the default character type is qse_wchar_t. */

		if (flags & QSE_PIO_SHELL) mcmd = (qse_mchar_t*)cmd;
		else
		{
			mcmd = qse_mbsdup ((const qse_mchar_t*)cmd, pio->mmgr);
			if (mcmd == QSE_NULL) 
			{
				pio->errnum = QSE_PIO_ENOMEM;
				goto oops;
			}

			fcnt = qse_mbsspl (mcmd, QSE_MT(""), 
				QSE_MT('\"'), QSE_MT('\"'), QSE_MT('\\')); 
			if (fcnt <= 0) 
			{
				/* no field or an error */
				pio->errnum = QSE_PIO_EINVAL;
				goto oops; 
			}
		}
	}
	else
	{
		qse_size_t n, mn, wl;

		if (flags & QSE_PIO_SHELL)
		{
			if (qse_wcstombs (cmd, &wl, QSE_NULL, &mn) <= -1)
			{
				/* cmd has illegal sequence */
				pio->errnum = QSE_PIO_EINVAL;
				goto oops;
			}
		}
		else
		{
			wcmd = qse_strdup (cmd, pio->mmgr);
			if (wcmd == QSE_NULL) 
			{
				pio->errnum = QSE_PIO_ENOMEM;
				goto oops;
			}

			fcnt = qse_strspl (wcmd, QSE_T(""), 
				QSE_T('\"'), QSE_T('\"'), QSE_T('\\')); 
			if (fcnt <= 0)
			{
				/* no field or an error */
				pio->errnum = QSE_PIO_EINVAL;
				goto oops;
			}
			
			/* calculate the length of the string after splitting */
			for (wl = 0, n = fcnt; n > 0; )
			{
				if (wcmd[wl++] == QSE_T('\0')) n--;
			}

			if (qse_wcsntombsn (wcmd, &wl, QSE_NULL, &mn) <= -1) 
			{
				pio->errnum = QSE_PIO_EINVAL;
				goto oops;
			}
		}

		/* prepare to reserve 1 more slot for the terminating '\0'
		 * by incrementing mn by 1. */
		mn = mn + 1;

		if (mn <= QSE_COUNTOF(param->fixed_mbuf))
		{
			mcmd = param->fixed_mbuf;
			mn = QSE_COUNTOF(param->fixed_mbuf);
		}
		else
		{
			mcmd = QSE_MMGR_ALLOC (pio->mmgr, mn * QSE_SIZEOF(*mcmd));
			if (mcmd == QSE_NULL) 
			{
				pio->errnum = QSE_PIO_ENOMEM;
				goto oops;
			}
		}

		if (flags & QSE_PIO_SHELL)
		{
			QSE_ASSERT (wcmd == QSE_NULL);
			/* qse_wcstombs() should succeed as 
			 * it was successful above */
			qse_wcstombs (cmd, &wl, mcmd, &mn);
			/* qse_wcstombs() null-terminate mcmd */
		}
		else
		{
			QSE_ASSERT (wcmd != QSE_NULL);
			/* qse_wcsntombsn() should succeed as 
			 * it was was successful above */
			qse_wcsntombsn (wcmd, &wl, mcmd, &mn);
			/* qse_wcsntombsn() doesn't null-terminate mcmd */
			mcmd[mn] = QSE_MT('\0');

			QSE_MMGR_FREE (pio->mmgr, wcmd);
			wcmd = QSE_NULL;
		}
	}
#endif

	if (flags & QSE_PIO_SHELL)
	{
		param->argv = param->fixed_argv;
		param->argv[0] = QSE_MT("/bin/sh");
		param->argv[1] = QSE_MT("-c");
		param->argv[2] = mcmd;
		param->argv[3] = QSE_NULL;
	}
	else
	{
		int i;
		qse_mchar_t** argv;
		qse_mchar_t* mcmdptr;

		param->argv = QSE_MMGR_ALLOC (
			pio->mmgr, (fcnt + 1) * QSE_SIZEOF(argv[0]));
		if (param->argv == QSE_NULL) 
		{
			pio->errnum = QSE_PIO_ENOMEM;
			goto oops;
		}

		mcmdptr = mcmd;
		for (i = 0; i < fcnt; i++)
		{
			param->argv[i] = mcmdptr;
			while (*mcmdptr != QSE_MT('\0')) mcmdptr++;
			mcmdptr++;
		}
		param->argv[i] = QSE_NULL;
	}

#if defined(QSE_CHAR_IS_MCHAR)
	if (mcmd && mcmd != (qse_mchar_t*)cmd) param->mcmd = mcmd;
#else
	if (mcmd && mcmd != (qse_mchar_t*)cmd && 
	    mcmd != param->fixed_mbuf) param->mcmd = mcmd;
#endif
	return 0;

oops:
#if defined(QSE_CHAR_IS_MCHAR)
	if (mcmd && mcmd != cmd) QSE_MMGR_FREE (pio->mmgr, mcmd);
#else
	if (mcmd && mcmd != (qse_mchar_t*)cmd && 
	    mcmd != param->fixed_mbuf) QSE_MMGR_FREE (pio->mmgr, mcmd);
	if (wcmd) QSE_MMGR_FREE (pio->mmgr, wcmd);
#endif
	return -1;
}

static int assert_executable (qse_pio_t* pio, const qse_mchar_t* path)
{
	qse_lstat_t st;

	if (QSE_ACCESS(path, X_OK) <= -1) 
	{
		pio->errnum = syserr_to_errnum (errno);
		return -1;
	}

	/*if (QSE_LSTAT(path, &st) <= -1)*/
	if (QSE_STAT(path, &st) <= -1)
	{
		pio->errnum = syserr_to_errnum (errno);
		return -1;
	}

	if (!S_ISREG(st.st_mode)) 
	{
		pio->errnum = QSE_PIO_EACCES;
		return -1;
	}

	return 0;
}

static QSE_INLINE int is_fd_valid (int fd)
{
	return fcntl (fd, F_GETFD) != -1 || errno != EBADF;
}

static QSE_INLINE int is_fd_valid_and_nocloexec (int fd)
{
	int flags = fcntl (fd, F_GETFD);
	if (flags == -1)
	{
		if (errno == EBADF) return 0; /* invalid. return false */
		return -1; /* unknown. true but negative to indicate unknown */
	}
	return !(flags & FD_CLOEXEC)? 1: 0;
}

static int get_highest_fd (void)
{
	/* TODO: consider if reading from /proc/self/fd is 
	 *       a better idea. */
	struct rlimit rlim;
	int fd = -1;

#if defined(F_MAXFD)
	fd = fcntl (0, F_MAXFD);
#endif
	if (fd == -1)
	{
		if (QSE_GETRLIMIT (RLIMIT_NOFILE, &rlim) <= -1 ||
		    rlim.rlim_max == RLIM_INFINITY) 
		{
		#if defined(HAVE_SYSCONF)
			fd = sysconf (_SC_OPEN_MAX);
		#endif
		}
		else fd = rlim.rlim_max;
	}
	if (fd == -1) fd = 1024; /* fallback */
	return fd;
}

#endif

int qse_pio_init (
	qse_pio_t* pio, qse_mmgr_t* mmgr, const qse_char_t* cmd, 
	qse_env_t* env, int flags)
{
	qse_pio_hnd_t handle[6] = 
	{ 
		QSE_PIO_HND_NIL, 
		QSE_PIO_HND_NIL,
		QSE_PIO_HND_NIL,
		QSE_PIO_HND_NIL,
		QSE_PIO_HND_NIL,
		QSE_PIO_HND_NIL
	};

	qse_tio_t* tio[3] = 
	{ 
		QSE_NULL, 
		QSE_NULL, 
		QSE_NULL 
	};

	int i, minidx = -1, maxidx = -1;

#if defined(_WIN32)
	SECURITY_ATTRIBUTES secattr; 
	PROCESS_INFORMATION procinfo;
	STARTUPINFO startup;
	HANDLE windevnul = INVALID_HANDLE_VALUE;
	BOOL apiret;
	qse_char_t* dupcmd;
	int create_retried;

#elif defined(__OS2__)
	APIRET rc;
	ULONG pipe_size = 4096;
	UCHAR load_error[CCHMAXPATH];
	RESULTCODES child_rc;
	HFILE old_in = QSE_PIO_HND_NIL;
	HFILE old_out = QSE_PIO_HND_NIL;
	HFILE old_err = QSE_PIO_HND_NIL;
	HFILE std_in = 0, std_out = 1, std_err = 2;
	qse_mchar_t* cmd_line = QSE_NULL;
	qse_mchar_t* cmd_file;
	HFILE os2devnul = (HFILE)-1;

#elif defined(__DOS__)

	/* DOS not multi-processed. can't support pio */

#elif defined(HAVE_POSIX_SPAWN) && !(defined(QSE_SYSCALL0) && defined(SYS_vfork))
	posix_spawn_file_actions_t fa;
	int fa_inited = 0;
	int pserr;
	posix_spawnattr_t psattr;
	qse_pio_pid_t pid;
	param_t param;
	extern char** environ;
#elif defined(QSE_SYSCALL0) && defined(SYS_vfork)
	qse_pio_pid_t pid;
	param_t param;
	extern char** environ;
	char** envarr;
	int highest_fd;
	int dummy;
#else
	qse_pio_pid_t pid;
	param_t param;
	extern char** environ;
#endif

	QSE_MEMSET (pio, 0, QSE_SIZEOF(*pio));
	pio->mmgr = mmgr;
	pio->flags = flags;

#if defined(_WIN32)
	/* http://msdn.microsoft.com/en-us/library/ms682499(VS.85).aspx */

	secattr.nLength = QSE_SIZEOF(secattr);
	secattr.bInheritHandle = TRUE;
	secattr.lpSecurityDescriptor = QSE_NULL;

	if (flags & QSE_PIO_WRITEIN)
	{
		/* child reads, parent writes */
		if (CreatePipe (&handle[0], &handle[1], &secattr, 0) == FALSE) 
		{
			pio->errnum = syserr_to_errnum (GetLastError());
			goto oops;
		}

		/* don't inherit write handle */
		if (SetHandleInformation (handle[1], HANDLE_FLAG_INHERIT, 0) == FALSE) 
		{
			DWORD e = GetLastError();
			if (e != ERROR_CALL_NOT_IMPLEMENTED)
			{
				/* SetHandleInformation() is not implemented on win9x.
				 * so let's care only if it is implemented */
				pio->errnum = syserr_to_errnum (e);
				goto oops;
			}
		}

		minidx = 0; maxidx = 1;
	}

	if (flags & QSE_PIO_READOUT)
	{
		/* child writes, parent reads */
		if (CreatePipe (&handle[2], &handle[3], &secattr, 0) == FALSE) 
		{
			pio->errnum = syserr_to_errnum (GetLastError());
			goto oops;
		}

		/* don't inherit read handle */
		if (SetHandleInformation (handle[2], HANDLE_FLAG_INHERIT, 0) == FALSE) 
		{
			DWORD e = GetLastError();
			if (e != ERROR_CALL_NOT_IMPLEMENTED)
			{
				/* SetHandleInformation() is not implemented on win9x.
				 * so let's care only if it is implemented */
				pio->errnum = syserr_to_errnum (e);
				goto oops;
			}
		}

		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (flags & QSE_PIO_READERR)
	{
		/* child writes, parent reads */
		if (CreatePipe (&handle[4], &handle[5], &secattr, 0) == FALSE)
		{
			pio->errnum = syserr_to_errnum (GetLastError());
			goto oops;
		}

		/* don't inherit read handle */
		if (SetHandleInformation (handle[4], HANDLE_FLAG_INHERIT, 0) == FALSE)
		{
			DWORD e = GetLastError();
			if (e != ERROR_CALL_NOT_IMPLEMENTED)
			{
				/* SetHandleInformation() is not implemented on win9x.
				 * so let's care only if it is implemented */
				pio->errnum = syserr_to_errnum (e);
				goto oops;
			}
		}

		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		goto oops;
	}

	if ((flags & QSE_PIO_INTONUL) || 
	    (flags & QSE_PIO_OUTTONUL) ||
	    (flags & QSE_PIO_ERRTONUL))
	{
		windevnul = CreateFile(
			QSE_T("NUL"), GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			&secattr, OPEN_EXISTING, 0, NULL
		);
		if (windevnul == INVALID_HANDLE_VALUE) 
		{
			pio->errnum = syserr_to_errnum (GetLastError());
			goto oops;
		}
	}

	QSE_MEMSET (&procinfo, 0, QSE_SIZEOF(procinfo));
	QSE_MEMSET (&startup, 0, QSE_SIZEOF(startup));

	startup.cb = QSE_SIZEOF(startup);

	/*
	startup.hStdInput = INVALID_HANDLE_VALUE;
	startup.hStdOutput = INVALID_HANDLE_VALUE;
	startup.hStdOutput = INVALID_HANDLE_VALUE;
	*/

	startup.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
	startup.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
	startup.hStdOutput = GetStdHandle (STD_ERROR_HANDLE);
	if (startup.hStdInput == INVALID_HANDLE_VALUE ||
	    startup.hStdOutput == INVALID_HANDLE_VALUE ||
	    startup.hStdError == INVALID_HANDLE_VALUE) 
	{
		pio->errnum = syserr_to_errnum (GetLastError());
		goto oops;
	}

	if (flags & QSE_PIO_WRITEIN) 
	{
		startup.hStdInput = handle[0];
	}

	if (flags & QSE_PIO_READOUT)
	{
		startup.hStdOutput = handle[3];
		if (flags & QSE_PIO_ERRTOOUT) startup.hStdError = handle[3];
	}

	if (flags & QSE_PIO_READERR)
	{
		startup.hStdError = handle[5];
		if (flags & QSE_PIO_OUTTOERR) startup.hStdOutput = handle[5];
	}

	if (flags & QSE_PIO_INTONUL) startup.hStdInput = windevnul;
	if (flags & QSE_PIO_OUTTONUL) startup.hStdOutput = windevnul;
	if (flags & QSE_PIO_ERRTONUL) startup.hStdError = windevnul;

	if (flags & QSE_PIO_DROPIN) startup.hStdInput = INVALID_HANDLE_VALUE;
	if (flags & QSE_PIO_DROPOUT) startup.hStdOutput = INVALID_HANDLE_VALUE;
	if (flags & QSE_PIO_DROPERR) startup.hStdError = INVALID_HANDLE_VALUE;

	startup.dwFlags |= STARTF_USESTDHANDLES;

	/* there is nothing to do for QSE_PIO_SHELL as CreateProcess
	 * takes the entire command line */

	create_retried = 0;

create_process:	
	if (flags & QSE_PIO_SHELL) 
	{
		static const qse_char_t* cmdname[] =
		{
			QSE_T("cmd.exe /c "),
			QSE_T("command.com /c ")
		};
		static const qse_mchar_t* mcmdname[] =
		{
			QSE_MT("cmd.exe /c "),
			QSE_MT("command.com /c ")
		};

	#if defined(QSE_CHAR_IS_WCHAR)
		if (flags & QSE_PIO_MBSCMD)
		{
			const qse_mchar_t* x[3];
			x[0] = mcmdname[create_retried];
			x[1] = (const qse_mchar_t*)cmd;
			x[2] = QSE_NULL;
			dupcmd = qse_mbsatowcsdup (x, QSE_NULL, mmgr);
		}
		else
		{
	#endif
			dupcmd = qse_strdup2 (cmdname[create_retried], cmd, mmgr);
	#if defined(QSE_CHAR_IS_WCHAR)
		}
	#endif
	}
	else 
	{
	#if defined(QSE_CHAR_IS_WCHAR)
		if (flags & QSE_PIO_MBSCMD)
		{
			dupcmd = qse_mbstowcsdup ((const qse_mchar_t*)cmd, QSE_NULL, mmgr);
		}
		else
		{
	#endif
			/* CreateProcess requires command buffer to be read-write. */
			dupcmd = qse_strdup (cmd, mmgr);
	#if defined(QSE_CHAR_IS_WCHAR)
		}
	#endif
	}

	if (dupcmd == QSE_NULL)
	{
		pio->errnum = QSE_PIO_ENOMEM;
		goto oops;
	}

	apiret = CreateProcess (
		QSE_NULL,  /* LPCTSTR lpApplicationName */
		dupcmd,    /* LPTSTR lpCommandLine */
		QSE_NULL,  /* LPSECURITY_ATTRIBUTES lpProcessAttributes */
		QSE_NULL,  /* LPSECURITY_ATTRIBUTES lpThreadAttributes */
		TRUE,      /* BOOL bInheritHandles */
	#ifdef QSE_CHAR_IS_MCHAR
		0,         /* DWORD dwCreationFlags */
	#else
		CREATE_UNICODE_ENVIRONMENT, /* DWORD dwCreationFlags */
	#endif
		(env? qse_env_getstr(env): QSE_NULL), /* LPVOID lpEnvironment */
		QSE_NULL, /* LPCTSTR lpCurrentDirectory */
		&startup, /* LPSTARTUPINFO lpStartupInfo */
		&procinfo /* LPPROCESS_INFORMATION lpProcessInformation */
	);

	QSE_MMGR_FREE (mmgr, dupcmd); 
	if (apiret == FALSE) 
	{
		DWORD e = GetLastError();
		if (create_retried == 0 && (flags & QSE_PIO_SHELL) && 
		    e == ERROR_FILE_NOT_FOUND)
		{
			/* if it failed to exeucte cmd.exe, 
			 * attempt to execute command.com.
			 * this is provision for old windows platforms */
			create_retried = 1;
			goto create_process;
		}

		pio->errnum = syserr_to_errnum (e);
		goto oops;
	}

	if (windevnul != INVALID_HANDLE_VALUE)
	{
		CloseHandle (windevnul); 
		windevnul = INVALID_HANDLE_VALUE;
	}

	if (flags & QSE_PIO_WRITEIN)
	{
		CloseHandle (handle[0]);
		handle[0] = QSE_PIO_HND_NIL;
	}
	if (flags & QSE_PIO_READOUT)
	{
		CloseHandle (handle[3]);
		handle[3] = QSE_PIO_HND_NIL;
	}
	if (flags & QSE_PIO_READERR)
	{
		CloseHandle (handle[5]);
		handle[5] = QSE_PIO_HND_NIL;
	}

	CloseHandle (procinfo.hThread);
	pio->child = procinfo.hProcess;

#elif defined(__OS2__)

	#define DOS_DUP_HANDLE(x,y) QSE_BLOCK ( \
		rc = DosDupHandle(x,y); \
		if (rc != NO_ERROR) \
		{ \
			pio->errnum = syserr_to_errnum (rc); \
			goto oops; \
		} \
	)

	if (flags & QSE_PIO_WRITEIN)
	{
		/* child reads, parent writes */		
		rc = DosCreatePipe (&handle[0], &handle[1], pipe_size);
		if (rc != NO_ERROR) 
		{
			pio->errnum = syserr_to_errnum (rc);
			goto oops;
		}

		/* the parent writes to handle[1] and the child reads from 
		 * handle[0] inherited. set the flag not to inherit handle[1]. */
		rc = DosSetFHState (handle[1], OPEN_FLAGS_NOINHERIT);
		if (rc != NO_ERROR) 
		{
			pio->errnum = syserr_to_errnum (rc);
			goto oops;
		}

		/* Need to do somthing like this to set the flag instead? 
		ULONG state;               
		DosQueryFHState (handle[1], &state);
		DosSetFHState (handle[1], state | OPEN_FLAGS_NOINHERIT); */

		minidx = 0; maxidx = 1;
	}

	if (flags & QSE_PIO_READOUT)
	{
		/* child writes, parent reads */
		rc = DosCreatePipe (&handle[2], &handle[3], pipe_size);
		if (rc != NO_ERROR) 
		{
			pio->errnum = syserr_to_errnum (rc);
			goto oops;
		}

		/* the parent reads from handle[2] and the child writes to 
		 * handle[3] inherited. set the flag not to inherit handle[2] */
		rc = DosSetFHState (handle[2], OPEN_FLAGS_NOINHERIT);
		if (rc != NO_ERROR) 
		{
			pio->errnum = syserr_to_errnum (rc);
			goto oops;
		}

		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (flags & QSE_PIO_READERR)
	{
		/* child writes, parent reads */
		rc = DosCreatePipe (&handle[4], &handle[5], pipe_size);
		if (rc != NO_ERROR) 
		{
			pio->errnum = syserr_to_errnum (rc);
			goto oops;
		}

		/* the parent reads from handle[4] and the child writes to 
		 * handle[5] inherited. set the flag not to inherit handle[4] */
		rc = DosSetFHState (handle[4], OPEN_FLAGS_NOINHERIT);
		if (rc != NO_ERROR) 
		{
			pio->errnum = syserr_to_errnum (rc);
			goto oops;
		}

		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		goto oops;
	}

	if ((flags & QSE_PIO_INTONUL) || 
	    (flags & QSE_PIO_OUTTONUL) ||
	    (flags & QSE_PIO_ERRTONUL))
	{
		ULONG action_taken;
		LONGLONG zero;

		zero.ulLo = 0;
		zero.ulHi = 0;

		rc = DosOpenL (
			QSE_MT("NUL"),
			&os2devnul,
			&action_taken,
			zero,
			FILE_NORMAL,		
			OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
			OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE,
			0L
		);
		if (rc != NO_ERROR) 
		{
			pio->errnum = syserr_to_errnum (rc);
			goto oops;
		}
	}

	/* duplicate the current stdin/out/err to old_in/out/err as a new handle */
	
	rc = DosDupHandle (std_in, &old_in);
	if (rc != NO_ERROR) 
	{
		pio->errnum = syserr_to_errnum (rc);
		goto oops;
	}
	rc = DosDupHandle (std_out, &old_out);
	if (rc != NO_ERROR) 
	{
		pio->errnum = syserr_to_errnum (rc);
		DosClose (old_in); old_in = QSE_PIO_HND_NIL;
		goto oops;
	}
	rc = DosDupHandle (std_err, &old_err);
	if (rc != NO_ERROR) 
	{
		pio->errnum = syserr_to_errnum (rc);
		DosClose (old_out); old_out = QSE_PIO_HND_NIL;
		DosClose (old_in); old_in = QSE_PIO_HND_NIL;
		goto oops;
	}    	

	/* we must not let our own stdin/out/err duplicated 
	 * into old_in/out/err be inherited */
	DosSetFHState (old_in, OPEN_FLAGS_NOINHERIT);
	DosSetFHState (old_out, OPEN_FLAGS_NOINHERIT); 
	DosSetFHState (old_err, OPEN_FLAGS_NOINHERIT);

	if (flags & QSE_PIO_WRITEIN)
	{
		/* the child reads from handle[0] inherited and expects it to
		 * be stdin(0). so we duplicate handle[0] to stdin */
		DOS_DUP_HANDLE (handle[0], &std_in);

		/* the parent writes to handle[1] but does not read from handle[0].
		 * so we close it */
		DosClose (handle[0]); handle[0] = QSE_PIO_HND_NIL;
	}

	if (flags & QSE_PIO_READOUT)
	{
		/* the child writes to handle[3] inherited and expects it to
		 * be stdout(1). so we duplicate handle[3] to stdout. */
		DOS_DUP_HANDLE (handle[3], &std_out);
		if (flags & QSE_PIO_ERRTOOUT) DOS_DUP_HANDLE (handle[3], &std_err);
		/* the parent reads from handle[2] but does not write to handle[3].
		 * so we close it */
		DosClose (handle[3]); handle[3] = QSE_PIO_HND_NIL;
	}

	if (flags & QSE_PIO_READERR)
	{
		DOS_DUP_HANDLE (handle[5], &std_err);
		if (flags & QSE_PIO_OUTTOERR) DOS_DUP_HANDLE (handle[5], &std_out);
		DosClose (handle[5]); handle[5] = QSE_PIO_HND_NIL;
	}

	if (flags & QSE_PIO_INTONUL) DOS_DUP_HANDLE (os2devnul, &std_in);
	if (flags & QSE_PIO_OUTTONUL) DOS_DUP_HANDLE (os2devnul, &std_out);
	if (flags & QSE_PIO_ERRTONUL) DOS_DUP_HANDLE (os2devnul, &std_err);

	if (os2devnul != QSE_PIO_HND_NIL)
	{
	    	/* close NUL early as we've duplicated it already */
		DosClose (os2devnul); 
		os2devnul = QSE_PIO_HND_NIL;
	}
	
	/* at this moment, stdin/out/err are already redirected to pipes
	 * if proper flags have been set. we close them selectively if 
	 * dropping is requested */
	if (flags & QSE_PIO_DROPIN) DosClose (std_in);
	if (flags & QSE_PIO_DROPOUT) DosClose (std_out);
	if (flags & QSE_PIO_DROPERR) DosClose (std_err);

	if (flags & QSE_PIO_SHELL) 
	{
		qse_size_t n, mn;

	#ifdef QSE_CHAR_IS_MCHAR
		mn = qse_strlen(cmd);
	#else
		if (flags & QSE_PIO_MBSCMD)
		{
			mn = qse_mbslen((const qse_mchar_t*)cmd);
		}
		else
		{
			if (qse_wcstombs (cmd, &n, QSE_NULL, &mn) <= -1)
			{
				pio->errnum = QSE_PIO_EINVAL;
				goto oops; /* illegal sequence found */
			}
		}
	#endif
		cmd_line = QSE_MMGR_ALLOC (
			mmgr, ((11+mn+1+1) * QSE_SIZEOF(*cmd_line)));
		if (cmd_line == QSE_NULL) 
		{		
			pio->errnum = QSE_PIO_ENOMEM;
			goto oops;
		}

		qse_mbscpy (cmd_line, QSE_MT("cmd.exe")); /* cmd.exe\0/c */ 
		qse_mbscpy (&cmd_line[8], QSE_MT("/c "));
	#ifdef QSE_CHAR_IS_MCHAR
		qse_mbscpy (&cmd_line[11], cmd);
	#else
		if (flags & QSE_PIO_MBSCMD)
		{
			qse_mbscpy (&cmd_line[11], (const qse_mchar_t*)cmd);
		}
		else
		{
			mn = mn + 1; /* update the buffer size */
			qse_wcstombs (cmd, &n, &cmd_line[11], &mn);
		}
	#endif
		cmd_line[11+mn+1] = QSE_MT('\0'); /* additional \0 after \0 */    
		
		cmd_file = QSE_MT("cmd.exe");
	}
	else
	{
		qse_mchar_t* mptr;
		qse_size_t mn;

	#ifdef QSE_CHAR_IS_MCHAR
		mn = qse_strlen(cmd);
		cmd_line = qse_strdup2 (cmd, QSE_T(" "), pio->mmgr);
		if (cmd_line == QSE_NULL) 
		{
			pio->errnum = QSE_PIO_ENOMEM;
			goto oops;
		}
	#else   
		if (flags & QSE_PIO_MBSCMD)
		{
			mn = qse_mbslen((const qse_mchar_t*)cmd);
			cmd_line = qse_mbsdup2 ((const qse_mchar_t*)cmd, QSE_MT(" "), pio->mmgr);
			if (cmd_line == QSE_NULL) 
			{
				pio->errnum = QSE_PIO_ENOMEM;
				goto oops;
			}
		}
		else
		{
			qse_size_t n;

			if (qse_wcstombs (cmd, &n, QSE_NULL, &mn) <= -1)
			{
				pio->errnum = QSE_PIO_EINVAL;
				goto oops; /* illegal sequence in cmd */
			}
	
			mn = mn + 1;
			cmd_line = QSE_MMGR_ALLOC (pio->mmgr, mn * QSE_SIZEOF(qse_char_t));
			if (cmd_line == QSE_NULL) 
			{
				pio->errnum = QSE_PIO_ENOMEM;
				goto oops;
			}
  
			qse_wcstombs (cmd, &n, cmd_line, &mn);
		}
	#endif

		/* TODO: enhance this part by:
		 *          supporting file names containing whitespaces.
		 *          detecting the end of the file name better.
		 *          doing better parsing of the command line.
		 */

		/* NOTE: you must separate the command name and the parameters 
		 *       with a space. "pstat.exe /c" is ok while "pstat.exe/c"
		 *       is not. */
		mptr = qse_mbspbrk (cmd_line, QSE_MT(" \t"));
		if (mptr) *mptr = QSE_MT('\0');
		cmd_line[mn+1] = QSE_MT('\0'); /* the second '\0' at the end */
		cmd_file = cmd_line;
	}

	/* execute the command line */
	rc = DosExecPgm (
		&load_error,
		QSE_SIZEOF(load_error), 
		EXEC_ASYNCRESULT,
		cmd_line,
		(env? qse_env_getstr(env): QSE_NULL),
		&child_rc,
		cmd_file
	);

	QSE_MMGR_FREE (mmgr, cmd_line);
	cmd_line = QSE_NULL;

	/* Once execution is completed regardless of success or failure,
	 * Restore stdin/out/err using handles duplicated into old_in/out/err */
	DosDupHandle (old_in, &std_in); /* I can't do much if this fails */
	DosClose (old_in); old_in = QSE_PIO_HND_NIL;
	DosDupHandle (old_out, &std_out);
	DosClose (old_out); old_out = QSE_PIO_HND_NIL;
	DosDupHandle (old_err, &std_err);
	DosClose (old_err); old_err = QSE_PIO_HND_NIL;

	if (rc != NO_ERROR) 
	{
		pio->errnum = syserr_to_errnum (rc);
		goto oops;
	}
	pio->child = child_rc.codeTerminate;

#elif defined(__DOS__)
		
	/* DOS not multi-processed. can't support pio */
	pio->errnum = QSE_PIO_ENOIMPL;
	return -1;

#elif defined(HAVE_POSIX_SPAWN) && !(defined(QSE_SYSCALL0) && defined(SYS_vfork))
	if (flags & QSE_PIO_WRITEIN)
	{
		if (QSE_PIPE(&handle[0]) <= -1) 
		{
			pio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
		minidx = 0; maxidx = 1;
	}

	if (flags & QSE_PIO_READOUT)
	{
		if (QSE_PIPE(&handle[2]) <= -1) 
		{
			pio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (flags & QSE_PIO_READERR)
	{
		if (QSE_PIPE(&handle[4]) <= -1) 
		{
			pio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		goto oops;
	}

	if ((pserr = posix_spawn_file_actions_init (&fa)) != 0) 
	{
		pio->errnum = syserr_to_errnum (pserr);
		goto oops;
	}
	fa_inited = 1;

	if (flags & QSE_PIO_WRITEIN)
	{
		/* child should read */
		if ((pserr = posix_spawn_file_actions_addclose (&fa, handle[1])) != 0) 
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((pserr = posix_spawn_file_actions_adddup2 (&fa, handle[0], 0)) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((pserr = posix_spawn_file_actions_addclose (&fa, handle[0])) != 0) 
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
	}

	if (flags & QSE_PIO_READOUT)
	{
		/* child should write */
		if ((pserr = posix_spawn_file_actions_addclose (&fa, handle[2])) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((pserr = posix_spawn_file_actions_adddup2 (&fa, handle[3], 1)) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((flags & QSE_PIO_ERRTOOUT) &&
		    (pserr = posix_spawn_file_actions_adddup2 (&fa, handle[3], 2)) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((pserr = posix_spawn_file_actions_addclose (&fa, handle[3])) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
	}

	if (flags & QSE_PIO_READERR)
	{
		/* child should write */
		if ((pserr = posix_spawn_file_actions_addclose (&fa, handle[4])) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((pserr = posix_spawn_file_actions_adddup2 (&fa, handle[5], 2)) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((flags & QSE_PIO_OUTTOERR) &&
		    (pserr = posix_spawn_file_actions_adddup2 (&fa, handle[5], 1)) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((pserr = posix_spawn_file_actions_addclose (&fa, handle[5])) != 0) 
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
	}

	{
		int oflags = O_RDWR;
	#if defined(O_LARGEFILE)
		oflags |= O_LARGEFILE;
	#endif

		if ((flags & QSE_PIO_INTONUL) &&
		    (pserr = posix_spawn_file_actions_addopen (&fa, 0, QSE_MT("/dev/null"), oflags, 0)) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((flags & QSE_PIO_OUTTONUL) &&
		    (pserr = posix_spawn_file_actions_addopen (&fa, 1, QSE_MT("/dev/null"), oflags, 0)) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
		if ((flags & QSE_PIO_ERRTONUL) &&
		    (pserr = posix_spawn_file_actions_addopen (&fa, 2, QSE_MT("/dev/null"), oflags, 0)) != 0)
		{
			pio->errnum = syserr_to_errnum (pserr);
			goto oops;
		}
	}

	/* there remains the chance of race condition that
	 * 0, 1, 2 can be closed between addclose() and posix_spawn().
	 * so checking the file descriptors with is_fd_valid() is
	 * just on the best-effort basis.
	 */
	if ((flags & QSE_PIO_DROPIN) && is_fd_valid(0) &&
	    (pserr = posix_spawn_file_actions_addclose (&fa, 0)) != 0) 
	{
		pio->errnum = syserr_to_errnum (pserr);
		goto oops;
	}
	if ((flags & QSE_PIO_DROPOUT) && is_fd_valid(1) &&
	    (pserr = posix_spawn_file_actions_addclose (&fa, 1)) != 0) 
	{
		pio->errnum = syserr_to_errnum (pserr);
		goto oops;
	}
	if ((flags & QSE_PIO_DROPERR) && is_fd_valid(2) &&
	    (pserr = posix_spawn_file_actions_addclose (&fa, 2)) != 0)
	{
		pio->errnum = syserr_to_errnum (pserr);
		goto oops;
	}

	if (!(flags & QSE_PIO_NOCLOEXEC))
	{
		int fd = get_highest_fd ();
		while (--fd > 2)
		{
			if (fd == handle[0] || fd == handle[1] ||
			    fd == handle[2] || fd == handle[3] ||
			    fd == handle[4] || fd == handle[5]) continue;

			/* closing attempt on a best-effort basis.
			 * posix_spawn() fails if the file descriptor added
			 * with addclose() is closed before posix_spawn().
			 * addclose() if no FD_CLOEXEC is set or it's unknown. */
			if (is_fd_valid_and_nocloexec(fd) && 
			    (pserr = posix_spawn_file_actions_addclose (&fa, fd)) != 0) 
			{
				pio->errnum = syserr_to_errnum (pserr);
				goto oops;
			}
		}
	}

	if (make_param (pio, cmd, flags, &param) <= -1) goto oops;

	/* check if the command(the command requested or /bin/sh) is 
	 * exectuable to return an error without trying to execute it
	 * though this check alone isn't sufficient */
	if (assert_executable (pio, param.argv[0]) <= -1)
	{
		free_param (pio, &param); 
		goto oops;
	}

	posix_spawnattr_init (&psattr);
#if defined(__linux)
#if !defined(POSIX_SPAWN_USEVFORK)
#	define POSIX_SPAWN_USEVFORK 0x40
#endif
	posix_spawnattr_setflags (&psattr, POSIX_SPAWN_USEVFORK);
#endif

	pserr = posix_spawn(
		&pid, param.argv[0], &fa, &psattr, param.argv,
		(env? qse_env_getarr(env): environ));

#if defined(__linux)
	posix_spawnattr_destroy (&psattr);
#endif

	free_param (pio, &param); 
	if (fa_inited) 
	{
		posix_spawn_file_actions_destroy (&fa);
		fa_inited = 0;
	}
	if (pserr != 0) 
	{
		pio->errnum = syserr_to_errnum (pserr);
		goto oops;
	}

	pio->child = pid;
	if (flags & QSE_PIO_WRITEIN)
	{
		QSE_CLOSE (handle[0]);
		handle[0] = QSE_PIO_HND_NIL;
	}
	if (flags & QSE_PIO_READOUT)
	{
		QSE_CLOSE (handle[3]);
		handle[3] = QSE_PIO_HND_NIL;
	}
	if (flags & QSE_PIO_READERR)
	{
		QSE_CLOSE (handle[5]);
		handle[5] = QSE_PIO_HND_NIL;
	}

#elif defined(QSE_SYSCALL0) && defined(SYS_vfork)
	if (flags & QSE_PIO_WRITEIN)
	{
		if (QSE_PIPE(&handle[0]) <= -1) 
		{
			pio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
		minidx = 0; maxidx = 1;
	}

	if (flags & QSE_PIO_READOUT)
	{
		if (QSE_PIPE(&handle[2]) <= -1) 
		{
			pio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (flags & QSE_PIO_READERR)
	{
		if (QSE_PIPE(&handle[4]) <= -1) 
		{
			pio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		goto oops;
	}

	if (make_param (pio, cmd, flags, &param) <= -1) goto oops;

	/* check if the command(the command requested or /bin/sh) is 
	 * exectuable to return an error without trying to execute it
	 * though this check alone isn't sufficient */
	if (assert_executable (pio, param.argv[0]) <= -1)
	{
		free_param (pio, &param); 
		goto oops;
	}

	/* prepare some data before vforking for vfork limitation.
	 * the child in vfork should not make function calls or 
	 * change data shared with the parent. */
	if (!(flags & QSE_PIO_NOCLOEXEC)) 
		highest_fd = get_highest_fd ();
	envarr = env? qse_env_getarr(env): environ;

	QSE_SYSCALL0 (pid, SYS_vfork);
	if (pid <= -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		free_param (pio, &param);
		goto oops;
	}

	if (pid == 0)
	{
		/* the child after vfork should not make function calls.
		 * since the system call like close() are also normal
		 * functions, i have to use assembly macros to make
		 * system calls. */

		qse_pio_hnd_t devnull = -1;

		if (!(flags & QSE_PIO_NOCLOEXEC))
		{
			int fd = highest_fd;

			/* close all other unknown open handles except 
			 * stdin/out/err and the pipes. */
			while (--fd > 2)
			{
				if (fd != handle[0] && fd != handle[1] &&
				    fd != handle[2] && fd != handle[3] &&
				    fd != handle[4] && fd != handle[5]) 
				{
					QSE_SYSCALL1 (dummy, SYS_close, fd);
				}
			}
		}

		if (flags & QSE_PIO_WRITEIN)
		{
			/* child should read */
			QSE_SYSCALL1 (dummy, SYS_close, handle[1]);
			QSE_SYSCALL2 (dummy, SYS_dup2, handle[0], 0);
			if (dummy <= -1) goto child_oops;
			QSE_SYSCALL1 (dummy, SYS_close, handle[0]);
		}

		if (flags & QSE_PIO_READOUT)
		{
			/* child should write */
			QSE_SYSCALL1 (dummy, SYS_close, handle[2]);
			QSE_SYSCALL2 (dummy, SYS_dup2, handle[3], 1);
			if (dummy <= -1) goto child_oops;

			if (flags & QSE_PIO_ERRTOOUT)
			{
				QSE_SYSCALL2 (dummy, SYS_dup2, handle[3], 2);
				if (dummy <= -1) goto child_oops;
			}

			QSE_SYSCALL1 (dummy, SYS_close, handle[3]);
		}

		if (flags & QSE_PIO_READERR)
		{
			/* child should write */
			QSE_SYSCALL1 (dummy, SYS_close, handle[4]);
			QSE_SYSCALL2 (dummy, SYS_dup2, handle[5], 2);
			if (dummy <= -1) goto child_oops;

			if (flags & QSE_PIO_OUTTOERR)
			{
				QSE_SYSCALL2 (dummy, SYS_dup2, handle[5], 1);
				if (dummy <= -1) goto child_oops;
			}

			QSE_SYSCALL1 (dummy, SYS_close, handle[5]);
		}

		if ((flags & QSE_PIO_INTONUL) || 
		    (flags & QSE_PIO_OUTTONUL) ||
		    (flags & QSE_PIO_ERRTONUL))
		{
		#if defined(O_LARGEFILE)
			QSE_SYSCALL3 (devnull, SYS_open, QSE_MT("/dev/null"), O_RDWR|O_LARGEFILE, 0);
		#else
			QSE_SYSCALL3 (devnull, SYS_open, QSE_MT("/dev/null"), O_RDWR, 0);
		#endif
			if (devnull <= -1) goto child_oops;
		}

		if (flags & QSE_PIO_INTONUL)
		{
			QSE_SYSCALL2 (dummy, SYS_dup2, devnull, 0);
			if (dummy <= -1) goto child_oops;
		}
		if (flags & QSE_PIO_OUTTONUL)
		{
			QSE_SYSCALL2 (dummy, SYS_dup2, devnull, 1);
			if (dummy <= -1) goto child_oops;
		}
		if (flags & QSE_PIO_ERRTONUL)
		{
			QSE_SYSCALL2 (dummy, SYS_dup2, devnull, 2);
			if (dummy <= -1) goto child_oops;
		}

		if ((flags & QSE_PIO_INTONUL) || 
		    (flags & QSE_PIO_OUTTONUL) ||
		    (flags & QSE_PIO_ERRTONUL)) 
		{
			QSE_SYSCALL1 (dummy, SYS_close, devnull);
			devnull = -1;
		}

		if (flags & QSE_PIO_DROPIN) QSE_SYSCALL1 (dummy, SYS_close, 0);
		if (flags & QSE_PIO_DROPOUT) QSE_SYSCALL1 (dummy, SYS_close, 1);
		if (flags & QSE_PIO_DROPERR) QSE_SYSCALL1 (dummy, SYS_close, 2);

		QSE_SYSCALL3 (dummy, SYS_execve, param.argv[0], param.argv, envarr);
		/*free_param (pio, &param); don't free this in the vfork version */

	child_oops:
		if (devnull >= 0) QSE_SYSCALL1 (dummy, SYS_close, devnull);
		QSE_SYSCALL1 (dummy, SYS_exit, 128);
	}

	/* parent */
	free_param (pio, &param);
	pio->child = pid;

	if (flags & QSE_PIO_WRITEIN)
	{
		QSE_CLOSE (handle[0]);
		handle[0] = QSE_PIO_HND_NIL;
	}

	if (flags & QSE_PIO_READOUT)
	{
		QSE_CLOSE (handle[3]);
		handle[3] = QSE_PIO_HND_NIL;
	}

	if (flags & QSE_PIO_READERR)
	{
		QSE_CLOSE (handle[5]);
		handle[5] = QSE_PIO_HND_NIL;
	}
#else

	if (flags & QSE_PIO_WRITEIN)
	{
		if (QSE_PIPE(&handle[0]) <= -1) 
		{
			pio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
		minidx = 0; maxidx = 1;
	}

	if (flags & QSE_PIO_READOUT)
	{
		if (QSE_PIPE(&handle[2]) <= -1) 
		{
			pio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (flags & QSE_PIO_READERR)
	{
		if (QSE_PIPE(&handle[4]) <= -1) 
		{
			pio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		goto oops;
	}

	if (make_param (pio, cmd, flags, &param) <= -1) goto oops;

	/* check if the command(the command requested or /bin/sh) is 
	 * exectuable to return an error without trying to execute it
	 * though this check alone isn't sufficient */
	if (assert_executable (pio, param.argv[0]) <= -1)
	{
		free_param (pio, &param); 
		goto oops;
	}

	pid = QSE_FORK();
	if (pid <= -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		free_param (pio, &param);
		goto oops;
	}

	if (pid == 0)
	{
		/* child */
		qse_pio_hnd_t devnull = -1;

		if (!(flags & QSE_PIO_NOCLOEXEC))
		{
			int fd = get_highest_fd ();

			/* close all other unknown open handles except 
			 * stdin/out/err and the pipes. */
			while (--fd > 2)
			{
				if (fd != handle[0] && fd != handle[1] &&
				    fd != handle[2] && fd != handle[3] &&
				    fd != handle[4] && fd != handle[5]) 
				{
					QSE_CLOSE (fd);
				}
			}
		}

		if (flags & QSE_PIO_WRITEIN)
		{
			/* child should read */
			QSE_CLOSE (handle[1]);
			handle[1] = QSE_PIO_HND_NIL;
			if (QSE_DUP2 (handle[0], 0) <= -1) goto child_oops;
			QSE_CLOSE (handle[0]);
			handle[0] = QSE_PIO_HND_NIL;
		}

		if (flags & QSE_PIO_READOUT)
		{
			/* child should write */
			QSE_CLOSE (handle[2]);
			handle[2] = QSE_PIO_HND_NIL;
			if (QSE_DUP2 (handle[3], 1) <= -1) goto child_oops;

			if (flags & QSE_PIO_ERRTOOUT)
			{
				if (QSE_DUP2 (handle[3], 2) <= -1) goto child_oops;
			}

			QSE_CLOSE (handle[3]); 
			handle[3] = QSE_PIO_HND_NIL;
		}

		if (flags & QSE_PIO_READERR)
		{
			/* child should write */
			QSE_CLOSE (handle[4]); 
			handle[4] = QSE_PIO_HND_NIL;
			if (QSE_DUP2 (handle[5], 2) <= -1) goto child_oops;

			if (flags & QSE_PIO_OUTTOERR)
			{
				if (QSE_DUP2 (handle[5], 1) <= -1) goto child_oops;
			}

			QSE_CLOSE (handle[5]);
			handle[5] = QSE_PIO_HND_NIL;
		}

		if ((flags & QSE_PIO_INTONUL) || 
		    (flags & QSE_PIO_OUTTONUL) ||
		    (flags & QSE_PIO_ERRTONUL))
		{
		#if defined(O_LARGEFILE)
			devnull = QSE_OPEN (QSE_MT("/dev/null"), O_RDWR|O_LARGEFILE, 0);
		#else
			devnull = QSE_OPEN (QSE_MT("/dev/null"), O_RDWR, 0);
		#endif
			if (devnull <= -1) goto child_oops;
		}

		if ((flags & QSE_PIO_INTONUL)  &&
		    QSE_DUP2(devnull,0) <= -1) goto child_oops;
		if ((flags & QSE_PIO_OUTTONUL) &&
		    QSE_DUP2(devnull,1) <= -1) goto child_oops;
		if ((flags & QSE_PIO_ERRTONUL) &&
		    QSE_DUP2(devnull,2) <= -1) goto child_oops;

		if ((flags & QSE_PIO_INTONUL) || 
		    (flags & QSE_PIO_OUTTONUL) ||
		    (flags & QSE_PIO_ERRTONUL)) 
		{
			QSE_CLOSE (devnull);
			devnull = -1;
		}

		if (flags & QSE_PIO_DROPIN) QSE_CLOSE(0);
		if (flags & QSE_PIO_DROPOUT) QSE_CLOSE(1);
		if (flags & QSE_PIO_DROPERR) QSE_CLOSE(2);

		/*if (make_param (pio, cmd, flags, &param) <= -1) goto child_oops;*/
		QSE_EXECVE (param.argv[0], param.argv, (env? qse_env_getarr(env): environ));
		free_param (pio, &param); 

	child_oops:
		if (devnull >= 0) QSE_CLOSE (devnull);
		QSE_EXIT (128);
	}

	/* parent */
	free_param (pio, &param);
	pio->child = pid;

	if (flags & QSE_PIO_WRITEIN)
	{
		/* 
		 * 012345
		 * rw----
		 * X  
		 * WRITE => 1
		 */
		QSE_CLOSE (handle[0]);
		handle[0] = QSE_PIO_HND_NIL;
	}

	if (flags & QSE_PIO_READOUT)
	{
		/* 
		 * 012345
		 * --rw--
		 *    X
		 * READ => 2
		 */
		QSE_CLOSE (handle[3]);
		handle[3] = QSE_PIO_HND_NIL;
	}

	if (flags & QSE_PIO_READERR)
	{
		/* 
		 * 012345
		 * ----rw
		 *      X   
		 * READ => 4
		 */
		QSE_CLOSE (handle[5]);
		handle[5] = QSE_PIO_HND_NIL;
	}

#endif

	/* store back references */
	pio->pin[QSE_PIO_IN].self = pio;
	pio->pin[QSE_PIO_OUT].self = pio;
	pio->pin[QSE_PIO_ERR].self = pio;

	/* store actual pipe handles */
	pio->pin[QSE_PIO_IN].handle = handle[1];
	pio->pin[QSE_PIO_OUT].handle = handle[2];
	pio->pin[QSE_PIO_ERR].handle = handle[4];

	if (flags & QSE_PIO_TEXT)
	{
		int topt = 0;

		if (flags & QSE_PIO_IGNOREMBWCERR) topt |= QSE_TIO_IGNOREMBWCERR;
		if (flags & QSE_PIO_NOAUTOFLUSH) topt |= QSE_TIO_NOAUTOFLUSH;

		QSE_ASSERT (QSE_COUNTOF(tio) == QSE_COUNTOF(pio->pin));
		for (i = 0; i < QSE_COUNTOF(tio); i++)
		{
			int r;

			tio[i] = qse_tio_open (pio->mmgr, QSE_SIZEOF(&pio->pin[i]), topt);
			if (tio[i] == QSE_NULL) 
			{
				pio->errnum = QSE_PIO_ENOMEM;
				goto oops;
			}
			
			/**(qse_pio_pin_t**)qse_tio_getxtn(tio[i]) = &pio->pin[i]; */
			*(qse_pio_pin_t**)QSE_XTN(tio[i]) = &pio->pin[i];

			r = (i == QSE_PIO_IN)?
				qse_tio_attachout (tio[i], pio_output, QSE_NULL, 4096):
				qse_tio_attachin (tio[i], pio_input, QSE_NULL, 4096);
			if (r <= -1) 
			{
				if (pio->errnum == QSE_PIO_ENOERR) 
					pio->errnum = tio_errnum_to_pio_errnum (tio[i]);
				goto oops;
			}

			pio->pin[i].tio = tio[i];
		}
	}

	return 0;

oops:
	if (pio->errnum == QSE_PIO_ENOERR) pio->errnum = QSE_PIO_ESYSERR;

#if defined(_WIN32)
	if (windevnul != INVALID_HANDLE_VALUE) CloseHandle (windevnul);

#elif defined(__OS2__)
	if (cmd_line) QSE_MMGR_FREE (mmgr, cmd_line);
	if (old_in != QSE_PIO_HND_NIL)
	{
		DosDupHandle (old_in, &std_in);
		DosClose (old_in); 
	}
	if (old_out != QSE_PIO_HND_NIL)
	{
		DosDupHandle (old_out, &std_out);
		DosClose (old_out);
	}
	if (old_err != QSE_PIO_HND_NIL)
	{
		DosDupHandle (old_err, &std_err);
		DosClose (old_err);
	}
	if (os2devnul != QSE_PIO_HND_NIL) DosClose (os2devnul);
#endif

	for (i = 0; i < QSE_COUNTOF(tio); i++) 
	{
		if (tio[i]) qse_tio_close (tio[i]);
	}

#if defined(_WIN32)
	for (i = minidx; i < maxidx; i++) CloseHandle (handle[i]);
#elif defined(__OS2__)
	for (i = minidx; i < maxidx; i++) 
	{
    		if (handle[i] != QSE_PIO_HND_NIL) DosClose (handle[i]);
	}
#elif defined(__DOS__)

	/* DOS not multi-processed. can't support pio */
#elif defined(HAVE_POSIX_SPAWN) && !(defined(QSE_SYSCALL0) && defined(SYS_vfork))
	if (fa_inited) 
	{
		posix_spawn_file_actions_destroy (&fa);
		fa_inited = 0;
	}
	for (i = minidx; i < maxidx; i++) 
	{
    		if (handle[i] != QSE_PIO_HND_NIL) QSE_CLOSE (handle[i]);
	}
#elif defined(QSE_SYSCALL0) && defined(SYS_vfork)
	for (i = minidx; i < maxidx; i++) 
	{
    		if (handle[i] != QSE_PIO_HND_NIL) QSE_CLOSE (handle[i]);
	}
#else
	for (i = minidx; i < maxidx; i++) 
	{
    		if (handle[i] != QSE_PIO_HND_NIL) QSE_CLOSE (handle[i]);
	}
#endif

	return -1;
}

void qse_pio_fini (qse_pio_t* pio)
{
	qse_pio_end (pio, QSE_PIO_ERR);
	qse_pio_end (pio, QSE_PIO_OUT);
	qse_pio_end (pio, QSE_PIO_IN);

	/* when closing, enable blocking and retrying */
	pio->flags &= ~QSE_PIO_WAITNOBLOCK;
	pio->flags &= ~QSE_PIO_WAITNORETRY;
	qse_pio_wait (pio);
}

qse_mmgr_t* qse_pio_getmmgr (qse_pio_t* pio)
{
	return pio->mmgr;
}

void* qse_pio_getxtn (qse_pio_t* pio)
{
	return QSE_XTN (pio);
}

qse_pio_errnum_t qse_pio_geterrnum (const qse_pio_t* pio)
{
	return pio->errnum;
}

qse_cmgr_t* qse_pio_getcmgr (qse_pio_t* pio, qse_pio_hid_t hid)
{
	return pio->pin[hid].tio? 
		qse_tio_getcmgr(pio->pin[hid].tio): QSE_NULL;
}

void qse_pio_setcmgr (qse_pio_t* pio, qse_pio_hid_t hid, qse_cmgr_t* cmgr)
{
	if (pio->pin[hid].tio) qse_tio_setcmgr (pio->pin[hid].tio, cmgr);
}

qse_pio_hnd_t qse_pio_gethandle (const qse_pio_t* pio, qse_pio_hid_t hid)
{
	return pio->pin[hid].handle;
}

qse_ubi_t qse_pio_gethandleasubi (const qse_pio_t* pio, qse_pio_hid_t hid)
{
	qse_ubi_t handle;

#if defined(_WIN32)
	handle.ptr = pio->pin[hid].handle;
#elif defined(__OS2__)
	handle.ul = pio->pin[hid].handle;
#elif defined(__DOS__)
	handle.i = pio->pin[hid].handle;
#else
	handle.i = pio->pin[hid].handle;
#endif

	return handle;
}

qse_pio_pid_t qse_pio_getchild (const qse_pio_t* pio)
{
	return pio->child;
}

static qse_ssize_t pio_read (
	qse_pio_t* pio, void* buf, qse_size_t size, qse_pio_hnd_t hnd)
{
#if defined(_WIN32)
	DWORD count;
#elif defined(__OS2__)
	ULONG count;
	APIRET rc;
#elif defined(__DOS__)
	int n;
#else
	qse_ssize_t n;
#endif

	if (hnd == QSE_PIO_HND_NIL) 
	{
		/* the stream is already closed */
		pio->errnum = QSE_PIO_ENOHND;
		return (qse_ssize_t)-1;
	}

#if defined(_WIN32)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD);

	if (ReadFile(hnd, buf, (DWORD)size, &count, QSE_NULL) == FALSE) 
	{
		/* ReadFile receives ERROR_BROKEN_PIPE when the write end
		 * is closed in the child process */
		if (GetLastError() == ERROR_BROKEN_PIPE) return 0;
		pio->errnum = syserr_to_errnum(GetLastError());
		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__OS2__)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG);

	rc = DosRead (hnd, buf, (ULONG)size, &count);
	if (rc != NO_ERROR)
	{
    		if (rc == ERROR_BROKEN_PIPE) return 0; /* TODO: check this */
		pio->errnum = syserr_to_errnum(rc);
    		return -1;
    	}
	return (qse_ssize_t)count;

#elif defined(__DOS__)
	/* TODO: verify this */

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int);

	n = read (hnd, buf, size);
	if (n <= -1) pio->errnum = syserr_to_errnum(errno);
	return n;

#else

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);

reread:
	n = QSE_READ (hnd, buf, size);
	if (n <= -1) 
	{
		if (errno == EINTR)
		{
			if (pio->flags & QSE_PIO_READNORETRY) 
				pio->errnum = QSE_PIO_EINTR;
			else goto reread;
		}
		else
		{
			pio->errnum = syserr_to_errnum (errno);
		}
	}

	return n;
#endif
}

qse_ssize_t qse_pio_read (
	qse_pio_t* pio, qse_pio_hid_t hid, void* buf, qse_size_t size)
{
	if (pio->pin[hid].tio == QSE_NULL) 
		return pio_read (pio, buf, size, pio->pin[hid].handle);
	else
	{
		qse_ssize_t n;

		pio->errnum = QSE_PIO_ENOERR;
		n = qse_tio_read (pio->pin[hid].tio, buf, size);
		if (n <= -1 && pio->errnum == QSE_PIO_ENOERR) 
			pio->errnum = tio_errnum_to_pio_errnum (pio->pin[hid].tio);

		return n;
	}
}

static qse_ssize_t pio_write (
	qse_pio_t* pio, const void* data, qse_size_t size, qse_pio_hnd_t hnd)
{
#if defined(_WIN32)
	DWORD count;
#elif defined(__OS2__)
	ULONG count;
	APIRET rc;
#elif defined(__DOS__)
	int n;
#else
	qse_ssize_t n;
#endif

	if (hnd == QSE_PIO_HND_NIL) 
	{
		/* the stream is already closed */
		pio->errnum = QSE_PIO_ENOHND;
		return (qse_ssize_t)-1;
	}

#if defined(_WIN32)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD);

	if (WriteFile (hnd, data, (DWORD)size, &count, QSE_NULL) == FALSE)
	{
		pio->errnum = syserr_to_errnum(GetLastError());
		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__OS2__)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG);

	rc = DosWrite (hnd, (PVOID)data, (ULONG)size, &count);
	if (rc != NO_ERROR)
	{
		pio->errnum = syserr_to_errnum(rc);
    		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__DOS__)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int);

	n = write (hnd, data, size);
	if (n <= -1) pio->errnum = syserr_to_errnum (errno);
	return n;

#else

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);

rewrite:
	n = QSE_WRITE (hnd, data, size);
	if (n <= -1) 
	{
		if (errno == EINTR)
		{
			if (pio->flags & QSE_PIO_WRITENORETRY)
				pio->errnum = QSE_PIO_EINTR;
			else goto rewrite;
		}
		else
		{
			pio->errnum = syserr_to_errnum (errno);
		}
	}
	return n;

#endif
}

qse_ssize_t qse_pio_write (
	qse_pio_t* pio, qse_pio_hid_t hid,
	const void* data, qse_size_t size)
{
	if (pio->pin[hid].tio == QSE_NULL)
		return pio_write (pio, data, size, pio->pin[hid].handle);
	else
	{
		qse_ssize_t n;

		pio->errnum = QSE_PIO_ENOERR;	
		n = qse_tio_write (pio->pin[hid].tio, data, size);
		if (n <= -1 && pio->errnum == QSE_PIO_ENOERR) 
			pio->errnum = tio_errnum_to_pio_errnum (pio->pin[hid].tio);

		return n;
	}
}

qse_ssize_t qse_pio_flush (qse_pio_t* pio, qse_pio_hid_t hid)
{
	qse_ssize_t n;

	if (pio->pin[hid].tio == QSE_NULL) return 0;

	pio->errnum = QSE_PIO_ENOERR;	
	n = qse_tio_flush (pio->pin[hid].tio);
	if (n <= -1 && pio->errnum == QSE_PIO_ENOERR) 
		pio->errnum = tio_errnum_to_pio_errnum (pio->pin[hid].tio);

	return n;
}

void qse_pio_purge (qse_pio_t* pio, qse_pio_hid_t hid)
{
	if (pio->pin[hid].tio) qse_tio_purge (pio->pin[hid].tio);
}

void qse_pio_end (qse_pio_t* pio, qse_pio_hid_t hid)
{
	if (pio->pin[hid].tio)
	{
		qse_tio_close (pio->pin[hid].tio);
		pio->pin[hid].tio = QSE_NULL;
	}

	if (pio->pin[hid].handle != QSE_PIO_HND_NIL)
	{
#if defined(_WIN32)
		CloseHandle (pio->pin[hid].handle);
#elif defined(__OS2__)
		DosClose (pio->pin[hid].handle);
#elif defined(__DOS__)
		close (pio->pin[hid].handle);
#else
		QSE_CLOSE (pio->pin[hid].handle);
#endif
		pio->pin[hid].handle = QSE_PIO_HND_NIL;
	}
}

int qse_pio_wait (qse_pio_t* pio)
{
#if defined(_WIN32)

	DWORD ecode, w;

	if (pio->child == QSE_PIO_PID_NIL) 
	{		
		pio->errnum = QSE_PIO_ECHILD;
		return -1;
	}

	w = WaitForSingleObject (pio->child, 
		((pio->flags & QSE_PIO_WAITNOBLOCK)? 0: INFINITE)
	);
	if (w == WAIT_TIMEOUT)
	{
		/* the child process is still alive */
		return 255 + 1;
	}
	if (w != WAIT_OBJECT_0)
	{
		/* WAIT_FAILED, WAIT_ABANDONED */
		pio->errnum = QSE_PIO_ESYSERR;
		return -1;
	}

	QSE_ASSERT (w == WAIT_OBJECT_0);
	
	if (GetExitCodeProcess (pio->child, &ecode) == FALSE) 
	{
		/* close the handle anyway to prevent further 
		 * errors when this function is called again */
		CloseHandle (pio->child); 
		pio->child = QSE_PIO_PID_NIL;

		pio->errnum = QSE_PIO_ESYSERR;
		return -1;
	}

	/* close handle here to emulate waitpid() as much as possible. */
	CloseHandle (pio->child); 
	pio->child = QSE_PIO_PID_NIL;

	if (ecode == STILL_ACTIVE)
	{
		/* this should not happen as the control reaches here
		 * only when WaitforSingleObject() is successful.
		 * if it happends,  close the handle and return an error */
		pio->errnum = QSE_PIO_ESYSERR;
		return -1;
	}

	return ecode;

#elif defined(__OS2__)

	APIRET rc;
	RESULTCODES child_rc;
	PID ppid;

	if (pio->child == QSE_PIO_PID_NIL) 
	{		
		pio->errnum = QSE_PIO_ECHILD;
		return -1;
	}

	rc = DosWaitChild (
		DCWA_PROCESSTREE,
		((pio->flags & QSE_PIO_WAITNOBLOCK)? DCWW_NOWAIT: DCWW_WAIT),
		&child_rc,
		&ppid,
		pio->child
	);
	if (rc == ERROR_CHILD_NOT_COMPLETE)
	{
		/* the child process is still alive */
		return 255 + 1;
	}
	if (rc != NO_ERROR)
	{
		/* WAIT_FAILED, WAIT_ABANDONED */
		pio->errnum = QSE_PIO_ESYSERR;
		return -1;
	}

	/* close handle here to emulate waitpid() as much as possible. */
	/*DosClose (pio->child);*/
	pio->child = QSE_PIO_PID_NIL;

	return (child_rc.codeTerminate == TC_EXIT)? 
		child_rc.codeResult: (255 + 1 + child_rc.codeTerminate);

#elif defined(__DOS__)

	pio->errnum = QSE_PIO_ENOIMPL;
	return -1;

#else

	int opt = 0;
	int ret = -1;

	if (pio->child == QSE_PIO_PID_NIL) 
	{
		pio->errnum = QSE_PIO_ECHILD;
		return -1;
	}

	if (pio->flags & QSE_PIO_WAITNOBLOCK) opt |= WNOHANG;

	while (1)
	{
		int status, n;

		n = QSE_WAITPID (pio->child, &status, opt);
		if (n <= -1)
		{
			if (errno == ECHILD)
			{
				/* most likely, the process has already been 
				 * waitpid()ed on. */
				pio->child = QSE_PIO_PID_NIL;
				pio->errnum = QSE_PIO_ECHILD;
			}
			else if (errno == EINTR)
			{
				if (pio->flags & QSE_PIO_WAITNORETRY) 
					pio->errnum = QSE_PIO_EINTR;
				else continue;
			}
			else pio->errnum = syserr_to_errnum (errno);

			break;
		}

		if (n == 0) 
		{
			/* when WNOHANG is not specified, 0 can't be returned */
			QSE_ASSERT (pio->flags & QSE_PIO_WAITNOBLOCK);

			ret = 255 + 1;
			/* the child process is still alive */
			break;
		}

		if (n == pio->child)
		{
			if (WIFEXITED(status))
			{
				/* the child process ended normally */
				ret = WEXITSTATUS(status);
			}
			else if (WIFSIGNALED(status))
			{
				/* the child process was killed by a signal */
				ret = 255 + 1 + WTERMSIG (status);
			}
			else
			{
				/* not interested in WIFSTOPPED & WIFCONTINUED.
				 * in fact, this else-block should not be reached
				 * as WIFEXITED or WIFSIGNALED must be true.
				 * anyhow, just set the return value to 0. */
				ret = 0;
			}

			pio->child = QSE_PIO_PID_NIL;
			break;
		}
	}

	return ret;
#endif
}

int qse_pio_kill (qse_pio_t* pio)
{
#if defined(_WIN32)
	DWORD n;
#elif defined(__OS2__)
	APIRET rc;
#elif defined(__DOS__)
	/* TODO: implement this */
#else
	int n;
#endif

	if (pio->child == QSE_PIO_PID_NIL) 
	{
		pio->errnum = QSE_PIO_ECHILD;
		return -1;
	}

#if defined(_WIN32)
	/* 9 was chosen below to treat TerminateProcess as kill -KILL. */
	n = TerminateProcess (pio->child, 255 + 1 + 9);
	if (n == FALSE) 
	{
		pio->errnum = QSE_PIO_ESYSERR;
		return -1;
	}
	return 0;

#elif defined(__OS2__)
/*TODO: must use DKP_PROCESS? */
	rc = DosKillProcess (pio->child, DKP_PROCESSTREE);
	if (rc != NO_ERROR)
	{
		pio->errnum = QSE_PIO_ESYSERR;
		return -1;
	}
	return 0;	

#elif defined(__DOS__)

	pio->errnum = QSE_PIO_ENOIMPL;
	return -1;

#else
	n = QSE_KILL (pio->child, SIGKILL);
	if (n <= -1) pio->errnum = QSE_PIO_ESYSERR;
	return n;
#endif
}

static qse_ssize_t pio_input (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size)
{
	if (cmd == QSE_TIO_DATA) 
	{
		/*qse_pio_pin_t* pin = (qse_pio_pin_t*)qse_tio_getxtn(tio);*/
		qse_pio_pin_t* pin = *(qse_pio_pin_t**)QSE_XTN(tio);
		QSE_ASSERT (pin != QSE_NULL);
		QSE_ASSERT (pin->self != QSE_NULL);
		return pio_read (pin->self, buf, size, pin->handle);
	}

	/* take no actions for OPEN and CLOSE as they are handled
	 * by pio */
	return 0;
}

static qse_ssize_t pio_output (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size)
{
	if (cmd == QSE_TIO_DATA) 
	{
		/*qse_pio_pin_t* pin = (qse_pio_pin_t*)qse_tio_getxtn(tio);*/
		qse_pio_pin_t* pin = *(qse_pio_pin_t**)QSE_XTN(tio);
		QSE_ASSERT (pin != QSE_NULL);
		QSE_ASSERT (pin->self != QSE_NULL);
		return pio_write (pin->self, buf, size, pin->handle);
	}

	/* take no actions for OPEN and CLOSE as they are handled
	 * by pio */
	return 0;
}
