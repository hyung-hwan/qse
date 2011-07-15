
#include <qse/net/httpd.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <signal.h>

typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
	const qse_httpd_cbs_t* orgcbs;
};

static int handle_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	httpd_xtn_t* xtn = (httpd_xtn_t*) qse_httpd_getxtn (httpd);
	return xtn->orgcbs->handle_request (httpd, client, req);
}

static int handle_expect_continue (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	httpd_xtn_t* xtn = (httpd_xtn_t*) qse_httpd_getxtn (httpd);
	return xtn->orgcbs->handle_expect_continue (httpd, client, req);
}

static qse_httpd_t* httpd = NULL;

static void sigint (int sig)
{
	qse_httpd_stop (httpd);
}

static qse_httpd_cbs_t httpd_cbs =
{
	handle_request,
	handle_expect_continue
};

int httpd_main (int argc, qse_char_t* argv[])
{
	int n;
	httpd_xtn_t* xtn;

	if (argc != 2)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <listener_uri>\n"), argv[0]);
		return -1;
	}


	httpd = qse_httpd_open (QSE_NULL, QSE_SIZEOF(httpd_xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		return -1;
	}

	xtn = (httpd_xtn_t*)qse_httpd_getxtn (httpd);
	xtn->orgcbs = qse_httpd_getcbs (httpd);

	if (qse_httpd_addlisteners (httpd, argv[1]) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Failed to add httpd listeners\n"));
		qse_httpd_close (httpd);
		return -1;
	}

	qse_httpd_setcbs (httpd, &httpd_cbs);

	signal (SIGINT, sigint);
	signal (SIGPIPE, SIG_IGN);

	n = qse_httpd_loop (httpd);

	signal (SIGINT, SIG_DFL);
	signal (SIGPIPE, SIG_DFL);

	qse_httpd_close (httpd);
	return n;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, httpd_main);
}

