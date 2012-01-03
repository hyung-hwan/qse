#include <qse/cmn/fmt.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static int test_main (int argc, qse_char_t* argv[], qse_char_t* envp[])
{
	qse_char_t buf[19];	
	int bases[] = { 2, 8, 10, 16 };
	qse_char_t* prefix[] = { QSE_T("0b"), QSE_T("0"), QSE_NULL, QSE_T("0x") };
	int flags[] =
	{
		QSE_FMTINTMAX_NOTRUNC | 0,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_UPPERCASE,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_FILLRIGHT,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_PLUSSIGN,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_FILLRIGHT,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_FILLCENTER,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_FILLRIGHT | QSE_FMTINTMAX_PLUSSIGN,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_FILLCENTER | QSE_FMTINTMAX_PLUSSIGN,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL | 0,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL | QSE_FMTINTMAX_UPPERCASE,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL | QSE_FMTINTMAX_FILLRIGHT,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_PLUSSIGN,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_FILLRIGHT,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_FILLCENTER,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_FILLRIGHT | QSE_FMTINTMAX_PLUSSIGN,
		QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL | QSE_FMTINTMAX_UPPERCASE | QSE_FMTINTMAX_FILLCENTER | QSE_FMTINTMAX_PLUSSIGN
	};
	int i, j;
	int num = 0xF0F3;

	for (i = 0; i < QSE_COUNTOF(bases); i++)
	{
		for (j = 0; j < QSE_COUNTOF(flags); j++)
		{
			int n = qse_fmtuintmax (buf, QSE_COUNTOF(buf), num, bases[i] | flags[j], -1, QSE_T('.'), prefix[i]);
			if (n <= -1)
			{
				qse_printf (QSE_T("%8X => [%4d:%05X] ERROR[%d]\n"), num, bases[i], flags[j],  n);
			}
			else
			{
				qse_printf (QSE_T("%8X => [%4d:%05X] [%.*s][%d]\n"), num, bases[i], flags[j], n, buf, n);
			}
		}
	}

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[], qse_achar_t* envp[])
{
#if defined(_WIN32)
     char locale[100];
	UINT codepage = GetConsoleOutputCP();	
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgr (qse_utf8cmgr);
	}
	else
	{
     	sprintf (locale, ".%u", (unsigned int)codepage);
     	setlocale (LC_ALL, locale);
		qse_setdflcmgr (qse_slmbcmgr);
	}
#else
     setlocale (LC_ALL, "");
	qse_setdflcmgr (qse_slmbcmgr);
#endif
     return qse_runmainwithenv (argc, argv, envp, test_main);
}

