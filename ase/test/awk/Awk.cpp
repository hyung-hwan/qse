/*
 * $Id: Awk.cpp,v 1.6 2007/05/07 09:30:28 bacon Exp $
 */

#include <ase/awk/StdAwk.hpp>
#include <ase/cmn/str.h>

#include <ase/utl/ctype.h>
#include <ase/utl/stdio.h>
#include <ase/utl/main.h>

#include <stdlib.h>
#include <math.h>

#if defined(_WIN32)
#include <windows.h>
#endif

class TestAwk: public ASE::StdAwk
{
public:
	~TestAwk ()
	{
		close ();
	}

	int parse (const char_t* name)
	{
		ase_strxcpy (sourceInName, ASE_COUNTOF(sourceInName), name);
		return StdAwk::parse ();
	}

protected:
	int openSource (Source& io)
	{
		Source::Mode mode = io.getMode();

		if (mode == Source::READ)
		{
			FILE* fp = ase_fopen (sourceInName, ASE_T("r"));
			if (fp == ASE_NULL) return -1;
			io.setHandle (fp);
			return 1;
		}
		else if (mode == Source::WRITE)
		{
			return 0;
		}

		return -1;
	}

	int closeSource (Source& io)
	{
		Source::Mode mode = io.getMode();

		if (mode == Source::READ)
		{
			fclose ((FILE*)io.getHandle());
			io.setHandle (ASE_NULL);
			return 0;
		}
		else if (mode == Source::WRITE)
		{
			return 0;
		}

		return -1;
	}

	ssize_t readSource (Source& io, char_t* buf, size_t count)
	{
		if (count <= 0) return -1;

		// TOOD: read more characters...
		cint_t c = ase_fgetc ((FILE*)io.getHandle());
		if (c == ASE_CHAR_EOF) return 0;
		buf[0] = (ase_char_t)c;
		return 1;
	}

	ssize_t writeSource (Source& io, char_t* buf, size_t count)
	{
		return 0;
	}


	// pipe io handlers 
	int     openPipe  (Pipe& io) { return -1; }
	int     closePipe (Pipe& io) { return 0; }
	ssize_t readPipe  (Pipe& io, char_t* buf, size_t len) { return 0; }
	ssize_t writePipe (Pipe& io, char_t* buf, size_t len) { return 0; }
	int     flushPipe (Pipe& io) { return 0; }
	int     nextPipe  (Pipe& io) { return 0; }

	// file io handlers 
	int     openFile  (File& io) { return -1; }
	int     closeFile (File& io) { return 0; }
	ssize_t readFile  (File& io, char_t* buf, size_t len) { return 0; }
	ssize_t writeFile (File& io, char_t* buf, size_t len) { return 0; }
	int     flushFile (File& io) { return 0; }
	int     nextFile  (File& io) { return 0; }

	// console io handlers 
	int openConsole  (Console& io) { return 1; }
	int closeConsole (Console& io) { return 0; }
	ssize_t readConsole  (Console& io, char_t* buf, size_t len) 
	{
		return 0;
	}
	ssize_t writeConsole (Console& io, char_t* buf, size_t len) 
	{
		return ase_printf (ASE_T("%.*s"), len, buf);
	}
	int flushConsole (Console& io) { return 0; }
	int nextConsole  (Console& io) { return 0; }

	// primitive operations 
	void* allocMem  (size_t n) { return ::malloc (n); }
	void* reallocMem (void* ptr, size_t n) { return ::realloc (ptr, n); }
	void  freeMem    (void* ptr) { ::free (ptr); }

	bool_t isUpper  (cint_t c) { return ase_isupper (c); }
	bool_t isLower  (cint_t c) { return ase_islower (c); }
	bool_t isAlpha  (cint_t c) { return ase_isalpha (c); }
	bool_t isDigit  (cint_t c) { return ase_isdigit (c); }
	bool_t isXdigit (cint_t c) { return ase_isxdigit (c); }
	bool_t isAlnum  (cint_t c) { return ase_isalnum (c); }
	bool_t isSpace  (cint_t c) { return ase_isspace (c); }
	bool_t isPrint  (cint_t c) { return ase_isprint (c); }
	bool_t isGraph  (cint_t c) { return ase_isgraph (c); }
	bool_t isCntrl  (cint_t c) { return ase_iscntrl (c); }
	bool_t isPunct  (cint_t c) { return ase_ispunct (c); }
	cint_t toUpper  (cint_t c) { return ase_toupper (c); }
	cint_t toLower  (cint_t c) { return ase_tolower (c); }

	real_t pow (real_t x, real_t y) 
	{ 
		return ::pow (x, y); 
	}

	int vsprintf (char_t* buf, size_t size, const char_t* fmt, va_list arg) 
	{
		return ase_vsprintf (buf, size, fmt, arg);
	}

	void vdprintf (const char_t* fmt, va_list arg) 
	{
		ase_vfprintf (stderr, fmt, arg);
	}

private:
	char_t sourceInName[1024+1];
};

#ifndef NDEBUG
void ase_assert_abort (void)
{
	abort ();
}

void ase_assert_printf (const ase_char_t* fmt, ...)
{
	va_list ap;
#ifdef _WIN32
	int n;
	ase_char_t buf[1024];
#endif

	va_start (ap, fmt);
#if defined(_WIN32)
	n = _vsntprintf (buf, ASE_COUNTOF(buf), fmt, ap);
	if (n < 0) buf[ASE_COUNTOF(buf)-1] = ASE_T('\0');

	#if defined(_MSC_VER) && (_MSC_VER<1400)
	MessageBox (NULL, buf, 
		ASE_T("Assertion Failure"), MB_OK|MB_ICONERROR);
	#else
	MessageBox (NULL, buf, 
		ASE_T("\uB2DD\uAE30\uB9AC \uC870\uB610"), MB_OK|MB_ICONERROR);
	#endif
#else
	ase_vprintf (fmt, ap);
#endif
	va_end (ap);
}
#endif

int awk_main (int argc, ase_char_t* argv[])
{
	TestAwk awk;

	if (awk.open() == -1)
	{
		ase_fprintf (stderr, ASE_T("cannot open awk\n"));
		//awk.close ();
		return -1;
	}

	if (awk.parse(ASE_T("t.awk")) == -1)
	{
		ase_fprintf (stderr, ASE_T("cannot parse\n"));
		//awk.close ();
		return -1;
	}

	if (awk.run () == -1)
	{
		ase_fprintf (stderr, ASE_T("cannot run\n"));
		//awk.close ();
		return -1;
	}

//	awk.close ();
	return 0;
}

extern "C" int ase_main (int argc, ase_char_t* argv[])
{
	int n;

#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif

	n = awk_main (argc, argv);

#if defined(__linux) && defined(_DEBUG)
	muntrace ();
#endif
#if defined(_WIN32) && defined(_DEBUG)
	#if defined(_MSC_VER)
	_CrtDumpMemoryLeaks ();
	#endif
	_tprintf (_T("Press ENTER to quit\n"));
	getchar ();
#endif

	return n;
}
