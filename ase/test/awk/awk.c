/*
 * $Id: awk.c,v 1.155 2007-01-26 16:08:55 bacon Exp $
 */

#include <ase/awk/awk.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#if defined(_WIN32)
	#include <windows.h>
	#include <tchar.h>
	#pragma warning (disable: 4996)
	#pragma warning (disable: 4296)
#elif defined(ASE_CHAR_IS_MCHAR)
	#include <ctype.h>
	#include <locale.h>
#else
	#include <wchar.h>
	#include <wctype.h>
	#include <locale.h>
	#include "../../etc/printf.c"
	#include "../../etc/main.c"
#endif

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if defined(__linux) && defined(_DEBUG)
#include <mcheck.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

struct src_io
{
	const ase_char_t* input_file;
	FILE* input_handle;
};

static ase_real_t awk_pow (ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static void awk_abort (void* custom_data)
{
	abort ();
}

static int awk_sprintf (
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
#elif defined(ASE_CHAR_IS_MCHAR)
	n = vsnprintf (buf, len, fmt, ap);
#else
	n = ase_vsprintf (buf, len, fmt, ap);
#endif
	va_end (ap);
	return n;
}

static void awk_aprintf (const ase_char_t* fmt, ...)
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
#elif defined(ASE_CHAR_IS_MCHAR)
	vprintf (fmt, ap);
#else
	ase_vprintf (fmt, ap);
#endif
	va_end (ap);
}

static void awk_dprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);

#if defined(_WIN32)
	_vftprintf (stderr, fmt, ap);
#elif defined(ASE_CHAR_IS_MCHAR)
	vfprintf (stderr, fmt, ap);
#else
	ase_vfprintf (stderr, fmt, ap);
#endif

	va_end (ap);
}

static void awk_printf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);

#if defined(_WIN32)
	_vtprintf (fmt, ap);
#elif defined(ASE_CHAR_IS_MCHAR)
	vprintf (fmt, ap);
#else
	ase_vprintf (fmt, ap);
#endif
	va_end (ap);
}

static FILE* awk_fopen (const ase_char_t* path, const ase_char_t* mode)
{
#ifdef _WIN32
	return _tfopen (path, mode);
#elif defined(ASE_CHAR_IS_MCHAR)
	return fopen (path, mode);
#else

	char path_mb[PATH_MAX + 1];
	char mode_mb[32];
	size_t n;

	n = wcstombs (path_mb, path, ASE_COUNTOF(path_mb));
	if (n == -1) return NULL;
	if (n == ASE_COUNTOF(path_mb)) path_mb[ASE_COUNTOF(path_mb)-1] = '\0';

	n = wcstombs (mode_mb, mode, ASE_COUNTOF(mode_mb));
	if (n == -1) return NULL;
	if (n == ASE_COUNTOF(mode_mb)) path_mb[ASE_COUNTOF(mode_mb)-1] = '\0';

	return fopen (path_mb, mode_mb);
#endif
}

static FILE* awk_popen (const ase_char_t* cmd, const ase_char_t* mode)
{
#if defined(_WIN32)
	return _tpopen (cmd, mode);
#elif defined(__MSDOS__)
	/* TODO: support this */
	return NULL;
#elif defined(ASE_CHAR_IS_MCHAR)
	return popen (cmd, mode);
#else
	char cmd_mb[PATH_MAX + 1];
	char mode_mb[32];
	size_t n;

	n = wcstombs (cmd_mb, cmd, ASE_COUNTOF(cmd_mb));
	if (n == -1) return NULL;
	if (n == ASE_COUNTOF(cmd_mb)) cmd_mb[ASE_COUNTOF(cmd_mb)-1] = '\0';

	n = wcstombs (mode_mb, mode, ASE_COUNTOF(mode_mb));
	if (n == -1) return NULL;
	if (n == ASE_COUNTOF(mode_mb)) cmd_mb[ASE_COUNTOF(mode_mb)-1] = '\0';

	return popen (cmd_mb, mode_mb);
#endif
}

#if defined(_WIN32)
	#define awk_fgets _fgetts
	#define awk_fgetc _fgettc
	#define awk_fputs _fputts
	#define awk_fputc _fputtc
#elif defined(ASE_CHAR_IS_MCHAR)
	#define awk_fgets fgets
	#define awk_fgetc fgetc
	#define awk_fputs fputs
	#define awk_fputc fputc
#else
	#define awk_fgets fgetws
	#define awk_fgetc fgetwc
	#define awk_fputs fputws
	#define awk_fputc fputwc
#endif

static ase_ssize_t process_source (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	struct src_io* src_io = (struct src_io*)arg;
	ase_char_t c;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		if (src_io->input_file == ASE_NULL) return 0;
		src_io->input_handle = awk_fopen (src_io->input_file, ASE_T("r"));
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
		c = awk_fgetc ((FILE*)src_io->input_handle);
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
	else if (cmd == ASE_AWK_IO_CLOSE) 
	{
		fflush (stdout);
		return 0;
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		ase_size_t i;
		for (i = 0; i < size; i++)
		{
			if (awk_fputc (data[i], stdout) == ASE_CHAR_EOF) return -1;
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

			awk_dprintf (ASE_T("opening %s of type %d (pipe)\n"),  epa->name, epa->type);
			handle = awk_popen (epa->name, mode);
			if (handle == NULL) return -1;
			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			awk_dprintf (ASE_T("closing %s of type (pipe) %d\n"),  epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			if (awk_fgets (data, size, (FILE*)epa->handle) == ASE_NULL) return 0;
			return ase_awk_strlen(data);
		}

		case ASE_AWK_IO_WRITE:
		{
			int n;
/*
			ase_size_t i;
			for (i = 0; i < size; i++)
			{
				if (awk_fputc (data[i], (FILE*)epa->handle) == ASE_CHAR_EOF) return -1;
			}
*/
		#if defined(_WIN32)
			n = _ftprintf ((FILE*)epa->handle, ASE_T("%.*s"), size, data);
		#else
			n = ase_fprintf ((FILE*)epa->handle, ASE_T("%.*s"), size, data);
		#endif
			if (n < 0) return -1;

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

			awk_dprintf (ASE_T("opening %s of type %d (file)\n"), epa->name, epa->type);
			handle = awk_fopen (epa->name, mode);
			if (handle == NULL) return -1;

			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			awk_dprintf (ASE_T("closing %s of type %d (file)\n"), epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			if (awk_fgets (data, size, (FILE*)epa->handle) == ASE_NULL) 
				return 0;
			return ase_awk_strlen(data);
		}

		case ASE_AWK_IO_WRITE:
		{
			/*
			ase_size_t i;
			for (i = 0; i < size; i++)
			{
				if (awk_fputc (data[i], (FILE*)epa->handle) == ASE_CHAR_EOF) return -1;
			}
			*/

			int n;
		#if defined(_WIN32)
			n = _ftprintf (epa->handle, ASE_T("%.*s"), size, data);
		#else
			n = ase_fprintf ((FILE*)epa->handle, ASE_T("%.*s"), size, data);
		#endif
			if (n < 0) return -1;
		

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
		while (awk_fgets (data, size, (FILE*)epa->handle) == ASE_NULL)
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
				    epa->handle != stderr) 
				{
					fclose ((FILE*)epa->handle);
				}

				epa->handle = ASE_NULL;
				*/

				return 0;
			}

			if (infiles[infile_no][0] == ASE_T('\0'))
			{
				if (epa->handle != ASE_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) 
				{
					fclose ((FILE*)epa->handle);
				}
				epa->handle = stdin;
			}
			else
			{
				FILE* fp = awk_fopen (infiles[infile_no], ASE_T("r"));
				if (fp == ASE_NULL)
				{
					awk_dprintf (ASE_T("failed to open the next console of type %x - fopen failure\n"), epa->type);
					return -1;
				}

				if (epa->handle != ASE_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) 
				{
					fclose ((FILE*)epa->handle);
				}

				awk_dprintf (ASE_T("open the next console [%s]\n"), infiles[infile_no]);
				epa->handle = fp;
			}

			infile_no++;	
		}

		return ase_awk_strlen(data);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		ase_size_t i;
		for (i = 0; i < size; i++)
		{
			if (awk_fputc (data[i], (FILE*)epa->handle) == ASE_CHAR_EOF) return -1;
		}

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

	awk_dprintf (ASE_T("opening console[%s] of type %x\n"), epa->name, epa->type);

	if (epa->mode == ASE_AWK_EXTIO_CONSOLE_READ)
	{
		if (infiles[infile_no] == ASE_NULL)
		{
			/* no more input file */
			awk_dprintf (ASE_T("console - no more file\n"));;
			return 0;
		}

		if (infiles[infile_no][0] == ASE_T('\0'))
		{
			awk_dprintf (ASE_T("    console(r) - <standard input>\n"));
			epa->handle = stdin;
		}
		else
		{
			/* a temporary variable fp is used here not to change 
			 * any fields of epa when the open operation fails */
			FILE* fp = awk_fopen (infiles[infile_no], ASE_T("r"));
			if (fp == ASE_NULL)
			{
				awk_dprintf (ASE_T("cannot open console of type %x - fopen failure\n"), epa->type);
				return -1;
			}

			awk_dprintf (ASE_T("    console(r) - %s\n"), infiles[infile_no]);
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
		awk_dprintf (ASE_T("    console(w) - <standard output>\n"));
		/* TODO: does output console has a name??? */
		/*ase_awk_setconsolename (ASE_T(""));*/
		epa->handle = stdout;
		return 1;
	}

	return -1;
}

static int close_extio_console (ase_awk_extio_t* epa)
{
	awk_dprintf (ASE_T("closing console of type %x\n"), epa->type);

	if (epa->handle != ASE_NULL &&
	    epa->handle != stdin && 
	    epa->handle != stdout && 
	    epa->handle != stderr)
	{
		fclose ((FILE*)epa->handle);
	}

	/* TODO: CloseConsole in GUI APPLICATION */
	return 0;
}

static int next_extio_console (ase_awk_extio_t* epa)
{
	int n;
	FILE* fp = (FILE*)epa->handle;

	awk_dprintf (ASE_T("switching console[%s] of type %x\n"), epa->name, epa->type);

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
ase_awk_run_t* app_run = NULL;

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

static void on_run_start (
	ase_awk_t* awk, ase_awk_run_t* run, void* custom_data)
{
	app_awk = awk;	
	app_run = run;

	awk_dprintf (ASE_T("AWK ABOUT TO START...\n"));
}

static void on_run_end (
	ase_awk_t* awk, ase_awk_run_t* run, 
	int errnum, void* custom_data)
{
	if (errnum != ASE_AWK_ENOERR)
	{
		awk_dprintf (ASE_T("AWK ENDED WITH AN ERROR\n"));
		awk_dprintf (ASE_T("CODE [%d] LINE [%u] %s\n"),
			errnum, 
			(unsigned int)ase_awk_getrunerrlin(run),
			ase_awk_getrunerrmsg(run));
	}
	else awk_dprintf (ASE_T("AWK ENDED SUCCESSFULLY\n"));

	app_awk = NULL;	
	app_run = NULL;
}

#ifdef _WIN32
typedef struct sysfns_data_t sysfns_data_t;
struct sysfns_data_t
{
	HANDLE heap;
};
#endif

static void* awk_malloc (ase_size_t n, void* custom_data)
{
#ifdef _WIN32
	return HeapAlloc (((sysfns_data_t*)custom_data)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* awk_realloc (void* ptr, ase_size_t n, void* custom_data)
{
#ifdef _WIN32
	/* HeapReAlloc behaves differently from realloc */
	if (ptr == NULL)
		return HeapAlloc (((sysfns_data_t*)custom_data)->heap, 0, n);
	else
		return HeapReAlloc (((sysfns_data_t*)custom_data)->heap, 0, ptr, n);
#else
	return realloc (ptr, n);
#endif
}

static void awk_free (void* ptr, void* custom_data)
{
#ifdef _WIN32
	HeapFree (((sysfns_data_t*)custom_data)->heap, 0, ptr);
#else
	free (ptr);
#endif
}

#if defined(ASE_CHAR_IS_MCHAR) 
	#if (__TURBOC__<=513) /* turboc 2.01 or earlier */
		static int awk_isupper (int c) { return isupper (c); }
		static int awk_islower (int c) { return islower (c); }
		static int awk_isalpha (int c) { return isalpha (c); }
		static int awk_isdigit (int c) { return isdigit (c); }
		static int awk_isxdigit (int c) { return isxdigit (c); }
		static int awk_isalnum (int c) { return isalnum (c); }
		static int awk_isspace (int c) { return isspace (c); }
		static int awk_isprint (int c) { return isprint (c); }
		static int awk_isgraph (int c) { return isgraph (c); }
		static int awk_iscntrl (int c) { return iscntrl (c); }
		static int awk_ispunct (int c) { return ispunct (c); }
		static int awk_toupper (int c) { return toupper (c); }
		static int awk_tolower (int c) { return tolower (c); }
	#else
		#define awk_isupper  isupper
		#define awk_islower  islower
		#define awk_isalpha  isalpha
		#define awk_isdigit  isdigit
		#define awk_isxdigit isxdigit
		#define awk_isalnum  isalnum
		#define awk_isspace  isspace
		#define awk_isprint  isprint
		#define awk_isgraph  isgraph
		#define awk_iscntrl  iscntrl
		#define awk_ispunct  ispunct
		#define awk_toupper  tolower
		#define awk_tolower  tolower
	#endif
#else
	#define awk_isupper  iswupper
	#define awk_islower  iswlower
	#define awk_isalpha  iswalpha
	#define awk_isdigit  iswdigit
	#define awk_isxdigit iswxdigit
	#define awk_isalnum  iswalnum
	#define awk_isspace  iswspace
	#define awk_isprint  iswprint
	#define awk_isgraph  iswgraph
	#define awk_iscntrl  iswcntrl
	#define awk_ispunct  iswpunct

	#define awk_toupper  towlower
	#define awk_tolower  towlower
#endif

static int __main (int argc, ase_char_t* argv[])
{
	ase_awk_t* awk;
	ase_awk_srcios_t srcios;
	ase_awk_runcbs_t runcbs;
	ase_awk_runios_t runios;
	ase_awk_runarg_t runarg[10];
	ase_awk_sysfns_t sysfns;
	struct src_io src_io = { NULL, NULL };
	int opt, i, file_count = 0, errnum;
#ifdef _WIN32
	sysfns_data_t sysfns_data;
#endif
	const ase_char_t* mfn = ASE_NULL;

	opt = ASE_AWK_IMPLICIT | 
	      ASE_AWK_EXPLICIT | 
	      ASE_AWK_UNIQUEFN | 
	      ASE_AWK_IDIV |
	      ASE_AWK_SHADING | 
	      ASE_AWK_SHIFT | 
	      ASE_AWK_EXTIO | 
	      /*ASE_AWK_COPROC |*/
	      ASE_AWK_BLOCKLESS | 
	      ASE_AWK_STRBASEONE | 
	      ASE_AWK_STRIPSPACES | 
	      ASE_AWK_NEXTOFILE;

	if (argc <= 1)
	{
		awk_printf (ASE_T("Usage: %s [-m] source_file [data_file ...]\n"), argv[0]);
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
			awk_printf (ASE_T("Usage: %s [-m] [-f source_file] [data_file ...]\n"), argv[0]);
			return -1;
		}
	}

	memset (&sysfns, 0, ASE_SIZEOF(sysfns));

	sysfns.malloc  = awk_malloc;
	sysfns.realloc = awk_realloc;
	sysfns.free    = awk_free;
	sysfns.memcpy  = memcpy;
	sysfns.memset  = memset;

	sysfns.is_upper  = (ase_awk_isctype_t)awk_isupper;
	sysfns.is_lower  = (ase_awk_isctype_t)awk_islower;
	sysfns.is_alpha  = (ase_awk_isctype_t)awk_isalpha;
	sysfns.is_digit  = (ase_awk_isctype_t)awk_isdigit;
	sysfns.is_xdigit = (ase_awk_isctype_t)awk_isxdigit;
	sysfns.is_alnum  = (ase_awk_isctype_t)awk_isalnum;
	sysfns.is_space  = (ase_awk_isctype_t)awk_isspace;
	sysfns.is_print  = (ase_awk_isctype_t)awk_isprint;
	sysfns.is_graph  = (ase_awk_isctype_t)awk_isgraph;
	sysfns.is_cntrl  = (ase_awk_isctype_t)awk_iscntrl;
	sysfns.is_punct  = (ase_awk_isctype_t)awk_ispunct;
	sysfns.to_upper  = (ase_awk_toctype_t)awk_toupper;
	sysfns.to_lower  = (ase_awk_toctype_t)awk_tolower;

	sysfns.pow     = awk_pow;
	sysfns.sprintf = awk_sprintf;
	sysfns.aprintf = awk_aprintf;
	sysfns.dprintf = awk_dprintf;
	sysfns.abort   = awk_abort;
	sysfns.lock    = NULL;
	sysfns.unlock  = NULL;

#ifdef _WIN32
	sysfns_data.heap = HeapCreate (0, 1000000, 1000000);
	if (sysfns_data.heap == NULL)
	{
		awk_printf (ASE_T("Error: cannot create an awk heap\n"));
		return -1;
	}

	sysfns.custom_data = &sysfns_data;
#endif

	if ((awk = ase_awk_open(&sysfns, ASE_NULL, &errnum)) == ASE_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (sysfns_data.heap);
#endif
		awk_printf (
			ASE_T("ERROR: cannot open awk [%d] %s\n"), 
			errnum, ase_awk_geterrstr(errnum));
		return -1;
	}

	ase_awk_setoption (awk, opt);

	srcios.in = process_source;
	srcios.out = dump_source;
	srcios.custom_data = &src_io;


	ase_awk_setmaxdepth (
		awk, ASE_AWK_DEPTH_BLOCK_PARSE | ASE_AWK_DEPTH_EXPR_PARSE, 20);
	ase_awk_setmaxdepth (
		awk, ASE_AWK_DEPTH_BLOCK_RUN | ASE_AWK_DEPTH_EXPR_RUN, 50);

	if (ase_awk_parse (awk, &srcios) == -1) 
	{
		int errnum = ase_awk_geterrnum(awk);
		awk_printf (
			ASE_T("ERROR: cannot parse program - line %u [%d] %s\n"), 
			(unsigned int)ase_awk_geterrlin(awk), 
			errnum, ase_awk_geterrmsg(awk));
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

	runcbs.on_start = on_run_start;
	runcbs.on_end = on_run_end;
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
		awk_printf (
			ASE_T("error: cannot run program - [%d] %s\n"), 
			errnum, ase_awk_geterrstr(errnum));
		ase_awk_close (awk);
		return -1;
	}

	ase_awk_close (awk);
#ifdef _WIN32
	HeapDestroy (sysfns_data.heap);
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
#elif defined(__MSDOS__) || defined(ASE_CHAR_IS_MCHAR)
int main (int argc, ase_char_t* argv[])
#else
int ase_main (int argc, ase_char_t* argv[])
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

