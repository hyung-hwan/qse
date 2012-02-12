/*
 * $Id: tio.c 565 2011-09-11 02:48:21Z hyunghwan.chung $
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

#include <qse/cmn/tio.h>
#include <qse/cmn/mbwc.h> 
#include "mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (tio)

static int detach_in (qse_tio_t* tio, int fini);
static int detach_out (qse_tio_t* tio, int fini);

qse_tio_t* qse_tio_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, int flags)
{
	qse_tio_t* tio;

	tio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_tio_t) + xtnsize);
	if (tio == QSE_NULL) return QSE_NULL;

	if (qse_tio_init (tio, mmgr, flags) <= -1)
	{
		QSE_MMGR_FREE (mmgr, tio);
		return QSE_NULL;
	}

	return tio;
}

int qse_tio_close (qse_tio_t* tio)
{
	int n = qse_tio_fini (tio);
	QSE_MMGR_FREE (tio->mmgr, tio);
	return n;
}

int qse_tio_init (qse_tio_t* tio, qse_mmgr_t* mmgr, int flags)
{
	QSE_MEMSET (tio, 0, QSE_SIZEOF(*tio));

	tio->mmgr = mmgr;
	tio->cmgr = qse_getdflcmgr();

	/* mask off internal bits when storing the flags for safety */
	tio->flags = flags & ~(QSE_TIO_DYNINBUF | QSE_TIO_DYNOUTBUF);

	/*
	tio->input_func = QSE_NULL;
	tio->input_arg = QSE_NULL;
	tio->output_func = QSE_NULL;
	tio->output_arg = QSE_NULL;

	tio->input_status = 0;
	tio->inbuf_cur = 0;
	tio->inbuf_len = 0;
	tio->outbuf_len = 0;
	*/

	tio->errnum = QSE_TIO_ENOERR;
	return 0;
}

int qse_tio_fini (qse_tio_t* tio)
{
	int ret = 0;

	qse_tio_flush (tio); /* don't care about the result */
	if (detach_in (tio, 1) <= -1) ret = -1;
	if (detach_out (tio, 1) <= -1) ret = -1;

	return ret;
}

qse_tio_errnum_t qse_tio_geterrnum (qse_tio_t* tio)
{
	return tio->errnum;
}

qse_cmgr_t* qse_tio_getcmgr (qse_tio_t* tio)
{
	return tio->cmgr;
}

void qse_tio_setcmgr (qse_tio_t* tio, qse_cmgr_t* cmgr)
{
	tio->cmgr = cmgr;
}

int qse_tio_attachin (
	qse_tio_t* tio, qse_tio_io_fun_t input,
	qse_mchar_t* bufptr, qse_size_t bufcapa)
{
	qse_mchar_t* xbufptr;

	if (input == QSE_NULL || bufcapa < QSE_TIO_MININBUFCAPA) 
	{
		tio->errnum = QSE_TIO_EINVAL;
		return -1;
	}

	if (qse_tio_detachin(tio) <= -1) return -1;

	QSE_ASSERT (tio->in.fun == QSE_NULL);

	xbufptr = bufptr;
	if (xbufptr == QSE_NULL)
	{
		xbufptr = QSE_MMGR_ALLOC (
			tio->mmgr, QSE_SIZEOF(qse_mchar_t) * bufcapa);
		if (xbufptr == QSE_NULL)
		{
			tio->errnum = QSE_TIO_ENOMEM;
			return -1;	
		}
	}

	tio->errnum = QSE_TIO_ENOERR;
	if (input (tio, QSE_TIO_OPEN, QSE_NULL, 0) <= -1) 
	{
		if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
		if (xbufptr != bufptr) QSE_MMGR_FREE (tio->mmgr, xbufptr);
		return -1;
	}

	/* if i defined tio->io[2] instead of tio->in and tio-out, 
	 * i would be able to shorten code amount. but fields to initialize
	 * are not symmetric between input and output.
	 * so it's just a bit clumsy that i repeat almost the same code
	 * in qse_tio_attachout().
	 */

	tio->in.fun = input;
	tio->in.buf.ptr = xbufptr;
	tio->in.buf.capa = bufcapa;

	tio->input_status = 0;
	tio->inbuf_cur = 0;
	tio->inbuf_len = 0;

	if (xbufptr != bufptr) tio->flags |= QSE_TIO_DYNINBUF;
	return 0;
}

static int detach_in (qse_tio_t* tio, int fini)
{
	int ret = 0;

	if (tio->in.fun)
	{
		tio->errnum = QSE_TIO_ENOERR;
		if (tio->in.fun (tio, QSE_TIO_CLOSE, QSE_NULL, 0) <= -1) 
		{
			if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;

			/* returning with an error here allows you to retry detaching */
			if (!fini) return -1; 

			/* otherwise, you can't retry since the input handler information
			 * is reset below */
			ret = -1; 
		}

		if (tio->flags & QSE_TIO_DYNINBUF) 
		{
			QSE_MMGR_FREE (tio->mmgr, tio->in.buf.ptr);
			tio->flags &= ~QSE_TIO_DYNINBUF;
		}

		tio->in.fun = QSE_NULL;
		tio->in.buf.ptr = QSE_NULL;
		tio->in.buf.capa = 0;
	}
		
	return ret;
}

int qse_tio_detachin (qse_tio_t* tio)
{
	return detach_in (tio, 0);
}

int qse_tio_attachout (
	qse_tio_t* tio, qse_tio_io_fun_t output, 
	qse_mchar_t* bufptr, qse_size_t bufcapa)
{
	qse_mchar_t* xbufptr;

	if (output == QSE_NULL || bufcapa < QSE_TIO_MINOUTBUFCAPA)  
	{
		tio->errnum = QSE_TIO_EINVAL;
		return -1;
	}

	if (qse_tio_detachout(tio) == -1) return -1;

	QSE_ASSERT (tio->out.fun == QSE_NULL);

	xbufptr = bufptr;
	if (xbufptr == QSE_NULL)
	{
		xbufptr = QSE_MMGR_ALLOC (
			tio->mmgr, QSE_SIZEOF(qse_mchar_t) * bufcapa);
		if (xbufptr == QSE_NULL)
		{
			tio->errnum = QSE_TIO_ENOMEM;
			return -1;	
		}
	}

	tio->errnum = QSE_TIO_ENOERR;
	if (output (tio, QSE_TIO_OPEN, QSE_NULL, 0) == -1) 
	{
		if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
		if (xbufptr != bufptr) QSE_MMGR_FREE (tio->mmgr, xbufptr);
		return -1;
	}

	tio->out.fun = output;
	tio->out.buf.ptr = xbufptr;
	tio->out.buf.capa = bufcapa;

	tio->outbuf_len = 0;

	if (xbufptr != bufptr) tio->flags |= QSE_TIO_DYNOUTBUF;
	return 0;
}

static int detach_out (qse_tio_t* tio, int fini)
{
	int ret = 0;

	if (tio->out.fun)
	{
		qse_tio_flush (tio); /* don't care about the result */

		tio->errnum = QSE_TIO_ENOERR;
		if (tio->out.fun (tio, QSE_TIO_CLOSE, QSE_NULL, 0) <= -1) 
		{
			if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
			/* returning with an error here allows you to retry detaching */
			if (!fini) return -1;

			/* otherwise, you can't retry since the input handler information
			 * is reset below */
			ret = -1;
		}
	
		if (tio->flags & QSE_TIO_DYNOUTBUF) 
		{
			QSE_MMGR_FREE (tio->mmgr, tio->out.buf.ptr);
			tio->flags &= ~QSE_TIO_DYNOUTBUF;
		}

		tio->out.fun = QSE_NULL;
		tio->out.buf.ptr = QSE_NULL;
		tio->out.buf.capa = 0;
	}
		
	return ret;
}

int qse_tio_detachout (qse_tio_t* tio)
{
	return detach_out (tio, 0);
}

qse_ssize_t qse_tio_flush (qse_tio_t* tio)
{
	qse_size_t left, count;
	qse_ssize_t n;
	qse_mchar_t* cur;

	if (tio->out.fun == QSE_NULL)
	{
		tio->errnum = QSE_TIO_ENOUTF;
		return (qse_ssize_t)-1;
	}

	left = tio->outbuf_len;
	cur = tio->out.buf.ptr;
	while (left > 0) 
	{
		tio->errnum = QSE_TIO_ENOERR;
		n = tio->out.fun (tio, QSE_TIO_DATA, cur, left);
		if (n <= -1) 
		{
			if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
			QSE_MEMCPY (tio->out.buf.ptr, cur, left);
			tio->outbuf_len = left;
			return -1;
		}
		if (n == 0) 
		{
			QSE_MEMCPY (tio->out.buf.ptr, cur, left);
			break;
		}
	
		left -= n;
		cur += n;
	}

	count = tio->outbuf_len - left;
	tio->outbuf_len = left;

	return (qse_ssize_t)count;
}

void qse_tio_purge (qse_tio_t* tio)
{
	tio->input_status = 0;
	tio->inbuf_cur = 0;
	tio->inbuf_len = 0;
	tio->outbuf_len = 0;
	tio->errnum = QSE_TIO_ENOERR;
}
