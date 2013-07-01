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

#include <qse/xli/stdxli.h>
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
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include <unistd.h>
#	include <errno.h>
#endif

static qse_char_t* g_input_file = QSE_NULL;
static qse_char_t* g_output_file = QSE_NULL;
static qse_char_t* g_lookup_key = QSE_NULL;
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
	qse_printf (QSE_T("QSEXLI version %hs\n"), QSE_PACKAGE_VERSION);
}

static void print_usage (QSE_FILE* out, int argc, qse_char_t* argv[])
{
	const qse_char_t* b = qse_basename (argv[0]);

	qse_fprintf (out, QSE_T("USAGE: %s [options] -f input-file [key]\n"), b);

	qse_fprintf (out, QSE_T("options as follows:\n"));
	qse_fprintf (out, QSE_T(" -h/--help                 show this message\n"));
	qse_fprintf (out, QSE_T(" --version                 show version\n"));
	qse_fprintf (out, QSE_T(" -i                 file   specify an input file\n"));
	qse_fprintf (out, QSE_T(" -o                 file   specify an output file\n"));
	qse_fprintf (out, QSE_T(" -a                        allow a key alias\n"));
	qse_fprintf (out, QSE_T(" -f                        keep file inclusion info\n"));
	qse_fprintf (out, QSE_T(" -t                        keep comment text\n"));
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
		QSE_T("hi:o:aftm:X:"),
#else
		QSE_T("hi:o:aftm:"),
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

			case QSE_T('i'):
				g_input_file = opt.arg;
				break;

			case QSE_T('o'):
				g_output_file = opt.arg;
				break;

			case QSE_T('a'):
				g_trait |= QSE_XLI_KEYALIAS;
				break;

			case QSE_T('f'):
				g_trait |= QSE_XLI_KEEPFILE;
				break;

			case QSE_T('t'):
				g_trait |= QSE_XLI_KEEPTEXT;
				break;

			case QSE_T('m'):
				g_memlimit = qse_strtoulong (opt.arg);
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

	if (opt.ind < argc) g_lookup_key = argv[opt.ind++];

	if (opt.ind < argc)
	{
		print_usage (QSE_STDERR, argc, argv);
		goto oops;
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
		xma_mmgr.ctx = qse_xma_open (QSE_MMGR_GETDFL(), 0, g_memlimit);
		if (xma_mmgr.ctx == QSE_NULL)
		{
			qse_printf (QSE_T("ERROR: cannot open memory heap\n"));
			goto oops;
		}
		mmgr = &xma_mmgr;
	}

	xli = qse_xli_openstdwithmmgr (mmgr, 0);
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

	if (qse_xli_readstd (xli, &in) <= -1)
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

	if (g_lookup_key)
	{
		qse_xli_pair_t* pair;
		pair = qse_xli_findpairbyalias (xli, QSE_NULL, g_lookup_key);
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
				qse_printf (QSE_T("#NIL\n"));
			}
			else
			{
				qse_printf (QSE_T("#LIST\n"));
			}
		}
	}


	out.type = QSE_XLI_IOSTD_FILE;
	out.u.file.path = g_output_file? g_output_file: QSE_T("-");
	out.u.file.cmgr = g_outfile_cmgr;
	ret = qse_xli_writestd (xli, &out);

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
		sprintf (locale, ".%u", (unsigned int)codepage);
		setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	return qse_runmain (argc, argv, xli_main);
}

