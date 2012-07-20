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

#include <qse/cut/std.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/path.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static qse_char_t* g_selector = QSE_NULL;

static int g_infile_start = 0;
static int g_infile_end = 0;
static const qse_char_t* g_infile = QSE_NULL;
static const qse_char_t* g_outfile = QSE_NULL;
static int g_option = 0;

static void print_usage (QSE_FILE* out, int argc, qse_char_t* argv[])
{
	const qse_char_t* b = qse_basename (argv[0]);

	qse_fprintf (out, QSE_T("USAGE: %s -c selectors [options] [file]\n"), b);
	qse_fprintf (out, QSE_T("       %s -f selectors [options] [file]\n"), b);

	qse_fprintf (out, QSE_T("mandatory options as follows:\n"));
	qse_fprintf (out, QSE_T(" -c selectors  select characters\n"));
	qse_fprintf (out, QSE_T(" -f selectors  select fields\n"));
	qse_fprintf (out, QSE_T("options as follows:\n"));
	qse_fprintf (out, QSE_T(" -h            show this message\n"));
	qse_fprintf (out, QSE_T(" -d delimiter  specify a field delimiter(require -f)\n"));
	qse_fprintf (out, QSE_T(" -D delimiter  specify a field delimiter for output(require -f)\n"));
	qse_fprintf (out, QSE_T(" -s            do not show undelimited lines(require -f)\n"));
	qse_fprintf (out, QSE_T(" -w            treat any whitespaces as a field delimiter(require -f)\n"));
	qse_fprintf (out, QSE_T(" -F            fold adjacent field delimiters(require -f)\n"));
	qse_fprintf (out, QSE_T(" -t            trim off leading and trailing whitespaces\n"));
	qse_fprintf (out, QSE_T(" -n            normalize whitespaces\n"));
	qse_fprintf (out, QSE_T(" -o file       specify the output file\n"));
}

static int handle_args (int argc, qse_char_t* argv[])
{
	static qse_opt_t opt = 
	{
		QSE_T("hc:f:d:D:swFtno:"),
		QSE_NULL
	};
	qse_cint_t c;
	qse_cint_t din = QSE_CHAR_EOF;
	qse_cint_t dout = QSE_CHAR_EOF;

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

			case QSE_T('c'):
			case QSE_T('f'):
			{
				/* 
				 * 1 character for c or f.
				 * 4 characters to hold Dxx, in case 
				 * a delimiter is specified 
				 * 1 character for the terminating '\0';
				 */
				qse_char_t x[6] = QSE_T("     ");
				
				if (g_selector != QSE_NULL)
				{
					qse_fprintf (QSE_STDERR, 
						QSE_T("ERROR: multiple selectors specified\n"));
					print_usage (QSE_STDERR, argc, argv);
					return -1;
				}

				x[4] = c;
				g_selector = qse_strdup2 (x, opt.arg, QSE_MMGR_GETDFL());
				if (g_selector == QSE_NULL)
				{
					qse_fprintf (QSE_STDERR, QSE_T("ERROR: insufficient memory\n"));
					return -1;
				}
				break;
			}

			case QSE_T('d'):
				if (qse_strlen(opt.arg) > 1)
				{
					print_usage (QSE_STDERR, argc, argv);
					return -1;
				}

				din = opt.arg[0];
				break;

			case QSE_T('D'):
				if (qse_strlen(opt.arg) > 1)
				{
					print_usage (QSE_STDERR, argc, argv);
					return -1;
				}

				dout = opt.arg[0];
				break;

			case QSE_T('s'):
				g_option |= QSE_CUT_DELIMONLY;
				break;

			case QSE_T('w'):
				g_option |= QSE_CUT_WHITESPACE;
				break;

			case QSE_T('F'):
				g_option |= QSE_CUT_FOLDDELIMS;
				break;

			case QSE_T('t'):
				g_option |= QSE_CUT_TRIMSPACE;
				break;

			case QSE_T('n'):
				g_option |= QSE_CUT_NORMSPACE;
				break;

			case QSE_T('o'):
				if (g_outfile != QSE_NULL)
				{
					qse_fprintf (QSE_STDERR, 
						QSE_T("ERROR: -o specified more than once\n"));
					print_usage (QSE_STDERR, argc, argv);
					return -1;
				}

				g_outfile = opt.arg;
				break;
		}
	}


	g_infile_start = opt.ind;
	g_infile_end = argc;

	if (g_selector == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: neither -c nor -f specified\n"));
		print_usage (QSE_STDERR, argc, argv);
		return -1;
	}

	if (din == QSE_CHAR_EOF) din = QSE_T('\t');
	if (dout == QSE_CHAR_EOF) dout = din;

	if (din != QSE_CHAR_EOF)
	{
		QSE_ASSERT (dout != QSE_CHAR_EOF);
		g_selector[0] = QSE_T('D'),
		g_selector[1] = din;
		g_selector[2] = dout;
		g_selector[3] = QSE_T(',');
	}

	if (g_selector[4] == QSE_T('c') &&
	    (din != QSE_CHAR_EOF || dout != QSE_CHAR_EOF || 
	     (g_option & QSE_CUT_WHITESPACE) || (g_option & QSE_CUT_FOLDDELIMS)))
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: -d, -D, -w, or -F specified with -c\n"));
		print_usage (QSE_STDERR, argc, argv);
		return -1;
	}

	if (din != QSE_CHAR_EOF && (g_option & QSE_CUT_WHITESPACE))
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: both -d and -w specified\n"));
		print_usage (QSE_STDERR, argc, argv);
		return -1;
	}

	if (g_selector[4] == QSE_T('c') &&
	    (g_option & QSE_CUT_DELIMONLY))
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: -s specified with -c\n"));
		print_usage (QSE_STDERR, argc, argv);
		return -1;
	}

	return 1;
}

int cut_main (int argc, qse_char_t* argv[])
{
	qse_cut_t* cut = QSE_NULL;
	int ret = -1;

	ret = handle_args (argc, argv);
	if (ret <= 0) goto oops;

	ret = -1;

	cut = qse_cut_openstd (0);
	if (cut == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot open cut\n"));
		goto oops;
	}
	
	qse_cut_setoption (cut, g_option);

	if (qse_cut_compstd (cut, g_selector) <= -1)
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("cannot compile - %s\n"),
			qse_cut_geterrmsg(cut)
		);
		goto oops;
	}

	if (g_infile_start < g_infile_end)
	{
		do
		{
			g_infile = argv[g_infile_start];
			if (g_infile && g_infile[0] == QSE_T('-') && g_infile[1] == QSE_T('\0')) g_infile = QSE_NULL;
			if (qse_cut_execstd (cut, g_infile, g_outfile) <= -1)
			{
				qse_fprintf (QSE_STDERR, 
					QSE_T("cannot execute - %s\n"),
					qse_cut_geterrmsg(cut)
				);
				goto oops;
			}

			/*qse_cut_clear (cut);*/
		} 
		while (++g_infile_start < g_infile_end);
	}
	else
	{
		if (qse_cut_execstd (cut, QSE_NULL, g_outfile) <= -1)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("cannot execute - %s\n"),
				qse_cut_geterrmsg(cut)
			);
			goto oops;
		}
	}

	ret = 0;

oops:
	if (cut != QSE_NULL) qse_cut_close (cut);
	if (g_selector != QSE_NULL) QSE_MMGR_FREE (QSE_MMGR_GETDFL(), g_selector);
	return ret;
}

int qse_main (int argc, char* argv[])
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
		qse_setdflcmgr (qse_slmbcmgr);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgr (qse_slmbcmgr);
#endif

	return qse_runmain (argc, argv, cut_main);
}
