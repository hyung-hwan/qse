/*
 * $Id: lsp.c,v 1.5 2007/05/16 09:15:14 bacon Exp $
 */

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

#ifndef NDEBUG
void ase_assert_abort (void)
{
	abort ();
}

void ase_assert_printf (const ase_char_t* fmt, ...)
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

static ase_bool_t custom_lsp_isupper (void* custom, ase_cint_t c)  
{ 
	return ase_isupper (c); 
}

static ase_bool_t custom_lsp_islower (void* custom, ase_cint_t c)  
{ 
	return ase_islower (c); 
}

static ase_bool_t custom_lsp_isalpha (void* custom, ase_cint_t c)  
{ 
	return ase_isalpha (c); 
}

static ase_bool_t custom_lsp_isdigit (void* custom, ase_cint_t c)  
{ 
	return ase_isdigit (c); 
}

static ase_bool_t custom_lsp_isxdigit (void* custom, ase_cint_t c) 
{ 
	return ase_isxdigit (c); 
}

static ase_bool_t custom_lsp_isalnum (void* custom, ase_cint_t c)
{ 
	return ase_isalnum (c); 
}

static ase_bool_t custom_lsp_isspace (void* custom, ase_cint_t c)
{ 
	return ase_isspace (c); 
}

static ase_bool_t custom_lsp_isprint (void* custom, ase_cint_t c)
{ 
	return ase_isprint (c); 
}

static ase_bool_t custom_lsp_isgraph (void* custom, ase_cint_t c)
{
	return ase_isgraph (c); 
}

static ase_bool_t custom_lsp_iscntrl (void* custom, ase_cint_t c)
{
	return ase_iscntrl (c);
}

static ase_bool_t custom_lsp_ispunct (void* custom, ase_cint_t c)
{
	return ase_ispunct (c);
}

static ase_cint_t custom_lsp_toupper (void* custom, ase_cint_t c)
{
	return ase_toupper (c);
}

static ase_cint_t custom_lsp_tolower (void* custom, ase_cint_t c)
{
	return ase_tolower (c);
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

	prmfns.mmgr.malloc  = custom_lsp_malloc;
	prmfns.mmgr.realloc = custom_lsp_realloc;
	prmfns.mmgr.free    = custom_lsp_free;
#ifdef _WIN32
	prmfns_data.heap = HeapCreate (0, 1000000, 1000000);
	if (prmfns_data.heap == NULL)
	{
		ase_printf (ASE_T("Error: cannot create an lsp heap\n"));
		return -1;
	}

	prmfns.mmgr.custom_data = &prmfns_data;
#else
	prmfns.mmgr.custom_data = ASE_NULL;
#endif

	prmfns.ccls.is_upper  = custom_lsp_isupper;
	prmfns.ccls.is_lower  = custom_lsp_islower;
	prmfns.ccls.is_alpha  = custom_lsp_isalpha;
	prmfns.ccls.is_digit  = custom_lsp_isdigit;
	prmfns.ccls.is_xdigit = custom_lsp_isxdigit;
	prmfns.ccls.is_alnum  = custom_lsp_isalnum;
	prmfns.ccls.is_space  = custom_lsp_isspace;
	prmfns.ccls.is_print  = custom_lsp_isprint;
	prmfns.ccls.is_graph  = custom_lsp_isgraph;
	prmfns.ccls.is_cntrl  = custom_lsp_iscntrl;
	prmfns.ccls.is_punct  = custom_lsp_ispunct;
	prmfns.ccls.to_upper  = custom_lsp_toupper;
	prmfns.ccls.to_lower  = custom_lsp_tolower;
	prmfns.ccls.custom_data  = ASE_NULL;

	prmfns.misc.sprintf = custom_lsp_sprintf;
	prmfns.misc.dprintf = custom_lsp_dprintf;
	prmfns.misc.custom_data = ASE_NULL;

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
