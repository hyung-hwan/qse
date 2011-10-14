
#include <qse/fs/dir.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

int path_main (int argc, qse_char_t* argv[])
{
	if (argc != 2)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <directory>\n"), argv[0]);
		return -1;
	}

	qse_printf (QSE_T("[%s] => [%s]\n"), argv[1], qse_canonpath (argv[1]));
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, path_main);
}

