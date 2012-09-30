#include <qse/cmn/uri.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static void xxx (const qse_wchar_t* ptr, qse_size_t len)
{
	if (ptr)
	{
		qse_size_t i;
		qse_printf (QSE_T(" ["));
		for (i = 0; i < len; i++)
			qse_printf (QSE_T("%hc"), ptr[i]);
		qse_printf (QSE_T("] "));
	}
	else
	{
		qse_printf (QSE_T(" (null) "));
	}
}

static int test_main (int argc, qse_char_t* argv[], qse_char_t* envp[])
{
	static const qse_wchar_t* wcs[] =
	{
		QSE_WT("http://www.google.com"),
		QSE_WT("http://www.google.com:80"),
		QSE_WT("http://abc@www.google.com:80"),
		QSE_WT("http://abc:def@www.google.com:80"),
		QSE_WT("http://abc:def@www.google.com:80/"),
		QSE_WT("http://abc:def@www.google.com:80/?#"),
		QSE_WT("http://abc:def@www.google.com:80/abcdef/ghi?kkk=23#fragment"),
		QSE_WT("http://abc:def@www.google.com:80/abcdef/ghi#fragment#fragment"),
		QSE_WT("http://abc:def@www@.google.com:80/abcdef/ghi#fragment#fragment"),
		QSE_WT("http://@@@@@abc:def@www@.google.com:80/abcdef/ghi#fragment#fragment")
	};

	qse_size_t i;
	qse_uri_t uri;

	for (i = 0; i < QSE_COUNTOF(wcs); i++)
	{
		qse_printf (QSE_T("[%ls] => "), wcs[i]);
		if (qse_wcstouri (wcs[i], &uri, 0) <= -1)
		{
			qse_printf (QSE_T("[ERROR]"));
		}
		else
		{
			xxx (uri.scheme.ptr, uri.scheme.len);
			xxx (uri.auth.user.ptr, uri.auth.user.len);
			xxx (uri.auth.pass.ptr, uri.auth.pass.len);
			xxx (uri.host.ptr, uri.host.len);
			xxx (uri.port.ptr, uri.port.len);
			xxx (uri.path.ptr, uri.path.len);
			xxx (uri.query.ptr, uri.query.len);
			xxx (uri.frag.ptr, uri.frag.len);
		}
		qse_printf (QSE_T("\n"));
	}

	qse_printf (QSE_T("================================================\n"));

	for (i = 0; i < QSE_COUNTOF(wcs); i++)
	{
		qse_printf (QSE_T("[%ls] => "), wcs[i]);
		if (qse_wcstouri (wcs[i], &uri, QSE_WCSTOURI_NOQUERY | QSE_WCSTOURI_NOAUTH) <= -1)
		{
			qse_printf (QSE_T("[ERROR]"));
		}
		else
		{
			xxx (uri.scheme.ptr, uri.scheme.len);
			xxx (uri.auth.user.ptr, uri.auth.user.len);
			xxx (uri.auth.pass.ptr, uri.auth.pass.len);
			xxx (uri.host.ptr, uri.host.len);
			xxx (uri.port.ptr, uri.port.len);
			xxx (uri.path.ptr, uri.path.len);
			xxx (uri.query.ptr, uri.query.len);
			xxx (uri.frag.ptr, uri.frag.len);
		}
		qse_printf (QSE_T("\n"));
	}

	qse_printf (QSE_T("================================================\n"));

	for (i = 0; i < QSE_COUNTOF(wcs); i++)
	{
		qse_printf (QSE_T("[%ls] => "), wcs[i]);
		if (qse_wcstouri (wcs[i], &uri, QSE_WCSTOURI_NOQUERY | QSE_WCSTOURI_NOAUTH | QSE_WCSTOURI_NOFRAG) <= -1)
		{
			qse_printf (QSE_T("[ERROR]"));
		}
		else
		{
			xxx (uri.scheme.ptr, uri.scheme.len);
			xxx (uri.auth.user.ptr, uri.auth.user.len);
			xxx (uri.auth.pass.ptr, uri.auth.pass.len);
			xxx (uri.host.ptr, uri.host.len);
			xxx (uri.port.ptr, uri.port.len);
			xxx (uri.path.ptr, uri.path.len);
			xxx (uri.query.ptr, uri.query.len);
			xxx (uri.frag.ptr, uri.frag.len);
		}
		qse_printf (QSE_T("\n"));
	}

	qse_printf (QSE_T("================================================\n"));
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

