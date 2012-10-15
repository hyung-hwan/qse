#include <qse/cmn/rex.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/path.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static int rex_main (int argc, qse_char_t* argv[])
{
	qse_rex_t* rex;
	qse_rex_node_t* start;
	qse_cstr_t str;
	qse_cstr_t matstr;
	int n;

	if (argc != 3)
	{
		qse_printf (QSE_T("USAGE: %s pattern string\n"), 
			qse_basename(argv[0]));
		return -1;
	}

	rex = qse_rex_open (QSE_MMGR_GETDFL(), 0, QSE_NULL);
	if (rex == QSE_NULL)
	{
		qse_printf (QSE_T("ERROR: cannot open rex\n"));
		return -1;
	}

	qse_rex_setoption (rex, QSE_REX_STRICT);

	start = qse_rex_comp (rex, argv[1], qse_strlen(argv[1]));
	if (start == QSE_NULL)
	{
		qse_printf (QSE_T("ERROR: cannot compile - %s\n"),
			qse_rex_geterrmsg(rex));
		qse_rex_close (rex);
		return -1;
	}

	str.ptr = argv[2];
	str.len = qse_strlen(argv[2]);

	qse_printf (QSE_T("compile ok\n"));

	n = qse_rex_exec (rex, &str, &str, &matstr);
	if (n <= -1)
	{
		qse_printf (QSE_T("ERROR: cannot execute - %s\n"),
			qse_rex_geterrmsg(rex));
		qse_rex_close (rex);
		return -1;
	}
	if (n >= 1)
	{
		qse_printf (QSE_T("MATCH: [%.*s] beginning from char #%d\n"), 
			(int)matstr.len, matstr.ptr,
			(int)(matstr.ptr - str.ptr + 1));
	}

	qse_rex_close (rex);
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
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
     	sprintf (locale, ".%u", (unsigned int)codepage);
     	setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
     setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif
	return qse_runmain (argc, argv, rex_main);
}


