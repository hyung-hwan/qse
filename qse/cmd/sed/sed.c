/**
 * $Id$
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

#include <qse/sed/sed.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/opt.h>

static const qse_char_t* g_script = QSE_NULL;
static const qse_char_t* g_infile = QSE_NULL;
static int g_option = 0;

static qse_ssize_t in (
	qse_sed_t* sed, qse_sed_io_cmd_t cmd, qse_sed_io_arg_t* arg)
{
	switch (cmd)
	{
		case QSE_SED_IO_OPEN:
			if (arg->path == QSE_NULL ||
			    arg->path[0] == QSE_T('\0'))
			{
				if (g_infile)
				{
					arg->handle = qse_fopen (g_infile, QSE_T("r"));
					if (arg->handle == QSE_NULL) 
					{
						qse_cstr_t errarg;
						errarg.ptr = g_infile;
						errarg.len = qse_strlen(g_infile);
						qse_sed_seterror (sed, QSE_SED_EIOFIL, &errarg, QSE_NULL);
						return -1;
					}
				}
				else arg->handle = QSE_STDIN;
			}
			else
			{
				arg->handle = qse_fopen (arg->path, QSE_T("r"));
				if (arg->handle == QSE_NULL) return -1;
			}
			return 1;

		case QSE_SED_IO_CLOSE:
			if (arg->handle != QSE_STDIN) 
				qse_fclose (arg->handle);
			return 0;

		case QSE_SED_IO_READ:
		{
			qse_cint_t c;
			c = qse_fgetc (arg->handle);
			if (c == QSE_CHAR_EOF) return 0;
			arg->u.r.buf[0] = c;
			return 1;
		}

		default:
			return -1;
	}
}

static qse_ssize_t out (
	qse_sed_t* sed, qse_sed_io_cmd_t cmd, qse_sed_io_arg_t* arg)
{
	switch (cmd)
	{
		case QSE_SED_IO_OPEN:
			if (arg->path == QSE_NULL ||
			    arg->path[0] == QSE_T('\0'))
			{
				arg->handle = QSE_STDOUT;
			}
			else
			{
				arg->handle = qse_fopen (arg->path, QSE_T("w"));
				if (arg->handle == QSE_NULL) return -1;
			}
			return 1;

		case QSE_SED_IO_CLOSE:
			if (arg->handle != QSE_STDOUT) 
				qse_fclose (arg->handle);
			return 0;

		case QSE_SED_IO_WRITE:
		{
			qse_size_t i = 0;
			for (i = 0; i < arg->u.w.len; i++) 
				qse_fputc (arg->u.w.data[i], arg->handle);
			return arg->u.w.len;
		}

		default:
			return -1;
	}
}

static void print_usage (QSE_FILE* out, int argc, qse_char_t* argv[])
{
	qse_fprintf (out, QSE_T("%s [options] script [file]\n"), argv[0]);
	qse_fprintf (out, QSE_T("options as follows:\n"));
	qse_fprintf (out, QSE_T(" -h    show this message\n"));
	qse_fprintf (out, QSE_T(" -n    disable auto-print\n"));
	qse_fprintf (out, QSE_T(" -a    perform strict address check\n"));
	qse_fprintf (out, QSE_T(" -r    allows {n,m} in a regular expression\n"));
	qse_fprintf (out, QSE_T(" -s    allows text on the same line as c, a, i\n"));
	qse_fprintf (out, QSE_T(" -l    ensures a newline at the text end"));
}

static int handle_args (int argc, qse_char_t* argv[])
{
	static qse_opt_t opt = 
	{
		QSE_T("hnarsl"),
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

			case QSE_T('a'):
				g_option |= QSE_SED_STRICT;
				break;

			case QSE_T('r'):
				g_option |= QSE_SED_REXBOUND;
				break;

			case QSE_T('s'):
				g_option |= QSE_SED_SAMELINE;
				break;

			case QSE_T('l'):
				g_option |= QSE_SED_ENSURENL;
				break;
		}
	}


	if (opt.ind < argc) g_script = argv[opt.ind++];
	if (opt.ind < argc) g_infile = argv[opt.ind++];

	if (g_script == QSE_NULL || opt.ind < argc)
	{
		print_usage (QSE_STDERR, argc, argv);
		return -1;
	}


	return 1;
}

int sed_main (int argc, qse_char_t* argv[])
{
	qse_sed_t* sed = QSE_NULL;
	int ret = -1;

	ret = handle_args (argc, argv);
	if (ret <= -1) return -1;
	if (ret == 0) return 0;

	ret = -1;

	sed = qse_sed_open (QSE_NULL, 0);
	if (sed == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot open a stream editor\n"));
		goto oops;
	}
	
	qse_sed_setoption (sed, g_option);

	if (qse_sed_comp (sed, g_script, qse_strlen(g_script)) == -1)
	{
		const qse_sed_loc_t* errloc = qse_sed_geterrloc(sed);
		if (errloc->lin > 0 || errloc->col > 0)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("cannot compile - %s at line %lu column %lu\n"),
				qse_sed_geterrmsg(sed),
				(unsigned long)errloc->lin,
				(unsigned long)errloc->col
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
		if (errloc->lin > 0 || errloc->col > 0)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("cannot execute - %s at line %lu column %lu\n"),
				qse_sed_geterrmsg(sed),
				(unsigned long)errloc->lin,
				(unsigned long)errloc->col
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
	if (sed != QSE_NULL) qse_sed_close (sed);
	return ret;
}

int qse_main (int argc, char* argv[])
{
	return qse_runmain (argc, argv, sed_main);
}
