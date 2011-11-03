#include <qse/cmn/fmt.h>
#include <qse/cmn/main.h>

static int test_main (int argc, qse_char_t* argv[], qse_char_t* envp[])
{
	qse_char_t buf[50];	
	int bases[] = { 2, 8, 10, 16 };
	int flags[] =
	{
		0,
		QSE_FMTULONG_UPPERCASE,
		QSE_FMTULONG_FILLRIGHT,
		QSE_FMTULONG_UPPERCASE | QSE_FMTULONG_FILLRIGHT
	};
	int i, j;
	int num = 0xF0F3;

	for (i = 0; i < QSE_COUNTOF(bases); i++)
	{
		for (j = 0; j < QSE_COUNTOF(flags); j++)
		{
			qse_fmtulong (buf, QSE_COUNTOF(buf), num, bases[i] | flags[j], QSE_T('.'));
			qse_printf (QSE_T("%8X => [%4d:%04X] [%s]\n"), num, bases[i], flags[j],  buf);
		}
	}

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[], qse_achar_t* envp[])
{
     return qse_runmainwithenv (argc, argv, envp, test_main);
}

