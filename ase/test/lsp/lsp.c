#include <ase/lsp/lsp.h>
#include "../../etc/printf.c"
#include "../../etc/main.c"

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include <string.h>
#include <wctype.h>
#include <stdlib.h>

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#ifdef __linux
#include <mcheck.h>
#endif

#if defined(_WIN32)
	#define lsp_fgets _fgetts
	#define lsp_fgetc _fgettc
	#define lsp_fputs _fputts
	#define lsp_fputc _fputtc
#elif defined(ASE_CHAR_IS_MCHAR)
	#define lsp_fgets fgets
	#define lsp_fgetc fgetc
	#define lsp_fputs fputs
	#define lsp_fputc fputc
#else
	#define lsp_fgets fgetws
	#define lsp_fgetc fgetwc
	#define lsp_fputs fputws
	#define lsp_fputc fputwc
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
			if (lsp_fgets (data, size, stdin) == ASE_NULL) 
			{
				if (ferror(stdin)) return -1;
				return 0;
			}
			return ase_lsp_strlen(data);
			*/

			ase_cint_t c;

			if (size <= 0) return -1;
			c = lsp_fgetc (stdin);

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

static void* __lsp_malloc (ase_size_t n, void* custom_data)
{
#ifdef _WIN32
	return HeapAlloc (((prmfns_data_t*)custom_data)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* __lsp_realloc (void* ptr, ase_size_t n, void* custom_data)
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

static void __lsp_free (void* ptr, void* custom_data)
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

#if defined(ASE_CHAR_IS_MCHAR) 
	#if (__TURBOC__<=513) /* turboc 2.01 or earlier */
		static int lsp_isupper (int c) { return isupper (c); }
		static int lsp_islower (int c) { return islower (c); }
		static int lsp_isalpha (int c) { return isalpha (c); }
		static int lsp_isdigit (int c) { return isdigit (c); }
		static int lsp_isxdigit (int c) { return isxdigit (c); }
		static int lsp_isalnum (int c) { return isalnum (c); }
		static int lsp_isspace (int c) { return isspace (c); }
		static int lsp_isprint (int c) { return isprint (c); }
		static int lsp_isgraph (int c) { return isgraph (c); }
		static int lsp_iscntrl (int c) { return iscntrl (c); }
		static int lsp_ispunct (int c) { return ispunct (c); }
		static int lsp_toupper (int c) { return toupper (c); }
		static int lsp_tolower (int c) { return tolower (c); }
	#else
		#define lsp_isupper  isupper
		#define lsp_islower  islower
		#define lsp_isalpha  isalpha
		#define lsp_isdigit  isdigit
		#define lsp_isxdigit isxdigit
		#define lsp_isalnum  isalnum
		#define lsp_isspace  isspace
		#define lsp_isprint  isprint
		#define lsp_isgraph  isgraph
		#define lsp_iscntrl  iscntrl
		#define lsp_ispunct  ispunct
		#define lsp_toupper  tolower
		#define lsp_tolower  tolower
	#endif
#else
	#define lsp_isupper  iswupper
	#define lsp_islower  iswlower
	#define lsp_isalpha  iswalpha
	#define lsp_isdigit  iswdigit
	#define lsp_isxdigit iswxdigit
	#define lsp_isalnum  iswalnum
	#define lsp_isspace  iswspace
	#define lsp_isprint  iswprint
	#define lsp_isgraph  iswgraph
	#define lsp_iscntrl  iswcntrl
	#define lsp_ispunct  iswpunct

	#define lsp_toupper  towlower
	#define lsp_tolower  towlower
#endif

static void lsp_abort (void* custom_data)
{
	abort ();
}

static int lsp_sprintf (
	ase_char_t* buf, ase_size_t len, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = ase_vsprintf (buf, len, fmt, ap);
	va_end (ap);
	return n;
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
	prmfns.malloc = __lsp_malloc;
	prmfns.realloc = __lsp_realloc;
	prmfns.free = __lsp_free;

	prmfns.is_upper  = (ase_lsp_isctype_t)lsp_isupper;
	prmfns.is_lower  = (ase_lsp_isctype_t)lsp_islower;
	prmfns.is_alpha  = (ase_lsp_isctype_t)lsp_isalpha;
	prmfns.is_digit  = (ase_lsp_isctype_t)lsp_isdigit;
	prmfns.is_xdigit = (ase_lsp_isctype_t)lsp_isxdigit;
	prmfns.is_alnum  = (ase_lsp_isctype_t)lsp_isalnum;
	prmfns.is_space  = (ase_lsp_isctype_t)lsp_isspace;
	prmfns.is_print  = (ase_lsp_isctype_t)lsp_isprint;
	prmfns.is_graph  = (ase_lsp_isctype_t)lsp_isgraph;
	prmfns.is_cntrl  = (ase_lsp_isctype_t)lsp_iscntrl;
	prmfns.is_punct  = (ase_lsp_isctype_t)lsp_ispunct;
	prmfns.to_upper  = (ase_lsp_toctype_t)lsp_toupper;
	prmfns.to_lower  = (ase_lsp_toctype_t)lsp_tolower;

	prmfns.memcpy = lsp_memcpy;
	prmfns.memset = lsp_memset;
	prmfns.sprintf = lsp_sprintf;
	prmfns.aprintf = lsp_aprintf;
	prmfns.dprintf = lsp_dprintf;
	prmfns.abort = lsp_abort;

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
