/*
 * $Id: awk.c 333 2008-08-19 03:16:02Z baconevi $
 */

#include <ase/awk/awk.h>
#include <ase/cmn/sll.h>
#include <ase/cmn/mem.h>

#include <ase/utl/helper.h>
#include <ase/utl/getopt.h>
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

	#if defined(_MSC_VER) && defined(_DEBUG)
		#define _CRTDBG_MAP_ALLOC
		#include <crtdbg.h>
	#endif
#endif

#define SRCIO_FILE 1
#define SRCIO_STR  2

typedef struct srcio_data_t
{
	int type; /* file or string */

	union
	{
		struct 
		{
			ase_char_t* ptr;
			ase_char_t* cur;
		} str;
		struct 
		{
			ase_sll_t* sll;
			ase_sll_node_t* cur;
			FILE* handle;
		} file;
	} data;
} srcio_data_t;

static void local_dprintf (const ase_char_t* fmt, ...)
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
	return HeapAlloc ((HANDLE)custom, 0, n);
#else
	return malloc (n);
#endif
}

static void* custom_awk_realloc (void* custom, void* ptr, ase_size_t n)
{
#ifdef _WIN32
	/* HeapReAlloc behaves differently from realloc */
	return (ptr == NULL)?
		HeapAlloc ((HANDLE)custom, 0, n):
		HeapReAlloc ((HANDLE)custom, 0, ptr, n);
#else
	return realloc (ptr, n);
#endif
}

static void custom_awk_free (void* custom, void* ptr)
{
#ifdef _WIN32
	HeapFree ((HANDLE)custom, 0, ptr);
#else
	free (ptr);
#endif
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

static ase_ssize_t awk_srcio_in_str (
	int cmd, srcio_data_t* siod, ase_char_t* data, ase_size_t size)
{
	return -1;
}

static ase_ssize_t awk_srcio_in_file (
	int cmd, srcio_data_t* siod, ase_char_t* data, ase_size_t size)
{
	ase_cint_t c;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		if (siod->data.file.cur == ASE_NULL) return 0;
		siod->data.file.handle = ase_fopen (ASE_SLL_DPTR(siod->data.file.cur), ASE_T("r"));
		if (siod->data.file.handle == NULL) return -1;

		/*
		ase_awk_setsinname ();
		*/

		return 1;
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		if (siod->data.file.cur == ASE_NULL) return 0;
		fclose (siod->data.file.handle);
		/*
		if (srcio->input_file == ASE_NULL) return 0;
		fclose ((FILE*)srcio->input_handle);
		*/
		
		return 0;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		ase_ssize_t n = 0;
		FILE* fp;

	retry:
		fp = siod->data.file.handle;
		while (!ase_feof(fp) && n < size)
		{
			ase_cint_t c = ase_fgetc (fp);
			if (c == ASE_CHAR_EOF) 
			{
				if (ase_ferror(fp)) n = -1;
				break;
			}
			data[n++] = c;
		}

		if (n == 0)
		{
			siod->data.file.cur = ASE_SLL_NEXT(siod->data.file.cur);
			if (siod->data.file.cur != ASE_NULL)
			{
				ase_fclose (fp);
				siod->data.file.handle = ase_fopen (ASE_SLL_DPTR(siod->data.file.cur), ASE_T("r"));
				if (siod->data.file.handle == NULL) return -1;
				goto retry;
			}
		}
		return n;
	}

	return -1;
}

static ase_ssize_t awk_srcio_in (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_cint_t c;

	if (((srcio_data_t*)arg)->type == SRCIO_STR)
	{
		return awk_srcio_in_str (cmd, arg, data, size);
	}
	else
	{
		return awk_srcio_in_file (cmd, arg, data, size);
	}
}

static ase_ssize_t awk_srcio_out (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	/*struct srcio_data_t* srcio = (struct srcio_data_t*)arg;*/

	if (cmd == ASE_AWK_IO_OPEN) return 1;
	else if (cmd == ASE_AWK_IO_CLOSE) 
	{
		fflush (stdout);
		return 0;
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		ase_size_t left = size;

		while (left > 0)
		{
			if (*data == ASE_T('\0')) 
			{
				if (ase_fputc (*data, stdout) == ASE_CHAR_EOF) return -1;
				left -= 1; data += 1;
			}
			else
			{
				int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;
				int n = ase_fprintf (stdout, ASE_T("%.*s"), chunk, data);
				if (n < 0) return -1;
				left -= n; data += n;
			}
		}

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

			local_dprintf (ASE_T("opening %s of type %d (pipe)\n"),  epa->name, epa->type);
			handle = ase_popen (epa->name, mode);
			if (handle == NULL) return -1;
			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			local_dprintf (ASE_T("closing %s of type (pipe) %d\n"),  epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			/*
			int chunk = (size > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)size;
			if (ase_fgets (data, chunk, (FILE*)epa->handle) == ASE_NULL) 
			{
				if (ferror((FILE*)epa->handle)) return -1;
				return 0;
			}
			return ase_strlen(data);
			*/
			ase_ssize_t n = 0;
			FILE* fp = (FILE*)epa->handle;
			while (!ase_feof(fp) && n < size)
			{
				ase_cint_t c = ase_fgetc (fp);
				if (c == ASE_CHAR_EOF) 
				{
					if (ase_ferror(fp)) n = -1;
					break;
				}
				data[n++] = c;
			}
			return n;
		}

		case ASE_AWK_IO_WRITE:
		{
			FILE* fp = (FILE*)epa->handle;
			size_t left = size;

			while (left > 0)
			{
				if (*data == ASE_T('\0')) 
				{
				#if defined(ASE_CHAR_IS_WCHAR) && defined(__linux)
					if (fputc ('\0', fp) == EOF)
				#else
					if (ase_fputc (*data, fp) == ASE_CHAR_EOF) 
				#endif
					{
						return -1;
					}
					left -= 1; data += 1;
				}
				else
				{
				#if defined(ASE_CHAR_IS_WCHAR) && defined(__linux)
				/* fwprintf seems to return an error with the file
				 * pointer opened by popen, as of this writing. 
				 * anyway, hopefully the following replacement 
				 * will work all the way. */
					int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;
					int n = fprintf (fp, "%.*ls", chunk, data);
					if (n >= 0)
					{
						size_t x;
						for (x = 0; x < chunk; x++)
						{
							if (data[x] == ASE_T('\0')) break;
						}
						n = x;
					}
				#else
					int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;
					int n = ase_fprintf (fp, ASE_T("%.*s"), chunk, data);
				#endif
	
					if (n < 0 || n > chunk) return -1;
					left -= n; data += n;
				}
			}

			return size;
		}

		case ASE_AWK_IO_FLUSH:
		{
			if (epa->mode == ASE_AWK_EXTIO_PIPE_READ) return -1;
			return fflush ((FILE*)epa->handle);
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

			local_dprintf (ASE_T("opening %s of type %d (file)\n"), epa->name, epa->type);
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
			local_dprintf (ASE_T("closing %s of type %d (file)\n"), epa->name, epa->type);
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			/*
			int chunk = (size > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)size;
			if (ase_fgets (data, chunk, (FILE*)epa->handle) == ASE_NULL) 
			{
				if (ferror((FILE*)epa->handle)) return -1;
				return 0;
			}
			return ase_strlen(data);
			*/
			ase_ssize_t n = 0;
			FILE* fp = (FILE*)epa->handle;
			while (!ase_feof(fp) && n < size)
			{
				ase_cint_t c = ase_fgetc (fp);
				if (c == ASE_CHAR_EOF) 
				{
					if (ase_ferror(fp)) n = -1;
					break;
				}
				data[n++] = c;
			}
			return n;
		}

		case ASE_AWK_IO_WRITE:
		{
			FILE* fp = (FILE*)epa->handle;
			ase_ssize_t left = size;

			while (left > 0)
			{
				if (*data == ASE_T('\0')) 
				{
					if (ase_fputc (*data, fp) == ASE_CHAR_EOF) return -1;
					left -= 1; data += 1;
				}
				else
				{
					int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;
					int n = ase_fprintf (fp, ASE_T("%.*s"), chunk, data);
					if (n < 0) return -1;
					left -= n; data += n;
				}
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
static const ase_char_t* infiles[1000] =
{
	ASE_T(""),
	ASE_NULL
};

static ase_ssize_t getdata (ase_char_t* data, ase_size_t size, FILE* fp)
{
	ase_ssize_t n = 0;
	while (!ase_feof(fp) && n < size)
	{
		ase_cint_t c = ase_fgetc (fp);
		if (c == ASE_CHAR_EOF) 
		{
			if (ase_ferror(fp)) n = -1;
			break;
		}
		data[n++] = c;
		if (c == ASE_T('\n')) break;
	}
	return n;
}

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
		ase_ssize_t n;
		/*int chunk = (size > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)size;

		while (ase_fgets (data, chunk, (FILE*)epa->handle) == ASE_NULL)
		{
			if (ferror((FILE*)epa->handle)) return -1;*/

		while ((n = getdata(data,size,(FILE*)epa->handle)) == 0)
		{
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

						errarg.ptr = ASE_T("console");
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

				if (ase_awk_setglobal (
					epa->run, ASE_AWK_GLOBAL_FNR, ase_awk_val_zero) == -1)
				{
					/* need to reset FNR */
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

				local_dprintf (ASE_T("open the next console [%s]\n"), infiles[infile_no]);
				epa->handle = fp;
			}

			infile_no++;	
		}

		/*return ase_strlen(data);*/
		return n;
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		FILE* fp = (FILE*)epa->handle;
		ase_ssize_t left = size;

		while (left > 0)
		{
			if (*data == ASE_T('\0')) 
			{
				if (ase_fputc (*data, fp) == ASE_CHAR_EOF) return -1;
				left -= 1; data += 1;
			}
			else
			{
				int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;
				int n = ase_fprintf (fp, ASE_T("%.*s"), chunk, data);
				if (n < 0) return -1;
				left -= n; data += n;
			}
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

	local_dprintf (ASE_T("opening console[%s] of type %x\n"), epa->name, epa->type);

	if (epa->mode == ASE_AWK_EXTIO_CONSOLE_READ)
	{
		if (infiles[infile_no] == ASE_NULL)
		{
			/* no more input file */
			local_dprintf (ASE_T("console - no more file\n"));;
			return 0;
		}

		if (infiles[infile_no][0] == ASE_T('\0'))
		{
			local_dprintf (ASE_T("    console(r) - <standard input>\n"));
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

			local_dprintf (ASE_T("    console(r) - %s\n"), infiles[infile_no]);
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
		local_dprintf (ASE_T("    console(w) - <standard output>\n"));

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
	local_dprintf (ASE_T("closing console of type %x\n"), epa->type);

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

	local_dprintf (ASE_T("switching console[%s] of type %x\n"), epa->name, epa->type);

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
	local_dprintf (ASE_T("[AWK ABOUT TO START]\n"));
}

static int print_awk_value (ase_pair_t* pair, void* arg)
{
	ase_awk_run_t* run = (ase_awk_run_t*)arg;
	local_dprintf (ASE_T("%.*s = "), (int)pair->key.len, pair->key.ptr);
	ase_awk_dprintval (run, (ase_awk_val_t*)pair->val);
	local_dprintf (ASE_T("\n"));
	return 0;
}

static void on_run_statement (
	ase_awk_run_t* run, ase_size_t line, void* custom)
{
	/*local_dprintf (L"running %d\n", (int)line);*/
}

static void on_run_return (
	ase_awk_run_t* run, ase_awk_val_t* ret, void* custom)
{
	local_dprintf (ASE_T("[RETURN] - "));
	ase_awk_dprintval (run, ret);
	local_dprintf (ASE_T("\n"));

	local_dprintf (ASE_T("[NAMED VARIABLES]\n"));
	ase_map_walk (ase_awk_getrunnamedvarmap(run), print_awk_value, run);
	local_dprintf (ASE_T("[END NAMED VARIABLES]\n"));
}

static void on_run_end (ase_awk_run_t* run, int errnum, void* data)
{
	if (errnum != ASE_AWK_ENOERR)
	{
		local_dprintf (ASE_T("[AWK ENDED WITH AN ERROR]\n"));
		ase_printf (ASE_T("RUN ERROR: CODE [%d] LINE [%u] %s\n"),
			errnum, 
			(unsigned int)ase_awk_getrunerrlin(run),
			ase_awk_getrunerrmsg(run));
	}
	else local_dprintf (ASE_T("[AWK ENDED SUCCESSFULLY]\n"));

	app_run = NULL;
}

/* TODO: remove otab... */
static struct
{
	const ase_char_t* name;
	int opt;
} otab[] =
{
	{ ASE_T("implicit"),    ASE_AWK_IMPLICIT },
	{ ASE_T("explicit"),    ASE_AWK_EXPLICIT },
	{ ASE_T("bxor"),        ASE_AWK_BXOR },
	{ ASE_T("shift"),       ASE_AWK_SHIFT },
	{ ASE_T("idiv"),        ASE_AWK_IDIV },
	{ ASE_T("extio"),       ASE_AWK_EXTIO },
	{ ASE_T("newline"),     ASE_AWK_NEWLINE },
	{ ASE_T("baseone"),     ASE_AWK_BASEONE },
	{ ASE_T("stripspaces"), ASE_AWK_STRIPSPACES },
	{ ASE_T("nextofile"),   ASE_AWK_NEXTOFILE },
	{ ASE_T("crfl"),        ASE_AWK_CRLF },
	{ ASE_T("argstomain"),  ASE_AWK_ARGSTOMAIN },
	{ ASE_T("reset"),       ASE_AWK_RESET },
	{ ASE_T("maptovar"),    ASE_AWK_MAPTOVAR },
	{ ASE_T("pablock"),     ASE_AWK_PABLOCK }
};

static void print_usage (const ase_char_t* argv0)
{
	int j;

	ase_printf (ASE_T("Usage: %s [-m] [-d] [-a argument]* -f source-file [data-file]*\n"), argv0);

	ase_printf (ASE_T("\nYou may specify the following options to change the behavior of the interpreter.\n"));
	for (j = 0; j < ASE_COUNTOF(otab); j++)
	{
		ase_printf (ASE_T("    -%-20s -no%-20s\n"), otab[j].name, otab[j].name);
	}
}

static int run_awk (ase_awk_t* awk, 
	const ase_char_t* mfn, ase_awk_runarg_t* runarg)
{
	ase_awk_runcbs_t runcbs;
	ase_awk_runios_t runios;

	runios.pipe = awk_extio_pipe;
	runios.file = awk_extio_file;
	runios.console = awk_extio_console;
	runios.data = ASE_NULL;

	runcbs.on_start = on_run_start;
	runcbs.on_statement = on_run_statement;
	runcbs.on_return = on_run_return;
	runcbs.on_end = on_run_end;
	runcbs.data = ASE_NULL;

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

static int bfn_sleep (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* a0;
	ase_long_t lv;
	ase_real_t rv;
	ase_awk_val_t* r;
	int n;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 1);

	a0 = ase_awk_getarg (run, 0);

	n = ase_awk_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (ase_long_t)rv;

#ifdef _WIN32
	Sleep ((DWORD)(lv * 1000));
	n = 0;
#else
	n = sleep (lv);	
#endif

	r = ase_awk_makeintval (run, n);
	if (r == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	ase_awk_setretval (run, r);
	return 0;
}

static void out_of_memory (void)
{
	ase_fprintf (ASE_STDERR, ASE_T("Error: out of memory\n"));	
}

static int handle_args (int argc, ase_char_t* argv[], srcio_data_t* siod)
{
	static ase_opt_lng_t lng[] = 
	{
		{ ASE_T("implicit"),         0 },
		{ ASE_T("explicit"),         0 },
		{ ASE_T("bxor"),             0 },
		{ ASE_T("shift"),            0 },
		{ ASE_T("idiv"),             0 },
		{ ASE_T("extio"),            0 },
		{ ASE_T("newline"),          0 },
		{ ASE_T("baseone"),          0 },
		{ ASE_T("stripspaces"),      0 },
		{ ASE_T("nextofile"),        0 },
		{ ASE_T("crlf"),             0 },
		{ ASE_T("argstomain"),       0 },
		{ ASE_T("reset"),            0 },
		{ ASE_T("maptovar"),         0 },
		{ ASE_T("pablock"),          0 },

		{ ASE_T(":main"),            ASE_T('m') },
		{ ASE_T(":file"),            ASE_T('f') },
		{ ASE_T(":field-separator"), ASE_T('F') },
		{ ASE_T(":assign"),          ASE_T('v') },

		{ ASE_T("help"),             ASE_T('h') }
	};

	static ase_opt_t opt = 
	{
		ASE_T("hm:f:F:v:"),
		lng
	};

	ase_cint_t c;
	ase_sll_t* sf = ASE_NULL;

	while ((c = ase_getopt (argc, argv, &opt)) != ASE_CHAR_EOF)
	{
		switch (c)
		{
			case 0:
				ase_printf (ASE_T(">>> [%s] [%s]\n"), opt.lngopt, opt.arg);
				break;

			case ASE_T('h'):
				print_usage (argv[0]);
				if (sf != NULL) ase_sll_close (sf);
				return 1;

			case ASE_T('f'):
			{
				ase_size_t sz = ase_strlen(opt.arg) + 1;
				sz *= ASE_SIZEOF(*opt.arg);

				if (sf == ASE_NULL) 
				{
					sf = ase_sll_open (ASE_NULL, 0, ASE_NULL);
					if (sf == ASE_NULL)
					{
						out_of_memory ();
						return -1;	
					}
				}

				if (ase_sll_pushtail(sf, opt.arg, sz) == ASE_NULL)
				{
					out_of_memory ();
					return -1;	
				}

				break;
			}

			case ASE_T('F'):
				ase_printf  (ASE_T("[field separator] = %s\n"), opt.arg);
				break;

			case ASE_T('?'):
				if (opt.lngopt)
				{
					ase_printf (ASE_T("Error: illegal option - %s\n"), opt.lngopt);
				}
				else
				{
					ase_printf (ASE_T("Error: illegal option - %c\n"), opt.opt);
				}

				if (sf != ASE_NULL) ase_sll_close (sf);
				return -1;

			case ASE_T(':'):
				if (opt.lngopt)
				{
					ase_printf (ASE_T("Error: bad argument for %s\n"), opt.lngopt);
				}
				else
				{
					ase_printf (ASE_T("Error: bad argument for %c\n"), opt.opt);
				}

				if (sf != ASE_NULL) ase_sll_close (sf);
				return -1;

			default:
				if (sf != ASE_NULL) ase_sll_close (sf);
				return -1;
		}
	}


	if (ase_sll_getsize(sf) == 0)
	{
		if (opt.ind >= argc)
		{
			/* no source code specified */
			return -1;
		}

		siod->type = SRCIO_STR;
		siod->data.str.ptr = argv[opt.ind++];
		siod->data.str.cur = NULL;
	}
	else
	{
		/* read source code from the file */
		siod->type = SRCIO_FILE;
		siod->data.file.sll = sf;
		siod->data.file.cur = ase_sll_gethead(sf);
		siod->data.file.handle = NULL;
	}

	/* remaining args are input(console) file names */

	return 0;
}

typedef struct extension_t
{
	ase_mmgr_t mmgr;
	ase_awk_prmfns_t prmfns;
} 
extension_t;

static void init_extension (ase_awk_t* awk)
{
	extension_t* ext = ase_awk_getextension(awk);

	ext->mmgr = *ase_awk_getmmgr(awk);
	ase_awk_setmmgr (awk, &ext->mmgr);
	ase_awk_setccls (awk, ASE_GETCCLS());

	ext->prmfns.pow         = custom_awk_pow;
	ext->prmfns.sprintf     = custom_awk_sprintf;
	ext->prmfns.dprintf     = custom_awk_dprintf;
	ext->prmfns.data = ASE_NULL;
	ase_awk_setprmfns (awk, &ext->prmfns);
}

static ase_awk_t* open_awk (void)
{
	ase_awk_t* awk;
	ase_mmgr_t mmgr;

	memset (&mmgr, 0, ASE_SIZEOF(mmgr));
	mmgr.malloc  = custom_awk_malloc;
	mmgr.realloc = custom_awk_realloc;
	mmgr.free    = custom_awk_free;

#ifdef _WIN32
	mmgr.data = (void*)HeapCreate (0, 1000000, 1000000); /* TODO: get size from xxxx */
	if (mmgr.data == NULL)
	{
		ase_printf (ASE_T("Error: cannot create an awk heap\n"));
		return ASE_NULL;
	}
#else
	mmgr.data = ASE_NULL;
#endif

	awk = ase_awk_open (&mmgr, ASE_SIZEOF(extension_t), init_extension);
	if (awk == ASE_NULL)
	{
#ifdef _WIN32
		HeapDestroy ((HANDLE)mmgr.data);
#endif
		ase_printf (ASE_T("ERROR: cannot open awk\n"));
		return ASE_NULL;
	}

	ase_awk_setoption (awk, 
		ASE_AWK_IMPLICIT | ASE_AWK_EXTIO | ASE_AWK_NEWLINE | 
		ASE_AWK_BASEONE | ASE_AWK_PABLOCK);

	/* TODO: get depth from command line */
	ase_awk_setmaxdepth (
		awk, ASE_AWK_DEPTH_BLOCK_PARSE | ASE_AWK_DEPTH_EXPR_PARSE, 50);
	ase_awk_setmaxdepth (
		awk, ASE_AWK_DEPTH_BLOCK_RUN | ASE_AWK_DEPTH_EXPR_RUN, 500);

	/*
	ase_awk_seterrstr (awk, ASE_AWK_EGBLRED, 
		ASE_T("\uC804\uC5ED\uBCC0\uC218 \'%.*s\'\uAC00 \uC7AC\uC815\uC758 \uB418\uC5C8\uC2B5\uB2C8\uB2E4"));
	ase_awk_seterrstr (awk, ASE_AWK_EAFNRED, 
		ASE_T("\uD568\uC218 \'%.*s\'\uAC00 \uC7AC\uC815\uC758 \uB418\uC5C8\uC2B5\uB2C8\uB2E4"));
	*/
	/*ase_awk_setkeyword (awk, ASE_T("func"), 4, ASE_T("FX"), 2);*/

	if (ase_awk_addfunc (awk, 
		ASE_T("sleep"), 5, 0,
		1, 1, ASE_NULL, bfn_sleep) == ASE_NULL)
	{
		ase_awk_close (awk);
#ifdef _WIN32
		HeapDestroy ((HANDLE)mmgr.data);
#endif
		ase_printf (ASE_T("ERROR: cannot add function 'sleep'\n"));
		return ASE_NULL;
	}

	return awk;
}

static void close_awk (ase_awk_t* awk)
{
	extension_t* ext = ase_awk_getextension(awk);
	
#ifdef _WIN32
	HANDLE heap = (HANDLE)ext->mmgr->data;
#endif

	ase_awk_close (awk);

#ifdef _WIN32
	HeapDestroy (heap);
#endif
}

static int awk_main (int argc, ase_char_t* argv[])
{
	ase_awk_t* awk;
	srcio_data_t siod;

	ase_awk_srcios_t srcios;
	int i, file_count = 0;
	const ase_char_t* mfn = ASE_NULL;
	int mode = 0;
	int runarg_count = 0;
	ase_awk_runarg_t runarg[128];
	int deparse = 0;

	i = handle_args (argc, argv, &siod);
	if (i == -1)
	{
		print_usage (argv[0]);
		return -1;
	}
	if (i == 1) return 0;

	infiles[file_count] = ASE_NULL;
	runarg[runarg_count].ptr = NULL;
	runarg[runarg_count].len = 0;

	awk = open_awk ();
	if (awk == ASE_NULL) return -1;

	app_awk = awk;

	srcios.in = awk_srcio_in;
	srcios.out = deparse? awk_srcio_out: NULL;
	srcios.data = &siod;

	if (ase_awk_parse (awk, &srcios) == -1) 
	{
		ase_printf (
			ASE_T("PARSE ERROR: CODE [%d] LINE [%u] %s\n"), 
			ase_awk_geterrnum(awk),
			(unsigned int)ase_awk_geterrlin(awk), 
			ase_awk_geterrmsg(awk));
		close_awk (awk);
		return -1;
	}

// TODO: close siod....

#ifdef _WIN32
	SetConsoleCtrlHandler (stop_run, TRUE);
#else
	signal (SIGINT, stop_run);
#endif

	if (run_awk (awk, mfn, runarg) == -1)
	{
		close_awk (awk);
		return -1;
	}

	close_awk (awk);

	return 0;
}

int ase_main (int argc, ase_achar_t* argv[])
{
	int n;

#if defined(_WIN32) && defined(_DEBUG) && defined(_MSC_VER)
	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
#endif

	n = ase_runmain (argc, argv, awk_main);

#if defined(_WIN32) && defined(_DEBUG)
	/*#if defined(_MSC_VER)
	_CrtDumpMemoryLeaks ();
	#endif*/
#endif

	return n;
}
