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

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h> /* sockaddr_in6 */
#	include <windows.h>

#elif defined(__OS2__)
	/* TODO */

#elif defined(__DOS__)
	/* TODO */

#else
#	include "../cmn/syscall.h"
#	include <sys/socket.h>
#	include <netinet/in.h>
#	if defined(HAVE_SYS_SENDFILE_H)
#		include <sys/sendfile.h>
#	endif
#	if defined(HAVE_EPOLL) && defined(HAVE_SYS_EPOLL_H)
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

#include <qse/cmn/stdio.h> /* TODO: remove this */

#define DEFAULT_PORT        80
#define DEFAULT_SECURE_PORT 443

enum server_xtn_cfg_id_t
{
	SERVER_XTN_CFG_DOCROOT = 0,
	SERVER_XTN_CFG_REALM,
	SERVER_XTN_CFG_AUTH, /* basic auth */
	SERVER_XTN_CFG_DIRCSS, /* can't be too long due to internal buffer size  */
	SERVER_XTN_CFG_ERRCSS,
	SERVER_XTN_CFG_MAX
};

typedef struct server_xtn_t server_xtn_t;
struct server_xtn_t
{
	qse_mchar_t* cfg[SERVER_XTN_CFG_MAX];

	union
	{
		void* a[4];
		struct
		{
			qse_httpd_server_cbstd_t*    cbstd;
			qse_httpd_server_cgistd_t*   cgistd;
			qse_httpd_server_mimestd_t*  mimestd;
			qse_httpd_server_idxstd_t*   idxstd;
		} s;
	} cfg2;

	/* private */
	qse_httpd_server_predetach_t predetach;
};

/* ------------------------------------------------------------------- */

#if defined(_WIN32)
static qse_httpd_errnum_t syserr_to_errnum (DWORD e)
{

	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			return QSE_HTTPD_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_HTTPD_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_HTTPD_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_HTTPD_ENOENT;

		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			return QSE_HTTPD_EEXIST;

		default:
			return QSE_HTTPD_ESYSERR;
	}
}
#elif defined(__OS2__)
static qse_httpd_errnum_t syserr_to_errnum (APIRET e)
{
	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
			return QSE_HTTPD_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_HTTPD_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_HTTPD_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_HTTPD_ENOENT;

		case ERROR_ALREADY_EXISTS:
			return QSE_HTTPD_EEXIST;

		default:
			return QSE_HTTPD_ESYSERR;
	}
}
#elif defined(__DOS__)
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
	
		default:
			return QSE_HTTPD_ESYSERR;
	}
}

#else
static qse_httpd_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_HTTPD_ENOMEM;

		case EINVAL:
			return QSE_HTTPD_EINVAL;

		case EACCES:
		case ECONNREFUSED:
			return QSE_HTTPD_EACCES;

		case ENOENT:
		case ENOTDIR:
			/* ENOTDIR can be returned in this situation.
			 *   i want to access /tmp/t1.cgi/abc/def
			 *   while /tmp/t1.cgi is an existing file.
			 * I'm not sure if it is really good to translate
			 * ENOTDIR to QSE_HTTPD_ENOENT.	
			 */
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
#endif

/* ------------------------------------------------------------------- */

#define MAX_SEND_SIZE 4096

#if defined(_WIN32)
	/* TODO */
	/* TODO: WIN32 TransmitFile */
#elif defined(__OS2__)
	/* TODO */
#elif defined(__DOS__)
 	/* TODO */

#elif defined(HAVE_SENDFILE) && defined(HAVE_SENDFILE64)
#	if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_SENDFILE64)
#		define xsendfile(out,in,offset,count) sendfile64(out,in,offset,count)
#	else
#		define xsendfile(out,in,offset,count) sendfile(out,in,offset,count)
#	endif

#elif defined(HAVE_SENDFILE)
#	define xsendfile(out,in,offset,count) sendfile(out,in,offset,count)

#elif defined(HAVE_SENDFILE64)
#	define xsendfile(out,in,offset,count) sendfile64(out,in,offset,count)

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
		vec.sfv_off = QSE_LSEEK (in_fd, 0, SEEK_CUR); 
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

	if (offset && QSE_LSEEK (in_fd, *offset, SEEK_SET) != *offset)  
		return (qse_ssize_t)-1;

	if (count > QSE_COUNTOF(buf)) count = QSE_COUNTOF(buf);
	n = read (in_fd, buf, count);
	if (n == (qse_ssize_t)-1 || n == 0) return n;

	n = send (out_fd, buf, n, 0);
	if (n > 0 && offset) *offset = *offset + n;

	return n;
}

#endif

/* ------------------------------------------------------------------- */

#if defined(HAVE_SSL)
static qse_ssize_t xsendfile_ssl (
	SSL* out, int in_fd, qse_foff_t* offset, qse_size_t count)
{
	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_ssize_t n;

	if (offset && QSE_LSEEK (in_fd, *offset, SEEK_SET) != *offset)  
		return (qse_ssize_t)-1;

	if (count > QSE_COUNTOF(buf)) count = QSE_COUNTOF(buf);
	n = read (in_fd, buf, count);
	if (n == (qse_ssize_t)-1 || n == 0) return n;

	n = SSL_write (out, buf, count);
	if (n > 0 && offset) *offset = *offset + n;

	return n;
}
#endif

/* ------------------------------------------------------------------- */

typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
#if defined(HAVE_SSL)
	SSL_CTX* ssl_ctx;
#endif
};

#if defined(HAVE_SSL)
static int init_xtn_ssl (
	httpd_xtn_t* xtn,
	const qse_mchar_t* pemfile,
	const qse_mchar_t* keyfile/*,
	const qse_mchar_t* chainfile*/)
{
	SSL_CTX* ctx;

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
/* TODO: logging */
qse_fprintf (QSE_STDERR, QSE_T("Error: %hs\n"), buf);
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

	static qse_httpd_ecb_t std_ecb = 
	{
		QSE_FV(.close, cleanup_standard_httpd)
	};

	httpd = qse_httpd_open (mmgr, QSE_SIZEOF(httpd_xtn_t) + xtnsize);
	if (httpd == QSE_NULL) return QSE_NULL;

	xtn = (httpd_xtn_t*)qse_httpd_getxtn (httpd);
	QSE_MEMSET (xtn, 0, QSE_SIZEOF(httpd_xtn_t) + xtnsize);

#if defined(HAVE_SSL)
	/*init_xtn_ssl (xtn, "http01.pem", "http01.key");*/
#endif

	qse_httpd_pushecb (httpd, &std_ecb);
	return httpd;	
}

void* qse_httpd_getxtnstd (qse_httpd_t* httpd)
{
	return (void*)((httpd_xtn_t*)QSE_XTN(httpd) + 1);
}

/* ------------------------------------------------------------------- */

union sockaddr_t
{
	struct sockaddr_in in4;
#if defined(AF_INET6)
	struct sockaddr_in6 in6;
#endif
};

typedef union sockaddr_t sockaddr_t;

#define SOCKADDR_FAMILY(x) (((struct sockaddr_in*)(x))->sin_family)

static int sockaddr_to_nwad (const sockaddr_t* addr, qse_nwad_t* nwad)
{
	int addrsize = -1;

	switch (SOCKADDR_FAMILY(addr))
	{
		case AF_INET:
		{
			struct sockaddr_in* in;
			in = (struct sockaddr_in*)addr;
			addrsize = QSE_SIZEOF(*in);

			QSE_MEMSET (nwad, 0, QSE_SIZEOF(*nwad));
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

			QSE_MEMSET (nwad, 0, QSE_SIZEOF(*nwad));
			nwad->type = QSE_NWAD_IN6;
			QSE_MEMCPY (&nwad->u.in6.addr, &in->sin6_addr, QSE_SIZEOF(nwad->u.in6.addr));
			nwad->u.in6.scope = in->sin6_scope_id;
			nwad->u.in6.port = in->sin6_port;
			break;
		}
#endif
	}

	return addrsize;
}

static int nwad_to_sockaddr (const qse_nwad_t* nwad, sockaddr_t* addr)
{
	int addrsize = -1;

	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
		{
			struct sockaddr_in* in;

			in = (struct sockaddr_in*)addr;
			addrsize = QSE_SIZEOF(*in);
			QSE_MEMSET (in, 0, addrsize);

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
			QSE_MEMSET (in, 0, addrsize);

			in->sin6_family = AF_INET6;
			QSE_MEMCPY (&in->sin6_addr, &nwad->u.in6.addr, QSE_SIZEOF(nwad->u.in6.addr));
			in->sin6_scope_id = nwad->u.in6.scope;
			in->sin6_port = nwad->u.in6.port;
#endif
			break;
		}
	}

	return addrsize;
}

/* ------------------------------------------------------------------- */

static int server_open (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	int fd = -1, flag;
	sockaddr_t addr;
	int addrsize;

	addrsize = nwad_to_sockaddr (&server->nwad, &addr);
	if (addrsize <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	}

	fd = socket (SOCKADDR_FAMILY(&addr), SOCK_STREAM, IPPROTO_TCP);
	if (fd <= -1) goto oops;

	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);

	flag = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &flag, QSE_SIZEOF(flag));

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

	if (server->flags & QSE_HTTPD_SERVER_BINDTONWIF)
	{
		qse_mchar_t tmp[64];
		qse_size_t len;

		len = qse_nwifindextombs (server->nwif, tmp, QSE_COUNTOF(tmp));

		if (len <= 0 || setsockopt (fd, SOL_SOCKET, SO_BINDTODEVICE, tmp, len) <= -1)
		{
			/* TODO: logging ... */
			goto oops;
		}
	}

	/* Solaris 8 returns EINVAL if QSE_SIZEOF(addr) is passed in as the
	 * address size for AF_INET. */
	/*if (bind (s, (struct sockaddr*)&addr, QSE_SIZEOF(addr)) <= -1) goto oops_esocket;*/
	if (bind (fd, (struct sockaddr*)&addr, addrsize) <= -1)
	{
#if defined(IPV6_V6ONLY)
		if (errno == EADDRINUSE && SOCKADDR_FAMILY(&addr) == AF_INET6)
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

	flag = fcntl (fd, F_GETFL);
	if (flag >= 0) fcntl (fd, F_SETFL, flag | O_NONBLOCK);

	server->handle.i = fd;
	return 0;

oops:
	qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
	if (fd >= 0) QSE_CLOSE (fd);
	return -1;
}

static void server_close (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	QSE_CLOSE (server->handle.i);
}

static int server_accept (
	qse_httpd_t* httpd, qse_httpd_server_t* server, qse_httpd_client_t* client)
{
	sockaddr_t addr;

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
		QSE_CLOSE (fd);
		return -1;
	}
#endif
	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);

	flag = fcntl (fd, F_GETFL);
	if (flag >= 0) fcntl (fd, F_SETFL, flag | O_NONBLOCK);


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

#if defined(SO_ORIGINAL_DST)
	/* if REDIRECT is used, SO_ORIGINAL_DST returns the original
	 * destination. If TPROXY is used, getsockname() above returns
	 * the original address. */

	addrlen = QSE_SIZEOF(addr);
	if (getsockopt (fd, SOL_IP, SO_ORIGINAL_DST, (char*)&addr, &addrlen) <= -1 ||
	    sockaddr_to_nwad (&addr, &client->orgdst_addr) <= -1)
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
}

/* ------------------------------------------------------------------- */

static int peer_open (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
	int fd = -1, flag;
	sockaddr_t connaddr, bindaddr;
	int connaddrsize, bindaddrsize;
	int connected = 1;

	connaddrsize = nwad_to_sockaddr (&peer->nwad, &connaddr);
	bindaddrsize = nwad_to_sockaddr (&peer->local, &bindaddr);
	if (connaddrsize <= -1 || bindaddrsize <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
		return -1;
	}

	fd = socket (SOCKADDR_FAMILY(&connaddr), SOCK_STREAM, IPPROTO_TCP);
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

	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);

	flag = fcntl (fd, F_GETFL);
	if (flag >= 0) fcntl (fd, F_SETFL, flag | O_NONBLOCK);

	if (connect (fd, (struct sockaddr*)&connaddr, connaddrsize) <= -1)
	{
		if (errno != EINPROGRESS) goto oops;
		connected = 0;
	}

	/* restore flags */
	if (fcntl (fd, F_SETFL, flag) <= -1) goto oops;

	peer->handle.i = fd;
	return connected;

oops:
	qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
	if (fd >= 0) QSE_CLOSE (fd);
	return -1;
}

static void peer_close (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
	QSE_CLOSE (peer->handle.i);
}

static int peer_connected (qse_httpd_t* httpd, qse_httpd_peer_t* peer)
{
#ifdef HAVE_SOCKLEN_T
	socklen_t len;
#else
	int len;
#endif
	int ret;

	len = QSE_SIZEOF(ret);
	if (getsockopt (peer->handle.i, SOL_SOCKET, SO_ERROR, &ret, &len) <= -1) return -1;

	if (ret == EINPROGRESS) return 0;
	if (ret != 0)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum (ret));
		return -1;
	}

	return 1; /* connection completed */
}

static qse_ssize_t peer_recv (
	qse_httpd_t* httpd, qse_httpd_peer_t* peer,
	qse_mchar_t* buf, qse_size_t bufsize)
{
	ssize_t ret = read (peer->handle.i, buf, bufsize);
	if (ret <= -1) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
	return ret;
}

static qse_ssize_t peer_send (
	qse_httpd_t* httpd, qse_httpd_peer_t* peer,
	const qse_mchar_t* buf, qse_size_t bufsize)
{
	ssize_t ret = write (peer->handle.i, buf, bufsize);
	if (ret <= -1) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
	return ret;
}

/* ------------------------------------------------------------------- */

struct mux_ev_t
{
	qse_ubi_t handle;
	int reqmask;
	qse_httpd_muxcb_t cbfun;
	void* cbarg;
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

	struct
	{
		struct mux_ev_t** ptr;
		qse_size_t        capa;
	} mev;
};

#define MUX_EV_ALIGN 64

static void* mux_open (qse_httpd_t* httpd)
{
	struct mux_t* mux;

	mux = qse_httpd_allocmem (httpd, QSE_SIZEOF(*mux));
	if (mux == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (mux, 0, QSE_SIZEOF(*mux));

#if defined(HAVE_EPOLL_CREATE1) && defined(O_CLOEXEC)
	mux->fd = epoll_create1 (O_CLOEXEC);
#else
	mux->fd = epoll_create (100);
#endif
	if (mux->fd <= -1)
	{
		qse_httpd_freemem (httpd, mux);
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return QSE_NULL;
	}

#if defined(HAVE_EPOLL_CREATE1) && defined(O_CLOEXEC)
	/* nothing else to do */
#else
	{
		int flag = fcntl (mux->fd, F_GETFD);
		if (flag >= 0) fcntl (mux->fd, F_SETFD, flag | FD_CLOEXEC);
	}
#endif

	return mux;
}

static void mux_close (qse_httpd_t* httpd, void* vmux)
{
	struct mux_t* mux = (struct mux_t*)vmux;
	if (mux->ee.ptr) qse_httpd_freemem (httpd, mux->ee.ptr);
	if (mux->mev.ptr)
	{
		qse_size_t i;
		for (i = 0; i < mux->mev.capa; i++)
			if (mux->mev.ptr[i]) qse_httpd_freemem (httpd, mux->mev.ptr[i]);
		qse_httpd_freemem (httpd, mux->mev.ptr);
	}
	QSE_CLOSE (mux->fd);
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

	if (ev.events == 0 || handle.i <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		return -1;
	}

	if (handle.i >= mux->mev.capa)
	{
		struct mux_ev_t** tmp;
		qse_size_t tmpcapa, i;

		tmpcapa = (((handle.i + MUX_EV_ALIGN) / MUX_EV_ALIGN) * MUX_EV_ALIGN);

		tmp = (struct mux_ev_t**) qse_httpd_reallocmem (
			httpd, mux->mev.ptr,
			QSE_SIZEOF(*mux->mev.ptr) * tmpcapa);
		if (tmp == QSE_NULL) return -1;

		for (i = mux->mev.capa; i < tmpcapa; i++) tmp[i] = QSE_NULL;
		mux->mev.ptr = tmp;
		mux->mev.capa = tmpcapa;
	}

	if (mux->mev.ptr[handle.i] == QSE_NULL)
	{
		/* the location of the data passed to epoll_ctl()
		 * must not change unless i update the info with epoll()
		 * whenever there is reallocation. so i simply
		 * make mux-mev.ptr reallocatable but auctual
		 * data fixed once allocated. */
		mux->mev.ptr[handle.i] = qse_httpd_allocmem (
			httpd, QSE_SIZEOF(*mux->mev.ptr[handle.i]));
		if (mux->mev.ptr[handle.i] == QSE_NULL) return -1;
	}

	if (mux->ee.len >= mux->ee.capa)
	{
		struct epoll_event* tmp;

		tmp = qse_httpd_reallocmem (
			httpd, mux->ee.ptr,
			QSE_SIZEOF(*mux->ee.ptr) * (mux->ee.capa + 1) * 2);
		if (tmp == QSE_NULL) return -1;

		mux->ee.ptr = tmp;
		mux->ee.capa = (mux->ee.capa + 1) * 2;
	}

	mev = mux->mev.ptr[handle.i];
	mev->handle = handle;
	mev->reqmask = mask;
	mev->cbfun = cbfun;
	mev->cbarg = cbarg;

	ev.data.ptr = mev;

	if (epoll_ctl (mux->fd, EPOLL_CTL_ADD, handle.i, &ev) <= -1)
	{
		/* don't rollback ee.ptr */
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
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

		if (mux->ee.ptr[i].events & EPOLLIN)
			mask |= QSE_HTTPD_MUX_READ;
		if (mux->ee.ptr[i].events & EPOLLOUT)
			mask |= QSE_HTTPD_MUX_WRITE;

		if (mux->ee.ptr[i].events & EPOLLHUP)
		{
			if (mev->reqmask & QSE_HTTPD_MUX_READ)
				mask |= QSE_HTTPD_MUX_READ;
			if (mev->reqmask & QSE_HTTPD_MUX_WRITE)
				mask |= QSE_HTTPD_MUX_WRITE;
		}

		mev->cbfun (httpd, mux, mev->handle, mask, mev->cbarg);
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


static int stat_file (
	qse_httpd_t* httpd, const qse_mchar_t* path,
	qse_httpd_stat_t* hst, int regonly)
{
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
	hst->mtime = QSE_SECNSEC_TO_MSEC(st.st_mtim.tv_sec,st.st_mtim.tv_nsec);
#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC)
	hst->mtime = QSE_SECNSEC_TO_MSEC(st.st_mtimespec.tv_sec,st.st_mtimespec.tv_nsec);
#else
	hst->mtime = QSE_SEC_TO_MSEC(st.st_mtime);
#endif

	return 0;
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
	return stat_file (httpd, path, hst, 1);	
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
	fd = QSE_OPEN (path, flags, 0);
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
	fd = QSE_OPEN (path, flags, 0644);
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
	QSE_CLOSE (handle.i);
}

static qse_ssize_t file_read (
	qse_httpd_t* httpd, qse_ubi_t handle,
	qse_mchar_t* buf, qse_size_t len)
{
	return QSE_READ (handle.i, buf, len);
}

static qse_ssize_t file_write (
	qse_httpd_t* httpd, qse_ubi_t handle,
	const qse_mchar_t* buf, qse_size_t len)
{
	return QSE_WRITE (handle.i, buf, len);
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

	d->dp = QSE_OPENDIR (path);
	if (d->dp == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		QSE_MMGR_FREE (httpd->mmgr, d->path);
		QSE_MMGR_FREE (httpd->mmgr, d);
		return -1;
	}
		
	handle->ptr = d;
	return 0;
}

static void dir_close (qse_httpd_t* httpd, qse_ubi_t handle)
{
	dir_t* d;

	d = (dir_t*)handle.ptr;

	QSE_CLOSEDIR (d->dp);

	QSE_MMGR_FREE (httpd->mmgr, d->path);
	QSE_MMGR_FREE (httpd->mmgr, d);
}

static int dir_read (qse_httpd_t* httpd, qse_ubi_t handle, qse_httpd_dirent_t* dirent)
{
	dir_t* d;
	qse_dirent_t* de;
	qse_mchar_t* fpath;
	int n;

	d = (dir_t*)handle.ptr;

	errno = 0;
	de = QSE_READDIR (d->dp);
	if (de == QSE_NULL) 
	{
		if (errno == 0) return 0;
		qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return -1;
	}

	/* i assume that d->path ends with a slash */
	fpath = qse_mbsdup2 (d->path, de->d_name, httpd->mmgr);
	if (fpath == QSE_NULL)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return -1;
	}

	n = stat_file (httpd, fpath, &dirent->stat, 0);	
	QSE_MMGR_FREE (httpd->mmgr, fpath);
	if (n <= -1) QSE_MEMSET (dirent, 0, QSE_SIZEOF(*dirent));

	dirent->name = de->d_name;
	return 1;
}

/* ------------------------------------------------------------------- */
static void client_close (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	QSE_CLOSE (client->handle.i);
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
		return -1;
#endif
	}
	else
	{
		ssize_t ret;
		ret = recv (client->handle.i, buf, bufsize, 0);
		if (ret <= -1) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return ret;
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
		return -1;
#endif
	}
	else
	{
		ssize_t ret = send (client->handle.i, buf, bufsize, 0);
		if (ret <= -1) qse_httpd_seterrnum (httpd, syserr_to_errnum(errno));
		return ret;
	}
}

static qse_ssize_t client_sendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_ubi_t handle, qse_foff_t* offset, qse_size_t count)
{
	if (client->status & CLIENT_SECURE)
	{
#if defined(HAVE_SSL)
		return xsendfile_ssl (client->handle2.ptr, handle.i, offset, count);
#else
		return -1;
#endif
	}
	else
	{
		return xsendfile (client->handle.i, handle.i, offset, count);
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
			if (init_xtn_ssl (xtn, "http01.pem", "http01.key") <= -1) 
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

qse_printf (QSE_T("SSL ACCEPTING %d\n"), client->handle.i);
qse_fflush (QSE_STDOUT);
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
			if (SSL_get_error(ssl,ret) == SSL_ERROR_WANT_READ)
			{
				/* handshaking isn't complete. */
				return 0;
			}

			qse_fprintf (QSE_STDERR, QSE_T("Error: SSL ACCEPT ERROR\n"));
			/* SSL_free (ssl); */
			return -1;
		}
qse_printf (QSE_T("SSL ACCEPTED %d\n"), client->handle.i);
qse_fflush (QSE_STDOUT);
#else
		qse_fprintf (QSE_STDERR, QSE_T("Error: NO SSL SUPPORT\n"));
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
			else if (server_xtn->cfg2.s.cbstd->makersrc (httpd, client, req, &rsrc) <= -1)
			{
				qse_httpd_discardcontent (httpd, req);
				task = qse_httpd_entaskerr (httpd, client, QSE_NULL, 500, req);
			}
			else
			{
				task = qse_httpd_entaskrsrc (httpd, client, QSE_NULL, &rsrc, req);
				if (server_xtn->cfg2.s.cbstd->freersrc) server_xtn->cfg2.s.cbstd->freersrc (httpd, client, req, &rsrc);
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

static int handle_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	return process_request (httpd, client, req, 0);
}

static int format_err (
	qse_httpd_t* httpd, qse_httpd_client_t* client, int code, qse_mchar_t* buf, int bufsz)
{
	int n;
	server_xtn_t* server_xtn;
	const qse_mchar_t* css, * msg;

	server_xtn = qse_httpd_getserverxtn (httpd, client->server);

	css = server_xtn->cfg[SERVER_XTN_CFG_ERRCSS];
	if (!css) css = QSE_MT("");

	msg = qse_httpstatustombs(code);

/* TODO: use my own version of snprintf replacement */
	n = snprintf (buf, bufsz,
		QSE_MT("<html><head>%s<title>%s</title></head><body><div class='header'>HTTP ERROR</div><div class='body'>%d %s</div><div class='footer'>%s</div></body></html>"), 
		css, msg, code, msg, qse_httpd_getname(httpd));
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

			css = server_xtn->cfg[SERVER_XTN_CFG_DIRCSS];
			if (!css) css = QSE_MT("");

/* TODO: html escaping of qpath */
			n = snprintf (buf, bufsz,
				QSE_MT("<html><head>%s</head><body><div class='header'>%s</div><div class='body'><table>%s"), css, qpath,
				(is_root? QSE_MT(""): QSE_MT("<tr><td class='name'><a href='../'>..</a></td><td class='time'></td><td class='size'></td></tr>"))
			);
		}
		else
		{
			/* footer */
			n = snprintf (buf, bufsz, QSE_MT("</table></div><div class='footer'>%s</div></body></html>"), qse_httpd_getname(httpd));
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

		qse_localtime (dirent->stat.mtime, &bt);
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
	{ file_executable,
	  file_stat,
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
	peek_request,
	handle_request,
	format_err,
	format_dir
};

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

static int attempt_cgi (
	qse_httpd_t* httpd, const qse_mchar_t* docroot, 
	qse_mchar_t* xpath, const qse_mchar_t* qpath, const qse_mchar_t* idxfile,
	qse_httpd_server_cgistd_t cgistd[], qse_httpd_rsrc_t* target)
{
	qse_mchar_t* ext;
	qse_mchar_t* script, * suffix;
	qse_size_t i;

	if (idxfile)
	{
		for (i = 0; cgistd[i].ext; i++)
		{
			if (qse_mbsend (idxfile, cgistd[i].ext))
			{
				script = merge_paths (httpd, qpath, idxfile);
				if (script == QSE_NULL) return -1;

				target->type = QSE_HTTPD_RSRC_CGI;
				target->u.cgi.nph = cgistd[i].nph;
				target->u.cgi.path = xpath;
				target->u.cgi.script = script;
				target->u.cgi.suffix = QSE_NULL;
				target->u.cgi.docroot = docroot;
				return 1;
			}
		}
	}
	else
	{
		for (i = 0; cgistd[i].ext; i++)
		{
	/* TODO: attempt other segments if qpath is like
	 *       /abc/x.cgi/y.cgi/ttt. currently, it tries x.cgi only.
	 *       x.cgi could be a directory name .
	 */
			ext = qse_mbsstr (qpath, cgistd[i].ext);
	
			if (ext && (ext[cgistd[i].len] == QSE_MT('/') || 
			            ext[cgistd[i].len] == QSE_MT('\0'))) 
			{
				if (ext[cgistd[i].len] == QSE_MT('/')) 
				{
					/* it has a path suffix */
					script = qse_mbsxdup (qpath, ext - qpath + cgistd[i].len, httpd->mmgr);
					suffix = qse_mbsdup (&ext[cgistd[i].len], httpd->mmgr);
					if (script == QSE_NULL || suffix == QSE_NULL)
					{
						if (suffix) QSE_MMGR_FREE (httpd->mmgr, suffix);
						if (script) QSE_MMGR_FREE (httpd->mmgr, script);
						httpd->errnum = QSE_HTTPD_ENOMEM;
						return -1;
					}
	
					/* drop the suffix part */
					xpath[qse_mbslen(xpath) - qse_mbslen(suffix)] = QSE_MT('\0');
				}
				else
				{
					/* it has no path suffix  */
					script = qpath;
					suffix = QSE_NULL;
				}
	
				target->type = QSE_HTTPD_RSRC_CGI;
				target->u.cgi.nph = cgistd[i].nph;
				target->u.cgi.path = xpath;
				target->u.cgi.script = script;
				target->u.cgi.suffix = suffix;
				target->u.cgi.docroot = docroot;
				return 1;
			}
		}

	}

	return 0;
}

static int make_resource (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_htre_t* req, qse_httpd_rsrc_t* target)
{
	server_xtn_t* server_xtn;
	const qse_mchar_t* qpath;
	const qse_mchar_t* idxfile;
	qse_mchar_t* xpath; 
	qse_stat_t st;
	qse_size_t i;
	int n;

	qpath = qse_htre_getqpath(req);

	QSE_MEMSET (target, 0, QSE_SIZEOF(*target));

	server_xtn = qse_httpd_getserverxtn (httpd, client->server);

	if (server_xtn->cfg[SERVER_XTN_CFG_REALM] &&
	    server_xtn->cfg[SERVER_XTN_CFG_AUTH])
	{
		const qse_htre_hdrval_t* auth;

		auth = qse_htre_getheaderval (req, QSE_MT("Authorization"));
		if (auth)
		{
			while (auth->next) auth = auth->next;

			if (qse_mbszcasecmp(auth->ptr, QSE_MT("Basic "), 6) == 0) 
			{
				if (qse_mbscmp (&auth->ptr[6], server_xtn->cfg[SERVER_XTN_CFG_AUTH]) == 0) goto auth_ok;
			}
		}

		target->type = QSE_HTTPD_RSRC_AUTH;
		target->u.auth.realm = server_xtn->cfg[SERVER_XTN_CFG_REALM];
		return 0;
	}

auth_ok:
	idxfile = QSE_NULL;
	xpath = merge_paths (httpd, server_xtn->cfg[SERVER_XTN_CFG_DOCROOT], qpath);
	if (xpath == QSE_NULL) return -1;

	if (QSE_STAT (xpath, &st) == 0 && S_ISDIR(st.st_mode))
	{
		/* it is a directory */
		if (server_xtn->cfg2.s.idxstd)
		{
			/* try to locate an index file */
			for (i = 0; server_xtn->cfg2.s.idxstd[i].name; i++)
			{
				qse_mchar_t* tpath;

				tpath = merge_paths (httpd, xpath, server_xtn->cfg2.s.idxstd[i].name);
				if (tpath == QSE_NULL) 
				{
					QSE_MMGR_FREE (httpd->mmgr, xpath);
					return -1;
				}

				if (QSE_STAT (tpath, &st) == 0 && S_ISREG(st.st_mode))
				{
					/* the index file is found */
					QSE_MMGR_FREE (httpd->mmgr, xpath);
					xpath = tpath;
					idxfile = server_xtn->cfg2.s.idxstd[i].name;
					goto attempt_file;
				}	

				QSE_MMGR_FREE (httpd->mmgr, tpath);
			}
		}

		target->type = QSE_HTTPD_RSRC_DIR;
		target->u.dir.path = xpath;
	}
	else
	{
	attempt_file:
		if (server_xtn->cfg2.s.cgistd)
		{
			/* check if the request can resolve to a cgi script */
			n = attempt_cgi (httpd, server_xtn->cfg[SERVER_XTN_CFG_DOCROOT], 
			                 xpath, qpath, idxfile, server_xtn->cfg2.s.cgistd, target);
			if (n <= -1) 
			{
				QSE_MMGR_FREE (httpd->mmgr, xpath);
				return -1;
			}
			if (n >= 1) return 0;
		}

		/* fall back to a normal file. */
		target->type = QSE_HTTPD_RSRC_FILE;
		target->u.file.path = xpath;
		target->u.file.mime = QSE_NULL;
		if (server_xtn->cfg2.s.mimestd)
		{
			for (i = 0; server_xtn->cfg2.s.mimestd[i].ext; i++)
			{
/* TODO: require the table sorted and so the binary search */
				if (qse_mbsend (qpath, server_xtn->cfg2.s.mimestd[i].ext))
					target->u.file.mime = server_xtn->cfg2.s.mimestd[i].type; 
			}
		}
	}

	return 0;
}

static qse_httpd_server_cbstd_t server_cbstd =
{
	make_resource,
	free_resource	
};
/* ------------------------------------------------------------------- */

static void predetach_server (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	server_xtn_t* server_xtn;
	qse_size_t i;

	server_xtn = (server_xtn_t*) qse_httpd_getserverxtn (httpd, server);
	
	if (server_xtn->predetach) server_xtn->predetach (httpd, server);

	for (i = QSE_COUNTOF(server_xtn->cfg); i > 0; )
	{
		if (server_xtn->cfg[--i])
		{
			QSE_MMGR_FREE (httpd->mmgr, server_xtn->cfg[i]);
			server_xtn->cfg[i] = QSE_NULL;
		}
	}
}

qse_httpd_server_t* qse_httpd_attachserverstd (
	qse_httpd_t* httpd, const qse_char_t* uri, 
	qse_httpd_server_predetach_t predetach, qse_size_t xtnsize)
{
	qse_httpd_server_t server;
	qse_httpd_server_t* xserver;
	server_xtn_t* server_xtn;
	qse_mxstr_t ba;
	qse_size_t balen2;

	qse_uint16_t default_port;
	qse_uri_t xuri;

	static qse_httpd_server_cgistd_t server_cgistd[] = 
	{
		{ QSE_MT(".cgi"), 4, 0 },
		{ QSE_MT(".nph"), 4, 1 },
		{ QSE_NULL,       0, 0 }
	};

	static qse_httpd_server_mimestd_t server_mimestd[] =
	{
		{ QSE_MT(".html"), QSE_MT("text/html") },
		{ QSE_MT(".htm"),  QSE_MT("text/htm") },
		{ QSE_MT(".txt"),  QSE_MT("text/plain") },
		{ QSE_MT(".log"),  QSE_MT("text/plain") },
		{ QSE_MT(".css"),  QSE_MT("text/css") },
		{ QSE_MT(".xml"),  QSE_MT("text/xml") },
		{ QSE_MT(".js"),   QSE_MT("application/javascript") },
		{ QSE_MT(".jpg"),  QSE_MT("image/jpeg") },
		{ QSE_MT(".png"),  QSE_MT("image/png") },
		{ QSE_MT(".mp4"),  QSE_MT("video/mp4") },
		{ QSE_MT(".mp3"),  QSE_MT("audio/mpeg") }, 
		{ QSE_MT(".c"),    QSE_MT("text/plain") }, 
		{ QSE_MT(".h"),    QSE_MT("text/plain") },
		{ QSE_MT(".cpp"),  QSE_MT("text/plain") }, 
		{ QSE_MT(".hpp"),  QSE_MT("text/plain") },
		{ QSE_NULL,        QSE_NULL }
	};

#if defined(QSE_CHAR_IS_MCHAR)
	qse_mcstr_t tmp[4] = 
	{ 
		{ QSE_MT(""),  0 },
		{ QSE_MT(":"), 1 },
		{ QSE_MT(""),  0 },
		{ QSE_NULL,    0 } 
	};
#else
	qse_wcstr_t tmp[4] = 
	{ 
		{ QSE_WT(""),  0 },
		{ QSE_WT(":"), 1 },
		{ QSE_WT(""),  0 },
		{ QSE_NULL,    0 } 
	};
#endif

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

	xserver = qse_httpd_attachserver (
		httpd, &server, predetach_server, QSE_SIZEOF(*server_xtn) + xtnsize);
	if (xserver == QSE_NULL) return QSE_NULL;

	server_xtn = qse_httpd_getserverxtn (httpd, xserver);

	if (!xuri.path.ptr)
	{
#if defined(QSE_CHAR_IS_MCHAR)
		xuri.path.ptr = QSE_MT("/");
#else
		xuri.path.ptr = QSE_WT("/");
#endif
		xuri.path.len = 1;
	}

	if (xuri.auth.user.ptr)
	{
		tmp[0].ptr = xuri.auth.user.ptr;
		tmp[0].len = xuri.auth.user.len;
	}
	if (xuri.auth.pass.ptr)
	{
		tmp[2].ptr = xuri.auth.pass.ptr;
		tmp[2].len = xuri.auth.pass.len;
	}

#if defined(QSE_CHAR_IS_MCHAR)
	server_xtn->cfg[SERVER_XTN_CFG_DOCROOT] = qse_mbsxdup (xuri.path.ptr, xuri.path.len, httpd->mmgr);
	if (xuri.frag.ptr) server_xtn->cfg[SERVER_XTN_CFG_REALM] = qse_mbsxdup (xuri.frag.ptr, xuri.frag.len, httpd->mmgr);
	ba.ptr = qse_mbsxadup (tmp, &ba.len, httpd->mmgr);
	
#else
	server_xtn->cfg[SERVER_XTN_CFG_DOCROOT] = qse_wcsntombsdup (xuri.path.ptr, xuri.path.len, QSE_NULL, httpd->mmgr);
	if (xuri.frag.ptr) server_xtn->cfg[SERVER_XTN_CFG_REALM] = qse_wcsntombsdup (xuri.frag.ptr, xuri.frag.len, QSE_NULL, httpd->mmgr);
	ba.ptr = qse_wcsnatombsdup (tmp, &ba.len, httpd->mmgr);
#endif

	if ((!server_xtn->cfg[SERVER_XTN_CFG_DOCROOT]) ||
	    (xuri.frag.ptr && !server_xtn->cfg[SERVER_XTN_CFG_REALM]) ||
	    !ba.ptr) 
	{
		if (ba.ptr) QSE_MMGR_FREE (httpd->mmgr, ba.ptr);
		goto nomem_after_attach;
	}

	balen2 = ((ba.len / 3) + 1) * 4;
	server_xtn->cfg[SERVER_XTN_CFG_AUTH] = QSE_MMGR_ALLOC (
		httpd->mmgr, (balen2 + 1) * QSE_SIZEOF(qse_mchar_t));
	if (!server_xtn->cfg[SERVER_XTN_CFG_AUTH]) 
	{
		QSE_MMGR_FREE (httpd->mmgr, ba.ptr);
		goto nomem_after_attach;
	}

	qse_enbase64 (
		ba.ptr, ba.len,
		server_xtn->cfg[SERVER_XTN_CFG_AUTH],
		balen2,
		&balen2
	);
	QSE_MMGR_FREE (httpd->mmgr, ba.ptr);
	(server_xtn->cfg[SERVER_XTN_CFG_AUTH])[balen2] = QSE_MT('\0');

	server_xtn->predetach = predetach;
	server_xtn->cfg2.s.cbstd = &server_cbstd;
	server_xtn->cfg2.s.cgistd = server_cgistd;
	server_xtn->cfg2.s.mimestd = server_mimestd;
	server_xtn->cfg2.s.idxstd = QSE_NULL;

	return xserver;

invalid:
	httpd->errnum = QSE_HTTPD_EINVAL;
	return QSE_NULL;

nomem_after_attach:
	qse_httpd_detachserver (httpd, xserver);	
	httpd->errnum = QSE_HTTPD_ENOMEM;
	return QSE_NULL;
}

int qse_httpd_getserveroptstd (
	qse_httpd_t* httpd, qse_httpd_server_t* server,
	qse_httpd_server_optstd_t id, void** value)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverxtn (httpd, server);

	switch (id)
	{
		case QSE_HTTPD_SERVER_DOCROOT:
		case QSE_HTTPD_SERVER_REALM:
		case QSE_HTTPD_SERVER_AUTH:
		case QSE_HTTPD_SERVER_ERRCSS:
		case QSE_HTTPD_SERVER_DIRCSS:
			*value = (void*)server_xtn->cfg[id - QSE_HTTPD_SERVER_DOCROOT];
			return 0;

		case QSE_HTTPD_SERVER_CBSTD:
		case QSE_HTTPD_SERVER_CGISTD:
		case QSE_HTTPD_SERVER_MIMESTD:
		case QSE_HTTPD_SERVER_IDXSTD:
			*value = (void*)server_xtn->cfg2.a[id - QSE_HTTPD_SERVER_CBSTD];
			return 0;
	}	

	httpd->errnum = QSE_HTTPD_EINVAL;
	return -1;
}

int qse_httpd_setserveroptstd (
	qse_httpd_t* httpd, qse_httpd_server_t* server,
	qse_httpd_server_optstd_t id, void* value)
{
	server_xtn_t* server_xtn;
	qse_mchar_t* mctmp;

	server_xtn = qse_httpd_getserverxtn (httpd, server);

	switch (id)
	{
		case QSE_HTTPD_SERVER_DOCROOT:
		case QSE_HTTPD_SERVER_REALM:
		case QSE_HTTPD_SERVER_AUTH:
		case QSE_HTTPD_SERVER_ERRCSS:
		case QSE_HTTPD_SERVER_DIRCSS:
			mctmp = (qse_mchar_t*)value;
			if(mctmp)
			{	
				mctmp = qse_mbsdup ((qse_mchar_t*)mctmp, httpd->mmgr);
				if (mctmp == QSE_NULL)
				{
					httpd->errnum = QSE_HTTPD_ENOMEM;
					return -1;
				}
			}

			if (server_xtn->cfg[id - QSE_HTTPD_SERVER_DOCROOT])
			{
				QSE_MMGR_FREE (httpd->mmgr, 
					server_xtn->cfg[id - QSE_HTTPD_SERVER_DOCROOT]);
			}
		
			server_xtn->cfg[id - QSE_HTTPD_SERVER_DOCROOT] = mctmp;
			return 0;

		case QSE_HTTPD_SERVER_CBSTD:
		case QSE_HTTPD_SERVER_CGISTD:
		case QSE_HTTPD_SERVER_MIMESTD:
		case QSE_HTTPD_SERVER_IDXSTD:
			server_xtn->cfg2.a[id - QSE_HTTPD_SERVER_CBSTD] = value;
			return 0;
	}	

	httpd->errnum = QSE_HTTPD_EINVAL;
	return -1;
}


void* qse_httpd_getserverxtnstd (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	server_xtn_t* xtn = qse_httpd_getserverxtn (httpd, server);
	return (void*)(xtn + 1);
}

/* ------------------------------------------------------------------- */

int qse_httpd_loopstd (qse_httpd_t* httpd, qse_ntime_t timeout)
{
	return qse_httpd_loop (
		httpd, &httpd_system_callbacks,
		&httpd_request_callbacks, timeout);	
}
