/*
 * $Id: pio.c 400 2011-03-16 08:37:06Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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
#include <qse/cmn/str.h>
#include "mem.h"

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSERRORS
#	include <os2.h>
#else
#	include "syscall.h"
#	include <fcntl.h>
#	include <sys/wait.h>
#endif

QSE_IMPLEMENT_COMMON_FUNCTIONS (pio)

static qse_ssize_t pio_input (int cmd, void* arg, void* buf, qse_size_t size);
static qse_ssize_t pio_output (int cmd, void* arg, void* buf, qse_size_t size);

qse_pio_t* qse_pio_open (
	qse_mmgr_t* mmgr, qse_size_t ext, const qse_char_t* path, int oflags)
{
	qse_pio_t* pio;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	pio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_pio_t) + ext);
	if (pio == QSE_NULL) return QSE_NULL;

	if (qse_pio_init (pio, mmgr, path, oflags) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, pio);
		return QSE_NULL;
	}

	return pio;
}

void qse_pio_close (qse_pio_t* pio)
{
	qse_pio_fini (pio);
	QSE_MMGR_FREE (pio->mmgr, pio);
}

qse_pio_t* qse_pio_init (
	qse_pio_t* pio, qse_mmgr_t* mmgr, const qse_char_t* cmd, int oflags)
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
	qse_char_t* dup = QSE_NULL;
	HANDLE windevnul = INVALID_HANDLE_VALUE;
	BOOL x;
#elif defined(__OS2__)
	/* TODO: implmenet this for os/2 */
#else
	qse_pio_pid_t pid;
#endif

	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (pio, 0, QSE_SIZEOF(*pio));
	pio->mmgr = mmgr;

#ifdef _WIN32
	/* http://msdn.microsoft.com/en-us/library/ms682499(VS.85).aspx */

	secattr.nLength = QSE_SIZEOF(secattr);
	secattr.bInheritHandle = TRUE;
	secattr.lpSecurityDescriptor = QSE_NULL;

	if (oflags & QSE_PIO_WRITEIN)
	{
		/* child reads, parent writes */
		if (CreatePipe (
			&handle[0], &handle[1], 
			&secattr, 0) == FALSE) goto oops;

		/* don't inherit write handle */
		if (SetHandleInformation (
			handle[1], HANDLE_FLAG_INHERIT, 0) == FALSE) goto oops;

		minidx = 0; maxidx = 1;
	}

	if (oflags & QSE_PIO_READOUT)
	{
		/* child writes, parent reads */
		if (CreatePipe (
			&handle[2], &handle[3],
			&secattr, 0) == FALSE) goto oops;

		/* don't inherit read handle */
		if (SetHandleInformation (
			handle[2], HANDLE_FLAG_INHERIT, 0) == FALSE) goto oops;

		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (oflags & QSE_PIO_READERR)
	{
		/* child writes, parent reads */
		if (CreatePipe (
			&handle[4], &handle[5],
			&secattr, 0) == FALSE) goto oops;

		/* don't inherit read handle */
		if (SetHandleInformation (
			handle[4], HANDLE_FLAG_INHERIT, 0) == FALSE) goto oops;

		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if ((oflags & QSE_PIO_INTONUL) || 
	    (oflags & QSE_PIO_OUTTONUL) ||
	    (oflags & QSE_PIO_ERRTONUL))
	{
		windevnul = CreateFile(
			QSE_T("NUL"), GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			&secattr, OPEN_EXISTING, 0, NULL
		);
		if (windevnul == INVALID_HANDLE_VALUE) goto oops;
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
	    startup.hStdError == INVALID_HANDLE_VALUE) goto oops;

	if (oflags & QSE_PIO_WRITEIN) startup.hStdInput = handle[0];

	if (oflags & QSE_PIO_READOUT)
	{
		startup.hStdOutput = handle[3];
		if (oflags & QSE_PIO_ERRTOOUT) startup.hStdError = handle[3];
	}

	if (oflags & QSE_PIO_READERR)
	{
		startup.hStdError = handle[5];
		if (oflags & QSE_PIO_OUTTOERR) startup.hStdOutput = handle[5];
	}

	if (oflags & QSE_PIO_INTONUL) startup.hStdOutput = windevnul;
	if (oflags & QSE_PIO_OUTTONUL) startup.hStdOutput = windevnul;
	if (oflags & QSE_PIO_ERRTONUL) startup.hStdError = windevnul;

	if (oflags & QSE_PIO_DROPIN) startup.hStdInput = INVALID_HANDLE_VALUE;
	if (oflags & QSE_PIO_DROPOUT) startup.hStdOutput = INVALID_HANDLE_VALUE;
	if (oflags & QSE_PIO_DROPERR) startup.hStdError = INVALID_HANDLE_VALUE;

	startup.dwFlags |= STARTF_USESTDHANDLES;

	/* there is nothing to do for QSE_PIO_SHELL as CreateProcess
	 * takes the entire command line */

	if (oflags & QSE_PIO_SHELL) 
	{
		dup = QSE_MMGR_ALLOC (
			mmgr, (11+qse_strlen(cmd)+1 )*QSE_SIZEOF(qse_char_t));
		if (dup == QSE_NULL) goto oops;

		qse_strcpy (dup, QSE_T("cmd.exe /c "));
		qse_strcpy (&dup[11], cmd);
	}
	else
	{
		dup = qse_strdup (cmd, mmgr);
		if (dup == QSE_NULL) goto oops;
	}

	x = CreateProcess (
		NULL, /* LPCTSTR lpApplicationName */
		dup,  /* LPTSTR lpCommandLine */
		NULL, /* LPSECURITY_ATTRIBUTES lpProcessAttributes */
		NULL, /* LPSECURITY_ATTRIBUTES lpThreadAttributes */
		TRUE, /* BOOL bInheritHandles */
		0,    /* DWORD dwCreationFlags */
		NULL, /* LPVOID lpEnvironment */
		NULL, /* LPCTSTR lpCurrentDirectory */
		&startup, /* LPSTARTUPINFO lpStartupInfo */
		&procinfo /* LPPROCESS_INFORMATION lpProcessInformation */
	);

	QSE_MMGR_FREE (mmgr, dup); dup = QSE_NULL;
	CloseHandle (windevnul); windevnul = INVALID_HANDLE_VALUE;

	if (x == FALSE) goto oops;

	if (oflags & QSE_PIO_WRITEIN)
	{
		CloseHandle (handle[0]);
		handle[0] = QSE_PIO_HND_NIL;
	}
	if (oflags & QSE_PIO_READOUT)
	{
		CloseHandle (handle[3]);
		handle[3] = QSE_PIO_HND_NIL;
	}
	if (oflags & QSE_PIO_READERR)
	{
		CloseHandle (handle[5]);
		handle[5] = QSE_PIO_HND_NIL;
	}

	CloseHandle (procinfo.hThread);
	pio->child = procinfo.hProcess;
#elif defined(__OS2__)
	/* TODO: implement this for OS/2 */
#else

	if (oflags & QSE_PIO_WRITEIN)
	{
		if (QSE_PIPE(&handle[0]) <= -1) goto oops;
		minidx = 0; maxidx = 1;
	}

	if (oflags & QSE_PIO_READOUT)
	{
		if (QSE_PIPE(&handle[2]) <= -1) goto oops;
		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (oflags & QSE_PIO_READERR)
	{
		if (QSE_PIPE(&handle[4]) <= -1) goto oops;
		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1) goto oops;

	pid = QSE_FORK();
	if (pid <= -1) goto oops;

	if (pid == 0)
	{
		/* child */

		qse_pio_hnd_t devnull;
		qse_mchar_t* mcmd;
		extern char** environ; 
		int fcnt = 0;
	#ifndef QSE_CHAR_IS_MCHAR
       		qse_size_t n, mn, wl;
		qse_char_t* wcmd = QSE_NULL;
		qse_mchar_t buf[64];
	#endif

		/* TODO: consider if reading from /proc/self/fd is 
		 *       a better idea. */

		struct rlimit rlim;
		int fd;

		if (QSE_GETRLIMIT (RLIMIT_NOFILE, &rlim) <= -1 ||
		    rlim.rlim_max == RLIM_INFINITY) 
		{
		#ifdef HAVE_SYSCONF
			fd = sysconf (_SC_OPEN_MAX);
			if (fd <= 0)
		#endif
				fd = 1024;
		}
		else fd = rlim.rlim_max;

		while (--fd > 2)
		{
			if (fd != handle[0] &&
			    fd != handle[1] &&
			    fd != handle[2] &&
			    fd != handle[3] &&
			    fd != handle[4] &&
			    fd != handle[5]) QSE_CLOSE (fd);
		}

		if (oflags & QSE_PIO_WRITEIN)
		{
			/* child should read */
			QSE_CLOSE (handle[1]);
			handle[1] = QSE_PIO_HND_NIL;
			if (QSE_DUP2 (handle[0], 0) <= -1) goto child_oops;
			QSE_CLOSE (handle[0]);
			handle[0] = QSE_PIO_HND_NIL;
		}

		if (oflags & QSE_PIO_READOUT)
		{
			/* child should write */
			QSE_CLOSE (handle[2]);
			handle[2] = QSE_PIO_HND_NIL;
			if (QSE_DUP2 (handle[3], 1) <= -1) goto child_oops;

			if (oflags & QSE_PIO_ERRTOOUT)
			{
				if (QSE_DUP2 (handle[3], 2) <= -1) goto child_oops;
			}

			QSE_CLOSE (handle[3]); 
			handle[3] = QSE_PIO_HND_NIL;
		}

		if (oflags & QSE_PIO_READERR)
		{
			/* child should write */
			QSE_CLOSE (handle[4]); 
			handle[4] = QSE_PIO_HND_NIL;
			if (QSE_DUP2 (handle[5], 2) <= -1) goto child_oops;

			if (oflags & QSE_PIO_OUTTOERR)
			{
				if (QSE_DUP2 (handle[5], 1) <= -1) goto child_oops;
			}

			QSE_CLOSE (handle[5]);
			handle[5] = QSE_PIO_HND_NIL;
		}

		if ((oflags & QSE_PIO_INTONUL) || 
		    (oflags & QSE_PIO_OUTTONUL) ||
		    (oflags & QSE_PIO_ERRTONUL))
		{
		#ifdef O_LARGEFILE
			devnull = QSE_OPEN ("/dev/null", O_RDWR|O_LARGEFILE, 0);
		#else
			devnull = QSE_OPEN ("/dev/null", O_RDWR, 0);
		#endif
			if (devnull <= -1) goto child_oops;
		}

		if ((oflags & QSE_PIO_INTONUL)  &&
		    QSE_DUP2(devnull,0) <= -1) goto child_oops;
		if ((oflags & QSE_PIO_OUTTONUL) &&
		    QSE_DUP2(devnull,1) <= -1) goto child_oops;
		if ((oflags & QSE_PIO_ERRTONUL) &&
		    QSE_DUP2(devnull,2) <= -1) goto child_oops;

		if ((oflags & QSE_PIO_INTONUL) || 
		    (oflags & QSE_PIO_OUTTONUL) ||
		    (oflags & QSE_PIO_ERRTONUL)) QSE_CLOSE (devnull);

		if (oflags & QSE_PIO_DROPIN) QSE_CLOSE(0);
		if (oflags & QSE_PIO_DROPOUT) QSE_CLOSE(1);
		if (oflags & QSE_PIO_DROPERR) QSE_CLOSE(2);

	#ifdef QSE_CHAR_IS_MCHAR
		if (oflags & QSE_PIO_SHELL) mcmd = (qse_char_t*)cmd;
		else
		{
			mcmd =  qse_strdup (cmd, pio->mmgr);
			if (mcmd == QSE_NULL) goto child_oops;

			fcnt = qse_strspl (mcmd, QSE_T(""), 
				QSE_T('\"'), QSE_T('\"'), QSE_T('\'')); 
			if (fcnt <= 0) 
			{
				/* no field or an error */
				goto child_oops; 
			}
		}
	#else	
		if (oflags & QSE_PIO_SHELL)
		{
       			n = qse_wcstombslen (cmd, &mn);
			if (cmd[n] != QSE_WT('\0')) 
			{
				/* cmd has illegal sequence */
				goto child_oops;
			}
		}
		else
		{
			wcmd = qse_strdup (cmd, pio->mmgr);
			if (wcmd == QSE_NULL) goto child_oops;

			fcnt = qse_strspl (wcmd, QSE_T(""), 
				QSE_T('\"'), QSE_T('\"'), QSE_T('\'')); 
			if (fcnt <= 0)
			{
				/* no field or an error */
				goto child_oops;
			}
			
			for (wl = 0, n = fcnt; n > 0; )
			{
				if (wcmd[wl++] == QSE_T('\0')) n--;
			}

			n = qse_wcsntombsnlen (wcmd, wl, &mn);
			if (n != wl) goto child_oops;
		}

		mn = mn + 1;

		if (mn <= QSE_COUNTOF(buf)) 
		{
			mcmd = buf;
			mn = QSE_COUNTOF(buf);
		}
		else
		{
			mcmd = QSE_MMGR_ALLOC (
				pio->mmgr, mn*QSE_SIZEOF(*mcmd));
			if (mcmd == QSE_NULL) goto child_oops;
		}

		if (oflags & QSE_PIO_SHELL)
		{
			/* qse_wcstombs() should succeed as 
			 * qse_wcstombslen() was successful above */
			qse_wcstombs (cmd, mcmd, &mn);
			/* qse_wcstombs() null-terminate mcmd */
		}
		else
		{
			QSE_ASSERT (wcmd != QSE_NULL);
			/* qse_wcsntombsn() should succeed as 
			 * qse_wcsntombsnlen() was successful above */
			qse_wcsntombsn (wcmd, wl, mcmd, &mn);
			/* qse_wcsntombsn() doesn't null-terminate mcmd */
			mcmd[mn] = QSE_MT('\0');
		}
	#endif

		if (oflags & QSE_PIO_SHELL)
		{
			const qse_mchar_t* argv[4];

			argv[0] = QSE_MT("/bin/sh");
			argv[1] = QSE_MT("-c");
			argv[2] = mcmd;
			argv[3] = QSE_NULL;

			QSE_EXECVE (
				QSE_MT("/bin/sh"),
				(qse_mchar_t*const*)argv, environ);
		}
		else
		{
			int i;
			qse_mchar_t** argv;

			argv = QSE_MMGR_ALLOC (pio->mmgr, (fcnt+1)*QSE_SIZEOF(argv[0]));
			if (argv == QSE_NULL) goto child_oops;

			for (i = 0; i < fcnt; i++)
			{
				argv[i] = mcmd;
				while (*mcmd != QSE_MT('\0')) mcmd++;
				mcmd++;
			}
			argv[i] = QSE_NULL;

			QSE_EXECVE (argv[0], argv, environ);
		}

	child_oops:
		QSE_EXIT (128);
	}

	/* parent */
	pio->child = pid;

	if (oflags & QSE_PIO_WRITEIN)
	{
		/* 
		 * 012345
		 * rw----
		 * X  
		 * WRITE => 1
		 */
		QSE_CLOSE (handle[0]); handle[0] = QSE_PIO_HND_NIL;
	}

	if (oflags & QSE_PIO_READOUT)
	{
		/* 
		 * 012345
		 * --rw--
		 *    X
		 * READ => 2
		 */
		QSE_CLOSE (handle[3]); handle[3] = QSE_PIO_HND_NIL;
	}

	if (oflags & QSE_PIO_READERR)
	{
		/* 
		 * 012345
		 * ----rw
		 *      X   
		 * READ => 4
		 */
		QSE_CLOSE (handle[5]); handle[5] = QSE_PIO_HND_NIL;
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

	if (oflags & QSE_PIO_TEXT)
	{
		for (i = 0; i < QSE_COUNTOF(tio); i++)
		{
			int r;

			tio[i] = qse_tio_open (pio->mmgr, 0);
			if (tio[i] == QSE_NULL) goto oops;

			r = (i == QSE_PIO_IN)?
				qse_tio_attachout (tio[i], pio_output, &pio->pin[i]):
				qse_tio_attachin (tio[i], pio_input, &pio->pin[i]);

			if (r <= -1) goto oops;

			pio->pin[i].tio = tio[i];
		}
	}

	pio->option = 0;
	return pio;

oops:
#ifdef _WIN32
	if (windevnul != INVALID_HANDLE_VALUE) CloseHandle (windevnul);
	if (dup != QSE_NULL) QSE_MMGR_FREE (mmgr, dup);
#endif

	for (i = 0; i < QSE_COUNTOF(tio); i++) 
	{
		if (tio[i] != QSE_NULL) qse_tio_close (tio[i]);
	}
#if defined(_WIN32)
	for (i = minidx; i < maxidx; i++) CloseHandle (handle[i]);
#elif defined(__OS2__)
	/* TODO: */
	for (i = minidx; i < maxidx; i++) DosClose (handle[i]);
#else
	for (i = minidx; i < maxidx; i++) QSE_CLOSE (handle[i]);
#endif
	return QSE_NULL;
}

void qse_pio_fini (qse_pio_t* pio)
{
	qse_pio_end (pio, QSE_PIO_ERR);
	qse_pio_end (pio, QSE_PIO_OUT);
	qse_pio_end (pio, QSE_PIO_IN);

	pio->option &= ~QSE_PIO_WAIT_NOBLOCK;
	pio->option &= ~QSE_PIO_WAIT_NORETRY;
	qse_pio_wait (pio);
}

int qse_pio_getoption (qse_pio_t* pio)
{
	return pio->option;
}

void qse_pio_setoption (qse_pio_t* pio, int opt)
{
	pio->option = opt;
}

qse_pio_errnum_t qse_pio_geterrnum (qse_pio_t* pio)
{
	return pio->errnum;
}

const qse_char_t* qse_pio_geterrmsg (qse_pio_t* pio)
{
	static const qse_char_t* __errstr[] =
	{
		QSE_T("no error"),
		QSE_T("out of memory"),
		QSE_T("no handle available"),
		QSE_T("child process not valid"),
		QSE_T("interruped"),
		QSE_T("broken pipe"),
		QSE_T("systeam call error"),
		QSE_T("unknown error")
	};

	return __errstr[
		(pio->errnum < 0 || pio->errnum >= QSE_COUNTOF(__errstr))? 
		QSE_COUNTOF(__errstr) - 1: pio->errnum];
}

qse_pio_hnd_t qse_pio_gethandle (qse_pio_t* pio, qse_pio_hid_t hid)
{
	return pio->pin[hid].handle;
}

qse_pio_pid_t qse_pio_getchild (qse_pio_t* pio)
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
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (ReadFile(hnd, buf, (DWORD)size, &count, QSE_NULL) == FALSE) 
	{
		/* ReadFile receives ERROR_BROKEN_PIPE when the write end
		 * is closed in the child process */
		if (GetLastError() == ERROR_BROKEN_PIPE) return 0;
		pio->errnum = QSE_PIO_ESUBSYS;
		return -1;
	}
	return (qse_ssize_t)count;
#elif defined(__OS2__)
	if (size > QSE_TYPE_MAX(ULONG)) size = QSE_TYPE_MAX(ULONG);
	rc = DosRead (hnd, buf, (ULONG)size, &count);
	if (rc != NO_ERROR)
	{
    		if (rc == ERROR_BROKEN_PIPE) return 0; /* TODO: check this */
    		pio->errnum = QSE_PIO_ESUBSYS;
    		return -1;
    	}
	return (qse_ssize_t)count;
#else

	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);

reread:
	n = QSE_READ (hnd, buf, size);
	if (n == -1) 
	{
		if (errno == EINTR)
		{
			if (pio->option & QSE_PIO_READ_NORETRY) 
				pio->errnum = QSE_PIO_EINTR;
			else goto reread;
		}
		else if (errno == EPIPE)
		{
			pio->errnum = QSE_PIO_EPIPE;
		}
		else
		{
			pio->errnum = QSE_PIO_ESUBSYS;
		}
	}

	return n;
#endif
}

qse_ssize_t qse_pio_read (
	qse_pio_t* pio, void* buf, qse_size_t size, qse_pio_hid_t hid)
{
	if (pio->pin[hid].tio == QSE_NULL) 
		return pio_read (pio, buf, size, pio->pin[hid].handle);
	else
		return qse_tio_read (pio->pin[hid].tio, buf, size);
}

static qse_ssize_t pio_write (
	qse_pio_t* pio, const void* data, qse_size_t size, qse_pio_hnd_t hnd)
{
#if defined(_WIN32)
	DWORD count;
#elif defined(__OS2__)
	ULONG count;
	APIRET rc;
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
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (WriteFile (hnd, data, (DWORD)size, &count, QSE_NULL) == FALSE)
	{
		pio->errnum = (GetLastError() == ERROR_BROKEN_PIPE)?
			QSE_PIO_EPIPE: QSE_PIO_ESUBSYS;
		return -1;
	}
	return (qse_ssize_t)count;
#elif defined(__OS2__)
	if (size > QSE_TYPE_MAX(ULONG)) size = QSE_TYPE_MAX(ULONG);
	rc = DosWrite (hnd, (PVOID)data, (ULONG)size, &count);
	if (rc != NO_ERROR)
	{
    		pio->errnum = (rc == ERROR_BROKEN_PIPE)? 
			QSE_PIO_EPIPE: QSE_PIO_ESUBSYS; /* TODO: check this */
    		return -1;
	}
	return (qse_ssize_t)count;

#else
	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);

rewrite:
	n = QSE_WRITE (hnd, data, size);
	if (n == -1) 
	{
		if (errno == EINTR)
		{
			if (pio->option & QSE_PIO_WRITE_NORETRY)
				pio->errnum = QSE_PIO_EINTR;
			else goto rewrite;
		}
		else if (errno == EPIPE)
		{
			pio->errnum = QSE_PIO_EPIPE;
		}
		else
		{
			pio->errnum = QSE_PIO_ESUBSYS;
		}
	}
	return n;
#endif
}

qse_ssize_t qse_pio_write (
	qse_pio_t* pio, const void* data, qse_size_t size,
	qse_pio_hid_t hid)
{
	if (pio->pin[hid].tio == QSE_NULL)
		return pio_write (pio, data, size, pio->pin[hid].handle);
	else
		return qse_tio_write (pio->pin[hid].tio, data, size);
}

qse_ssize_t qse_pio_flush (qse_pio_t* pio, qse_pio_hid_t hid)
{
	if (pio->pin[hid].tio == QSE_NULL) return 0;
	return qse_tio_flush (pio->pin[hid].tio);
}

void qse_pio_end (qse_pio_t* pio, qse_pio_hid_t hid)
{
	if (pio->pin[hid].tio != QSE_NULL)
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
		((pio->option & QSE_PIO_WAIT_NOBLOCK)? 0: INFINITE)
	);
	if (w == WAIT_TIMEOUT)
	{
		/* the child process is still alive */
		return 255 + 1;
	}
	if (w != WAIT_OBJECT_0)
	{
		/* WAIT_FAILED, WAIT_ABANDONED */
		pio->errnum = QSE_PIO_ESUBSYS;
		return -1;
	}

	QSE_ASSERT (w == WAIT_OBJECT_0);
	
	if (GetExitCodeProcess (pio->child, &ecode) == FALSE) 
	{
		/* close the handle anyway to prevent further 
		 * errors when this function is called again */
		CloseHandle (pio->child); 
		pio->child = QSE_PIO_PID_NIL;

		pio->errnum = QSE_PIO_ESUBSYS;
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
		pio->errnum = QSE_PIO_ESUBSYS;
		return -1;
	}

	return ecode;
#elif defined(__OS2__)
	/* TODO: implement this */
	return -1;
#else
	int opt = 0;
	int ret = -1;

	if (pio->child == QSE_PIO_PID_NIL) 
	{
		pio->errnum = QSE_PIO_ECHILD;
		return -1;
	}

	if (pio->option & QSE_PIO_WAIT_NOBLOCK) opt |= WNOHANG;

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
				if (pio->option & QSE_PIO_WAIT_NORETRY) 
					pio->errnum = QSE_PIO_EINTR;
				else continue;
			}
			else pio->errnum = QSE_PIO_ESUBSYS;

			break;
		}

		if (n == 0) 
		{
			/* when WNOHANG is not specified, 0 can't be returned */
			QSE_ASSERT (pio->option & QSE_PIO_WAIT_NOBLOCK);

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
	APIRET n;
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
		pio->errnum = QSE_PIO_ESUBSYS;
		return -1;
	}
	return 0;
#elif defined(__OS2__)

/*TODO: must use DKP_PROCESSTREE? */
	n = DosKillProcess (pio->child, DKP_PROCESS);
	if (n != NO_ERROR)
	{
		pio->errnum = QSE_PIO_ESUBSYS;
		return -1;
	}
	return 0;	
#else
	n = QSE_KILL (pio->child, SIGKILL);
	if (n <= -1) pio->errnum = QSE_PIO_ESUBSYS;
	return n;
#endif
}

static qse_ssize_t pio_input (int cmd, void* arg, void* buf, qse_size_t size)
{
	qse_pio_pin_t* pin = (qse_pio_pin_t*)arg;
	QSE_ASSERT (pin != QSE_NULL);
	if (cmd == QSE_TIO_IO_DATA) 
	{
		QSE_ASSERT (pin->self != QSE_NULL);
		return pio_read (pin->self, buf, size, pin->handle);
	}

	/* take no actions for OPEN and CLOSE as they are handled
	 * by pio */
	return 0;
}

static qse_ssize_t pio_output (int cmd, void* arg, void* buf, qse_size_t size)
{
	qse_pio_pin_t* pin = (qse_pio_pin_t*)arg;
	QSE_ASSERT (pin != QSE_NULL);
	if (cmd == QSE_TIO_IO_DATA) 
	{
		QSE_ASSERT (pin->self != QSE_NULL);
		return pio_write (pin->self, buf, size, pin->handle);
	}

	/* take no actions for OPEN and CLOSE as they are handled
	 * by pio */
	return 0;
}
