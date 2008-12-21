/*
 * $Id: lsp.c,v 1.5 2007/05/16 09:15:14 bacon Exp $
 */

#include <qse/lsp/lsp.h>

#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/opt.h>

#include <qse/utl/stdio.h>
#include <qse/utl/main.h>

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

static qse_ssize_t get_input (
	int cmd, void* arg, qse_char_t* data, qse_size_t size)
{
	switch (cmd) 
	{
		case QSE_LSP_IO_OPEN:
		case QSE_LSP_IO_CLOSE:
			return 0;

		case QSE_LSP_IO_READ:
		{
			/*
			if (qse_fgets (data, size, stdin) == QSE_NULL) 
			{
				if (ferror(stdin)) return -1;
				return 0;
			}
			return qse_lsp_strlen(data);
			*/

			qse_cint_t c;

			if (size <= 0) return -1;
			c = qse_fgetc (stdin);

			if (c == QSE_CHAR_EOF) 
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

static qse_ssize_t put_output (
	int cmd, void* arg, qse_char_t* data, qse_size_t size)
{
	switch (cmd) 
	{
		case QSE_LSP_IO_OPEN:
		case QSE_LSP_IO_CLOSE:
			return 0;

		case QSE_LSP_IO_WRITE:
		{
			int n = qse_fprintf (
				stdout, QSE_T("%.*s"), size, data);
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

static void* custom_lsp_malloc (void* custom, qse_size_t n)
{
#ifdef _WIN32
	return HeapAlloc (((prmfns_data_t*)custom)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* custom_lsp_realloc (void* custom, void* ptr, qse_size_t n)
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


static void custom_lsp_dprintf (void* custom, const qse_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	qse_vfprintf (stderr, fmt, ap);
	va_end (ap);
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
	qse_lsp_prmfns_t prmfns;
#ifdef _WIN32
	prmfns_data_t prmfns_data;
#endif

	if (handle_args (argc, argv) == -1) return -1;
	
	qse_memset (&prmfns, 0, QSE_SIZEOF(prmfns));

	prmfns.mmgr.alloc  = custom_lsp_malloc;
	prmfns.mmgr.realloc = custom_lsp_realloc;
	prmfns.mmgr.free    = custom_lsp_free;
#ifdef _WIN32
	prmfns_data.heap = HeapCreate (0, 1000000, 1000000);
	if (prmfns_data.heap == NULL)
	{
		qse_printf (QSE_T("Error: cannot create an lsp heap\n"));
		return -1;
	}

	prmfns.mmgr.data = &prmfns_data;
#else
	prmfns.mmgr.data = QSE_NULL;
#endif

	/* TODO: change prmfns ...... lsp_oepn... etc */
	qse_memcpy (&prmfns.ccls, QSE_CCLS_GETDFL(), QSE_SIZEOF(prmfns.ccls));

	prmfns.misc.sprintf = custom_lsp_sprintf;
	prmfns.misc.dprintf = custom_lsp_dprintf;
	prmfns.misc.data = QSE_NULL;

	lsp = qse_lsp_open (&prmfns, opt_memsize, opt_meminc);
	if (lsp == QSE_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (prmfns_data.heap);
#endif
		qse_printf (QSE_T("Error: cannot create a lsp instance\n"));
		return -1;
	}

	qse_printf (QSE_T("ASELSP 0.0001\n"));

	qse_lsp_attinput (lsp, get_input, QSE_NULL);
	qse_lsp_attoutput (lsp, put_output, QSE_NULL);

	while (1)
	{
		qse_printf (QSE_T("ASELSP $ "));
		qse_fflush (stdout);

		obj = qse_lsp_read (lsp);
		if (obj == QSE_NULL) 
		{
			int errnum;
			const qse_char_t* errmsg;

			qse_lsp_geterror (lsp, &errnum, &errmsg);

			if (errnum != QSE_LSP_EEND && 
			    errnum != QSE_LSP_EEXIT) 
			{
				qse_printf (
					QSE_T("error in read: [%d] %s\n"), 
					errnum, errmsg);
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
			int errnum;
			const qse_char_t* errmsg;

			qse_lsp_geterror (lsp, &errnum, &errmsg);
			if (errnum == QSE_LSP_EEXIT) break;

			qse_printf (
				QSE_T("error in eval: [%d] %s\n"), 
				errnum, errmsg);
		}
	}

	qse_lsp_close (lsp);

#ifdef _WIN32
	HeapDestroy (prmfns_data.heap);
#endif
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int n;

#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif

	n = qse_runmain (argc, argv, lsp_main);

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
