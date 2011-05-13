/*
 * $Id: awk.c 456 2011-05-12 14:55:53Z hyunghwan.chung $
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

#include <qse/awk/awk.h>
#include <qse/awk/std.h>
#include <qse/cmn/sll.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/misc.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/xma.h>

#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>

#define ENABLE_CALLBACK
#define ABORT(label) goto label

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	include <process.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include <unistd.h>
#	include <errno.h>
#endif

static qse_awk_rtx_t* app_rtx = QSE_NULL;
static int app_debug = 0;

struct arg_t
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
	qse_htb_t*   gvm;  /* global variable map */
	qse_char_t*  fs;   /* field separator */
	qse_char_t*  call; /* function to call */

	int          opton;
	int          optoff;
	qse_ulong_t  memlimit;
};

struct gvmv_t
{
	int         idx;
	qse_char_t* ptr;
	qse_size_t  len;
};

static void dprint (const qse_char_t* fmt, ...)
{
	if (app_debug)
	{
		va_list ap;
		va_start (ap, fmt);
		qse_vfprintf (QSE_STDERR, fmt, ap);
		va_end (ap);
	}
}

#if defined(_WIN32)
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
#elif defined(__OS2__)

static ULONG _System stop_run (
	PEXCEPTIONREPORTRECORD p1,
	PEXCEPTIONREGISTRATIONRECORD p2,
	PCONTEXTRECORD p3,
	PVOID pv)
{
	if (p1->ExceptionNum == XCPT_SIGNAL)
	{
		if (p1->ExceptionInfo[0] == XCPT_SIGNAL_INTR ||
		    p1->ExceptionInfo[0] == XCPT_SIGNAL_KILLPROC ||
		    p1->ExceptionInfo[0] == XCPT_SIGNAL_BREAK)
		{
			APIRET rc;

			qse_awk_rtx_stop (app_rtx);
			rc = DosAcknowledgeSignalException (p1->ExceptionInfo[0]);
			return (rc != NO_ERROR)? 1: XCPT_CONTINUE_EXECUTION; 
		}
	}

	return XCPT_CONTINUE_SEARCH; /* exception not resolved */
}

#elif defined(__DOS__)

static void setsignal (int sig, void(*handler)(int))
{
	signal (sig, handler);
}

static void stop_run (int sig)
{
	qse_awk_rtx_stop (app_rtx);
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

#if defined(__OS2__)
static EXCEPTIONREGISTRATIONRECORD os2_excrr = { 0 };
#endif

static void set_intr_run (void)
{
#if defined(_WIN32)
	SetConsoleCtrlHandler (stop_run, TRUE);
#elif defined(__OS2__)
	APIRET rc;
	os2_excrr.ExceptionHandler = (ERR)stop_run;
	rc = DosSetExceptionHandler (&os2_excrr);
	/*if (rc != NO_ERROR)...*/
#elif defined(__DOS__)
	setsignal (SIGINT, stop_run);
#else
	/*setsignal (SIGINT, stop_run, 1); TO BE MORE COMPATIBLE WITH WIN32*/
	setsignal (SIGINT, stop_run, 0);
#endif
}

static void unset_intr_run (void)
{
#if defined(_WIN32)
	SetConsoleCtrlHandler (stop_run, FALSE);
#elif defined(__OS2__)
	APIRET rc;
	rc = DosUnsetExceptionHandler (&os2_excrr);
	/*if (rc != NO_ERROR) ...*/
#elif defined(__DOS__)
	setsignal (SIGINT, SIG_DFL);
#else
	setsignal (SIGINT, SIG_DFL, 1);
#endif
}

static qse_htb_walk_t print_awk_value (
	qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	qse_awk_rtx_t* rtx = (qse_awk_rtx_t*)arg;
	qse_char_t* str;
	qse_size_t len;
	qse_awk_errinf_t oerrinf;

	qse_awk_rtx_geterrinf (rtx, &oerrinf);

	str = qse_awk_rtx_valtocpldup (rtx, QSE_HTB_VPTR(pair), &len);
	if (str == QSE_NULL)
	{
		if (qse_awk_rtx_geterrnum(rtx) == QSE_AWK_EVALTYPE)
		{
			dprint (QSE_T("%.*s = [not printable]\n"), 
				(int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair));

			qse_awk_rtx_seterrinf (rtx, &oerrinf);
		}
		else
		{
			dprint (QSE_T("***OUT OF MEMORY***\n"));
		}	
	}
	else
	{
		dprint (QSE_T("%.*s = %.*s\n"), 
			(int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), 
			(int)len, str);
		qse_awk_free (qse_awk_rtx_getawk(rtx), str);
	}

	return QSE_HTB_WALK_FORWARD;
}

static qse_htb_walk_t set_global (
	qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	qse_awk_val_t* v;
	qse_awk_rtx_t* rtx = (qse_awk_rtx_t*)arg;
	struct gvmv_t* gvmv = (struct gvmv_t*)QSE_HTB_VPTR(pair);

	v = qse_awk_rtx_makenstrval (rtx, gvmv->ptr, gvmv->len);
	if (v == QSE_NULL) return QSE_HTB_WALK_STOP;

	qse_awk_rtx_refupval (rtx, v);
	qse_awk_rtx_setgbl (rtx, gvmv->idx, v);
	qse_awk_rtx_refdownval (rtx, v);

	return QSE_HTB_WALK_FORWARD;
}

static int apply_fs_and_gvm (qse_awk_rtx_t* rtx, struct arg_t* arg)
{
	if (arg->fs != QSE_NULL)
	{
		qse_awk_val_t* fs;

		/* compose a string value to use to set FS to */
		fs = qse_awk_rtx_makestrval0 (rtx, arg->fs);
		if (fs == QSE_NULL) return -1;

		/* change FS according to the command line argument */
		qse_awk_rtx_refupval (rtx, fs);
		qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_FS, fs);
		qse_awk_rtx_refdownval (rtx, fs);
	}

	if (arg->gvm != QSE_NULL)
	{
		/* set the value of user-defined global variables 
		 * to a runtime context */
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOERR, QSE_NULL);
		qse_htb_walk (arg->gvm, set_global, rtx);
		if (qse_awk_rtx_geterrnum(rtx) != QSE_AWK_ENOERR) return -1;
	}

	return 0;
}

static void dprint_return (qse_awk_rtx_t* rtx, qse_awk_val_t* ret)
{
	qse_size_t len;
	qse_char_t* str;

	if (ret == qse_awk_val_nil)
	{
		dprint (QSE_T("[RETURN] - ***nil***\n"));
	}
	else
	{
		str = qse_awk_rtx_valtocpldup (rtx, ret, &len);
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
	qse_htb_walk (qse_awk_rtx_getnvmap(rtx), print_awk_value, rtx);
	dprint (QSE_T("[END NAMED VARIABLES]\n"));
}

#ifdef ENABLE_CALLBACK
static void on_statement (
	qse_awk_rtx_t* rtx, qse_awk_nde_t* nde, void* data)
{
	dprint (L"running %d at line %d\n", (int)nde->type, (int)nde->loc.line);
}
#endif

static int fnc_sleep (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
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

#if defined(_WIN32)
	Sleep ((DWORD)(lv * 1000));
	n = 0;
#elif defined(__OS2__)
	DosSleep ((ULONG)(lv * 1000));
	n = 0;
#else
	n = sleep (lv);	
#endif

	r = qse_awk_rtx_makeintval (run, n);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (run, r);
	return 0;
}

static void print_err (const qse_char_t* fmt, ...)
{
	va_list va;

	qse_fprintf (QSE_STDERR, QSE_T("ERROR: "));

	va_start (va, fmt);
	qse_vfprintf (QSE_STDERR, fmt, va);
	va_end (va);
}

struct opttab_t
{
	const qse_char_t* name;
	int opt;
	const qse_char_t* desc;
} opttab[] =
{
	{ QSE_T("implicit"),     QSE_AWK_IMPLICIT,       QSE_T("allow undeclared variables") },
	{ QSE_T("explicit"),     QSE_AWK_EXPLICIT,       QSE_T("allow declared variables(local,global)") },
	{ QSE_T("extraops"),     QSE_AWK_EXTRAOPS,       QSE_T("enable extra operators(<<,>>,^^,//)") },
	{ QSE_T("rio"),          QSE_AWK_RIO,            QSE_T("enable builtin I/O including getline & print") },
	{ QSE_T("rwpipe"),       QSE_AWK_RWPIPE,         QSE_T("allow a dual-directional pipe") },
	{ QSE_T("newline"),      QSE_AWK_NEWLINE,        QSE_T("enable a newline to terminate a statement") },
	{ QSE_T("striprecspc"),  QSE_AWK_STRIPRECSPC,    QSE_T("strip spaces in splitting a record") },
	{ QSE_T("stripstrspc"),  QSE_AWK_STRIPSTRSPC,    QSE_T("strip spaces in converting a string to a number") },
	{ QSE_T("nextofile"),    QSE_AWK_NEXTOFILE,      QSE_T("enable 'nextofile'") },
	{ QSE_T("reset"),        QSE_AWK_RESET,          QSE_T("enable 'reset'") },
	{ QSE_T("crlf"),         QSE_AWK_CRLF,           QSE_T("use CRLF for a newline") },
	{ QSE_T("maptovar"),     QSE_AWK_MAPTOVAR,       QSE_T("allow a map to be assigned or returned") },
	{ QSE_T("pablock"),      QSE_AWK_PABLOCK,        QSE_T("enable pattern-action loop") },
	{ QSE_T("rexbound"),     QSE_AWK_REXBOUND,       QSE_T("enable {n,m} in a regular expression") },
	{ QSE_T("ncmponstr"),    QSE_AWK_NCMPONSTR,      QSE_T("perform numeric comparsion on numeric strings") },
	{ QSE_T("strictnaming"), QSE_AWK_STRICTNAMING,   QSE_T("enable the strict naming rule") },
	{ QSE_T("include"),      QSE_AWK_INCLUDE,        QSE_T("enable 'include'") },
	{ QSE_NULL,             0 }
};

static void print_usage (QSE_FILE* out, const qse_char_t* argv0)
{
	int j;
	const qse_char_t* b = qse_basename (argv0);

	qse_fprintf (out, QSE_T("USAGE: %s [options] -f sourcefile [ -- ] [datafile]*\n"), b);
	qse_fprintf (out, QSE_T("       %s [options] [ -- ] sourcestring [datafile]*\n"), b);
	qse_fprintf (out, QSE_T("Where options are:\n"));
	qse_fprintf (out, QSE_T(" -h/--help                         print this message\n"));
	qse_fprintf (out, QSE_T(" -d                                show extra information\n"));
	qse_fprintf (out, QSE_T(" -c/--call            name         call a function instead of entering\n"));
	qse_fprintf (out, QSE_T("                                   the pattern-action loop\n"));
	qse_fprintf (out, QSE_T(" -f/--file            sourcefile   set the source script file\n"));
	qse_fprintf (out, QSE_T(" -o/--deparsed-file   deparsedfile set the deparsing output file\n"));
	qse_fprintf (out, QSE_T(" -F/--field-separator string       set a field separator(FS)\n"));
	qse_fprintf (out, QSE_T(" -v/--assign          var=value    add a global variable with a value\n"));
	qse_fprintf (out, QSE_T(" -m/--memory-limit    number       limit the memory usage (bytes)\n"));

	for (j = 0; opttab[j].name != QSE_NULL; j++)
	{
		qse_fprintf (out, QSE_T(" --%-18s on/off       %s\n"), opttab[j].name, opttab[j].desc);
	}
}

static int comparg (int argc, qse_char_t* argv[], struct arg_t* arg)
{
	static qse_opt_lng_t lng[] = 
	{
		{ QSE_T(":implicit"),        QSE_T('\0') },
		{ QSE_T(":explicit"),        QSE_T('\0') },
		{ QSE_T(":extraops"),        QSE_T('\0') },
		{ QSE_T(":rio"),             QSE_T('\0') },
		{ QSE_T(":rwpipe"),          QSE_T('\0') },
		{ QSE_T(":newline"),         QSE_T('\0') },
		{ QSE_T(":striprecspc"),     QSE_T('\0') },
		{ QSE_T(":stripstrspc"),     QSE_T('\0') },
		{ QSE_T(":nextofile"),       QSE_T('\0') },
		{ QSE_T(":reset"),           QSE_T('\0') },
		{ QSE_T(":crlf"),            QSE_T('\0') },
		{ QSE_T(":maptovar"),        QSE_T('\0') },
		{ QSE_T(":pablock"),         QSE_T('\0') },
		{ QSE_T(":rexbound"),        QSE_T('\0') },
		{ QSE_T(":ncmponstr"),       QSE_T('\0') },
		{ QSE_T(":strictnaming"),    QSE_T('\0') },
		{ QSE_T(":include"),         QSE_T('\0') },

		{ QSE_T(":call"),            QSE_T('c') },
		{ QSE_T(":file"),            QSE_T('f') },
		{ QSE_T(":field-separator"), QSE_T('F') },
		{ QSE_T(":deparsed-file"),   QSE_T('o') },
		{ QSE_T(":assign"),          QSE_T('v') },
		{ QSE_T(":memory-limit"),    QSE_T('m') },

		{ QSE_T("help"),             QSE_T('h') }
	};

	static qse_opt_t opt = 
	{
		QSE_T("dc:f:F:o:v:m:h"),
		lng
	};

	qse_cint_t c;

	qse_size_t isfc = 16; /* the capacity of isf */
	qse_size_t isfl = 0; /* number of input source files */

	qse_size_t icfc = 0; /* the capacity of icf */
	qse_size_t icfl = 0; /* the number of input console files */

	qse_char_t** isf = QSE_NULL; /* input source files */
	qse_char_t*  osf = QSE_NULL; /* output source file */
	qse_char_t** icf = QSE_NULL; /* input console files */

	qse_htb_t* gvm = QSE_NULL;  /* global variable map */
	qse_char_t* fs = QSE_NULL; /* field separator */
	qse_char_t* call = QSE_NULL; /* function to call */

	memset (arg, 0, QSE_SIZEOF(*arg));

	isf = (qse_char_t**) malloc (QSE_SIZEOF(*isf) * isfc);
	if (isf == QSE_NULL)
	{
		print_err (QSE_T("out of memory\n"));
		goto oops;
	}

	gvm = qse_htb_open (
		QSE_NULL, 0, 30, 70,
		QSE_SIZEOF(qse_char_t), QSE_SIZEOF(struct gvmv_t)
	); 
	if (gvm == QSE_NULL)
	{
		print_err (QSE_T("out of memory\n"));
		goto oops;
	}

	qse_htb_setmancbs (gvm,
		qse_htb_mancbs(QSE_HTB_MANCBS_INLINE_VALUE_COPIER)
	);

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			case QSE_T('h'):
				if (isf != QSE_NULL) free (isf);
				if (gvm != QSE_NULL) qse_htb_close (gvm);
				return 0;

			case QSE_T('d'):
			{
				app_debug = 1;
				break;
			}

			case QSE_T('c'):
			{
				call = opt.arg;
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
						print_err (QSE_T("out of memory\n"));
						goto oops;
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
				struct gvmv_t gvmv;
				qse_char_t* eq;

				eq = qse_strchr(opt.arg, QSE_T('='));
				if (eq == QSE_NULL) 
				{
					if (opt.lngopt)
						print_err (QSE_T("no value for '%s' in '%s'\n"), opt.arg, opt.lngopt);
					else
						print_err (QSE_T("no value for '%s' in '%c'\n"), opt.arg, opt.opt);
					goto oops;
				}

				*eq = QSE_T('\0');

				gvmv.idx = -1;
				gvmv.ptr = ++eq;
				gvmv.len = qse_strlen(eq);

				if (qse_htb_upsert (gvm, opt.arg, qse_strlen(opt.arg), &gvmv, 1) == QSE_NULL)
				{
					print_err (QSE_T("out of memory\n"));
					goto oops;
				}
				break;
			}

			case QSE_T('m'):
			{
				arg->memlimit = qse_strtoulong (opt.arg);
				break;
			}

			case QSE_T('\0'):
			{
				/* a long option with no corresponding short option */
				qse_size_t i;
				for (i = 0; opttab[i].name != QSE_NULL; i++)
				{
					if (qse_strcmp (opt.lngopt, opttab[i].name) == 0)
					{
						if (qse_strcmp (opt.arg, QSE_T("off")) == 0)
							arg->optoff |= opttab[i].opt;
						else if (qse_strcmp (opt.arg, QSE_T("on")) == 0)
							arg->opton |= opttab[i].opt;
						else
						{
							print_err (QSE_T("invalid value for '%s' - '%s'\n"), opt.lngopt, opt.arg);
							goto oops;
						}
						break;
					}
				}
				break;
			}
			
			case QSE_T('?'):
			{
				if (opt.lngopt)
					print_err (QSE_T("illegal option - '%s'\n"), opt.lngopt);
				else
					print_err (QSE_T("illegal option - '%c'\n"), opt.opt);

				goto oops;
			}

			case QSE_T(':'):
			{
				if (opt.lngopt)
					print_err (QSE_T("bad argument for '%s'\n"), opt.lngopt);
				else
					print_err (QSE_T("bad argument for '%c'\n"), opt.opt);

				goto oops;
			}

			default:
				goto oops;
		}
	}

	isf[isfl] = QSE_NULL;

	if (isfl <= 0)
	{
		if (opt.ind >= argc)
		{
			/* no source code specified */
			goto oops;
		}

		/* the source code is the string, not from the file */
		arg->ist = QSE_AWK_PARSESTD_CP;
		arg->isp.str = argv[opt.ind++];

		free (isf);
	}
	else
	{
		arg->ist = QSE_AWK_PARSESTD_FILE;
		arg->isp.files = isf;
	}

	if (opt.ind < argc) 
	{
		/* the remaining arguments are input console file names */

		icfc = argc - opt.ind + 1;
		icf = (qse_char_t**) malloc (QSE_SIZEOF(qse_char_t*)*icfc);
		if (icf == QSE_NULL)
		{
			print_err (QSE_T("out of memory\n"));
			goto oops;
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
	}

	arg->ost = QSE_AWK_PARSESTD_FILE;
	arg->osf = osf;

	arg->icf = icf;
	arg->icfl = icfl;
	arg->gvm = gvm;
	arg->fs = fs;
	arg->call = call;

	return 1;

oops:
	if (gvm != QSE_NULL) qse_htb_close (gvm);
	if (icf != QSE_NULL) free (icf);
	if (isf != QSE_NULL) free (isf);
	return -1;
}

static void freearg (struct arg_t* arg)
{
	if (arg->ist == QSE_AWK_PARSESTD_FILE &&
	    arg->isp.files != QSE_NULL) free (arg->isp.files);
	/*if (arg->osf != QSE_NULL) free (arg->osf);*/
	if (arg->icf != QSE_NULL) free (arg->icf);
	if (arg->gvm != QSE_NULL) qse_htb_close (arg->gvm);
}

static void print_awkerr (qse_awk_t* awk)
{
	const qse_awk_loc_t* loc = qse_awk_geterrloc (awk);

	print_err ( 
		QSE_T("CODE %d LINE %u COLUMN %u %s%s%s- %s\n"), 
		qse_awk_geterrnum(awk),
		(unsigned int)loc->line,
		(unsigned int)loc->colm,
		((loc->file == QSE_NULL)? QSE_T(""): QSE_T("FILE ")),
		((loc->file == QSE_NULL)? QSE_T(""): loc->file),
		((loc->file == QSE_NULL)? QSE_T(""): QSE_T(" ")),
		qse_awk_geterrmsg(awk)
	);
}

static void print_rtxerr (qse_awk_rtx_t* rtx)
{
	const qse_awk_loc_t* loc = qse_awk_rtx_geterrloc (rtx);

	print_err (
		QSE_T("CODE %d LINE %u COLUMN %u %s%s%s- %s\n"),
		qse_awk_rtx_geterrnum(rtx),
		(unsigned int)loc->line,
		(unsigned int)loc->colm,
		((loc->file == QSE_NULL)? QSE_T(""): QSE_T("FILE ")),
		((loc->file == QSE_NULL)? QSE_T(""): loc->file),
		((loc->file == QSE_NULL)? QSE_T(""): QSE_T(" ")),
		qse_awk_rtx_geterrmsg(rtx)
	);
}

qse_htb_walk_t add_global (qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	qse_awk_t* awk = (qse_awk_t*)arg;
	struct gvmv_t* gvmv = (struct gvmv_t*)QSE_HTB_VPTR(pair);

	gvmv->idx = qse_awk_addgbl (awk, QSE_HTB_KPTR(pair), QSE_HTB_KLEN(pair));
	if (gvmv->idx <= -1)
	{
		return QSE_HTB_WALK_STOP;
	}

	return QSE_HTB_WALK_FORWARD;
}

static qse_mmgr_t xma_mmgr = 
{
	(qse_mmgr_alloc_t)qse_xma_alloc,
	(qse_mmgr_realloc_t)qse_xma_realloc,
	(qse_mmgr_free_t)qse_xma_free,
	QSE_NULL
};

static int awk_main (int argc, qse_char_t* argv[])
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_val_t* retv;
#ifdef ENABLE_CALLBACK
	qse_awk_rcb_t rcb;
#endif
	int i;
	struct arg_t arg;
	int ret = -1;

	/* TODO: change it to support multiple source files */
	qse_awk_parsestd_in_t psin;
	qse_awk_parsestd_out_t psout;
	qse_mmgr_t* mmgr = QSE_NULL;

	memset (&arg, 0, QSE_SIZEOF(arg));

	i = comparg (argc, argv, &arg);
	if (i <= 0)
	{
		print_usage (((i == 0)? QSE_STDOUT: QSE_STDERR), argv[0]);
		return i;
	}

	psin.type = arg.ist;
	if (arg.ist == QSE_AWK_PARSESTD_CP) psin.u.cp = arg.isp.str;
	else psin.u.file = arg.isp.files[0];

	if (arg.osf != QSE_NULL)
	{
		psout.type = arg.ost;
		psout.u.file = arg.osf;
	}

	if (arg.memlimit > 0)
	{
		xma_mmgr.udd = qse_xma_open (QSE_NULL, 0, arg.memlimit);
		if (xma_mmgr.udd == QSE_NULL)
		{
			qse_printf (QSE_T("ERROR: cannot open memory heap\n"));
			goto oops;
		}
		mmgr = &xma_mmgr;
	}

	awk = qse_awk_openstdwithmmgr (mmgr, 0);
	/*awk = qse_awk_openstd (0);*/
	if (awk == QSE_NULL)
	{
		qse_printf (QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

	i = qse_awk_getoption (awk);
	if (arg.opton) i |= arg.opton;
	if (arg.optoff) i &= ~arg.optoff;
	qse_awk_setoption (awk, i);

	/* TODO: get depth from command line */
	qse_awk_setmaxdepth (
		awk, QSE_AWK_DEPTH_BLOCK_PARSE | QSE_AWK_DEPTH_EXPR_PARSE, 50);
	qse_awk_setmaxdepth (
		awk, QSE_AWK_DEPTH_BLOCK_RUN | QSE_AWK_DEPTH_EXPR_RUN, 500);
	qse_awk_setmaxdepth (awk, QSE_AWK_DEPTH_INCLUDE, 32);

	if (qse_awk_addfnc (awk, 
		QSE_T("sleep"), 5, 0,
		1, 1, QSE_NULL, fnc_sleep) == QSE_NULL)
	{
		print_awkerr (awk);
		goto oops;
	}

	qse_awk_seterrnum (awk, QSE_AWK_ENOERR, QSE_NULL);
	qse_htb_walk (arg.gvm, add_global, awk);
	if (qse_awk_geterrnum(awk) != QSE_AWK_ENOERR)
	{
		print_awkerr (awk);
		goto oops;
	}
	
	if (qse_awk_parsestd (awk, &psin, 
		((arg.osf == QSE_NULL)? QSE_NULL: &psout)) == -1)
	{
		print_awkerr (awk);
		goto oops;
	}

#ifdef ENABLE_CALLBACK
	rcb.stm = on_statement;
	rcb.udd = &arg;
#endif

	rtx = qse_awk_rtx_openstd (
		awk, 0, QSE_T("qseawk"),
		(const qse_char_t*const*)arg.icf, QSE_NULL);
	if (rtx == QSE_NULL) 
	{
		print_awkerr (awk);
		goto oops;
	}

	if (apply_fs_and_gvm (rtx, &arg) <= -1)
	{
		print_awkerr (awk);
		goto oops;
	}

	app_rtx = rtx;
#ifdef ENABLE_CALLBACK
	qse_awk_rtx_setrcb (rtx, &rcb);
#endif

	set_intr_run ();

	retv = (arg.call == QSE_NULL)?
		qse_awk_rtx_loop (rtx):
		qse_awk_rtx_call (rtx, arg.call, QSE_NULL, 0);
	if (retv != QSE_NULL)
	{
		qse_awk_rtx_refdownval (rtx, retv);
		ret = 0;

		dprint_return (rtx, retv);
	}

	unset_intr_run ();

	if (ret <= -1)
	{
		print_rtxerr (rtx);
		goto oops;
	}

oops:
	if (rtx) qse_awk_rtx_close (rtx);
	if (awk) qse_awk_close (awk);

	if (xma_mmgr.udd) qse_xma_close (xma_mmgr.udd);
	freearg (&arg);

	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, awk_main);
}

