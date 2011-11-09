#include <qse/cmn/fmt.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>

static int test_main (int argc, qse_char_t* argv[], qse_char_t* envp[])
{
	qse_char_t buf[129];	
	int bases[] = { 2, 8, 10, 16 };
	qse_char_t* prefix[] = { QSE_T("0b"), QSE_T("0"), QSE_NULL, QSE_T("0x") };
	int flags[] =
	{
		QSE_FMTINTMAX_PLUSSIGN,
		QSE_FMTINTMAX_FILLRIGHT | QSE_FMTINTMAX_PLUSSIGN,
		QSE_FMTINTMAX_FILLCENTER | QSE_FMTINTMAX_PLUSSIGN
	};
	int i, j, k;
	qse_intmax_t nums[] = { -12345, 12345, 0, QSE_TYPE_MAX(qse_intmax_t)  };

	/* note that nums[3] may not be printed properly with %d in printf(). 
	 * however, the formatting result with qse_fmtintmax() should be printed
	 * properly.
	 */


	for (k = 0; k < QSE_COUNTOF(nums) ; k++)
	{
		for (i = 0; i < QSE_COUNTOF(bases); i++)
		{
			for (j = 0; j < QSE_COUNTOF(flags); j++)
			{
				int n = qse_fmtintmax (buf, QSE_COUNTOF(buf), nums[k], bases[i] | flags[j] | QSE_FMTINTMAX_NOTRUNC, QSE_T('.'), prefix[i]);
				if (n <= -1)
				{
					qse_printf (QSE_T("%8d => [%4d:%04X] ERROR[%d]\n"), (int)nums[k], bases[i], flags[j],  n);
				}
				else
				{
					qse_printf (QSE_T("%8d => [%4d:%04X] [%s]\n"), (int)nums[k], bases[i], flags[j],  buf);
				}
			}
		}
		qse_printf (QSE_T("------------------------------\n"));
	}

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[], qse_achar_t* envp[])
{
     return qse_runmainwithenv (argc, argv, envp, test_main);
}

