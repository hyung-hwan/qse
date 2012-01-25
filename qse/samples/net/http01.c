
#include <qse/net/httpd.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>

#include <signal.h>
#include <locale.h>
#if	defined(_WIN32)
#	include <windows.h>
#endif

#define MAX_SENDFILE_SIZE 4096
typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
	const qse_httpd_cbs_t* orgcbs;
};

static qse_htb_walk_t walk (qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
{
qse_printf (QSE_T("HEADER OK %d[%hs] %d[%hs]\n"),  (int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)QSE_HTB_VLEN(pair), QSE_HTB_VPTR(pair));
	return QSE_HTB_WALK_FORWARD;
}

static int handle_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	int method;
	qse_httpd_task_t* x;

#if 0
	httpd_xtn_t* xtn = (httpd_xtn_t*) qse_httpd_getxtn (httpd);
	return xtn->orgcbs->handle_request (httpd, client, req);
#endif

qse_printf (QSE_T("================================\n"));
qse_printf (QSE_T("REQUEST ==> [%hs] version[%d.%d] method[%d]\n"), 
     qse_htre_getqpathptr(req),
     qse_htre_getmajorversion(req),
     qse_htre_getminorversion(req),
     qse_htre_getqmethod(req)
);
if (qse_htre_getqparamlen(req) > 0)
{
qse_printf (QSE_T("PARAMS ==> [%hs]\n"), qse_htre_getqparamptr(req));
}

qse_htb_walk (&req->hdrtab, walk, QSE_NULL);
if (QSE_MBS_LEN(&req->content) > 0)
{
qse_printf (QSE_T("content = [%.*S]\n"),
          (int)QSE_MBS_LEN(&req->content),
          QSE_MBS_PTR(&req->content));
}

	method = qse_htre_getqmethod (req);

	if (method == QSE_HTTP_GET || method == QSE_HTTP_POST)
	{
		const qse_mchar_t* rangestr;
		qse_http_range_t range;

		const qse_mchar_t* qpath = qse_htre_getqpathptr(req);
		const qse_mchar_t* dot = qse_mbsrchr (qpath, QSE_MT('.'));

		if (dot && qse_mbscmp (dot, QSE_MT(".cgi")) == 0)
		{
			/* cgi */
			x = qse_httpd_entaskcgi (
				httpd, client, QSE_NULL, qpath, req);
			if (x == QSE_NULL) goto oops;

#if 0
			x = qse_httpd_entasknphcgi (httpd, client, QSE_NULL, QSE_T("/tmp/test.cgi"), qse_htre_getversion(req));
#endif
		}
		else
		{
			rangestr = qse_htre_getheaderval (req, QSE_MT("Range"));
			if (rangestr && qse_parsehttprange (rangestr, &range) <= -1)
			{
#if 0
qse_httpd_entaskstatictext (httpd, client, QSE_NULL, QSE_MT("HTTP/1.1 416 Requested range not satisfiable\r\nContent-Length: 5\r\n\r\nA\r\n\r\n"));
#endif
				const qse_mchar_t* msg;
				msg = QSE_MT("<html><head><title>Requested range not satisfiable</title></head><body><b>REQUESTED RANGE NOT SATISFIABLE</b></body></html>");
				x = qse_httpd_entaskformat (
					httpd, client, QSE_NULL,
					QSE_MT("HTTP/%d.%d 416 Requested range not satisfiable\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"), 
					req->version.major, req->version.minor,
					(int)qse_mbslen(msg) + 4, msg
				);
				if (x == QSE_NULL) goto oops;
			}
			else
			{
				x = qse_httpd_entaskpath (
					httpd, client, QSE_NULL,
					qse_htre_getqpathptr(req),
					(rangestr? &range: QSE_NULL),
					qse_htre_getversion(req),
					req->attr.keepalive	
				);
				if (x == QSE_NULL) goto oops;
			}
		}
	}
	else
	{
		const qse_mchar_t* msg = QSE_MT("<html><head><title>Method not allowed</title></head><body><b>REQUEST METHOD NOT ALLOWED</b></body></html>");
		x = qse_httpd_entaskformat (
			httpd, client, QSE_NULL,
			QSE_MT("HTTP/%d.%d 405 Method not allowed\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"), 
			req->version.major, req->version.minor,
			(int)qse_mbslen(msg) + 4, msg
		);
		if (x == QSE_NULL) goto oops;
	}

done:
	if (!req->attr.keepalive)
	{
		x = qse_httpd_entaskdisconnect (httpd, client, QSE_NULL);
		if (x == QSE_NULL) goto oops;
	}

	return 0;

oops:
	/*qse_httpd_markclientbad (httpd, client);*/
	return 0;
}

static int handle_expect_continue (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	httpd_xtn_t* xtn = (httpd_xtn_t*) qse_httpd_getxtn (httpd);
	return xtn->orgcbs->handle_expect_continue (httpd, client, req);
}

const qse_mchar_t* get_mime_type (qse_httpd_t* httpd, const qse_mchar_t* path)
{
	if (qse_mbsend (path, QSE_MT(".html"))) return QSE_MT("text/html");
	if (qse_mbsend (path, QSE_MT(".txt"))) return QSE_MT("text/plain");
	if (qse_mbsend (path, QSE_MT(".jpg"))) return QSE_MT("image/jpeg");
	if (qse_mbsend (path, QSE_MT(".mp4"))) return QSE_MT("video/mp4");
	return QSE_NULL;
}

int list_directory (qse_httpd_t* httpd, const qse_mchar_t* path)
{
	return 404;
}

static qse_httpd_t* httpd = NULL;

static void sigint (int sig)
{
	qse_httpd_stop (httpd);
}

static qse_httpd_cbs_t httpd_cbs =
{
	handle_request,
	handle_expect_continue,
	get_mime_type,
	list_directory
};

int httpd_main (int argc, qse_char_t* argv[])
{
	int n;
	httpd_xtn_t* xtn;

	if (argc <= 1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <listener_uri> ...\n"), argv[0]);
		return -1;
	}

	httpd = qse_httpd_open (QSE_MMGR_GETDFL(), QSE_SIZEOF(httpd_xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		return -1;
	}

	xtn = (httpd_xtn_t*)qse_httpd_getxtn (httpd);
	xtn->orgcbs = qse_httpd_getcbs (httpd);

	for (n = 1; n < argc; n++)
	{
		if (qse_httpd_addlistener (httpd, argv[n]) <= -1)
		{
			qse_fprintf (QSE_STDERR, 	
				QSE_T("Failed to add httpd listener - %s\n"), argv[n]);
			qse_httpd_close (httpd);
			return -1;
		}
	}

	qse_httpd_setcbs (httpd, &httpd_cbs);

	signal (SIGINT, sigint);
	signal (SIGPIPE, SIG_IGN);

	n = qse_httpd_loop (httpd, 0);

	signal (SIGINT, SIG_DFL);
	signal (SIGPIPE, SIG_DFL);

	if (n <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Httpd error\n"));
	}

	qse_httpd_close (httpd);
	return n;
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
		qse_setdflcmgr (qse_slmbcmgr);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgr (qse_slmbcmgr);
#endif
	return qse_runmain (argc, argv, httpd_main);
}

