#include <ase/lsp/lsp.h>
#include "../../etc/printf.c"
#include "../../etc/main.c"

#ifdef _WIN32
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

static ase_ssize_t get_input (int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_ssize_t n;

	switch (cmd) 
	{
		case ASE_LSP_IO_OPEN:
		case ASE_LSP_IO_CLOSE:
			return 0;

		case ASE_LSP_IO_READ:
		{
			if (size < 0) return -1;
			n = xp_sio_getc (xp_sio_in, data);
			if (n == 0) return 0;
			if (n != 1) return -1;
			return n;
		}
	}

	return -1;
}

static ase_ssize_t put_output (int cmd, void* arg, ase_char_t* data, ase_size_t size)
{

	switch (cmd) 
	{
		case ASE_LSP_IO_OPEN:
		case ASE_LSP_IO_CLOSE:
			return 0;

		case ASE_LSP_IO_WRITE:
			return xp_sio_putsx (xp_sio_out, data, size);
	}

	return -1;
}


int to_int (const ase_char_t* str)
{
	int r = 0;

	while (*str != ASE_T('\0')) 
	{
		if (!xp_isdigit(*str))	break;
		r = r * 10 + (*str - ASE_T('0'));
		str++;
	}

	return r;
}

#include <locale.h>

int handle_cli_error (
	const xp_cli_t* cli, int code, 
	const ase_char_t* name, const ase_char_t* value)
{
	xp_printf (ASE_T("usage: %s /memory=nnn /increment=nnn\n"), cli->verb);

	if (code == ASE_CLI_ERROR_INVALID_OPTNAME) {
		xp_printf (ASE_T("unknown option - %s\n"), name);
	}
	else if (code == ASE_CLI_ERROR_MISSING_OPTNAME) {
		xp_printf (ASE_T("missing option - %s\n"), name);
	}
	else if (code == ASE_CLI_ERROR_REDUNDANT_OPTVAL) {
		xp_printf (ASE_T("redundant value %s for %s\n"), value, name);
	}
	else if (code == ASE_CLI_ERROR_MISSING_OPTVAL) {
		xp_printf (ASE_T("missing value for %s\n"), name);
	}
	else if (code == ASE_CLI_ERROR_MEMORY) {
		xp_printf (ASE_T("memory error in processing %s\n"), name);
	}
	else {
		xp_printf (ASE_T("error code: %d\n"), code);
	}

	return -1;
}

xp_cli_t* parse_cli (int argc, ase_char_t* argv[])
{
	static const ase_char_t* optsta[] =
	{
		ASE_T("/"), ASE_T("--"), ASE_NULL
	};

	static xp_cliopt_t opts[] =
	{
		{ ASE_T("memory"), ASE_CLI_OPTNAME | ASE_CLI_OPTVAL },
		{ ASE_T("increment"), ASE_CLI_OPTNAME | ASE_CLI_OPTVAL },
		{ ASE_NULL, 0 }
	};

	static xp_cli_t cli =
	{
		handle_cli_error,
		optsta,
		ASE_T("="),
		opts
	};

	if (xp_parsecli (argc, argv, &cli) == -1) return ASE_NULL;
	return &cli;
}

#ifdef _WIN32
typedef struct syscas_data_t syscas_data_t;
struct syscas_data_t
{
	HANDLE heap;
};
#endif

static void* __lsp_malloc (ase_size_t n, void* custom_data)
{
#ifdef _WIN32
	return HeapAlloc (((syscas_data_t*)custom_data)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* __lsp_realloc (void* ptr, ase_size_t n, void* custom_data)
{
#ifdef _WIN32
	/* HeapReAlloc behaves differently from realloc */
	if (ptr == NULL)
		return HeapAlloc (((syscas_data_t*)custom_data)->heap, 0, n);
	else
		return HeapReAlloc (((syscas_data_t*)custom_data)->heap, 0, ptr, n);
#else
	return realloc (ptr, n);
#endif
}

static void __lsp_free (void* ptr, void* custom_data)
{
#ifdef _WIN32
	HeapFree (((syscas_data_t*)custom_data)->heap, 0, ptr);
#else
	free (ptr);
#endif
}

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

static void lsp_printf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vprintf (fmt, ap);
	va_end (ap);
}

int __main (int argc, ase_char_t* argv[])
{
	ase_lsp_t* lsp;
	ase_lsp_obj_t* obj;
	xp_cli_t* cli;
	int mem, inc;
	ase_lsp_syscas_t syscas;
#ifdef _WIN32
	syscas_data_t syscas_data;
#endif


	/*
	if (xp_setlocale () == -1) {
		xp_fprintf (xp_stderr,
			ASE_T("error: cannot set locale\n"));
		return -1;
	}
	*/

	if ((cli = parse_cli (argc, argv)) == ASE_NULL) return -1;
	mem = to_int(xp_getclioptval(cli, ASE_T("memory")));
	inc = to_int(xp_getclioptval(cli, ASE_T("increment")));
	xp_clearcli (cli);

	if (mem <= 0) 
	{
		xp_fprintf (xp_stderr,
			ASE_T("error: invalid memory size given\n"));
		return -1;
	}


	memset (&syscas, 0, sizeof(syscas));
	syscas.malloc = __lsp_malloc;
	syscas.realloc = __lsp_realloc;
	syscas.free = __lsp_free;

#ifdef ASE_T_IS_MCHAR
	syscas.is_upper  = isupper;
	syscas.is_lower  = islower;
	syscas.is_alpha  = isalpha;
	syscas.is_digit  = isdigit;
	syscas.is_xdigit = isxdigit;
	syscas.is_alnum  = isalnum;
	syscas.is_space  = isspace;
	syscas.is_print  = isprint;
	syscas.is_graph  = isgraph;
	syscas.is_cntrl  = iscntrl;
	syscas.is_punct  = ispunct;
	syscas.to_upper  = toupper;
	syscas.to_lower  = tolower;
#else
	syscas.is_upper  = iswupper;
	syscas.is_lower  = iswlower;
	syscas.is_alpha  = iswalpha;
	syscas.is_digit  = iswdigit;
	syscas.is_xdigit = iswxdigit;
	syscas.is_alnum  = iswalnum;
	syscas.is_space  = iswspace;
	syscas.is_print  = iswprint;
	syscas.is_graph  = iswgraph;
	syscas.is_cntrl  = iswcntrl;
	syscas.is_punct  = iswpunct;
	syscas.to_upper  = towupper;
	syscas.to_lower  = towlower;
#endif
	syscas.memcpy = memcpy;
	syscas.memset = memset;
	syscas.sprintf = xp_sprintf;
	syscas.aprintf = lsp_aprintf;
	syscas.dprintf = lsp_dprintf;
	syscas.abort = lsp_abort;

#ifdef _WIN32
	syscas_data.heap = HeapCreate (0, 1000000, 1000000);
	if (syscas_data.heap == NULL)
	{
		xp_printf (ASE_T("Error: cannot create an lsp heap\n"));
		return -1;
	}

	syscas.custom_data = &syscas_data;
#endif



	lsp = ase_lsp_open (&syscas, mem, inc);
	if (lsp == ASE_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (syscas_data.heap);
#endif
		xp_fprintf (xp_stderr, 
			ASE_T("error: cannot create a lsp instance\n"));
		return -1;
	}

	xp_printf (ASE_T("LSP 0.0001\n"));

	ase_lsp_attach_input (lsp, get_input, ASE_NULL);
	ase_lsp_attach_output (lsp, put_output, ASE_NULL);

	while (1)
	{
		xp_sio_puts (xp_sio_out, ASE_T("["));
		xp_sio_puts (xp_sio_out, argv[0]);
		xp_sio_puts (xp_sio_out, ASE_T("]"));
		xp_sio_flush (xp_sio_out);

		obj = ase_lsp_read (lsp);
		if (obj == ASE_NULL) 
		{
			int errnum = ase_lsp_geterrnum(lsp);
			const ase_char_t* errstr;

			if (errnum != ASE_LSP_ERR_END && 
			    errnum != ASE_LSP_ERR_EXIT) 
			{
				errstr = ase_lsp_geterrstr(errnum);
				xp_fprintf (xp_stderr, 
					ASE_T("error in read: [%d] %s\n"), errnum, errstr);
			}

			if (errnum < ASE_LSP_ERR_SYNTAX) break;
			continue;
		}

		if ((obj = ase_lsp_eval (lsp, obj)) != ASE_NULL) 
		{
			ase_lsp_print (lsp, obj);
			xp_sio_puts (xp_sio_out, ASE_T("\n"));
		}
		else 
		{
			int errnum;
			const ase_char_t* errstr;

			errnum = ase_lsp_geterrnum(lsp);
			if (errnum == ASE_LSP_ERR_EXIT) break;

			errstr = ase_lsp_geterrstr(errnum);
			xp_fprintf (xp_stderr, 
				ASE_T("error in eval: [%d] %s\n"), errnum, errstr);
		}
	}

	ase_lsp_close (lsp);

#ifdef _WIN32
	HeapDestroy (syscas_data.heap);
#endif
	return 0;
}

int xp_main (int argc, ase_char_t* argv[])
{
	int n;

#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif

	n = __main (argc, argv);

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
