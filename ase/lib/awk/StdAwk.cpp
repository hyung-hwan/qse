/*
 * $Id: StdAwk.cpp 501 2008-12-17 08:39:15Z baconevi $
 *
 * {License}
 */

#if defined(hpux) || defined(__hpux) || defined(__hpux__)
	#define _INCLUDE__STDC_A1_SOURCE
#endif

#include <ase/awk/StdAwk.hpp>
#include <ase/cmn/str.h>
#include <ase/cmn/time.h>
#include <ase/utl/stdio.h>

#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
	#include <tchar.h>
#else
	#include <wchar.h>
#endif

/////////////////////////////////
ASE_BEGIN_NAMESPACE(ASE)
/////////////////////////////////

StdAwk::StdAwk ()
{
}

#define ADD_FUNC(name,min,max,impl) \
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

	ADD_FUNC (ASE_T("sin"),        1, 1, &StdAwk::sin);
	ADD_FUNC (ASE_T("cos"),        1, 1, &StdAwk::cos);
	ADD_FUNC (ASE_T("tan"),        1, 1, &StdAwk::tan);
	ADD_FUNC (ASE_T("atan"),       1, 1, &StdAwk::atan);
	ADD_FUNC (ASE_T("atan2"),      2, 2, &StdAwk::atan2);
	ADD_FUNC (ASE_T("log"),        1, 1, &StdAwk::log);
	ADD_FUNC (ASE_T("exp"),        1, 1, &StdAwk::exp);
	ADD_FUNC (ASE_T("sqrt"),       1, 1, &StdAwk::sqrt);
	ADD_FUNC (ASE_T("int"),        1, 1, &StdAwk::fnint);
	ADD_FUNC (ASE_T("rand"),       0, 0, &StdAwk::rand);
	ADD_FUNC (ASE_T("srand"),      0, 1, &StdAwk::srand);
	ADD_FUNC (ASE_T("systime"),    0, 0, &StdAwk::systime);
	ADD_FUNC (ASE_T("strftime"),   0, 2, &StdAwk::strftime);
	ADD_FUNC (ASE_T("strfgmtime"), 0, 2, &StdAwk::strfgmtime);
	ADD_FUNC (ASE_T("system"),     1, 1, &StdAwk::system);

	return 0;
}

int StdAwk::run (const char_t* main, const char_t** args, size_t nargs)
{
	ase_ntime_t now;

	if (ase_gettime(&now) == -1) this->seed = 0;
	else this->seed = (unsigned int)now;

	::srand (this->seed);

	return Awk::run (main, args, nargs);
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
	return ret.set ((long_t)::rand());
}

int StdAwk::srand (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	unsigned int prevSeed = this->seed;

	if (nargs == 0)
	{
		ase_ntime_t now;

		if (ase_gettime (&now) == -1)
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

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER>=1400)
	#define time_t __time64_t
	#define time _time64
	#define localtime _localtime64
	#define gmtime _gmtime64
#endif

int StdAwk::systime (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	ase_ntime_t now;

	if (ase_gettime(&now) == -1) 
		return ret.set (ASE_TYPE_MIN(long_t));
	else
		return ret.set ((long_t)now / ASE_MSEC_IN_SEC);
}

int StdAwk::strftime (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	const char_t* fmt;
	size_t fln;
       
	fmt = (nargs < 1)? ASE_T("%c"): args[0].toStr(&fln);
	time_t t = (nargs < 2)? ::time(NULL): (time_t)args[1].toInt();

	char_t buf[128]; 
	struct tm* tm;
#ifdef _WIN32
	tm = ::localtime (&t);
#else
	struct tm tmb;
	tm = ::localtime_r (&t, &tmb);
#endif

#ifdef ASE_CHAR_IS_MCHAR
	size_t l = ::strftime (buf, ASE_COUNTOF(buf), fmt, tm);
#else
	size_t l = ::wcsftime (buf, ASE_COUNTOF(buf), fmt, tm);
#endif

	return ret.set (buf, l);	
}

int StdAwk::strfgmtime (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	const char_t* fmt;
	size_t fln;
       
	fmt = (nargs < 1)? ASE_T("%c"): args[0].toStr(&fln);
	time_t t = (nargs < 2)? ::time(NULL): (time_t)args[1].toInt();

	char_t buf[128]; 
	struct tm* tm;
#ifdef _WIN32
	tm = ::gmtime (&t);
#else
	struct tm tmb;
	tm = ::gmtime_r (&t, &tmb);
#endif

#ifdef ASE_CHAR_IS_MCHAR
	size_t l = ::strftime (buf, ASE_COUNTOF(buf), fmt, tm);
#else
	size_t l = ::wcsftime (buf, ASE_COUNTOF(buf), fmt, tm);
#endif

	return ret.set (buf, l);	
}

int StdAwk::system (Run& run, Return& ret, const Argument* args, size_t nargs,
	const char_t* name, size_t len)
{
	size_t l;
	const char_t* ptr = args[0].toStr(&l);

#ifdef _WIN32
	return ret.set ((long_t)::_tsystem(ptr));
#elif defined(ASE_CHAR_IS_MCHAR)
	return ret.set ((long_t)::system(ptr));
#else
	char* mbs = (char*) ase_awk_alloc (awk, l*5+1);
	if (mbs == ASE_NULL) return -1;

	::size_t mbl = ::wcstombs (mbs, ptr, l*5);
	if (mbl == (::size_t)-1) 
	{
		ase_awk_free (awk, mbs);
		return -1;
	}
	mbs[mbl] = '\0';
	int n = ret.set ((long_t)::system(mbs));

	ase_awk_free (awk, mbs);
	return n;
#endif
}

int StdAwk::openPipe (Pipe& io) 
{ 
	Awk::Pipe::Mode mode = io.getMode();
	FILE* fp = NULL;

	switch (mode)
	{
		case Awk::Pipe::READ:
			fp = ase_popen (io.getName(), ASE_T("r"));
			break;
		case Awk::Pipe::WRITE:
			fp = ase_popen (io.getName(), ASE_T("w"));
			break;
	}

	if (fp == NULL) return -1;

	io.setHandle (fp);
	return 1;
}

int StdAwk::closePipe (Pipe& io) 
{
	fclose ((FILE*)io.getHandle());
	return 0; 
}

StdAwk::ssize_t StdAwk::readPipe (Pipe& io, char_t* buf, size_t len) 
{ 
	FILE* fp = (FILE*)io.getHandle();
	ssize_t n = 0;

	while (n < (ssize_t)len)
	{
		ase_cint_t c = ase_fgetc (fp);
		if (c == ASE_CHAR_EOF)
		{ 
			if (ase_ferror(fp)) n = -1;
			break;
		}

		buf[n++] = c;
		if (c == ASE_T('\n')) break;
	}

	return n;
}

StdAwk::ssize_t StdAwk::writePipe (Pipe& io, char_t* buf, size_t len) 
{ 
	FILE* fp = (FILE*)io.getHandle();
	size_t left = len;

	while (left > 0)
	{
		if (*buf == ASE_T('\0')) 
		{
		#if defined(ASE_CHAR_IS_WCHAR) && defined(__linux)
			if (fputc ('\0', fp) == EOF)
		#else
			if (ase_fputc (*buf, fp) == ASE_CHAR_EOF) 
		#endif
			{
				return -1;
			}
			left -= 1; buf += 1;
		}
		else
		{
		#if defined(ASE_CHAR_IS_WCHAR) && defined(__linux)
		// fwprintf seems to return an error with the file
		// pointer opened by popen, as of this writing. 
		// anyway, hopefully the following replacement 
		// will work all the way.
			int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;	
			int n = fprintf (fp, "%.*ls", chunk, buf);
			if (n >= 0)
			{
				size_t x;
				for (x = 0; x < chunk; x++)
				{
					if (buf[x] == ASE_T('\0')) break;
				}
				n = x;
			}
		#else
			int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;
			int n = ase_fprintf (fp, ASE_T("%.*s"), chunk, buf);
		#endif

			if (n < 0 || n > chunk) return -1;
			left -= n; buf += n;
		}
	}

	return len;
}

int StdAwk::flushPipe (Pipe& io) 
{ 
	return ::fflush ((FILE*)io.getHandle()); 
}

// file io handlers 
int StdAwk::openFile (File& io) 
{ 
	Awk::File::Mode mode = io.getMode();
	FILE* fp = NULL;

	switch (mode)
	{
		case Awk::File::READ:
			fp = ase_fopen (io.getName(), ASE_T("r"));
			break;
		case Awk::File::WRITE:
			fp = ase_fopen (io.getName(), ASE_T("w"));
			break;
		case Awk::File::APPEND:
			fp = ase_fopen (io.getName(), ASE_T("a"));
			break;
	}

	if (fp == NULL) return -1;

	io.setHandle (fp);
	return 1;
}

int StdAwk::closeFile (File& io) 
{ 
	fclose ((FILE*)io.getHandle());
	return 0; 
}

StdAwk::ssize_t StdAwk::readFile (File& io, char_t* buf, size_t len) 
{
	FILE* fp = (FILE*)io.getHandle();
	ssize_t n = 0;

	while (n < (ssize_t)len)
	{
		ase_cint_t c = ase_fgetc (fp);
		if (c == ASE_CHAR_EOF)
		{
			if (ase_ferror(fp)) n = -1;
			break;
		}

		buf[n++] = c;
		if (c == ASE_T('\n')) break;
	}

	return n;
}

StdAwk::ssize_t StdAwk::writeFile (File& io, char_t* buf, size_t len)
{
	FILE* fp = (FILE*)io.getHandle();
	size_t left = len;

	while (left > 0)
	{
		if (*buf == ASE_T('\0')) 
		{
			if (ase_fputc (*buf, fp) == ASE_CHAR_EOF) return -1;
			left -= 1; buf += 1;
		}
		else
		{
			int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;
			int n = ase_fprintf (fp, ASE_T("%.*s"), chunk, buf);
			if (n < 0 || n > chunk) return -1;
			left -= n; buf += n;
		}
	}

	return len;
}

int StdAwk::flushFile (File& io) 
{ 
	return ::fflush ((FILE*)io.getHandle()); 
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

// character handling primitive
Awk::bool_t StdAwk::isType (cint_t c, ccls_type_t type)
{
	return ase_ccls_is (c, (ase_ccls_type_t)type);
}

Awk::cint_t StdAwk::transCase (cint_t c, ccls_type_t type)
{
	return ase_ccls_to (c, (ase_ccls_type_t)type);
}

// miscellaneous primitive
StdAwk::real_t StdAwk::pow (real_t x, real_t y) 
{ 
	return ::pow (x, y); 
}

int StdAwk::vsprintf (
	char_t* buf, size_t size, const char_t* fmt, va_list arg) 
{
	return ase_vsprintf (buf, size, fmt, arg);
}

/////////////////////////////////
ASE_END_NAMESPACE(ASE)
/////////////////////////////////

