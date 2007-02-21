#include <ase/lsp/lsp.h>

#include <ase/utl/ctype.h>
#include <ase/utl/stdio.h>
#include <ase/utl/main.h>

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

#ifdef __linux
#include <mcheck.h>
#endif

#if defined(vms) || defined(__vms)
/* it seems that the main function should be placed in the main object file
 * in OpenVMS. otherwise, the first function in the main object file seems
 * to become the main function resulting in program start-up failure. */
#include <ase/utl/main.c>
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

static void* lsp_malloc (ase_size_t n, void* custom_data)
{
#ifdef _WIN32
	return HeapAlloc (((prmfns_data_t*)custom_data)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* lsp_realloc (void* ptr, ase_size_t n, void* custom_data)
{
#ifdef _WIN32
	/* HeapReAlloc behaves differently from realloc */
	if (ptr == NULL)
		return HeapAlloc (((prmfns_data_t*)custom_data)->heap, 0, n);
	else
		return HeapReAlloc (((prmfns_data_t*)custom_data)->heap, 0, ptr, n);
#else
	return realloc (ptr, n);
#endif
}

static void lsp_free (void* ptr, void* custom_data)
{
#ifdef _WIN32
	HeapFree (((prmfns_data_t*)custom_data)->heap, 0, ptr);
#else
	free (ptr);
#endif
}

static void* lsp_memcpy  (void* dst, const void* src, ase_size_t n)
{
	return memcpy (dst, src, n);
}

static void* lsp_memset (void* dst, int val, ase_size_t n)
{
	return memset (dst, val, n);
}

static void lsp_abort (void* custom_data)
{
	abort ();
}

static void lsp_aprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
#ifdef _WIN32
	int n;
	ase_char_t buf[1024];
#endif

	va_start (ap, fmt);
#if defined(_WIN32)
	n = _vsntprintf (buf, ASE_COUNTOF(buf), fmt, ap);
	if (n < 0) buf[ASE_COUNTOF(buf)-1] = ASE_T('\0');

	#if defined(_MSC_VER) && (_MSC_VER<1400)
	MessageBox (NULL, buf, 
		ASE_T("Assertion Failure"), MB_OK|MB_ICONERROR);
	#else
	MessageBox (NULL, buf, 
		ASE_T("\uB2DD\uAE30\uB9AC \uC870\uB610"), MB_OK|MB_ICONERROR);
	#endif
#else
	ase_vprintf (fmt, ap);
#endif
	va_end (ap);
}

static void lsp_dprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

int lsp_main (int argc, ase_char_t* argv[])
{
	ase_lsp_t* lsp;
	ase_lsp_obj_t* obj;
	int mem, inc;
	ase_lsp_prmfns_t prmfns;
#ifdef _WIN32
	prmfns_data_t prmfns_data;
#endif

	mem = 1000;
	inc = 1000;

	if (mem <= 0) 
	{
		ase_printf (ASE_T("error: invalid memory size given\n"));
		return -1;
	}

	memset (&prmfns, 0, sizeof(prmfns));
	prmfns.malloc  = lsp_malloc;
	prmfns.realloc = lsp_realloc;
	prmfns.free    = lsp_free;
	prmfns.memcpy  = lsp_memcpy;
	prmfns.memset  = lsp_memset;

	prmfns.is_upper  = (ase_lsp_isctype_t)ase_isupper;
	prmfns.is_lower  = (ase_lsp_isctype_t)ase_islower;
	prmfns.is_alpha  = (ase_lsp_isctype_t)ase_isalpha;
	prmfns.is_digit  = (ase_lsp_isctype_t)ase_isdigit;
	prmfns.is_xdigit = (ase_lsp_isctype_t)ase_isxdigit;
	prmfns.is_alnum  = (ase_lsp_isctype_t)ase_isalnum;
	prmfns.is_space  = (ase_lsp_isctype_t)ase_isspace;
	prmfns.is_print  = (ase_lsp_isctype_t)ase_isprint;
	prmfns.is_graph  = (ase_lsp_isctype_t)ase_isgraph;
	prmfns.is_cntrl  = (ase_lsp_isctype_t)ase_iscntrl;
	prmfns.is_punct  = (ase_lsp_isctype_t)ase_ispunct;
	prmfns.to_upper  = (ase_lsp_toctype_t)ase_toupper;
	prmfns.to_lower  = (ase_lsp_toctype_t)ase_tolower;

	prmfns.sprintf = ase_sprintf;
	prmfns.aprintf = lsp_aprintf;
	prmfns.dprintf = lsp_dprintf;
	prmfns.abort   = lsp_abort;

#ifdef _WIN32
	prmfns_data.heap = HeapCreate (0, 1000000, 1000000);
	if (prmfns_data.heap == NULL)
	{
		ase_printf (ASE_T("Error: cannot create an lsp heap\n"));
		return -1;
	}

	prmfns.custom_data = &prmfns_data;
#endif

	lsp = ase_lsp_open (&prmfns, mem, inc);
	if (lsp == ASE_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (prmfns_data.heap);
#endif
		ase_printf (ASE_T("error: cannot create a lsp instance\n"));
		return -1;
	}

	ase_printf (ASE_T("ASELSP 0.0001\n"));

	ase_lsp_attinput (lsp, get_input, ASE_NULL);
	ase_lsp_attoutput (lsp, put_output, ASE_NULL);

	while (1)
	{
		ase_printf (ASE_T("ASELSP $ "));
		fflush (stdout);

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

int ase_main (int argc, ase_char_t* argv[])
{
	int n;

#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif

	n = lsp_main (argc, argv);

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
