/*
 * $Id: pcp.c,v 1.23 2006/06/30 04:18:47 bacon Exp $
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

#include <qse/cmn/pcp.h>
#include <qse/cmn/str.h>
#include "mem.h"

#ifdef _WIN32
#	include <windows.h>
#	include <tchar.h>
#else
#	include "syscall.h"
#	include <fcntl.h>
#	include <errno.h>
#	include <sys/wait.h>
#endif

QSE_IMPLEMENT_STD_FUNCTIONS (pcp)

static qse_ssize_t pcp_read (
	qse_pcp_t* pcp, void* buf, qse_size_t size, qse_pcp_hnd_t hnd);
static qse_ssize_t pcp_write (
	qse_pcp_t* pcp, const void* data, qse_size_t size, qse_pcp_hnd_t hnd);

static qse_ssize_t pcp_input (int cmd, void* arg, void* buf, qse_size_t size);
static qse_ssize_t pcp_output (int cmd, void* arg, void* buf, qse_size_t size);

qse_pcp_t* qse_pcp_open (
	qse_mmgr_t* mmgr, qse_size_t ext,
	const qse_char_t* path, int flags)
{
	qse_pcp_t* pcp;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	pcp = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_pcp_t) + ext);
	if (pcp == QSE_NULL) return QSE_NULL;

	if (qse_pcp_init (pcp, mmgr, path, flags) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, pcp);
		return QSE_NULL;
	}

	return pcp;
}

void qse_pcp_close (qse_pcp_t* pcp)
{
	qse_pcp_fini (pcp);
	QSE_MMGR_FREE (pcp->mmgr, pcp);
}

qse_pcp_t* qse_pcp_init (
	qse_pcp_t* pcp, qse_mmgr_t* mmgr, const qse_char_t* cmd, int flags)
{
	qse_pcp_pid_t pid;
	qse_pcp_hnd_t handle[6] = 
	{ 
		QSE_PCP_HND_NIL, 
		QSE_PCP_HND_NIL,
		QSE_PCP_HND_NIL,
		QSE_PCP_HND_NIL,
		QSE_PCP_HND_NIL,
		QSE_PCP_HND_NIL
	};
	qse_tio_t* tio[3] = 
	{ 
		QSE_NULL, 
		QSE_NULL, 
		QSE_NULL 
	};
	int i, minidx = -1, maxidx = -1;

	QSE_MEMSET (pcp, 0, QSE_SIZEOF(*pcp));
	pcp->mmgr = mmgr;

#ifdef _WIN32
	/* TODO: XXXXXXXXXXXXXXXXX */
http://msdn.microsoft.com/en-us/library/ms682499(VS.85).aspx
#else

	if (flags & QSE_PCP_WRITEIN)
	{
		if (QSE_PIPE(&handle[0]) == -1) goto oops;
		minidx = 0; maxidx = 1;
	}

	if (flags & QSE_PCP_READOUT)
	{
		if (QSE_PIPE(&handle[2]) == -1) goto oops;
		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (flags & QSE_PCP_READERR)
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
		qse_pcp_hnd_t devnull;
		qse_mchar_t* mcmd;
		extern char** environ; 
		int fcnt = 0;
	#ifndef QSE_CHAR_IS_MCHAR
       		qse_size_t n, mn, wl;
		qse_char_t* wcmd = QSE_NULL;
		qse_mchar_t buf[64];
	#endif

		if (flags & QSE_PCP_WRITEIN)
		{
			/* child should read */
			QSE_CLOSE (handle[1]);
			if (QSE_DUP2 (handle[0], 0) == -1) goto child_oops;
			QSE_CLOSE (handle[0]);
		}

		if (flags & QSE_PCP_READOUT)
		{
			/* child should write */
			QSE_CLOSE (handle[2]);
			if (QSE_DUP2 (handle[3], 1) == -1) goto child_oops;

			if (flags & QSE_PCP_ERRTOOUT)
			{
				if (QSE_DUP2 (handle[3], 2) == -1) goto child_oops;
			}

			QSE_CLOSE (handle[3]);
		}

		if (flags & QSE_PCP_READERR)
		{
			/* child should write */
			QSE_CLOSE (handle[4]);
			if (QSE_DUP2 (handle[5], 2) == -1) goto child_oops;

			if (flags & QSE_PCP_OUTTOERR)
			{
				if (QSE_DUP2 (handle[5], 1) == -1) goto child_oops;
			}

			QSE_CLOSE (handle[5]);
		}

		if ((flags & QSE_PCP_INTONUL) || 
		    (flags & QSE_PCP_OUTTONUL) ||
		    (flags & QSE_PCP_ERRTONUL))
		{
		#ifdef O_LARGEFILE
			devnull = QSE_OPEN ("/dev/null", O_RDWR|O_LARGEFILE, 0);
		#else
			devnull = QSE_OPEN ("/dev/null", O_RDWR, 0);
		#endif
			if (devnull == -1) goto oops;
		}

		if ((flags & QSE_PCP_INTONUL)  &&
		    QSE_DUP2(devnull,0) == -1) goto child_oops;
		if ((flags & QSE_PCP_OUTTONUL) &&
		    QSE_DUP2(devnull,1) == -1) goto child_oops;
		if ((flags & QSE_PCP_ERRTONUL) &&
		    QSE_DUP2(devnull,2) == -1) goto child_oops;

		if ((flags & QSE_PCP_INTONUL) || 
		    (flags & QSE_PCP_OUTTONUL) ||
		    (flags & QSE_PCP_ERRTONUL)) QSE_CLOSE (devnull);

		if (flags & QSE_PCP_DROPIN) QSE_CLOSE(0);
		if (flags & QSE_PCP_DROPOUT) QSE_CLOSE(1);
		if (flags & QSE_PCP_DROPERR) QSE_CLOSE(2);

	#ifdef QSE_CHAR_IS_MCHAR
		if (flags & QSE_PCP_SHELL) mcmd = (qse_char_t*)cmd;
		else
		{
			mcmd =  qse_strdup (cmd, pcp->mmgr);
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
		if (flags & QSE_PCP_SHELL)
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
			wcmd = qse_strdup (cmd, pcp->mmgr);
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
				pcp->mmgr, mn*QSE_SIZEOF(*mcmd));
			if (mcmd == QSE_NULL) goto child_oops;
		}

		if (flags & QSE_PCP_SHELL)
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

		if (flags & QSE_PCP_SHELL)
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

			argv = QSE_MMGR_ALLOC (pcp->mmgr, (fcnt+1)*QSE_SIZEOF(argv[0]));
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
	pcp->child = pid;

	if (flags & QSE_PCP_WRITEIN)
	{
		/* 
		 * 012345
		 * rw----
		 * X  
		 * WRITE => 1
		 */
		QSE_CLOSE (handle[0]); handle[0] = QSE_PCP_HND_NIL;
	}

	if (flags & QSE_PCP_READOUT)
	{
		/* 
		 * 012345
		 * --rw--
		 *    X
		 * READ => 2
		 */
		QSE_CLOSE (handle[3]); handle[3] = QSE_PCP_HND_NIL;
	}

	if (flags & QSE_PCP_READERR)
	{
		/* 
		 * 012345
		 * ----rw
		 *      X   
		 * READ => 4
		 */
		QSE_CLOSE (handle[5]); handle[5] = QSE_PCP_HND_NIL;
	}

#endif

	/* store back references */
	pcp->pip[QSE_PCP_IN].self = pcp;
	pcp->pip[QSE_PCP_OUT].self = pcp;
	pcp->pip[QSE_PCP_ERR].self = pcp;

	/* store actual pipe handles */
	pcp->pip[QSE_PCP_IN].handle = handle[1];
	pcp->pip[QSE_PCP_OUT].handle = handle[2];
	pcp->pip[QSE_PCP_ERR].handle = handle[4];

	if (flags & QSE_PCP_TEXT)
	{
		for (i = 0; i < QSE_COUNTOF(tio); i++)
		{
			int r;

			tio[i] = qse_tio_open (pcp->mmgr, 0);
			if (tio[i] == QSE_NULL) goto oops;

			r = (i == QSE_PCP_IN)?
				qse_tio_attachout (tio[i], pcp_output, &pcp->pip[i]):
				qse_tio_attachin (tio[i], pcp_input, &pcp->pip[i]);

			if (r == -1) goto oops;

			pcp->pip[i].tio = tio[i];
		}
	}

	return pcp;

oops:
	for (i = 0; i < QSE_COUNTOF(tio); i++) qse_tio_close (tio[i]);
	for (i = minidx; i < maxidx; i++) QSE_CLOSE (handle[i]);
	return QSE_NULL;
}

void qse_pcp_fini (qse_pcp_t* pcp)
{
	qse_pcp_end (pcp, QSE_PCP_IN);
	qse_pcp_end (pcp, QSE_PCP_OUT);
	qse_pcp_end (pcp, QSE_PCP_ERR);
	qse_pcp_wait (pcp, QSE_PCP_IGNINTR);
}

qse_pcp_err_t qse_pcp_geterrnum (qse_pcp_t* pcp)
{
	return pcp->errnum;
}

const qse_char_t* qse_pcp_geterrstr (qse_pcp_t* pcp)
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
		(pcp->errnum < 0 || pcp->errnum >= QSE_COUNTOF(__errstr))? 
		QSE_COUNTOF(__errstr) - 1: pcp->errnum];
}

qse_pcp_hnd_t qse_pcp_gethandle (qse_pcp_t* pcp, qse_pcp_hid_t hid)
{
	return pcp->pip[hid].handle;
}

qse_pcp_pid_t qse_pcp_getchild (qse_pcp_t* pcp)
{
	return pcp->child;
}

qse_ssize_t qse_pcp_read (
	qse_pcp_t* pcp, void* buf, qse_size_t size, qse_pcp_hid_t hid)
{
	if (pcp->pip[hid].tio == QSE_NULL) 
		return pcp_read (pcp, buf, size, pcp->pip[hid].handle);
	else
		return qse_tio_read (pcp->pip[hid].tio, buf, size);
}

qse_ssize_t qse_pcp_write (
	qse_pcp_t* pcp, const void* data, qse_size_t size, qse_pcp_hid_t hid)
{
	if (pcp->pip[hid].tio == QSE_NULL)
		return pcp_write (pcp, data, size, pcp->pip[hid].handle);
	else
		return qse_tio_write (pcp->pip[hid].tio, data, size);
}

qse_ssize_t qse_pcp_flush (qse_pcp_t* pcp, qse_pcp_hid_t hid)
{
	if (pcp->pip[hid].tio == QSE_NULL) return 0;
	return qse_tio_flush (pcp->pip[hid].tio);
}

static qse_ssize_t pcp_read (
	qse_pcp_t* pcp, void* buf, qse_size_t size, qse_pcp_hnd_t hnd)
{
#ifdef _WIN32
	DWORD count;
#else
	qse_ssize_t n;
#endif

	if (hnd == QSE_PCP_HND_NIL) 
	{
		/* the stream is already closed */
		pcp->errnum = QSE_PCP_ENOHND;
		return (qse_ssize_t)-1;
	}

#ifdef _WIN32
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (ReadFile(hnd, buf, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else

	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	n = QSE_READ (hnd, buf, size);
	if (n == -1) 
	{
		pcp->errnum = (errno == EINTR)? 
			QSE_PCP_EINTR: QSE_PCP_ESYSCALL;
	}

	return n;
#endif
}

static qse_ssize_t pcp_write (
	qse_pcp_t* pcp, const void* data, qse_size_t size, qse_pcp_hnd_t hnd)
{
#ifdef _WIN32
	DWORD count;
#else
	qse_ssize_t n;
#endif

	if (hnd == QSE_PCP_HND_NIL) 
	{
		/* the stream is already closed */
		pcp->errnum = QSE_PCP_ENOHND;
		return (qse_ssize_t)-1;
	}

#ifdef _WIN32
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (WriteFile (hnd, data, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else
	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	n = QSE_WRITE (hnd, data, size);
	if (n == -1) 
	{
		pcp->errnum = (errno == EINTR)? 
			QSE_PCP_EINTR: QSE_PCP_ESYSCALL;
	}
	return n;
#endif
}

void qse_pcp_end (qse_pcp_t* pcp, qse_pcp_hid_t hid)
{
	if (pcp->pip[hid].tio != QSE_NULL)
	{
		qse_tio_close (pcp->pip[hid].tio);
		pcp->pip[hid].tio = QSE_NULL;
	}

	if (pcp->pip[hid].handle != QSE_PCP_HND_NIL)
	{
		QSE_CLOSE (pcp->pip[hid].handle);
		pcp->pip[hid].handle = QSE_PCP_HND_NIL;
	}
}

int qse_pcp_wait (qse_pcp_t* pcp, int flags)
{
#ifdef _WIN32
	DWORD ec;

	if (pcp->child == QSE_PCP_PID_NIL) 
	{
		
		pcp->errnum = QSE_PCP_ECHILD;
		return -1;
	}

	WaitForSingleObject (pcp->child, -1);
	if (GetExitCodeProcess (pcp->child, &ec) == -1)
	/* close handle here to emulate waitpid() as much as possible. */
	CloseHandle (pcp->child); 
	pcp->child = QSE_PCP_PID_NIL;

#else
	int opt = 0;
	int ret = -1;

	if (pcp->child == QSE_PCP_PID_NIL) 
	{
		pcp->errnum = QSE_PCP_ECHILD;
		return -1;
	}

	if (flags & QSE_PCP_NOHANG) opt |= WNOHANG;

	while (1)
	{
		int status, n;

		n = QSE_WAITPID (pcp->child, &status, opt);

		if (n == -1)
		{
			if (errno == ECHILD)
			{
				/* most likely, the process has already been 
				 * waitpid()ed on. */
				pcp->child = QSE_PCP_PID_NIL;
				pcp->errnum = QSE_PCP_ECHILD;
			}
			else if (errno == EINTR)
			{
				if (flags & QSE_PCP_IGNINTR) continue;
				pcp->errnum = QSE_PCP_EINTR;
			}
			else pcp->errnum = QSE_PCP_ESYSCALL;

			break;
		}

		if (n == 0) 
		{
			/* when WNOHANG is not specified, 0 can't be returned */
			QSE_ASSERT (flags & QSE_PCP_NOHANG);

			ret = 255 + 1;
			/* the child process is still alive */
			break;
		}

		if (n == pcp->child)
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

			pcp->child = QSE_PCP_PID_NIL;
			break;
		}
	}

	return ret;
#endif
}

int qse_pcp_kill (qse_pcp_t* pcp)
{
#ifdef _WIN32
	DWORD n;
#else
	int n;
#endif

	if (pcp->child == QSE_PCP_PID_NIL) 
	{
		pcp->errnum = QSE_PCP_ECHILD;
		return -1;
	}

#ifdef _WIN32
	/* 9 was chosen below to treat TerminateProcess as kill -KILL. */
	n = TerminateProcess (pcp->child, 255 + 1 + 9);
	if (n == FALSE) 
	{
		pcp->errnum = QSE_PCP_SYSCALL;
		return -1;
	}
	return 0;
#else
	n = QSE_KILL (pcp->child, SIGKILL);
	if (n == -1) pcp->errnum = QSE_PCP_ESYSCALL;
	return n;
#endif
}

static qse_ssize_t pcp_input (int cmd, void* arg, void* buf, qse_size_t size)
{
        qse_pcp_pip_t* pip = (qse_pcp_pip_t*)arg;
        QSE_ASSERT (pip != QSE_NULL);
        if (cmd == QSE_TIO_IO_DATA) 
	{
		QSE_ASSERT (pip->self != QSE_NULL);
		return pcp_read (pip->self, buf, size, pip->handle);
	}

	/* take no actions for OPEN and CLOSE as they are handled
	 * by pcp */
        return 0;
}

static qse_ssize_t pcp_output (int cmd, void* arg, void* buf, qse_size_t size)
{
        qse_pcp_pip_t* pip = (qse_pcp_pip_t*)arg;
        QSE_ASSERT (pip != QSE_NULL);
        if (cmd == QSE_TIO_IO_DATA) 
	{
		QSE_ASSERT (pip->self != QSE_NULL);
		return pcp_write (pip->self, buf, size, pip->handle);
	}

	/* take no actions for OPEN and CLOSE as they are handled
	 * by pcp */
        return 0;
}
