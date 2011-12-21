/*
 * $Id: pio.c 565 2011-09-11 02:48:21Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
#	define INCL_DOSQUEUES
#	define INCL_DOSPROCESS
#	define INCL_DOSERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <io.h>
#else
#	include "syscall.h"
#	include <fcntl.h>
#	include <sys/wait.h>
#endif

QSE_IMPLEMENT_COMMON_FUNCTIONS (pio)

static qse_ssize_t pio_input (qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size);
static qse_ssize_t pio_output (qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size);

qse_pio_t* qse_pio_open (
	qse_mmgr_t* mmgr, qse_size_t ext, 
	const qse_char_t* cmd, qse_env_t* env, int oflags)
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

	if (qse_pio_init (pio, mmgr, cmd, env, oflags) <= -1)
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

int qse_pio_init (
	qse_pio_t* pio, qse_mmgr_t* mmgr, const qse_char_t* cmd, 
	qse_env_t* env, int oflags)
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

#else
	qse_pio_pid_t pid;
#endif

	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (pio, 0, QSE_SIZEOF(*pio));
	pio->mmgr = mmgr;

#if defined(_WIN32)
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

	if (maxidx == -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		goto oops;
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

	if (oflags & QSE_PIO_INTONUL) startup.hStdInput = windevnul;
	if (oflags & QSE_PIO_OUTTONUL) startup.hStdOutput = windevnul;
	if (oflags & QSE_PIO_ERRTONUL) startup.hStdError = windevnul;

	if (oflags & QSE_PIO_DROPIN) startup.hStdInput = INVALID_HANDLE_VALUE;
	if (oflags & QSE_PIO_DROPOUT) startup.hStdOutput = INVALID_HANDLE_VALUE;
	if (oflags & QSE_PIO_DROPERR) startup.hStdError = INVALID_HANDLE_VALUE;

	startup.dwFlags |= STARTF_USESTDHANDLES;

	/* there is nothing to do for QSE_PIO_SHELL as CreateProcess
	 * takes the entire command line */

	{
		qse_char_t* dupcmd;
		BOOL x;

		if (oflags & QSE_PIO_SHELL) 
		{
		#if defined(QSE_CHAR_IS_WCHAR)
			if (oflags & QSE_PIO_MBSCMD)
			{
				const qse_mchar_t* x[3];
				x[0] = QSE_MT("cmd.exe /c ");
				x[1] = (const qse_mchar_t*)cmd;
				x[2] = QSE_NULL;
				dupcmd = qse_mbsatowcsdup (x, mmgr);
			}
			else
			{
		#endif
				dupcmd = qse_strdup2 (QSE_T("cmd.exe /c "), cmd, mmgr);
		#if defined(QSE_CHAR_IS_WCHAR)
			}
		#endif
		}
		else 
		{
		#if defined(QSE_CHAR_IS_WCHAR)
			if (oflags & QSE_PIO_MBSCMD)
			{
				dupcmd = qse_mbstowcsdup ((const qse_mchar_t*)cmd, mmgr);
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

		x = CreateProcess (
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
		if (x == FALSE) goto oops;
	}

	if (windevnul != INVALID_HANDLE_VALUE)
	{
		CloseHandle (windevnul); 
		windevnul = INVALID_HANDLE_VALUE;
	}

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

	#define DOS_DUP_HANDLE(x,y) QSE_BLOCK ( \
		if (DosDupHandle(x,y) != NO_ERROR) goto oops; \
	)

	if (oflags & QSE_PIO_WRITEIN)
	{
		/* child reads, parent writes */		
		if (DosCreatePipe (
			&handle[0], &handle[1], pipe_size) != NO_ERROR) goto oops;

		/* the parent writes to handle[1] and the child reads from 
		 * handle[0] inherited. set the flag not to inherit handle[1]. */
		if (DosSetFHState (handle[1], OPEN_FLAGS_NOINHERIT) != NO_ERROR) goto oops;

		/* Need to do somthing like this to set the flag instead? 
		ULONG state;               
		DosQueryFHState (handle[1], &state);
		DosSetFHState (handle[1], state | OPEN_FLAGS_NOINHERIT); */

		minidx = 0; maxidx = 1;
	}

	if (oflags & QSE_PIO_READOUT)
	{
		/* child writes, parent reads */
		if (DosCreatePipe (
			&handle[2], &handle[3], pipe_size) != NO_ERROR) goto oops;

		/* the parent reads from handle[2] and the child writes to 
		 * handle[3] inherited. set the flag not to inherit handle[2] */
		if (DosSetFHState (handle[2], OPEN_FLAGS_NOINHERIT) != NO_ERROR) goto oops;

		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (oflags & QSE_PIO_READERR)
	{
		/* child writes, parent reads */
		if (DosCreatePipe (
			&handle[4], &handle[5], pipe_size) != NO_ERROR) goto oops;

		/* the parent reads from handle[4] and the child writes to 
		 * handle[5] inherited. set the flag not to inherit handle[4] */
		if (DosSetFHState (handle[4], OPEN_FLAGS_NOINHERIT) != NO_ERROR) goto oops;

		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		goto oops;
	}

	if ((oflags & QSE_PIO_INTONUL) || 
	    (oflags & QSE_PIO_OUTTONUL) ||
	    (oflags & QSE_PIO_ERRTONUL))
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
		if (rc != NO_ERROR) goto oops;
	}

	/* duplicate the current stdin/out/err to old_in/out/err as a new handle */
	
	if (DosDupHandle (std_in, &old_in) != NO_ERROR) 
	{
		goto oops;
	}
	if (DosDupHandle (std_out, &old_out) != NO_ERROR) 
	{
		DosClose (old_in); old_in = QSE_PIO_HND_NIL;
		goto oops;
	}
	if (DosDupHandle (std_err, &old_err) != NO_ERROR)
	{
		DosClose (old_out); old_out = QSE_PIO_HND_NIL;
		DosClose (old_in); old_in = QSE_PIO_HND_NIL;
		goto oops;
	}    	

	/* we must not let our own stdin/out/err duplicated 
	 * into old_in/out/err be inherited */
	DosSetFHState (old_in, OPEN_FLAGS_NOINHERIT);
	DosSetFHState (old_out, OPEN_FLAGS_NOINHERIT); 
	DosSetFHState (old_err, OPEN_FLAGS_NOINHERIT);

	if (oflags & QSE_PIO_WRITEIN)
	{
		/* the child reads from handle[0] inherited and expects it to
		 * be stdin(0). so we duplicate handle[0] to stdin */
		DOS_DUP_HANDLE (handle[0], &std_in);

		/* the parent writes to handle[1] but does not read from handle[0].
		 * so we close it */
		DosClose (handle[0]); handle[0] = QSE_PIO_HND_NIL;
	}

	if (oflags & QSE_PIO_READOUT)
	{
		/* the child writes to handle[3] inherited and expects it to
		 * be stdout(1). so we duplicate handle[3] to stdout. */
		DOS_DUP_HANDLE (handle[3], &std_out);
		if (oflags & QSE_PIO_ERRTOOUT) DOS_DUP_HANDLE (handle[3], &std_err);
		/* the parent reads from handle[2] but does not write to handle[3].
		 * so we close it */
		DosClose (handle[3]); handle[3] = QSE_PIO_HND_NIL;
	}

	if (oflags & QSE_PIO_READERR)
	{
		DOS_DUP_HANDLE (handle[5], &std_err);
		if (oflags & QSE_PIO_OUTTOERR) DOS_DUP_HANDLE (handle[5], &std_out);
		DosClose (handle[5]); handle[5] = QSE_PIO_HND_NIL;
	}

	if (oflags & QSE_PIO_INTONUL) DOS_DUP_HANDLE (os2devnul, &std_in);
	if (oflags & QSE_PIO_OUTTONUL) DOS_DUP_HANDLE (os2devnul, &std_out);
	if (oflags & QSE_PIO_ERRTONUL) DOS_DUP_HANDLE (os2devnul, &std_err);

	if (os2devnul != QSE_PIO_HND_NIL)
	{
	    	/* close NUL early as we've duplicated it already */
		DosClose (os2devnul); 
		os2devnul = QSE_PIO_HND_NIL;
	}
	
	/* at this moment, stdin/out/err are already redirected to pipes
	 * if proper flags have been set. we close them selectively if 
	 * dropping is requested */
	if (oflags & QSE_PIO_DROPIN) DosClose (std_in);
	if (oflags & QSE_PIO_DROPOUT) DosClose (std_out);
	if (oflags & QSE_PIO_DROPERR) DosClose (std_err);

	if (oflags & QSE_PIO_SHELL) 
	{
		qse_size_t n, mn;

	#ifdef QSE_CHAR_IS_MCHAR
		mn = qse_strlen(cmd);
	#else
		if (oflags & QSE_PIO_MBSCMD)
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
		if (oflags & QSE_PIO_MBSCMD)
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
		if (oflags & QSE_PIO_MBSCMD)
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

	if (rc != NO_ERROR) goto oops;
	pio->child = child_rc.codeTerminate;

#elif defined(__DOS__)
		
	/* DOS not multi-processed. can't support pio */
	return -1;

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

	if (maxidx == -1) 
	{
		pio->errnum = QSE_PIO_EINVAL;
		goto oops;
	}

	pid = QSE_FORK();
	if (pid <= -1) goto oops;

	if (pid == 0)
	{
		/* child */

		qse_pio_hnd_t devnull;
		qse_mchar_t* mcmd;
		int fcnt = 0;
	#ifndef QSE_CHAR_IS_MCHAR
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
			devnull = QSE_OPEN (QSE_MT("/dev/null"), O_RDWR|O_LARGEFILE, 0);
		#else
			devnull = QSE_OPEN (QSE_MT("/dev/null"), O_RDWR, 0);
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
				QSE_T('\"'), QSE_T('\"'), QSE_T('\\')); 
			if (fcnt <= 0) 
			{
				/* no field or an error */
				goto child_oops; 
			}
		}
	#else	
		if (oflags & QSE_PIO_MBSCMD) 
		{
			/* the cmd is flagged to be of qse_mchar_t 
			 * while the default character type is qse_wchar_t. */

			if (oflags & QSE_PIO_SHELL) mcmd = (qse_mchar_t*)cmd;
			else
			{
				mcmd =  qse_mbsdup ((const qse_mchar_t*)cmd, pio->mmgr);
				if (mcmd == QSE_NULL) goto child_oops;

				fcnt = qse_mbsspl (mcmd, QSE_MT(""), 
					QSE_MT('\"'), QSE_MT('\"'), QSE_MT('\\')); 
				if (fcnt <= 0) 
				{
					/* no field or an error */
					goto child_oops; 
				}
			}
		}
		else
		{
			qse_size_t n, mn, wl;
			qse_char_t* wcmd = QSE_NULL;

			if (oflags & QSE_PIO_SHELL)
			{
				if (qse_wcstombs (cmd, &wl, QSE_NULL, &mn) <= -1)
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
					QSE_T('\"'), QSE_T('\"'), QSE_T('\\')); 
				if (fcnt <= 0)
				{
					/* no field or an error */
					goto child_oops;
				}
				
				/* calculate the length of the string after splitting */
				for (wl = 0, n = fcnt; n > 0; )
				{
					if (wcmd[wl++] == QSE_T('\0')) n--;
				}
	
#if 0
				n = qse_wcsntombsnlen (wcmd, wl, &mn);
				if (n != wl) goto child_oops;
#endif
				if (qse_wcsntombsn (wcmd, &wl, QSE_NULL, &mn) <= -1) goto child_oops;
			}
	
			/* prepare to reserve 1 more slot for the terminating '\0'
			 * by incrementing mn by 1. */
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
			}
		}
	#endif

		if (oflags & QSE_PIO_SHELL)
		{
			const qse_mchar_t* argv[4];
			extern char** environ;

			argv[0] = QSE_MT("/bin/sh");
			argv[1] = QSE_MT("-c");
			argv[2] = mcmd;
			argv[3] = QSE_NULL;

			QSE_EXECVE (
				QSE_MT("/bin/sh"),
				(qse_mchar_t*const*)argv, 
				(env? qse_env_getarr(env): environ)
			);
		}
		else
		{
			int i;
			qse_mchar_t** argv;
			extern char** environ;

			argv = QSE_MMGR_ALLOC (pio->mmgr, (fcnt+1)*QSE_SIZEOF(argv[0]));
			if (argv == QSE_NULL) goto child_oops;

			for (i = 0; i < fcnt; i++)
			{
				argv[i] = mcmd;
				while (*mcmd != QSE_MT('\0')) mcmd++;
				mcmd++;
			}
			argv[i] = QSE_NULL;

			QSE_EXECVE (argv[0], argv, (env? qse_env_getarr(env): environ));

			/* this won't be reached if execve succeeds */
			QSE_MMGR_FREE (pio->mmgr, argv);
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
		QSE_CLOSE (handle[0]);
		handle[0] = QSE_PIO_HND_NIL;
	}

	if (oflags & QSE_PIO_READOUT)
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

	if (oflags & QSE_PIO_READERR)
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

	if (oflags & QSE_PIO_TEXT)
	{
		int topt = 0;

		if (oflags & QSE_PIO_IGNOREMBWCERR) topt |= QSE_TIO_IGNOREMBWCERR;
		if (oflags & QSE_PIO_NOAUTOFLUSH) topt |= QSE_TIO_NOAUTOFLUSH;

		for (i = 0; i < QSE_COUNTOF(tio); i++)
		{
			int r;

			tio[i] = qse_tio_open (pio->mmgr, 0, topt);
			if (tio[i] == QSE_NULL) 
			{
				pio->errnum = QSE_PIO_ENOMEM;
				goto oops;
			}

			r = (i == QSE_PIO_IN)?
				qse_tio_attachout (tio[i], pio_output, &pio->pin[i]):
				qse_tio_attachin (tio[i], pio_input, &pio->pin[i]);

			if (r <= -1) goto oops;

			pio->pin[i].tio = tio[i];
		}
	}

	pio->option = 0;
	return 0;

oops:
	if (pio->errnum == QSE_PIO_ENOERR) 
		pio->errnum = QSE_PIO_ESUBSYS;

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
		QSE_T("invalid parameter"),
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
		pio->errnum = QSE_PIO_ESUBSYS;
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
    		pio->errnum = QSE_PIO_ESUBSYS;
    		return -1;
    	}
	return (qse_ssize_t)count;

#elif defined(__DOS__)
	/* TODO: verify this */

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int);

	n = read (hnd, buf, size);
	if (n == -1) pio->errnum = QSE_PIO_ESUBSYS;
	return n;

#else

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);

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
#elif defined(__DOS__)
	int n;
#else
	qse_ssize_t n;
#endif

	if (hnd == QSE_PIO_HND_NIL) 
	{
		/* the stream is already closed */
		pio->errnum = QSE_PIO_ENOHND;
		return -1;
	}

#if defined(_WIN32)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD);

	if (WriteFile (hnd, data, (DWORD)size, &count, QSE_NULL) == FALSE)
	{
		pio->errnum = (GetLastError() == ERROR_BROKEN_PIPE)?
			QSE_PIO_EPIPE: QSE_PIO_ESUBSYS;
		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__OS2__)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG);

	rc = DosWrite (hnd, (PVOID)data, (ULONG)size, &count);
	if (rc != NO_ERROR)
	{
    		pio->errnum = (rc == ERROR_BROKEN_PIPE)? 
			QSE_PIO_EPIPE: QSE_PIO_ESUBSYS; /* TODO: check this */
    		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__DOS__)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int);

	n = write (hnd, data, size);
	if (n == -1) pio->errnum = QSE_PIO_ESUBSYS;
	return n;

#else

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);

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
		((pio->option & QSE_PIO_WAIT_NOBLOCK)? DCWW_NOWAIT: DCWW_WAIT),
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
		pio->errnum = QSE_PIO_ESUBSYS;
		return -1;
	}

	/* close handle here to emulate waitpid() as much as possible. */
	/*DosClose (pio->child);*/
	pio->child = QSE_PIO_PID_NIL;

	return (child_rc.codeTerminate == TC_EXIT)? 
		child_rc.codeResult: (255 + 1 + child_rc.codeTerminate);

#elif defined(__DOS__)
	/* TOOD: implement this */
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
		pio->errnum = QSE_PIO_ESUBSYS;
		return -1;
	}
	return 0;

#elif defined(__OS2__)
/*TODO: must use DKP_PROCESS? */
	rc = DosKillProcess (pio->child, DKP_PROCESSTREE);
	if (rc != NO_ERROR)
	{
		pio->errnum = QSE_PIO_ESUBSYS;
		return -1;
	}
	return 0;	

#elif defined(__DOS__)
	/* TODO: implement this*/
	return -1;

#else
	n = QSE_KILL (pio->child, SIGKILL);
	if (n <= -1) pio->errnum = QSE_PIO_ESUBSYS;
	return n;
#endif
}

static qse_ssize_t pio_input (qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size)
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

static qse_ssize_t pio_output (qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size)
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
