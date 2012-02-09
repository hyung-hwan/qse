
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
#else
#	include <unistd.h>
#	include <errno.h>
#	include <fcntl.h>
#	include <sys/stat.h>
#endif

#include <openssl/ssl.h>


// TODO: remove this and export structured needed like qse_httpd_client_t
#include "../../lib/net/httpd.h"

/* ------------------------------------------------------------------- */

#define MAX_SEND_SIZE 4096
#if defined(HAVE_SYS_SENDFILE_H)
#	include <sys/sendfile.h>
#endif

#if defined(HAVE_SENDFILE) && defined(HAVE_SENDFILE64)
#	if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_SENDFILE64)
#		define xsendfile sendfile64
#	else
#		define xsendfile sendfile
#	endif
#elif defined(HAVE_SENDFILE)
#	define xsendfile sendfile
#elif defined(HAVE_SENDFILE64)
#	define xsendfile sendfile64
#elif defined(HAVE_SENDFILEV) || defined(HAVE_SENDFILEV64)

static qse_ssize_t xsendfile (
	int out_fd, int in_fd, qse_foff_t* offset, qse_size_t count)
{
#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_SENDFILE64)
	struct sendfilevec64 vec;
#else
	struct sendfilevec vec;
#endif
	size_t xfer;
	ssize_t n;

	vec.sfv_fd = in_fd;
	vec.sfv_flag = 0;
	if (offset)
	{
		vec.sfv_off = *offset; 
	}
	else
	{
		vec.sfv_off = lseek (in_fd, 0, SEEK_CUR); /* TODO: lseek64 or llseek.. */
		if (vec.sfv_off == (off_t)-1) return (qse_ssize_t)-1;
	}
	vec.sfv_len = count;

#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_SENDFILE64)
	n = sendfilev64 (out_fd, &vec, 1, &xfer);
#else
	n = sendfilev (out_fd, &vec, 1, &xfer);
#endif
	if (offset) *offset = *offset + xfer;

/* TODO: xfer contains number of byte written even on failure
on success xfer == n.
on failure xfer != n.
 */
	return n;
}

#else

static qse_ssize_t xsendfile (
	int out_fd, int in_fd, qse_foff_t* offset, qse_size_t count)
{
	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_ssize_t n;

	if (offset && lseek (in_fd, *offset, SEEK_SET) != *offset)  //* 64bit version of lseek...
		return (qse_ssize_t)-1;

	if (count > QSE_COUNTOF(buf)) count = QSE_COUNTOF(buf);
	n = read (in_fd, buf, count);
	if (n == (qse_ssize_t)-1 || n == 0) return n;

	n = send (out_fd, buf, n, 0);
	if (n > 0 && offset) *offset = *offset + n;

	return n;
}
#endif

static qse_ssize_t xsendfile_ssl (
	SSL* out, int in_fd, qse_foff_t* offset, qse_size_t count)
{
	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_ssize_t n;

	if (offset && lseek (in_fd, *offset, SEEK_SET) != *offset)  //* 64bit version of lseek...
		return (qse_ssize_t)-1;

	if (count > QSE_COUNTOF(buf)) count = QSE_COUNTOF(buf);
	n = read (in_fd, buf, count);
	if (n == (qse_ssize_t)-1 || n == 0) return n;

	n = SSL_write (out, buf, count);
	if (n > 0 && offset) *offset = *offset + n;

	return n;
}

/* ------------------------------------------------------------------- */
typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
	SSL_CTX* ssl_ctx;
};

/* ------------------------------------------------------------------- */

static int init_xtn_ssl (
	httpd_xtn_t* xtn, 
	const qse_mchar_t* pemfile, 
	const qse_mchar_t* keyfile/*,
	const qse_mchar_t* chainfile*/)
{
	SSL_CTX* ctx;

	SSL_library_init ();
	SSL_load_error_strings ();
	/*SSLeay_add_ssl_algorithms();*/

	ctx = SSL_CTX_new (SSLv23_server_method());
	if (ctx == QSE_NULL) return -1;

	/*SSL_CTX_set_info_callback(ctx,ssl_info_callback);*/

	if (SSL_CTX_use_certificate_file (ctx, pemfile, SSL_FILETYPE_PEM) == 0 ||
	    SSL_CTX_use_PrivateKey_file (ctx, keyfile, SSL_FILETYPE_PEM) == 0 ||
	    SSL_CTX_check_private_key (ctx) == 0 /*|| 
	    SSL_CTX_use_certificate_chain_file (ctx, chainfile) == 0*/)
	{
		qse_mchar_t buf[128];
		ERR_error_string_n(ERR_get_error(), buf, QSE_COUNTOF(buf));
		qse_fprintf (QSE_STDERR, QSE_T("Error: %hs\n"), buf);
		SSL_CTX_free (ctx);
		return -1;
	}


// TODO: CRYPTO_set_id_callback ();
// TODO: CRYPTO_set_locking_callback ();

	xtn->ssl_ctx = ctx;
	return 0;
}

static void fini_xtn_ssl (httpd_xtn_t* xtn)
{
// TODO: CRYPTO_set_id_callback (QSE_NULL);
// TODO: CRYPTO_set_locking_callback (QSE_NULL);
	SSL_CTX_free (xtn->ssl_ctx);


//	ERR_remove_state ();

	ENGINE_cleanup ();

	ERR_free_strings ();
	EVP_cleanup ();
	CRYPTO_cleanup_all_ex_data ();
}

/* ------------------------------------------------------------------- */

static int mux_readable (qse_httpd_t* httpd, qse_ubi_t handle, qse_ntoff_t msec)
{
	fd_set r;
	struct timeval tv, * tvp;

	if (msec >= 0)
	{
		tv.tv_sec = (msec / 1000);
		tv.tv_usec = ((msec % 1000) * 1000);
		tvp = &tv;
	}
	else tvp = QSE_NULL;

	FD_ZERO (&r);
	FD_SET (handle.i, &r);

	return select (handle.i + 1, &r, QSE_NULL, QSE_NULL, tvp); 
}

static int mux_writable (qse_httpd_t* httpd, qse_ubi_t handle, qse_ntoff_t msec)
{
	fd_set w;
	struct timeval tv, * tvp;

	if (msec >= 0)
	{
		tv.tv_sec = (msec / 1000);
		tv.tv_usec = ((msec % 1000) * 1000);
		tvp = &tv;
	}
	else tvp = QSE_NULL;

	FD_ZERO (&w);
	FD_SET (handle.i, &w);

	return select (handle.i + 1, QSE_NULL, &w, QSE_NULL, tvp);
}

/* ------------------------------------------------------------------- */

static int path_executable (qse_httpd_t* httpd, const qse_mchar_t* path)
{
	if (access (path, X_OK) == -1)
		return (errno == EACCES)? 0 /*no*/: -1 /*error*/;
	return 1; /* yes */
}

/* ------------------------------------------------------------------- */

static int file_ropen (
	qse_httpd_t* httpd, const qse_mchar_t* path, 
	qse_ubi_t* handle, qse_foff_t* size)
{
	int fd;
	int flags;
	struct stat st;

	flags = O_RDONLY;
#if defined(O_LARGEFILE)
	flags |= O_LARGEFILE;
#endif

qse_printf (QSE_T("opening file [%hs] for reading\n"), path);
	fd = open (path, flags, 0);
	if (fd <= -1) return -1;

     flags = fcntl (fd, F_GETFD);
     if (flags >= 0) fcntl (fd, F_SETFD, flags | FD_CLOEXEC);

/* TODO: fstat64??? */
	if (fstat (fd, &st) <= -1)
     {
		close (fd);
		return -1;
     }    

	if (S_ISDIR(st.st_mode))
	{
		close (fd);
		return -1;
	}

     *size = (st.st_size <= 0)? 0: st.st_size;
	handle->i = fd;
qse_printf (QSE_T("opened file %hs\n"), path);
	return 0;
}

static int file_wopen (
	qse_httpd_t* httpd, const qse_mchar_t* path, 
	qse_ubi_t* handle)
{
	int fd;
	int flags;

	flags = O_WRONLY | O_CREAT | O_TRUNC;
#if defined(O_LARGEFILE)
	flags |= O_LARGEFILE;
#endif

qse_printf (QSE_T("opening file [%hs] for writing\n"), path);
	fd = open (path, flags, 0644);
	if (fd <= -1) return -1;

	handle->i = fd;
	return 0;
}

static void file_close (qse_httpd_t* httpd, qse_ubi_t handle)
{
qse_printf (QSE_T("closing file %d\n"), handle.i);
	close (handle.i);
}

static qse_ssize_t file_read (
	qse_httpd_t* httpd, qse_ubi_t handle, 
	qse_mchar_t* buf, qse_size_t len)
{
	return read (handle.i, buf, len);
}

static qse_ssize_t file_write (
	qse_httpd_t* httpd, qse_ubi_t handle, 
	const qse_mchar_t* buf, qse_size_t len)
{
	return write (handle.i, buf, len);
}

/* ------------------------------------------------------------------- */
static qse_ssize_t client_recv (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_mchar_t* buf, qse_size_t bufsize)
{
	if (client->secure)
	{
		return SSL_read (client->handle2.ptr, buf, bufsize);
	}
	else
	{
		return read (client->handle.i, buf, bufsize);
	}
}

static qse_ssize_t client_send (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	const qse_mchar_t* buf, qse_size_t bufsize)
{
	if (client->secure)
	{
		return SSL_write (client->handle2.ptr, buf, bufsize);
	}
	else
	{
		return write (client->handle.i, buf, bufsize);
	}
}

static qse_ssize_t client_sendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_ubi_t handle, qse_foff_t* offset, qse_size_t count)
{
	if (client->secure)
	{
		return xsendfile_ssl (client->handle2.ptr, handle.i, offset, count);
	}
	else
	{
		return xsendfile (client->handle.i, handle.i, offset, count);
	}
}

static int client_accepted (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	httpd_xtn_t* xtn = (httpd_xtn_t*) qse_httpd_getxtn (httpd);

	if (client->secure)
	{
		int ret;
		SSL* ssl;

		if (client->handle2.ptr)
		{
			ssl = client->handle2.ptr;
		}
		else
		{
			ssl = SSL_new (xtn->ssl_ctx);
			if (ssl == QSE_NULL) return -1;

			client->handle2.ptr = ssl;

qse_printf (QSE_T("SSL ACCEPTING %d\n"), client->handle.i);
qse_fflush (QSE_STDOUT);
			if (SSL_set_fd (ssl, client->handle.i) == 0)
			{
				/* don't free ssl here since client_closed() 
				 * will be closed */
				return -1;
			}
		}

		ret = SSL_accept (ssl);
		if (ret <= 0)
		{
			if (SSL_get_error(ssl,ret) == SSL_ERROR_WANT_READ) 
			{
				/* handshaking isn't complete. */
				return 0;
			}

			qse_fprintf (QSE_STDERR, QSE_T("Error: SSL ACCEPT ERROR\n"));
			/* SSL_free (ssl); */
			return -1;
		}
	}

	return 1; /* accept completed */
}

static void client_closed (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	if (client->secure)
	{
		if (client->handle2.ptr)
		{
			SSL_shutdown ((SSL*)client->handle2.ptr); /* is this needed? */
			SSL_free ((SSL*)client->handle2.ptr);
		}
	}
}

/* ------------------------------------------------------------------- */
static qse_htb_walk_t walk (qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
{
qse_printf (QSE_T("HEADER OK %d[%hs] %d[%hs]\n"),  (int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)QSE_HTB_VLEN(pair), QSE_HTB_VPTR(pair));
	return QSE_HTB_WALK_FORWARD;
}

static int process_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req, int peek)
{
	int method;
	qse_httpd_task_t* task;
	int content_received;

	method = qse_htre_getqmethod(req);
	content_received = (qse_htre_getcontentlen(req) > 0);

qse_printf (QSE_T("================================\n"));
qse_printf (QSE_T("[%lu] %hs REQUEST ==> [%hs] version[%d.%d] method[%hs]\n"), 
	(unsigned long)time(NULL),
	(peek? QSE_MT("PEEK"): QSE_MT("HANDLE")),
     qse_htre_getqpathptr(req),
     qse_htre_getmajorversion(req),
     qse_htre_getminorversion(req),
	qse_htre_getqmethodname(req)
);
if (qse_htre_getqparamlen(req) > 0) qse_printf (QSE_T("PARAMS ==> [%hs]\n"), qse_htre_getqparamptr(req));
qse_htb_walk (&req->hdrtab, walk, QSE_NULL);
if (qse_htre_getcontentlen(req) > 0) 
{
	qse_printf (QSE_T("CONTENT before discard = [%.*S]\n"), (int)qse_htre_getcontentlen(req), qse_htre_getcontentptr(req));
}

	if (peek)
	{
		if (method != QSE_HTTP_POST && method != QSE_HTTP_PUT)
		{	
			/* i'll discard request contents if the method is none of 
			 * post and put */
			qse_httpd_discardcontent (httpd, req);
		}

		if (req->attr.expect && 
		    (req->version.major > 1 || 
		     (req->version.major == 1 && req->version.minor >= 1)) && 
		    !content_received)
		{
/* TODO: check method.... */
			/* "expect" in the header, version 1.1 or higher, 
			 * and no content received yet */
	
			if (qse_mbscasecmp(req->attr.expect, QSE_MT("100-continue")) != 0)
			{
				if (qse_httpd_entaskerror (
					httpd, client, QSE_NULL, 417, req) == QSE_NULL) return -1;
				if (qse_httpd_entaskdisconnect (
					httpd, client, QSE_NULL) == QSE_NULL) return -1;
			}
			else
			{
				/* TODO: determine if to return 100-continue or other errors */
				if (qse_httpd_entaskcontinue (
					httpd, client, QSE_NULL, req) == QSE_NULL) return -1;
			}
		}
	}

if (qse_htre_getcontentlen(req) > 0) 
{
	qse_printf (QSE_T("CONTENT after discard = [%.*S]\n"), (int)qse_htre_getcontentlen(req), qse_htre_getcontentptr(req));
}

	if (method == QSE_HTTP_GET || method == QSE_HTTP_POST)
	{
		const qse_mchar_t* qpath = qse_htre_getqpathptr(req);
		const qse_mchar_t* dot = qse_mbsrchr (qpath, QSE_MT('.'));

		if (dot && qse_mbscmp (dot, QSE_MT(".cgi")) == 0)
		{
			if (peek)
			{
				/* cgi */
				if (req->attr.chunked)
				{
qse_printf (QSE_T("chunked cgi... delaying until contents are received\n"));
				#if 0
					req->attr.keepalive = 0;
					task = qse_httpd_entaskerror (
						httpd, client, QSE_NULL, 411, req);
					/* 411 can't keep alive */
					if (task) qse_httpd_entaskdisconnect (httpd, client, QSE_NULL);
				#endif
				}
				else if (method == QSE_HTTP_POST && 
				         !req->attr.content_length_set)
				{
					req->attr.keepalive = 0;
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
			else
			{
				/* to support the chunked request,
				 * i need to wait until it's compelted and invoke cgi */
				if (req->attr.chunked)
				{
qse_printf (QSE_T("Entasking chunked CGI...\n"));
					task = qse_httpd_entaskcgi (
						httpd, client, QSE_NULL, qpath, req);
					if (task == QSE_NULL) goto oops;
				}
			}
			return 0;
		}
		else if (dot && qse_mbscmp (dot, QSE_MT(".nph")) == 0)
		{
			if (peek)
			{
				/* nph-cgi */
				task = qse_httpd_entasknph (
					httpd, client, QSE_NULL, qpath, req);
				if (task == QSE_NULL) goto oops;
			}
			return 0;
		}
		else
		{
			if (!peek)
			{
				/* file or directory */
				task = qse_httpd_entaskfile (
					httpd, client, QSE_NULL, qpath, req);
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

	if (!req->attr.keepalive)
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
	return 0; /* TODO: return failure??? */
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

const qse_mchar_t* get_mime_type (qse_httpd_t* httpd, const qse_mchar_t* path)
{
	if (qse_mbsend (path, QSE_MT(".html"))) return QSE_MT("text/html");
	if (qse_mbsend (path, QSE_MT(".txt"))) return QSE_MT("text/plain");
	if (qse_mbsend (path, QSE_MT(".jpg"))) return QSE_MT("image/jpeg");
	if (qse_mbsend (path, QSE_MT(".mp4"))) return QSE_MT("video/mp4");
	if (qse_mbsend (path, QSE_MT(".mp3"))) return QSE_MT("audio/mpeg");
	return QSE_NULL;
}

int list_directory (qse_httpd_t* httpd, const qse_mchar_t* path)
{
	return 404;
}

static qse_httpd_cbs_t httpd_cbs =
{
	/* multiplexer */
	{ mux_readable, mux_writable },

	/* path operation */
	{ path_executable },

	/* file operation */
	{ file_ropen,
	  file_wopen,
	  file_close,
	  file_read,
	  file_write
	},

	/* client connection */
	{ client_recv, 
	  client_send, 
	  client_sendfile,
	  client_accepted, 
	  client_closed },

	/* http request */
	peek_request,
	handle_request,

	get_mime_type,
	list_directory
};

static qse_httpd_t* g_httpd = QSE_NULL;

static void sigint (int sig)
{
	if (g_httpd) qse_httpd_stop (g_httpd);
}

int httpd_main (int argc, qse_char_t* argv[])
{
	qse_httpd_t* httpd = QSE_NULL;
	httpd_xtn_t* xtn;
	int ret = -1, i;
	int ssl_xtn_inited = 0;

	if (argc <= 1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <listener_uri> ...\n"), argv[0]);
		goto oops;
	}

	httpd = qse_httpd_open (QSE_MMGR_GETDFL(), QSE_SIZEOF(httpd_xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		goto oops;
	}

	xtn = (httpd_xtn_t*)qse_httpd_getxtn (httpd);

	if (init_xtn_ssl (xtn, "http01.pem", "http01.key") <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		goto oops;
	}
	ssl_xtn_inited = 1;

	for (i = 1; i < argc; i++)
	{
		if (qse_httpd_addlistener (httpd, argv[i]) <= -1)
		{
			qse_fprintf (QSE_STDERR, 	
				QSE_T("Failed to add httpd listener - %s\n"), argv[i]);
			goto oops;
		}
	}

	g_httpd = httpd;
	signal (SIGINT, sigint);
	signal (SIGPIPE, SIG_IGN);

	qse_httpd_setoption (httpd, QSE_HTTPD_CGIERRTONUL);
	ret = qse_httpd_loop (httpd, &httpd_cbs);

	signal (SIGINT, SIG_DFL);
	signal (SIGPIPE, SIG_DFL);
	g_httpd = QSE_NULL;

	if (ret <= -1) qse_fprintf (QSE_STDERR, QSE_T("Httpd error\n"));

oops:
	if (ssl_xtn_inited) fini_xtn_ssl (xtn);
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
		qse_setdflcmgr (qse_slmbcmgr);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgr (qse_slmbcmgr);
#endif
	return qse_runmain (argc, argv, httpd_main);
}

