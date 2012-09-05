#include <qse/cmn/ipad.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static int test_ipad4 (void)
{
	qse_ipad4_t ipad4;
	qse_char_t buf[32];
	qse_mchar_t mbsbuf[32];
	qse_wchar_t wcsbuf[32];
	qse_size_t i;

	static qse_char_t* ipstr[] =
	{
		QSE_T("192.168.1.1"),
		QSE_T("255.255.255.255"),
		QSE_T("4.3.0.0"),
		QSE_T("4.3.0.0X"),
		QSE_T("65.1234.11.34"),
		QSE_T("65.123.11.34"),
		QSE_T("1.1.1.1")
	};

	static qse_mchar_t* ipstr_mbs[] =
	{
		QSE_MT("192.168.1.1"),
		QSE_MT("255.255.255.255"),
		QSE_MT("4.3.0.0"),
		QSE_MT("4.3.0.0X"),
		QSE_MT("65.1234.11.34"),
		QSE_MT("65.123.11.34"),
		QSE_MT("1.1.1.1")
	};

	static qse_wchar_t* ipstr_wcs[] =
	{
		QSE_WT("192.168.1.1"),
		QSE_WT("255.255.255.255"),
		QSE_WT("4.3.0.0"),
		QSE_WT("4.3.0.0X"),
		QSE_WT("65.1234.11.34"),
		QSE_WT("65.123.11.34"),
		QSE_WT("1.1.1.1")
	};

	for (i = 0; i < QSE_COUNTOF(ipstr); i++)
	{
		if (qse_strtoipad4 (ipstr[i], &ipad4) <= -1)
		{
			qse_printf (QSE_T("Failed to convert %s\n"), ipstr[i]);
		}
		else
		{
			qse_ipad4tostr (&ipad4, buf, QSE_COUNTOF(buf));
			qse_printf (QSE_T("Converted [%s] to [%s]\n"), ipstr[i], buf);
		}
	}

	qse_printf (QSE_T("-------------------\n"));
	for (i = 0; i < QSE_COUNTOF(ipstr_mbs); i++)
	{
		if (qse_mbstoipad4 (ipstr_mbs[i], &ipad4) <= -1)
		{
			qse_printf (QSE_T("Failed to convert %hs\n"), ipstr_mbs[i]);
		}
		else
		{
			qse_ipad4tombs (&ipad4, mbsbuf, QSE_COUNTOF(mbsbuf));
			qse_printf (QSE_T("Converted [%hs] to [%hs]\n"), ipstr[i], mbsbuf);
		}
	}

	qse_printf (QSE_T("-------------------\n"));
	for (i = 0; i < QSE_COUNTOF(ipstr_wcs); i++)
	{
		if (qse_wcstoipad4 (ipstr_wcs[i], &ipad4) <= -1)
		{
			qse_printf (QSE_T("Failed to convert %ls\n"), ipstr_wcs[i]);
		}
		else
		{
			qse_ipad4towcs (&ipad4, wcsbuf, QSE_COUNTOF(wcsbuf));
			qse_printf (QSE_T("Converted [%ls] to [%ls]\n"), ipstr[i], wcsbuf);
		}
	}

	return 0;
}

static int test_ipad6 (void)
{
	qse_ipad6_t ipad6;
	qse_char_t buf[32];
	qse_mchar_t mbsbuf[32];
	qse_wchar_t wcsbuf[32];
	qse_size_t i;

	static qse_char_t* ipstr[] =
	{
		QSE_T("::"),
		QSE_T("::1"),
		QSE_T("fe80::f27b:cbff:fea3:f40c"),
		QSE_T("2001:0db8:85a3:0000:0000:8a2e:0370:7334"),
		QSE_T("2001:db8:1234:ffff:ffff:ffff:ffff:ffff"),
		QSE_T("::ffff:0:0"),
		QSE_T("::ffff:192.168.1.1")
	};

	static qse_mchar_t* ipstr_mbs[] =
	{
		QSE_MT("::"),
		QSE_MT("::1"),
		QSE_MT("fe80::f27b:cbff:fea3:f40c"),
		QSE_MT("2001:0db8:85a3:0000:0000:8a2e:0370:7334"),
		QSE_MT("2001:db8:1234:ffff:ffff:ffff:ffff:ffff"),
		QSE_MT("::ffff:0:0"),
		QSE_MT("::ffff:192.168.1.1")
	};

	static qse_wchar_t* ipstr_wcs[] =
	{
		QSE_WT("::"),
		QSE_WT("::1"),
		QSE_WT("fe80::f27b:cbff:fea3:f40c"),
		QSE_WT("2001:0db8:85a3:0000:0000:8a2e:0370:7334"),
		QSE_WT("2001:db8:1234:ffff:ffff:ffff:ffff:ffff"),
		QSE_WT("::ffff:0:0"),
		QSE_WT("::ffff:192.168.1.1")
	};

	for (i = 0; i < QSE_COUNTOF(ipstr); i++)
	{
		if (qse_strtoipad6 (ipstr[i], &ipad6) <= -1)
		{
			qse_printf (QSE_T("Failed to convert %s\n"), ipstr[i]);
		}
		else
		{
			qse_ipad6tostr (&ipad6, buf, QSE_COUNTOF(buf));
			qse_printf (QSE_T("Converted [%s] to [%s]\n"), ipstr[i], buf);
		}
	}

	qse_printf (QSE_T("-------------------\n"));
	for (i = 0; i < QSE_COUNTOF(ipstr_mbs); i++)
	{
		if (qse_mbstoipad6 (ipstr_mbs[i], &ipad6) <= -1)
		{
			qse_printf (QSE_T("Failed to convert %hs\n"), ipstr_mbs[i]);
		}
		else
		{
			qse_ipad6tombs (&ipad6, mbsbuf, QSE_COUNTOF(mbsbuf));
			qse_printf (QSE_T("Converted [%hs] to [%hs]\n"), ipstr_mbs[i], mbsbuf);
		}
	}

	qse_printf (QSE_T("-------------------\n"));
	for (i = 0; i < QSE_COUNTOF(ipstr_wcs); i++)
	{
		if (qse_wcstoipad6 (ipstr_wcs[i], &ipad6) <= -1)
		{
			qse_printf (QSE_T("Failed to convert %ls\n"), ipstr_wcs[i]);
		}
		else
		{
			qse_ipad6towcs (&ipad6, wcsbuf, QSE_COUNTOF(wcsbuf));
			qse_printf (QSE_T("Converted [%ls] to [%ls]\n"), ipstr_wcs[i], wcsbuf);
		}
	}

	return 0;
}

static int test_main (int argc, qse_char_t* argv[], qse_char_t* envp[])
{
	test_ipad4 ();
	qse_printf (QSE_T("==============\n"));
	test_ipad6 ();
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
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
     setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif
     return qse_runmainwithenv (argc, argv, envp, test_main);
}

