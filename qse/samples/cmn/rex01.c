#include <qse/cmn/rex.h>
#include <qse/cmn/str.h>
#include <qse/cmn/main.h>
#include <qse/cmn/misc.h>
#include <qse/cmn/stdio.h>

static int rex_main (int argc, qse_char_t* argv[])
{
	qse_rex_t* rex;
	qse_rex_node_t* start;

	if (argc != 3)
	{
		qse_printf (QSE_T("USAGE: %s pattern string\n"), 
			qse_basename(argv[0]));
		return -1;
	}

	rex = qse_rex_open (QSE_NULL, 0, QSE_NULL);
	if (rex == QSE_NULL)
	{
		qse_printf (QSE_T("ERROR: cannot open rex\n"));
		return -1;
	}

	start = qse_rex_comp (rex, argv[1], qse_strlen(argv[1]));
	if (start == QSE_NULL)
	{
		qse_printf (QSE_T("ERROR: cannot compile - %s\n"),
			qse_rex_geterrmsg(rex));
		qse_rex_close (rex);
		return -1;
	}

	if (qse_rex_exec (rex, 
		argv[2], qse_strlen(argv[2]),
		argv[2], qse_strlen(argv[2])) <= -1)
	{
		qse_printf (QSE_T("ERROR: cannot execute - %s\n"),
			qse_rex_geterrmsg(rex));
		qse_rex_close (rex);
		return -1;
	}

	qse_rex_close (rex);
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, rex_main);
}


