/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#if defined(_WIN32)
#	include <windows.h> /* for the UGLY hack */
#elif defined(__OS2__)
	/* nothing */
#elif defined(__DOS__)
	/* nothing */
#else
#	include "syscall.h"
#endif

/* internal status codes */
enum
{
	STATUS_UTF8_CONSOLE = (1 << 0)
};


static qse_ssize_t file_input (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size);
static qse_ssize_t file_output (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size);

static qse_sio_errnum_t fio_errnum_to_sio_errnum (qse_fio_t* fio)
{
	switch (fio->errnum)
	{
		case QSE_FIO_ENOMEM:
			return QSE_SIO_ENOMEM;
		case QSE_FIO_EINVAL:
			return QSE_SIO_EINVAL;
		case QSE_FIO_EACCES:
			return QSE_SIO_EACCES;
		case QSE_FIO_ENOENT:
			return QSE_SIO_ENOENT;
		case QSE_FIO_EEXIST:
			return QSE_SIO_EEXIST;
		case QSE_FIO_EINTR:
			return QSE_SIO_EINTR;
		case QSE_FIO_EPIPE:
			return QSE_SIO_EPIPE;
		case QSE_FIO_EAGAIN:
			return QSE_SIO_EAGAIN;
		case QSE_FIO_ESYSERR:
			return QSE_SIO_ESYSERR;
		case QSE_FIO_ENOIMPL:
			return QSE_SIO_ENOIMPL;
		default:
			return QSE_SIO_EOTHER;
	}
}

static qse_sio_errnum_t tio_errnum_to_sio_errnum (qse_tio_t* tio)
{
	switch (tio->errnum)
	{
		case QSE_TIO_ENOMEM:
			return QSE_SIO_ENOMEM;
		case QSE_TIO_EINVAL:
			return QSE_SIO_EINVAL;
		case QSE_TIO_EACCES:
			return QSE_SIO_EACCES;
		case QSE_TIO_ENOENT:
			return QSE_SIO_ENOENT;
		case QSE_TIO_EILSEQ:
			return QSE_SIO_EILSEQ;
		case QSE_TIO_EICSEQ:
			return QSE_SIO_EICSEQ;
		case QSE_TIO_EILCHR:
			return QSE_SIO_EILCHR;
		default:
			return QSE_SIO_EOTHER;
	}
}

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

	QSE_MEMSET (sio + 1, 0, xtnsize);
	return sio;
}

qse_sio_t* qse_sio_openstd (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;
	qse_fio_hnd_t hnd;

	/* Is this necessary?
	if (flags & QSE_SIO_KEEPATH)
	{
		sio->errnum = QSE_SIO_EINVAL;
		return QSE_NULL;
	}
	*/

	if (qse_getstdfiohandle (std, &hnd) <= -1) return QSE_NULL;

	sio = qse_sio_open (mmgr, xtnsize, 
		(const qse_char_t*)&hnd, flags | QSE_SIO_HANDLE | QSE_SIO_NOCLOSE);

#if defined(_WIN32)
	if (sio) 
	{
		DWORD mode;
		if (GetConsoleMode (sio->file.handle, &mode) == TRUE &&
		    GetConsoleOutputCP() == CP_UTF8)
		{
			sio->status |= STATUS_UTF8_CONSOLE;
		}
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
	qse_sio_t* sio, qse_mmgr_t* mmgr, const qse_char_t* path, int flags)
{
	int mode;
	int topt = 0;

	QSE_MEMSET (sio, 0, QSE_SIZEOF(*sio));
	sio->mmgr = mmgr;

	mode = QSE_FIO_RUSR | QSE_FIO_WUSR | 
	       QSE_FIO_RGRP | QSE_FIO_ROTH;

	/* sio flag enumerators redefines most fio flag enumerators and 
	 * compose a superset of fio flag enumerators. when a user calls 
	 * this function, a user can specify a sio flag enumerator not 
	 * present in the fio flag enumerator. mask off such an enumerator. */
	if (qse_fio_init (
		&sio->file, mmgr, path, 
		(flags & ~QSE_FIO_RESERVED), mode) <= -1) 
	{
		sio->errnum = fio_errnum_to_sio_errnum (&sio->file);
		goto oops00;
	}

	if (flags & QSE_SIO_IGNOREMBWCERR) topt |= QSE_TIO_IGNOREMBWCERR;
	if (flags & QSE_SIO_NOAUTOFLUSH) topt |= QSE_TIO_NOAUTOFLUSH;

	if ((flags & QSE_SIO_KEEPPATH) && !(flags & QSE_SIO_HANDLE))
	{
		sio->path = qse_strdup (path, sio->mmgr);
		if (sio->path == QSE_NULL)
		{
			sio->errnum = QSE_SIO_ENOMEM;
			goto oops01;
		}
	}

	if (qse_tio_init(&sio->tio.io, mmgr, topt) <= -1)
	{
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);
		goto oops02;
	}
	/* store the back-reference to sio in the extension area.*/
	QSE_ASSERT (QSE_XTN(&sio->tio.io) == &sio->tio.xtn);
	*(qse_sio_t**)QSE_XTN(&sio->tio.io) = sio;

	if (qse_tio_attachin (&sio->tio.io, file_input, sio->inbuf, QSE_COUNTOF(sio->inbuf)) <= -1 ||
	    qse_tio_attachout (&sio->tio.io, file_output, sio->outbuf, QSE_COUNTOF(sio->outbuf)) <= -1)
	{
		if (sio->errnum == QSE_SIO_ENOERR) 
			sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);
		goto oops03;
	}

	return 0;

oops03:
	qse_tio_fini (&sio->tio.io);	
oops02:
	if (sio->path) QSE_MMGR_FREE (sio->mmgr, sio->path);
oops01:
	qse_fio_fini (&sio->file);
oops00:
	return -1;
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
		DWORD mode;
		if (GetConsoleMode (sio->file.handle, &mode) == TRUE &&
		    GetConsoleOutputCP() == CP_UTF8)
		{
			sio->status |= STATUS_UTF8_CONSOLE;
		}
	}
#endif

	return n;
}

void qse_sio_fini (qse_sio_t* sio)
{
	/*if (qse_sio_flush (sio) <= -1) return -1;*/
	qse_sio_flush (sio);
	qse_tio_fini (&sio->tio.io);
	qse_fio_fini (&sio->file);
	if (sio->path) QSE_MMGR_FREE (sio->mmgr, sio->path);
}

qse_mmgr_t* qse_sio_getmmgr (qse_sio_t* sio)
{
	return sio->mmgr;
}

void* qse_sio_getxtn (qse_sio_t* sio)
{
	return QSE_XTN (sio);
}

qse_sio_errnum_t qse_sio_geterrnum (const qse_sio_t* sio)
{
	return sio->errnum;
}

qse_cmgr_t* qse_sio_getcmgr (qse_sio_t* sio)
{
	return qse_tio_getcmgr (&sio->tio.io);
}

void qse_sio_setcmgr (qse_sio_t* sio, qse_cmgr_t* cmgr)
{
	qse_tio_setcmgr (&sio->tio.io, cmgr);
}

qse_sio_hnd_t qse_sio_gethandle (const qse_sio_t* sio)
{
	/*return qse_fio_gethandle (&sio->file);*/
	return QSE_FIO_HANDLE(&sio->file);
}

qse_ubi_t qse_sio_gethandleasubi (const qse_sio_t* sio)
{
	return qse_fio_gethandleasubi (&sio->file);
}

const qse_char_t* qse_sio_getpath (qse_sio_t* sio)
{
	/* this path is valid if QSE_SIO_HANDLE is off and QSE_SIO_KEEPPATH is on */
	return sio->path;
}

qse_ssize_t qse_sio_flush (qse_sio_t* sio)
{
	qse_ssize_t n;

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_flush (&sio->tio.io);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);
	return n;
}

void qse_sio_purge (qse_sio_t* sio)
{
	qse_tio_purge (&sio->tio.io);
}

qse_ssize_t qse_sio_getmb (qse_sio_t* sio, qse_mchar_t* c)
{
	qse_ssize_t n;

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_readmbs (&sio->tio.io, c, 1);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);

	return n;
}

qse_ssize_t qse_sio_getwc (qse_sio_t* sio, qse_wchar_t* c)
{
	qse_ssize_t n;

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_readwcs (&sio->tio.io, c, 1);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);

	return n;
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

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_readmbs (&sio->tio.io, buf, size - 1);
	if (n <= -1) 
	{
		if (sio->errnum == QSE_SIO_ENOERR)
			sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);
		return -1;
	}
	buf[n] = QSE_MT('\0');
	return n;
}

qse_ssize_t qse_sio_getmbsn (
	qse_sio_t* sio, qse_mchar_t* buf, qse_size_t size)
{
	qse_ssize_t n;
#if defined(_WIN32)
	/* Using ReadConsoleA() didn't help at all.
	 * so I don't implement any hack here */
#endif

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_readmbs (&sio->tio.io, buf, size);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);
	return n;
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

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_readwcs (&sio->tio.io, buf, size - 1);
	if (n <= -1) 
	{
		if (sio->errnum == QSE_SIO_ENOERR)
			sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);
		return -1;
	}
	buf[n] = QSE_WT('\0');
	return n;
}

qse_ssize_t qse_sio_getwcsn (
	qse_sio_t* sio, qse_wchar_t* buf, qse_size_t size)
{
	qse_ssize_t n;

#if defined(_WIN32)
	/* Using ReadConsoleW() didn't help at all.
	 * so I don't implement any hack here */
#endif

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_readwcs (&sio->tio.io, buf, size);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);

	return n;
}

qse_ssize_t qse_sio_putmb (qse_sio_t* sio, qse_mchar_t c)
{
	qse_ssize_t n;

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_writembs (&sio->tio.io, &c, 1);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);

	return n;
}

qse_ssize_t qse_sio_putwc (qse_sio_t* sio, qse_wchar_t c)
{
	qse_ssize_t n;

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_writewcs (&sio->tio.io, &c, 1);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);

	return n;
}

qse_ssize_t qse_sio_putmbs (qse_sio_t* sio, const qse_mchar_t* str)
{
	qse_ssize_t n;

#if defined(_WIN32)
	/* Using WriteConsoleA() didn't help at all.
	 * so I don't implement any hacks here */
#endif

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_writembs (&sio->tio.io, str, (qse_size_t)-1);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);

	return n;
}

qse_ssize_t qse_sio_putmbsn (
	qse_sio_t* sio, const qse_mchar_t* str, qse_size_t size)
{
	qse_ssize_t n;

#if defined(_WIN32)
	/* Using WriteConsoleA() didn't help at all.
	 * so I don't implement any hacks here */
#endif

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_writembs (&sio->tio.io, str, size);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);

	return n;
}

qse_ssize_t qse_sio_putwcs (qse_sio_t* sio, const qse_wchar_t* str)
{
	qse_ssize_t n;

#if defined(_WIN32)
	/* DAMN UGLY: See comment in qse_sio_putwcsn() */
	if (sio->status & STATUS_UTF8_CONSOLE)
	{
		DWORD count, left;
		const qse_wchar_t* cur;

		if (qse_sio_flush (sio) <= -1) return -1; /* can't do buffering */

		for (cur = str, left = qse_wcslen(str); left > 0; cur += count, left -= count)
		{
			if (WriteConsoleW (
				sio->file.handle, cur, left,
				&count, QSE_NULL) == FALSE) 
			{
				sio->errnum = QSE_SIO_ESYSERR;
				return -1;
			}
			if (count == 0) break;

			if (count > left) 
			{
				sio->errnum = QSE_SIO_ESYSERR;
				return -1;
			}
		}
		return cur - str;	
	}
#endif

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_writewcs (&sio->tio.io, str, (qse_size_t)-1);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);
	return n;
}

qse_ssize_t qse_sio_putwcsn (
	qse_sio_t* sio, const qse_wchar_t* str, qse_size_t size)
{
	qse_ssize_t n;

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
	if (sio->status & STATUS_UTF8_CONSOLE)
	{
		DWORD count, left;
		const qse_wchar_t* cur;

		if (qse_sio_flush (sio) <= -1) return -1; /* can't do buffering */

		for (cur = str, left = size; left > 0; cur += count, left -= count)
		{
			if (WriteConsoleW (
				sio->file.handle, cur, left, 
				&count, QSE_NULL) == FALSE) 
			{
				sio->errnum = QSE_SIO_ESYSERR;
				return -1;
			}
			if (count == 0) break;

			/* Note:
			 * WriteConsoleW() in unicosw.dll on win 9x/me returns
			 * the number of bytes via 'count'. If a double byte 
			 * string is given, 'count' can be greater than 'left'.
			 * this case is a miserable failure. however, i don't
			 * think there is CP_UTF8 codepage for console on win9x/me.
			 * so let me make this function fail if that ever happens.
			 */
			if (count > left) 
			{
				sio->errnum = QSE_SIO_ESYSERR;
				return -1;
			}
		}
		return cur - str;
	}	

#endif

	sio->errnum = QSE_SIO_ENOERR;
	n = qse_tio_writewcs (&sio->tio.io, str, size);
	if (n <= -1 && sio->errnum == QSE_SIO_ENOERR) 
		sio->errnum = tio_errnum_to_sio_errnum (&sio->tio.io);
	return n;
}

int qse_sio_getpos (qse_sio_t* sio, qse_sio_pos_t* pos)
{
	qse_fio_off_t off;

	off = qse_fio_seek (&sio->file, 0, QSE_FIO_CURRENT);
	if (off == (qse_fio_off_t)-1) 
	{
		sio->errnum = fio_errnum_to_sio_errnum (&sio->file);
		return -1;
	}

	*pos = off;
	return 0;
}

int qse_sio_setpos (qse_sio_t* sio, qse_sio_pos_t pos)
{
   	qse_fio_off_t off;

	if (qse_sio_flush(sio) <= -1) return -1;
	off = qse_fio_seek (&sio->file, pos, QSE_FIO_BEGIN);
	if (off == (qse_fio_off_t)-1)
	{
		sio->errnum = fio_errnum_to_sio_errnum (&sio->file);
		return -1;
	}

	return 0;
}

#if 0
int qse_sio_seek (qse_sio_t* sio, qse_sio_seek_t pos)
{
	/* TODO: write this function - more flexible positioning ....
	 *       can move to the end of the stream also.... */

	if (qse_sio_flush(sio) <= -1) return -1;
	return (qse_fio_seek (&sio->file, 
		0, QSE_FIO_END) == (qse_fio_off_t)-1)? -1: 0;

	/* TODO: write this function */
	if (qse_sio_flush(sio) <= -1) return -1;
	return (qse_fio_seek (&sio->file, 
		0, QSE_FIO_BEGIN) == (qse_fio_off_t)-1)? -1: 0;

}
#endif

static qse_ssize_t file_input (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size)
{

	if (cmd == QSE_TIO_DATA) 
	{
		qse_ssize_t n;
		qse_sio_t* sio;

		sio = *(qse_sio_t**)QSE_XTN(tio);
		QSE_ASSERT (sio != QSE_NULL);

		n = qse_fio_read (&sio->file, buf, size);
		if (n <= -1) sio->errnum = fio_errnum_to_sio_errnum (&sio->file);
		return n;
	}

	return 0;
}

static qse_ssize_t file_output (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size)
{
	if (cmd == QSE_TIO_DATA) 
	{
		qse_ssize_t n;
		qse_sio_t* sio;

		sio = *(qse_sio_t**)QSE_XTN(tio);
		QSE_ASSERT (sio != QSE_NULL);

		n = qse_fio_write (&sio->file, buf, size);
		if (n <= -1) sio->errnum = fio_errnum_to_sio_errnum (&sio->file);
		return n;
	}

	return 0;
}


