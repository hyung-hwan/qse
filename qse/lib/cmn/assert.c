/*
 * $Id: assert.c 223 2008-06-26 06:44:41Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

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

#include <qse/types.h>
#include <qse/macros.h>

#ifndef NDEBUG

#include <qse/cmn/sio.h>

#ifdef _WIN32
#       include <windows.h>
#else
#	include "syscall.h"
#endif

#define NTOC(n) (((n) >= 10)? (((n) - 10) + QSE_T('A')): (n) + QSE_T('0'))
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
	qse_sio_puts (QSE_SIO_ERR, QSE_T("=[ASSERTION FAILURE]============================================================\n"));

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
	qse_sio_puts (QSE_SIO_ERR, QSE_T("================================================================================\n"));
	qse_sio_flush (QSE_SIO_ERR);

#ifdef _WIN32
	ExitProcess (1);
#else
	QSE_KILL (QSE_GETPID(), SIGABRT);
	QSE_EXIT (1);
#endif
}

#endif

