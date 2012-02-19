#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/stdio.h>

#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#endif

static int test_main (int argc, qse_char_t* argv[])
{
	int i;

	for (i = 0; i < argc; i++)
	{
		qse_printf (QSE_T("%d => [%s]\n"), i, argv[i]);
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

	return qse_runmain (argc, argv, test_main);
}

