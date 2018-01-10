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

#include <qse/xli/stdxli.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/xma.h>
#include <qse/cmn/path.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/fmt.h>
#include <qse/si/fs.h>
#include <qse/si/glob.h>
#include <qse/si/sio.h>

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
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include <unistd.h>
#	include <errno.h>
#endif

#define IO_FLAG_INI_INPUT (1 << 0)
#define IO_FLAG_INI_OUTPUT (1 << 1)
#define IO_FLAG_JSON_INPUT (1 << 2)
#define IO_FLAG_JSON_OUTPUT (1 << 3)

static int g_io_flags = 0;
static qse_char_t* g_input_file = QSE_NULL;
static qse_char_t* g_output_file = QSE_NULL;
static qse_char_t* g_lookup_key = QSE_NULL;
static qse_char_t* g_value = QSE_NULL;
static qse_ulong_t g_memlimit = 0;
static int g_trait = 0;

static qse_cmgr_t* g_infile_cmgr = QSE_NULL;
static qse_cmgr_t* g_outfile_cmgr = QSE_NULL;

#if defined(QSE_BUILD_DEBUG)
#include <stdlib.h>
static qse_ulong_t g_failmalloc = 0;
static qse_ulong_t debug_mmgr_count = 0;
static qse_ulong_t debug_mmgr_alloc_count = 0;
static qse_ulong_t debug_mmgr_realloc_count = 0;
static qse_ulong_t debug_mmgr_free_count = 0;

static void* debug_mmgr_alloc (qse_mmgr_t* mmgr, qse_size_t size)
{
	void* ptr;
	debug_mmgr_count++;
	if (debug_mmgr_count % g_failmalloc == 0) return QSE_NULL;
	ptr = malloc (size);
	if (ptr) debug_mmgr_alloc_count++;
	return ptr;
}

static void* debug_mmgr_realloc (qse_mmgr_t* mmgr, void* ptr, qse_size_t size)
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

static void debug_mmgr_free (qse_mmgr_t* mmgr, void* ptr)
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

static void* xma_alloc (qse_mmgr_t* mmgr, qse_size_t size)
{
	return qse_xma_alloc (mmgr->ctx, size);
}

static void* xma_realloc (qse_mmgr_t* mmgr, void* ptr, qse_size_t size)
{
	return qse_xma_realloc (mmgr->ctx, ptr, size);
}

static void xma_free (qse_mmgr_t* mmgr, void* ptr)
{
	qse_xma_free (mmgr->ctx, ptr);
}

static qse_mmgr_t xma_mmgr = 
{
	xma_alloc,
	xma_realloc,
	xma_free,
	QSE_NULL
};

static void print_version (void)
{
	qse_printf (QSE_T("QSEXLI version %hs\n"), QSE_PACKAGE_VERSION);
}

static void print_usage (qse_sio_t* out, int argc, qse_char_t* argv[])
{
	const qse_char_t* b = qse_basename (argv[0]);

	qse_fprintf (out, QSE_T("USAGE: %s [options] -i input-file [key [value]]\n"), b);

	qse_fprintf (out, QSE_T("options as follows:\n"));
	qse_fprintf (out, QSE_T(" -h/--help                 show this message\n"));
	qse_fprintf (out, QSE_T(" --version                 show version\n"));
	qse_fprintf (out, QSE_T(" -i                 file   specify an input file\n"));
	qse_fprintf (out, QSE_T(" -o                 file   specify an output file\n"));
	qse_fprintf (out, QSE_T(" -I                 file   specify an ini input file\n"));
	qse_fprintf (out, QSE_T(" -O                 file   specify an ini output file\n"));
	qse_fprintf (out, QSE_T(" -j                 file   specify a json input file\n"));
	qse_fprintf (out, QSE_T(" -J                 file   specify a json output file\n"));
	qse_fprintf (out, QSE_T(" -u                        disallow duplicate keys\n"));
	qse_fprintf (out, QSE_T(" -a                        allow a key alias\n"));
	qse_fprintf (out, QSE_T(" -b                        allow true and false as boolean values\n"));
	qse_fprintf (out, QSE_T(" -f                        keep file inclusion info\n"));
	qse_fprintf (out, QSE_T(" -t                        keep comment text\n"));
	qse_fprintf (out, QSE_T(" -s                        allow multi-segmented strings\n"));
	qse_fprintf (out, QSE_T(" -d                        allow a leading digit in identifiers\n"));
	qse_fprintf (out, QSE_T(" -n                        disallow nil\n"));
	qse_fprintf (out, QSE_T(" -l                        disallow lists\n"));
	qse_fprintf (out, QSE_T(" -K                        allow key tags\n"));
	qse_fprintf (out, QSE_T(" -S                        allow string tags\n"));
	qse_fprintf (out, QSE_T(" -v                        perform validation\n"));
	qse_fprintf (out, QSE_T(" -m                 number specify the maximum amount of memory to use in bytes\n"));
#if defined(QSE_BUILD_DEBUG)
	qse_fprintf (out, QSE_T(" -X                 number fail the number'th memory allocation\n"));
#endif
#if defined(QSE_CHAR_IS_WCHAR)
     qse_fprintf (out, QSE_T(" --infile-encoding  string specify input file encoding name\n"));
     qse_fprintf (out, QSE_T(" --outfile-encoding string specify output file encoding name\n"));
#endif
}

static int handle_args (int argc, qse_char_t* argv[])
{
	static qse_opt_lng_t lng[] = 
	{
#if defined(QSE_CHAR_IS_WCHAR)
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
		QSE_T("hi:o:I:O:j:J:uabftsdnlKSvm:X:"),
#else
		QSE_T("hi:o:I:O:j:J:uabftsdnlKSvm:"),
#endif
		lng
	};
	qse_cint_t c;

	while ((c = qse_getopt(argc, argv, &opt)) != QSE_CHAR_EOF)
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

			case QSE_T('i'):
				g_input_file = opt.arg;
				g_io_flags &= ~(IO_FLAG_INI_INPUT | IO_FLAG_JSON_OUTPUT);
				break;

			case QSE_T('I'):
				g_input_file = opt.arg;
				g_io_flags |= IO_FLAG_INI_INPUT;
				break;

			case QSE_T('j'):
				g_input_file = opt.arg;
				g_io_flags |= IO_FLAG_JSON_INPUT;
				break;

			case QSE_T('o'):
				g_output_file = opt.arg;
				g_io_flags &= ~(IO_FLAG_INI_OUTPUT | IO_FLAG_JSON_OUTPUT);
				break;

			case QSE_T('O'):
				g_output_file = opt.arg;
				g_io_flags |= IO_FLAG_INI_OUTPUT;
				break;

			case QSE_T('J'):
				g_output_file = opt.arg;
				g_io_flags |= IO_FLAG_JSON_OUTPUT;
				break;

			case QSE_T('u'):
				g_trait |= QSE_XLI_KEYNODUP;
				break;

			case QSE_T('a'):
				g_trait |= QSE_XLI_KEYALIAS;
				break;

			case QSE_T('b'):
				g_trait |= QSE_XLI_BOOLEAN;
				break;

			case QSE_T('f'):
				g_trait |= QSE_XLI_KEEPFILE;
				break;

			case QSE_T('t'):
				g_trait |= QSE_XLI_KEEPTEXT;
				break;

			case QSE_T('s'):
				g_trait |= QSE_XLI_MULSEGSTR;
				break;

			case QSE_T('d'):
				g_trait |= QSE_XLI_LEADDIGIT;
				break;

			case QSE_T('n'):
				g_trait |= QSE_XLI_NONIL;
				break;

			case QSE_T('l'):
				g_trait |= QSE_XLI_NOLIST;
				break;

			case QSE_T('K'):
				g_trait |= QSE_XLI_KEYTAG;
				break;

			case QSE_T('S'):
				g_trait |= QSE_XLI_STRTAG;
				break;

			case QSE_T('v'):
				g_trait |= QSE_XLI_VALIDATE;
				break;

			case QSE_T('m'):
				g_memlimit = qse_strtoulong (opt.arg, 10, QSE_NULL);
				break;

#if defined(QSE_BUILD_DEBUG)
			case QSE_T('X'):
				g_failmalloc = qse_strtoulong (opt.arg, 10, QSE_NULL);
				break;
#endif

			case QSE_T('\0'):
			{
				if (qse_strcmp(opt.lngopt, QSE_T("version")) == 0)
				{
					print_version ();
					goto done;
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

	if (!g_input_file)
	{
		print_usage (QSE_STDERR, argc, argv);
		goto oops;
	}

	if (opt.ind < argc) 
	{
		g_lookup_key = argv[opt.ind++];
		if (opt.ind < argc) 
		{
			g_value = argv[opt.ind++];

			if (opt.ind < argc)
			{
				print_usage (QSE_STDERR, argc, argv);
				goto oops;
			}
		}
	}

	return 1;

oops:
	return -1;

done:
	return 0;
}

void print_exec_error (qse_xli_t* xli)
{
#if 0
	const qse_xli_loc_t* errloc = qse_xli_geterrloc(xli);
	if (errloc->line > 0 || errloc->colm > 0)
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: cannot execute - %s at line %lu column %lu\n"),
			qse_xli_geterrmsg(xli),
			(unsigned long)errloc->line,
			(unsigned long)errloc->colm
		);
	}
	else
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: cannot execute - %s\n"),
			qse_xli_geterrmsg(xli)
		);
	}
#endif
}

static int xli_main (int argc, qse_char_t* argv[])
{
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL();
	qse_xli_t* xli = QSE_NULL;
	qse_xli_iostd_t in, out;
	int ret = -1, n;

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
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open memory heap\n"));
			goto oops;
		}
		mmgr = &xma_mmgr;
	}

	xli = qse_xli_openstdwithmmgr (mmgr, 0, 0, QSE_NULL);
	if (xli == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open stream editor\n"));
		goto oops;
	}

	{
		int trait;
		qse_xli_getopt (xli, QSE_XLI_TRAIT, &trait);
		g_trait |= trait;
		qse_xli_setopt (xli, QSE_XLI_TRAIT, &g_trait);
	}

	in.type = QSE_XLI_IOSTD_FILE;
	in.u.file.path = g_input_file;
	in.u.file.cmgr = g_infile_cmgr;

	n = (g_io_flags & IO_FLAG_JSON_INPUT)? qse_xli_readjsonstd(xli, &in):
	    (g_io_flags & IO_FLAG_INI_INPUT)?  qse_xli_readinistd(xli, &in):
	                                       qse_xli_readstd(xli, &in);
	if (n <= -1)
	{
		const qse_xli_loc_t* errloc;

		errloc = qse_xli_geterrloc (xli);
		if (errloc->line > 0 || errloc->colm > 0)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("ERROR: cannot read %s - %s at line %lu column %lu%s%s\n"),
				g_input_file,
				qse_xli_geterrmsg(xli),
				(unsigned long)errloc->line,
				(unsigned long)errloc->colm,
				(errloc->file? QSE_T(" in "): QSE_T("")),
				(errloc->file? errloc->file: QSE_T(""))
			);
		}
		else
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("ERROR: cannot read %s - %s\n"),
				g_input_file,
				qse_xli_geterrmsg(xli)
			);
		}
		goto oops;
	}

#if 0
	{
		static const qse_cstr_t strs[] =
		{
			{ QSE_T("hello"), 5 },
			{ QSE_T("xli"),   3 },
			{ QSE_T("world"), 5 }
		};

		if (qse_xli_insertpairwithstrs (xli, qse_xli_getroot(xli), QSE_NULL, QSE_T("test-key"), QSE_NULL, QSE_NULL, strs, QSE_COUNTOF(strs)) == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("ERROR: cannot insert a string pair - %s \n"),
				qse_xli_geterrmsg(xli)
			);
		}
	}
#endif

	if (g_lookup_key)
	{
		if (g_value)
		{
			qse_cstr_t v;

			v.ptr = g_value;
			v.len = qse_strlen(g_value);
			if (qse_xli_setpairwithstr (xli, QSE_NULL, g_lookup_key, &v, QSE_NULL) == QSE_NULL)
			{
				qse_fprintf (QSE_STDERR, 
					QSE_T("ERROR: cannot set a string pair - %s \n"),
					qse_xli_geterrmsg(xli)
				);
				goto oops;
			}
		}
		else
		{
			qse_xli_pair_t* pair;
			qse_size_t count;

			count = qse_xli_countpairs (xli, QSE_NULL, g_lookup_key);
			qse_printf (QSE_T("COUNT: %lu\n"), (unsigned long)count);

			pair = qse_xli_findpair (xli, QSE_NULL, g_lookup_key);
			if (pair == QSE_NULL)
			{
				qse_fprintf (QSE_STDERR, 
					QSE_T("ERROR: cannot find %s - %s \n"),
					g_lookup_key,
					qse_xli_geterrmsg(xli)
				);
				goto oops;
			}
			else
			{
				if (pair->val->type == QSE_XLI_STR)
				{
					qse_xli_str_t* str = (qse_xli_str_t*)pair->val;
					qse_printf (QSE_T("[%.*s]\n"), (int)str->len, str->ptr);
				}
				else if (pair->val->type == QSE_XLI_NIL)
				{
					qse_printf (QSE_T("nil\n"));
				}
				else if (pair->val->type == QSE_XLI_TRUE)
				{
					qse_printf (QSE_T("true\n"));
				}
				else if (pair->val->type == QSE_XLI_FALSE)
				{
					qse_printf (QSE_T("false\n"));
				}
				else
				{
					out.type = QSE_XLI_IOSTD_FILE;
					out.u.file.path = QSE_T("-");
					out.u.file.cmgr = g_outfile_cmgr;
					qse_xli_writestd (xli, (qse_xli_list_t*)pair->val, &out);
				}
			}
		}
	}

	out.type = QSE_XLI_IOSTD_FILE;
	out.u.file.path = g_output_file? g_output_file: QSE_T("-");
	out.u.file.cmgr = g_outfile_cmgr;

	ret = (g_io_flags & IO_FLAG_JSON_OUTPUT)? qse_xli_writejsonstd(xli, QSE_NULL, &out):
	      (g_io_flags & IO_FLAG_INI_OUTPUT)?  qse_xli_writeinistd(xli, QSE_NULL, &out):
	                                          qse_xli_writestd(xli, QSE_NULL, &out);
	if (ret <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("WARNING: cannot write - %s\n"), qse_xli_geterrmsg(xli));
	}

oops:
	if (xli) qse_xli_close (xli);
	if (xma_mmgr.ctx) qse_xma_close (xma_mmgr.ctx);

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
	int x;

#if defined(_WIN32)
	char locale[100];
	UINT codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		/* .codepage */
		qse_fmtuintmaxtombs (locale, QSE_COUNTOF(locale),
			codepage, 10, -1, QSE_MT('\0'), QSE_MT("."));
		setlocale (LC_ALL, locale);
		/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
	}
#else
	setlocale (LC_ALL, "");
	/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
#endif

	qse_open_stdsios ();
	x = qse_runmain (argc, argv, xli_main);
	qse_close_stdsios ();

	return x;
}

