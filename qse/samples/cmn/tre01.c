
#include <qse/cmn/tre.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>

static int test_main (int argc, qse_char_t* argv[], qse_char_t* envp[])
{
	qse_tre_t tre;

	qse_tre_init (&tre, QSE_NULL);

	if (qse_tre_comp (&tre, argv[1], QSE_TRE_EXTENDED|QSE_TRE_NOSUBREG) <= -1)
	{
		qse_printf (QSE_T("Cannot compile pattern [%s] - %d\n"), argv[1], QSE_TRE_ERRNUM(&tre));
		goto oops;
	}	

	if (qse_tre_exec(&tre, argv[2], (size_t) 0, NULL, 0) <= -1)
	{
		if (QSE_TRE_ERRNUM(&tre) == QSE_TRE_ENOMATCH) qse_printf (QSE_T("no match\n"));
		else qse_printf (QSE_T("ERROR %d\n"), QSE_TRE_ERRNUM(&tre));
		goto oops;
	}
	else
	{
		qse_printf (QSE_T("match...\n"));
	}

	qse_tre_fini (&tre);
	return 0;

oops:
	qse_tre_fini (&tre);
	return -1;
}

int qse_main (int argc, qse_achar_t* argv[], qse_achar_t* envp[])
{
	return qse_runmainwithenv (argc, argv, envp, test_main);
}

