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

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/io/sio.h>
#include "mem.h"

#if defined(HAVE_EXECINFO_H)
#	include <execinfo.h>
#	include <stdlib.h>
#	include <qse/cmn/str.h>
#endif

#if defined(_WIN32)
#	include <windows.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSFILEMGR
#	include <os2.h>
#elif defined(__DOS__)
#	include <dos.h>
#	include <dosfunc.h>
#elif defined(vms) || defined(__vms)
#	define __NEW_STARLET 1
#	include <starlet.h> /* (SYS$...) */
#	include <ssdef.h> /* (SS$...) */
#	include <lib$routines.h> /* (lib$...) */
#elif defined(macintosh)
#	include <Process.h>
#	include <Dialogs.h>
#	include <TextUtils.h>
#	include <qse/cmn/str.h>
#else
#	include "syscall.h"
#endif

void qse_assert_failed (
	const qse_char_t* expr, const qse_char_t* desc, 
	const qse_char_t* file, qse_size_t line)
{
#if defined(_WIN32)
	HANDLE stderr;

	stderr = GetStdHandle (STD_ERROR_HANDLE);
	if (stderr != INVALID_HANDLE_VALUE)
	{
		DWORD mode;
		if (GetConsoleMode (stderr, &mode) == FALSE)
			stderr = INVALID_HANDLE_VALUE;
	}

	if (stderr == INVALID_HANDLE_VALUE)
	{
		/* Use a message box if stderr is not available */

		qse_char_t tmp[1024];
		qse_strxfmt (tmp, QSE_COUNTOF(tmp), 
			QSE_T("[FILE %s LINE %lu]\r\n%s%s%s"), 
			file, line, expr, 
			(desc? QSE_T("\n\n"): QSE_T("")),
			(desc? desc: QSE_T(""))
		);
		MessageBox (QSE_NULL, tmp, QSE_T("ASSERTION FAILURE"), MB_OK | MB_ICONERROR);
	}
	else
	{
		qse_char_t tmp[1024];
		DWORD written;

		WriteConsole (stderr, QSE_T("[ASSERTION FAILURE]\r\n"), 21, &written, QSE_NULL);

		qse_strxfmt (tmp, QSE_COUNTOF(tmp), QSE_T("[FILE %s LINE %lu]\r\n"), file, (unsigned long)line);
		WriteConsole (stderr, tmp, qse_strlen(tmp), &written, QSE_NULL);

		WriteConsole (stderr, QSE_T("[EXPRESSION] "), 13, &written, QSE_NULL);
		WriteConsole (stderr, expr, qse_strlen(expr), &written, QSE_NULL);
		WriteConsole (stderr, QSE_T("\r\n"), 2, &written, QSE_NULL);

		if (desc)
		{
			WriteConsole (stderr, QSE_T("[DESCRIPTION] "), 14, &written, QSE_NULL);
			WriteConsole (stderr, desc, qse_strlen(desc), &written, QSE_NULL);
			WriteConsole (stderr, QSE_T("\r\n"), 2, &written, QSE_NULL);
		}
	}
#elif defined(__OS2__)
	HFILE stderr = (HFILE)2;
	ULONG written;
	qse_mchar_t tmp[1024];

	DosWrite (stderr, QSE_T("[ASSERTION FAILURE]\r\n"), 21, &written);

	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("FILE %hs LINE %lu\r\n"), file, (unsigned long)line);
	#else
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("FILE %ls LINE %lu\r\n"), file, (unsigned long)line);
	#endif
	DosWrite (stderr, tmp, qse_mbslen(tmp), &written);

	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[EXPRESSION] %hs\r\n"), expr);
	#else
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[EXPRESSION] %ls\r\n"), expr);
	#endif
	DosWrite (stderr, tmp, qse_mbslen(tmp), &written);

	if (desc)
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[DESCRIPTION] %hs\r\n"), desc);
	#else
		qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[DESCRIPTION] %ls\r\n"), desc);
	#endif
		DosWrite (stderr, tmp, qse_mbslen(tmp), &written);
	}

#elif defined(__DOS__)
	int stderr = 2;
	qse_mchar_t tmp[1024];

	write (stderr, QSE_T("[ASSERTION FAILURE]\r\n"), 21);

	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("FILE %hs LINE %lu\r\n"), file, (unsigned long)line);
	#else
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("FILE %ls LINE %lu\r\n"), file, (unsigned long)line);
	#endif
	write (stderr, tmp, qse_mbslen(tmp));

	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[EXPRESSION] %hs\r\n"), expr);
	#else
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[EXPRESSION] %ls\r\n"), expr);
	#endif
	write (stderr, tmp, qse_mbslen(tmp));

	if (desc)
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[DESCRIPTION] %hs\r\n"), desc);
	#else
		qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[DESCRIPTION] %ls\r\n"), desc);
	#endif
		write (stderr, tmp, qse_mbslen(tmp));
	}


#elif defined(macintosh)
	/* note 'desc' is not used for macintosh at this moment.
	 * TODO: include 'desc' in the message */
	Str255 ptitle;
	Str255 ptext;
	SInt16 res;

	qse_mchar_t tmp[256];

	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("ASSERTION FAILURE AT FILE %hs LINE %lu"), file, (unsigned long)line);
	#else
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("ASSERTION FAILURE AT FILE %ls LINE %lu"), file, (unsigned long)line);
	#endif
	CopyCStringToPascal (tmp, ptitle);

	#if defined(QSE_CHAR_IS_MCHAR)
	CopyCStringToPascal (expr, ptext);
	#else
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("%ls"), expr);
	CopyCStringToPascal (tmp, ptext);
	#endif

	InitCursor ();
	StandardAlert (kAlertStopAlert, ptitle, ptext, nil, &res);

/*
#elif defined(vms) || defined(__vms)
	WHAT TO DO????
*/


#else
	static qse_mchar_t* static_msg[] = 
	{
		QSE_MT("=[ASSERTION FAILURE]============================================================\n"),
		QSE_MT("                         __ \n"),
		QSE_MT(" _____ _____ _____ _____|  |\n"),
		QSE_MT("|     |     |  _  |   __|  |\n"),
		QSE_MT("|  |  |  |  |   __|__   |__|\n"),
		QSE_MT("|_____|_____|__|  |_____|__|\n"),
		QSE_MT("                            \n")
	};
	static qse_mchar_t* static_bthdr = QSE_MT("=[BACKTRACES]===================================================================\n");
	static qse_mchar_t* static_footer= QSE_MT("================================================================================\n");

	qse_mchar_t tmp[1024];
	qse_size_t i;

	#if defined(HAVE_BACKTRACE)
	void* btarray[128];
	qse_size_t btsize;
	char** btsyms;
	#endif

	for (i = 0; i < QSE_COUNTOF(static_msg); i++)
	{
		write (2, static_msg[i], qse_mbslen(static_msg[i]));
	}

	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("FILE %hs LINE %lu\n"), file, (unsigned long)line);
	#else
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("FILE %ls LINE %lu\n"), file, (unsigned long)line);
	#endif
	write (2, tmp, qse_mbslen(tmp));

	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[EXPRESSION] %hs\n"), expr);
	#else
	qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[EXPRESSION] %ls\n"), expr);
	#endif
	write (2, tmp, qse_mbslen(tmp));

	if (desc)
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[DESCRIPTION] %hs\n"), desc);
	#else
		qse_mbsxfmt (tmp, QSE_COUNTOF(tmp), QSE_MT("[DESCRIPTION] %ls\n"), desc);
	#endif
		write (2, tmp, qse_mbslen(tmp));
	}

	#if defined(HAVE_BACKTRACE)
	btsize = backtrace (btarray, QSE_COUNTOF(btarray));
	btsyms = backtrace_symbols (btarray, btsize);
	if (btsyms != QSE_NULL)
	{
		write (2, static_bthdr, qse_mbslen(static_bthdr));

		for (i = 0; i < btsize; i++)
		{
			write (2, btsyms[i], qse_mbslen(btsyms[i]));
			write (2, QSE_MT("\n"), 1);
		}

		free (btsyms);
	}
	#endif

	write (2, static_footer, qse_mbslen(static_footer));
#endif


#if defined(_WIN32)
	ExitProcess (249);
#elif defined(__OS2__)
	DosExit (EXIT_PROCESS, 249);
#elif defined(__DOS__)
	{
		union REGS regs;
		regs.h.ah = DOS_EXIT;
		regs.h.al = 249;
		intdos (&regs, &regs);
	}
#elif defined(vms) || defined(__vms)
	lib$stop (SS$_ABORT); /* use SS$_OPCCUS instead? */

	/* this won't be reached since lib$stop() terminates the process */
	sys$exit (SS$_ABORT); /* this condition code can be shown with 
	                       * 'show symbol $status' from the command-line. */
#elif defined(macintosh)
	ExitToShell ();
#else
	QSE_KILL (QSE_GETPID(), SIGABRT);
	QSE_EXIT (1);
#endif
}

