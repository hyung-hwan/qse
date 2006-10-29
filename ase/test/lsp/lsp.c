#include <ase/lsp/lsp.h>
#include <xp/bas/stdio.h>
#include <xp/bas/ctype.h>
#include <xp/bas/stdcli.h>
#include <xp/bas/locale.h>
#include <xp/bas/sio.h>

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


static xp_ssize_t get_input (int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_ssize_t n;

	switch (cmd) 
	{
	case ASE_LSP_IO_OPEN:
	case ASE_LSP_IO_CLOSE:
		return 0;

	case ASE_LSP_IO_READ:
		if (size < 0) return -1;
		n = xp_sio_getc (xp_sio_in, data);
		if (n == 0) return 0;
		if (n != 1) return -1;
		return n;
	}

	return -1;
}

static xp_ssize_t put_output (int cmd, void* arg, xp_char_t* data, xp_size_t size)
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


int to_int (const xp_char_t* str)
{
	int r = 0;

	while (*str != XP_CHAR('\0')) 
	{
		if (!xp_isdigit(*str))	break;
		r = r * 10 + (*str - XP_CHAR('0'));
		str++;
	}

	return r;
}

#include <locale.h>

int handle_cli_error (
	const xp_cli_t* cli, int code, 
	const xp_char_t* name, const xp_char_t* value)
{
	xp_printf (XP_T("usage: %s /memory=nnn /increment=nnn\n"), cli->verb);

	if (code == XP_CLI_ERROR_INVALID_OPTNAME) {
		xp_printf (XP_T("unknown option - %s\n"), name);
	}
	else if (code == XP_CLI_ERROR_MISSING_OPTNAME) {
		xp_printf (XP_T("missing option - %s\n"), name);
	}
	else if (code == XP_CLI_ERROR_REDUNDANT_OPTVAL) {
		xp_printf (XP_T("redundant value %s for %s\n"), value, name);
	}
	else if (code == XP_CLI_ERROR_MISSING_OPTVAL) {
		xp_printf (XP_T("missing value for %s\n"), name);
	}
	else if (code == XP_CLI_ERROR_MEMORY) {
		xp_printf (XP_T("memory error in processing %s\n"), name);
	}
	else {
		xp_printf (XP_T("error code: %d\n"), code);
	}

	return -1;
}

xp_cli_t* parse_cli (int argc, xp_char_t* argv[])
{
	static const xp_char_t* optsta[] =
	{
		XP_T("/"), XP_T("--"), XP_NULL
	};

	static xp_cliopt_t opts[] =
	{
		{ XP_T("memory"), XP_CLI_OPTNAME | XP_CLI_OPTVAL },
		{ XP_T("increment"), XP_CLI_OPTNAME | XP_CLI_OPTVAL },
		{ XP_NULL, 0 }
	};

	static xp_cli_t cli =
	{
		handle_cli_error,
		optsta,
		XP_T("="),
		opts
	};

	if (xp_parsecli (argc, argv, &cli) == -1) return XP_NULL;
	return &cli;
}

#ifdef _WIN32
typedef struct syscas_data_t syscas_data_t;
struct syscas_data_t
{
	HANDLE heap;
};
#endif

static void* __lsp_malloc (xp_size_t n, void* custom_data)
{
#ifdef _WIN32
	return HeapAlloc (((syscas_data_t*)custom_data)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* __lsp_realloc (void* ptr, xp_size_t n, void* custom_data)
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

static int __aprintf (const xp_char_t* fmt, ...)
{
	int n;
	va_list ap;
#ifdef _WIN32
	xp_char_t buf[1024];
#endif

	va_start (ap, fmt);
#ifdef _WIN32
	n = xp_vsprintf (buf, xp_countof(buf), fmt, ap);
#if defined(_MSC_VER) && (_MSC_VER>=1400)
	MessageBox (NULL, buf, ASE_T("\uD655\uC778\uC2E4\uD328 Assertion Failure"), MB_OK | MB_ICONERROR);
#else
	MessageBox (NULL, buf, ASE_T("Assertion Failure"), MB_OK | MB_ICONERROR);
#endif

#else
	n = xp_vprintf (fmt, ap);
#endif
	va_end (ap);
	return n;
}

static int __dprintf (const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;
	va_start (ap, fmt);

#ifdef _WIN32
	n = _vftprintf (stderr, fmt, ap);
#else
	n = xp_vfprintf (stderr, fmt, ap);
#endif

	va_end (ap);
	return n;
}

int __main (int argc, xp_char_t* argv[])
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
			XP_T("error: cannot set locale\n"));
		return -1;
	}
	*/

	if ((cli = parse_cli (argc, argv)) == XP_NULL) return -1;
	mem = to_int(xp_getclioptval(cli, XP_T("memory")));
	inc = to_int(xp_getclioptval(cli, XP_T("increment")));
	xp_clearcli (cli);

	if (mem <= 0) 
	{
		xp_fprintf (xp_stderr,
			XP_T("error: invalid memory size given\n"));
		return -1;
	}


	memset (&syscas, 0, sizeof(syscas));
	syscas.malloc = __lsp_malloc;
	syscas.realloc = __lsp_realloc;
	syscas.free = __lsp_free;

#ifdef ASE_CHAR_IS_MCHAR
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
	syscas.aprintf = __aprintf;
	syscas.dprintf = __dprintf;
	syscas.abort = abort;

#ifdef _WIN32
	syscas_data.heap = HeapCreate (0, 1000000, 1000000);
	if (syscas_data.heap == NULL)
	{
		xp_printf (ASE_T("Error: cannot create an awk heap\n"));
		return -1;
	}

	syscas.custom_data = &syscas_data;
#endif



	lsp = ase_lsp_open (&syscas, mem, inc);
	if (lsp == XP_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (syscas_data.heap);
#endif
		xp_fprintf (xp_stderr, 
			XP_T("error: cannot create a lsp instance\n"));
		return -1;
	}

	xp_printf (XP_T("LSP 0.0001\n"));

	ase_lsp_attach_input (lsp, get_input, XP_NULL);
	ase_lsp_attach_output (lsp, put_output, XP_NULL);

	while (1)
	{
		xp_sio_puts (xp_sio_out, XP_T("["));
		xp_sio_puts (xp_sio_out, argv[0]);
		xp_sio_puts (xp_sio_out, XP_T("]"));
		xp_sio_flush (xp_sio_out);

		obj = ase_lsp_read (lsp);
		if (obj == XP_NULL) 
		{
			int errnum = ase_lsp_geterrnum(lsp);
			const xp_char_t* errstr;

			if (errnum != ASE_LSP_ERR_END && 
			    errnum != ASE_LSP_ERR_EXIT) 
			{
				errstr = ase_lsp_geterrstr(errnum);
				xp_fprintf (xp_stderr, 
					XP_T("error in read: [%d] %s\n"), errnum, errstr);
			}

			if (errnum < ASE_LSP_ERR_SYNTAX) break;
			continue;
		}

		if ((obj = ase_lsp_eval (lsp, obj)) != XP_NULL) 
		{
			ase_lsp_print (lsp, obj);
			xp_sio_puts (xp_sio_out, XP_T("\n"));
		}
		else 
		{
			int errnum;
			const xp_char_t* errstr;

			errnum = ase_lsp_geterrnum(lsp);
			if (errnum == ASE_LSP_ERR_EXIT) break;

			errstr = ase_lsp_geterrstr(errnum);
			xp_fprintf (xp_stderr, 
				XP_T("error in eval: [%d] %s\n"), errnum, errstr);
		}
	}

	ase_lsp_close (lsp);

#ifdef _WIN32
	HeapDestroy (syscas_data.heap);
#endif
	return 0;
}


int xp_main (int argc, xp_char_t* argv[])
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
