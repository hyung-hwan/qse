
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

#if defined(HAVE_SSL)
#	include <openssl/ssl.h>
#	include <openssl/err.h>
#	include <openssl/engine.h>
#endif

static qse_httpd_t* g_httpd = QSE_NULL;

static void sigint (int sig)
{
	if (g_httpd) qse_httpd_stop (g_httpd);
}

static int httpd_main (int argc, qse_char_t* argv[])
{
	qse_httpd_t* httpd = QSE_NULL;
	int ret = -1, i;

	if (argc <= 1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <listener_uri> ...\n"), argv[0]);
		goto oops;
	}

	httpd = qse_httpd_openstd (0);
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		goto oops;
	}

	for (i = 1; i < argc; i++)
	{
		qse_httpd_server_t* server;
		qse_httpd_server_xtn_t* server_xtn;

		server = qse_httpd_attachserverstd (httpd, argv[i], QSE_NULL, 0);
		if (server == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR,
				QSE_T("Failed to add httpd listener - %s\n"), argv[i]);
			goto oops;
		}

		server_xtn = qse_httpd_getserverxtn (httpd, server);
		server_xtn->cfg[QSE_HTTPD_SERVER_XTN_CFG_DIRCSS].ptr = QSE_MT("body { background-color:#d0e4fe; font-size: 0.9em; font-family: Ubuntu,'Trebuchet MS',sans-serif; }");
		server_xtn->cfg[QSE_HTTPD_SERVER_XTN_CFG_DIRCSS].len = qse_mbslen(server_xtn->cfg[QSE_HTTPD_SERVER_XTN_CFG_DIRCSS].ptr);
	}

	g_httpd = httpd;
	signal (SIGINT, sigint);
	signal (SIGPIPE, SIG_IGN);

	qse_httpd_setoption (httpd, QSE_HTTPD_CGIERRTONUL);
	ret = qse_httpd_loopstd (httpd, 10000);

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

