#include <qse/cmn/nwif.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static void print_nwifcfg (qse_nwifcfg_t* ptr)
{
	qse_char_t tmp[128];

	qse_printf (QSE_T("[%s] ifindex=[%u] "), ptr->name, ptr->index);
		
	qse_nwadtostr (&ptr->addr, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
	qse_printf (QSE_T("addr=[%s] "), tmp);
	qse_nwadtostr (&ptr->mask, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
	qse_printf (QSE_T("mask=[%s] "), tmp);

	if (ptr->flags & QSE_NWIFCFG_BCAST)
	{
		qse_nwadtostr (&ptr->bcast, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
		qse_printf (QSE_T("bcast=[%s] "), tmp);
	}

	qse_printf (QSE_T("mtu=[%d] "), (int)ptr->mtu);
	qse_printf (QSE_T("\n"));
}

static int test_main (int argc, qse_char_t* argv[])
{
	qse_nwifcfg_t cfg;
	int i;

	for (i = 1; ;i++)
	{
		if (qse_nwifindextostr (i, cfg.name, QSE_COUNTOF(cfg.name)) <= -1) 
		{
			qse_printf (QSE_T("ifindex %d failed for IN4\n"), i);
			break;
		}
		
		cfg.type = QSE_NWIFCFG_IN4;
		if (qse_getnwifcfg (&cfg) <= -1)
			qse_printf (QSE_T("Cannot get v4 configuration - %s\n"), cfg.name);
		else print_nwifcfg (&cfg);
	}

	for (i = 1; ;i++)
	{
		if (qse_nwifindextostr (i, cfg.name, QSE_COUNTOF(cfg.name)) <= -1) 
		{
			qse_printf (QSE_T("ifindex %d failed for IN6\n"), i);
			break;
		}

		cfg.type = QSE_NWIFCFG_IN6;
		if (qse_getnwifcfg (&cfg) <= -1)
			qse_printf (QSE_T("Cannot get v6 configuration - %s\n"), cfg.name);
		else print_nwifcfg (&cfg);
	}
	qse_printf (QSE_T("================================================\n"));
	return 0;
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
     return qse_runmain (argc, argv, test_main);
}

