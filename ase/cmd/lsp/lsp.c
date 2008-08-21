/*
 * $Id: lsp.c,v 1.5 2007/05/16 09:15:14 bacon Exp $
 */

#include <ase/lsp/lsp.h>

#include <ase/utl/stdio.h>
#include <ase/utl/main.h>
#include <ase/utl/getopt.h>

#include <ase/cmn/mem.h>
#include <ase/cmn/chr.h>
#include <ase/cmn/str.h>

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
		case ASE_LSP_IO_OPEN:
		case ASE_LSP_IO_CLOSE:
			return 0;

		case ASE_LSP_IO_READ:
		{
			/*
			if (ase_fgets (data, size, stdin) == ASE_NULL) 
			{
				if (ferror(stdin)) return -1;
				return 0;
			}
			return ase_lsp_strlen(data);
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
		case ASE_LSP_IO_OPEN:
		case ASE_LSP_IO_CLOSE:
			return 0;

		case ASE_LSP_IO_WRITE:
		{
			int n = ase_fprintf (
				stdout, ASE_T("%.*s"), size, data);
			if (n < 0) return -1;

			return size;
		}
	}

	return -1;
}

#ifdef _WIN32
typedef struct prmfns_data_t prmfns_data_t;
struct prmfns_data_t
{
	HANDLE heap;
};
#endif

static void* custom_lsp_malloc (void* custom, ase_size_t n)
{
#ifdef _WIN32
	return HeapAlloc (((prmfns_data_t*)custom)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* custom_lsp_realloc (void* custom, void* ptr, ase_size_t n)
{
#ifdef _WIN32
	/* HeapReAlloc behaves differently from realloc */
	if (ptr == NULL)
		return HeapAlloc (((prmfns_data_t*)custom)->heap, 0, n);
	else
		return HeapReAlloc (((prmfns_data_t*)custom)->heap, 0, ptr, n);
#else
	return realloc (ptr, n);
#endif
}

static void custom_lsp_free (void* custom, void* ptr)
{
#ifdef _WIN32
	HeapFree (((prmfns_data_t*)custom)->heap, 0, ptr);
#else
	free (ptr);
#endif
}

static int custom_lsp_sprintf (
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


static void custom_lsp_dprintf (void* custom, const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

static int opt_memsize = 1000;
static int opt_meminc = 1000;

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
	opt.str = ASE_T("hm:i:");

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

int lsp_main (int argc, ase_char_t* argv[])
{
	ase_lsp_t* lsp;
	ase_lsp_obj_t* obj;
	ase_lsp_prmfns_t prmfns;
#ifdef _WIN32
	prmfns_data_t prmfns_data;
#endif

	if (handle_args (argc, argv) == -1) return -1;
	
	ase_memset (&prmfns, 0, ASE_SIZEOF(prmfns));

	prmfns.mmgr.alloc  = custom_lsp_malloc;
	prmfns.mmgr.realloc = custom_lsp_realloc;
	prmfns.mmgr.free    = custom_lsp_free;
#ifdef _WIN32
	prmfns_data.heap = HeapCreate (0, 1000000, 1000000);
	if (prmfns_data.heap == NULL)
	{
		ase_printf (ASE_T("Error: cannot create an lsp heap\n"));
		return -1;
	}

	prmfns.mmgr.data = &prmfns_data;
#else
	prmfns.mmgr.data = ASE_NULL;
#endif

	/* TODO: change prmfns ...... lsp_oepn... etc */
	ase_memcpy (&prmfns.ccls, ASE_CCLS_GETDFL(), ASE_SIZEOF(prmfns.ccls));

	prmfns.misc.sprintf = custom_lsp_sprintf;
	prmfns.misc.dprintf = custom_lsp_dprintf;
	prmfns.misc.data = ASE_NULL;

	lsp = ase_lsp_open (&prmfns, opt_memsize, opt_meminc);
	if (lsp == ASE_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (prmfns_data.heap);
#endif
		ase_printf (ASE_T("Error: cannot create a lsp instance\n"));
		return -1;
	}

	ase_printf (ASE_T("ASELSP 0.0001\n"));

	ase_lsp_attinput (lsp, get_input, ASE_NULL);
	ase_lsp_attoutput (lsp, put_output, ASE_NULL);

	while (1)
	{
		ase_printf (ASE_T("ASELSP $ "));
		ase_fflush (stdout);

		obj = ase_lsp_read (lsp);
		if (obj == ASE_NULL) 
		{
			int errnum;
			const ase_char_t* errmsg;

			ase_lsp_geterror (lsp, &errnum, &errmsg);

			if (errnum != ASE_LSP_EEND && 
			    errnum != ASE_LSP_EEXIT) 
			{
				ase_printf (
					ASE_T("error in read: [%d] %s\n"), 
					errnum, errmsg);
			}

			/* TODO: change the following check */
			if (errnum < ASE_LSP_ESYNTAX) break; 
			continue;
		}

		if ((obj = ase_lsp_eval (lsp, obj)) != ASE_NULL) 
		{
			ase_lsp_print (lsp, obj);
			ase_printf (ASE_T("\n"));
		}
		else 
		{
			int errnum;
			const ase_char_t* errmsg;

			ase_lsp_geterror (lsp, &errnum, &errmsg);
			if (errnum == ASE_LSP_EEXIT) break;

			ase_printf (
				ASE_T("error in eval: [%d] %s\n"), 
				errnum, errmsg);
		}
	}

	ase_lsp_close (lsp);

#ifdef _WIN32
	HeapDestroy (prmfns_data.heap);
#endif
	return 0;
}

int ase_main (int argc, ase_achar_t* argv[])
{
	int n;

#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif

	n = ase_runmain (argc, argv, lsp_main);

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
