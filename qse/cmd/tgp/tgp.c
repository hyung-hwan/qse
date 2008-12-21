/*
 * $Id: tgp.c,v 1.5 2007/05/16 09:15:14 bacon Exp $
 */

#include <qse/tgp/tgp.h>

#include <qse/utl/stdio.h>
#include <qse/utl/main.h>

#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/opt.h>

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if defined(__linux) && defined(_DEBUG)
#include <mcheck.h>
#endif

static void print_usage (const qse_char_t* argv0)
{
	qse_fprintf (QSE_STDERR, 
		QSE_T("Usage: %s [options] [file]\n"), argv0);
	qse_fprintf (QSE_STDERR, 
		QSE_T("  -h          print this message\n"));

	qse_fprintf (QSE_STDERR,
		QSE_T("  -u          user id\n"));
	qse_fprintf (QSE_STDERR,
		QSE_T("  -g          group id\n"));
	qse_fprintf (QSE_STDERR,
		QSE_T("  -r          chroot\n"));
	qse_fprintf (QSE_STDERR,
		QSE_T("  -U          enable upload\n"));
}

static int handle_args (int argc, qse_char_t* argv[])
{
	qse_opt_t opt;
	qse_cint_t c;

	qse_memset (&opt, 0, QSE_SIZEOF(opt));
	opt.str = QSE_T("hu:g:r:");

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			case QSE_T('h'):
				print_usage (argv[0]);
				return -1;

			case QSE_T('?'):
				qse_fprintf (QSE_STDERR, QSE_T("Error: illegal option - %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;

			case QSE_T(':'):
				qse_fprintf (QSE_STDERR, QSE_T("Error: missing argument for %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;

			case QSE_T('u'):
				//opt.arg;
				break;
			case QSE_T('g'):
				//opt.arg;
				break;
			case QSE_T('r'):
				//opt.arg;
				break;
			case QSE_T('U'):
				//opt.arg;
				break;
		}
	}

	if (opt.ind < argc)
	{
		qse_printf (QSE_T("Error: redundant argument - %s\n"), argv[opt.ind]);
		print_usage (argv[0]);
		return -1;
	}

	return 0;
}

typedef struct xin_t
{
	const qse_char_t* name;
	QSE_FILE* fp;
} xin_t;

typedef struct xout_t
{
	const qse_char_t* name;
	QSE_FILE* fp;
} xout_t;


static int io_in (int cmd, void* arg, qse_char_t* buf, int len)
{
	xin_t* xin = (xin_t*)arg;

	switch (cmd)
	{
		case QSE_TGP_IO_OPEN:
			xin->fp = (xin->name == QSE_NULL)?
				QSE_STDIN:
				qse_fopen(xin->name,QSE_T("r"));

			return (xin->fp == NULL)? -1: 0;

		case QSE_TGP_IO_CLOSE:
			if (xin->name != QSE_NULL) qse_fclose (xin->fp);
			return 0;

		case QSE_TGP_IO_READ:
			if (qse_fgets (buf, len, xin->fp) == QSE_NULL) return 0;
			return qse_strlen(buf);
	}	

	return -1;
}

static int io_out (int cmd, void* arg, qse_char_t* buf, int len)
{
	xout_t* xout = (xout_t*)arg;

	switch (cmd)
	{
		case QSE_TGP_IO_OPEN:
			xout->fp = (xout->name == QSE_NULL)?
				QSE_STDOUT:
				qse_fopen(xout->name,QSE_T("r"));

			return (xout->fp == NULL)? -1: 0;

		case QSE_TGP_IO_CLOSE:
			if (xout->name != QSE_NULL) qse_fclose (xout->fp);
			return 0;

		case QSE_TGP_IO_WRITE:
			qse_fprintf (xout->fp, QSE_T("%.*s"), len, buf);
			return len;
	}	

	return -1;
}

int tgp_main (int argc, qse_char_t* argv[])
{
	qse_tgp_t* tgp;

	xin_t xin;
	xout_t xout;

	int ret = 0;

	if (handle_args (argc, argv) == -1) return -1;
	
	tgp = qse_tgp_open (QSE_MMGR_GETDFL());
	if (tgp == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("Error: cannot create a tgp instance\n"));
		return -1;
	}

	xin.name = QSE_T("x.tgp");
	xout.name = QSE_NULL;

	qse_tgp_attachin (tgp, io_in, &xin);
	qse_tgp_attachout (tgp, io_out, &xout);
	/*
	qse_tgp_setexecin (tgp, io, );
	qse_tgp_setexecout (tgp, io, );
	*/

	if (qse_tgp_run (tgp) == -1)
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("Error: cannot run a tgp instance\n"));
		ret = -1;
	}

	qse_tgp_close (tgp);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int n;

#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif

	n = qse_runmain (argc, argv, tgp_main);

#if defined(__linux) && defined(_DEBUG)
	muntrace ();
#endif
#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
	_CrtDumpMemoryLeaks ();
	wprintf (L"Press ENTER to quit\n");
	getchar ();
#endif

	return n;
}
