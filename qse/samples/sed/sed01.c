#include <qse/sed/std.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>
#include "sed00.h"

static int sed_main (int argc, qse_char_t* argv[])
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

	/* create a sed object */
	sed = qse_sed_openstd (0);
	if (sed == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open sed\n"));
		goto oops;
	}

	/* compile commands */
	if (qse_sed_compstdstr (sed, argv[1]) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_sed_geterrmsg(sed));
		goto oops;
	}

	infile = (argc >= 3)? argv[2]: QSE_NULL;
	outfile = (argc >= 4)? argv[3]: QSE_NULL;

	/* executes the compiled commands over the intput and output files specified */
	if (qse_sed_execstdfile (sed, infile, outfile, QSE_NULL) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_sed_geterrmsg(sed));
		goto oops;
	}

oops:
	/* destroy the sed object */
	if (sed) qse_sed_close (sed);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	init_sed_sample_locale ();
	return qse_runmain (argc, argv, sed_main);
}
