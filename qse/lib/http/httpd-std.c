/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <htrd://www.gnu.org/licenses/>.
 */

#include <qse/http/stdhttpd.h>
#include "httpd.h"
#include "../cmn/mem.h"
#include <qse/cmn/hton.h>
#include <qse/cmn/nwif.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/str.h>
#include <qse/cmn/uri.h>
#include <qse/cmn/alg.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/path.h>
#include <qse/cmn/mux.h>
#include <qse/cmn/dir.h>
#include <qse/cmn/fio.h>

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h> /* sockaddr_in6 */
#	include <windows.h>

#	define EPOCH_DIFF_YEARS (QSE_EPOCH_YEAR-QSE_EPOCH_YEAR_WIN)
#	define EPOCH_DIFF_DAYS  ((qse_long_t)EPOCH_DIFF_YEARS*365+EPOCH_DIFF_YEARS/4-3)
#	define EPOCH_DIFF_SECS  ((qse_long_t)EPOCH_DIFF_DAYS*24*60*60)

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
#	include <os2.h>

#elif defined(__DOS__)
	/* TODO */
#	include <errno.h>

#else
#	include "../cmn/syscall.h"
#	include <sys/socket.h>
#	include <netinet/in.h>
#	if defined(HAVE_SYS_SENDFILE_H)
#		include <sys/sendfile.h>
#	endif
#	if defined(HAVE_SYS_EPOLL_H)
#		include <sys/epoll.h>
#	endif
#	if defined(__linux__)
#		include <limits.h>
#		include <linux/netfilter_ipv4.h> /* SO_ORIGINAL_DST */
#		if !defined(IP_TRANSPARENT)
#			define IP_TRANSPARENT 19
#		endif
#	endif
#endif

#if defined(HAVE_SSL)
#	include <openssl/ssl.h>
#	include <openssl/err.h>
#	include <openssl/engine.h>
#endif

#include <stdio.h> /* TODO: remove this */

#define DEFAULT_PORT        80
#define DEFAULT_SECURE_PORT 443

typedef struct server_xtn_t server_xtn_t;
struct server_xtn_t
{
	qse_httpd_server_detach_t detach;

	qse_httpd_serverstd_query_t query;
	qse_httpd_serverstd_makersrc_t makersrc;
	qse_httpd_serverstd_freersrc_t freersrc;

	/* temporary buffer to handle authorization */
	qse_mxstr_t auth;
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

#elif defined(__DOS__)
static qse_httpd_errnum_t skerr_to_errnum (int e)
{
	/* TODO: */
	return QSE_HTTPD_ESYSERR;
}

#define SKERR_TO_ERRNUM() skerr_to_errnum(errno)

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

		case EAGAIN:
			return QSE_HTTPD_EAGAIN;

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

#define MAX_SEND_SIZE 4096

static QSE_INLINE qse_ssize_t __send_file (
	qse_httpd_t* httpd, int out_fd, qse_ubi_t in_fd, 
	qse_foff_t* offset, qse_size_t count)
{
	/* TODO: os2 warp 4.5 has send_file. support it??? load it dynamically??? */

#if defined(__DOS__)
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;

#elif defined(HAVE_SENDFILE) && defined(HAVE_SENDFILE64)

	qse_ssize_t ret;
	qse_ubi_t infd = qse_fio_gethandleasubi (in_fd.ptr);

	#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_SENDFILE64)
	ret =  sendfile64 (out_fd, infd.i, offset, count);
	#else
	ret =  sendfile (out_fd, infd.i, offset, count);
	#endif
	if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	return ret;

#elif defined(HAVE_SENDFILE)

	qse_ssize_t ret;
	qse_ubi_t infd = qse_fio_gethandleasubi (in_fd.ptr);
	ret = sendfile (out_fd, infd.i, offset, count);
	if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	return ret;

#elif defined(HAVE_SENDFILE64)
	qse_ssize_t ret;
	qse_ubi_t infd = qse_fio_gethandleasubi (in_fd.ptr);
	ret = sendfile64 (out_fd, in_fd.i, offset, count);
	if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	return ret;

#elif defined(HAVE_SENDFILEV) || defined(HAVE_SENDFILEV64)

	#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_SENDFILE64)
	struct sendfilevec64 vec;
	#else
	struct sendfilevec vec;
	#endif
	size_t xfer;
	ssize_t ret;

	vec.sfv_fd = in_fd.i;
	vec.sfv_flag = 0;
	if (offset)
	{
		vec.sfv_off = *offset;
	}
	else
	{
		vec.sfv_off = QSE_LSEEK (in_fd.i, 0, SEEK_CUR); 
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

	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_ssize_t ret;
	qse_foff_t foff;

	if (offset && (foff = qse_fio_seek (in_fd.ptr, *offset, QSE_FIO_BEGIN)) != *offset)  
	{
		if (foff == (qse_foff_t)-1) 	
			qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(in_fd.ptr)));	
		else
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		return (qse_ssize_t)-1;
	}

	if (count > QSE_COUNTOF(buf)) count = QSE_COUNTOF(buf);
	ret = qse_fio_read (in_fd.ptr, buf, count);
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
		qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(in_fd.ptr)));	
	}

	return ret;

#endif
}

/* ------------------------------------------------------------------- */

static QSE_INLINE qse_ssize_t __send_file_ssl (
	qse_httpd_t* httpd, void* xout, qse_ubi_t in_fd, 
	qse_foff_t* offset, qse_size_t count)
{
#if defined(HAVE_SSL)
	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_ssize_t ret;
	qse_foff_t foff;
	SSL* out = (SSL*)xout;
	
	if (offset && (foff = qse_fio_seek (in_fd.ptr, *offset, QSE_FIO_BEGIN)) != *offset)  
	{
		if (foff == (qse_foff_t)-1) 	
			qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(in_fd.ptr)));	
		else
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		return (qse_ssize_t)-1;
	}

	if (count > QSE_COUNTOF(buf)) count = QSE_COUNTOF(buf);
	ret = qse_fio_read (in_fd.ptr, buf, count);
	if (ret > 0)
	{
		ret = SSL_write (out, buf, count);
		if (ret > 0)
		{
			if (offset) *offset = *offset + ret;
		}
		else if (ret <= -1)
		{
			if (SSL_get_error(out, ret) == SSL_ERROR_WANT_WRITE)
				qse_httpd_seterrnum (httpd, QSE_HTTPD_EAGAIN);
			else
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		}
	}
	else if (ret <= -1)
	{
		qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(in_fd.ptr)));	
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
#if defined(HAVE_SSL)
	SSL_CTX* ssl_ctx;
#endif
	qse_httpd_ecb_t ecb;
};

#if defined(HAVE_SSL)
static int init_xtn_ssl (
	qse_httpd_t* httpd,
	const qse_mchar_t* pemfile,
	const qse_mchar_t* keyfile/*,
	const qse_mchar_t* chainfile*/)
{
	SSL_CTX* ctx;
	httpd_xtn_t* xtn;

	xtn = (httpd_xtn_t*)qse_httpd_getxtn (httpd);

	ctx = SSL_CTX_new (SSLv23_server_method());
	if (ctx == QSE_NULL) return -1;

	/*SSL_CTX_set_info_callback(ctx,ssl_info_callback);*/

	if (SSL_CTX_use_certificate_file (ctx, pemfile, SSL_FILETYPE_PEM) == 0 ||
	    SSL_CTX_use_PrivateKey_file (ctx, keyfile, SSL_FILETYPE_PEM) == 0 ||
	    SSL_CTX_check_private_key (ctx) == 0 /*||
	    SSL_CTX_use_certificate_chain_file (ctx, chainfile) == 0*/)
	{
		if (httpd->opt.trait & QSE_HTTPD_LOGACT)
		{
			qse_httpd_act_t msg;
			msg.code = QSE_HTTPD_CATCH_MERRMSG;
			ERR_error_string_n (ERR_get_error(), msg.u.merrmsg, QSE_COUNTOF(msg.u.merrmsg));
			httpd->opt.rcb.logact (httpd, &msg);
		}

		SSL_CTX_free (ctx);
		return -1;
	}


	/* TODO: CRYPTO_set_id_callback (); */
	/* TODO: CRYPTO_set_locking_callback (); */

	SSL_CTX_set_read_ahead (ctx, 0);
	xtn->ssl_ctx = ctx;
	return 0;
}

static void fini_xtn_ssl (httpd_xtn_t* xtn)
{
	/* TODO: CRYPTO_set_id_callback (QSE_NULL); */
	/* TODO: CRYPTO_set_locking_callback (QSE_NULL); */
	SSL_CTX_free (xtn->ssl_ctx);
}
#endif

/* ------------------------------------------------------------------- */

static void cleanup_standard_httpd (qse_httpd_t* httpd)
{
#if defined(HAVE_SSL)
	httpd_xtn_t* xtn;
	xtn = (httpd_xtn_t*)qse_httpd_getxtn (httpd);
	if (xtn->ssl_ctx) fini_xtn_ssl (xtn);
#endif
}

qse_httpd_t* qse_httpd_openstd (qse_size_t xtnsize)
{
	return qse_httpd_openstdwithmmgr (QSE_MMGR_GETDFL(), xtnsize);
}

qse_httpd_t* qse_httpd_openstdwithmmgr (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_httpd_t* httpd;
	httpd_xtn_t* xtn;

	httpd = qse_httpd_open (mmgr, QSE_SIZEOF(httpd_xtn_t) + xtnsize);
	if (httpd == QSE_NULL) return QSE_NULL;

	xtn = (httpd_xtn_t*)qse_httpd_getxtn (httpd);

#if defined(HAVE_SSL)
	/*init_xtn_ssl (httpd, "http01.pem", "http01.key");*/
#endif

	set_httpd_callbacks (httpd);

	xtn->ecb.close = cleanup_standard_httpd;
	qse_httpd_pushecb (httpd, &xtn->ecb);

	return httpd;	
}

void* qse_httpd_getxtnstd (qse_httpd_t* httpd)
{
	return (void*)((httpd_xtn_t*)QSE_XTN(httpd) + 1);
}

/* ------------------------------------------------------------------- */

static int server_open (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
#if defined(__DOS__)
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#else
	int fd = -1, flag;
	qse_skad_t addr;
	int addrsize;

	addrsize = qse_nwadtoskad (&server->dope.nwad, &addr);
	if (addrsize <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	}

	fd = socket (qse_skadfamily(&addr), SOCK_STREAM, IPPROTO_TCP);
	if (fd <= -1) goto oops;

	#if defined(FD_CLOEXEC)
	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);
	#endif

	#if defined(SO_REUSEADDR)
	flag = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (void*)&flag, QSE_SIZEOF(flag));
	#endif

/* TODO: linux. use capset() to set required capabilities just in case */
	#if defined(IP_TRANSPARENT)
	/* remove the ip routing restriction that a packet can only
	 * be sent using a local ip address. this option is useful
	 * if transparency is achieved with TPROXY */

	/*
	ip rule add fwmark 0x1/0x1 lookup 100
	ip route add local 0.0.0.0/0 dev lo table 100

	iptables -t mangle -N DIVERT
	iptables -t mangle -A PREROUTING -p tcp -m socket --transparent -j DIVERT
	iptables -t mangle -A DIVERT -j MARK --set-mark 0x1/0x1
	iptables -t mangle -A DIVERT -j ACCEPT

	iptables -t mangle -A PREROUTING -p tcp --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-port 8000

	----------------------------------------------------------------------

	if the socket is bound to 99.99.99.99:8000, you may do...
	iptables -t mangle -A PREROUTING -p tcp --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-ip 99.99.99.99 --on-port 8000

	iptables -t mangle -A PREROUTING -p tcp  ! -s 127.0.0.0/255.0.0.0 --dport 80 -j TPROXY --tproxy-mark 0x1/0x1 --on-ip 0.0.0.0 --on-port 8000

	IP_TRANSPRENT is needed for:
	- accepting TPROXYed connections
	- binding to a non-local IP address (IP address the local system doesn't have)
	- using a non-local IP address as a source
	- 
	 */
	flag = 1;
	setsockopt (fd, SOL_IP, IP_TRANSPARENT, &flag, QSE_SIZEOF(flag));
	#endif

	if (server->dope.flags & QSE_HTTPD_SERVER_BINDTONWIF)
	{
	#if defined(SO_BINDTODEVICE)
		qse_mchar_t tmp[64];
		qse_size_t len;

		len = qse_nwifindextombs (server->dope.nwif, tmp, QSE_COUNTOF(tmp));

		if (len <= 0 || setsockopt (fd, SOL_SOCKET, SO_BINDTODEVICE, tmp, len) <= -1)
		{
			/* TODO: logging ... */
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
			if (bind (fd, (struct sockaddr*)&addr, addrsize) <= -1)  goto oops;
		}
		else goto oops;
	#else
		goto oops;
	#endif
	}


	if (listen (fd, 10) <= -1) goto oops;

	#if defined(O_NONBLOCK)
	flag = fcntl (fd, F_GETFL);
	if (flag >= 0) fcntl (fd, F_SETFL, flag | O_NONBLOCK);
	#endif

	server->handle.i = fd;
	return 0;

oops:
	qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	#if defined(_WIN32)
	if (fd != INVALID_SOCKET) closesocket (fd);
	#elif defined(__OS2__)
	if (fd >= 0) soclose (fd);
	#elif defined(__DOS__)
		/* TODO: */
	#else
	if (fd >= 0) QSE_CLOSE (fd);
	#endif
	return -1;
#endif
}

static void server_close (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
#if defined(_WIN32)
	closesocket (server->handle.i);
#elif defined(__OS2__)
	soclose (server->handle.i);
#elif defined(__DOS__)
	/* TODO: */
#else
	QSE_CLOSE (server->handle.i);
#endif
}

static int server_accept (
	qse_httpd_t* httpd, qse_httpd_server_t* server, qse_httpd_client_t* client)
{
#if defined(__DOS__)
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;

#else
	qse_skad_t addr;

	#if defined(HAVE_SOCKLEN_T)
	socklen_t addrlen;
	#else
	int addrlen;
	#endif
	int fd, flag;

	addrlen = QSE_SIZEOF(addr);
	fd = accept (server->handle.i, (struct sockaddr*)&addr, &addrlen);
	if (fd <= -1)
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		return -1;
	}

	#if 0
	if (fd >= FD_SETSIZE)
	{
qse_fprintf (QSE_STDERR, QSE_T("Error: too many client?\n"));
		/*TODO: qse_httpd_seterrnum (httpd, QSE_HTTPD_EXXXXX);*/
		QSE_CLOSE (fd);
		return -1;
	}
	#endif

	#if defined(FD_CLOEXEC)
	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);
	#endif

	#if defined(O_NONBLOCK)
	flag = fcntl (fd, F_GETFL);
	if (flag >= 0) fcntl (fd, F_SETFL, flag | O_NONBLOCK);
	#endif

	if (qse_skadtonwad (&addr, &client->remote_addr) <= -1)
	{
/* TODO: logging */
		client->remote_addr = server->dope.nwad;
	}

	addrlen = QSE_SIZEOF(addr);
	if (getsockname (fd, (struct sockaddr*)&addr, &addrlen) <= -1 ||
	    qse_skadtonwad (&addr, &client->local_addr) <= -1)
	{
/* TODO: logging */
		client->local_addr = server->dope.nwad;
	}

	#if defined(SO_ORIGINAL_DST)
	/* if REDIRECT is used, SO_ORIGINAL_DST returns the original
	 * destination. If TPROXY is used, getsockname() above returns
	 * the original address. */

	addrlen = QSE_SIZEOF(addr);
	if (getsockopt (fd, SOL_IP, SO_ORIGINAL_DST, (char*)&addr, &addrlen) <= -1 ||
	    qse_skadtonwad (&addr, &client->orgdst_addr) <= -1)
	{
		client->orgdst_addr = client->local_addr;
	}
	#else
	client->orgdst_addr = client->local_addr;
	#endif


	#if 0
	client->initial_ifindex = resolve_ifindex (fd, client->local_addr);
	if (client->ifindex <= -1)
	{
		/* the local_address is not one of a local address.
		 * it's probably proxied. */
	}
	#endif

	client->handle.i = fd;
	return 0;
#endif
}

/* ------------------------------------------------------------------- */

static int peer_open (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
#if defined(__DOS__)
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;

#else

	/* -------------------------------------------------------------------- */

	qse_skad_t connaddr, bindaddr;
	int connaddrsize, bindaddrsize;
	int connected = 1;
#if defined(_WIN32)
	SOCKET fd = -1; 
	unsigned long cmd;
#elif defined(__OS2__)
	int fd = -1; 
	int cmd;
#elif defined(__DOS__)
	int fd = -1; 
	int flag;
#else
	int fd = -1; 
	int flag;
#endif

	connaddrsize = qse_nwadtoskad (&peer->nwad, &connaddr);
	bindaddrsize = qse_nwadtoskad (&peer->local, &bindaddr);
	if (connaddrsize <= -1 || bindaddrsize <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	}

	fd = socket (qse_skadfamily(&connaddr), SOCK_STREAM, IPPROTO_TCP);
	if (fd <= -1) goto oops;

	#if defined(IP_TRANSPARENT)
	flag = 1;
	setsockopt (fd, SOL_IP, IP_TRANSPARENT, &flag, QSE_SIZEOF(flag));
	#endif

	if (bind (fd, (struct sockaddr*)&bindaddr, bindaddrsize) <= -1) 
	{
		/* i won't care about binding faiulre */
		/* TODO: some logging for this failure though */
	}

#if defined(_WIN32)
	cmd = 1;
	if (ioctlsocket(fd, FIONBIO, &cmd) == SOCKET_ERROR) goto oops;

	if (connect (fd, (struct sockaddr*)&connaddr, connaddrsize) <= -1)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK) goto oops;
		connected = 0;
	}

	cmd = 0;
	if (ioctlsocket(fd, FIONBIO, &cmd) == SOCKET_ERROR) goto oops;

#elif defined(__OS2__)

	cmd = 1;
	if (ioctl(fd, FIONBIO, &cmd, QSE_SIZEOF(cmd)) == -1) goto oops;

	if (connect (fd, (struct sockaddr*)&connaddr, connaddrsize) == -1)
	{
		if (sock_errno() != SOCEINPROGRESS) goto oops;
		connected = 0;
	}

	cmd = 0;
	if (ioctl(fd, FIONBIO, &cmd, QSE_SIZEOF(cmd)) == -1) goto oops;

#elif defined(__DOS__)

	/* TODO: */

#else

	#if defined(FD_CLOEXEC)
	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);
	#endif

	flag = fcntl (fd, F_GETFL);
	if (flag >= 0) fcntl (fd, F_SETFL, flag | O_NONBLOCK);

	if (connect (fd, (struct sockaddr*)&connaddr, connaddrsize) <= -1)
	{
		if (errno != EINPROGRESS) goto oops;
		connected = 0;
	}

	/* restore flags */
	if (fcntl (fd, F_SETFL, flag) <= -1) goto oops;

#endif

	peer->handle.i = fd;
	return connected;

oops:
	qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
#if defined(_WIN32)
	if (fd != INVALID_SOCKET) closesocket (fd);
#elif defined(__OS2__)
	if (fd >= 0) soclose (fd);
#elif defined(__DOS__)
	/* TODO: */
#else
	if (fd >= 0) QSE_CLOSE (fd);
#endif
	return -1;

	/* -------------------------------------------------------------------- */
#endif
}

static void peer_close (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
#if defined(_WIN32)
	closesocket (peer->handle.i);
#elif defined(__OS2__)
	soclose (peer->handle.i);
#elif defined(__DOS__)
	/* TODO: */
#else
	QSE_CLOSE (peer->handle.i);
#endif
}

static int peer_connected (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
#if defined(_WIN32)
	int len;
	DWORD ret;

	len = QSE_SIZEOF(ret);
	if (getsockopt (peer->handle.i, SOL_SOCKET, SO_ERROR, (char*)&ret, &len) == SOCKET_ERROR) 
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
	if (getsockopt (peer->handle.i, SOL_SOCKET, SO_ERROR, (char*)&ret, &len) == -1)
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

#elif defined(__DOS__)

	/* TODO */
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;

#else

	#if defined(HAVE_SOCKLEN_T)
	socklen_t len;
	#else
	int len;
	#endif
	int ret;

	len = QSE_SIZEOF(ret);
	if (getsockopt (peer->handle.i, SOL_SOCKET, SO_ERROR, &ret, &len) <= -1) 
	{
		qse_httpd_seterrnum (httpd, skerr_to_errnum (ret));
		return -1;
	}

	if (ret == EINPROGRESS) return 0;
	if (ret != 0)
	{
		qse_httpd_seterrnum (httpd, skerr_to_errnum (ret));
		return -1;
	}

	return 1; /* connection completed */
#endif
}

static qse_ssize_t peer_recv (
	qse_httpd_t* httpd, qse_httpd_peer_t* peer,
	qse_mchar_t* buf, qse_size_t bufsize)
{
#if defined(__DOS__)
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#else
	qse_ssize_t ret = recv (peer->handle.i, buf, bufsize, 0);
	if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	return ret;
#endif
}

static qse_ssize_t peer_send (
	qse_httpd_t* httpd, qse_httpd_peer_t* peer,
	const qse_mchar_t* buf, qse_size_t bufsize)
{
#if defined(__DOS__)
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#else
	qse_ssize_t ret = send (peer->handle.i, buf, bufsize, 0);
	if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
	return ret;
#endif
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
	qse_ubi_t ubi;

	xtn = qse_mux_getxtn (mux);
	ubi.i = evt->hnd;
	xtn->cbfun (xtn->httpd, mux, ubi, evt->mask, evt->data);
}

static void* mux_open (qse_httpd_t* httpd, qse_httpd_muxcb_t cbfun)
{
	qse_mux_t* mux;
	mux_xtn_t* xtn;

	mux = qse_mux_open (httpd->mmgr, QSE_SIZEOF(*xtn), dispatch_muxcb, 256, QSE_NULL);
	if (!mux)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		return QSE_NULL;
	}

	xtn = qse_mux_getxtn (mux);
	xtn->httpd = httpd;
	xtn->cbfun = cbfun;
	return mux;
}

static void mux_close (qse_httpd_t* httpd, void* vmux)
{
	qse_mux_close ((qse_mux_t*)vmux);
}

static int mux_addhnd (
	qse_httpd_t* httpd, void* vmux, qse_ubi_t handle, int mask, void* data)
{
	qse_mux_evt_t evt;	

	evt.hnd = handle.i;	
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

static int mux_delhnd (qse_httpd_t* httpd, void* vmux, qse_ubi_t handle)
{
	qse_mux_evt_t evt;	
	evt.hnd = handle.i;	
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

static int mux_readable (qse_httpd_t* httpd, qse_ubi_t handle, const qse_ntime_t* tmout)
{
#if defined(__OS2__) && !defined(TCPV40HDRS)
	long tv;

	tv = tmout? QSE_SECNSEC_TO_MSEC (tmout->sec, tmout->nsec): -1;
	return os2_select (&handle.i, 1, 0, 0, tv);

#elif defined(__DOS__)

	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;

#else
	fd_set r;
	struct timeval tv, * tvp;

	FD_ZERO (&r);
	FD_SET (handle.i, &r);

	if (tmout)
	{
		tv.tv_sec = tmout->sec;
		tv.tv_usec = tmout->nsec;
		tvp = &tv;
	}
	else tvp = QSE_NULL;

	return select (handle.i + 1, &r, QSE_NULL, QSE_NULL, tvp);
#endif
}

static int mux_writable (qse_httpd_t* httpd, qse_ubi_t handle, const qse_ntime_t* tmout)
{
#if defined(__OS2__) && !defined(TCPV40HDRS)
	long tv;
	tv = tmout? QSE_SECNSEC_TO_MSEC (tmout->sec, tmout->nsec): -1;
	return os2_select (&handle.i, 0, 1, 0, tv);

#elif defined(__DOS__)

	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;

#else
	fd_set w;
	struct timeval tv, * tvp;

	FD_ZERO (&w);
	FD_SET (handle.i, &w);

	if (tmout)
	{
		tv.tv_sec = tmout->sec;
		tv.tv_usec = tmout->nsec;
		tvp = &tv;
	}
	else tvp = QSE_NULL;

	return select (handle.i + 1, QSE_NULL, &w, QSE_NULL, tvp);
#endif
}

/* ------------------------------------------------------------------- */

static int stat_file (
	qse_httpd_t* httpd, const qse_mchar_t* path,
	qse_httpd_stat_t* hst, int regonly)
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
	/* TODO: */
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#else
	qse_stat_t st;

/* TODO: lstat? or stat? */

	if (QSE_STAT (path, &st) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

	/* stating for a file. it should be a regular file.
	 * i don't allow other file types. */
	if (regonly && !S_ISREG(st.st_mode))
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
	 * note that 1 passes 1 to stat_file for it */
	return stat_file (httpd, path, hst, 1);	
}

static int file_ropen (
	qse_httpd_t* httpd, const qse_mchar_t* path, qse_ubi_t* handle)
{
	qse_fio_t* fio;

	fio = QSE_MMGR_ALLOC (httpd->mmgr, QSE_SIZEOF(*fio));
	if (fio == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return -1;
	}

	if (qse_fio_init (
		fio, httpd->mmgr, (const qse_char_t*)path,
		QSE_FIO_READ | QSE_FIO_MBSPATH, 0) <= -1)
	{
		qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(fio)));	
		QSE_MMGR_FREE (httpd->mmgr, fio);
		return -1;
	}

	handle->ptr = fio;

	if (httpd->opt.trait & QSE_HTTPD_LOGACT)
	{
		qse_httpd_act_t msg;
		qse_size_t pos;
		msg.code = QSE_HTTPD_CATCH_MDBGMSG;
		pos = qse_mbscpy (msg.u.mdbgmsg, QSE_MT("ropened file "));
		qse_mbsxcpy (&msg.u.mdbgmsg[pos], QSE_COUNTOF(msg.u.mdbgmsg) - pos, path);
		httpd->opt.rcb.logact (httpd, &msg);
	}
	return 0;

}

static int file_wopen (
	qse_httpd_t* httpd, const qse_mchar_t* path,
	qse_ubi_t* handle)
{
	qse_fio_t* fio;

	fio = QSE_MMGR_ALLOC (httpd->mmgr, QSE_SIZEOF(*fio));
	if (fio == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return -1;
	}

	if (qse_fio_init (
		fio, httpd->mmgr, (const qse_char_t*)path, 
		QSE_FIO_WRITE | QSE_FIO_CREATE | 
		QSE_FIO_TRUNCATE | QSE_FIO_MBSPATH, 0644) <= -1)
	{
		qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(fio)));	
		QSE_MMGR_FREE (httpd->mmgr, fio);
		return -1;
	}

	handle->ptr = fio;

	if (httpd->opt.trait & QSE_HTTPD_LOGACT)
	{
		qse_httpd_act_t msg;
		qse_size_t pos;
		msg.code = QSE_HTTPD_CATCH_MDBGMSG;
		pos = qse_mbscpy (msg.u.mdbgmsg, QSE_MT("wopened file "));
		qse_mbsxcpy (&msg.u.mdbgmsg[pos], QSE_COUNTOF(msg.u.mdbgmsg) - pos, path);
		httpd->opt.rcb.logact (httpd, &msg);
	}

	return 0;
}

static void file_close (qse_httpd_t* httpd, qse_ubi_t handle)
{
	qse_fio_fini (handle.ptr);
	QSE_MMGR_FREE (httpd->mmgr, handle.ptr);
}

static qse_ssize_t file_read (
	qse_httpd_t* httpd, qse_ubi_t handle,
	qse_mchar_t* buf, qse_size_t len)
{
	qse_ssize_t n;
	n = qse_fio_read (handle.ptr, buf, len);
	if (n <= -1) qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(handle.ptr)));	
	return n;
}

static qse_ssize_t file_write (
	qse_httpd_t* httpd, qse_ubi_t handle,
	const qse_mchar_t* buf, qse_size_t len)
{
	qse_ssize_t n;
	n = qse_fio_write (handle.ptr, buf, len);
	if (n <= -1) qse_httpd_seterrnum (httpd, fioerr_to_errnum(qse_fio_geterrnum(handle.ptr)));	
	return n;
}

/* ------------------------------------------------------------------- */

typedef struct dir_t dir_t;
struct dir_t
{
	qse_mchar_t* path;
	qse_dir_t* dp;
};

static int dir_open (qse_httpd_t* httpd, const qse_mchar_t* path, qse_ubi_t* handle)
{
	dir_t* d;
	qse_dir_errnum_t direrrnum;

	d = QSE_MMGR_ALLOC (httpd->mmgr, QSE_SIZEOF(*d));
	if (d == QSE_NULL) 
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return -1;
	}

	d->path = qse_mbsdup (path, httpd->mmgr);
	if (d->path == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		QSE_MMGR_FREE (httpd->mmgr, d);
		return -1;
	}

	d->dp = qse_dir_open (
		httpd->mmgr, 0, 
		(const qse_char_t*)d->path, 
		QSE_DIR_MBSPATH | QSE_DIR_SORT,
		&direrrnum
	);
	if (d->dp == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, direrr_to_errnum(direrrnum));
		QSE_MMGR_FREE (httpd->mmgr, d->path);
		QSE_MMGR_FREE (httpd->mmgr, d);
		return -1;
	}
		
	if (httpd->opt.trait & QSE_HTTPD_LOGACT)
	{
		qse_httpd_act_t msg;
		qse_size_t pos;
		msg.code = QSE_HTTPD_CATCH_MDBGMSG;
		pos = qse_mbscpy (msg.u.mdbgmsg, QSE_MT("opened dir "));
		qse_mbsxcpy (&msg.u.mdbgmsg[pos], QSE_COUNTOF(msg.u.mdbgmsg) - pos, path);
		httpd->opt.rcb.logact (httpd, &msg);
	}

	handle->ptr = d;
	return 0;
}

static void dir_close (qse_httpd_t* httpd, qse_ubi_t handle)
{
	dir_t* d;

	d = (dir_t*)handle.ptr;

	qse_dir_close (d->dp);

	QSE_MMGR_FREE (httpd->mmgr, d->path);
	QSE_MMGR_FREE (httpd->mmgr, d);
}

static int dir_read (qse_httpd_t* httpd, qse_ubi_t handle, qse_httpd_dirent_t* dirent)
{
	dir_t* d;
	qse_dir_ent_t de;
	qse_mchar_t* fpath;
	int n;

	d = (dir_t*)handle.ptr;

	n = qse_dir_read (d->dp, &de);
	if (n <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		return -1;
	}
	else if (n == 0) return 0;

	/* i assume that d->path ends with a slash */
	fpath = qse_mbsdup2 (d->path, (const qse_mchar_t*)de.name, httpd->mmgr);
	if (fpath == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return -1;
	}

	n = stat_file (httpd, fpath, &dirent->stat, 0);	
	QSE_MMGR_FREE (httpd->mmgr, fpath);
	if (n <= -1) QSE_MEMSET (dirent, 0, QSE_SIZEOF(*dirent));

	dirent->name = (const qse_mchar_t*)de.name;
	return 1;
}

/* ------------------------------------------------------------------- */
static void client_close (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
#if defined(_WIN32)
	/* TODO: */
#elif defined(__OS2__)
	/* TODO: */
#elif defined(__DOS__)
	/* TODO: */
#else
	QSE_CLOSE (client->handle.i);
#endif
}

static void client_shutdown (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
#if defined(__DOS__)
	/* TODO */
#elif defined(SHUT_RDWR)
	shutdown (client->handle.i, SHUT_RDWR);
#else
	shutdown (client->handle.i, 2);
#endif
}

static qse_ssize_t client_recv (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_mchar_t* buf, qse_size_t bufsize)
{
	if (client->status & CLIENT_SECURE)
	{
	#if defined(HAVE_SSL)
		int ret = SSL_read (client->handle2.ptr, buf, bufsize);
		if (ret <= -1)
		{
			if (SSL_get_error(client->handle2.ptr,ret) == SSL_ERROR_WANT_READ)
				qse_httpd_seterrnum (httpd, QSE_HTTPD_EAGAIN);
			else
				qse_httpd_seterrnum (httpd, QSE_HTTPD_ESYSERR);
		}

		if (SSL_pending (client->handle2.ptr) > 0) 
			client->status |= CLIENT_PENDING;
		else
			client->status &= ~CLIENT_PENDING;

		return ret;
	#else
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	#endif
	}
	else
	{
	#if defined(__DOS__)
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	#else
		qse_ssize_t ret;
		ret = recv (client->handle.i, buf, bufsize, 0);
		if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		return ret;
	#endif
	}
}

static qse_ssize_t client_send (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	const qse_mchar_t* buf, qse_size_t bufsize)
{
	if (client->status & CLIENT_SECURE)
	{
	#if defined(HAVE_SSL)
		int ret = SSL_write (client->handle2.ptr, buf, bufsize);
		if (ret <= -1)
		{
			if (SSL_get_error(client->handle2.ptr,ret) == SSL_ERROR_WANT_WRITE)
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
	#if defined(__DOS__)
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	#else
		qse_ssize_t ret = send (client->handle.i, buf, bufsize, 0);
		if (ret <= -1) qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		return ret;
	#endif
	}
}

static qse_ssize_t client_sendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_ubi_t handle, qse_foff_t* offset, qse_size_t count)
{
	if (client->status & CLIENT_SECURE)
	{
		return __send_file_ssl (httpd, client->handle2.ptr, handle, offset, count);
	}
	else
	{
		return __send_file (httpd, client->handle.i, handle, offset, count);
	}
}

static int client_accepted (qse_httpd_t* httpd, qse_httpd_client_t* client)
{

	if (client->status & CLIENT_SECURE)
	{
	#if defined(HAVE_SSL)
		int ret;
		SSL* ssl;
		httpd_xtn_t* xtn;

		xtn = (httpd_xtn_t*) qse_httpd_getxtn (httpd);
		if (!xtn->ssl_ctx)
		{
			/* delayed initialization of ssl */
/* TODO: certificate from options */
			if (init_xtn_ssl (httpd, "http01.pem", "http01.key") <= -1) 
			{
				return -1;
			}
		}
	
		QSE_ASSERT (xtn->ssl_ctx != QSE_NULL);

		if (client->handle2.ptr)
		{
			ssl = client->handle2.ptr;
		}
		else
		{
			ssl = SSL_new (xtn->ssl_ctx);
			if (ssl == QSE_NULL) return -1;

			client->handle2.ptr = ssl;
			if (SSL_set_fd (ssl, client->handle.i) == 0)
			{
				/* don't free ssl here since client_closed()
				 * will free it */
				return -1;
			}

			SSL_set_read_ahead (ssl, 0);
		}

		ret = SSL_accept (ssl);
		if (ret <= 0)
		{
			int err;
			if ((err = SSL_get_error(ssl,ret)) == SSL_ERROR_WANT_READ)
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

			/* SSL_free (ssl); */
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
	if (client->status & CLIENT_SECURE)
	{
	#if defined(HAVE_SSL)
		if (client->handle2.ptr)
		{
			SSL_shutdown ((SSL*)client->handle2.ptr); /* is this needed? */
			SSL_free ((SSL*)client->handle2.ptr);
		}
	#endif
	}
}

/* ------------------------------------------------------------------- */
#if 0
static qse_htb_walk_t walk (qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
{
	qse_htre_hdrval_t* val;

	val = QSE_HTB_VPTR(pair);
	while (val)
	{
qse_printf (QSE_T("HEADER OK %d[%hs] %d[%hs]\n"),  (int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)val->len, val->ptr);
		val = val->next;
	}
	return QSE_HTB_WALK_FORWARD;
}
#endif

static int process_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req, int peek)
{
	int method;
	qse_httpd_task_t* task;
	int content_received;
	server_xtn_t* server_xtn;

	server_xtn = (server_xtn_t*)qse_httpd_getserverxtn (httpd, client->server);

	method = qse_htre_getqmethodtype(req);
	content_received = (qse_htre_getcontentlen(req) > 0);

	/* percent-decode the query path to the original buffer
	 * since i'm not gonna need it in the original form
	 * any more. once it's decoded in the peek mode,
	 * the decoded query path is made available in the
	 * non-peek mode as well */
	if (peek) qse_perdechttpstr (qse_htre_getqpath(req), qse_htre_getqpath(req));

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

	if (peek)
	{
		if (method != QSE_HTTP_POST && method != QSE_HTTP_PUT)
		{
			/* i'll discard request contents if the method is none of
			 * post and put */
			qse_httpd_discardcontent (httpd, req);
		}

		if ((req->attr.flags & QSE_HTRE_ATTR_EXPECT100) &&
		    (req->version.major > 1 ||
		     (req->version.major == 1 && req->version.minor >= 1)) &&
		    !content_received)
		{
/* TODO: check method.... */
			/* "expect" in the header, version 1.1 or higher,
			 * and no content received yet */

			/* TODO: determine if to return 100-continue or other errors */
			if (qse_httpd_entaskcontinue (
				httpd, client, QSE_NULL, req) == QSE_NULL) return -1;
		}
	}

	if (method == QSE_HTTP_GET || method == QSE_HTTP_POST)
	{
		if (peek)
		{
			qse_httpd_rsrc_t rsrc;

			if (method == QSE_HTTP_POST &&
			    !(req->attr.flags & QSE_HTRE_ATTR_LENGTH) &&
			    !(req->attr.flags & QSE_HTRE_ATTR_CHUNKED))
			{
				req->attr.flags &= ~QSE_HTRE_ATTR_KEEPALIVE;
				qse_httpd_discardcontent (httpd, req);
				task = qse_httpd_entaskerr (httpd, client, QSE_NULL, 411, req);
				if (task) 
				{
					/* 411 Length Required - can't keep alive. Force disconnect */
					task = qse_httpd_entaskdisconnect (httpd, client, QSE_NULL);
				}
			}
			else if (server_xtn->makersrc (httpd, client, req, &rsrc) <= -1)
			{
				qse_httpd_discardcontent (httpd, req);
				task = qse_httpd_entaskerr (httpd, client, QSE_NULL, 500, req);
			}
			else
			{
				task = qse_httpd_entaskrsrc (httpd, client, QSE_NULL, &rsrc, req);
				server_xtn->freersrc (httpd, client, req, &rsrc);
			}
			if (task == QSE_NULL) goto oops;
		}
	}
	else
	{
		if (peek)
		{
			qse_httpd_discardcontent (httpd, req);
		}
		else
		{
			task = qse_httpd_entaskerr (httpd, client, QSE_NULL, 405, req);
			if (task == QSE_NULL) goto oops;
		}
	}

	if (!(req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE))
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

static int format_error (
	qse_httpd_t* httpd, qse_httpd_client_t* client, int code, qse_mchar_t* buf, int bufsz)
{
	int n;
	server_xtn_t* server_xtn;
	const qse_mchar_t* css, * msg, * name;

	server_xtn = qse_httpd_getserverxtn (httpd, client->server);

	if (server_xtn->query (httpd, client->server, QSE_NULL, QSE_NULL, QSE_HTTPD_SERVERSTD_ERRCSS, &css) <= -1) css = QSE_NULL;
	if (css == QSE_NULL) css = QSE_MT("");

	if (server_xtn->query (httpd, client->server, QSE_NULL, QSE_NULL, QSE_HTTPD_SERVERSTD_NAME, &name) <= -1) name = QSE_NULL;
	if (name == QSE_NULL) name = qse_httpd_getname(httpd);

	msg = qse_httpstatustombs(code);

/* TODO: use my own version of snprintf replacement */
	n = snprintf (buf, bufsz,
		QSE_MT("<html><head>%s<title>%s</title></head><body><div class='header'>HTTP ERROR</div><div class='body'>%d %s</div><div class='footer'>%s</div></body></html>"), 
		css, msg, code, msg, name);
	if (n < 0 || n >= bufsz) 
	{
		httpd->errnum = QSE_HTTPD_ENOBUF;
		return -1;
	}

	return n;
}

static int format_dir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	const qse_mchar_t* qpath, const qse_httpd_dirent_t* dirent,
	qse_mchar_t* buf, int bufsz)
{
/* TODO: page encoding?? utf-8??? or derive name from cmgr or current locale??? */
/* TODO: html escaping of ctx->qpath.ptr */
	int n;
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverxtn (httpd, client->server);

	if (dirent == QSE_NULL)
	{
		if (qpath)
		{
			/* header */
			const qse_mchar_t* css;
			int is_root = (qse_mbscmp (qpath, QSE_MT("/")) == 0);

			if (server_xtn->query (httpd, client->server, QSE_NULL, QSE_NULL, QSE_HTTPD_SERVERSTD_DIRCSS, &css) <= -1) css = QSE_NULL;
			if (css == QSE_NULL) css = QSE_MT("");

/* TODO: html escaping of qpath */
			n = snprintf (buf, bufsz,
				QSE_MT("<html><head>%s</head><body><div class='header'>%s</div><div class='body'><table>%s"), css, qpath,
				(is_root? QSE_MT(""): QSE_MT("<tr><td class='name'><a href='../'>..</a></td><td class='time'></td><td class='size'></td></tr>"))
			);
		}
		else
		{
			/* footer */
			const qse_mchar_t* name;

			if (server_xtn->query (httpd, client->server, QSE_NULL, QSE_NULL, QSE_HTTPD_SERVERSTD_NAME, &name) <= -1) name = QSE_NULL;
			if (name == QSE_NULL) name = qse_httpd_getname(httpd);

			n = snprintf (buf, bufsz, QSE_MT("</table></div><div class='footer'>%s</div></body></html>"), name);
		}
	}
	else
	{
		/* main entry */
		qse_mchar_t* encname;
		qse_btime_t bt;
		qse_mchar_t tmbuf[32];
		qse_mchar_t fszbuf[64];

		/* TODO: better buffer management in case there are 
		 *       a lot of file names to escape. */
		encname = qse_perenchttpstrdup (dirent->name, httpd->mmgr);
		if (encname == QSE_NULL)
		{
			httpd->errnum = QSE_HTTPD_ENOMEM;
			return -1;
		}

		qse_localtime (&dirent->stat.mtime, &bt);
		snprintf (tmbuf, QSE_COUNTOF(tmbuf),
			QSE_MT("%04d-%02d-%02d %02d:%02d:%02d"),
         		bt.year + QSE_BTIME_YEAR_BASE, bt.mon + 1, bt.mday,
			bt.hour, bt.min, bt.sec);

		if (dirent->stat.isdir)
		{
			fszbuf[0] = QSE_MT('\0');
		}
		else
		{
			qse_fmtuintmaxtombs (
				fszbuf, QSE_COUNTOF(fszbuf),
				dirent->stat.size, 10, -1, QSE_MT('\0'), QSE_NULL
			);
		}

		n = snprintf (
			buf, bufsz,
			QSE_MT("<tr><td class='name'><a href='%s%s'>%s%s</a></td><td class='time'>%s</td><td class='size'>%s</td></tr>"),
			encname,
			(dirent->stat.isdir? QSE_MT("/"): QSE_MT("")),
			dirent->name, /* TODO: html escaping for entry name */
			(dirent->stat.isdir? QSE_MT("/"): QSE_MT("")),
			tmbuf, fszbuf
		);

		if (encname != dirent->name) QSE_MMGR_FREE (httpd->mmgr, encname);
	}

	if (n <= -1 || n >= bufsz)
	{
		httpd->errnum = QSE_HTTPD_ENOBUF;
		return -1;
	}

	return n;
}

static void impede_httpd (qse_httpd_t* httpd)
{
	/* do nothing */
}

static void logact_httpd (qse_httpd_t* httpd, const qse_httpd_act_t* act)
{
	/* do nothing */
}


static qse_httpd_scb_t httpd_system_callbacks =
{
	/* server */
	{ server_open,
	  server_close,
	  server_accept },

	{ peer_open,
	  peer_close,
	  peer_connected,
	  peer_recv,
	  peer_send },

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
	{ file_stat,
	  file_ropen,
	  file_wopen,
	  file_close,
	  file_read,
	  file_write
	},

	/* directory operation */
	{ dir_open,
	  dir_close,
	  dir_read
	},
	

	/* client connection */
	{ client_close,
	  client_shutdown,
	  client_recv,
	  client_send,
	  client_sendfile,
	  client_accepted,
	  client_closed }
};

static qse_httpd_rcb_t httpd_request_callbacks =
{
	QSE_STRUCT_FIELD(peekreq, peek_request),
	QSE_STRUCT_FIELD(pokereq, poke_request),
	QSE_STRUCT_FIELD(fmterr,  format_error),
	QSE_STRUCT_FIELD(fmtdir,  format_dir),
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
				QSE_MMGR_FREE (httpd->mmgr, (qse_mchar_t*)target->u.cgi.suffix);
			if (target->u.cgi.script != qpath) 
				QSE_MMGR_FREE (httpd->mmgr, (qse_mchar_t*)target->u.cgi.script);
			if (target->u.cgi.path != qpath)
				QSE_MMGR_FREE (httpd->mmgr, (qse_mchar_t*)target->u.cgi.path);
			if (target->u.cgi.shebang)
				QSE_MMGR_FREE (httpd->mmgr, (qse_mchar_t*)target->u.cgi.shebang);

			break;

		case QSE_HTTPD_RSRC_DIR:
			if (target->u.dir.path != qpath)
				QSE_MMGR_FREE (httpd->mmgr, (qse_mchar_t*)target->u.dir.path);
			break;

		case QSE_HTTPD_RSRC_FILE:
			if (target->u.cgi.path != qpath)
				QSE_MMGR_FREE (httpd->mmgr, (qse_mchar_t*)target->u.cgi.path);
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
	ta[idx++] = QSE_MT("/");	
	ta[idx++] = path;
	ta[idx++] = QSE_NULL;
	xpath = qse_mbsadup (ta, QSE_NULL, httpd->mmgr);
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
	len += qse_mbscpy (&xpath[len], QSE_MT("/"));
	if (plen == (qse_size_t)-1)
		len += qse_mbscpy (&xpath[len], path);
	else
		len += qse_mbsncpy (&xpath[len], path, plen);
	qse_canonmbspath (xpath, xpath, 0);
}

struct rsrc_tmp_t
{
	const qse_mchar_t* qpath;
	const qse_mchar_t* idxfile;
	qse_mchar_t* xpath;

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
	server_xtn_t* server_xtn;
	qse_mchar_t* shebang = QSE_NULL;
	qse_mchar_t* suffix = QSE_NULL;
	qse_mchar_t* script = QSE_NULL;
	qse_httpd_serverstd_cgi_t cgi;

	server_xtn = qse_httpd_getserverxtn (httpd, client->server);

	if (tmp->final_match)
	{
		/* it is a final match. tmp->xpath is tmp->root + tmp->qpath  */
		if (server_xtn->query (httpd, client->server, req, tmp->xpath, QSE_HTTPD_SERVERSTD_CGI, &cgi) >= 0 && cgi.cgi)
		{
			if (tmp->idxfile)
			{
				script = merge_paths (httpd, tmp->qpath, tmp->idxfile);
				if (script == QSE_NULL) goto oops;
			}
			else script = (qse_mchar_t*)tmp->qpath;

			if (cgi.shebang)
			{
				shebang = qse_mbsdup (cgi.shebang, httpd->mmgr);
				if (shebang == QSE_NULL) goto oops;
			}

			goto bingo;
		}
	}
	else
	{
		/* inspect each segment from the head. */
		const qse_mchar_t* ptr;
		const qse_mchar_t* slash;
		int xpath_changed = 0;

		QSE_ASSERT (tmp->qpath[0] == QSE_T('/'));

		ptr = tmp->qpath + 1;
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
					 * the subsegments of the original query path and docroot. */
					merge_paths_to_buf (httpd, tmp->root.u.path, tmp->qpath, slash - tmp->qpath, tmp->xpath);
					xpath_changed = 1;
	
					stx = stat_file (httpd, tmp->xpath, &st, 0);
					if (stx <= -1) 
					{
						/* stop at the current segment if stating fails. 
						 * if the current semgment can't be stat-ed, it's not likely that
						 * the next segment can be successfully stat-ed */
						break; 
					}

					if (!st.isdir)
					{
						if (server_xtn->query (httpd, client->server, req, tmp->xpath, QSE_HTTPD_SERVERSTD_CGI, &cgi) >= 0 && cgi.cgi)
						{
							script = qse_mbsxdup (tmp->qpath, slash - tmp->qpath , httpd->mmgr);
							suffix = qse_mbsdup (slash, httpd->mmgr);
							if (!script || !suffix) goto oops;

							if (cgi.shebang)
							{
								shebang = qse_mbsdup (cgi.shebang, httpd->mmgr);
								if (shebang == QSE_NULL) goto oops;
							}

							goto bingo;
						}
					}
				}

				ptr = slash + 1;
			}
			else 
			{
				/* no more slash is found. the last segement doesn't have to be checked 
				 * here since it's attempted by the caller. */
				break;
			}
		}

		/* restore the xpath because it has changed... */
		if (xpath_changed) merge_paths_to_buf (httpd, tmp->root.u.path, tmp->qpath, (qse_size_t)-1, tmp->xpath);
	}

	return 0; /* not a cgi */

bingo:
	target->type = QSE_HTTPD_RSRC_CGI;
	target->u.cgi.nph = cgi.nph;
	target->u.cgi.path = tmp->xpath;
	target->u.cgi.script = script;
	target->u.cgi.suffix = suffix;
	target->u.cgi.root = tmp->root.u.path;
	target->u.cgi.shebang = shebang;
	return 1;

oops:
	httpd->errnum = QSE_HTTPD_ENOMEM;
	if (shebang) QSE_MMGR_FREE (httpd->mmgr, shebang);
	if (suffix) QSE_MMGR_FREE (httpd->mmgr, suffix);
	if (script && script != tmp->qpath) QSE_MMGR_FREE (httpd->mmgr, script);
	return -1;
}

static int make_resource (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_htre_t* req, qse_httpd_rsrc_t* target)
{
	server_xtn_t* server_xtn;
	struct rsrc_tmp_t tmp;

	qse_httpd_stat_t st;
	int n, stx;

	QSE_MEMSET (&tmp, 0, QSE_SIZEOF(tmp));
	tmp.qpath = qse_htre_getqpath(req);

	QSE_MEMSET (target, 0, QSE_SIZEOF(*target));

	server_xtn = qse_httpd_getserverxtn (httpd, client->server);

	if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_ROOT, &tmp.root) <= -1) return -1;
	if (tmp.root.type == QSE_HTTPD_SERVERSTD_ROOT_NWAD)
	{
		/* proxy the request */
		target->type = QSE_HTTPD_RSRC_PROXY;
		/*target->u.proxy.dst = client->orgdst_addr;*/
		target->u.proxy.dst = tmp.root.u.nwad;
		target->u.proxy.src = client->remote_addr;
		return 0;
	}

	QSE_ASSERT (tmp.root.type == QSE_HTTPD_SERVERSTD_ROOT_PATH);

	if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_REALM, &tmp.realm) <= -1 ||
	    server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_INDEX, &tmp.index) <= -1)
	{
		return -1;
	}


	/* default to the root directory. */
	if (!tmp.root.u.path) tmp.root.u.path = QSE_MT("/"); 

	if (tmp.realm.authreq && tmp.realm.name)
	{
		const qse_htre_hdrval_t* authv;

		authv = qse_htre_getheaderval (req, QSE_MT("Authorization"));
		if (authv)
		{
			while (authv->next) authv = authv->next;

			if (qse_mbszcasecmp(authv->ptr, QSE_MT("Basic "), 6) == 0) 
			{
				qse_size_t authl, authl2;

				/* basic authorization is a base64-encoded string of username:password. */

				authl = qse_mbslen(&authv->ptr[6]);	
				if (authl > server_xtn->auth.len)
				{
					qse_mchar_t* tmp;
					tmp = qse_httpd_reallocmem (httpd, server_xtn->auth.ptr, authl * QSE_SIZEOF(qse_mchar_t));
					if (!tmp) return -1;

					server_xtn->auth.ptr = tmp;
					/* the maximum capacity that can hold the largest authorization value */
					server_xtn->auth.len = authl;	 
				}

				/* decoding a base64-encoded string result in a shorter value than the input.
				 * so passing the length of the input(authl) as the output buffer size is ok */
				authl2 = qse_debase64 (&authv->ptr[6], authl, server_xtn->auth.ptr, authl, QSE_NULL);

				tmp.auth.key.ptr = server_xtn->auth.ptr;
				tmp.auth.key.len = authl2;
	    			if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_AUTH, &tmp.auth) >= 0 && tmp.auth.authok) goto auth_ok;
			}
		}

		target->type = QSE_HTTPD_RSRC_AUTH;
		target->u.auth.realm = tmp.realm.name; 
		return 0;
	}

auth_ok:
	tmp.xpath = merge_paths (httpd, tmp.root.u.path, tmp.qpath);
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
						QSE_MMGR_FREE (httpd->mmgr, tmp.xpath);
						return -1;
					}

					if (httpd->opt.scb.file.stat (httpd, tpath, &st) >= 0 && !st.isdir)
					{
						/* the index file is found */
						QSE_MMGR_FREE (httpd->mmgr, tmp.xpath);
						tmp.xpath = tpath;
						tmp.idxfile = ptr;
						goto attempt_file;
					}	

					QSE_MMGR_FREE (httpd->mmgr, tpath);
				}
			}

			/* it is a directory - should i allow it? */
			if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_DIRACC, &target->u.err.code) <= -1) target->u.err.code = 500;
			if (target->u.err.code != 200)
			{
				target->type = QSE_HTTPD_RSRC_ERR;
				/* free xpath since it won't be used */
				QSE_MMGR_FREE (httpd->mmgr, tmp.xpath);
			}
			else
			{
				target->type = QSE_HTTPD_RSRC_DIR;
				target->u.dir.path = tmp.xpath;
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
			QSE_MMGR_FREE (httpd->mmgr, tmp.xpath);
			return -1;
		}
		if (n >= 1) return 0;

		/* check file's access permission */
		if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_FILEACC, &target->u.err.code) <= -1) target->u.err.code = 500;
		if (target->u.err.code != 200)
		{
			target->type = QSE_HTTPD_RSRC_ERR;
			/* free xpath since it won't be used */
			QSE_MMGR_FREE (httpd->mmgr, tmp.xpath);
		}
		else
		{
			/* fall back to a normal file. */
			target->type = QSE_HTTPD_RSRC_FILE;
			target->u.file.path = tmp.xpath;

			if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_MIME, &target->u.file.mime) <= -1)
			{
				/* don't care about failure */
				target->u.file.mime = QSE_NULL;
			}
		}
	}

	return 0;
}

/* ------------------------------------------------------------------- */

static void detach_server (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	server_xtn_t* server_xtn;

	server_xtn = (server_xtn_t*) qse_httpd_getserverxtn (httpd, server);
	if (server_xtn->detach) server_xtn->detach (httpd, server);
	if (server_xtn->auth.ptr) QSE_MMGR_FREE (httpd->mmgr, server_xtn->auth.ptr);
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
	{ QSE_MT(".cgi"), { 1, 0, QSE_NULL } },
	{ QSE_MT(".nph"), { 1, 1, QSE_NULL } },
};

static int query_server (
	qse_httpd_t* httpd, qse_httpd_server_t* server, 
	qse_htre_t* req, const qse_mchar_t* xpath,
	qse_httpd_serverstd_query_code_t code, void* result)
{
	qse_size_t i;

	switch (code)
	{
		case QSE_HTTPD_SERVERSTD_NAME:
			*(const qse_mchar_t**)result = QSE_NULL;
			break;

		case QSE_HTTPD_SERVERSTD_ROOT:
			((qse_httpd_serverstd_root_t*)result)->type = QSE_HTTPD_SERVERSTD_ROOT_PATH;
			((qse_httpd_serverstd_root_t*)result)->u.path = QSE_NULL;
			break;
		
		case QSE_HTTPD_SERVERSTD_REALM:
			((qse_httpd_serverstd_realm_t*)result)->name = QSE_NULL;
			((qse_httpd_serverstd_realm_t*)result)->authreq = 0;
			break;

		case QSE_HTTPD_SERVERSTD_AUTH:
			((qse_httpd_serverstd_auth_t*)result)->authok = 1;
			break;
			
		case QSE_HTTPD_SERVERSTD_ERRCSS:
		case QSE_HTTPD_SERVERSTD_DIRCSS:
			*(const qse_mchar_t**)result = QSE_NULL;
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
			for (i = 0; i < QSE_COUNTOF(cgitab); i++)
			{
				if (qse_mbsend (xpath, cgitab[i].suffix))
				{
					QSE_MEMCPY (cgi, &cgitab[i].cgi, QSE_SIZEOF(*cgi));
					return 0;
				}
			}

			QSE_MEMSET (cgi,0, QSE_SIZEOF(*cgi));
			return 0;
		}

		case QSE_HTTPD_SERVERSTD_MIME:
			/* TODO: binary search if the table is large */
			for (i = 0; i < QSE_COUNTOF(mimetab); i++)
			{
				if (qse_mbsend (xpath, mimetab[i].suffix))
				{
					*(const qse_mchar_t**)result = mimetab[i].type;
					return 0;
				}
			}			

			*(const qse_mchar_t**)result = QSE_NULL;
			return 0;

		case QSE_HTTPD_SERVERSTD_DIRACC:
		case QSE_HTTPD_SERVERSTD_FILEACC:
			*(int*)result = 200;
			return 0;
			
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
	xdope.detach = detach_server;

	xserver = qse_httpd_attachserver (httpd, &xdope, QSE_SIZEOF(*server_xtn) + xtnsize);
	if (xserver == QSE_NULL) return QSE_NULL;

	server_xtn = qse_httpd_getserverxtn (httpd, xserver);

	server_xtn->detach = dope->detach;

	server_xtn->query = query_server;
	server_xtn->makersrc = make_resource;
	server_xtn->freersrc = free_resource;

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
		server.flags |= QSE_HTTPD_SERVER_SECURE;
		default_port = DEFAULT_SECURE_PORT;
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
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverxtn (httpd, server);

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
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverxtn (httpd, server);

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


void* qse_httpd_getserverstdxtn (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	server_xtn_t* xtn = qse_httpd_getserverxtn (httpd, server);
	return (void*)(xtn + 1);
}

/* ------------------------------------------------------------------- */

int qse_httpd_loopstd (qse_httpd_t* httpd)
{
	return qse_httpd_loop (httpd);	
}
