/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/HeapMmgr.hpp>
#include <qse/cmn/opt.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/sio.h>
#include <cstring>

#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	include <os2.h>
#else
#	include <unistd.h>
#	include <signal.h>
#	include <errno.h>
#endif

/* these three definitions for doxygen cross-reference */
typedef QSE::StdAwk StdAwk;
typedef QSE::StdAwk::Run Run;
typedef QSE::StdAwk::Value Value;

class MyAwk: public StdAwk
{
public:
	MyAwk (QSE::Mmgr* mmgr = QSE_NULL): StdAwk (mmgr) { }
	~MyAwk () { close (); }

	int open ()
	{
		if (StdAwk::open () <= -1) return -1;

		idLastSleep = addGlobal (QSE_T("LAST_SLEEP"));
		if (idLastSleep <= -1) goto oops;

		/* this is for demonstration only. 
		 * you can use sys::sleep() instead */
		if (addFunction (QSE_T("sleep"), 1, 1, QSE_NULL,
		    	(FunctionHandler)&MyAwk::sleep) <= -1) goto oops;

		if (addFunction (QSE_T("sumintarray"), 1, 1, QSE_NULL,
		    	(FunctionHandler)&MyAwk::sumintarray) <= -1) goto oops;

		if (addFunction (QSE_T("arrayindices"), 1, 1, QSE_NULL,
		    	(FunctionHandler)&MyAwk::arrayindices) <= -1) goto oops;

		return 0;

	oops:
		StdAwk::close ();
		return -1;
	}

	int sleep (
		Run& run, Value& ret, Value* args, size_t nargs, 
		const char_t* name, size_t len)
	{
		if (args[0].isIndexed()) 
		{
			run.setError (QSE_AWK_EINVAL);
			return -1;
		}

		Awk::int_t x = args[0].toInt();

		/*Value arg;
		if (run.getGlobal(idLastSleep, arg) == 0)
			qse_printf (QSE_T("GOOD: [%d]\n"), (int)arg.toInt());
		else { qse_printf (QSE_T("BAD:\n")); }
		*/

		if (run.setGlobal (idLastSleep, x) <= -1) return -1;

	#if defined(_WIN32)
		::Sleep ((DWORD)(x * 1000));
		return ret.setInt (0);
	#elif defined(__OS2__)
		::DosSleep ((ULONG)(x * 1000));
		return ret.setInt (0);
	#else
		return ret.setInt (::sleep (x));
	#endif
	}

	int sumintarray (
		Run& run, Value& ret, Value* args, size_t nargs, 
		const char_t* name, size_t len)
	{
		// BEGIN { 
		//   for(i=0;i<=10;i++) x[i]=i; 
		//   print sumintarray(x);
		// }
		long_t x = 0;

		if (args[0].isIndexed()) 
		{
			Value val(run);
			Value::Index idx;
			Value::IndexIterator ii;

			ii = args[0].getFirstIndex (&idx);
			while (ii != ii.END)
			{
				if (args[0].getIndexed(idx, &val) <= -1) return -1;
				x += val.toInt ();

				ii = args[0].getNextIndex (&idx, ii);
			}
		}
		else x += args[0].toInt();

		return ret.setInt (x);
	}

	int arrayindices (
		Run& run, 
		Value& ret,
		Value* args,
		size_t nargs, 
		const char_t* name, 
		size_t len)
	{
		// create another array composed of array indices
		// BEGIN { 
		//   for(i=0;i<=10;i++) x[i]=i; 
		//   y=arrayindices(x); 
		//   for (i in y) print y[i]; 
		// }
		if (!args[0].isIndexed()) return 0;

		Value::Index idx;
		Value::IndexIterator ii;
		long_t i;

		ii = args[0].getFirstIndex (&idx);
		for (i = 0; ii != ii.END ; i++)
		{
			Value::IntIndex iidx (i);
			if (ret.setIndexedStr (
				iidx, idx.pointer(), idx.length()) <= -1) return -1;
			ii = args[0].getNextIndex (&idx, ii);
		}
	
		return 0;
	}
	
private:
	int idLastSleep;
};

static MyAwk* app_awk = QSE_NULL;

static void print_error (const qse_char_t* fmt, ...)
{
	va_list va;

	qse_fprintf (QSE_STDERR, QSE_T("ERROR: "));

	va_start (va, fmt);
	qse_vfprintf (QSE_STDERR, fmt, va);
	va_end (va);
}

static void print_error (MyAwk& awk)
{
	MyAwk::loc_t loc = awk.getErrorLocation();

	if (loc.file)
	{
		print_error (QSE_T("line %u at %s - %s\n"), (unsigned)loc.line, loc.file, awk.getErrorMessage());
	}
	else
	{
		print_error (QSE_T("line %u - %s\n"), (unsigned)loc.line, awk.getErrorMessage());
	}
}

#ifdef _WIN32
static BOOL WINAPI stop_run (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		if (app_awk) app_awk->stop ();
		return TRUE;
	}

	return FALSE;
}
#else

static int setsignal (int sig, void(*handler)(int), int restart)
{
	struct sigaction sa_int;

	sa_int.sa_handler = handler;
	sigemptyset (&sa_int.sa_mask);
	
	sa_int.sa_flags = 0;

	if (restart)
	{
	#ifdef SA_RESTART
		sa_int.sa_flags |= SA_RESTART;
	#endif
	}
	else
	{
	#ifdef SA_INTERRUPT
		sa_int.sa_flags |= SA_INTERRUPT;
	#endif
	}
	return sigaction (sig, &sa_int, NULL);
}

static void stop_run (int sig)
{
	int e = errno;
	if (app_awk) app_awk->stop ();
	errno = e;
}
#endif

static void set_signal (void)
{
#ifdef _WIN32
	SetConsoleCtrlHandler (stop_run, TRUE);
#else
	/*setsignal (SIGINT, stop_run, 1); TO BE MORE COMPATIBLE WITH WIN32*/
	setsignal (SIGINT, stop_run, 0);
#endif
}

static void unset_signal (void)
{
#ifdef _WIN32
	SetConsoleCtrlHandler (stop_run, FALSE);
#else
	setsignal (SIGINT, SIG_DFL, 1);
#endif
}

static void print_usage (qse_sio_t* out, const qse_char_t* argv0)
{
	qse_fprintf (out, QSE_T("USAGE: %s [options] -f sourcefile [ -- ] [datafile]*\n"), argv0);
	qse_fprintf (out, QSE_T("       %s [options] [ -- ] sourcestring [datafile]*\n"), argv0);
	qse_fprintf (out, QSE_T("Where options are:\n"));
	qse_fprintf (out, QSE_T(" -h                print this message\n"));
	qse_fprintf (out, QSE_T(" -f sourcefile     set the source script file\n"));
	qse_fprintf (out, QSE_T(" -d deparsedfile   set the deparsing output file\n"));
	qse_fprintf (out, QSE_T(" -o outputfile     set the console output file\n"));
	qse_fprintf (out, QSE_T(" -F string         set a field separator(FS)\n"));
}

struct cmdline_t
{
	qse_char_t* ins;
	qse_char_t* inf;
	qse_char_t* outf;
	qse_char_t* outc;
	qse_char_t* fs;
};

static int handle_cmdline (
	MyAwk& awk, int argc, qse_char_t* argv[], cmdline_t* cmdline)
{
	static qse_opt_t opt =
	{
		QSE_T("hF:f:d:o:"),
		QSE_NULL
	};
	qse_cint_t c;

	std::memset (cmdline, 0, QSE_SIZEOF(*cmdline));
	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			case QSE_T('h'):
				print_usage (QSE_STDOUT, argv[0]);
				return 0;

			case QSE_T('F'):
				cmdline->fs = opt.arg;
				break;

			case QSE_T('f'):
				cmdline->inf = opt.arg;
				break;

			case QSE_T('d'):
				cmdline->outf = opt.arg;
				break;

			case QSE_T('o'):
				cmdline->outc = opt.arg;
				break;

			case QSE_T('?'):
				print_error (QSE_T("illegal option - '%c'\n"), opt.opt);
				return -1;

			case QSE_T(':'):
				print_error (QSE_T("bad argument for '%c'\n"), opt.opt);
				return -1;

			default:
				print_usage (QSE_STDERR, argv[0]);
				return -1;
		}
	}

	if (opt.ind < argc && !cmdline->inf)
		cmdline->ins = argv[opt.ind++];

	while (opt.ind < argc)
	{
		if (awk.addArgument (argv[opt.ind++]) <= -1) 
		{
			print_error (awk);
			return -1;
		}
	}

	if (!cmdline->ins && !cmdline->inf)
	{
		print_usage (QSE_STDERR, argv[0]);
		return -1;
	}

	return 1;
}


static int awk_main_2 (MyAwk& awk, int argc, qse_char_t* argv[])
{
	MyAwk::Run* run;
	cmdline_t cmdline;
	int n;

	awk.setTrait (awk.getTrait() | QSE_AWK_FLEXMAP | QSE_AWK_RWPIPE | QSE_AWK_NEXTOFILE);

	// ARGV[0]
	if (awk.addArgument (QSE_T("awk25")) <= -1)
	{
		print_error (awk); 
		return -1; 
	}

	if ((n = handle_cmdline (awk, argc, argv, &cmdline)) <= 0) return n;

	MyAwk::Source* in, * out;
	MyAwk::SourceString in_str (cmdline.ins);
	MyAwk::SourceFile in_file (cmdline.inf); 
	MyAwk::SourceFile out_file (cmdline.outf);

	in = (cmdline.ins)? (MyAwk::Source*)&in_str: (MyAwk::Source*)&in_file;
	out = (cmdline.outf)? (MyAwk::Source*)&out_file: &MyAwk::Source::NONE;
	run = awk.parse (*in, *out);
	if (run == QSE_NULL) 
	{
		print_error (awk); 
		return -1; 
	}

	if (cmdline.fs)
	{
		MyAwk::Value fs (run);
		if (fs.setStr (cmdline.fs) <= -1) 
		{
			print_error (awk); 
			return -1; 
		}
		if (awk.setGlobal (QSE_AWK_GBL_FS, fs) <= -1) 
		{
			print_error (awk); 
			return -1; 
		}
	}

	if (cmdline.outc) 
	{
		if (awk.addConsoleOutput (cmdline.outc) <= -1)
		{
			print_error (awk); 
			return -1; 
		}
	}

	MyAwk::Value ret;
	if (awk.loop (&ret) <= -1) 
	{ 
		print_error (awk); 
		return -1; 
	}

	return 0;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	//QSE::HeapMmgr hm (1000000);
	//MyAwk awk (&hm);
	MyAwk awk;

	if (awk.open() <= -1)
	{
		print_error (awk);
		return -1;
	}
	app_awk = &awk;

	set_signal ();
	int n = awk_main_2 (awk, argc, argv);
	unset_signal ();

	app_awk = QSE_NULL;
	awk.close ();
	return n;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int ret;

	qse_openstdsios ();

	{
	#if defined(_WIN32)
		char locale[100];
		UINT codepage;
		WSADATA wsadata;
	
		codepage = GetConsoleOutputCP();	
		if (codepage == CP_UTF8)
		{
			/*SetConsoleOUtputCP (CP_UTF8);*/
			qse_setdflcmgrbyid (QSE_CMGR_UTF8);
		}
		else
		{
			sprintf (locale, ".%u", (unsigned int)codepage);
			setlocale (LC_ALL, locale);
			qse_setdflcmgrbyid (QSE_CMGR_SLMB);
		}
	
		if (WSAStartup (MAKEWORD(2,0), &wsadata) != 0)
		{
			print_error (QSE_T("Failed to start up winsock\n"));
			return -1;
		}
	
	#else
		setlocale (LC_ALL, "");
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	#endif
	}

	ret = qse_runmain (argc, argv, awk_main);

#if defined(_WIN32)
	WSACleanup ();
#endif

	qse_closestdsios ();
	return ret;
}
