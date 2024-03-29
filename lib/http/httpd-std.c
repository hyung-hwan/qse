/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/http/stdhttpd.h>
#include "httpd.h"
#include "../cmn/mem-prv.h"
#include <qse/cmn/hton.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/str.h>
#include <qse/cmn/uri.h>
#include <qse/cmn/alg.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/path.h>
#include <qse/si/dir.h>
#include <qse/si/fio.h>
#include <qse/si/sio.h>
#include <qse/si/mux.h>
#include <qse/si/nwif.h>
#include <qse/si/sck.h>

#define STAT_REG   1
#define STAT_DIR   2

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h> /* sockaddr_in6 */
#	include <windows.h>

#	define EPOCH_DIFF_YEARS (QSE_EPOCH_YEAR-QSE_EPOCH_YEAR_WIN)
#	define EPOCH_DIFF_DAYS  ((qse_long_t)EPOCH_DIFF_YEARS*365+EPOCH_DIFF_YEARS/4-3)
#	define EPOCH_DIFF_SECS  ((qse_long_t)EPOCH_DIFF_DAYS*24*60*60)
#	if defined(QSE_HAVE_CONFIG_H) && defined(QSE_ENABLE_LIBLTDL)
#		include <ltdl.h>
#		define USE_LTDL
#	endif

#	undef AF_UNIX

#elif defined(__OS2__)
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <sys/ioctl.h>
#	include <nerrno.h>
#	if defined(TCPV40HDRS)
#		include <sys/select.h>
#	else
#		include <unistd.h>
#	endif
#	define INCL_DOSERRORS
#	define INCL_DOSFILEMGR
#	define INCL_DOSMODULEMGR
#	include <os2.h>

#	undef AF_UNIX

#elif defined(__DOS__)

#	include <io.h>
#	include <direct.h>
#	include <errno.h>
#	include <tcp.h> /* watt-32 */
#	include <sys/ioctl.h> /* watt-32 */
#	include <sys/stat.h>

#	define memset QSE_MEMSET
#	define select select_s 
#	undef AF_UNIX

#else
#	include "../cmn/syscall.h"
#	include <sys/socket.h>
#	include <netinet/in.h>
#	if defined(HAVE_SYS_UN_H)
#		include <sys/un.h>
#	endif
#	if defined(QSE_SIZEOF_STRUCT_SOCKADDR_IN6) && (QSE_SIZEOF_STRUCT_SOCKADDR_IN6 <= 0)
#		undef AF_INET6
#	endif
#	if defined(QSE_SIZEOF_STRUCT_SOCKADDR_UN) && (QSE_SIZEOF_STRUCT_SOCKADDR_UN <= 0)
#		undef AF_UNIX
#	endif
#	if defined(HAVE_SYS_SENDFILE_H)
#		include <sys/sendfile.h>
#	endif
#	if defined(HAVE_SYS_EPOLL_H)
#		include <sys/epoll.h>
#	endif
#	if defined(HAVE_POLL_H)
#		include <poll.h>
#	endif
#	if defined(__linux__)
#		include <limits.h>
#		if defined(HAVE_LINUX_NETFILTER_IPV4_H)
#			include <linux/netfilter_ipv4.h> /* SO_ORIGINAL_DST */
#		endif
#		if !defined(SO_ORIGINAL_DST)
#			define SO_ORIGINAL_DST 80
#		endif
#		if !defined(IP_TRANSPARENT)
#			define IP_TRANSPARENT 19
#		endif
#		if !defined(SO_REUSEPORT)
#			define SO_REUSEPORT 15
#		endif
#	endif
#	if defined(HAVE_NETINET_SCTP_H)
#		include <netinet/sctp.h>
#	endif

#	include <unistd.h>

#	if defined(QSE_ENABLE_LIBLTDL)
#		include <ltdl.h>
#		define USE_LTDL
#	elif defined(HAVE_DLFCN_H)
#		include <dlfcn.h>
#		define USE_DLFCN
#	else
#		error UNSUPPORTED DYNAMIC LINKER
#	endif
#endif

#if defined(HAVE_OPENSSL_SSL_H) && defined(HAVE_SSL)
#	include <openssl/ssl.h>
#	if defined(HAVE_OPENSSL_ERR_H)
#		include <openssl/err.h>
#	endif
#	if defined(HAVE_OPENSSL_ENGINE_H)
#		include <openssl/engine.h>
#	endif
#	define USE_SSL
#endif


#define HANDLE_TO_FIO(x) ((qse_fio_t*)(x))
#define FIO_TO_HANDLE(x) ((qse_httpd_hnd_t)(x))

#if defined(USE_SSL)
#define HANDLE_TO_SSL(x) ((SSL*)(x))
#define SSL_TO_HANDLE(x) ((qse_httpd_hnd_t)(x))
#endif

typedef struct server_xtn_t server_xtn_t;
struct server_xtn_t
{
	qse_httpd_server_detach_t detach;
	qse_httpd_serverstd_query_t query;
	qse_httpd_serverstd_makersrc_t makersrc;
	qse_httpd_serverstd_freersrc_t freersrc;

#if defined(USE_SSL)
	SSL_CTX* ssl_ctx;
#endif

	/* temporary buffer to handle authorization */
	qse_mcstr_t auth;
};

static void set_httpd_callbacks (qse_httpd_t* httpd);

/* ------------------------------------------------------------------- */

#include "../cmn/syserr.h"
IMPLEMENT_SYSERR_TO_ERRNUM (httpd, HTTPD)

#if defined(_WIN32)
static qse_httpd_errnum_t skerr_to_errnum (DWORD e)
{
	switch (e)
	{
		case WSA_NOT_ENOUGH_MEMORY:
			return QSE_HTTPD_ENOMEM;

		case WSA_INVALID_PARAMETER:
		case WSA_INVALID_HANDLE:
			return QSE_HTTPD_EINVAL;

		case WSAEACCES:
			return QSE_HTTPD_EACCES;

		case WSAEINTR:
			return QSE_HTTPD_EINTR;

		case WSAEWOULDBLOCK:
			return QSE_HTTPD_EAGAIN;

		case WSAECONNREFUSED:
		case WSAENETUNREACH:
		case WSAEHOSTUNREACH:
		case WSAEHOSTDOWN:
			return QSE_HTTPD_ECONN;

		default:
			return QSE_HTTPD_ESYSERR;
	}
}

#define SKERR_TO_ERRNUM() skerr_to_errnum(WSAGetLastError())

#elif defined(__OS2__)
static qse_httpd_errnum_t skerr_to_errnum (int e)
{
	switch (e)
	{
	#if defined(SOCENOMEM)
		case SOCENOMEM:
			return QSE_HTTPD_ENOMEM;
	#endif

		case SOCEINVAL:
			return QSE_HTTPD_EINVAL;

		case SOCEACCES:
			return QSE_HTTPD_EACCES;

	#if defined(SOCENOENT)
		case SOCENOENT:
			return QSE_HTTPD_ENOENT;
	#endif

	#if defined(SOCEXIST)
		case SOCEEXIST:
			return QSE_HTTPD_EEXIST;
	#endif
	
		case SOCEINTR:
			return QSE_HTTPD_EINTR;

		case SOCEPIPE:
			return QSE_HTTPD_EPIPE;

		case SOCEAGAIN:
			return QSE_HTTPD_EAGAIN;

		case SOCECONNREFUSED:
		case SOCENETUNREACH:
		case SOCEHOSTUNREACH:
		case SOCEHOSTDOWN:
			return QSE_HTTPD_ECONN;

		default:
			return QSE_HTTPD_ESYSERR;
	}
}

#define SKERR_TO_ERRNUM() skerr_to_errnum(sock_errno())

#else
static qse_httpd_errnum_t skerr_to_errnum (int e)
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

		case EPIPE:
			return QSE_HTTPD_EPIPE;

#if defined(EAGAIN) || defined(EWOULDBLOCK)

	#if defined(EAGAIN) && defined(EWOULDBLOCK)
		case EAGAIN:
		#if (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
		#endif
	#elif defined(EAGAIN)
		case EAGAIN:
	#else
		case EWOULDBLOCK;
	#endif
			return QSE_HTTPD_EAGAIN;
#endif

#if defined(ECONNREFUSED) || defined(ENETUNREACH) || defined(EHOSTUNREACH) || defined(EHOSTDOWN)
	#if defined(ECONNREFUSED) 
		case ECONNREFUSED:
	#endif
	#if defined(ENETUNREACH) 
		case ENETUNREACH:
	#endif
	#if defined(EHOSTUNREACH) 
		case EHOSTUNREACH:
	#endif
	#if defined(EHOSTDOWN) 
		case EHOSTDOWN:
	#endif
			return QSE_HTTPD_ECONN;
#endif

		default:
			return QSE_HTTPD_ESYSERR;
	}
}

#define SKERR_TO_ERRNUM() skerr_to_errnum(errno)

#endif


static qse_httpd_errnum_t muxerr_to_errnum (qse_mux_errnum_t e)
{
	switch (e)
	{
		case QSE_MUX_ENOMEM:
			return QSE_HTTPD_ENOMEM;

		case QSE_MUX_EINVAL:
			return QSE_HTTPD_EINVAL;

		case QSE_MUX_EACCES:
			return QSE_HTTPD_EACCES;

		case QSE_MUX_ENOENT:
			return QSE_HTTPD_ENOENT;

		case QSE_MUX_EEXIST:
			return QSE_HTTPD_EEXIST;

		case QSE_MUX_EINTR:
			return QSE_HTTPD_EINTR;

		case QSE_MUX_EPIPE:
			return QSE_HTTPD_EPIPE;

		case QSE_MUX_EAGAIN:
			return QSE_HTTPD_EAGAIN;

		default:
			return QSE_HTTPD_ESYSERR;
	}
}

static qse_httpd_errnum_t fioerr_to_errnum (qse_fio_errnum_t e)
{
	switch (e)
	{
		case QSE_FIO_ENOMEM:
			return QSE_HTTPD_ENOMEM;

		case QSE_FIO_EINVAL:
			return QSE_HTTPD_EINVAL;

		case QSE_FIO_EACCES:
			return QSE_HTTPD_EACCES;

		case QSE_FIO_ENOENT:
			return QSE_HTTPD_ENOENT;

		case QSE_FIO_EEXIST:
			return QSE_HTTPD_EEXIST;

		case QSE_FIO_EINTR:
			return QSE_HTTPD_EINTR;

		case QSE_FIO_EPIPE:
			return QSE_HTTPD_EPIPE;

		case QSE_FIO_EAGAIN:
			return QSE_HTTPD_EAGAIN;

		default:
			return QSE_HTTPD_ESYSERR;
	}
}

static qse_httpd_errnum_t direrr_to_errnum (qse_dir_errnum_t e)
{
	switch (e)
	{
		case QSE_DIR_ENOMEM:
			return QSE_HTTPD_ENOMEM;

		case QSE_DIR_EINVAL:
			return QSE_HTTPD_EINVAL;

		case QSE_DIR_EACCES:
			return QSE_HTTPD_EACCES;

		case QSE_DIR_ENOENT:
			return QSE_HTTPD_ENOENT;

		case QSE_DIR_EEXIST:
			return QSE_HTTPD_EEXIST;

		case QSE_DIR_EINTR:
			return QSE_HTTPD_EINTR;

		case QSE_DIR_EPIPE:
			return QSE_HTTPD_EPIPE;

		case QSE_DIR_EAGAIN:
			return QSE_HTTPD_EAGAIN;

		default:
			return QSE_HTTPD_ESYSERR;
	}
}

/* ------------------------------------------------------------------- */

static QSE_INLINE qse_ssize_t __send_file (
	qse_httpd_t* httpd, qse_sck_hnd_t out_fd, qse_httpd_hnd_t in_fd, 
	qse_foff_t* offset, qse_size_t count)
{
	/* TODO: os2 warp 4.5 has send_file. support it??? load it dynamically??? */

#if defined(HAVE_SENDFILE) && defined(HAVE_SENDFILE64)

	qse_ssize_t ret;
	qse_fio_hnd_t fh;

	fh = qse_fio_gethnd (HANDLE_TO_FIO(in_fd));

	#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_SENDFILE64)
	ret =  sendfile64 (out_fd, fh, offset, count);
	#else
	ret =  sendfile (out_fd, fh, offset, count);
	#endif
	if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	return ret;

#elif defined(HAVE_SENDFILE)

	qse_ssize_t ret;
	qse_fio_hnd_t fh;

	fh = qse_fio_gethnd (HANDLE_TO_FIO(in_fd));
	#if defined(__FreeBSD__)
	{
		off_t nsent;
		ret = sendfile (fh, out_fd, *offset, count, QSE_NULL, &nsent, 0);
		if (ret == 0) 
		{
			*offset += nsent;
			ret = nsent;	
		}
		else qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	}
	#else
	ret = sendfile (out_fd, fh, offset, count);
	if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	#endif
	return ret;

#elif defined(HAVE_SENDFILE64)

	qse_ssize_t ret;
	qse_fio_hnd_t fh;

	fh = qse_fio_gethnd (HANDLE_TO_FIO(in_fd));
	ret = sendfile64 (out_fd, fh, offset, count);
	if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	return ret;

#elif defined(HAVE_SENDFILEV) || defined(HAVE_SENDFILEV64)

	/* solaris */

	#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_SENDFILE64)
	struct sendfilevec64 vec;
	#else
	struct sendfilevec vec;
	#endif
	size_t xfer;
	ssize_t ret;
	qse_fio_hnd_t fh;

	fh = qse_fio_gethnd (HANDLE_TO_FIO(in_fd));

	vec.sfv_fd = fh;
	vec.sfv_flag = 0;
	if (offset)
	{
		vec.sfv_off = *offset;
	}
	else
	{
		vec.sfv_off = QSE_LSEEK (fh, 0, SEEK_CUR); 
		if (vec.sfv_off == (off_t)-1) return (qse_ssize_t)-1;
	}
	vec.sfv_len = count;

	#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_SENDFILE64)
	ret = sendfilev64 (out_fd, &vec, 1, &xfer);
	#else
	ret = sendfilev (out_fd, &vec, 1, &xfer);
	#endif

	if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	else if (offset) *offset = *offset + xfer;

/* TODO: xfer contains number of byte written even on failure
on success xfer == ret.
on failure xfer != ret.
 */
	return ret;

#else

	qse_mchar_t buf[MAX_SEND_SIZE]; /* TODO: move this into client, server, or httpd */
	qse_ssize_t ret;
	qse_foff_t foff;

	if (offset && (foff = qse_fio_seek (HANDLE_TO_FIO(in_fd), *offset, QSE_FIO_BEGIN)) != *offset)  
	{
		if (foff == (qse_foff_t)-1)
			qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(HANDLE_TO_FIO(in_fd))));
		else
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		return (qse_ssize_t)-1;
	}

	if (count > QSE_COUNTOF(buf)) count = QSE_COUNTOF(buf);
	ret = qse_fio_read (HANDLE_TO_FIO(in_fd), buf, count);
	if (ret > 0)
	{
		ret = send (out_fd, buf, ret, 0);
		if (ret > 0)
		{
			if (offset) *offset = *offset + ret;
		}
		else if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	}
	else if (ret <= -1)
	{
		qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(HANDLE_TO_FIO(in_fd))));
	}

	return ret;

#endif
}

/* ------------------------------------------------------------------- */

static QSE_INLINE qse_ssize_t __send_file_ssl (
	qse_httpd_t* httpd, void* xout, qse_httpd_hnd_t in_fd, 
	qse_foff_t* offset, qse_size_t count)
{
#if defined(USE_SSL)
	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_ssize_t ret;
	qse_foff_t foff;
	SSL* out = HANDLE_TO_SSL((qse_sck_hnd_t)xout);
	
	if (offset && (foff = qse_fio_seek (HANDLE_TO_FIO(in_fd), *offset, QSE_FIO_BEGIN)) != *offset)  
	{
		if (foff == (qse_foff_t)-1)
			qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(HANDLE_TO_FIO(in_fd))));
		else
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		return (qse_ssize_t)-1;
	}

	if (count > QSE_COUNTOF(buf)) count = QSE_COUNTOF(buf);
	ret = qse_fio_read (HANDLE_TO_FIO(in_fd), buf, count);
	if (ret > 0)
	{
		ret = SSL_write (out, buf, count);
		if (ret > 0)
		{
			if (offset) *offset = *offset + ret;
		}
		else if (ret <= -1)
		{
			int err = SSL_get_error(out, ret);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
				qse_httpd_seterrnum (httpd, QSE_HTTPD_EAGAIN);
			else
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		}
	}
	else if (ret <= -1)
	{
		qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(HANDLE_TO_FIO(in_fd))));
	}

	return ret;
#else
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#endif
}

/* ------------------------------------------------------------------- */

typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
#if defined(USE_SSL)
	SSL_CTX* ssl_peer_ctx;
#endif
	qse_httpd_ecb_t ecb;
	qse_httpd_dnsstd_t dns;
	qse_httpd_ursstd_t urs;
};

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE httpd_xtn_t* GET_HTTPD_XTN(qse_httpd_t* httpd) { return (httpd_xtn_t*)((qse_uint8_t*)qse_httpd_getxtn(httpd) - QSE_SIZEOF(httpd_xtn_t)); }
static QSE_INLINE server_xtn_t* GET_SERVER_XTN(qse_httpd_t* httpd,qse_httpd_server_t* server) { return (server_xtn_t*)((qse_uint8_t*)qse_httpd_getserverxtn(httpd, server) - QSE_SIZEOF(server_xtn_t)); }
#else
#define GET_HTTPD_XTN(httpd) ((httpd_xtn_t*)((qse_uint8_t*)qse_httpd_getxtn(httpd) - QSE_SIZEOF(httpd_xtn_t)))
#define GET_SERVER_XTN(httpd,server) ((server_xtn_t*)((qse_uint8_t*)qse_httpd_getserverxtn(httpd, server) - QSE_SIZEOF(server_xtn_t)))
#endif


#if defined(USE_SSL)
static int init_server_ssl (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	SSL_CTX* ssl_ctx = QSE_NULL;
	server_xtn_t* server_xtn = GET_SERVER_XTN(httpd, server);
	qse_httpd_serverstd_ssl_t ssl;

	if (server_xtn->query (httpd, server, QSE_HTTPD_SERVERSTD_SSL, QSE_NULL, &ssl) <= -1)
	{
		goto oops;
	}

	if (ssl.certfile == QSE_NULL || ssl.keyfile == QSE_NULL)
	{
		httpd->errnum = QSE_HTTPD_EINVAL;
		goto oops;
	}

	ssl_ctx = SSL_CTX_new (SSLv23_server_method());
	if (!ssl_ctx) 
	{
		httpd->errnum = QSE_HTTPD_ESYSERR;
		goto oops;
	}

	/*SSL_CTX_set_info_callback(ctx,ssl_info_callback);*/

	if (SSL_CTX_use_certificate_file (ssl_ctx, ssl.certfile, SSL_FILETYPE_PEM) == 0 ||
	    SSL_CTX_use_PrivateKey_file (ssl_ctx, ssl.keyfile, SSL_FILETYPE_PEM) == 0 ||
	    SSL_CTX_check_private_key (ssl_ctx) == 0 /*||
	    SSL_CTX_use_certificate_chain_file (ssl_ctx, chainfile) == 0*/)
	{
		if (httpd->opt.trait & QSE_HTTPD_LOGACT)
		{
			qse_httpd_act_t msg;
			qse_size_t len;
			msg.code = QSE_HTTPD_CATCH_MERRMSG;
			len = qse_mbscpy (msg.u.merrmsg, QSE_MT("cert/key file error - "));
			ERR_error_string_n (ERR_get_error(), &msg.u.merrmsg[len], QSE_COUNTOF(msg.u.merrmsg) - len);
			httpd->opt.rcb.logact (httpd, &msg);
		}

		httpd->errnum = QSE_HTTPD_ESYSERR; /* TODO: define a better error code */
		goto oops;
	}

	/* TODO: SSL_CTX_set_verify(); SSL_CTX_set_verify_depth() */
	/* TODO: CRYPTO_set_id_callback (); */
	/* TODO: CRYPTO_set_locking_callback (); */
	SSL_CTX_set_read_ahead (ssl_ctx, 0);

	server_xtn->ssl_ctx = ssl_ctx;

	return 0;

oops:
	if (ssl_ctx) SSL_CTX_free (ssl_ctx);
	return -1;
}

static void fini_server_ssl (server_xtn_t* xtn)
{
	/* TODO: CRYPTO_set_id_callback (QSE_NULL); */
	/* TODO: CRYPTO_set_locking_callback (QSE_NULL); */
	SSL_CTX_free (xtn->ssl_ctx);
}

static int init_xtn_peer_ssl (qse_httpd_t* httpd)
{
	SSL_CTX* peer_ctx = QSE_NULL;
	httpd_xtn_t* xtn = GET_HTTPD_XTN(httpd);

	peer_ctx = SSL_CTX_new (SSLv23_client_method());
	if (!peer_ctx) 
	{
		httpd->errnum = QSE_HTTPD_ESYSERR;
		goto oops;
	}

	xtn->ssl_peer_ctx = peer_ctx;
	return 0;

oops:
	if (peer_ctx) SSL_CTX_free (peer_ctx);
	return -1;
}

static void fini_xtn_peer_ssl (httpd_xtn_t* xtn)
{
	/* TODO: CRYPTO_set_id_callback (QSE_NULL); */
	/* TODO: CRYPTO_set_locking_callback (QSE_NULL); */
	SSL_CTX_free (xtn->ssl_peer_ctx);
}
#endif

/* ------------------------------------------------------------------- */

static void cleanup_standard_httpd (qse_httpd_t* httpd)
{
	httpd_xtn_t* xtn = GET_HTTPD_XTN(httpd);

#if defined(USE_SSL)
	if (xtn->ssl_peer_ctx) fini_xtn_peer_ssl (xtn);
#endif

#if defined(USE_LTDL)
	lt_dlexit ();
#endif
}

qse_httpd_t* qse_httpd_openstd (qse_size_t xtnsize, qse_httpd_errnum_t* errnum)
{
	return qse_httpd_openstdwithmmgr (QSE_MMGR_GETDFL(), xtnsize, errnum);
}

qse_httpd_t* qse_httpd_openstdwithmmgr (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_httpd_errnum_t* errnum)
{
	qse_httpd_t* httpd = QSE_NULL;
	httpd_xtn_t* xtn = QSE_NULL;
#if defined(USE_LTDL)
	int lt_dlinited = 0;
#endif

	httpd = qse_httpd_open (mmgr, QSE_SIZEOF(httpd_xtn_t) + xtnsize, errnum);
	if (httpd == QSE_NULL) goto oops;

	httpd->_instsize += QSE_SIZEOF(httpd_xtn_t);

	xtn = GET_HTTPD_XTN(httpd);
	/* the extension area has been cleared in qse_httpd_open().
	 * QSE_MEMSET (xtn, 0, QSE_SIZEOF(*xtn));*/

#if defined(USE_LTDL)
	/* lt_dlinit() can be called more than once and 
	 * lt_dlexit() shuts down libltdl if it's called as many times as
	 * corresponding lt_dlinit(). so it's safe to call lt_dlinit()
	 * and lt_dlexit() at the library level. */
	if (lt_dlinit () != 0) 
	{
		if (errnum) *errnum = QSE_HTTPD_ESYSERR;
		goto oops;
	}
	lt_dlinited = 1;
#elif defined(USE_DLFCN)
	/* don't care about failure */
	if (qse_httpd_setopt (httpd, QSE_HTTPD_MODPOSTFIX, QSE_T(".so")) <= -1)  
	{
		if (errnum) *errnum = qse_httpd_geterrnum(httpd);
		goto oops;
	}
#endif


#if defined(USE_SSL)
	if (init_xtn_peer_ssl (httpd) <= -1) 
	{
		if (errnum) *errnum = qse_httpd_geterrnum(httpd);
		goto oops;
	}
#endif

	set_httpd_callbacks (httpd);

	xtn->ecb.close = cleanup_standard_httpd;
	qse_httpd_pushecb (httpd, &xtn->ecb);

	return httpd;

oops:
#if defined(USE_SSL)
	if (xtn && xtn->ssl_peer_ctx) fini_xtn_peer_ssl (xtn);
#endif
#if defined(USE_LTDL)
	if (lt_dlinited) lt_dlexit ();
#endif
	if (httpd) qse_httpd_close (httpd);
	return QSE_NULL;
}

/* ------------------------------------------------------------------- */

static qse_sck_hnd_t open_client_socket (qse_httpd_t* httpd, int domain, int type, int proto)
{
	qse_sck_hnd_t fd;
	int flag;

	fd = socket (domain, type, proto);
	if (!qse_is_sck_valid(fd)) 
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		goto oops;
	}

	#if defined(FD_CLOEXEC)
	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);
	#endif

	#if defined(SO_REUSEADDR)
	flag = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (void*)&flag, QSE_SIZEOF(flag));
	#endif

	/* client socket may not need this 
	#if defined(SO_REUSEPORT)
	flag = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEPORT, (void*)&flag, QSE_SIZEOF(flag));
	#endif
	*/

	#if defined(AF_INET6) && defined(IPPROTO_IPV6) && defined(IPV6_V6ONLY)
	if (domain == AF_INET6)
	{
		flag = 1;
		setsockopt (fd, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&flag, QSE_SIZEOF(flag));
	}
	#endif

	/*
	#if defined(AF_UNIX)
	if (domain == AF_UNIX)
	{
		flag = 1000000;
		setsockopt (fd, SOL_SOCKET, SO_SNDBUF,  (void*)&flag, QSE_SIZEOF(flag));
	}
	#endif
 	*/

	if (qse_set_sck_nonblock (fd, 1) <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		goto oops;
	}

	#if defined(IPPROTO_SCTP)
	if (proto == IPPROTO_SCTP)
	{
		#if defined(SOL_SCTP)
		struct sctp_initmsg im;
		struct sctp_paddrparams hb;

		QSE_MEMSET (&im, 0, QSE_SIZEOF(im));
		im.sinit_num_ostreams = 1;
		im.sinit_max_instreams = 1;
		im.sinit_max_attempts = 1;

		if (setsockopt (fd, SOL_SCTP, SCTP_INITMSG, &im, QSE_SIZEOF(im)) <= -1) 
		{
			qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
			goto oops;
		}

		QSE_MEMSET (&hb, 0, QSE_SIZEOF(hb));
		hb.spp_flags = SPP_HB_ENABLE;
		hb.spp_hbinterval = 5000;
		hb.spp_pathmaxrxt = 1;

		if (setsockopt (fd, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &hb, QSE_SIZEOF(hb)) <= -1)
		{
			qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
			goto oops;
		}
		#endif
	}
	#endif

	return fd;

oops:
	if (qse_is_sck_valid(fd)) qse_close_sck (fd);
	return QSE_INVALID_SCKHND;
}

/* ------------------------------------------------------------------- */

static int server_open (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	qse_sck_hnd_t fd = QSE_INVALID_SCKHND, flag;
	qse_skad_t addr;
	int addrsize;
	qse_sck_len_t addrlen;

	addrsize = qse_nwadtoskad (&server->dope.nwad, &addr);
	if (addrsize <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	}

	fd = socket (qse_skadfamily(&addr), SOCK_STREAM, IPPROTO_TCP);
	if (!qse_is_sck_valid(fd)) 
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		goto oops;
	}

	#if defined(FD_CLOEXEC)
	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);
	#endif

	#if defined(SO_REUSEADDR)
	flag = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (void*)&flag, QSE_SIZEOF(flag));
	#endif

	#if defined(SO_REUSEPORT)
	flag = 1;
	if (setsockopt (fd, SOL_SOCKET, SO_REUSEPORT, &flag, QSE_SIZEOF(flag)) <= -1)
	{
		/* TODO: logging. warning only */
		/* this is not a hard failure */
		HTTPD_DBGOUT1 ("Failed to set SO_REUSERPORT on %zd\n", (qse_size_t)fd);
	}
	#endif


/* TODO: linux. use capset() to set required capabilities just in case */
	if (server->dope.flags & QSE_HTTPD_SERVER_TRANSPARENT)
	{
	#if defined(IP_TRANSPARENT)
		/* remove the ip routing restriction that a packet can only
		 * be sent using a local ip address. this option is useful
		 * if transparency is achieved with TPROXY */
	
		/*
		1)
		ip rule add fwmark 0x1/0x1 lookup 100
		ip route add local 0.0.0.0/0 dev lo table 100
	
		2)
		iptables -t mangle -N DIVERT
		iptables -t mangle -A DIVERT -j MARK --set-mark 0x1/0x1
		iptables -t mangle -A DIVERT -j ACCEPT
		iptables -t mangle -A PREROUTING -p tcp -m socket --transparent -j DIVERT
		iptables -t mangle -A PREROUTING -p tcp --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-port 8000
	
		3)
		iptables -t mangle -A PREROUTING -p tcp -m socket --transparent -j MARK --set-mark 0x1/0x1
		iptables -t mangle -A PREROUTING -p tcp -m mark 0x1/0x1 -j RETURN
		iptables -t mangle -A PREROUTING -p tcp --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-port 8000
	
		4)
		iptables -t mangle -A PREROUTING -p tcp --sport 80 -j MARK --set-mark 0x1/0x1
		iptables -t mangle -A PREROUTING -p tcp -m mark 0x1/0x1 -j RETURN
	        iptables -t mangle -A PREROUTING -p tcp --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-port 8000
	
	
		1) is required.
	        one of 2), 3), 4), and a variant is needed.
		Specifying -i and -o can narrow down the amount of packets when the upstream interface
		and the downstream interface are obvious.

		If eth2 is an upstream and the eth1 is a downstream interface,
		iptables -t mangle -A PREROUTING -i eth2 -p tcp --sport 80 -j MARK --set-mark 0x1/0x1
		iptables -t mangle -A PREROUTING -p tcp -m mark 0x1/0x1 -j RETURN
		iptables -t mangle -A PREROUTING -i eth1 -p tcp --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-port 8000

		----------------------------------------------------------------------

		if the socket is bound to 99.99.99.99:8000, you may do...
		iptables -t mangle -A PREROUTING -p tcp --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-ip 99.99.99.99 --on-port 8000

		iptables -t mangle -A PREROUTING -p tcp  ! -s 127.0.0.0/255.0.0.0 --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-ip 0.0.0.0 --on-port 8000

		IP_TRANSPRENT is needed for:
			- accepting TPROXYed connections
			- binding to a non-local IP address (IP address the local system doesn't have)
			- using a non-local IP address as a source
		 */
		flag = 1;
		if (setsockopt (fd, SOL_IP, IP_TRANSPARENT, &flag, QSE_SIZEOF(flag)) <= -1)
		{
			/* TODO: logging. warning only */
			/* this is not a hard failure */
			HTTPD_DBGOUT1 ("Failed to set IP_TRANSPARENT on %zd\n", (qse_size_t)fd);
		}
	#endif
	}

	if (server->dope.flags & QSE_HTTPD_SERVER_BINDTONWIF)
	{
	#if defined(SO_BINDTODEVICE)
		qse_mchar_t tmp[64];
		qse_size_t len;

		len = qse_nwifindextombs (server->dope.nwif, tmp, QSE_COUNTOF(tmp));
		if (len <= 0 || setsockopt (fd, SOL_SOCKET, SO_BINDTODEVICE, tmp, len) <= -1)
		{
			qse_httpd_seterrnum (httpd, ((len <= 0)? QSE_HTTPD_EINVAL: SKERR_TO_ERRNUM()));
			HTTPD_DBGOUT2 ("Failed to set SO_BINDTODEVICE to %hs on %zd\n", tmp, (qse_size_t)fd);
			goto oops;
		}
	#endif
	}

	/* Solaris 8 returns EINVAL if QSE_SIZEOF(addr) is passed in as the
	 * address size for AF_INET. */
	/*if (bind (s, (struct sockaddr*)&addr, QSE_SIZEOF(addr)) <= -1) goto oops_esocket;*/
	if (bind (fd, (struct sockaddr*)&addr, addrsize) <= -1)
	{
	#if defined(IPV6_V6ONLY) && defined(EADDRINUSE)
		if (errno == EADDRINUSE && qse_skadfamily(&addr) == AF_INET6)
		{
			int on = 1;
			setsockopt (fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
			if (bind (fd, (struct sockaddr*)&addr, addrsize) == 0) goto bind_ok;
		}
	#endif

		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		goto oops;
	}

bind_ok:

	server->nwad = server->dope.nwad;
	addrlen = QSE_SIZEOF(addr);
	if (getsockname (fd, (struct sockaddr*)&addr, &addrlen) >= 0)
	{
		qse_nwad_t tmpnwad;
		if (qse_skadtonwad (&addr, &tmpnwad) >= 0)
		{
			/* this is the actual binding address */
			server->nwad = tmpnwad;
		}
	}

	HTTPD_DBGOUT1 ("Setting backlog size to %d\n", server->dope.backlog_size);
	if (listen (fd, server->dope.backlog_size) <= -1) 
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		goto oops;
	}

	if (qse_set_sck_nonblock (fd, 1) <= -1) 
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		goto oops;
	}

	server->handle = fd;
	return 0;

oops:
	if (qse_is_sck_valid(fd)) qse_close_sck (fd);
	return -1;
}

static void server_close (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	qse_close_sck (server->handle);
}

static int server_accept (
	qse_httpd_t* httpd, qse_httpd_server_t* server, qse_httpd_client_t* client)
{
	qse_skad_t addr;
	qse_sck_len_t addrlen;
	qse_sck_hnd_t fd = QSE_INVALID_SCKHND;
	int flag;

	addrlen = QSE_SIZEOF(addr);
	fd = accept (server->handle, (struct sockaddr*)&addr, &addrlen);
	if (!qse_is_sck_valid(fd)) 
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		goto oops;
	}

	#if 0
/* TODO: implement maximum number of client per server??? */
	if (fd >= FD_SETSIZE)
	{
		HTTPD_DEBUG ("ERROR: too many client - max %d, fd %d\n", (FD_SETSIZE, fd));
		/*TODO: qse_httpd_seterrnum (httpd, QSE_HTTPD_EXXXXX);*/
		goto oops;
	}
	#endif

	#if defined(FD_CLOEXEC)
	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);
	#endif

	if (qse_set_sck_nonblock (fd, 1) <= -1) 
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		goto oops;
	}

	if (qse_skadtonwad (&addr, &client->remote_addr) <= -1)
	{
/* TODO: logging */
		client->remote_addr = server->nwad;
	}

	addrlen = QSE_SIZEOF(addr);
	if (getsockname (fd, (struct sockaddr*)&addr, &addrlen) <= -1 ||
	    qse_skadtonwad (&addr, &client->local_addr) <= -1)
	{
/* TODO: logging */
		client->local_addr = server->nwad;
	}

	#if defined(SO_ORIGINAL_DST)
	/* if REDIRECT is used, SO_ORIGINAL_DST returns the original
	 * destination address. When REDIRECT is not used, it returnes
	 * the address of the local socket. In this case, it should 
	 * be same as the result of getsockname(). */
	addrlen = QSE_SIZEOF(addr);
	if (getsockopt (fd, SOL_IP, SO_ORIGINAL_DST, (char*)&addr, &addrlen) <= -1 ||
	    qse_skadtonwad (&addr, &client->orgdst_addr) <= -1)
	{
		client->orgdst_addr = client->local_addr;
	}
	#else
	client->orgdst_addr = client->local_addr;
	#endif

	if (!qse_nwadequal(&client->orgdst_addr, &client->local_addr))
	{
		client->status |= QSE_HTTPD_CLIENT_INTERCEPTED;
	}
	else if (qse_getnwadport(&client->local_addr) != 
	         qse_getnwadport(&server->nwad))
	{
		/* When TPROXY is used, getsockname() and SO_ORIGNAL_DST return
		 * the same addresses. however, the port number may be different
		 * as a typical TPROXY rule is set to change the port number.
		 * However, this check is fragile if the server port number is
		 * set to 0. 
		 *
		 * Take note that if the TPROXY rule doesn't change the port 
		 * number the above assumption gets wrong. so it won't be able
		 * to handle such a TPROXYed packet without port transformation. */
		client->status |= QSE_HTTPD_CLIENT_INTERCEPTED;
	}
	#if 0
	else if ((client->initial_ifindex = resolve_ifindex (fd, client->local_addr)) <= -1)
	{
		/* the local_address is not one of a local address.
		 * it's probably proxied. */
		client->status |= QSE_HTTPD_CLIENT_INTERCEPTED;
	}
	#endif

	client->handle = fd;
	return 0;

oops:
	if (qse_is_sck_valid(fd)) qse_close_sck (fd);
	return -1;
}

/* ------------------------------------------------------------------- */

static void client_close (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_shut_sck (client->handle, QSE_SHUT_SCK_RW);
	qse_close_sck (client->handle);
}

static void client_shutdown (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_shut_sck (client->handle, QSE_SHUT_SCK_RW);
}

static qse_ssize_t client_recv (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_mchar_t* buf, qse_size_t bufsize)
{
	if (client->status & QSE_HTTPD_CLIENT_SECURE)
	{
	#if defined(USE_SSL)
		int ret = SSL_read (HANDLE_TO_SSL(client->handle2), buf, bufsize);
		if (ret <= -1)
		{
			int err = SSL_get_error(HANDLE_TO_SSL(client->handle2),ret);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
				qse_httpd_seterrnum (httpd, QSE_HTTPD_EAGAIN);
			else
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		}

		if (SSL_pending (HANDLE_TO_SSL(client->handle2)) > 0) 
			client->status |= QSE_HTTPD_CLIENT_PENDING;
		else
			client->status &= ~QSE_HTTPD_CLIENT_PENDING;

		return ret;
	#else
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	#endif
	}
	else
	{
		qse_ssize_t ret;
		ret = recv (client->handle, buf, bufsize, 0);
		if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		return ret;
	}
}

static qse_ssize_t client_send (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	const qse_mchar_t* buf, qse_size_t bufsize)
{
	if (client->status & QSE_HTTPD_CLIENT_SECURE)
	{
	#if defined(USE_SSL)
		int ret = SSL_write (HANDLE_TO_SSL(client->handle2), buf, bufsize);
		if (ret <= -1)
		{
			int err = SSL_get_error(HANDLE_TO_SSL(client->handle2),ret);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
				qse_httpd_seterrnum (httpd, QSE_HTTPD_EAGAIN);
			else
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		}
		return ret;
	#else
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	#endif
	}
	else
	{
		qse_ssize_t ret = send (client->handle, buf, bufsize, 0);
		if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		return ret;
	}
}

static qse_ssize_t client_sendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_httpd_hnd_t handle, qse_foff_t* offset, qse_size_t count)
{
	if (client->status & QSE_HTTPD_CLIENT_SECURE)
	{
		return __send_file_ssl (httpd, (void*)client->handle2, handle, offset, count);
	}
	else
	{
		return __send_file (httpd, client->handle, handle, offset, count);
	}
}

static int client_accepted (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	if (client->status & QSE_HTTPD_CLIENT_SECURE)
	{
	#if defined(USE_SSL)
		int ret;
		SSL* ssl;
		server_xtn_t* server_xtn = GET_SERVER_XTN(httpd, client->server);

		if (!server_xtn->ssl_ctx)
		{
			/* performed the delayed ssl initialization */
			if (init_server_ssl (httpd, client->server) <= -1) return -1;
		}

		QSE_ASSERT (server_xtn->ssl_ctx != QSE_NULL);
		QSE_ASSERT (QSE_SIZEOF(client->handle2) >= QSE_SIZEOF(ssl));

		if (HANDLE_TO_SSL(client->handle2))
		{
			ssl = HANDLE_TO_SSL(client->handle2);
		}
		else
		{
			ssl = SSL_new (server_xtn->ssl_ctx);
			if (ssl == QSE_NULL) 
			{
				httpd->errnum = QSE_HTTPD_ESYSERR;
				return -1;
			}

			client->handle2 = SSL_TO_HANDLE(ssl);
			if (SSL_set_fd (ssl, client->handle) == 0)
			{
				/* don't free ssl here since client_closed()
				 * will free it */
				httpd->errnum = QSE_HTTPD_ESYSERR;
				return -1;
			}

			SSL_set_read_ahead (ssl, 0);
		}

		ret = SSL_accept (ssl);
		if (ret <= 0)
		{
			int err = SSL_get_error(ssl, ret);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
			{
				/* handshaking isn't complete. */
				return 0;
			}

			if (httpd->opt.trait & QSE_HTTPD_LOGACT)
			{
				qse_httpd_act_t msg;
				msg.code = QSE_HTTPD_CATCH_MERRMSG;
				ERR_error_string_n (err, msg.u.merrmsg, QSE_COUNTOF(msg.u.merrmsg));
				httpd->opt.rcb.logact (httpd, &msg);
			}

			/* client_closed() free this. no SSL_free() here.
			SSL_free (ssl); */
			return -1;
		}

	#else
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	#endif
	}

	return 1; /* accept completed */
}

static void client_closed (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	if (client->status & QSE_HTTPD_CLIENT_SECURE)
	{
	#if defined(USE_SSL)
		if ((SSL*)client->handle2)
		{
			SSL_shutdown ((SSL*)client->handle2); /* is this needed? */
			SSL_free ((SSL*)client->handle2);
		}
	#endif
	}
}

/* ------------------------------------------------------------------- */

static int peer_open (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
	/* -------------------------------------------------------------------- */

	httpd_xtn_t* xtn = GET_HTTPD_XTN(httpd);
	qse_skad_t connaddr, bindaddr;
	int connaddrsize, bindaddrsize;
	int connected = 1;
	qse_sck_hnd_t fd = QSE_INVALID_SCKHND;

#if defined(USE_SSL)
	SSL* ssl = QSE_NULL;
#endif

#if defined(_WIN32)
	unsigned long cmd;
#elif defined(__OS2__)
	int cmd;
#elif defined(__DOS__)
	int flag;
#else
	int flag;
#endif

	
	/* turn off internally used bits */
	peer->flags &= ~QSE_HTTPD_PEER_ALL_INTERNALS;

	connaddrsize = qse_nwadtoskad (&peer->nwad, &connaddr);
	if (connaddrsize <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	}

	bindaddrsize = qse_nwadtoskad (&peer->local, &bindaddr);

	fd = socket (qse_skadfamily(&connaddr), SOCK_STREAM, IPPROTO_TCP);
	if (!qse_is_sck_valid(fd)) 
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		goto oops;
	}

	if (peer->client && peer->client->server && 
	    (peer->client->server->dope.flags & QSE_HTTPD_SERVER_TRANSPARENT))
	{
	#if defined(IP_TRANSPARENT)
		flag = 1;
		if (setsockopt (fd, SOL_IP, IP_TRANSPARENT, &flag, QSE_SIZEOF(flag)) <= -1)
		{
			/* this is not a hard failure */
			HTTPD_DBGOUT1 ("Failed to set IP_TRANSPARENT on peer socket %zd\n", (qse_size_t)fd);
		}
	#endif
	}

	/* don't use invalid binding address */
	if (bindaddrsize >= 0 &&
	    bind (fd, (struct sockaddr*)&bindaddr, bindaddrsize) <= -1) 
	{
		/* i won't care about binding faiulre */
		/* TODO: some logging for this failure though */
	}

	#if defined(FD_CLOEXEC)
	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);
	#endif

	if (qse_set_sck_nonblock (fd, 1) <= -1) 
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		goto oops;
	}

	if (peer->flags & QSE_HTTPD_PEER_SECURE)
	{
	#if defined(USE_SSL)
		QSE_ASSERT (xtn->ssl_peer_ctx != QSE_NULL);

		ssl = SSL_new (xtn->ssl_peer_ctx);
		if (ssl == QSE_NULL) 
		{
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR); /* TODO: better error code */
			goto oops;
		}

		if (SSL_set_fd (ssl, fd) == 0) 
		{
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR); /* TODO: better error code */
			goto oops;
		}
	#else
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		goto oops;
	#endif
	}

#if defined(_WIN32)
	if (connect (fd, (struct sockaddr*)&connaddr, connaddrsize) <= -1)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK) 
		{
			qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
			goto oops;
		}
		connected = 0;
	}

#elif defined(__OS2__)
	if (connect (fd, (struct sockaddr*)&connaddr, connaddrsize) == -1)
	{
		if (sock_errno() != SOCEINPROGRESS) 
		{
			qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
			goto oops;
		}
		connected = 0;
	}

#else
	if (connect (fd, (struct sockaddr*)&connaddr, connaddrsize) <= -1)
	{
		if (errno != EINPROGRESS) 
		{
			qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
			goto oops;
		}
		connected = 0;
	}
#endif

	if ((peer->flags & QSE_HTTPD_PEER_SECURE) && connected)
	{
	#if defined(USE_SSL)
		int ret = SSL_connect (ssl);
		if (ret <= 0)
		{
			int err = SSL_get_error(ssl, ret);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
			{
				/* handshaking isn't complete. */
				peer->flags |= QSE_HTTPD_PEER_CONNECTED;
				connected = 0; /* not fully connected yet */
			}
			else
			{
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESCONN);
				goto oops;
			}
		}
		else
		{
			peer->flags |= QSE_HTTPD_PEER_CONNECTED;
			/* socket connected + ssl connected */
		}
	#endif
	}

	/* take note the socket handle is in the non-blocking mode here */
	peer->handle = fd;
	if (peer->flags & QSE_HTTPD_PEER_SECURE)
	{
	#if defined(USE_SSL)
		peer->handle2 = SSL_TO_HANDLE(ssl);
	#endif
	}
	return connected;

oops:
	qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
#if defined(USE_SSL)
	if (ssl) SSL_free (ssl);
#endif
	if (qse_is_sck_valid(fd)) qse_close_sck (fd);
	return -1;

	/* -------------------------------------------------------------------- */
}

static void peer_close (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
	if (peer->flags & QSE_HTTPD_PEER_SECURE)
	{
	#if defined(USE_SSL)
		SSL_free (HANDLE_TO_SSL(peer->handle2));
	#endif
	}
	qse_close_sck (peer->handle);
}

static int is_peer_socket_connected (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
#if defined(_WIN32)
	int len;
	DWORD ret;

	len = QSE_SIZEOF(ret);
	if (getsockopt (peer->handle, SOL_SOCKET, SO_ERROR, (char*)&ret, &len) == SOCKET_ERROR) 
	{
		qse_httpd_seterrnum (httpd, skerr_to_errnum (ret));
		return -1;
	}

	if (ret == WSAEWOULDBLOCK) return 0;
	if (ret != 0)
	{
		qse_httpd_seterrnum (httpd, skerr_to_errnum (ret));
		return -1;
	}

	return 1; /* connection completed */

#elif defined(__OS2__)

	int len;
	int ret;

	len = QSE_SIZEOF(ret);
	if (getsockopt (peer->handle, SOL_SOCKET, SO_ERROR, (char*)&ret, &len) == -1)
	{
		qse_httpd_seterrnum (httpd, skerr_to_errnum (ret));
		return -1;
	}

	if (ret == SOCEINPROGRESS) return 0;
	if (ret != 0)
	{
		qse_httpd_seterrnum (httpd, skerr_to_errnum (ret));
		return -1;
	}

	return 1; /* connection completed */

#else

	qse_sck_len_t len;
	int ret;

	len = QSE_SIZEOF(ret);
	if (getsockopt (peer->handle, SOL_SOCKET, SO_ERROR, &ret, &len) <= -1) 
	{
		qse_httpd_seterrnum (httpd, skerr_to_errnum (ret));
		return -1;
	}

	if (ret == EINPROGRESS) return 0;
#if defined(__DOS__)
	if (ret == EISCONN) return 1; /* watt-32 gives EISCONN when connected */
#endif
	if (ret != 0)
	{
		qse_httpd_seterrnum (httpd, skerr_to_errnum (ret));
		return -1;
	}

	return 1; /* connection completed */
#endif
}

static int is_peer_connected_securely (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
#if defined(USE_SSL)
	int ret = SSL_connect (HANDLE_TO_SSL(peer->handle2));
	if (ret <= 0)
	{
		int err = SSL_get_error(HANDLE_TO_SSL(peer->handle2), ret);
		if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
		{
			/* handshaking isn't complete. */
			return 0; /* not connected */
		}
		else
		{
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ESCONN);
			return -1;
		}
	}
	return 1;
#else
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#endif
}

static int peer_connected (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
	if (peer->flags & QSE_HTTPD_PEER_SECURE)
	{
		if (peer->flags & QSE_HTTPD_PEER_CONNECTED)
		{
			return is_peer_connected_securely (httpd, peer);
		}
		else
		{
			int ret = is_peer_socket_connected (httpd, peer);
			if (ret <= 0) return ret;
			peer->flags |= QSE_HTTPD_PEER_CONNECTED;
			return 0;
		}
	}
	else
	{
		return is_peer_socket_connected (httpd, peer);
	}
}

static qse_ssize_t peer_recv (
	qse_httpd_t* httpd, qse_httpd_peer_t* peer,
	qse_mchar_t* buf, qse_size_t bufsize)
{
	if (peer->flags & QSE_HTTPD_PEER_SECURE)
	{
	#if defined(USE_SSL)
		int ret = SSL_read (HANDLE_TO_SSL(peer->handle2), buf, bufsize);
		if (ret <= -1)
		{
			int err = SSL_get_error(HANDLE_TO_SSL(peer->handle2),ret);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
				qse_httpd_seterrnum (httpd, QSE_HTTPD_EAGAIN);
			else
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		}

		if (SSL_pending (HANDLE_TO_SSL(peer->handle2)) > 0) 
			peer->flags |= QSE_HTTPD_PEER_PENDING;
		else
			peer->flags &= ~QSE_HTTPD_PEER_PENDING;

		return ret;
	#else
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	#endif
	}
	else
	{
		qse_ssize_t ret = recv (peer->handle, buf, bufsize, 0);
		if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		return ret;
	}
}

static qse_ssize_t peer_send (
	qse_httpd_t* httpd, qse_httpd_peer_t* peer,
	const qse_mchar_t* buf, qse_size_t bufsize)
{
	if (peer->flags & QSE_HTTPD_PEER_SECURE)
	{
	#if defined(USE_SSL)
		int ret = SSL_write (HANDLE_TO_SSL(peer->handle2), buf, bufsize);
		if (ret <= -1)
		{
			int err = SSL_get_error(HANDLE_TO_SSL(peer->handle2),ret);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
				qse_httpd_seterrnum (httpd, QSE_HTTPD_EAGAIN);
			else
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		}
		return ret;
	#else
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	#endif
	}
	else
	{
		qse_ssize_t ret = send (peer->handle, buf, bufsize, 0);
		if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		return ret;
	}
}

/* ------------------------------------------------------------------- */

typedef struct mux_xtn_t mux_xtn_t;
struct mux_xtn_t
{
	qse_httpd_t* httpd;
	qse_httpd_muxcb_t cbfun;
};

static void dispatch_muxcb (qse_mux_t* mux, const qse_mux_evt_t* evt)
{
	mux_xtn_t* xtn;
	qse_httpd_hnd_t handle;
	int mask = 0;

	xtn = qse_mux_getxtn(mux);
	handle = evt->hnd;

	if (evt->mask & QSE_MUX_IN) mask |= QSE_HTTPD_MUX_READ;
	if (evt->mask & QSE_MUX_OUT) mask |= QSE_HTTPD_MUX_WRITE;

	xtn->cbfun (xtn->httpd, mux, handle, mask, evt->data);
	/* ignore return code of xtn->cbfun */
}

static void* mux_open (qse_httpd_t* httpd, qse_httpd_muxcb_t cbfun)
{
	qse_mux_t* mux;
	mux_xtn_t* xtn;

	mux = qse_mux_open (qse_httpd_getmmgr(httpd), QSE_SIZEOF(*xtn), dispatch_muxcb, 256, QSE_NULL);
	if (!mux)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		return QSE_NULL;
	}

	xtn = qse_mux_getxtn(mux);
	xtn->httpd = httpd;
	xtn->cbfun = cbfun;
	return mux;
}

static void mux_close (qse_httpd_t* httpd, void* vmux)
{
	qse_mux_close ((qse_mux_t*)vmux);
}

static int mux_addhnd (
	qse_httpd_t* httpd, void* vmux, qse_httpd_hnd_t handle, int mask, void* data)
{
	qse_mux_evt_t evt;

	evt.hnd = handle;
	evt.mask = 0;
	if (mask & QSE_HTTPD_MUX_READ) evt.mask |= QSE_MUX_IN;
	if (mask & QSE_HTTPD_MUX_WRITE) evt.mask |= QSE_MUX_OUT;
	evt.data = data;

	if (qse_mux_insert ((qse_mux_t*)vmux, &evt) <= -1)
	{
		qse_httpd_seterrnum (httpd, muxerr_to_errnum(qse_mux_geterrnum((qse_mux_t*)vmux)));
		return -1;
	}

	return 0;
}

static int mux_delhnd (qse_httpd_t* httpd, void* vmux, qse_httpd_hnd_t handle)
{
	qse_mux_evt_t evt;
	evt.hnd = handle;
	if (qse_mux_delete ((qse_mux_t*)vmux, &evt) <= -1)
	{
		qse_httpd_seterrnum (httpd, muxerr_to_errnum(qse_mux_geterrnum((qse_mux_t*)vmux)));
		return -1;
	}
	return 0;
}

static int mux_poll (qse_httpd_t* httpd, void* vmux, const qse_ntime_t* tmout)
{
	if (qse_mux_poll ((qse_mux_t*)vmux, tmout) <= -1)
	{
		qse_httpd_seterrnum (httpd, muxerr_to_errnum(qse_mux_geterrnum((qse_mux_t*)vmux)));
		return -1;
	}

	return 0;
}

static int mux_readable (qse_httpd_t* httpd, qse_httpd_hnd_t handle, const qse_ntime_t* tmout)
{
#if defined(__OS2__) && !defined(TCPV40HDRS)
	long tv;

	tv = tmout? QSE_SECNSEC_TO_MSEC (tmout->sec, tmout->nsec): -1;
	return os2_select (&handle, 1, 0, 0, tv);

#else
	fd_set r;
	struct timeval tv, * tvp;

	FD_ZERO (&r);
	FD_SET (handle, &r);

	if (tmout)
	{
		tv.tv_sec = tmout->sec;
		tv.tv_usec = tmout->nsec;
		tvp = &tv;
	}
	else tvp = QSE_NULL;

	return select (handle + 1, &r, QSE_NULL, QSE_NULL, tvp);
#endif
}

static int mux_writable (qse_httpd_t* httpd, qse_httpd_hnd_t handle, const qse_ntime_t* tmout)
{
#if defined(__OS2__) && !defined(TCPV40HDRS)
	long tv;
	tv = tmout? QSE_SECNSEC_TO_MSEC (tmout->sec, tmout->nsec): -1;
	return os2_select (&handle, 0, 1, 0, tv);

#elif defined(HAVE_POLL_H)
	struct pollfd p;
	int tv;

	p.fd = handle;
	p.events = POLLOUT;
	p.revents = 0;

	tv = tmout? QSE_SECNSEC_TO_MSEC(tmout->sec, tmout->nsec): -1;
	return poll (&p, 1, tv);
#else

	fd_set w;
	struct timeval tv, * tvp;

	#if defined(FD_SETSIZE)
	/* NOTE: when the handle exceeds FD_SETSIZE, 
	 * select() may screw the entire program. */
	if (handle >= FD_SETSIZE) return -1;
	#endif

	FD_ZERO (&w);
	FD_SET (handle, &w);

	if (tmout)
	{
		tv.tv_sec = tmout->sec;
		tv.tv_usec = tmout->nsec;
		tvp = &tv;
	}
	else tvp = QSE_NULL;

	return select (handle + 1, QSE_NULL, &w, QSE_NULL, tvp);
#endif
}

/* ------------------------------------------------------------------- */

static int stat_file (
	qse_httpd_t* httpd, const qse_mchar_t* path,
	qse_httpd_stat_t* hst, int filter)
{

#if defined(_WIN32)

	HANDLE fh;
	WIN32_FIND_DATAA fdata;
	ULARGE_INTEGER li;

	if ((path[0] == QSE_MT('/') && path[1] ==  QSE_MT('\0')) ||
	    (qse_ismbsdriveabspath(path) && qse_mbslen(path) == 3))
	{
		/* the root directory doesn't work well with FindFirstFile()
		 * if it's not appened with an wildcard letter like C:\*.*.
		 * since i'm not interested in the files under the root 
		 * directory, let me just hard-code it to indicate that
		 * it is a directory.
		 */
		QSE_MEMSET (hst, 0, QSE_SIZEOF(*hst));
		hst->isdir = 1;
	}
	else
	{
		/* TODO: hst->dev can be set to the drive letter's index. */

		/* fail if the path name contains a wilecard letter */
		if (qse_mbspbrk (path, QSE_MT("?*")) != QSE_NULL) return -1;

		fh = FindFirstFileA (path, &fdata);
		if (fh == INVALID_HANDLE_VALUE) 
		{
			qse_httpd_seterrnum (httpd, syserr_to_errnum(GetLastError()));
			return -1;
		}

		QSE_MEMSET (hst, 0, QSE_SIZEOF(*hst));
		if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) hst->isdir = 1;

		hst->size = ((qse_foff_t)fdata.nFileSizeHigh << 32) | fdata.nFileSizeLow;
		li.LowPart = fdata.ftLastWriteTime.dwLowDateTime;
		li.HighPart = fdata.ftLastWriteTime.dwHighDateTime;

		/* li.QuadPart is in the 100-nanosecond intervals */
		hst->mtime.sec = (li.QuadPart / (QSE_NSECS_PER_SEC / 100)) - EPOCH_DIFF_SECS;
		hst->mtime.nsec = (li.QuadPart % (QSE_NSECS_PER_SEC / 100)) * 100;

		FindClose (fh);
	}
	
	return 0;

#elif defined(__OS2__)
	APIRET rc;
	qse_btime_t bt;
	qse_ntime_t nt;

	FILESTATUS3L ffb;

	rc = DosQueryPathInfo (path, FIL_STANDARDL, &ffb, QSE_SIZEOF(ffb));
	if (rc != NO_ERROR) 
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(rc));
		return -1;
	}

	QSE_MEMSET (&bt, 0, QSE_SIZEOF(bt));
	bt.mday = ffb.fdateLastWrite.day;
	bt.mon = ffb.fdateLastWrite.month - 1;
	bt.year = ffb.fdateLastWrite.year + 80;
	bt.hour = ffb.ftimeLastWrite.hours;
	bt.min = ffb.ftimeLastWrite.minutes;
	bt.min = ffb.ftimeLastWrite.twosecs * 2;
	bt.isdst = -1;
	if (qse_timelocal (&bt, &nt) <= -1) return -1;

	QSE_MEMSET (hst, 0, QSE_SIZEOF(*hst));
	if (ffb.attrFile & FILE_DIRECTORY) hst->isdir = 1;
	hst->size = ((qse_foff_t)ffb.cbFile.ulHi << 32) | ffb.cbFile.ulLo;
	hst->mtime = nt;

	if (path[0] != QSE_MT('\0') && path[1] == QSE_MT(':'))
	{
		if (path[0] >= QSE_MT('a') && path[0] <= QSE_MT('z')) 
			hst->dev = path[0] - QSE_MT('a');
		else if (path[0] >= QSE_MT('A') && path[0] <= QSE_MT('Z')) 
			hst->dev = path[0] - QSE_MT('A');
	}
	else
	{
		ULONG num, map;
		if (DosQueryCurrentDisk (&num, &map) == NO_ERROR) hst->dev = num - 1;
	}

	return 0;

#elif defined(__DOS__)

	struct stat st;

	if (stat (path, &st) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

	if ((filter == STAT_REG && !S_ISREG(st.st_mode)) ||
	    (filter == STAT_DIR && !S_ISDIR(st.st_mode)))
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EACCES);
		return -1;
	}

	QSE_MEMSET (hst, 0, QSE_SIZEOF(*hst));

	hst->isdir = S_ISDIR(st.st_mode);
	hst->dev = st.st_dev;
	hst->ino = st.st_ino;
	hst->size = st.st_size;
	hst->mtime.sec = st.st_mtime;
	hst->mtime.nsec = 0;

	return 0;
#else
	qse_stat_t st;

/* TODO: lstat? or stat? */

	if (QSE_STAT (path, &st) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

	if ((filter == STAT_REG && !S_ISREG(st.st_mode)) ||
	    (filter == STAT_DIR && !S_ISDIR(st.st_mode)))
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EACCES);
		return -1;
	}

	QSE_MEMSET (hst, 0, QSE_SIZEOF(*hst));

	hst->isdir = S_ISDIR(st.st_mode);
	hst->dev = st.st_dev;
	hst->ino = st.st_ino;
	hst->size = st.st_size;
	#if defined(HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)
	hst->mtime.sec = st.st_mtim.tv_sec;
	hst->mtime.nsec = st.st_mtim.tv_nsec;
	#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC)
	hst->mtime.sec = st.st_mtimespec.tv_sec;
	hst->mtime.nsec = st.st_mtimespec.tv_nsec;
	#else
	hst->mtime.sec = st.st_mtime;
	hst->mtime.nsec = 0;
	#endif

	return 0;
#endif
}

/* ------------------------------------------------------------------- */

static int file_stat (
	qse_httpd_t* httpd, const qse_mchar_t* path, qse_httpd_stat_t* hst)
{
	/* this callback is not required to be a general stat function
	 * for a file. it is mainly used to get a file size and timestamps
	 * of a regular file. so it should fail for a non-regular file.
	 * note that STAT_REG is passed to stat_file for it */
	return stat_file (httpd, path, hst, STAT_REG);
}

static int file_purge (qse_httpd_t* httpd, const qse_mchar_t* path)
{
#if defined(_WIN32)
	if (DeleteFileA (path) == FALSE)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(GetLastError()));
		return -1;
	}
	return 0;

#elif defined(__OS2__)

	APIRET rc;

	rc = DosDelete (path); /* TODO: is DosForceDelete better? */
	if (rc != NO_ERROR)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(rc));
		return -1;
	}
	return 0;

#elif defined(__DOS__)

	if (unlink (path) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}
	return 0;

#else

	if (QSE_UNLINK (path) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}
	return 0;

#endif
}

static qse_fio_t* __open_file (qse_httpd_t* httpd, const qse_mchar_t* path, int fio_flags, int fio_mode)
{
	qse_fio_t* fio;

	fio = qse_httpd_allocmem (httpd, QSE_SIZEOF(*fio));
	if (fio == QSE_NULL) return QSE_NULL;

	if (qse_fio_init (fio, qse_httpd_getmmgr(httpd), (const qse_char_t*)path, fio_flags, fio_mode) <= -1)
	{
		qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(fio)));
		qse_httpd_freemem (httpd, fio);
		return QSE_NULL;
	}

	return fio;
}

static int file_ropen (
	qse_httpd_t* httpd, const qse_mchar_t* path, qse_httpd_hnd_t* handle)
{
	qse_fio_t* fio;

	fio = __open_file (httpd, path, QSE_FIO_READ | QSE_FIO_MBSPATH, 0);
	if (fio == QSE_NULL) return -1;

	*handle = FIO_TO_HANDLE(fio);

	HTTPD_DBGOUT2 ("Opened file [%hs] for reading - %zd\n", path, (qse_size_t)*handle);
	return 0;
}

static int file_wopen (
	qse_httpd_t* httpd, const qse_mchar_t* path,
	qse_httpd_hnd_t* handle)
{
	qse_fio_t* fio;

	fio = __open_file (httpd, path, QSE_FIO_WRITE | QSE_FIO_CREATE | QSE_FIO_TRUNCATE | QSE_FIO_MBSPATH, 0644);
	if (fio == QSE_NULL) return -1;

	*handle = FIO_TO_HANDLE(fio);

	HTTPD_DBGOUT2 ("Opened file [%hs] for writing - %zd\n", path, (qse_size_t)*handle);
	return 0;
}

static void file_close (qse_httpd_t* httpd, qse_httpd_hnd_t handle)
{
	HTTPD_DBGOUT1 ("Closed file %zd\n", (qse_size_t)handle);

	qse_fio_fini (HANDLE_TO_FIO(handle));
	qse_httpd_freemem (httpd, HANDLE_TO_FIO(handle));
}

static qse_ssize_t file_read (
	qse_httpd_t* httpd, qse_httpd_hnd_t handle, qse_mchar_t* buf, qse_size_t len)
{
	qse_ssize_t n;
	n = qse_fio_read (HANDLE_TO_FIO(handle), buf, len);
	if (n <= -1) qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(HANDLE_TO_FIO(handle))));
	return n;
}

static qse_ssize_t file_write (
	qse_httpd_t* httpd, qse_httpd_hnd_t handle, const qse_mchar_t* buf, qse_size_t len)
{
	qse_ssize_t n;
	n = qse_fio_write (HANDLE_TO_FIO(handle), buf, len);
	if (n <= -1) qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(HANDLE_TO_FIO(handle))));
	return n;
}

/* ------------------------------------------------------------------- */

static int dir_stat (
	qse_httpd_t* httpd, const qse_mchar_t* path, qse_httpd_stat_t* hst)
{
	/* this callback is not required to be a general stat function
	 * for a file. it is mainly used to get a file size and timestamps
	 * of a regular file. so it should fail for a non-regular file.
	 * note that STAT_REG is passed to stat_file for it */
	return stat_file (httpd, path, hst, STAT_DIR);
}

static int dir_make (qse_httpd_t* httpd, const qse_mchar_t* path)
{
#if defined(_WIN32)

	if (CreateDirectoryA (path, QSE_NULL) == FALSE)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(GetLastError()));
		return -1;
	}

	return 0;
	
#elif defined(__OS2__)

	APIRET rc;

	rc = DosCreateDir (path, QSE_NULL); /* TODO: is DosForceDelete better? */
	if (rc != NO_ERROR)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(rc));
		return -1;
	}
	return 0;

#elif defined(__DOS__)

	if (mkdir (path) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

	return 0;

#else

	if (QSE_MKDIR (path, 0755) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}
	return 0;

#endif
}

static int dir_purge (qse_httpd_t* httpd, const qse_mchar_t* path)
{
#if defined(_WIN32)
	if (RemoveDirectoryA (path) == FALSE)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(GetLastError()));
		return -1;
	}
#elif defined(__OS2__)

	APIRET rc;

	rc = DosDeleteDir (path); /* TODO: is DosForceDelete better? */
	if (rc != NO_ERROR)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(rc));
		return -1;
	}
	return 0;

#elif defined(__DOS__)

	if (rmdir (path) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}
	return 0;

#else

	if (QSE_RMDIR (path) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}
	return 0;

#endif
}

typedef struct dir_t dir_t;
struct dir_t
{
	qse_mchar_t* path;
	qse_dir_t* dp;
};

#define HANDLE_TO_DIR(x) ((dir_t*)(x))
#define DIR_TO_HANDLE(x) ((qse_httpd_hnd_t)(x))

static int dir_open (qse_httpd_t* httpd, const qse_mchar_t* path, qse_httpd_hnd_t* handle)
{
	dir_t* d;
	qse_dir_errnum_t direrrnum;

	d = QSE_MMGR_ALLOC (qse_httpd_getmmgr(httpd), QSE_SIZEOF(*d));
	if (d == QSE_NULL) 
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return -1;
	}

	d->path = qse_mbsdup (path, qse_httpd_getmmgr(httpd));
	if (d->path == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), d);
		return -1;
	}

	d->dp = qse_dir_open (
		qse_httpd_getmmgr(httpd), 0, 
		(const qse_char_t*)d->path, 
		QSE_DIR_MBSPATH | QSE_DIR_SORT,
		&direrrnum
	);
	if (d->dp == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, direrr_to_errnum(direrrnum));
		QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), d->path);
		QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), d);
		return -1;
	}

	*handle = DIR_TO_HANDLE(d);

	HTTPD_DBGOUT2 ("Opened directory [%hs] - %zd\n", path, (qse_size_t)*handle);
	return 0;
}

static void dir_close (qse_httpd_t* httpd, qse_httpd_hnd_t handle)
{
	dir_t* d;

	HTTPD_DBGOUT1 ("Closing directory %zd\n", (qse_size_t)handle);

	d = HANDLE_TO_DIR(handle);

	qse_dir_close (d->dp);

	QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), d->path);
	QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), d);
}

static int dir_read (qse_httpd_t* httpd, qse_httpd_hnd_t handle, qse_httpd_dirent_t* dirent)
{
	dir_t* d;
	qse_dir_ent_t de;
	qse_mchar_t* fpath;
	int n;

	d = HANDLE_TO_DIR(handle);

	n = qse_dir_read (d->dp, &de);
	if (n <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		return -1;
	}
	else if (n == 0) return 0;

	/* i assume that d->path ends with a slash */
	fpath = qse_mbsdup2 (d->path, (const qse_mchar_t*)de.name, qse_httpd_getmmgr(httpd));
	if (fpath == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return -1;
	}

	n = stat_file (httpd, fpath, &dirent->stat, 0);	
	QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), fpath);
	if (n <= -1) QSE_MEMSET (dirent, 0, QSE_SIZEOF(*dirent));

	dirent->name = (const qse_mchar_t*)de.name;
	return 1;
}

/* ------------------------------------------------------------------- */
#if 0
static qse_htb_walk_t walk (qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
{
	qse_htre_hdrval_t* val;

	val = QSE_HTB_VPTR(pair);
	while (val)
	{

		HTTPD_DEBUG ((QSE_T("HEADER OK %d[%hs] %d[%hs]\n"), (int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)val->len, val->ptr));
		val = val->next;
	}
	return QSE_HTB_WALK_FORWARD;
}
#endif

static int process_request (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req, int peek)
{
	server_xtn_t* server_xtn = GET_SERVER_XTN(httpd, client->server);
	qse_httpd_task_t* task;
	qse_http_method_t mth;
	qse_httpd_rsrc_t rsrc;


	/* percent-decode the query path to the original buffer
	 * since i'm not going to need it in the original form
	 * any more. once it's decoded in the peek mode,
	 * the decoded query path is made available in the
	 * non-peek mode as well */
	if (peek) 
	{
		qse_htre_perdecqpath(req);

		/* TODO: proper request logging */

		HTTPD_DBGOUT2 ("%s %s\n", qse_htre_getqmethodname(req), qse_htre_getqpath(req));
	}


#if 0
qse_printf (QSE_T("================================\n"));
qse_printf (QSE_T("[%lu] %hs REQUEST ==> [%hs] version[%d.%d %hs] method[%hs]\n"),
	(unsigned long)time(NULL),
	(peek? QSE_MT("PEEK"): QSE_MT("HANDLE")),
	qse_htre_getqpath(req),
	qse_htre_getmajorversion(req),
	qse_htre_getminorversion(req),
	qse_htre_getverstr(req),
	qse_htre_getqmethodname(req)
);
if (qse_htre_getqparam(req))
	qse_printf (QSE_T("PARAMS ==> [%hs]\n"), qse_htre_getqparam(req));

qse_htb_walk (&req->hdrtab, walk, QSE_NULL);
if (qse_htre_getcontentlen(req) > 0)
{
	qse_printf (QSE_T("CONTENT [%.*S]\n"), (int)qse_htre_getcontentlen(req), qse_htre_getcontentptr(req));
}
#endif

	mth = qse_htre_getqmethodtype(req);

	if (peek)
	{
		/* determine what to do once the header fields are all received.
		 * i don't want to delay this until the contents are received.
		 * if you don't like this behavior, you must implement your own
		 * callback function for request handling. */

#if 0
		/* TODO support X-HTTP-Method-Override */
		if (data.method == QSE_HTTP_POST)
		{
			tmp = qse_htre_getheaderval(req, QSE_MT("X-HTTP-Method-Override"));
			if (tmp)
			{
				/*while (tmp->next) tmp = tmp->next;*/ /* get the last value */
				data.method = qse_mbstohttpmethod (tmp->ptr);
			}
		}
#endif

		if (mth == QSE_HTTP_CONNECT)
		{
			/* CONNECT method must not have content set. 
			 * however, arrange to discard it if so. 
			 *
			 * NOTE: CONNECT is implemented to ignore many headers like
			 *       'Expect: 100-continue' and 'Connection: keep-alive'. */
			qse_httpd_discardcontent (httpd, req);
		}
		else 
		{
			if (mth == QSE_HTTP_POST &&
			    !(req->flags & QSE_HTRE_ATTR_LENGTH) &&
			    !(req->flags & QSE_HTRE_ATTR_CHUNKED))
			{
				/* POST without Content-Length nor not chunked */
				req->flags &= ~QSE_HTRE_ATTR_KEEPALIVE;
				qse_httpd_discardcontent (httpd, req);
				task = qse_httpd_entaskerror (httpd, client, QSE_NULL, 411, req);
				if (task) 
				{
					/* 411 Length Required - can't keep alive. Force disconnect */
					task = qse_httpd_entaskdisconnect (httpd, client, QSE_NULL);
				}
			}
			else if (server_xtn->makersrc (httpd, client, req, &rsrc) <= -1)
			{
				/* failed to make a resource. just send the internal server error.
				 * the makersrc handler can return a negative number to return 
				 * '500 Internal Server Error'. If it wants to return a specific
				 * error code, it should return 0 with the QSE_HTTPD_RSRC_ERROR
				 * resource. */
				qse_httpd_discardcontent (httpd, req);
				task = qse_httpd_entaskerror (httpd, client, QSE_NULL, 500, req);
			}
			else
			{
				task = QSE_NULL;

				if ((rsrc.flags & QSE_HTTPD_RSRC_100_CONTINUE) &&
				    (task = qse_httpd_entaskcontinue (httpd, client, task, req)) == QSE_NULL) 
				{
					/* inject '100 continue' first if it is needed */
					goto oops;
				}

				/* arrange the actual resource to be returned */
				task = qse_httpd_entaskrsrc (httpd, client, task, &rsrc, req);
				server_xtn->freersrc (httpd, client, req, &rsrc);

				/* if the resource is indicating to return an error,
				 * discard the contents since i won't return them */
				if (rsrc.type == QSE_HTTPD_RSRC_ERROR) 
				{ 
					qse_httpd_discardcontent (httpd, req); 
				}
			}

			if (task == QSE_NULL) goto oops;
		}
	}
	else
	{
		/* contents are all received */

		if (mth == QSE_HTTP_CONNECT)
		{
			HTTPD_DBGOUT1 ("Switching HTRD to DUMMY for [%hs]\n", qse_htre_getqpath(req));

			/* Switch the http read to a dummy mode so that the subsqeuent
			 * input(request) is just treated as data to the request just 
			 * completed */
			qse_htrd_dummify (client->htrd);

			if (server_xtn->makersrc (httpd, client, req, &rsrc) <= -1)
			{
				HTTPD_DBGOUT1 ("Cannot make resource for [%hs]\n", qse_htre_getqpath(req));

				/* failed to make a resource. just send the internal server error.
				 * the makersrc handler can return a negative number to return 
				 * '500 Internal Server Error'. If it wants to return a specific
				 * error code, it should return 0 with the QSE_HTTPD_RSRC_ERROR
				 * resource. */
				task = qse_httpd_entaskerror (httpd, client, QSE_NULL, 500, req);
			}
			else
			{
				/* arrange the actual resource to be returned */
				task = qse_httpd_entaskrsrc (httpd, client, QSE_NULL, &rsrc, req);
				server_xtn->freersrc (httpd, client, req, &rsrc);
			}

			if (task == QSE_NULL) goto oops;
		}
		else if (req->flags & QSE_HTRE_ATTR_PROXIED)
		{
			/* the contents should be proxied. 
			 * do nothing locally */
		}
		else
		{
			/* when the request is handled locally, 
			 * there's nothing special to do here */
		}
	}

	if (!(req->flags & QSE_HTRE_ATTR_KEEPALIVE) || mth == QSE_HTTP_CONNECT)
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

static int poke_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	return process_request (httpd, client, req, 0);
}

static int format_error (qse_httpd_t* httpd, qse_httpd_client_t* client, int code, qse_mchar_t* buf, int bufsz)
{
	server_xtn_t* server_xtn = GET_SERVER_XTN(httpd, client->server);
	qse_size_t n;
	const qse_mchar_t* head, * foot, * msg;

	if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_ERRHEAD, QSE_NULL, &head) <= -1) head = QSE_NULL;
	if (head == QSE_NULL) head = QSE_MT("<style type='text/css'>body { background-color:#d0e4fe; font-size: 0.9em; } div.header { font-weight: bold; margin-bottom: 5px; } div.footer { border-top: 1px solid #99AABB; text-align: right; }</style>");

	if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_ERRFOOT, QSE_NULL, &foot) <= -1) foot = QSE_NULL;
	if (foot == QSE_NULL) foot = qse_httpd_getname(httpd);

	msg = qse_httpstatustombs(code);

	n = qse_mbsxfmts (buf, bufsz,
		QSE_MT("<html><head>%s<title>%s</title></head><body><div class='header'>HTTP ERROR</div><div class='body'>%d %s</div><div class='footer'>%s</div></body></html>"), 
		head, msg, code, msg, foot);
	if (n == (qse_size_t)-1)
	{
		httpd->errnum = QSE_HTTPD_ENOBUF;
		return -1;
	}

	return 0;
}

static void impede_httpd (qse_httpd_t* httpd)
{
	/* do nothing */
}

static void logact_httpd (qse_httpd_t* httpd, const qse_httpd_act_t* act)
{
	/* do nothing */
}

/* ------------------------------------------------------------------- */

static qse_size_t hash_string (const qse_mchar_t *str)
{
	qse_size_t h = 0;
	while (*str) h = ((h << 5) + h) ^ *str++;
	return h;
}

#include "httpd-std-mod.h"
#include "httpd-std-dns.h"
#include "httpd-std-urs.h"

/* ------------------------------------------------------------------- */

static qse_httpd_scb_t httpd_system_callbacks =
{
	/* module */
	{
		mod_open,
		mod_close,
		mod_symbol
	},

	/* server */
	{ 
		server_open,
		server_close,
		server_accept
	},

	/* client connection */
	{ 
		client_close,
		client_shutdown,
		client_recv,
		client_send,
		client_sendfile,
		client_accepted,
		client_closed
	},

	/* proxy peer */
	{ 
		peer_open,
		peer_close,
		peer_connected,
		peer_recv,
		peer_send
	},

	/* multiplexer */
	{ 
		mux_open,
		mux_close,
		mux_addhnd,
		mux_delhnd,
		mux_poll,

		mux_readable,
		mux_writable
	},

	/* file operation */
	{
		file_stat,
		file_purge,
		file_ropen,
		file_wopen,
		file_close,
		file_read,
		file_write
	},

	/* directory operation */
	{
		dir_stat,
		dir_make,
		dir_purge,
		dir_open,
		dir_close,
		dir_read
	},

	/* dns */
	{
		dns_open,
		dns_close,
		dns_recv,
		dns_send,
		dns_preresolve
	},

	/* urs */
	{
		urs_open,
		urs_close,
		urs_recv,
		urs_send,
		urs_prerewrite
	}
};

static qse_httpd_rcb_t httpd_request_callbacks =
{
	QSE_STRUCT_FIELD(peekreq, peek_request),
	QSE_STRUCT_FIELD(pokereq, poke_request),
	QSE_STRUCT_FIELD(fmterr,  format_error),
	QSE_STRUCT_FIELD(impede,  impede_httpd),
	QSE_STRUCT_FIELD(logact,  logact_httpd)
};

static void set_httpd_callbacks (qse_httpd_t* httpd)
{
	qse_httpd_setopt (httpd, QSE_HTTPD_SCB, &httpd_system_callbacks);
	qse_httpd_setopt (httpd, QSE_HTTPD_RCB, &httpd_request_callbacks);
}
/* ------------------------------------------------------------------- */

static void free_resource (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_htre_t* req, qse_httpd_rsrc_t* target)
{
	const qse_mchar_t* qpath = qse_htre_getqpath(req);
 
	switch (target->type)
	{
		case QSE_HTTPD_RSRC_CGI:
			if (target->u.cgi.suffix) 
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), (qse_mchar_t*)target->u.cgi.suffix);
			if (target->u.cgi.script != qpath) 
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), (qse_mchar_t*)target->u.cgi.script);
			if (!(target->u.cgi.flags & QSE_HTTPD_RSRC_CGI_FNC))
			{
				if (target->u.cgi.path != qpath)
					QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), (qse_mchar_t*)target->u.cgi.path);
				if (target->u.cgi.shebang)
					QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), (qse_mchar_t*)target->u.cgi.shebang);
			}

			break;

		case QSE_HTTPD_RSRC_DIR:
			if (target->u.dir.path != qpath)
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), (qse_mchar_t*)target->u.dir.path);
			break;

		case QSE_HTTPD_RSRC_FILE:
			if (target->u.file.path != qpath)
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), (qse_mchar_t*)target->u.file.path);
			break;

		case QSE_HTTPD_RSRC_RELOC:
			if (target->u.reloc.target != qpath)
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), (qse_mchar_t*)target->u.reloc.target);
			break;

		default:
			/* nothing to do */
			break;
	}
}

static qse_mchar_t* merge_paths (
	qse_httpd_t* httpd, const qse_mchar_t* base, const qse_mchar_t* path)
{
	qse_mchar_t* xpath;
	const qse_mchar_t* ta[4];
	qse_size_t idx = 0;

	ta[idx++] = base;
	if (path[0] != QSE_MT('\0'))
	{
		ta[idx++] = QSE_MT("/");
		ta[idx++] = path;
	}
	ta[idx++] = QSE_NULL;
	xpath = qse_mbsadup (ta, QSE_NULL, qse_httpd_getmmgr(httpd));
	if (xpath == QSE_NULL)
	{
		httpd->errnum = QSE_HTTPD_ENOMEM;
		return QSE_NULL;
	}

	qse_canonmbspath (xpath, xpath, 0);
	return xpath;
}

static void merge_paths_to_buf (
	qse_httpd_t* httpd, const qse_mchar_t* base, 
	const qse_mchar_t* path, qse_size_t plen, qse_mchar_t* xpath)
{
	/* this function merges two path names into a buffer large enough
	 * to hold the result. it doesn't duplicate the result */
	qse_size_t len = 0;
	len += qse_mbscpy (&xpath[len], base);

	if (plen == (qse_size_t)-1)
	{
		len += qse_mbscpy (&xpath[len], QSE_MT("/"));
		len += qse_mbscpy (&xpath[len], path);
	}
	else if (plen > 0)
	{
		len += qse_mbscpy (&xpath[len], QSE_MT("/"));
		len += qse_mbsncpy (&xpath[len], path, plen);
	}
	qse_canonmbspath (xpath, xpath, 0);
}

struct rsrc_tmp_t
{
	const qse_mchar_t* qpath; /* query path in the request */
	const qse_mchar_t* idxfile;
	qse_mchar_t* xpath;

	qse_size_t qpath_len;

	/* pointer to the first query path segment excluding the location name.
	 * for example, if a query path /a/b/c matches a location '/a',
	 * it points to '/b/c'. '/a' is replaced by the document root.
	 * and '/b/c' is concatenated to the document root. if the document
	 * root is '/var/www', the final path becomes /var/www/b/c'.  */
	const qse_mchar_t* qpath_rp;

	qse_httpd_serverstd_root_t root;
	qse_httpd_serverstd_realm_t realm;
	qse_httpd_serverstd_auth_t auth;
	qse_httpd_serverstd_index_t index;

	int final_match;
};

static int attempt_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req, 
	struct rsrc_tmp_t* tmp, qse_httpd_rsrc_t* target)
{
	server_xtn_t* server_xtn = GET_SERVER_XTN(httpd, client->server);
	qse_mchar_t* shebang = QSE_NULL;
	qse_mchar_t* suffix = QSE_NULL;
	qse_mchar_t* script = QSE_NULL;
	qse_httpd_serverstd_cgi_t cgi;

	if (tmp->final_match)
	{
		qse_httpd_serverstd_query_info_t qinfo;

		/* it is a final match. tmp->xpath is tmp->root + tmp->qpath  */

		QSE_MEMSET (&qinfo, 0, QSE_SIZEOF(qinfo));
		qinfo.req = req;
		qinfo.xpath = tmp->xpath;

		if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_CGI, &qinfo, &cgi) >= 0 && cgi.cgi)
		{
			if (tmp->idxfile)
			{
				#if 0
				script = merge_paths (httpd, tmp->qpath, tmp->idxfile);
				if (script == QSE_NULL) goto oops;
				#endif

				/* create a relocation resource */
				QSE_MEMSET (target, 0, QSE_SIZEOF(*target));
				target->type = QSE_HTTPD_RSRC_RELOC;
				target->u.reloc.target = merge_paths (httpd, tmp->qpath, tmp->idxfile);
				if (target->u.reloc.target == QSE_NULL) goto oops;

				/* free tmp->xpath here upon success since it's not used for relocation.
				 * upon failure, it is freed by the caller. so the 'oops' part 
				 * of this function doesn't free it. */
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tmp->xpath);
				return 1;
			}
			else script = (qse_mchar_t*)tmp->qpath;

			goto bingo;
		}
	}
	else if (tmp->qpath_rp[0] != QSE_MT('\0'))
	{
		/* inspect each segment from the head. */
		const qse_mchar_t* ptr;
		const qse_mchar_t* slash;
		int xpath_changed = 0;

		QSE_ASSERT (tmp->qpath[0] == QSE_T('/'));

		ptr = tmp->qpath_rp + 1;
		while (*ptr != QSE_MT('\0'))
		{
			slash = qse_mbschr (ptr, QSE_MT('/'));
			if (slash)
			{
				if (slash > ptr)
				{
					qse_httpd_stat_t st;
					int stx;

					/* a slash is found and the segment is not empty.
					 *
					 * tmp->xpath should be large enough to hold the merge path made of
					 * the subsegments of the original query path and docroot. 
					 */
					merge_paths_to_buf (httpd, tmp->root.u.path.val, tmp->qpath_rp, slash - tmp->qpath_rp, tmp->xpath);
					xpath_changed = 1;

					stx = stat_file (httpd, tmp->xpath, &st, 0);
					if (stx <= -1) 
					{
						/* instead of stopping here, let's give a non-existent 
						 * segment to be a virtual cgi script(function pointer). */
						st.isdir = 0;
					}

					if (!st.isdir)
					{
						qse_httpd_serverstd_query_info_t qinfo;

						QSE_MEMSET (&qinfo, 0, QSE_SIZEOF(qinfo));
						qinfo.req = req;
						qinfo.xpath = tmp->xpath;
						qinfo.xpath_nx = (stx <= -1);

						if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_CGI, &qinfo, &cgi) >= 0 && cgi.cgi)
						{
							if (cgi.fncptr == QSE_NULL && stx <= -1) 
							{
								/* normal cgi script must exist. */
								break;
							}

							/* the script name is composed of the orginal query path.
							 * the pointer held in 'slash' is valid for tmp->qpath as
							 * tmp->qpath_rp is at most the tail part of tmp->qpath. */
							script = qse_mbsxdup (tmp->qpath, slash - tmp->qpath, qse_httpd_getmmgr(httpd));
							suffix = qse_mbsdup (slash, qse_httpd_getmmgr(httpd));
							if (!script || !suffix) goto oops;

							goto bingo;
						}
					}

					if (stx <= -1) 
					{
						/* stop at the current segment if stat() fails. 
						 * if the current segment can't be stat-ed, it's not likely that
						 * the next segment can be stat-ed successfully */
						break;
					}
				}

				ptr = slash + 1;
			}
			else 
			{
				/* no more slash is found. this is the last segment.
				 * the caller has called stat() against the last segment
				 * before having called this function. so it's known
				 * that the path disn't exist.
				 * 
				 * however, a virtual cgi script may not exist. a check
				 * for it is still required here */
				
				qse_httpd_serverstd_query_info_t qinfo;

				if (xpath_changed)
				{
					/* restore the tmp->xpath to the original value by 
					 * combining the full path with the document root. */
					merge_paths_to_buf (httpd, tmp->root.u.path.val, tmp->qpath_rp, (qse_size_t)-1, tmp->xpath);
					xpath_changed = 0;
				}

				QSE_MEMSET (&qinfo, 0, QSE_SIZEOF(qinfo));
				qinfo.req = req;
				qinfo.xpath = tmp->xpath;
				qinfo.xpath_nx = 1;

				if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_CGI, &qinfo, &cgi) >= 0 && cgi.cgi && cgi.fncptr)
				{
					/* virtual cgi script */
					script = qse_mbsdup (tmp->qpath, qse_httpd_getmmgr(httpd));
					if (!script) goto oops;
					suffix = QSE_NULL;

					goto bingo;
				}

				/* not a virtual cgi script. just break */
				break;
			}
		}

		/* restore xpath because it has changed... */
		if (xpath_changed) merge_paths_to_buf (httpd, tmp->root.u.path.val, tmp->qpath_rp, (qse_size_t)-1, tmp->xpath);
	}

	return 0; /* not a cgi */

bingo:
	target->type = QSE_HTTPD_RSRC_CGI;
	target->u.cgi.flags = 0;
	if (cgi.nph) target->u.cgi.flags |= QSE_HTTPD_RSRC_CGI_NPH;

	if (cgi.fncptr) 
	{
		/* the type casting here is guly */
		target->u.cgi.path = (qse_mchar_t*)cgi.fncptr;
		target->u.cgi.shebang = cgi.shebang;
		target->u.cgi.flags |= QSE_HTTPD_RSRC_CGI_FNC;
	}
	else 
	{
		if (cgi.shebang)
		{
			shebang = qse_mbsdup (cgi.shebang, qse_httpd_getmmgr(httpd));
			if (shebang == QSE_NULL) goto oops;
		}

		target->u.cgi.path = tmp->xpath;
		target->u.cgi.shebang = shebang;
	}

	target->u.cgi.script = script;
	target->u.cgi.suffix = suffix;
	target->u.cgi.root = tmp->root.u.path.val;

	return 1;

oops:
	httpd->errnum = QSE_HTTPD_ENOMEM;
	if (shebang) QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), shebang);
	if (suffix) QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), suffix);
	if (script && script != tmp->qpath) QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), script);
	return -1;
}

static int make_resource (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_htre_t* req, qse_httpd_rsrc_t* target)
{
	server_xtn_t* server_xtn = GET_SERVER_XTN(httpd, client->server);
	struct rsrc_tmp_t tmp;

	qse_httpd_stat_t st;
	int n, stx, acc;

	qse_httpd_serverstd_query_info_t qinfo;

	QSE_MEMSET (&tmp, 0, QSE_SIZEOF(tmp));
	tmp.qpath = qse_htre_getqpath(req);
	tmp.qpath_len = qse_mbslen (tmp.qpath);

	QSE_MEMSET (target, 0, QSE_SIZEOF(*target));

	QSE_MEMSET (&qinfo, 0, QSE_SIZEOF(qinfo));
	qinfo.req = req;
	qinfo.client = client;

	if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_ROOT, &qinfo, &tmp.root) <= -1) return -1;
	switch (tmp.root.type)
	{
		case QSE_HTTPD_SERVERSTD_ROOT_TEXT:
			target->type = QSE_HTTPD_RSRC_TEXT;
			target->u.text.ptr = tmp.root.u.text.ptr;
			target->u.text.mime = tmp.root.u.text.mime;
			return 0;

		case QSE_HTTPD_SERVERSTD_ROOT_PROXY:
			target->type = QSE_HTTPD_RSRC_PROXY;
			target->u.proxy = tmp.root.u.proxy;
			req->flags |= QSE_HTRE_ATTR_PROXIED;
			return 0;

		case QSE_HTTPD_SERVERSTD_ROOT_RELOC:
			target->type = QSE_HTTPD_RSRC_RELOC;
			target->u.reloc = tmp.root.u.reloc;
			return 0;

		case QSE_HTTPD_SERVERSTD_ROOT_ERROR:
			target->type = QSE_HTTPD_RSRC_ERROR;
			target->u.error.code = tmp.root.u.error.code;
			return 0;
	}

	/* handle the request locally */
	QSE_ASSERT (tmp.root.type == QSE_HTTPD_SERVERSTD_ROOT_PATH);

	if (tmp.qpath[0] != QSE_MT('/'))
	{
		/* this implementation doesn't accept a query path 
		 * not beginning with a slash when handling the request
		 * locally. */
		target->type = QSE_HTTPD_RSRC_ERROR;
		target->u.error.code = 400;
		return 0;
	}

/*****************************************************************************
 * BUG BUG BUG !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
 * TODO: calling the realm query here is wrong especially if the prefix path is resolved to a cgi.
 * for example, /abc/def/test.cgi/x/y/z, 
 * when this function queries for REALM, it's not known that /abc/def/test.cgi is a cgi script.

 *****************************************************************************/
	if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_REALM, &qinfo, &tmp.realm) <= -1 ||
	    server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_INDEX, &qinfo, &tmp.index) <= -1)
	{
		return -1;
	}

	/* default to the root directory. */
	if (!tmp.root.u.path.val) tmp.root.u.path.val = QSE_MT("/"); 

	/* query path /a/b/c
	 * location matched /a/b
	 * rpl is set to 4(/a/b). rpl, in fact, can't be larger than the query length.
	 * qpath_rp points to /c in /a/b/c
	 *
	 * query path /
	 * location matched /
	 * rpl is set to 1.
	 * qpath_rp points to an empty string (pointer a null character in the query path)
	 */ 
	tmp.qpath_rp = (tmp.root.u.path.rpl >= tmp.qpath_len)? 
		&tmp.qpath[tmp.qpath_len]: &tmp.qpath[tmp.root.u.path.rpl];

	if (tmp.realm.authreq && tmp.realm.name)
	{
		const qse_htre_hdrval_t* authv;

		authv = qse_htre_getheaderval (req, QSE_MT("Authorization"));
		if (authv)
		{
			/*while (authv->next) authv = authv->next;*/

			if (qse_mbszcasecmp(authv->ptr, QSE_MT("Basic "), 6) == 0) 
			{
				qse_size_t authl, authl2;

				/* basic authorization is a base64-encoded string of username:password. */

				authl = qse_mbslen(&authv->ptr[6]);	
				if (authl > server_xtn->auth.len)
				{
					qse_mchar_t* tptr;
					tptr = qse_httpd_reallocmem (httpd, server_xtn->auth.ptr, authl * QSE_SIZEOF(qse_mchar_t));
					if (!tptr) return -1;

					server_xtn->auth.ptr = tptr;
					/* the maximum capacity that can hold the largest authorization value */
					server_xtn->auth.len = authl;
				}

				/* decoding a base64-encoded string result in a shorter value than the input.
				 * so passing the length of the input(authl) as the output buffer size is ok */
				authl2 = qse_debase64 (&authv->ptr[6], authl, server_xtn->auth.ptr, authl, QSE_NULL);

				tmp.auth.key.ptr = server_xtn->auth.ptr;
				tmp.auth.key.len = authl2;
				if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_AUTH, &qinfo, &tmp.auth) >= 0 && tmp.auth.authok) goto auth_ok;
			}
		}

		target->type = QSE_HTTPD_RSRC_AUTH;
		target->u.auth.realm = tmp.realm.name; 
		return 0;
	}

auth_ok:

	/* if authentication is ok or no authentication is required,
	 * handle 'Expect: 100-continue' if it is contained in the header */
	if ((req->flags & QSE_HTRE_ATTR_EXPECT) &&
	    qse_comparehttpversions (&req->version, &qse_http_v11) >= 0 && 
	    qse_htre_getcontentlen(req) <= 0)
	{
		/* "Expect" in the header, version 1.1 or higher,
		 * and no content received yet. don't care about the method type. 
		 *
		 * if the partial or complete content is already received,
		 * we don't need to send '100 continue'. */

		if (req->flags & QSE_HTRE_ATTR_EXPECT100)
		{
			/* "Expect: 100-continue" in the header.
			 * mark to return "100 continue" */
			target->flags |= QSE_HTTPD_RSRC_100_CONTINUE;
		}
		else
		{
			/* Expectation Failed */
			qse_htre_discardcontent (req);
			target->type = QSE_HTTPD_RSRC_ERROR;
			target->u.error.code = 417;
			return 0;
		}
	}

	tmp.xpath = merge_paths (httpd, tmp.root.u.path.val, tmp.qpath_rp);
	if (tmp.xpath == QSE_NULL) return -1;

	stx = stat_file (httpd, tmp.xpath, &st, 0);
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (stx <= -1)
	{
		/* this OS may fail in stat_file() if the path contains the trailing 
		 * separator. it's beause of the way FindFirstFile() or DosQueryPathInfo()
		 * is used in stat_file(). let me work around it here. */
		qse_size_t pl = qse_mbslen(tmp.xpath);
		if (pl > 1 && tmp.xpath[pl - 1] == QSE_MT('/')) 
		{
			tmp.xpath[pl-1] = QSE_MT('\0');
			stx = stat_file (httpd, tmp.xpath, &st, 0);
			tmp.xpath[pl-1] = QSE_MT('/');
		}
	}
#endif

	if (stx >= 0)
	{
		/* xpath/qpath is a final match.
		 * mark that the segments in the query path don't need inspection. */
		tmp.final_match = 1;

		if (st.isdir)
		{
			/* it is a directory */
			if (tmp.index.count > 0)
			{
				/* try to locate an index file */
				qse_size_t i;
				const qse_mchar_t* ptr;

				ptr = tmp.index.files;
				for (i = 0; i < tmp.index.count; i++, ptr += qse_mbslen(ptr) + 1)
				{
					qse_mchar_t* tpath;

					tpath = merge_paths (httpd, tmp.xpath, ptr);
					if (tpath == QSE_NULL) 
					{
						QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tmp.xpath);
						return -1;
					}

					if (httpd->opt.scb.file.stat (httpd, tpath, &st) >= 0 && !st.isdir)
					{
						/* the index file is found */
						QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tmp.xpath);
						tmp.xpath = tpath;
						tmp.idxfile = ptr;
						goto attempt_file;
					}

					QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tpath);
				}
			}

			qinfo.xpath = tmp.xpath;

			/* it is a directory - should i allow it? */
			if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_DIRACC, &qinfo, &target->u.error.code) <= -1) target->u.error.code = 500;
			if (target->u.error.code < 200 || target->u.error.code > 299)
			{
				qse_htre_discardcontent (req);
				target->type = QSE_HTTPD_RSRC_ERROR;
				/* free xpath since it won't be used */
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tmp.xpath);
			}
			else if (tmp.qpath[tmp.qpath_len - 1] != QSE_MT('/'))
			{
				/* the query path doesn't end with a slash. so redirect it  */
				qse_htre_discardcontent (req);
				target->type = QSE_HTTPD_RSRC_RELOC;
				target->u.reloc.flags = QSE_HTTPD_RSRC_RELOC_APPENDSLASH | QSE_HTTPD_RSRC_RELOC_PERMANENT;
				target->u.reloc.target = tmp.qpath;
				/* free xpath since it won't be used */
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tmp.xpath);
			}
			else
			{
				target->type = QSE_HTTPD_RSRC_DIR;
				target->u.dir.path = tmp.xpath;
				if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_DIRHEAD, &qinfo, &target->u.dir.head) <= -1) target->u.dir.head = QSE_NULL;
				if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_DIRFOOT, &qinfo, &target->u.dir.foot) <= -1) target->u.dir.foot = QSE_NULL;
			}
		}
		else
		{
			/* let me treat it as a file. */
			goto attempt_file;
		}
	}
	else
	{
		/* well, stat failed. i don't know if it is a file.
		 * i must try each segment in the query path. */
	attempt_file:
		/* check if the request can resolve to a cgi script */
		n = attempt_cgi (httpd, client, req, &tmp, target);
		if (n <= -1) 
		{
			QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tmp.xpath);
			return -1;
		}
		if (n >= 1) 
		{
			if (target->u.cgi.flags & QSE_HTTPD_RSRC_CGI_FNC)
			{
				/* tmp.xpath is not set to target->u.cgi.path when
				 * this flag is set. it must be deallocated */
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tmp.xpath);
			}
			return 0;
		}

		qinfo.xpath = tmp.xpath;

		/* check file's access permission */
		acc = (tmp.idxfile || !qse_mbsend(tmp.qpath, QSE_MT("/")))? 
			QSE_HTTPD_SERVERSTD_FILEACC: QSE_HTTPD_SERVERSTD_DIRACC;
		if (server_xtn->query (httpd, client->server, acc, &qinfo, &target->u.error.code) <= -1) target->u.error.code = 500;

		if (target->u.error.code < 200 || target->u.error.code > 299)
		{
			/* free xpath since it won't be used */
			qse_htre_discardcontent (req);
			QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tmp.xpath);
			target->type = QSE_HTTPD_RSRC_ERROR;
		}
		else
		{
			/* fall back to a normal file. */
			if (tmp.idxfile)
			{
				qse_htre_discardcontent (req);

				/* free xpath since it won't be used */
				QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), tmp.xpath);

				/* create a relocation resource */
				target->type = QSE_HTTPD_RSRC_RELOC;
				target->u.reloc.target = merge_paths (httpd, tmp.qpath, tmp.idxfile);
				if (target->u.reloc.target == QSE_NULL) return -1;
			}
			else if (acc == QSE_HTTPD_SERVERSTD_DIRACC)
			{
				target->type = QSE_HTTPD_RSRC_DIR;
				target->u.dir.path = tmp.xpath;

				if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_DIRHEAD, &qinfo, &target->u.dir.head) <= -1) target->u.dir.head = QSE_NULL;
				if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_DIRFOOT, &qinfo, &target->u.dir.foot) <= -1) target->u.dir.foot = QSE_NULL;
			}
			else
			{
				target->type = QSE_HTTPD_RSRC_FILE;
				target->u.file.path = tmp.xpath;

				if (server_xtn->query (httpd, client->server, QSE_HTTPD_SERVERSTD_MIME, &qinfo, &target->u.file.mime) <= -1)
				{
					/* don't care about failure */
					target->u.file.mime = QSE_NULL;
				}
			}
		}
	}

	return 0;
}

/* ------------------------------------------------------------------- */

static void detach_server (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	server_xtn_t* server_xtn = GET_SERVER_XTN(httpd, server);
	if (server_xtn->detach) server_xtn->detach (httpd, server);
	if (server_xtn->auth.ptr) QSE_MMGR_FREE (qse_httpd_getmmgr(httpd), server_xtn->auth.ptr);

#if defined(USE_SSL)
	if (server_xtn->ssl_ctx) fini_server_ssl (server_xtn);
#endif
}

struct mime_tab_t
{
	const qse_mchar_t* suffix;
	const qse_mchar_t* type;
};
static struct mime_tab_t mimetab[] =
{
	{ QSE_MT(".htm"),  QSE_MT("text/html") },
	{ QSE_MT(".html"), QSE_MT("text/html") },
	{ QSE_MT(".txt"),  QSE_MT("text/plain") }
};

struct cgi_tab_t
{
	const qse_mchar_t* suffix;
	qse_httpd_serverstd_cgi_t cgi;
};
static struct cgi_tab_t cgitab[] =
{
	{ QSE_MT(".cgi"), { 1, 0, QSE_NULL, QSE_NULL } },
	{ QSE_MT(".nph"), { 1, 1, QSE_NULL, QSE_NULL } },
};

static int query_server (
	qse_httpd_t* httpd, qse_httpd_server_t* server, 
	qse_httpd_serverstd_query_code_t code, 
	const qse_httpd_serverstd_query_info_t* qinfo,
	void* result)
{
	qse_size_t i;

	switch (code)
	{
		case QSE_HTTPD_SERVERSTD_SSL:
			/* you must specify the certificate and the key file to be able
			 * to use SSL. not supported by this sample implmentation. */
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOENT);
			return -1;

		case QSE_HTTPD_SERVERSTD_ROOT:
			((qse_httpd_serverstd_root_t*)result)->type = QSE_HTTPD_SERVERSTD_ROOT_PATH;
			((qse_httpd_serverstd_root_t*)result)->u.path.val = QSE_NULL;
			((qse_httpd_serverstd_root_t*)result)->u.path.rpl = 0;
			break;
		
		case QSE_HTTPD_SERVERSTD_REALM:
			((qse_httpd_serverstd_realm_t*)result)->name = QSE_NULL;
			((qse_httpd_serverstd_realm_t*)result)->authreq = 0;
			break;

		case QSE_HTTPD_SERVERSTD_AUTH:
			((qse_httpd_serverstd_auth_t*)result)->authok = 1;
			break;
			
		case QSE_HTTPD_SERVERSTD_ERRHEAD:
		case QSE_HTTPD_SERVERSTD_DIRHEAD:
			*(const qse_mchar_t**)result = QSE_NULL;
			return 0;

		case QSE_HTTPD_SERVERSTD_ERRFOOT:
		case QSE_HTTPD_SERVERSTD_DIRFOOT:
			*(const qse_mchar_t**)result = qse_httpd_getname(httpd);
			return 0;

		case QSE_HTTPD_SERVERSTD_INDEX:
		{
			qse_httpd_serverstd_index_t* index = (qse_httpd_serverstd_index_t*)result;
			index->count = 2;
			index->files = QSE_MT("index.html\0index.cgi\0");
			return 0;
		}

		case QSE_HTTPD_SERVERSTD_CGI:
		{
			qse_httpd_serverstd_cgi_t* cgi = (qse_httpd_serverstd_cgi_t*)result;

			if (!qinfo->xpath_nx) 
			{
				/* this standard implementation supports a normal cgi script only */

				for (i = 0; i < QSE_COUNTOF(cgitab); i++)
				{
					if (qse_mbsend (qinfo->xpath, cgitab[i].suffix))
					{
						QSE_MEMCPY (cgi, &cgitab[i].cgi, QSE_SIZEOF(*cgi));
						return 0;
					}
				}
			}

			QSE_MEMSET (cgi,0, QSE_SIZEOF(*cgi));
			return 0;
		}

		case QSE_HTTPD_SERVERSTD_MIME:
			/* TODO: binary search if the table is large */
			for (i = 0; i < QSE_COUNTOF(mimetab); i++)
			{
				if (qse_mbsend (qinfo->xpath, mimetab[i].suffix))
				{
					*(const qse_mchar_t**)result = mimetab[i].type;
					return 0;
				}
			}

			*(const qse_mchar_t**)result = QSE_NULL;
			return 0;

		case QSE_HTTPD_SERVERSTD_DIRACC:
		case QSE_HTTPD_SERVERSTD_FILEACC:
		{
			/* i don't allow PUT or DELET by default.
			 * override this query result if you want to change
			 * the behavior. */
			switch (qse_htre_getqmethodtype(qinfo->req))
			{
				case QSE_HTTP_OPTIONS:
				case QSE_HTTP_HEAD:
				case QSE_HTTP_GET:
				case QSE_HTTP_POST:
					*(int*)result = 200;
					break;
		
				default:
					/* method not allowed */
					*(int*)result = 405;
					break;
			}
			return 0;
		}
	}

	qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
	return -1;
}

qse_httpd_server_t* qse_httpd_attachserverstd (
	qse_httpd_t* httpd, const qse_httpd_server_dope_t* dope, qse_size_t xtnsize)
{
	qse_httpd_server_dope_t xdope;
	qse_httpd_server_t* xserver;
	server_xtn_t* server_xtn;

	xdope = *dope;
	/* detach_server() is called when the server is detached */
	xdope.detach = detach_server;

	xserver = qse_httpd_attachserver(httpd, &xdope, QSE_SIZEOF(*server_xtn) + xtnsize);
	if (xserver == QSE_NULL) return QSE_NULL;

	xserver->_instsize += QSE_SIZEOF(*server_xtn);

	server_xtn = GET_SERVER_XTN(httpd, xserver);
	QSE_MEMSET (server_xtn, 0, QSE_SIZEOF(*server_xtn));

	/* chain the original detach function */
	server_xtn->detach = dope->detach;

	server_xtn->query = query_server;
	server_xtn->makersrc = make_resource;
	server_xtn->freersrc = free_resource;

	/* init_server_ssl() queries the server instance for the SSL key and cert files.
	 * the standard query handler 'query_server' returns failure for this.
	 * the actual handler can be set with qse_httpd_setserverstdopt().
	 * calling init_server_ssl() causes server attachment to fail for this reason.
	 * so initialization of ssl must be delayed. So the call is commented out.
	if (init_server_ssl (httpd, xserver) <= -1)
	{
		/ * reset server_xtn->detach as i want detach_server() to 
		 * skip calling dope->detach. dope->detach should be called
		 * upon detachment only if qse_httpd_attachserverstd() has
		 * been successful. * /
		server_xtn->detach = QSE_NULL;
		qse_httpd_detachserver (httpd, xserver);
		xserver = QSE_NULL;
	}
	*/

	return xserver;
}

#if 0
qse_httpd_server_t* qse_httpd_attachserverstdwithuri (
	qse_httpd_t* httpd, const qse_char_t* uri, 
	qse_httpd_server_detach_t detach, 
	qse_httpd_serverstd_query_t query, 
	qse_size_t xtnsize)
{
	qse_httpd_serverstd_t server;
	qse_uint16_t default_port;
	qse_uri_t xuri;

	QSE_MEMSET (&server, 0, QSE_SIZEOF(server));

	if (qse_strtouri (uri, &xuri, QSE_STRTOURI_NOQUERY) <= -1)  goto invalid;

	if (qse_strxcasecmp (xuri.scheme.ptr, xuri.scheme.len, QSE_T("http")) == 0) 
	{
		default_port = DEFAULT_PORT;
	}
	else if (qse_strxcasecmp (xuri.scheme.ptr, xuri.scheme.len, QSE_T("https")) == 0) 
	{
		server.flags |= QSE_HTTPD_QSE_HTTPD_SERVER_SECURE;
		default_port = QSE_HTTPD_DEFAULT_SECURE_PORT;
	}
	else goto invalid;

	if (qse_strntonwad (
		xuri.host.ptr, 
		xuri.host.len + (xuri.port.ptr? (xuri.port.len + 1): 0),
		&server.nwad) <= -1) goto invalid;

	if (server.nwad.type == QSE_NWAD_IN4)
	{
		if (server.nwad.u.in4.port == 0)
			server.nwad.u.in4.port = qse_hton16(default_port);
	}
	else if (server.nwad.type == QSE_NWAD_IN6)
	{
		if (server.nwad.u.in6.port == 0) 
			server.nwad.u.in6.port = qse_hton16(default_port);
	}

	server.detach = detach;
#if 0
	server.docroot = xuri.path;
	if (server.docroot.ptr && qse_ismbsdriveabspath((const qse_mchar_t*)server.docroot.ptr + 1))
	{
		/* if the path name is something like /C:/xxx on support platforms ... */
		server.docroot.ptr++;
		server.docroot.len--;
	}
	server.realm = xuri.frag;
	server.user = xuri.auth.user;
	server.pass = xuri.auth.pass;
#endif
	return qse_httpd_attachserverstd (httpd, &server, xtnsize);

invalid:
	httpd->errnum = QSE_HTTPD_EINVAL;
	return QSE_NULL;
}
#endif

int qse_httpd_getserverstdopt (
	qse_httpd_t* httpd, qse_httpd_server_t* server,
	qse_httpd_serverstd_opt_t id, void* value)
{
	server_xtn_t* server_xtn = GET_SERVER_XTN(httpd, server);

	switch (id)
	{
		case QSE_HTTPD_SERVERSTD_QUERY:
			*(qse_httpd_serverstd_query_t*)value = server_xtn->query;
			return 0;

		case QSE_HTTPD_SERVERSTD_MAKERSRC:
			*(qse_httpd_serverstd_makersrc_t*)value = server_xtn->makersrc;
			return 0;

		case QSE_HTTPD_SERVERSTD_FREERSRC:
			*(qse_httpd_serverstd_freersrc_t*)value = server_xtn->freersrc;
			return 0;
	}	

	httpd->errnum = QSE_HTTPD_EINVAL;
	return -1;
}


int qse_httpd_setserverstdopt (
	qse_httpd_t* httpd, qse_httpd_server_t* server,
	qse_httpd_serverstd_opt_t id, const void* value)
{
	server_xtn_t* server_xtn = GET_SERVER_XTN(httpd, server);

	switch (id)
	{
		case QSE_HTTPD_SERVERSTD_QUERY:
			server_xtn->query = (qse_httpd_serverstd_query_t)value;
			return 0;

		case QSE_HTTPD_SERVERSTD_MAKERSRC:
			server_xtn->makersrc = (qse_httpd_serverstd_makersrc_t)value;
			return 0;

		case QSE_HTTPD_SERVERSTD_FREERSRC:
			server_xtn->freersrc = (qse_httpd_serverstd_freersrc_t)value;
			return 0;
	}	

	httpd->errnum = QSE_HTTPD_EINVAL;
	return -1;
}

/* ------------------------------------------------------------------- */

int qse_httpd_loopstd (qse_httpd_t* httpd, const qse_httpd_dnsstd_t* dns, const qse_httpd_ursstd_t* urs)
{
	httpd_xtn_t* httpd_xtn = GET_HTTPD_XTN(httpd);

	/* default dns server info */
	if (dns)
	{
		httpd_xtn->dns = *dns;
	}
	else
	{
		httpd_xtn->dns.nwad.type = QSE_NWAD_NX;
		httpd_xtn->dns.tmout.sec = QSE_HTTPD_DNSSTD_DEFAULT_TMOUT;
		httpd_xtn->dns.tmout.nsec = 0;
		httpd_xtn->dns.retries = QSE_HTTPD_DNSSTD_DEFAULT_RETRIES;
		httpd_xtn->dns.cache_ttl = QSE_HTTPD_DNSSTD_DEFAULT_CACHE_TTL;
		httpd_xtn->dns.cache_minttl = QSE_HTTPD_DNSSTD_DEFAULT_CACHE_MINTTL;
		httpd_xtn->dns.cache_negttl = QSE_HTTPD_DNSSTD_DEFAULT_CACHE_NEGTTL;
	}

	/* default urs server info */
	if (urs)
	{
		httpd_xtn->urs = *urs;
	}
	else
	{
		httpd_xtn->urs.nwad.type = QSE_NWAD_NX;
		httpd_xtn->urs.tmout.sec = QSE_HTTPD_URSSTD_DEFAULT_TMOUT;
		httpd_xtn->urs.tmout.nsec = 0;
		httpd_xtn->urs.retries = QSE_HTTPD_URSSTD_DEFAULT_RETRIES;
	}

	/* main loop */
	return qse_httpd_loop (httpd);
}

