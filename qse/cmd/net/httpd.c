
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
#	include <winsock2.h>
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


#if defined(HAVE_SSL)
#	include <openssl/ssl.h>
#	include <openssl/err.h>
#	include <openssl/engine.h>
#endif

/* --------------------------------------------------------------------- */

static qse_httpd_t* g_httpd = QSE_NULL;

static void sigint (int sig)
{
	if (g_httpd) qse_httpd_stop (g_httpd);
}

/* --------------------------------------------------------------------- */

typedef struct server_xtn_t server_xtn_t;
struct server_xtn_t
{
	int tproxy;
	int nodir; /* no directory listing */
	qse_httpd_server_cbstd_t* orgcbstd;
};

static int makersrc (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_htre_t* req, qse_httpd_rsrc_t* rsrc)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverxtnstd (httpd, client->server);

	if (server_xtn->tproxy)
	{
		if (qse_nwadequal(&client->orgdst_addr, &client->local_addr)) /* both equal and error */
		{
			/* TODO: implement a better check that the
			 *       destination is not one of the local addresses */

			rsrc->type = QSE_HTTPD_RSRC_ERR;
			rsrc->u.err.code = 500;
		}
		else
		{
			rsrc->type = QSE_HTTPD_RSRC_PROXY;
			rsrc->u.proxy.dst = client->orgdst_addr;
			rsrc->u.proxy.src = client->remote_addr;
	
			if (rsrc->u.proxy.src.type == QSE_NWAD_IN4)
				rsrc->u.proxy.src.u.in4.port = 0; /* reset the port to 0. */
			else if (rsrc->u.proxy.src.type == QSE_NWAD_IN6)
				rsrc->u.proxy.src.u.in6.port = 0; /* reset the port to 0. */
		}

		return 0;
	}
	else
	{
		if (server_xtn->orgcbstd->makersrc (httpd, client, req, rsrc) <= -1) return -1;
		if (server_xtn->nodir && rsrc->type == QSE_HTTPD_RSRC_DIR)
		{
			/* prohibit no directory listing */
			if (server_xtn->orgcbstd->freersrc)
				server_xtn->orgcbstd->freersrc (httpd, client, req, rsrc);
			rsrc->type = QSE_HTTPD_RSRC_ERR;
			rsrc->u.err.code = 403;
		}
		return 0;
	}
}

static void freersrc (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_htre_t* req, qse_httpd_rsrc_t* rsrc)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverxtnstd (httpd, client->server);

	if (server_xtn->tproxy)
	{
		/* nothing to do */
	}
	else
	{
		if (server_xtn->orgcbstd->freersrc) 
			server_xtn->orgcbstd->freersrc (httpd, client, req, rsrc);
	}
}

/* --------------------------------------------------------------------- */

static qse_httpd_server_t* attach_server (
	qse_httpd_t* httpd, qse_char_t* uri, qse_httpd_server_cbstd_t* cbstd)
{
	qse_httpd_server_t* server;
	server_xtn_t* server_xtn;
	int tproxy = 0;

	static qse_httpd_server_idxstd_t idxstd[] =
	{
		{ QSE_MT("index.cgi")  },
		{ QSE_MT("index.html") },
		{ QSE_NULL             }
	};

	if (qse_strzcasecmp (uri, QSE_T("http-tproxy://"), 14) == 0)
	{
		tproxy = 1;
		qse_strcpy (&uri[4], &uri[11]); 
	}

	server = qse_httpd_attachserverstd (
		httpd, uri, QSE_NULL, QSE_SIZEOF(server_xtn_t));
	if (server == QSE_NULL) return QSE_NULL;

	/* qse_httpd_getserverxtnstd() returns the pointer to 
	 * the extension space requested above, of the size
	 * QSE_SIZEOF(server_xtn_t) */
	server_xtn = qse_httpd_getserverxtnstd (httpd, server);
	server_xtn->tproxy = tproxy;

	/* qse_httpd_getserverxtn() returns the pointer to the
	 * extension space created by qse_httpd_attachserverstd()
	 * internally.
	 */
	/* remember the callback set in qse_httpd_attachserverstd() */
	qse_httpd_getserveroptstd (
		httpd, server, 
		QSE_HTTPD_SERVER_CBSTD, (void**)&server_xtn->orgcbstd);
	/* override it with a new callback for chaining */
	qse_httpd_setserveroptstd (
		httpd, server,
		QSE_HTTPD_SERVER_CBSTD, cbstd);

	/* totally override idxstd without remembering the old idxstd */
	qse_httpd_setserveroptstd (
		httpd, server,
		QSE_HTTPD_SERVER_IDXSTD, idxstd);

	qse_httpd_setserveroptstd (
		httpd, server, QSE_HTTPD_SERVER_DIRCSS, 
		QSE_MT("<style type='text/css'>body { background-color:#d0e4fe; font-size: 0.9em; } div.header { font-weight: bold; margin-bottom: 5px; } div.footer { border-top: 1px solid #99AABB; text-align: right; } table { font-size: 0.9em; } td { white-space: nowrap; } td.size { text-align: right; }</style>"));

	qse_httpd_setserveroptstd (
		httpd, server, QSE_HTTPD_SERVER_ERRCSS, 
		QSE_MT("<style type='text/css'>body { background-color:#d0e4fe; font-size: 0.9em; } div.header { font-weight: bold; margin-bottom: 5px; } div.footer { border-top: 1px solid #99AABB; text-align: right; }</style>"));
	
	return server;
}
/* --------------------------------------------------------------------- */
static int httpd_main (int argc, qse_char_t* argv[])
{
	qse_httpd_t* httpd = QSE_NULL;
	qse_ntime_t tmout;
	int ret = -1, i;
	int trait;
	static qse_httpd_server_cbstd_t cbstd = { makersrc, freersrc };

	if (argc <= 1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <listener_uri> ...\n"), argv[0]);
		goto oops;
	}

	httpd = qse_httpd_openstd (QSE_SIZEOF(server_xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		goto oops;
	}

	for (i = 1; i < argc; i++)
	{
		if (attach_server (httpd, argv[i], &cbstd) == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR,
				QSE_T("Failed to add httpd listener - %s\n"), argv[i]);
			goto oops;
		}
	}

	g_httpd = httpd;
	signal (SIGINT, sigint);
#if defined(SIGPIPE)
	signal (SIGPIPE, SIG_IGN);
#endif

	qse_httpd_setname (httpd, QSE_MT("qsehttpd 1.0"));

	trait = QSE_HTTPD_CGIERRTONUL;
	qse_httpd_setopt (httpd, QSE_HTTPD_TRAIT, &trait);

	tmout.sec = 10;
	tmout.nsec = 0;
	ret = qse_httpd_loopstd (httpd, &tmout);

	signal (SIGINT, SIG_DFL);
#if defined(SIGPIPE)
	signal (SIGPIPE, SIG_DFL);
#endif
	g_httpd = QSE_NULL;

	if (ret <= -1) qse_fprintf (QSE_STDERR, QSE_T("Httpd error\n"));

oops:
	if (httpd) qse_httpd_close (httpd);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int ret;

#if defined(_WIN32)
	char locale[100];
	UINT codepage;
	WSADATA wsadata;

	codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		sprintf (locale, ".%u", (unsigned int)codepage);
		setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}

	if (WSAStartup (MAKEWORD(2,0), &wsadata) != 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Failed to start up winsock\n"));
		return -1;
	}

#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

#if defined(HAVE_SSL)    
	SSL_load_error_strings ();
	SSL_library_init ();
#endif

	ret = qse_runmain (argc, argv, httpd_main);

#if defined(HAVE_SSL)
	/*ERR_remove_state ();*/
	ENGINE_cleanup ();
	ERR_free_strings ();
	EVP_cleanup ();
	CRYPTO_cleanup_all_ex_data ();
#endif

#if defined(_WIN32)
	WSACleanup ();	
#endif

	return ret;
}

