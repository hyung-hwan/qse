/**
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

#include <qse/sed/std.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/xma.h>
#include <qse/cmn/path.h>
#include <qse/cmn/fs.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/glob.h>

#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	include <process.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#	include <signal.h>
#elif defined(__DOS__)
#	include <dos.h>
#	include <signal.h>
#else
#	include <unistd.h>
#	include <errno.h>
#	include <signal.h>
#endif

static struct
{
	qse_sed_iostd_t* io;
	qse_size_t capa;
	qse_size_t size;
} g_script = 
{
	QSE_NULL,
	0,
	0
};
	
static qse_char_t* g_output_file = QSE_NULL;
static int g_infile_pos = 0;
static int g_option = 0;
static int g_separate = 0;
static int g_inplace = 0;
static int g_wildcard = 0;
#if defined(QSE_ENABLE_SEDTRACER)
static int g_trace = 0;
#endif
static qse_ulong_t g_memlimit = 0;
static qse_sed_t* g_sed = QSE_NULL;

static qse_cmgr_t* g_script_cmgr = QSE_NULL;
static qse_cmgr_t* g_infile_cmgr = QSE_NULL;
static qse_cmgr_t* g_outfile_cmgr = QSE_NULL;

#if defined(QSE_BUILD_DEBUG)
#include <stdlib.h>
static qse_ulong_t g_failmalloc = 0;
static qse_ulong_t debug_mmgr_count = 0;
static qse_ulong_t debug_mmgr_alloc_count = 0;
static qse_ulong_t debug_mmgr_realloc_count = 0;
static qse_ulong_t debug_mmgr_free_count = 0;

static void* debug_mmgr_alloc (void* ctx, qse_size_t size)
{
	void* ptr;
	debug_mmgr_count++;
	if (debug_mmgr_count % g_failmalloc == 0) return QSE_NULL;
	ptr = malloc (size);
	if (ptr) debug_mmgr_alloc_count++;
	return ptr;
}

static void* debug_mmgr_realloc (void* ctx, void* ptr, qse_size_t size)
{
	void* rptr;
	debug_mmgr_count++;
	if (debug_mmgr_count % g_failmalloc == 0) return QSE_NULL;
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

static qse_mmgr_t xma_mmgr = 
{
	(qse_mmgr_alloc_t)qse_xma_alloc,
	(qse_mmgr_realloc_t)qse_xma_realloc,
	(qse_mmgr_free_t)qse_xma_free,
	QSE_NULL
};

static void print_version (void)
{
	qse_printf (QSE_T("QSESED version %hs\n"), QSE_PACKAGE_VERSION);
}

static void print_usage (QSE_FILE* out, int argc, qse_char_t* argv[])
{
	const qse_char_t* b = qse_basename (argv[0]);

	qse_fprintf (out, QSE_T("USAGE: %s [options] script [file]\n"), b);
	qse_fprintf (out, QSE_T("       %s [options] -f script-file [file]\n"), b);
	qse_fprintf (out, QSE_T("       %s [options] -e script [file]\n"), b);

	qse_fprintf (out, QSE_T("options as follows:\n"));
	qse_fprintf (out, QSE_T(" -h/--help                 show this message\n"));
	qse_fprintf (out, QSE_T(" --version                 show version\n"));
	qse_fprintf (out, QSE_T(" -n                        disable auto-print\n"));
	qse_fprintf (out, QSE_T(" -e                 script specify a script\n"));
	qse_fprintf (out, QSE_T(" -f                 file   specify a script file\n"));
	qse_fprintf (out, QSE_T(" -o                 file   specify an output file\n"));
	qse_fprintf (out, QSE_T(" -r                        use the extended regular expression\n"));
	qse_fprintf (out, QSE_T(" -R                        enable non-standard extensions to the regular\n"));
	qse_fprintf (out, QSE_T("                           expression\n"));
	qse_fprintf (out, QSE_T(" -i                        perform in-place editing. imply -s\n"));
	qse_fprintf (out, QSE_T(" -s                        process input files separately\n"));
	qse_fprintf (out, QSE_T(" -a                        perform strict address and label check\n"));
	qse_fprintf (out, QSE_T(" -b                        allow extended address formats\n"));
	qse_fprintf (out, QSE_T("                           <start~step>,<start,+line>,<start,~line>,<0,/regex/>\n"));
	qse_fprintf (out, QSE_T(" -x                        allow text on the same line as c, a, i\n"));
	qse_fprintf (out, QSE_T(" -y                        ensure a newline at text end\n"));
	qse_fprintf (out, QSE_T(" -m                 number specify the maximum amount of memory to use in bytes\n"));
	qse_fprintf (out, QSE_T(" -w                        expand file wildcards\n"));
#if defined(QSE_ENABLE_SEDTRACER)
	qse_fprintf (out, QSE_T(" -t                        print command traces\n"));
#endif
#if defined(QSE_BUILD_DEBUG)
	qse_fprintf (out, QSE_T(" -X                 number fail the number'th memory allocation\n"));
#endif
#if defined(QSE_CHAR_IS_WCHAR)
     qse_fprintf (out, QSE_T(" --script-encoding  string specify script file encoding name\n"));
     qse_fprintf (out, QSE_T(" --infile-encoding  string specify input file encoding name\n"));
     qse_fprintf (out, QSE_T(" --outfile-encoding string specify output file encoding name\n"));
#endif
}

static int add_script (const qse_char_t* str, int mem)
{
	if (g_script.size >= g_script.capa)
	{
		qse_sed_iostd_t* tmp;

		tmp = QSE_MMGR_REALLOC (
			QSE_MMGR_GETDFL(), 
			g_script.io, 
			QSE_SIZEOF(*g_script.io) * (g_script.capa + 16 + 1));
		if (tmp == QSE_NULL) 
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: out of memory while processing %s\n"), str);
			return -1;
		}

		g_script.io = tmp;
		g_script.capa += 16;
	} 

	if (mem)
	{
		g_script.io[g_script.size].type = QSE_SED_IOSTD_STR;
		/* though its type is not qualified to be const, 
		 * u.mem.ptr is actually const when used for input */
		g_script.io[g_script.size].u.str.ptr = (qse_char_t*)str;
		g_script.io[g_script.size].u.str.len = qse_strlen(str);
	}
	else
	{
		g_script.io[g_script.size].type = QSE_SED_IOSTD_FILE;
		g_script.io[g_script.size].u.file.path = 
			(qse_strcmp (str, QSE_T("-")) == 0)? QSE_NULL: str;
		g_script.io[g_script.size].u.file.cmgr = g_script_cmgr;
	}
	g_script.size++;
	return 0;
}

static void free_scripts (void)
{
	if (g_script.io)
	{
		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), g_script.io);
		g_script.io = QSE_NULL;
		g_script.capa = 0;
		g_script.size = 0;
	}
}

static int handle_args (int argc, qse_char_t* argv[])
{
	static qse_opt_lng_t lng[] = 
	{
#if defined(QSE_CHAR_IS_WCHAR)
		{ QSE_T(":script-encoding"),  QSE_T('\0') },
		{ QSE_T(":infile-encoding"),  QSE_T('\0') },
		{ QSE_T(":outfile-encoding"), QSE_T('\0') },
#endif

		{ QSE_T("version"),          QSE_T('\0') },
		{ QSE_T("help"),             QSE_T('h') },
		{ QSE_NULL,                  QSE_T('\0') }                  
	};
	static qse_opt_t opt = 
	{
#if defined(QSE_BUILD_DEBUG)
		QSE_T("hne:f:o:rRisabxytm:wX:"),
#else
		QSE_T("hne:f:o:rRisabxytm:w"),
#endif
		lng
	};
	qse_cint_t c;

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			default:
				print_usage (QSE_STDERR, argc, argv);
				goto oops;

			case QSE_T('?'):
				qse_fprintf (QSE_STDERR, 
					QSE_T("ERROR: bad option - %c\n"),
					opt.opt
				);
				print_usage (QSE_STDERR, argc, argv);
				goto oops;

			case QSE_T(':'):
				qse_fprintf (QSE_STDERR, 
					QSE_T("ERROR: bad parameter for %c\n"),
					opt.opt
				);
				print_usage (QSE_STDERR, argc, argv);
				goto oops;

			case QSE_T('h'):
				print_usage (QSE_STDOUT, argc, argv);
				goto done;

			case QSE_T('n'):
				g_option |= QSE_SED_QUIET;
				break;

			case QSE_T('e'):
				if (add_script (opt.arg, 1) <= -1) goto oops;
				break;

			case QSE_T('f'):
				if (add_script (opt.arg, 0) <= -1) goto oops;
				break;

			case QSE_T('o'):
				g_output_file = opt.arg;
				break;

			case QSE_T('r'):
				g_option |= QSE_SED_EXTENDEDREX;
				break;

			case QSE_T('R'):
				g_option |= QSE_SED_NONSTDEXTREX;
				break;

			case QSE_T('i'):
				/* 'i' implies 's'. */
				g_inplace = 1;

			case QSE_T('s'):
				g_separate = 1;
				break;
		
			case QSE_T('a'):
				g_option |= QSE_SED_STRICT;
				break;

			case QSE_T('b'):
				g_option |= QSE_SED_EXTENDEDADR;
				break;

			case QSE_T('x'):
				g_option |= QSE_SED_SAMELINE;
				break;

			case QSE_T('y'):
				g_option |= QSE_SED_ENSURENL;
				break;

			case QSE_T('t'):
#if defined(QSE_ENABLE_SEDTRACER)
				g_trace = 1;
				break;
#else
				print_usage (QSE_STDERR, argc, argv);
				goto oops;
#endif
 
			case QSE_T('m'):
				g_memlimit = qse_strtoulong (opt.arg);
				break;

			case QSE_T('w'):
				g_wildcard = 1;
				break;

#if defined(QSE_BUILD_DEBUG)
			case QSE_T('X'):
                    g_failmalloc = qse_strtoulong (opt.arg);
				break;
#endif

			case QSE_T('\0'):
			{
				if (qse_strcmp(opt.lngopt, QSE_T("version")) == 0)
				{
					print_version ();
                         goto done;
                    }
				else if (qse_strcmp(opt.lngopt, QSE_T("script-encoding")) == 0)
				{
					g_script_cmgr = qse_findcmgr (opt.arg);
					if (g_script_cmgr == QSE_NULL)
					{
						qse_fprintf (QSE_STDERR, QSE_T("ERROR: unknown script encoding - %s\n"), opt.arg);
						goto oops;
					}
				}
				else if (qse_strcmp(opt.lngopt, QSE_T("infile-encoding")) == 0)
				{
					g_infile_cmgr = qse_findcmgr (opt.arg);
					if (g_infile_cmgr == QSE_NULL)
					{
						qse_fprintf (QSE_STDERR, QSE_T("ERROR: unknown input file encoding - %s\n"), opt.arg);
						goto oops;
					}
				}
				else if (qse_strcmp(opt.lngopt, QSE_T("outfile-encoding")) == 0)
				{
					g_outfile_cmgr = qse_findcmgr (opt.arg);
					if (g_outfile_cmgr == QSE_NULL)
					{
						qse_fprintf (QSE_STDERR, QSE_T("ERROR: unknown output file encoding - %s\n"), opt.arg);
						goto oops;
					}
				}
				break;
			}

		}
	}

	if (opt.ind < argc && g_script.size <= 0) 
	{
		if (add_script (argv[opt.ind++], 1) <= -1) goto oops;
	}
	if (opt.ind < argc) g_infile_pos = opt.ind;

	if (g_script.size <= 0)
	{
		print_usage (QSE_STDERR, argc, argv);
		goto oops;
	}

	g_script.io[g_script.size].type = QSE_SED_IOSTD_NULL;
	return 1;

oops:
	free_scripts ();
	return -1;

done:
	free_scripts ();
	return 0;
}

void print_exec_error (qse_sed_t* sed)
{
	const qse_sed_loc_t* errloc = qse_sed_geterrloc(sed);
	if (errloc->line > 0 || errloc->colm > 0)
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: cannot execute - %s at line %lu column %lu\n"),
			qse_sed_geterrmsg(sed),
			(unsigned long)errloc->line,
			(unsigned long)errloc->colm
		);
	}
	else
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: cannot execute - %s\n"),
			qse_sed_geterrmsg(sed)
		);
	}
}

#if defined(_WIN32)
static BOOL WINAPI stop_run (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		qse_sed_stop (g_sed);
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

			qse_sed_stop (g_sed);
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
	qse_sed_stop (g_sed);
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
	qse_sed_stop (g_sed);
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

#if defined(QSE_ENABLE_SEDTRACER)
static void trace_exec (qse_sed_t* sed, qse_sed_exec_op_t op, const qse_sed_cmd_t* cmd)
{
	switch (op)
	{
		case QSE_SED_EXEC_READ:
			/*qse_fprintf (QSE_STDERR, QSE_T("reading...\n"));*/
			break;
		case QSE_SED_EXEC_WRITE:
			/*qse_fprintf (QSE_STDERR, QSE_T("wrting...\n"));*/
			break;

		/* TODO: use function to get hold space and pattern space and print them */

		case QSE_SED_EXEC_MATCH:
			qse_fprintf (QSE_STDERR, QSE_T("%s:%lu [%c] MA\n"), 
				((cmd->lid && cmd->lid[0])? cmd->lid: QSE_T("<<UNKNOWN>>")), 
				(unsigned long)cmd->loc.line,
				cmd->type
			);
			break;

		case QSE_SED_EXEC_EXEC:
			qse_fprintf (QSE_STDERR, QSE_T("%s:%lu [%c] EC\n"), 
				((cmd->lid && cmd->lid[0])? cmd->lid: QSE_T("<<UNKNOWN>>")), 
				(unsigned long)cmd->loc.line,
				cmd->type
			);
			break;
	}
}
#endif

struct xarg_t
{
	qse_mmgr_t*  mmgr;
	qse_char_t** ptr;
	qse_size_t   size;
	qse_size_t   capa;
};

typedef struct xarg_t xarg_t;

static int collect_into_xarg (const qse_cstr_t* path, void* ctx)
{
	xarg_t* xarg = (xarg_t*)ctx;

	if (xarg->size <= xarg->capa)
	{
		qse_char_t** tmp;

		tmp = QSE_MMGR_REALLOC (xarg->mmgr, xarg->ptr, QSE_SIZEOF(*tmp) * (xarg->capa + 128));
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

static int expand_wildcards (int argc, qse_char_t* argv[], int glob, xarg_t* xarg)
{
	int i;
	qse_cstr_t tmp;

	for (i = 0; i < argc; i++)
	{
		int x;

		if (glob)
		{
			x = qse_glob (argv[i], collect_into_xarg, xarg, 
				QSE_GLOB_NOESCAPE | QSE_GLOB_PERIOD | QSE_GLOB_IGNORECASE, 
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

	return 0;
}

static int sed_main (int argc, qse_char_t* argv[])
{
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL();
	qse_sed_t* sed = QSE_NULL;
	qse_fs_t* fs = QSE_NULL;
	qse_size_t script_count;
	int ret = -1;

	xarg_t xarg;
	int xarg_inited = 0;

	ret = handle_args (argc, argv);
	if (ret <= -1) return -1;
	if (ret == 0) return 0;

	ret = -1;

#if defined(QSE_BUILD_DEBUG)
	if (g_failmalloc > 0)
	{
		debug_mmgr.ctx = QSE_NULL;
		mmgr = &debug_mmgr;
	}
	else
#endif
	if (g_memlimit > 0)
	{
		xma_mmgr.ctx = qse_xma_open (QSE_MMGR_GETDFL(), 0, g_memlimit);
		if (xma_mmgr.ctx == QSE_NULL)
		{
			qse_printf (QSE_T("ERROR: cannot open memory heap\n"));
			goto oops;
		}
		mmgr = &xma_mmgr;
	}

	if (g_separate && g_infile_pos > 0 && g_inplace)
	{
		fs = qse_fs_open (mmgr, 0);
		if (fs == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open file system handler\n"));
			goto oops;
		}

		if (qse_fs_chdir (fs, QSE_T(".")) <= -1)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("ERROR: cannot change direcotry in file system handler\n"));
			goto oops;
		}
	}

	sed = qse_sed_openstdwithmmgr (mmgr, 0);
	if (sed == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open stream editor\n"));
		goto oops;
	}

	qse_sed_setoption (sed, g_option);

	if (qse_sed_compstd (sed, g_script.io, &script_count) <= -1)
	{
		const qse_sed_loc_t* errloc = qse_sed_geterrloc(sed);
		const qse_char_t* target;
		qse_char_t exprbuf[128];
	
		if (g_script.io[script_count].type == QSE_SED_IOSTD_FILE)
		{
			target = g_script.io[script_count].u.file.path;
		}
		else 
		{
			/* i dont' use QSE_SED_IOSTD_SIO for input */	
			QSE_ASSERT (g_script.io[script_count].type == QSE_SED_IOSTD_STR);
			qse_sprintf (exprbuf, QSE_COUNTOF(exprbuf), 
				QSE_T("expression #%lu"), (unsigned long)script_count);
			target = exprbuf;
		}

		if (errloc->line > 0 || errloc->colm > 0)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("ERROR: cannot compile %s - %s at line %lu column %lu\n"),
				target,
				qse_sed_geterrmsg(sed),
				(unsigned long)errloc->line,
				(unsigned long)errloc->colm
			);
		}
		else
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("ERROR: cannot compile %s - %s\n"),
				target,
				qse_sed_geterrmsg(sed)
			);
		}
		goto oops;
	}

#if defined(QSE_ENABLE_SEDTRACER)
	if (g_trace) qse_sed_setexectracer (sed, trace_exec);
#endif

	qse_memset (&xarg, 0, QSE_SIZEOF(xarg));
	xarg.mmgr = qse_sed_getmmgr(sed);	
	xarg_inited = 1;

	if (g_separate && g_infile_pos > 0)
	{
		/* 's' and input files are specified on the command line */
		qse_sed_iostd_t out_file;
		qse_sed_iostd_t out_inplace;
		qse_sed_iostd_t* output_file = QSE_NULL;
		qse_sed_iostd_t* output = QSE_NULL;
		int inpos;

		if (g_output_file && 
		    qse_strcmp (g_output_file, QSE_T("-")) != 0)
		{
			out_file.type = QSE_SED_IOSTD_SIO;
			out_file.u.sio = qse_sio_open (
				qse_sed_getmmgr(sed),
				0,
				g_output_file,
				QSE_SIO_WRITE |
				QSE_SIO_CREATE |
				QSE_SIO_TRUNCATE |
				QSE_SIO_IGNOREMBWCERR
			);
			if (out_file.u.sio == QSE_NULL)
			{
				qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open %s\n"), g_output_file);
				goto oops;
			}

			output_file = &out_file;
			output = output_file;
		}

		/* perform wild-card expansions for non-unix platforms */
		if (expand_wildcards (argc - g_infile_pos, &argv[g_infile_pos], g_wildcard, &xarg) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: out of memory\n"));
			goto oops;
		}

		for (inpos = 0; inpos < xarg.size; inpos++)
		{
			qse_sed_iostd_t in[2];
			qse_char_t* tmpl_tmpfile;
			
			in[0].type = QSE_SED_IOSTD_FILE;
			in[0].u.file.path =
				(qse_strcmp (xarg.ptr[inpos], QSE_T("-")) == 0)?  QSE_NULL: xarg.ptr[inpos];
			in[0].u.file.cmgr = g_infile_cmgr;
			in[1].type = QSE_SED_IOSTD_NULL;

			tmpl_tmpfile = QSE_NULL;
			if (g_inplace && in[0].u.file.path)
			{
				int retried = 0;

				tmpl_tmpfile = qse_strdup2 (in[0].u.file.path, QSE_T(".XXXX"),  qse_sed_getmmgr(sed));
				if (tmpl_tmpfile == QSE_NULL)
				{
					qse_fprintf (QSE_STDERR, QSE_T("ERROR: out of memory\n"));
					goto oops;
				}

			open_temp:
				out_inplace.type = QSE_SED_IOSTD_SIO;
				out_inplace.u.sio = qse_sio_open (
					qse_sed_getmmgr(sed),
					0,
					tmpl_tmpfile,
					QSE_SIO_WRITE |
					QSE_SIO_CREATE |
					QSE_SIO_IGNOREMBWCERR |
					QSE_SIO_EXCLUSIVE |
					QSE_SIO_TEMPORARY
				);
				if (out_inplace.u.sio == QSE_NULL)
				{
					if (retried) 
					{
						qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open %s\n"), tmpl_tmpfile);
						QSE_MMGR_FREE (qse_sed_getmmgr(sed), tmpl_tmpfile);
						goto oops;
					}
					else
					{
						/* retry to open the file with shorter names */
						QSE_MMGR_FREE (qse_sed_getmmgr(sed), tmpl_tmpfile);
						tmpl_tmpfile = qse_strdup (QSE_T("TMP-XXXX"),  qse_sed_getmmgr(sed));
						if (tmpl_tmpfile == QSE_NULL)
						{
							qse_fprintf (QSE_STDERR, QSE_T("ERROR: out of memory\n"));
							goto oops;
						}
						retried = 1;
						goto open_temp;
					}
				}

				output = &out_inplace;
			}

			if (qse_sed_execstd (sed, in, output) <= -1)
			{
				if (output) qse_sio_close (output->u.sio);

				if (tmpl_tmpfile) 
				{
					qse_fs_delete (fs, tmpl_tmpfile);
					QSE_MMGR_FREE (qse_sed_getmmgr(sed), tmpl_tmpfile);
				}
				print_exec_error (sed);
				goto oops;
			}

			if (tmpl_tmpfile)
			{
				QSE_ASSERT (output == &out_inplace);
				qse_sio_close (output->u.sio);
				output = output_file;

				if (qse_fs_move (fs, tmpl_tmpfile, in[0].u.file.path) <= -1)
				{
					qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot rename %s to %s. not deleting %s\n"), 
						tmpl_tmpfile, in[0].u.file.path, tmpl_tmpfile);
					QSE_MMGR_FREE (qse_sed_getmmgr(sed), tmpl_tmpfile);
					goto oops;
				}

				QSE_MMGR_FREE (qse_sed_getmmgr(sed), tmpl_tmpfile);
			}

			if (qse_sed_isstop (sed)) break;
		}

		if (output) qse_sio_close (output->u.sio);
	}
	else
	{
		int xx;
		qse_sed_iostd_t* in = QSE_NULL;
		qse_sed_iostd_t out;

		if (g_infile_pos > 0)
		{
			int i;
			const qse_char_t* tmp;

			/* input files are specified on the command line */

			/* perform wild-card expansions for non-unix platforms */
			if (expand_wildcards (argc - g_infile_pos, &argv[g_infile_pos], g_wildcard, &xarg) <= -1)
			{
				qse_fprintf (QSE_STDERR, QSE_T("ERROR: out of memory\n"));
				goto oops;
			}

			in = QSE_MMGR_ALLOC (qse_sed_getmmgr(sed), QSE_SIZEOF(*in) * (xarg.size + 1));
			if (in == QSE_NULL)
			{
				qse_fprintf (QSE_STDERR, QSE_T("ERROR: out of memory\n"));
				goto oops;
			}

			for (i = 0; i < xarg.size; i++)
			{
				in[i].type = QSE_SED_IOSTD_FILE;
				tmp = xarg.ptr[i];
				in[i].u.file.path =
					(qse_strcmp (tmp, QSE_T("-")) == 0)? QSE_NULL: tmp;
				in[i].u.file.cmgr = g_infile_cmgr;
			}

			in[i].type = QSE_SED_IOSTD_NULL;
		}

		if (g_output_file)
		{
			out.type = QSE_SED_IOSTD_FILE;
			out.u.file.path = 
				(qse_strcmp (g_output_file, QSE_T("-")) == 0)? 
				QSE_NULL: g_output_file;
			out.u.file.cmgr = g_outfile_cmgr;
		}
		else
		{
			/* arrange to be able to specify cmgr.
			 * if not for cmgr, i could simply pass QSE_NULL 
			 * to qse_sed_execstd() below like
			 *   xx = qse_sed_execstd (sed, in, QSE_NULL); */
			out.type = QSE_SED_IOSTD_FILE;
			out.u.file.path = QSE_NULL;
			out.u.file.cmgr = g_outfile_cmgr;
		}

		g_sed = sed;
		set_intr_run ();
		xx = qse_sed_execstd (sed, in, &out);
		if (in) QSE_MMGR_FREE (qse_sed_getmmgr(sed), in);
		unset_intr_run ();
		g_sed = QSE_NULL;

		if (xx <= -1)
		{
			print_exec_error (sed);
			goto oops;
		}
	}

	ret = 0;

oops:
	if (xarg_inited) purge_xarg (&xarg);
	if (sed) qse_sed_close (sed);
	if (fs) qse_fs_close (fs);
	if (xma_mmgr.ctx) qse_xma_close (xma_mmgr.ctx);
	free_scripts ();

#if defined(QSE_BUILD_DEBUG)
	if (g_failmalloc > 0)
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

int qse_main (int argc, qse_achar_t* argv[])
{
#if defined(_WIN32)
	char locale[100];
	UINT codepage = GetConsoleOutputCP();	
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgr (qse_utf8cmgr);
	}
	else
	{
		sprintf (locale, ".%u", (unsigned int)codepage);
		setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	return qse_runmain (argc, argv, sed_main);
}
