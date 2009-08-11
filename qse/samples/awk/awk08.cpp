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

#include <stdlib.h>
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
static bool verbose = false;

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

static void print_error (const qse_char_t* msg)
{
	qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), msg);
}


struct opttab_t
{
	const qse_char_t* name;
	int opt;
	const qse_char_t* desc;
} opttab[] =
{
	{ QSE_T("implicit"),    MyAwk::OPT_IMPLICIT,       QSE_T("allow undeclared variables") },
	{ QSE_T("explicit"),    MyAwk::OPT_EXPLICIT,       QSE_T("allow declared variables(local,global)") },
	{ QSE_T("extraops"),    MyAwk::OPT_EXTRAOPS,       QSE_T("enable extra operators(<<,>>,^^,//)") },
	{ QSE_T("rio"),         MyAwk::OPT_RIO,            QSE_T("enable builtin I/O including getline & print") },
	{ QSE_T("rwpipe"),      MyAwk::OPT_RWPIPE,         QSE_T("allow a dual-directional pipe") },
	{ QSE_T("newline"),     MyAwk::OPT_NEWLINE,        QSE_T("enable a newline to terminate a statement") },
	{ QSE_T("striprecspc"), MyAwk::OPT_STRIPRECSPC,    QSE_T("strip spaces in splitting a record") },
	{ QSE_T("stripstrspc"), MyAwk::OPT_STRIPSTRSPC,    QSE_T("strip spaces in converting a string to a number") },
	{ QSE_T("nextofile"),   MyAwk::OPT_NEXTOFILE,      QSE_T("enable 'nextofile'") },
	{ QSE_T("reset"),       MyAwk::OPT_RESET,          QSE_T("enable 'reset'") },
	{ QSE_T("crlf"),        MyAwk::OPT_CRLF,           QSE_T("use CRLF for a newline") },
	{ QSE_T("maptovar"),    MyAwk::OPT_MAPTOVAR,       QSE_T("allow a map to be assigned or returned") },
	{ QSE_T("pablock"),     MyAwk::OPT_PABLOCK,        QSE_T("enable pattern-action loop") },
	{ QSE_T("rexbound"),    MyAwk::OPT_REXBOUND,       QSE_T("enable {n,m} in a regular expression") },
	{ QSE_T("ncmponstr"),   MyAwk::OPT_NCMPONSTR,      QSE_T("perform numeric comparsion on numeric strings") },
	{ QSE_T("strictnaming"), MyAwk::OPT_STRICTNAMING,  QSE_T("enable the strict naming rule") },
	{ QSE_T("include"),     MyAwk::OPT_INCLUDE,        QSE_T("enable 'include'") },
	{ QSE_NULL,             0 }
};

static void print_usage (const qse_char_t* argv0)
{
	const qse_char_t* base;
	int j;
	
	base = qse_strrchr(argv0, QSE_T('/'));
	if (base == QSE_NULL) base = qse_strrchr(argv0, QSE_T('\\'));
	if (base == QSE_NULL) base = argv0; else base++;

	qse_printf (QSE_T("Usage: %s [-si file]? [-so file]? [-ci file]* [-co file]* [-w o:n]* \n"), base);
	qse_printf (QSE_T("    -si file  Specify the input source file\n"));
	qse_printf (QSE_T("              The source code is read from stdin when it is not specified\n"));
	qse_printf (QSE_T("    -so file  Specify the output source file\n"));
	qse_printf (QSE_T("              The deparsed code is not output when is it not specified\n"));
	qse_printf (QSE_T("    -ci file  Specify the input console file\n"));
	qse_printf (QSE_T("    -co file  Specify the output console file\n"));
	qse_printf (QSE_T("    -w  o:n   Specify an old and new word pair\n"));
	qse_printf (QSE_T("              o - an original word\n"));
	qse_printf (QSE_T("              n - the new word to replace the original\n"));
	qse_printf (QSE_T("    -v        Print extra messages\n"));


	qse_printf (QSE_T("\nYou may specify the following options to change the behavior of the interpreter.\n"));
	for (j = 0; j < (int)QSE_COUNTOF(opttab); j++)
	{
		qse_printf (QSE_T("    -%-20s -no%-20s\n"), opttab[j].name, opttab[j].name);
	}
}

static int awk_main (int argc, qse_char_t* argv[])
{
	MyAwk awk;
	MyAwk::Run* run;

	int mode = 0;
	const qse_char_t* srcin = QSE_T("");
	const qse_char_t* srcout = NULL;
	qse_size_t nsrcins = 0;
	qse_size_t nsrcouts = 0;

	if (awk.open() <= -1)
	{
		print_error (awk.getErrorMessage());
		return -1;
	}

	// ARGV[0]
	if (awk.addArgument (QSE_T("awk05")) <= -1)
	{
		print_error (awk.getErrorMessage());
		awk.close ();
		return -1;
	}

	for (int i = 1; i < argc; i++)
	{
		if (mode == 0)
		{
			if (qse_strcmp(argv[i], QSE_T("-si")) == 0) mode = 1;
			else if (qse_strcmp(argv[i], QSE_T("-so")) == 0) mode = 2;
			else if (qse_strcmp(argv[i], QSE_T("-ci")) == 0) mode = 3;
			else if (qse_strcmp(argv[i], QSE_T("-co")) == 0) mode = 4;
			else if (qse_strcmp(argv[i], QSE_T("-w")) == 0) mode = 5;
			else if (qse_strcmp(argv[i], QSE_T("-v")) == 0)
			{
				verbose = true;
			}
			else 
			{
				if (argv[i][0] == QSE_T('-'))
				{
					int j;

					if (argv[i][1] == QSE_T('n') && argv[i][2] == QSE_T('o'))
					{
						for (j = 0; j < (int)QSE_COUNTOF(opttab); j++)
						{
							if (qse_strcmp(&argv[i][3], opttab[j].name) == 0)
							{
								awk.setOption (awk.getOption() & ~opttab[j].opt);
								goto ok_valid;
							}
						}
					}
					else
					{
						for (j = 0; j < (int)QSE_COUNTOF(opttab); j++)
						{
							if (qse_strcmp(&argv[i][1], opttab[j].name) == 0)
							{
								awk.setOption (awk.getOption() | opttab[j].opt);
								goto ok_valid;
							}
						}
					}
				}

				print_usage (argv[0]);
				return -1;

			ok_valid:
				;
			}
		}
		else
		{
			if (argv[i][0] == QSE_T('-'))
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
				if (awk.addArgument (argv[i]) <= -1)
				{
					print_error (QSE_T("too many console inputs"));
					return -1;
				}

				mode = 0;
			}
			else if (mode == 4) // console output
			{
				if (awk.addConsoleOutput (argv[i]) <= -1)
				{
					print_error (QSE_T("too many console outputs"));
					return -1;
				}

				mode = 0;
			}
			else if (mode == 5) // word replacement
			{
				const qse_char_t* p;
				qse_size_t l;
				qse_cstr_t ow, nw;

				p = qse_strchr(argv[i], QSE_T(':'));
				if (p == QSE_NULL)
				{
					print_usage (argv[0]);
					return -1;
				}

				l = qse_strlen (argv[i]);

				ow.ptr = argv[i];
				ow.len = p - argv[i];

				nw.ptr = p + 1;
				nw.len = l - (ow.len + 1);

				awk.setWord (&ow, &nw);
				mode = 0;
			}
		}
	}

	if (mode != 0)
	{
		print_usage (argv[0]);
		awk.close ();
		return -1;
	}

	MyAwk::SourceFile sin (srcin); 
	MyAwk::SourceFile sout (srcout);

	run = awk.parse (sin, sout);
	if (run == QSE_NULL)
	{
		qse_fprintf (stderr, QSE_T("cannot parse: LINE[%d] %s\n"), 
			awk.getErrorLine(), awk.getErrorMessage());
		awk.close ();
		return -1;
	}

	app_awk = &awk;

	MyAwk::Value ret;
	if (awk.loop (&ret) <= -1)
	{
		qse_fprintf (stderr, QSE_T("cannot run: LINE[%d] %s\n"), 
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
