#include <qse/cmn/nwad.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static int test_main (int argc, qse_char_t* argv[], qse_char_t* envp[])
{
	qse_nwad_t nwad;
	qse_char_t buf[64];
	qse_mchar_t mbsbuf[64];
	qse_wchar_t wcsbuf[64];
	qse_size_t i;

	static qse_char_t* ipstr[] =
	{
		QSE_T("192.168.1.1"),
		QSE_T("255.255.255.255"),
		QSE_T("4.3.0.0"),
		QSE_T("4.3.0.0X"),
		QSE_T("65.1234.11.34"),
		QSE_T("65.123.11.34"),
		QSE_T("1.1.1.1"),
		QSE_T("::"),
		QSE_T("::1"),
		QSE_T("fe80::f27b:cbff:fea3:f40c"),
		QSE_T("2001:0db8:85a3:0000:0000:8a2e:0370:7334"),
		QSE_T("2001:db8:1234:ffff:ffff:ffff:ffff:ffff"),
		QSE_T("::ffff:0:0"),
		QSE_T("::ffff:192.168.1.1"),
		QSE_T("::ffff:192.168.1.1%88"),
		QSE_T("[::]:10"),
		QSE_T("[::1]:20"),
		QSE_T("[fe80::f27b:cbff:fea3:f40c]:30"),
		QSE_T("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:60"),
		QSE_T("[2001:db8:1234:ffff:ffff:ffff:ffff:ffff]:50"),
		QSE_T("[::ffff:0:0]:60"),
		QSE_T("[::ffff:192.168.1.1]:70"),
		QSE_T("[::ffff:192.168.1.1%999]:70")
	};

	static qse_mchar_t* ipstr_mbs[] =
	{
		QSE_MT("192.168.1.1"),
		QSE_MT("255.255.255.255"),
		QSE_MT("4.3.0.0"),
		QSE_MT("4.3.0.0X"),
		QSE_MT("65.1234.11.34"),
		QSE_MT("65.123.11.34"),
		QSE_MT("1.1.1.1"),
		QSE_MT("::"),
		QSE_MT("::1"),
		QSE_MT("fe80::f27b:cbff:fea3:f40c"),
		QSE_MT("2001:0db8:85a3:0000:0000:8a2e:0370:7334"),
		QSE_MT("2001:db8:1234:ffff:ffff:ffff:ffff:ffff"),
		QSE_MT("::ffff:0:0"),
		QSE_MT("::ffff:192.168.1.1"),
		QSE_MT("::ffff:192.168.1.1%88"),
		QSE_MT("[::]:10"),
		QSE_MT("[::1]:20"),
		QSE_MT("[fe80::f27b:cbff:fea3:f40c]:30"),
		QSE_MT("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:60"),
		QSE_MT("[2001:db8:1234:ffff:ffff:ffff:ffff:ffff]:50"),
		QSE_MT("[::ffff:0:0]:60"),
		QSE_MT("[::ffff:192.168.1.1]:70"),
		QSE_MT("[::ffff:192.168.1.1%999]:70")
	};

	static qse_wchar_t* ipstr_wcs[] =
	{
		QSE_WT("192.168.1.1"),
		QSE_WT("255.255.255.255"),
		QSE_WT("4.3.0.0"),
		QSE_WT("4.3.0.0X"),
		QSE_WT("65.1234.11.34"),
		QSE_WT("65.123.11.34"),
		QSE_WT("1.1.1.1"),
		QSE_WT("::"),
		QSE_WT("::1"),
		QSE_WT("fe80::f27b:cbff:fea3:f40c"),
		QSE_WT("2001:0db8:85a3:0000:0000:8a2e:0370:7334"),
		QSE_WT("2001:db8:1234:ffff:ffff:ffff:ffff:ffff"),
		QSE_WT("::ffff:0:0"),
		QSE_WT("::ffff:192.168.1.1"),
		QSE_WT("::ffff:192.168.1.1%88"),
		QSE_WT("[::]:10"),
		QSE_WT("[::1]:20"),
		QSE_WT("[fe80::f27b:cbff:fea3:f40c]:30"),
		QSE_WT("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:60"),
		QSE_WT("[2001:db8:1234:ffff:ffff:ffff:ffff:ffff]:50"),
		QSE_WT("[::ffff:0:0]:60"),
		QSE_WT("[::ffff:192.168.1.1]:70"),
		QSE_WT("[::ffff:192.168.1.1%999]:70")
	};

	for (i = 0; i < QSE_COUNTOF(ipstr); i++)
	{
		if (qse_strtonwad (ipstr[i], &nwad) <= -1)
		{
			qse_printf (QSE_T("Failed to convert %s\n"), ipstr[i]);
		}
		else
		{
			qse_nwadtostr (&nwad, buf, QSE_COUNTOF(buf));
			qse_printf (QSE_T("Converted <%s> to <%s>\n"), ipstr[i], buf);
		}
	}

	qse_printf (QSE_T("-------------------\n"));
	for (i = 0; i < QSE_COUNTOF(ipstr_mbs); i++)
	{
		if (qse_mbstonwad (ipstr_mbs[i], &nwad) <= -1)
		{
			qse_printf (QSE_T("Failed to convert %hs\n"), ipstr_mbs[i]);
		}
		else
		{
			qse_nwadtombs (&nwad, mbsbuf, QSE_COUNTOF(mbsbuf));
			qse_printf (QSE_T("Converted <%hs> to <%hs>\n"), ipstr_mbs[i], mbsbuf);
		}
	}

	qse_printf (QSE_T("-------------------\n"));
	for (i = 0; i < QSE_COUNTOF(ipstr_wcs); i++)
	{
		if (qse_wcstonwad (ipstr_wcs[i], &nwad) <= -1)
		{
			qse_printf (QSE_T("Failed to convert %ls\n"), ipstr_wcs[i]);
		}
		else
		{
			qse_nwadtowcs (&nwad, wcsbuf, QSE_COUNTOF(wcsbuf));
			qse_printf (QSE_T("Converted <%ls> to <%ls>\n"), ipstr_wcs[i], wcsbuf);
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

