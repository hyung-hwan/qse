
#include <qse/http/std.h>
#include <qse/xli/std.h>
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

static void sighup (int sig)
{
	if (g_httpd) qse_httpd_reconfig (g_httpd);
}

static void setup_signal_handlers ()
{
	struct sigaction act;

#if defined(SIGINT)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = sigint;
	sigaction (SIGINT, &act, QSE_NULL);
#endif

#if defined(SIGHUP)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = sighup;
	sigaction (SIGHUP, &act, QSE_NULL);
#endif

#if defined(SIGPIPE)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = SIG_IGN;
	sigaction (SIGPIPE, &act, QSE_NULL);
#endif
}

static void restore_signal_handlers ()
{
	struct sigaction act;

#if defined(SIGINT)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = SIG_DFL;
	sigaction (SIGINT, &act, QSE_NULL);
#endif

#if defined(SIGHUP)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = SIG_DFL;
	sigaction (SIGHUP, &act, QSE_NULL);
#endif

#if defined(SIGPIPE)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = SIG_DFL;
	sigaction (SIGPIPE, &act, QSE_NULL);
#endif
}

/* --------------------------------------------------------------------- */

typedef struct server_xtn_t server_xtn_t;
struct server_xtn_t
{
	int tproxy;
	int nodir; /* no directory listing */

	qse_httpd_serverstd_makersrc_t orgmakersrc;
	qse_httpd_serverstd_freersrc_t orgfreersrc;
	qse_httpd_serverstd_query_t orgquery;

	qse_mchar_t* docroot;
	qse_mchar_t* realm;
	qse_mchar_t* auth;
	qse_mchar_t* dircss;
	qse_mchar_t* errcss;
	
	qse_httpd_serverstd_index_t index;
};

static int make_resource (
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
		if (server_xtn->orgmakersrc (httpd, client, req, rsrc) <= -1) return -1;
		if (server_xtn->nodir && rsrc->type == QSE_HTTPD_RSRC_DIR)
		{
			/* prohibit no directory listing */
			if (server_xtn->orgfreersrc)
				server_xtn->orgfreersrc (httpd, client, req, rsrc);
			rsrc->type = QSE_HTTPD_RSRC_ERR;
			rsrc->u.err.code = 403;
		}
		return 0;
	}
}

static void free_resource (
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
		if (server_xtn->orgfreersrc) 
			server_xtn->orgfreersrc (httpd, client, req, rsrc);
	}
}
/* --------------------------------------------------------------------- */
static void predetach_server (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverxtnstd (httpd, server);

	if (server_xtn->docroot) qse_httpd_freemem (httpd, server_xtn->docroot);
	if (server_xtn->realm) qse_httpd_freemem (httpd, server_xtn->realm);
	if (server_xtn->auth) qse_httpd_freemem (httpd, server_xtn->auth);
	if (server_xtn->dircss) qse_httpd_freemem (httpd, server_xtn->dircss);
	if (server_xtn->errcss) qse_httpd_freemem (httpd, server_xtn->errcss);
	if (server_xtn->index.files) qse_httpd_freemem (httpd, server_xtn->index.files);
}

static void reconfig_server (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	qse_printf (QSE_T("reconfiguring server.....\n"));
}

static int query_server (
	qse_httpd_t* httpd, qse_httpd_server_t* server, 
	qse_htre_t* req, const qse_mchar_t* xpath,
	qse_httpd_serverstd_query_code_t code, void* result)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverxtnstd (httpd, server);

	switch (code)
	{
		case QSE_HTTPD_SERVERSTD_DOCROOT:
			*(const qse_mchar_t**)result = server_xtn->docroot;
			return 0;

		case QSE_HTTPD_SERVERSTD_REALM:
			*(const qse_mchar_t**)result = server_xtn->realm;
			return 0;

		case QSE_HTTPD_SERVERSTD_AUTH:
			*(const qse_mchar_t**)result = server_xtn->auth;
			return 0;

		case QSE_HTTPD_SERVERSTD_DIRCSS:
			*(const qse_mchar_t**)result = server_xtn->dircss;
			return 0;

		case QSE_HTTPD_SERVERSTD_ERRCSS:
			*(const qse_mchar_t**)result = server_xtn->errcss;
			return 0;

		case QSE_HTTPD_SERVERSTD_INDEX:
			*(qse_httpd_serverstd_index_t*)result = server_xtn->index;
			return 0;


#if 0
		case QSE_HTTPD_SERVERSTD_CGI:
		case QSE_HTTPD_SERVERSTD_MIME:
#endif
	}

	return server_xtn->orgquery (httpd, server, req, xpath, code, result);
}

/* --------------------------------------------------------------------- */

static int load_server (qse_httpd_t* httpd, qse_xli_t* xli, qse_xli_list_t* list)
{
	qse_httpd_serverstd_t server; 
	qse_httpd_server_t* xserver;
	server_xtn_t* server_xtn;
	qse_xli_pair_t* pair;

	pair = qse_xli_findpairbyname (xli, list, QSE_T("bind"));
	if (pair == QSE_NULL)
	{
		/* TOOD: logging */
		qse_printf (QSE_T("WARNING: no bind specified for  ....\n"));
		return -1;
	}

	if (pair->val->type != QSE_XLI_STR)
	{
		/*  TOOD: logging */
		qse_printf (QSE_T("WARNING: non-string value for bind\n"));
		return -1;
	}

	qse_memset (&server, 0, QSE_SIZEOF(server));
	if (qse_strtonwad (((qse_xli_str_t*)pair->val)->ptr, &server.nwad) <= -1)
	{
		/*  TOOD: logging */
		qse_printf (QSE_T("WARNING: invalid value for bind - %s\n"), ((qse_xli_str_t*)pair->val)->ptr);
		return -1;
	}

	server.predetach = predetach_server;
	xserver = qse_httpd_attachserverstd (httpd, &server, QSE_SIZEOF(server_xtn_t));
	if (xserver == QSE_NULL) 
	{
		/* TODO: logging */
		qse_printf (QSE_T("WARNING: failed to attach server\n"));
		return -1;
	}

	server_xtn = qse_httpd_getserverxtnstd (httpd, xserver);

	qse_httpd_getserveroptstd (httpd, xserver, QSE_HTTPD_SERVERSTD_QUERY, &server_xtn->orgquery);
	qse_httpd_setserveroptstd (httpd, xserver, QSE_HTTPD_SERVERSTD_QUERY, query_server);

	qse_httpd_getserveroptstd (httpd, xserver, QSE_HTTPD_SERVERSTD_MAKERSRC, &server_xtn->orgmakersrc);
	qse_httpd_setserveroptstd (httpd, xserver, QSE_HTTPD_SERVERSTD_MAKERSRC, make_resource);

	qse_httpd_getserveroptstd (httpd, xserver, QSE_HTTPD_SERVERSTD_FREERSRC, &server_xtn->orgfreersrc);
	qse_httpd_setserveroptstd (httpd, xserver, QSE_HTTPD_SERVERSTD_FREERSRC, free_resource);

	/* --------------------------------------------------------------------- */
	pair = qse_xli_findpairbyname (xli, list, QSE_T("host['*'].location['/'].docroot"));
	if (!pair) pair = qse_xli_findpairbyname (xli, QSE_NULL, QSE_T("default.docroot"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		/* TODO: use a table */

		server_xtn->docroot = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
		if (server_xtn->docroot == QSE_NULL) 
		{
			qse_printf (QSE_T("WARNING: fail to copy docroot - %s\n"), ((qse_xli_str_t*)pair->val)->ptr);
			return -1;
		}
	}

	pair = qse_xli_findpairbyname (xli, list, QSE_T("host['*'].location['/'].realm"));
	if (!pair) pair = qse_xli_findpairbyname (xli, QSE_NULL, QSE_T("default.realm"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		/* TODO: use a table */

		server_xtn->realm = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
		if (server_xtn->realm == QSE_NULL) 
		{
			qse_printf (QSE_T("WARNING: fail to copy realm - %s\n"), ((qse_xli_str_t*)pair->val)->ptr);
			return -1;
		}
	}

	pair = qse_xli_findpairbyname (xli, list, QSE_T("host['*'].location['/'].auth"));
	if (!pair) pair = qse_xli_findpairbyname (xli, QSE_NULL, QSE_T("default.auth"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		/* TODO: use a table */
		server_xtn->auth = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
		if (server_xtn->auth == QSE_NULL) 
		{
			qse_printf (QSE_T("WARNING: fail to copy auth - %s\n"), ((qse_xli_str_t*)pair->val)->ptr);
			return -1;
		}

		if (qse_mbschr (server_xtn->auth, QSE_MT(':')) == QSE_NULL)
		{
			qse_printf (QSE_T("WARNING: no colon in the auth string - [%hs]\n"), server_xtn->auth);
		}
	}

	pair = qse_xli_findpairbyname (xli, list, QSE_T("host['*'].location['/'].css.dir"));
	if (!pair) pair = qse_xli_findpairbyname (xli, QSE_NULL, QSE_T("default.css.dir"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		/* TODO: use a table */
		server_xtn->dircss = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
		if (server_xtn->dircss == QSE_NULL) 
		{
			qse_printf (QSE_T("WARNING: fail to copy dircss - %s\n"), ((qse_xli_str_t*)pair->val)->ptr);
			return -1;
		}
	}

	pair = qse_xli_findpairbyname (xli, list, QSE_T("host['*'].location['/'].css.error"));
	if (!pair) pair = qse_xli_findpairbyname (xli, QSE_NULL, QSE_T("default.css.error"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		/* TODO: use a table */

		server_xtn->errcss = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
		if (server_xtn->errcss == QSE_NULL) 
		{
			qse_printf (QSE_T("WARNING: fail to copy dircss - %s\n"), ((qse_xli_str_t*)pair->val)->ptr);
			return -1;
		}
	}

	pair = qse_xli_findpairbyname (xli, list, QSE_T("host['*'].location['/'].index"));
	if (!pair) pair = qse_xli_findpairbyname (xli, QSE_NULL, QSE_T("default.index"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		const qse_char_t* tmpptr, * tmpend;
		qse_size_t count;

		tmpptr = ((qse_xli_str_t*)pair->val)->ptr;
		tmpend = tmpptr + ((qse_xli_str_t*)pair->val)->len;
	
		for (count = 0; tmpptr < tmpend; count++) tmpptr += qse_strlen (tmpptr) + 1;

		server_xtn->index.count = count;
		server_xtn->index.files = qse_httpd_strntombsdup (
			httpd, ((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len);
		if (server_xtn->index.files == QSE_NULL) 
		{
			qse_printf (QSE_T("WARNING: fail to copy index\n"));
			return -1;
		}
	}

	return 0;
}

static int load_config (qse_httpd_t* httpd, qse_xli_t* xli, const qse_char_t* file)
{
	qse_xli_iostd_t xli_in;
	qse_xli_pair_t* pair;
	int i;

	xli_in.type = QSE_XLI_IOSTD_FILE;
	xli_in.u.file.path = file;
	xli_in.u.file.cmgr = QSE_NULL;

	if (qse_xli_readstd (xli, &xli_in) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot load %s - %s\n"), xli_in.u.file.path, qse_xli_geterrmsg(xli));
		return - 1;
	}

	for (i = 0; ; i++)
	{
		qse_char_t buf[32];
		qse_sprintf (buf, QSE_COUNTOF(buf), QSE_T("server[%d]"), i);
		pair = qse_xli_findpairbyname (xli, QSE_NULL, buf);
		if (pair == QSE_NULL) break;

		if (pair->val->type != QSE_XLI_LIST)
		{
			qse_fprintf (QSE_STDERR, QSE_T("WARNING: non-list value for server\n"));
		}
		else
		{
			load_server (httpd, xli, (qse_xli_list_t*)pair->val);
		}
	}

	if (i == 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("No valid server specified in %s\n"), xli_in.u.file.path);
		return - 1;
	}

	return 0;
}

/* --------------------------------------------------------------------- */
static int httpd_main (int argc, qse_char_t* argv[])
{
	qse_httpd_t* httpd = QSE_NULL;
	qse_xli_t* xli = QSE_NULL;
	qse_ntime_t tmout;
	int ret = -1, i;
	int trait;

	if (argc != 2)
	{
		/* TODO: proper check... */
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s -f config-file\n"), argv[0]);
		goto oops;
	}

	httpd = qse_httpd_openstd (QSE_SIZEOF(server_xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		goto oops;
	}

	xli = qse_xli_openstd (0);
	if (xli == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open xli\n"));
		goto oops;
	}

	qse_xli_getopt (xli, QSE_XLI_TRAIT, &trait);
	trait |= QSE_XLI_KEYNAME;
	qse_xli_setopt (xli, QSE_XLI_TRAIT, &trait);
	if (load_config (httpd, xli, argv[1]) <= -1) goto oops;


	g_httpd = httpd;
	setup_signal_handlers ();

	qse_httpd_setname (httpd, QSE_MT("qsehttpd 1.0"));

	qse_httpd_getopt (httpd, QSE_HTTPD_TRAIT, &trait);
	trait |= QSE_HTTPD_CGIERRTONUL;
	qse_httpd_setopt (httpd, QSE_HTTPD_TRAIT, &trait);

	tmout.sec = 10;
	tmout.nsec = 0;
	ret = qse_httpd_loopstd (httpd, &tmout);

	restore_signal_handlers ();
	g_httpd = QSE_NULL;

	if (ret <= -1) qse_fprintf (QSE_STDERR, QSE_T("Httpd error - %d\n"), qse_httpd_geterrnum (httpd));

oops:
	if (xli) qse_xli_close (xli);
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

