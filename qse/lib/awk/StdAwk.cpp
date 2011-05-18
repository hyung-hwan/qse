/*
 * $Id: StdAwk.cpp 460 2011-05-17 14:56:54Z hyunghwan.chung $
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

#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/str.h>
#include <qse/cmn/time.h>
#include <qse/cmn/fio.h>
#include <qse/cmn/pio.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/misc.h>
#include <qse/cmn/stdio.h>
#include "awk.h"

#include <stdlib.h>
#include <math.h>

#ifdef _WIN32
	#include <tchar.h>
#else
	#include <wchar.h>
#endif

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

#define ADDFNC(name,min,max,impl) \
	do { \
		if (addFunction (name, min, max, \
			(FunctionHandler)impl) == -1)  \
		{ \
			Awk::close (); \
			return -1; \
		} \
	} while (0)

int StdAwk::open () 
{
	int n = Awk::open ();
	if (n == -1) return n;

	ADDFNC (QSE_T("int"),        1, 1, &StdAwk::fnint);
	ADDFNC (QSE_T("rand"),       0, 0, &StdAwk::rand);
	ADDFNC (QSE_T("srand"),      0, 1, &StdAwk::srand);
	ADDFNC (QSE_T("system"),     1, 1, &StdAwk::system);

	qse_ntime_t now;

	if (qse_gettime(&now) == -1) this->seed = 0;
	else this->seed = (unsigned int)now;

	::srand (this->seed);
	return 0;
}

void StdAwk::close () 
{
	clearConsoleOutputs ();
	Awk::close ();
}

int StdAwk::fnint (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.setInt (args[0].toInt());
}

int StdAwk::rand (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.setReal ((real_t)(::rand() % RAND_MAX) / RAND_MAX);
}

int StdAwk::srand (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	unsigned int prevSeed = this->seed;

	if (nargs == 0)
	{
		qse_ntime_t now;

		if (qse_gettime (&now) == -1)
			this->seed = (unsigned int)now;
		else this->seed >>= 1;
	}
	else
	{
		this->seed = (unsigned int)args[0].toInt();
	}

	::srand (this->seed);
	return ret.setInt ((long_t)prevSeed);
}

int StdAwk::system (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	size_t l;
	const char_t* ptr = args[0].toStr(&l);

#ifdef _WIN32
	return ret.setInt ((long_t)::_tsystem(ptr));
#elif defined(QSE_CHAR_IS_MCHAR)
	return ret.setInt ((long_t)::system(ptr));
#else
	char* mbs = (char*) qse_awk_alloc ((awk_t*)(Awk*)run, l*5+1);
	if (mbs == QSE_NULL) return -1;

	/* at this point, the string is guaranteed to be 
	 * null-terminating. so qse_wcstombs() can be used to convert
	 * the string, not qse_wcsntombsn(). */

	qse_size_t mbl = l * 5;
	if (qse_wcstombs (ptr, mbs, &mbl) != l && mbl >= l * 5) 
	{
		/* not the entire string is converted.
		 * mbs is not null-terminated properly. */
		qse_awk_free ((awk_t*)(Awk*)run, mbs);
		return -1;
	}

	mbs[mbl] = '\0';
	int n = ret.setInt ((long_t)::system(mbs));

	qse_awk_free ((awk_t*)(Awk*)run, mbs);
	return n;
#endif
}

int StdAwk::openPipe (Pipe& io) 
{ 
	Awk::Pipe::Mode mode = io.getMode();
	qse_pio_t* pio = QSE_NULL;
	int flags = QSE_PIO_TEXT | QSE_PIO_SHELL;

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
		flags
	);
	if (pio == QSE_NULL) return -1;

	io.setHandle (pio);
	return 1;
}

int StdAwk::closePipe (Pipe& io) 
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
	return 0; 
}

StdAwk::ssize_t StdAwk::readPipe (Pipe& io, char_t* buf, size_t len) 
{ 
	return qse_pio_read ((qse_pio_t*)io.getHandle(), buf, len, QSE_PIO_OUT);
}

StdAwk::ssize_t StdAwk::writePipe (Pipe& io, const char_t* buf, size_t len) 
{ 
	return qse_pio_write ((qse_pio_t*)io.getHandle(), buf, len, QSE_PIO_IN);
}

int StdAwk::flushPipe (Pipe& io) 
{ 
	return qse_pio_flush ((qse_pio_t*)io.getHandle(), QSE_PIO_IN); 
}

int StdAwk::openFile (File& io) 
{ 
	Awk::File::Mode mode = io.getMode();
	qse_fio_t* fio = QSE_NULL;
	int flags = QSE_FIO_TEXT;

	switch (mode)
	{
		case Awk::File::READ:
			flags |= QSE_FIO_READ;
			break;
		case Awk::File::WRITE:
			flags |= QSE_FIO_WRITE | 
			         QSE_FIO_CREATE | 
			         QSE_FIO_TRUNCATE;
			break;
		case Awk::File::APPEND:
			flags |= QSE_FIO_APPEND |
			         QSE_FIO_CREATE;
			break;
	}

	fio = qse_fio_open (
		this->getMmgr(),
		0, 
		io.getName(), 
		flags,
		QSE_FIO_RUSR | QSE_FIO_WUSR |
		QSE_FIO_RGRP | QSE_FIO_ROTH
	);	
	if (fio == NULL) return -1;

	io.setHandle (fio);
	return 1;
}

int StdAwk::closeFile (File& io) 
{ 
	qse_fio_close ((qse_fio_t*)io.getHandle());
	return 0; 
}

StdAwk::ssize_t StdAwk::readFile (File& io, char_t* buf, size_t len) 
{
	return qse_fio_read ((qse_fio_t*)io.getHandle(), buf, len);
}

StdAwk::ssize_t StdAwk::writeFile (File& io, const char_t* buf, size_t len)
{
	return qse_fio_write ((qse_fio_t*)io.getHandle(), buf, len);
}

int StdAwk::flushFile (File& io) 
{ 
	return qse_fio_flush ((qse_fio_t*)io.getHandle());
}

int StdAwk::addConsoleOutput (const char_t* arg, size_t len) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = ofile.add (awk, arg, len);
	if (n <= -1) setError (QSE_AWK_ENOMEM);
	return n;
}

int StdAwk::addConsoleOutput (const char_t* arg) 
{
	return addConsoleOutput (arg, qse_strlen(arg));
}

void StdAwk::clearConsoleOutputs () 
{
	ofile.clear (awk);
}

int StdAwk::open_console_in (Console& io) 
{ 
	qse_awk_rtx_t* rtx = (rtx_t*)io;

	if (runarg.ptr == QSE_NULL) 
	{
		QSE_ASSERT (runarg.len == 0 && runarg.capa == 0);

		if (runarg_count == 0) 
		{
			io.setHandle (qse_sio_in);
			runarg_count++;
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
		qse_awk_rtx_valtostr_out_t out;

	nextfile:
		file = runarg.ptr[runarg_index].ptr;

		if (file == QSE_NULL)
		{
			/* no more input file */

			if (runarg_count == 0)
			{
				/* all ARGVs are empty strings. 
				 * so no console files were opened.
				 * open the standard input here.
				 *
				 * 'BEGIN { ARGV[1]=""; ARGV[2]=""; }
				 *        { print $0; }' file1 file2
				 */
				io.setHandle (qse_sio_in);
				runarg_count++;
				return 1;
			}

			return 0;
		}

		if (qse_strlen(file) != runarg.ptr[runarg_index].len)
		{
			cstr_t arg;
			arg.ptr = file;
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
		argv = qse_awk_rtx_getgbl (rtx, QSE_AWK_GBL_ARGV);
		QSE_ASSERT (argv != QSE_NULL);
		QSE_ASSERT (argv->type == QSE_AWK_VAL_MAP);

		map = ((qse_awk_val_map_t*)argv)->map;
		QSE_ASSERT (map != QSE_NULL);
		
		// ok to find ARGV[runarg_index] as ARGV[0]
		// has been skipped.
		ibuflen = qse_awk_longtostr (
			rtx->awk, runarg_index, 
			10, QSE_NULL,
			ibuf, QSE_COUNTOF(ibuf)
		);

		pair = qse_htb_search (map, ibuf, ibuflen);
		QSE_ASSERT (pair != QSE_NULL);

		v = (qse_awk_val_t*)QSE_HTB_VPTR(pair);
		QSE_ASSERT (v != QSE_NULL);

		out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
		if (qse_awk_rtx_valtostr (rtx, v, &out) == QSE_NULL) return -1;

		if (out.u.cpldup.len == 0)
		{
			/* the name is empty */
			qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
			runarg_index++;
			goto nextfile;
		}

		if (qse_strlen(out.u.cpldup.ptr) < out.u.cpldup.len)
		{
			/* the name contains one or more '\0' */
			cstr_t arg;
			arg.ptr = out.u.cpldup.ptr;
			arg.len = qse_strlen (arg.ptr);
			((Run*)io)->setError (QSE_AWK_EIONMNL, &arg);
			qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
			return -1;
		}

		file = out.u.cpldup.ptr;

		if (file[0] == QSE_T('-') && file[1] == QSE_T('\0'))
		{
			/* special file name '-' */
			sio = qse_sio_in;
		}
		else
		{
			sio = qse_sio_open (
				rtx->awk->mmgr, 0, file, QSE_SIO_READ);
			if (sio == QSE_NULL)
			{
				cstr_t arg;
				arg.ptr = file;
				arg.len = qse_strlen (arg.ptr);
				((Run*)io)->setError (QSE_AWK_EOPEN, &arg);
				qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
				return -1;
			}
		}

		if (qse_awk_rtx_setfilename (
			rtx, file, qse_strlen(file)) == -1)
		{
			if (sio != qse_sio_in) qse_sio_close (sio);
			qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
			return -1;
		}

		qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
		io.setHandle (sio);

		/* increment the counter of files successfully opened */
		runarg_count++;
		runarg_index++;
		return 1;
	}

}

int StdAwk::open_console_out (Console& io) 
{
	qse_awk_rtx_t* rtx = (rtx_t*)io;

	if (ofile.ptr == QSE_NULL)
	{
		QSE_ASSERT (ofile.len == 0 && ofile.capa == 0);

		if (ofile_count == 0) 
		{
			io.setHandle (qse_sio_out);
			ofile_count++;
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

		file = ofile.ptr[ofile_index].ptr;

		if (file == QSE_NULL)
		{
			/* no more input file */
			return 0;
		}

		if (qse_strlen(file) != ofile.ptr[ofile_index].len)
		{	
			cstr_t arg;
			arg.ptr = file;
			arg.len = qse_strlen (arg.ptr);
			((Run*)io)->setError (QSE_AWK_EIONMNL, &arg);
			return -1;
		}

		if (file[0] == QSE_T('-') && file[1] == QSE_T('\0'))
		{
			/* special file name '-' */
			sio = qse_sio_out;
		}
		else
		{
			sio = qse_sio_open (
				rtx->awk->mmgr, 0, file, QSE_SIO_READ);
			if (sio == QSE_NULL)
			{
				cstr_t arg;
				arg.ptr = file;
				arg.len = qse_strlen (arg.ptr);
				((Run*)io)->setError (QSE_AWK_EOPEN, &arg);
				return -1;
			}
		}
		
		if (qse_awk_rtx_setofilename (
			rtx, file, qse_strlen(file)) == -1)
		{
			qse_sio_close (sio);
			return -1;
		}

		io.setHandle (sio);

		ofile_index++;
		ofile_count++;
		return 1;
	}
}

int StdAwk::openConsole (Console& io) 
{
	Console::Mode mode = io.getMode();

	if (mode == Console::READ)
	{
		runarg_count = 0;
		runarg_index = 0;
		if (runarg.len > 0) 
		{
			// skip ARGV[0]
			runarg_index++;
		}
		return open_console_in (io);
	}
	else
	{
		QSE_ASSERT (mode == Console::WRITE);

		ofile_count = 0;
		ofile_index = 0;
		return open_console_out (io);
	}
}

int StdAwk::closeConsole (Console& io) 
{ 
	qse_sio_t* sio;

	sio = (qse_sio_t*)io.getHandle();
	if (sio != qse_sio_in &&
	    sio != qse_sio_out &&
	    sio != qse_sio_err)
	{
		qse_sio_close (sio);
	}

	return 0;
}

StdAwk::ssize_t StdAwk::readConsole (Console& io, char_t* data, size_t size) 
{
	qse_ssize_t nn;

	while ((nn = qse_sio_getsn((qse_sio_t*)io.getHandle(),data,size)) == 0)
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

		if (sio != QSE_NULL && 
		    sio != qse_sio_in && 
		    sio != qse_sio_out &&
		    sio != qse_sio_err) 
		{
			qse_sio_close (sio);
		}
	}

	return nn;
}

StdAwk::ssize_t StdAwk::writeConsole (Console& io, const char_t* data, size_t size) 
{
	return qse_sio_putsn (
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

	if (sio != QSE_NULL && 
	    sio != qse_sio_in && 
	    sio != qse_sio_out &&
	    sio != qse_sio_err)
	{
		qse_sio_close (sio);
	}

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
int StdAwk::vsprintf (
	char_t* buf, size_t size, const char_t* fmt, va_list arg) 
{
	return qse_vsprintf (buf, size, fmt, arg);
}

StdAwk::real_t StdAwk::pow (real_t x, real_t y) 
{ 
#if defined(HAVE_POWL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ::powl (x, y);
#elif defined(HAVE_POW)
	return ::pow (x, y);
#elif defined(HAVE_POWF)
	return ::powf (x, y);
#else
	#error ### no pow function available ###
#endif
}

StdAwk::real_t StdAwk::sin (real_t x)
{ 
#if defined(HAVE_SINL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ::sinl (x);
#elif defined(HAVE_SIN)
	return ::sin (x);
#elif defined(HAVE_SINF)
	return ::sinf (x);
#else
	#error ### no sin function available ###
#endif
}

StdAwk::real_t StdAwk::cos (real_t x)
{ 
#if defined(HAVE_COSL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ::cosl (x);
#elif defined(HAVE_COS)
	return ::cos (x);
#elif defined(HAVE_COSF)
	return ::cosf (x);
#else
	#error ### no cos function available ###
#endif
}

StdAwk::real_t StdAwk::tan (real_t x)
{ 
#if defined(HAVE_TANL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ::tanl (x);
#elif defined(HAVE_TAN)
	return ::tan (x);
#elif defined(HAVE_TANF)
	return ::tanf (x);
#else
	#error ### no tan function available ###
#endif
}

StdAwk::real_t StdAwk::atan (real_t x)
{ 
#if defined(HAVE_ATANL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ::atanl (x);
#elif defined(HAVE_ATAN)
	return ::atan (x);
#elif defined(HAVE_ATANF)
	return ::atanf (x);
#else
	#error ### no atan function available ###
#endif
}

StdAwk::real_t StdAwk::atan2 (real_t x, real_t y) 
{ 
#if defined(HAVE_ATAN2L) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ::atan2l (x, y);
#elif defined(HAVE_ATAN2)
	return ::atan2 (x, y);
#elif defined(HAVE_ATAN2F)
	return ::atan2f (x, y);
#else
	#error ### no atan2 function available ###
#endif
}

StdAwk::real_t StdAwk::log (real_t x)
{ 
#if defined(HAVE_LOGL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ::logl (x);
#elif defined(HAVE_LOG)
	return ::log (x);
#elif defined(HAVE_LOGF)
	return ::logf (x);
#else
	#error ### no log function available ###
#endif
}

StdAwk::real_t StdAwk::exp (real_t x)
{ 
#if defined(HAVE_EXPL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ::expl (x);
#elif defined(HAVE_EXP)
	return ::exp (x);
#elif defined(HAVE_EXPF)
	return ::expf (x);
#else
	#error ### no exp function available ###
#endif
}

StdAwk::real_t StdAwk::sqrt (real_t x)
{ 
#if defined(HAVE_SQRTL) && (QSE_SIZEOF_LONG_DOUBLE > QSE_SIZEOF_DOUBLE)
	return ::sqrtl (x);
#elif defined(HAVE_SQRT)
	return ::sqrt (x);
#elif defined(HAVE_SQRTF)
	return ::sqrtf (x);
#else
	#error ### no sqrt function available ###
#endif
}

int StdAwk::SourceFile::open (Data& io)
{
	qse_sio_t* sio;
	const char_t* ioname = io.getName();

	if (ioname == QSE_NULL)
	{
		// open the main source file.

		if (name[0] == QSE_T('-') && name[1] == QSE_T('\0'))
		{
			sio = (io.getMode() == READ)? qse_sio_in: qse_sio_out;
		}
		else
		{
			const qse_char_t* base;

			sio = qse_sio_open (
				((awk_t*)io)->mmgr,
				0,
				name,
				(io.getMode() == READ? 
					QSE_SIO_READ: 
					(QSE_SIO_WRITE|QSE_SIO_CREATE|QSE_SIO_TRUNCATE))
			);
			if (sio == QSE_NULL) 
			{
				qse_cstr_t ea;
				ea.ptr = name;
				ea.len = qse_strlen(name);
				((Awk*)io)->setError (QSE_AWK_EOPEN, &ea);
				return -1;
			}

			base = qse_basename (name);
			if (base != name)
			{
				dir.ptr = name;
				dir.len = base - name;
			}
		}
	}
	else
	{
		// open an included file

		const char_t* file = ioname;
		char_t fbuf[64];
		char_t* dbuf = QSE_NULL;
	
		if (dir.len > 0 && ioname[0] != QSE_T('/'))
		{
			size_t tmplen, totlen;
			
			totlen = qse_strlen(ioname) + dir.len;
			if (totlen >= QSE_COUNTOF(fbuf))
			{
				dbuf = (qse_char_t*) QSE_MMGR_ALLOC (
					((awk_t*)io)->mmgr,
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

			tmplen = qse_strncpy (
				(char_t*)file, dir.ptr, dir.len);
			qse_strcpy ((char_t*)file + tmplen, ioname);
		}

		sio = qse_sio_open (
			((awk_t*)io)->mmgr,
			0,
			file,
			(io.getMode() == READ? 
				QSE_SIO_READ: 
				(QSE_SIO_WRITE|QSE_SIO_CREATE|QSE_SIO_TRUNCATE))
		);

		if (dbuf != QSE_NULL) QSE_MMGR_FREE (((awk_t*)io)->mmgr, dbuf);
		if (sio == QSE_NULL)
		{
			qse_cstr_t ea;
			ea.ptr = file;
			ea.len = qse_strlen(file);
			((Awk*)io)->setError (QSE_AWK_EOPEN, &ea);
			return -1;
		}
	}

	io.setHandle (sio);
	return 1;
}

int StdAwk::SourceFile::close (Data& io)
{
	qse_sio_t* sio = (qse_sio_t*)io.getHandle();

	qse_sio_flush (sio);

	if (sio != qse_sio_in && sio != qse_sio_out && sio != qse_sio_err)
	{
		qse_sio_close (sio);
	}

	return 0;
}

StdAwk::ssize_t StdAwk::SourceFile::read (Data& io, char_t* buf, size_t len)
{
	return qse_sio_getsn ((qse_sio_t*)io.getHandle(), buf, len);
}

StdAwk::ssize_t StdAwk::SourceFile::write (Data& io, const char_t* buf, size_t len)
{
	return qse_sio_putsn ((qse_sio_t*)io.getHandle(), buf, len);
}

int StdAwk::SourceString::open (Data& io)
{
	qse_sio_t* sio;
	const char_t* ioname = io.getName();

	if (ioname == QSE_NULL)
	{
		// open the main source file.
		// SourceString does not support writing.
		if (io.getMode() == WRITE) return -1;
		ptr = str;
	}
	else
	{
		// open an included file 
		sio = qse_sio_open (
			((awk_t*)io)->mmgr,
			0,
			ioname,
			(io.getMode() == READ? 
				QSE_SIO_READ: 
				(QSE_SIO_WRITE|QSE_SIO_CREATE|QSE_SIO_TRUNCATE))
		);
		if (sio == QSE_NULL)
		{
			qse_cstr_t ea;
			ea.ptr = ioname;
			ea.len = qse_strlen(ioname);
			((Awk*)io)->setError (QSE_AWK_EOPEN, &ea);
			return -1;
		}
		io.setHandle (sio);
	}

	return 1;
}

int StdAwk::SourceString::close (Data& io)
{
	if (io.getName() != QSE_NULL)
		qse_sio_close ((qse_sio_t*)io.getHandle());
	return 0;
}

StdAwk::ssize_t StdAwk::SourceString::read (Data& io, char_t* buf, size_t len)
{
	if (io.getName() == QSE_NULL)
	{
		qse_size_t n = 0;
		while (*ptr != QSE_T('\0') && n < len) buf[n++] = *ptr++;
		return n;
	}
	else
	{
		return qse_sio_getsn ((qse_sio_t*)io.getHandle(), buf, len);
	}
}

StdAwk::ssize_t StdAwk::SourceString::write (Data& io, const char_t* buf, size_t len)
{
	if (io.getName() == QSE_NULL)
	{
		return -1;
	}
	else
	{
		// in fact, this block will never be reached as
		// there is no included file concept for deparsing 
		return qse_sio_putsn ((qse_sio_t*)io.getHandle(), buf, len);
	}
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

