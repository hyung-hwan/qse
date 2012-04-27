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
#include "mem.h"

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
#elif defined(vms) || defined(__vms)
#	include <starlet.h> /* (SYS$...) */
#	include <ssdef.h> /* (SS$...) */
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
#ifdef HAVE_BACKTRACE
	void *btarray[128];
	qse_size_t btsize, i;
	char **btsyms;
#endif
	qse_sio_t* sio, siobuf;

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

#ifdef HAVE_BACKTRACE
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
	sys$exit (SS$_ABORT); /* this condition code can be shown with 
	                       * 'show symbol $status' from the command-line. */
#else
	QSE_KILL (QSE_GETPID(), SIGABRT);
	QSE_EXIT (1);
#endif
}

#endif

