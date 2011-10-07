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

typedef struct xtn_in_t xtn_in_t;
struct xtn_in_t
{
	qse_sed_iostd_t* ptr;
	qse_sed_iostd_t* cur;
	qse_size_t       mempos;
};

typedef struct xtn_out_t xtn_out_t;
struct xtn_out_t
{
	qse_sed_iostd_t* ptr;
	qse_str_t*       memstr;
};

typedef struct xtn_t xtn_t;
struct xtn_t
{
	struct
	{
		xtn_in_t in;
		qse_char_t last;
		int newline_squeezed;
	} s;
	struct
	{
		xtn_in_t  in;
		xtn_out_t out;
	} e;
};


static int int_to_str (qse_size_t val, qse_char_t* buf, qse_size_t buflen)
{
	qse_size_t t;
	qse_size_t rlen = 0;

	t = val;
	if (t == 0) rlen++;
	else
	{
		/* non-zero values */
		if (t < 0) { t = -t; rlen++; }
		while (t > 0) { rlen++; t /= 10; }
	}

	if (rlen >= buflen) return -1; /* buffer too small */

	buf[rlen] = QSE_T('\0');

	t = val;
	if (t == 0) buf[0] = QSE_T('0'); 
	else
	{
		if (t < 0) t = -t;

		/* fill in the buffer with digits */
		while (t > 0) 
		{
			buf[--rlen] = (qse_char_t)(t % 10) + QSE_T('0');
			t /= 10;
		}

		/* insert the negative sign if necessary */
		if (val < 0) buf[--rlen] = QSE_T('-');
	}

	return 0;
}

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

static int verify_iostd_in  (qse_sed_t* sed, qse_sed_iostd_t in[])
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

	return 0;
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

static qse_sio_t* open_sio_std (qse_sed_t* sed, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;
	static const qse_char_t* std_names[] =
	{
		QSE_T("stdin"),
		QSE_T("stdout"),
		QSE_T("stderr"),
	};

	sio = qse_sio_openstd (sed->mmgr, 0, std, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t ea;
		ea.ptr = std_names[std];
		ea.len = qse_strlen (std_names[std]);
		qse_sed_seterrnum (sed, QSE_SED_EIOFIL, &ea);
	}
	return sio;
}

static void close_main_stream (
	qse_sed_t* sed, qse_sed_io_arg_t* arg, qse_sed_iostd_t* io)
{
	if (io->type == QSE_SED_IOSTD_FILE) qse_sio_close (arg->handle);
}

static int open_input_stream (
	qse_sed_t* sed, qse_sed_io_arg_t* arg, qse_sed_iostd_t* io, xtn_in_t* base)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);

	QSE_ASSERT (io != QSE_NULL);
	switch (io->type)
	{
		case QSE_SED_IOSTD_SIO:
			arg->handle = io->u.sio;
			break;

		case QSE_SED_IOSTD_FILE:
		{
			qse_sio_t* sio;
			sio = (io->u.file == QSE_NULL)?
				open_sio_std (sed, QSE_SIO_STDIN, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR):
				open_sio (sed, io->u.file, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
			if (sio == QSE_NULL) return -1;
			arg->handle = sio;
			break;
		}

		case QSE_SED_IOSTD_MEM:
			/* don't store anything to arg->handle */
			base->mempos = 0;
			break;

		default:
			QSE_ASSERTX (
				!"should never happen",
				"io-type must be one of SIO,FILE,MEM"
			);
			qse_sed_seterrnum (sed, QSE_SED_EINTERN, QSE_NULL);
			return -1;
	}


	if (base == &xtn->s.in)
	{
		/* reset script location */
		if (io->type == QSE_SED_IOSTD_FILE) 
		{
			qse_sed_setcompid (sed, io->u.file);
		}
		else 
		{
			qse_char_t buf[64];
			buf[0] = (io->type == QSE_SED_IOSTD_MEM)? QSE_T('M'): QSE_T('S');
			buf[1] = QSE_T('#');
			if (int_to_str (io - xtn->s.in.ptr, &buf[2], QSE_COUNTOF(buf) - 2) >= 0)
				qse_sed_setcompid (sed, buf);
		}
		sed->src.loc.line = 1; 
		sed->src.loc.colm = 1;
	}
	return 0;
}

static int open_output_stream (qse_sed_t* sed, qse_sed_io_arg_t* arg, qse_sed_iostd_t* io)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);

	QSE_ASSERT (io != QSE_NULL);
	switch (io->type)
	{
		case QSE_SED_IOSTD_SIO:
			arg->handle = io->u.sio;
			break;

		case QSE_SED_IOSTD_FILE:
		{
			qse_sio_t* sio;
			if (io->u.file == QSE_NULL)
			{
				sio = open_sio_std (
					sed, QSE_SIO_STDOUT,
					QSE_SIO_WRITE |
					QSE_SIO_CREATE |
					QSE_SIO_TRUNCATE |
					QSE_SIO_IGNOREMBWCERR
				);
			}
			else
			{
				sio = open_sio (
					sed, io->u.file,
					QSE_SIO_WRITE |
					QSE_SIO_CREATE |
					QSE_SIO_TRUNCATE |
					QSE_SIO_IGNOREMBWCERR
				);
			}
			if (sio == QSE_NULL) return -1;
			arg->handle = sio;
			break;
		}

		case QSE_SED_IOSTD_MEM:
			/* don't store anything to arg->handle */
			xtn->e.out.memstr = qse_str_open (sed->mmgr, 0, 512);
			if (xtn->e.out.memstr == QSE_NULL)
			{
				qse_sed_seterrnum (sed, QSE_SED_ENOMEM, QSE_NULL);
				return -1;
			}
			break;

		default:
			QSE_ASSERTX (
				!"should never happen",
				"io-type must be one of SIO,FILE,MEM"
			);
			qse_sed_seterrnum (sed, QSE_SED_EINTERN, QSE_NULL);
			return -1;
	}

	return 0;
}

static qse_ssize_t read_input_stream (
	qse_sed_t* sed, qse_sed_io_arg_t* arg,
	qse_char_t* buf, qse_size_t len, xtn_in_t* base)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);
	qse_sed_iostd_t* io, * next;
	void* old, * new;
	qse_ssize_t n = 0;

	if (len > QSE_TYPE_MAX(qse_ssize_t)) len = QSE_TYPE_MAX(qse_ssize_t);

	do
	{
		io = base->cur;

		if (base == &xtn->s.in && xtn->s.newline_squeezed) 
		{
			xtn->s.newline_squeezed = 0;
			goto open_next;
		}

		QSE_ASSERT (io != QSE_NULL);

		if (io->type == QSE_SED_IOSTD_MEM)
		{
			n = 0;
			while (base->mempos < io->u.mem.len && n < len)
				buf[n++] = io->u.mem.ptr[base->mempos++];
		}
		else n = qse_sio_getsn (arg->handle, buf, len);
		if (n != 0) 
		{
			if (n <= -1)
			{
				if (io->type == QSE_SED_IOSTD_FILE)
				{
					qse_cstr_t ea;
					ea.ptr = io->u.file;
					ea.len = qse_strlen (io->u.file);
					qse_sed_seterrnum (sed, QSE_SED_EIOFIL, &ea);
				}
			}
			else if (base == &xtn->s.in)
			{
				xtn->s.last = buf[n-1];
			}

			break;
		}
	
		/* ============================================= */
		/* == end of file on the current input stream == */
		/* ============================================= */

		if (base == &xtn->s.in && xtn->s.last != QSE_T('\n'))
		{
			/* TODO: different line termination convension */
			buf[0] = QSE_T('\n'); 
			n = 1;
			xtn->s.newline_squeezed = 1;
			break;
		}

	open_next:
		next = base->cur + 1;
		if (next->type == QSE_SED_IOSTD_NULL) 
		{
			/* no next stream available - return 0 */	
			break; 
		}

		old = arg->handle;

		/* try to open the next input stream */
		if (open_input_stream (sed, arg, next, base) <= -1)
		{
			/* failed to open the next input stream */
			if (next->type == QSE_SED_IOSTD_FILE)
			{
				qse_cstr_t ea;
				ea.ptr = next->u.file;
				ea.len = qse_strlen (next->u.file);
				qse_sed_seterrnum (sed, QSE_SED_EIOFIL, &ea);
			}

			n = -1;
			break;
		}

		/* successfuly opened the next input stream */
		new = arg->handle;

		arg->handle = old;

		/* close the previous stream */
		close_main_stream (sed, arg, io);

		arg->handle = new;

		base->cur++;
	}
	while (1);

	return n;
}

static qse_ssize_t s_in (
	qse_sed_t* sed, qse_sed_io_cmd_t cmd, qse_sed_io_arg_t* arg,
	qse_char_t* buf, qse_size_t len)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);

	switch (cmd)
	{
		case QSE_SED_IO_OPEN:
		{
			if (open_input_stream (sed, arg, xtn->s.in.cur, &xtn->s.in) <= -1) return -1;
			return 1;
		}

		case QSE_SED_IO_CLOSE:
		{
			close_main_stream (sed, arg, xtn->s.in.cur);
			return 0;
		}

		case QSE_SED_IO_READ:
		{
			return read_input_stream (sed, arg, buf, len, &xtn->s.in);
		}

		default:
		{
			QSE_ASSERTX (
				!"should never happen",
				"cmd must be one of OPEN,CLOSE,READ"
			);
			qse_sed_seterrnum (sed, QSE_SED_EINTERN, QSE_NULL);
			return -1;
		}
	}
}

static qse_ssize_t x_in (
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
				/* no file specified. console stream */
				if (xtn->e.in.ptr == QSE_NULL) 
				{
					sio = open_sio_std (
						sed, QSE_SIO_STDIN, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
					if (sio == QSE_NULL) return -1;
					arg->handle = sio;
				}
				else
				{
					if (open_input_stream (sed, arg, xtn->e.in.cur, &xtn->e.in) <= -1) return -1;
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
				if (xtn->e.in.ptr == QSE_NULL) 
					qse_sio_close (arg->handle);
				else
					close_main_stream (sed, arg, xtn->e.in.cur);
			}
			else
			{
				qse_sio_close (arg->handle);
			}

			return 0;
		}

		case QSE_SED_IO_READ:
		{
			if (arg->path == QSE_NULL)
			{
				/* main data stream */
				if (xtn->e.in.ptr == QSE_NULL)
				{
					qse_ssize_t n;
					n = qse_sio_getsn (arg->handle, buf, len);
					if (n <= -1)
					{
						qse_cstr_t ea;
						ea.ptr = QSE_T("stdin");
						ea.len = 5;
						qse_sed_seterrnum (sed, QSE_SED_EIOFIL, &ea);
					}
					return n;
				}
				else
					return read_input_stream (sed, arg, buf, len, &xtn->e.in);
			}
			else
			{
				qse_ssize_t n;
				n = qse_sio_getsn (arg->handle, buf, len);
				if (n <= -1)
				{
					qse_cstr_t ea;
					ea.ptr = arg->path;
					ea.len = qse_strlen (arg->path);
					qse_sed_seterrnum (sed, QSE_SED_EIOFIL, &ea);
				}
				return n;
			}
		}

		default:
			QSE_ASSERTX (
				!"should never happen",
				"cmd must be one of OPEN,CLOSE,READ"
			);
			qse_sed_seterrnum (sed, QSE_SED_EINTERN, QSE_NULL);
			return -1;
	}
}

static qse_ssize_t x_out (
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
				if (xtn->e.out.ptr== QSE_NULL) 
				{
					sio = open_sio_std (
						sed, QSE_SIO_STDOUT,
						QSE_SIO_WRITE |
						QSE_SIO_CREATE |
						QSE_SIO_TRUNCATE |
						QSE_SIO_IGNOREMBWCERR
					);
					if (sio == QSE_NULL) return -1;
					arg->handle = sio;
				}
				else
				{
					if (open_output_stream (sed, arg, xtn->e.out.ptr) <= -1) return -1;
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
				if (xtn->e.out.ptr== QSE_NULL) 
					qse_sio_close (arg->handle);
				else
					close_main_stream (sed, arg, xtn->e.out.ptr);
			}
			else
			{
				qse_sio_close (arg->handle);
			}
			return 0;
		}

		case QSE_SED_IO_WRITE:
		{
			if (arg->path == QSE_NULL)
			{
				/* main data stream */
				if (xtn->e.out.ptr== QSE_NULL)
				{
					qse_ssize_t n;
					n = qse_sio_putsn (arg->handle, dat, len);
					if (n <= -1)
					{
						qse_cstr_t ea;
						ea.ptr = QSE_T("stdin");
						ea.len = 5;
						qse_sed_seterrnum (sed, QSE_SED_EIOFIL, &ea);
					}
					return n;
				}
				else
				{
					qse_sed_iostd_t* io = xtn->e.out.ptr;
					if (io->type == QSE_SED_IOSTD_MEM)
					{
						if (len > QSE_TYPE_MAX(qse_ssize_t)) len = QSE_TYPE_MAX(qse_ssize_t);

						if (qse_str_ncat (xtn->e.out.memstr, dat, len) == (qse_size_t)-1)
						{
							qse_sed_seterrnum (sed, QSE_SED_ENOMEM, QSE_NULL);
							return -1;
						}

						return len;
					}
					else
					{
						qse_ssize_t n;
						n = qse_sio_putsn (arg->handle, dat, len);
						if (n <= -1)
						{
							qse_cstr_t ea;
							ea.ptr = io->u.file;
							ea.len = qse_strlen(io->u.file);
							qse_sed_seterrnum (sed, QSE_SED_EIOFIL, &ea);
						}
						return n;
					}
				}
			}
			else
			{
				qse_ssize_t n;
				n = qse_sio_putsn (arg->handle, dat, len);
				if (n <= -1)
				{
					qse_cstr_t ea;
					ea.ptr = arg->path;
					ea.len = qse_strlen(arg->path);
					qse_sed_seterrnum (sed, QSE_SED_EIOFIL, &ea);
				}
				return n;
			}
		}
	
		default:
			QSE_ASSERTX (
				!"should never happen",
				"cmd must be one of OPEN,CLOSE,WRITE"
			);
			qse_sed_seterrnum (sed, QSE_SED_EINTERN, QSE_NULL);
			return -1;
	}
}

int qse_sed_compstd (qse_sed_t* sed, qse_sed_iostd_t in[], qse_size_t* count)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);
	int ret;

	if (in == QSE_NULL)
	{
		/* it requires a valid array unlike qse_sed_execstd(). */
		qse_sed_seterrnum (sed, QSE_SED_EINVAL, QSE_NULL);
		return -1;
	}
	if (verify_iostd_in (sed, in) <= -1) return -1;

	QSE_MEMSET (&xtn->s, 0, QSE_SIZEOF(xtn->s));
	xtn->s.in.ptr = in;
	xtn->s.in.cur = in;

	ret = qse_sed_comp (sed, s_in);

	if (count) *count = xtn->s.in.cur - xtn->s.in.ptr;

	return ret;
}

int qse_sed_execstd (
	qse_sed_t* sed, qse_sed_iostd_t in[], qse_sed_iostd_t* out)
{
	int n;
	xtn_t* xtn = (xtn_t*) QSE_XTN (sed);

	if (in && verify_iostd_in (sed, in) <= -1) return -1;

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
	xtn->e.in.ptr = in;
	xtn->e.in.cur = in;
	xtn->e.out.ptr= out;

	n = qse_sed_exec (sed, x_in, x_out);

	if (n >= 0 && out && out->type == QSE_SED_IOSTD_MEM)
	{
		QSE_ASSERT (xtn->e.out.memstr != QSE_NULL);
		qse_str_yield (xtn->e.out.memstr, &out->u.mem, 0);
	}
	if (xtn->e.out.memstr) qse_str_close (xtn->e.out.memstr);

	return n;
}

int qse_sed_execstdfile (
	qse_sed_t* sed, const qse_char_t* infile, const qse_char_t* outfile)
{
	qse_sed_iostd_t in[2];
	qse_sed_iostd_t out;
	qse_sed_iostd_t* pin = QSE_NULL, * pout = QSE_NULL;

	if (infile)
	{
		in[0].type = QSE_SED_IOSTD_FILE;
		in[0].u.file = infile;
		in[1].type = QSE_SED_IOSTD_NULL;
		pin = in;
	}

	if (outfile)
	{
		out.type = QSE_SED_IOSTD_FILE;
		out.u.file = outfile;
		pout = &out;
	}

	return qse_sed_execstd (sed, pin, pout);
}
