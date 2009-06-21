/*
 * $Id: awk.c 205 2009-06-20 12:47:34Z hyunghwan.chung $
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

#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>

#define ABORT(label) goto label

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	include <process.h>
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
	qse_map_t*   gvm;  /* global variable map */
	qse_char_t*  fs;   /* field separator */
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
	qse_awk_rtx_t* rtx = (qse_awk_rtx_t*)arg;
	qse_char_t* str;
	qse_size_t len;
	qse_awk_errinf_t oerrinf;

	qse_awk_rtx_geterrinf (rtx, &oerrinf);

	str = qse_awk_rtx_valtocpldup (rtx, QSE_MAP_VPTR(pair), &len);
	if (str == QSE_NULL)
	{
		if (qse_awk_rtx_geterrnum(rtx) == QSE_AWK_EVALTYPE)
		{
			dprint (QSE_T("%.*s = [not printable]\n"), 
				(int)QSE_MAP_KLEN(pair), QSE_MAP_KPTR(pair));

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
			(int)QSE_MAP_KLEN(pair), QSE_MAP_KPTR(pair), 
			(int)len, str);
		qse_awk_free (qse_awk_rtx_getawk(rtx), str);
	}

	return QSE_MAP_WALK_FORWARD;
}

static void on_run_statement (
	qse_awk_rtx_t* run, qse_size_t line, void* data)
{
	/*dprint (L"running %d\n", (int)line);*/
}

static qse_map_walk_t set_global (
	qse_map_t* map, qse_map_pair_t* pair, void* arg)
{
	qse_awk_val_t* v;
	qse_awk_rtx_t* rtx = (qse_awk_rtx_t*)arg;
	struct gvmv_t* gvmv = (struct gvmv_t*)QSE_MAP_VPTR(pair);

	v = qse_awk_rtx_makenstrval (rtx, gvmv->ptr, gvmv->len);
	if (v == QSE_NULL) return QSE_MAP_WALK_STOP;

	qse_awk_rtx_refupval (rtx, v);
	qse_awk_rtx_setgbl (rtx, gvmv->idx, v);
	qse_awk_rtx_refdownval (rtx, v);

	return QSE_MAP_WALK_FORWARD;
}

static int on_run_enter (qse_awk_rtx_t* rtx, void* data)
{
	struct arg_t* arg = (struct arg_t*)data;

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
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOERR);
		qse_map_walk (arg->gvm, set_global, rtx);
		if (qse_awk_rtx_geterrnum(rtx) != QSE_AWK_ENOERR) return -1;
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
	qse_printf (QSE_T(" -v/--assign          var=value    add a global variable with a value\n"));

	qse_printf (QSE_T("\nYou may specify the following options to change the behavior of the interpreter.\n"));
	for (j = 0; j < QSE_COUNTOF(otab); j++)
	{
		qse_printf (QSE_T("    --%-20s --no%-20s\n"), otab[j].name, otab[j].name);
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

static int comparg (int argc, qse_char_t* argv[], struct arg_t* arg)
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
		{ QSE_T("stripspaces"),      0 },
		{ QSE_T("nextofile"),        0 },
		{ QSE_T("crlf"),             0 },
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

	qse_size_t argl = 0;
	qse_size_t icfc = 0; /* the capacity of icf */
	qse_size_t icfl = 0; /* the number of input console files */

	qse_char_t** isf = QSE_NULL; /* input source files */
	qse_char_t*  osf = QSE_NULL; /* output source file */
	qse_char_t** icf = QSE_NULL; /* input console files */

	qse_map_t* gvm = QSE_NULL;  /* global variable map */
	qse_char_t* fs = QSE_NULL; /* field separator */

	memset (arg, 0, QSE_SIZEOF(*arg));

	isf = (qse_char_t**) malloc (QSE_SIZEOF(*isf) * isfc);
	if (isf == QSE_NULL)
	{
		out_of_memory ();
		goto oops;
	}

	gvm = qse_map_open (QSE_NULL, 0, 30, 70); 
	if (gvm == QSE_NULL)
	{
		out_of_memory ();
		goto oops;
	}
	/*qse_map_setcopier (gvm, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);*/
	qse_map_setscale (gvm, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));
	qse_map_setcopier (gvm, QSE_MAP_VAL, QSE_MAP_COPIER_INLINE);
	qse_map_setscale (gvm, QSE_MAP_VAL, QSE_SIZEOF(struct gvmv_t));

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			case 0:
				/* TODO: handle long options ... */
				qse_printf (QSE_T(">>> [%s] [%s]\n"), opt.lngopt, opt.arg);
				break;

			case QSE_T('h'):
				print_usage (argv[0]);
				if (isf != QSE_NULL) free (isf);
				if (gvm != QSE_NULL) qse_map_close (gvm);
				return 0;

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
					/* INVALID VALUE... */
					goto oops;
				}

				*eq = QSE_T('\0');

				gvmv.idx = -1;
				gvmv.ptr = ++eq;
				gvmv.len = qse_strlen(eq);

				if (qse_map_upsert (gvm, opt.arg, qse_strlen(opt.arg), &gvmv, 1) == QSE_NULL)
				{
					out_of_memory ();
					goto oops;
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

				goto oops;
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
			out_of_memory ();
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

	return 1;

oops:
	if (gvm != QSE_NULL) qse_map_close (gvm);
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
	if (arg->gvm != QSE_NULL) qse_map_close (arg->gvm);
}

static void print_awkerr (qse_awk_t* awk)
{
	qse_printf (
		QSE_T("ERROR: CODE [%d] LINE [%u] %s\n"), 
		qse_awk_geterrnum(awk),
		(unsigned int)qse_awk_geterrlin(awk), 
		qse_awk_geterrmsg(awk)
	);
}

static void print_rtxerr (qse_awk_rtx_t* rtx)
{
	qse_printf (
		QSE_T("ERROR: CODE [%d] LINE [%u] %s\n"),
		qse_awk_rtx_geterrnum(rtx),
		(unsigned int)qse_awk_rtx_geterrlin(rtx),
		qse_awk_rtx_geterrmsg(rtx)
	);
}

qse_map_walk_t add_global (qse_map_t* map, qse_map_pair_t* pair, void* arg)
{
	qse_awk_t* awk = (qse_awk_t*)arg;
	struct gvmv_t* gvmv = (struct gvmv_t*)QSE_MAP_VPTR(pair);

	gvmv->idx = qse_awk_addgbl (awk, QSE_MAP_KPTR(pair), QSE_MAP_KLEN(pair));
	if (gvmv->idx <= -1)
	{
		return QSE_MAP_WALK_STOP;
	}

	return QSE_MAP_WALK_FORWARD;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	qse_awk_t* awk;
	qse_awk_rtx_t* rtx;
	qse_awk_rcb_t rcb;

	int i;
	struct arg_t arg;
	int ret = -1;

	/* TODO: change it to support multiple source files */
	qse_awk_parsestd_in_t psin;
	qse_awk_parsestd_out_t psout;

	qse_memset (&arg, 0, QSE_SIZEOF(arg));

	i = comparg (argc, argv, &arg);
	if (i <= -1)
	{
		print_usage (argv[0]);
		return -1;
	}
	if (i == 0) return 0;

	psin.type = arg.ist;
	if (arg.ist == QSE_AWK_PARSESTD_CP) psin.u.cp = arg.isp.str;
	else psin.u.file = arg.isp.files[0];

	if (arg.osf != QSE_NULL)
	{
		psout.type = arg.ost;
		psout.u.file = arg.osf;
	}

	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)
	{
		qse_printf (QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

	/* TODO: get depth from command line */
	qse_awk_setmaxdepth (
		awk, QSE_AWK_DEPTH_BLOCK_PARSE | QSE_AWK_DEPTH_EXPR_PARSE, 50);
	qse_awk_setmaxdepth (
		awk, QSE_AWK_DEPTH_BLOCK_RUN | QSE_AWK_DEPTH_EXPR_RUN, 500);

	if (qse_awk_addfnc (awk, 
		QSE_T("sleep"), 5, 0,
		1, 1, QSE_NULL, fnc_sleep) == QSE_NULL)
	{
		print_awkerr (awk);
		goto oops;
	}

	qse_awk_seterrnum (awk, QSE_AWK_ENOERR);
	qse_map_walk (arg.gvm, add_global, awk);
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

	rcb.on_enter = on_run_enter;
	rcb.on_statement = on_run_statement;
	rcb.on_exit = on_run_exit;
	rcb.data = &arg;

	rtx = qse_awk_rtx_openstd (awk, 0, QSE_T("qseawk"), arg.icf, QSE_NULL);
	if (rtx == QSE_NULL) 
	{
		print_awkerr (awk);
		goto oops;
	}

	app_rtx = rtx;
	qse_awk_rtx_setrcb (rtx, &rcb);

	set_intr_run ();
	ret = qse_awk_rtx_loop (rtx);
	unset_intr_run ();

	if (ret == -1)
	{
		print_rtxerr (rtx);
		goto oops;
	}

oops:
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	if (awk != QSE_NULL) qse_awk_close (awk);

	freearg (&arg);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, awk_main);
}
