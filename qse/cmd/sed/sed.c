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

#include <qse/sed/sed.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/xma.h>
#include <qse/cmn/misc.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

static const qse_char_t* g_script_file = QSE_NULL;
static qse_char_t* g_script = QSE_NULL;
static const qse_char_t* g_infile = QSE_NULL;
static int g_option = 0;
static qse_ulong_t g_memlimit = 0;

static qse_mmgr_t xma_mmgr = 
{
	(qse_mmgr_alloc_t)qse_xma_alloc,
	(qse_mmgr_realloc_t)qse_xma_realloc,
	(qse_mmgr_free_t)qse_xma_free,
	QSE_NULL
};

static qse_ssize_t in (
	qse_sed_t* sed, qse_sed_io_cmd_t cmd,
	qse_sed_io_arg_t* arg, qse_char_t* buf, qse_size_t size)
{
	switch (cmd)
	{
		case QSE_SED_IO_OPEN:
		{
			const qse_char_t* file;

			if (arg->path == QSE_NULL || arg->path[0] == QSE_T('\0'))
			{
				file = (g_infile == QSE_NULL)? QSE_NULL: g_infile;
			}
			else file = arg->path;


			if (file == QSE_NULL) arg->handle = qse_sio_in;
			else
			{
				arg->handle = qse_sio_open (
					qse_sed_getmmgr(sed),
					0,
					file,
					QSE_SIO_READ
				);	

				if (arg->handle == QSE_NULL) return -1;
			}

			return 1;
		}

		case QSE_SED_IO_CLOSE:
			if (arg->handle != qse_sio_in) qse_sio_close (arg->handle);
			return 0;

		case QSE_SED_IO_READ:
			return qse_sio_getsn (arg->handle, buf, size);

		default:
			return -1;
	}
}

static qse_ssize_t out (
	qse_sed_t* sed, qse_sed_io_cmd_t cmd,
	qse_sed_io_arg_t* arg, qse_char_t* data, qse_size_t len)
{
	switch (cmd)
	{
		case QSE_SED_IO_OPEN:
			if (arg->path == QSE_NULL || arg->path[0] == QSE_T('\0'))
			{
				arg->handle = qse_sio_out;
			}
			else
			{
				arg->handle = qse_sio_open (
					qse_sed_getmmgr(sed),
					0,
					arg->path,
					QSE_SIO_WRITE |
					QSE_SIO_CREATE |
					QSE_SIO_TRUNCATE
				);	

				if (arg->handle == QSE_NULL) return -1;
			}
			return 1;

		case QSE_SED_IO_CLOSE:
			qse_sio_flush (arg->handle);
			if (arg->handle != qse_sio_out) qse_sio_close (arg->handle);
			return 0;

		case QSE_SED_IO_WRITE:
			return qse_sio_putsn (arg->handle, data, len);

		default:
			return -1;
	}
}

static void print_usage (QSE_FILE* out, int argc, qse_char_t* argv[])
{
	const qse_char_t* b = qse_basename (argv[0]);

	qse_fprintf (out, QSE_T("USAGE: %s [options] script [file]\n"), b);
	qse_fprintf (out, QSE_T("       %s [options] -f script-file [file]\n"), b);

	qse_fprintf (out, QSE_T("options as follows:\n"));
	qse_fprintf (out, QSE_T(" -h        show this message\n"));
	qse_fprintf (out, QSE_T(" -n        disable auto-print\n"));
	qse_fprintf (out, QSE_T(" -f file   specify a script file\n"));
	qse_fprintf (out, QSE_T(" -r        use the extended regular expression\n"));
	qse_fprintf (out, QSE_T(" -a        perform strict address check\n"));
	qse_fprintf (out, QSE_T(" -w        allow address format of start~step\n"));
	qse_fprintf (out, QSE_T(" -x        allow text on the same line as c, a, i\n"));
	qse_fprintf (out, QSE_T(" -y        ensure a newline at text end\n"));
	qse_fprintf (out, QSE_T(" -m number specify the maximum amount of memory to use in bytes\n"));
}

static int handle_args (int argc, qse_char_t* argv[])
{
	static qse_opt_t opt = 
	{
		QSE_T("hnf:rawxym:"),
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

			case QSE_T('r'):
				g_option |= QSE_SED_EXTENDEDREX;
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

			case QSE_T('m'):
				g_memlimit = qse_strtoulong (opt.arg);
				break;
		}
	}


	if (opt.ind < argc && g_script_file == QSE_NULL) 
		g_script = argv[opt.ind++];
	if (opt.ind < argc) g_infile = argv[opt.ind++];

	if ((g_script_file == QSE_NULL && g_script == QSE_NULL) || 
	    opt.ind < argc)
	{
		print_usage (QSE_STDERR, argc, argv);
		return -1;
	}


	return 1;
}

qse_char_t* load_script_file (const qse_char_t* file)
{
	qse_cint_t c;
	qse_str_t script;
	QSE_FILE* fp;
	qse_xstr_t xstr;

	fp = qse_fopen (file, QSE_T("r"));
	if (fp == QSE_NULL) return QSE_NULL;

	if (qse_str_init (&script, QSE_MMGR_GETDFL(), 1024) <= -1)
	{
		qse_fclose (fp);
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot load %s\n"), file);
		return QSE_NULL;
	}

	while ((c = qse_fgetc (fp)) != QSE_CHAR_EOF)
	{
		if (qse_str_ccat (&script, c) == (qse_size_t)-1)
		{
			qse_str_fini (&script);
			qse_fclose (fp);
			return QSE_NULL;
		}		
	}
	if (qse_ferror(fp)) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot read %s\n"), file);
		qse_str_fini (&script);
		qse_fclose (fp);
		return QSE_NULL;		
	}

	qse_str_yield (&script, &xstr, 0);
	qse_str_fini (&script);
	qse_fclose (fp);

	return xstr.ptr;
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

	sed = qse_sed_open (mmgr, 0);
	if (sed == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot open a stream editor\n"));
		goto oops;
	}
	
	qse_sed_setoption (sed, g_option);

	if (g_script_file)
	{
		QSE_ASSERT (g_script == QSE_NULL);

		g_script = load_script_file (g_script_file);
		if (g_script == QSE_NULL) goto oops;
	}

	if (qse_sed_comp (sed, g_script, qse_strlen(g_script)) == -1)
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

	if (qse_sed_exec (sed, in, out) == -1)
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
		goto oops;
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
