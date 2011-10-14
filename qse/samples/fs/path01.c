
#include <qse/fs/dir.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>

int path_main (int argc, qse_char_t* argv[])
{
	qse_char_t* canon;
	qse_size_t len;

	if (argc != 2)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <directory>\n"), argv[0]);
		return -1;
	}

	canon = QSE_MMGR_ALLOC (QSE_MMGR_GETDFL(), (qse_strlen(argv[1]) + 1) * QSE_SIZEOF(*canon));
	if (canon == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Error: out of memory\n"));
		return -1;
	}

	len = qse_canonpath (argv[1], canon);
	qse_printf (QSE_T("[%s] => [%s] %d chars\n"), argv[1], canon, (int)len);
	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), canon);
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, path_main);
}

