/*
 * $Id: tgp.c,v 1.5 2007/05/16 09:15:14 bacon Exp $
 */

#include <ase/tgp/tgp.h>

#include <ase/utl/ctype.h>
#include <ase/utl/stdio.h>
#include <ase/utl/main.h>
#include <ase/utl/getopt.h>
#include <ase/cmn/mem.h>
#include <ase/cmn/str.h>

#include <ase/utl/helper.h>
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

static void print_usage (const ase_char_t* argv0)
{
	ase_fprintf (ASE_STDERR, 
		ASE_T("Usage: %s [options] [file]\n"), argv0);
	ase_fprintf (ASE_STDERR, 
		ASE_T("  -h          print this message\n"));

	ase_fprintf (ASE_STDERR,
		ASE_T("  -u          user id\n"));
	ase_fprintf (ASE_STDERR,
		ASE_T("  -g          group id\n"));
	ase_fprintf (ASE_STDERR,
		ASE_T("  -r          chroot\n"));
	ase_fprintf (ASE_STDERR,
		ASE_T("  -U          enable upload\n"));
}

static int handle_args (int argc, ase_char_t* argv[])
{
	ase_opt_t opt;
	ase_cint_t c;

	ase_memset (&opt, 0, ASE_SIZEOF(opt));
	opt.str = ASE_T("hu:g:r:");

	while ((c = ase_getopt (argc, argv, &opt)) != ASE_CHAR_EOF)
	{
		switch (c)
		{
			case ASE_T('h'):
				print_usage (argv[0]);
				return -1;

			case ASE_T('?'):
				ase_fprintf (ASE_STDERR, ASE_T("Error: illegal option - %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;

			case ASE_T(':'):
				ase_fprintf (ASE_STDERR, ASE_T("Error: missing argument for %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;

			case ASE_T('u'):
				//opt.arg;
				break;
			case ASE_T('g'):
				//opt.arg;
				break;
			case ASE_T('r'):
				//opt.arg;
				break;
			case ASE_T('U'):
				//opt.arg;
				break;
		}
	}

	if (opt.ind < argc)
	{
		ase_printf (ASE_T("Error: redundant argument - %s\n"), argv[opt.ind]);
		print_usage (argv[0]);
		return -1;
	}

	return 0;
}

struct xin_t
{
	const ase_char_t* name;
	ASE_FILE* fp;
};

struct xout_t
{
	const ase_char_t* name;
	ASE_FILE* fp;
};


static int io_1 (ase_tgp_t* tgp, int cmd, ase_char_t* buf, int len)
{
	xin_t* xin = (xin_t*)arg;

	switch (cmd)
	{
		case ASE_IO_OPEN:
			xin->fp = ase_fopen (ASE_T("abc.tgp"), ASE_T("r"));
			return (xin->fp == NULL) -1: 0;

		case ASE_IO_CLOSE
			ase_fclose (xin->fp);
			return 0;

		case ASE_IO_READ:
			ase_fgets (xin->fp);
			return 0;
	}	

	return -1;
}

static int io_2 (ase_tgp_t* tgp, int cmd, ase_char_t* buf, int len)
{
}

int tgp_main (int argc, ase_char_t* argv[])
{
	ase_tgp_t* tgp;
	int ret = 0;

	if (handle_args (argc, argv) == -1) return -1;
	
	tgp = ase_tgp_open (ASE_GETMMGR());
	if (tgp == ASE_NULL) 
	{
		ase_fprintf (ASE_STDERR, 
			ASE_T("Error: cannot create a tgp instance\n"));
		return -1;
	}

	ase_tgp_setstdin (tgp, io, xin);
	ase_tgp_setstdout (tgp, io, ASE_NULL);
	/*
	ase_tgp_setexecin (tgp, io, );
	ase_tgp_setexecout (tgp, io, );
	*/

	if (ase_tgp_run (tgp) == -1)
	{
		ase_fprintf (ASE_STDERR, 
			ASE_T("Error: cannot run a tgp instance\n"));
		ret = -1;
	}

	ase_tgp_close (tgp);
	return ret;
}

int ase_main (int argc, ase_achar_t* argv[])
{
	int n;

#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif

	n = ase_runmain (argc, argv, tgp_main);

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
