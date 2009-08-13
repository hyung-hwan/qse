/*
 * $Id: Awk.cpp 341 2008-08-20 10:58:19Z baconevi $
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
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/mem.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(_WIN32)
#	include <windows.h>
#else
#	include <unistd.h>
#	include <signal.h>
#	include <errno.h>
#endif

class MyAwk;
#ifdef _WIN32
static BOOL WINAPI stop_run (DWORD ctrl_type);
#else
static void stop_run (int sig);
#endif

static void set_intr_run (void);
static void unset_intr_run (void);

MyAwk* app_awk = QSE_NULL;

static void print_error (const qse_char_t* fmt, ...)
{
	va_list va;

	qse_fprintf (QSE_STDERR, QSE_T("ERROR: "));

	va_start (va, fmt);
	qse_vfprintf (QSE_STDERR, fmt, va);
	va_end (va);
}

class MyAwk: public QSE::StdAwk
{
public:
	MyAwk () { }
	~MyAwk () { close (); }

	int open ()
	{
		if (StdAwk::open () <= -1) return -1;

		idLastSleep = addGlobal (QSE_T("LAST_SLEEP"));
		if (idLastSleep <= -1) goto failure;

		if (addFunction (QSE_T("sleep"), 1, 1,
		    	(FunctionHandler)&MyAwk::sleep) <= -1) goto failure;

		if (addFunction (QSE_T("sumintarray"), 1, 1,
		    	(FunctionHandler)&MyAwk::sumintarray) <= -1) goto failure;

		if (addFunction (QSE_T("arrayindices"), 1, 1,
		    	(FunctionHandler)&MyAwk::arrayindices) <= -1) goto failure;
		return 0;

	failure:
		StdAwk::close ();
		return -1;
	}

	int sleep (Run& run, Value& ret, const Value* args, size_t nargs, 
		const char_t* name, size_t len)
	{
		if (args[0].isIndexed()) 
		{
			run.setError (ERR_INVAL);
			return -1;
		}

		long_t x = args[0].toInt();

		/*Value arg;
		if (run.getGlobal(idLastSleep, arg) == 0)
			qse_printf (QSE_T("GOOD: [%d]\n"), (int)arg.toInt());
		else { qse_printf (QSE_T("BAD:\n")); }
		*/

		if (run.setGlobal (idLastSleep, x) <= -1) return -1;

	#ifdef _WIN32
		::Sleep ((DWORD)(x * 1000));
		return ret.setInt (0);
	#else
		return ret.setInt (::sleep (x));
	#endif
	}

	int sumintarray (Run& run, Value& ret, const Value* args, size_t nargs, 
		const char_t* name, size_t len)
	{
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

	int arrayindices (Run& run, Value& ret, const Value* args, size_t nargs, 
		const char_t* name, size_t len)
	{
		if (!args[0].isIndexed()) return 0;

		Value::Index idx;
		Value::IndexIterator ii;
		long_t i;

		ii = args[0].getFirstIndex (&idx);
		for (i = 0; ii != ii.END ; i++)
		{
			Value::IntIndex iidx (i);
			if (ret.setIndexedStr (
				iidx, idx.ptr, idx.len) <= -1) return -1;
			ii = args[0].getNextIndex (&idx, ii);
		}
	
		return 0;
	}
	
protected:

#if 0
	bool onLoopEnter (Run& run)
	{
		set_intr_run ();
		return true;
	}

	void onLoopExit (Run& run, const Value& ret)
	{
		unset_intr_run ();

		if (verbose)
		{
			size_t len;
			const char_t* ptr = ret.toStr (&len);
			qse_printf (QSE_T("*** return [%.*s] ***\n"), (int)len, ptr);
		}
	}
#endif
	
private:
	int idLastSleep;
};

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

static void set_intr_run (void)
{
#ifdef _WIN32
	SetConsoleCtrlHandler (stop_run, TRUE);
#else
	/*setsignal (SIGINT, stop_run, 1); TO BE MORE COMPATIBLE WITH WIN32*/
	setsignal (SIGINT, stop_run, 0);
#endif
}

static void unset_intr_run (void)
{
#ifdef _WIN32
	SetConsoleCtrlHandler (stop_run, FALSE);
#else
	setsignal (SIGINT, SIG_DFL, 1);
#endif
}

static void print_usage (QSE_FILE* out, const qse_char_t* argv0)
{
	qse_fprintf (out, QSE_T("USAGE: %s [options] -f sourcefile [ -- ] [datafile]*\n"), argv0);
	qse_fprintf (out, QSE_T("       %s [options] [ -- ] sourcestring [datafile]*\n"), argv0);
	qse_fprintf (out, QSE_T("Where options are:\n"));
	qse_fprintf (out, QSE_T(" -h                print this message\n"));
	qse_fprintf (out, QSE_T(" -f sourcefile     set the source script file\n"));
	qse_fprintf (out, QSE_T(" -o deparsedfile   set the deparsing output file\n"));
	qse_fprintf (out, QSE_T(" -F string         set a field separator(FS)\n"));
}

struct cmdline_t
{
	qse_char_t* ins;
	qse_char_t* inf;
	qse_char_t* outf;
	qse_char_t* fs;
};

static int handle_cmdline (MyAwk& awk, int argc, qse_char_t* argv[], cmdline_t* cmdline)
{
	static qse_opt_t opt =
	{
		QSE_T("hF:f:o:"),
		QSE_NULL
	};
	qse_cint_t c;

	memset (cmdline, 0, QSE_SIZEOF(*cmdline));
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

			case QSE_T('o'):
				cmdline->outf = opt.arg;
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
		if (awk.addArgument (argv[opt.ind++]) <= -1) return -1;
	}

	return 1;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	MyAwk awk;
	MyAwk::Run* run;
	cmdline_t cmdline;
	int n;

	if (awk.open() <= -1)
	{
		print_error (QSE_T("%s\n"), awk.getErrorMessage());
		return -1;
	}

	awk.setOption (awk.getOption() | awk.OPT_INCLUDE);

	// ARGV[0]
	if (awk.addArgument (QSE_T("awk08")) <= -1)
	{
		print_error (QSE_T("%s\n"), awk.getErrorMessage());
		awk.close ();
		return -1;
	}

	if ((n = handle_cmdline (awk, argc, argv, &cmdline)) <= 0)
	{
		awk.close ();
		return n;
	}


	MyAwk::Source* in, * out;
	MyAwk::SourceString in_str (cmdline.ins);
	MyAwk::SourceFile in_file (cmdline.inf); 
	MyAwk::SourceFile out_file (cmdline.outf);

	in = (cmdline.ins)? (MyAwk::Source*)&in_str: (MyAwk::Source*)&in_file;
	out = (cmdline.outf)? (MyAwk::Source*)&out_file: &MyAwk::Source::NONE;
	run = awk.parse (*in, *out);
	if (run == QSE_NULL)
	{
		print_error (
			QSE_T("ERROR: LINE[%d] %s\n"), 
			awk.getErrorLine(), awk.getErrorMessage());
		awk.close ();
		return -1;
	}

	if (cmdline.fs)
	{
// TODO: print error.... handle error properly
		MyAwk::Value fs (run);
		if (fs.setStr (cmdline.fs) <= -1) return -1;
		if (awk.setGlobal (awk.GBL_FS, fs) <= -1) return -1;
	}

	app_awk = &awk;

	MyAwk::Value ret;
	if (awk.loop (&ret) <= -1)
	{
		print_error (
			QSE_T("ERROR: LINE[%d] %s\n"), 
			awk.getErrorLine(), awk.getErrorMessage());
		awk.close ();
		return -1;
	}

	app_awk = QSE_NULL;
	awk.close ();

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc,argv,awk_main);
}
