/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#include "xli.h"
#include <qse/xli/stdxli.h>
#include <qse/cmn/path.h>
#include "../cmn/mem.h"

typedef struct xtn_t
{
	struct
	{
		struct 
		{
			qse_xli_iostd_t* x;

			union
			{
				/* 
					nothing to maintain for file here 
				 */

				struct 
				{
					const qse_char_t* ptr;	
					const qse_char_t* end;	
				} str;
			} u;
		} in;

		struct
		{
			qse_xli_iostd_t* x;
			union
			{
				struct
				{
					qse_sio_t* sio;
				} file;
				struct 
				{
					qse_str_t* buf;
				} str;
			} u;
		} out;
	} s; 

	qse_xli_ecb_t ecb;
} xtn_t;

qse_xli_t* qse_xli_openstd (qse_size_t xtnsize, qse_size_t rootxtnsize)
{
	return qse_xli_openstdwithmmgr (QSE_MMGR_GETDFL(), xtnsize, rootxtnsize);
}

static void fini_xtn (qse_xli_t* xli)
{
	/* nothing to do */
}

static void clear_xtn (qse_xli_t* xli)
{
	/* nothing to do */
}

qse_xli_t* qse_xli_openstdwithmmgr (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_size_t rootxtnsize)
{
	qse_xli_t* xli;
	xtn_t* xtn;

	/* create an object */
	xli = qse_xli_open (mmgr, QSE_SIZEOF(xtn_t) + xtnsize, rootxtnsize);
	if (xli == QSE_NULL) goto oops;

	/* initialize extension */
	xtn = (xtn_t*) QSE_XTN (xli);

	xtn->ecb.close = fini_xtn;
	xtn->ecb.clear = clear_xtn;
	qse_xli_pushecb (xli, &xtn->ecb);

	return xli;

oops:
	if (xli) qse_xli_close (xli);
	return QSE_NULL;
}

void* qse_xli_getxtnstd (qse_xli_t* xli)
{
	return (void*)((xtn_t*)QSE_XTN(xli) + 1);
}

static qse_sio_t* open_sio (qse_xli_t* xli, const qse_char_t* file, int flags)
{
	qse_sio_t* sio;
	sio = qse_sio_open (xli->mmgr, 0, file, flags);
	if (sio == QSE_NULL)
	{
		qse_xstr_t errarg;
		errarg.ptr = file;
		errarg.len = qse_strlen(file);
		qse_xli_seterrnum (xli, QSE_XLI_EIOFIL, &errarg);
	}
	return sio;
}

static qse_xstr_t sio_std_names[] =
{
	{ QSE_T("stdin"),   5 },
	{ QSE_T("stdout"),  6 },
	{ QSE_T("stderr"),  6 }
};

static qse_sio_t* open_sio_std (qse_xli_t* xli, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;
	sio = qse_sio_openstd (xli->mmgr, 0, std, flags);
	if (sio == QSE_NULL) qse_xli_seterrnum (xli, QSE_XLI_EIOFIL, &sio_std_names[std]);
	return sio;
}

static qse_ssize_t sf_in_open (qse_xli_t* xli, qse_xli_io_arg_t* arg, xtn_t* xtn)
{
	if (arg->prev == QSE_NULL)
	{
		qse_xli_iostd_t* psin = xtn->s.in.x;
	
		QSE_ASSERT (arg == &xli->rio.top);

		switch (psin->type)
		{
			/* normal source files */
			case QSE_XLI_IOSTD_FILE:
				if (psin->u.file.path == QSE_NULL ||
				    (psin->u.file.path[0] == QSE_T('-') &&
				     psin->u.file.path[1] == QSE_T('\0')))
				{
					/* no path name or - -> stdin */
					arg->handle = open_sio_std (xli, QSE_SIO_STDIN, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_LINEBREAK);
				}
				else
				{
					arg->handle = open_sio (xli, psin->u.file.path, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_KEEPPATH);
				}
				if (arg->handle == QSE_NULL) return -1;
				if (psin->u.file.cmgr) qse_sio_setcmgr (arg->handle, psin->u.file.cmgr);

				/* update the object name to something more specific */
				arg->name = psin->u.file.path;
				if (arg->name == QSE_NULL) arg->name = sio_std_names[QSE_SIO_STDIN].ptr;

				return 0;
	
			case QSE_XLI_IOSTD_STR:
				xtn->s.in.u.str.ptr = psin->u.str.ptr;
				xtn->s.in.u.str.end = psin->u.str.ptr + psin->u.str.len;
				return 0;
	
			default:
				qse_xli_seterrnum (xli, QSE_XLI_EINTERN, QSE_NULL);
				return -1;
		}
	}
	else
	{
		/* handle the included file - @include */
		const qse_char_t* path;
		qse_char_t fbuf[64];
		qse_char_t* dbuf = QSE_NULL;
	
		QSE_ASSERT (arg->name != QSE_NULL);

		path = arg->name;
		if (arg->prev->handle)
		{
			const qse_char_t* outer;

			outer = qse_sio_getpath (arg->prev->handle);

			if (outer)
			{
				const qse_char_t* base;
	
				/* i'm being included from another file */
				base = qse_basename (outer);
				if (base != outer && arg->name[0] != QSE_T('/'))
				{
					qse_size_t tmplen, totlen, dirlen;
	
					dirlen = base - outer;	
					
					totlen = qse_strlen(arg->name) + dirlen;
					if (totlen >= QSE_COUNTOF(fbuf))
					{
						dbuf = qse_xli_allocmem (
							xli, QSE_SIZEOF(qse_char_t) * (totlen + 1)
						);
						if (dbuf == QSE_NULL) return -1;
		
						path = dbuf;
					}
					else path = fbuf;
		
					tmplen = qse_strncpy ((qse_char_t*)path, outer, dirlen);
					qse_strcpy ((qse_char_t*)path + tmplen, arg->name);
				}
			}
		}

		arg->handle = qse_sio_open (
			xli->mmgr, 0, path, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_KEEPPATH
		);

		if (dbuf) QSE_MMGR_FREE (xli->mmgr, dbuf);
		if (arg->handle == QSE_NULL)
		{
			qse_xstr_t ea;
			ea.ptr = arg->name;
			ea.len = qse_strlen(ea.ptr);
			qse_xli_seterrnum (xli, QSE_XLI_EIOFIL, &ea);
			return -1;
		}

		return 0;
	}
}


static qse_ssize_t sf_in_close (
	qse_xli_t* xli, qse_xli_io_arg_t* arg, xtn_t* xtn)
{
	if (arg->prev == QSE_NULL)
	{
		switch (xtn->s.in.x->type)
		{
			case QSE_XLI_IOSTD_FILE:
				QSE_ASSERT (arg->handle != QSE_NULL);
				qse_sio_close (arg->handle);
				break;

			case QSE_XLI_IOSTD_STR:
				/* nothing to close */
				break;

			default:
				/* nothing to close */
				break;
		}
	}
	else
	{
		/* handle the included source file - @include */
		QSE_ASSERT (arg->handle != QSE_NULL);
		qse_sio_close (arg->handle);
	}

	return 0;
}

static qse_ssize_t sf_in_read (
	qse_xli_t* xli, qse_xli_io_arg_t* arg,
	qse_char_t* data, qse_size_t size, xtn_t* xtn)
{
	if (arg->prev == QSE_NULL)
	{
		qse_ssize_t n;

		switch (xtn->s.in.x->type)
		{
			case QSE_XLI_IOSTD_FILE:
				QSE_ASSERT (arg->handle != QSE_NULL);
				n = qse_sio_getstrn (arg->handle, data, size);
				if (n <= -1)
				{
					qse_xstr_t ea;
					ea.ptr = xtn->s.in.x->u.file.path;
					if (ea.ptr == QSE_NULL) ea.ptr = sio_std_names[QSE_SIO_STDIN].ptr;
					ea.len = qse_strlen(ea.ptr);
					qse_xli_seterrnum (xli, QSE_XLI_EIOFIL, &ea);
				}
				break;

			case QSE_XLI_IOSTD_STR:
				n = 0;
				while (n < size && xtn->s.in.u.str.ptr < xtn->s.in.u.str.end)
				{
					data[n++] = *xtn->s.in.u.str.ptr++;
				}
				break;

			default:
				/* this should never happen */
				qse_xli_seterrnum (xli, QSE_XLI_EINTERN, QSE_NULL);
				n = -1;
				break;
		}

		return n;
	}
	else
	{
		/* handle the included source file - @include */
		qse_ssize_t n;

		QSE_ASSERT (arg->name != QSE_NULL);
		QSE_ASSERT (arg->handle != QSE_NULL);

		n = qse_sio_getstrn (arg->handle, data, size);
		if (n <= -1)
		{
			qse_xstr_t ea;
			ea.ptr = arg->name;
			ea.len = qse_strlen(ea.ptr);
			qse_xli_seterrnum (xli, QSE_XLI_EIOFIL, &ea);
		}
		return n;
	}
}

static qse_ssize_t sf_in (
	qse_xli_t* xli, qse_xli_io_cmd_t cmd, 
	qse_xli_io_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	xtn_t* xtn = QSE_XTN (xli);

	QSE_ASSERT (arg != QSE_NULL);

	switch (cmd)
	{
		case QSE_XLI_IO_OPEN:
			return sf_in_open (xli, arg, xtn);

		case QSE_XLI_IO_CLOSE:
			return sf_in_close (xli, arg, xtn);

		case QSE_XLI_IO_READ:
			return sf_in_read (xli, arg, data, size, xtn);

		default:
			qse_xli_seterrnum (xli, QSE_XLI_EINTERN, QSE_NULL);
			return -1;
	}
}

static qse_ssize_t sf_out_open (qse_xli_t* xli, qse_xli_io_arg_t* arg, xtn_t* xtn)
{
	if (arg->prev == QSE_NULL)
	{
		qse_xli_iostd_t* psout = xtn->s.out.x;
	
		QSE_ASSERT (arg == &xli->wio.top);

		switch (psout->type)
		{
			/* normal source files */
			case QSE_XLI_IOSTD_FILE:
				if (psout->u.file.path == QSE_NULL ||
				    (psout->u.file.path[0] == QSE_T('-') &&
				     psout->u.file.path[1] == QSE_T('\0')))
				{
					/* no path name or - -> stdout */
					arg->handle = open_sio_std (xli, QSE_SIO_STDOUT, QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR | QSE_SIO_LINEBREAK);
				}
				else
				{
					arg->handle = open_sio (xli, psout->u.file.path, QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR | QSE_SIO_KEEPPATH);
				}
				if (arg->handle == QSE_NULL) return -1;
				if (psout->u.file.cmgr) qse_sio_setcmgr (arg->handle, psout->u.file.cmgr);

				/* update the object name to something more specific */
				arg->name = psout->u.file.path;
				if (arg->name == QSE_NULL) arg->name = sio_std_names[QSE_SIO_STDOUT].ptr;

				return 0;
	
			case QSE_XLI_IOSTD_STR:
				xtn->s.out.u.str.buf = qse_str_open (xli->mmgr, 0, 512);
				if (xtn->s.out.u.str.buf == QSE_NULL)
				{
					qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL);
					return -1;
				}

				return 0;
			default:
				qse_xli_seterrnum (xli, QSE_XLI_EINTERN, QSE_NULL);
				return -1;
		}
	}
	else
	{
		/* handle the included file - @include */
		const qse_char_t* path;
		qse_char_t fbuf[64];
		qse_char_t* dbuf = QSE_NULL;
	
		QSE_ASSERT (arg->name != QSE_NULL);

		path = arg->name;
		if (arg->prev->handle)
		{
			const qse_char_t* outer;

			outer = qse_sio_getpath (arg->prev->handle);

			if (outer)
			{
				const qse_char_t* base;
	
				/* i'm being included from another file */
				base = qse_basename (outer);
				if (base != outer && arg->name[0] != QSE_T('/'))
				{
					qse_size_t tmplen, totlen, dirlen;
	
					dirlen = base - outer;	
					
					totlen = qse_strlen(arg->name) + dirlen;
					if (totlen >= QSE_COUNTOF(fbuf))
					{
						dbuf = qse_xli_allocmem (
							xli, QSE_SIZEOF(qse_char_t) * (totlen + 1)
						);
						if (dbuf == QSE_NULL) return -1;
		
						path = dbuf;
					}
					else path = fbuf;
		
					tmplen = qse_strncpy ((qse_char_t*)path, outer, dirlen);
					qse_strcpy ((qse_char_t*)path + tmplen, arg->name);
				}
			}
		}

		arg->handle = qse_sio_open (
			xli->mmgr, 0, path, QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR | QSE_SIO_KEEPPATH
		);

		if (dbuf) QSE_MMGR_FREE (xli->mmgr, dbuf);
		if (arg->handle == QSE_NULL)
		{
			qse_xstr_t ea;
			ea.ptr = arg->name;
			ea.len = qse_strlen(ea.ptr);
			qse_xli_seterrnum (xli, QSE_XLI_EIOFIL, &ea);
			return -1;
		}

		return 0;
	}
}


static qse_ssize_t sf_out_close (
	qse_xli_t* xli, qse_xli_io_arg_t* arg, xtn_t* xtn)
{
	if (arg->prev == QSE_NULL)
	{
		switch (xtn->s.out.x->type)
		{
			case QSE_XLI_IOSTD_FILE:
				QSE_ASSERT (arg->handle != QSE_NULL);
				qse_sio_close (arg->handle);
				break;

			case QSE_XLI_IOSTD_STR:
				/* nothing to close */
				break;

			default:
				/* nothing to close */
				break;
		}
	}
	else
	{
		/* handle the included source file - @include */
		QSE_ASSERT (arg->handle != QSE_NULL);
		qse_sio_close (arg->handle);
	}

	return 0;
}

static qse_ssize_t sf_out_write (
	qse_xli_t* xli, qse_xli_io_arg_t* arg,
	qse_char_t* data, qse_size_t size, xtn_t* xtn)
{
	if (arg->prev == QSE_NULL)
	{
		qse_ssize_t n;

		switch (xtn->s.out.x->type)
		{
			case QSE_XLI_IOSTD_FILE:
				QSE_ASSERT (arg->handle != QSE_NULL);
				n = qse_sio_putstrn (arg->handle, data, size);
				if (n <= -1)
				{
					qse_xstr_t ea;
					ea.ptr = xtn->s.out.x->u.file.path;
					if (ea.ptr == QSE_NULL) ea.ptr = sio_std_names[QSE_SIO_STDOUT].ptr;
					ea.len = qse_strlen(ea.ptr);
					qse_xli_seterrnum (xli, QSE_XLI_EIOFIL, &ea);
				}
				break;

			case QSE_XLI_IOSTD_STR:
				if (size > QSE_TYPE_MAX(qse_ssize_t)) size = QSE_TYPE_MAX(qse_ssize_t);
				if (qse_str_ncat (xtn->s.out.u.str.buf, data, size) == (qse_size_t)-1)
				{
					qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL);
					return -1;
				}
				n = size;
				break;

			default:
				/* this should never happen */
				qse_xli_seterrnum (xli, QSE_XLI_EINTERN, QSE_NULL);
				n = -1;
				break;
		}

		return n;
	}
	else
	{
		/* handle the included source file - @include */
		qse_ssize_t n;

		QSE_ASSERT (arg->name != QSE_NULL);
		QSE_ASSERT (arg->handle != QSE_NULL);

		n = qse_sio_putstrn (arg->handle, data, size);
		if (n <= -1)
		{
			qse_xstr_t ea;
			ea.ptr = arg->name;
			ea.len = qse_strlen(ea.ptr);
			qse_xli_seterrnum (xli, QSE_XLI_EIOFIL, &ea);
		}
		return n;
	}
}

static qse_ssize_t sf_out (
	qse_xli_t* xli, qse_xli_io_cmd_t cmd, 
	qse_xli_io_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	xtn_t* xtn = QSE_XTN (xli);

	QSE_ASSERT (arg != QSE_NULL);

	switch (cmd)
	{
		case QSE_XLI_IO_OPEN:
			return sf_out_open (xli, arg, xtn);

		case QSE_XLI_IO_CLOSE:
			return sf_out_close (xli, arg, xtn);

		case QSE_XLI_IO_WRITE:
			return sf_out_write (xli, arg, data, size, xtn);

		default:
			qse_xli_seterrnum (xli, QSE_XLI_EINTERN, QSE_NULL);
			return -1;
	}
}







int qse_xli_readstd (qse_xli_t* xli, qse_xli_iostd_t* in)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (xli);

	if (in == QSE_NULL || (in->type != QSE_XLI_IOSTD_FILE && 
	                       in->type != QSE_XLI_IOSTD_STR))
	{
		/* the input is a must. at least 1 file or 1 string 
		 * must be specified */
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
		return -1;
	}

	xtn->s.in.x = in;
	return qse_xli_read (xli, sf_in);
}

int qse_xli_writestd (qse_xli_t* xli, qse_xli_iostd_t* out)
{
	int n;
	xtn_t* xtn = (xtn_t*) QSE_XTN (xli);

	if (out == QSE_NULL || (out->type != QSE_XLI_IOSTD_FILE && 
	                        out->type != QSE_XLI_IOSTD_STR))
	{
		/* the input is a must. at least 1 file or 1 string 
		 * must be specified */
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
		return -1;
	}

	xtn->s.out.x = out;
	n = qse_xli_write (xli, sf_out);

	if (out->type == QSE_XLI_IOSTD_STR)
	{
		if (n >= 0)
		{
			QSE_ASSERT (xtn->s.out.u.str.buf != QSE_NULL);
			qse_str_yield (xtn->s.out.u.str.buf, &out->u.str, 0);
		}
		if (xtn->s.out.u.str.buf) qse_str_close (xtn->s.out.u.str.buf);
	}

	return n;
}
