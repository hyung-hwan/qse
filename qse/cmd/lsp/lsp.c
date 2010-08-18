/*
 * $Id: lsp.c,v 1.5 2007/05/16 09:15:14 bacon Exp $
 */

#include <qse/lsp/lsp.h>

#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/opt.h>

#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

#include <string.h>
#include <stdlib.h>

static qse_ssize_t get_input (
	qse_lsp_t* lsp, qse_lsp_io_cmd_t cmd, 
	qse_lsp_io_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	switch (cmd) 
	{
		case QSE_LSP_IO_OPEN:
			arg->handle = stdin;
			return 1;

		case QSE_LSP_IO_CLOSE:
			return 0;

		case QSE_LSP_IO_READ:
		{
			qse_cint_t c;

			if (size <= 0) return -1;
			c = qse_fgetc ((FILE*)arg->handle);

			if (c == QSE_CHAR_EOF) 
			{
				if (ferror((FILE*)arg->handle)) return -1;
				return 0;
			}

			data[0] = c;
			return 1;
		}

		default:
			return -1;
	}
}

static qse_ssize_t put_output (
	qse_lsp_t* lsp, qse_lsp_io_cmd_t cmd, 
	qse_lsp_io_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	switch (cmd) 
	{
		case QSE_LSP_IO_OPEN:
			arg->handle = stdout;
			return 1;

		case QSE_LSP_IO_CLOSE:
			return 0;

		case QSE_LSP_IO_WRITE:
		{
			int n = qse_fprintf (
				(FILE*)arg->handle, QSE_T("%.*s"), size, data);
			if (n < 0) return -1;

			return size;
		}

		default:
			return -1;
	}
}

static int custom_lsp_sprintf (
	void* custom, qse_char_t* buf, qse_size_t size, 
	const qse_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = qse_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

static int opt_memsize = 1000;
static int opt_meminc = 1000;

static void print_usage (const qse_char_t* argv0)
{
	qse_fprintf (QSE_STDERR, 
		QSE_T("Usage: %s [options]\n"), argv0);
	qse_fprintf (QSE_STDERR, 
		QSE_T("  -h          print this message\n"));
	qse_fprintf (QSE_STDERR, 
		QSE_T("  -m integer  number of memory cells\n"));
	qse_fprintf (QSE_STDERR, 
		QSE_T("  -i integer  number of memory cell increments\n"));
}

static int handle_args (int argc, qse_char_t* argv[])
{
	qse_opt_t opt;
	qse_cint_t c;

	qse_memset (&opt, 0, QSE_SIZEOF(opt));
	opt.str = QSE_T("hm:i:");

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			case QSE_T('h'):
				print_usage (argv[0]);
				return -1;

			case QSE_T('m'):
				opt_memsize = qse_strtoi(opt.arg);
				break;

			case QSE_T('i'):
				opt_meminc = qse_strtoi(opt.arg);
				break;

			case QSE_T('?'):
				qse_fprintf (QSE_STDERR, QSE_T("Error: illegal option - %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;

			case QSE_T(':'):
				qse_fprintf (QSE_STDERR, QSE_T("Error: missing argument for %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;
		}
	}

	if (opt.ind < argc)
	{
		qse_printf (QSE_T("Error: redundant argument - %s\n"), argv[opt.ind]);
		print_usage (argv[0]);
		return -1;
	}

	if (opt_memsize <= 0)
	{
		qse_printf (QSE_T("Error: invalid memory size given\n"));
		return -1;
	}
	return 0;
}

int lsp_main (int argc, qse_char_t* argv[])
{
	qse_lsp_t* lsp;
	qse_lsp_obj_t* obj;
	qse_lsp_prm_t prm;

	if (handle_args (argc, argv) == -1) return -1;
	
	prm.sprintf = custom_lsp_sprintf;
	prm.udd = QSE_NULL;

	lsp = qse_lsp_open (QSE_NULL, 0, &prm, opt_memsize, opt_meminc);
	if (lsp == QSE_NULL) 
	{
		qse_printf (QSE_T("Error: cannot create a lsp instance\n"));
		return -1;
	}

	qse_printf (QSE_T("QSELSP 0.0001\n"));

	{
		qse_lsp_io_t io = { get_input, put_output };
		qse_lsp_attachio (lsp, &io);
	}

	while (1)
	{
		qse_printf (QSE_T("QSELSP $ "));
		qse_fflush (stdout);

qse_lsp_gc (lsp);

		obj = qse_lsp_read (lsp);
		if (obj == QSE_NULL) 
		{
			qse_lsp_errnum_t errnum;
			qse_lsp_loc_t errloc;
			const qse_char_t* errmsg;

			qse_lsp_geterror (lsp, &errnum, &errmsg, &errloc);

			if (errnum != QSE_LSP_EEND && 
			    errnum != QSE_LSP_EEXIT) 
			{
				qse_printf (
					QSE_T("error in read: [%d] %s at line %d column %d\n"), 
					errnum, errmsg, (int)errloc.line, (int)errloc.colm);
			}

			/* TODO: change the following check */
			if (errnum < QSE_LSP_ESYNTAX) break; 
			continue;
		}

		if ((obj = qse_lsp_eval (lsp, obj)) != QSE_NULL) 
		{
			qse_lsp_print (lsp, obj);
			qse_printf (QSE_T("\n"));
		}
		else 
		{
			qse_lsp_errnum_t errnum;
			qse_lsp_loc_t errloc;
			const qse_char_t* errmsg;

			qse_lsp_geterror (lsp, &errnum, &errmsg, &errloc);
			if (errnum == QSE_LSP_EEXIT) break;

			qse_printf (
				QSE_T("error in eval: [%d] %s at line %d column %d\n"), 
				errnum, errmsg, (int)errloc.line, (int)errloc.colm);
		}
	}

	qse_lsp_close (lsp);

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, lsp_main);
}
