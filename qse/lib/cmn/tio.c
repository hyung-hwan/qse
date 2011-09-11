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
#include "mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (tio)

qse_tio_t* qse_tio_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, int flags)
{
	qse_tio_t* tio;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

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
	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (tio, 0, QSE_SIZEOF(*tio));

	tio->mmgr = mmgr;
	tio->flags = flags;

	/*
	tio->input_func = QSE_NULL;
	tio->input_arg = QSE_NULL;
	tio->output_func = QSE_NULL;
	tio->output_arg = QSE_NULL;

	tio->input_status = 0;
	tio->inbuf_curp = 0;
	tio->inbuf_len = 0;
	tio->outbuf_len = 0;
	*/

	tio->errnum = QSE_TIO_ENOERR;
	return 0;
}

int qse_tio_fini (qse_tio_t* tio)
{
	qse_tio_flush (tio); /* don't care about the result */
	if (qse_tio_detachin(tio) == -1) return -1;
	if (qse_tio_detachout(tio) == -1) return -1;
	return 0;
}

qse_tio_errnum_t qse_tio_geterrnum (qse_tio_t* tio)
{
	return tio->errnum;
}

const qse_char_t* qse_tio_geterrmsg (qse_tio_t* tio)
{
	static const qse_char_t* __errmsg[] =
	{
		QSE_T("no error"),
		QSE_T("out of memory"),
		QSE_T("no more space"),
		QSE_T("illegal multibyte sequence"),
		QSE_T("incomplete multibyte sequence"),
		QSE_T("illegal wide character"),
		QSE_T("no input function attached"),
		QSE_T("input function returned an error"),
		QSE_T("input function failed to open"),
		QSE_T("input function failed to closed"),
		QSE_T("no output function attached"),
		QSE_T("output function returned an error"),
		QSE_T("output function failed to open"),
		QSE_T("output function failed to closed"),
		QSE_T("unknown error")
	};

	return __errmsg[
		(tio->errnum < 0 || tio->errnum >= QSE_COUNTOF(__errmsg))? 
		QSE_COUNTOF(__errmsg) - 1: tio->errnum];
}

int qse_tio_attachin (qse_tio_t* tio, qse_tio_io_t input, void* arg)
{
	if (qse_tio_detachin(tio) == -1) return -1;

	QSE_ASSERT (tio->input_func == QSE_NULL);

	if (input(QSE_TIO_IO_OPEN, arg, QSE_NULL, 0) == -1) 
	{
		tio->errnum = QSE_TIO_EINPOP;
		return -1;
	}

	tio->input_func = input;
	tio->input_arg = arg;

	tio->input_status = 0;
	tio->inbuf_curp = 0;
	tio->inbuf_len = 0;

	return 0;
}

int qse_tio_detachin (qse_tio_t* tio)
{
	if (tio->input_func != QSE_NULL) 
	{
		if (tio->input_func (
			QSE_TIO_IO_CLOSE, tio->input_arg, QSE_NULL, 0) == -1) 
		{
			tio->errnum = QSE_TIO_EINPCL;
			return -1;
		}

		tio->input_func = QSE_NULL;
		tio->input_arg = QSE_NULL;
	}
		
	return 0;
}

int qse_tio_attachout (qse_tio_t* tio, qse_tio_io_t output, void* arg)
{
	if (qse_tio_detachout(tio) == -1) return -1;

	QSE_ASSERT (tio->output_func == QSE_NULL);

	if (output(QSE_TIO_IO_OPEN, arg, QSE_NULL, 0) == -1) 
	{
		tio->errnum = QSE_TIO_EOUTOP;
		return -1;
	}

	tio->output_func = output;
	tio->output_arg = arg;
	tio->outbuf_len = 0;

	return 0;
}

int qse_tio_detachout (qse_tio_t* tio)
{
	if (tio->output_func != QSE_NULL) 
	{
		qse_tio_flush (tio); /* don't care about the result */

		if (tio->output_func (
			QSE_TIO_IO_CLOSE, tio->output_arg, QSE_NULL, 0) == -1) 
		{
			tio->errnum = QSE_TIO_EOUTCL;
			return -1;
		}

		tio->output_func = QSE_NULL;
		tio->output_arg = QSE_NULL;
	}
		
	return 0;
}

qse_ssize_t qse_tio_flush (qse_tio_t* tio)
{
	qse_size_t left, count;

	if (tio->output_func == QSE_NULL) 
	{
		tio->errnum = QSE_TIO_ENOUTF;
		return (qse_ssize_t)-1;
	}

	left = tio->outbuf_len;
	while (left > 0) 
	{
		qse_ssize_t n;

		n = tio->output_func (
			QSE_TIO_IO_DATA, tio->output_arg, tio->outbuf, left);
		if (n <= -1) 
		{
			tio->outbuf_len = left;
			tio->errnum = QSE_TIO_EOUTPT;
			return -1;
		}
		if (n == 0) break;
	
		left -= n;
		QSE_MEMCPY (tio->outbuf, &tio->inbuf[n], left);
	}

	count = tio->outbuf_len - left;
	tio->outbuf_len = left;

	return (qse_ssize_t)count;
}

void qse_tio_purge (qse_tio_t* tio)
{
	tio->input_status = 0;
	tio->inbuf_curp = 0;
	tio->inbuf_len = 0;
	tio->outbuf_len = 0;
	tio->errnum = QSE_TIO_ENOERR;
}
