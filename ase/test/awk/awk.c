/*
 * $Id: awk.c,v 1.132 2006-11-29 03:55:56 bacon Exp $
 */

#include <ase/awk/awk.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>

#ifdef ASE_CHAR_IS_WCHAR
	#include <wchar.h>
	#include <wctype.h>
#endif

#if defined(_WIN32)
	#include <windows.h>
	#include <tchar.h>

	#define xp_printf _tprintf
	#pragma warning (disable: 4996)
	#pragma warning (disable: 4296)
#elif defined(__MSDOS__)
	#include <ctype.h>
	#include <stdlib.h>

	#define xp_printf printf
#else
	#include <xp/bas/stdio.h>
	#include <xp/bas/stdlib.h>
	#include <xp/bas/string.h>
	#include <xp/bas/memory.h>
	#include <xp/bas/sysapi.h>
	#include <xp/bas/locale.h>
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
	const ase_char_t* input_file;
	FILE* input_handle;
};

static FILE* fopen_t (const ase_char_t* path, const ase_char_t* mode)
{
#ifdef _WIN32
	return _tfopen (path, mode);
#else
	#ifdef ASE_CHAR_IS_MCHAR
	const ase_mchar_t* path_mb;
	const ase_mchar_t* mode_mb;
	#else
	ase_mchar_t path_mb[XP_PATH_MAX + 1];
	ase_mchar_t mode_mb[32];
	#endif

	#ifdef ASE_CHAR_IS_MCHAR
	path_mb = path;
	mode_mb = mode;
	#else
	if (xp_wcstomcs_strict (
		path, path_mb, ASE_COUNTOF(path_mb)) == -1) return ASE_NULL;
	if (xp_wcstomcs_strict (
		mode, mode_mb, ASE_COUNTOF(mode_mb)) == -1) return ASE_NULL;
	#endif

	return fopen (path_mb, mode_mb);
#endif
}

static int __awk_sprintf (
	ase_char_t* buf, ase_size_t len, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
#if defined(_WIN32)
	n = _vsntprintf (buf, len, fmt, ap);
	if (n < 0 || (ase_size_t)n >= len)
	{
		if (len > 0) buf[len-1] = ASE_T('\0');
		n = -1;
	}
#elif defined(__MSDOS__)
	/* TODO: check buffer overflow */
	n = vsprintf (buf, fmt, ap);
#else
	n = xp_vsprintf (buf, len, fmt, ap);
#endif
	va_end (ap);
	return n;
}

static void __awk_aprintf (const ase_char_t* fmt, ...)
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
#elif defined(__MSDOS__)
	vprintf (fmt, ap);
#else
	xp_vprintf (fmt, ap);
#endif
	va_end (ap);
}

static void __awk_dprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);

#if defined(_WIN32)
	_vftprintf (stderr, fmt, ap);
#elif defined(__MSDOS__)
	vfprintf (stderr, fmt, ap);
#else
	xp_vfprintf (stderr, fmt, ap);
#endif

	va_end (ap);
}

static ase_real_t __awk_pow (ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static FILE* popen_t (const ase_char_t* cmd, const ase_char_t* mode)
{
#if defined(_WIN32)
	return _tpopen (cmd, mode);
#elif defined(__MSDOS__)
	/* TODO: support this */
	return NULL;
#else
	#ifdef ASE_CHAR_IS_MCHAR
	const ase_mchar_t* cmd_mb;
	const ase_mchar_t* mode_mb;
	#else
	ase_mchar_t cmd_mb[2048];
	ase_mchar_t mode_mb[32];
	#endif

	#ifdef ASE_CHAR_IS_MCHAR
	cmd_mb = cmd;
	mode_mb = mode;
	#else
	if (xp_wcstomcs_strict (
		cmd, cmd_mb, ASE_COUNTOF(cmd_mb)) == -1) return ASE_NULL;
	if (xp_wcstomcs_strict (
		mode, mode_mb, ASE_COUNTOF(mode_mb)) == -1) return ASE_NULL;
	#endif

	return popen (cmd_mb, mode_mb);
#endif
}

#ifdef WIN32
	#define fgets_t _fgetts
	#define fputs_t _fputts
	#define fputc_t _fputtc
#else
	#ifdef ASE_CHAR_IS_MCHAR
		#define fgets_t fgets
		#define fputs_t fputs
		#define fputc_t fputc
	#else
		#define fgets_t fgetws
		#define fputc_t fputwc
	#endif
#endif

static ase_ssize_t process_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	struct src_io* src_io = (struct src_io*)arg;
	ase_char_t c;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		if (src_io->input_file == ASE_NULL) return 0;
		src_io->input_handle = fopen_t (src_io->input_file, ASE_T("r"));
		if (src_io->input_handle == NULL) return -1;
		return 1;
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		if (src_io->input_file == ASE_NULL) return 0;
		fclose ((FILE*)src_io->input_handle);
		return 0;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		if (size <= 0) return -1;
	#ifdef ASE_CHAR_IS_MCHAR
		c = fgetc ((FILE*)src_io->input_handle);
	#else
		c = fgetwc ((FILE*)src_io->input_handle);
	#endif
		if (c == ASE_CHAR_EOF) return 0;
		*data = c;
		return 1;
	}

	return -1;
}

static ase_ssize_t dump_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	/*struct src_io* src_io = (struct src_io*)arg;*/

	if (cmd == ASE_AWK_IO_OPEN) return 1;
	else if (cmd == ASE_AWK_IO_CLOSE) return 0;
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		ase_size_t i;
		for (i = 0; i < size; i++)
		{
		#ifdef ASE_CHAR_IS_MCHAR
			fputc (data[i], stdout);
		#else
			fputwc (data[i], stdout);
		#endif
		}
		return size;
	}

	return -1;
}

static ase_ssize_t process_extio_pipe (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;

	switch (cmd)
	{
		case ASE_AWK_IO_OPEN:
		{
			FILE* handle;
			const ase_char_t* mode;

			if (epa->mode == ASE_AWK_EXTIO_PIPE_READ)
				mode = ASE_T("r");
			else if (epa->mode == ASE_AWK_EXTIO_PIPE_WRITE)
				mode = ASE_T("w");
			else return -1; /* TODO: any way to set the error number? */
			__awk_dprintf (ASE_T("opending %s of type %d (pipe)\n"),  epa->name, epa->type);
			handle = popen_t (epa->name, mode);
			if (handle == NULL) return -1;
			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			__awk_dprintf (ASE_T("closing %s of type (pipe) %d\n"),  epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			if (fgets_t (data, size, (FILE*)epa->handle) == ASE_NULL) 
				return 0;
			return ase_awk_strlen(data);
		}

		case ASE_AWK_IO_WRITE:
		{
			ase_size_t i;
			/* TODO: how to return error or 0 */
			for (i = 0; i < size; i++)
			{
				fputc_t (data[i], (FILE*)epa->handle);
			}
			return size;
		}

		case ASE_AWK_IO_FLUSH:
		{
			if (epa->mode == ASE_AWK_EXTIO_PIPE_READ) return -1;
			else return 0;
		}

		case ASE_AWK_IO_NEXT:
		{
			return -1;
		}
	}

	return -1;
}

static ase_ssize_t process_extio_file (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;

	switch (cmd)
	{
		case ASE_AWK_IO_OPEN:
		{
			FILE* handle;
			const ase_char_t* mode;

			if (epa->mode == ASE_AWK_EXTIO_FILE_READ)
				mode = ASE_T("r");
			else if (epa->mode == ASE_AWK_EXTIO_FILE_WRITE)
				mode = ASE_T("w");
			else if (epa->mode == ASE_AWK_EXTIO_FILE_APPEND)
				mode = ASE_T("a");
			else return -1; /* TODO: any way to set the error number? */

			__awk_dprintf (ASE_T("opending %s of type %d (file)\n"), epa->name, epa->type);
			handle = fopen_t (epa->name, mode);
			if (handle == NULL) return -1;

			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			__awk_dprintf (ASE_T("closing %s of type %d (file)\n"), epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			if (fgets_t (data, size, (FILE*)epa->handle) == ASE_NULL) 
				return 0;
			return ase_awk_strlen(data);
		}

		case ASE_AWK_IO_WRITE:
		{
			ase_size_t i;
			/* TODO: how to return error or 0 */
			for (i = 0; i < size; i++)
			{
				fputc_t (data[i], (FILE*)epa->handle);
			}
			return size;
		}

		case ASE_AWK_IO_FLUSH:
		{
			if (fflush ((FILE*)epa->handle) == EOF) return -1;
			return 0;
		}

		case ASE_AWK_IO_NEXT:
		{
			return -1;
		}

	}

	return -1;
}

static int open_extio_console (ase_awk_extio_t* epa);
static int close_extio_console (ase_awk_extio_t* epa);
static int next_extio_console (ase_awk_extio_t* epa);

static ase_size_t infile_no = 0;
static const ase_char_t* infiles[10000] =
{
	/*
	ASE_T("c1.txt"),
	ASE_T("c2.txt"),
	ASE_T("c3.txt"),
	*/
	ASE_T(""),
	ASE_NULL
};


static ase_ssize_t process_extio_console (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		return open_extio_console (epa);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		return close_extio_console (epa);
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		while (fgets_t (data, size, epa->handle) == ASE_NULL)
		{
			/* it has reached the end of the current file.
			 * open the next file if available */
			if (infiles[infile_no] == ASE_NULL) 
			{
				/* no more input console */

				/* is this correct??? */
				/*
				if (epa->handle != ASE_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) fclose (epa->handle);
				epa->handle = ASE_NULL;
				*/

				return 0;
			}

			if (infiles[infile_no][0] == ASE_T('\0'))
			{
				if (epa->handle != ASE_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) fclose (epa->handle);
				epa->handle = stdin;
			}
			else
			{
				FILE* fp = fopen_t (infiles[infile_no], ASE_T("r"));
				if (fp == ASE_NULL)
				{
					__awk_dprintf (ASE_T("failed to open the next console of type %x - fopen failure\n"), epa->type);
					return -1;
				}

				if (epa->handle != ASE_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) fclose (epa->handle);

				__awk_dprintf (ASE_T("open the next console [%s]\n"), infiles[infile_no]);
				epa->handle = fp;
			}

			infile_no++;	
		}

		return ase_awk_strlen(data);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		ase_size_t i;
		/* TODO: how to return error or 0 */
		for (i = 0; i < size; i++)
		{
			fputc_t (data[i], (FILE*)epa->handle);
		}

		/*MessageBox (NULL, data, data, MB_OK);*/
		return size;
	}
	else if (cmd == ASE_AWK_IO_FLUSH)
	{
		if (fflush ((FILE*)epa->handle) == EOF) return -1;
		return 0;
	}
	else if (cmd == ASE_AWK_IO_NEXT)
	{
		return next_extio_console (epa);
	}

	return -1;
}

static int open_extio_console (ase_awk_extio_t* epa)
{
	/* TODO: OpenConsole in GUI APPLICATION */

	/* epa->name is always empty for console */
	assert (epa->name[0] == ASE_T('\0'));

	__awk_dprintf (ASE_T("opening console[%s] of type %x\n"), epa->name, epa->type);

	if (epa->mode == ASE_AWK_EXTIO_CONSOLE_READ)
	{
		if (infiles[infile_no] == ASE_NULL)
		{
			/* no more input file */
			__awk_dprintf (ASE_T("console - no more file\n"));;
			return 0;
		}

		if (infiles[infile_no][0] == ASE_T('\0'))
		{
			__awk_dprintf (ASE_T("    console(r) - <standard input>\n"));
			epa->handle = stdin;
		}
		else
		{
			/* a temporary variable fp is used here not to change 
			 * any fields of epa when the open operation fails */
			FILE* fp = fopen_t (infiles[infile_no], ASE_T("r"));
			if (fp == ASE_NULL)
			{
				__awk_dprintf (ASE_T("cannot open console of type %x - fopen failure\n"), epa->type);
				return -1;
			}

			__awk_dprintf (ASE_T("    console(r) - %s\n"), infiles[infile_no]);
			if (ase_awk_setfilename (
				epa->run, infiles[infile_no], 
				ase_awk_strlen(infiles[infile_no])) == -1)
			{
				fclose (fp);
				return -1;
			}

			epa->handle = fp;
		}

		infile_no++;
		return 1;
	}
	else if (epa->mode == ASE_AWK_EXTIO_CONSOLE_WRITE)
	{
		__awk_dprintf (ASE_T("    console(w) - <standard output>\n"));
		/* TODO: does output console has a name??? */
		/*ase_awk_setconsolename (ASE_T(""));*/
		epa->handle = stdout;
		return 1;
	}

	return -1;
}

static int close_extio_console (ase_awk_extio_t* epa)
{
	__awk_dprintf (ASE_T("closing console of type %x\n"), epa->type);

	if (epa->handle != ASE_NULL &&
	    epa->handle != stdin && 
	    epa->handle != stdout && 
	    epa->handle != stderr)
	{
		fclose (epa->handle);
	}

	/* TODO: CloseConsole in GUI APPLICATION */
	return 0;
}

static int next_extio_console (ase_awk_extio_t* epa)
{
	int n;
	FILE* fp = epa->handle;

	__awk_dprintf (ASE_T("switching console[%s] of type %x\n"), epa->name, epa->type);

	n = open_extio_console(epa);
	if (n == -1) return -1;

	if (n == 0) 
	{
		/* if there is no more file, keep the previous handle */
		return 0;
	}

	if (fp != ASE_NULL && fp != stdin && 
	    fp != stdout && fp != stderr) fclose (fp);

	return n;
}


ase_awk_t* app_awk = NULL;
void* app_run = NULL;

#ifdef _WIN32
static BOOL WINAPI __stop_run (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		ase_awk_stop (app_awk, app_run);
		return TRUE;
	}

	return FALSE;
}
#else
static void __stop_run (int sig)
{
	signal  (SIGINT, SIG_IGN);
	ase_awk_stop (app_awk, app_run);
	/*ase_awk_stoprun (awk, handle);*/
	/*ase_awk_stopallruns (awk); */
	signal  (SIGINT, __stop_run);
}
#endif

static void __on_run_start (ase_awk_t* awk, void* handle, void* arg)
{
	app_awk = awk;	
	app_run = handle;

	__awk_dprintf (ASE_T("AWK PRORAM ABOUT TO START...\n"));
}

static void __on_run_end (ase_awk_t* awk, void* handle, int errnum, void* arg)
{
	if (errnum != ASE_AWK_ENOERR)
	{
		__awk_dprintf (ASE_T("AWK PRORAM ABOUT TO END WITH AN ERROR - %d - %s\n"), errnum, ase_awk_geterrstr (errnum));
	}
	else __awk_dprintf (ASE_T("AWK PRORAM ENDED SUCCESSFULLY\n"));

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

static void* __awk_malloc (ase_size_t n, void* custom_data)
{
#ifdef _WIN32
	return HeapAlloc (((syscas_data_t*)custom_data)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* __awk_realloc (void* ptr, ase_size_t n, void* custom_data)
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

#if defined(ASE_CHAR_IS_MCHAR) 
	#if (__TURBOC__<=513) /* turboc 2.01 or earlier */
		static int __awk_isupper (int c) { return isupper (c); }
		static int __awk_islower (int c) { return islower (c); }
		static int __awk_isalpha (int c) { return isalpha (c); }
		static int __awk_isdigit (int c) { return isdigit (c); }
		static int __awk_isxdigit (int c) { return isxdigit (c); }
		static int __awk_isalnum (int c) { return isalnum (c); }
		static int __awk_isspace (int c) { return isspace (c); }
		static int __awk_isprint (int c) { return isprint (c); }
		static int __awk_isgraph (int c) { return isgraph (c); }
		static int __awk_iscntrl (int c) { return iscntrl (c); }
		static int __awk_ispunct (int c) { return ispunct (c); }
		static int __awk_toupper (int c) { return toupper (c); }
		static int __awk_tolower (int c) { return tolower (c); }
	#else
		#define __awk_isupper  isupper
		#define __awk_islower  islower
		#define __awk_isalpha  isalpha
		#define __awk_isdigit  isdigit
		#define __awk_isxdigit isxdigit
		#define __awk_isalnum  isalnum
		#define __awk_isspace  isspace
		#define __awk_isprint  isprint
		#define __awk_isgraph  isgraph
		#define __awk_iscntrl  iscntrl
		#define __awk_ispunct  ispunct
		#define __awk_toupper  tolower
		#define __awk_tolower  tolower
	#endif
#else
	#define __awk_isupper  iswupper
	#define __awk_islower  iswlower
	#define __awk_isalpha  iswalpha
	#define __awk_isdigit  iswdigit
	#define __awk_isxdigit iswxdigit
	#define __awk_isalnum  iswalnum
	#define __awk_isspace  iswspace
	#define __awk_isprint  iswprint
	#define __awk_isgraph  iswgraph
	#define __awk_iscntrl  iswcntrl
	#define __awk_ispunct  iswpunct

	#define __awk_toupper  towlower
	#define __awk_tolower  towlower
#endif

static int __handle_bfn (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	xp_printf (ASE_T("__handle_bfn\n"));
}

static int __main (int argc, ase_char_t* argv[])
{
	ase_awk_t* awk;
	ase_awk_srcios_t srcios;
	ase_awk_runcbs_t runcbs;
	ase_awk_runios_t runios;
	ase_awk_runarg_t runarg[10];
	ase_awk_syscas_t syscas;
	struct src_io src_io = { NULL, NULL };
	int opt, i, file_count = 0;
#ifdef _WIN32
	syscas_data_t syscas_data;
#endif
	const ase_char_t* mfn = ASE_NULL;

	opt = ASE_AWK_IMPLICIT | 
	      ASE_AWK_EXPLICIT | 
	      ASE_AWK_UNIQUEAFN | 
	      ASE_AWK_HASHSIGN | 
	      /*ASE_AWK_DBLSLASHES |*/ 
	      ASE_AWK_SHADING | 
	      ASE_AWK_SHIFT | 
	      ASE_AWK_EXTIO | 
	      ASE_AWK_BLOCKLESS | 
	      ASE_AWK_STRINDEXONE | 
	      ASE_AWK_STRIPSPACES | 
	      ASE_AWK_NEXTOFILE;

	if (argc <= 1)
	{
		xp_printf (ASE_T("Usage: %s [-m] source_file [data_file ...]\n"), argv[0]);
		return -1;
	}

	for (i = 1; i < argc; i++)
	{
		if (ase_awk_strcmp(argv[i], ASE_T("-m")) == 0)
		{
			mfn = ASE_T("main");
		}
		else if (file_count == 0)
		{
			src_io.input_file = argv[i];
			file_count++;
		}
		else if (file_count >= 1 && file_count < ASE_COUNTOF(infiles)-1)
		{
			infiles[file_count-1] = argv[i];
			infiles[file_count] = ASE_NULL;
			file_count++;
		}
		else
		{
			xp_printf (ASE_T("Usage: %s [-m] [-f source_file] [data_file ...]\n"), argv[0]);
			return -1;
		}
	}

	memset (&syscas, 0, ASE_SIZEOF(syscas));
	syscas.malloc = __awk_malloc;
	syscas.realloc = __awk_realloc;
	syscas.free = __awk_free;

	syscas.lock = NULL;
	syscas.unlock = NULL;

	syscas.is_upper  = __awk_isupper;
	syscas.is_lower  = __awk_islower;
	syscas.is_alpha  = __awk_isalpha;
	syscas.is_digit  = __awk_isdigit;
	syscas.is_xdigit = __awk_isxdigit;
	syscas.is_alnum  = __awk_isalnum;
	syscas.is_space  = __awk_isspace;
	syscas.is_print  = __awk_isprint;
	syscas.is_graph  = __awk_isgraph;
	syscas.is_cntrl  = __awk_iscntrl;
	syscas.is_punct  = __awk_ispunct;
	syscas.to_upper  = __awk_toupper;
	syscas.to_lower  = __awk_tolower;

	syscas.memcpy = memcpy;
	syscas.memset = memset;
	syscas.pow = __awk_pow;
	syscas.sprintf = __awk_sprintf;
	syscas.aprintf = __awk_aprintf;
	syscas.dprintf = __awk_dprintf;
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

	if ((awk = ase_awk_open(&syscas)) == ASE_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (syscas_data.heap);
#endif
		xp_printf (ASE_T("Error: cannot open awk\n"));
		return -1;
	}

	ase_awk_setopt (awk, opt);

	srcios.in = process_source;
	srcios.out = dump_source;
	srcios.custom_data = &src_io;


ase_awk_addbfn (awk, ASE_T("bufa"), 4, 0, 
	1, 1, ASE_NULL, __handle_bfn);
	

	ase_awk_setmaxparsedepth (
		awk, ASE_AWK_DEPTH_BLOCK | ASE_AWK_DEPTH_EXPR, 20);

	if (ase_awk_parse (awk, &srcios) == -1) 
	{
		int errnum = ase_awk_geterrnum(awk);
		xp_printf (
			ASE_T("ERROR: cannot parse program - line %u [%d] %s\n"), 
			(unsigned int)ase_awk_getsrcline(awk), 
			errnum, ase_awk_geterrstr(errnum));
		ase_awk_close (awk);
		return -1;
	}

#ifdef _WIN32
	SetConsoleCtrlHandler (__stop_run, TRUE);
#else
	signal (SIGINT, __stop_run);
#endif

	runios.pipe = process_extio_pipe;
	runios.coproc = ASE_NULL;
	runios.file = process_extio_file;
	runios.console = process_extio_console;

	runcbs.on_start = __on_run_start;
	runcbs.on_end = __on_run_end;
	runcbs.custom_data = ASE_NULL;

	runarg[0].ptr = ASE_T("argument 0");
	runarg[0].len = ase_awk_strlen(runarg[0].ptr);
	runarg[1].ptr = ASE_T("argumetn 1");
	runarg[1].len = ase_awk_strlen(runarg[1].ptr);
	runarg[2].ptr = ASE_T("argumetn 2");
	runarg[2].len = ase_awk_strlen(runarg[2].ptr);
	runarg[3].ptr = ASE_NULL;
	runarg[3].len = 0;

	if (ase_awk_run (awk, mfn, &runios, &runcbs, runarg, ASE_NULL) == -1)
	{
		int errnum = ase_awk_geterrnum(awk);
		xp_printf (
			ASE_T("error: cannot run program - [%d] %s\n"), 
			errnum, ase_awk_geterrstr(errnum));
		ase_awk_close (awk);
		return -1;
	}

	ase_awk_close (awk);
#ifdef _WIN32
	HeapDestroy (syscas_data.heap);
#endif
	return 0;
}

#ifdef _WIN32
/*
NTSYSAPI PTEB NTAPI NtCurrentTeb();
Function NtCurrentTeb returns address of TEB (Thread Environment Block) for calling thread. 
NtCurrentTeb isn't typical NT CALL realised via INT 2E, becouse TEB is accessable at address fs:[0018h].
Microsoft declare NtCurrentTeb as __cdecl, but ntdll.dll export it as __stdcall (it don't have metter, becouse function don't have any parameters), so you cannot use ntdll.dll export. In this case the better way is write NtCurrentTeb manually, declaring it as __cdecl. 

typedef UCHAR BOOLEAN;

typedef struct _TEB {

  NT_TIB                  Tib;
  PVOID                   EnvironmentPointer;
  CLIENT_ID               Cid;
  PVOID                   ActiveRpcInfo;
  PVOID                   ThreadLocalStoragePointer;
  PPEB                    Peb;
  ULONG                   LastErrorValue;
  ULONG                   CountOfOwnedCriticalSections;
  PVOID                   CsrClientThread;
  PVOID                   Win32ThreadInfo;
  ULONG                   Win32ClientInfo[0x1F];
  PVOID                   WOW32Reserved;
  ULONG                   CurrentLocale;
  ULONG                   FpSoftwareStatusRegister;
  PVOID                   SystemReserved1[0x36];
  PVOID                   Spare1;
  ULONG                   ExceptionCode;
  ULONG                   SpareBytes1[0x28];
  PVOID                   SystemReserved2[0xA];
  ULONG                   GdiRgn;
  ULONG                   GdiPen;
  ULONG                   GdiBrush;
  CLIENT_ID               RealClientId;
  PVOID                   GdiCachedProcessHandle;
  ULONG                   GdiClientPID;
  ULONG                   GdiClientTID;
  PVOID                   GdiThreadLocaleInfo;
  PVOID                   UserReserved[5];
  PVOID                   GlDispatchTable[0x118];
  ULONG                   GlReserved1[0x1A];
  PVOID                   GlReserved2;
  PVOID                   GlSectionInfo;
  PVOID                   GlSection;
  PVOID                   GlTable;
  PVOID                   GlCurrentRC;
  PVOID                   GlContext;
  NTSTATUS                LastStatusValue;
  UNICODE_STRING          StaticUnicodeString;
  WCHAR                   StaticUnicodeBuffer[0x105];
  PVOID                   DeallocationStack;
  PVOID                   TlsSlots[0x40];
  LIST_ENTRY              TlsLinks;
  PVOID                   Vdm;
  PVOID                   ReservedForNtRpc;
  PVOID                   DbgSsReserved[0x2];
  ULONG                   HardErrorDisabled;
  PVOID                   Instrumentation[0x10];
  PVOID                   WinSockData;
  ULONG                   GdiBatchCount;
  ULONG                   Spare2;
  ULONG                   Spare3;
  ULONG                   Spare4;
  PVOID                   ReservedForOle;
  ULONG                   WaitingOnLoaderLock;
  PVOID                   StackCommit;
  PVOID                   StackCommitMax;
  PVOID                   StackReserved;

} TEB, *PTEB;

typedef struct _PEB {
  BOOLEAN                 InheritedAddressSpace;
  BOOLEAN                 ReadImageFileExecOptions;
  BOOLEAN                 BeingDebugged;
  BOOLEAN                 Spare;
  HANDLE                  Mutant;
  PVOID                   ImageBaseAddress;
  PPEB_LDR_DATA           LoaderData;
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  PVOID                   SubSystemData;
  PVOID                   ProcessHeap;
  PVOID                   FastPebLock;
  PPEBLOCKROUTINE         FastPebLockRoutine;
  PPEBLOCKROUTINE         FastPebUnlockRoutine;
  ULONG                   EnvironmentUpdateCount;
  PPVOID                  KernelCallbackTable;
  PVOID                   EventLogSection;
  PVOID                   EventLog;
  PPEB_FREE_BLOCK         FreeList;
  ULONG                   TlsExpansionCounter;
  PVOID                   TlsBitmap;
  ULONG                   TlsBitmapBits[0x2];
  PVOID                   ReadOnlySharedMemoryBase;
  PVOID                   ReadOnlySharedMemoryHeap;
  PPVOID                  ReadOnlyStaticServerData;
  PVOID                   AnsiCodePageData;
  PVOID                   OemCodePageData;
  PVOID                   UnicodeCaseTableData;
  ULONG                   NumberOfProcessors;
  ULONG                   NtGlobalFlag;
  BYTE                    Spare2[0x4];
  LARGE_INTEGER           CriticalSectionTimeout;
  ULONG                   HeapSegmentReserve;
  ULONG                   HeapSegmentCommit;
  ULONG                   HeapDeCommitTotalFreeThreshold;
  ULONG                   HeapDeCommitFreeBlockThreshold;
  ULONG                   NumberOfHeaps;
  ULONG                   MaximumNumberOfHeaps;
  PPVOID                  *ProcessHeaps;
  PVOID                   GdiSharedHandleTable;
  PVOID                   ProcessStarterHelper;
  PVOID                   GdiDCAttributeList;
  PVOID                   LoaderLock;
  ULONG                   OSMajorVersion;
  ULONG                   OSMinorVersion;
  ULONG                   OSBuildNumber;
  ULONG                   OSPlatformId;
  ULONG                   ImageSubSystem;
  ULONG                   ImageSubSystemMajorVersion;
  ULONG                   ImageSubSystemMinorVersion;
  ULONG                   GdiHandleBuffer[0x22];
  ULONG                   PostProcessInitRoutine;
  ULONG                   TlsExpansionBitmap;
  BYTE                    TlsExpansionBitmapBits[0x80];
  ULONG                   SessionId;

} PEB, *PPEB;


*/
void* /*__declspec(naked)*/ get_current_teb (void)
{
	_asm
	{
		mov eax, fs:[0x18]
	}
}

void* get_current_peb (void)
{
	void* teb = get_current_teb ();
	return *(void**)((char*)teb + 0x30);
}

int is_debugger_present (void)
{
	void *peb = get_current_peb ();
	return *((char*)peb+0x02);
}


int /*__declspec(naked)*/ is_debugger_present2 (void)
{
	_asm
	{
		mov eax, fs:[0x18]
		mov ebx, [eax+0x30]
		xor eax, eax
		mov al, byte ptr[ebx+0x02]
	}
}
#endif

#if defined(_WIN32)
int _tmain (int argc, ase_char_t* argv[])
#elif defined(__MSDOS__)
int main (int argc, ase_char_t* argv[])
#else
int xp_main (int argc, ase_char_t* argv[])
#endif
{
	int n;
#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif
/*#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif*/

#if defined(_WIN32)
	if (IsDebuggerPresent ())
	{
		_tprintf (_T("Running application in a debugger....\n"));
	}
	if (is_debugger_present ())
	{
		_tprintf (_T("Running application in a debugger by is_debugger_present...\n"));
	}
	if (is_debugger_present2 ())
	{
		_tprintf (_T("Running application in a debugger by is_debugger_present2...\n"));
	}
#endif

#if defined(__unix) || defined(__unix__)
	xp_setlocale ();
#endif

	n = __main (argc, argv);

#if defined(__linux) && defined(_DEBUG)
	muntrace ();
#endif
#if defined(_WIN32) && defined(_DEBUG)
	#if defined(_MSC_VER)
	_CrtDumpMemoryLeaks ();
	#endif
	_tprintf (_T("Press ENTER to quit\n"));
	getchar ();
#endif

	return n;
}

