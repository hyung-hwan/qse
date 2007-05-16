/*
 * $Id: Awk.cpp,v 1.12 2007/05/14 08:40:13 bacon Exp $
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
	TestAwk (): srcInName(ASE_NULL), srcOutName(ASE_NULL), 
	            numConInFiles(0), numConOutFiles(0)
	{
	}

	~TestAwk ()
	{
		close ();
	}

	int addConsoleInput (const char_t* file)
	{
		if (numConInFiles < ASE_COUNTOF(conInFile))
		{
			conInFile[numConInFiles++] = file;
			return 0;
		}

		return -1;
	}

	int addConsoleOutput (const char_t* file)
	{
		if (numConOutFiles < ASE_COUNTOF(conOutFile))
		{
			conOutFile[numConOutFiles++] = file;
			return 0;
		}

		return -1;
	}

	int parse (const char_t* in, const char_t* out)
	{
		srcInName = in;
		srcOutName = out;
		return StdAwk::parse ();
	}

protected:

	void onRunStart (const Run& run)
	{
		wprintf (L"*** awk run started ***\n");
	}

	void onRunEnd (const Run& run, int errnum)
	{
		wprintf (L"*** awk run ended ***\n");
	}

	int openSource (Source& io)
	{
		Source::Mode mode = io.getMode();
		FILE* fp = ASE_NULL;

		if (mode == Source::READ)
		{
			if (srcInName == ASE_NULL) 
			{
				io.setHandle (stdin);
				return 0;
			}

			if (srcInName[0] == ASE_T('\0')) fp = stdin;
			else fp = ase_fopen (srcInName, ASE_T("r"));
		}
		else if (mode == Source::WRITE)
		{
			if (srcOutName == ASE_NULL)
			{
				io.setHandle (stdout);
				return 0;
			}

			if (srcOutName[0] == ASE_T('\0')) fp = stdout;
			else fp = ase_fopen (srcOutName, ASE_T("w"));
		}

		if (fp == ASE_NULL) return -1;
		io.setHandle (fp);
		return 1;
	}

	int closeSource (Source& io)
	{
		Source::Mode mode = io.getMode();
		FILE* fp = (FILE*)io.getHandle();
		if (fp != stdin && fp != stdout && fp != stderr) fclose (fp);
		io.setHandle (ASE_NULL);
		return 0;
	}

	ssize_t readSource (Source& io, char_t* buf, size_t len)
	{
		if (len <= 0) return -1;

		// TOOD: read more characters...
		cint_t c = ase_fgetc ((FILE*)io.getHandle());
		if (c == ASE_CHAR_EOF) return 0;
		buf[0] = (ase_char_t)c;
		return 1;
	}

	ssize_t writeSource (Source& io, char_t* buf, size_t len)
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
				int n = ase_fprintf (fp, ASE_T("%.*s"), left, buf);
				if (n < 0) return -1;
				left -= n; buf += n;
			}
		}

		return len;
	}

	// pipe io handlers 
	int openPipe (Pipe& io) 
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

	int closePipe (Pipe& io) 
	{
		fclose ((FILE*)io.getHandle());
		return 0; 
	}

	ssize_t readPipe  (Pipe& io, char_t* buf, size_t len) 
	{ 
		FILE* fp = (FILE*)io.getHandle();
		if (ase_fgets (buf, len, fp) == ASE_NULL)
		{
			if (ferror(fp)) return -1;
			return 0;
		}

		return ase_strlen(buf);
	}

	ssize_t writePipe (Pipe& io, char_t* buf, size_t len) 
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
			/* fwprintf seems to return an error with the file
			 * pointer opened by popen, as of this writing. 
			 * anyway, hopefully the following replacement 
			 * will work all the way. */
				int n = fprintf (fp, "%.*ls", left, buf);
				if (n >= 0)
				{
					size_t x;
					for (x = 0; x < left; x++)
					{
						if (buf[x] == ASE_T('\0')) break;
					}
					n = x;
				}
			#else
				int n = ase_fprintf (fp, ASE_T("%.*s"), left, buf);
			#endif

				if (n < 0 || n > left) return -1;
				left -= n; buf += n;
			}
		}

		return len;
	}

	int flushPipe (Pipe& io) { return ::fflush ((FILE*)io.getHandle()); }

	// file io handlers 
	int openFile (File& io) 
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

	int closeFile (File& io) 
	{ 
		fclose ((FILE*)io.getHandle());
		return 0; 
	}

	ssize_t readFile (File& io, char_t* buf, size_t len) 
	{
		FILE* fp = (FILE*)io.getHandle();
		if (ase_fgets (buf, len, fp) == ASE_NULL)
		{
			if (ferror(fp)) return -1;
			return 0;
		}

		return ase_strlen(buf);
	}

	ssize_t writeFile (File& io, char_t* buf, size_t len)
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
				int n = ase_fprintf (fp, ASE_T("%.*s"), left, buf);
				if (n < 0 || n > left) return -1;
				left -= n; buf += n;
			}
		}

		return len;
	}

	int flushFile (File& io) { return ::fflush ((FILE*)io.getHandle()); }

	// console io handlers 
	int openConsole (Console& io) 
	{ 
		Awk::Console::Mode mode = io.getMode();
		FILE* fp = ASE_NULL;
		const char_t* fn = ASE_NULL;

		switch (mode)
		{
			case Awk::Console::READ:
				if (numConInFiles == 0) fp = stdin;
				else 
				{
					fn = conInFile[0];
					fp = ase_fopen (fn, ASE_T("r"));
				}
				break;

			case Awk::Console::WRITE:
				if (numConOutFiles == 0) fp = stdout;
				else 
				{
					fn = conOutFile[0];
					fp = ase_fopen (fn, ASE_T("w"));
				}
				break;
		}

		if (fp == NULL) return -1;

		ConTrack* t = (ConTrack*) ase_awk_malloc (awk, ASE_SIZEOF(ConTrack));
		if (t == ASE_NULL)
		{
			if (fp != stdin && fp != stdout) fclose (fp);
			return -1;
		}

		t->handle = fp;
		t->nextConIdx = 1;

		if (fn != ASE_NULL) 
		{
			if (io.setFileName (fn) == -1)
			{
				if (fp != stdin && fp != stdout) fclose (fp);
				ase_awk_free (awk, t);
				return -1;
			}
		}

		io.setHandle (t);
		return 1;
	}

	int closeConsole (Console& io) 
	{ 
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;

		if (fp != stdin && fp != stdout && fp != stderr) fclose (fp);

		ase_awk_free (awk, t);
		return 0;
	}

	ssize_t readConsole (Console& io, char_t* buf, size_t len) 
	{
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;

		if (ase_fgets (buf, len, fp) == ASE_NULL)
		{
			if (ferror(fp)) return -1;
			return 0;
		}

		return ase_strlen(buf);
	}

	ssize_t writeConsole (Console& io, char_t* buf, size_t len) 
	{
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;
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
				int n = ase_fprintf (fp, ASE_T("%.*s"), left, buf);
				if (n < 0) return -1;
				left -= n; buf += n;
			}
		}

		return len;
	}

	int flushConsole (Console& io) 
	{ 
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* fp = t->handle;
		return ::fflush (fp);
	}

	int nextConsole (Console& io) 
	{ 
		Awk::Console::Mode mode = io.getMode();
		ConTrack* t = (ConTrack*)io.getHandle();
		FILE* ofp = t->handle;
		FILE* nfp = ASE_NULL;
		const char_t* fn = ASE_NULL;

		switch (mode)
		{
			case Awk::Console::READ:
				if (t->nextConIdx >= numConInFiles) return 0;
				fn = conInFile[t->nextConIdx];
				nfp = ase_fopen (fn, ASE_T("r"));
				break;

			case Awk::Console::WRITE:
				if (t->nextConIdx >= numConOutFiles) return 0;
				fn = conOutFile[t->nextConIdx];
				nfp = ase_fopen (fn, ASE_T("w"));
				break;
		}

		if (nfp == ASE_NULL) return -1;

		if (fn != ASE_NULL) 
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


	// primitive operations 
	void* allocMem   (size_t n) { return ::malloc (n); }
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
	const char_t* srcInName;
	const char_t* srcOutName;
	
	struct ConTrack
	{
		FILE* handle;
		size_t nextConIdx;
	};

	size_t        numConInFiles;
	const char_t* conInFile[128];

	size_t        numConOutFiles;
	const char_t* conOutFile[128];
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

static void print_error (const ase_char_t* msg)
{
	ase_printf (ASE_T("Error: %s\n"), msg);
}

static void print_usage (const ase_char_t* argv0)
{
	ase_printf (ASE_T("Usage: %s [-m main-function] [-si source-in-file] [-so source-out-file] [-ci console-in-file]* [-co console-out-file]* [-a argument]*\n"), argv0);
}

int awk_main (int argc, ase_char_t* argv[])
{
	TestAwk awk;
	int mode = 0;
	const ase_char_t* mainfn = NULL;
	const ase_char_t* srcin = ASE_T("");
	const ase_char_t* srcout = NULL;
	const ase_char_t* args[256];
	ase_size_t nargs = 0;
	ase_size_t nsrcins = 0;
	ase_size_t nsrcouts = 0;

	for (int i = 1; i < argc; i++)
	{
		if (mode == 0)
		{
			if (ase_strcmp(argv[i], ASE_T("-si")) == 0) mode = 1;
			else if (ase_strcmp(argv[i], ASE_T("-so")) == 0) mode = 2;
			else if (ase_strcmp(argv[i], ASE_T("-ci")) == 0) mode = 3;
			else if (ase_strcmp(argv[i], ASE_T("-co")) == 0) mode = 4;
			else if (ase_strcmp(argv[i], ASE_T("-a")) == 0) mode = 5;
			else if (ase_strcmp(argv[i], ASE_T("-m")) == 0) mode = 6;
			else 
			{
				print_usage (argv[0]);
				return -1;
			}
		}
		else
		{
			if (argv[i][0] == ASE_T('-'))
			{
				print_usage (argv[0]);
				return -1;
			}

			if (mode == 1) // source input 
			{
				if (nsrcins != 0) 
				{
					print_usage (argv[0]);
					return -1;
				}
	
				srcin = argv[i];
				nsrcins++;
				mode = 0;
			}
			else if (mode == 2) // source output 
			{
				if (nsrcouts != 0) 
				{
					print_usage (argv[0]);
					return -1;
				}
	
				srcout = argv[i];
				nsrcouts++;
				mode = 0;
			}
			else if (mode == 3) // console input
			{
				if (awk.addConsoleInput (argv[i]) == -1)
				{
					print_error (ASE_T("too many console inputs"));
					return -1;
				}

				mode = 0;
			}
			else if (mode == 4) // console output
			{
				if (awk.addConsoleOutput (argv[i]) == -1)
				{
					print_error (ASE_T("too many console outputs"));
					return -1;
				}

				mode = 0;
			}
			else if (mode == 5) // argument mode
			{
				if (nargs >= ASE_COUNTOF(args))
				{
					print_usage (argv[0]);
					return -1;
				}

				args[nargs++] = argv[i];
				mode = 0;
			}
			else if (mode == 6)
			{
				if (mainfn != NULL) 
				{
					print_usage (argv[0]);
					return -1;
				}

				mainfn = argv[i];
				mode = 0;
			}
		}
	}

	if (mode != 0)
	{
		print_usage (argv[0]);
		return -1;
	}

	if (awk.open() == -1)
	{
		ase_fprintf (stderr, ASE_T("cannot open awk\n"));
		return -1;
	}

	if (awk.parse (srcin, srcout) == -1)
	{
		ase_fprintf (stderr, ASE_T("cannot parse\n"));
		return -1;
	}

	if (awk.run (mainfn, args, nargs) == -1)
	{
		ase_fprintf (stderr, ASE_T("cannot run\n"));
		return -1;
	}

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
