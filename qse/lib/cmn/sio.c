/*
 * $Id: sio.c 573 2011-09-21 05:50:23Z hyunghwan.chung $
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

#include <qse/cmn/sio.h>
#include "mem.h"

static qse_ssize_t __sio_input (qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size);
static qse_ssize_t __sio_output (qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size);

#if defined(_WIN32)
#	include <windows.h> /* for the UGLY hack */
#endif

qse_sio_t* qse_sio_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, const qse_char_t* file, int flags)
{
	qse_sio_t* sio;

	sio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_sio_t) + xtnsize);
	if (sio == QSE_NULL) return QSE_NULL;

	if (qse_sio_init (sio, mmgr, file, flags) <= -1)
	{
		QSE_MMGR_FREE (mmgr, sio);
		return QSE_NULL;
	}

	return sio;
}

qse_sio_t* qse_sio_openstd (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;
	qse_fio_hnd_t hnd;

	if (qse_getstdfiohandle (std, &hnd) <= -1) return QSE_NULL;
	sio = qse_sio_open (mmgr, xtnsize, 
		(const qse_char_t*)&hnd, flags | QSE_SIO_HANDLE | QSE_SIO_NOCLOSE);

#if defined(_WIN32)
	if (sio) 
	{
		QSE_ASSERT (std >= 0 && std < QSE_TYPE_MAX(int));
		sio->status = std + 1;
	}
#endif

	return sio;
}

void qse_sio_close (qse_sio_t* sio)
{
	qse_sio_fini (sio);
	QSE_MMGR_FREE (sio->mmgr, sio);
}

int qse_sio_init (
	qse_sio_t* sio, qse_mmgr_t* mmgr, const qse_char_t* file, int flags)
{
	int mode;
	int topt = 0;

	QSE_MEMSET (sio, 0, QSE_SIZEOF(*sio));
	sio->mmgr = mmgr;

	mode = QSE_FIO_RUSR | QSE_FIO_WUSR | 
	       QSE_FIO_RGRP | QSE_FIO_ROTH;

	/* sio flags redefines most fio flags. fio can be opened in the
	 * text mode. that way, fio is also buffered. since sio performs
	 * its own buffering, i don't want a caller to specify text mode
	 * flags accidentally. i mask off those bits here to avoid mishap. */
	if (qse_fio_init (&sio->fio, mmgr, file, 
		(flags & ~(QSE_FIO_TEXT|QSE_FIO_NOAUTOFLUSH)), mode) <= -1) return -1;

	if (flags & QSE_SIO_IGNOREMBWCERR) topt |= QSE_TIO_IGNOREMBWCERR;
	if (flags & QSE_SIO_NOAUTOFLUSH) topt |= QSE_TIO_NOAUTOFLUSH;

	if (qse_tio_init(&sio->tio, mmgr, topt) <= -1)
	{
		qse_fio_fini (&sio->fio);
		return -1;
	}

	if (qse_tio_attachin (&sio->tio, __sio_input, sio, sio->inbuf, QSE_COUNTOF(sio->inbuf)) <= -1 ||
	    qse_tio_attachout (&sio->tio, __sio_output, sio, sio->outbuf, QSE_COUNTOF(sio->outbuf)) <= -1)
	{
		qse_tio_fini (&sio->tio);	
		qse_fio_fini (&sio->fio);
		return -1;
	}

	return 0;
}

int qse_sio_initstd (
	qse_sio_t* sio, qse_mmgr_t* mmgr, qse_sio_std_t std, int flags)
{
	int n;
	qse_fio_hnd_t hnd;

	if (qse_getstdfiohandle (std, &hnd) <= -1) return -1;
	n = qse_sio_init (sio, mmgr, 
		(const qse_char_t*)&hnd, flags | QSE_SIO_HANDLE | QSE_SIO_NOCLOSE);

#if defined(_WIN32)
	if (n >= 0) 
	{
		QSE_ASSERT (std >= 0 && std < QSE_TYPE_MAX(int));
		sio->status = std + 1;
	}
#endif

	return n;
}

void qse_sio_fini (qse_sio_t* sio)
{
	/*if (qse_sio_flush (sio) <= -1) return -1;*/
	qse_sio_flush (sio);
	qse_tio_fini (&sio->tio);
	qse_fio_fini (&sio->fio);
}

qse_sio_errnum_t qse_sio_geterrnum (qse_sio_t* sio)
{
	return QSE_TIO_ERRNUM(&sio->tio);
}

qse_sio_hnd_t qse_sio_gethandle (qse_sio_t* sio)
{
	/*return qse_fio_gethandle (&sio->fio);*/
	return QSE_FIO_HANDLE(&sio->fio);
}

qse_ssize_t qse_sio_flush (qse_sio_t* sio)
{
	return qse_tio_flush (&sio->tio);
}

void qse_sio_purge (qse_sio_t* sio)
{
	qse_tio_purge (&sio->tio);
}

qse_ssize_t qse_sio_getmb (qse_sio_t* sio, qse_mchar_t* c)
{
	return qse_tio_readmbs (&sio->tio, c, 1);
}

qse_ssize_t qse_sio_getwc (qse_sio_t* sio, qse_wchar_t* c)
{
	return qse_tio_readwcs (&sio->tio, c, 1);
}

qse_ssize_t qse_sio_getmbs (
	qse_sio_t* sio, qse_mchar_t* buf, qse_size_t size)
{
	qse_ssize_t n;

	if (size <= 0) return 0;

#if defined(_WIN32)
	/* Using ReadConsoleA() didn't help at all.
	 * so I don't implement any hack here */
#endif

	n = qse_tio_readmbs (&sio->tio, buf, size - 1);

	if (n <= -1) return -1;
	buf[n] = QSE_MT('\0');
	return n;
}

qse_ssize_t qse_sio_getmbsn (
	qse_sio_t* sio, qse_mchar_t* buf, qse_size_t size)
{
#if defined(_WIN32)
	/* Using ReadConsoleA() didn't help at all.
	 * so I don't implement any hack here */
#endif

	return qse_tio_readmbs (&sio->tio, buf, size);
}

qse_ssize_t qse_sio_getwcs (
	qse_sio_t* sio, qse_wchar_t* buf, qse_size_t size)
{
	qse_ssize_t n;

	if (size <= 0) return 0;

#if defined(_WIN32)
	/* Using ReadConsoleA() didn't help at all.
	 * so I don't implement any hack here */
#endif

	n = qse_tio_readwcs (&sio->tio, buf, size - 1);

	if (n <= -1) return -1;
	buf[n] = QSE_WT('\0');
	return n;
}

qse_ssize_t qse_sio_getwcsn (
	qse_sio_t* sio, qse_wchar_t* buf, qse_size_t size)
{
#if defined(_WIN32)
	/* Using ReadConsoleW() didn't help at all.
	 * so I don't implement any hack here */
#endif
	return qse_tio_readwcs (&sio->tio, buf, size);
}

qse_ssize_t qse_sio_putmb (qse_sio_t* sio, qse_mchar_t c)
{
	return qse_tio_writembs (&sio->tio, &c, 1);
}

qse_ssize_t qse_sio_putwc (qse_sio_t* sio, qse_wchar_t c)
{
	return qse_tio_writewcs (&sio->tio, &c, 1);
}

qse_ssize_t qse_sio_putmbs (qse_sio_t* sio, const qse_mchar_t* str)
{
#if defined(_WIN32)
	/* Using WriteConsoleA() didn't help at all.
	 * so I don't implement any hack here */
#endif

	return qse_tio_writembs (&sio->tio, str, (qse_size_t)-1);
}

qse_ssize_t qse_sio_putmbsn (
	qse_sio_t* sio, const qse_mchar_t* str, qse_size_t size)
{
#if defined(_WIN32)
	/* Using WriteConsoleA() didn't help at all.
	 * so I don't implement any hack here */
#endif

	return qse_tio_writembs (&sio->tio, str, size);
}

qse_ssize_t qse_sio_putwcs (qse_sio_t* sio, const qse_wchar_t* str)
{
#if defined(_WIN32)
	/* DAMN UGLY: See comment in qse_sio_putwcsn() */
	if (sio->status)
	{
		DWORD mode;

		if (GetConsoleMode (sio->fio.handle, &mode) == FALSE)
		{
			return qse_tio_writewcs (&sio->tio, str, (qse_size_t)-1);
		}
		else
		{
			DWORD count, left;
			const qse_wchar_t* cur;
			if (qse_sio_flush (sio) <= -1) return -1; /* can't do buffering */
			for (cur = str, left = qse_wcslen(str); left > 0; cur += count, left -= count)
			{
				if (WriteConsoleW (
					sio->fio.handle, cur, left,
					&count, QSE_NULL) == FALSE) return -1;
				if (count == 0) break;
			}
			return cur - str;	
		}
	}
#endif

	return qse_tio_writewcs (&sio->tio, str, (qse_size_t)-1);
}

qse_ssize_t qse_sio_putwcsn (
	qse_sio_t* sio, const qse_wchar_t* str, qse_size_t size)
{
#if defined(_WIN32)
	/* DAMN UGLY:
	 *  WriteFile returns wrong number of bytes written if it is 
	 *  requested to write a utf8 string on utf8 console (codepage 65001). 
	 *  it seems to return a number of characters written instead. so 
	 *  i have to use an alternate API for console output for 
	 *  wide-character strings. Conversion to either an OEM codepage or 
	 *  the utf codepage is handled by the API. This hack at least
	 *  lets you do proper utf8 output on utf8 console using wide-character.
	 * 
	 *  Note that the multibyte functions qse_sio_putmbs() and
	 *  qse_sio_putmbsn() doesn't handle this. So you may still suffer.
	 */
	if (sio->status)
	{
		DWORD mode;

		if (GetConsoleMode (sio->fio.handle, &mode) == FALSE)
		{
			return qse_tio_writewcs (&sio->tio, str, size);
		}
		else
		{
			DWORD count, left;
			const qse_wchar_t* cur;
	
			if (qse_sio_flush (sio) <= -1) return -1; /* can't do buffering */
			for (cur = str, left = size; left > 0; cur += count, left -= count)
			{
				if (WriteConsoleW (
					sio->fio.handle, cur, left, 
					&count, QSE_NULL) == FALSE) return -1;
				if (count == 0) break;
			}
			return cur - str;
		}
	}	
#endif

	return qse_tio_writewcs (&sio->tio, str, size);
}

int qse_sio_getpos (qse_sio_t* sio, qse_sio_pos_t* pos)
{
	qse_fio_off_t off;

	off = qse_fio_seek (&sio->fio, 0, QSE_FIO_CURRENT);
	if (off == (qse_fio_off_t)-1) return -1;

	*pos = off;
	return 0;
}

int qse_sio_setpos (qse_sio_t* sio, qse_sio_pos_t pos)
{
   	qse_fio_off_t off;

	if (qse_sio_flush(sio) <= -1) return -1;
	off = qse_fio_seek (&sio->fio, pos, QSE_FIO_BEGIN);

	return (off == (qse_fio_off_t)-1)? -1: 0;
}

#if 0
int qse_sio_seek (qse_sio_t* sio, qse_sio_seek_t pos)
{
	/* TODO: write this function - more flexible positioning ....
	 *       can move to the end of the stream also.... */

	if (qse_sio_flush(sio) <= -1) return -1;
	return (qse_fio_seek (&sio->fio, 
		0, QSE_FIO_END) == (qse_fio_off_t)-1)? -1: 0;

	/* TODO: write this function */
	if (qse_sio_flush(sio) <= -1) return -1;
	return (qse_fio_seek (&sio->fio, 
		0, QSE_FIO_BEGIN) == (qse_fio_off_t)-1)? -1: 0;

}
#endif

static qse_ssize_t __sio_input (
	qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size)
{
	qse_sio_t* sio = (qse_sio_t*)arg;

	QSE_ASSERT (sio != QSE_NULL);

	if (cmd == QSE_TIO_DATA) 
	{
		return qse_fio_read (&sio->fio, buf, size);
	}

	return 0;
}

static qse_ssize_t __sio_output (
	qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size)
{
	qse_sio_t* sio = (qse_sio_t*)arg;

	QSE_ASSERT (sio != QSE_NULL);

	if (cmd == QSE_TIO_DATA) 
	{
		return qse_fio_write (&sio->fio, buf, size);
	}

	return 0;
}

