
#include <qse/net/httpd.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>

#include <signal.h>
#include <locale.h>
#include <string.h>
#if	defined(_WIN32)
#	include <windows.h>
#else
#	include <unistd.h>
#	include <errno.h>
#	include <fcntl.h>
#	include <sys/stat.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <sys/epoll.h>
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/engine.h>

/* ------------------------------------------------------------------- */

#define MAX_SEND_SIZE 4096
#if defined(HAVE_SYS_SENDFILE_H)
#	include <sys/sendfile.h>
#endif

/* TODO: WIN32 TransmitFile */
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
static qse_httpd_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_HTTPD_ENOMEM;

		case EINVAL:
			return QSE_HTTPD_EINVAL;

		case EACCES:
			return QSE_HTTPD_EACCES;

		case ENOENT:
			return QSE_HTTPD_ENOENT;

		case EEXIST:
			return QSE_HTTPD_EEXIST;
	
		case EINTR:
			return QSE_HTTPD_EINTR;

		case EAGAIN:
		/*case EWOULDBLOCK:*/
			return QSE_HTTPD_EAGAIN;

		default:
			return QSE_HTTPD_ESYSERR;
	}
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

static int sockaddr_to_nwad (
	const struct sockaddr_storage* addr, qse_nwad_t* nwad)
{
	int addrsize = -1;

	switch (addr->ss_family)
	{
		case AF_INET:
		{
			struct sockaddr_in* in; 
			in = (struct sockaddr_in*)addr;
			addrsize = QSE_SIZEOF(*in);

			memset (nwad, 0, QSE_SIZEOF(*nwad));
			nwad->type = QSE_NWAD_IN4;
			nwad->u.in4.addr.value = in->sin_addr.s_addr;
			nwad->u.in4.port = in->sin_port;
			break;
		}

#if defined(AF_INET6)
		case AF_INET6:
		{
			struct sockaddr_in6* in; 
			in = (struct sockaddr_in6*)addr;
			addrsize = QSE_SIZEOF(*in);

			memset (nwad, 0, QSE_SIZEOF(*nwad));
			nwad->type = QSE_NWAD_IN6;
			memcpy (&nwad->u.in6.addr, &in->sin6_addr, QSE_SIZEOF(nwad->u.in6.addr));
			nwad->u.in6.scope = in->sin6_scope_id;
			nwad->u.in6.port = in->sin6_port;
			break;
		}
#endif
	}

	return addrsize;
}

static int nwad_to_sockaddr (
	const qse_nwad_t* nwad, struct sockaddr_storage* addr)
{
	int addrsize = -1;

	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
		{
			struct sockaddr_in* in; 

			in = (struct sockaddr_in*)addr;
			addrsize = QSE_SIZEOF(*in);
			memset (in, 0, addrsize);

			in->sin_family = AF_INET;
			in->sin_addr.s_addr = nwad->u.in4.addr.value;
			in->sin_port = nwad->u.in4.port;
			break;
		}

		case QSE_NWAD_IN6:
		{
#if defined(AF_INET6)
			struct sockaddr_in6* in; 

			in = (struct sockaddr_in6*)addr;
			addrsize = QSE_SIZEOF(*in);
			memset (in, 0, addrsize);

			in->sin6_family = AF_INET6;
			memcpy (&in->sin6_addr, &nwad->u.in6.addr, QSE_SIZEOF(nwad->u.in6.addr));
			in->sin6_scope_id = nwad->u.in6.scope;
			in->sin6_port = nwad->u.in6.port;
#endif
			break;
		}
	}

	return addrsize;
}

static int server_open (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	int fd = -1, flag;
/* TODO: if AF_INET6 is not defined sockaddr_storage is not available...
 * create your own union or somehting similar... */
	struct sockaddr_storage addr;
	int addrsize;

	addrsize = nwad_to_sockaddr (&server->nwad, &addr);
	if (addrsize <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	}

	fd = socket (addr.ss_family, SOCK_STREAM, IPPROTO_TCP);
	if (fd <= -1) goto oops;

	flag = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &flag, QSE_SIZEOF(flag));

	/* Solaris 8 returns EINVAL if QSE_SIZEOF(addr) is passed in as the 
	 * address size for AF_INET. */
	/*if (bind (s, (struct sockaddr*)&addr, QSE_SIZEOF(addr)) <= -1) goto oops_esocket;*/
	if (bind (fd, (struct sockaddr*)&addr, addrsize) <= -1) goto oops;
	if (listen (fd, 10) <= -1) goto oops;

	flag = fcntl (fd, F_GETFL);
	if (flag >= 0) fcntl (fd, F_SETFL, flag | O_NONBLOCK);

	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);

	server->handle.i = fd;
	return 0;

oops:
	qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
	if (fd >= 0) close (fd);
	return -1;
}

static void server_close (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	close (server->handle.i);
}

static int server_accept (
	qse_httpd_t* httpd, 
	qse_httpd_server_t* server, qse_httpd_client_t* client)
{
	struct sockaddr_storage addr;

#ifdef HAVE_SOCKLEN_T
	socklen_t addrlen;
#else
	int addrlen;
#endif
	int fd, flag;

	addrlen = QSE_SIZEOF(addr);
	fd = accept (server->handle.i, (struct sockaddr*)&addr, &addrlen);
	if (fd <= -1) 
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum (errno));
		return -1;
	}

#if 0
	if (fd >= FD_SETSIZE)
	{
qse_fprintf (QSE_STDERR, QSE_T("Error: too many client?\n"));
		/*TODO: qse_httpd_seterrnum (httpd, QSE_HTTPD_EXXXXX);*/
		close (fd);
		return -1;
	}
#endif

	flag = fcntl (fd, F_GETFL);
	if (flag >= 0) fcntl (fd, F_SETFL, flag | O_NONBLOCK);

	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);

	if (sockaddr_to_nwad (&addr, &client->remote_addr) <= -1)
	{
/* TODO: logging */
          client->remote_addr = server->nwad;
	}

	addrlen = QSE_SIZEOF(addr);
	if (getsockname (fd, (struct sockaddr*)&addr, &addrlen) <= -1 ||
	    sockaddr_to_nwad (&addr, &client->local_addr) <= -1)
	{
/* TODO: logging */
          client->local_addr = server->nwad;
	}
		
	client->handle.i = fd;
	return 0;
}

/* ------------------------------------------------------------------- */

struct mux_ev_t
{
	qse_ubi_t handle;
	int reqmask;
	qse_httpd_muxcb_t cbfun;
	void* cbarg;
	struct mux_ee_t* next;
};

struct mux_t
{
	int fd;

	struct
	{
		struct epoll_event* ptr;
		qse_size_t len;	
		qse_size_t capa;
	} ee;
};

static void* mux_open (qse_httpd_t* httpd)
{
	struct mux_t* mux;

	mux = qse_httpd_allocmem (httpd, QSE_SIZEOF(*mux));
	if (mux == QSE_NULL) return QSE_NULL;

	memset (mux, 0, QSE_SIZEOF(*mux));

	mux->fd = epoll_create (100);
	if (mux->fd <= -1) 
	{
		qse_httpd_freemem (httpd, mux);
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return QSE_NULL;
	}

	return mux;
}

static void mux_close (qse_httpd_t* httpd, void* vmux)
{
	struct mux_t* mux = (struct mux_t*)vmux;
	if (mux->ee.ptr) qse_httpd_freemem (httpd, mux->ee.ptr);
	close (mux->fd);
	qse_httpd_freemem (httpd, mux);
}

static int mux_addhnd (
	qse_httpd_t* httpd, void* vmux, qse_ubi_t handle, 
	int mask, qse_httpd_muxcb_t cbfun, void* cbarg)
{
	struct mux_t* mux = (struct mux_t*)vmux;
	struct epoll_event ev;
	struct mux_ev_t* mev;

	ev.events = 0;
	if (mask & QSE_HTTPD_MUX_READ) ev.events |= EPOLLIN;
	if (mask & QSE_HTTPD_MUX_WRITE) ev.events |= EPOLLOUT;

	if (ev.events == 0)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		return -1;
	}

	mev = qse_httpd_allocmem (httpd, QSE_SIZEOF(*mev));
	if (mev == QSE_NULL) return -1;

	if (mux->ee.len >= mux->ee.capa)
	{
		struct epoll_event* tmp;

		tmp = qse_httpd_reallocmem (
			httpd, mux->ee.ptr, 
			QSE_SIZEOF(*mux->ee.ptr) * (mux->ee.capa + 1) * 2);
		if (tmp == QSE_NULL)
		{
			qse_httpd_freemem (httpd, mev);
			return -1;
		}

		mux->ee.ptr = tmp;
		mux->ee.capa = (mux->ee.capa + 1) * 2;
	}

	mev->handle = handle;
	mev->reqmask = mask;
	mev->cbfun = cbfun;
	mev->cbarg = cbarg;

	ev.data.ptr = mev;

	if (epoll_ctl (mux->fd, EPOLL_CTL_ADD, handle.i, &ev) <= -1)
	{
		/* don't rollback ee.ptr */
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		qse_httpd_freemem (httpd, mev);
		return -1;
	}

	mux->ee.len++;
	return 0;
}

static int mux_delhnd (qse_httpd_t* httpd, void* vmux, qse_ubi_t handle)
{
	struct mux_t* mux = (struct mux_t*)vmux;

	if (epoll_ctl (mux->fd, EPOLL_CTL_DEL, handle.i, QSE_NULL) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

	mux->ee.len--;
	return 0;
}

static int mux_poll (qse_httpd_t* httpd, void* vmux, qse_ntime_t timeout)
{
	struct mux_t* mux = (struct mux_t*)vmux;
	struct mux_ev_t* mev;
	int mask, nfds, i;

	nfds = epoll_wait (mux->fd, mux->ee.ptr, mux->ee.len, timeout);
	if (nfds <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

	for (i = 0; i < nfds; i++)
	{
		mev = mux->ee.ptr[i].data.ptr;

		mask = 0;

		if (mux->ee.ptr[i].events & EPOLLIN) mask |= QSE_HTTPD_MUX_READ;
		if (mux->ee.ptr[i].events & EPOLLOUT) mask |= QSE_HTTPD_MUX_WRITE;

		if (mux->ee.ptr[i].events & EPOLLHUP) 
		{
			if (mev->reqmask & QSE_HTTPD_MUX_READ) mask |= QSE_HTTPD_MUX_READ;
			if (mev->reqmask & QSE_HTTPD_MUX_WRITE) mask |= QSE_HTTPD_MUX_WRITE;
		}

		mev->cbfun (httpd, mux, mev->handle, mask, mev->cbarg);

//if (cbfun fails and the client is deleted???) other pending events should also be dropped???
	}
	return 0;
}

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

static int file_executable (qse_httpd_t* httpd, const qse_mchar_t* path)
{
	if (access (path, X_OK) == -1)
		return (errno == EACCES)? 0 /*no*/: -1 /*error*/;
	return 1; /* yes */
}

static int file_stat (
	qse_httpd_t* httpd, const qse_mchar_t* path, qse_httpd_stat_t* hst)
{
	struct stat st;

/* TODO: lstat? or stat? */
	if (stat (path, &st) <= -1)
     {
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
     }    

	/* stating for a file. it should be a regular file. 
	 * i don't allow other file types. */
	if (!S_ISREG(st.st_mode))
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EACCES);
		return -1;
	}

	memset (hst, 0, QSE_SIZEOF(*hst));

	hst->size = st.st_size;
#if defined(HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)
	hst->mtime = QSE_SECNSEC_TO_MSEC(st.st_mtim.tv_sec,st.st_mtim.tv_nsec);
#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC)
	hst->mtime = QSE_SECNSEC_TO_MSEC(st.st_mtimespec.tv_sec,st.st_mtimespec.tv_nsec);
#else
	hst->mtime = st.st_mtime * QSE_MSECS_PER_SEC;
#endif

	hst->mime = qse_mbsend (path, QSE_MT(".html"))? QSE_MT("text/html"):
	            qse_mbsend (path, QSE_MT(".txt"))?  QSE_MT("text/plain"):
	            qse_mbsend (path, QSE_MT(".jpg"))?  QSE_MT("image/jpeg"):
	            qse_mbsend (path, QSE_MT(".mp4"))?  QSE_MT("video/mp4"):
	            qse_mbsend (path, QSE_MT(".mp3"))?  QSE_MT("audio/mpeg"): QSE_NULL;
	return 0;
}

static int file_ropen (
	qse_httpd_t* httpd, const qse_mchar_t* path, qse_ubi_t* handle)
{
	int fd;
	int flags;

	flags = O_RDONLY;
#if defined(O_LARGEFILE)
	flags |= O_LARGEFILE;
#endif

qse_printf (QSE_T("opening file [%hs] for reading\n"), path);
	fd = open (path, flags, 0);
	if (fd <= -1) 
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

     flags = fcntl (fd, F_GETFD);
     if (flags >= 0) fcntl (fd, F_SETFD, flags | FD_CLOEXEC);

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
	if (fd <= -1) 
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

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
static void client_close (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	close (client->handle.i);
}

static void client_shutdown (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
#if defined(SHUT_RDWR)
	shutdown (client->handle.i, SHUT_RDWR);
#else
	shutdown (client->handle.i, 2);
#endif
}
	
static qse_ssize_t client_recv (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_mchar_t* buf, qse_size_t bufsize)
{
	if (client->secure)
	{
		int ret = SSL_read (client->handle2.ptr, buf, bufsize);
		if (ret <= -1)
		{
			if (SSL_get_error(client->handle2.ptr,ret) == SSL_ERROR_WANT_READ) 
				qse_httpd_seterrnum (httpd, QSE_HTTPD_EAGAIN);
			else
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		}
		return ret;
	}
	else
	{
		ssize_t ret = read (client->handle.i, buf, bufsize);
		if (ret <= -1) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return ret;
	}
}

static qse_ssize_t client_send (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	const qse_mchar_t* buf, qse_size_t bufsize)
{
	if (client->secure)
	{
		int ret = SSL_write (client->handle2.ptr, buf, bufsize);
		if (ret <= -1)
		{
			if (SSL_get_error(client->handle2.ptr,ret) == SSL_ERROR_WANT_WRITE) 
				qse_httpd_seterrnum (httpd, QSE_HTTPD_EAGAIN);
			else
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		}
		return ret;
	}
	else
	{
		ssize_t ret = write (client->handle.i, buf, bufsize);
		if (ret <= -1) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return ret;
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
				const qse_mchar_t* auth;
				int authorized = 0;

				auth = qse_htre_getheaderval (req, QSE_MT("Authorization"));
				if (auth)
				{
					/* TODO: PERFORM authorization... */	
					/* BASE64 decode... */
					authorized = 1;
				}

				if (authorized)
				{
					/* nph-cgi */
					task = qse_httpd_entasknph (
						httpd, client, QSE_NULL, qpath, req);
				}
				else
				{
					task = qse_httpd_entaskauth (
						httpd, client, QSE_NULL, QSE_MT("Secure Area"), req);
				}
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

int list_directory (qse_httpd_t* httpd, const qse_mchar_t* path)
{
	return 404;
}

static qse_httpd_cbs_t httpd_cbs =
{
	/* server */
	{ server_open, server_close, server_accept },

	/* multiplexer */
	{ mux_open,
	  mux_close,
	  mux_addhnd,
	  mux_delhnd,
	  mux_poll,
	
	  mux_readable,
	  mux_writable 
	},

	/* file operation */
	{ file_executable,
	  file_stat,
	  file_ropen,
	  file_wopen,
	  file_close,
	  file_read,
	  file_write
	},

	/* client connection */
	{ client_close,
	  client_shutdown, 
	  client_recv, 
	  client_send, 
	  client_sendfile,
	  client_accepted, 
	  client_closed },

	/* http request */
	peek_request,
	handle_request,

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

	qse_httpd_setoption (httpd, QSE_HTTPD_CGIERRTONUL);
	ret = qse_httpd_loop (httpd, &httpd_cbs, 10000);

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

