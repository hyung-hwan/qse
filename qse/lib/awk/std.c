/*
 * $Id$
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

#include "awk.h"
#include <qse/cmn/sio.h>
#include <qse/cmn/pio.h>
#include <qse/cmn/str.h>
#include <qse/cmn/time.h>
#include <qse/utl/stdio.h> /* TODO: remove dependency on qse_vsprintf */

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

typedef struct xtn_t
{
	struct
	{
		struct 
		{
			int                     type;
			union
			{
				const qse_char_t*const* files; 
				const qse_char_t* str;
				qse_cstr_t cstr;
			} u;
			qse_size_t              index;  /* current file index */
			qse_sio_t*              handle; /* the handle to an open file */
		} in;

		struct
		{
			union
			{
				const qse_char_t* file;
				qse_char_t*       str;
				qse_cstr_t        cstr;
			} u;
			qse_sio_t*              handle;
		} out;

	} s;
} xtn_t;


typedef struct rxtn_t
{
	unsigned int seed;	

	struct
	{
		struct {
			const qse_char_t*const* files;
			qse_size_t index;
		} in; 

		struct 
		{
			const qse_char_t*const* files;
			qse_size_t index;
		} out;
	} c;  /* console */

} rxtn_t;

const qse_char_t* qse_awk_rtx_opensimple_stdio[] = 
{ 
	QSE_T(""), 
	QSE_NULL 
};

static qse_real_t custom_awk_pow (qse_awk_t* awk, qse_real_t x, qse_real_t y)
{
#if defined(HAVE_POWL)
	return powl (x, y);
#elif defined(HAVE_POW)
	return pow (x, y);
#elif defined(HAVE_POWF)
	return powf (x, y);
#else
	#error ### no pow function available ###
#endif
}

static int custom_awk_sprintf (
	qse_awk_t* awk, qse_char_t* buf, qse_size_t size, 
	const qse_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = qse_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

static qse_bool_t custom_awk_isccls (
	qse_awk_t* awk, qse_cint_t c, qse_ccls_id_t id)
{
	qse_ccls_t* ccls = QSE_CCLS_GETDFL();
	return ccls->is (ccls->data, c, id);
}

static qse_cint_t custom_awk_toccls (
	qse_awk_t* awk, qse_cint_t c, qse_ccls_id_t id)
{
	qse_ccls_t* ccls = QSE_CCLS_GETDFL();
	return ccls->to (ccls->data, c, id);
}

static int add_functions (qse_awk_t* awk);

qse_awk_t* qse_awk_opensimple (void)
{
	qse_awk_t* awk;
	qse_awk_prm_t prm;
	xtn_t* xtn;

	prm.pow     = custom_awk_pow;
	prm.sprintf = custom_awk_sprintf;
	prm.isccls  = custom_awk_isccls;
	prm.toccls  = custom_awk_toccls;

	/* create an object */
	awk = qse_awk_open (QSE_MMGR_GETDFL(), QSE_SIZEOF(xtn_t), &prm);
	if (awk == QSE_NULL) return QSE_NULL;

	/* initialize extension */
	xtn = (xtn_t*) QSE_XTN (awk);
	QSE_MEMSET (xtn, 0, QSE_SIZEOF(xtn_t));

	/* set default options */
	qse_awk_setoption (awk, QSE_AWK_CLASSIC);

	/* add intrinsic functions */
	if (add_functions (awk) == -1)
	{
		qse_awk_close (awk);
		return QSE_NULL;
	}

	return awk;
}

/*** PARSESIMPLE ***/

static qse_ssize_t sf_in (
	qse_awk_t* awk, qse_awk_sio_cmd_t cmd, 
	qse_char_t* data, qse_size_t size)
{
	xtn_t* xtn = QSE_XTN (awk);

	if (cmd == QSE_AWK_SIO_OPEN)
	{
		if (xtn->s.in.type == QSE_AWK_PARSESIMPLE_FILE)
		{
			if (xtn->s.in.u.files == QSE_NULL) return -1;
			if (xtn->s.in.u.files[xtn->s.in.index] == QSE_NULL) return 0;

			if (xtn->s.in.u.files[xtn->s.in.index][0] == QSE_T('\0'))
			{
				xtn->s.in.handle = qse_sio_in;
			}
			else
			{
				xtn->s.in.handle = qse_sio_open (
					awk->mmgr,
					0,
					xtn->s.in.u.files[xtn->s.in.index],
					QSE_SIO_READ
				);
				if (xtn->s.in.handle == QSE_NULL) return -1;
			}

			//qse_awk_setsource (awk, xtn->s.in.u.files[xtn->s.in.index]);
		}

		return 1;
	}
	else if (cmd == QSE_AWK_SIO_CLOSE)
	{
		if (xtn->s.in.handle != QSE_NULL && 
		    xtn->s.in.handle != qse_sio_in &&
		    xtn->s.in.handle != qse_sio_out &&
		    xtn->s.in.handle != qse_sio_err) 
		{
			qse_sio_close (xtn->s.in.handle);
		}

		return 0;
	}
	else if (cmd == QSE_AWK_SIO_READ)
	{
		qse_ssize_t n = 0;

		if (xtn->s.in.type == QSE_AWK_PARSESIMPLE_FILE)
		{
			qse_sio_t* sio;

		retry:
			sio = xtn->s.in.handle;

			n = qse_sio_getsn (sio, data, size);
			if (n == 0 && xtn->s.in.u.files[++xtn->s.in.index] != QSE_NULL)
			{
				if (sio != qse_sio_in) qse_sio_close (sio);
				if (xtn->s.in.u.files[xtn->s.in.index][0] == QSE_T('\0'))
				{
					xtn->s.in.handle = qse_sio_in;
				}
				else
				{
					xtn->s.in.handle = qse_sio_open (
						awk->mmgr,
						0,
						xtn->s.in.u.files[xtn->s.in.index],
						QSE_SIO_READ
					);
					if (xtn->s.in.handle == QSE_NULL) return -1;
				}

				/* TODO: reset internal line counters...
					set new source name....
					qse_awk_setsinname ();
				*/

				goto retry;
			}
		}
		else
		{
			while (n < size && xtn->s.in.u.str[xtn->s.in.index] != QSE_T('\0'))
			{
				data[n++] = xtn->s.in.u.str[xtn->s.in.index++];
			}
		}

		return n;
	}

	return -1;
}

static qse_ssize_t sf_out (
	qse_awk_t* awk, qse_awk_sio_cmd_t cmd,
	qse_char_t* data, qse_size_t size)
{
	xtn_t* xtn = QSE_XTN (awk);

	if (cmd == QSE_AWK_SIO_OPEN) 
	{
		if (xtn->s.out.file[0] == QSE_T('\0'))
		{
			xtn->s.out.handle = qse_sio_out;
		}
		else 
		{
			xtn->s.out.handle = qse_sio_open (
				awk->mmgr,
				0,
				xtn->s.out.file, 
				QSE_SIO_WRITE|QSE_SIO_CREATE|QSE_SIO_TRUNCATE
			);
			if (xtn->s.out.handle == QSE_NULL) return -1;
		}

		return 1;
	}
	else if (cmd == QSE_AWK_SIO_CLOSE) 
	{
		if (xtn->s.out.handle != QSE_NULL)
		{
			qse_sio_flush (xtn->s.out.handle);
			if (xtn->s.out.handle != qse_sio_in &&
			    xtn->s.out.handle != qse_sio_out &&
			    xtn->s.out.handle != qse_sio_err) 
			{
				qse_sio_close (xtn->s.out.handle);
			}
		}

		return 0;
	}
	else if (cmd == QSE_AWK_SIO_WRITE)
	{
		return qse_sio_putsn (xtn->s.out.handle, data, size);
	}

	return -1;
}

int qse_awk_parsesimple (
	qse_awk_t* awk, 
	qse_awk_parsesimple_type_t ist,
	const void*                isp,
	qse_awk_parsesimple_type_t ost,
	const void*                osp)
{
	qse_awk_sio_t sio;
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);

	if (isp == QSE_NULL)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL);
		return -1;
	}

	if (ist == QSE_AWK_PARSESIMPLE_FILE) 
	{
		xtn->s.in.u.files = (const qse_char_t*const*)isp;
	}
	else if (ist == QSE_AWK_PARSESIMPLE_STR) 
	{
		xtn->s.in.u.str = (const qse_char_t*)isp;
	}
	else if (ist == QSE_AWK_PARSESIMPLE_STRL)
	{
		xtn->s.in.u.cstr.ptr = (const qse_cstr_t*)isp->ptr;
		xtn->s.in.u.cstr.len = (const qse_cstr_t*)isp->len;
	}
	else
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL);
		return -1;
	}

	if (ost == QSE_AWK_PARSESIMPLE_FILE)
	{
	}
	else if (ost == QSE_AWK_PARSESIMPLE_STR)
	{
	}
	else if (ost == QSE_AWK_PARSESIMPLE_STRL)
	{
	}
	else if (ost == QSE_AWK_PARSESIMPLE_NONE)
	{
	}
	else
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL);
		return -1;
	}

	xtn->s.in.type = ist;
	xtn->s.in.index = 0;
	xtn->s.in.handle = QSE_NULL;

	xtn->s.out.file = osf;
	xtn->s.out.handle = QSE_NULL;
	
        sio.in = sf_in;
        sio.out = (osf == QSE_NULL)? QSE_NULL: sf_out;

	return qse_awk_parse (awk, &sio);
}

/*** RUNSIMPLE ***/

static qse_ssize_t awk_rio_pipe (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_riod_t* riod,
	qse_char_t* data, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_AWK_RIO_OPEN:
		{
			qse_pio_t* handle;
			int flags;

			if (riod->mode == QSE_AWK_RIO_PIPE_READ)
			{
				/* TODO: should we specify ERRTOOUT? */
				flags = QSE_PIO_READOUT | 
				        QSE_PIO_ERRTOOUT;
			}
			else if (riod->mode == QSE_AWK_RIO_PIPE_WRITE)
			{
				flags = QSE_PIO_WRITEIN;
			}
			else if (riod->mode == QSE_AWK_RIO_PIPE_RW)
			{
				flags = QSE_PIO_READOUT | 
				        QSE_PIO_ERRTOOUT |
				        QSE_PIO_WRITEIN;
			}
			else return -1; /* TODO: any way to set the error number? */

			/*dprint (QSE_T("opening %s of type %d (pipe)\n"),  riod->name, riod->type);*/

			handle = qse_pio_open (
				rtx->awk->mmgr,
				0, 
				riod->name, 
				flags|QSE_PIO_SHELL|QSE_PIO_TEXT
			);

			if (handle == QSE_NULL) return -1;
			riod->handle = (void*)handle;
			return 1;
		}

		case QSE_AWK_RIO_CLOSE:
		{
			/*dprint (QSE_T("closing %s of type (pipe) %d\n"),  riod->name, riod->type);*/
			qse_pio_close ((qse_pio_t*)riod->handle);
			riod->handle = QSE_NULL;
			return 0;
		}

		case QSE_AWK_RIO_READ:
		{
			return qse_pio_read (
				(qse_pio_t*)riod->handle,
				data,	
				size,
				QSE_PIO_OUT
			);
		}

		case QSE_AWK_RIO_WRITE:
		{
			return qse_pio_write (
				(qse_pio_t*)riod->handle,
				data,	
				size,
				QSE_PIO_IN
			);
		}

		case QSE_AWK_RIO_FLUSH:
		{
			/*if (riod->mode == QSE_AWK_RIO_PIPE_READ) return -1;*/
			return qse_pio_flush ((qse_pio_t*)riod->handle, QSE_PIO_IN);
		}

		case QSE_AWK_RIO_NEXT:
		{
			return -1;
		}
	}

	return -1;
}

static qse_ssize_t awk_rio_file (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_riod_t* riod,
	qse_char_t* data, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_AWK_RIO_OPEN:
		{
			qse_fio_t* handle;
			int flags;

			if (riod->mode == QSE_AWK_RIO_FILE_READ)
			{
				flags = QSE_FIO_READ;
			}
			else if (riod->mode == QSE_AWK_RIO_FILE_WRITE)
			{
				flags = QSE_FIO_WRITE |
				       QSE_FIO_CREATE |
				       QSE_FIO_TRUNCATE;
			}
			else if (riod->mode == QSE_AWK_RIO_FILE_APPEND)
			{
				flags = QSE_FIO_APPEND |
				       QSE_FIO_CREATE;
			}
			else return -1; /* TODO: any way to set the error number? */

			/*dprint (QSE_T("opening %s of type %d (file)\n"), riod->name, riod->type);*/
			handle = qse_fio_open (
				rtx->awk->mmgr,
				0,
				riod->name, 
				flags | QSE_FIO_TEXT,
				QSE_FIO_RUSR | QSE_FIO_WUSR | 
				QSE_FIO_RGRP | QSE_FIO_ROTH
			);
			if (handle == QSE_NULL) 
			{
				qse_cstr_t errarg;

				errarg.ptr = riod->name;
				errarg.len = qse_strlen(riod->name);

				qse_awk_rtx_seterror (rtx, QSE_AWK_EOPEN, 0, &errarg);
				return -1;
			}

			riod->handle = (void*)handle;
			return 1;
		}

		case QSE_AWK_RIO_CLOSE:
		{
			/*dprint (QSE_T("closing %s of type %d (file)\n"), riod->name, riod->type);*/
			qse_fio_close ((qse_fio_t*)riod->handle);
			riod->handle = QSE_NULL;
			return 0;
		}

		case QSE_AWK_RIO_READ:
		{
			return qse_fio_read (
				(qse_fio_t*)riod->handle,
				data,	
				size
			);
		}

		case QSE_AWK_RIO_WRITE:
		{
			return qse_fio_write (
				(qse_fio_t*)riod->handle,
				data,	
				size
			);
		}

		case QSE_AWK_RIO_FLUSH:
		{
			return qse_fio_flush ((qse_fio_t*)riod->handle);
		}

		case QSE_AWK_RIO_NEXT:
		{
			return -1;
		}

	}

	return -1;
}

static int open_rio_console (qse_awk_rtx_t* rtx, qse_awk_riod_t* riod)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);

	/*dprint (QSE_T("opening console[%s] of type %x\n"), riod->name, riod->type);*/

	if (riod->mode == QSE_AWK_RIO_CONSOLE_READ)
	{
		if (rxtn->c.in.files == QSE_NULL)
		{
			/* no input console file given */
			return -1;
		}

		if (rxtn->c.in.files[rxtn->c.in.index] == QSE_NULL)
		{
			/* no more input file */
			/*dprint (QSE_T("console - no more file\n"));*/
			return 0;
		}

		if (rxtn->c.in.files[rxtn->c.in.index][0] == QSE_T('\0'))
		{
			/*dprint (QSE_T("    console(r) - <standard input>\n"));*/
			riod->handle = qse_sio_in;
		}
		else
		{
			/* a temporary variable sio is used here not to change 
			 * any fields of riod when the open operation fails */
			qse_sio_t* sio;

			sio = qse_sio_open (
				rtx->awk->mmgr,
				0,
				rxtn->c.in.files[rxtn->c.in.index],
				QSE_SIO_READ
			);
			if (sio == QSE_NULL)
			{
				qse_cstr_t errarg;

				errarg.ptr = rxtn->c.in.files[rxtn->c.in.index];
				errarg.len = qse_strlen(rxtn->c.in.files[rxtn->c.in.index]);

				qse_awk_rtx_seterror (rtx, QSE_AWK_EOPEN, 0, &errarg);
				return -1;
			}

			/*dprint (QSE_T("    console(r) - %s\n"), rxtn->c.in.files[rxtn->c.in.index]);*/
			if (qse_awk_rtx_setfilename (
				rtx, rxtn->c.in.files[rxtn->c.in.index], 
				qse_strlen(rxtn->c.in.files[rxtn->c.in.index])) == -1)
			{
				qse_sio_close (sio);
				return -1;
			}

			riod->handle = sio;
		}

		rxtn->c.in.index++;
		return 1;
	}
	else if (riod->mode == QSE_AWK_RIO_CONSOLE_WRITE)
	{
		if (rxtn->c.out.files == QSE_NULL)
		{
			/* no output console file given */
			return -1;
		}

		if (rxtn->c.out.files[rxtn->c.out.index] == QSE_NULL)
		{
			/* no more input file */
			/*dprint (QSE_T("console - no more file\n"));*/
			return 0;
		}

		if (rxtn->c.out.files[rxtn->c.out.index][0] == QSE_T('\0'))
		{
			/*dprint (QSE_T("    console(w) - <standard output>\n"));*/
			riod->handle = qse_sio_out;
		}
		else
		{
			/* a temporary variable sio is used here not to change 
			 * any fields of riod when the open operation fails */
			qse_sio_t* sio;

			sio = qse_sio_open (
				rtx->awk->mmgr,
				0,
				rxtn->c.out.files[rxtn->c.out.index],
				QSE_SIO_READ
			);
			if (sio == QSE_NULL)
			{
				qse_cstr_t errarg;

				errarg.ptr = rxtn->c.out.files[rxtn->c.out.index];
				errarg.len = qse_strlen(rxtn->c.out.files[rxtn->c.out.index]);

				qse_awk_rtx_seterror (rtx, QSE_AWK_EOPEN, 0, &errarg);
				return -1;
			}

			/*dprint (QSE_T("    console(w) - %s\n"), rxtn->c.out.files[rxtn->c.out.index]);*/
			if (qse_awk_rtx_setofilename (
				rtx, rxtn->c.out.files[rxtn->c.out.index], 
				qse_strlen(rxtn->c.out.files[rxtn->c.out.index])) == -1)
			{
				qse_sio_close (sio);
				return -1;
			}

			riod->handle = sio;
		}

		rxtn->c.out.index++;
		return 1;
	}

	return -1;
}

static qse_ssize_t awk_rio_console (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_riod_t* riod,
	qse_char_t* data, qse_size_t size)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);

	if (cmd == QSE_AWK_RIO_OPEN)
	{
		return open_rio_console (rtx, riod);
	}
	else if (cmd == QSE_AWK_RIO_CLOSE)
	{
		/*dprint (QSE_T("closing console of type %x\n"), riod->type);*/

		if (riod->handle != QSE_NULL &&
		    riod->handle != qse_sio_in && 
		    riod->handle != qse_sio_out && 
		    riod->handle != qse_sio_err)
		{
			qse_sio_close ((qse_sio_t*)riod->handle);
		}

		return 0;
	}
	else if (cmd == QSE_AWK_RIO_READ)
	{
		qse_ssize_t n;

		while ((n = qse_sio_getsn((qse_sio_t*)riod->handle,data,size)) == 0)
		{
			/* it has reached the end of the current file.
			 * open the next file if available */
			if (rxtn->c.in.files[rxtn->c.in.index] == QSE_NULL) 
			{
				/* no more input console */
				return 0;
			}

			if (rxtn->c.in.files[rxtn->c.in.index][0] == QSE_T('\0'))
			{
				if (riod->handle != QSE_NULL &&
				    riod->handle != qse_sio_in &&
				    riod->handle != qse_sio_out &&
				    riod->handle != qse_sio_err) 
				{
					qse_sio_close ((qse_sio_t*)riod->handle);
				}

				riod->handle = qse_sio_in;
			}
			else
			{
				qse_sio_t* sio;

				sio = qse_sio_open (
					rtx->awk->mmgr,
					0,
					rxtn->c.in.files[rxtn->c.in.index],
					QSE_SIO_READ
				);

				if (sio == QSE_NULL)
				{
					qse_cstr_t errarg;

					errarg.ptr = rxtn->c.in.files[rxtn->c.in.index];
					errarg.len = qse_strlen(rxtn->c.in.files[rxtn->c.in.index]);

					qse_awk_rtx_seterror (rtx, QSE_AWK_EOPEN, 0, &errarg);
					return -1;
				}

				if (qse_awk_rtx_setfilename (
					rtx, rxtn->c.in.files[rxtn->c.in.index], 
					qse_strlen(rxtn->c.in.files[rxtn->c.in.index])) == -1)
				{
					qse_sio_close (sio);
					return -1;
				}

				if (qse_awk_rtx_setgbl (
					rtx, QSE_AWK_GBL_FNR, qse_awk_val_zero) == -1)
				{
					/* need to reset FNR */
					qse_sio_close (sio);
					return -1;
				}

				if (riod->handle != QSE_NULL &&
				    riod->handle != qse_sio_in &&
				    riod->handle != qse_sio_out &&
				    riod->handle != qse_sio_err) 
				{
					qse_sio_close ((qse_sio_t*)riod->handle);
				}

				/*dprint (QSE_T("open the next console [%s]\n"), rxtn->c.in.files[rxtn->c.in.index]);*/
				riod->handle = sio;
			}

			rxtn->c.in.index++;	
		}

		return n;
	}
	else if (cmd == QSE_AWK_RIO_WRITE)
	{
		return qse_sio_putsn (	
			(qse_sio_t*)riod->handle,
			data,
			size
		);
	}
	else if (cmd == QSE_AWK_RIO_FLUSH)
	{
		return qse_sio_flush ((qse_sio_t*)riod->handle);
	}
	else if (cmd == QSE_AWK_RIO_NEXT)
	{
		int n;
		qse_sio_t* sio = (qse_sio_t*)riod->handle;

		/*dprint (QSE_T("switching console[%s] of type %x\n"), riod->name, riod->type);*/

		n = open_rio_console (rtx, riod);
		if (n == -1) return -1;

		if (n == 0) 
		{
			/* if there is no more file, keep the previous handle */
			return 0;
		}

		if (sio != QSE_NULL && 
		    sio != qse_sio_in && 
		    sio != qse_sio_out &&
		    sio != qse_sio_err) 
		{
			qse_sio_close (sio);
		}

		return n;
	}

	return -1;
}

qse_awk_rtx_t* qse_awk_rtx_opensimple (
	qse_awk_t* awk, const qse_char_t* icf[], const qse_char_t* ocf[])
{
	qse_awk_rtx_t* rtx;
	qse_awk_rio_t rio;
	rxtn_t* rxtn;
	qse_ntime_t now;

	rio.pipe    = awk_rio_pipe;
	rio.file    = awk_rio_file;
	rio.console = awk_rio_console;

	rtx = qse_awk_rtx_open (
		awk, 
		QSE_SIZEOF(rxtn_t),
		&rio,
		QSE_NULL/*runarg*/
	);
	if (rtx == QSE_NULL) return QSE_NULL;

	rxtn = (rxtn_t*) QSE_XTN (rtx);
	QSE_MEMSET (rxtn, 0, QSE_SIZEOF(rxtn_t));

	if (qse_gettime (&now) == -1) rxtn->seed = 0;
	else rxtn->seed = (unsigned int) now;
	srand (rxtn->seed);

	rxtn->c.in.files = icf;
	rxtn->c.in.index = 0;
	rxtn->c.out.files = ocf;
	rxtn->c.out.index = 0;

	return rtx;
}

/*** EXTRA BUILTIN FUNCTIONS ***/
enum
{
	FNC_MATH_LD,
	FNC_MATH_D,
	FNC_MATH_F
};

static int fnc_math_1 (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl, int type, void* f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (run, 0);

	n = qse_awk_rtx_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 0) rv = (qse_real_t)lv;

	if (type == FNC_MATH_LD)
	{
		long double (*rf) (long double) = 
			(long double(*)(long double))f;
		r = qse_awk_rtx_makerealval (run, rf(rv));
	}
	else if (type == FNC_MATH_D)
	{
		double (*rf) (double) = (double(*)(double))f;
		r = qse_awk_rtx_makerealval (run, rf(rv));
	}
	else 
	{
		QSE_ASSERT (type == FNC_MATH_F);
		float (*rf) (float) = (float(*)(float))f;
		r = qse_awk_rtx_makerealval (run, rf(rv));
	}
	
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_math_2 (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl, int type, void* f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_long_t lv0, lv1;
	qse_real_t rv0, rv1;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 2);

	a0 = qse_awk_rtx_getarg (run, 0);
	a1 = qse_awk_rtx_getarg (run, 1);

	n = qse_awk_rtx_valtonum (run, a0, &lv0, &rv0);
	if (n == -1) return -1;
	if (n == 0) rv0 = (qse_real_t)lv0;

	n = qse_awk_rtx_valtonum (run, a1, &lv1, &rv1);
	if (n == -1) return -1;
	if (n == 0) rv1 = (qse_real_t)lv1;

	if (type == FNC_MATH_LD)
	{
		long double (*rf) (long double,long double) = 
			(long double(*)(long double,long double))f;
		r = qse_awk_rtx_makerealval (run, rf(rv0,rv1));
	}
	else if (type == FNC_MATH_D)
	{
		double (*rf) (double,double) = (double(*)(double,double))f;
		r = qse_awk_rtx_makerealval (run, rf(rv0,rv1));
	}
	else 
	{
		QSE_ASSERT (type == FNC_MATH_F);
		float (*rf) (float,float) = (float(*)(float,float))f;
		r = qse_awk_rtx_makerealval (run, rf(rv0,rv1));
	}
	
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_sin (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return fnc_math_1 (
		run, fnm, fnl, 
	#if defined(HAVE_SINL)
		FNC_MATH_LD, (void*)sinl
	#elif defined(HAVE_SIN)
		FNC_MATH_D, (void*)sin
	#elif defined(HAVE_SINF)
		FNC_MATH_F, (void*)sinf
	#else
		#error ### no sin function available ###
	#endif
	);
}

static int fnc_cos (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return fnc_math_1 (
		run, fnm, fnl, 
	#if defined(HAVE_COSL)
		FNC_MATH_LD, (void*)cosl
	#elif defined(HAVE_COS)
		FNC_MATH_D, (void*)cos
	#elif defined(HAVE_COSF)
		FNC_MATH_F, (void*)cosf
	#else
		#error ### no cos function available ###
	#endif
	);
}

static int fnc_tan (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return fnc_math_1 (
		run, fnm, fnl, 
	#if defined(HAVE_TANL)
		FNC_MATH_LD, (void*)tanl
	#elif defined(HAVE_TAN)
		FNC_MATH_D, (void*)tan
	#elif defined(HAVE_TANF)
		FNC_MATH_F, (void*)tanf
	#else
		#error ### no tan function available ###
	#endif
	);
}

static int fnc_atan (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return fnc_math_1 (
		run, fnm, fnl, 
	#if defined(HAVE_ATANL)
		FNC_MATH_LD, (void*)atanl
	#elif defined(HAVE_ATAN)
		FNC_MATH_D, (void*)atan
	#elif defined(HAVE_ATANF)
		FNC_MATH_F, (void*)atanf
	#else
		#error ### no atan function available ###
	#endif
	);
}

static int fnc_atan2 (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return fnc_math_2 (
		run, fnm, fnl, 
	#if defined(HAVE_ATAN2L)
		FNC_MATH_LD, (void*)atan2l
	#elif defined(HAVE_ATAN2)
		FNC_MATH_D, (void*)atan2
	#elif defined(HAVE_ATAN2F)
		FNC_MATH_F, (void*)atan2f
	#else
		#error ### no atan2 function available ###
	#endif
	);
}

static int fnc_log (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return fnc_math_1 (
		run, fnm, fnl, 
	#if defined(HAVE_LOGL)
		FNC_MATH_LD, (void*)logl
	#elif defined(HAVE_LOG)
		FNC_MATH_D, (void*)log
	#elif defined(HAVE_LOGF)
		FNC_MATH_F, (void*)logf
	#else
		#error ### no log function available ###
	#endif
	);
}

static int fnc_exp (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return fnc_math_1 (
		run, fnm, fnl, 
	#if defined(HAVE_EXPL)
		FNC_MATH_LD, (void*)expl
	#elif defined(HAVE_EXP)
		FNC_MATH_D, (void*)exp
	#elif defined(HAVE_EXPF)
		FNC_MATH_F, (void*)expf
	#else
		#error ### no exp function available ###
	#endif
	);
}

static int fnc_sqrt (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return fnc_math_1 (
		run, fnm, fnl, 
	#if defined(HAVE_SQRTL)
		FNC_MATH_LD, (void*)sqrtl
	#elif defined(HAVE_SQRT)
		FNC_MATH_D, (void*)sqrt
	#elif defined(HAVE_SQRTF)
		FNC_MATH_F, (void*)sqrtf
	#else
		#error ### no sqrt function available ###
	#endif
	);
}

static int fnc_int (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (run, 0);

	n = qse_awk_rtx_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (qse_long_t)rv;

	r = qse_awk_rtx_makeintval (run, lv);
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_rand (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_awk_val_t* r;

	r = qse_awk_rtx_makeintval (run, rand());
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_srand (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;
	unsigned int prev;
	rxtn_t* rxtn;

	rxtn = (rxtn_t*) QSE_XTN (run);
	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 0 || nargs == 1);

	prev = rxtn->seed;

	if (nargs == 1)
	{
		a0 = qse_awk_rtx_getarg (run, 0);

		n = qse_awk_rtx_valtonum (run, a0, &lv, &rv);
		if (n == -1) return -1;
		if (n == 1) lv = (qse_long_t)rv;

		rxtn->seed = lv;
	}
	else
	{
		qse_ntime_t now;

		if (qse_gettime(&now) == -1) rxtn->seed >>= 1;
		else rxtn->seed = (unsigned int)now;
	}

        srand (rxtn->seed);

	r = qse_awk_rtx_makeintval (run, prev);
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_system (qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* v;
	qse_char_t* str, * ptr, * end;
	qse_size_t len;
	int n = 0;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);
	
	v = qse_awk_rtx_getarg (run, 0);
	if (v->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)v)->ptr;
		len = ((qse_awk_val_str_t*)v)->len;
	}
	else
	{
		str = qse_awk_rtx_valtostr (
			run, v, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (str == QSE_NULL) return -1;
	}

	/* the target name contains a null character.
	 * make system return -1 */
	ptr = str; end = str + len;
	while (ptr < end)
	{
		if (*ptr == QSE_T('\0')) 
		{
			n = -1;
			goto skip_system;
		}

		ptr++;
	}

#if defined(_WIN32)
	n = _tsystem (str);
#elif defined(QSE_CHAR_IS_MCHAR)
	n = system (str);
#else
	{
		char* mbs;
		qse_size_t mbl;

		mbs = (char*) qse_awk_alloc (run->awk, len*5+1);
		if (mbs == QSE_NULL) 
		{
			n = -1;
			goto skip_system;
		}

		/* at this point, the string is guaranteed to be 
		 * null-terminating. so qse_wcstombs() can be used to convert
		 * the string, not qse_wcsntombsn(). */

		mbl = len * 5;
		if (qse_wcstombs (str, mbs, &mbl) != len && mbl >= len * 5) 
		{
			/* not the entire string is converted.
			 * mbs is not null-terminated properly. */
			n = -1;
			goto skip_system_mbs;
		}

		mbs[mbl] = '\0';
		n = system (mbs);

	skip_system_mbs:
		qse_awk_free (run->awk, mbs);
	}
#endif

skip_system:
	if (v->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);

	v = qse_awk_rtx_makeintval (run, (qse_long_t)n);
	if (v == QSE_NULL)
	{
		/*qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_rtx_setretval (run, v);
	return 0;
}

#define ADDFNC(awk,name,min,max,fnc) \
        if (qse_awk_addfnc (\
		(awk), (name), qse_strlen(name), \
		0, (min), (max), QSE_NULL, (fnc)) == QSE_NULL) return -1;

static int add_functions (qse_awk_t* awk)
{
        ADDFNC (awk, QSE_T("sin"),        1, 1, fnc_sin);
        ADDFNC (awk, QSE_T("cos"),        1, 1, fnc_cos);
        ADDFNC (awk, QSE_T("tan"),        1, 1, fnc_tan);
        ADDFNC (awk, QSE_T("atan"),       1, 1, fnc_atan);
        ADDFNC (awk, QSE_T("atan2"),      2, 2, fnc_atan2);
        ADDFNC (awk, QSE_T("log"),        1, 1, fnc_log);
        ADDFNC (awk, QSE_T("exp"),        1, 1, fnc_exp);
        ADDFNC (awk, QSE_T("sqrt"),       1, 1, fnc_sqrt);
        ADDFNC (awk, QSE_T("int"),        1, 1, fnc_int);
        ADDFNC (awk, QSE_T("rand"),       0, 0, fnc_rand);
        ADDFNC (awk, QSE_T("srand"),      0, 1, fnc_srand);
        ADDFNC (awk, QSE_T("system"),     1, 1, fnc_system);
	return 0;
}
