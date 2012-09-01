#include <qse/cmn/glob.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/path.h>


#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static int print (const qse_cstr_t* path, void* ctx)
{
	qse_printf  (QSE_T("[%.*s]\n"), (int)path->len, path->ptr);
	return 0;
}

static int glob_main (int argc, qse_char_t* argv[])
{
	int i;

	if (argc <= 1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s file-pattern ...\n"), qse_basename(argv[0]));
		return -1;
	}

	for (i = 1; i < argc; i++)
	{
		if (qse_glob (argv[i], print, QSE_NULL, QSE_GLOB_PERIOD, QSE_MMGR_GETDFL()) <= -1) return -1;
	}

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
#if defined(_WIN32)
     char locale[100];
	UINT codepage = GetConsoleOutputCP();	
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgr (qse_utf8cmgr);
	}
	else
	{
     	sprintf (locale, ".%u", (unsigned int)codepage);
     	setlocale (LC_ALL, locale);
		qse_setdflcmgr (qse_slmbcmgr);
	}
#else
     setlocale (LC_ALL, "");
	qse_setdflcmgr (qse_slmbcmgr);
#endif
	return qse_runmain (argc, argv, glob_main);
}

