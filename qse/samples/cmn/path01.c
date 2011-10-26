
#include <qse/cmn/path.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>

int path_main (int argc, qse_char_t* argv[])
{
	qse_char_t* canon;
	qse_size_t len;
	int i;
	int options[] = 
	{ 
		0,
		QSE_CANONPATH_EMPTYSINGLEDOT,
		QSE_CANONPATH_KEEPDOUBLEDOTS,
		QSE_CANONPATH_DROPTRAILINGSEP,
		QSE_CANONPATH_EMPTYSINGLEDOT | QSE_CANONPATH_KEEPDOUBLEDOTS,
		QSE_CANONPATH_EMPTYSINGLEDOT | QSE_CANONPATH_DROPTRAILINGSEP,
		QSE_CANONPATH_KEEPDOUBLEDOTS | QSE_CANONPATH_DROPTRAILINGSEP,
		QSE_CANONPATH_EMPTYSINGLEDOT | QSE_CANONPATH_KEEPDOUBLEDOTS | QSE_CANONPATH_DROPTRAILINGSEP
	};

	if (argc != 2)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <directory>\n"), argv[0]);
		return -1;
	}

	canon = QSE_MMGR_ALLOC (
		QSE_MMGR_GETDFL(), (qse_strlen(argv[1]) + 1) * QSE_SIZEOF(*canon));
	if (!canon)
	{
		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), canon); 
		qse_fprintf (QSE_STDERR, QSE_T("Error: out of memory\n"));
		return -1;
	}

	for (i = 0; i < QSE_COUNTOF(options); i++)
	{
		len = qse_canonpath (argv[1], canon, options[i]);
		qse_printf (QSE_T("OPT[%c%c%c] [%s]: [%5d] [%s]\n"), 
			((options[i] & QSE_CANONPATH_EMPTYSINGLEDOT)? QSE_T('E'): QSE_T(' ')),
			((options[i] & QSE_CANONPATH_KEEPDOUBLEDOTS)? QSE_T('K'): QSE_T(' ')),
			((options[i] & QSE_CANONPATH_DROPTRAILINGSEP)? QSE_T('D'): QSE_T(' ')),
			argv[1], (int)len, canon);
	}

	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), canon); 

#if 0
	qse_printf (QSE_T("[%s] => "), argv[1]);
	len = qse_canonpath (argv[1], argv[1], 0);
	qse_printf (QSE_T("[%s] %d chars\n"), argv[1], (int)len);
#endif

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, path_main);
}

