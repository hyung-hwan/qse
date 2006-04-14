/*
 * $Id: awk.c,v 1.21 2006-04-14 11:14:58 bacon Exp $
 */

#include <xp/awk/awk.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>

#ifdef XP_CHAR_IS_MCHAR
#define xp_printf printf
#else
#define xp_printf wprintf
#endif

#ifdef XP_CHAR_IS_MCHAR
#define xp_strcmp strcmp
#else
#define xp_strcmp wcscmp
#endif

static xp_ssize_t process_source (int cmd, void* arg, xp_char_t* data, xp_size_t size)
{
	xp_char_t c;

	switch (cmd) {
	case XP_AWK_IO_OPEN:
	case XP_AWK_IO_CLOSE:
		return 0;

	case XP_AWK_IO_DATA:
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

	return -1;
}

#ifdef __linux
#include <mcheck.h>
#endif

#if defined(vms) || defined(__vms) || defined(_SCO_DS) 
int main (int argc, char* argv[])
#else
int xp_main (int argc, xp_char_t* argv[])
#endif
{
	xp_awk_t* awk;

#ifdef __linux
	mtrace ();
#endif

	if ((awk = xp_awk_open()) == XP_NULL) 
	{
		xp_printf (XP_TEXT("Error: cannot open awk\n"));
		return -1;
	}

	if (xp_awk_attsrc(awk, process_source, XP_NULL) == -1) 
	{
		xp_awk_close (awk);
		xp_printf (XP_TEXT("error: cannot attach source\n"));
		return -1;
	}

	xp_awk_setparseopt (awk, 
		XP_AWK_EXPLICIT | XP_AWK_UNIQUE | 
		XP_AWK_SHADING | XP_AWK_IMPLICIT | XP_AWK_SHIFT);

	if (argc == 2) 
	{
#if defined(vms) || defined(__vms) || defined(_SCO_DS) 
		if (strcmp(argv[1], "-m") == 0)
#else
		if (xp_strcmp(argv[1], XP_TEXT("-m")) == 0)
#endif
		{
			xp_awk_setrunopt (awk, XP_AWK_RUNMAIN);
		}
	}

	if (xp_awk_parse(awk) == -1) 
	{
		xp_printf (
			XP_TEXT("error: cannot parse program - [%d] %s\n"), 
			xp_awk_geterrnum(awk), xp_awk_geterrstr(awk));
		xp_awk_close (awk);
		return -1;
	}

	if (xp_awk_run(awk) == -1) 
	{
		xp_printf (
			XP_TEXT("error: cannot run program - [%d] %s\n"), 
			xp_awk_geterrnum(awk), xp_awk_geterrstr(awk));
		xp_awk_close (awk);
		return -1;
	}

	xp_awk_close (awk);

#ifdef __linux
	muntrace ();
#endif
	return 0;
}
