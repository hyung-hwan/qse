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
#	include <windows.h>
#elif defined(__OS2__)
#	define INCL_DOSFILEMGR
#	include <os2.h>
#endif

static qse_sio_t __sio_in = 
{
	QSE_NULL, /* mmgr */

	/* fio */
	{
		QSE_NULL,                      /* mmgr */
		0,                             /* errnum */

	#if defined(_WIN32)
		/* this is not a handle. it is adjusted to 
		 * an actual handle in __sio_input () */
		(HANDLE)STD_INPUT_HANDLE,      /* handle */
	#elif defined(__OS2__)
		(HFILE)0,                      /* handle */
	#elif defined(__DOS__)
		0,                             /* handle */
	#else
		0,                             /* handle */
	#endif

		0,                             /* flags */
		QSE_NULL                       /* tio */
	},

	/* tio */
	{
		QSE_NULL,
		0,
		QSE_TIO_IGNOREMBWCERR,

		__sio_input,
		__sio_output,
		&__sio_in,	
		&__sio_in,

		0,
		0,
		0,
		0,

		{ 0 },
		{ 0 }
	}
};

static qse_sio_t __sio_out = 
{
	QSE_NULL, /* mmgr */

	/* fio */
	{
		QSE_NULL,
		0,

	#if defined(_WIN32)
		/* this is not a handle. it is adjusted to 
		 * an actual handle in __sio_output () */
		(HANDLE)STD_OUTPUT_HANDLE,
	#elif defined(__OS2__)
		(HFILE)1,
	#elif defined(__DOS__)
		1,
	#else
		1,
	#endif

		0,
		QSE_NULL
	},

	/* tio */
	{
		QSE_NULL,
		0,
		QSE_TIO_IGNOREMBWCERR,

		__sio_input,
		__sio_output,
		&__sio_out,	
		&__sio_out,

		0,
		0,
		0,
		0,

		{ 0 },
		{ 0 }
	}
};

static qse_sio_t __sio_err = 
{
	QSE_NULL, /* mmgr */

	/* fio */
	{
		QSE_NULL,
		0,

	#if defined(_WIN32)
		/* this is not a handle. it is adjusted to 
		 * an actual handle in __sio_output () */
		(HANDLE)STD_ERROR_HANDLE,
	#elif defined(__OS2__)
		(HFILE)2,
	#elif defined(__DOS__)
		2,
	#else
		2,
	#endif

		0,
		QSE_NULL
	},

	/* tio */
	{
		QSE_NULL,
		0,
		QSE_TIO_IGNOREMBWCERR,

		__sio_input,
		__sio_output,
		&__sio_err,	
		&__sio_err,

		0,
		0,
		0,
		0,

		{ 0 },
		{ 0 }
	}
};

qse_sio_t* qse_sio_in = &__sio_in;
qse_sio_t* qse_sio_out = &__sio_out;
qse_sio_t* qse_sio_err = &__sio_err;

qse_sio_t* qse_sio_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, const qse_char_t* file, int flags)
{
	qse_sio_t* sio;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

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
	qse_fio_hnd_t hnd;
	if (qse_getstdfiohandle (std, &hnd) <= -1) return QSE_NULL;
	return qse_sio_open (mmgr, xtnsize, 
		(const qse_char_t*)&hnd, flags | QSE_SIO_HANDLE | QSE_SIO_NOCLOSE);
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

	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (sio, 0, QSE_SIZEOF(*sio));
	sio->mmgr = mmgr;

	mode = QSE_FIO_RUSR | QSE_FIO_WUSR | 
	       QSE_FIO_RGRP | QSE_FIO_ROTH;

	if (qse_fio_init (&sio->fio, mmgr, file, flags, mode) <= -1) return -1;

	if (flags & QSE_SIO_IGNOREMBWCERR) topt |= QSE_TIO_IGNOREMBWCERR;

	if (qse_tio_init(&sio->tio, mmgr, topt) <= -1)
	{
		qse_fio_fini (&sio->fio);
		return -1;
	}

	if (qse_tio_attachin(&sio->tio, __sio_input, sio) <= -1 ||
	    qse_tio_attachout(&sio->tio, __sio_output, sio) <= -1) 
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
	qse_fio_hnd_t hnd;
	if (qse_getstdfiohandle (std, &hnd) <= -1) return -1;
	return qse_sio_init (sio, mmgr, 
		(const qse_char_t*)&hnd, flags | QSE_SIO_HANDLE);
}

void qse_sio_fini (qse_sio_t* sio)
{
	/*if (qse_sio_flush (sio) == -1) return -1;*/
	qse_sio_flush (sio);
	qse_tio_fini (&sio->tio);
	qse_fio_fini (&sio->fio);

	if (sio == qse_sio_in) qse_sio_in = QSE_NULL;
	else if (sio == qse_sio_out) qse_sio_out = QSE_NULL;
	else if (sio == qse_sio_err) qse_sio_err = QSE_NULL;
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

qse_ssize_t qse_sio_getc (qse_sio_t* sio, qse_char_t* c)
{
	return qse_tio_read (&sio->tio, c, 1);
}

qse_ssize_t qse_sio_gets (
	qse_sio_t* sio, qse_char_t* buf, qse_size_t size)
{
	qse_ssize_t n;

	if (size <= 0) return 0;
	n = qse_tio_read (&sio->tio, buf, size -1);
	if (n == -1) return -1;
	buf[n] = QSE_T('\0');
	return n;
}

qse_ssize_t qse_sio_getsn (
	qse_sio_t* sio, qse_char_t* buf, qse_size_t size)
{
	return qse_tio_read (&sio->tio, buf, size);
}

qse_ssize_t qse_sio_putc (qse_sio_t* sio, qse_char_t c)
{
	return qse_tio_write (&sio->tio, &c, 1);
}

qse_ssize_t qse_sio_puts (qse_sio_t* sio, const qse_char_t* str)
{
	return qse_tio_write (&sio->tio, str, (qse_size_t)-1);
}

qse_ssize_t qse_sio_putsn (
	qse_sio_t* sio, const qse_char_t* str, qse_size_t size)
{
	return qse_tio_write (&sio->tio, str, size);
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

static qse_ssize_t __sio_input (qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size)
{
	qse_sio_t* sio = (qse_sio_t*)arg;

	QSE_ASSERT (sio != QSE_NULL);

	if (cmd == QSE_TIO_IO_DATA) 
	{
	#if defined(_WIN32)
		/* TODO: I hate this way of adjusting the handle value
		 *       Is there any good ways to do it statically? */
		HANDLE h = sio->fio.handle;
		if (h == (HANDLE)STD_INPUT_HANDLE ||
		    h == (HANDLE)STD_OUTPUT_HANDLE ||
		    h == (HANDLE)STD_ERROR_HANDLE)
		{
			h = GetStdHandle((DWORD)h);
			if (h != INVALID_HANDLE_VALUE && h != NULL) 
			{
				sio->fio.handle = h; 
			}
		}
	#endif
		return qse_fio_read (&sio->fio, buf, size);
	}

	return 0;
}

static qse_ssize_t __sio_output (qse_tio_cmd_t cmd, void* arg, void* buf, qse_size_t size)
{
	qse_sio_t* sio = (qse_sio_t*)arg;

	QSE_ASSERT (sio != QSE_NULL);

	if (cmd == QSE_TIO_IO_DATA) 
	{
	#if defined(_WIN32)
		/* TODO: I hate this way of adjusting the handle value
		 *       Is there any good ways to do it statically? */
		HANDLE h = sio->fio.handle;
		if (h == (HANDLE)STD_INPUT_HANDLE ||
		    h == (HANDLE)STD_OUTPUT_HANDLE ||
		    h == (HANDLE)STD_ERROR_HANDLE)
		{
			h = GetStdHandle((DWORD)h);
			if (h != INVALID_HANDLE_VALUE && h != NULL) 
			{
				sio->fio.handle = h; 
			}
		}
	#endif
		return qse_fio_write (&sio->fio, buf, size);
	}

	return 0;
}

