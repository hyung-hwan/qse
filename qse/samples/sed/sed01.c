#include <qse/sed/std.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

int sed_main (int argc, qse_char_t* argv[])
{
	qse_sed_t* sed = QSE_NULL;
	qse_char_t* infile;
	qse_char_t* outfile;
	int ret = -1;

	if (argc <  2 || argc > 4)
	{
		qse_fprintf (QSE_STDERR, QSE_T("USAGE: %s command-string [input-file [output-file]]\n"), argv[0]);
		return -1;
	}

	sed = qse_sed_openstd (0);
	if (sed == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open sed\n"));
		goto oops;
	}

	if (qse_sed_compstdstr (sed, argv[1]) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_sed_geterrmsg(sed));
		goto oops;
	}

	infile = (argc >= 3)? argv[2]: QSE_NULL;
	outfile = (argc >= 4)? argv[3]: QSE_NULL;

	if (qse_sed_execstdfile (sed, infile, outfile, QSE_NULL) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_sed_geterrmsg(sed));
		goto oops;
	}

oops:
	if (sed != QSE_NULL) qse_sed_close (sed);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
#if defined(_WIN32)
	char locale[100];
	UINT codepage = GetConsoleOutputCP();	
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOutputCP (CP_UTF8);*/
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
	return qse_runmain (argc, argv, sed_main);
}

