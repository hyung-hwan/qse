/*
 * $Id: std.c 306 2009-11-22 13:58:53Z baconevi $
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

#include "sed.h"
#include <qse/sed/std.h>
#include <qse/cmn/str.h>
#include <qse/cmn/sio.h>
#include "../cmn/mem.h"

struct xtn_t
{
	struct
	{
		qse_sed_iostd_t* in;
		qse_sed_iostd_t* out;

		qse_sed_iostd_t* in_cur;
		qse_size_t in_mempos;
		qse_str_t* out_memstr;
	} e;
};

typedef struct xtn_t xtn_t;

qse_sed_t* qse_sed_openstd (qse_size_t xtnsize)
{
	return qse_sed_openstdwithmmgr (QSE_MMGR_GETDFL(), xtnsize);
}

qse_sed_t* qse_sed_openstdwithmmgr (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_sed_t* sed;
	xtn_t* xtn;

	/* create an object */
	sed = qse_sed_open (mmgr, QSE_SIZEOF(xtn_t) + xtnsize);
	if (sed == QSE_NULL) return QSE_NULL;

	/* initialize extension */
	xtn = (xtn_t*) QSE_XTN (sed);
	QSE_MEMSET (xtn, 0, QSE_SIZEOF(xtn_t));

	return sed;
}

void* qse_sed_getxtnstd (qse_sed_t* sed)
{
	return (void*)((xtn_t*)QSE_XTN(sed) + 1);
}

int qse_sed_compstd (qse_sed_t* sed, const qse_char_t* sptr)
{
	return qse_sed_comp (sed, sptr, qse_strlen(sptr));
}

static qse_sio_t* open_sio (qse_sed_t* sed, const qse_char_t* file, int flags)
{
	qse_sio_t* sio;

	sio = qse_sio_open (sed->mmgr, 0, file, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t ea;
		ea.ptr = file;
		ea.len = qse_strlen (file);
		qse_sed_seterrnum (sed, QSE_SED_EIOFIL, &ea);
	}
	return sio;
}

static void close_main_stream (
	qse_sed_t* sed, qse_sed_io_arg_t* arg, qse_sed_iostd_t* io)
{
	if (io->type == QSE_SED_IOSTD_FILE)
	{
		qse_sio_t* sio = (qse_sio_t*)arg->handle;
		if (sio != qse_sio_in && sio != qse_sio_out && sio != qse_sio_err)
		qse_sio_close (sio);
	}
}

static int open_main_input_stream (qse_sed_t* sed, qse_sed_io_arg_t* arg, qse_sed_iostd_t* io)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);

	QSE_ASSERT (io != QSE_NULL);
	switch (io->type)
	{
		case QSE_SED_IOSTD_SIO:
			arg->handle = io->u.sio;
			break;

		case QSE_SED_IOSTD_FILE:
			if (io->u.file == QSE_NULL)
			{
				arg->handle = qse_sio_in;
			}
			else
			{
				qse_sio_t* sio;
				sio = open_sio (sed, io->u.file, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
				if (sio == QSE_NULL) return -1;
				arg->handle = sio;
			}
			break;

		case QSE_SED_IOSTD_MEM:
			/* don't store anything to arg->handle */
			xtn->e.in_mempos = 0;
			break;
	}

	return 0;
}

static int open_main_output_stream (qse_sed_t* sed, qse_sed_io_arg_t* arg, qse_sed_iostd_t* io)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);

	QSE_ASSERT (io != QSE_NULL);
	switch (io->type)
	{
		case QSE_SED_IOSTD_SIO:
			arg->handle = io->u.sio;
			break;

		case QSE_SED_IOSTD_FILE:
			if (io->u.file == QSE_NULL)
			{
				arg->handle = qse_sio_out;
			}
			else
			{
				qse_sio_t* sio;
				sio = open_sio (sed, io->u.file,
					QSE_SIO_WRITE |
					QSE_SIO_CREATE |
					QSE_SIO_TRUNCATE |
					QSE_SIO_IGNOREMBWCERR
				);
				if (sio == QSE_NULL) return -1;
				arg->handle = sio;
			}
			break;

		case QSE_SED_IOSTD_MEM:
			/* don't store anything to arg->handle */
			xtn->e.out_memstr = qse_str_open (sed->mmgr, 0, 512);
			if (xtn->e.out_memstr == QSE_NULL)
			{
				qse_sed_seterrnum (sed, QSE_SED_ENOMEM, QSE_NULL);
				return -1;
			}
			break;
	}

	return 0;
}

static qse_ssize_t read_main_input_stream (
	qse_sed_t* sed, qse_sed_io_arg_t* arg, qse_char_t* buf, qse_size_t len)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);
	qse_sed_iostd_t* io, * next;
	void* old, * new;
	qse_ssize_t n = 0;

	if (len > QSE_TYPE_MAX(qse_ssize_t)) len = QSE_TYPE_MAX(qse_ssize_t);

	do
	{
		io = xtn->e.in_cur;

		QSE_ASSERT (io != QSE_NULL);
		if (io->type == QSE_SED_IOSTD_MEM)
		{
			n = 0;
			while (xtn->e.in_mempos < io->u.mem.len && n < len)
				buf[n++] = io->u.mem.ptr[xtn->e.in_mempos++];
		}
		else n = qse_sio_getsn (arg->handle, buf, len);

		if (n != 0) break;
	
		/* end of file on the current input stream */

		next = xtn->e.in_cur + 1;
		if (next->type == QSE_SED_IOSTD_NULL) 
		{
			/* no next stream available - return 0 */	
			break; 
		}

		old = arg->handle;

		/* try to open the next input stream */
		if (open_main_input_stream (sed, arg, next) <= -1)
		{
			/* failed to open the next input stream */
			n = -1;
			break;
		}
		new = arg->handle;

		arg->handle = old;
		close_main_stream (sed, arg, io);

		arg->handle = new;
		xtn->e.in_cur++;
	}
	while (1);

	return n;
}

static qse_ssize_t xin (
	qse_sed_t* sed, qse_sed_io_cmd_t cmd, qse_sed_io_arg_t* arg,
	qse_char_t* buf, qse_size_t len)
{
	qse_sio_t* sio;
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);

	switch (cmd)
	{
		case QSE_SED_IO_OPEN:
		{
			if (arg->path == QSE_NULL)
			{
				/* main data stream */
				if (xtn->e.in == QSE_NULL) arg->handle = qse_sio_in;
				else
				{
					if (open_main_input_stream (sed, arg, xtn->e.in_cur) <= -1) return -1;
				}
			}
			else
			{
				sio = open_sio (sed, arg->path, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
				if (sio == QSE_NULL) return -1;
				arg->handle = sio;
			}

			return 1;
		}

		case QSE_SED_IO_CLOSE:
		{
			if (arg->path == QSE_NULL)
			{
				/* main data stream */
				if (xtn->e.in) close_main_stream (sed, arg, xtn->e.in_cur);
			}
			else
			{
				sio = (qse_sio_t*)arg->handle;
				if (sio != qse_sio_in && sio != qse_sio_out && sio != qse_sio_err)
					qse_sio_close (sio);
			}

			return 0;
		}

		case QSE_SED_IO_READ:
		{
			if (arg->path == QSE_NULL)
			{
				/* main data stream */
				if (xtn->e.in == QSE_NULL)
					return qse_sio_getsn (arg->handle, buf, len);
				else
					return read_main_input_stream (sed, arg, buf, len);
			}
			else
			{
				return qse_sio_getsn (arg->handle, buf, len);
			}
		}

		default:
			return -1;
	}
}

static qse_ssize_t xout (
	qse_sed_t* sed, qse_sed_io_cmd_t cmd, qse_sed_io_arg_t* arg,
	qse_char_t* dat, qse_size_t len)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);
	qse_sio_t* sio;

	switch (cmd)
	{
		case QSE_SED_IO_OPEN:
		{
			if (arg->path == QSE_NULL)
			{
				if (xtn->e.out == QSE_NULL) arg->handle = qse_sio_out;
				else
				{
					if (open_main_output_stream (sed, arg, xtn->e.out) <= -1) return -1;
				}
			}
			else
			{
				sio = open_sio (
					sed, arg->path,
					QSE_SIO_WRITE |
					QSE_SIO_CREATE |
					QSE_SIO_TRUNCATE |
					QSE_SIO_IGNOREMBWCERR
				);
				if (sio == QSE_NULL) return -1;
				arg->handle = sio;
			}

			return 1;
		}

		case QSE_SED_IO_CLOSE:
		{
			if (arg->path == QSE_NULL)
			{
				if (xtn->e.out) close_main_stream (sed, arg, xtn->e.out);
			}
			else
			{
				sio = (qse_sio_t*)arg->handle;
				qse_sio_flush (sio);
				if (sio != qse_sio_in && sio != qse_sio_out && sio != qse_sio_err)
					qse_sio_close (sio);
			}
			return 0;
		}

		case QSE_SED_IO_WRITE:
		{
			if (arg->path == QSE_NULL)
			{
				/* main data stream */
				if (xtn->e.out == QSE_NULL)
					return qse_sio_putsn (arg->handle, dat, len);
				else
				{
					qse_sed_iostd_t* io = xtn->e.out;
					if (io->type == QSE_SED_IOSTD_MEM)
					{
						if (len > QSE_TYPE_MAX(qse_ssize_t)) len = QSE_TYPE_MAX(qse_ssize_t);

						if (qse_str_ncat (xtn->e.out_memstr, dat, len) == (qse_size_t)-1)
						{
							qse_sed_seterrnum (sed, QSE_SED_ENOMEM, QSE_NULL);
							return -1;
						}

						return len;
					}
					else
					{
						return qse_sio_putsn (arg->handle, dat, len);
					}
				}
			}
			else
			{
				return qse_sio_putsn (arg->handle, dat, len);
			}
		}
	
		default:
			return -1;
	}
}

int qse_sed_execstd (
	qse_sed_t* sed, qse_sed_iostd_t in[], qse_sed_iostd_t* out)
{
	int n;
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);

	if (in)
	{
		qse_size_t i;

		if (in[0].type == QSE_SED_IOSTD_NULL)
		{
			/* if 'in' is specified, it must contains at least one 
			 * valid entry */
			qse_sed_seterrnum (sed, QSE_SED_EINVAL, QSE_NULL);
			return -1;
		}

		for (i = 0; in[i].type != QSE_SED_IOSTD_NULL; i++)
		{
			if (in[i].type != QSE_SED_IOSTD_SIO &&
			    in[i].type != QSE_SED_IOSTD_FILE &&
			    in[i].type != QSE_SED_IOSTD_MEM)
			{
				qse_sed_seterrnum (sed, QSE_SED_EINVAL, QSE_NULL);
				return -1;
			}
		}
	}

	if (out)
	{
		if (out->type != QSE_SED_IOSTD_SIO &&
		    out->type != QSE_SED_IOSTD_FILE &&
		    out->type != QSE_SED_IOSTD_MEM)
		{
			qse_sed_seterrnum (sed, QSE_SED_EINVAL, QSE_NULL);
			return -1;
		}
	}

	QSE_MEMSET (&xtn->e, 0, QSE_SIZEOF(xtn->e));
	xtn->e.in = in;
	xtn->e.out = out;
	xtn->e.in_cur = in;

	n = qse_sed_exec (sed, xin, xout);

	if (n >= 0 && out && out->type == QSE_SED_IOSTD_MEM)
	{
		QSE_ASSERT (xtn->e.out_memstr != QSE_NULL);
		qse_str_yield (xtn->e.out_memstr, &out->u.mem, 0);
	}
	if (xtn->e.out_memstr) qse_str_close (xtn->e.out_memstr);

	/* if some output without a newline is emitted before 
	 * last flush, they can be lost because qse_sio_out
	 * is not explicitly closed() with qse_sio_close()
	 * which in turn calls qse_sio_flush(). let's call it
	 * here directly. I don't care whether some other 
	 * threads could have written to this. */
	qse_sio_flush (qse_sio_out);

	return n;
}

