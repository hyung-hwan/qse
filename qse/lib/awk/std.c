/*
 * $Id: std.c 538 2011-08-09 16:08:26Z hyunghwan.chung $
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

#include "awk.h"
#include <qse/awk/std.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/pio.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>
#include <qse/cmn/path.h>
#include <qse/cmn/stdio.h> /* TODO: remove dependency on qse_vsprintf */
#include "../cmn/mem.h"

#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#if defined(_WIN32)
#	include <tchar.h>
#endif

#ifndef QSE_HAVE_CONFIG_H
#	if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#		define HAVE_POW
#		define HAVE_FMOD
#		define HAVE_SIN
#		define HAVE_COS
#		define HAVE_TAN
#		define HAVE_ATAN
#		define HAVE_ATAN2
#		define HAVE_LOG
#		define HAVE_EXP
#		define HAVE_SQRT
#	endif
#endif

typedef struct xtn_t
{
	struct
	{
		struct 
		{
			qse_awk_parsestd_type_t type;
			union
			{
				const qse_char_t* file; 
				const qse_char_t* cp;
				struct 
				{
					const qse_char_t* ptr;	
					const qse_char_t* end;	
				} cpl;
			} u;
			qse_cstr_t dir;
			qse_sio_t* handle; /* the handle to an open file */
		} in;

		struct
		{
			qse_awk_parsestd_type_t type;
			union
			{
				const qse_char_t* file;
				qse_char_t*       cp;
				struct 
				{
					qse_xstr_t* osp;
					qse_char_t* ptr;	
					qse_char_t* end;	
				} cpl;
			} u;
			qse_sio_t* handle;
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
			qse_size_t count;
		} in; 

		struct 
		{
			const qse_char_t*const* files;
			qse_size_t index;
			qse_size_t count;
		} out;
	} c;  /* console */

} rxtn_t;

static qse_flt_t custom_awk_pow (qse_awk_t* awk, qse_flt_t x, qse_flt_t y)
{
#if defined(HAVE_POWL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return powl (x, y);
#elif defined(HAVE_POW)
	return pow (x, y);
#elif defined(HAVE_POWF)
	return powf (x, y);
#else
	#error ### no pow function available ###
#endif
}

static qse_flt_t custom_awk_mod (qse_awk_t* awk, qse_flt_t x, qse_flt_t y)
{
#if defined(HAVE_FMODL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return fmodl (x, y);
#elif defined(HAVE_FMOD)
	return fmod (x, y);
#elif defined(HAVE_FMODF)
	return fmodf (x, y);
#else
	#error ### no fmod function available ###
#endif
}

static qse_flt_t custom_awk_sin (qse_awk_t* awk, qse_flt_t x)
{
#if defined(HAVE_SINL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return sinl (x);
#elif defined(HAVE_SIN)
	return sin (x);
#elif defined(HAVE_SINF)
	return sinf (x);
#else
	#error ### no sin function available ###
#endif
}

static qse_flt_t custom_awk_cos (qse_awk_t* awk, qse_flt_t x)
{
#if defined(HAVE_COSL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return cosl (x);
#elif defined(HAVE_COS)
	return cos (x);
#elif defined(HAVE_COSF)
	return cosf (x);
#else
	#error ### no cos function available ###
#endif
}

static qse_flt_t custom_awk_tan (qse_awk_t* awk, qse_flt_t x)
{
#if defined(HAVE_TANL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return tanl (x);
#elif defined(HAVE_TAN)
	return tan (x);
#elif defined(HAVE_TANF)
	return tanf (x);
#else
	#error ### no tan function available ###
#endif
}

static qse_flt_t custom_awk_atan (qse_awk_t* awk, qse_flt_t x)
{
#if defined(HAVE_ATANL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return atanl (x);
#elif defined(HAVE_ATAN)
	return atan (x);
#elif defined(HAVE_ATANF)
	return atanf (x);
#else
	#error ### no atan function available ###
#endif
}

static qse_flt_t custom_awk_atan2 (qse_awk_t* awk, qse_flt_t x, qse_flt_t y)
{
#if defined(HAVE_ATAN2L) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return atan2l (x, y);
#elif defined(HAVE_ATAN2)
	return atan2 (x, y);
#elif defined(HAVE_ATAN2F)
	return atan2f (x, y);
#else
	#error ### no atan2 function available ###
#endif
}

static qse_flt_t custom_awk_log (qse_awk_t* awk, qse_flt_t x)
{
#if defined(HAVE_LOGL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return logl (x);
#elif defined(HAVE_LOG)
	return log (x);
#elif defined(HAVE_LOGF)
	return logf (x);
#else
	#error ### no log function available ###
#endif
}

static qse_flt_t custom_awk_exp (qse_awk_t* awk, qse_flt_t x)
{
#if defined(HAVE_EXPL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return expl (x);
#elif defined(HAVE_EXP)
	return exp (x);
#elif defined(HAVE_EXPF)
	return expf (x);
#else
	#error ### no exp function available ###
#endif
}

static qse_flt_t custom_awk_sqrt (qse_awk_t* awk, qse_flt_t x)
{
#if defined(HAVE_SQRTL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return sqrtl (x);
#elif defined(HAVE_SQRT)
	return sqrt (x);
#elif defined(HAVE_SQRTF)
	return sqrtf (x);
#else
	#error ### no sqrt function available ###
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

static int add_functions (qse_awk_t* awk);

qse_awk_t* qse_awk_openstd (qse_size_t xtnsize)
{
	return qse_awk_openstdwithmmgr (QSE_MMGR_GETDFL(), xtnsize);
}

qse_awk_t* qse_awk_openstdwithmmgr (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_awk_t* awk;
	qse_awk_prm_t prm;
	xtn_t* xtn;

	prm.sprintf  = custom_awk_sprintf;

	prm.math.pow = custom_awk_pow;
	prm.math.mod = custom_awk_mod;
	prm.math.sin = custom_awk_sin;
	prm.math.cos = custom_awk_cos;
	prm.math.tan = custom_awk_tan;
	prm.math.atan = custom_awk_atan;
	prm.math.atan2 = custom_awk_atan2;
	prm.math.log = custom_awk_log;
	prm.math.exp = custom_awk_exp;
	prm.math.sqrt = custom_awk_sqrt;

	/* create an object */
	awk = qse_awk_open (mmgr, QSE_SIZEOF(xtn_t) + xtnsize, &prm);
	if (awk == QSE_NULL) return QSE_NULL;

	/* initialize extension */
	xtn = (xtn_t*) QSE_XTN (awk);
	QSE_MEMSET (xtn, 0, QSE_SIZEOF(xtn_t));

	/* add intrinsic functions */
	if (add_functions (awk) <= -1)
	{
		qse_awk_close (awk);
		return QSE_NULL;
	}

	return awk;
}

void* qse_awk_getxtnstd (qse_awk_t* awk)
{
	return (void*)((xtn_t*)QSE_XTN(awk) + 1);
}

static qse_sio_t* open_sio (qse_awk_t* awk, const qse_char_t* file, int flags)
{
	qse_sio_t* sio;
	sio = qse_sio_open (awk->mmgr, 0, file, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t errarg;
		errarg.ptr = file;
		errarg.len = qse_strlen(file);
		qse_awk_seterrnum (awk, QSE_AWK_EOPEN, &errarg);
	}
	return sio;
}

static qse_sio_t* open_sio_rtx (qse_awk_rtx_t* rtx, const qse_char_t* file, int flags)
{
	qse_sio_t* sio;
	sio = qse_sio_open (rtx->awk->mmgr, 0, file, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t errarg;
		errarg.ptr = file;
		errarg.len = qse_strlen(file);
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EOPEN, &errarg);
	}
	return sio;
}

static const qse_char_t* sio_std_names[] =
{
	QSE_T("stdin"),
	QSE_T("stdout"),
	QSE_T("stderr"),
};

static qse_sio_t* open_sio_std (qse_awk_t* awk, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;
	sio = qse_sio_openstd (awk->mmgr, 0, std, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t ea;
		ea.ptr = sio_std_names[std];
		ea.len = qse_strlen (sio_std_names[std]);
		qse_awk_seterrnum (awk, QSE_AWK_EOPEN, &ea);
	}
	return sio;
}

static qse_sio_t* open_sio_std_rtx (qse_awk_rtx_t* rtx, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;

	sio = qse_sio_openstd (rtx->awk->mmgr, 0, std, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t ea;
		ea.ptr = sio_std_names[std];
		ea.len = qse_strlen (sio_std_names[std]);
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EOPEN, &ea);
	}
	return sio;
}

/*** PARSESTD ***/
static qse_ssize_t sf_in_open (
	qse_awk_t* awk, qse_awk_sio_arg_t* arg, xtn_t* xtn)
{
	if (arg == QSE_NULL || arg->name == QSE_NULL)
	{
		switch (xtn->s.in.type)
		{
			case QSE_AWK_PARSESTD_FILE:
				if (xtn->s.in.u.file == QSE_NULL) return -1;

				if (xtn->s.in.u.file[0] == QSE_T('-') &&
				    xtn->s.in.u.file[1] == QSE_T('\0'))
				{
					/* special file name '-' */
					xtn->s.in.handle = open_sio_std (
						awk, QSE_SIO_STDIN, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
					if (xtn->s.in.handle == QSE_NULL) return -1;
				}
				else
				{
					const qse_char_t* base;

					xtn->s.in.handle = open_sio (
						awk, xtn->s.in.u.file,
						QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR
					);
					if (xtn->s.in.handle == QSE_NULL) return -1;

					base = qse_basename (xtn->s.in.u.file);
					if (base != xtn->s.in.u.file)
					{
						xtn->s.in.dir.ptr = xtn->s.in.u.file;
						xtn->s.in.dir.len = base - xtn->s.in.u.file;
					}
				}
				return 1;

			case QSE_AWK_PARSESTD_STDIO:
				xtn->s.in.handle = open_sio_std (
					awk, QSE_SIO_STDIN, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
				if (xtn->s.in.handle == QSE_NULL) return -1;
				return 1;

			case QSE_AWK_PARSESTD_CP:
			case QSE_AWK_PARSESTD_CPL:
				xtn->s.in.handle = QSE_NULL;
				return 1;
		}

		/* this should never happen */
		qse_awk_seterrnum (awk, QSE_AWK_EINTERN, QSE_NULL);
		return -1;
	}
	else
	{
		const qse_char_t* file = arg->name;
		qse_char_t fbuf[64];
		qse_char_t* dbuf = QSE_NULL;
	
		if (xtn->s.in.dir.len > 0 && arg->name[0] != QSE_T('/'))
		{
			qse_size_t tmplen, totlen;
			
			totlen = qse_strlen(arg->name) + xtn->s.in.dir.len;
			if (totlen >= QSE_COUNTOF(fbuf))
			{
				dbuf = QSE_MMGR_ALLOC (
					awk->mmgr,
					QSE_SIZEOF(qse_char_t) * (totlen + 1)
				);
				if (dbuf == QSE_NULL)
				{
					qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
					return -1;
				}

				file = dbuf;
			}
			else file = fbuf;

			tmplen = qse_strncpy (
				(qse_char_t*)file,
				xtn->s.in.dir.ptr,
				xtn->s.in.dir.len
			);
			qse_strcpy ((qse_char_t*)file + tmplen, arg->name);
		}

		arg->handle = qse_sio_open (
			awk->mmgr, 0, file, QSE_SIO_READ
		);

		if (dbuf != QSE_NULL) QSE_MMGR_FREE (awk->mmgr, dbuf);
		if (arg->handle == QSE_NULL)
		{
			qse_cstr_t ea;
			ea.ptr = arg->name;
			ea.len = qse_strlen(ea.ptr);
			qse_awk_seterrnum (awk, QSE_AWK_EOPEN, &ea);
			return -1;
		}

		return 1;
	}
}

static qse_ssize_t sf_in_close (
	qse_awk_t* awk, qse_awk_sio_arg_t* arg, xtn_t* xtn)
{
	if (arg == QSE_NULL || arg->name == QSE_NULL)
	{
		if (xtn->s.in.handle) qse_sio_close (xtn->s.in.handle);
	}
	else
	{
		QSE_ASSERT (arg->handle != QSE_NULL);
		qse_sio_close (arg->handle);
	}

	return 0;
}

static qse_ssize_t sf_in_read (
	qse_awk_t* awk, qse_awk_sio_arg_t* arg,
	qse_char_t* data, qse_size_t size, xtn_t* xtn)
{
	if (arg == QSE_NULL || arg->name == QSE_NULL)
	{
		switch (xtn->s.in.type)
		{
			case QSE_AWK_PARSESTD_FILE:
			case QSE_AWK_PARSESTD_STDIO:
			{
				qse_ssize_t n;

				QSE_ASSERT (xtn->s.in.handle != QSE_NULL);
				n = qse_sio_getstrn (xtn->s.in.handle, data, size);
				if (n == -1)
				{
					qse_cstr_t ea;
					ea.ptr = xtn->s.in.u.file;
					ea.len = qse_strlen(ea.ptr);
					qse_awk_seterrnum (awk, QSE_AWK_EREAD, &ea);
				}
				return n;
			}

			case QSE_AWK_PARSESTD_CP:
			{
				qse_size_t n = 0;
				while (n < size && *xtn->s.in.u.cp != QSE_T('\0'))
				{
					data[n++] = *xtn->s.in.u.cp++;
				}
				return n;
			}
		
			case QSE_AWK_PARSESTD_CPL:
			{
				qse_size_t n = 0;
				while (n < size && xtn->s.in.u.cpl.ptr < xtn->s.in.u.cpl.end)
				{
					data[n++] = *xtn->s.in.u.cpl.ptr++;
				}
				return n;
			}
		}

		qse_awk_seterrnum (awk, QSE_AWK_EINTERN, QSE_NULL);
		return -1;
	}
	else
	{
		qse_ssize_t n;

		QSE_ASSERT (arg->handle != QSE_NULL);
		n = qse_sio_getstrn (arg->handle, data, size);
		if (n == -1)
		{
			qse_cstr_t ea;
			ea.ptr = arg->name;
			ea.len = qse_strlen(ea.ptr);
			qse_awk_seterrnum (awk, QSE_AWK_EREAD, &ea);
		}
		return n;
	}
}

static qse_ssize_t sf_in (
	qse_awk_t* awk, qse_awk_sio_cmd_t cmd, 
	qse_awk_sio_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	xtn_t* xtn = QSE_XTN (awk);

	switch (cmd)
	{
		case QSE_AWK_SIO_OPEN:
			return sf_in_open (awk, arg, xtn);

		case QSE_AWK_SIO_CLOSE:
			return sf_in_close (awk, arg, xtn);

		case QSE_AWK_SIO_READ:
			return sf_in_read (awk, arg, data, size, xtn);

	}

	qse_awk_seterrnum (awk, QSE_AWK_EINTERN, QSE_NULL);
	return -1;
}

static qse_ssize_t sf_out (
	qse_awk_t* awk, qse_awk_sio_cmd_t cmd, 
	qse_awk_sio_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	xtn_t* xtn = QSE_XTN (awk);

	switch (cmd)
	{
		case QSE_AWK_SIO_OPEN:
		{
			switch (xtn->s.out.type)
			{
				case QSE_AWK_PARSESTD_FILE:
					if (xtn->s.out.u.file == QSE_NULL) return -1;
	
					if (xtn->s.out.u.file[0] == QSE_T('-') &&
					    xtn->s.out.u.file[1] == QSE_T('\0'))
					{
						/* special file name '-' */
						xtn->s.out.handle = open_sio_std (
							awk, QSE_SIO_STDOUT, 
							QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR
						);
						if (xtn->s.out.handle == QSE_NULL) return -1;
					}
					else
					{
						xtn->s.out.handle = open_sio (
							awk, xtn->s.out.u.file, 
							QSE_SIO_WRITE | QSE_SIO_CREATE | 
							QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR
						);
						if (xtn->s.out.handle == QSE_NULL) return -1;
					}
					return 1;

				case QSE_AWK_PARSESTD_STDIO:
					xtn->s.out.handle = open_sio_std (
						awk, QSE_SIO_STDOUT,
						QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR
					);
					if (xtn->s.out.handle == QSE_NULL) return -1;
					return 1;

				case QSE_AWK_PARSESTD_CP:
				case QSE_AWK_PARSESTD_CPL:
					xtn->s.out.handle = QSE_NULL;
					return 1;
			}

			break;
		}


		case QSE_AWK_SIO_CLOSE:
		{
			switch (xtn->s.out.type)
			{
				case QSE_AWK_PARSESTD_FILE:
				case QSE_AWK_PARSESTD_STDIO:
	
					qse_sio_flush (xtn->s.out.handle);
					qse_sio_close (xtn->s.out.handle);
					return 0;

				case QSE_AWK_PARSESTD_CP:
					*xtn->s.out.u.cp = QSE_T('\0');
					return 0;
	
				case QSE_AWK_PARSESTD_CPL:
					xtn->s.out.u.cpl.osp->len = 
						xtn->s.out.u.cpl.ptr -
						xtn->s.out.u.cpl.osp->ptr;
					return 0;
			}

			break;
		}

		case QSE_AWK_SIO_WRITE:
		{
			switch (xtn->s.out.type)
			{
				case QSE_AWK_PARSESTD_FILE:
				case QSE_AWK_PARSESTD_STDIO:
				{
					qse_ssize_t n;
					QSE_ASSERT (xtn->s.out.handle != QSE_NULL);
					n = qse_sio_putstrn (xtn->s.out.handle, data, size);
					if (n == -1)
					{
						qse_cstr_t ea;
						ea.ptr = xtn->s.in.u.file;
						ea.len = qse_strlen(ea.ptr);
						qse_awk_seterrnum (awk, QSE_AWK_EWRITE, &ea);
					}
	
					return n;
				}
	
				case QSE_AWK_PARSESTD_CP:
				{
					qse_size_t n = 0;
					while (n < size && *xtn->s.out.u.cp != QSE_T('\0'))
					{
						*xtn->s.out.u.cp++ = data[n++];
					}
					return n;
				}

				case QSE_AWK_PARSESTD_CPL:
				{
					qse_size_t n = 0;
					while (n < size && xtn->s.out.u.cpl.ptr < xtn->s.out.u.cpl.end)
					{
						*xtn->s.out.u.cpl.ptr++ = data[n++];
					}
					return n;
				}
			}

			break;
		}
	}

	qse_awk_seterrnum (awk, QSE_AWK_EINTERN, QSE_NULL);
	return -1;
}

int qse_awk_parsestd (
	qse_awk_t* awk, 
        const qse_awk_parsestd_in_t* in,
        qse_awk_parsestd_out_t* out)
{
	qse_awk_sio_t sio;
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);

	if (in == QSE_NULL)
	{
		/* the input is a must */
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
		return -1;
	}

	switch (in->type)
	{
		case QSE_AWK_PARSESTD_FILE:
			xtn->s.in.u.file = in->u.file;
			break;

		case QSE_AWK_PARSESTD_CP:
			xtn->s.in.u.cp = in->u.cp;
			break;

		case QSE_AWK_PARSESTD_CPL:
			xtn->s.in.u.cpl.ptr = in->u.cpl.ptr;
			xtn->s.in.u.cpl.end = in->u.cpl.ptr + in->u.cpl.len;
			break;

		case QSE_AWK_PARSESTD_STDIO:
			/* nothing to do */
			break;

		default:
			qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
			return -1;
	}

	xtn->s.in.type = in->type;
	xtn->s.in.handle = QSE_NULL;
	sio.in = sf_in;

	if (out == QSE_NULL) sio.out = QSE_NULL;
	else
	{
		switch (out->type)
		{
			case QSE_AWK_PARSESTD_FILE:
				xtn->s.out.u.file = out->u.file;
				break;
	
			case QSE_AWK_PARSESTD_CP:
				xtn->s.out.u.cp = out->u.cp;
				break;
	
			case QSE_AWK_PARSESTD_CPL:
				xtn->s.out.u.cpl.osp = &out->u.cpl;
				xtn->s.out.u.cpl.ptr = out->u.cpl.ptr;
				xtn->s.out.u.cpl.end = out->u.cpl.ptr + out->u.cpl.len;
				break;

			case QSE_AWK_PARSESTD_STDIO:
				/* nothing to do */
				break;
	
			default:
				qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
				return -1;
		}
		xtn->s.out.type = out->type;
		xtn->s.out.handle = QSE_NULL;
		sio.out = sf_out;
	}

	return qse_awk_parse (awk, &sio);
}

/*** RTX_OPENSTD ***/
static qse_ssize_t awk_rio_pipe (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
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

			handle = qse_pio_open (
				rtx->awk->mmgr,
				0, 
				riod->name, 
				QSE_NULL,
				flags|QSE_PIO_SHELL|QSE_PIO_TEXT
			);

			if (handle == QSE_NULL) return -1;
			riod->handle = (void*)handle;
			return 1;
		}

		case QSE_AWK_RIO_CLOSE:
		{
			qse_pio_t* pio = (qse_pio_t*)riod->handle;
			if (riod->mode == QSE_AWK_RIO_PIPE_RW)
			{
				/* specialy treatment is needef for rwpipe.
				 * inspect rwcmode to see if partial closing is
				 * requested. */
				if (riod->rwcmode == QSE_AWK_RIO_CLOSE_READ)
				{
					qse_pio_end (pio, QSE_PIO_IN);
					return 0;
				}
				if (riod->rwcmode == QSE_AWK_RIO_CLOSE_WRITE)
				{
					qse_pio_end (pio, QSE_PIO_OUT);
					return 0;
				}
			}

			qse_pio_close (pio);
			riod->handle = QSE_NULL;
			return 0;
		}

		case QSE_AWK_RIO_READ:
		{
			return qse_pio_read (
				(qse_pio_t*)riod->handle,
				QSE_PIO_OUT, data, size
			);
		}

		case QSE_AWK_RIO_WRITE:
		{
			return qse_pio_write (
				(qse_pio_t*)riod->handle,
				QSE_PIO_IN, data, size
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
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
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

				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EOPEN, &errarg);
				return -1;
			}

			riod->handle = (void*)handle;
			return 1;
		}

		case QSE_AWK_RIO_CLOSE:
		{
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

static int open_rio_console (qse_awk_rtx_t* rtx, qse_awk_rio_arg_t* riod)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	qse_sio_t* sio;

	if (riod->mode == QSE_AWK_RIO_CONSOLE_READ)
	{
		if (rxtn->c.in.files == QSE_NULL)
		{
			/* if no input files is specified, 
			 * open the standard input */
			QSE_ASSERT (rxtn->c.in.index == 0);

			if (rxtn->c.in.count == 0)
			{
				sio = open_sio_std_rtx (
					rtx, QSE_SIO_STDIN,
					QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR
				);
				if (sio == QSE_NULL) return -1;

				riod->handle = sio;
				rxtn->c.in.count++;
				return 1;
			}

			return 0;
		}
		else
		{
			/* a temporary variable sio is used here not to change 
			 * any fields of riod when the open operation fails */
			const qse_char_t* file;
			qse_awk_val_t* argv;
			qse_htb_t* map;
			qse_htb_pair_t* pair;
			qse_char_t ibuf[128];
			qse_size_t ibuflen;
			qse_awk_val_t* v;
			qse_awk_rtx_valtostr_out_t out;

		nextfile:
			file = rxtn->c.in.files[rxtn->c.in.index];

			if (file == QSE_NULL)
			{
				/* no more input file */

				if (rxtn->c.in.count == 0)
				{
					/* all ARGVs are empty strings. 
					 * so no console files were opened.
					 * open the standard input here.
					 *
					 * 'BEGIN { ARGV[1]=""; ARGV[2]=""; }
					 *        { print $0; }' file1 file2
					 */
					sio = open_sio_std_rtx (
						rtx, QSE_SIO_STDIN,
						QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR
					);
					if (sio == QSE_NULL) return -1;

					riod->handle = sio;
					rxtn->c.in.count++;
					return 1;
				}

				return 0;
			}

			/* handle special case when ARGV[x] has been altered.
			 * so from here down, the file name gotten from 
			 * rxtn->c.in.files is not important and is overridden 
			 * from ARGV.
			 * 'BEGIN { ARGV[1]="file3"; } 
			 *        { print $0; }' file1 file2
			 */
			argv = qse_awk_rtx_getgbl (rtx, QSE_AWK_GBL_ARGV);
			QSE_ASSERT (argv != QSE_NULL);
			QSE_ASSERT (argv->type == QSE_AWK_VAL_MAP);

			map = ((qse_awk_val_map_t*)argv)->map;
			QSE_ASSERT (map != QSE_NULL);
			
			ibuflen = qse_awk_longtostr (
				rtx->awk, rxtn->c.in.index + 1, 10, QSE_NULL,
				ibuf, QSE_COUNTOF(ibuf));

			pair = qse_htb_search (map, ibuf, ibuflen);
			QSE_ASSERT (pair != QSE_NULL);

			v = QSE_HTB_VPTR(pair);
			QSE_ASSERT (v != QSE_NULL);

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr (rtx, v, &out) <= -1) return -1;

			if (out.u.cpldup.len == 0)
			{
				/* the name is empty */
				qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
				rxtn->c.in.index++;
				goto nextfile;
			}

			if (qse_strlen(out.u.cpldup.ptr) < out.u.cpldup.len)
			{
				/* the name contains one or more '\0' */
				qse_cstr_t errarg;

				errarg.ptr = out.u.cpldup.ptr;
				/* use this length not to contains '\0'
				 * in an error message */
				errarg.len = qse_strlen(out.u.cpldup.ptr);

				qse_awk_rtx_seterrnum (
					rtx, QSE_AWK_EIONMNL, &errarg);

				qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
				return -1;
			}

			file = out.u.cpldup.ptr;

			sio = (file[0] == QSE_T('-') && file[1] == QSE_T('\0'))?
				open_sio_std_rtx (rtx, QSE_SIO_STDIN, 
					QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR):
				open_sio_rtx (rtx, file,
					QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
			if (sio == QSE_NULL)
			{
				qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
				return -1;
			}

			if (qse_awk_rtx_setfilename (
				rtx, file, qse_strlen(file)) <= -1)
			{
				qse_sio_close (sio);
				qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
				return -1;
			}

			qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
			riod->handle = sio;

			/* increment the counter of files successfully opened */
			rxtn->c.in.count++;
			rxtn->c.in.index++;
			return 1;
		}

	}
	else if (riod->mode == QSE_AWK_RIO_CONSOLE_WRITE)
	{
		if (rxtn->c.out.files == QSE_NULL)
		{
			QSE_ASSERT (rxtn->c.out.index == 0);

			if (rxtn->c.out.count == 0)
			{
				sio = open_sio_std_rtx (
					rtx, QSE_SIO_STDOUT,
					QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR
				);
				if (sio == QSE_NULL) return -1;

				riod->handle = sio;
				rxtn->c.out.count++;
				return 1;
			}

			return 0;
		}
		else
		{
			/* a temporary variable sio is used here not to change 
			 * any fields of riod when the open operation fails */
			qse_sio_t* sio;
			const qse_char_t* file;

			file = rxtn->c.out.files[rxtn->c.out.index];

			if (file == QSE_NULL)
			{
				/* no more input file */
				return 0;
			}

			sio = (file[0] == QSE_T('-') && file[1] == QSE_T('\0'))?
				open_sio_std_rtx (
					rtx, QSE_SIO_STDOUT,
					QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR):
				open_sio_rtx (
					rtx, file, 
					QSE_SIO_WRITE | QSE_SIO_CREATE | 
					QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR);
			if (sio == QSE_NULL) return -1;
			
			if (qse_awk_rtx_setofilename (
				rtx, file, qse_strlen(file)) <= -1)
			{
				qse_sio_close (sio);
				return -1;
			}

			riod->handle = sio;
			rxtn->c.out.index++;
			rxtn->c.out.count++;
			return 1;
		}

	}

	return -1;
}

static qse_ssize_t awk_rio_console (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
	qse_char_t* data, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_AWK_RIO_OPEN:
			return open_rio_console (rtx, riod);

		case QSE_AWK_RIO_CLOSE:
			if (riod->handle) qse_sio_close ((qse_sio_t*)riod->handle);
			return 0;

		case QSE_AWK_RIO_READ:
		{
			qse_ssize_t nn;

			while ((nn = qse_sio_getstrn((qse_sio_t*)riod->handle,data,size)) == 0)
			{
				int n;
				qse_sio_t* sio = (qse_sio_t*)riod->handle;
	
				n = open_rio_console (rtx, riod);
				if (n <= -1) return -1;
	
				if (n == 0) 
				{
					/* no more input console */
					return 0;
				}
	
				if (sio) qse_sio_close (sio);
			}

			return nn;
		}

		case QSE_AWK_RIO_WRITE:
			return qse_sio_putstrn (	
				(qse_sio_t*)riod->handle,
				data,
				size
			);

		case QSE_AWK_RIO_FLUSH:
			return qse_sio_flush ((qse_sio_t*)riod->handle);

		case QSE_AWK_RIO_NEXT:
		{
			int n;
			qse_sio_t* sio = (qse_sio_t*)riod->handle;
	
			n = open_rio_console (rtx, riod);
			if (n <= -1) return -1;
	
			if (n == 0) 
			{
				/* if there is no more file, keep the previous handle */
				return 0;
			}

			if (sio) qse_sio_close (sio);
			return n;
		}

	}

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINTERN, QSE_NULL);
	return -1;
}

qse_awk_rtx_t* qse_awk_rtx_openstd (
	qse_awk_t*             awk,
	qse_size_t             xtnsize,
	const qse_char_t*      id,
	const qse_char_t*const icf[],
	const qse_char_t*const ocf[])
{
	qse_awk_rtx_t* rtx;
	qse_awk_rio_t rio;
	rxtn_t* rxtn;
	qse_ntime_t now;

	const qse_char_t*const* p;
	qse_size_t argc = 0;
	qse_cstr_t argv[16];
	qse_cstr_t* argvp = QSE_NULL, * p2;
	
	rio.pipe    = awk_rio_pipe;
	rio.file    = awk_rio_file;
	rio.console = awk_rio_console;

	if (icf != QSE_NULL)
	{
		for (p = icf; *p != QSE_NULL; p++);
		argc = p - icf;
	}

	argc++; /* for id */

	if (argc < QSE_COUNTOF(argv)) argvp = argv;
	else
	{
		argvp = QSE_AWK_ALLOC (
			awk, QSE_SIZEOF(*argvp) * (argc + 1));
		if (argvp == QSE_NULL)
		{
			qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
			return QSE_NULL;
		}
	}

	p2 = argvp;

	p2->ptr = id;
	p2->len = qse_strlen(id);
	p2++;
	
	if (icf != QSE_NULL)
	{
		for (p = icf; *p != QSE_NULL; p++, p2++) 
		{
			p2->ptr = *p;
			p2->len = qse_strlen(*p);
		}
	}

	p2->ptr = QSE_NULL;
	p2->len = 0;

	rtx = qse_awk_rtx_open (
		awk, 
		QSE_SIZEOF(rxtn_t) + xtnsize,
		&rio,
		argvp
	);

	if (argvp != QSE_NULL && argvp != argv) QSE_AWK_FREE (awk, argvp);
	if (rtx == QSE_NULL) return QSE_NULL;

	rxtn = (rxtn_t*) QSE_XTN (rtx);
	QSE_MEMSET (rxtn, 0, QSE_SIZEOF(rxtn_t));

	if (qse_gettime (&now) <= -1) rxtn->seed = 0;
	else rxtn->seed = (unsigned int) now;
	srand (rxtn->seed);

	rxtn->c.in.files = icf;
	rxtn->c.in.index = 0;
	rxtn->c.in.count = 0;
	rxtn->c.out.files = ocf;
	rxtn->c.out.index = 0;
	rxtn->c.out.count = 0;

	
	/* FILENAME can be set when the input console is opened.
	 * so we skip setting it here even if an explicit console file
	 * is specified.  So the value of FILENAME is empty in the 
	 * BEGIN block unless getline is ever met. 
	 *
	 * However, OFILENAME is different. The output
	 * console is treated as if it's already open upon start-up. 
	 * otherwise, 'BEGIN { print OFILENAME; }' prints an empty
	 * string regardless of output console files specified.
	 * That's because OFILENAME is evaluated before the output
	 * console file is opened. 
	 */
	if (ocf && ocf[0])
	{
		if (qse_awk_rtx_setofilename (rtx, ocf[0], qse_strlen(ocf[0])) <= -1)
		{
			awk->errinf = rtx->errinf; /* transfer error info */
			qse_awk_rtx_close (rtx);
			return QSE_NULL;
		}
	}

	return rtx;
}

void* qse_awk_rtx_getxtnstd (qse_awk_rtx_t* rtx)
{
	return (void*)((rxtn_t*)QSE_XTN(rtx) + 1);
}

static int fnc_rand (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	qse_awk_val_t* r;

	/*
	rxtn_t* rxtn;
	rxtn = (rxtn_t*) QSE_XTN (run);
	r = qse_awk_rtx_makefltval (
		run, (qse_flt_t)(rand_r(rxtn->seed) % RAND_MAX) / RAND_MAX );
	*/
	r = qse_awk_rtx_makefltval (
		run, (qse_flt_t)(rand() % RAND_MAX) / RAND_MAX);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_srand (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
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
		n = qse_awk_rtx_valtolong (run, a0, &lv);
		if (n <= -1) return -1;

		rxtn->seed = lv;
	}
	else
	{
		qse_ntime_t now;
		if (qse_gettime(&now) <= -1) rxtn->seed >>= 1;
		else rxtn->seed = (unsigned int)now;
	}

	srand (rxtn->seed);

	r = qse_awk_rtx_makeintval (run, prev);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int fnc_system (qse_awk_rtx_t* rtx, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* v;
	qse_char_t* str, * ptr, * end;
	qse_size_t len;
	int n = 0;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1);
	
	v = qse_awk_rtx_getarg (rtx, 0);
	if (v->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)v)->val.ptr;
		len = ((qse_awk_val_str_t*)v)->val.len;
	}
	else
	{
		str = qse_awk_rtx_valtocpldup (rtx, v, &len);
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
		qse_mchar_t* mbs;
		mbs = qse_wcstombsdup (str, rtx->awk->mmgr);
		if (mbs == QSE_NULL) 
		{
			n = -1;
			goto skip_system;
		}
		n = system (mbs);
		QSE_AWK_FREE (rtx->awk, mbs);
	}

#endif

skip_system:
	if (v->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (rtx->awk, str);

	v = qse_awk_rtx_makeintval (rtx, (qse_long_t)n);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

#define ADDFNC(awk,name,min,max,fnc) \
        if (qse_awk_addfnc (\
		(awk), (name), qse_strlen(name), \
		0, (min), (max), QSE_NULL, (fnc)) == QSE_NULL) return -1;

static int add_functions (qse_awk_t* awk)
{
	ADDFNC (awk, QSE_T("rand"),       0, 0, fnc_rand);
	ADDFNC (awk, QSE_T("srand"),      0, 1, fnc_srand);
	ADDFNC (awk, QSE_T("system"),     1, 1, fnc_system);
	return 0;
}
