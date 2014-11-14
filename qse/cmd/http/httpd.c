
#include <qse/http/stdhttpd.h>
#include <qse/xli/stdxli.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>
#include <qse/cmn/path.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/hton.h>

#include <signal.h>
#include <locale.h>

#include <stdio.h> /* TODO: remove this header file */

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
#	include <tcp.h> /* watt-32 */
#else
#	include <unistd.h>
#	include <errno.h>
#	include <fcntl.h>
#endif

#if defined(HAVE_SSL)
#	include <openssl/ssl.h>
#	if defined(HAVE_OPENSSL_ERR_H)
#		include <openssl/err.h>
#	endif
#	if defined(HAVE_OPENSSL_ENGINE_H)
#		include <openssl/engine.h>
#	endif
#endif

#if defined(HAVE_SYS_PRCTL_H)
#	include <sys/prctl.h>
#endif

#if defined(HAVE_SYS_RESOURCE_H)
#	include <sys/resource.h>
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
	XCFG_ROOT,
	XCFG_REALM,
	XCFG_AUTH,
	XCFG_DIRHEAD,
	XCFG_DIRFOOT,
	XCFG_ERRHEAD,
	XCFG_ERRFOOT,
	XCFG_MAX
};

struct cgi_t
{
	enum 
	{
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

struct auth_rule_t
{
	enum {
		AUTH_RULE_PREFIX,
		AUTH_RULE_SUFFIX,
		AUTH_RULE_NAME,
		AUTH_RULE_OTHER,
		AUTH_RULE_MAX
	} type;

	qse_mchar_t* spec;
	/* TODO: add individual realm and auth string */
	int noauth;

	struct auth_rule_t* next;
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
	enum 
	{
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
	qse_mcstr_t locname;

	qse_mchar_t* xcfg[XCFG_MAX];

	enum
	{
		ROOT_TYPE_PATH = 0,
		ROOT_TYPE_NWAD,
		ROOT_TYPE_NWAD_SECURE,
		ROOT_TYPE_HOST,
		ROOT_TYPE_HOST_SECURE,
		ROOT_TYPE_RELOC,
		ROOT_TYPE_ERROR
	} root_type;
	union
	{
		qse_nwad_t nwad;
		const qse_mchar_t* host;
		int error_code;
		qse_httpd_rsrc_reloc_t reloc;
	} root;

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
		struct auth_rule_t* head;
		struct auth_rule_t* tail;
	} auth_rule[AUTH_RULE_MAX];

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

	struct
	{
		unsigned int allow_http: 1;
		unsigned int allow_https: 1;
		unsigned int allow_connect: 1;
		unsigned int allow_intercept: 2; /* 0: no, 1: proxy, 2: local */
		unsigned int allow_upgrade: 1;
		unsigned int dns_enabled: 1;
		unsigned int urs_enabled: 1;
		unsigned int x_forwarded: 1;
		qse_nwad_t dns_nwad; /* TODO: multiple dns */
		qse_nwad_t urs_nwad; /* TODO: multiple urs */
		int dns_timeout;
		int dns_retries;
		int dns_queries;
		int urs_timeout;
		int urs_retries;
		qse_httpd_mod_t* dns_preresolve_mod;
		qse_httpd_mod_t* urs_prerewrite_mod;
		qse_mchar_t pseudonym[128]; /* TODO: good size? */
	} proxy;

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
	int nodir; /* no directory listing */

	int num; /* the server number in the xli configuration */
	qse_nwad_t bind; /* binding address */
	int secure; /* ssl */
	qse_mchar_t* scfg[SCFG_MAX];

	qse_httpd_serverstd_makersrc_t orgmakersrc;
	qse_httpd_serverstd_freersrc_t orgfreersrc;
	qse_httpd_serverstd_query_t orgquery;

	qse_htb_t* cfgtab; /* key: host name, value: server_hostcfg_t */
};

/* --------------------------------------------------------------------- */

typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
	const  qse_char_t* cfgfile;
	qse_xli_t* xli;
	qse_httpd_impede_t org_impede;
	int impede_code;

	qse_httpd_urs_prerewrite_t org_urs_prerewrite;
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

int xxxx (void* ctx, qse_env_char_t** envir)
{
/* NOTE: this is for experiment only */
#if 0
	extern char** environ;
char buf[1000];
char* cl;
int cl_i = 0;
	environ = envir;
	printf ("Content-Type: text/html\n\n");
	printf ("<html><body><pre>\n");
	system ("ls -laF /tmp");
	printf ("--------------------\n");
	system ("printenv");
 cl = getenv("CONTENT_LENGTH");
if (cl) cl_i = atoi(cl);
/*if (cl_i)
{  */
while (fgets (buf, sizeof(buf), stdin) != NULL)
{
printf ("%s", buf);
}
/*}
	system ("while read xxx; do echo $xxx; done; echo 123 456 789");*/
	printf ("</pre></body></html>\n");
#endif
	printf ("Content-Type: text/html\n\n");
	printf ("<html><body><pre>\n");
	system ("ls -laF /tmp");
	printf ("</pre></body></html>\n");

	return 0;
}

static int make_resource (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_htre_t* req, qse_httpd_rsrc_t* rsrc)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverstdxtn (httpd, client->server);

	if (server_xtn->orgmakersrc (httpd, client, req, rsrc) <= -1) return -1;
	if (server_xtn->nodir && rsrc->type == QSE_HTTPD_RSRC_DIR)
	{
		/* prohibit no directory listing */
		server_xtn->orgfreersrc (httpd, client, req, rsrc);
		rsrc->type = QSE_HTTPD_RSRC_ERROR;
		rsrc->u.error.code = 403;
	}
	return 0;
}

static void free_resource (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_htre_t* req, qse_httpd_rsrc_t* rsrc)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverstdxtn (httpd, client->server);
	server_xtn->orgfreersrc (httpd, client, req, rsrc);
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

static int get_server_root (
	qse_httpd_t* httpd,
	qse_httpd_server_t* server, 
	loccfg_t* loccfg,
	const qse_httpd_serverstd_query_info_t* qinfo,
	qse_httpd_serverstd_root_t* root)
{
	qse_http_method_t mth;
	qse_mchar_t* qpath;
	int proto_len;

	qse_memset (root, 0, QSE_SIZEOF(*root));
	mth = qse_htre_getqmethodtype (qinfo->req);
	qpath = qse_htre_getqpath(qinfo->req);

	qse_memset (root, 0, QSE_SIZEOF(*root));

	if (qinfo->client->status & QSE_HTTPD_CLIENT_INTERCEPTED)
	{
		/* transparent proxying */
		if (loccfg->proxy.allow_intercept <= 0)
		{
			root->type = QSE_HTTPD_SERVERSTD_ROOT_ERROR;
			root->u.error.code = 403; /* forbidden */
			return 0;
		}
		else if (loccfg->proxy.allow_intercept <= 1)
		{
			root->type = QSE_HTTPD_SERVERSTD_ROOT_PROXY;
			root->u.proxy.dst.nwad = qinfo->client->orgdst_addr;
			/* if TPROXY is used, set the source to the original source.
			root->u.proxy.src.nwad = qinfo->client->remote_addr;
			qse_setnwadport (&root->u.proxy.src.nwad, 0);*/

			if (mth == QSE_HTTP_CONNECT) 
				root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_RAW;
			else if (loccfg->proxy.pseudonym[0]) 
				root->u.proxy.pseudonym = loccfg->proxy.pseudonym;

			goto proxy_ok;
		}
	}

	if (mth == QSE_HTTP_CONNECT)
	{
		if (loccfg->proxy.allow_connect)
		{
			/* TODO: disallow connecting back to self */
			/* TODO: Proxy-Authorization???? */

			root->type = QSE_HTTPD_SERVERSTD_ROOT_PROXY;
			root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_RAW;
			/* no pseudonym for RAW proxying */

			if (qse_mbstonwad(qpath, &root->u.proxy.dst.nwad) <= -1) 
			{
				root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_DST_STR;
				root->u.proxy.dst.str = qpath;
			}
			else
			{
				/* make the source binding type the same as destination */
				/* no default port for raw proxying */
				root->u.proxy.src.nwad.type = root->u.proxy.dst.nwad.type;
			}

			goto proxy_ok;
		}
		else
		{
			root->type = QSE_HTTPD_SERVERSTD_ROOT_ERROR;
			root->u.error.code = 403; /* forbidden */
			return 0;
		}
	}

	if ((loccfg->proxy.allow_http && qse_mbszcasecmp (qpath, QSE_MT("http://"), (proto_len = 7)) == 0) ||
	    (loccfg->proxy.allow_https && qse_mbszcasecmp (qpath, QSE_MT("https://"), (proto_len = 8)) == 0))
	{
		qse_mchar_t* host, * slash;

		host = qpath + proto_len;
		slash = qse_mbschr (host, QSE_MT('/'));

		if (slash && slash - host > 0)
		{
			qse_size_t len_before_slash;
			qse_mchar_t* org_qpath = QSE_NULL;

			len_before_slash = slash - qpath;

			if (!(qinfo->req->flags & QSE_HTRE_QPATH_PERDEC) ||
			    qse_mbszcmp (qpath, (org_qpath = qse_htre_getorgqpath(qinfo->req)), len_before_slash) == 0)
			{
				/* this block ensures to proxy a request whose protocol and
				 * host name part were not percent-encoded in the original
				 * request */
				
				/* e.g. proto://hostname/XXXX 
				 *      slash should point to the slash before XXXX.
				 *      if hostname is empty, this 'if' block is skipped. */

				root->type = QSE_HTTPD_SERVERSTD_ROOT_PROXY;

				if (loccfg->proxy.pseudonym[0]) 
					root->u.proxy.pseudonym = loccfg->proxy.pseudonym;

/* TODO: refrain from manipulating the request like this */

				/* move the host name part backward by 1 byte to make a room for
				 * terminating null. An orginal input of http://www.yahoo.com/ab/cd
				 * becomes http:/www.yahoo.com\0ab/cd. host gets to point to 
				 * www.yahoo.com. qpath(qinfo->req.u.q.path) is updated to ab/cd. */
				qse_memmove (host - 1, host, slash - host); 
				slash[-1] = QSE_MT('\0');
				host = host - 1;
				root->u.proxy.host = host;

				if (proto_len == 8) root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_DST_SECURE;
				if (qse_mbstonwad (host, &root->u.proxy.dst.nwad) <= -1) 
				{
					root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_DST_STR;
					root->u.proxy.dst.str = host;
				}
				else
				{
					/* make the source binding type the same as destination */
					if (qse_getnwadport(&root->u.proxy.dst.nwad) == 0)
						qse_setnwadport (&root->u.proxy.dst.nwad, qse_hton16(80));
					root->u.proxy.src.nwad.type = root->u.proxy.dst.nwad.type;
				}

	/* TODO: refrain from manipulating the request like this */
				qinfo->req->u.q.path.len -= len_before_slash;
				qinfo->req->u.q.path.ptr = slash; /* TODO: use setqpath or something... */
				if (org_qpath)
				{
					qinfo->req->orgqpath.len -= len_before_slash;
					qinfo->req->orgqpath.ptr += len_before_slash;
				}

				goto proxy_ok;
			}
		}
		else
		{
			root->type = QSE_HTTPD_SERVERSTD_ROOT_ERROR;
			root->u.error.code = 403; /* forbidden */
			return 0;
		}
	}

	switch (loccfg->root_type)
	{
		case ROOT_TYPE_NWAD_SECURE:
			root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_DST_SECURE;
			/* fall thru */
		case ROOT_TYPE_NWAD:
			/* simple forwarding. it's not controlled by proxy.http, proxy.https or proxy.connect  */
			root->type = QSE_HTTPD_SERVERSTD_ROOT_PROXY;

			root->u.proxy.dst.nwad = loccfg->root.nwad;
			root->u.proxy.src.nwad.type = root->u.proxy.dst.nwad.type;

			if (loccfg->proxy.pseudonym[0]) 
				root->u.proxy.pseudonym = loccfg->proxy.pseudonym;

			goto proxy_ok;

		case ROOT_TYPE_HOST_SECURE:
			root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_DST_SECURE;
		case ROOT_TYPE_HOST:
			root->type = QSE_HTTPD_SERVERSTD_ROOT_PROXY;
			root->u.proxy.dst.str = loccfg->root.host;
			root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_DST_STR;
			if (loccfg->proxy.pseudonym[0]) 
				root->u.proxy.pseudonym = loccfg->proxy.pseudonym;
			goto proxy_ok;

		case ROOT_TYPE_RELOC:
			root->type = QSE_HTTPD_SERVERSTD_ROOT_RELOC;
			root->u.reloc.flags = loccfg->root.reloc.flags;
			root->u.reloc.target = qse_mbsdup (loccfg->root.reloc.target, qse_httpd_getmmgr(httpd));
			if (root->u.reloc.target == QSE_NULL)
			{
				root->type = QSE_HTTPD_SERVERSTD_ROOT_ERROR;
				root->u.error.code = 500; /* internal server error */
			}
			break;

		case ROOT_TYPE_ERROR:
			root->type = QSE_HTTPD_SERVERSTD_ROOT_ERROR;
			root->u.error.code = loccfg->root.error_code;
			break;

		default:
			/* local file system */
			root->type = QSE_HTTPD_SERVERSTD_ROOT_PATH;
			root->u.path.val = loccfg->xcfg[XCFG_ROOT];
			root->u.path.rpl = loccfg->locname.len;
			break;
	}

	return 0;

proxy_ok:
	if (loccfg->proxy.dns_enabled)
	{
		root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_ENABLE_DNS | QSE_HTTPD_RSRC_PROXY_DNS_SERVER;
		root->u.proxy.dns_server.nwad = loccfg->proxy.dns_nwad;
		root->u.proxy.dns_server.tmout.sec = loccfg->proxy.dns_timeout;
		root->u.proxy.dns_server.retries = loccfg->proxy.dns_retries;
		root->u.proxy.dns_server.flags = loccfg->proxy.dns_queries;
		root->u.proxy.dns_preresolve_mod = loccfg->proxy.dns_preresolve_mod;
	}

	if (loccfg->proxy.urs_enabled)
	{
		root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_ENABLE_URS | QSE_HTTPD_RSRC_PROXY_URS_SERVER;
		root->u.proxy.urs_server.nwad = loccfg->proxy.urs_nwad;
		root->u.proxy.urs_server.tmout.sec = loccfg->proxy.urs_timeout;
		root->u.proxy.urs_server.retries = loccfg->proxy.urs_retries;
		root->u.proxy.urs_prerewrite_mod = loccfg->proxy.urs_prerewrite_mod;
	}

	if (loccfg->proxy.x_forwarded)
	{
		root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_X_FORWARDED;
	}

	if (loccfg->proxy.allow_upgrade)
	{
		root->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_ALLOW_UPGRADE;
	}

	return 0;
}

static int query_server (
	qse_httpd_t* httpd, qse_httpd_server_t* server, 
	qse_httpd_serverstd_query_code_t code,
	qse_httpd_serverstd_query_info_t* qinfo, void* result)
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

	if (server_xtn->cfgtab)
	{
		if (qinfo && qinfo->req && server_xtn->cfgtab)
		{
			const qse_mchar_t* host = QSE_NULL;
			const qse_mchar_t* qpath;
			qse_http_method_t mth;
			const qse_htre_hdrval_t* hosthdr;

			mth = qse_htre_getqmethodtype (qinfo->req);
			qpath = qse_htre_getqpath (qinfo->req);
			if (mth == QSE_HTTP_CONNECT) 
			{
				qse_nwad_t nwad;

				/* the query path for CONNECT is not a path name, but 
				 * a host name. the path is adjusted to the root directory. */
				host = qpath;
				qpath = QSE_MT("/");

				if (qse_mbstonwad (host, &nwad) >= 0)
				{
					/* the host is a numeric. try he Host: header.
					 * i expect the values of the host and qpath variables
					 * are maintained if Host: is not found after goto. */
					goto try_host; 
				}
			}
			else
			{
			try_host:
				hosthdr = qse_htre_getheaderval (qinfo->req, QSE_MT("Host"));
				if (hosthdr) 
				{
					/*while (hosthdr->next) hosthdr = hosthdr->next; */
					host = hosthdr->ptr;
				}
			}

			if (host)
			{
				const qse_mchar_t* colon;
				qse_size_t hostlen;

				/* remove :port-number if the host name contains it */
				colon = qse_mbsrchr(host, QSE_MT(':'));
				if (colon) hostlen = colon - host;
				else hostlen = qse_mbslen(host);

				/* Wild card search
				 * 
				 * www.tango.com => 
				 *    www.tango.com 
				 *    tango.com   
				 *    com           <-- up to here
				 *    *             
				 *
				 * tango.com =>
				 *    tango.com
				 *    com           <-- up to here
				 *    *
				 */
				while (hostlen > 0)
				{
					qse_mchar_t c;

					loccfg = find_loccfg (httpd, server_xtn->cfgtab, host, hostlen, qpath);
					if (loccfg) goto found;

					/* skip the current segment */
					do
					{
						c = *host++;
						hostlen--;
					}
					while (c != QSE_MT('.') && hostlen > 0);
				}

			}

			if (loccfg == QSE_NULL) loccfg = find_loccfg (httpd, server_xtn->cfgtab, QSE_MT("*"), 1, qpath);
		}

		if (loccfg == QSE_NULL) loccfg = find_loccfg (httpd, server_xtn->cfgtab, QSE_MT("*"), 1, QSE_MT("/"));
	}
	if (loccfg == QSE_NULL) loccfg = &httpd_xtn->dflcfg;

found:
	switch (code)
	{
		case QSE_HTTPD_SERVERSTD_ROOT:
			#if 0
			if (qse_mbscmp (qse_htre_getqpath(qinfo->req), QSE_MT("/version")) == 0)
			{
				/* return static text without inspecting further */
			
				/*((qse_httpd_serverstd_root_t*)result)->type = QSE_HTTPD_SERVERSTD_ROOT_TEXT;
				((qse_httpd_serverstd_root_t*)result)->u.text.ptr = QSE_MT(QSE_PACKAGE_NAME " " QSE_PACKAGE_VERSION);
				((qse_httpd_serverstd_root_t*)result)->u.text.mime = QSE_MT("text/plain");*/
			else
			#endif

			return get_server_root (httpd, server, loccfg, qinfo, result);

		case QSE_HTTPD_SERVERSTD_REALM:
		{
			const qse_mchar_t* apath;
			qse_size_t i;

			((qse_httpd_serverstd_realm_t*)result)->name = loccfg->xcfg[XCFG_REALM];

			/* qinfo->xpath is not available for the REALM query in the std implementation.
			 * let me check if it's available in case the implementation changes */
			apath = qinfo->xpath? qinfo->xpath: qse_htre_getqpath (qinfo->req);
			if (apath)
			{
				const qse_mchar_t* base;
				base = qse_mbsbasename (apath);

				for (i = 0; i < QSE_COUNTOF(loccfg->auth_rule); i++)
				{
					struct auth_rule_t* auth_rule;
					for (auth_rule = loccfg->auth_rule[i].head; auth_rule; auth_rule = auth_rule->next)
					{
						if ((auth_rule->type == AUTH_RULE_PREFIX && qse_mbsbeg (base, auth_rule->spec)) ||
						    (auth_rule->type == AUTH_RULE_SUFFIX && qse_mbsend (base, auth_rule->spec)) ||
						    (auth_rule->type == AUTH_RULE_NAME && qse_mbscmp (base, auth_rule->spec) == 0) ||
						    auth_rule->type == AUTH_RULE_OTHER)
						{
							if (auth_rule->noauth) 
							{
								/* if noauth is set, authorization is not required */
								((qse_httpd_serverstd_realm_t*)result)->authreq = 0;
								return 0;
							}
							else 
							{
								/* proceed to perform authorization */
								break;
							}
						}
					}
				}
			}

			((qse_httpd_serverstd_realm_t*)result)->authreq = (loccfg->xcfg[XCFG_REALM] != QSE_NULL);
			return 0;
		}

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

		case QSE_HTTPD_SERVERSTD_DIRHEAD:
			*(const qse_mchar_t**)result = loccfg->xcfg[XCFG_DIRHEAD];
			return 0;

		case QSE_HTTPD_SERVERSTD_DIRFOOT:
			*(const qse_mchar_t**)result = loccfg->xcfg[XCFG_DIRFOOT];
			return 0;

		case QSE_HTTPD_SERVERSTD_ERRHEAD:
			*(const qse_mchar_t**)result = loccfg->xcfg[XCFG_ERRHEAD];
			return 0;

		case QSE_HTTPD_SERVERSTD_ERRFOOT:
			*(const qse_mchar_t**)result = loccfg->xcfg[XCFG_ERRFOOT];
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
#if 0
			qse_mchar_t* qpath;

			qpath = qse_htre_getqpath(qinfo->req);
#endif

			scgi = (qse_httpd_serverstd_cgi_t*)result;
			qse_memset (scgi, 0, QSE_SIZEOF(*scgi));

#if 0
			printf ("qinfo->xpath [%s] %d [%s]\n", qinfo->xpath, qinfo->xpath_nx, qpath);
			/*if (qse_mbscmp (qinfo->xpath, QSE_MT("/tmp/version.cgi")) == 0)*/
			if (qse_mbscmp (qpath, QSE_MT("/local/version.cgi")) == 0)
			{
				scgi->cgi = 1;
				scgi->nph = 0;
				scgi->fncptr = xxxx;
				scgi->shebang = QSE_NULL;
				return 0;
			}
#endif
			if (!qinfo->xpath_nx)
			{
				xpath_base = qse_mbsbasename (qinfo->xpath);
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
			}

			return 0;
		}

		case QSE_HTTPD_SERVERSTD_MIME:
		{
			qse_size_t i;
			const qse_mchar_t* xpath_base;

			xpath_base = qse_mbsbasename (qinfo->xpath);

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
			switch (qse_htre_getqmethodtype(qinfo->req))
			{
				case QSE_HTTP_OPTIONS:
				case QSE_HTTP_HEAD:
				case QSE_HTTP_GET:
				case QSE_HTTP_POST:
				case QSE_HTTP_PUT:
				case QSE_HTTP_DELETE:
				{
					qse_size_t i;
					const qse_mchar_t* xpath_base;
					int id;

					id = (code == QSE_HTTPD_SERVERSTD_DIRACC)? 0: 1;
		
					xpath_base = qse_mbsbasename (qinfo->xpath);

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
					break;
				}
	
				default:
					*(int*)result = 405; /* method not allowed */
					break;
			}

			return 0;
		}
	}

	return server_xtn->orgquery (httpd, server, code, qinfo, result);
}

/* --------------------------------------------------------------------- */

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

	for (i = 0; i < QSE_COUNTOF(loccfg->auth_rule); i++)
	{
		struct auth_rule_t* auth_rule = loccfg->auth_rule[i].head;
		while (auth_rule)
		{
			struct auth_rule_t* x = auth_rule;
			auth_rule = x->next;

			if (x->spec) qse_httpd_freemem (httpd, x->spec);
			if (x) qse_httpd_freemem (httpd, x);
		}

		loccfg->auth_rule[i].head = QSE_NULL;
		loccfg->auth_rule[i].tail = QSE_NULL;
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

static int get_boolean (const qse_xli_str_t* v)
{
	return (qse_strxcasecmp (v->ptr, v->len, QSE_T("yes")) == 0 ||
	        qse_strxcasecmp (v->ptr, v->len, QSE_T("on")) == 0);
}

static int get_intercept (const qse_xli_str_t* v)
{
	if (qse_strxcasecmp (v->ptr, v->len, QSE_T("local")) == 0) return 2;
	if (qse_strxcasecmp (v->ptr, v->len, QSE_T("proxy")) == 0 ||
	    qse_strxcasecmp (v->ptr, v->len, QSE_T("yes")) == 0 ||
	    qse_strxcasecmp (v->ptr, v->len, QSE_T("on")) == 0) return 1;
	return 0;
}

static int get_integer (const qse_xli_str_t* v)
{
/* TODO: be more strict */
	return qse_strxtoi (v->ptr, v->len, 10);
}

static int parse_dns_query_types (qse_httpd_t* httpd, const qse_xli_str_t* str)
{
	int flags = 0;

	while (str)
	{
		if (qse_strxcasecmp (str->ptr, str->len, QSE_T("a")) == 0) flags |= QSE_HTTPD_DNS_SERVER_A;
		else if (qse_strxcasecmp (str->ptr, str->len, QSE_T("aaaa")) == 0) flags |= QSE_HTTPD_DNS_SERVER_AAAA;
		else 
		{
			qse_printf (QSE_T("ERROR: invalid dns query type for proxy dns - %s\n"), str->ptr);
			return -1;
		}

		str = str->next;
	}

	return flags;
}


static int load_loccfg_basic (qse_httpd_t* httpd, qse_xli_t* xli, qse_xli_list_t* list, loccfg_t* cfg)
{
	static struct 
	{
		const qse_char_t* x;
		const qse_char_t* y;
	} loc_xcfg_items[] =
	{
		{ QSE_T("root"),        QSE_T("server-default.root") },
		{ QSE_T("realm"),       QSE_T("server-default.realm") },
		{ QSE_T("auth"),        QSE_T("server-default.auth") },
		{ QSE_T("dir-head"),    QSE_T("server-default.dir-head") },
		{ QSE_T("dir-foot"),    QSE_T("server-default.dir-foot") },
		{ QSE_T("error-head"),  QSE_T("server-default.error-head") },
		{ QSE_T("error-foot"),  QSE_T("server-default.error-foot") }
	};

	int i;
	qse_xli_pair_t* pair;

	for (i = 0; i < QSE_COUNTOF(loc_xcfg_items); i++)
	{
		pair = qse_xli_findpair (xli, list, loc_xcfg_items[i].x);
		if (!pair) pair = qse_xli_findpair (xli, QSE_NULL, loc_xcfg_items[i].y);
		if (pair && pair->val->type == QSE_XLI_STR)
		{
			cfg->xcfg[i] = qse_httpd_strntombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len);
			if (cfg->xcfg[i] == QSE_NULL) 
			{
				/*qse_printf (QSE_T("ERROR in copying - %s\n"), qse_httpd_geterrmsg (httpd));*/
				qse_printf (QSE_T("ERROR in copying\n"));
				return -1;
			}
		}
	}

	return 0;
}

static int load_loccfg_index (qse_httpd_t* httpd, qse_xli_t* xli, qse_xli_list_t* list, loccfg_t* cfg)
{
	qse_xli_pair_t* pair;

	pair = qse_xli_findpair (xli, list, QSE_T("index"));
	if (!pair) pair = qse_xli_findpair (xli, QSE_NULL, QSE_T("server-default.index"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		qse_char_t* duptmp;
		qse_size_t count, duplen;

		duptmp = qse_xli_dupflatstr (xli, (qse_xli_str_t*)pair->val, &duplen, &count);
		if (duptmp == QSE_NULL)
		{
			qse_printf (QSE_T("ERROR: memory failure in copying index\n"));
			return -1;
		}

		cfg->index.files = qse_httpd_strntombsdup (httpd, duptmp, duplen);
		qse_xli_freemem (xli, duptmp);

		if (cfg->index.files == QSE_NULL) 
		{
			qse_printf (QSE_T("ERROR: memory failure in copying index\n"));
			return -1;
		}

		cfg->index.count = count;
	}

	return 0;
}

static int load_loccfg_cgi (qse_httpd_t* httpd, qse_xli_t* xli, qse_xli_list_t* list, loccfg_t* cfg)
{
	qse_xli_pair_t* pair;
	qse_xli_atom_t* atom;

	pair = qse_xli_findpair (xli, list, QSE_T("cgi"));
	if (!pair) pair = qse_xli_findpair (xli, QSE_NULL, QSE_T("server-default.cgi"));
	if (pair && pair->val->type == QSE_XLI_LIST)
	{
		qse_xli_list_t* cgilist = (qse_xli_list_t*)pair->val;
		for (atom = cgilist->head; atom; atom = atom->next)
		{
			struct cgi_t* cgi;
			int type;

			if (atom->type != QSE_XLI_PAIR) continue;

			pair = (qse_xli_pair_t*)atom;

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
			cgi->spec = qse_httpd_strtombsdup (httpd, pair->alias);
			if (!cgi->spec)
			{
				qse_httpd_freemem (httpd, cgi);
				qse_printf (QSE_T("ERROR: memory failure in copying cgi name\n"));
				return -1;
			}
			if (pair->val->type == QSE_XLI_STR) 
			{
				qse_xli_str_t* str;
				qse_size_t count = 0;

				for (str = (qse_xli_str_t*)pair->val; str; str = str->next)
				{
					if (count == 0)
					{
						/* the first segment */
						if (qse_strxcmp (str->ptr, str->len, QSE_T("nph")) == 0) cgi->nph = 1;
					}
					else if (count == 1)
					{
						/* second segment */
						cgi->shebang = qse_httpd_strntombsdup (httpd, str->ptr, str->len);
						if (!cgi->shebang)
						{
							qse_httpd_freemem (httpd, cgi->spec);
							qse_httpd_freemem (httpd, cgi);
							qse_printf (QSE_T("ERROR: memory failure in copying cgi shebang\n"));
							return -1;
						}
					}

					count++;

					/* TODO: more sanity check like the number of maximum segments or the value of the first segment...*/
				}

			}
			if (cfg->cgi[type].tail)
				cfg->cgi[type].tail->next = cgi;
			else
				cfg->cgi[type].head = cgi;
			cfg->cgi[type].tail = cgi;
		}
	}

	return 0;
}

static int load_loccfg_authrule (qse_httpd_t* httpd, qse_xli_t* xli, qse_xli_list_t* list, loccfg_t* cfg)
{
	qse_xli_pair_t* pair;
	qse_xli_atom_t* atom;

	pair = qse_xli_findpair (xli, list, QSE_T("auth-rule"));
	if (!pair) pair = qse_xli_findpair (xli, QSE_NULL, QSE_T("server-default.auth-rule"));
	if (pair && pair->val->type == QSE_XLI_LIST)
	{
		qse_xli_list_t* auth_rule_list = (qse_xli_list_t*)pair->val;
		for (atom = auth_rule_list->head; atom; atom = atom->next)
		{
			struct auth_rule_t* auth_rule;
			int type;

			if (atom->type != QSE_XLI_PAIR) continue;

			pair = (qse_xli_pair_t*)atom;

			if (qse_strcmp (pair->key, QSE_T("prefix")) == 0) type = AUTH_RULE_PREFIX;
			else if (qse_strcmp (pair->key, QSE_T("suffix")) == 0) type = AUTH_RULE_SUFFIX;
			else if (qse_strcmp (pair->key, QSE_T("name")) == 0) type = AUTH_RULE_NAME;
			else if (qse_strcmp (pair->key, QSE_T("other")) == 0) type = AUTH_RULE_OTHER;
			else continue;

			auth_rule = qse_httpd_callocmem (httpd, QSE_SIZEOF(*auth_rule));
			if (auth_rule == QSE_NULL)
			{
				qse_printf (QSE_T("ERROR: memory failure in copying auth-rule\n"));
				return -1;
			}

			auth_rule->type = type;
			if (pair->alias)
			{
				auth_rule->spec = qse_httpd_strtombsdup (httpd, pair->alias);
				if (!auth_rule->spec)
				{
					qse_httpd_freemem (httpd, auth_rule);
					qse_printf (QSE_T("ERROR: memory failure in copying auth-rule\n"));
					return -1;
				}
			}

			auth_rule->noauth = 0;
			if (qse_strxcmp (((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len, QSE_T("noauth")) == 0) auth_rule->noauth = 1;

			if (cfg->auth_rule[type].tail)
				cfg->auth_rule[type].tail->next = auth_rule;
			else
				cfg->auth_rule[type].head = auth_rule;
			cfg->auth_rule[type].tail = auth_rule;
		}
	}

	/* TODO: support multiple auth entries  */

	return 0;
}

static int load_loccfg_mime (qse_httpd_t* httpd, qse_xli_t* xli, qse_xli_list_t* list, loccfg_t* cfg)
{
	qse_xli_pair_t* pair;
	qse_xli_atom_t* atom;

	pair = qse_xli_findpair (xli, list, QSE_T("mime"));
	if (!pair) pair = qse_xli_findpair (xli, QSE_NULL, QSE_T("server-default.mime"));
	if (pair && pair->val->type == QSE_XLI_LIST)
	{
		qse_xli_list_t* mimelist = (qse_xli_list_t*)pair->val;
		for (atom = mimelist->head; atom; atom = atom->next)
		{
			struct mime_t* mime;
			int type;

			if (atom->type != QSE_XLI_PAIR) continue;

			pair = (qse_xli_pair_t*)atom;

			if (qse_strcmp (pair->key, QSE_T("prefix")) == 0) type = MIME_PREFIX;
			else if (qse_strcmp (pair->key, QSE_T("suffix")) == 0) type = MIME_SUFFIX;
			else if (qse_strcmp (pair->key, QSE_T("name")) == 0) type = MIME_NAME;
			else if (qse_strcmp (pair->key, QSE_T("other")) == 0) type = MIME_OTHER;
			else continue;

			mime = qse_httpd_callocmem (httpd, QSE_SIZEOF(*mime));
			if (mime == QSE_NULL)
			{
				qse_printf (QSE_T("ERROR: memory failure in copying mime\n"));
				return -1;
			}

			mime->type = type;
			mime->spec = qse_httpd_strtombsdup (httpd, pair->alias);
			if (!mime->spec)
			{
				qse_httpd_freemem (httpd, mime);
				qse_printf (QSE_T("ERROR: memory failure in copying mime\n"));
				return -1;
			}

			mime->value = qse_httpd_strntombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len);
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

	return 0;
}

static int load_loccfg_access (qse_httpd_t* httpd, qse_xli_t* xli, qse_xli_list_t* list, loccfg_t* cfg)
{
	/* local access items */
	static struct 
	{
		const qse_char_t* x;
		const qse_char_t* y;
	} loc_acc_items[] = 
	{
		{ QSE_T("dir-access"), QSE_T("server-default.dir-access") },
		{ QSE_T("file-access"), QSE_T("server-default.file-access") }
	};

	int i;
	qse_xli_pair_t* pair;
	qse_xli_atom_t* atom;

	for (i = 0; i < 2;  i++)
	{
		pair = qse_xli_findpair (xli, list, loc_acc_items[i].x);
		if (!pair) pair = qse_xli_findpair (xli, QSE_NULL, loc_acc_items[i].y);
		if (pair && pair->val->type == QSE_XLI_LIST)
		{
			qse_xli_list_t* acclist = (qse_xli_list_t*)pair->val;
			for (atom = acclist->head; atom; atom = atom->next)
			{
				struct access_t* acc;
				const qse_char_t* tmp;
				qse_size_t len;
				int type, value;

				if (atom->type != QSE_XLI_PAIR) continue;
	
				pair = (qse_xli_pair_t*)atom;
	
				if (qse_strcmp (pair->key, QSE_T("prefix")) == 0) type = ACCESS_PREFIX;
				else if (qse_strcmp (pair->key, QSE_T("suffix")) == 0) type = ACCESS_SUFFIX;
				else if (qse_strcmp (pair->key, QSE_T("name")) == 0) type = ACCESS_NAME;
				else if (qse_strcmp (pair->key, QSE_T("other")) == 0) type = ACCESS_OTHER;
				else continue;

				tmp = ((qse_xli_str_t*)pair->val)->ptr;
				len = ((qse_xli_str_t*)pair->val)->len;
				if (qse_strxcmp (tmp, len, QSE_T("noent")) == 0) value = 404;
				else if (qse_strxcmp (tmp, len, QSE_T("forbid")) == 0) value = 403;
				else if (qse_strxcmp (tmp, len, QSE_T("ok")) == 0) value = 200;
				else continue;
				/* TODO: more sanity check */

				acc = qse_httpd_callocmem (httpd, QSE_SIZEOF(*acc));
				if (acc == QSE_NULL)
				{
					qse_printf (QSE_T("ERROR: memory failure in copying acc\n"));
					return -1;
				}

				acc->type = type;
				if (pair->alias)
				{
					acc->spec = qse_httpd_strtombsdup (httpd, pair->alias);
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

	return 0;
}

static int load_loccfg_proxy (qse_httpd_t* httpd, qse_xli_t* xli, qse_xli_list_t* list, loccfg_t* cfg)
{
	qse_xli_pair_t* pair;
	qse_xli_list_t* proxy = QSE_NULL;
	qse_xli_list_t* default_proxy = QSE_NULL;

	pair = qse_xli_findpair (xli, list, QSE_T("proxy"));
	if (pair) 
	{
		QSE_ASSERT (pair->val->type == QSE_XLI_LIST);
		proxy = (qse_xli_list_t*)pair->val;
	}

	pair = qse_xli_findpair (xli, QSE_NULL, QSE_T("server-default.proxy"));
	if (pair)
	{
		QSE_ASSERT (pair->val->type == QSE_XLI_LIST);
		default_proxy = (qse_xli_list_t*)pair->val;
	}

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("http")); /* server.host[].location[].proxy.http */
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("http")); /* server-default.proxy.http */
	if (pair) cfg->proxy.allow_http = get_boolean ((qse_xli_str_t*)pair->val);

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("https")); /* server.host[].location[].proxy.https */
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("https")); /* server-default.proxy.https */
	if (pair) cfg->proxy.allow_https = get_boolean ((qse_xli_str_t*)pair->val);

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("connect"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("connect"));
	if (pair) cfg->proxy.allow_connect = get_boolean ((qse_xli_str_t*)pair->val);

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("intercept"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("intercept"));
	if (pair) cfg->proxy.allow_intercept = get_intercept ((qse_xli_str_t*)pair->val);

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("upgrade"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("upgrade"));
	if (pair) cfg->proxy.allow_upgrade = get_boolean ((qse_xli_str_t*)pair->val);

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("x-forwarded"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("x-forwarded"));
	if (pair) cfg->proxy.x_forwarded = get_boolean ((qse_xli_str_t*)pair->val);

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("pseudonym"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("pseudonym"));
	if (pair) 
	{
		qse_xli_str_t* str = (qse_xli_str_t*)pair->val;

	#if defined(QSE_CHAR_IS_MCHAR)
		qse_mbsxcpy (cfg->proxy.pseudonym, QSE_COUNTOF(cfg->proxy.pseudonym), str->ptr);
	#else
		qse_size_t wcslen, mbslen = QSE_COUNTOF(cfg->proxy.pseudonym);
		if (qse_wcstombs (str->ptr, &wcslen, cfg->proxy.pseudonym, &mbslen) <= -1)
		{
			qse_printf (QSE_T("ERROR: invalid pseudonym for proxy - %s"), str->ptr);
			return -1;
		}
	#endif
	}

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("dns-enabled"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("dns-enabled"));
	if (pair) cfg->proxy.dns_enabled = get_boolean ((qse_xli_str_t*)pair->val);

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("dns-server"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("dns-server"));
	if (pair) 
	{
		qse_xli_str_t* str = (qse_xli_str_t*)pair->val;
		if (qse_strtonwad (str->ptr, &cfg->proxy.dns_nwad) <= -1)
		{
			qse_printf (QSE_T("ERROR: invalid address for proxy dns - %s"), str->ptr);
			return -1;
		}
	}

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("dns-timeout"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("dns-timeout"));
	if (pair) cfg->proxy.dns_timeout = get_integer ((qse_xli_str_t*)pair->val);
	else cfg->proxy.dns_timeout = -1;

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("dns-retries"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("dns-retries"));
	if (pair) cfg->proxy.dns_retries = get_integer ((qse_xli_str_t*)pair->val);
	else cfg->proxy.dns_retries = -1;

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("dns-queries"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("dns-queries"));
	if (pair) 
	{
		cfg->proxy.dns_queries = parse_dns_query_types (httpd, (qse_xli_str_t*)pair->val);
		if (cfg->proxy.dns_queries <= -1) return -1;
	}
	else cfg->proxy.dns_queries = QSE_HTTPD_DNS_SERVER_A | QSE_HTTPD_DNS_SERVER_AAAA;

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("dns-preresolve-hook"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("dns-preresolve-hook"));
	if (pair) 
	{
		cfg->proxy.dns_preresolve_mod = qse_httpd_findmod (httpd, ((qse_xli_str_t*)pair->val)->ptr);
		if (!cfg->proxy.dns_preresolve_mod)
			qse_printf (QSE_T("WARNING: dns-preresolve-hook not found - %s\n"), ((qse_xli_str_t*)pair->val)->ptr); 
	}

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("urs-enabled"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("urs-enabled"));
	if (pair) cfg->proxy.urs_enabled = get_boolean ((qse_xli_str_t*)pair->val);

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("urs-server"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("urs-server"));
	if (pair)
	{
		qse_xli_str_t* str = (qse_xli_str_t*)pair->val;
		if (qse_strtonwad (str->ptr, &cfg->proxy.urs_nwad) <= -1)
		{
			qse_printf (QSE_T("ERROR: invalid address for proxy urs - %s"), str->ptr);
			return -1;
		}
	}

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("urs-timeout"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("urs-timeout"));
	if (pair) cfg->proxy.urs_timeout = get_integer ((qse_xli_str_t*)pair->val);
	else cfg->proxy.urs_timeout = -1;

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("urs-retries"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("urs-retries"));
	if (pair) cfg->proxy.urs_retries = get_integer ((qse_xli_str_t*)pair->val);
	else cfg->proxy.urs_retries = -1;

	pair = QSE_NULL;
	if (proxy) pair = qse_xli_findpair (xli, proxy, QSE_T("urs-prerewrite-hook"));
	if (!pair && default_proxy) pair = qse_xli_findpair (xli, default_proxy, QSE_T("urs-prerewrite-hook"));
	if (pair) 
	{
		cfg->proxy.urs_prerewrite_mod = qse_httpd_findmod (httpd, ((qse_xli_str_t*)pair->val)->ptr);
		if (!cfg->proxy.urs_prerewrite_mod)
			qse_printf (QSE_T("WARNING: urs-prerewrite-hook not found - %s\n"), ((qse_xli_str_t*)pair->val)->ptr); 
	}

	return 0;
}

static int load_loccfg (qse_httpd_t* httpd, qse_xli_t* xli, qse_xli_list_t* list, loccfg_t* cfg)
{
	/*httpd_xtn_t* httpd_xtn;

	httpd_xtn = qse_httpd_getxtnstd (httpd);*/

	if (load_loccfg_basic (httpd, xli, list, cfg) <= -1 ||
	    load_loccfg_index (httpd, xli, list, cfg) <= -1 ||
	    load_loccfg_cgi (httpd, xli, list, cfg) <= -1 ||
	    load_loccfg_authrule (httpd, xli, list, cfg) <= -1 ||
	    load_loccfg_mime (httpd, xli, list, cfg) <= -1 ||
	    load_loccfg_access (httpd, xli, list, cfg) <= -1 ||
	    load_loccfg_proxy (httpd, xli, list, cfg) <= -1) return -1;

#if 0
	/* TODO: perform more sanity check */
	if (qse_mbschr (cfg->xcfg[XCFG_AUTH], QSE_MT(':')) == QSE_NULL)
	{
		qse_printf (QSE_T("WARNING: no colon in the auth string - [%hs]\n"), cfg->xcfg[XCFG_AUTH]);
	}
#endif

	cfg->root_type = ROOT_TYPE_PATH; /* default type */

	if (cfg->xcfg[XCFG_ROOT])
	{
		/* check if the root value is special */
		const qse_mchar_t* root = cfg->xcfg[XCFG_ROOT];
		int proto_len = 0;

		if (root[0] == QSE_MT('<') && QSE_ISMDIGIT(root[1]))
		{
			int code = 0;

			root++;
			while (QSE_ISMDIGIT(*root))
			{
				code = code * 10 + (*root - QSE_MT('0'));
				root++;
			}

			if (code >= 400 && code <= 599 && root[0] == QSE_MT('>') && root[1] == QSE_MT('\0'))
			{
				cfg->root_type = ROOT_TYPE_ERROR;
				cfg->root.error_code = code;
				goto done;
			}

			if ((code == 301 || code == 302 || code == 303 || code == 307 || code == 308) && *root == QSE_MT('>'))
			{
				root++;
				if (QSE_ISMPRINT(*root))
				{
					cfg->root_type = ROOT_TYPE_RELOC;
					cfg->root.reloc.flags = 0;
					switch (code)
					{
						case 308:
							cfg->root.reloc.flags |= QSE_HTTPD_RSRC_RELOC_PERMANENT;
							/* fall thru */
						case 307:
							cfg->root.reloc.flags |= QSE_HTTPD_RSRC_RELOC_KEEPMETHOD;
							break;

						case 301:
							cfg->root.reloc.flags |= QSE_HTTPD_RSRC_RELOC_PERMANENT;
							break;
					}
					cfg->root.reloc.target = root;
					goto done;
				}
			}

			root = cfg->xcfg[XCFG_ROOT];
		}
		else
		{
			if (qse_mbszcasecmp (root, QSE_MT("http://"), (proto_len = 7)) == 0 ||
			    qse_mbszcasecmp (root, QSE_MT("https://"), (proto_len = 8)) == 0) 
			{
				root += proto_len;
			}
			else
			{
				proto_len = 0;
			}
		}

		if (qse_mbstonwad (root, &cfg->root.nwad) >= 0) 
		{
			if (cfg->root.nwad.type != QSE_NWAD_IN4 && cfg->root.nwad.type != QSE_NWAD_IN6)
			{
				qse_printf (QSE_T("ERROR: invalid address for root - [%hs]\n"), cfg->xcfg[XCFG_ROOT]);
				return -1;
			}

			cfg->root_type = (proto_len == 8)? ROOT_TYPE_NWAD_SECURE: ROOT_TYPE_NWAD;
			goto done;
		}
		else if (proto_len > 0 && *root != QSE_MT('\0'))
		{
			/* it begins with http:// or https:// */
			cfg->root_type = (proto_len == 8)? ROOT_TYPE_HOST_SECURE: ROOT_TYPE_HOST;
			cfg->root.host = root;
			goto done;
		}
	}

done:
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

	static struct
	{
		const qse_char_t* x;
		const qse_char_t* y;
	} scfg_items[] =
	{
		{ QSE_T("ssl-cert-file"),  QSE_T("server-default.ssl-cert-file") },
		{ QSE_T("ssl-key-file"),   QSE_T("server-default.ssl-key-file") }
	};

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

		pair = qse_xli_findpair (httpd_xtn->xli, list, scfg_items[i].x);
		if (!pair) pair = qse_xli_findpair (httpd_xtn->xli, QSE_NULL, scfg_items[i].y);
		if (pair && pair->val->type == QSE_XLI_STR)
		{
			server_xtn->scfg[i] = qse_httpd_strntombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len);
			if (server_xtn->scfg[i] == QSE_NULL) 
			{
				/*qse_printf (QSE_T("ERROR in copying - %s\n"), qse_httpd_geterrmsg (httpd));*/
				qse_printf (QSE_T("ERROR in copying\n"));
				return -1;
			}
		}
	}

	/* load host/location specific configuration */
	host_count = qse_xli_countpairs (httpd_xtn->xli, list, QSE_T("host"));
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

		qse_strxfmt (buf, QSE_COUNTOF(buf), QSE_T("host[%d]"), i);
		host = qse_xli_findpair (httpd_xtn->xli, list, buf);
		if (!host) break;

		if (host->val->type == QSE_XLI_LIST && host->alias) 
		{
			loc_count = qse_xli_countpairs (httpd_xtn->xli, (qse_xli_list_t*)host->val, QSE_T("location"));

			if (((hostcfg = qse_httpd_callocmem (httpd, QSE_SIZEOF(*hostcfg))) == QSE_NULL) ||
			    ((hostcfg->hostname = qse_httpd_strtombsdup (httpd, (host->alias[0] == QSE_T('\0')? QSE_T("*"):host->alias))) == QSE_NULL)) goto oops;
			
			for (j = loc_count; j > 0; )
			{
				qse_xli_pair_t* loc;

				j--;

				qse_strxfmt (buf, QSE_COUNTOF(buf), QSE_T("location[%d]"), j);
				loc = qse_xli_findpair (httpd_xtn->xli, (qse_xli_list_t*)host->val, buf);
				if (!loc) break;

				if (loc->val->type == QSE_XLI_LIST && loc->alias) 
				{
					loccfg = qse_httpd_callocmem (httpd, QSE_SIZEOF(*loccfg));
					if (loccfg == QSE_NULL) goto oops;

					/* just chain it before loading the actual data */
					loccfg->next = hostcfg->loccfg;
					hostcfg->loccfg = loccfg;

					/* load the data now */
					if (load_loccfg (httpd,  httpd_xtn->xli, (qse_xli_list_t*)loc->val, loccfg) <= -1) goto oops;

					/* clone the location name  */
					loccfg->locname.ptr = qse_httpd_strtombsdup (httpd, 
						(loc->alias[0] == QSE_T('\0')? QSE_T("/"): loc->alias));
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

	pair = qse_xli_findpair (httpd_xtn->xli, list, QSE_T("bind"));
	if (pair == QSE_NULL || pair->val->type != QSE_XLI_STR)
	{
		/* TOOD: logging */
		qse_printf (QSE_T("WARNING: no value or invalid value specified for bind\n"));
		return QSE_NULL;
	}

	qse_memset (&dope, 0, QSE_SIZEOF(dope));
	if (qse_strntonwad (((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len, &dope.nwad) <= -1)
	{
		/*  TOOD: logging */
		qse_printf (QSE_T("WARNING: invalid value for bind - %s\n"), ((qse_xli_str_t*)pair->val)->ptr);
		return QSE_NULL;
	}

	pair = qse_xli_findpair (httpd_xtn->xli, list, QSE_T("ssl"));
	if (!pair) pair = qse_xli_findpair (httpd_xtn->xli, QSE_NULL, QSE_T("server-default.ssl"));
	if (pair && pair->val->type == QSE_XLI_STR && 
	    qse_strxcmp (((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len, QSE_T("yes")) == 0) dope.flags |= QSE_HTTPD_SERVER_SECURE;	

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

static int load_hook_modules (qse_httpd_t* httpd, qse_xli_list_t* hook_list)
{
	qse_char_t buf[32];
	qse_xli_pair_t* file, * cfg, * mod;
	qse_httpd_mod_t* module;
	httpd_xtn_t* httpd_xtn;
	
	int i;

	httpd_xtn = qse_httpd_getxtnstd (httpd);

	for (i = 0; ; i++)
	{
		qse_strxfmt (buf, QSE_COUNTOF(buf), QSE_T("module[%d]"), i);
		mod = qse_xli_findpair (httpd_xtn->xli, hook_list, buf);
		if (mod == QSE_NULL) break;

		file = qse_xli_findpair (httpd_xtn->xli, (qse_xli_list_t*)mod->val, QSE_T("file"));
		if (file == QSE_NULL)
		{
			/* TODO: log warning when file is not found in module */
		}
		else
		{
			module = qse_httpd_loadmod (httpd, ((qse_xli_str_t*)file->val)->ptr);
			if (!module)
			{
				/* TODO: better error handling and logging */
				qse_printf (QSE_T("WARNING: failed to load module [%s]\n"), ((qse_xli_str_t*)file->val)->ptr);
			}
			else
			{
				cfg = qse_xli_findpair (httpd_xtn->xli, (qse_xli_list_t*)mod->val, QSE_T("config"));
				if (cfg)
				{
					if (!module->config)
					{
						qse_printf (QSE_T("WARNING: unneeded configuration for [%s]. no configuration handler\n"), 
							((qse_xli_str_t*)file->val)->ptr);
					}
					else
					{
						const qse_xli_atom_t* atom;
						const qse_xli_pair_t* pair;
						int x;

						for (atom = ((qse_xli_list_t*)(cfg->val))->head; atom; atom = atom->next)
						{
							if (atom->type != QSE_XLI_PAIR) continue;
							pair = (qse_xli_pair_t*)atom;

							if (pair->val->type != QSE_XLI_STR) continue;
							x = qse_httpd_configmod (httpd, module, pair->key, ((qse_xli_str_t*)pair->val)->ptr);
							if (x <= -1)
							{
								qse_printf (QSE_T("WARNING: failed to set module configuration [%s] to [%s] for [%s]\n"), 
									pair->key, ((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)file->val)->ptr);
							}
							else if (x == 0)
							{
								qse_printf (QSE_T("WARNING: invalid module configuration item [%s] for [%s]\n"), 
									pair->key, ((qse_xli_str_t*)file->val)->ptr);
							}
						}
					}
				}
			}
		}
	}

	return i;
}

static int open_config_file (qse_httpd_t* httpd)
{
	httpd_xtn_t* httpd_xtn;
	qse_xli_iostd_t xli_in;
	int trait, i;

	static struct
	{
		const qse_char_t* name;
		qse_xli_scm_t scm;
	} defs[] =
	{
		{ QSE_T("name"),                                             { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("max-nofile"),                                       { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("max-nproc"),                                        { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },

		{ QSE_T("hooks"),                                            { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("hooks.module"),                                     { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYALIAS, 0, 0      }  },
		{ QSE_T("hooks.module.file"),                                { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("hooks.module.config"),                              { QSE_XLI_SCM_VALLIST |
		                                                               QSE_XLI_SCM_KEYNODUP | QSE_XLI_SCM_VALIFFY, 0, 0      }  },

		{ QSE_T("server-default"),                                   { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server-default.ssl-cert-file"),                     { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.ssl-key-file"),                      { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.root"),                              { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.realm"),                             { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 0, 1      }  },
		{ QSE_T("server-default.auth"),                              { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 0, 1      }  },
		{ QSE_T("server-default.index"),                             { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 0xFFFF }  },
		{ QSE_T("server-default.auth-rule"),                         { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server-default.auth-rule.prefix"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.auth-rule.suffix"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.auth-rule.name"),                    { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.auth-rule.other"),                   { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.cgi"),                               { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server-default.cgi.prefix"),                        { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 0, 2      }  },
		{ QSE_T("server-default.cgi.suffix"),                        { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 0, 2      }  },
		{ QSE_T("server-default.cgi.name"),                          { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 0, 2      }  },
		{ QSE_T("server-default.mime"),                              { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server-default.mime.prefix"),                       { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.mime.suffix"),                       { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.mime.name"),                         { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.mime.other"),                        { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.dir-access"),                        { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server-default.dir-access.prefix"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.dir-access.suffix"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.dir-access.name"),                   { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.dir-access.other"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.file-access"),                       { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server-default.file-access.prefix"),                { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.file-access.suffix"),                { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.file-access.name"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server-default.file-access.other"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.dir-head"),                          { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.dir-foot"),                          { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.error-head"),                        { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.error-foot"),                        { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy"),                             { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server-default.proxy.http"),                        { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.https"),                       { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.connect"),                     { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.intercept"),                   { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.upgrade"),                     { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.x-forwarded"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.pseudonym"),                   { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.dns-enabled"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.dns-server"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.dns-timeout"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.dns-retries"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.dns-queries"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 0, 0xFFFF }  },
		{ QSE_T("server-default.proxy.dns-preresolve-hook"),         { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.urs-enabled"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.urs-server"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.urs-timeout"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.urs-retries"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server-default.proxy.urs-prerewrite-hook"),         { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },

		{ QSE_T("server"),                                           { QSE_XLI_SCM_VALLIST,                        0, 0      }  },
		{ QSE_T("server.bind"),                                      { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.ssl"),                                       { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.ssl-cert-file"),                             { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.ssl-key-file"),                              { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host"),                                      { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYALIAS, 0, 0      }  },
		{ QSE_T("server.host.location"),                             { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYALIAS, 0, 0      }  },
		{ QSE_T("server.host.location.root"),                        { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 0, 1      }  },
		{ QSE_T("server.host.location.realm"),                       { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 0, 1      }  },
		{ QSE_T("server.host.location.auth"),                        { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.index"),                       { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 0xFFFF }  },
		{ QSE_T("server.host.location.auth-rule"),                   { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server.host.location.auth-rule.prefix"),            { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.auth-rule.suffix"),            { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.auth-rule.name"),              { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.auth-rule.other"),             { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.cgi"),                         { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server.host.location.cgi.prefix"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 0, 2      }  },
		{ QSE_T("server.host.location.cgi.suffix"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 0, 2      }  },
		{ QSE_T("server.host.location.cgi.name"),                    { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 0, 2      }  },
		{ QSE_T("server.host.location.mime"),                        { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server.host.location.mime.prefix"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.mime.suffix"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.mime.name"),                   { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.mime.other"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.dir-access"),                  { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server.host.location.dir-access.prefix"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.dir-access.suffix"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.dir-access.name"),             { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.dir-access.other"),            { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.file-access"),                 { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server.host.location.file-access.prefix"),          { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.file-access.suffix"),          { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.file-access.name"),            { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYALIAS, 1, 1      }  },
		{ QSE_T("server.host.location.file-access.other"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.dir-head"),                    { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.dir-foot"),                    { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.error-head"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.error-foot"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy"),                       { QSE_XLI_SCM_VALLIST | QSE_XLI_SCM_KEYNODUP, 0, 0      }  },
		{ QSE_T("server.host.location.proxy.http"),                  { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.https"),                 { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.connect"),               { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.intercept"),             { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.upgrade"),               { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.x-forwarded"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.pseudonym"),             { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.dns-enabled"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.dns-server"),            { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.dns-timeout"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.dns-retries"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.dns-queries"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 0, 0xFFFF }  },
		{ QSE_T("server.host.location.proxy.dns-preresolve-hook"),   { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.urs-enabled"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.urs-server"),            { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.urs-timeout"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.urs-retries"),           { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  },
		{ QSE_T("server.host.location.proxy.urs-prerewrite-hook"),   { QSE_XLI_SCM_VALSTR  | QSE_XLI_SCM_KEYNODUP, 1, 1      }  }
	};


	httpd_xtn = (httpd_xtn_t*) qse_httpd_getxtnstd (httpd);
	QSE_ASSERT (httpd_xtn->xli == QSE_NULL);

	httpd_xtn->xli = qse_xli_openstd (0, 0);
	if (httpd_xtn->xli == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open xli\n"));
		return -1;
	}
 
	qse_xli_getopt (httpd_xtn->xli, QSE_XLI_TRAIT, &trait);
	trait |= QSE_XLI_MULSEGSTR | QSE_XLI_VALIDATE;
	qse_xli_setopt (httpd_xtn->xli, QSE_XLI_TRAIT, &trait);

	for (i = 0; i < QSE_COUNTOF(defs); i++)
	{
		if (qse_xli_definepair (httpd_xtn->xli, defs[i].name, &defs[i].scm) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("Cannot define %s - %s\n"), defs[i].name, qse_xli_geterrmsg(httpd_xtn->xli));
			qse_xli_close (httpd_xtn->xli);
			httpd_xtn->xli = QSE_NULL;
			return -1;
		}
	}

	xli_in.type = QSE_XLI_IOSTD_FILE;
	xli_in.u.file.path = httpd_xtn->cfgfile;
	xli_in.u.file.cmgr = QSE_NULL;

	if (qse_xli_readstd (httpd_xtn->xli, &xli_in) <= -1)
	{
		const qse_xli_loc_t* errloc;

		errloc = qse_xli_geterrloc (httpd_xtn->xli);

		if (errloc->line > 0 || errloc->colm > 0)
		{
			qse_fprintf (QSE_STDERR, QSE_T("Cannot load %s at line %lu column %lu - %s\n"), 
				xli_in.u.file.path, (unsigned long int)errloc->line, (unsigned long int)errloc->colm, qse_xli_geterrmsg(httpd_xtn->xli));
		}
		else
		{
			qse_fprintf (QSE_STDERR, QSE_T("Cannot load %s - %s\n"), xli_in.u.file.path, qse_xli_geterrmsg(httpd_xtn->xli));
		}

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

static void set_limit (qse_httpd_t* httpd, const qse_char_t* name, int what)
{
	qse_xli_pair_t* pair;
	httpd_xtn_t* httpd_xtn;

	httpd_xtn = (httpd_xtn_t*)qse_httpd_getxtnstd (httpd);

	pair = qse_xli_findpair (httpd_xtn->xli, QSE_NULL, name);
	if (pair)
	{
#if defined(HAVE_GETRLIMIT) && defined(HAVE_SETRLIMIT)
		struct rlimit lim;

		QSE_ASSERT (pair->val->type == QSE_XLI_STR);

		if (getrlimit (what, &lim) == 0)
		{
			const qse_char_t* str;
			qse_size_t len;

			str = ((qse_xli_str_t*)pair->val)->ptr;
			len = ((qse_xli_str_t*)pair->val)->len;
			if (qse_strxcmp (str, len, QSE_T("none")) != 0)
			{
				if (qse_strxcmp (str, len, QSE_T("unlimited")) == 0)
					lim.rlim_cur = RLIM_INFINITY;
				else
					lim.rlim_cur = qse_strtoui (((qse_xli_str_t*)pair->val)->ptr, 10);
				if (setrlimit (what, &lim) <= -1)
				{
					/* TODO: warning */
				}
			}
		}
#endif
	}
}

static int load_config (qse_httpd_t* httpd)
{
	qse_xli_pair_t* pair;
	httpd_xtn_t* httpd_xtn;
	int i;

	httpd_xtn = (httpd_xtn_t*)qse_httpd_getxtnstd (httpd);

	if (open_config_file (httpd) <= -1) goto oops;

	pair = qse_xli_findpair (httpd_xtn->xli, QSE_NULL, QSE_T("name"));
	if (pair)
	{
		qse_mchar_t* tmp;

		QSE_ASSERT (pair->val->type == QSE_XLI_STR);
		tmp = qse_httpd_strntombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len);
		if (tmp) qse_httpd_setname (httpd, tmp);
		qse_httpd_freemem (httpd, tmp);
	}

#if defined(RLIMIT_NOFILE)
	set_limit (httpd, QSE_T("max-nofile"), RLIMIT_NOFILE);
#endif
#if defined(RLIMIT_NPROC)
	set_limit (httpd, QSE_T("max-nproc"), RLIMIT_NPROC);
#endif

	pair = qse_xli_findpair (httpd_xtn->xli, QSE_NULL, QSE_T("hooks"));
	if (pair) load_hook_modules (httpd, (qse_xli_list_t*)pair->val);

	for (i = 0; ; i++)
	{
		qse_char_t buf[32];
		qse_strxfmt (buf, QSE_COUNTOF(buf), QSE_T("server[%d]"), i);
		pair = qse_xli_findpair (httpd_xtn->xli, QSE_NULL, buf);
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
	pair = qse_xli_findpair (httpd_xtn->xli, QSE_NULL, QSE_T("server-default"));
	if (pair && pair->val->type == QSE_XLI_LIST)
	{
		if (load_loccfg (httpd, httpd_xtn->xli, (qse_xli_list_t*)pair->val, &httpd_xtn->dflcfg) <=  -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("failed to load configuration from %s\n"), httpd_xtn->cfgfile);
			goto oops;
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

	/* reconfigure the server when the server is impeded in sig_reconf(). */

	httpd_xtn = qse_httpd_getxtnstd (httpd);
	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	if (httpd_xtn->xli)
	{
		qse_char_t buf[32];
		qse_strxfmt (buf, QSE_COUNTOF(buf), QSE_T("server[%d]"), server_xtn->num);
		pair = qse_xli_findpair (httpd_xtn->xli, QSE_NULL, buf);

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

	/* reconfigure the server when the server is impeded in sig_reconf(). */

	httpd_xtn = qse_httpd_getxtnstd (httpd);
	if (httpd_xtn->impede_code == 9999)
	{
		qse_httpd_stop (httpd);
	}
	else
	{
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
	}

	/* chain-call the orignal impedence function */
	if (httpd_xtn->org_impede) httpd_xtn->org_impede (httpd);
}

static int prerewrite_url (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req, const qse_mchar_t* host, qse_mchar_t** url)
{
	httpd_xtn_t* httpd_xtn;

	httpd_xtn = qse_httpd_getxtnstd (httpd);

printf ("PREREWRITING.....................\n");
/*
	if (qse_htre_getqmethodtype(req) == QSE_HTTP_CONNECT) 
	{
		*url = QSE_NULL;
		return 0;
	}
*/

/* TODO: override prerewrite url */
	return httpd_xtn->org_urs_prerewrite (httpd, client, req, host, url);
}

static void logact_httpd (qse_httpd_t* httpd, const qse_httpd_act_t* act)
{
	/*httpd_xtn_t* httpd_xtn;*/
	qse_char_t tmp[128], tmp2[128], tmp3[128];

	/*httpd_xtn = qse_httpd_getxtnstd (httpd);*/

	switch (act->code)
	{
		case QSE_HTTPD_CATCH_MERRMSG:
			qse_printf (QSE_T("ERROR: %hs\n"), act->u.merrmsg);
			break;

		case QSE_HTTPD_CATCH_MWARNMSG:
			qse_printf (QSE_T("WARNING: %hs\n"), act->u.mwarnmsg);
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
	qse_fprintf (QSE_STDOUT, QSE_T("QSEHTTPD %hs\n"), QSE_PACKAGE_VERSION);
	qse_fprintf (QSE_STDOUT, QSE_T("Copyright 2006-2014 Chung, Hyung-Hwan\n"));
}

static void print_usage (qse_sio_t* out, int argc, qse_char_t* argv[])
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
	qse_httpd_scb_t scb;

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

	qse_httpd_getopt (httpd, QSE_HTTPD_SCB, &scb);
	httpd_xtn->org_urs_prerewrite = scb.urs.prerewrite;
	scb.urs.prerewrite = prerewrite_url;
	qse_httpd_setopt (httpd, QSE_HTTPD_SCB, &scb);

	qse_httpd_getopt (httpd, QSE_HTTPD_RCB, &rcb);
	httpd_xtn->org_impede = rcb.impede;
	rcb.impede = impede_httpd; /* executed when qse_httpd_impede() is called */
	if (g_debug) rcb.logact = logact_httpd; /* i don't remember old logging handler */
	qse_httpd_setopt (httpd, QSE_HTTPD_RCB, &rcb);

	ret = qse_httpd_loopstd (httpd, QSE_NULL, QSE_NULL);

	restore_signal_handlers ();
	g_httpd = QSE_NULL;

	if (ret <= -1) qse_fprintf (QSE_STDERR, QSE_T("Httpd error - %d\n"), qse_httpd_geterrnum (httpd));

oops:
	if (httpd) qse_httpd_close (httpd);
	return -1;
}

#if defined(__DOS__)
static void interrupt (*old_keyboard_handler)() = QSE_NULL;
static int impeded_for_keyboard = 0;
static void interrupt new_keyboard_handler (void)
{
	if (!impeded_for_keyboard && g_httpd) 
	{
		httpd_xtn_t* httpd_xtn;
		int c;

/* TODO: read a keystroke... etc */
		httpd_xtn = qse_httpd_getxtnstd (g_httpd);
		httpd_xtn->impede_code = 9999;
		qse_httpd_impede (g_httpd);
		impeded_for_keyboard = 1;
	}

	if (old_keyboard_handler) old_keyboard_handler ();
}
#endif


int qse_main (int argc, qse_achar_t* argv[])
{
	int ret;

#if defined(_WIN32)
	char locale[100];
	UINT codepage;
	WSADATA wsadata;
#elif defined(__DOS__)
	extern BOOL _watt_do_exit;
#else
	/* nothing */
#endif

#if defined(_WIN32)

	codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		/* .codepage */
		qse_fmtuintmaxtombs (locale, QSE_COUNTOF(locale),
			codepage, 10, -1, QSE_MT('\0'), QSE_MT("."));
		setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}

#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	qse_openstdsios ();

#if defined(__DOS__)
	old_keyboard_handler = _dos_getvect (0x09);
	_dos_setvect (0x09, new_keyboard_handler);
#endif

#if defined(_WIN32)
	if (WSAStartup (MAKEWORD(2,0), &wsadata) != 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Failed to start up winsock\n"));
		ret = -1;
		goto oops;
	}
#elif defined(__DOS__)
	

	_watt_do_exit = 0; /* prevent sock_init from exiting upon failure */
	if (sock_init () != 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Failed to initialize watt-32\n"));
		ret = -1;
		goto oops;
	}

	/*trace2com_init (1, 38400);*/
#endif

#if defined(HAVE_SSL)    
	SSL_load_error_strings ();
	SSL_library_init ();
#endif

	ret = qse_runmain (argc, argv, httpd_main);

#if defined(HAVE_SSL)
	/* ERR_remove_state() should be called for each thread if the application is thread */
	ERR_remove_state (0); 
	#if defined(HAVE_ENGINE_CLEANUP)
	ENGINE_cleanup ();
	#endif
	ERR_free_strings ();
	EVP_cleanup ();
	#if defined(HAVE_CRYPTO_CLEANUP_ALL_EX_DATA)
	CRYPTO_cleanup_all_ex_data ();
	#endif
#endif

#if defined(_WIN32)
	WSACleanup ();
#elif defined(__DOS__)
	sock_exit ();
#endif

oops:
#if defined(__DOS__)
	if (old_keyboard_handler) 
		_dos_setvect (0x09, old_keyboard_handler);
#endif
	qse_closestdsios ();
	return ret;
}
