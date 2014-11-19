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
#include <qse/cmn/sio.h>
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

#define NTOC(n) (QSE_MT("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ")[n])
#define WRITE_CHAR(sio,c) \
do { \
	qse_mchar_t __xxx_c = c; \
	if (qse_sio_putmbsn (sio, &__xxx_c, 1) != 1) return -1; \
} while (0)

static int write_num (qse_sio_t* sio, qse_size_t x, int base)
{
	qse_size_t last = x % base;
	qse_size_t y = 0; 
	int dig = 0;

	x = x / base;

	while (x > 0)
	{
		y = y * base + (x % base);
		x = x / base;
		dig++;
	}

	while (y > 0)
	{
		WRITE_CHAR (sio, NTOC(y % base));
		y = y / base;
		dig--;
	}

	while (dig > 0) 
	{ 
		dig--; 
		WRITE_CHAR (sio, QSE_T('0'));
	}
	WRITE_CHAR (sio, NTOC(last));
	return 0;
}

void qse_assert_failed (
	const qse_char_t* expr, const qse_char_t* desc, 
	const qse_char_t* file, qse_size_t line)
{
#if defined(macintosh)
	/* note 'desc' is not used for macintosh at this moment.
	 * TODO: include 'desc' in the message */

	Str255 ptitle;
	Str255 ptext;
	SInt16 res;

	{
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
	}

	InitCursor ();
	StandardAlert (kAlertStopAlert, ptitle, ptext, nil, &res);

#else  /* macintosh */


#if defined(HAVE_BACKTRACE)
	void* btarray[128];
	qse_size_t btsize, i;
	char** btsyms;
#endif
	qse_sio_t* sio, siobuf;


	#if defined(_WIN32)
	{
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

			qse_char_t tmp[512];
			qse_strxfmt (tmp, QSE_COUNTOF(tmp), 
				QSE_T("FILE %s LINE %lu - %s%s%s"), 
				file, line, expr, 
				(desc? QSE_T("\n\n"): QSE_T("")),
				(desc? desc: QSE_T(""))
			);
			MessageBox (QSE_NULL, tmp, QSE_T("ASSERTION FAILURE"), MB_OK | MB_ICONERROR);
			goto done;
		}
	}
	#endif

	sio = &siobuf;
	qse_sio_initstd (
		sio, QSE_MMGR_GETDFL(), QSE_SIO_STDERR, 
		QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR | QSE_SIO_NOAUTOFLUSH);

	qse_sio_putmbs (sio, QSE_MT("=[ASSERTION FAILURE]============================================================\n"));

#if 1
	qse_sio_putmbs (sio, QSE_MT("                         __ \n"));
	qse_sio_putmbs (sio, QSE_MT(" _____ _____ _____ _____|  |\n"));
	qse_sio_putmbs (sio, QSE_MT("|     |     |  _  |   __|  |\n"));
	qse_sio_putmbs (sio, QSE_MT("|  |  |  |  |   __|__   |__|\n"));
	qse_sio_putmbs (sio, QSE_MT("|_____|_____|__|  |_____|__|\n"));
	qse_sio_putmbs (sio, QSE_MT("                            \n"));
#else
	qse_sio_putmbs (sio, QSE_MT("                            __ \n"));
	qse_sio_putmbs (sio, QSE_MT(" _____ _____ _____ _____   |  |\n"));
	qse_sio_putmbs (sio, QSE_MT("|     |     |  _  |   __|  |  |\n"));
	qse_sio_putmbs (sio, QSE_MT("|  |  |  |  |   __|__   |  |__|\n"));
	qse_sio_putmbs (sio, QSE_MT("|_____|_____|__|  |_____|  |__|\n"));
	qse_sio_putmbs (sio, QSE_MT("                            __ \n"));
#endif

	qse_sio_putmbs (sio, QSE_MT("FILE: "));
	qse_sio_putstr (sio, file);
	qse_sio_putmbs (sio, QSE_MT(" LINE: "));

	write_num (sio, line, 10);

	qse_sio_putmbs (sio, QSE_MT("\nEXPRESSION: "));
	qse_sio_putstr (sio, expr);
	qse_sio_putmbs (sio, QSE_MT("\n"));

	if (desc != QSE_NULL)
	{
		qse_sio_putmbs (sio, QSE_MT("DESCRIPTION: "));
		qse_sio_putstr (sio, desc);
		qse_sio_putmbs (sio, QSE_MT("\n"));
	}

#if defined(HAVE_BACKTRACE)
	btsize = backtrace (btarray, QSE_COUNTOF(btarray));
	btsyms = backtrace_symbols (btarray, btsize);
	if (btsyms != QSE_NULL)
	{
		qse_sio_putmbs (sio, QSE_MT("=[BACKTRACES]===================================================================\n"));

		for (i = 0; i < btsize; i++)
		{
			qse_sio_putmbs (sio, btsyms[i]);
			qse_sio_putmbs (sio, QSE_MT("\n"));
		}

		free (btsyms);
	}
#endif

	qse_sio_putmbs (sio, QSE_MT("================================================================================\n"));
	qse_sio_flush (sio);
	qse_sio_fini (sio);
#endif /* macintosh */

done:
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

