#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>

static int test_main (int argc, qse_char_t* argv[], qse_char_t* envp[])
{
	int i;

	for (i = 0; i < argc; i++)
	{
		qse_printf (QSE_T("ARG %d => [%s]\n"), i, argv[i]);
	}

	for (i = 0; envp[i]; i++)
	{
		qse_printf (QSE_T("ENV %d => [%s]\n"), i, envp[i]);
	}

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[], qse_achar_t* envp[])
{
	return qse_runmainwithenv (argc, argv, envp, test_main);
}

