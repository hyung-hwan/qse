/*
 * $Id: awk.c,v 1.84 2006-09-01 06:23:57 bacon Exp $
 */

#include <xp/awk/awk.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#ifdef XP_CHAR_IS_WCHAR
	#include <wchar.h>
#endif

#ifndef __STAND_ALONE
	#include <xp/bas/stdio.h>
	#include <xp/bas/string.h>
	#include <xp/bas/memory.h>
	#include <xp/bas/sysapi.h>
	#include <xp/bas/assert.h>	
	#include <xp/bas/locale.h>	
#else
	#include <limits.h>
	#ifndef PATH_MAX
		#define XP_PATH_MAX 4096
	#else
		#define XP_PATH_MAX PATH_MAX
	#endif
#endif

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#pragma warning (disable: 4996)
#endif

#ifdef __STAND_ALONE
	#include <assert.h>
	#define xp_assert assert
#endif

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if defined(__linux) && defined(_DEBUG)
#include <mcheck.h>
#endif

struct src_io
{
	const xp_char_t* input_file;
	FILE* input_handle;
};

static FILE* fopen_t (const xp_char_t* path, const xp_char_t* mode)
{
#ifdef _WIN32
	return _tfopen (path, mode);
#else
	#ifdef XP_CHAR_IS_MCHAR
	const xp_mchar_t* path_mb;
	const xp_mchar_t* mode_mb;
	#else
	xp_mchar_t path_mb[XP_PATH_MAX + 1];
	xp_mchar_t mode_mb[32];
	#endif

	#ifdef XP_CHAR_IS_MCHAR
	path_mb = path;
	mode_mb = mode;
	#else
	if (xp_wcstomcs_strict (
		path, path_mb, xp_countof(path_mb)) == -1) return XP_NULL;
	if (xp_wcstomcs_strict (
		mode, mode_mb, xp_countof(mode_mb)) == -1) return XP_NULL;
	#endif

	return fopen (path_mb, mode_mb);
#endif
}

static FILE* popen_t (const xp_char_t* cmd, const xp_char_t* mode)
{
#ifdef _WIN32
	return _tpopen (cmd, mode);
#else
	#ifdef XP_CHAR_IS_MCHAR
	const xp_mchar_t* cmd_mb;
	const xp_mchar_t* mode_mb;
	#else
	xp_mchar_t cmd_mb[2048];
	xp_mchar_t mode_mb[32];
	#endif

	#ifdef XP_CHAR_IS_MCHAR
	cmd_mb = cmd;
	mode_mb = mode;
	#else
	if (xp_wcstomcs_strict (
		cmd, cmd_mb, xp_countof(cmd_mb)) == -1) return XP_NULL;
	if (xp_wcstomcs_strict (
		mode, mode_mb, xp_countof(mode_mb)) == -1) return XP_NULL;
	#endif

	return popen (cmd_mb, mode_mb);
#endif
}

#ifdef WIN32
	#define fgets_t _fgetts
	#define fputs_t _fputts
#else
	#ifdef XP_CHAR_IS_MCHAR
		#define fgets_t fgets
		#define fputs_t fputs
	#else
		#define fgets_t fgetws
		#define fputs_t fputws
	#endif
#endif

static xp_ssize_t process_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	struct src_io* src_io = (struct src_io*)arg;
	xp_char_t c;

	if (cmd == XP_AWK_IO_OPEN)
	{
		if (src_io->input_file == XP_NULL) return 0;
		src_io->input_handle = fopen_t (src_io->input_file, XP_T("r"));
		if (src_io->input_handle == NULL) return -1;
		return 1;
	}
	else if (cmd == XP_AWK_IO_CLOSE)
	{
		if (src_io->input_file == XP_NULL) return 0;
		fclose ((FILE*)src_io->input_handle);
		return 0;
	}
	else if (cmd == XP_AWK_IO_READ)
	{
		if (size <= 0) return -1;
	#ifdef XP_CHAR_IS_MCHAR
		c = fgetc ((FILE*)src_io->input_handle);
	#else
		c = fgetwc ((FILE*)src_io->input_handle);
	#endif
		if (c == XP_CHAR_EOF) return 0;
		*data = c;
		return 1;
	}

	return -1;
}

static xp_ssize_t dump_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	/*struct src_io* src_io = (struct src_io*)arg;*/

	if (cmd == XP_AWK_IO_OPEN || cmd == XP_AWK_IO_CLOSE) return 0;
	else if (cmd == XP_AWK_IO_WRITE)
	{
		xp_size_t i;
		for (i = 0; i < size; i++)
		{
	#ifdef XP_CHAR_IS_MCHAR
			fputc (data[i], stdout);
	#else
			fputwc (data[i], stdout);
	#endif
		}
		return size;
	}

	return -1;
}

static xp_ssize_t process_extio_pipe (
	int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_awk_extio_t* epa = (xp_awk_extio_t*)arg;

	switch (cmd)
	{
		case XP_AWK_IO_OPEN:
		{
			FILE* handle;
			const xp_char_t* mode;

			if (epa->mode == XP_AWK_IO_PIPE_READ)
				mode = XP_T("r");
			else if (epa->mode == XP_AWK_IO_PIPE_WRITE)
				mode = XP_T("w");
			else return -1; /* TODO: any way to set the error number? */
xp_printf (XP_TEXT("opending %s of type %d (pipe)\n"),  epa->name, epa->type);
			handle = popen_t (epa->name, mode);
			if (handle == NULL) return -1;
			epa->handle = (void*)handle;
			return 1;
		}

		case XP_AWK_IO_CLOSE:
		{
xp_printf (XP_TEXT("closing %s of type (pipe) %d\n"),  epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case XP_AWK_IO_READ:
		{
			if (fgets_t (data, size, (FILE*)epa->handle) == XP_NULL) 
				return 0;
			return xp_awk_strlen(data);
		}

		case XP_AWK_IO_WRITE:
		{
			/* TODO: size... */
			fputs_t (data, (FILE*)epa->handle);
			return size;
		}

		case XP_AWK_IO_FLUSH:
		{
			if (epa->mode == XP_AWK_IO_PIPE_READ) return -1;
			else return 0;
		}

		case XP_AWK_IO_NEXT:
		{
			return -1;
		}
	}

	return -1;
}

static xp_ssize_t process_extio_file (
	int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_awk_extio_t* epa = (xp_awk_extio_t*)arg;

	switch (cmd)
	{
		case XP_AWK_IO_OPEN:
		{
			FILE* handle;
			const xp_char_t* mode;

			if (epa->mode == XP_AWK_IO_FILE_READ)
				mode = XP_T("r");
			else if (epa->mode == XP_AWK_IO_FILE_WRITE)
				mode = XP_T("w");
			else if (epa->mode == XP_AWK_IO_FILE_APPEND)
				mode = XP_T("a");
			else return -1; /* TODO: any way to set the error number? */

xp_printf (XP_TEXT("opending %s of type %d (file)\n"),  epa->name, epa->type);
			handle = fopen_t (epa->name, mode);
			if (handle == NULL) return -1;
			epa->handle = (void*)handle;
			return 1;
		}

		case XP_AWK_IO_CLOSE:
		{
xp_printf (XP_TEXT("closing %s of type %d (file)\n"),  epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case XP_AWK_IO_READ:
		{
			if (fgets_t (data, size, (FILE*)epa->handle) == XP_NULL) 
				return 0;
			return xp_awk_strlen(data);
		}

		case XP_AWK_IO_WRITE:
		{
			/* TODO: how to return error or 0 */
			fputs_t (data, /*size,*/ (FILE*)epa->handle);
			return size;
		}

		case XP_AWK_IO_FLUSH:
		{
			if (fflush ((FILE*)epa->handle) == EOF) return -1;
			return 0;
		}

		case XP_AWK_IO_NEXT:
		{
			return -1;
		}

	}

	return -1;
}

static int open_extio_console (xp_awk_extio_t* epa);
static int close_extio_console (xp_awk_extio_t* epa);
static int next_extio_console (xp_awk_extio_t* epa);

static const xp_char_t* infiles[] =
{
	//XP_T(""),
	XP_T("awk.in"),
	XP_NULL
};

static xp_size_t infile_no = 0;

static xp_ssize_t process_extio_console (
	int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_awk_extio_t* epa = (xp_awk_extio_t*)arg;

	if (cmd == XP_AWK_IO_OPEN)
	{
		return open_extio_console (epa);
	}
	else if (cmd == XP_AWK_IO_CLOSE)
	{
		return close_extio_console (epa);
	}
	else if (cmd == XP_AWK_IO_READ)
	{
		while (fgets_t (data, size, epa->handle) == XP_NULL)
		{
			/* it has reached the end of the current file.
			 * open the next file if available */
			if (infiles[infile_no] == XP_NULL) 
			{
				/* no more input console */

				/* is this correct??? */
				/*
				if (epa->handle != XP_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) fclose (epa->handle);
				epa->handle = XP_NULL;
				*/

				return 0;
			}

			if (infiles[infile_no][0] == XP_T('\0'))
			{
				if (epa->handle != XP_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) fclose (epa->handle);
				epa->handle = stdin;
			}
			else
			{
				FILE* fp = fopen_t (infiles[infile_no], XP_T("r"));
				if (fp == XP_NULL)
				{
xp_printf (XP_TEXT("failed to open the next console of type %x - fopen failure\n"), epa->type);
					return -1;
				}

				if (epa->handle != XP_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) fclose (epa->handle);

xp_printf (XP_TEXT("open the next console [%s]\n"), infiles[infile_no]);
				epa->handle = fp;
			}

			infile_no++;	
		}

		return xp_awk_strlen(data);
	}
	else if (cmd == XP_AWK_IO_WRITE)
	{
		/* TODO: how to return error or 0 */
		fputs_t (data, /*size,*/ (FILE*)epa->handle);
		/*MessageBox (NULL, data, data, MB_OK);*/
		return size;
	}
	else if (cmd == XP_AWK_IO_FLUSH)
	{
		if (fflush ((FILE*)epa->handle) == EOF) return -1;
		return 0;
	}
	else if (cmd == XP_AWK_IO_NEXT)
	{
		return next_extio_console (epa);
	}

	return -1;
}

static int open_extio_console (xp_awk_extio_t* epa)
{
	/* TODO: OpenConsole in GUI APPLICATION */

	/* epa->name is always empty for console */
	xp_assert (epa->name[0] == XP_T('\0'));

xp_printf (XP_TEXT("opening console[%s] of type %x\n"), epa->name, epa->type);

	if (epa->mode == XP_AWK_IO_CONSOLE_READ)
	{
		if (infiles[infile_no] == XP_NULL)
		{
			/* no more input file */
xp_printf (XP_TEXT("console - no more file\n"));;
			return 0;
		}

		if (infiles[infile_no][0] == XP_T('\0'))
		{
xp_printf (XP_T("    console(r) - <standard input>\n"));
			epa->handle = stdin;
		}
		else
		{
			/* a temporary variable fp is used here not to change 
			 * any fields of epa when the open operation fails */
			FILE* fp = fopen_t (infiles[infile_no], XP_T("r"));
			if (fp == XP_NULL)
			{
xp_printf (XP_TEXT("failed to open console of type %x - fopen failure\n"), epa->type);
				return -1;
			}

xp_printf (XP_T("    console(r) - %s\n"), infiles[infile_no]);
			epa->handle = fp;
		}

		infile_no++;
		return 1;
	}
	else if (epa->mode == XP_AWK_IO_CONSOLE_WRITE)
	{
xp_printf (XP_T("    console(w) - <standard output>\n"));
		epa->handle = stdout;
		return 1;
	}

	return -1;
}

static int close_extio_console (xp_awk_extio_t* epa)
{
xp_printf (XP_TEXT("closing console of type %x\n"), epa->type);

	if (epa->handle != XP_NULL &&
	    epa->handle != stdin && 
	    epa->handle != stdout && 
	    epa->handle != stderr)
	{
		fclose (epa->handle);
	}

	/* TODO: CloseConsole in GUI APPLICATION */
	return 0;
}

static int next_extio_console (xp_awk_extio_t* epa)
{
	int n;
	FILE* fp = epa->handle;
xp_printf (XP_TEXT("switching console[%s] of type %x\n"), epa->name, epa->type);

	n = open_extio_console(epa);
	if (n == -1) return -1;

	if (n == 0) 
	{
		/* if there is no more file, keep the previous handle */
		return 0;
	}

	if (fp != XP_NULL && fp != stdin && 
	    fp != stdout && fp != stderr) fclose (fp);

	return n;
}


xp_awk_t* app_awk = NULL;
void* app_run = NULL;

#ifdef _WIN32
static BOOL WINAPI __stop_run (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		xp_awk_stop (app_awk, app_run);
		return TRUE;
	}

	return FALSE;
}
#else
static void __stop_run (int sig)
{
	signal  (SIGINT, SIG_IGN);
	xp_awk_stop (app_awk, app_run);
	//xp_awk_stoprun (awk, handle);
	/*xp_awk_stopallruns (awk); */
	signal  (SIGINT, __stop_run);
}
#endif

static void __on_run_start (xp_awk_t* awk, void* handle, void* arg)
{
	app_awk = awk;	
	app_run = handle;
xp_printf (XP_T("AWK PRORAM ABOUT TO START...\n"));
}

static void __on_run_end (xp_awk_t* awk, void* handle, int errnum, void* arg)
{
	if (errnum != XP_AWK_ENOERR)
	{
		xp_printf (XP_T("AWK PRORAM ABOUT TO END WITH AN ERROR - %d - %s\n"), errnum, xp_awk_geterrstr (errnum));
	}
	else xp_printf (XP_T("AWK PRORAM ENDED SUCCESSFULLY\n"));

	app_awk = NULL;	
	app_run = NULL;
}

#ifdef _WIN32
typedef struct syscas_data_t syscas_data_t;
struct syscas_data_t
{
	HANDLE heap;
};
#endif

static void* __awk_malloc (xp_size_t n, void* custom_data)
{
#ifdef _WIN32
	return HeapAlloc (((syscas_data_t*)custom_data)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* __awk_realloc (void* ptr, xp_size_t n, void* custom_data)
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

static void __awk_free (void* ptr, void* custom_data)
{
#ifdef _WIN32
	HeapFree (((syscas_data_t*)custom_data)->heap, 0, ptr);
#else
	free (ptr);
#endif
}

#if defined(__STAND_ALONE) && !defined(_WIN32)
static int __main (int argc, char* argv[])
#else
static int __main (int argc, xp_char_t* argv[])
#endif
{
	xp_awk_t* awk;
	xp_awk_srcios_t srcios;
	xp_awk_runcbs_t runcbs;
	xp_awk_runios_t runios;
	xp_awk_syscas_t syscas;
	struct src_io src_io = { NULL, NULL };
	int opt, i, file_count = 0;
#ifdef _WIN32
	syscas_data_t syscas_data;
#endif

	opt = XP_AWK_EXPLICIT | XP_AWK_UNIQUE | XP_AWK_HASHSIGN |
		/*XP_AWK_DBLSLASHES |*/
		XP_AWK_SHADING | XP_AWK_IMPLICIT | XP_AWK_SHIFT | 
		XP_AWK_EXTIO | XP_AWK_BLOCKLESS | XP_AWK_STRINDEXONE;

	if (argc <= 1)
	{
		xp_printf (XP_T("Usage: %s [-m] source_file [data_file]\n"), argv[0]);
		return -1;
	}

	for (i = 1; i < argc; i++)
	{
#if defined(__STAND_ALONE) && !defined(_WIN32)
		if (strcmp(argv[i], "-m") == 0)
#else
		if (xp_awk_strcmp(argv[i], XP_T("-m")) == 0)
#endif
		{
			opt |= XP_AWK_RUNMAIN;
		}
		else if (file_count == 0)
		{
			src_io.input_file = argv[i];
			file_count++;
		}
		else if (file_count == 1)
		{
			infiles[0] = argv[i];
			file_count++;
		}
		else
		{
			xp_awk_close (awk);
			xp_printf (XP_T("Usage: %s [-m] source_file [data_file]\n"), argv[0]);
			return -1;
		}
	}

	memset (&syscas, 0, sizeof(syscas));
	syscas.malloc = __awk_malloc;
	syscas.realloc = __awk_realloc;
	syscas.free = __awk_free;

	syscas.lock = NULL;
	syscas.unlock = NULL;

#ifdef XP_CHAR_IS_MCHAR
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

#ifdef _WIN32
	syscas_data.heap = HeapCreate (0, 1000000, 1000000);
	if (syscas_data.heap == NULL)
	{
		xp_printf (XP_T("Error: cannot create an awk heap\n"));
		return -1;
	}

	syscas.custom_data = &syscas_data;
#endif

	if ((awk = xp_awk_open(&syscas)) == XP_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (syscas_data.heap);
#endif
		xp_printf (XP_T("Error: cannot open awk\n"));
		return -1;
	}

	xp_awk_setopt (awk, opt);

	srcios.in = process_source;
	srcios.out = dump_source;
	srcios.custom_data = &src_io;

	if (xp_awk_parse (awk, &srcios) == -1) 
	{
		int errnum = xp_awk_geterrnum(awk);
#if defined(__STAND_ALONE) && !defined(_WIN32) && defined(XP_CHAR_IS_WCHAR)
		xp_printf (
			XP_T("ERROR: cannot parse program - line %u [%d] %ls\n"), 
			(unsigned int)xp_awk_getsrcline(awk), 
			errnum, xp_awk_geterrstr(errnum));
#else
		xp_printf (
			XP_T("ERROR: cannot parse program - line %u [%d] %s\n"), 
			(unsigned int)xp_awk_getsrcline(awk), 
			errnum, xp_awk_geterrstr(errnum));
#endif
		xp_awk_close (awk);
		return -1;
	}

#ifdef _WIN32
	SetConsoleCtrlHandler (__stop_run, TRUE);
#else
	signal (SIGINT, __stop_run);
#endif

	runios.pipe = process_extio_pipe;
	runios.coproc = XP_NULL;
	runios.file = process_extio_file;
	runios.console = process_extio_console;

	runcbs.on_start = __on_run_start;
	runcbs.on_end = __on_run_end;
	runcbs.custom_data = XP_NULL;

	if (xp_awk_run (awk, &runios, &runcbs) == -1)
	{
		int errnum = xp_awk_geterrnum(awk);
#if defined(__STAND_ALONE) && !defined(_WIN32) && defined(XP_CHAR_IS_WCHAR)
		xp_printf (
			XP_T("error: cannot run program - [%d] %ls\n"), 
			errnum, xp_awk_geterrstr(errnum));
#else
		xp_printf (
			XP_T("error: cannot run program - [%d] %s\n"), 
			errnum, xp_awk_geterrstr(errnum));
#endif

		xp_awk_close (awk);
		return -1;
	}

	xp_awk_close (awk);
#ifdef _WIN32
	HeapDestroy (syscas_data.heap);
#endif
	return 0;
}

#if defined(__STAND_ALONE) && !defined(_WIN32)
int main (int argc, char* argv[])
#else
int xp_main (int argc, xp_char_t* argv[])
#endif
{
	int n;
#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif
/*#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif*/

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

