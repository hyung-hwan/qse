/*
 * $Id: awk.c,v 1.30 2006-05-12 09:39:20 bacon Exp $
 */

#include <xp/awk/awk.h>
#include <stdio.h>
#include <string.h>
#ifdef XP_CHAR_IS_CHAR
#include <wchar.h>
#endif

#ifndef __STAND_ALONE
#include <xp/bas/stdio.h>
#include <xp/bas/string.h>
#endif

#ifdef __STAND_ALONE
	#define xp_printf xp_awk_printf
	extern int xp_awk_printf (const xp_char_t* fmt, ...); 
	#define xp_strcmp xp_awk_strcmp
	extern int xp_awk_strcmp (const xp_char_t* s1, const xp_char_t* s2);
#endif

#if defined(_WIN32) && defined(__STAND_ALONE) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if defined(__linux) && !defined(NDEBUG)
#include <mcheck.h>
#endif

static xp_ssize_t process_source (
	int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_char_t c;

	switch (cmd) {
	case XP_AWK_INPUT_OPEN:
	case XP_AWK_INPUT_CLOSE:
	case XP_AWK_INPUT_NEXT:
		return 0;

	case XP_AWK_INPUT_DATA:
		if (size <= 0) return -1;
#ifdef XP_CHAR_IS_MCHAR
		c = fgetc (stdin);
#else
		c = fgetwc (stdin);
#endif
		if (c == XP_CHAR_EOF) return 0;
		*data = c;
		return 1;

	case XP_AWK_OUTPUT_OPEN:
	case XP_AWK_OUTPUT_CLOSE:
	case XP_AWK_OUTPUT_NEXT:
	case XP_AWK_OUTPUT_DATA:
		return 0;
	}

	return -1;
}

struct data_io
{
	const char* input_file;
	FILE* input_handle;
};

static xp_ssize_t process_data (
	int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	struct data_io* io = (struct data_io*)arg;
	xp_char_t c;

	switch (cmd) {
	case XP_AWK_INPUT_OPEN:
		io->input_handle = fopen (io->input_file, "r");
		if (io->input_handle == NULL) return -1;
		return 0;

	case XP_AWK_INPUT_CLOSE:
		fclose (io->input_handle);
		io->input_handle = NULL;
		return 0;

	case XP_AWK_INPUT_NEXT:
		/* input switching not supported for the time being... */
		return -1;

	case XP_AWK_INPUT_DATA:
		if (size <= 0) return -1;
#ifdef XP_CHAR_IS_MCHAR
		c = fgetc (io->input_handle);
#else
		c = fgetwc (io->input_handle);
#endif
		if (c == XP_CHAR_EOF) return 0;
		*data = c;
		return 1;

	case XP_AWK_OUTPUT_OPEN:
	case XP_AWK_OUTPUT_CLOSE:
	case XP_AWK_OUTPUT_NEXT:
	case XP_AWK_OUTPUT_DATA:
		return -1;
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

	xp_awk_setparseopt (awk, 
		XP_AWK_EXPLICIT | XP_AWK_UNIQUE | 
		XP_AWK_SHADING | XP_AWK_IMPLICIT | XP_AWK_SHIFT);

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
			XP_T("error: cannot parse program - [%d] %ls\n"), 
			xp_awk_geterrnum(awk), xp_awk_geterrstr(awk));
#else
		xp_printf (
			XP_T("error: cannot parse program - [%d] %s\n"), 
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
#if defined(__linux) && !defined(NDEBUG)
	mtrace ();
#endif

	n = __main (argc, argv);

#if defined(__linux) && !defined(NDEBUG)
	muntrace ();
#endif
#if defined(_WIN32) && defined(__STAND_ALONE) && defined(_DEBUG)
	_CrtDumpMemoryLeaks ();
#endif
	return n;
}

