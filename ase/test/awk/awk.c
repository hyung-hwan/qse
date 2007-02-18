/*
 * $Id: awk.c,v 1.166 2007-02-18 11:21:19 bacon Exp $
 */

#include <ase/awk/awk.h>
#include <ase/awk/val.h>
#include <ase/awk/map.h>
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
#endif

#include "../../etc/printf.c"
#include "../../etc/main.c"

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
	n = ase_vsprintf (buf, len, fmt, ap);
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
#else
	ase_vprintf (fmt, ap);
#endif
	va_end (ap);
}

static void awk_dprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

static void awk_printf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vprintf (fmt, ap);
	va_end (ap);
}

static FILE* awk_fopen (const ase_char_t* path, const ase_char_t* mode)
{
#if defined(_WIN32)
	return _tfopen (path, mode);
#elif defined(ASE_CHAR_IS_MCHAR)
	return fopen (path, mode);
#else

	char path_mb[PATH_MAX + 1];
	char mode_mb[32];
	size_t n;

	n = wcstombs (path_mb, path, ASE_COUNTOF(path_mb));
	if (n == (size_t)-1) return NULL;
	if (n == ASE_COUNTOF(path_mb)) path_mb[ASE_COUNTOF(path_mb)-1] = '\0';

	n = wcstombs (mode_mb, mode, ASE_COUNTOF(mode_mb));
	if (n == (size_t)-1) return NULL;
	if (n == ASE_COUNTOF(mode_mb)) path_mb[ASE_COUNTOF(mode_mb)-1] = '\0';

	return fopen (path_mb, mode_mb);
#endif
}

static FILE* awk_popen (const ase_char_t* cmd, const ase_char_t* mode)
{
#if defined(_WIN32)
	return _tpopen (cmd, mode);
#elif defined(ASE_CHAR_IS_MCHAR)
	return popen (cmd, mode);
#else
	char cmd_mb[PATH_MAX + 1];
	char mode_mb[32];
	size_t n;

	n = wcstombs (cmd_mb, cmd, ASE_COUNTOF(cmd_mb));
	if (n == (size_t)-1) return NULL;
	if (n == ASE_COUNTOF(cmd_mb)) cmd_mb[ASE_COUNTOF(cmd_mb)-1] = '\0';

	n = wcstombs (mode_mb, mode, ASE_COUNTOF(mode_mb));
	if (n == (size_t)-1) return NULL;
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
	ase_cint_t c;

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
		*data = (ase_char_t)c;
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
			if (awk_fgets (data, size, (FILE*)epa->handle) == ASE_NULL) 
			{
				if (ferror((FILE*)epa->handle)) return -1;
				return 0;
			}
			return ase_awk_strlen(data);
		}

		case ASE_AWK_IO_WRITE:
		{
		#if defined(ASE_CHAR_IS_WCHAR) && defined(__linux)
			/* fwprintf seems to return an error with the file
			 * pointer opened by popen, as of this writing. 
			 * anyway, hopefully the following replacement 
			 * will work all the way. */
			int n = fprintf (
				(FILE*)epa->handle, "%.*ls", size, data);
		#else
			int n = ase_fprintf (
				(FILE*)epa->handle, ASE_T("%.*s"), size, data);
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
			{
				if (ferror((FILE*)epa->handle)) return -1;
				return 0;
			}
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

			int n = ase_fprintf (
				(FILE*)epa->handle, ASE_T("%.*s"), size, data);
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
			if (ferror((FILE*)epa->handle)) return -1;

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
static BOOL WINAPI stop_run (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		ase_awk_stop (ase_awk_getrunawk(app_run), app_run);
		return TRUE;
	}

	return FALSE;
}
#else
static void stop_run (int sig)
{
	signal  (SIGINT, SIG_IGN);
	ase_awk_stop (ase_awk_getrunawk(app_run), app_run);
	/*ase_awk_stopall (app_awk); */
	/*ase_awk_stopall (ase_awk_getrunawk(app_run)); */
	signal  (SIGINT, stop_run);
}
#endif

static void on_run_start (ase_awk_run_t* run, void* custom_data)
{
	app_run = run;
	awk_dprintf (ASE_T("[AWK ABOUT TO START]\n"));
}

static int __printval (ase_awk_pair_t* pair, void* arg)
{
	ase_awk_run_t* run = (ase_awk_run_t*)arg;
	awk_dprintf (ASE_T("%s = "), (const ase_char_t*)pair->key);
	ase_awk_dprintval (run, (ase_awk_val_t*)pair->val);
	awk_dprintf (ASE_T("\n"));
	return 0;
}

static void on_run_return (
	ase_awk_run_t* run, ase_awk_val_t* ret, void* custom_data)
{
	app_run = run;

	awk_dprintf (ASE_T("[RETURN] - "));
	ase_awk_dprintval (run, ret);
	awk_dprintf (ASE_T("\n"));

	awk_dprintf (ASE_T("[NAMED VARIABLES]\n"));
	ase_awk_map_walk (ase_awk_getrunnamedvarmap(run), __printval, run);
	awk_dprintf (ASE_T("[END NAMED VARIABLES]\n"));
}

static void on_run_end (ase_awk_run_t* run, int errnum, void* custom_data)
{
	if (errnum != ASE_AWK_ENOERR)
	{
		awk_dprintf (ASE_T("[AWK ENDED WITH AN ERROR] - "));
		awk_dprintf (ASE_T("CODE [%d] LINE [%u] %s\n"),
			errnum, 
			(unsigned int)ase_awk_getrunerrlin(run),
			ase_awk_getrunerrmsg(run));
	}
	else awk_dprintf (ASE_T("[AWK ENDED SUCCESSFULLY]\n"));

	app_run = NULL;
}

#ifdef _WIN32
typedef struct prmfns_data_t prmfns_data_t;
struct prmfns_data_t
{
	HANDLE heap;
};
#endif

static void* awk_malloc (ase_size_t n, void* custom_data)
{
#ifdef _WIN32
	return HeapAlloc (((prmfns_data_t*)custom_data)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* awk_realloc (void* ptr, ase_size_t n, void* custom_data)
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

static void awk_free (void* ptr, void* custom_data)
{
#ifdef _WIN32
	HeapFree (((prmfns_data_t*)custom_data)->heap, 0, ptr);
#else
	free (ptr);
#endif
}

static void* awk_memcpy  (void* dst, const void* src, ase_size_t n)
{
	return memcpy (dst, src, n);
}

static void* awk_memset (void* dst, int val, ase_size_t n)
{
	return memset (dst, val, n);
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

static void print_usage (const ase_char_t* argv0)
{
	awk_printf (ASE_T("Usage: %s [-m] [-d] [-a argument]* -f source-file [data-file]*\n"), argv0);
}

static int awk_main (int argc, ase_char_t* argv[])
{
	ase_awk_t* awk;
	ase_awk_srcios_t srcios;
	ase_awk_runcbs_t runcbs;
	ase_awk_runios_t runios;
	ase_awk_prmfns_t prmfns;
	struct src_io src_io = { NULL, NULL };
	int opt, i, file_count = 0, errnum;
#ifdef _WIN32
	prmfns_data_t prmfns_data;
#endif
	const ase_char_t* mfn = ASE_NULL;
	int mode = 0;
	int runarg_count = 0;
	ase_awk_runarg_t runarg[128];
	int deparse = 0;

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
		print_usage (argv[0]);
		return -1;
	}

	for (i = 1; i < argc; i++)
	{
		if (mode == 0)
		{
			if (ase_awk_strcmp(argv[i], ASE_T("-m")) == 0)
			{
				mfn = ASE_T("main");
			}
			else if (ase_awk_strcmp(argv[i], ASE_T("-d")) == 0)
			{
				deparse = 1;
			}
			else if (ase_awk_strcmp(argv[i], ASE_T("-f")) == 0)
			{
				/* specify source file */
				mode = 1;
			}
			else if (ase_awk_strcmp(argv[i], ASE_T("-a")) == 0)
			{
				/* specify arguments */
				mode = 2;
			}
			else if (argv[i][0] == ASE_T('-'))
			{
				print_usage (argv[0]);
				return -1;
			}
			else if (file_count < ASE_COUNTOF(infiles)-1)
			{
				infiles[file_count] = argv[i];
				file_count++;
			}
			else
			{
				print_usage (argv[0]);
				return -1;
			}
		}
		else if (mode == 1) /* source mode */
		{
			if (argv[i][0] == ASE_T('-'))
			{
				print_usage (argv[0]);
				return -1;
			}

			if (src_io.input_file != NULL) 
			{
				print_usage (argv[0]);
				return -1;
			}

			src_io.input_file = argv[i];
			mode = 0;
		}
		else if (mode == 2) /* argument mode */
		{
			if (argv[i][0] == ASE_T('-'))
			{
				print_usage (argv[0]);
				return -1;
			}

			if (runarg_count >= ASE_COUNTOF(runarg)-1)
			{
				print_usage (argv[0]);
				return -1;
			}

			runarg[runarg_count].ptr = argv[i];
			runarg[runarg_count].len = ase_awk_strlen(argv[i]);
			runarg_count++;
			mode = 0;
		}
	}

	infiles[file_count] = ASE_NULL;
	runarg[runarg_count].ptr = NULL;
	runarg[runarg_count].len = 0;

	if (mode != 0 || src_io.input_file == NULL)
	{
		print_usage (argv[0]);
		return -1;
	}

	memset (&prmfns, 0, ASE_SIZEOF(prmfns));

	prmfns.malloc  = awk_malloc;
	prmfns.realloc = awk_realloc;
	prmfns.free    = awk_free;
	prmfns.memcpy  = awk_memcpy;
	prmfns.memset  = awk_memset;

	prmfns.is_upper  = (ase_awk_isctype_t)awk_isupper;
	prmfns.is_lower  = (ase_awk_isctype_t)awk_islower;
	prmfns.is_alpha  = (ase_awk_isctype_t)awk_isalpha;
	prmfns.is_digit  = (ase_awk_isctype_t)awk_isdigit;
	prmfns.is_xdigit = (ase_awk_isctype_t)awk_isxdigit;
	prmfns.is_alnum  = (ase_awk_isctype_t)awk_isalnum;
	prmfns.is_space  = (ase_awk_isctype_t)awk_isspace;
	prmfns.is_print  = (ase_awk_isctype_t)awk_isprint;
	prmfns.is_graph  = (ase_awk_isctype_t)awk_isgraph;
	prmfns.is_cntrl  = (ase_awk_isctype_t)awk_iscntrl;
	prmfns.is_punct  = (ase_awk_isctype_t)awk_ispunct;
	prmfns.to_upper  = (ase_awk_toctype_t)awk_toupper;
	prmfns.to_lower  = (ase_awk_toctype_t)awk_tolower;

	prmfns.pow     = awk_pow;
	prmfns.sprintf = awk_sprintf;
	prmfns.aprintf = awk_aprintf;
	prmfns.dprintf = awk_dprintf;
	prmfns.abort   = awk_abort;
	prmfns.lock    = NULL;
	prmfns.unlock  = NULL;

#ifdef _WIN32
	prmfns_data.heap = HeapCreate (0, 1000000, 1000000);
	if (prmfns_data.heap == NULL)
	{
		awk_printf (ASE_T("Error: cannot create an awk heap\n"));
		return -1;
	}

	prmfns.custom_data = &prmfns_data;
#endif

	if ((awk = ase_awk_open(&prmfns, ASE_NULL, &errnum)) == ASE_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (prmfns_data.heap);
#endif
		awk_printf (
			ASE_T("ERROR: cannot open awk [%d] %s\n"), 
			errnum, ase_awk_geterrstr(errnum));
		return -1;
	}

	app_awk = awk;

	ase_awk_setoption (awk, opt);

	srcios.in = process_source;
	srcios.out = deparse? dump_source: NULL;
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
	SetConsoleCtrlHandler (stop_run, TRUE);
#else
	signal (SIGINT, stop_run);
#endif

	runios.pipe = process_extio_pipe;
	runios.coproc = ASE_NULL;
	runios.file = process_extio_file;
	runios.console = process_extio_console;

	runcbs.on_start = on_run_start;
	runcbs.on_return = on_run_return;
	runcbs.on_end = on_run_end;
	runcbs.custom_data = ASE_NULL;

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
/*#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif*/

	n = awk_main (argc, argv);

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

