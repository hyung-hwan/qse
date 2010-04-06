#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>

static int test_main (int argc, qse_char_t* argv[])
{
	int i;

	for (i = 0; i < argc; i++)
	{
		qse_printf (QSE_T("%d => [%s]\n"), i, argv[i]);
	}

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, test_main);
}

