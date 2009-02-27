/*
 * $Id: awk.c 86 2009-02-26 12:55:05Z hyunghwan.chung $
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

#include <qse/awk/awk.h>
#include <qse/awk/std.h>
#include <qse/cmn/sll.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/opt.h>

#include <qse/utl/stdio.h>
#include <qse/utl/main.h>

#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>

#define ABORT(label) goto label

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	include <process.h>
#	if defined(_MSC_VER) && defined(_DEBUG)
#		define _CRTDBG_MAP_ALLOC
#		include <crtdbg.h>
#	endif
#else
#	include <unistd.h>
#	include <errno.h>
#endif

static qse_awk_rtx_t* app_rtx = NULL;
static int app_debug = 0;

struct argout_t
{
	qse_awk_parsestd_type_t ist;  /* input source type */
	union
	{
		const qse_char_t* str;
		qse_char_t**      files;
	} isp;
	qse_size_t   isfl; /* the number of input source files */

	qse_awk_parsestd_type_t ost;  /* output source type */
	qse_char_t*  osf;  /* output source file */


	qse_char_t** icf;  /* input console files */
	qse_size_t   icfl; /* the number of input console files */
	qse_map_t*   vm;   /* global variable map */
	qse_char_t*  fs;   /* field separator */
};

static void dprint (const qse_char_t* fmt, ...)
{
	if (app_debug)
	{
		va_list ap;
		va_start (ap, fmt);
		qse_vfprintf (stderr, fmt, ap);
		va_end (ap);
	}
}


#ifdef _WIN32
static BOOL WINAPI stop_run (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		qse_awk_rtx_stop (app_rtx);
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
	qse_awk_rtx_stop (app_rtx);
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

static qse_map_walk_t print_awk_value (
	qse_map_t* map, qse_map_pair_t* pair, void* arg)
{
	qse_awk_rtx_t* run = (qse_awk_rtx_t*)arg;
	qse_char_t* str;
	qse_size_t len;

	str = qse_awk_rtx_valtostr (run, QSE_MAP_VPTR(pair), 0, QSE_NULL, &len);
	if (str == QSE_NULL)
	{
		dprint (QSE_T("***OUT OF MEMORY***\n"));
	}
	else
	{
		dprint (QSE_T("%.*s = %.*s\n"), 
			(int)QSE_MAP_KLEN(pair), QSE_MAP_KPTR(pair), 
			(int)len, str);
		qse_awk_free (qse_awk_rtx_getawk(run), str);
	}

	return QSE_MAP_WALK_FORWARD;
}

static void on_run_statement (
	qse_awk_rtx_t* run, qse_size_t line, void* data)
{
	/*dprint (L"running %d\n", (int)line);*/
}

static int on_run_enter (qse_awk_rtx_t* rtx, void* data)
{
	struct argout_t* ao = (struct argout_t*)data;

	if (ao->fs != QSE_NULL)
	{
		qse_awk_val_t* fs;

		/* compose a string value to use to set FS to */
		fs = qse_awk_rtx_makestrval0 (rtx, ao->fs);
		if (fs == QSE_NULL) return -1;

		/* change FS according to the command line argument */
		qse_awk_rtx_refupval (rtx, fs);
		qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_FS, fs);
		qse_awk_rtx_refdownval (rtx, fs);
	}

	return 0;
}

static void on_run_exit (
	qse_awk_rtx_t* rtx, qse_awk_val_t* ret, void* data)
{
	qse_size_t len;
	qse_char_t* str;

	if (ret == qse_awk_val_nil)
	{
		dprint (QSE_T("[RETURN] - ***nil***\n"));
	}
	else
	{
		str = qse_awk_rtx_valtostr (rtx, ret, 0, QSE_NULL, &len);
		if (str == QSE_NULL)
		{
			dprint (QSE_T("[RETURN] - ***OUT OF MEMORY***\n"));
		}
		else
		{
			dprint (QSE_T("[RETURN] - [%.*s]\n"), (int)len, str);
			qse_awk_free (qse_awk_rtx_getawk(rtx), str);
		}
	}

	dprint (QSE_T("[NAMED VARIABLES]\n"));
	qse_map_walk (qse_awk_rtx_getnvmap(rtx), print_awk_value, rtx);
	dprint (QSE_T("[END NAMED VARIABLES]\n"));
}

/* TODO: remove otab... */
static struct
{
	const qse_char_t* name;
	int opt;
} otab[] =
{
	{ QSE_T("implicit"),    QSE_AWK_IMPLICIT },
	{ QSE_T("explicit"),    QSE_AWK_EXPLICIT },
	{ QSE_T("bxor"),        QSE_AWK_BXOR },
	{ QSE_T("shift"),       QSE_AWK_SHIFT },
	{ QSE_T("idiv"),        QSE_AWK_IDIV },
	{ QSE_T("rio"),         QSE_AWK_RIO },
	{ QSE_T("rwpipe"),      QSE_AWK_RWPIPE },
	{ QSE_T("newline"),     QSE_AWK_NEWLINE },
	{ QSE_T("stripspaces"), QSE_AWK_STRIPSPACES },
	{ QSE_T("nextofile"),   QSE_AWK_NEXTOFILE },
	{ QSE_T("crfl"),        QSE_AWK_CRLF },
	{ QSE_T("reset"),       QSE_AWK_RESET },
	{ QSE_T("maptovar"),    QSE_AWK_MAPTOVAR },
	{ QSE_T("pablock"),     QSE_AWK_PABLOCK }
};

static void print_usage (const qse_char_t* argv0)
{
	int j;

	qse_printf (QSE_T("Usage: %s [options] -f sourcefile [ -- ] [datafile]*\n"), argv0);
	qse_printf (QSE_T("       %s [options] [ -- ] sourcestring [datafile]*\n"), argv0);
	qse_printf (QSE_T("Where options are:\n"));
	qse_printf (QSE_T(" -h                                print this message\n"));
	qse_printf (QSE_T(" -d                                show extra information\n"));
	qse_printf (QSE_T(" -f/--file            sourcefile   set the source script file\n"));
	qse_printf (QSE_T(" -o/--deparsed-file   deparsedfile set the deparsing output file\n"));
	qse_printf (QSE_T(" -F/--field-separator string       set a field separator(FS)\n"));

	qse_printf (QSE_T("\nYou may specify the following options to change the behavior of the interpreter.\n"));
	for (j = 0; j < QSE_COUNTOF(otab); j++)
	{
		qse_printf (QSE_T("    -%-20s -no%-20s\n"), otab[j].name, otab[j].name);
	}
}

static int fnc_sleep (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (run, 0);

	n = qse_awk_rtx_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (qse_long_t)rv;

#ifdef _WIN32
	Sleep ((DWORD)(lv * 1000));
	n = 0;
#else
	n = sleep (lv);	
#endif

	r = qse_awk_rtx_makeintval (run, n);
	if (r == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static void out_of_memory (void)
{
	qse_fprintf (QSE_STDERR, QSE_T("Error: out of memory\n"));	
}

static int handle_args (int argc, qse_char_t* argv[], struct argout_t* ao)
{
	static qse_opt_lng_t lng[] = 
	{
		{ QSE_T("implicit"),         0 },
		{ QSE_T("explicit"),         0 },
		{ QSE_T("bxor"),             0 },
		{ QSE_T("shift"),            0 },
		{ QSE_T("idiv"),             0 },
		{ QSE_T("extio"),            0 },
		{ QSE_T("newline"),          0 },
		{ QSE_T("baseone"),          0 },
		{ QSE_T("stripspaces"),      0 },
		{ QSE_T("nextofile"),        0 },
		{ QSE_T("crlf"),             0 },
		{ QSE_T("argstomain"),       0 },
		{ QSE_T("reset"),            0 },
		{ QSE_T("maptovar"),         0 },
		{ QSE_T("pablock"),          0 },

		{ QSE_T(":main"),            QSE_T('m') },
		{ QSE_T(":file"),            QSE_T('f') },
		{ QSE_T(":field-separator"), QSE_T('F') },
		{ QSE_T(":deparsed-file"),   QSE_T('o') },
		{ QSE_T(":assign"),          QSE_T('v') },

		{ QSE_T("help"),             QSE_T('h') }
	};

	static qse_opt_t opt = 
	{
		QSE_T("hdm:f:F:o:v:"),
		lng
	};

	qse_cint_t c;

	qse_size_t isfc = 16; /* the capacity of isf */
	qse_size_t isfl = 0; /* number of input source files */

	qse_size_t icfc = 0; /* the capacity of icf */
	qse_size_t icfl = 0;  /* the number of input console files */

	qse_char_t** isf = QSE_NULL; /* input source files */
	qse_char_t*  osf = QSE_NULL; /* output source file */
	qse_char_t** icf = QSE_NULL; /* input console files */

	qse_map_t* vm = QSE_NULL;  /* global variable map */
	qse_char_t* fs = QSE_NULL; /* field separator */

	memset (ao, 0, QSE_SIZEOF(*ao));

	isf = (qse_char_t**) malloc (QSE_SIZEOF(*isf) * isfc);
	if (isf == QSE_NULL)
	{
		out_of_memory ();
		ABORT (oops);
	}

	vm = qse_map_open (QSE_NULL, 0, 30, 70); 
	if (vm == QSE_NULL)
	{
		out_of_memory ();
		ABORT (oops);
	}
	qse_map_setcopier (vm, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setcopier (vm, QSE_MAP_VAL, QSE_MAP_COPIER_INLINE);
	qse_map_setscale (vm, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));
	qse_map_setscale (vm, QSE_MAP_VAL, QSE_SIZEOF(qse_char_t));

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			case 0:
				qse_printf (QSE_T(">>> [%s] [%s]\n"), opt.lngopt, opt.arg);
				break;

			case QSE_T('h'):
				print_usage (argv[0]);
				if (isf != QSE_NULL) free (isf);
				if (vm != QSE_NULL) qse_map_close (vm);
				return 1;

			case QSE_T('d'):
			{
				app_debug = 1;
				break;
			}

			case QSE_T('f'):
			{
				if (isfl >= isfc-1) /* -1 for last QSE_NULL */
				{
					qse_char_t** tmp;
					tmp = (qse_char_t**) realloc (isf, QSE_SIZEOF(*isf)*(isfc+16));
					if (tmp == QSE_NULL)
					{
						out_of_memory ();
						ABORT (oops);
					}

					isf = tmp;
					isfc = isfc + 16;
				}

				isf[isfl++] = opt.arg;
				break;
			}

			case QSE_T('F'):
			{
				fs = opt.arg;
				break;
			}

			case QSE_T('o'):
			{
				osf = opt.arg;
				break;
			}

			case QSE_T('v'):
			{
				qse_char_t* eq = qse_strchr(opt.arg, QSE_T('='));
				if (eq == QSE_NULL)
				{
					/* INVALID VALUE... */
					ABORT (oops);
				}

				*eq = QSE_T('\0');

				if (qse_map_upsert (vm, opt.arg, qse_strlen(opt.arg)+1, eq, qse_strlen(eq)+1) == QSE_NULL)
				{
					out_of_memory ();
					ABORT (oops);
				}
				break;
			}

			case QSE_T('?'):
			{
				if (opt.lngopt)
				{
					qse_printf (QSE_T("Error: illegal option - %s\n"), opt.lngopt);
				}
				else
				{
					qse_printf (QSE_T("Error: illegal option - %c\n"), opt.opt);
				}

				ABORT (oops);
			}

			case QSE_T(':'):
			{
				if (opt.lngopt)
				{
					qse_printf (QSE_T("Error: bad argument for %s\n"), opt.lngopt);
				}
				else
				{
					qse_printf (QSE_T("Error: bad argument for %c\n"), opt.opt);
				}

				ABORT (oops);
			}

			default:
				ABORT (oops);
		}
	}

	isf[isfl] = QSE_NULL;

	if (isfl <= 0)
	{
		if (opt.ind >= argc)
		{
			/* no source code specified */
			ABORT (oops);
		}

		/* the source code is the string, not from the file */
		ao->ist = QSE_AWK_PARSESTD_CP;
		ao->isp.str = argv[opt.ind++];

		free (isf);
	}
	else
	{
		ao->ist = QSE_AWK_PARSESTD_FILE;
		ao->isp.files = isf;
	}

	/* the remaining arguments are input console file names */
	icfc = (opt.ind >= argc)? 2: (argc - opt.ind + 1);
	icf = (qse_char_t**) malloc (QSE_SIZEOF(*icf)*icfc);
	if (icf == QSE_NULL)
	{
		out_of_memory ();
		ABORT (oops);
	}

	if (opt.ind >= argc)
	{
		/* no input(console) file names are specified.
		 * the standard input becomes the input console */
		icf[icfl++] = QSE_T("");
	}
	else
	{	
		do { icf[icfl++] = argv[opt.ind++]; } while (opt.ind < argc);
	}
	icf[icfl] = QSE_NULL;

	ao->ost = QSE_AWK_PARSESTD_FILE;
	ao->osf = osf;

	ao->icf = icf;
	ao->icfl = icfl;
	ao->vm = vm;
	ao->fs = fs;

	return 0;

oops:
	if (vm != QSE_NULL) qse_map_close (vm);
	if (icf != QSE_NULL) free (icf);
	if (isf != QSE_NULL) free (isf);
	return -1;
}

static qse_awk_t* open_awk (void)
{
	qse_awk_t* awk;

	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)
	{
		qse_printf (QSE_T("ERROR: cannot open awk\n"));
		return QSE_NULL;
	}
	
	/* TODO: get depth from command line */
	qse_awk_setmaxdepth (
		awk, QSE_AWK_DEPTH_BLOCK_PARSE | QSE_AWK_DEPTH_EXPR_PARSE, 50);
	qse_awk_setmaxdepth (
		awk, QSE_AWK_DEPTH_BLOCK_RUN | QSE_AWK_DEPTH_EXPR_RUN, 500);

	/*
	qse_awk_seterrstr (awk, QSE_AWK_EGBLRED, 
		QSE_T("\uC804\uC5ED\uBCC0\uC218 \'%.*s\'\uAC00 \uC7AC\uC815\uC758 \uB418\uC5C8\uC2B5\uB2C8\uB2E4"));
	qse_awk_seterrstr (awk, QSE_AWK_EFUNRED, 
		QSE_T("\uD568\uC218 \'%.*s\'\uAC00 \uC7AC\uC815\uC758 \uB418\uC5C8\uC2B5\uB2C8\uB2E4"));
	*/
	/*qse_awk_setkeyword (awk, QSE_T("func"), 4, QSE_T("FX"), 2);*/

	if (qse_awk_addfnc (awk, 
		QSE_T("sleep"), 5, 0,
		1, 1, QSE_NULL, fnc_sleep) == QSE_NULL)
	{
		qse_awk_close (awk);
		qse_printf (QSE_T("ERROR: cannot add function 'sleep'\n"));
		return QSE_NULL;
	}

	return awk;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	qse_awk_t* awk;
	qse_awk_rtx_t* rtx;
	qse_awk_rcb_t rcb;

	int i;
	int runarg_count = 0;
	qse_cstr_t runarg[128];
	struct argout_t ao;
	int ret = 0;

	qse_memset (&ao, 0, QSE_SIZEOF(ao));

	i = handle_args (argc, argv, &ao);
	if (i == -1)
	{
		print_usage (argv[0]);
		return -1;
	}
	if (i == 1) return 0;

	runarg[runarg_count].ptr = NULL;
	runarg[runarg_count].len = 0;

	awk = open_awk ();
	if (awk == QSE_NULL) return -1;

	/* TODO: change it to support multiple source files */
	qse_awk_parsestd_in_t psin;
	qse_awk_parsestd_out_t psout;
	
	psin.type = ao.ist;
	if (ao.ist == QSE_AWK_PARSESTD_CP) psin.u.cp = ao.isp.str;
	else psin.u.file = ao.isp.files[0];

	if (ao.osf != QSE_NULL)
	{
		psout.type = ao.ost;
		psout.u.file = ao.osf;
	}

	if (qse_awk_parsestd (awk, &psin, 
		((ao.osf == QSE_NULL)? QSE_NULL: &psout)) == -1)
	{
		qse_printf (
			QSE_T("PARSE ERROR: CODE [%d] LINE [%u] %s\n"), 
			qse_awk_geterrnum(awk),
			(unsigned int)qse_awk_geterrlin(awk), 
			qse_awk_geterrmsg(awk)
		);

		ret = -1;
		goto oops;
	}

	rcb.on_enter = on_run_enter;
	rcb.on_statement = on_run_statement;
	rcb.on_exit = on_run_exit;
	rcb.data = &ao;

	rtx = qse_awk_rtx_openstd (
		awk, 0, ao.icf, QSE_AWK_RTX_OPENSTD_STDIO);
	if (rtx == QSE_NULL) 
	{
		qse_printf (
			QSE_T("PARSE ERROR: CODE [%d] LINE [%u] %s\n"), 
			qse_awk_geterrnum(awk),
			(unsigned int)qse_awk_geterrlin(awk), 
			qse_awk_geterrmsg(awk)
		);

		ret = -1;
	}
	else
	{
		app_rtx = rtx;
		set_intr_run ();

		qse_awk_rtx_setrcb (rtx, &rcb);
		ret = qse_awk_rtx_loop (rtx);

		unset_intr_run ();

		if (ret == -1)
		{
			qse_printf (QSE_T("RUN ERROR: CODE [%d] LINE [%u] %s\n"),
				(unsigned int)qse_awk_rtx_geterrnum(rtx),
				(unsigned int)qse_awk_rtx_geterrlin(rtx),
				qse_awk_rtx_geterrmsg(rtx)
			);
		}

		qse_awk_rtx_close (rtx);
	}

oops:
	qse_awk_close (awk);

	if (ao.ist == QSE_AWK_PARSESTD_FILE && 
	    ao.isp.files != QSE_NULL) free (ao.isp.files);
	/*if (ao.osf != QSE_NULL) free (ao.osf);*/
	if (ao.icf != QSE_NULL) free (ao.icf);
	if (ao.vm != QSE_NULL) qse_map_close (ao.vm);

	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int n;

#if defined(_WIN32) && defined(_DEBUG) && defined(_MSC_VER)
	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
#endif

	n = qse_runmain (argc, argv, awk_main);

#if defined(_WIN32) && defined(_DEBUG)
	/*#if defined(_MSC_VER)
	_CrtDumpMemoryLeaks ();
	#endif*/
#endif

	return n;
}
