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

const qse_char_t* instream = QSE_NULL;

static qse_ssize_t in (
	qse_sed_t* sed, qse_sed_io_cmd_t cmd, qse_sed_io_arg_t* arg)
{
	switch (cmd)
	{
		case QSE_SED_IO_OPEN:
			if (arg->path == QSE_NULL ||
			    arg->path[0] == QSE_T('\0'))
			{
				if (instream)
				{
					arg->handle = qse_fopen (instream, QSE_T("r"));
					if (arg->handle == QSE_NULL) 
					{
						qse_cstr_t errarg;
						errarg.ptr = instream;
						errarg.len = qse_strlen(instream);
						qse_sed_seterror (sed, QSE_SED_EIOFIL, 0, &errarg);
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

int sed_main (int argc, qse_char_t* argv[])
{
	qse_sed_t* sed = QSE_NULL;
	int ret = -1;

	if (argc != 2 && argc != 3)
	{
		qse_fprintf (QSE_STDERR, QSE_T("usage: %s string [file]\n"), argv[0]);
		return -1;
	}

	if (argc == 3) instream = argv[2];

	sed = qse_sed_open (QSE_NULL, 0);
	if (sed == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot open a stream editor\n"));
		goto oops;
	}
	
	//if (argc == 3) qse_sed_setoption (sed, qse_strtoi(argv[2]));
	qse_sed_setoption (sed, QSE_SED_QUIET);

	if (qse_sed_comp (sed, argv[1], qse_strlen(argv[1])) == -1)
	{
		qse_size_t errlin = qse_sed_geterrlin(sed);
		if (errlin > 0)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("cannot compile - %s at line %lu\n"),
				qse_sed_geterrmsg(sed),
				(unsigned long)errlin
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
		qse_size_t errlin = qse_sed_geterrlin(sed);
		if (errlin > 0)
		{
			qse_fprintf (QSE_STDERR, 
				QSE_T("cannot execute - %s at line %lu\n"),
				qse_sed_geterrmsg(sed),
				(unsigned long)errlin
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
