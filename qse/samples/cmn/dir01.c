#include <qse/cmn/dir.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>
#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#endif

static int list_dir (int argc, qse_char_t* argv[], int flags)
{
	qse_dir_t* dir = QSE_NULL;
	qse_dir_ent_t dirent;
	void* xpath = QSE_NULL;
	void* xdirpath;

	if (argc < 2)
	{
		if (flags & QSE_DIR_MBSPATH)
			xdirpath = QSE_MT(".");
		else
			xdirpath = QSE_WT(".");

		dir = qse_dir_open (QSE_MMGR_GETDFL(), 0, xdirpath, flags, QSE_NULL);
		if (!dir)
		{
			qse_printf (QSE_T("Cannot open .\n"));
			goto oops;
		}
	}
	else
	{
		if (flags & QSE_DIR_MBSPATH)
			xpath = qse_strtombsdup (argv[1], QSE_MMGR_GETDFL());
		else 
			xpath = qse_strtowcsdup (argv[1], QSE_MMGR_GETDFL());

		xdirpath = xpath;
		dir = qse_dir_open (QSE_MMGR_GETDFL(), 0, xdirpath, flags, QSE_NULL);
		if (!dir)
		{
			qse_printf (QSE_T("Cannot open %s\n"), argv[1]);
			goto oops;
		}
	}
	

	while (qse_dir_read (dir, &dirent) > 0)
	{
		if (flags & QSE_DIR_MBSPATH)
			qse_printf (QSE_T("%hs\n"), dirent.name);
		else
			qse_printf (QSE_T("%ls\n"), dirent.name);
	}

	qse_printf (QSE_T("----------------------------------------\n"));
	if (qse_dir_reset (dir, xdirpath) <= -1)
	{
		qse_printf (QSE_T("Cannot reset\n"));
		goto oops;
	}
	

	while (qse_dir_read (dir, &dirent) > 0)
	{
		if (flags & QSE_DIR_MBSPATH)
			qse_printf (QSE_T("%hs\n"), dirent.name);
		else
			qse_printf (QSE_T("%ls\n"), dirent.name);
	}

	QSE_MMGR_FREE(QSE_MMGR_GETDFL(), xpath);
	qse_dir_close (dir);
	return 0;

oops:
	if (xpath) QSE_MMGR_FREE(QSE_MMGR_GETDFL(), xpath);
	if (dir) qse_dir_close(dir);
	return -1;
}

static int dir_main (int argc, qse_char_t* argv[])
{
	if (list_dir (argc, argv, QSE_DIR_MBSPATH) <= -1) return -1;
	qse_printf (QSE_T("===============================\n"));
	if (list_dir (argc, argv, QSE_DIR_MBSPATH | QSE_DIR_SORT) <= -1) return -1;
	qse_printf (QSE_T("===============================\n"));
	if (list_dir (argc, argv, QSE_DIR_WCSPATH) <= -1) return -1;
	qse_printf (QSE_T("===============================\n"));
	if (list_dir (argc, argv, QSE_DIR_WCSPATH | QSE_DIR_SORT) <= -1) return -1;
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

	x = qse_runmain (argc, argv, dir_main);

	qse_closestdsios ();

	return x;
}

