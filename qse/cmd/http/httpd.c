
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

/* --------------------------------------------------------------------- */

static qse_httpd_t* g_httpd = QSE_NULL;
static const qse_char_t* g_cfgfile = QSE_NULL;
static int g_daemon = 0;

/* --------------------------------------------------------------------- */

typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
	const  qse_char_t* cfgfile;
	qse_xli_t* xli;
	qse_httpd_impede_t orgimpede;
	int impede_code;
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

enum
{
	SCFG_NAME,
	SCFG_ROOT,
	SCFG_REALM,
	SCFG_AUTH,
	SCFG_DIRCSS,
	SCFG_ERRCSS,

	SCFG_MAX
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

typedef struct server_xtn_t server_xtn_t;
struct server_xtn_t
{
	int tproxy;
	int nodir; /* no directory listing */

	int num;
	qse_nwad_t bind;

	qse_httpd_serverstd_makersrc_t orgmakersrc;
	qse_httpd_serverstd_freersrc_t orgfreersrc;
	qse_httpd_serverstd_query_t orgquery;

	qse_mchar_t* scfg[SCFG_MAX];
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
};


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
static void clear_server_config (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	server_xtn_t* server_xtn;
	qse_size_t i, j;

	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	for (i = 0; i < QSE_COUNTOF(server_xtn->scfg); i++)
	{
		if (server_xtn->scfg[i]) 
		{
			qse_httpd_freemem (httpd, server_xtn->scfg[i]);
			server_xtn->scfg[i] = QSE_NULL;
		}
	}

	if (server_xtn->index.files) 
	{
		qse_httpd_freemem (httpd, server_xtn->index.files);
		server_xtn->index.files = QSE_NULL;
		server_xtn->index.count = 0;
	}

	for (i = 0; i < QSE_COUNTOF(server_xtn->cgi); i++)
	{
		struct cgi_t* cgi = server_xtn->cgi[i].head;
		while (cgi)
		{
			struct cgi_t* x = cgi;
			cgi = x->next;

			if (x->shebang) qse_httpd_freemem (httpd, x->shebang);
			if (x->spec) qse_httpd_freemem (httpd, x->spec);
			if (x) qse_httpd_freemem (httpd, x);
		}

		server_xtn->cgi[i].head = QSE_NULL;
		server_xtn->cgi[i].tail = QSE_NULL;
	}

	for (i = 0; i < QSE_COUNTOF(server_xtn->mime); i++)
	{
		struct mime_t* mime = server_xtn->mime[i].head;
		while (mime)
		{
			struct mime_t* x = mime;
			mime = x->next;

			if (x->spec) qse_httpd_freemem (httpd, x->spec);
			if (x->value) qse_httpd_freemem (httpd, x->value);
			if (x) qse_httpd_freemem (httpd, x);
		}

		server_xtn->mime[i].head = QSE_NULL;
		server_xtn->mime[i].tail = QSE_NULL;
	}

	for (j = 0; j < QSE_COUNTOF(server_xtn->access); j++)
	{
		for (i = 0; i < QSE_COUNTOF(server_xtn->access[j]); i++)
		{
			struct access_t* access = server_xtn->access[j][i].head;
			while (access)
			{
				struct access_t* x = access;
				access = x->next;

				if (x->spec) qse_httpd_freemem (httpd, x->spec);
				if (x) qse_httpd_freemem (httpd, x);
			}

			server_xtn->access[j][i].head = QSE_NULL;
			server_xtn->access[j][i].tail = QSE_NULL;
		}
	}
}

static void detach_server (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	clear_server_config (httpd, server);
}

static int query_server (
	qse_httpd_t* httpd, qse_httpd_server_t* server, 
	qse_htre_t* req, const qse_mchar_t* xpath,
	qse_httpd_serverstd_query_code_t code, void* result)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	switch (code)
	{
		case QSE_HTTPD_SERVERSTD_NAME:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_NAME];
			return 0;

		case QSE_HTTPD_SERVERSTD_ROOT:
			if (server_xtn->root_is_nwad)
			{
				((qse_httpd_serverstd_root_t*)result)->type = QSE_HTTPD_SERVERSTD_ROOT_NWAD;
				((qse_httpd_serverstd_root_t*)result)->u.nwad = server_xtn->root_nwad;
			}
			else
			{
				((qse_httpd_serverstd_root_t*)result)->type = QSE_HTTPD_SERVERSTD_ROOT_PATH;
				((qse_httpd_serverstd_root_t*)result)->u.path = server_xtn->scfg[SCFG_ROOT];
			}
			return 0;

		case QSE_HTTPD_SERVERSTD_REALM:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_REALM];
			return 0;

		case QSE_HTTPD_SERVERSTD_AUTH:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_AUTH];
			return 0;

		case QSE_HTTPD_SERVERSTD_DIRCSS:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_DIRCSS];
			return 0;

		case QSE_HTTPD_SERVERSTD_ERRCSS:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_ERRCSS];
			return 0;

		case QSE_HTTPD_SERVERSTD_INDEX:
			((qse_httpd_serverstd_index_t*)result)->count = server_xtn->index.count;
			((qse_httpd_serverstd_index_t*)result)->files = server_xtn->index.files;
			return 0;

		case QSE_HTTPD_SERVERSTD_CGI:
		{
			qse_size_t i;
			qse_httpd_serverstd_cgi_t* scgi;
			const qse_mchar_t* xpath_base;

			xpath_base = qse_mbsbasename (xpath);

			scgi = (qse_httpd_serverstd_cgi_t*)result;
			qse_memset (scgi, 0, QSE_SIZEOF(*scgi));

			for (i = 0; i < QSE_COUNTOF(server_xtn->cgi); i++)
			{
				struct cgi_t* cgi;
				for (cgi = server_xtn->cgi[i].head; cgi; cgi = cgi->next)
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
			for (i = 0; i < QSE_COUNTOF(server_xtn->mime); i++)
			{
				struct mime_t* mime;
				for (mime = server_xtn->mime[i].head; mime; mime = mime->next)
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
			for (i = 0; i < QSE_COUNTOF(server_xtn->access[id]); i++)
			{
				struct access_t* access;
				for (access = server_xtn->access[id][i].head; access; access = access->next)
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
	{ QSE_T("host['*'].location['/'].name"),      QSE_T("default.name") },
	{ QSE_T("host['*'].location['/'].root"),      QSE_T("default.root") },
	{ QSE_T("host['*'].location['/'].realm"),     QSE_T("default.realm") },
	{ QSE_T("host['*'].location['/'].auth"),      QSE_T("default.auth") },
	{ QSE_T("host['*'].location['/'].dir-css"),   QSE_T("default.dir-css") },
	{ QSE_T("host['*'].location['/'].error-css"), QSE_T("default.error-css") }
};

static int load_server_config (
	qse_httpd_t* httpd, qse_httpd_server_t* server, qse_xli_list_t* list)
{
	qse_size_t i;
	httpd_xtn_t* httpd_xtn;
	server_xtn_t* server_xtn;
	qse_xli_pair_t* pair;
	qse_xli_atom_t* atom;

	httpd_xtn = qse_httpd_getxtnstd (httpd);
	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	for (i = 0; i < QSE_COUNTOF(scfg_items); i++)
	{
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

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("host['*'].location['/'].index"));
	if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.index"));
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
			qse_printf (QSE_T("ERROR: in copying index\n"));
			return -1;
		}
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("host['*'].location['/'].cgi"));
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
				if (server_xtn->cgi[type].tail)
					server_xtn->cgi[type].tail->next = cgi;
				else
					server_xtn->cgi[type].head = cgi;
				server_xtn->cgi[type].tail = cgi;
			}
		}	
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("host['*'].location['/'].mime"));
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

				if (server_xtn->mime[type].tail)
					server_xtn->mime[type].tail->next = mime;
				else
					server_xtn->mime[type].head = mime;
				server_xtn->mime[type].tail = mime;
			}
		}	
	}

	for (i = 0; i < 2;  i++)
	{
		static struct 
		{
			const qse_char_t* x;
			const qse_char_t* y;
		} acc_items[] =
		{
			{ QSE_T("host['*'].location['/'].dir-access"), QSE_T("default.dir-access") },
			{ QSE_T("host['*'].location['/'].file-access"), QSE_T("default.file-access") },
		};

		pair = qse_xli_findpairbyname (httpd_xtn->xli, list, acc_items[i].x);
		if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, acc_items[i].y);
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
	
					if (server_xtn->access[i][type].tail)
						server_xtn->access[i][type].tail->next = acc;
					else
						server_xtn->access[i][type].head = acc;
					server_xtn->access[i][type].tail = acc;
				}
			}	
		}
	}

	/* perform more sanity check */
	if (qse_mbschr (server_xtn->scfg[SCFG_AUTH], QSE_MT(':')) == QSE_NULL)
	{
		qse_printf (QSE_T("WARNING: no colon in the auth string - [%hs]\n"), server_xtn->scfg[SCFG_AUTH]);
	}

	if (qse_mbstonwad (server_xtn->scfg[SCFG_ROOT], &server_xtn->root_nwad) >= 0) server_xtn->root_is_nwad = 1;
	return 0;
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

	dope.detach = detach_server;
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
	qse_xli_iostd_t xli_in;
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
				load_server_config (httpd, server, (qse_xli_list_t*)pair->val);
				/* TODO: error check */
			}
		}
	}

	if (i == 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("No valid server specified in %s\n"), xli_in.u.file.path);
		goto oops;
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.name"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		qse_mchar_t* name = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
		if (name)
		{
			qse_httpd_setname (httpd, name);
			qse_httpd_freemem (httpd, name);
		}
		else
		{
			/* TODO: warning */
		}
	}

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
			clear_server_config (httpd, server);
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
		QSE_T("c:dh"),
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

static int httpd_main (int argc, qse_char_t* argv[])
{
	qse_httpd_t* httpd = QSE_NULL;
	httpd_xtn_t* httpd_xtn;
	qse_ntime_t tmout;
	int trait, ret;
	qse_httpd_rcb_t rcb;

	ret = handle_args (argc, argv);
	if (ret <= -1) return -1;
	if (ret == 0) return 0;

	httpd = qse_httpd_openstd (QSE_SIZEOF(httpd_xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: Cannot open httpd\n"));
		goto oops;
	}

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
	rcb.logact = logact_httpd; /* i don't remember this */
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

