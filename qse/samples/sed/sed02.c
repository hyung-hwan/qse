#include <qse/sed/stdsed.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>
#include "sed00.h"

static int sed_main (int argc, qse_char_t* argv[])
{
	qse_sed_t* sed = QSE_NULL;
	qse_sed_iostd_t in[2];
	qse_sed_iostd_t out;
	int ret = -1;

	if (argc != 3)
	{
		qse_fprintf (QSE_STDERR, QSE_T("USAGE: %s command-string input-string\n"), argv[0]);
		return -1;
	}

	/* create the sed object */
	sed = qse_sed_openstd (0);
	if (sed == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open sed\n"));
		goto oops;
	}

	/* compile the command string */
	if (qse_sed_compstdstr (sed, argv[1]) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_sed_geterrmsg(sed));
		goto oops;
	}

	/* arrange to use the second argument as input data */
	in[0].type = QSE_SED_IOSTD_STR;
	in[0].u.str.ptr = argv[2];
	in[0].u.str.len = qse_strlen (argv[2]);
	in[1].type = QSE_SED_IOSTD_NULL;

	/* indicate that the output should be placed in a 
	 * dynamically allocated memory chunk */
	out.type = QSE_SED_IOSTD_STR;

	/* execute the compiled command */
	if (qse_sed_execstd (sed, in, &out) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_sed_geterrmsg(sed));
		goto oops;
	}

	/* access the output data produced */
	qse_printf (QSE_T("%.*s\n"), (int)out.u.str.len, out.u.str.ptr);

	/* free the output data */
	qse_sed_freemem (sed, out.u.str.ptr);	

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
