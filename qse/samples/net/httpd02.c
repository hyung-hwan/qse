
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

static int process_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_htre_t* req, int peek)
{
	int method;
	qse_httpd_task_t* task;
	int content_received;
	xtn_t* xtn;

	method = qse_htre_getqmethodtype(req);
	content_received = (qse_htre_getcontentlen(req) > 0);

	xtn = (xtn_t*) qse_httpd_getxtn (httpd);

	if (peek) qse_perdechttpstr (qse_htre_getqpath(req), qse_htre_getqpath(req));

qse_printf (QSE_T("================================\n"));
qse_printf (QSE_T("[%lu] %hs REQUEST ==> [%hs] version[%d.%d %hs] method[%hs]\n"),
	(unsigned long)time(NULL),
	(peek? QSE_MT("PEEK"): QSE_MT("HANDLE")),
	qse_htre_getqpath(req),
	qse_htre_getmajorversion(req),
	qse_htre_getminorversion(req),
	qse_htre_getverstr(req),
	qse_htre_getqmethodname(req)
);
if (qse_htre_getqparam(req))
	qse_printf (QSE_T("PARAMS ==> [%hs]\n"), qse_htre_getqparam(req));

	if (peek)
	{
		if (method != QSE_HTTP_POST && method != QSE_HTTP_PUT)
		{
			/* i'll discard request contents if the method is none of
			 * post and put */
			qse_httpd_discardcontent (httpd, req);
		}

		if ((req->attr.flags & QSE_HTRE_ATTR_EXPECT100) &&
		    (req->version.major > 1 ||
		     (req->version.major == 1 && req->version.minor >= 1)) &&
		    !content_received)
		{
/* TODO: check method.... */
			/* "expect" in the header, version 1.1 or higher,
			 * and no content received yet */

			/* TODO: determine if to return 100-continue or other errors */
			if (qse_httpd_entaskcontinue (
				httpd, client, QSE_NULL, req) == QSE_NULL) return -1;
		}
	}

	if (method == QSE_HTTP_GET || method == QSE_HTTP_POST)
	{
		const qse_mchar_t* qpath = qse_htre_getqpath(req);
		const qse_mchar_t* dot = qse_mbsrchr (qpath, QSE_MT('.'));

		if (dot && qse_mbscmp (dot, QSE_MT(".cgi")) == 0)
		{
			if (peek)
			{
				/* cgi */
				if (method == QSE_HTTP_POST &&
				    !(req->attr.flags & QSE_HTRE_ATTR_LENGTH) &&
				    !(req->attr.flags & QSE_HTRE_ATTR_CHUNKED))
				{
					req->attr.flags &= ~QSE_HTRE_ATTR_KEEPALIVE;
					task = qse_httpd_entaskerror (
						httpd, client, QSE_NULL, 411, req);
					/* 411 can't keep alive */
					if (task) qse_httpd_entaskdisconnect (httpd, client, QSE_NULL);
				}
				else
				{
					task = qse_httpd_entaskcgi (
						httpd, client, QSE_NULL, qpath, req);
					if (task == QSE_NULL) goto oops;
				}
			}

			return 0;
		}
		else
		{
			if (peek)
			{
/* TODO: combine qpath with xtn->basedir */
				qse_httpd_discardcontent (httpd, req);
				task = qse_httpd_entaskpath (httpd, client, QSE_NULL, qpath, req);
				if (task == QSE_NULL) goto oops;
			}
		}
	}
	else
	{
		if (!peek)
		{
			task = qse_httpd_entaskerror (httpd, client, QSE_NULL, 405, req);
			if (task == QSE_NULL) goto oops;
		}
	}

	if (!(req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE))
	{
		if (!peek)
		{
			task = qse_httpd_entaskdisconnect (httpd, client, QSE_NULL);
			if (task == QSE_NULL) goto oops;
		}
	}

	return 0;

oops:
	/*qse_httpd_markbadclient (httpd, client);*/
	return -1;
}

static int peek_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	return process_request (httpd, client, req, 1);
}

static int handle_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	return process_request (httpd, client, req, 0);
}

/* --------------------------------------------------------------------- */

static qse_httpd_t* g_httpd = QSE_NULL;

static void sigint (int sig)
{
	if (g_httpd) qse_httpd_stop (g_httpd);
}

/* --------------------------------------------------------------------- */
static int httpd_main (int argc, qse_char_t* argv[])
{
	qse_httpd_t* httpd = QSE_NULL;
	int ret = -1, i;
	static qse_httpd_rcb_t rcb = { peek_request, handle_request };

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
		if (qse_httpd_addserver (httpd, argv[i]) <= -1)
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
	ret = qse_httpd_loopstd (httpd, &rcb, 10000);

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

