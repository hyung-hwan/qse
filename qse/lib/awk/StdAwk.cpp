/*
 * $Id: StdAwk.cpp 220 2009-07-01 13:14:39Z hyunghwan.chung $
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

int StdAwk::sin (Run& run, Return& ret, const Argument* args, size_t nargs, 
	const char_t* name, size_t len)
{
	return ret.set (
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

int StdAwk::cos (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.set (
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

int StdAwk::tan (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.set (
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

int StdAwk::atan (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.set (
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

int StdAwk::atan2 (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.set (
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

int StdAwk::log (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.set (
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

int StdAwk::exp (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.set (
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

int StdAwk::sqrt (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.set (
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

int StdAwk::fnint (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.set (args[0].toInt());
}

int StdAwk::rand (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	return ret.set ((real_t)(::rand() % RAND_MAX) / RAND_MAX);
}

int StdAwk::srand (Run& run, Return& ret, const Argument* args, size_t nargs,
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
	return ret.set ((long_t)prevSeed);
}

int StdAwk::system (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	size_t l;
	const char_t* ptr = args[0].toStr(&l);

#ifdef _WIN32
	return ret.set ((long_t)::_tsystem(ptr));
#elif defined(QSE_CHAR_IS_MCHAR)
	return ret.set ((long_t)::system(ptr));
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
	int n = ret.set ((long_t)::system(mbs));

	qse_awk_free ((awk_t*)(Awk*)run, mbs);
	return n;
#endif
}

int StdAwk::openPipe (Pipe& io) 
{ 
	Awk::Pipe::Mode mode = io.getMode();
	qse_pio_t* pio = QSE_NULL;
	int flags;

	switch (mode)
	{
		case Awk::Pipe::READ:
			/* TODO: should we specify ERRTOOUT? */
			flags = QSE_PIO_READOUT |
			        QSE_PIO_ERRTOOUT;
			break;
		case Awk::Pipe::WRITE:
			flags = QSE_PIO_WRITEIN;
			break;
		case Awk::Pipe::RW:
			flags = QSE_PIO_READOUT |
			        QSE_PIO_ERRTOOUT |
			        QSE_PIO_WRITEIN;
			break;
	}

	pio = qse_pio_open (
		((Awk*)io)->getMmgr(),
		0, 
		io.getName(), 
		flags|QSE_PIO_TEXT|QSE_PIO_SHELL
	);
	if (pio == QSE_NULL) return -1;

	io.setHandle (pio);
	return 1;
}

int StdAwk::closePipe (Pipe& io) 
{
	qse_pio_close ((qse_pio_t*)io.getHandle());
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

// file io handlers 
int StdAwk::openFile (File& io) 
{ 
	Awk::File::Mode mode = io.getMode();
	qse_fio_t* fio = QSE_NULL;
	int flags;

	switch (mode)
	{
		case Awk::File::READ:
			flags = QSE_FIO_READ;
			break;
		case Awk::File::WRITE:
			flags = QSE_FIO_WRITE | 
			        QSE_FIO_CREATE | 
			        QSE_FIO_TRUNCATE;
			break;
		case Awk::File::APPEND:
			flags = QSE_FIO_APPEND |
			        QSE_FIO_CREATE;
			break;
	}

	fio = qse_fio_open (
		((Awk*)io)->getMmgr(),
		0, 
		io.getName(), 
		flags | QSE_FIO_TEXT,
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

#if 0
// console io handlers 
int StdAwk::openConsole (Console& io) 
{ 
	qse_sio_t* fp;
	Console::Mode mode = io.getMode();

	switch (mode)
	{
		case Console::READ:
		{
			if (runarg.ptr == QSE_NULL) 
			{
				io.setHandle (qse_sio_in);
				return 1;
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
				file = runarg.ptr[runarg_index];
	
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
	
				pair = qse_map_search (map, ibuf, ibuflen);
				QSE_ASSERT (pair != QSE_NULL);
	
				v = QSE_MAP_VPTR(pair);
				QSE_ASSERT (v != QSE_NULL);
	
				out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
				if (qse_awk_rtx_valtostr (rtx, v, &out) == QSE_NULL) return -1;
	
				if (out.u.cpldup.len == 0)
				{
					/* the name is empty */
					qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
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
	
					qse_awk_rtx_seterror (
						rtx, QSE_AWK_EIONMNL, 0, &errarg);
	
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
						qse_cstr_t errarg;
	
						errarg.ptr = file;
						errarg.len = qse_strlen(file);
	
						qse_awk_rtx_seterror (
							rtx, QSE_AWK_EOPEN, 0, &errarg);
	
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
				riod->handle = sio;
	
				/* increment the counter of files successfully opened */
				rxtn->c.in.count++;
			}
	
			rxtn->c.in.index++;
			}

			break;
		}

		case Console::WRITE:
			break;	
	}
		
#if 0
	FILE* fp = QSE_NULL;
	const char_t* fn = QSE_NULL;

	switch (mode)
	{
		case StdAwk::Console::READ:

			if (numConInFiles == 0) fp = stdin;
			else
			{
				fn = conInFile[0];
				fp = qse_fopen (fn, QSE_T("r"));
			}
			break;

		case StdAwk::Console::WRITE:

			if (numConOutFiles == 0) fp = stdout;
			else
			{
				fn = conOutFile[0];
				fp = qse_fopen (fn, QSE_T("w"));
			}
			break;
	}

	if (fp == NULL) return -1;

	ConTrack* t = (ConTrack*) 
		qse_awk_alloc (awk, QSE_SIZEOF(ConTrack));
	if (t == QSE_NULL)
	{
		if (fp != stdin && fp != stdout) fclose (fp);
		return -1;
	}

	t->handle = fp;
	t->nextConIdx = 1;

	if (fn != QSE_NULL) 
	{
		if (io.setFileName(fn) == -1)
		{
			if (fp != stdin && fp != stdout) fclose (fp);
			qse_awk_free (awk, t);
			return -1;
		}
	}

	io.setHandle (t);
	return 1;
#endif
}

int StdAwk::closeConsole (Console& io) 
{ 
	ConTrack* t = (ConTrack*)io.getHandle();
	FILE* fp = t->handle;

	if (fp == stdout || fp == stderr) fflush (fp);
	if (fp != stdin && fp != stdout && fp != stderr) fclose (fp);

	qse_awk_free (awk, t);
	return 0;
}

ssize_t StdAwk::readConsole (Console& io, char_t* buf, size_t len) 
{
	ConTrack* t = (ConTrack*)io.getHandle();
	FILE* fp = t->handle;
	ssize_t n = 0;

	while (n < (ssize_t)len)
	{
		qse_cint_t c = qse_fgetc (fp);
		if (c == QSE_CHAR_EOF) 
		{
			if (qse_ferror(fp)) return -1;
			if (t->nextConIdx >= numConInFiles) break;

			const char_t* fn = conInFile[t->nextConIdx];
			FILE* nfp = qse_fopen (fn, QSE_T("r"));
			if (nfp == QSE_NULL) return -1;

			if (io.setFileName(fn) == -1 || io.setFNR(0) == -1)
			{
				fclose (nfp);
				return -1;
			}

			fclose (fp);
			fp = nfp;
			t->nextConIdx++;
			t->handle = fp;

			if (n == 0) continue;
			else break;
		}

		buf[n++] = c;
		if (c == QSE_T('\n')) break;
	}

	return n;
}

ssize_t StdAwk::writeConsole (Console& io, const char_t* buf, size_t len) 
{
	ConTrack* t = (ConTrack*)io.getHandle();
	FILE* fp = t->handle;
	size_t left = len;

	while (left > 0)
	{
		if (*buf == QSE_T('\0')) 
		{
			if (qse_fputc(*buf,fp) == QSE_CHAR_EOF) return -1;
			left -= 1; buf += 1;
		}
		else
		{
			int chunk = (left > QSE_TYPE_MAX(int))? QSE_TYPE_MAX(int): (int)left;
			int n = qse_fprintf (fp, QSE_T("%.*s"), chunk, buf);
			if (n < 0 || n > chunk) return -1;
			left -= n; buf += n;
		}
	}

	return len;
}

int StdAwk::flushConsole (Console& io) 
{ 
	ConTrack* t = (ConTrack*)io.getHandle();
	FILE* fp = t->handle;
	return ::fflush (fp);
}

int StdAwk::nextConsole (Console& io) 
{ 
	StdAwk::Console::Mode mode = io.getMode();

	ConTrack* t = (ConTrack*)io.getHandle();
	FILE* ofp = t->handle;
	FILE* nfp = QSE_NULL;
	const char_t* fn = QSE_NULL;

	switch (mode)
	{
		case StdAwk::Console::READ:

			if (t->nextConIdx >= numConInFiles) return 0;
			fn = conInFile[t->nextConIdx];
			nfp = qse_fopen (fn, QSE_T("r"));
			break;

		case StdAwk::Console::WRITE:

			if (t->nextConIdx >= numConOutFiles) return 0;
			fn = conOutFile[t->nextConIdx];
			nfp = qse_fopen (fn, QSE_T("w"));
			break;
	}

	if (nfp == QSE_NULL) return -1;

	if (fn != QSE_NULL)
	{
		if (io.setFileName (fn) == -1)
		{
			fclose (nfp);
			return -1;
		}
	}

	fclose (ofp);

	t->nextConIdx++;
	t->handle = nfp;

	return 1;
}
#endif

// memory allocation primitives
void* StdAwk::allocMem (size_t n) throw ()
{ 
	return ::malloc (n); 
}

void* StdAwk::reallocMem (void* ptr, size_t n) throw ()
{ 
	return ::realloc (ptr, n); 
}

void  StdAwk::freeMem (void* ptr) throw ()
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

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

