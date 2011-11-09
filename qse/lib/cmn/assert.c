/*
 * $Id: assert.c 223 2008-06-26 06:44:41Z baconevi $
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

#include <qse/types.h>
#include <qse/macros.h>

#ifndef NDEBUG

#include <qse/cmn/sio.h>

#ifdef HAVE_EXECINFO_H
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
#else
#	include "syscall.h"
#endif

#define NTOC(n) (QSE_T("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ")[n])
#define WRITE_CHAR(c) \
do { \
	qse_char_t __xxx_c = c; \
	if (qse_sio_putsn (QSE_SIO_ERR, &__xxx_c, 1) != 1) return -1; \
} while (0)

static int write_num (qse_size_t x, int base)
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
		WRITE_CHAR (NTOC(y % base));
		y = y / base;
		dig--;
	}

	while (dig > 0) 
	{ 
		dig--; 
		WRITE_CHAR (QSE_T('0'));
	}
	WRITE_CHAR (NTOC(last));
	return 0;
}

void qse_assert_failed (
	const qse_char_t* expr, const qse_char_t* desc, 
	const qse_char_t* file, qse_size_t line)
{
#ifdef HAVE_BACKTRACE
	void *btarray[128];
	qse_size_t btsize, i;
	char **btsyms;

#ifdef QSE_CHAR_IS_WCHAR
	qse_wchar_t wcs[256];
#endif

#endif

	qse_sio_puts (QSE_SIO_ERR, QSE_T("=[ASSERTION FAILURE]============================================================\n"));

#if 1
	qse_sio_puts (QSE_SIO_ERR, QSE_T("                         __ \n"));
	qse_sio_puts (QSE_SIO_ERR, QSE_T(" _____ _____ _____ _____|  |\n"));
	qse_sio_puts (QSE_SIO_ERR, QSE_T("|     |     |  _  |   __|  |\n"));
	qse_sio_puts (QSE_SIO_ERR, QSE_T("|  |  |  |  |   __|__   |__|\n"));
	qse_sio_puts (QSE_SIO_ERR, QSE_T("|_____|_____|__|  |_____|__|\n"));
#else
	qse_sio_puts (QSE_SIO_ERR, QSE_T("                            __ \n"));
	qse_sio_puts (QSE_SIO_ERR, QSE_T(" _____ _____ _____ _____   |  |\n"));
	qse_sio_puts (QSE_SIO_ERR, QSE_T("|     |     |  _  |   __|  |  |\n"));
	qse_sio_puts (QSE_SIO_ERR, QSE_T("|  |  |  |  |   __|__   |  |__|\n"));
	qse_sio_puts (QSE_SIO_ERR, QSE_T("|_____|_____|__|  |_____|  |__|\n"));
#endif

	qse_sio_puts (QSE_SIO_ERR, QSE_T("FILE "));
	qse_sio_puts (QSE_SIO_ERR, file);
	qse_sio_puts (QSE_SIO_ERR, QSE_T(" LINE "));

	write_num (line, 10);

	qse_sio_puts (QSE_SIO_ERR, QSE_T(": "));
	qse_sio_puts (QSE_SIO_ERR, expr);
	qse_sio_puts (QSE_SIO_ERR, QSE_T("\n"));

	if (desc != QSE_NULL)
	{
		qse_sio_puts (QSE_SIO_ERR, QSE_T("DESCRIPTION: "));
		qse_sio_puts (QSE_SIO_ERR, desc);
		qse_sio_puts (QSE_SIO_ERR, QSE_T("\n"));
	}

#ifdef HAVE_BACKTRACE
	btsize = backtrace (btarray, QSE_COUNTOF(btarray));
	btsyms = backtrace_symbols (btarray, btsize);
	if (btsyms != QSE_NULL)
	{
		qse_sio_puts (QSE_SIO_ERR, QSE_T("=[BACKTRACES]===================================================================\n"));

		for (i = 0; i < btsize; i++)
		{
		#ifdef QSE_CHAR_IS_MCHAR
			qse_sio_puts (QSE_SIO_ERR, btsyms[i]);
		#else
			qse_size_t wcslen = QSE_COUNTOF(wcs);
			qse_mbstowcs (btsyms[i], wcs, &wcslen);
			qse_sio_puts (QSE_SIO_ERR, wcs);
		#endif
			qse_sio_puts (QSE_SIO_ERR, QSE_T("\n"));
		}

		free (btsyms);
	}
#endif

	qse_sio_puts (QSE_SIO_ERR, QSE_T("================================================================================\n"));
	qse_sio_flush (QSE_SIO_ERR);

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
#else
	QSE_KILL (QSE_GETPID(), SIGABRT);
	QSE_EXIT (1);
#endif
}

#endif

