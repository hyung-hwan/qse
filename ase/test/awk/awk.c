/*
 * $Id: awk.c,v 1.1 2007/03/28 14:05:29 bacon Exp $
 */

#include <ase/awk/awk.h>
#include <ase/awk/val.h>
#include <ase/awk/map.h>

#include <ase/utl/ctype.h>
#include <ase/utl/stdio.h>
#include <ase/utl/main.h>

#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>

#if defined(_WIN32)
	#include <windows.h>
	#include <tchar.h>
	#include <process.h>
	#pragma warning (disable: 4996)
	#pragma warning (disable: 4296)
#endif

#if defined(__linux) && defined(_DEBUG)
#include <mcheck.h>
#endif

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

struct awk_src_io
{
	const ase_char_t* input_file;
	FILE* input_handle;
};

#if defined(_WIN32)
struct mmgr_data_t
{
	HANDLE heap;
};
#endif

#if defined(vms) || defined(__vms)
/* it seems that the main function should be placed in the main object file
 * in OpenVMS. otherwise, the first function in the main object file seems
 * to become the main function resulting in program start-up failure. */
#include <ase/utl/main.c>
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

static void dprintf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

static void custom_awk_dprintf (void* custom, const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}


/* custom memory management function */
static void* custom_awk_malloc (void* custom, ase_size_t n)
{
#ifdef _WIN32
	return HeapAlloc (((struct mmgr_data_t*)custom)->heap, 0, n);
#else
	return malloc (n);
#endif
}

static void* custom_awk_realloc (void* custom, void* ptr, ase_size_t n)
{
#ifdef _WIN32
	/* HeapReAlloc behaves differently from realloc */
	if (ptr == NULL)
		return HeapAlloc (((struct mmgr_data_t*)custom)->heap, 0, n);
	else
		return HeapReAlloc (((struct mmgr_data_t*)custom)->heap, 0, ptr, n);
#else
	return realloc (ptr, n);
#endif
}

static void custom_awk_free (void* custom, void* ptr)
{
#ifdef _WIN32
	HeapFree (((struct mmgr_data_t*)custom)->heap, 0, ptr);
#else
	free (ptr);
#endif
}

/* custom character class functions */
static ase_bool_t custom_awk_isupper (void* custom, ase_cint_t c)  
{ 
	return ase_isupper (c); 
}

static ase_bool_t custom_awk_islower (void* custom, ase_cint_t c)  
{ 
	return ase_islower (c); 
}

static ase_bool_t custom_awk_isalpha (void* custom, ase_cint_t c)  
{ 
	return ase_isalpha (c); 
}

static ase_bool_t custom_awk_isdigit (void* custom, ase_cint_t c)  
{ 
	return ase_isdigit (c); 
}

static ase_bool_t custom_awk_isxdigit (void* custom, ase_cint_t c) 
{ 
	return ase_isxdigit (c); 
}

static ase_bool_t custom_awk_isalnum (void* custom, ase_cint_t c)
{ 
	return ase_isalnum (c); 
}

static ase_bool_t custom_awk_isspace (void* custom, ase_cint_t c)
{ 
	return ase_isspace (c); 
}

static ase_bool_t custom_awk_isprint (void* custom, ase_cint_t c)
{ 
	return ase_isprint (c); 
}

static ase_bool_t custom_awk_isgraph (void* custom, ase_cint_t c)
{
	return ase_isgraph (c); 
}

static ase_bool_t custom_awk_iscntrl (void* custom, ase_cint_t c)
{
	return ase_iscntrl (c);
}

static ase_bool_t custom_awk_ispunct (void* custom, ase_cint_t c)
{
	return ase_ispunct (c);
}

static ase_cint_t custom_awk_toupper (void* custom, ase_cint_t c)
{
	return ase_toupper (c);
}

static ase_cint_t custom_awk_tolower (void* custom, ase_cint_t c)
{
	return ase_tolower (c);
}


/* custom miscellaneous functions */

static ase_real_t custom_awk_pow (void* custom, ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}


static int custom_awk_sprintf (
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


static ase_ssize_t awk_srcio_in (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	struct awk_src_io* src_io = (struct awk_src_io*)arg;
	ase_cint_t c;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		if (src_io->input_file == ASE_NULL) return 0;
		src_io->input_handle = ase_fopen (src_io->input_file, ASE_T("r"));
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
		c = ase_fgetc ((FILE*)src_io->input_handle);
		if (c == ASE_CHAR_EOF) return 0;
		*data = (ase_char_t)c;
		return 1;
	}

	return -1;
}

static ase_ssize_t awk_srcio_out (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	/*struct awk_src_io* src_io = (struct awk_src_io*)arg;*/

	if (cmd == ASE_AWK_IO_OPEN) return 1;
	else if (cmd == ASE_AWK_IO_CLOSE) 
	{
		fflush (stdout);
		return 0;
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		int n = ase_fprintf (stdout, ASE_T("%.*s"), size, data);
		if (n < 0) return -1;

		return size;
	}

	return -1;
}

static ase_ssize_t awk_extio_pipe (
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

			dprintf (ASE_T("opening %s of type %d (pipe)\n"),  epa->name, epa->type);
			handle = ase_popen (epa->name, mode);
			if (handle == NULL) return -1;
			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			dprintf (ASE_T("closing %s of type (pipe) %d\n"),  epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			if (ase_fgets (data, size, (FILE*)epa->handle) == ASE_NULL) 
			{
				if (ferror((FILE*)epa->handle)) return -1;
				return 0;
			}
			return ase_strlen(data);
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

static ase_ssize_t awk_extio_file (
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

			dprintf (ASE_T("opening %s of type %d (file)\n"), epa->name, epa->type);
			handle = ase_fopen (epa->name, mode);
			if (handle == NULL) 
			{
				ase_cstr_t errarg;

				errarg.ptr = epa->name;
				errarg.len = ase_strlen(epa->name);


				ase_awk_setrunerror (epa->run, ASE_AWK_EOPEN, 0, &errarg, 1);
				return -1;
			}

			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			dprintf (ASE_T("closing %s of type %d (file)\n"), epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			if (ase_fgets (data, size, (FILE*)epa->handle) == ASE_NULL) 
			{
				if (ferror((FILE*)epa->handle)) return -1;
				return 0;
			}
			return ase_strlen(data);
		}

		case ASE_AWK_IO_WRITE:
		{
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
static const ase_char_t* infiles[1000] =
{
	ASE_T(""),
	ASE_NULL
};

static ase_ssize_t awk_extio_console (
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
		while (ase_fgets (data, size, (FILE*)epa->handle) == ASE_NULL)
		{
			if (ferror((FILE*)epa->handle)) return -1;

			/* it has reached the end of the current file.
			 * open the next file if available */
			if (infiles[infile_no] == ASE_NULL) 
			{
				/* no more input console */
				return 0;
			}

			if (infiles[infile_no][0] == ASE_T('\0'))
			{
				if (epa->handle != ASE_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) 
				{
					/* TODO: ................................ */
					if (fclose ((FILE*)epa->handle) == EOF)
					{
						ase_cstr_t errarg;

						errarg.ptr = ASE_T("consolXXXe");
						errarg.len = 7;

						ase_awk_setrunerror (epa->run, ASE_AWK_ECLOSE, 0, &errarg, 1);
						return -1;
					}
				}

				epa->handle = stdin;
			}
			else
			{
				FILE* fp = ase_fopen (infiles[infile_no], ASE_T("r"));
				if (fp == ASE_NULL)
				{
					ase_cstr_t errarg;

					errarg.ptr = infiles[infile_no];
					errarg.len = ase_strlen(infiles[infile_no]);

					ase_awk_setrunerror (epa->run, ASE_AWK_EOPEN, 0, &errarg, 1);
					return -1;
				}

				if (ase_awk_setfilename (
					epa->run, infiles[infile_no], 
					ase_strlen(infiles[infile_no])) == -1)
				{
					fclose (fp);
					return -1;
				}

				if (epa->handle != ASE_NULL &&
				    epa->handle != stdin &&
				    epa->handle != stdout &&
				    epa->handle != stderr) 
				{
					/* TODO: ................................ */
					if (fclose ((FILE*)epa->handle) == EOF)
					{
						ase_cstr_t errarg;

						errarg.ptr = ASE_T("console");
						errarg.len = 7;

						ase_awk_setrunerror (epa->run, ASE_AWK_ECLOSE, 0, &errarg, 1);

						fclose (fp);
						return -1;
					}
				}

				dprintf (ASE_T("open the next console [%s]\n"), infiles[infile_no]);
				epa->handle = fp;
			}

			infile_no++;	
		}

		return ase_strlen(data);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		int n = ase_fprintf (
			(FILE*)epa->handle, ASE_T("%.*s"), size, data);
		if (n < 0) return -1;

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

	dprintf (ASE_T("opening console[%s] of type %x\n"), epa->name, epa->type);

	if (epa->mode == ASE_AWK_EXTIO_CONSOLE_READ)
	{
		if (infiles[infile_no] == ASE_NULL)
		{
			/* no more input file */
			dprintf (ASE_T("console - no more file\n"));;
			return 0;
		}

		if (infiles[infile_no][0] == ASE_T('\0'))
		{
			dprintf (ASE_T("    console(r) - <standard input>\n"));
			epa->handle = stdin;
		}
		else
		{
			/* a temporary variable fp is used here not to change 
			 * any fields of epa when the open operation fails */
			FILE* fp = ase_fopen (infiles[infile_no], ASE_T("r"));
			if (fp == ASE_NULL)
			{
				ase_cstr_t errarg;

				errarg.ptr = infiles[infile_no];
				errarg.len = ase_strlen(infiles[infile_no]);

				ase_awk_setrunerror (epa->run, ASE_AWK_EOPEN, 0, &errarg, 1);
				return -1;
			}

			dprintf (ASE_T("    console(r) - %s\n"), infiles[infile_no]);
			if (ase_awk_setfilename (
				epa->run, infiles[infile_no], 
				ase_strlen(infiles[infile_no])) == -1)
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
		dprintf (ASE_T("    console(w) - <standard output>\n"));

		if (ase_awk_setofilename (epa->run, ASE_T(""), 0) == -1)
		{
			return -1;
		}

		epa->handle = stdout;
		return 1;
	}

	return -1;
}

static int close_extio_console (ase_awk_extio_t* epa)
{
	dprintf (ASE_T("closing console of type %x\n"), epa->type);

	if (epa->handle != ASE_NULL &&
	    epa->handle != stdin && 
	    epa->handle != stdout && 
	    epa->handle != stderr)
	{
		if (fclose ((FILE*)epa->handle) == EOF)
		{
			ase_cstr_t errarg;

			errarg.ptr = epa->name;
			errarg.len = ase_strlen(epa->name);

			ase_awk_setrunerror (epa->run, ASE_AWK_ECLOSE, 0, &errarg, 1);
			return -1;
		}
	}

	return 0;
}

static int next_extio_console (ase_awk_extio_t* epa)
{
	int n;
	FILE* fp = (FILE*)epa->handle;

	dprintf (ASE_T("switching console[%s] of type %x\n"), epa->name, epa->type);

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
		ase_awk_stop (app_run);
		return TRUE;
	}

	return FALSE;
}
#else
static void stop_run (int sig)
{
	signal  (SIGINT, SIG_IGN);
	ase_awk_stop (app_run);
	signal  (SIGINT, stop_run);
}
#endif

static void on_run_start (ase_awk_run_t* run, void* custom)
{
	app_run = run;
	dprintf (ASE_T("[AWK ABOUT TO START]\n"));
}

static int print_awk_value (ase_awk_pair_t* pair, void* arg)
{
	ase_awk_run_t* run = (ase_awk_run_t*)arg;
	dprintf (ASE_T("%s = "), (const ase_char_t*)pair->key);
	ase_awk_dprintval (run, (ase_awk_val_t*)pair->val);
	dprintf (ASE_T("\n"));
	return 0;
}

static void on_run_statement (
	ase_awk_run_t* run, ase_size_t line, void* custom)
{
	/*dprintf (L"running %d\n", (int)line);*/
}

static void on_run_return (
	ase_awk_run_t* run, ase_awk_val_t* ret, void* custom)
{
	dprintf (ASE_T("[RETURN] - "));
	ase_awk_dprintval (run, ret);
	dprintf (ASE_T("\n"));

	dprintf (ASE_T("[NAMED VARIABLES]\n"));
	ase_awk_map_walk (ase_awk_getrunnamedvarmap(run), print_awk_value, run);
	dprintf (ASE_T("[END NAMED VARIABLES]\n"));
}

static void on_run_end (ase_awk_run_t* run, int errnum, void* custom_data)
{
	if (errnum != ASE_AWK_ENOERR)
	{
		dprintf (ASE_T("[AWK ENDED WITH AN ERROR]\n"));
		ase_printf (ASE_T("RUN ERROR: CODE [%d] LINE [%u] %s\n"),
			errnum, 
			(unsigned int)ase_awk_getrunerrlin(run),
			ase_awk_getrunerrmsg(run));
	}
	else dprintf (ASE_T("[AWK ENDED SUCCESSFULLY]\n"));

	app_run = NULL;
}

static void print_usage (const ase_char_t* argv0)
{
	ase_printf (ASE_T("Usage: %s [-m] [-d] [-a argument]* -f source-file [data-file]*\n"), argv0);
}

static int run_awk (ase_awk_t* awk, 
	const ase_char_t* mfn, ase_awk_runarg_t* runarg)
{
	ase_awk_runcbs_t runcbs;
	ase_awk_runios_t runios;

	runios.pipe = awk_extio_pipe;
	runios.file = awk_extio_file;
	runios.console = awk_extio_console;
	runios.custom_data = ASE_NULL;

	runcbs.on_start = on_run_start;
	runcbs.on_statement = on_run_statement;
	runcbs.on_return = on_run_return;
	runcbs.on_end = on_run_end;
	runcbs.custom_data = ASE_NULL;

	if (ase_awk_run (awk, mfn, &runios, &runcbs, runarg, ASE_NULL) == -1)
	{
		ase_printf (
			ASE_T("RUN ERROR: CODE [%d] LINE [%u] %s\n"), 
			ase_awk_geterrnum(awk),
			(unsigned int)ase_awk_geterrlin(awk), 
			ase_awk_geterrmsg(awk));

		return -1;
	}

	return 0;
}

#if defined(_WIN32) && defined(TEST_THREAD)
typedef struct thr_arg_t thr_arg_t;
struct thr_arg_t
{
	ase_awk_t* awk;
	const ase_char_t* mfn;
	ase_awk_runarg_t* runarg;
};

static unsigned int __stdcall run_awk_thr (void* arg)
{
	int n;
	thr_arg_t* x = (thr_arg_t*)arg;

	n = run_awk (x->awk, x->mfn, x->runarg);

	_endthreadex (n);
	return  0;
}
#endif

static int awk_main (int argc, ase_char_t* argv[])
{
	ase_awk_t* awk;
	ase_awk_srcios_t srcios;
	ase_awk_prmfns_t prmfns;
	struct awk_src_io src_io = { NULL, NULL };
	int opt, i, file_count = 0;
#ifdef _WIN32
	struct mmgr_data_t mmgr_data;
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
			if (ase_strcmp(argv[i], ASE_T("-m")) == 0)
			{
				mfn = ASE_T("main");
			}
			else if (ase_strcmp(argv[i], ASE_T("-d")) == 0)
			{
				deparse = 1;
			}
			else if (ase_strcmp(argv[i], ASE_T("-f")) == 0)
			{
				/* specify source file */
				mode = 1;
			}
			else if (ase_strcmp(argv[i], ASE_T("-a")) == 0)
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
			runarg[runarg_count].len = ase_strlen(argv[i]);
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

	prmfns.mmgr.malloc  = custom_awk_malloc;
	prmfns.mmgr.realloc = custom_awk_realloc;
	prmfns.mmgr.free    = custom_awk_free;
#ifdef _WIN32
	mmgr_data.heap = HeapCreate (0, 1000000, 1000000);
	if (mmgr_data.heap == NULL)
	{
		ase_printf (ASE_T("Error: cannot create an awk heap\n"));
		return -1;
	}

	prmfns.mmgr.custom_data = &mmgr_data;
#else
	prmfns.mmgr.custom_data = NULL;
#endif

	prmfns.ccls.is_upper    = custom_awk_isupper;
	prmfns.ccls.is_lower    = custom_awk_islower;
	prmfns.ccls.is_alpha    = custom_awk_isalpha;
	prmfns.ccls.is_digit    = custom_awk_isdigit;
	prmfns.ccls.is_xdigit   = custom_awk_isxdigit;
	prmfns.ccls.is_alnum    = custom_awk_isalnum;
	prmfns.ccls.is_space    = custom_awk_isspace;
	prmfns.ccls.is_print    = custom_awk_isprint;
	prmfns.ccls.is_graph    = custom_awk_isgraph;
	prmfns.ccls.is_cntrl    = custom_awk_iscntrl;
	prmfns.ccls.is_punct    = custom_awk_ispunct;
	prmfns.ccls.to_upper    = custom_awk_toupper;
	prmfns.ccls.to_lower    = custom_awk_tolower;
	prmfns.ccls.custom_data = NULL;

	prmfns.misc.pow         = custom_awk_pow;
	prmfns.misc.sprintf     = custom_awk_sprintf;
	prmfns.misc.dprintf     = custom_awk_dprintf;
	prmfns.misc.custom_data = NULL;

	if ((awk = ase_awk_open(&prmfns, ASE_NULL)) == ASE_NULL) 
	{
#ifdef _WIN32
		HeapDestroy (mmgr_data.heap);
#endif
		ase_printf (ASE_T("ERROR: cannot open awk\n"));
		return -1;
	}

	app_awk = awk;

	ase_awk_setoption (awk, opt);

	/*
	ase_awk_seterrstr (awk, ASE_AWK_EGBLRED, ASE_T("\uC804\uC5ED\uBCC0\uC218 \'%.*s\'\uAC00 \uC7AC\uC815\uC758 \uB418\uC5C8\uC2B5\uB2C8\uB2E4"));
	ase_awk_seterrstr (awk, ASE_AWK_EAFNRED, ASE_T("\uD568\uC218 \'%.*s\'\uAC00 \uC7AC\uC815\uC758 \uB418\uC5C8\uC2B5\uB2C8\uB2E4"));
	*/

	srcios.in = awk_srcio_in;
	srcios.out = deparse? awk_srcio_out: NULL;
	srcios.custom_data = &src_io;

	ase_awk_setmaxdepth (
		awk, ASE_AWK_DEPTH_BLOCK_PARSE | ASE_AWK_DEPTH_EXPR_PARSE, 20);
	ase_awk_setmaxdepth (
		awk, ASE_AWK_DEPTH_BLOCK_RUN | ASE_AWK_DEPTH_EXPR_RUN, 50);

	if (ase_awk_parse (awk, &srcios) == -1) 
	{
		ase_printf (
			ASE_T("PARSE ERROR: CODE [%d] LINE [%u] %s\n"), 
			ase_awk_geterrnum(awk),
			(unsigned int)ase_awk_geterrlin(awk), 
			ase_awk_geterrmsg(awk));
		ase_awk_close (awk);
		return -1;
	}

#ifdef _WIN32
	SetConsoleCtrlHandler (stop_run, TRUE);
#else
	signal (SIGINT, stop_run);
#endif

#if defined(_WIN32) && defined(TEST_THREAD)
	{
		unsigned int tid;
		HANDLE thr[5];
		thr_arg_t arg;
		int y;

		arg.awk = awk;
		arg.mfn = mfn;
		arg.runarg = runarg;

		for (y = 0; y < ASE_COUNTOF(thr); y++)
		{
			thr[y] = (HANDLE)_beginthreadex (NULL, 0, run_awk_thr, &arg, 0, &tid);
			if (thr[y] == (HANDLE)0) 
			{
				ase_printf (ASE_T("ERROR: cannot create a thread %d\n"), y);
			}
		}

		for (y = 0; y < ASE_COUNTOF(thr); y++)
		{
			if (thr[y]) WaitForSingleObject (thr[y], INFINITE);
		}
	}
#else
	if (run_awk (awk, mfn, runarg) == -1)
	{
		ase_awk_close (awk);
		return -1;
	}
#endif

	ase_awk_close (awk);
#ifdef _WIN32
	HeapDestroy (mmgr_data.heap);
#endif
	return 0;
}

int ase_main (int argc, ase_char_t* argv[])
{
	int n;

#if defined(__linux) && defined(_DEBUG)
	mtrace ();
#endif
/*
#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif
*/

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
