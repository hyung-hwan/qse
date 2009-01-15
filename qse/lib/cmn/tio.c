/*
 * $Id: tio.c,v 1.13 2006/01/01 13:50:24 bacon Exp $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

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

#include <qse/cmn/tio.h>
#include "mem.h"

QSE_IMPLEMENT_STD_FUNCTIONS (tio)

qse_tio_t* qse_tio_open (qse_mmgr_t* mmgr, qse_size_t ext)
{
	qse_tio_t* tio;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	tio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_tio_t) + ext);
	if (tio == QSE_NULL) return QSE_NULL;

	if (qse_tio_init (tio, mmgr) == QSE_NULL)
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

qse_tio_t* qse_tio_init (qse_tio_t* tio, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (tio, 0, QSE_SIZEOF(*tio));

	tio->mmgr = mmgr;

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

	return tio;
}

int qse_tio_fini (qse_tio_t* tio)
{
	qse_tio_flush (tio); /* don't care about the result */
	if (qse_tio_detachin(tio) == -1) return -1;
	if (qse_tio_detachout(tio) == -1) return -1;
	return 0;
}

qse_tio_err_t qse_tio_geterrnum (qse_tio_t* tio)
{
	return tio->errnum;
}

const qse_char_t* qse_tio_geterrstr (qse_tio_t* tio)
{
	static const qse_char_t* __errstr[] =
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

	return __errstr[
		(tio->errnum < 0 || tio->errnum >= QSE_COUNTOF(__errstr))? 
		QSE_COUNTOF(__errstr) - 1: tio->errnum];
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
