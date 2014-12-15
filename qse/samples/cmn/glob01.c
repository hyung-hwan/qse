#include <qse/cmn/glob.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/fmt.h>
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
		if (qse_glob (argv[i], print, QSE_NULL, QSE_GLOB_PERIOD, 
		              qse_getdflmmgr(), qse_getdflcmgr()) <= -1) return -1;
	}

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int x;
#if defined(_WIN32)
 	char locale[100];
	UINT codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		/* .codepage */
		qse_fmtuintmaxtombs (locale, QSE_COUNTOF(locale),
			codepage, 10, -1, QSE_MT('\0'), QSE_MT("."));
		setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	qse_openstdsios ();

	x = qse_runmain (argc, argv, glob_main);

	qse_closestdsios ();

	return x;
}

