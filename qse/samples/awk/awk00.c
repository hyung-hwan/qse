/* awk00.c */

#include "awk00.h"
#include <qse/cmn/mbwc.h>
#include <qse/cmn/fmt.h>

#include <locale.h>
#if defined(_WIN32)
#    include <windows.h>
#endif

void init_awk_sample_locale (void)
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
		qse_fmtuintmaxtombs (locale, QSE_COUNTOF(locale),
			codepage, 10, -1, QSE_MT('\0'), QSE_MT("."));
		setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif
}
