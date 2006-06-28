/*
 * $Id: awk.c,v 1.45 2006-06-28 08:56:59 bacon Exp $
 */

#include <xp/awk/awk.h>
#include <stdio.h>
#include <string.h>
#ifdef XP_CHAR_IS_WCHAR
#include <wchar.h>
#endif

#ifndef __STAND_ALONE
#include <xp/bas/stdio.h>
#include <xp/bas/string.h>
#endif

#ifdef _WIN32
#include <tchar.h>
#endif

#ifdef __STAND_ALONE
	#define xp_printf xp_awk_printf
	extern int xp_awk_printf (const xp_char_t* fmt, ...); 
	#define xp_strcmp xp_awk_strcmp
	extern int xp_awk_strcmp (const xp_char_t* s1, const xp_char_t* s2);
	#define xp_strlen xp_awk_strlen
	extern int xp_awk_strlen (const xp_char_t* s);
#endif

#if defined(_WIN32) && defined(__STAND_ALONE) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if defined(__linux) && defined(_DEBUG)
#include <mcheck.h>
#endif

static xp_ssize_t process_source (
	int cmd, int opt, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_char_t c;

	switch (cmd) 
	{
		case XP_AWK_IO_OPEN:
		case XP_AWK_IO_CLOSE:
		case XP_AWK_IO_NEXT:
		{
			return 0;
		}

		case XP_AWK_IO_READ:
		{
			if (size <= 0) return -1;
		#ifdef XP_CHAR_IS_MCHAR
			c = fgetc (stdin);
		#else
			c = fgetwc (stdin);
		#endif
			if (c == XP_CHAR_EOF) return 0;
			*data = c;
			return 1;
		}

		case XP_AWK_IO_WRITE:
		{
			return -1;
		}
	}

	return -1;
}


struct data_io
{
	const char* input_file;
	FILE* input_handle;
};

static xp_ssize_t process_data (
	int cmd, int opt, void* arg, xp_char_t* data, xp_size_t size)
{
	struct data_io* io = (struct data_io*)arg;
	xp_char_t c;

	switch (cmd) 
	{
		case XP_AWK_IO_OPEN:
		{
			io->input_handle = fopen (io->input_file, "r");
			if (io->input_handle == NULL) return -1;
			return 0;
		}

		case XP_AWK_IO_CLOSE:
		{
			fclose (io->input_handle);
			io->input_handle = NULL;
			return 0;
		}

		case XP_AWK_IO_READ:
		{
			if (size <= 0) return -1;
		#ifdef XP_CHAR_IS_MCHAR
			c = fgetc (io->input_handle);
		#else
			c = fgetwc (io->input_handle);
		#endif
			if (c == XP_CHAR_EOF) return 0;
			*data = c;
			return 1;
		}

		case XP_AWK_IO_WRITE:
		{
			return -1;
		}

		case XP_AWK_IO_NEXT:
		{
			/* input switching not supported for the time being... */
			return -1;
		}

	}

	return -1;
}

static xp_ssize_t process_extio_pipe (
	int cmd, int opt, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_awk_extio_t* epa = (xp_awk_extio_t*)arg;

	switch (cmd)
	{
		case XP_AWK_IO_OPEN:
		{
			FILE* handle;
			const xp_char_t* mode;

			if (opt == XP_AWK_IO_PIPE_READ)
				mode = XP_T("r");
			else if (opt == XP_AWK_IO_PIPE_WRITE)
				mode = XP_T("w");
			else return -1; /* TODO: any way to set the error number? */
xp_printf (XP_TEXT("opending %s of type %d (pipe)\n"),  epa->name, epa->type);
			handle = _tpopen (epa->name, mode);
			if (handle == NULL) return -1;
			epa->handle = (void*)handle;
			return 0;
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
			if (_fgetts (data, size, (FILE*)epa->handle) == XP_NULL) 
				return 0;
			return xp_strlen(data);
		}

		case XP_AWK_IO_WRITE:
		{
			/*
			if (_fputts (data, size, (FILE*)epa->handle) == XP_NULL) 
				return 0;
			return size;
			*/
			return -1;
		}

		case XP_AWK_IO_NEXT:
		{
			return -1;
		}
	}

	return -1;
}

static xp_ssize_t process_extio_file (
	int cmd, int opt, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_awk_extio_t* epa = (xp_awk_extio_t*)arg;

	switch (cmd)
	{
		case XP_AWK_IO_OPEN:
		{
			FILE* handle;
			const xp_char_t* mode;

			if (opt == XP_AWK_IO_FILE_READ)
				mode = XP_T("r");
			else if (opt == XP_AWK_IO_FILE_WRITE)
				mode = XP_T("w");
			else if (opt == XP_AWK_IO_FILE_APPEND)
				mode = XP_T("a");
			else return -1; /* TODO: any way to set the error number? */

xp_printf (XP_TEXT("opending %s of type %d (file)\n"),  epa->name, epa->type);
			handle = _tfopen (epa->name, mode);
			if (handle == NULL) return -1;
			epa->handle = (void*)handle;
			return 0;
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
			if (_fgetts (data, size, (FILE*)epa->handle) == XP_NULL) 
				return 0;
			return xp_strlen(data);
		}

		case XP_AWK_IO_WRITE:
		{
			/* TODO: how to return error or 0 */
			_fputts (data, /*size,*/ (FILE*)epa->handle);
			return -1;
		}

		case XP_AWK_IO_NEXT:
		{
			return -1;
		}

	}

	return -1;
}

static xp_ssize_t process_extio_console (
	int cmd, int opt, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_awk_extio_t* epa = (xp_awk_extio_t*)arg;

	switch (cmd)
	{
		case XP_AWK_IO_OPEN:
		{
			/* TODO: OpenConsole in GUI APPLICATION */
			/* opt: XP_AWK_IO_CONSOLE_READ, 
			 *      XP_AWK_IO_CONSOLE_WRITE */

xp_printf (XP_TEXT("opending %s of type %d (console)\n"),  epa->name, epa->type);
			if (opt == XP_AWK_IO_CONSOLE_READ)
				epa->handle = stdin;
			else if (opt == XP_AWK_IO_CONSOLE_WRITE)
				epa->handle = stdout;
			else return -1;
		}

		case XP_AWK_IO_CLOSE:
		{
xp_printf (XP_TEXT("closing %s of type %d (console)\n"),  epa->name, epa->type);
			/* TODO: CloseConsole in GUI APPLICATION */
			return 0;
		}

		case XP_AWK_IO_READ:
		{
			if (_fgetts (data, size, (FILE*)epa->handle) == XP_NULL) 
				return 0;
			return xp_strlen(data);
		}

		case XP_AWK_IO_WRITE:
		{
			/* TODO: how to return error or 0 */
			_fputts (data, /*size,*/ (FILE*)epa->handle);
			return size;
		}

		case XP_AWK_IO_NEXT:
		{
			return -1;
		}

	}

	return -1;
}

#if defined(__STAND_ALONE) && !defined(_WIN32)
static int __main (int argc, char* argv[])
#else
static int __main (int argc, xp_char_t* argv[])
#endif
{
	xp_awk_t* awk;
	struct data_io data_io = { "awk.in", NULL };

	if ((awk = xp_awk_open()) == XP_NULL) 
	{
		xp_printf (XP_T("Error: cannot open awk\n"));
		return -1;
	}

	if (xp_awk_attsrc(awk, process_source, XP_NULL) == -1) 
	{
		xp_awk_close (awk);
		xp_printf (XP_T("Error: cannot attach source\n"));
		return -1;
	}

/* TODO: */
	if (xp_awk_setextio (awk, 
		XP_AWK_EXTIO_PIPE, process_extio_pipe, XP_NULL) == -1)
	{
		xp_awk_close (awk);
		xp_printf (XP_T("Error: cannot set extio pipe\n"));
		return -1;
	}

/* TODO: */
	if (xp_awk_setextio (awk, 
		XP_AWK_EXTIO_FILE, process_extio_file, XP_NULL) == -1)
	{
		xp_awk_close (awk);
		xp_printf (XP_T("Error: cannot set extio file\n"));
		return -1;
	}

	if (xp_awk_setextio (awk, 
		XP_AWK_EXTIO_CONSOLE, process_extio_console, XP_NULL) == -1)
	{
		xp_awk_close (awk);
		xp_printf (XP_T("Error: cannot set extio file\n"));
		return -1;
	}

	xp_awk_setparseopt (awk, 
		XP_AWK_EXPLICIT | XP_AWK_UNIQUE | XP_AWK_DBLSLASHES |
		XP_AWK_SHADING | XP_AWK_IMPLICIT | XP_AWK_SHIFT | XP_AWK_EXTIO);

	if (argc == 2) 
	{
#if defined(__STAND_ALONE) && !defined(_WIN32)
		if (strcmp(argv[1], "-m") == 0)
#else
		if (xp_strcmp(argv[1], XP_T("-m")) == 0)
#endif
		{
			xp_awk_setrunopt (awk, XP_AWK_RUNMAIN);
		}
	}

	if (xp_awk_parse(awk) == -1) 
	{
#if defined(__STAND_ALONE) && !defined(_WIN32) && defined(XP_CHAR_IS_WCHAR)
		xp_printf (
			XP_T("error: cannot parse program - line %u [%d] %ls\n"), 
			(unsigned int)xp_awk_getsrcline(awk), 
			xp_awk_geterrnum(awk), xp_awk_geterrstr(awk));
#else
		xp_printf (
			XP_T("error: cannot parse program - line %u [%d] %s\n"), 
			(unsigned int)xp_awk_getsrcline(awk), 
			xp_awk_geterrnum(awk), xp_awk_geterrstr(awk));
#endif
		xp_awk_close (awk);
		return -1;
	}

	if (xp_awk_run (awk, process_data, (void*)&data_io) == -1) 
	{
#if defined(__STAND_ALONE) && !defined(_WIN32) && defined(XP_CHAR_IS_WCHAR)
		xp_printf (
			XP_T("error: cannot run program - [%d] %ls\n"), 
			xp_awk_geterrnum(awk), xp_awk_geterrstr(awk));
#else
		xp_printf (
			XP_T("error: cannot run program - [%d] %s\n"), 
			xp_awk_geterrnum(awk), xp_awk_geterrstr(awk));
#endif
		xp_awk_close (awk);
		return -1;
	}

	xp_awk_close (awk);
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
/*#if defined(_WIN32) && defined(__STAND_ALONE) && defined(_DEBUG)
	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif*/

	n = __main (argc, argv);

#if defined(__linux) && defined(_DEBUG)
	muntrace ();
#endif
#if defined(_WIN32) && defined(__STAND_ALONE) && defined(_DEBUG)
	_CrtDumpMemoryLeaks ();
	wprintf (L"Press ENTER to quit\n");
	getchar ();
#endif

	return n;
}

