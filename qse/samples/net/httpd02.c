
#include <qse/net/httpd.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>

#include <signal.h>
#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	include <process.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include <unistd.h>
#	include <errno.h>
#endif


/* --------------------------------------------------------------------- */

typedef struct xtn_t xtn_t;
struct xtn_t
{
	qse_mchar_t basedir[4096];
};

static int makersrc (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req, qse_httpd_rsrc_t* rsrc)
{
	return -1;
}

static void freersrc (qse_httpd_t* httpd, qse_httpd_rsrc_t* rsrc)
{
}

/* --------------------------------------------------------------------- */

static qse_httpd_t* g_httpd = QSE_NULL;

static void sigint (int sig)
{
	if (g_httpd) qse_httpd_stop (g_httpd);
}

/* --------------------------------------------------------------------- */
static qse_httpd_server_t* attach_server (qse_httpd_t* httpd, const qse_char_t* uri)
{
	qse_httpd_server_t server, * xserver;
	const qse_char_t* docroot;
	server_xtn_t* server_xtn;
	qse_uri_t xuri;

	if (qse_ripuri (uri, &xuri, QSE_RIPURI_NOQUERY | QSE_RIP_URI_NOFRAGMENT) <= -1)
		return QSE_NULL;

/*	if (parse_server_uri (httpd, uri, &server, &docroot) <= -1) return QSE_NULL;*/
	server.predetach = predetach_server;

	xserver = qse_httpd_attachserver (
		httpd, &server, QSE_SIZEOF(*server_xtn) + xtnsize);
	if (xserver == QSE_NULL) return QSE_NULL;

	if (docroot[0] == QSE_T('/') && docroot[1] != QSE_T('\0'))
	{
		server_xtn = qse_httpd_getserverxtn (httpd, xserver);

#if defined(QSE_CHAR_IS_MCHAR)
		server_xtn->docroot.ptr = qse_mbsdup (docroot, httpd->mmgr);
#else
		server_xtn->docroot.ptr = qse_wcstombsdup (docroot, httpd->mmgr);
#endif
		if (server_xtn->docroot.ptr == QSE_NULL)
		{
			qse_httpd_detachserver (httpd, xserver);	
			httpd->errnum = QSE_HTTPD_ENOMEM;
			return QSE_NULL;
		}

		server_xtn->docroot.len = qse_mbslen(server_xtn->docroot.ptr);
	}

	return xserver;
}

/* --------------------------------------------------------------------- */
static int httpd_main (int argc, qse_char_t* argv[])
{
	qse_httpd_t* httpd = QSE_NULL;
	int ret = -1, i;
	static qse_httpd_cbstd_t cbstd = { makersrc, freersrc };

	if (argc <= 1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <listener_uri> ...\n"), argv[0]);
		goto oops;
	}

	httpd = qse_httpd_openstd (QSE_SIZEOF(xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		goto oops;
	}

	for (i = 1; i < argc; i++)
	{
		if (attach_server (httpd, argv[i]) == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR,
				QSE_T("Failed to add httpd listener - %s\n"), argv[i]);
			goto oops;
		}
	}

	g_httpd = httpd;
	signal (SIGINT, sigint);
	signal (SIGPIPE, SIG_IGN);

	qse_httpd_setname (httpd, QSE_MT("httpd02/qse 1.0"));

	qse_httpd_setoption (httpd, QSE_HTTPD_CGIERRTONUL);
	ret = qse_httpd_loopstd (httpd, &cbstd, 10000);

	signal (SIGINT, SIG_DFL);
	signal (SIGPIPE, SIG_DFL);
	g_httpd = QSE_NULL;

	if (ret <= -1) qse_fprintf (QSE_STDERR, QSE_T("Httpd error\n"));

oops:
	if (httpd) qse_httpd_close (httpd);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
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

	return qse_runmain (argc, argv, httpd_main);
}

