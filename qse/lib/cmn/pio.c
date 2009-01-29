/*
 * $Id: pio.c,v 1.23 2006/06/30 04:18:47 bacon Exp $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/cmn/pio.h>
#include <qse/cmn/str.h>
#include "mem.h"

#ifdef _WIN32
#	include <windows.h>
#	include <tchar.h>
#else
#	include "syscall.h"
#	include <fcntl.h>
#	include <sys/wait.h>
#endif

QSE_IMPLEMENT_STD_FUNCTIONS (pio)

static qse_ssize_t pio_input (int cmd, void* arg, void* buf, qse_size_t size);
static qse_ssize_t pio_output (int cmd, void* arg, void* buf, qse_size_t size);

qse_pio_t* qse_pio_open (
	qse_mmgr_t* mmgr, qse_size_t ext,
	const qse_char_t* path, int flags)
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

	if (qse_pio_init (pio, mmgr, path, flags) == QSE_NULL)
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
	qse_pio_t* pio, qse_mmgr_t* mmgr, const qse_char_t* cmd, int flags)
{
	qse_pio_pid_t pid;
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

	QSE_MEMSET (pio, 0, QSE_SIZEOF(*pio));
	pio->mmgr = mmgr;

#ifdef _WIN32
	/* TODO: XXXXXXXXXXXXXXXXX */
http://msdn.microsoft.com/en-us/library/ms682499(VS.85).aspx
#else

	if (flags & QSE_PIO_WRITEIN)
	{
		if (QSE_PIPE(&handle[0]) == -1) goto oops;
		minidx = 0; maxidx = 1;
	}

	if (flags & QSE_PIO_READOUT)
	{
		if (QSE_PIPE(&handle[2]) == -1) goto oops;
		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (flags & QSE_PIO_READERR)
	{
		if (QSE_PIPE(&handle[4]) == -1) goto oops;
		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1) goto oops;

	pid = QSE_FORK();
	if (pid == -1) goto oops;

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

		if (flags & QSE_PIO_WRITEIN)
		{
			/* child should read */
			QSE_CLOSE (handle[1]);
			if (QSE_DUP2 (handle[0], 0) == -1) goto child_oops;
			QSE_CLOSE (handle[0]);
		}

		if (flags & QSE_PIO_READOUT)
		{
			/* child should write */
			QSE_CLOSE (handle[2]);
			if (QSE_DUP2 (handle[3], 1) == -1) goto child_oops;

			if (flags & QSE_PIO_ERRTOOUT)
			{
				if (QSE_DUP2 (handle[3], 2) == -1) goto child_oops;
			}

			QSE_CLOSE (handle[3]);
		}

		if (flags & QSE_PIO_READERR)
		{
			/* child should write */
			QSE_CLOSE (handle[4]);
			if (QSE_DUP2 (handle[5], 2) == -1) goto child_oops;

			if (flags & QSE_PIO_OUTTOERR)
			{
				if (QSE_DUP2 (handle[5], 1) == -1) goto child_oops;
			}

			QSE_CLOSE (handle[5]);
		}

		if ((flags & QSE_PIO_INTONUL) || 
		    (flags & QSE_PIO_OUTTONUL) ||
		    (flags & QSE_PIO_ERRTONUL))
		{
		#ifdef O_LARGEFILE
			devnull = QSE_OPEN ("/dev/null", O_RDWR|O_LARGEFILE, 0);
		#else
			devnull = QSE_OPEN ("/dev/null", O_RDWR, 0);
		#endif
			if (devnull == -1) goto oops;
		}

		if ((flags & QSE_PIO_INTONUL)  &&
		    QSE_DUP2(devnull,0) == -1) goto child_oops;
		if ((flags & QSE_PIO_OUTTONUL) &&
		    QSE_DUP2(devnull,1) == -1) goto child_oops;
		if ((flags & QSE_PIO_ERRTONUL) &&
		    QSE_DUP2(devnull,2) == -1) goto child_oops;

		if ((flags & QSE_PIO_INTONUL) || 
		    (flags & QSE_PIO_OUTTONUL) ||
		    (flags & QSE_PIO_ERRTONUL)) QSE_CLOSE (devnull);

		if (flags & QSE_PIO_DROPIN) QSE_CLOSE(0);
		if (flags & QSE_PIO_DROPOUT) QSE_CLOSE(1);
		if (flags & QSE_PIO_DROPERR) QSE_CLOSE(2);

	#ifdef QSE_CHAR_IS_MCHAR
		if (flags & QSE_PIO_SHELL) mcmd = (qse_char_t*)cmd;
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
		if (flags & QSE_PIO_SHELL)
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

		if (flags & QSE_PIO_SHELL)
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

		if (flags & QSE_PIO_SHELL)
		{
			const qse_mchar_t* argv[4];

			argv[0] = QSE_MT("/bin/sh");
			argv[1] = QSE_MT("-c");
			argv[2] = mcmd;
			argv[3] = QSE_NULL;

			QSE_EXECVE (QSE_MT("/bin/sh"), argv, environ);
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

	if (flags & QSE_PIO_WRITEIN)
	{
		/* 
		 * 012345
		 * rw----
		 * X  
		 * WRITE => 1
		 */
		QSE_CLOSE (handle[0]); handle[0] = QSE_PIO_HND_NIL;
	}

	if (flags & QSE_PIO_READOUT)
	{
		/* 
		 * 012345
		 * --rw--
		 *    X
		 * READ => 2
		 */
		QSE_CLOSE (handle[3]); handle[3] = QSE_PIO_HND_NIL;
	}

	if (flags & QSE_PIO_READERR)
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

	if (flags & QSE_PIO_TEXT)
	{
		for (i = 0; i < QSE_COUNTOF(tio); i++)
		{
			int r;

			tio[i] = qse_tio_open (pio->mmgr, 0);
			if (tio[i] == QSE_NULL) goto oops;

			r = (i == QSE_PIO_IN)?
				qse_tio_attachout (tio[i], pio_output, &pio->pin[i]):
				qse_tio_attachin (tio[i], pio_input, &pio->pin[i]);

			if (r == -1) goto oops;

			pio->pin[i].tio = tio[i];
		}
	}

	pio->flags = 0;
	return pio;

oops:
	for (i = 0; i < QSE_COUNTOF(tio); i++) qse_tio_close (tio[i]);
	for (i = minidx; i < maxidx; i++) QSE_CLOSE (handle[i]);
	return QSE_NULL;
}

void qse_pio_fini (qse_pio_t* pio)
{
	qse_pio_end (pio, QSE_PIO_IN);
	qse_pio_end (pio, QSE_PIO_OUT);
	qse_pio_end (pio, QSE_PIO_ERR);

	qse_pio_setflags (pio, QSE_PIO_WAIT_NOBLOCK|QSE_PIO_WAIT_NORETRY, -1);
	qse_pio_wait (pio);
}

int qse_pio_getflags (qse_pio_t* pio)
{
	return pio->flags;
}

void qse_pio_setflags (qse_pio_t* pio, int flags, int op)
{
	/*
	op => set
	op => off
	op => on
	*/

	if (op == 0) pio->flags = flags;
	else if (op > 0) pio->flags |= flags;
	else /*if (op < 0)*/ pio->flags &= ~flags;
}

qse_pio_err_t qse_pio_geterrnum (qse_pio_t* pio)
{
	return pio->errnum;
}

const qse_char_t* qse_pio_geterrstr (qse_pio_t* pio)
{
	static const qse_char_t* __errstr[] =
	{
		QSE_T("no error"),
		QSE_T("out of memory"),
		QSE_T("no handle available"),
		QSE_T("child process not valid"),
		QSE_T("interruped"),
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
#ifdef _WIN32
	DWORD count;
#else
	qse_ssize_t n;
#endif

	if (hnd == QSE_PIO_HND_NIL) 
	{
		/* the stream is already closed */
		pio->errnum = QSE_PIO_ENOHND;
		return (qse_ssize_t)-1;
	}

#ifdef _WIN32
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (ReadFile(hnd, buf, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else

	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);

reread:
	n = QSE_READ (hnd, buf, size);
	if (n == -1) 
	{
		if (errno == EINTR)
		{
			if (pio->flags & QSE_PIO_READ_NORETRY) 
				pio->errnum = QSE_PIO_EINTR;
			else goto reread;
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
#ifdef _WIN32
	DWORD count;
#else
	qse_ssize_t n;
#endif

	if (hnd == QSE_PIO_HND_NIL) 
	{
		/* the stream is already closed */
		pio->errnum = QSE_PIO_ENOHND;
		return (qse_ssize_t)-1;
	}

#ifdef _WIN32
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (WriteFile (hnd, data, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else
	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);

rewrite:
	n = QSE_WRITE (hnd, data, size);
	if (n == -1) 
	{
		if (errno == EINTR)
		{
			if (pio->flags & QSE_PIO_WRITE_NORETRY)
				pio->errnum = QSE_PIO_EINTR;
			else goto rewrite;
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
		QSE_CLOSE (pio->pin[hid].handle);
		pio->pin[hid].handle = QSE_PIO_HND_NIL;
	}
}

int qse_pio_wait (qse_pio_t* pio)
{
#ifdef _WIN32
	DWORD ec;

	if (pio->child == QSE_PIO_PID_NIL) 
	{
		
		pio->errnum = QSE_PIO_ECHILD;
		return -1;
	}

	WaitForSingleObject (pio->child, -1);
	if (GetExitCodeProcess (pio->child, &ec) == FALSE) ....
	/* close handle here to emulate waitpid() as much as possible. */
	CloseHandle (pio->child); 
	pio->child = QSE_PIO_PID_NIL;

#else
	int opt = 0;
	int ret = -1;

	if (pio->child == QSE_PIO_PID_NIL) 
	{
		pio->errnum = QSE_PIO_ECHILD;
		return -1;
	}

	if (pio->flags & QSE_PIO_WAIT_NOBLOCK) opt |= WNOHANG;

	while (1)
	{
		int status, n;

		n = QSE_WAITPID (pio->child, &status, opt);
		if (n == -1)
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
				if (pio->flags & QSE_PIO_WAIT_NORETRY) 
					pio->errnum = QSE_PIO_EINTR;
				else continue;
			}
			else pio->errnum = QSE_PIO_ESUBSYS;

			break;
		}

		if (n == 0) 
		{
			/* when WNOHANG is not specified, 0 can't be returned */
			QSE_ASSERT (pio->flags & QSE_PIO_WAIT_NOBLOCK);

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
				 * in fact, this else block should not be reached
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
#ifdef _WIN32
	DWORD n;
#else
	int n;
#endif

	if (pio->child == QSE_PIO_PID_NIL) 
	{
		pio->errnum = QSE_PIO_ECHILD;
		return -1;
	}

#ifdef _WIN32
	/* 9 was chosen below to treat TerminateProcess as kill -KILL. */
	n = TerminateProcess (pio->child, 255 + 1 + 9);
	if (n == FALSE) 
	{
		pio->errnum = QSE_PIO_SYSCALL;
		return -1;
	}
	return 0;
#else
	n = QSE_KILL (pio->child, SIGKILL);
	if (n == -1) pio->errnum = QSE_PIO_ESUBSYS;
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
