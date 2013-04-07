#include <qse/cmn/dir.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/mem.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static int test_main (int argc, qse_char_t* argv[])
{
	qse_dir_t* dir;
	qse_dir_ent_t dirent;

	dir = qse_dir_open (QSE_MMGR_GETDFL(), 0, (argc < 2? QSE_T("."): argv[1]), QSE_DIR_SORT, QSE_NULL);

	while (qse_dir_read (dir, &dirent) > 0)
	{
		qse_printf (QSE_T("%s\n"), dirent.name);
	}

	qse_printf (QSE_T("----------------------------------------\n"));
	qse_dir_reset (dir, (argc < 2? QSE_T("."): argv[1]));

	while (qse_dir_read (dir, &dirent) > 0)
	{
		qse_printf (QSE_T("%s\n"), dirent.name);
	}
	qse_dir_close (dir);
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
	return qse_runmain (argc, argv, test_main);
}

