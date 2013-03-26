
#include <qse/http/stdhttpd.h>
#include <qse/xli/stdxli.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>
#include <qse/cmn/path.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/htb.h>

#include <signal.h>
#include <locale.h>
#include <stdio.h>

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
#	include <fcntl.h>
#endif

#if defined(HAVE_SSL)
#	include <openssl/ssl.h>
#	include <openssl/err.h>
#	include <openssl/engine.h>
#endif

#if defined(HAVE_SYS_PRCTL_H)
#	include <sys/prctl.h>
#endif

/* --------------------------------------------------------------------- */

static qse_httpd_t* g_httpd = QSE_NULL;
static const qse_char_t* g_cfgfile = QSE_NULL;
static int g_daemon = 0;
static int g_debug = 0;

/* --------------------------------------------------------------------- */

enum 
{
	SCFG_SSLCERTFILE,
	SCFG_SSLKEYFILE,
	SCFG_MAX
};

enum
{
	XCFG_NAME,
	XCFG_ROOT,
	XCFG_REALM,
	XCFG_AUTH,
	XCFG_DIRCSS,
	XCFG_ERRCSS,
	XCFG_MAX
};

struct cgi_t
{
	enum {
		CGI_PREFIX,
		CGI_SUFFIX,
		CGI_NAME,
		CGI_MAX
	} type;

	qse_mchar_t* spec;
	int nph;
	qse_mchar_t* shebang;

	struct cgi_t* next;
};

struct mime_t
{
	enum {
		MIME_PREFIX,
		MIME_SUFFIX,
		MIME_NAME,
		MIME_OTHER,
		MIME_MAX
	} type;

	qse_mchar_t* spec;
	qse_mchar_t* value;

	struct mime_t* next;
};

struct access_t
{
	/* TODO: support more types like ACCESS_GLOB 
	         not-only the base name, find a way to use query path or xpath */
	enum {
		ACCESS_PREFIX,
		ACCESS_SUFFIX,
		ACCESS_NAME,
		ACCESS_OTHER,
		ACCESS_MAX
	} type;

	qse_mchar_t* spec;
	int value;

	struct access_t* next;
};


typedef struct loccfg_t loccfg_t;
struct loccfg_t
{
	qse_mxstr_t locname;

	qse_mchar_t* xcfg[XCFG_MAX];
	int root_is_nwad;
	qse_nwad_t root_nwad;
	struct
	{
		qse_size_t count;
		qse_mchar_t* files;
	} index;

	struct
	{
		struct cgi_t* head;
		struct cgi_t* tail;
	} cgi[CGI_MAX];

	struct
	{
		struct mime_t* head;
		struct mime_t* tail;
	} mime[MIME_MAX];

	struct
	{
		struct access_t* head;
		struct access_t* tail;
	} access[2][ACCESS_MAX];

	loccfg_t* next;
};

typedef struct server_hostcfg_t server_hostcfg_t;
struct server_hostcfg_t
{
	qse_mchar_t* hostname;
	loccfg_t* loccfg;
	server_hostcfg_t* next;
};

typedef struct server_xtn_t server_xtn_t;
struct server_xtn_t
{
	int tproxy;
	int nodir; /* no directory listing */

	int num;
	qse_nwad_t bind;
	int secure;
	qse_mchar_t* scfg[SCFG_MAX];

	qse_httpd_serverstd_makersrc_t orgmakersrc;
	qse_httpd_serverstd_freersrc_t orgfreersrc;
	qse_httpd_serverstd_query_t orgquery;

	qse_htb_t* cfgtab;
};

/* --------------------------------------------------------------------- */

typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
	const  qse_char_t* cfgfile;
	qse_xli_t* xli;
	qse_httpd_impede_t orgimpede;
	int impede_code;

	loccfg_t dflcfg;
};

/* --------------------------------------------------------------------- */

static void sig_stop (int sig)
{
	if (g_httpd) qse_httpd_stop (g_httpd);
}

static void sig_reconf (int sig)
{
	if (g_httpd) 
	{
		httpd_xtn_t* httpd_xtn;
		httpd_xtn = qse_httpd_getxtnstd (g_httpd);
		httpd_xtn->impede_code = sig;
		qse_httpd_impede (g_httpd);
	}
}

static void setup_signal_handler (int signum, void(*handler)(int))
{
#if defined(HAVE_SIGACTION)
	struct sigaction act;
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = handler;
	sigaction (signum, &act, QSE_NULL);
#else
	signal (signum, handler);
#endif
}

static void setup_signal_handlers ()
{
#if defined(SIGINT)
	setup_signal_handler (SIGINT, sig_stop);
#endif
#if defined(SIGTERM)
	setup_signal_handler (SIGTERM, sig_stop);
#endif

#if defined(SIGHUP)
	setup_signal_handler (SIGHUP, sig_reconf);
#endif
#if defined(SIGUSR1)
	setup_signal_handler (SIGUSR1, sig_reconf);
#endif

#if defined(SIGPIPE)
	setup_signal_handler (SIGPIPE, SIG_IGN);
#endif
}

static void restore_signal_handlers ()
{
#if defined(SIGINT)
	setup_signal_handler (SIGINT, SIG_DFL);
#endif
#if defined(SIGTERM)
	setup_signal_handler (SIGTERM, SIG_DFL);
#endif

#if defined(SIGHUP)
	setup_signal_handler (SIGHUP, SIG_DFL);
#endif
#if defined(SIGUSR1)
	setup_signal_handler (SIGUSR1, SIG_DFL);
#endif

#if defined(SIGPIPE)
	setup_signal_handler (SIGPIPE, SIG_DFL);
#endif
}

static int daemonize (int devnull)
{

#if defined(HAVE_FORK)
	switch (fork())
	{
		case -1: return -1;
		case 0: break; /* child */
		default: _exit (0); /* parent */
	}

	if (setsid () <= -1) return -1;

	/*umask (0);*/
	chdir ("/");

	if (devnull)
	{
		/* redirect stdin/out/err to /dev/null */
		int fd = open ("/dev/null", O_RDWR);	
		if (fd >= 0)
		{
			dup2 (fd, 0);
			dup2 (fd, 1);
			dup2 (fd, 2);
			close (fd);
		}
	}

	return 0;

#else

	return -1;
#endif
}

/* --------------------------------------------------------------------- */


static int make_resource (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_htre_t* req, qse_httpd_rsrc_t* rsrc)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverstdxtn (httpd, client->server);

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

	server_xtn = qse_httpd_getserverstdxtn (httpd, client->server);

	if (server_xtn->tproxy)
	{
		/* nothing to do */
	}
	else
	{
		server_xtn->orgfreersrc (httpd, client, req, rsrc);
	}
}
/* --------------------------------------------------------------------- */

static loccfg_t* find_loccfg (
	qse_httpd_t* httpd, qse_htb_t* cfgtab, 
	const qse_mchar_t* host, qse_size_t hostlen, const qse_mchar_t* qpath)
{
	qse_htb_pair_t* pair;
	server_hostcfg_t* hostcfg;
	loccfg_t* loccfg;

	pair = qse_htb_search (cfgtab, host, hostlen);
	if (pair) 
	{
		hostcfg = (server_hostcfg_t*)QSE_HTB_VPTR(pair);
		
		/* the location names are inspected in the order as shown
		 * in the configuration. */
		for (loccfg = hostcfg->loccfg; loccfg; loccfg = loccfg->next)
		{
			QSE_ASSERT (loccfg->locname.len > 0);

			if (qse_mbsbeg (qpath, loccfg->locname.ptr) && 
			    (loccfg->locname.ptr[loccfg->locname.len - 1] == QSE_MT('/') ||
			     qpath[loccfg->locname.len] == QSE_MT('/') || 
			     qpath[loccfg->locname.len] == QSE_MT('\0'))) 
			{
				return loccfg;
			}
		}
	}

	return QSE_NULL;
}


static int query_server (
	qse_httpd_t* httpd, qse_httpd_server_t* server, 
	qse_htre_t* req, const qse_mchar_t* xpath,
	qse_httpd_serverstd_query_code_t code, void* result)
{
	httpd_xtn_t* httpd_xtn;
	server_xtn_t* server_xtn;
	loccfg_t* loccfg = QSE_NULL;

	httpd_xtn = qse_httpd_getxtnstd (httpd);
	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	if (code == QSE_HTTPD_SERVERSTD_SSL)
	{
		/* SSL is a server-wide configuration item. 
		 * handle this without inspecting the host and the query path. */
		qse_httpd_serverstd_ssl_t* ssl;
		ssl = (qse_httpd_serverstd_ssl_t*)result;
		ssl->certfile = server_xtn->scfg[SCFG_SSLCERTFILE];
		ssl->keyfile = server_xtn->scfg[SCFG_SSLKEYFILE];
		return 0;
	}

	if (req)
	{
		const qse_htre_hdrval_t* hosthdr;
		const qse_mchar_t* host;
		const qse_mchar_t* qpath;

		qpath = qse_htre_getqpath (req);

		hosthdr = qse_htre_getheaderval (req, QSE_MT("Host"));
		if (hosthdr)
		{
			const qse_mchar_t* colon;
			qse_size_t hostlen;

			/* take the last host value and search */
			while (hosthdr->next) hosthdr = hosthdr->next;
			host = hosthdr->ptr;

			/* remove :port-number if the host name contains it */
			colon = qse_mbsrchr(host, QSE_MT(':'));
			if (colon) hostlen = colon - host;
			else hostlen = qse_mbslen(host);

			loccfg = find_loccfg (httpd, server_xtn->cfgtab, host, hostlen, qpath);
		}
		if (loccfg == QSE_NULL) loccfg = find_loccfg (httpd, server_xtn->cfgtab, QSE_MT("*"), 1, qpath);
	}
	if (loccfg == QSE_NULL) loccfg = find_loccfg (httpd, server_xtn->cfgtab, QSE_MT("*"), 1, QSE_MT("/"));
	if (loccfg == QSE_NULL) loccfg = &httpd_xtn->dflcfg;

	switch (code)
	{

		case QSE_HTTPD_SERVERSTD_NAME:
			*(const qse_mchar_t**)result = loccfg->xcfg[XCFG_NAME];
			return 0;

		case QSE_HTTPD_SERVERSTD_ROOT:
			if (loccfg->root_is_nwad)
			{
				((qse_httpd_serverstd_root_t*)result)->type = QSE_HTTPD_SERVERSTD_ROOT_NWAD;
				((qse_httpd_serverstd_root_t*)result)->u.nwad = loccfg->root_nwad;
			}
			else
			{
				((qse_httpd_serverstd_root_t*)result)->type = QSE_HTTPD_SERVERSTD_ROOT_PATH;
				((qse_httpd_serverstd_root_t*)result)->u.path.val = loccfg->xcfg[XCFG_ROOT];
				((qse_httpd_serverstd_root_t*)result)->u.path.rpl = loccfg->locname.len;
			}
			return 0;

		case QSE_HTTPD_SERVERSTD_REALM:
			((qse_httpd_serverstd_realm_t*)result)->name = loccfg->xcfg[XCFG_REALM];
			((qse_httpd_serverstd_realm_t*)result)->authreq = (loccfg->xcfg[XCFG_REALM] != QSE_NULL);
			return 0;

		case QSE_HTTPD_SERVERSTD_AUTH:
		{
			qse_httpd_serverstd_auth_t* auth;

			auth = (qse_httpd_serverstd_auth_t*)result;
			auth->authok = 0;

			if (loccfg->xcfg[XCFG_AUTH])
			{
				if (qse_mbsxcmp (auth->key.ptr, auth->key.len, loccfg->xcfg[XCFG_AUTH]) == 0)
				{
					auth->authok = 1;
				}
			}
			return 0;
		}

		case QSE_HTTPD_SERVERSTD_DIRCSS:
			*(const qse_mchar_t**)result = loccfg->xcfg[XCFG_DIRCSS];
			return 0;

		case QSE_HTTPD_SERVERSTD_ERRCSS:
			*(const qse_mchar_t**)result = loccfg->xcfg[XCFG_ERRCSS];
			return 0;

		case QSE_HTTPD_SERVERSTD_INDEX:
			((qse_httpd_serverstd_index_t*)result)->count = loccfg->index.count;
			((qse_httpd_serverstd_index_t*)result)->files = loccfg->index.files;
			return 0;

		case QSE_HTTPD_SERVERSTD_CGI:
		{
			qse_size_t i;
			qse_httpd_serverstd_cgi_t* scgi;
			const qse_mchar_t* xpath_base;

			xpath_base = qse_mbsbasename (xpath);

			scgi = (qse_httpd_serverstd_cgi_t*)result;
			qse_memset (scgi, 0, QSE_SIZEOF(*scgi));

			for (i = 0; i < QSE_COUNTOF(loccfg->cgi); i++)
			{
				struct cgi_t* cgi;
				for (cgi = loccfg->cgi[i].head; cgi; cgi = cgi->next)
				{
					if ((cgi->type == CGI_PREFIX && qse_mbsbeg (xpath_base, cgi->spec)) ||
					    (cgi->type == CGI_SUFFIX && qse_mbsend (xpath_base, cgi->spec)) ||
					    (cgi->type == CGI_NAME && qse_mbscmp (xpath_base, cgi->spec) == 0))
					{
						scgi->cgi = 1;
						scgi->nph = cgi->nph;		
						scgi->shebang = cgi->shebang;
						return 0;
					}
				}
			}

			return 0;
		}

		case QSE_HTTPD_SERVERSTD_MIME:
		{
			qse_size_t i;
			const qse_mchar_t* xpath_base;

			xpath_base = qse_mbsbasename (xpath);

			*(const qse_mchar_t**)result = QSE_NULL;
			for (i = 0; i < QSE_COUNTOF(loccfg->mime); i++)
			{
				struct mime_t* mime;
				for (mime = loccfg->mime[i].head; mime; mime = mime->next)
				{
					if ((mime->type == MIME_PREFIX && qse_mbsbeg (xpath_base, mime->spec)) ||
					    (mime->type == MIME_SUFFIX && qse_mbsend (xpath_base, mime->spec)) ||
					    (mime->type == MIME_NAME && qse_mbscmp (xpath_base, mime->spec) == 0) ||
					    mime->type == MIME_OTHER)
					{
						*(const qse_mchar_t**)result = mime->value;
						return 0;
					}
				}
			}
			return 0;
		}

		case QSE_HTTPD_SERVERSTD_DIRACC:
		case QSE_HTTPD_SERVERSTD_FILEACC:
		{
			qse_size_t i;
			const qse_mchar_t* xpath_base;
			int id;

			id = (code == QSE_HTTPD_SERVERSTD_DIRACC)? 0: 1;

			xpath_base = qse_mbsbasename (xpath);

			*(int*)result = 200;
			for (i = 0; i < QSE_COUNTOF(loccfg->access[id]); i++)
			{
				struct access_t* access;
				for (access = loccfg->access[id][i].head; access; access = access->next)
				{
					if ((access->type == ACCESS_PREFIX && qse_mbsbeg (xpath_base, access->spec)) ||
					    (access->type == ACCESS_SUFFIX && qse_mbsend (xpath_base, access->spec)) ||
					    (access->type == ACCESS_NAME && qse_mbscmp (xpath_base, access->spec) == 0) ||
					    access->type == ACCESS_OTHER)
					{
						*(int*)result = access->value;
						return 0;
					}
				}
			}
			return 0;
		}
	}

	return server_xtn->orgquery (httpd, server, req, xpath, code, result);
}

/* --------------------------------------------------------------------- */

static struct
{
	const qse_char_t* x;
	const qse_char_t* y;
} scfg_items[] =
{
	{ QSE_T("ssl-cert-file"),  QSE_T("default.ssl-cert-file") },
	{ QSE_T("ssl-key-file"),   QSE_T("default.ssl-key-file") }
};

static struct 
{
	const qse_char_t* x;
	const qse_char_t* y;
} loc_xcfg_items[] =
{
	{ QSE_T("name"),      QSE_T("default.name") },
	{ QSE_T("root"),      QSE_T("default.root") },
	{ QSE_T("realm"),     QSE_T("default.realm") },
	{ QSE_T("auth"),      QSE_T("default.auth") },
	{ QSE_T("dir-css"),   QSE_T("default.dir-css") },
	{ QSE_T("error-css"), QSE_T("default.error-css") }
};

static struct 
{
	const qse_char_t* x;
	const qse_char_t* y;
} loc_acc_items[] =
{
	{ QSE_T("dir-access"), QSE_T("default.dir-access") },
	{ QSE_T("file-access"), QSE_T("default.file-access") }
};

static void free_loccfg_contents (qse_httpd_t* httpd, loccfg_t* loccfg)
{
	qse_size_t i, j;

	for (i = 0; i < QSE_COUNTOF(loccfg->xcfg); i++)
	{
		if (loccfg->xcfg[i]) 
		{
			qse_httpd_freemem (httpd, loccfg->xcfg[i]);
			loccfg->xcfg[i] = QSE_NULL;
		}
	}

	if (loccfg->index.files) 
	{
		qse_httpd_freemem (httpd, loccfg->index.files);
		loccfg->index.files = QSE_NULL;
		loccfg->index.count = 0;
	}

	for (i = 0; i < QSE_COUNTOF(loccfg->cgi); i++)
	{
		struct cgi_t* cgi = loccfg->cgi[i].head;
		while (cgi)
		{
			struct cgi_t* x = cgi;
			cgi = x->next;

			if (x->shebang) qse_httpd_freemem (httpd, x->shebang);
			if (x->spec) qse_httpd_freemem (httpd, x->spec);
			if (x) qse_httpd_freemem (httpd, x);
		}

		loccfg->cgi[i].head = QSE_NULL;
		loccfg->cgi[i].tail = QSE_NULL;
	}

	for (i = 0; i < QSE_COUNTOF(loccfg->mime); i++)
	{
		struct mime_t* mime = loccfg->mime[i].head;
		while (mime)
		{
			struct mime_t* x = mime;
			mime = x->next;

			if (x->spec) qse_httpd_freemem (httpd, x->spec);
			if (x->value) qse_httpd_freemem (httpd, x->value);
			if (x) qse_httpd_freemem (httpd, x);
		}

		loccfg->mime[i].head = QSE_NULL;
		loccfg->mime[i].tail = QSE_NULL;
	}

	for (j = 0; j < QSE_COUNTOF(loccfg->access); j++)
	{
		for (i = 0; i < QSE_COUNTOF(loccfg->access[j]); i++)
		{
			struct access_t* access = loccfg->access[j][i].head;
			while (access)
			{
				struct access_t* x = access;
				access = x->next;

				if (x->spec) qse_httpd_freemem (httpd, x->spec);
				if (x) qse_httpd_freemem (httpd, x);
			}

			loccfg->access[j][i].head = QSE_NULL;
			loccfg->access[j][i].tail = QSE_NULL;
		}
	}

	if (loccfg->locname.ptr) qse_httpd_freemem (httpd, loccfg->locname.ptr);
}

static int load_loccfg (qse_httpd_t* httpd, qse_xli_list_t* list, loccfg_t* cfg)
{
	qse_size_t i;
	qse_xli_pair_t* pair;
	qse_xli_atom_t* atom;
	httpd_xtn_t* httpd_xtn;

	httpd_xtn = qse_httpd_getxtnstd (httpd);

	for (i = 0; i < QSE_COUNTOF(loc_xcfg_items); i++)
	{
		pair = qse_xli_findpairbyname (httpd_xtn->xli, list, loc_xcfg_items[i].x);
		if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, loc_xcfg_items[i].y);
		if (pair && pair->val->type == QSE_XLI_STR)
		{
			cfg->xcfg[i] = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
			if (cfg->xcfg[i] == QSE_NULL) 
			{
				/*qse_printf (QSE_T("ERROR in copying - %s\n"), qse_httpd_geterrmsg (httpd));*/
				qse_printf (QSE_T("ERROR in copying\n"));
				return -1;
			}
		}
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("index"));
	if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.index"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		const qse_char_t* tmpptr, * tmpend;
		qse_size_t count;

		tmpptr = ((qse_xli_str_t*)pair->val)->ptr;
		tmpend = tmpptr + ((qse_xli_str_t*)pair->val)->len;
	
		for (count = 0; tmpptr < tmpend; count++) tmpptr += qse_strlen (tmpptr) + 1;

		cfg->index.count = count;
		cfg->index.files = qse_httpd_strntombsdup (
			httpd, ((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len);
		if (cfg->index.files == QSE_NULL) 
		{
			qse_printf (QSE_T("ERROR: in copying index\n"));
			return -1;
		}
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("cgi"));
	if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.cgi"));
	if (pair && pair->val->type == QSE_XLI_LIST)
	{
		/* TODO: more sanity check... this can be done with xli schema... if supported */
		qse_xli_list_t* cgilist = (qse_xli_list_t*)pair->val;
		for (atom = cgilist->head; atom; atom = atom->next)
		{
			if (atom->type != QSE_XLI_PAIR) continue;

			pair = (qse_xli_pair_t*)atom;
			if (pair->key && pair->name && 
			    (pair->val->type == QSE_XLI_NIL || pair->val->type == QSE_XLI_STR))
			{
				struct cgi_t* cgi;
				int type;

				if (qse_strcmp (pair->key, QSE_T("prefix")) == 0) type = CGI_PREFIX;
				else if (qse_strcmp (pair->key, QSE_T("suffix")) == 0) type = CGI_SUFFIX;
				else if (qse_strcmp (pair->key, QSE_T("name")) == 0) type = CGI_NAME;
				else continue;

				cgi = qse_httpd_callocmem (httpd, QSE_SIZEOF(*cgi));
				if (cgi == QSE_NULL)
				{
					qse_printf (QSE_T("ERROR: memory failure in copying cgi\n"));
					return -1;
				}

				cgi->type = type;
				cgi->spec = qse_httpd_strtombsdup (httpd, pair->name);
				if (!cgi->spec)
				{
					qse_httpd_freemem (httpd, cgi);
					qse_printf (QSE_T("ERROR: memory failure in copying cgi\n"));
					return -1;
				}
				if (pair->val->type == QSE_XLI_STR) 
				{
					const qse_char_t* tmpptr, * tmpend;
					qse_size_t count;

					tmpptr = ((qse_xli_str_t*)pair->val)->ptr;
					tmpend = tmpptr + ((qse_xli_str_t*)pair->val)->len;
	
					for (count = 0; tmpptr < tmpend; count++) 
					{
						if (count == 0)
						{
							if (qse_strcmp (tmpptr, QSE_T("nph")) == 0) cgi->nph = 1;
						}
						else if (count == 1)
						{
							cgi->shebang = qse_httpd_strtombsdup (httpd, tmpptr);
							if (!cgi->shebang)
							{
								qse_httpd_freemem (httpd, cgi->spec);
								qse_httpd_freemem (httpd, cgi);
								qse_printf (QSE_T("ERROR: memory failure in copying cgi\n"));
								return -1;
							}
						}

						tmpptr += qse_strlen (tmpptr) + 1;

						/* TODO: more sanity check */
					}

				}
				if (cfg->cgi[type].tail)
					cfg->cgi[type].tail->next = cgi;
				else
					cfg->cgi[type].head = cgi;
				cfg->cgi[type].tail = cgi;
			}
		}	
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("mime"));
	if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.mime"));
	if (pair && pair->val->type == QSE_XLI_LIST)
	{
		qse_xli_list_t* mimelist = (qse_xli_list_t*)pair->val;
		for (atom = mimelist->head; atom; atom = atom->next)
		{
			if (atom->type != QSE_XLI_PAIR) continue;

			pair = (qse_xli_pair_t*)atom;
			if (pair->key && pair->val->type == QSE_XLI_STR)
			{
				struct mime_t* mime;
				int type;

				if (qse_strcmp (pair->key, QSE_T("prefix")) == 0 && pair->name) type = MIME_PREFIX;
				else if (qse_strcmp (pair->key, QSE_T("suffix")) == 0 && pair->name) type = MIME_SUFFIX;
				else if (qse_strcmp (pair->key, QSE_T("name")) == 0 && pair->name) type = MIME_NAME;
				else if (qse_strcmp (pair->key, QSE_T("other")) == 0 && !pair->name) type = MIME_OTHER;
				else continue;

				mime = qse_httpd_callocmem (httpd, QSE_SIZEOF(*mime));
				if (mime == QSE_NULL)
				{
					qse_printf (QSE_T("ERROR: memory failure in copying mime\n"));
					return -1;
				}

				mime->type = type;
				mime->spec = qse_httpd_strtombsdup (httpd, pair->name);
				if (!mime->spec)
				{
					qse_httpd_freemem (httpd, mime);
					qse_printf (QSE_T("ERROR: memory failure in copying mime\n"));
					return -1;
				}

				mime->value = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
				if (!mime->value)
				{
					qse_httpd_freemem (httpd, mime->spec);
					qse_httpd_freemem (httpd, mime);
					qse_printf (QSE_T("ERROR: memory failure in copying mime\n"));
					return -1;
				}

				if (cfg->mime[type].tail)
					cfg->mime[type].tail->next = mime;
				else
					cfg->mime[type].head = mime;
				cfg->mime[type].tail = mime;
			}
		}	
	}

	for (i = 0; i < 2;  i++)
	{

		pair = qse_xli_findpairbyname (httpd_xtn->xli, list, loc_acc_items[i].x);
		if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, loc_acc_items[i].y);
		if (pair && pair->val->type == QSE_XLI_LIST)
		{
			qse_xli_list_t* acclist = (qse_xli_list_t*)pair->val;
			for (atom = acclist->head; atom; atom = atom->next)
			{
				if (atom->type != QSE_XLI_PAIR) continue;
	
				pair = (qse_xli_pair_t*)atom;
				if (pair->key && pair->val->type == QSE_XLI_STR)
				{
					struct access_t* acc;
					const qse_char_t* tmp;
					int type, value;
	
					if (qse_strcmp (pair->key, QSE_T("prefix")) == 0 && pair->name) type = ACCESS_PREFIX;
					else if (qse_strcmp (pair->key, QSE_T("suffix")) == 0 && pair->name) type = ACCESS_SUFFIX;
					else if (qse_strcmp (pair->key, QSE_T("name")) == 0 && pair->name) type = ACCESS_NAME;
					else if (qse_strcmp (pair->key, QSE_T("other")) == 0 && !pair->name) type = ACCESS_OTHER;
					else continue;
	
					tmp = ((qse_xli_str_t*)pair->val)->ptr;
					if (qse_strcmp (tmp, QSE_T("noent")) == 0) value = 404;
					else if (qse_strcmp (tmp, QSE_T("forbid")) == 0) value = 403;
					else if (qse_strcmp (tmp, QSE_T("ok")) == 0) value = 200;
					else continue;
					/* TODO: more sanity check */
	
					acc = qse_httpd_callocmem (httpd, QSE_SIZEOF(*acc));
					if (acc == QSE_NULL)
					{
						qse_printf (QSE_T("ERROR: memory failure in copying acc\n"));
						return -1;
					}
	
					acc->type = type;
					if (pair->name)
					{
						acc->spec = qse_httpd_strtombsdup (httpd, pair->name);
						if (!acc->spec)
						{
							qse_httpd_freemem (httpd, acc);
							qse_printf (QSE_T("ERROR: memory failure in copying access\n"));
							return -1;
						}
					}
					acc->value = value;
	
					if (cfg->access[i][type].tail)
						cfg->access[i][type].tail->next = acc;
					else
						cfg->access[i][type].head = acc;
					cfg->access[i][type].tail = acc;
				}
			}	
		}
	}

	/* TODO: support multiple auth entries here and above */

#if 0
	/* TODO: perform more sanity check */
	if (qse_mbschr (cfg->xcfg[XCFG_AUTH], QSE_MT(':')) == QSE_NULL)
	{
		qse_printf (QSE_T("WARNING: no colon in the auth string - [%hs]\n"), cfg->xcfg[XCFG_AUTH]);
	}
#endif

	if (cfg->xcfg[XCFG_ROOT] && qse_mbstonwad (cfg->xcfg[XCFG_ROOT], &cfg->root_nwad) >= 0) 
	{
		cfg->root_is_nwad = 1;
	}

	return 0;
}

static void free_server_hostcfg (qse_httpd_t* httpd, server_hostcfg_t* hostcfg)
{
	loccfg_t* lc, * cur;

	lc = hostcfg->loccfg;
	while (lc)
	{
		cur = lc;
		lc = lc->next;	

		free_loccfg_contents (httpd, cur);
		qse_httpd_freemem (httpd, cur);
	}

	if (hostcfg->hostname) qse_httpd_freemem (httpd, hostcfg->hostname);
	qse_httpd_freemem (httpd, hostcfg);
}

static void free_cfgtab_value (qse_htb_t* htb, void* vptr, qse_size_t vlen)
{
	qse_httpd_t* httpd;
	httpd = *(qse_httpd_t**) qse_htb_getxtn(htb);
	free_server_hostcfg (httpd, (server_hostcfg_t*)vptr);
}

static int load_server_config (qse_httpd_t* httpd, qse_httpd_server_t* server, qse_xli_list_t* list)
{
	qse_size_t i, j, host_count, loc_count;
	httpd_xtn_t* httpd_xtn;
	server_xtn_t* server_xtn;
	server_hostcfg_t* hostcfg;
	loccfg_t* loccfg;

	static qse_htb_style_t cfgtab_style =
	{
		{
			QSE_HTB_COPIER_DEFAULT,
			QSE_HTB_COPIER_DEFAULT 
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			free_cfgtab_value
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	};

	httpd_xtn = qse_httpd_getxtnstd (httpd);
	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	/* load the server-wide configuration not specific to host/location */
	for (i = 0; i < SCFG_MAX; i++)
	{
		qse_xli_pair_t* pair;

		pair = qse_xli_findpairbyname (httpd_xtn->xli, list, scfg_items[i].x);
		if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, scfg_items[i].y);
		if (pair && pair->val->type == QSE_XLI_STR)
		{
			server_xtn->scfg[i] = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
			if (server_xtn->scfg[i] == QSE_NULL) 
			{
				/*qse_printf (QSE_T("ERROR in copying - %s\n"), qse_httpd_geterrmsg (httpd));*/
				qse_printf (QSE_T("ERROR in copying\n"));
				return -1;
			}
		}
	}

	/* load host/location specific configuration */
	host_count = qse_xli_getnumpairsbyname (httpd_xtn->xli, list, QSE_T("host"));
	if (host_count <= 0) return 0; /* nothing to load */

	QSE_ASSERT (server_xtn->cfgtab == QSE_NULL);
	server_xtn->cfgtab = qse_htb_open (
		qse_httpd_getmmgr (httpd), QSE_SIZEOF(httpd), 
		host_count + (host_count / 3) + 1, 70, QSE_SIZEOF(qse_mchar_t), 0);
	if (server_xtn->cfgtab == QSE_NULL)
	{
		/* TOOD: print error */
		return -1;
	}

	/* store the back reference into the configuration table */
	*(qse_httpd_t**)qse_htb_getxtn (server_xtn->cfgtab) = httpd;
	qse_htb_setstyle (server_xtn->cfgtab, &cfgtab_style);

	for (i = 0; i < host_count; i++)
	{
		qse_xli_pair_t* host;
		qse_char_t buf[32];

		qse_sprintf (buf, QSE_COUNTOF(buf), QSE_T("host[%d]"), i);
		host = qse_xli_findpairbyname (httpd_xtn->xli, list, buf);
		if (!host) break;

		if (host->val->type == QSE_XLI_LIST && host->name) 
		{
			loc_count = qse_xli_getnumpairsbyname (httpd_xtn->xli, (qse_xli_list_t*)host->val, QSE_T("location"));

			if (((hostcfg = qse_httpd_callocmem (httpd, QSE_SIZEOF(*hostcfg))) == QSE_NULL) ||
			    ((hostcfg->hostname = qse_httpd_strtombsdup (httpd, (host->name[0] == QSE_T('\0')? QSE_T("*"):host->name))) == QSE_NULL)) goto oops;
			
			for (j = loc_count; j > 0; )
			{
				qse_xli_pair_t* loc;

				j--;

				qse_sprintf (buf, QSE_COUNTOF(buf), QSE_T("location[%d]"), j);
				loc = qse_xli_findpairbyname (httpd_xtn->xli, (qse_xli_list_t*)host->val, buf);
				if (!loc) break;

				if (loc->val->type == QSE_XLI_LIST && loc->name) 
				{
					loccfg = qse_httpd_callocmem (httpd, QSE_SIZEOF(*loccfg));
					if (loccfg == QSE_NULL) goto oops;

					/* just chain it before loading the actual data */
					loccfg->next = hostcfg->loccfg;
					hostcfg->loccfg = loccfg;

					/* load the data now */
					if (load_loccfg (httpd, (qse_xli_list_t*)loc->val, loccfg) <= -1) goto oops;

					/* clone the location name  */
					loccfg->locname.ptr = qse_httpd_strtombsdup (httpd, 
						(loc->name[0] == QSE_T('\0')? QSE_T("/"): loc->name));
					if (loccfg->locname.ptr == QSE_NULL) goto oops;

					loccfg->locname.len = qse_mbslen (loccfg->locname.ptr);
				}
			}

			if (qse_htb_upsert (server_xtn->cfgtab, hostcfg->hostname, qse_mbslen(hostcfg->hostname), hostcfg, 0) == QSE_NULL)
			{
				/* TODO: print error */
				goto oops;
			}
		}
	}

	return 0;

oops:
	if (hostcfg) free_server_hostcfg (httpd, hostcfg);
	return -1;
}

static void free_server_config (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	server_xtn_t* server_xtn;
	qse_size_t i;

	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	for (i = 0; i < SCFG_MAX; i++)
	{
		if (server_xtn->scfg[i]) 
		{
			qse_httpd_freemem (httpd, server_xtn->scfg[i]);
			server_xtn->scfg[i] = QSE_NULL;
		}
	}

	if (server_xtn->cfgtab) 
	{
		qse_htb_close (server_xtn->cfgtab);	
		server_xtn->cfgtab = QSE_NULL;
	}
}

static qse_httpd_server_t* attach_server (qse_httpd_t* httpd, int num, qse_xli_list_t* list)
{
	qse_httpd_server_dope_t dope; 
	qse_httpd_server_t* xserver;
	httpd_xtn_t* httpd_xtn;
	server_xtn_t* server_xtn;
	qse_xli_pair_t* pair;

	httpd_xtn = qse_httpd_getxtnstd (httpd);

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("bind"));
	if (pair == QSE_NULL || pair->val->type != QSE_XLI_STR)
	{
		/* TOOD: logging */
		qse_printf (QSE_T("WARNING: no value or invalid value specified for bind\n"));
		return QSE_NULL;
	}

	qse_memset (&dope, 0, QSE_SIZEOF(dope));
	if (qse_strtonwad (((qse_xli_str_t*)pair->val)->ptr, &dope.nwad) <= -1)
	{
		/*  TOOD: logging */
		qse_printf (QSE_T("WARNING: invalid value for bind - %s\n"), ((qse_xli_str_t*)pair->val)->ptr);
		return QSE_NULL;
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("ssl"));
	if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.ssl"));
	if (pair && pair->val->type == QSE_XLI_STR) dope.flags |= QSE_HTTPD_SERVER_SECURE;	

	dope.detach = free_server_config;
	xserver = qse_httpd_attachserverstd (httpd, &dope, QSE_SIZEOF(server_xtn_t));
	if (xserver == QSE_NULL) 
	{
		/* TODO: logging */
		qse_printf (QSE_T("WARNING: failed to attach server\n"));
		return QSE_NULL;
	}

	server_xtn = qse_httpd_getserverstdxtn (httpd, xserver);

	/* remember original callbacks  */
	qse_httpd_getserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_QUERY, &server_xtn->orgquery);
	qse_httpd_getserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_MAKERSRC, &server_xtn->orgmakersrc);
	qse_httpd_getserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_FREERSRC, &server_xtn->orgfreersrc);

	/* set changeable callbacks  */
	qse_httpd_setserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_QUERY, query_server);
	qse_httpd_setserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_MAKERSRC, make_resource);
	qse_httpd_setserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_FREERSRC, free_resource);

	/* remember the binding address used */
	server_xtn->num = num;
	server_xtn->bind = dope.nwad;
	server_xtn->secure = (dope.flags & QSE_HTTPD_SERVER_SECURE);

	return xserver;
}

static int open_config_file (qse_httpd_t* httpd)
{
	httpd_xtn_t* httpd_xtn;
	qse_xli_iostd_t xli_in;
	int trait;

	httpd_xtn = (httpd_xtn_t*) qse_httpd_getxtnstd (httpd);
	QSE_ASSERT (httpd_xtn->xli == QSE_NULL);

	httpd_xtn->xli = qse_xli_openstd (0);
	if (	httpd_xtn->xli == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open xli\n"));
		return -1;
	}
 
	qse_xli_getopt (httpd_xtn->xli, QSE_XLI_TRAIT, &trait);
	trait |= QSE_XLI_KEYNAME;
	qse_xli_setopt (httpd_xtn->xli, QSE_XLI_TRAIT, &trait);

	xli_in.type = QSE_XLI_IOSTD_FILE;
	xli_in.u.file.path = httpd_xtn->cfgfile;
	xli_in.u.file.cmgr = QSE_NULL;
 
	if (qse_xli_readstd (httpd_xtn->xli, &xli_in) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot load %s - %s\n"), xli_in.u.file.path, qse_xli_geterrmsg(httpd_xtn->xli));
		qse_xli_close (httpd_xtn->xli);
		httpd_xtn->xli = QSE_NULL;
		return -1;
	}
 
	return 0;
}

static int close_config_file (qse_httpd_t* httpd)
{
	httpd_xtn_t* httpd_xtn;

	httpd_xtn = (httpd_xtn_t*) qse_httpd_getxtnstd (httpd);
	if (httpd_xtn->xli)
	{
		qse_xli_close (httpd_xtn->xli);
		httpd_xtn->xli = QSE_NULL;
	}

	return 0;
}

static int load_config (qse_httpd_t* httpd)
{
	qse_xli_pair_t* pair;
	httpd_xtn_t* httpd_xtn;
	int i;

	httpd_xtn = (httpd_xtn_t*)qse_httpd_getxtnstd (httpd);

	if (open_config_file (httpd) <= -1) goto oops;

	for (i = 0; ; i++)
	{
		qse_char_t buf[32];
		qse_sprintf (buf, QSE_COUNTOF(buf), QSE_T("server[%d]"), i);
		pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, buf);
		if (pair == QSE_NULL) break;

		if (pair->val->type != QSE_XLI_LIST)
		{
			qse_fprintf (QSE_STDERR, QSE_T("WARNING: non-list value for server\n"));
		}
		else
		{
			qse_httpd_server_t* server;
	
			server = attach_server (httpd, i, (qse_xli_list_t*)pair->val);
			if (server)
			{
				if (load_server_config (httpd, server, (qse_xli_list_t*)pair->val) <= -1)
				{
					qse_fprintf (QSE_STDERR, QSE_T("failed to load configuration from %s\n"), httpd_xtn->cfgfile);
					goto oops;
				}
			}
		}
	}

	if (i == 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("No valid server specified in %s\n"), httpd_xtn->cfgfile);
		goto oops;
	}

	/* load the global default */
	pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default"));
	if (pair && pair->val->type == QSE_XLI_LIST)
	{
		if (load_loccfg (httpd, (qse_xli_list_t*)pair->val, &httpd_xtn->dflcfg) <=  -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("failed to load configuration from %s\n"), httpd_xtn->cfgfile);
			goto oops;
		}
	}

	if (httpd_xtn->dflcfg.xcfg[XCFG_NAME]) 
		qse_httpd_setname (httpd, httpd_xtn->dflcfg.xcfg[XCFG_NAME]);

	close_config_file (httpd);
	return 0;

oops:
	close_config_file (httpd);
	return -1;
}


static void reconf_server (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	httpd_xtn_t* httpd_xtn;
	server_xtn_t* server_xtn;
	qse_xli_pair_t* pair;

	/* reconfigure the server when the server is impeded. */

	httpd_xtn = qse_httpd_getxtnstd (httpd);
	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	if (httpd_xtn->xli)
	{
		qse_char_t buf[32];
		qse_sprintf (buf, QSE_COUNTOF(buf), QSE_T("server[%d]"), server_xtn->num);
		pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, buf);

		if (pair && pair->val->type == QSE_XLI_LIST) 
		{
			free_server_config (httpd, server);
			load_server_config (httpd, server, (qse_xli_list_t*)pair->val);
		}
	}
}

static void impede_httpd (qse_httpd_t* httpd)
{
	httpd_xtn_t* httpd_xtn;

	httpd_xtn = qse_httpd_getxtnstd (httpd);

	if (open_config_file (httpd) >= 0)
	{		
		qse_httpd_server_t* server;

		server = qse_httpd_getfirstserver (httpd);
		while (server)
		{
			if (server->dope.flags & QSE_HTTPD_SERVER_ACTIVE)
				reconf_server (httpd, server);	

			server = qse_httpd_getnextserver (httpd, server);
		}
		close_config_file (httpd);
	}

	/* chain-call the orignal impedence function */
	if (httpd_xtn->orgimpede) httpd_xtn->orgimpede (httpd);
}

static void logact_httpd (qse_httpd_t* httpd, const qse_httpd_act_t* act)
{
	httpd_xtn_t* httpd_xtn;
	qse_char_t tmp[128], tmp2[128], tmp3[128];

	httpd_xtn = qse_httpd_getxtnstd (httpd);

	switch (act->code)
	{
		case QSE_HTTPD_CATCH_MERRMSG:
			qse_printf (QSE_T("ERROR: %hs\n"), act->u.merrmsg);
			break;

		case QSE_HTTPD_CATCH_MDBGMSG:
			qse_printf (QSE_T("DEBUG: %hs\n"), act->u.mdbgmsg);
			break;

		case QSE_HTTPD_ACCEPT_CLIENT:
			qse_nwadtostr (&act->u.client->local_addr, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
			qse_nwadtostr (&act->u.client->orgdst_addr, tmp2, QSE_COUNTOF(tmp2), QSE_NWADTOSTR_ALL);
			qse_nwadtostr (&act->u.client->remote_addr, tmp3, QSE_COUNTOF(tmp3), QSE_NWADTOSTR_ALL);
			qse_printf (QSE_T("accepted client %s(%s) from %s\n"), tmp, tmp2, tmp3);

		case QSE_HTTPD_PURGE_CLIENT:
			qse_nwadtostr (&act->u.client->remote_addr, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
			qse_printf (QSE_T("purged client - %s\n"), tmp);
			break;		

		case QSE_HTTPD_READERR_CLIENT:
			qse_nwadtostr (&act->u.client->remote_addr, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
			qse_printf (QSE_T("failed to read client - %s\n"), tmp);
			break;		
	}
}

/* --------------------------------------------------------------------- */
static void print_version (void)
{
	qse_printf (QSE_T("QSEHTTPD version %hs\n"), QSE_PACKAGE_VERSION);
}

static void print_usage (QSE_FILE* out, int argc, qse_char_t* argv[])
{
	const qse_char_t* b = qse_basename (argv[0]);

	qse_fprintf (out, QSE_T("USAGE: %s [options] -c file\n"), b);
	qse_fprintf (out, QSE_T("       %s [options] --config-file file\n"), b);

	qse_fprintf (out, QSE_T("options as follows:\n"));
	qse_fprintf (out, QSE_T(" -h/--help                 show this message\n"));
	qse_fprintf (out, QSE_T(" --version                 show version\n"));
	qse_fprintf (out, QSE_T(" -c/--config-file file     specify a configuration file\n"));
	qse_fprintf (out, QSE_T(" -d/--daemon               run in the background\n"));
	qse_fprintf (out, QSE_T(" -n               string   specify a process name if supported\n"));
	qse_fprintf (out, QSE_T(" -x                        output debugging messages\n"));
}

static int handle_args (int argc, qse_char_t* argv[])
{
	static qse_opt_lng_t lng[] = 
	{
		{ QSE_T(":config-file"),     QSE_T('c') },
		{ QSE_T("daemon"),           QSE_T('d') },
		{ QSE_T("help"),             QSE_T('h') },
		{ QSE_T("version"),          QSE_T('\0') },
		{ QSE_NULL,                  QSE_T('\0') }                  
	};
	static qse_opt_t opt = 
	{
		QSE_T("c:dhn:x"),
		lng
	};
	qse_cint_t c;

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			default:
				goto wrongusage;

			case QSE_T('?'):
				qse_fprintf (QSE_STDERR, 
					QSE_T("ERROR: bad option - %c\n"),
					opt.opt
				);
				goto wrongusage;

			case QSE_T(':'):
				qse_fprintf (QSE_STDERR, 
					QSE_T("ERROR: bad parameter for %c\n"),
					opt.opt
				);
				goto wrongusage;

			case QSE_T('c'):
				g_cfgfile = opt.arg;
				break;

			case QSE_T('d'):
				g_daemon = 1;
				break;

			case QSE_T('h'):
				print_usage (QSE_STDOUT, argc, argv);
				return 0;

			case QSE_T('n'):
			{
				/* i don't care about failure to set the name */
				#if defined(HAVE_PRCTL)
					#if !defined(PR_SET_NAME)
					#	define PR_SET_NAME 15
					#endif
					#if defined(QSE_CHAR_IS_MCHAR)
						prctl (PR_SET_NAME, (unsigned long)opt.arg, 0, 0, 0);
					#else
						qse_mchar_t* mopt = qse_wcstombsdup (opt.arg, QSE_NULL, QSE_MMGR_GETDFL());
						if (mopt) 
						{
							prctl (PR_SET_NAME, (unsigned long)mopt, 0, 0, 0);
							QSE_MMGR_FREE (QSE_MMGR_GETDFL(), mopt);
						}
					#endif
				#endif
				break;
			}

			case QSE_T('x'):
				g_debug = 1;
				break;

			case QSE_T('\0'):
			{
				if (qse_strcmp(opt.lngopt, QSE_T("version")) == 0)
				{
					print_version ();
					return 0;
                    }
				break;
			}

		}
	}

	if (opt.ind < argc || g_cfgfile == QSE_NULL) goto wrongusage;

	return 1;

wrongusage:
	print_usage (QSE_STDERR, argc, argv);
	return -1;
}

static void free_httpd_xtn (qse_httpd_t* httpd)
{
	httpd_xtn_t* httpd_xtn;
	httpd_xtn = qse_httpd_getxtnstd (httpd);
	free_loccfg_contents (httpd, &httpd_xtn->dflcfg);
}

static int httpd_main (int argc, qse_char_t* argv[])
{
	qse_httpd_t* httpd = QSE_NULL;
	httpd_xtn_t* httpd_xtn;
	qse_ntime_t tmout;
	int trait, ret;
	qse_httpd_rcb_t rcb;
	qse_httpd_ecb_t ecb;

	ret = handle_args (argc, argv);
	if (ret <= -1) return -1;
	if (ret == 0) return 0;

	httpd = qse_httpd_openstd (QSE_SIZEOF(httpd_xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: Cannot open httpd\n"));
		goto oops;
	}

	qse_memset (&ecb, 0, QSE_SIZEOF(ecb));
	ecb.close = free_httpd_xtn;
	qse_httpd_pushecb (httpd, &ecb);
	httpd_xtn = qse_httpd_getxtnstd (httpd);
	httpd_xtn->cfgfile = g_cfgfile;

	if (load_config (httpd) <= -1) goto oops;

	if (g_daemon)
	{
		if (daemonize (1) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: Cannot daemonize\n"));
			goto oops;
		}
	}

	g_httpd = httpd;
	setup_signal_handlers ();

	qse_httpd_getopt (httpd, QSE_HTTPD_TRAIT, &trait);
	trait |= QSE_HTTPD_CGIERRTONUL | QSE_HTTPD_LOGACT;
	qse_httpd_setopt (httpd, QSE_HTTPD_TRAIT, &trait);

	tmout.sec = 10;
	tmout.nsec = 0;
	qse_httpd_setopt (httpd, QSE_HTTPD_TMOUT, &tmout);

	tmout.sec = 30;
	tmout.nsec = 0;
	qse_httpd_setopt (httpd, QSE_HTTPD_IDLELIMIT, &tmout);

	qse_httpd_getopt (httpd, QSE_HTTPD_RCB, &rcb);
	httpd_xtn->orgimpede = rcb.impede;
	rcb.impede = impede_httpd; /* executed when qse_httpd_impede() is called */
	if (g_debug) rcb.logact = logact_httpd; /* i don't remember old logging handler */
	qse_httpd_setopt (httpd, QSE_HTTPD_RCB, &rcb);
	
	ret = qse_httpd_loopstd (httpd);

	restore_signal_handlers ();
	g_httpd = QSE_NULL;

	if (ret <= -1) qse_fprintf (QSE_STDERR, QSE_T("Httpd error - %d\n"), qse_httpd_geterrnum (httpd));

oops:
	if (httpd) qse_httpd_close (httpd);
	return -1;
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
	/* ERR_remove_state() should be called for each thread if the application is thread */
	ERR_remove_state (0); 
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
