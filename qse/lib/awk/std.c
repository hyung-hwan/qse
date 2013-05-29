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

#include "awk.h"
#include <qse/awk/stdawk.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/pio.h>
#include <qse/cmn/nwio.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>
#include <qse/cmn/path.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/env.h>
#include <qse/cmn/alg.h>
#include <qse/cmn/stdio.h> /* TODO: remove dependency on qse_vsprintf */
#include "../cmn/mem.h"

#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	if defined(QSE_HAVE_CONFIG_H)
#		include <ltdl.h>
#		define USE_LTDL
#	endif
#elif defined(__OS2__)
#	define INCL_DOSMODULEMGR
#	define INCL_DOSPROCESS
#	define INCL_DOSERRORS
#    include <os2.h>
#elif defined(__DOS__)
#	include <cwdllfnc.h>
#else
#	include <unistd.h>
#	include <ltdl.h>
#	define USE_LTDL
#endif

#if !defined(QSE_HAVE_CONFIG_H)
#	if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#		define HAVE_POW
#		define HAVE_FMOD
#		define HAVE_SIN
#		define HAVE_COS
#		define HAVE_TAN
#		define HAVE_ATAN
#		define HAVE_ATAN2
#		define HAVE_LOG
#		define HAVE_LOG10
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
			qse_awk_parsestd_t* x;
			qse_size_t          xindex;

			union
			{
				/* nothing to maintain here for file */

				struct 
				{
					const qse_char_t* ptr;	
					const qse_char_t* end;	
				} str;
			} u;
		} in;

		struct
		{
			qse_awk_parsestd_t* x;
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
	} s; /* script/source handling */

	int gbl_argc;
	int gbl_argv;
	int gbl_environ;

	qse_awk_ecb_t ecb;
} xtn_t;

typedef struct rxtn_t
{
	qse_long_t seed;	
	qse_ulong_t prand; /* last random value returned */

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

		qse_cmgr_t* cmgr;
	} c;  /* console */

	int cmgrtab_inited;
	qse_htb_t cmgrtab;

	qse_awk_rtx_ecb_t ecb;
} rxtn_t;

typedef struct ioattr_t
{
	qse_cmgr_t* cmgr;
	qse_char_t cmgr_name[64]; /* i assume that the cmgr name never exceeds this length */
	qse_ntime_t tmout[4];
} ioattr_t;

static ioattr_t* get_ioattr (qse_htb_t* tab, const qse_char_t* ptr, qse_size_t len);

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

static qse_flt_t custom_awk_log10 (qse_awk_t* awk, qse_flt_t x)
{
#if defined(HAVE_LOG10L) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return log10l (x);
#elif defined(HAVE_LOG10)
	return log10 (x);
#elif defined(HAVE_LOG10F)
	return log10f (x);
#else
	#error ### no log10 function available ###
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

static void* custom_awk_modopen (qse_awk_t* awk, const qse_awk_mod_spec_t* spec)
{
#if defined(QSE_ENABLE_STATIC_MODULE)
	/* this won't be called at all when modules are linked into
	 * the main library. */
	qse_awk_seterrnum (awk, QSE_AWK_ENOIMPL, QSE_NULL);
	return QSE_NULL;

#elif defined(USE_LTDL)
	void* h;
	qse_mchar_t* modpath;
	const qse_char_t* tmp[4];
	int count;

	count = 0;
	if (spec->prefix) tmp[count++] = spec->prefix;
	tmp[count++] = spec->name;
	if (spec->postfix) tmp[count++] = spec->postfix;
	tmp[count] = QSE_NULL;

	#if defined(QSE_CHAR_IS_MCHAR)
	modpath = qse_mbsadup (tmp, QSE_NULL, awk->mmgr);
	#else
	modpath = qse_wcsatombsdup (tmp, QSE_NULL, awk->mmgr);
	#endif
	if (!modpath)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	h = lt_dlopenext (modpath);

	QSE_MMGR_FREE (awk->mmgr, modpath);

	return h;

#elif defined(_WIN32)

	HMODULE h;
	qse_char_t* modpath;
	const qse_char_t* tmp[4];
	int count;

	count = 0;
	if (spec->prefix) tmp[count++] = spec->prefix;
	tmp[count++] = spec->name;
	if (spec->postfix) tmp[count++] = spec->postfix;
	tmp[count] = QSE_NULL;

	modpath = qse_stradup (tmp, QSE_NULL, awk->mmgr);
	if (!modpath)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	h = LoadLibrary (modpath);

	QSE_MMGR_FREE (awk->mmgr, modpath);
	
	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));
	return h;

#elif defined(__OS2__)

	HMODULE h;
	qse_mchar_t* modpath;
	const qse_char_t* tmp[4];
	int count;
	char errbuf[CCHMAXPATH];
	APIRET rc;

	count = 0;
	if (spec->prefix) tmp[count++] = spec->prefix;
	tmp[count++] = spec->name;
	if (spec->postfix) tmp[count++] = spec->postfix;
	tmp[count] = QSE_NULL;

	#if defined(QSE_CHAR_IS_MCHAR)
	modpath = qse_mbsadup (tmp, QSE_NULL, awk->mmgr);
	#else
	modpath = qse_wcsatombsdup (tmp, QSE_NULL, awk->mmgr);
	#endif
	if (!modpath)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	/* DosLoadModule() seems to have severe limitation on 
	 * the file name it can load (max-8-letters.xxx) */
	rc = DosLoadModule (errbuf, QSE_COUNTOF(errbuf) - 1, modpath, &h);
	if (rc != NO_ERROR) h = QSE_NULL;

	QSE_MMGR_FREE (awk->mmgr, modpath);

	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));
	return h;

#elif defined(__DOS__)

	void* h;
	qse_mchar_t* modpath;
	const qse_char_t* tmp[4];
	int count;

	count = 0;
	if (spec->prefix) tmp[count++] = spec->prefix;
	tmp[count++] = spec->name;
	if (spec->postfix) tmp[count++] = spec->postfix;
	tmp[count] = QSE_NULL;

	#if defined(QSE_CHAR_IS_MCHAR)
	modpath = qse_mbsadup (tmp, QSE_NULL, awk->mmgr);
	#else
	modpath = qse_wcsatombsdup (tmp, QSE_NULL, awk->mmgr);
	#endif
	if (!modpath)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	h = LoadModule (modpath);

	QSE_MMGR_FREE (awk->mmgr, modpath);
	
	QSE_ASSERT (QSE_SIZEOF(h) <= QSE_SIZEOF(void*));
	return h;

#else
	qse_awk_seterrnum (awk, QSE_AWK_ENOIMPL, QSE_NULL);
	return QSE_NULL;
#endif
}

static void custom_awk_modclose (qse_awk_t* awk, void* handle)
{
#if defined(QSE_ENABLE_STATIC_MODULE)
	/* this won't be called at all when modules are linked into
	 * the main library. */
#elif defined(USE_LTDL)
	lt_dlclose (handle);
#elif defined(_WIN32)
	FreeLibrary ((HMODULE)handle);
#elif defined(__OS2__)
	DosFreeModule ((HMODULE)handle);
#elif defined(__DOS__)
	FreeModule (handle);
#else
	/* nothing to do */
#endif
}

static void* custom_awk_modsym (qse_awk_t* awk, void* handle, const qse_char_t* name)
{
	void* s;
	qse_mchar_t* mname;

#if defined(QSE_CHAR_IS_MCHAR)
	mname = name;
#else
	mname = qse_wcstombsdup (name, QSE_NULL, awk->mmgr);	
	if (!mname)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}
#endif

#if defined(QSE_ENABLE_STATIC_MODULE)
	/* this won't be called at all when modules are linked into
	 * the main library. */
	s = QSE_NULL;

#elif defined(USE_LTDL)
	s = lt_dlsym (handle, mname);

#elif defined(_WIN32)
	s = GetProcAddress ((HMODULE)handle, mname);

#elif defined(__OS2__)
	if (DosQueryProcAddr ((HMODULE)handle, 0, mname, (PFN*)&s) != NO_ERROR) s = QSE_NULL;

#elif defined(__DOS__)
	s = GetProcAddress (handle, mname);
	
#else
	s = QSE_NULL;
#endif

#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
#else
	QSE_MMGR_FREE (awk->mmgr, mname);
#endif

	return s;
}

static int add_globals (qse_awk_t* awk);
static int add_functions (qse_awk_t* awk);

qse_awk_t* qse_awk_openstd (qse_size_t xtnsize)
{
	return qse_awk_openstdwithmmgr (QSE_MMGR_GETDFL(), xtnsize);
}

static void fini_xtn (qse_awk_t* awk)
{
	/* nothing to do */
#if defined(USE_LTDL)
	lt_dlexit ();
#endif
}

static void clear_xtn (qse_awk_t* awk)
{
	/* nothing to do */
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
	prm.math.log10 = custom_awk_log10;
	prm.math.exp = custom_awk_exp;
	prm.math.sqrt = custom_awk_sqrt;

	prm.modopen = custom_awk_modopen;
	prm.modclose = custom_awk_modclose;
	prm.modsym = custom_awk_modsym;

	/* create an object */
	awk = qse_awk_open (mmgr, QSE_SIZEOF(xtn_t) + xtnsize, &prm);
	if (awk == QSE_NULL) return QSE_NULL;

	/* initialize extension */
	xtn = (xtn_t*) QSE_XTN (awk);

	/* add intrinsic global variables and functions */
	if (add_globals(awk) <= -1 ||
	    add_functions (awk) <= -1) goto oops;

#if defined(USE_LTDL)
	/* lt_dlinit() can be called more than once and 
	 * lt_dlexit() shuts down libltdl if it's called as many times as
	 * corresponding lt_dlinit(). so it's safe to call lt_dlinit()
	 * and lt_dlexit() at the library level. */
	if (lt_dlinit () != 0) goto oops;
#endif

	xtn->ecb.close = fini_xtn;
	xtn->ecb.clear = clear_xtn;
	qse_awk_pushecb (awk, &xtn->ecb);

	return awk;

oops:
	if (awk) qse_awk_close (awk);
	return QSE_NULL;
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

static qse_cstr_t sio_std_names[] =
{
	{ QSE_T("stdin"),   5 },
	{ QSE_T("stdout"),  6 },
	{ QSE_T("stderr"),  6 }
};

static qse_sio_t* open_sio_std (qse_awk_t* awk, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;
	sio = qse_sio_openstd (awk->mmgr, 0, std, flags);
	if (sio == QSE_NULL) qse_awk_seterrnum (awk, QSE_AWK_EOPEN, &sio_std_names[std]);
	return sio;
}

static qse_sio_t* open_sio_std_rtx (qse_awk_rtx_t* rtx, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;

	sio = qse_sio_openstd (rtx->awk->mmgr, 0, std, flags);
	if (sio == QSE_NULL) qse_awk_rtx_seterrnum (rtx, QSE_AWK_EOPEN, &sio_std_names[std]);
	return sio;
}

/*** PARSESTD ***/

static int open_parsestd (qse_awk_t* awk, qse_awk_sio_arg_t* arg, xtn_t* xtn, qse_size_t index)
{
	qse_awk_parsestd_t* psin = &xtn->s.in.x[index];

	switch (psin->type)
	{
		/* normal source files */

		case QSE_AWK_PARSESTD_FILE:
			if (psin->u.file.path == QSE_NULL ||
			    (psin->u.file.path[0] == QSE_T('-') &&
			     psin->u.file.path[1] == QSE_T('\0')))
			{
				/* no path name or - -> stdin */
				qse_sio_t* tmp;

				tmp = open_sio_std (awk, QSE_SIO_STDIN, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
				if (tmp == QSE_NULL) return -1;

				if (index >= 1 && xtn->s.in.x[index-1].type == QSE_AWK_PARSESTD_FILE)
					qse_sio_close (arg->handle);

				arg->handle = tmp;
			}
			else
			{
				qse_sio_t* tmp;

				tmp = open_sio (awk, psin->u.file.path, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_KEEPPATH);
				if (tmp == QSE_NULL) return -1;

				if (index >= 1 && xtn->s.in.x[index-1].type == QSE_AWK_PARSESTD_FILE)
					qse_sio_close (arg->handle);
				arg->handle = tmp;
			}
			if (psin->u.file.cmgr) qse_sio_setcmgr (arg->handle, psin->u.file.cmgr);
			return 0;

		case QSE_AWK_PARSESTD_STR:
			if (index >= 1 && xtn->s.in.x[index-1].type == QSE_AWK_PARSESTD_FILE)
				qse_sio_close (arg->handle);

			xtn->s.in.u.str.ptr = psin->u.str.ptr;
			xtn->s.in.u.str.end = psin->u.str.ptr + psin->u.str.len;
			return 0;

		default:
			qse_awk_seterrnum (awk, QSE_AWK_EINTERN, QSE_NULL);
			return -1;
	}
}

static qse_ssize_t sf_in_open (
	qse_awk_t* awk, qse_awk_sio_arg_t* arg, xtn_t* xtn)
{
	if (arg->prev == QSE_NULL)
	{
		/* handle normal source input streams specified 
		 * to qse_awk_parsestd() */

		qse_ssize_t x;

		QSE_ASSERT (arg == &awk->sio.arg);

		x = open_parsestd (awk, arg, xtn, 0);
		if (x >= 0) 
		{
			xtn->s.in.xindex = 0; /* update the current stream index */
			/* perform some manipulation about the top-level input information */
			if (xtn->s.in.x[0].type == QSE_AWK_PARSESTD_FILE)
			{
				arg->name = xtn->s.in.x[0].u.file.path;
				if (arg->name == QSE_NULL) arg->name = sio_std_names[QSE_SIO_STDIN].ptr;
			}
			else arg->name = QSE_NULL;
		}

		return x;
	}
	else
	{
		/* handle the included source file - @include */
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

				base = qse_basename (outer);
				if (base != outer && arg->name[0] != QSE_T('/'))
				{
					qse_size_t tmplen, totlen, dirlen;
			
					dirlen = base - outer;
					totlen = qse_strlen(arg->name) + dirlen;
					if (totlen >= QSE_COUNTOF(fbuf))
					{
						dbuf = qse_awk_allocmem (
							awk, QSE_SIZEOF(qse_char_t) * (totlen + 1)
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
			awk->mmgr, 0, path, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_KEEPPATH
		);

		if (dbuf) QSE_MMGR_FREE (awk->mmgr, dbuf);
		if (arg->handle == QSE_NULL)
		{
			qse_cstr_t ea;
			ea.ptr = arg->name;
			ea.len = qse_strlen(ea.ptr);
			qse_awk_seterrnum (awk, QSE_AWK_EOPEN, &ea);
			return -1;
		}

		return 0;
	}
}

static qse_ssize_t sf_in_close (
	qse_awk_t* awk, qse_awk_sio_arg_t* arg, xtn_t* xtn)
{
	if (arg->prev == QSE_NULL)
	{
		switch (xtn->s.in.x[xtn->s.in.xindex].type)
		{
			case QSE_AWK_PARSESTD_FILE:
				QSE_ASSERT (arg->handle != QSE_NULL);
				qse_sio_close (arg->handle);
				break;

			case QSE_AWK_PARSESTD_STR:
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
	qse_awk_t* awk, qse_awk_sio_arg_t* arg,
	qse_char_t* data, qse_size_t size, xtn_t* xtn)
{
	if (arg->prev == QSE_NULL)
	{
		qse_ssize_t n;

		QSE_ASSERT (arg == &awk->sio.arg);

	again:
		switch (xtn->s.in.x[xtn->s.in.xindex].type)
		{
			case QSE_AWK_PARSESTD_FILE:
				QSE_ASSERT (arg->handle != QSE_NULL);
				n = qse_sio_getstrn (arg->handle, data, size);
				if (n <= -1)
				{
					qse_cstr_t ea;
					ea.ptr = xtn->s.in.x[xtn->s.in.xindex].u.file.path;
					if (ea.ptr == QSE_NULL) ea.ptr = sio_std_names[QSE_SIO_STDIN].ptr;
					ea.len = qse_strlen(ea.ptr);
					qse_awk_seterrnum (awk, QSE_AWK_EREAD, &ea);
				}
				break;

			case QSE_AWK_PARSESTD_STR:
				n = 0;
				while (n < size && xtn->s.in.u.str.ptr < xtn->s.in.u.str.end)
				{
					data[n++] = *xtn->s.in.u.str.ptr++;
				}
				break;

			default:
				/* this should never happen */
				qse_awk_seterrnum (awk, QSE_AWK_EINTERN, QSE_NULL);
				n = -1;
				break;
		}

		if (n == 0)
		{
		 	/* reached end of the current stream. */
			qse_size_t next = xtn->s.in.xindex + 1;
			if (xtn->s.in.x[next].type != QSE_AWK_PARSESTD_NULL)
			{
				/* open the next stream if available. */
				if (open_parsestd (awk, arg, xtn, next) <= -1) n = -1;
				else 
				{
					xtn->s.in.xindex = next; /* update the next to the current */

					/* update the I/O object name */
					if (xtn->s.in.x[next].type == QSE_AWK_PARSESTD_FILE)
					{
						arg->name = xtn->s.in.x[next].u.file.path;
						if (arg->name == QSE_NULL) arg->name = sio_std_names[QSE_SIO_STDIN].ptr;
					}
					else
						arg->name = QSE_NULL;

					arg->line = 0; /* reset the line number */
					arg->colm = 0;
					goto again;
				}
			}
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

		default:
			qse_awk_seterrnum (awk, QSE_AWK_EINTERN, QSE_NULL);
			return -1;
	}
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
			switch (xtn->s.out.x->type)
			{
				case QSE_AWK_PARSESTD_FILE:
					if (xtn->s.out.x->u.file.path == QSE_NULL ||
					    (xtn->s.out.x->u.file.path[0] == QSE_T('-') &&
					     xtn->s.out.x->u.file.path[1] == QSE_T('\0')))

					{
						/* no path name or - -> stdout */
						xtn->s.out.u.file.sio = open_sio_std (
							awk, QSE_SIO_STDOUT, 
							QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR
						);
						if (xtn->s.out.u.file.sio == QSE_NULL) return -1;
					}
					else
					{
						xtn->s.out.u.file.sio = open_sio (
							awk, xtn->s.out.x->u.file.path, 
							QSE_SIO_WRITE | QSE_SIO_CREATE | 
							QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR
						);
						if (xtn->s.out.u.file.sio == QSE_NULL) return -1;
					}

					if (xtn->s.out.x->u.file.cmgr)
						qse_sio_setcmgr (xtn->s.out.u.file.sio, xtn->s.out.x->u.file.cmgr);
					return 1;

				case QSE_AWK_PARSESTD_STR:
					xtn->s.out.u.str.buf = qse_str_open (awk->mmgr, 0, 512);
					if (xtn->s.out.u.str.buf == QSE_NULL)
					{
						qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
						return -1;
					}

					return 1;
			}

			break;
		}


		case QSE_AWK_SIO_CLOSE:
		{
			switch (xtn->s.out.x->type)
			{
				case QSE_AWK_PARSESTD_FILE:
					qse_sio_close (xtn->s.out.u.file.sio);
					return 0;

				case QSE_AWK_PARSESTD_STR:
					/* i don't close xtn->s.out.u.str.buf intentionally here.
					 * it will be closed at the end of qse_awk_parsestd() */
					return 0;
			}

			break;
		}

		case QSE_AWK_SIO_WRITE:
		{
			switch (xtn->s.out.x->type)
			{
				case QSE_AWK_PARSESTD_FILE:
				{
					qse_ssize_t n;
					QSE_ASSERT (xtn->s.out.u.file.sio != QSE_NULL);
					n = qse_sio_putstrn (xtn->s.out.u.file.sio, data, size);
					if (n <= -1)
					{
						qse_cstr_t ea;
						ea.ptr = xtn->s.out.x->u.file.path;
						if (ea.ptr == QSE_NULL) ea.ptr = sio_std_names[QSE_SIO_STDOUT].ptr;
						ea.len = qse_strlen(ea.ptr);
						qse_awk_seterrnum (awk, QSE_AWK_EWRITE, &ea);
					}
	
					return n;
				}
	
				case QSE_AWK_PARSESTD_STR:
				{
					if (size > QSE_TYPE_MAX(qse_ssize_t)) size = QSE_TYPE_MAX(qse_ssize_t);
					if (qse_str_ncat (xtn->s.out.u.str.buf, data, size) == (qse_size_t)-1)
					{
						qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
						return -1;
					}
					return size;
				}
			}

			break;
		}

		default:
			/* other code must not trigger this function */
			break;
	}

	qse_awk_seterrnum (awk, QSE_AWK_EINTERN, QSE_NULL);
	return -1;
}

int qse_awk_parsestd (
	qse_awk_t* awk, qse_awk_parsestd_t in[], qse_awk_parsestd_t* out)
{
	qse_awk_sio_t sio;
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	int n;

	if (in == QSE_NULL || (in[0].type != QSE_AWK_PARSESTD_FILE && 
	                       in[0].type != QSE_AWK_PARSESTD_STR))
	{
		/* the input is a must. at least 1 file or 1 string 
		 * must be specified */
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
		return -1;
	}

	sio.in = sf_in;
	xtn->s.in.x = in;

	if (out == QSE_NULL) sio.out = QSE_NULL;
	else
	{
		if (out->type != QSE_AWK_PARSESTD_FILE &&
		    out->type != QSE_AWK_PARSESTD_STR)
		{
			qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
			return -1;
		}
		sio.out = sf_out;
		xtn->s.out.x = out;
	}

	n = qse_awk_parse (awk, &sio);

	if (out && out->type == QSE_AWK_PARSESTD_STR)
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

/*** RTX_OPENSTD ***/

static qse_ssize_t nwio_handler_open (
	qse_awk_rtx_t* rtx, qse_awk_rio_arg_t* riod, int flags, 
	qse_nwad_t* nwad, qse_nwio_tmout_t* tmout)
{
	qse_nwio_t* handle;

	handle = qse_nwio_open (
		qse_awk_rtx_getmmgr(rtx), 0, nwad, 
		flags | QSE_NWIO_TEXT | QSE_NWIO_IGNOREMBWCERR |
		QSE_NWIO_REUSEADDR | QSE_NWIO_READNORETRY | QSE_NWIO_WRITENORETRY,
		tmout
	);
	if (handle == QSE_NULL) return -1;

#if defined(QSE_CHAR_IS_WCHAR)
	{
		qse_cmgr_t* cmgr = qse_awk_rtx_getcmgrstd (rtx, riod->name);
		if (cmgr) qse_nwio_setcmgr (handle, cmgr);
	}
#endif

	riod->handle = (void*)handle;
	riod->uflags = 1; /* nwio indicator */
	return 1;
}

static qse_ssize_t nwio_handler_rest (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
	qse_char_t* data, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_AWK_RIO_OPEN:
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINTERN, QSE_NULL);
			return -1;
		}

		case QSE_AWK_RIO_CLOSE:
		{
			qse_nwio_close ((qse_nwio_t*)riod->handle);
			return 0;
		}

		case QSE_AWK_RIO_READ:
		{
			return qse_nwio_read ((qse_nwio_t*)riod->handle, data, size);
		}

		case QSE_AWK_RIO_WRITE:
		{
			return qse_nwio_write ((qse_nwio_t*)riod->handle, data, size);
		}

		case QSE_AWK_RIO_FLUSH:
		{
			/*if (riod->mode == QSE_AWK_RIO_PIPE_READ) return -1;*/
			return qse_nwio_flush ((qse_nwio_t*)riod->handle);
		}

		case QSE_AWK_RIO_NEXT:
			break;
	}

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINTERN, QSE_NULL);
	return -1;
}

static int parse_rwpipe_uri (const qse_char_t* uri, int* flags, qse_nwad_t* nwad)
{
	static struct
	{
		const qse_char_t* prefix;
		qse_size_t        len;
		int               flags;
	} x[] =
	{
		{ QSE_T("tcp://"),  6, QSE_NWIO_TCP },
		{ QSE_T("udp://"),  6, QSE_NWIO_UDP },
		{ QSE_T("tcpd://"), 7, QSE_NWIO_TCP | QSE_NWIO_PASSIVE },
		{ QSE_T("udpd://"), 7, QSE_NWIO_UDP | QSE_NWIO_PASSIVE }
	};
	int i;


	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		if (qse_strzcmp (uri, x[i].prefix, x[i].len) == 0)
		{
			if (qse_strtonwad (uri + x[i].len, nwad) <= -1) return -1;
			*flags = x[i].flags;
			return 0;
		}
	}

	return -1;
}

static qse_ssize_t pio_handler_open (
	qse_awk_rtx_t* rtx, qse_awk_rio_arg_t* riod)
{
	qse_pio_t* handle;
	int flags;

	if (riod->mode == QSE_AWK_RIO_PIPE_READ)
	{
		/* TODO: should ERRTOOUT be unset? */
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
	else 
	{
		/* this must not happen */
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINTERN, QSE_NULL);
		return -1;
	}

	handle = qse_pio_open (
		rtx->awk->mmgr,
		0, 
		riod->name, 
		QSE_NULL,
		flags|QSE_PIO_SHELL|QSE_PIO_TEXT|QSE_PIO_IGNOREMBWCERR
	);
	if (handle == QSE_NULL) return -1;

#if defined(QSE_CHAR_IS_WCHAR)
	{
		qse_cmgr_t* cmgr = qse_awk_rtx_getcmgrstd (rtx, riod->name);
		if (cmgr)	
		{
			qse_pio_setcmgr (handle, QSE_PIO_IN, cmgr);
			qse_pio_setcmgr (handle, QSE_PIO_OUT, cmgr);
			qse_pio_setcmgr (handle, QSE_PIO_ERR, cmgr);
		}
	}
#endif

	riod->handle = (void*)handle;
	riod->uflags = 0; /* pio indicator */
	return 1;
}

static qse_ssize_t pio_handler_rest (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
	qse_char_t* data, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_AWK_RIO_OPEN:
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINTERN, QSE_NULL);
			return -1;
		}

		case QSE_AWK_RIO_CLOSE:
		{
			qse_pio_t* pio = (qse_pio_t*)riod->handle;
			if (riod->mode == QSE_AWK_RIO_PIPE_RW)
			{
				/* specialy treatment is needed for rwpipe.
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
			break;
	}

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINTERN, QSE_NULL);
	return -1;
}

static qse_ssize_t awk_rio_pipe (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
	qse_char_t* data, qse_size_t size)
{
	if (cmd == QSE_AWK_RIO_OPEN)
	{
		int flags;
		qse_nwad_t nwad;

		if (riod->mode != QSE_AWK_RIO_PIPE_RW ||
		    parse_rwpipe_uri (riod->name, &flags, &nwad) <= -1) 
		{
			return pio_handler_open (rtx, riod);
		}
		else
		{
			qse_nwio_tmout_t tmout_buf;
			qse_nwio_tmout_t* tmout = QSE_NULL;
			ioattr_t* ioattr;
			rxtn_t* rxtn;

			rxtn = (rxtn_t*) QSE_XTN (rtx);

			ioattr = get_ioattr (
				&rxtn->cmgrtab, riod->name, qse_strlen(riod->name));
			if (ioattr)
			{
				tmout = &tmout_buf;
				tmout->r = ioattr->tmout[0];
				tmout->w = ioattr->tmout[1];
				tmout->c = ioattr->tmout[2];
				tmout->a = ioattr->tmout[3];
			}

			return nwio_handler_open (rtx, riod, flags, &nwad, tmout);
		}
	}
	else if (riod->uflags > 0)
		return nwio_handler_rest (rtx, cmd, riod, data, size);
	else
		return pio_handler_rest (rtx, cmd, riod, data, size);
}

static qse_ssize_t awk_rio_file (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
	qse_char_t* data, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_AWK_RIO_OPEN:
		{
			qse_sio_t* handle;
			int flags = QSE_SIO_IGNOREMBWCERR;

			switch (riod->mode)
			{
				case QSE_AWK_RIO_FILE_READ: 
					flags |= QSE_SIO_READ;
					break;
				case QSE_AWK_RIO_FILE_WRITE:
					flags |= QSE_SIO_WRITE |
					         QSE_SIO_CREATE |
					         QSE_SIO_TRUNCATE;
					break;
				case QSE_AWK_RIO_FILE_APPEND:
					flags |= QSE_SIO_APPEND |
					         QSE_SIO_CREATE;
					break;
				default:
					/* this must not happen */
					qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINTERN, QSE_NULL);
					return -1; 
			}

			handle = qse_sio_open (rtx->awk->mmgr, 0, riod->name, flags);
			if (handle == QSE_NULL) 
			{
				qse_cstr_t errarg;
				errarg.ptr = riod->name;
				errarg.len = qse_strlen(riod->name);
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EOPEN, &errarg);
				return -1;
			}

#if defined(QSE_CHAR_IS_WCHAR)
			{
				qse_cmgr_t* cmgr = qse_awk_rtx_getcmgrstd (rtx, riod->name);
				if (cmgr) qse_sio_setcmgr (handle, cmgr);
			}
#endif

			riod->handle = (void*)handle;
			return 1;
		}

		case QSE_AWK_RIO_CLOSE:
		{
			qse_sio_close ((qse_sio_t*)riod->handle);
			riod->handle = QSE_NULL;
			return 0;
		}

		case QSE_AWK_RIO_READ:
		{
			return qse_sio_getstrn (
				(qse_sio_t*)riod->handle,
				data,	
				size
			);
		}

		case QSE_AWK_RIO_WRITE:
		{
			return qse_sio_putstrn (
				(qse_sio_t*)riod->handle,
				data,	
				size
			);
		}

		case QSE_AWK_RIO_FLUSH:
		{
			return qse_sio_flush ((qse_sio_t*)riod->handle);
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
		xtn_t* xtn = (xtn_t*)QSE_XTN (rtx->awk);

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

				if (rxtn->c.cmgr) qse_sio_setcmgr (sio, rxtn->c.cmgr);	

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
			qse_xstr_t as;

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

					if (rxtn->c.cmgr) qse_sio_setcmgr (sio, rxtn->c.cmgr);	

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
			argv = qse_awk_rtx_getgbl (rtx, xtn->gbl_argv);
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

			as.ptr = qse_awk_rtx_valtostrdup (rtx, v, &as.len);
			if (as.ptr == QSE_NULL) return -1;

			if (as.len == 0)
			{
				/* the name is empty */
				qse_awk_rtx_freemem (rtx, as.ptr);
				rxtn->c.in.index++;
				goto nextfile;
			}

			if (qse_strlen(as.ptr) < as.len)
			{
				/* the name contains one or more '\0' */
				qse_cstr_t errarg;

				errarg.ptr = as.ptr;
				/* use this length not to contains '\0'
				 * in an error message */
				errarg.len = qse_strlen(as.ptr);

				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EIONMNL, &errarg);

				qse_awk_rtx_freemem (rtx, as.ptr);
				return -1;
			}

			file = as.ptr;

			sio = (file[0] == QSE_T('-') && file[1] == QSE_T('\0'))?
				open_sio_std_rtx (rtx, QSE_SIO_STDIN, 
					QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR):
				open_sio_rtx (rtx, file,
					QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
			if (sio == QSE_NULL)
			{
				qse_awk_rtx_freemem (rtx, as.ptr);
				return -1;
			}

			if (rxtn->c.cmgr) qse_sio_setcmgr (sio, rxtn->c.cmgr);	

			if (qse_awk_rtx_setfilename (
				rtx, file, qse_strlen(file)) <= -1)
			{
				qse_sio_close (sio);
				qse_awk_rtx_freemem (rtx, as.ptr);
				return -1;
			}

			qse_awk_rtx_freemem (rtx, as.ptr);
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

				if (rxtn->c.cmgr) qse_sio_setcmgr (sio, rxtn->c.cmgr);	

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

			if (rxtn->c.cmgr) qse_sio_setcmgr (sio, rxtn->c.cmgr);	
			
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

static void fini_rxtn (qse_awk_rtx_t* rtx)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	/*xtn_t* xtn = (xtn_t*) QSE_XTN (rtx->awk);*/

	if (rxtn->cmgrtab_inited)
	{
		qse_htb_fini (&rxtn->cmgrtab);
		rxtn->cmgrtab_inited = 0;
	}
}

static int build_argcv (
	qse_awk_rtx_t* rtx, int argc_id, int argv_id, 
	const qse_char_t* id, const qse_char_t*const icf[])
{
	const qse_char_t*const* p;
	qse_size_t argc;
	qse_awk_val_t* v_argc;
	qse_awk_val_t* v_argv;
	qse_awk_val_t* v_tmp;
	qse_char_t key[QSE_SIZEOF(qse_long_t)*8+2];
	qse_size_t key_len;

	v_argv = qse_awk_rtx_makemapval (rtx);
	if (v_argv == QSE_NULL) return -1;

	qse_awk_rtx_refupval (rtx, v_argv);

	/* make ARGV[0] */
	v_tmp = qse_awk_rtx_makestrvalwithstr (rtx, id);
	if (v_tmp == QSE_NULL) 
	{
		qse_awk_rtx_refdownval (rtx, v_argv);
		return -1;
	}

	/* increment reference count of v_tmp in advance as if 
	 * it has successfully been assigned into ARGV. */
	qse_awk_rtx_refupval (rtx, v_tmp);

	key_len = qse_strxcpy (key, QSE_COUNTOF(key), QSE_T("0"));
	if (qse_htb_upsert (
		((qse_awk_val_map_t*)v_argv)->map,
		key, key_len, v_tmp, 0) == QSE_NULL)
	{
		/* if the assignment operation fails, decrements
		 * the reference of v_tmp to free it */
		qse_awk_rtx_refdownval (rtx, v_tmp);

		/* the values previously assigned into the
		 * map will be freeed when v_argv is freed */
		qse_awk_rtx_refdownval (rtx, v_argv);

		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL); 
		return -1;
	}

	if (icf)
	{
		for (argc = 1, p = icf; *p; p++, argc++) 
		{
			/* the argument must compose a numeric string if it can be.
			 * so call qse_awk_rtx_makenstrvalwithstr() instead of
			 * qse_awk_rtx_makestrvalwithstr(). */
			v_tmp = qse_awk_rtx_makenstrvalwithstr (rtx, *p);
			if (v_tmp == QSE_NULL)
			{
				qse_awk_rtx_refdownval (rtx, v_argv);
				return -1;
			}

			key_len = qse_awk_longtostr (
				rtx->awk, argc, 10,
				QSE_NULL, key, QSE_COUNTOF(key));
			QSE_ASSERT (key_len != (qse_size_t)-1);

			qse_awk_rtx_refupval (rtx, v_tmp);

			if (qse_htb_upsert (
				((qse_awk_val_map_t*)v_argv)->map,
				key, key_len, v_tmp, 0) == QSE_NULL)
			{
				qse_awk_rtx_refdownval (rtx, v_tmp);
				qse_awk_rtx_refdownval (rtx, v_argv);
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL); 
				return -1;
			}
		}
	}
	else argc = 1;

	v_argc = qse_awk_rtx_makeintval (rtx, (qse_long_t)argc);
	if (v_argc == QSE_NULL)
	{
		qse_awk_rtx_refdownval (rtx, v_argv);
		return -1;
	}

	qse_awk_rtx_refupval (rtx, v_argc);

	if (qse_awk_rtx_setgbl (rtx, argc_id, v_argc) <= -1)
	{
		qse_awk_rtx_refdownval (rtx, v_argc);
		qse_awk_rtx_refdownval (rtx, v_argv);
		return -1;
	}

	if (qse_awk_rtx_setgbl (rtx, argv_id, v_argv) <= -1)
	{
		qse_awk_rtx_refdownval (rtx, v_argc);
		qse_awk_rtx_refdownval (rtx, v_argv);
		return -1;
	}

	qse_awk_rtx_refdownval (rtx, v_argc);
	qse_awk_rtx_refdownval (rtx, v_argv);

	return 0;
}
 
static int __build_environ (
	qse_awk_rtx_t* rtx, int gbl_id, qse_env_char_t* envarr[])
{
	qse_awk_val_t* v_env;
	qse_awk_val_t* v_tmp;

	v_env = qse_awk_rtx_makemapval (rtx);
	if (v_env == QSE_NULL) return -1;

	qse_awk_rtx_refupval (rtx, v_env);

	if (envarr)
	{
		qse_env_char_t* eq;
		qse_char_t* kptr, * vptr;
		qse_size_t klen, count;

		for (count = 0; envarr[count]; count++)
		{
		#if ((defined(QSE_ENV_CHAR_IS_MCHAR) && defined(QSE_CHAR_IS_MCHAR)) || \
		     (defined(QSE_ENV_CHAR_IS_WCHAR) && defined(QSE_CHAR_IS_WCHAR)))
			eq = qse_strchr (envarr[count], QSE_T('='));
			if (eq == QSE_NULL || eq == envarr[count]) continue;

			kptr = envarr[count];
			klen = eq - envarr[count];
			vptr = eq + 1;
		#elif defined(QSE_ENV_CHAR_IS_MCHAR)
			eq = qse_mbschr (envarr[count], QSE_MT('='));
			if (eq == QSE_NULL || eq == envarr[count]) continue;

			*eq = QSE_MT('\0');

			kptr = qse_mbstowcsdup (envarr[count], &klen, rtx->awk->mmgr); 
			vptr = qse_mbstowcsdup (eq + 1, QSE_NULL, rtx->awk->mmgr);
			if (kptr == QSE_NULL || vptr == QSE_NULL)
			{
				if (kptr) QSE_MMGR_FREE (rtx->awk->mmgr, kptr);
				qse_awk_rtx_refdownval (rtx, v_env);

				/* mbstowcsdup() may fail for invalid encoding.
				 * so setting the error code to ENOMEM may not
				 * be really accurate */
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL); 
				return -1;
			}			

			*eq = QSE_MT('=');
		#else
			eq = qse_wcschr (envarr[count], QSE_WT('='));
			if (eq == QSE_NULL || eq == envarr[count]) continue;

			*eq = QSE_WT('\0');

			kptr = qse_wcstombsdup (envarr[count], &klen, rtx->awk->mmgr); 
			vptr = qse_wcstombsdup (eq + 1, QSE_NULL, rtx->awk->mmgr);
			if (kptr == QSE_NULL || vptr == QSE_NULL)
			{
				if (kptr) QSE_MMGR_FREE (rtx->awk->mmgr, kptr);
				qse_awk_rtx_refdownval (rtx, v_env);

				/* mbstowcsdup() may fail for invalid encoding.
				 * so setting the error code to ENOMEM may not
				 * be really accurate */
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}			

			*eq = QSE_WT('=');
		#endif

			/* the string in ENVIRON should be a numeric string if
			 * it can be converted to a number. call makenstrval()
			 * instead of makestrval() */
			v_tmp = qse_awk_rtx_makenstrvalwithstr (rtx, vptr);
			if (v_tmp == QSE_NULL)
			{
		#if ((defined(QSE_ENV_CHAR_IS_MCHAR) && defined(QSE_CHAR_IS_MCHAR)) || \
		     (defined(QSE_ENV_CHAR_IS_WCHAR) && defined(QSE_CHAR_IS_WCHAR)))
				/* nothing to do */
		#else
				if (vptr) QSE_MMGR_FREE (rtx->awk->mmgr, vptr);
				if (kptr) QSE_MMGR_FREE (rtx->awk->mmgr, kptr);
		#endif
				qse_awk_rtx_refdownval (rtx, v_env);
				return -1;
			}


			/* increment reference count of v_tmp in advance as if 
			 * it has successfully been assigned into ARGV. */
			qse_awk_rtx_refupval (rtx, v_tmp);

			if (qse_htb_upsert (
				((qse_awk_val_map_t*)v_env)->map,
				kptr, klen, v_tmp, 0) == QSE_NULL)
			{
				/* if the assignment operation fails, decrements
				 * the reference of v_tmp to free it */
				qse_awk_rtx_refdownval (rtx, v_tmp);

		#if ((defined(QSE_ENV_CHAR_IS_MCHAR) && defined(QSE_CHAR_IS_MCHAR)) || \
		     (defined(QSE_ENV_CHAR_IS_WCHAR) && defined(QSE_CHAR_IS_WCHAR)))
				/* nothing to do */
		#else
				if (vptr) QSE_MMGR_FREE (rtx->awk->mmgr, vptr);
				if (kptr) QSE_MMGR_FREE (rtx->awk->mmgr, kptr);
		#endif

				/* the values previously assigned into the
				 * map will be freeed when v_env is freed */
				qse_awk_rtx_refdownval (rtx, v_env);

				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}

		#if ((defined(QSE_ENV_CHAR_IS_MCHAR) && defined(QSE_CHAR_IS_MCHAR)) || \
		     (defined(QSE_ENV_CHAR_IS_WCHAR) && defined(QSE_CHAR_IS_WCHAR)))
				/* nothing to do */
		#else
			if (vptr) QSE_MMGR_FREE (rtx->awk->mmgr, vptr);
			if (kptr) QSE_MMGR_FREE (rtx->awk->mmgr, kptr);
		#endif
		}
	}

	if (qse_awk_rtx_setgbl (rtx, gbl_id, v_env) == -1)
	{
		qse_awk_rtx_refdownval (rtx, v_env);
		return -1;
	}

	qse_awk_rtx_refdownval (rtx, v_env);
	return 0;
}

static int build_environ (qse_awk_rtx_t* rtx, int gbl_id)
{
	qse_env_t env;
	int xret;

	if (qse_env_init (&env, rtx->awk->mmgr, 1) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}

	xret = __build_environ (rtx, gbl_id, qse_env_getarr(&env));

	qse_env_fini (&env);
	return xret;
}

static int make_additional_globals (
	qse_awk_rtx_t* rtx, xtn_t* xtn, 
	const qse_char_t* id, const qse_char_t*const icf[])
{
	if (build_argcv (rtx, xtn->gbl_argc, xtn->gbl_argv, id, icf) <= -1 ||
	    build_environ (rtx, xtn->gbl_environ) <= -1) return -1;

	return 0;
}

qse_awk_rtx_t* qse_awk_rtx_openstd (
	qse_awk_t*        awk,
	qse_size_t        xtnsize,
	const qse_char_t* id,
	const qse_char_t* icf[],
	const qse_char_t* ocf[],
	qse_cmgr_t*       cmgr)
{
	qse_awk_rtx_t* rtx;
	qse_awk_rio_t rio;
	rxtn_t* rxtn;
	xtn_t* xtn;
	qse_ntime_t now;

	xtn = (xtn_t*)QSE_XTN (awk);

	rio.pipe = awk_rio_pipe;
	rio.file = awk_rio_file;
	rio.console = awk_rio_console;

	rtx = qse_awk_rtx_open (
		awk, 
		QSE_SIZEOF(rxtn_t) + xtnsize,
		&rio
	);
	if (rtx == QSE_NULL) return QSE_NULL;

	rxtn = (rxtn_t*) QSE_XTN (rtx);

	if (rtx->awk->opt.trait & QSE_AWK_RIO)
	{
		if (qse_htb_init (
			&rxtn->cmgrtab, awk->mmgr, 256, 70, 
			QSE_SIZEOF(qse_char_t), 1) <= -1)
		{
			qse_awk_rtx_close (rtx);
			qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
			return QSE_NULL;
		}
		qse_htb_setstyle (&rxtn->cmgrtab, 
			qse_gethtbstyle(QSE_HTB_STYLE_INLINE_COPIERS));
		rxtn->cmgrtab_inited = 1;
	}

	rxtn->ecb.close = fini_rxtn;
	qse_awk_rtx_pushecb (rtx, &rxtn->ecb);

	rxtn->seed = (qse_gettime (&now) <= -1)? 0u: ((qse_long_t)now.sec + (qse_long_t)now.nsec);
	/* i don't care if the seed becomes negative or overflows.
	 * i just convert the signed value to the unsigned one. */
	rxtn->prand = (qse_ulong_t)(rxtn->seed * rxtn->seed * rxtn->seed);
	/* make sure that the actual seeding is not 0 */
	if (rxtn->prand == 0) rxtn->prand++;

	rxtn->c.in.files = icf;
	rxtn->c.in.index = 0;
	rxtn->c.in.count = 0;
	rxtn->c.out.files = ocf;
	rxtn->c.out.index = 0;
	rxtn->c.out.count = 0;
	rxtn->c.cmgr = cmgr;
	
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

	if (make_additional_globals (rtx, xtn, id, icf) <= -1)
	{
		awk->errinf = rtx->errinf; /* transfer error info */
		qse_awk_rtx_close (rtx);
		return QSE_NULL;
	}

	return rtx;
}

void* qse_awk_rtx_getxtnstd (qse_awk_rtx_t* rtx)
{
	return (void*)((rxtn_t*)QSE_XTN(rtx) + 1);
}

static int fnc_rand (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
#define RANDV_MAX QSE_TYPE_MAX(qse_long_t)
	qse_awk_val_t* r;
	qse_long_t randv;
	rxtn_t* rxtn;

	rxtn = (rxtn_t*) QSE_XTN (rtx);

	rxtn->prand = qse_randxsulong (rxtn->prand);
	randv = rxtn->prand % RANDV_MAX;

	r = qse_awk_rtx_makefltval (rtx, (qse_flt_t)randv / RANDV_MAX);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
#undef RANDV_MAX
}

static int fnc_srand (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_awk_val_t* r;
	int n;
	qse_long_t prev;
	qse_ntime_t now;
	rxtn_t* rxtn;

	rxtn = (rxtn_t*) QSE_XTN (rtx);
	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 0 || nargs == 1);

	prev = rxtn->seed;

	if (nargs <= 0)
	{
		rxtn->seed = (qse_gettime (&now) <= -1)? 
			(rxtn->seed * rxtn->seed): ((qse_long_t)now.sec + (qse_long_t)now.nsec);
	}
	else
	{
		a0 = qse_awk_rtx_getarg (rtx, 0);
		n = qse_awk_rtx_valtolong (rtx, a0, &lv);
		if (n <= -1) return -1;
		rxtn->seed = lv;
	}
	/* i don't care if the seed becomes negative or overflows.
	 * i just convert the signed value to the unsigned one. */
	rxtn->prand = (qse_ulong_t)(rxtn->seed * rxtn->seed * rxtn->seed);
	/* make sure that the actual seeding is not 0 */
	if (rxtn->prand == 0) rxtn->prand++;

	r = qse_awk_rtx_makeintval (rtx, prev);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_system (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* v;
	qse_char_t* str;
	qse_size_t len;
	int n = 0;

	v = qse_awk_rtx_getarg (rtx, 0);
	if (v->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)v)->val.ptr;
		len = ((qse_awk_val_str_t*)v)->val.len;
	}
	else
	{
		str = qse_awk_rtx_valtostrdup (rtx, v, &len);
		if (str == QSE_NULL) return -1;
	}

	/* the target name contains a null character.
	 * make system return -1 */
	if (qse_strxchr (str, len, QSE_T('\0')))
	{
		n = -1;
		goto skip_system;
	}

#if defined(_WIN32)
	n = _tsystem (str);
#elif defined(QSE_CHAR_IS_MCHAR)
	n = system (str);
#else

	{
		qse_mchar_t* mbs;
		mbs = qse_wcstombsdup (str, QSE_NULL, rtx->awk->mmgr);
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

static int timeout_code (const qse_char_t* name)
{
	if (qse_strcasecmp (name, QSE_T("rtimeout")) == 0) return 0;
	if (qse_strcasecmp (name, QSE_T("wtimeout")) == 0) return 1;
	if (qse_strcasecmp (name, QSE_T("ctimeout")) == 0) return 2;
	if (qse_strcasecmp (name, QSE_T("atimeout")) == 0) return 3;
	return -1;
}

static QSE_INLINE void init_ioattr (ioattr_t* ioattr)
{
	int i;
	QSE_MEMSET (ioattr, 0, QSE_SIZEOF(*ioattr));
	for (i = 0; i < QSE_COUNTOF(ioattr->tmout); i++) 
	{
		/* a negative number for no timeout */
		ioattr->tmout[i].sec = -999;
		ioattr->tmout[i].nsec = 0;
	}
}

static ioattr_t* get_ioattr (
	qse_htb_t* tab, const qse_char_t* ptr, qse_size_t len)
{
	qse_htb_pair_t* pair;

	pair = qse_htb_search (tab, ptr, len);
	if (pair) return QSE_HTB_VPTR(pair);

	return QSE_NULL;
}

static ioattr_t* find_or_make_ioattr (
	qse_awk_rtx_t* rtx, qse_htb_t* tab, const qse_char_t* ptr, qse_size_t len)
{
	qse_htb_pair_t* pair;

	pair = qse_htb_search (tab, ptr, len);
	if (pair == QSE_NULL)
	{
		ioattr_t ioattr;

		init_ioattr (&ioattr);

		pair = qse_htb_insert (tab, (void*)ptr, len, (void*)&ioattr, QSE_SIZEOF(ioattr));
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		}
	}

	return (ioattr_t*)QSE_HTB_VPTR(pair);
}

static int fnc_setioattr (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	rxtn_t* rxtn;
	qse_awk_val_t* v[3];
	qse_char_t* ptr[3];
	qse_size_t len[3];
	int i, ret = 0, fret = 0;
	int tmout;

	rxtn = (rxtn_t*) QSE_XTN (rtx);
	QSE_ASSERT (rxtn->cmgrtab_inited == 1);

	for (i = 0; i < 3; i++)
	{
		v[i] = qse_awk_rtx_getarg (rtx, i);
		if (v[i]->type == QSE_AWK_VAL_STR)
		{
			ptr[i] = ((qse_awk_val_str_t*)v[i])->val.ptr;
			len[i] = ((qse_awk_val_str_t*)v[i])->val.len;
		}
		else
		{
			ptr[i] = qse_awk_rtx_valtostrdup (rtx, v[i], &len[i]);
			if (ptr[i] == QSE_NULL) 
			{
				ret = -1;
				goto done;
			}
		}

		if (qse_strxchr (ptr[i], len[i], QSE_T('\0')))
		{
			fret = -1;
			goto done;
		}
	}

	if ((tmout = timeout_code (ptr[1])) >= 0)
	{
		ioattr_t* ioattr;

		qse_long_t l;
		qse_flt_t r;
		int x;

		/* no error is returned by qse_awk_rtx_strnum() if the second 
		 * parameter is 0. so i don't check for an error */
		x = qse_awk_rtx_strtonum (rtx, 0, ptr[2], len[2], &l, &r);
		if (x == 0) r = (qse_flt_t)l;

		ioattr = find_or_make_ioattr (rtx, &rxtn->cmgrtab, ptr[0], len[0]);
		if (ioattr == QSE_NULL) 
		{
			ret = -1;
			goto done;
		}

		if (x == 0)
		{
			ioattr->tmout[tmout].sec = l;
			ioattr->tmout[tmout].nsec = 0;
		}
		else if (x >= 1)
		{
			qse_flt_t nsec;
			ioattr->tmout[tmout].sec = (qse_long_t)r;
			nsec = r - ioattr->tmout[tmout].sec;
			ioattr->tmout[tmout].nsec = QSE_SEC_TO_NSEC(nsec);
		}
	}
#if defined(QSE_CHAR_IS_WCHAR)
	else if (qse_strcasecmp (ptr[1], QSE_T("codepage")) == 0)
	{
		ioattr_t* ioattr;
		qse_cmgr_t* cmgr;

		if (ptr[2][0] == QSE_T('\0')) 
		{
			cmgr = QSE_NULL;
		}
		else
		{
			cmgr = qse_findcmgr (ptr[2]);
			if (cmgr == QSE_NULL) 
			{
				fret = -1;
				goto done;
			}
		}

		ioattr = find_or_make_ioattr (rtx, &rxtn->cmgrtab, ptr[0], len[0]);
		if (ioattr == QSE_NULL) 
		{
			ret = -1;
			goto done;
		}

		ioattr->cmgr = cmgr;
		qse_strxcpy (ioattr->cmgr_name, QSE_COUNTOF(ioattr->cmgr_name), ptr[2]);
	}
#endif
	else
	{
		/* unknown attribute name */
		fret = -1;
		goto done;
	}

done:
	while (i > 0)
	{
		i--;
		if (v[i]->type != QSE_AWK_VAL_STR) 
			QSE_AWK_FREE (rtx->awk, ptr[i]);
	}

	if (ret >= 0)
	{
		v[0] = qse_awk_rtx_makeintval (rtx, (qse_long_t)fret);
		if (v[0] == QSE_NULL) return -1;
		qse_awk_rtx_setretval (rtx, v[0]);
	}

	return ret;
}

static int fnc_getioattr (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	rxtn_t* rxtn;
	qse_awk_val_t* v[2];
	qse_char_t* ptr[2];
	qse_size_t len[2];
	int i, ret = 0;
	int tmout;
	qse_awk_val_t* rv = QSE_NULL;

	ioattr_t* ioattr;
	ioattr_t ioattr_buf;

	rxtn = (rxtn_t*) QSE_XTN (rtx);
	QSE_ASSERT (rxtn->cmgrtab_inited == 1);

	for (i = 0; i < 2; i++)
	{
		v[i] = qse_awk_rtx_getarg (rtx, i);
		if (v[i]->type == QSE_AWK_VAL_STR)
		{
			ptr[i] = ((qse_awk_val_str_t*)v[i])->val.ptr;
			len[i] = ((qse_awk_val_str_t*)v[i])->val.len;
		}
		else
		{
			ptr[i] = qse_awk_rtx_valtostrdup (rtx, v[i], &len[i]);
			if (ptr[i] == QSE_NULL) 
			{
				ret = -1;
				goto done;
			}
		}

		if (qse_strxchr (ptr[i], len[i], QSE_T('\0'))) goto done;
	}

	ioattr = get_ioattr (&rxtn->cmgrtab, ptr[0], len[0]);
	if (ioattr == QSE_NULL) 
	{
		init_ioattr (&ioattr_buf);
		ioattr = &ioattr_buf;
	}

	if ((tmout = timeout_code (ptr[1])) >= 0)
	{
		if (ioattr->tmout[tmout].nsec == 0)
			rv = qse_awk_rtx_makeintval (rtx, ioattr->tmout[tmout].sec);
		else
			rv = qse_awk_rtx_makefltval (rtx, (qse_flt_t)ioattr->tmout[tmout].sec + QSE_NSEC_TO_SEC((qse_flt_t)ioattr->tmout[tmout].nsec));
		if (rv == QSE_NULL) 
		{
			ret = -1;
			goto done;
		}
	}
#if defined(QSE_CHAR_IS_WCHAR)
	else if (qse_strcasecmp (ptr[1], QSE_T("codepage")) == 0)
	{
		rv = qse_awk_rtx_makestrvalwithstr (rtx, ioattr->cmgr_name);
		if (rv == QSE_NULL)
		{
			ret = -1;
			goto done;
		}
	}
#endif
	else
	{
		/* unknown attribute name */
		goto done;
	}

done:
	while (i > 0)
	{
		i--;
		if (v[i]->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (rtx->awk, ptr[i]);
	}

	if (ret >= 0)
	{
		if (rv)
		{
			qse_awk_rtx_refupval (rtx, rv);
			ret = qse_awk_rtx_setrefval (rtx, qse_awk_rtx_getarg (rtx, 2), rv);
			qse_awk_rtx_refdownval (rtx, rv);
			if (ret >= 0) qse_awk_rtx_setretval (rtx, qse_awk_val_zero);
		}
		else 
		{
			qse_awk_rtx_setretval (rtx, qse_awk_val_negone);
		}

	}

	return ret;
}

qse_cmgr_t* qse_awk_rtx_getcmgrstd (qse_awk_rtx_t* rtx, const qse_char_t* ioname)
{
#if defined(QSE_CHAR_IS_WCHAR)
	rxtn_t* rxtn;
	ioattr_t* ioattr;

	rxtn = (rxtn_t*) QSE_XTN (rtx);
	QSE_ASSERT (rxtn->cmgrtab_inited == 1);

	ioattr = get_ioattr (&rxtn->cmgrtab, ioname, qse_strlen(ioname));
	if (ioattr) return ioattr->cmgr;
#endif
	return QSE_NULL;
}

static int add_globals (qse_awk_t* awk)
{
	xtn_t* xtn;

	xtn = (xtn_t*) QSE_XTN (awk);

	xtn->gbl_argc = qse_awk_addgbl (awk, QSE_T("ARGC"));
	xtn->gbl_argv = qse_awk_addgbl (awk, QSE_T("ARGV"));
	xtn->gbl_environ = qse_awk_addgbl (awk,  QSE_T("ENVIRON"));

	return (xtn->gbl_argc <= -1 || 
	        xtn->gbl_argv <= -1 ||
	        xtn->gbl_environ <= -1)? -1: 0;
}

struct fnctab_t 
{
	const qse_char_t* name;
	qse_awk_fnc_spec_t spec;
};

static struct fnctab_t fnctab[] =
{
	{ QSE_T("rand"),      { {0, 0, QSE_NULL},     fnc_rand,      0           } },
	{ QSE_T("srand"),     { {0, 1, QSE_NULL},     fnc_srand,     0           } },
	{ QSE_T("system"),    { {1, 1, QSE_NULL},     fnc_system ,   0           } },
	{ QSE_T("setioattr"), { {3, 3, QSE_NULL},     fnc_setioattr, QSE_AWK_RIO } },
	{ QSE_T("getioattr"), { {3, 3, QSE_T("vvr")}, fnc_getioattr, QSE_AWK_RIO } }
};

static int add_functions (qse_awk_t* awk)
{
	int i;

	for (i = 0; i < QSE_COUNTOF(fnctab); i++)
	{
		if (qse_awk_addfnc (awk, fnctab[i].name, &fnctab[i].spec) == QSE_NULL) return -1;
	}

	return 0;
}

