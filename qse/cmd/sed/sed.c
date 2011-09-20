/**
 * $Id$
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

#include <qse/sed/std.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/xma.h>
#include <qse/cmn/misc.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

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

static const qse_char_t* g_script_file = QSE_NULL;
static qse_char_t* g_script = QSE_NULL;
static qse_char_t* g_output_file = QSE_NULL;
static int g_infile_pos = 0;
static int g_option = 0;
static int g_separate = 0;
static qse_ulong_t g_memlimit = 0;
static qse_sed_t* g_sed = QSE_NULL;

#if defined(QSE_BUILD_DEBUG)
#include <stdlib.h>
static qse_ulong_t g_failmalloc;
static qse_ulong_t debug_mmgr_count = 0;

static void* debug_mmgr_alloc (void* ctx, qse_size_t size)
{
	debug_mmgr_count++;
	if (debug_mmgr_count % g_failmalloc == 0) return QSE_NULL;
	return malloc (size);
}

static void* debug_mmgr_realloc (void* ctx, void* ptr, qse_size_t size)
{
	debug_mmgr_count++;
	if (debug_mmgr_count % g_failmalloc == 0) return QSE_NULL;
	return realloc (ptr, size);
}

static void debug_mmgr_free (void* ctx, void* ptr)
{
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

static void print_usage (QSE_FILE* out, int argc, qse_char_t* argv[])
{
	const qse_char_t* b = qse_basename (argv[0]);

	qse_fprintf (out, QSE_T("USAGE: %s [options] script [file]\n"), b);
	qse_fprintf (out, QSE_T("       %s [options] -f script-file [file]\n"), b);

	qse_fprintf (out, QSE_T("options as follows:\n"));
	qse_fprintf (out, QSE_T(" -h        show this message\n"));
	qse_fprintf (out, QSE_T(" -n        disable auto-print\n"));
	qse_fprintf (out, QSE_T(" -f file   specify a script file\n"));
	qse_fprintf (out, QSE_T(" -o file   specify an output file\n"));
	qse_fprintf (out, QSE_T(" -r        use the extended regular expression\n"));
	qse_fprintf (out, QSE_T(" -R        enable non-standard extensions to the regular expression\n"));
	qse_fprintf (out, QSE_T(" -s        processes input files separately\n"));
	qse_fprintf (out, QSE_T(" -a        perform strict address and label check\n"));
	qse_fprintf (out, QSE_T(" -w        allow address format of start~step\n"));
	qse_fprintf (out, QSE_T(" -x        allow text on the same line as c, a, i\n"));
	qse_fprintf (out, QSE_T(" -y        ensure a newline at text end\n"));
	qse_fprintf (out, QSE_T(" -z        allow 0,/regex/ address\n"));
	qse_fprintf (out, QSE_T(" -m number specify the maximum amount of memory to use in bytes\n"));
#if defined(QSE_BUILD_DEBUG)
	qse_fprintf (out, QSE_T(" -X number fail the number'th memory allocation\n"));
#endif
}

static int handle_args (int argc, qse_char_t* argv[])
{
	static qse_opt_t opt = 
	{
#if defined(QSE_BUILD_DEBUG)
		QSE_T("hnf:o:rRsawxyzm:X:"),
#else
		QSE_T("hnf:o:rRsawxyzm:"),
#endif
		QSE_NULL
	};
	qse_cint_t c;

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			default:
				print_usage (QSE_STDERR, argc, argv);
				return -1;

			case QSE_T('?'):
				qse_fprintf (QSE_STDERR, 
					QSE_T("ERROR: bad option - %c\n"),
					opt.opt
				);
				print_usage (QSE_STDERR, argc, argv);
				return -1;

			case QSE_T(':'):
				qse_fprintf (QSE_STDERR, 
					QSE_T("ERROR: bad parameter for %c\n"),
					opt.opt
				);
				print_usage (QSE_STDERR, argc, argv);
				return -1;

			case QSE_T('h'):
				print_usage (QSE_STDOUT, argc, argv);
				return 0;

			case QSE_T('n'):
				g_option |= QSE_SED_QUIET;
				break;

			case QSE_T('f'):
				g_script_file = opt.arg;
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

			case QSE_T('s'):
				g_separate = 1;
				break;

			case QSE_T('a'):
				g_option |= QSE_SED_STRICT;
				break;

			case QSE_T('w'):
				g_option |= QSE_SED_STARTSTEP;
				break;

			case QSE_T('x'):
				g_option |= QSE_SED_SAMELINE;
				break;

			case QSE_T('y'):
				g_option |= QSE_SED_ENSURENL;
				break;

			case QSE_T('z'):
				g_option |= QSE_SED_ZEROA1;
				break;

			case QSE_T('m'):
				g_memlimit = qse_strtoulong (opt.arg);
				break;

#if defined(QSE_BUILD_DEBUG)
			case QSE_T('X'):
                    g_failmalloc = qse_strtoulong (opt.arg);
				break;
#endif
		}
	}

	if (opt.ind < argc && g_script_file == QSE_NULL) 
		g_script = argv[opt.ind++];
	if (opt.ind < argc) g_infile_pos = opt.ind;

	if (g_script_file == QSE_NULL && g_script == QSE_NULL)
	{
		print_usage (QSE_STDERR, argc, argv);
		return -1;
	}


	return 1;
}

qse_char_t* load_script_file (qse_sed_t* sed, const qse_char_t* file)
{
	qse_str_t script;
	qse_sio_t* fp;
	qse_xstr_t xstr;
	qse_char_t buf[256];

	fp = qse_sio_open (
		qse_sed_getmmgr(sed), 0, file, 
		QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
	if (fp == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open %s\n"), file);
		return QSE_NULL;
	}

	if (qse_str_init (&script, QSE_MMGR_GETDFL(), 1024) <= -1)
	{
		qse_sio_close (fp);
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot load %s\n"), file);
		return QSE_NULL;
	}

	while (1)
	{
		qse_ssize_t n;

		n = qse_sio_gets (fp, buf, QSE_COUNTOF(buf));
		if (n == 0) break;
		if (n <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot read %s\n"), file);
			qse_str_fini (&script);
			qse_sio_close (fp);
			return QSE_NULL;		
		}

		if (qse_str_ncat (&script, buf, n) == (qse_size_t)-1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: out of memory\n"));
			qse_str_fini (&script);
			qse_sio_close (fp);
			return QSE_NULL;
		}		
	}

	qse_str_yield (&script, &xstr, 0);
	qse_str_fini (&script);
	qse_sio_close (fp);

	return xstr.ptr;
}

void print_exec_error (qse_sed_t* sed)
{
	const qse_sed_loc_t* errloc = qse_sed_geterrloc(sed);
	if (errloc->line > 0 || errloc->colm > 0)
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("cannot execute - %s at line %lu column %lu\n"),
			qse_sed_geterrmsg(sed),
			(unsigned long)errloc->line,
			(unsigned long)errloc->colm
		);
	}
	else
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("cannot execute - %s\n"),
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

int sed_main (int argc, qse_char_t* argv[])
{
	qse_mmgr_t* mmgr = QSE_NULL;
	qse_sed_t* sed = QSE_NULL;
	int ret = -1;

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
		xma_mmgr.ctx = qse_xma_open (QSE_NULL, 0, g_memlimit);
		if (xma_mmgr.ctx == QSE_NULL)
		{
			qse_printf (QSE_T("ERROR: cannot open memory heap\n"));
			goto oops;
		}
		mmgr = &xma_mmgr;
	}

	sed = qse_sed_openstdwithmmgr (mmgr, 0);
	if (sed == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot open a stream editor\n"));
		goto oops;
	}
	
	qse_sed_setoption (sed, g_option);

	if (g_script_file)
	{
		QSE_ASSERT (g_script == QSE_NULL);

		g_script = load_script_file (sed, g_script_file);
		if (g_script == QSE_NULL) goto oops;
	}

	if (qse_sed_compstd (sed, g_script) == -1)
	{
		const qse_sed_loc_t* errloc = qse_sed_geterrloc(sed);
		if (errloc->line > 0 || errloc->colm > 0)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("cannot compile - %s at line %lu column %lu\n"),
				qse_sed_geterrmsg(sed),
				(unsigned long)errloc->line,
				(unsigned long)errloc->colm
			);
		}
		else
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("cannot compile - %s\n"),
				qse_sed_geterrmsg(sed)
			);
		}
		goto oops;
	}

	if (g_separate && g_infile_pos > 0)
	{
		qse_sed_iostd_t out;
		qse_sed_iostd_t* output = QSE_NULL;

		if (g_output_file && 
		    qse_strcmp (g_output_file, QSE_T("-")) != 0)
		{
			out.type = QSE_SED_IOSTD_SIO;
			out.u.sio = qse_sio_open (
				qse_sed_getmmgr(sed),
				0,
				g_output_file,
				QSE_SIO_WRITE |
				QSE_SIO_CREATE |
				QSE_SIO_TRUNCATE |
				QSE_SIO_IGNOREMBWCERR
			);
			if (out.u.sio == QSE_NULL)
			{
				qse_fprintf (QSE_STDERR, QSE_T("cannot open %s\n"), g_output_file);
				goto oops;
			}

			output = &out;
		}

		while (g_infile_pos < argc)
		{
			qse_sed_iostd_t in[2];
			
			in[0].type = QSE_SED_IOSTD_FILE;
			in[0].u.file =
				(qse_strcmp (argv[g_infile_pos], QSE_T("-")) == 0)? 
				QSE_NULL: argv[g_infile_pos];
			in[1].type = QSE_SED_IOSTD_NULL;

			if (qse_sed_execstd (sed, in, output) <= -1)
			{
				if (output) qse_sio_close (output->u.sio);
				print_exec_error (sed);
				goto oops;
			}
			if (qse_sed_isstop (sed)) break;

			g_infile_pos++;
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
			int i, num_ins;

			num_ins = argc - g_infile_pos;
			in = QSE_MMGR_ALLOC (qse_sed_getmmgr(sed), QSE_SIZEOF(*in) * (num_ins + 1));
			if (in == QSE_NULL)
			{
				qse_fprintf (QSE_STDERR, QSE_T("out of memory\n"));
				goto oops;
			}

			for (i = 0; i < num_ins; i++)
			{
				in[i].type = QSE_SED_IOSTD_FILE;
				in[i].u.file =
					(qse_strcmp (argv[g_infile_pos], QSE_T("-")) == 0)? 
					QSE_NULL: argv[g_infile_pos];
				g_infile_pos++;
			}

			in[i].type = QSE_SED_IOSTD_NULL;
		}

		if (g_output_file)
		{
			out.type = QSE_SED_IOSTD_FILE;
			out.u.file = 
				(qse_strcmp (g_output_file, QSE_T("-")) == 0)? 
				QSE_NULL: g_output_file;
		}

		g_sed = sed;
		set_intr_run ();
		xx = qse_sed_execstd (sed, in, (g_output_file? &out: QSE_NULL));
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
	if (sed) qse_sed_close (sed);
	if (xma_mmgr.ctx) qse_xma_close (xma_mmgr.ctx);
	if (g_script_file != QSE_NULL && g_script != QSE_NULL) 
		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), g_script);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, sed_main);
}
