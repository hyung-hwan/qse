/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#include <qse/si/intr.h>

#if defined(_WIN32)
#	include <windows.h>

#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>

#elif defined(__DOS__)
#	include <dos.h>
#	include <signal.h>
#else
#	include <unistd.h>
#	include <signal.h>
#	include <errno.h>
#endif

static qse_intr_handler_t intr_handler = QSE_NULL;
static void* intr_handler_arg = QSE_NULL;

#if defined(_WIN32)
static BOOL WINAPI __intr_handler (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		if (intr_handler) intr_handler (intr_handler_arg);
		return TRUE;
	}

	return FALSE;
}

#elif defined(__OS2__)

static EXCEPTIONREGISTRATIONRECORD os2_excrr = { 0 };

static ULONG _System __intr_handler (
	PEXCEPTIONREPORTRECORD p1,
	PEXCEPTIONREGISTRATIONRECORD p2,
	PCONTEXTRECORD p3,
	PVOID pv)
{
	if (p1->ExceptionNum == XCPT_SIGNAL)
	{
		if (p1->ExceptionInfo[0] == XCPT_SIGNAL_INTR ||
		    p1->ExceptionInfo[0] == XCPT_SIGNAL_KILLPROC ||
		    p1->ExceptionInfo[0] == XCPT_SIGNAL_BREAK)
		 {
			APIRET rc;

			if (intr_handler) intr_handler (intr_handler_arg);

			rc = DosAcknowledgeSignalException (p1->ExceptionInfo[0]);
			return (rc != NO_ERROR)? 1: XCPT_CONTINUE_EXECUTION;
		 }
	}

	return XCPT_CONTINUE_SEARCH; /* exception not resolved */
}

#elif defined(__DOS__)

static void __intr_handler (void)
{
	if (intr_handler) intr_handler (intr_handler_arg);
}

#else

static void __intr_handler (int sig)
{
	if (intr_handler) intr_handler (intr_handler_arg);
}

static int setsignal (int sig, void(*handler)(int), int restart)
{
	struct sigaction sa_int;

	sa_int.sa_handler = handler;
	sigemptyset (&sa_int.sa_mask);

	sa_int.sa_flags = 0;

	if (restart)
	{
	#if defined(SA_RESTART)
		sa_int.sa_flags |= SA_RESTART;
	#endif
	}
	else
	{
	#if defined(SA_INTERRUPT)
		sa_int.sa_flags |= SA_INTERRUPT;
	#endif
	}
	return sigaction (sig, &sa_int, NULL);
}
#endif

void qse_set_intr_handler (qse_intr_handler_t handler, void* arg)
{
	intr_handler = handler;
	intr_handler_arg = arg;

#if defined(_WIN32)
	SetConsoleCtrlHandler (__intr_handler, TRUE);
#elif defined(__OS2__)
	os2_excrr.ExceptionHandler = (ERR)__intr_handler;
	DosSetExceptionHandler (&os2_excrr); /* TODO: check if NO_ERROR is returned */
#elif defined(__DOS__)
	signal (SIGINT, __intr_handler);
#else
	/*setsignal (SIGINT, __intr_handler, 1); TO BE MORE COMPATIBLE WITH WIN32*/
	setsignal (SIGINT, __intr_handler, 0);
#endif
}

void qse_clear_intr_handler (void)
{
	intr_handler = QSE_NULL;
	intr_handler_arg = QSE_NULL;

#if defined(_WIN32)
	SetConsoleCtrlHandler (__intr_handler, FALSE);
#elif defined(__OS2__)
	DosUnsetExceptionHandler (&os2_excrr);
#elif defined(__DOS__)
	signal (SIGINT, SIG_DFL);
#else
	setsignal (SIGINT, SIG_DFL, 1);
#endif
}
