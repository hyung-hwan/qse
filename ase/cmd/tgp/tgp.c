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

/*
#include <ase/std/io.h>
#include <ase/std/lib.h>
#include <ase/std/mem.h>
#include <ase/std/opt.h>
*/


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

static ase_ssize_t get_input (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	switch (cmd) 
	{
		case ASE_TGP_IO_OPEN:
		case ASE_TGP_IO_CLOSE:
			return 0;

		case ASE_TGP_IO_READ:
		{
			/*
			if (ase_fgets (data, size, stdin) == ASE_NULL) 
			{
				if (ferror(stdin)) return -1;
				return 0;
			}
			return ase_tgp_strlen(data);
			*/

			ase_cint_t c;

			if (size <= 0) return -1;
			c = ase_fgetc (stdin);

			if (c == ASE_CHAR_EOF) 
			{
				if (ferror(stdin)) return -1;
				return 0;
			}

			data[0] = c;
			return 1;
		}
	}

	return -1;
}

static ase_ssize_t put_output (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	switch (cmd) 
	{
		case ASE_TGP_IO_OPEN:
		case ASE_TGP_IO_CLOSE:
			return 0;

		case ASE_TGP_IO_WRITE:
		{
			int n = ase_fprintf (
				stdout, ASE_T("%.*s"), size, data);
			if (n < 0) return -1;

			return size;
		}
	}

	return -1;
}

static void* custom_tgp_malloc (void* custom, ase_size_t n)
{
	return malloc (n);
}

static void* custom_tgp_realloc (void* custom, void* ptr, ase_size_t n)
{
	return realloc (ptr, n);
}

static void custom_tgp_free (void* custom, void* ptr)
{
	free (ptr);
}

static int custom_tgp_sprintf (
	void* custom, ase_char_t* buf, ase_size_t size, 
	const ase_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = ase_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}


static void custom_tgp_dprintf (void* custom, const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

static void print_usage (const ase_char_t* argv0)
{
	ase_fprintf (ASE_STDERR, 
		ASE_T("Usage: %s [options]\n"), argv0);
	ase_fprintf (ASE_STDERR, 
		ASE_T("  -h          print this message\n"));
	ase_fprintf (ASE_STDERR, 
		ASE_T("  -m integer  number of memory cells\n"));
	ase_fprintf (ASE_STDERR, 
		ASE_T("  -i integer  number of memory cell increments\n"));
}

static int handle_args (int argc, ase_char_t* argv[])
{
	ase_opt_t opt;
	ase_cint_t c;

	ase_memset (&opt, 0, ASE_SIZEOF(opt));
	opt.str = ASE_T("m:i:");

	while ((c = ase_getopt (argc, argv, &opt)) != ASE_CHAR_EOF)
	{
		switch (c)
		{
			case ASE_T('h'):
				print_usage (argv[0]);
				return -1;

			case ASE_T('m'):
				opt_memsize = ase_strtoi(opt.arg);
				break;

			case ASE_T('i'):
				opt_meminc = ase_strtoi(opt.arg);
				break;

			case ASE_T('?'):
				ase_fprintf (ASE_STDERR, ASE_T("Error: illegal option - %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;

			case ASE_T(':'):
				ase_fprintf (ASE_STDERR, ASE_T("Error: missing argument for %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;
		}
	}

	if (opt.ind < argc)
	{
		ase_printf (ASE_T("Error: redundant argument - %s\n"), argv[opt.ind]);
		print_usage (argv[0]);
		return -1;
	}

	if (opt_memsize <= 0)
	{
		ase_printf (ASE_T("Error: invalid memory size given\n"));
		return -1;
	}
	return 0;
}

int tgp_main (int argc, ase_char_t* argv[])
{
	ase_tgp_t* tgp;

	if (handle_args (argc, argv) == -1) return -1;
	
	tgp = ase_tgp_open (ASE_GETMMGR());
	if (tgp == ASE_NULL) 
	{
		ase_fprintf (ASE_STDERR, 
			ASE_T("Error: cannot create a tgp instance\n"));
		return -1;
	}

	ase_tgp_attinput (tgp, get_input, ASE_NULL);
	ase_tgp_attoutput (tgp, put_output, ASE_NULL);

	ase_tgp_close (tgp);
	return 0;
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
