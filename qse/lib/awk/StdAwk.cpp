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

#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>
#include <qse/cmn/pio.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/nwio.h>
#include <qse/cmn/path.h>
#include "awk.h"
#include "std.h"

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
#	include <os2.h>
#elif defined(__DOS__)
#	if !defined(QSE_ENABLE_STATIC_MODULE)
#		include <cwdllfnc.h>
#	endif
#else
#    include <unistd.h>
#    include <ltdl.h>
#	define USE_LTDL
#endif

#ifndef QSE_HAVE_CONFIG_H
#    if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#		define HAVE_POW
#		define HAVE_FMOD
#	endif
#endif

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

StdAwk::ioattr_t StdAwk::default_ioattr;

static qse_sio_t* open_sio (Awk* awk, StdAwk::Run* run, const qse_char_t* file, int flags)
{
	qse_sio_t* sio;

	//sio = qse_sio_open ((run? ((Awk::awk_t*)*(Awk*)*run)->mmgr: awk->getMmgr()), 0, file, flags);
	sio = qse_sio_open ((run? ((Awk*)*run)->getMmgr(): awk->getMmgr()), 0, file, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t ea;
		ea.ptr = (StdAwk::char_t*)file;
		ea.len = qse_strlen (file);
		if (run) run->setError (QSE_AWK_EOPEN, &ea);
		else awk->setError (QSE_AWK_EOPEN, &ea);
	}
	return sio;
}

static qse_sio_t* open_sio_std (Awk* awk, StdAwk::Run* run, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;
	static const StdAwk::char_t* std_names[] =
	{
		QSE_T("stdin"),
		QSE_T("stdout"),
		QSE_T("stderr"),
	};

	//sio = qse_sio_openstd ((run? ((Awk::awk_t*)*(Awk*)*run)->mmgr: awk->getMmgr()), 0, std, flags);
	sio = qse_sio_openstd ((run? ((Awk*)*run)->getMmgr(): awk->getMmgr()), 0, std, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t ea;
		ea.ptr = (StdAwk::char_t*)std_names[std];
		ea.len = qse_strlen (std_names[std]);
		if (run) run->setError (QSE_AWK_EOPEN, &ea);
		else awk->setError (QSE_AWK_EOPEN, &ea);
	}
	return sio;
}

int StdAwk::open () 
{
	int n = Awk::open ();
	if (n == -1) return n;

	this->gbl_argc = addGlobal (QSE_T("ARGC"));
	this->gbl_argv = addGlobal (QSE_T("ARGV"));
	this->gbl_environ = addGlobal (QSE_T("ENVIRON"));
	if (this->gbl_argc <= -1 ||
	    this->gbl_argv <= -1 ||
	    this->gbl_environ <= -1)
	{
		goto oops;
	}

	if (addFunction (QSE_T("rand"),       1, 0, QSE_T("math"),   QSE_NULL,                            0) <= -1 ||
	    addFunction (QSE_T("srand"),      1, 0, QSE_T("math"),   QSE_NULL,                            0) <= -1 ||
	    addFunction (QSE_T("system"),     1, 0, QSE_T("sys"),    QSE_NULL,                            0) <= -1 ||
	    addFunction (QSE_T("setioattr"),  3, 3, QSE_NULL,        (FunctionHandler)&StdAwk::setioattr, QSE_AWK_RIO) <= -1 ||
	    addFunction (QSE_T("getioattr"),  3, 3, QSE_T("vvr"),    (FunctionHandler)&StdAwk::getioattr, QSE_AWK_RIO) <= -1)
	{
		goto oops;
	}

#if defined(USE_LTDL)
	/* lt_dlinit() can be called more than once and 
	 * lt_dlexit() shuts down libltdl if it's called as many times as
	 * corresponding lt_dlinit(). so it's safe to call lt_dlinit()
	 * and lt_dlexit() at the library level. */
	if (lt_dlinit() != 0) goto oops;
#endif

	this->cmgrtab_inited = false;
	return 0;

oops:
	Awk::close ();
	return -1;
}

void StdAwk::close () 
{
	if (this->cmgrtab_inited) 
	{
		qse_htb_fini (&this->cmgrtab);
		this->cmgrtab_inited = false;
	}

	clearConsoleOutputs ();
	Awk::close ();

#if defined(USE_LTDL)
	lt_dlexit ();
#endif
}

StdAwk::Run* StdAwk::parse (Source& in, Source& out)
{
	Run* run = Awk::parse (in, out);

	if (this->cmgrtab_inited) 
	{
		// if cmgrtab has already been initialized,
		// just clear the contents regardless of 
		// parse() result.
		qse_htb_clear (&this->cmgrtab);
	}
	else if (run && (this->getTrait() & QSE_AWK_RIO))
	{
		// it initialized cmgrtab only if QSE_AWK_RIO is set.
		// but if you call parse() multiple times while
		// setting and unsetting QSE_AWK_RIO in-between,
		// cmgrtab can still be initialized when QSE_AWK_RIO is not set.
		if (qse_htb_init (
			&this->cmgrtab, this->getMmgr(), 256, 70,
			QSE_SIZEOF(qse_char_t), 1) <= -1)
		{
			this->setError (QSE_AWK_ENOMEM);
			return QSE_NULL;
		}
		qse_htb_setstyle (&this->cmgrtab,
			qse_gethtbstyle(QSE_HTB_STYLE_INLINE_KEY_COPIER));
		this->cmgrtab_inited = true;
	}

	if (run && make_additional_globals (run) <= -1) return QSE_NULL;
	
	return run;
}

int StdAwk::build_argcv (Run* run)
{
	Value argv (run);

	for (size_t i = 0; i < this->runarg.len; i++)
	{
		if (argv.setIndexedStr (
			Value::IntIndex(i), 
			this->runarg.ptr[i].ptr, 
			this->runarg.ptr[i].len, true) <= -1) return -1;
	}
		
	run->setGlobal (this->gbl_argc, (int_t)this->runarg.len);
	run->setGlobal (this->gbl_argv, argv);
	return 0;
}

int StdAwk::__build_environ (Run* run, void* envptr)
{
	qse_env_char_t** envarr = (qse_env_char_t**)envptr;
	Value v_env (run);

	if (envarr)
	{
		qse_env_char_t* eq;
		qse_char_t* kptr, * vptr;
		qse_size_t klen, count;
		qse_mmgr_t* mmgr = ((Awk*)*run)->getMmgr();

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

			kptr = qse_mbstowcsdup (envarr[count], &klen, mmgr);
			vptr = qse_mbstowcsdup (eq + 1, QSE_NULL, mmgr);
			if (kptr == QSE_NULL || vptr == QSE_NULL)
			{
				if (kptr) QSE_MMGR_FREE (mmgr, kptr);
				if (vptr) QSE_MMGR_FREE (mmgr, vptr);

				/* mbstowcsdup() may fail for invalid encoding.
				 * so setting the error code to ENOMEM may not
				 * be really accurate */
				setError (QSE_AWK_ENOMEM);
				return -1;
			}			

			*eq = QSE_MT('=');
		#else
			eq = qse_wcschr (envarr[count], QSE_WT('='));
			if (eq == QSE_NULL || eq == envarr[count]) continue;

			*eq = QSE_WT('\0');

			kptr = qse_wcstombsdup (envarr[count], &klen, mmgr); 
			vptr = qse_wcstombsdup (eq + 1, QSE_NULL, mmgr);
			if (kptr == QSE_NULL || vptr == QSE_NULL)
			{
				if (kptr) QSE_MMGR_FREE (mmgr, kptr);
				if (vptr) QSE_MMGR_FREE (mmgr, vptr);

				/* mbstowcsdup() may fail for invalid encoding.
				 * so setting the error code to ENOMEM may not
				 * be really accurate */
				setError (QSE_AWK_ENOMEM);
				return -1;
			}			

			*eq = QSE_WT('=');
		#endif

			// numeric string
			v_env.setIndexedStr (Value::Index (kptr, klen), vptr, true);

		#if ((defined(QSE_ENV_CHAR_IS_MCHAR) && defined(QSE_CHAR_IS_MCHAR)) || \
		     (defined(QSE_ENV_CHAR_IS_WCHAR) && defined(QSE_CHAR_IS_WCHAR)))
				/* nothing to do */
		#else
			if (vptr) QSE_MMGR_FREE (mmgr, vptr);
			if (kptr) QSE_MMGR_FREE (mmgr, kptr);
		#endif
		}
	}

	return run->setGlobal (this->gbl_environ, v_env);
}

int StdAwk::build_environ (Run* run)
{
	qse_env_t env;
	int xret;

	if (qse_env_init (&env, ((Awk*)*run)->getMmgr(), 1) <= -1)
	{
		setError (QSE_AWK_ENOMEM);
		return -1;
	}

	xret = __build_environ (run, qse_env_getarr(&env));

	qse_env_fini (&env);
	return xret;
}

int StdAwk::make_additional_globals (Run* run)
{
	if (build_argcv (run) <= -1 ||
	    build_environ (run) <= -1) return -1;
	    
	return 0;
}

qse_cmgr_t* StdAwk::getcmgr (const char_t* ioname)
{
	QSE_ASSERT (this->cmgrtab_inited == true);

#if defined(QSE_CHAR_IS_WCHAR)
	ioattr_t* ioattr = get_ioattr (ioname, qse_strlen(ioname));
	if (ioattr) return ioattr->cmgr;
#endif
	return QSE_NULL;
}

StdAwk::ioattr_t* StdAwk::get_ioattr (const char_t* ptr, size_t len)
{
	qse_htb_pair_t* pair;

	pair = qse_htb_search (&this->cmgrtab, ptr, len);
	if (pair == QSE_NULL) return QSE_NULL;

	return (ioattr_t*)QSE_HTB_VPTR(pair);
}

StdAwk::ioattr_t* StdAwk::find_or_make_ioattr (const char_t* ptr, size_t len)
{
	qse_htb_pair_t* pair;

	pair = qse_htb_search (&this->cmgrtab, ptr, len);
	if (pair == QSE_NULL)
	{
		pair = qse_htb_insert (
			&this->cmgrtab, (void*)ptr, len, 
			(void*)&StdAwk::default_ioattr, 
			QSE_SIZEOF(StdAwk::default_ioattr));
		if (pair == QSE_NULL) 
		{
			setError (QSE_AWK_ENOMEM);
			return QSE_NULL;
		}
	}

	return (ioattr_t*)QSE_HTB_VPTR(pair);
}

static int timeout_code (const qse_char_t* name)
{
	if (qse_strcasecmp (name, QSE_T("rtimeout")) == 0) return 0;
	if (qse_strcasecmp (name, QSE_T("wtimeout")) == 0) return 1;
	if (qse_strcasecmp (name, QSE_T("ctimeout")) == 0) return 2;
	if (qse_strcasecmp (name, QSE_T("atimeout")) == 0) return 3;
	return -1;
}

int StdAwk::setioattr (
	Run& run, Value& ret, Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	QSE_ASSERT (this->cmgrtab_inited == true);
	size_t l[3];
	const char_t* ptr[3];

	ptr[0] = args[0].toStr(&l[0]);
	ptr[1] = args[1].toStr(&l[1]);
	ptr[2] = args[2].toStr(&l[2]);

	if (qse_strxchr (ptr[0], l[0], QSE_T('\0')) ||
	    qse_strxchr (ptr[1], l[1], QSE_T('\0')) ||
	    qse_strxchr (ptr[2], l[2], QSE_T('\0')))
	{
		return ret.setInt ((int_t)-1);
	}
	
	int tmout;
	if ((tmout = timeout_code (ptr[1])) >= 0)
	{
		int_t lv;
		flt_t fv;
		int n;

		n = args[2].getNum(&lv, &fv);
		if (n <= -1) return -1;

		ioattr_t* ioattr = find_or_make_ioattr (ptr[0], l[0]);
		if (ioattr == QSE_NULL) return -1;

		if (n == 0)
		{
			ioattr->tmout[tmout].sec = lv;
			ioattr->tmout[tmout].nsec = 0;
		}
		else
		{
			qse_awk_flt_t nsec;
			ioattr->tmout[tmout].sec = (qse_int_t)fv;
			nsec = fv - ioattr->tmout[tmout].sec;
			ioattr->tmout[tmout].nsec = QSE_SEC_TO_NSEC(nsec);
		}
		return ret.setInt ((int_t)0);
	}
#if defined(QSE_CHAR_IS_WCHAR)
	else if (qse_strcasecmp (ptr[1], QSE_T("codepage")) == 0 ||
	         qse_strcasecmp (ptr[1], QSE_T("encoding")) == 0)
	{
		ioattr_t* ioattr;
		qse_cmgr_t* cmgr;

		if (ptr[2][0] == QSE_T('\0')) cmgr = QSE_NULL;
		else
		{
			cmgr = qse_findcmgr (ptr[2]);
			if (cmgr == QSE_NULL) return ret.setInt ((int_t)-1);
		}
		
		ioattr = find_or_make_ioattr (ptr[0], l[0]);
		if (ioattr == QSE_NULL) return -1;

		ioattr->cmgr = cmgr;
		qse_strxcpy (ioattr->cmgr_name, QSE_COUNTOF(ioattr->cmgr_name), ptr[2]);
		return 0;
	}
#endif
	else
	{
		// unknown attribute name
		return ret.setInt ((int_t)-1);
	}
}

int StdAwk::getioattr (
	Run& run, Value& ret, Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	QSE_ASSERT (this->cmgrtab_inited == true);
	size_t l[2];
	const char_t* ptr[2];

	ptr[0] = args[0].toStr(&l[0]);
	ptr[1] = args[1].toStr(&l[1]);

	int xx = -1;

	/* ionames must not contains a null character */
	if (qse_strxchr (ptr[0], l[0], QSE_T('\0')) == QSE_NULL &&
	    qse_strxchr (ptr[1], l[1], QSE_T('\0')) == QSE_NULL)
	{
		ioattr_t* ioattr = get_ioattr (ptr[0], l[0]);
		if (ioattr == QSE_NULL) ioattr = &StdAwk::default_ioattr;

		int tmout;
		if ((tmout = timeout_code(ptr[1])) >= 0)
		{
			if (ioattr->tmout[tmout].nsec == 0)
				xx = args[2].setInt ((int_t)ioattr->tmout[tmout].sec);
			else
				xx = args[2].setFlt ((qse_awk_flt_t)ioattr->tmout[tmout].sec + QSE_NSEC_TO_SEC((qse_awk_flt_t)ioattr->tmout[tmout].nsec));
		}
	#if defined(QSE_CHAR_IS_WCHAR)
		else if (qse_strcasecmp (ptr[1], QSE_T("codepage")) == 0 ||
		         qse_strcasecmp (ptr[1], QSE_T("encoding")) == 0)
		{
			xx = args[2].setStr (ioattr->cmgr_name);
		}
	#endif
	}

	// unknown attribute name or errors
	return ret.setInt ((int_t)xx);
}

int StdAwk::open_nwio (Pipe& io, int flags, void* nwad)
{
	qse_nwio_tmout_t tmout_buf;
	qse_nwio_tmout_t* tmout = QSE_NULL;

	const qse_char_t* name = io.getName();
	ioattr_t* ioattr = get_ioattr (name, qse_strlen(name));
	if (ioattr)
	{
		tmout = &tmout_buf;
		tmout->r = ioattr->tmout[0];
		tmout->w = ioattr->tmout[1];
		tmout->c = ioattr->tmout[2];
		tmout->a = ioattr->tmout[3];
	}

	qse_nwio_t* handle = qse_nwio_open (
		this->getMmgr(), 0, (qse_nwad_t*)nwad,
		flags | QSE_NWIO_TEXT | QSE_NWIO_IGNOREMBWCERR |
		QSE_NWIO_REUSEADDR | QSE_NWIO_READNORETRY | QSE_NWIO_WRITENORETRY,
		tmout
	);
	if (handle == QSE_NULL) return -1;

#if defined(QSE_CHAR_IS_WCHAR)
	qse_cmgr_t* cmgr = this->getcmgr (io.getName());
	if (cmgr) qse_nwio_setcmgr (handle, cmgr);
#endif

	io.setHandle ((void*)handle);
	io.setUflags (1);

	return 1;
}

int StdAwk::open_pio (Pipe& io) 
{ 
	Awk::Pipe::Mode mode = io.getMode();
	qse_pio_t* pio = QSE_NULL;
	int flags = QSE_PIO_TEXT | QSE_PIO_SHELL | QSE_PIO_IGNOREMBWCERR;

	switch (mode)
	{
		case Awk::Pipe::READ:
			/* TODO: should we specify ERRTOOUT? */
			flags |= QSE_PIO_READOUT |
			         QSE_PIO_ERRTOOUT;
			break;

		case Awk::Pipe::WRITE:
			flags |= QSE_PIO_WRITEIN;
			break;

		case Awk::Pipe::RW:
			flags |= QSE_PIO_READOUT |
			         QSE_PIO_ERRTOOUT |
			         QSE_PIO_WRITEIN;
			break;
	}

	pio = qse_pio_open (
		this->getMmgr(),
		0, 
		io.getName(), 
		QSE_NULL,
		flags
	);
	if (pio == QSE_NULL) return -1;

#if defined(QSE_CHAR_IS_WCHAR)
	qse_cmgr_t* cmgr = this->getcmgr (io.getName());
	if (cmgr) 
	{
		qse_pio_setcmgr (pio, QSE_PIO_IN, cmgr);
		qse_pio_setcmgr (pio, QSE_PIO_OUT, cmgr);
		qse_pio_setcmgr (pio, QSE_PIO_ERR, cmgr);
	}
#endif
	io.setHandle (pio);
	io.setUflags (0);
	return 1;
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
	StdAwk::size_t i;

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

int StdAwk::openPipe (Pipe& io) 
{ 
	int flags;
	qse_nwad_t nwad;

	if (io.getMode() != Awk::Pipe::RW ||
	    parse_rwpipe_uri (io.getName(), &flags, &nwad) <= -1)
	{
		return open_pio (io);
	}
	else
	{
		return open_nwio (io, flags, &nwad);
	}
}

int StdAwk::closePipe (Pipe& io) 
{
	if (io.getUflags() > 0)
	{
		/* nwio can't honor partical close */
		qse_nwio_close ((qse_nwio_t*)io.getHandle());
	}
	else
	{
		qse_pio_t* pio = (qse_pio_t*)io.getHandle();
		if (io.getMode() == Awk::Pipe::RW)
		{
			Pipe::CloseMode rwcopt = io.getCloseMode();
			if (rwcopt == Awk::Pipe::CLOSE_READ)
			{
				qse_pio_end (pio, QSE_PIO_IN);
				return 0;
			}
			else if (rwcopt == Awk::Pipe::CLOSE_WRITE)
			{
				qse_pio_end (pio, QSE_PIO_OUT);
				return 0;
			}
		}

		qse_pio_close (pio);
	}
	return 0; 
}

StdAwk::ssize_t StdAwk::readPipe (Pipe& io, char_t* buf, size_t len) 
{ 
	return (io.getUflags() > 0)?
		qse_nwio_read ((qse_nwio_t*)io.getHandle(), buf, len):
		qse_pio_read ((qse_pio_t*)io.getHandle(), QSE_PIO_OUT, buf, len);
}

StdAwk::ssize_t StdAwk::writePipe (Pipe& io, const char_t* buf, size_t len) 
{ 
	return (io.getUflags() > 0)?
		qse_nwio_write ((qse_nwio_t*)io.getHandle(), buf, len):
		qse_pio_write ((qse_pio_t*)io.getHandle(), QSE_PIO_IN, buf, len);
}

int StdAwk::flushPipe (Pipe& io) 
{ 
	return (io.getUflags() > 0)?
		qse_nwio_flush ((qse_nwio_t*)io.getHandle()):
		qse_pio_flush ((qse_pio_t*)io.getHandle(), QSE_PIO_IN);
}

int StdAwk::openFile (File& io) 
{ 
	Awk::File::Mode mode = io.getMode();
	qse_sio_t* sio = QSE_NULL;
	int flags = QSE_SIO_IGNOREMBWCERR;

	switch (mode)
	{
		case Awk::File::READ:
			flags |= QSE_SIO_READ;
			break;
		case Awk::File::WRITE:
			flags |= QSE_SIO_WRITE | 
			         QSE_SIO_CREATE | 
			         QSE_SIO_TRUNCATE;
			break;
		case Awk::File::APPEND:
			flags |= QSE_SIO_APPEND |
			         QSE_SIO_CREATE;
			break;
	}

	sio = qse_sio_open (this->getMmgr(), 0, io.getName(), flags);	
	if (sio == NULL) return -1;
#if defined(QSE_CHAR_IS_WCHAR)
	qse_cmgr_t* cmgr = this->getcmgr (io.getName());
	if (cmgr) qse_sio_setcmgr (sio, cmgr);
#endif

	io.setHandle (sio);
	return 1;
}

int StdAwk::closeFile (File& io) 
{ 
	qse_sio_close ((qse_sio_t*)io.getHandle());
	return 0; 
}

StdAwk::ssize_t StdAwk::readFile (File& io, char_t* buf, size_t len) 
{
	return qse_sio_getstrn ((qse_sio_t*)io.getHandle(), buf, len);
}

StdAwk::ssize_t StdAwk::writeFile (File& io, const char_t* buf, size_t len)
{
	return qse_sio_putstrn ((qse_sio_t*)io.getHandle(), buf, len);
}

int StdAwk::flushFile (File& io) 
{ 
	return qse_sio_flush ((qse_sio_t*)io.getHandle());
}

void StdAwk::setConsoleCmgr (const qse_cmgr_t* cmgr)
{
	this->console_cmgr = (qse_cmgr_t*)cmgr;	
}

const qse_cmgr_t* StdAwk::getConsoleCmgr () const
{
	return this->console_cmgr;
}

int StdAwk::addConsoleOutput (const char_t* arg, size_t len) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = this->ofile.add (awk, arg, len);
	if (n <= -1) setError (QSE_AWK_ENOMEM);
	return n;
}

int StdAwk::addConsoleOutput (const char_t* arg) 
{
	return addConsoleOutput (arg, qse_strlen(arg));
}

void StdAwk::clearConsoleOutputs () 
{
	this->ofile.clear (awk);
}

int StdAwk::open_console_in (Console& io) 
{ 
	qse_awk_rtx_t* rtx = (rtx_t*)io;

	if (this->runarg.ptr == QSE_NULL) 
	{
		QSE_ASSERT (this->runarg.len == 0 && this->runarg.capa == 0);

		if (this->runarg_count == 0) 
		{
			qse_sio_t* sio;

			sio = open_sio_std (
				QSE_NULL, io, QSE_SIO_STDIN,
				QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
			if (sio == QSE_NULL) return -1;

			if (this->console_cmgr) 
				qse_sio_setcmgr (sio, this->console_cmgr);

			io.setHandle (sio);
			this->runarg_count++;
			return 1;
		}

		return 0;
	}
	else
	{
		qse_sio_t* sio;
		const qse_char_t* file;
		qse_awk_val_t* argv;
		qse_htb_t* map;
		qse_htb_pair_t* pair;
		qse_char_t ibuf[128];
		qse_size_t ibuflen;
		qse_awk_val_t* v;
		qse_cstr_t as;

	nextfile:
		file = this->runarg.ptr[this->runarg_index].ptr;

		if (file == QSE_NULL)
		{
			/* no more input file */

			if (this->runarg_count == 0)
			{
				/* all ARGVs are empty strings. 
				 * so no console files were opened.
				 * open the standard input here.
				 *
				 * 'BEGIN { ARGV[1]=""; ARGV[2]=""; }
				 *        { print $0; }' file1 file2
				 */
				sio = open_sio_std (
					QSE_NULL, io, QSE_SIO_STDIN,
					QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
				if (sio == QSE_NULL) return -1;

				if (this->console_cmgr)
					qse_sio_setcmgr (sio, this->console_cmgr);

				io.setHandle (sio);
				this->runarg_count++;
				return 1;
			}

			return 0;
		}

		if (qse_strlen(file) != this->runarg.ptr[this->runarg_index].len)
		{
			cstr_t arg;
			arg.ptr = (char_t*)file;
			arg.len = qse_strlen (arg.ptr);
			((Run*)io)->setError (QSE_AWK_EIONMNL, &arg);
			return -1;
		}

		/* handle special case when ARGV[x] has been altered.
		 * so from here down, the file name gotten from 
		 * rxtn->c.in.files is not important and is overridden 
		 * from ARGV.
		 * 'BEGIN { ARGV[1]="file3"; } 
		 *        { print $0; }' file1 file2
		 */
		argv = qse_awk_rtx_getgbl (rtx, this->gbl_argv);
		QSE_ASSERT (argv != QSE_NULL);
		QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE (rtx, argv) == QSE_AWK_VAL_MAP);

		map = ((qse_awk_val_map_t*)argv)->map;
		QSE_ASSERT (map != QSE_NULL);
		
		// ok to find ARGV[this->runarg_index] as ARGV[0]
		// has been skipped.
		ibuflen = qse_awk_inttostr (
			rtx->awk, this->runarg_index, 
			10, QSE_NULL,
			ibuf, QSE_COUNTOF(ibuf)
		);

		pair = qse_htb_search (map, ibuf, ibuflen);
		QSE_ASSERT (pair != QSE_NULL);

		v = (qse_awk_val_t*)QSE_HTB_VPTR(pair);
		QSE_ASSERT (v != QSE_NULL);

		as.ptr = qse_awk_rtx_getvalstr (rtx, v, &as.len);
		if (as.ptr == QSE_NULL) return -1;

		if (as.len == 0)
		{
			/* the name is empty */
			qse_awk_rtx_freevalstr (rtx, v, as.ptr);
			this->runarg_index++;
			goto nextfile;
		}

		if (qse_strlen(as.ptr) < as.len)
		{
			/* the name contains one or more '\0' */
			cstr_t arg;
			arg.ptr = as.ptr;
			arg.len = qse_strlen (as.ptr);
			((Run*)io)->setError (QSE_AWK_EIONMNL, &arg);
			qse_awk_rtx_freevalstr (rtx, v, as.ptr);
			return -1;
		}

		file = as.ptr;

		if (file[0] == QSE_T('-') && file[1] == QSE_T('\0'))
			sio = open_sio_std (QSE_NULL, io, QSE_SIO_STDIN, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
		else
			sio = open_sio (QSE_NULL, io, file, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
		if (sio == QSE_NULL) 
		{
			qse_awk_rtx_freevalstr (rtx, v, as.ptr);
			return -1;
		}
		
		if (qse_awk_rtx_setfilename (rtx, file, qse_strlen(file)) <= -1)
		{
			qse_sio_close (sio);
			qse_awk_rtx_freevalstr (rtx, v, as.ptr);
			return -1;
		}

		qse_awk_rtx_freevalstr (rtx, v, as.ptr);

		if (this->console_cmgr) qse_sio_setcmgr (sio, this->console_cmgr);

		io.setHandle (sio);

		/* increment the counter of files successfully opened */
		this->runarg_count++;
		this->runarg_index++;
		return 1;
	}

}

int StdAwk::open_console_out (Console& io) 
{
	qse_awk_rtx_t* rtx = (rtx_t*)io;

	if (this->ofile.ptr == QSE_NULL)
	{
		QSE_ASSERT (this->ofile.len == 0 && this->ofile.capa == 0);

		if (this->ofile_count == 0) 
		{
			qse_sio_t* sio;
			sio = open_sio_std (
				QSE_NULL, io, QSE_SIO_STDOUT,
				QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR | QSE_SIO_LINEBREAK);
			if (sio == QSE_NULL) return -1;

			if (this->console_cmgr)
				qse_sio_setcmgr (sio, this->console_cmgr);

			io.setHandle (sio);
			this->ofile_count++;
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

		file = this->ofile.ptr[this->ofile_index].ptr;

		if (file == QSE_NULL)
		{
			/* no more input file */
			return 0;
		}

		if (qse_strlen(file) != this->ofile.ptr[this->ofile_index].len)
		{	
			cstr_t arg;
			arg.ptr = (char_t*)file;
			arg.len = qse_strlen (arg.ptr);
			((Run*)io)->setError (QSE_AWK_EIONMNL, &arg);
			return -1;
		}

		if (file[0] == QSE_T('-') && file[1] == QSE_T('\0'))
			sio = open_sio_std (QSE_NULL, io, QSE_SIO_STDOUT, QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR | QSE_SIO_LINEBREAK);
		else
			sio = open_sio (QSE_NULL, io, file, QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR);
		if (sio == QSE_NULL) return -1;
		
		if (qse_awk_rtx_setofilename (
			rtx, file, qse_strlen(file)) == -1)
		{
			qse_sio_close (sio);
			return -1;
		}

		if (this->console_cmgr) 
			qse_sio_setcmgr (sio, this->console_cmgr);
		io.setHandle (sio);

		this->ofile_index++;
		this->ofile_count++;
		return 1;
	}
}

int StdAwk::openConsole (Console& io) 
{
	Console::Mode mode = io.getMode();

	if (mode == Console::READ)
	{
		this->runarg_count = 0;
		this->runarg_index = 0;
		if (this->runarg.len > 0) 
		{
			// skip ARGV[0]
			this->runarg_index++;
		}
		return open_console_in (io);
	}
	else
	{
		QSE_ASSERT (mode == Console::WRITE);

		this->ofile_count = 0;
		this->ofile_index = 0;
		return open_console_out (io);
	}
}

int StdAwk::closeConsole (Console& io) 
{ 
	qse_sio_close ((qse_sio_t*)io.getHandle());
	return 0;
}

StdAwk::ssize_t StdAwk::readConsole (Console& io, char_t* data, size_t size) 
{
	qse_ssize_t nn;

	while ((nn = qse_sio_getstrn((qse_sio_t*)io.getHandle(),data,size)) == 0)
	{
		int n;
		qse_sio_t* sio = (qse_sio_t*)io.getHandle();

		n = open_console_in (io);
		if (n == -1) return -1;

		if (n == 0) 
		{
			/* no more input console */
			return 0;
		}

		if (sio) qse_sio_close (sio);
	}

	return nn;
}

StdAwk::ssize_t StdAwk::writeConsole (Console& io, const char_t* data, size_t size) 
{
	return qse_sio_putstrn (
		(qse_sio_t*)io.getHandle(),
		data,
		size
	);
}

int StdAwk::flushConsole (Console& io) 
{ 
	return qse_sio_flush ((qse_sio_t*)io.getHandle());
}

int StdAwk::nextConsole (Console& io) 
{ 
	int n;
	qse_sio_t* sio = (qse_sio_t*)io.getHandle();

	n = (io.getMode() == Console::READ)? 
		open_console_in(io): open_console_out(io);
	if (n == -1) return -1;

	if (n == 0) 
	{
		/* if there is no more file, keep the previous handle */
		return 0;
	}

	if (sio) qse_sio_close (sio);
	return n;
}

// memory allocation primitives
void* StdAwk::allocMem (size_t n) 
{ 
	return ::malloc (n); 
}

void* StdAwk::reallocMem (void* ptr, size_t n) 
{ 
	return ::realloc (ptr, n); 
}

void  StdAwk::freeMem (void* ptr) 
{ 
	::free (ptr); 
}

// miscellaneous primitive

StdAwk::flt_t StdAwk::pow (flt_t x, flt_t y) 
{ 
	return qse_awk_stdmathpow (this->awk, x, y);
}

StdAwk::flt_t StdAwk::mod (flt_t x, flt_t y) 
{ 
	return qse_awk_stdmathmod (this->awk, x, y);
}

void* StdAwk::modopen (const mod_spec_t* spec)
{
	void* h;
	h = qse_awk_stdmodopen (this->awk, spec);
	if (!h) this->retrieveError ();
	return h;
}

void StdAwk::modclose (void* handle)
{
	qse_awk_stdmodclose (this->awk, handle);
}

void* StdAwk::modsym (void* handle, const qse_char_t* name)
{
	void* s;
	s = qse_awk_stdmodsym (this->awk, handle, name);
	if (!s) this->retrieveError ();
	return s;
}

int StdAwk::SourceFile::open (Data& io)
{
	qse_sio_t* sio;

	if (io.isMaster())
	{
		// open the main source file.

		if (this->name[0] == QSE_T('-') && this->name[1] == QSE_T('\0'))
		{
			if (io.getMode() == READ)
				sio = open_sio_std (
					io, QSE_NULL, QSE_SIO_STDIN, 
					QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
			else
				sio = open_sio_std (
					io, QSE_NULL, QSE_SIO_STDOUT, 
					QSE_SIO_WRITE | QSE_SIO_CREATE | 
					QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR | QSE_SIO_LINEBREAK);
			if (sio == QSE_NULL) return -1;
		}
		else
		{
			sio = open_sio (
				io, QSE_NULL, this->name,
				(io.getMode() == READ? 
					(QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_KEEPPATH): 
					(QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR))
			);
			if (sio == QSE_NULL) return -1;
		}

		if (this->cmgr) qse_sio_setcmgr (sio, this->cmgr);
		io.setName (this->name);
	}
	else
	{
		// open an included file
		const char_t* ioname, * file;
		char_t fbuf[64];
		char_t* dbuf = QSE_NULL;
	
		ioname = io.getName();
		QSE_ASSERT (ioname != QSE_NULL);

		file = ioname;
		if (io.getPrevHandle())
		{
			const char_t* outer;

			outer = qse_sio_getpath ((qse_sio_t*)io.getPrevHandle());
			if (outer)
			{
				const qse_char_t* base;

				base = qse_basename (outer);
				if (base != outer && ioname[0] != QSE_T('/'))
				{
					size_t tmplen, totlen, dirlen;
				
					dirlen = base - outer;
						totlen = qse_strlen(ioname) + dirlen;
					if (totlen >= QSE_COUNTOF(fbuf))
					{
						dbuf = (qse_char_t*) QSE_MMGR_ALLOC (
							((Awk*)io)->getMmgr(),
							QSE_SIZEOF(qse_char_t) * (totlen + 1)
						);
						if (dbuf == QSE_NULL)
						{
							((Awk*)io)->setError (QSE_AWK_ENOMEM);
							return -1;
						}

						file = dbuf;
					}
					else file = fbuf;

					tmplen = qse_strncpy ((char_t*)file, outer, dirlen);
					qse_strcpy ((char_t*)file + tmplen, ioname);
				}
			}
		}

		sio = open_sio (
			io, QSE_NULL, file,
			(io.getMode() == READ? 
				(QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_KEEPPATH): 
				(QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR))
		);
		if (dbuf) QSE_MMGR_FREE (((Awk*)io)->getMmgr(), dbuf);
		if (sio == QSE_NULL) return -1;
		if (this->cmgr) qse_sio_setcmgr (sio, this->cmgr);
	}

	io.setHandle (sio);
	return 1;
}

int StdAwk::SourceFile::close (Data& io)
{
	qse_sio_t* sio = (qse_sio_t*)io.getHandle();
	qse_sio_flush (sio);
	qse_sio_close (sio);
	return 0;
}

StdAwk::ssize_t StdAwk::SourceFile::read (Data& io, char_t* buf, size_t len)
{
	return qse_sio_getstrn ((qse_sio_t*)io.getHandle(), buf, len);
}

StdAwk::ssize_t StdAwk::SourceFile::write (Data& io, const char_t* buf, size_t len)
{
	return qse_sio_putstrn ((qse_sio_t*)io.getHandle(), buf, len);
}

int StdAwk::SourceString::open (Data& io)
{
	qse_sio_t* sio;

	if (io.getName() == QSE_NULL)
	{
		// open the main source file.
		// SourceString does not support writing.
		if (io.getMode() == WRITE) return -1;
		ptr = str;
	}
	else
	{
		// open an included file 

		const char_t* ioname, * file;
		char_t fbuf[64];
		char_t* dbuf = QSE_NULL;
		
		ioname = io.getName();
		QSE_ASSERT (ioname != QSE_NULL);

		file = ioname;
		if (io.getPrevHandle())
		{
			const qse_char_t* outer;

			outer = qse_sio_getpath ((qse_sio_t*)io.getPrevHandle());
			if (outer)
			{
				const qse_char_t* base;
	
				base = qse_basename (outer);
				if (base != outer && ioname[0] != QSE_T('/'))
				{
					size_t tmplen, totlen, dirlen;
				
					dirlen = base - outer;
					totlen = qse_strlen(ioname) + dirlen;
					if (totlen >= QSE_COUNTOF(fbuf))
					{
						dbuf = (qse_char_t*) QSE_MMGR_ALLOC (
							((Awk*)io)->getMmgr(),
							QSE_SIZEOF(qse_char_t) * (totlen + 1)
						);
						if (dbuf == QSE_NULL)
						{
							((Awk*)io)->setError (QSE_AWK_ENOMEM);
							return -1;
						}
	
						file = dbuf;
					}
					else file = fbuf;
	
					tmplen = qse_strncpy ((char_t*)file, outer, dirlen);
					qse_strcpy ((char_t*)file + tmplen, ioname);
				}
			}
		}

		sio = open_sio (
			io, QSE_NULL, file,
			(io.getMode() == READ? 
				(QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_KEEPPATH): 
				(QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR))
		);
		if (dbuf) QSE_MMGR_FREE (((Awk*)io)->getMmgr(), dbuf);
		if (sio == QSE_NULL) return -1;

		io.setHandle (sio);
	}

	return 1;
}

int StdAwk::SourceString::close (Data& io)
{
	if (!io.isMaster()) qse_sio_close ((qse_sio_t*)io.getHandle());
	return 0;
}

StdAwk::ssize_t StdAwk::SourceString::read (Data& io, char_t* buf, size_t len)
{
	if (io.isMaster())
	{
		qse_size_t n = 0;
		while (*ptr != QSE_T('\0') && n < len) buf[n++] = *ptr++;
		return n;
	}
	else
	{
		return qse_sio_getstrn ((qse_sio_t*)io.getHandle(), buf, len);
	}
}

StdAwk::ssize_t StdAwk::SourceString::write (Data& io, const char_t* buf, size_t len)
{
	if (io.isMaster())
	{
		return -1;
	}
	else
	{
		// in fact, this block will never be reached as
		// there is no included file concept for deparsing 
		return qse_sio_putstrn ((qse_sio_t*)io.getHandle(), buf, len);
	}
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

