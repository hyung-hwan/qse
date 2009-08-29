/*
 * $Id: StdAwk.cpp 272 2009-08-28 09:48:02Z hyunghwan.chung $
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

#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/str.h>
#include <qse/cmn/time.h>
#include <qse/cmn/fio.h>
#include <qse/cmn/pio.h>
#include <qse/cmn/sio.h>
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

	ADDFNC (QSE_T("sin"),        1, 1, &StdAwk::sin);
	ADDFNC (QSE_T("cos"),        1, 1, &StdAwk::cos);
	ADDFNC (QSE_T("tan"),        1, 1, &StdAwk::tan);
	ADDFNC (QSE_T("atan"),       1, 1, &StdAwk::atan);
	ADDFNC (QSE_T("atan2"),      2, 2, &StdAwk::atan2);
	ADDFNC (QSE_T("log"),        1, 1, &StdAwk::log);
	ADDFNC (QSE_T("exp"),        1, 1, &StdAwk::exp);
	ADDFNC (QSE_T("sqrt"),       1, 1, &StdAwk::sqrt);
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

int StdAwk::sin (Run& run, Value& ret, const Value* args, size_t nargs, 
	const char_t* name, size_t len)
{
	return ret.setReal (
	#if defined(HAVE_SINL)
		(real_t)::sinl(args[0].toReal())
	#elif defined(HAVE_SIN)
		(real_t)::sin(args[0].toReal())
	#elif defined(HAVE_SINF)
		(real_t)::sinf(args[0].toReal())
	#else
		#error ### no sin function available ###
	#endif
	);
}

int StdAwk::cos (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.setReal (
	#if defined(HAVE_COSL)
		(real_t)::cosl(args[0].toReal())
	#elif defined(HAVE_COS)
		(real_t)::cos(args[0].toReal())
	#elif defined(HAVE_COSF)
		(real_t)::cosf(args[0].toReal())
	#else
		#error ### no cos function available ###
	#endif
	);
}

int StdAwk::tan (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.setReal (
	#if defined(HAVE_TANL)
		(real_t)::tanl(args[0].toReal())
	#elif defined(HAVE_TAN)
		(real_t)::tan(args[0].toReal())
	#elif defined(HAVE_TANF)
		(real_t)::tanf(args[0].toReal())
	#else
		#error ### no tan function available ###
	#endif
	);
}

int StdAwk::atan (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.setReal (
	#if defined(HAVE_ATANL)
		(real_t)::atanl(args[0].toReal())
	#elif defined(HAVE_ATAN)
		(real_t)::atan(args[0].toReal())
	#elif defined(HAVE_ATANF)
		(real_t)::atanf(args[0].toReal())
	#else
		#error ### no atan function available ###
	#endif
	);
}

int StdAwk::atan2 (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.setReal (
	#if defined(HAVE_ATAN2L)
		(real_t)::atan2l(args[0].toReal(), args[1].toReal())
	#elif defined(HAVE_ATAN2)
		(real_t)::atan2(args[0].toReal(), args[1].toReal())
	#elif defined(HAVE_ATAN2F)
		(real_t)::atan2f(args[0].toReal(), args[1].toReal())
	#else
		#error ### no atan2 function available ###
	#endif
	);
}

int StdAwk::log (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.setReal (
	#if defined(HAVE_LOGL)
		(real_t)::logl(args[0].toReal())
	#elif defined(HAVE_LOG)
		(real_t)::log(args[0].toReal())
	#elif defined(HAVE_LOGF)
		(real_t)::logf(args[0].toReal())
	#else
		#error ### no log function available ###
	#endif
	);
}

int StdAwk::exp (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.setReal (
	#if defined(HAVE_EXPL)
		(real_t)::expl(args[0].toReal())
	#elif defined(HAVE_EXP)
		(real_t)::exp(args[0].toReal())
	#elif defined(HAVE_EXPF)
		(real_t)::expf(args[0].toReal())
	#else
		#error ### no exp function available ###
	#endif
	);
}

int StdAwk::sqrt (Run& run, Value& ret, const Value* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.setReal (
	#if defined(HAVE_SQRTL)
		(real_t)::sqrtl(args[0].toReal())
	#elif defined(HAVE_SQRT)
		(real_t)::sqrt(args[0].toReal())
	#elif defined(HAVE_SQRTF)
		(real_t)::sqrtf(args[0].toReal())
	#else
		#error ### no sqrt function available ###
	#endif
	);
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
		(qse_mmgr_t*)this,
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
		(qse_mmgr_t*)this,
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
	if (n <= -1) setError (ERR_NOMEM);
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
		qse_map_t* map;
		qse_map_pair_t* pair;
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
			((Run*)io)->setError (ERR_IONMNL, &arg);
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

		pair = qse_map_search (map, ibuf, ibuflen);
		QSE_ASSERT (pair != QSE_NULL);

		v = (qse_awk_val_t*)QSE_MAP_VPTR(pair);
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
			((Run*)io)->setError (ERR_IONMNL, &arg);
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
				((Run*)io)->setError (ERR_OPEN, &arg);
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
			((Run*)io)->setError (ERR_IONMNL, &arg);
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
				((Run*)io)->setError (ERR_OPEN, &arg);
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
StdAwk::real_t StdAwk::pow (real_t x, real_t y) 
{ 
	return ::pow (x, y); 
}

int StdAwk::vsprintf (
	char_t* buf, size_t size, const char_t* fmt, va_list arg) 
{
	return qse_vsprintf (buf, size, fmt, arg);
}

int StdAwk::SourceFile::open (Data& io)
{
	qse_sio_t* sio;
	const char_t* ioname = io.getName();

	if (ioname == QSE_NULL)
	{
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
				((Awk*)io)->setError (ERR_OPEN, &ea);
				return -1;
			}

			base = qse_awk_basename ((awk_t*)io, name);
			if (base != name)
			{
				dir.ptr = name;
				dir.len = base - name;
			}
		}
	}
	else
	{
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
					((Awk*)io)->setError (ERR_NOMEM);
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
			((Awk*)io)->setError (ERR_OPEN, &ea);
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

StdAwk::ssize_t StdAwk::SourceFile::write (Data& io, char_t* buf, size_t len)
{
	return qse_sio_putsn ((qse_sio_t*)io.getHandle(), buf, len);
}

int StdAwk::SourceString::open (Data& io)
{
	qse_sio_t* sio;
	const char_t* ioname = io.getName();

	if (ioname == QSE_NULL)
	{
		/* SourceString does not support writing */
		if (io.getMode() == WRITE) return -1;
		ptr = str;
	}
	else
	{
		/* open an included file */
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
			((Awk*)io)->setError (ERR_OPEN, &ea);
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

StdAwk::ssize_t StdAwk::SourceString::write (Data& io, char_t* buf, size_t len)
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

