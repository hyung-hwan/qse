/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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
#include <qse/cmn/path.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/xma.h>
#include <qse/cmn/glob.h>

#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <locale.h>

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
#	include <ltdl.h>
#	define USE_LTDL
#endif

static qse_awk_rtx_t* app_rtx = QSE_NULL;
static int app_debug = 0;

typedef struct arg_t arg_t;
typedef struct xarg_t xarg_t;

struct xarg_t
{
	qse_mmgr_t*  mmgr;
	qse_char_t** ptr;
	qse_size_t   size;
	qse_size_t   capa;
};

struct arg_t
{
	qse_awk_parsestd_type_t ist;  /* input source type */
	union
	{
		qse_char_t*  str;
		qse_char_t** files;
	} isp;
	qse_size_t   isfl; /* the number of input source files */
	qse_char_t*  osf;  /* output source file */
	xarg_t       icf; /* input console files */

	qse_htb_t*   gvm;  /* global variable map */
	qse_char_t*  fs;   /* field separator */
	qse_char_t*  call; /* function to call */
	qse_cmgr_t*  script_cmgr;
	qse_cmgr_t*  console_cmgr;

	unsigned int modern: 1;
	unsigned int classic: 1;
	int          opton;
	int          optoff;
	qse_ulong_t  memlimit;
#if defined(QSE_BUILD_DEBUG)
	qse_ulong_t  failmalloc;
#endif
};

struct gvmv_t
{
	int         idx;
	qse_cstr_t  str;
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
	setsignal (SIGPIPE, SIG_IGN, 0);
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
	setsignal (SIGPIPE, SIG_DFL, 0);
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

	str = qse_awk_rtx_valtostrdup (rtx, QSE_HTB_VPTR(pair), &len);
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
		qse_awk_freemem (qse_awk_rtx_getawk(rtx), str);
	}

	return QSE_HTB_WALK_FORWARD;
}

static qse_htb_walk_t set_global (
	qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	qse_awk_val_t* v;
	qse_awk_rtx_t* rtx = (qse_awk_rtx_t*)arg;
	struct gvmv_t* gvmv = (struct gvmv_t*)QSE_HTB_VPTR(pair);

	v = qse_awk_rtx_makenstrvalwithcstr (rtx, &gvmv->str);
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
		fs = qse_awk_rtx_makestrvalwithstr (rtx, arg->fs);
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
		str = qse_awk_rtx_valtostrdup (rtx, ret, &len);
		if (str == QSE_NULL)
		{
			dprint (QSE_T("[RETURN] - ***OUT OF MEMORY***\n"));
		}
		else
		{
			dprint (QSE_T("[RETURN] - [%.*s]\n"), (int)len, str);
			qse_awk_freemem (qse_awk_rtx_getawk(rtx), str);
		}
	}

	dprint (QSE_T("[NAMED VARIABLES]\n"));
	qse_htb_walk (qse_awk_rtx_getnvmap(rtx), print_awk_value, rtx);
	dprint (QSE_T("[END NAMED VARIABLES]\n"));
}

#ifdef ENABLE_CALLBACK
static void on_statement (qse_awk_rtx_t* rtx, qse_awk_nde_t* nde)
{
	dprint (QSE_T("running %d at line %d\n"), (int)nde->type, (int)nde->loc.line);
}
#endif

static void print_version (void)
{
	qse_printf (QSE_T("QSEAWK version %hs\n"), QSE_PACKAGE_VERSION);
}

static void print_error (const qse_char_t* fmt, ...)
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
	{ QSE_T("explicit"),     QSE_AWK_EXPLICIT,       QSE_T("enable local,global for variable declaration") },
	{ QSE_T("extrakws"),     QSE_AWK_EXTRAKWS,       QSE_T("enable abort,reset,nextofile,OFILENAME,@include") },
	{ QSE_T("rio"),          QSE_AWK_RIO,            QSE_T("enable builtin I/O including getline & print") },
	{ QSE_T("rwpipe"),       QSE_AWK_RWPIPE,         QSE_T("allow a dual-directional pipe") },
	{ QSE_T("newline"),      QSE_AWK_NEWLINE,        QSE_T("enable a newline to terminate a statement") },
	{ QSE_T("striprecspc"),  QSE_AWK_STRIPRECSPC,    QSE_T("strip spaces in splitting a record") },
	{ QSE_T("stripstrspc"),  QSE_AWK_STRIPSTRSPC,    QSE_T("strip spaces in string-to-number conversion") },
	{ QSE_T("blankconcat"),  QSE_AWK_BLANKCONCAT,    QSE_T("enable concatenation by blanks") },
	{ QSE_T("crlf"),         QSE_AWK_CRLF,           QSE_T("use CRLF for a newline") },
	{ QSE_T("maptovar"),     QSE_AWK_MAPTOVAR,       QSE_T("allow a map to be assigned or returned") },
	{ QSE_T("pablock"),      QSE_AWK_PABLOCK,        QSE_T("enable pattern-action loop") },
	{ QSE_T("rexbound"),     QSE_AWK_REXBOUND,       QSE_T("enable {n,m} in a regular expression") },
	{ QSE_T("ncmponstr"),    QSE_AWK_NCMPONSTR,      QSE_T("perform numeric comparsion on numeric strings") },
	{ QSE_T("strictnaming"), QSE_AWK_STRICTNAMING,   QSE_T("enable the strict naming rule") },
	{ QSE_T("tolerant"),     QSE_AWK_TOLERANT,       QSE_T("make more fault-tolerant") },
	{ QSE_NULL,              0,                      QSE_NULL }
};

static void print_usage (QSE_FILE* out, const qse_char_t* argv0)
{
	int j;
	const qse_char_t* b = qse_basename (argv0);

	qse_fprintf (out, QSE_T("USAGE: %s [options] -f sourcefile [ -- ] [datafile]*\n"), b);
	qse_fprintf (out, QSE_T("       %s [options] [ -- ] sourcestring [datafile]*\n"), b);
	qse_fprintf (out, QSE_T("Where options are:\n"));
	qse_fprintf (out, QSE_T(" -h/--help                         print this message\n"));
	qse_fprintf (out, QSE_T(" --version                         print version\n"));
	qse_fprintf (out, QSE_T(" -D                                show extra information\n"));
	qse_fprintf (out, QSE_T(" -c/--call            name         call a function instead of entering\n"));
	qse_fprintf (out, QSE_T("                                   the pattern-action loop. [datafile]* is\n"));
	qse_fprintf (out, QSE_T("                                   passed to the function as parameters\n"));
	qse_fprintf (out, QSE_T(" -f/--file            sourcefile   set the source script file\n"));
	qse_fprintf (out, QSE_T(" -d/--deparsed-file   deparsedfile set the deparsing output file\n"));
	qse_fprintf (out, QSE_T(" -F/--field-separator string       set a field separator(FS)\n"));
	qse_fprintf (out, QSE_T(" -v/--assign          var=value    add a global variable with a value\n"));
	qse_fprintf (out, QSE_T(" -m/--memory-limit    number       limit the memory usage (bytes)\n"));
	qse_fprintf (out, QSE_T(" -w                                expand datafile wildcards\n"));
	
#if defined(QSE_BUILD_DEBUG)
	qse_fprintf (out, QSE_T(" -X                   number       fail the number'th memory allocation\n"));
#endif
#if defined(QSE_CHAR_IS_WCHAR)
	qse_fprintf (out, QSE_T(" --script-encoding    string       specify script file encoding name\n"));
	qse_fprintf (out, QSE_T(" --console-encoding   string       specify console encoding name\n"));
#endif
	qse_fprintf (out, QSE_T(" --modern                          run in the modern mode(default)\n"));
	qse_fprintf (out, QSE_T(" --classic                         run in the classic mode\n"));

	for (j = 0; opttab[j].name; j++)
	{
		qse_fprintf (out, 
			QSE_T(" --%-18s on/off       %s\n"), 
			opttab[j].name, opttab[j].desc);
	}
}

/* ---------------------------------------------------------------------- */

static int collect_into_xarg (const qse_cstr_t* path, void* ctx)
{
	xarg_t* xarg = (xarg_t*)ctx;

	if (xarg->size <= xarg->capa)
	{
		qse_char_t** tmp;

		tmp = QSE_MMGR_REALLOC (
			xarg->mmgr, xarg->ptr, 
			QSE_SIZEOF(*tmp) * (xarg->capa + 128 + 1));
		if (tmp == QSE_NULL) return -1;

		xarg->ptr = tmp;
		xarg->capa += 128;
	}

	xarg->ptr[xarg->size] = qse_strdup (path->ptr, xarg->mmgr);
	if (xarg->ptr[xarg->size] == QSE_NULL) return -1;
	xarg->size++;

	return 0;
}

static void purge_xarg (xarg_t* xarg)
{
	if (xarg->ptr)
	{
		qse_size_t i;

		for (i = 0; i < xarg->size; i++)
			QSE_MMGR_FREE (xarg->mmgr, xarg->ptr[i]);
		QSE_MMGR_FREE (xarg->mmgr, xarg->ptr);

		xarg->size = 0;
		xarg->capa = 0;
		xarg->ptr = QSE_NULL;
	}
}

static int expand_wildcard (int argc, qse_char_t* argv[], int glob, xarg_t* xarg)
{
	int i;
	qse_cstr_t tmp;

	for (i = 0; i < argc; i++)
	{
		int x;

		if (glob)
		{
			x = qse_glob (argv[i], collect_into_xarg, xarg, 
				QSE_GLOB_TOLERANT |
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
				QSE_GLOB_NOESCAPE | QSE_GLOB_PERIOD | QSE_GLOB_IGNORECASE, 
#else
				QSE_GLOB_PERIOD,
#endif
				xarg->mmgr
			);
			if (x <= -1) return -1;
		}
		else x = 0;

		if (x == 0)
		{
			/* not expanded. just use it as is */
			tmp.ptr = argv[i];
			tmp.len = qse_strlen(argv[i]);
			if (collect_into_xarg (&tmp, xarg) <= -1) return -1;
		}
	}

	xarg->ptr[xarg->size] = QSE_NULL;
	return 0;
}

/* ---------------------------------------------------------------------- */

static int comparg (int argc, qse_char_t* argv[], struct arg_t* arg)
{
	static qse_opt_lng_t lng[] = 
	{

		{ QSE_T(":implicit"),        QSE_T('\0') },
		{ QSE_T(":explicit"),        QSE_T('\0') },
		{ QSE_T(":extrakws"),        QSE_T('\0') },
		{ QSE_T(":rio"),             QSE_T('\0') },
		{ QSE_T(":rwpipe"),          QSE_T('\0') },
		{ QSE_T(":newline"),         QSE_T('\0') },
		{ QSE_T(":striprecspc"),     QSE_T('\0') },
		{ QSE_T(":stripstrspc"),     QSE_T('\0') },
		{ QSE_T(":blankconcat"),     QSE_T('\0') },
		{ QSE_T(":crlf"),            QSE_T('\0') },
		{ QSE_T(":maptovar"),        QSE_T('\0') },
		{ QSE_T(":pablock"),         QSE_T('\0') },
		{ QSE_T(":rexbound"),        QSE_T('\0') },
		{ QSE_T(":ncmponstr"),       QSE_T('\0') },
		{ QSE_T(":strictnaming"),    QSE_T('\0') },
		{ QSE_T(":tolerant"),        QSE_T('\0') },

		{ QSE_T(":call"),            QSE_T('c') },
		{ QSE_T(":file"),            QSE_T('f') },
		{ QSE_T(":deparsed-file"),   QSE_T('d') },
		{ QSE_T(":field-separator"), QSE_T('F') },
		{ QSE_T(":assign"),          QSE_T('v') },
		{ QSE_T(":memory-limit"),    QSE_T('m') },

		{ QSE_T(":script-encoding"),  QSE_T('\0') },
		{ QSE_T(":console-encoding"), QSE_T('\0') },

		{ QSE_T("modern"),           QSE_T('\0') },
		{ QSE_T("classic"),          QSE_T('\0') },

		{ QSE_T("version"),          QSE_T('\0') },
		{ QSE_T("help"),             QSE_T('h') },
		{ QSE_NULL,                  QSE_T('\0') }                  
	};

	static qse_opt_t opt = 
	{
#if defined(QSE_BUILD_DEBUG)
		QSE_T("hDc:f:d:F:v:m:wX:"),
#else
		QSE_T("hDc:f:d:F:v:m:w"),
#endif
		lng
	};

	qse_cint_t c;

	qse_size_t isfc = 16; /* the capacity of isf */
	qse_size_t isfl = 0; /* number of input source files */
	qse_char_t** isf = QSE_NULL; /* input source files */
	qse_char_t*  osf = QSE_NULL; /* output source file */
	qse_htb_t* gvm = QSE_NULL;  /* global variable map */
	qse_char_t* fs = QSE_NULL; /* field separator */
	qse_char_t* call = QSE_NULL; /* function to call */

	int oops_ret = -1;
	int do_glob = 0;

	isf = (qse_char_t**) malloc (QSE_SIZEOF(*isf) * isfc);
	if (isf == QSE_NULL)
	{
		print_error (QSE_T("out of memory\n"));
		goto oops;
	}

	gvm = qse_htb_open (
		QSE_MMGR_GETDFL(), 0, 30, 70,
		QSE_SIZEOF(qse_char_t), QSE_SIZEOF(struct gvmv_t)
	); 
	if (gvm == QSE_NULL)
	{
		print_error (QSE_T("out of memory\n"));
		goto oops;
	}

	qse_htb_setmancbs (gvm,
		qse_gethtbmancbs(QSE_HTB_MANCBS_INLINE_VALUE_COPIER)
	);

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			case QSE_T('h'):
				oops_ret = 0;
				goto oops;

			case QSE_T('D'):
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
						print_error (QSE_T("out of memory\n"));
						goto oops;
					}

					isf = tmp;
					isfc = isfc + 16;
				}

				isf[isfl++] = opt.arg;
				break;
			}

			case QSE_T('d'):
			{
				osf = opt.arg;
				break;
			}

			case QSE_T('F'):
			{
				fs = opt.arg;
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
						print_error (QSE_T("no value for '%s' in '%s'\n"), opt.arg, opt.lngopt);
					else
						print_error (QSE_T("no value for '%s' in '%c'\n"), opt.arg, opt.opt);
					goto oops;
				}

				*eq = QSE_T('\0');

				gvmv.idx = -1;
				gvmv.str.ptr = ++eq;
				gvmv.str.len = qse_strlen(eq);

				/* +1 for null-termination of the key in the table */
				if (qse_htb_upsert (gvm, opt.arg, qse_strlen(opt.arg) + 1, &gvmv, 1) == QSE_NULL)
				{
					print_error (QSE_T("out of memory\n"));
					goto oops;
				}
				break;
			}

			case QSE_T('m'):
			{
				arg->memlimit = qse_strtoulong (opt.arg);
				break;
			}

			case QSE_T('w'):
			{
				do_glob = 1;
				break;
			}

#if defined(QSE_BUILD_DEBUG)
			case QSE_T('X'):
			{
				arg->failmalloc = qse_strtoulong (opt.arg);
				break;
			}
#endif

			case QSE_T('\0'):
			{
				/* a long option with no corresponding short option */
				qse_size_t i;

				if (qse_strcmp(opt.lngopt, QSE_T("version")) == 0)
				{
					print_version ();
					oops_ret = 2;
					goto oops;
				}
				else if (qse_strcmp(opt.lngopt, QSE_T("script-encoding")) == 0)
				{
					arg->script_cmgr = qse_findcmgr (opt.arg);
					if (arg->script_cmgr == QSE_NULL)
					{
						print_error (QSE_T("unknown script encoding - %s\n"), opt.arg);
						oops_ret = 3;
						goto oops;
					}
				}
				else if (qse_strcmp(opt.lngopt, QSE_T("console-encoding")) == 0)
				{
					arg->console_cmgr = qse_findcmgr (opt.arg);
					if (arg->console_cmgr == QSE_NULL)
					{
						print_error (QSE_T("unknown console encoding - %s\n"), opt.arg);
						oops_ret = 3;
						goto oops;
					}
				}
				else if (qse_strcmp(opt.lngopt, QSE_T("modern")) == 0)
				{
					arg->modern = 1;
				}
				else if (qse_strcmp(opt.lngopt, QSE_T("classic")) == 0)
				{
					arg->classic = 1;
				}
				else
				{
					for (i = 0; opttab[i].name; i++)
					{
						if (qse_strcmp (opt.lngopt, opttab[i].name) == 0)
						{
							if (qse_strcmp (opt.arg, QSE_T("off")) == 0)
								arg->optoff |= opttab[i].opt;
							else if (qse_strcmp (opt.arg, QSE_T("on")) == 0)
								arg->opton |= opttab[i].opt;
							else
							{
								print_error (QSE_T("invalid value '%s' for '%s' - use 'on' or 'off'\n"), opt.arg, opt.lngopt);
								oops_ret = 3;
								goto oops;
							}
							break;
						}
					}

				}
				break;
			}
			
			case QSE_T('?'):
			{
				if (opt.lngopt)
					print_error (QSE_T("illegal option - '%s'\n"), opt.lngopt);
				else
					print_error (QSE_T("illegal option - '%c'\n"), opt.opt);

				goto oops;
			}

			case QSE_T(':'):
			{
				if (opt.lngopt)
					print_error (QSE_T("bad argument for '%s'\n"), opt.lngopt);
				else
					print_error (QSE_T("bad argument for '%c'\n"), opt.opt);

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
		arg->ist = QSE_AWK_PARSESTD_STR;
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
		if (expand_wildcard (argc - opt.ind,  &argv[opt.ind], do_glob, &arg->icf) <= -1)
		{
			print_error (QSE_T("failed to expand wildcard\n"));
			goto oops;
		}
	}

	arg->osf = osf;
	arg->gvm = gvm;
	arg->fs = fs;
	arg->call = call;

	return 1;

oops:
	if (gvm != QSE_NULL) qse_htb_close (gvm);
	purge_xarg (&arg->icf);
	if (isf != QSE_NULL) free (isf);
	return oops_ret;
}

static void freearg (struct arg_t* arg)
{
	if (arg->ist == QSE_AWK_PARSESTD_FILE &&
	    arg->isp.files != QSE_NULL) free (arg->isp.files);
	/*if (arg->osf != QSE_NULL) free (arg->osf);*/
	purge_xarg (&arg->icf);
	if (arg->gvm != QSE_NULL) qse_htb_close (arg->gvm);
}

static void print_awkerr (qse_awk_t* awk)
{
	const qse_awk_loc_t* loc = qse_awk_geterrloc (awk);

	print_error ( 
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

	print_error (
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

	/* the key was inserted to the table with a null at the end
	 * and the key length was even incremetned for that.
	 * so i can pass the pointer without other adjustments. */
	gvmv->idx = qse_awk_addgbl (awk, QSE_HTB_KPTR(pair));
	if (gvmv->idx <= -1) return QSE_HTB_WALK_STOP;
	return QSE_HTB_WALK_FORWARD;
}

static qse_mmgr_t xma_mmgr = 
{
	(qse_mmgr_alloc_t)qse_xma_alloc,
	(qse_mmgr_realloc_t)qse_xma_realloc,
	(qse_mmgr_free_t)qse_xma_free,
	QSE_NULL
};

#if defined(QSE_BUILD_DEBUG)
static qse_ulong_t debug_mmgr_count = 0;
static qse_ulong_t debug_mmgr_alloc_count = 0;
static qse_ulong_t debug_mmgr_realloc_count = 0;
static qse_ulong_t debug_mmgr_free_count = 0;

static void* debug_mmgr_alloc (void* ctx, qse_size_t size)
{
	void* ptr;
	struct arg_t* arg = (struct arg_t*)ctx;
	debug_mmgr_count++;
	if (debug_mmgr_count % arg->failmalloc == 0) return QSE_NULL;
	ptr = malloc (size);
	if (ptr) debug_mmgr_alloc_count++;
	return ptr;
}

static void* debug_mmgr_realloc (void* ctx, void* ptr, qse_size_t size)
{
	void* rptr;
	struct arg_t* arg = (struct arg_t*)ctx;
	debug_mmgr_count++;
	if (debug_mmgr_count % arg->failmalloc == 0) return QSE_NULL;
	rptr = realloc (ptr, size);
	if (rptr)
	{
		if (ptr) debug_mmgr_realloc_count++;
		else debug_mmgr_alloc_count++;
	}
	return rptr;
}

static void debug_mmgr_free (void* ctx, void* ptr)
{
	debug_mmgr_free_count++;
	free (ptr);
}

static qse_mmgr_t debug_mmgr =
{
	debug_mmgr_alloc,
	debug_mmgr_realloc,
	debug_mmgr_free,
	QSE_NULL
};
#endif

static int awk_main (int argc, qse_char_t* argv[])
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_val_t* retv;
	int i;
	struct arg_t arg;
	int ret = -1;

#ifdef ENABLE_CALLBACK
	static qse_awk_rtx_ecb_t rtx_ecb =
	{
		QSE_FV(.close, QSE_NULL),
		QSE_FV(.stmt, on_statement)
	};
#endif

	/* TODO: change it to support multiple source files */
	qse_awk_parsestd_t psin;
	qse_awk_parsestd_t psout;
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL();

	qse_memset (&arg, 0, QSE_SIZEOF(arg));
	arg.icf.mmgr = mmgr;

	i = comparg (argc, argv, &arg);
	if (i <= 0)
	{
		print_usage (((i == 0)? QSE_STDOUT: QSE_STDERR), argv[0]);
		return i;
	}
	if (i == 2) return 0;
	if (i == 3) return -1;

	psin.type = arg.ist;
	if (arg.ist == QSE_AWK_PARSESTD_STR) 
	{
		psin.u.str.ptr = arg.isp.str;
		psin.u.str.len = qse_strlen(arg.isp.str);
	}
	else 
	{
		psin.u.file.path = arg.isp.files[0];
		psin.u.file.cmgr = arg.script_cmgr;
	}

	if (arg.osf != QSE_NULL)
	{
		psout.type = QSE_AWK_PARSESTD_FILE;
		psout.u.file.path = arg.osf;
		psout.u.file.cmgr = arg.script_cmgr;
	}

#if defined(QSE_BUILD_DEBUG)
	if (arg.failmalloc > 0)
	{
		debug_mmgr.ctx = &arg;
		mmgr = &debug_mmgr;	
	}
	else 
#endif
	if (arg.memlimit > 0)
	{
		xma_mmgr.ctx = qse_xma_open (QSE_MMGR_GETDFL(), 0, arg.memlimit);
		if (xma_mmgr.ctx == QSE_NULL)
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

	if (arg.modern) i = QSE_AWK_MODERN;
	else if (arg.classic) i = QSE_AWK_CLASSIC;
	else qse_awk_getopt (awk, QSE_AWK_TRAIT, &i);
	if (arg.opton) i |= arg.opton;
	if (arg.optoff) i &= ~arg.optoff;
	qse_awk_setopt (awk, QSE_AWK_TRAIT, &i);

	/* TODO: get depth from command line */
	{
		qse_size_t tmp;
		tmp = 50;
		qse_awk_setopt (awk, QSE_AWK_DEPTH_BLOCK_PARSE, &tmp);
		qse_awk_setopt (awk, QSE_AWK_DEPTH_EXPR_PARSE, &tmp);
		tmp = 500;
		qse_awk_setopt (awk, QSE_AWK_DEPTH_BLOCK_RUN, &tmp);
		qse_awk_setopt (awk, QSE_AWK_DEPTH_EXPR_RUN, &tmp);
		tmp = 64;
		qse_awk_setopt (awk, QSE_AWK_DEPTH_INCLUDE, &tmp);
	}

	qse_awk_seterrnum (awk, QSE_AWK_ENOERR, QSE_NULL);
	qse_htb_walk (arg.gvm, add_global, awk);
	if (qse_awk_geterrnum(awk) != QSE_AWK_ENOERR)
	{
		print_awkerr (awk);
		goto oops;
	}
	
	if (qse_awk_parsestd (awk, &psin, 
		((arg.osf == QSE_NULL)? QSE_NULL: &psout)) <= -1)
	{
		print_awkerr (awk);
		goto oops;
	}

	rtx = qse_awk_rtx_openstd (
		awk, 0, QSE_T("qseawk"),
		(arg.call? QSE_NULL: arg.icf.ptr), /* console input */
		QSE_NULL,  /* console output */
		arg.console_cmgr
	);
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
	qse_awk_rtx_pushecb (rtx, &rtx_ecb);
#endif

	set_intr_run ();

	retv = arg.call?
		qse_awk_rtx_callwithstrs (rtx, arg.call, arg.icf.ptr, arg.icf.size):
		qse_awk_rtx_loop (rtx);

	unset_intr_run ();

	if (retv)
	{
		qse_long_t tmp;

		qse_awk_rtx_refdownval (rtx, retv);
		if (app_debug) dprint_return (rtx, retv);

		ret = 0;
		if (qse_awk_rtx_valtolong (rtx, retv, &tmp) >= 0) ret = tmp;
	}
	else
	{
		print_rtxerr (rtx);
		goto oops;
	}

oops:
	if (rtx) qse_awk_rtx_close (rtx);
	if (awk) qse_awk_close (awk);

	if (xma_mmgr.ctx) qse_xma_close (xma_mmgr.ctx);
	freearg (&arg);

#if defined(QSE_BUILD_DEBUG)
	if (arg.failmalloc > 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("\n"));
		qse_fprintf (QSE_STDERR, QSE_T("-[MALLOC COUNTS]---------------------------------------\n"));
		qse_fprintf (QSE_STDERR, QSE_T("ALLOC: %lu FREE: %lu: REALLOC: %lu\n"), 
			(unsigned long)debug_mmgr_alloc_count,
			(unsigned long)debug_mmgr_free_count,
			(unsigned long)debug_mmgr_realloc_count);
		qse_fprintf (QSE_STDERR, QSE_T("-------------------------------------------------------\n"));
	}
#endif

	return ret;
}

struct mpi_t
{
	void* h;
	int (*i) (int argc, qse_achar_t* argv[]);
	void (*f) (void);
};

typedef struct mpi_t mpi_t;

static void open_mpi (mpi_t* mpi, int argc, qse_achar_t* argv[])
{
#if defined(USE_LTDL)
	lt_dladvise adv;
#endif

	qse_memset (mpi, 0, QSE_SIZEOF(*mpi));

#if defined(USE_LTDL)

	#if defined(QSE_ACHAR_IS_MCHAR)
	if (qse_mbscmp (qse_mbsbasename(argv[0]), QSE_MT("qseawkmp")) != 0 &&
	    qse_mbscmp (qse_mbsbasename(argv[0]), QSE_MT("qseawkmpi")) != 0) return;
	#else
	if (qse_wcscmp (qse_wcsbasename(argv[0]), QSE_WT("qseawkmp")) != 0 &&
	    qse_wcscmp (qse_wcsbasename(argv[0]), QSE_WT("qseawkmpi")) != 0) return;
	#endif

	if (lt_dlinit () != 0) return;

	if (lt_dladvise_init (&adv) != 0) goto oops;

	/* If i don't set the global option, loading may end up with an error
	 * like this depending on your MPI library.
	 *
	 *  symbol lookup error: /usr/lib/openmpi/lib/openmpi/mca_paffinity_linux.so: undefined symbol: mca_base_param_reg_int
	 */

	if (lt_dladvise_global (&adv) != 0 || lt_dladvise_ext (&adv) != 0)
	{
		lt_dladvise_destroy (&adv);
		goto oops;
	}

	mpi->h = lt_dlopenadvise (DEFAULT_MODPREFIX "mpi" DEFAULT_MODPOSTFIX, adv);
	lt_dladvise_destroy (&adv);

	if (mpi->h)
	{
		mpi->i = lt_dlsym (mpi->h, "mpi_init");
		mpi->f = lt_dlsym (mpi->h, "mpi_fini");

		if (mpi->i == QSE_NULL || 
		    mpi->f == QSE_NULL ||
		    mpi->i (argc, argv) <= -1)
		{
			lt_dlclose (mpi->h);
			mpi->h = QSE_NULL;
		}
	}

	return;

oops:
	lt_dlexit ();
#endif
}

static void close_mpi (mpi_t* mpi)
{
	if (mpi->h)
	{
#if defined(USE_LTDL)
		mpi->f ();
		lt_dlclose (mpi->h);
		mpi->h = QSE_NULL;
		lt_dlexit ();
#endif
	}
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int ret;
	mpi_t mpi;

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

	open_mpi (&mpi, argc, argv);
	
	ret = qse_runmain (argc, argv, awk_main);

	close_mpi (&mpi);

#if defined(_WIN32)
	WSACleanup ();
#endif

	return ret;
}

