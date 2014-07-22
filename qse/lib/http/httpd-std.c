/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#define STAT_REG   1
#define STAT_DIR   2

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
#		if defined(HAVE_LINUX_NETFILTER_IPV4_H)
#			include <linux/netfilter_ipv4.h> /* SO_ORIGINAL_DST */
#		endif
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
#if defined(_MSC_VER) || defined(__BORLANDC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
#	define snprintf _snprintf 
#endif


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
			int err = SSL_get_error(out, ret);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
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
static int init_xtn_ssl (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	SSL_CTX* ctx;
	httpd_xtn_t* xtn;
	server_xtn_t* server_xtn;
	qse_httpd_serverstd_ssl_t ssl;

	xtn = (httpd_xtn_t*)qse_httpd_getxtn (httpd);
	server_xtn = (server_xtn_t*)qse_httpd_getserverxtn (httpd, server);

	if (server_xtn->query (httpd, server, QSE_NULL, QSE_NULL, QSE_HTTPD_SERVERSTD_SSL, &ssl) <= -1)
	{
		return -1;
	}

	if (ssl.certfile == QSE_NULL || ssl.keyfile == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);	
		return -1;
	}

	ctx = SSL_CTX_new (SSLv23_server_method());
	if (ctx == QSE_NULL) return -1;

	/*SSL_CTX_set_info_callback(ctx,ssl_info_callback);*/

	if (SSL_CTX_use_certificate_file (ctx, ssl.certfile, SSL_FILETYPE_PEM) == 0 ||
	    SSL_CTX_use_PrivateKey_file (ctx, ssl.keyfile, SSL_FILETYPE_PEM) == 0 ||
	    SSL_CTX_check_private_key (ctx) == 0 /*||
	    SSL_CTX_use_certificate_chain_file (ctx, chainfile) == 0*/)
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
	
	#if defined(SO_REUSEPORT)
	flag = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEPORT, (void*)&flag, QSE_SIZEOF(flag));
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
	if (ioctl(fd, FIONBIO, (char*)&cmd, QSE_SIZEOF(cmd)) == -1) goto oops;

	if (connect (fd, (struct sockaddr*)&connaddr, connaddrsize) == -1)
	{
		if (sock_errno() != SOCEINPROGRESS) goto oops;
		connected = 0;
	}

	cmd = 0;
	if (ioctl(fd, FIONBIO, (char*)&cmd, QSE_SIZEOF(cmd)) == -1) goto oops;

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
	int mask = 0;

	xtn = qse_mux_getxtn (mux);
	ubi.i = evt->hnd;

	if (evt->mask & QSE_MUX_IN) mask |= QSE_HTTPD_MUX_READ;
	if (evt->mask & QSE_MUX_OUT) mask |= QSE_HTTPD_MUX_WRITE;

	xtn->cbfun (xtn->httpd, mux, ubi, mask, evt->data);
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
	/* TODO: */
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;

#else

	if (QSE_UNLINK (path) <= -1)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}
	return 0;

#endif
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
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
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
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
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
#if !defined(SHUT_RDWR)
#	define SHUT_RDWR 2
#endif

static void client_close (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{

#if defined(_WIN32)
	shutdown (client->handle.i, SHUT_RDWR);
	closesocket (client->handle.i);
#elif defined(__OS2__)
	shutdown (client->handle.i, SHUT_RDWR);
	soclose (client->handle.i);
#elif defined(__DOS__)
	/* TODO: */
#else
	shutdown (client->handle.i, SHUT_RDWR);
	QSE_CLOSE (client->handle.i);
#endif
}

static void client_shutdown (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
#if defined(_WIN32)
	shutdown (client->handle.i, SHUT_RDWR);
#elif defined(__OS2__)
	shutdown (client->handle.i, SHUT_RDWR);
#elif defined(__DOS__)
	/* TODO: */
#else
	shutdown (client->handle.i, SHUT_RDWR);
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
			int err = SSL_get_error(client->handle2.ptr,ret);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
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
			int err = SSL_get_error(client->handle2.ptr,ret);
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
			if (init_xtn_ssl (httpd, client->server) <= -1) 
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
			err = SSL_get_error(ssl,ret);
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
typedef struct dns_ctx_t dns_ctx_t;
typedef struct dns_req_t dns_req_t;
typedef struct dns_hdr_t dns_hdr_t;
typedef struct dns_qdtrail_t dns_qdtrail_t;
typedef struct dns_antrail_t dns_antrail_t;

struct dns_ctx_t
{
	qse_skad_t skad;
	int skadlen;

	qse_uint16_t seq;
	dns_req_t* reqs[1024]; /* TOOD: choose the right size */
};

struct dns_req_t
{
#define DNS_REQ_A_ERROR (1 << 0)
#define DNS_REQ_AAAA_ERROR (1 << 1)
	int flags;
	qse_uint16_t seqa, seqaaaa;

	qse_mchar_t* name;
	qse_uint8_t* dn;
	qse_size_t dnlen;

	qse_httpd_resol_t resol;
	void* ctx;

	qse_uint8_t qa[384];
	qse_uint8_t qaaaa[384];
	int qalen;
	int qaaaalen;

	dns_req_t* next;
};

enum
{
	DNS_OPCODE_QUERY = 0,
	DNS_OPCODE_IQUERY = 1,
	DNS_OPCODE_STATUS = 2,
	DNS_OPCODE_NOTIFY = 4,
	DNS_OPCODE_UPDATE = 5,

	DNS_RCODE_NOERROR = 0,
	DNS_RCODE_FORMERR = 1,
	DNS_RCODE_SERVFAIL = 2,
	DNS_RCODE_NXDOMAIN = 3,
	DNS_RCODE_NOTIMPL = 4,
	DNS_RCODE_REFUSED = 5,

	DNS_QTYPE_A = 1,
	DNS_QTYPE_NS = 2,
	DNS_QTYPE_CNAME = 5,
	DNS_QTYPE_SOA = 6,
	DNS_QTYPE_PTR = 12,
	DNS_QTYPE_MX = 15,
	DNS_QTYPE_TXT = 16,
	DNS_QTYPE_AAAA = 28,
	DNS_QTYPE_OPT = 41,
	DNS_QTYPE_ANY = 255,

	DNS_QCLASS_IN = 1, /* internet */
	DNS_QCLASS_CH = 3, /* chaos */
	DNS_QCLASS_HS = 4, /* hesiod */
	DNS_QCLASS_NONE = 254,
	DNS_QCLASS_ANY = 255
};

#include <qse/pack1.h>
struct dns_hdr_t
{
	qse_uint16_t id;

#if defined(QSE_ENDIAN_BIG)
	qse_uint16_t qr: 1; /* question or response  */
	qse_uint16_t opcode: 4; 
	qse_uint16_t aa: 1; /* authoritative answer */
	qse_uint16_t tc: 1; /* truncated message */
	qse_uint16_t rd: 1; /* recursion desired */

	qse_uint16_t ra: 1; /* recursion available */
	qse_uint16_t z: 1; 
	qse_uint16_t ad: 1;
	qse_uint16_t cd: 1;
	qse_uint16_t rcode: 4;
#else
	qse_uint16_t rd: 1;
	qse_uint16_t tc: 1;
	qse_uint16_t aa: 1;
	qse_uint16_t opcode: 4;
	qse_uint16_t qr: 1;

	qse_uint16_t rcode: 4;
	qse_uint16_t cd: 1;
	qse_uint16_t ad: 1;
	qse_uint16_t z: 1; 
	qse_uint16_t ra: 1;
#endif

	qse_uint16_t qdcount; /* questions */
	qse_uint16_t ancount; /* answers */
	qse_uint16_t nscount; /* name servers */
	qse_uint16_t arcount; /* additional resource */
};

struct dns_qdtrail_t
{
	qse_uint16_t qtype;
	qse_uint16_t qclass;
};

struct dns_antrail_t
{
	qse_uint16_t qtype;
	qse_uint16_t qclass;
	qse_uint32_t ttl;
	qse_uint16_t dlen; /* data length */
};
#include <qse/unpack.h>


#define DN_AT_END(ptr) (ptr[0] == QSE_MT('\0') || (ptr[0] == QSE_MT('.') && ptr[1] == QSE_MT('\0')))

static qse_size_t to_dn (const qse_mchar_t* str, qse_uint8_t* buf, qse_size_t bufsz)
{
	qse_uint8_t* bp = buf, * be = buf + bufsz;

	QSE_ASSERT (QSE_SIZEOF(qse_uint8_t) == QSE_SIZEOF(qse_mchar_t));

	if (!DN_AT_END(str))
	{
		qse_uint8_t* lp;
		qse_size_t len;
		const qse_mchar_t* seg;
		const qse_mchar_t* cur = str - 1;

		do
		{
			if (bp < be) lp = bp++;
			else lp = QSE_NULL;

			seg = ++cur;
			while (*cur != QSE_MT('\0') && *cur != QSE_MT('.'))
			{
				if (bp < be) *bp++ = *cur; 
				cur++;
			}
			len = cur - seg;
			if (len <= 0 || len > 63) return 0;

			if (lp) *lp = (qse_uint8_t)len;
		}
		while (!DN_AT_END(cur));
	}

	if (bp < be) *bp++ = 0;
	return bp - buf;
}

static qse_size_t dn_length (qse_uint8_t* ptr, qse_size_t len)
{
	qse_uint8_t* curptr;
	qse_size_t curlen, seglen;

	curptr = ptr;
	curlen = len;

	do
	{
		if (curlen <= 0) return 0;

		seglen = *curptr++;
		curlen = curlen - 1;
		if (seglen == 0) break;
		else if (seglen > curlen || seglen > 63) return 0;

		curptr += seglen;
		curlen -= seglen;
	}
	while (1);

	return curptr - ptr;
}

int init_dns_query (qse_uint8_t* buf, qse_size_t len, const qse_mchar_t* name, int qtype, qse_uint16_t seq)
{
	dns_hdr_t* hdr;
	dns_qdtrail_t* qdtrail;
	qse_size_t x;

	if (len < QSE_SIZEOF(*hdr)) return -1;

	QSE_MEMSET (buf, 0, len);
	hdr = (dns_hdr_t*)buf;
	hdr->id = qse_hton16(seq);
	hdr->opcode = DNS_OPCODE_QUERY; 
	hdr->rd = 1;  /* recursion desired*/
	hdr->qdcount = qse_hton16(1); /* 1 question */

	len -= QSE_SIZEOF(*hdr);
	
	x = to_dn (name, (qse_uint8_t*)(hdr + 1), len);
	if (x <= 0) return -1;
	len -= x;

	if (len < QSE_SIZEOF(*qdtrail)) return -1;
	qdtrail = (dns_qdtrail_t*)((qse_uint8_t*)(hdr + 1) + x);

	qdtrail->qtype = qse_hton16(qtype);
	qdtrail->qclass = qse_hton16(DNS_QCLASS_IN);
	return QSE_SIZEOF(*hdr) + x + QSE_SIZEOF(*qdtrail);
}

static int dns_open (qse_httpd_t* httpd, qse_httpd_dns_t* dns)
{
#if defined(__DOS__)
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#else
	int fd = -1, flag;
	qse_nwad_t nwad;
	dns_ctx_t* dc;

	dc = (dns_ctx_t*) qse_httpd_callocmem (httpd, QSE_SIZEOF(dns_ctx_t));
	if (dc == NULL) goto oops;

/* TODO: get this from configuration??? or /etc/resolv.conf */
	if (qse_mbstonwad ("8.8.8.8:53", &nwad) <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		goto oops;
	}

	dc->skadlen = qse_nwadtoskad (&nwad, &dc->skad);
	if (dc->skadlen <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	}

	fd = socket (qse_skadfamily(&dc->skad), SOCK_DGRAM, IPPROTO_UDP);
	if (fd <= -1) goto oops;

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
	setsockopt (fd, SOL_SOCKET, SO_REUSEPORT, (void*)&flag, QSE_SIZEOF(flag));
	#endif

	dns->handle.i = fd;
	dns->ctx = dc;
	return 0;

oops:
	if (fd >= 0) 
	{
	#if defined(_WIN32)
		closesocket (fd);
	#elif defined(__OS2__)
		soclose (fd);
	#else
		QSE_CLOSE (fd);
	#endif
	}

	if (dc) qse_httpd_freemem (httpd, dc);
	return -1;

#endif
}

static void dns_close (qse_httpd_t* httpd, qse_httpd_dns_t* dns)
{
#if defined(_WIN32)
	closesocket (dns->handle.i);
#elif defined(__OS2__)
	soclose (dns->handle.i);
#elif defined(__DOS__)
	/* TODO: */
#else
	QSE_CLOSE (dns->handle.i);
#endif

	qse_httpd_freemem (httpd, dns->ctx);
}

static int dns_recv (qse_httpd_t* httpd, qse_httpd_dns_t* dns)
{
	dns_ctx_t* dc = (dns_ctx_t*)dns->ctx;
	qse_skad_t fromaddr;
	socklen_t fromlen; /* TODO: change type */
	qse_uint8_t buf[384];
	qse_ssize_t len;
	dns_hdr_t* hdr;

printf ("RECV....\n");

	fromlen = QSE_SIZEOF(fromaddr);
	len = recvfrom (dns->handle.i, buf, QSE_SIZEOF(buf), 0, (struct sockaddr*)&fromaddr, &fromlen);

/* TODO: check if fromaddr matches the dc->skad... */

	if (len >= QSE_SIZEOF(*hdr))
	{
		qse_uint16_t id, qdcount, ancount, xid;

		hdr = (dns_hdr_t*)buf;
		id = qse_ntoh16(hdr->id);
		qdcount = qse_ntoh16(hdr->qdcount);
		ancount = qse_ntoh16(hdr->ancount);

		xid = (id >= QSE_COUNTOF(dc->reqs))? (id - QSE_COUNTOF(dc->reqs)): id;

printf ("%d qdcount %d ancount %d\n", id, qdcount, ancount);
		if (id >= 0 && id < QSE_COUNTOF(dc->reqs) * 2 && hdr->qr && hdr->opcode == DNS_OPCODE_QUERY && qdcount >= 1)
		{
			qse_uint8_t* plptr = (qse_uint8_t*)(hdr + 1);
			qse_size_t pllen = len - QSE_SIZEOF(*hdr);
			qse_uint8_t i, dnlen;
			dns_req_t* req = QSE_NULL, * preq = QSE_NULL;
			qse_size_t reclen;

printf ("finding match req...\n");
			for (i = 0; i < qdcount; i++)
			{
				dnlen = dn_length (plptr, pllen);
				if (dnlen <= 0) goto done; /* invalid dn name */

				reclen = dnlen + QSE_SIZEOF(dns_qdtrail_t);
				if (pllen < reclen) goto done;
printf ("1111111111111111111111\n");
				if (!req)
				{
					dns_qdtrail_t* qdtrail = (dns_qdtrail_t*)(plptr + dnlen);
					for (preq = QSE_NULL, req = dc->reqs[xid]; req; preq = req, req = req->next)
					{
printf ("checking req... %d %d\n",(int)req->dnlen, (int)dnlen);
						if (req->dnlen == dnlen && 
						    QSE_MEMCMP (req->dn, plptr, req->dnlen) == 0 &&
						    qdtrail->qclass == qse_hton16(DNS_QCLASS_IN) &&
						    (qdtrail->qtype == qse_hton16(DNS_QTYPE_A) || qdtrail->qtype == qse_hton16(DNS_QTYPE_AAAA)))
						{
							/* found a matching request */
printf ("found matching req...\n");
							break;
						}
					}
				}

				plptr += reclen; 
				pllen -= reclen;
			}

			if (!req) goto done;

			if (hdr->rcode == DNS_RCODE_NOERROR && ancount > 0)
			{
				dns_antrail_t* antrail;
				qse_uint16_t qtype, anlen;

printf ("checking answers.... pllen => %d\n", (int)pllen);
				for (i = 0; i < ancount; i++)
				{
					if (pllen < 1) goto done;
					if (*plptr > 63) dnlen = 2;
					else
					{
						dnlen = dn_length (plptr, pllen);
printf ("........... %d\n", dnlen);
						if (dnlen <= 0) goto done; /* invalid dn name */
					}

					reclen = dnlen + QSE_SIZEOF(dns_antrail_t);
					if (pllen < reclen) goto done;

					antrail = (dns_antrail_t*)(plptr + dnlen);
					reclen += qse_ntoh16(antrail->dlen);
					if (pllen < reclen) goto done;

					qtype = qse_ntoh16(antrail->qtype);
					anlen = qse_ntoh16(antrail->dlen);

					if (antrail->qclass == qse_hton16(DNS_QCLASS_IN))
					{
						if (qtype == DNS_QTYPE_A && anlen == 4)
						{
							qse_nwad_t nwad;

							QSE_MEMSET (&nwad, 0, QSE_SIZEOF(nwad));
							nwad.type = QSE_NWAD_IN4;
							QSE_MEMCPY (&nwad.u.in4.addr, antrail + 1, 4);
printf ("invoking resoll with ipv4 \n");
							req->resol (httpd, req->name, &nwad, req->ctx);

							/* delete the request from dc */
							if (preq) preq->next = req->next;
							else dc->reqs[xid] = req->next;
							qse_httpd_freemem (httpd, req);

							goto done;
						}
						else if (qtype == DNS_QTYPE_AAAA || anlen == 16)
						{
							qse_nwad_t nwad;

							QSE_MEMSET (&nwad, 0, QSE_SIZEOF(nwad));
							nwad.type = QSE_NWAD_IN6;
							QSE_MEMCPY (&nwad.u.in6.addr,  antrail + 1, 16);
printf ("invoking resoll with ipv6 \n");
							req->resol (httpd, req->name, &nwad, req->ctx);

							/* delete the request from dc */
							if (preq) preq->next = req->next;
							else dc->reqs[xid] = req->next;
							qse_httpd_freemem (httpd, req);

							goto done;
						}
					}

					plptr += reclen;
					pllen -= reclen;
				}
			}
			else
			{
				if (id == req->seqa) req->flags |= DNS_REQ_A_ERROR;
				else if (id == req->seqaaaa) req->flags |= DNS_REQ_AAAA_ERROR;

				if ((req->flags & (DNS_REQ_A_ERROR | DNS_REQ_AAAA_ERROR)) == (DNS_REQ_A_ERROR | DNS_REQ_AAAA_ERROR))
				{
					req->resol (httpd, req->name, QSE_NULL, req->ctx);

					/* delete the request from dc */
					if (preq) preq->next = req->next;
					else dc->reqs[xid] = req->next;
					qse_httpd_freemem (httpd, req);
				}
			}

			/*req->resol (httpd, req->name, QSE_NULL, req->ctx);*//* TODO: handle this better */
		}
		
	}

done:
	return 0;
}

static int dns_send (qse_httpd_t* httpd, qse_httpd_dns_t* dns, const qse_mchar_t* name, qse_httpd_resol_t resol, void* ctx)
{
	qse_uint32_t seq;
	dns_ctx_t* dc = (dns_ctx_t*)dns->ctx;
	dns_req_t* req;
	qse_size_t name_len;

	seq = dc->seq;
	seq = (seq + 1) % QSE_COUNTOF(dc->reqs);
	dc->seq = seq;

	name_len = qse_mbslen(name);

	/* dn is at most as long as the source length + 2.
     *  a.bb.ccc => 1a2bb3ccc0  => +2
     *  a.bb.ccc. => 1a2bb3ccc0  => +1 */
	req = qse_httpd_callocmem (httpd, QSE_SIZEOF(*req) + (name_len + 1) + (name_len + 2));
	if (req == QSE_NULL) return  -1;

	req->seqa = seq;
	req->seqaaaa = seq + QSE_COUNTOF(dc->reqs); /* this must not go beyond the qse_uint16_t max */
	req->name = (qse_mchar_t*)(req + 1);
	req->dn = (qse_uint8_t*)(req->name + name_len + 1);

	qse_mbscpy (req->name, name);
	req->dnlen = to_dn (name, req->dn, name_len + 2);
	if (req->dnlen <= 0)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		qse_httpd_freemem (httpd,req);
		return -1;
	}
	req->resol = resol;
	req->ctx = ctx;
	
	req->qalen = init_dns_query (req->qa, QSE_SIZEOF(req->qa), name, DNS_QTYPE_A, req->seqa);
	req->qaaaalen = init_dns_query (req->qaaaa, QSE_SIZEOF(req->qaaaa), name, DNS_QTYPE_AAAA, req->seqaaaa);
	if (req->qalen <= -1 || req->qaaaalen <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		qse_httpd_freemem (httpd,req);
		return -1;
	}

printf ("SENDING......\n");
	if (sendto (dns->handle.i, req->qa, req->qalen, 0, (struct sockaddr*)&dc->skad, dc->skadlen) != req->qalen ||
	    sendto (dns->handle.i, req->qaaaa, req->qaaaalen, 0, (struct sockaddr*)&dc->skad, dc->skadlen) != req->qaaaalen)
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		qse_httpd_freemem (httpd, req);
		return -1;
	}

	req->next = dc->reqs[seq];
	dc->reqs[seq] = req;

	return 0;
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
	qse_httpd_task_t* task;
	server_xtn_t* server_xtn;
	qse_http_method_t mth;
	qse_httpd_rsrc_t rsrc;

	server_xtn = (server_xtn_t*)qse_httpd_getserverxtn (httpd, client->server);

	/* percent-decode the query path to the original buffer
	 * since i'm not going to need it in the original form
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
				!(req->attr.flags & QSE_HTRE_ATTR_LENGTH) &&
				!(req->attr.flags & QSE_HTRE_ATTR_CHUNKED))
			{
				/* POST without Content-Length nor not chunked */
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
				/* failed to make a resource. just send the internal server error.
				 * the makersrc handler can return a negative number to return 
				 * '500 Internal Server Error'. If it wants to return a specific
				 * error code, it should return 0 with the QSE_HTTPD_RSRC_ERR
				 * resource. */
				qse_httpd_discardcontent (httpd, req);
				task = qse_httpd_entaskerr (httpd, client, QSE_NULL, 500, req);
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
				if (rsrc.type == QSE_HTTPD_RSRC_ERR) 
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
printf ("SWITCHING HTRD TO DUMMY.... %s\n", qse_htre_getqpath(req));
			/* Switch the http read to a dummy mode so that the subsqeuent
			 * input is just treaet as connects to the request just completed */
			qse_htrd_setoption (client->htrd, qse_htrd_getoption(client->htrd) | QSE_HTRD_DUMMY);

			if (server_xtn->makersrc (httpd, client, req, &rsrc) <= -1)
			{
printf ("CANOT MAKE RESOURCE.... %s\n", qse_htre_getqpath(req));
				/* failed to make a resource. just send the internal server error.
				 * the makersrc handler can return a negative number to return 
				 * '500 Internal Server Error'. If it wants to return a specific
				 * error code, it should return 0 with the QSE_HTTPD_RSRC_ERR
				 * resource. */
				task = qse_httpd_entaskerr (httpd, client, QSE_NULL, 500, req);
			}
			else
			{
				/* arrange the actual resource to be returned */
				task = qse_httpd_entaskrsrc (httpd, client, QSE_NULL, &rsrc, req);
				server_xtn->freersrc (httpd, client, req, &rsrc);
			}

			if (task == QSE_NULL) goto oops;
		}
		else if (req->attr.flags & QSE_HTRE_ATTR_PROXIED)
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

	if (!(req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE) || mth == QSE_HTTP_CONNECT)
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
	const qse_mchar_t* head, * foot, * msg;

	server_xtn = qse_httpd_getserverxtn (httpd, client->server);

	if (server_xtn->query (httpd, client->server, QSE_NULL, QSE_NULL, QSE_HTTPD_SERVERSTD_ERRHEAD, &head) <= -1) head = QSE_NULL;
	if (head == QSE_NULL) head = QSE_MT("<style type='text/css'>body { background-color:#d0e4fe; font-size: 0.9em; } div.header { font-weight: bold; margin-bottom: 5px; } div.footer { border-top: 1px solid #99AABB; text-align: right; }</style>");

	if (server_xtn->query (httpd, client->server, QSE_NULL, QSE_NULL, QSE_HTTPD_SERVERSTD_ERRFOOT, &foot) <= -1) foot = QSE_NULL;
	if (foot == QSE_NULL) foot = qse_httpd_getname(httpd);

	msg = qse_httpstatustombs(code);

/* TODO: use my own version of snprintf replacement */
	n = snprintf (buf, bufsz,
		QSE_MT("<html><head>%s<title>%s</title></head><body><div class='header'>HTTP ERROR</div><div class='body'>%d %s</div><div class='footer'>%s</div></body></html>"), 
		head, msg, code, msg, foot);
	if (n < 0 || n >= bufsz) 
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
	  file_purge,
	  file_ropen,
	  file_wopen,
	  file_close,
	  file_read,
	  file_write
	},

	/* directory operation */
	{ dir_stat,
	  dir_make,
	  dir_purge,
	  dir_open,
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
	  client_closed },


	/* dns */
	{ dns_open,
	  dns_close,
	  dns_recv,
	  dns_send }
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

		case QSE_HTTPD_RSRC_RELOC:
			if (target->u.reloc.dst != qpath)
				QSE_MMGR_FREE (httpd->mmgr, (qse_mchar_t*)target->u.reloc.dst);
			break;

		case QSE_HTTPD_RSRC_REDIR:
			if (target->u.redir.dst != qpath)
				QSE_MMGR_FREE (httpd->mmgr, (qse_mchar_t*)target->u.redir.dst);
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
	const qse_mchar_t* qpath;
	const qse_mchar_t* idxfile;
	qse_mchar_t* xpath;

	qse_size_t qpath_len;
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
				#if 0
				script = merge_paths (httpd, tmp->qpath, tmp->idxfile);
				if (script == QSE_NULL) goto oops;
				#endif

				/* create a relocation resource */
				target->type = QSE_HTTPD_RSRC_RELOC;
				target->u.reloc.dst = merge_paths (httpd, tmp->qpath, tmp->idxfile);
				if (target->u.reloc.dst == QSE_NULL) goto oops;
				/* free tmp->xpath here upon success since it's not used for relocation.
				 * it is freed by the called upon failure so the 'oops' part don't free it */
				QSE_MMGR_FREE (httpd->mmgr, tmp->xpath);
				return 1;
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
						/* stop at the current segment if stating fails. 
						 * if the current semgment can't be stat-ed, it's not likely that
						 * the next segment can be successfully stat-ed */
						break; 
					}

					if (!st.isdir)
					{
						if (server_xtn->query (httpd, client->server, req, tmp->xpath, QSE_HTTPD_SERVERSTD_CGI, &cgi) >= 0 && cgi.cgi)
						{
							/* the script name is composed of the orginal query path.
							 * the pointer held in 'slash' is valid for tmp->qpath as
							 * tmp->qpath_rp is at most the tail part of tmp->qpath. */
							script = qse_mbsxdup (tmp->qpath, slash - tmp->qpath, httpd->mmgr);
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

		/* restore xpath because it has changed... */
		if (xpath_changed) merge_paths_to_buf (httpd, tmp->root.u.path.val, tmp->qpath_rp, (qse_size_t)-1, tmp->xpath);
	}

	return 0; /* not a cgi */

bingo:
	target->type = QSE_HTTPD_RSRC_CGI;
	target->u.cgi.nph = cgi.nph;
	target->u.cgi.path = tmp->xpath;
	target->u.cgi.script = script;
	target->u.cgi.suffix = suffix;
	target->u.cgi.root = tmp->root.u.path.val;
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
	qse_http_method_t mth;

	qse_httpd_stat_t st;
	int n, stx, acc;

	QSE_MEMSET (&tmp, 0, QSE_SIZEOF(tmp));
	tmp.qpath = qse_htre_getqpath(req);
	tmp.qpath_len = qse_mbslen (tmp.qpath);

	QSE_MEMSET (target, 0, QSE_SIZEOF(*target));

	server_xtn = qse_httpd_getserverxtn (httpd, client->server);

	mth = qse_htre_getqmethodtype (req);
	if (mth == QSE_HTTP_CONNECT)
	{
		/* TODO: query if CONNECT is allowed */
		/* TODO: check on what conditions CONNECT is allowed.  */
		/* TODO: disallow connecting back to self */
		/* TODO: Proxy-Authorization???? */

		target->type = QSE_HTTPD_RSRC_PROXY;
		target->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_RAW;

		if (qse_mbstonwad (qse_htre_getqpath(req), &target->u.proxy.dst.nwad) <= -1) 
		{
			target->u.proxy.flags |= QSE_HTTPD_RSRC_PROXY_DST_STR;
			target->u.proxy.dst.str = qse_htre_getqpath(req);
		}
		else
		{
			/* make the source binding type the same as destination */
			target->u.proxy.src.nwad.type = target->u.proxy.dst.nwad.type;
		}

		/* mark that this request is going to be proxied. */
		req->attr.flags |= QSE_HTRE_ATTR_PROXIED;
		return 0;
	}

	if (server_xtn->query (httpd, client->server, req, QSE_NULL, QSE_HTTPD_SERVERSTD_ROOT, &tmp.root) <= -1) return -1;
	switch (tmp.root.type)
	{
		case QSE_HTTPD_SERVERSTD_ROOT_NWAD:
			/* proxy the request */
			target->type = QSE_HTTPD_RSRC_PROXY;
			target->u.proxy.flags = 0;

			/* transparent proxy may do the following
			target->u.proxy.dst = client->orgdst_addr; 
			target->u.proxy.src = client->remote_addr;
			*/
			target->u.proxy.dst.nwad = tmp.root.u.nwad;
			target->u.proxy.src.nwad.type = target->u.proxy.dst.nwad.type;

			/* mark that this request is going to be proxied. */
			req->attr.flags |= QSE_HTRE_ATTR_PROXIED;
			return 0;

		case QSE_HTTPD_SERVERSTD_ROOT_TEXT:
			target->type = QSE_HTTPD_RSRC_TEXT;
			target->u.text.ptr = tmp.root.u.text.ptr;
			target->u.text.mime = tmp.root.u.text.mime;
			return 0;
	}

	/* handle the request locally */
	QSE_ASSERT (tmp.root.type == QSE_HTTPD_SERVERSTD_ROOT_PATH);

	if (server_xtn->query (httpd, client->server, req, QSE_NULL, QSE_HTTPD_SERVERSTD_REALM, &tmp.realm) <= -1 ||
	    server_xtn->query (httpd, client->server, req, QSE_NULL, QSE_HTTPD_SERVERSTD_INDEX, &tmp.index) <= -1)
	{
		return -1;
	}

	/* default to the root directory. */
	if (!tmp.root.u.path.val) tmp.root.u.path.val = QSE_MT("/"); 

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
	    			if (server_xtn->query (httpd, client->server, req, QSE_NULL, QSE_HTTPD_SERVERSTD_AUTH, &tmp.auth) >= 0 && tmp.auth.authok) goto auth_ok;
			}
		}

		target->type = QSE_HTTPD_RSRC_AUTH;
		target->u.auth.realm = tmp.realm.name; 
		return 0;
	}

auth_ok:

	/* if authentication is ok or no authentication is required,
	 * handle 'Expect: 100-continue' if it is contained in the header */
	if ((req->attr.flags & QSE_HTRE_ATTR_EXPECT) &&
	    (req->version.major > 1 || (req->version.major == 1 && req->version.minor >= 1)) &&
	    qse_htre_getcontentlen(req) <= 0)
	{
		/* "Expect" in the header, version 1.1 or higher,
		 * and no content received yet. don't care about the method type. 
		 *
		 * if the partial or complete content is already received,
		 * we don't need to send '100 continue'. */

		if (req->attr.flags & QSE_HTRE_ATTR_EXPECT100)
		{
			/* "Expect: 100-continue" in the header.
			 * mark to return "100 continue" */
			target->flags |= QSE_HTTPD_RSRC_100_CONTINUE;
		}
		else
		{
			/* Expectation Failed */
			qse_htre_discardcontent (req);
			target->type = QSE_HTTPD_RSRC_ERR;
			target->u.err.code = 417;
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
			if (target->u.err.code < 200 || target->u.err.code > 299)
			{
				qse_htre_discardcontent (req);
				target->type = QSE_HTTPD_RSRC_ERR;
				/* free xpath since it won't be used */
				QSE_MMGR_FREE (httpd->mmgr, tmp.xpath);
			}
			else if (tmp.qpath[tmp.qpath_len - 1] != QSE_MT('/'))
			{
				/* the query path doesn't end with a slash. so redirect it  */
				qse_htre_discardcontent (req);
				target->type = QSE_HTTPD_RSRC_REDIR;
				target->u.redir.dst = tmp.qpath;
				/* free xpath since it won't be used */
				QSE_MMGR_FREE (httpd->mmgr, tmp.xpath);
			}
			else
			{
				target->type = QSE_HTTPD_RSRC_DIR;
				target->u.dir.path = tmp.xpath;
				if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_DIRHEAD, &target->u.dir.head) <= -1) target->u.dir.head = QSE_NULL;
				if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_DIRFOOT, &target->u.dir.foot) <= -1) target->u.dir.foot = QSE_NULL;
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

		acc = (tmp.idxfile || !qse_mbsend(tmp.qpath, QSE_MT("/")))? 
			QSE_HTTPD_SERVERSTD_FILEACC: QSE_HTTPD_SERVERSTD_DIRACC;

		/* check file's access permission */
		if (server_xtn->query (httpd, client->server, req, tmp.xpath, acc, &target->u.err.code) <= -1) target->u.err.code = 500;

		if (target->u.err.code < 200 || target->u.err.code > 299)
		{
			/* free xpath since it won't be used */
			qse_htre_discardcontent (req);
			QSE_MMGR_FREE (httpd->mmgr, tmp.xpath);
			target->type = QSE_HTTPD_RSRC_ERR;
		}
		else
		{
			/* fall back to a normal file. */
			if (tmp.idxfile)
			{
				qse_htre_discardcontent (req);

				/* free xpath since it won't be used */
				QSE_MMGR_FREE (httpd->mmgr, tmp.xpath);

				/* create a relocation resource */
				target->type = QSE_HTTPD_RSRC_RELOC;
				target->u.reloc.dst = merge_paths (httpd, tmp.qpath, tmp.idxfile);
				if (target->u.reloc.dst == QSE_NULL) return -1;
			}
			else if (acc == QSE_HTTPD_SERVERSTD_DIRACC)
			{
				target->type = QSE_HTTPD_RSRC_DIR;
				target->u.dir.path = tmp.xpath;

				if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_DIRHEAD, &target->u.dir.head) <= -1) target->u.dir.head = QSE_NULL;
				if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_DIRFOOT, &target->u.dir.foot) <= -1) target->u.dir.foot = QSE_NULL;
			}
			else
			{
				target->type = QSE_HTTPD_RSRC_FILE;
				target->u.file.path = tmp.xpath;

				if (server_xtn->query (httpd, client->server, req, tmp.xpath, QSE_HTTPD_SERVERSTD_MIME, &target->u.file.mime) <= -1)
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
		case QSE_HTTPD_SERVERSTD_SSL:
			/* you must specify the certificate and the key file to be able
			 * to use SSL */
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
		{
			/* i don't allow PUT or DELET by default.
			 * override this query result if you want to change
			 * the behavior. */
			switch (qse_htre_getqmethodtype(req))
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
