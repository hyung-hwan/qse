/**
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/cut/cut.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/misc.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

static qse_cut_sel_id_t g_selector_id = QSE_CUT_SEL_CHAR;
static qse_char_t* g_selector = QSE_NULL;

static int g_infile_start = 0;
static int g_infile_end = 0;
static const qse_char_t* g_infile = QSE_NULL;
static const qse_char_t* g_outfile = QSE_NULL;
static int g_option = 0;
static int g_infile_index = 0;

static qse_cint_t g_din = QSE_CHAR_EOF;
static qse_cint_t g_dout = QSE_CHAR_EOF;

static qse_ssize_t in (
	qse_cut_t* cut, qse_cut_io_cmd_t cmd,
	qse_cut_io_arg_t* arg, qse_char_t* buf, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_CUT_IO_OPEN:
		{
			if (g_infile == QSE_NULL ||
			    (g_infile[0] == QSE_T('-') &&
			     g_infile[1] == QSE_T('\0'))) 
			{
				arg->handle = qse_sio_in;
			}
			else
			{
				arg->handle = qse_sio_open (
					qse_cut_getmmgr(cut),
					0,
					g_infile,
					QSE_SIO_READ
				);	

				if (arg->handle == QSE_NULL) return -1;
			}

			return 1;
		}

		case QSE_CUT_IO_CLOSE:
			if (arg->handle != qse_sio_in) qse_sio_close (arg->handle);
			return 0;

		case QSE_CUT_IO_READ:
		{
			qse_ssize_t n = qse_sio_getsn (arg->handle, buf, size);
#if 0
			if (n == 0)
			{
				if (no more file) return 0;

				tmp = qse_sio_open (
					qse_cut_getmmgr(cut),
					0,
					g_infile[++g_infile],
					QSE_SIO_READ
				);	
				if (tmp == QSE_NULL) return -1;
				
				qse_sio_close (arg->handle);
				arg->handle = tmp;
				goto retry;
			}
#endif

			return n;
		}

		default:
			return -1;
	}
}

static qse_ssize_t out (
	qse_cut_t* cut, qse_cut_io_cmd_t cmd,
	qse_cut_io_arg_t* arg, qse_char_t* data, qse_size_t len)
{
	switch (cmd)
	{
		case QSE_CUT_IO_OPEN:
			if (g_outfile == QSE_NULL ||
			    (g_outfile[0] == QSE_T('-') &&
			     g_outfile[1] == QSE_T('\0'))) 
			{
				arg->handle = qse_sio_out;
			}
			else
			{
				arg->handle = qse_sio_open (
					qse_cut_getmmgr(cut),
					0,
					g_outfile,
					QSE_SIO_WRITE |
					QSE_SIO_CREATE |
					QSE_SIO_TRUNCATE
				);	

				if (arg->handle == QSE_NULL) return -1;
			}
			return 1;

		case QSE_CUT_IO_CLOSE:
			if (arg->handle != qse_sio_out) qse_sio_close (arg->handle);
			return 0;

		case QSE_CUT_IO_WRITE:
			return qse_sio_putsn (arg->handle, data, len);

		default:
			return -1;
	}
}

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
				if (g_selector != QSE_NULL)
				{
					qse_fprintf (QSE_STDERR, 
						QSE_T("ERROR: both -c and -f specified\n"));
					print_usage (QSE_STDERR, argc, argv);
					return -1;
				}

				g_selector = opt.arg;
				g_selector_id = QSE_CUT_SEL_CHAR;
				break;

			case QSE_T('f'):
				if (g_selector != QSE_NULL)
				{
					qse_fprintf (QSE_STDERR, 
						QSE_T("ERROR: both -c and -f specified\n"));
					print_usage (QSE_STDERR, argc, argv);
					return -1;
				}

				g_selector = opt.arg;
				g_selector_id = QSE_CUT_SEL_FIELD;
				break;


			case QSE_T('d'):
				if (qse_strlen(opt.arg) > 1)
				{
					print_usage (QSE_STDERR, argc, argv);
					return -1;
				}

				g_din = opt.arg[0];
				break;

			case QSE_T('D'):
				if (qse_strlen(opt.arg) > 1)
				{
					print_usage (QSE_STDERR, argc, argv);
					return -1;
				}

				g_dout = opt.arg[0];
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

	if (g_selector_id == QSE_CUT_SEL_CHAR &&
	    (g_din != QSE_CHAR_EOF || g_dout != QSE_CHAR_EOF || 
	     (g_option & QSE_CUT_WHITESPACE) || (g_option & QSE_CUT_FOLDDELIMS)))
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: -d, -D, -w, or -F specified with -c\n"));
		print_usage (QSE_STDERR, argc, argv);
		return -1;
	}

	if (g_din != QSE_CHAR_EOF && (g_option & QSE_CUT_WHITESPACE))
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("ERROR: both -d and -w specified\n"));
		print_usage (QSE_STDERR, argc, argv);
		return -1;
	}

	if (g_selector_id == QSE_CUT_SEL_CHAR &&
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
	if (ret <= -1) return -1;
	if (ret == 0) return 0;

	ret = -1;

	cut = qse_cut_open (QSE_NULL, 0);
	if (cut == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot open cut\n"));
		goto oops;
	}
	
	qse_cut_setoption (cut, g_option);

	if (g_din == QSE_CHAR_EOF) g_din = QSE_T('\t');
	if (g_dout == QSE_CHAR_EOF) g_dout = g_din;

	if (qse_cut_comp (cut, g_selector_id,
		g_selector, qse_strlen(g_selector), g_din, g_dout) == -1)
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
			if (qse_cut_exec (cut, in, out) == -1)
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
		if (qse_cut_exec (cut, in, out) == -1)
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
	return ret;
}

int qse_main (int argc, char* argv[])
{
	return qse_runmain (argc, argv, cut_main);
}
