/*
 * $Id$
 *
    Copyright (c) 2006-2016 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAfRRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <qse/si/aio-sck.h>
#include "aio-prv.h"

#include <qse/cmn/hton.h>

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#if defined(HAVE_NETPACKET_PACKET_H)
#	include <netpacket/packet.h>
#endif

#if defined(HAVE_NET_IF_DL_H)
#	include <net/if_dl.h>
#endif


#if defined(__linux__)
#	include <limits.h>
#	if defined(HAVE_LINUX_NETFILTER_IPV4_H)
#		include <linux/netfilter_ipv4.h> /* SO_ORIGINAL_DST */
#	endif
#	if !defined(SO_ORIGINAL_DST)
#		define SO_ORIGINAL_DST 80
#	endif
#	if !defined(IP_TRANSPARENT)
#		define IP_TRANSPARENT 19
#	endif
#	if !defined(SO_REUSEPORT)
#		define SO_REUSEPORT 15
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


/* ========================================================================= */
void qse_aio_closeasyncsck (qse_aio_t* aio, qse_aio_sckhnd_t sck)
{
#if defined(_WIN32)
	closesocket (sck);
#else
	close (sck);
#endif
}

int qse_aio_makesckasync (qse_aio_t* aio, qse_aio_sckhnd_t sck)
{
	return qse_aio_makesyshndasync (aio, (qse_aio_syshnd_t)sck);
}

qse_aio_sckhnd_t qse_aio_openasyncsck (qse_aio_t* aio, int domain, int type, int proto)
{
	qse_aio_sckhnd_t sck;

#if defined(_WIN32)
	sck = WSASocket (domain, type, proto, NULL, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
	if (sck == QSE_AIO_SCKHND_INVALID) 
	{
		/* qse_aio_seterrnum (dev->aio, QSE_AIO_ESYSERR); or translate errno to aio errnum */
		return QSE_AIO_SCKHND_INVALID;
	}
#else
	sck = socket (domain, type, proto); 
	if (sck == QSE_AIO_SCKHND_INVALID) 
	{
		aio->errnum = qse_aio_syserrtoerrnum(errno);
		return QSE_AIO_SCKHND_INVALID;
	}

#if defined(FD_CLOEXEC)
	{
		int flags = fcntl (sck, F_GETFD, 0);
		if (fcntl (sck, F_SETFD, flags | FD_CLOEXEC) == -1)
		{
			aio->errnum = qse_aio_syserrtoerrnum(errno);
			return QSE_AIO_SCKHND_INVALID;
		}
	}
#endif

	if (qse_aio_makesckasync (aio, sck) <= -1)
	{
		close (sck);
		return QSE_AIO_SCKHND_INVALID;
	}

#endif

	return sck;
}

int qse_aio_getsckaddrinfo (qse_aio_t* aio, const qse_aio_sckaddr_t* addr, qse_aio_scklen_t* len, qse_aio_sckfam_t* family)
{
	struct sockaddr* saddr = (struct sockaddr*)addr;

	switch (saddr->sa_family)
	{
		case AF_INET:
			if (len) *len = QSE_SIZEOF(struct sockaddr_in);
			if (family) *family = AF_INET;
			return 0;

		case AF_INET6:
			if (len) *len =  QSE_SIZEOF(struct sockaddr_in6);
			if (family) *family = AF_INET6;
			return 0;

	#if defined(AF_PACKET) && (QSE_SIZEOF_STRUCT_SOCKADDR_LL > 0)
		case AF_PACKET:
			if (len) *len =  QSE_SIZEOF(struct sockaddr_ll);
			if (family) *family = AF_PACKET;
			return 0;
	#elif defined(AF_LINK) && (QSE_SIZEOF_STRUCT_SOCKADDR_DL > 0)
		case AF_LINK:
			if (len) *len =  QSE_SIZEOF(struct sockaddr_dl);
			if (family) *family = AF_LINK;
			return 0;
	#endif

		/* TODO: more address type */
	}

	aio->errnum = QSE_AIO_EINVAL;
	return -1;
}

qse_uint16_t qse_aio_getsckaddrport (const qse_aio_sckaddr_t* addr)
{
	struct sockaddr* saddr = (struct sockaddr*)addr;

	switch (saddr->sa_family)
	{
		case AF_INET:
			return qse_ntoh16(((struct sockaddr_in*)addr)->sin_port);

		case AF_INET6:
			return qse_ntoh16(((struct sockaddr_in6*)addr)->sin6_port);
	}

	return 0;
}

int qse_aio_getsckaddrifindex (const qse_aio_sckaddr_t* addr)
{
	struct sockaddr* saddr = (struct sockaddr*)addr;

#if defined(AF_PACKET) && (QSE_SIZEOF_STRUCT_SOCKADDR_LL > 0)
	if (saddr->sa_family == AF_PACKET)
	{
		return ((struct sockaddr_ll*)addr)->sll_ifindex;
	}

#elif defined(AF_LINK) && (QSE_SIZEOF_STRUCT_SOCKADDR_DL > 0)
	if (saddr->sa_family == AF_LINK)
	{
		return ((struct sockaddr_dl*)addr)->sdl_index;
	}
#endif

	return 0;
}

int qse_aio_equalsckaddrs (qse_aio_t* aio, const qse_aio_sckaddr_t* addr1, const qse_aio_sckaddr_t* addr2)
{
	qse_aio_sckfam_t fam1, fam2;
	qse_aio_scklen_t len1, len2;

	qse_aio_getsckaddrinfo (aio, addr1, &len1, &fam1);
	qse_aio_getsckaddrinfo (aio, addr2, &len2, &fam2);
	return fam1 == fam2 && len1 == len2 && QSE_MEMCMP (addr1, addr2, len1) == 0;
}

/* ========================================================================= */

void qse_aio_sckaddr_initforip4 (qse_aio_sckaddr_t* sckaddr, qse_uint16_t port, qse_aio_ip4addr_t* ip4addr)
{
	struct sockaddr_in* sin = (struct sockaddr_in*)sckaddr;

	QSE_MEMSET (sin, 0, QSE_SIZEOF(*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = qse_hton16(port);
	if (ip4addr) QSE_MEMCPY (&sin->sin_addr, ip4addr, QSE_AIO_IP4ADDR_LEN);
}

void qse_aio_sckaddr_initforip6 (qse_aio_sckaddr_t* sckaddr, qse_uint16_t port, qse_aio_ip6addr_t* ip6addr)
{
	struct sockaddr_in6* sin = (struct sockaddr_in6*)sckaddr;

/* TODO: include sin6_scope_id */
	QSE_MEMSET (sin, 0, QSE_SIZEOF(*sin));
	sin->sin6_family = AF_INET;
	sin->sin6_port = qse_hton16(port);
	if (ip6addr) QSE_MEMCPY (&sin->sin6_addr, ip6addr, QSE_AIO_IP6ADDR_LEN);
}

void qse_aio_sckaddr_initforeth (qse_aio_sckaddr_t* sckaddr, int ifindex, qse_aio_ethaddr_t* ethaddr)
{
#if defined(AF_PACKET) && (QSE_SIZEOF_STRUCT_SOCKADDR_LL > 0)
	struct sockaddr_ll* sll = (struct sockaddr_ll*)sckaddr;
	QSE_MEMSET (sll, 0, QSE_SIZEOF(*sll));
	sll->sll_family = AF_PACKET;
	sll->sll_ifindex = ifindex;
	if (ethaddr)
	{
		sll->sll_halen = QSE_AIO_ETHADDR_LEN;
		QSE_MEMCPY (sll->sll_addr, ethaddr, QSE_AIO_ETHADDR_LEN);
	}

#elif defined(AF_LINK) && (QSE_SIZEOF_STRUCT_SOCKADDR_DL > 0)
	struct sockaddr_dl* sll = (struct sockaddr_dl*)sckaddr;
	QSE_MEMSET (sll, 0, QSE_SIZEOF(*sll));
	sll->sdl_family = AF_LINK;
	sll->sdl_index = ifindex;
	if (ethaddr)
	{
		sll->sdl_alen = QSE_AIO_ETHADDR_LEN;
		QSE_MEMCPY (sll->sdl_data, ethaddr, QSE_AIO_ETHADDR_LEN);
	}
#else
#	error UNSUPPORTED DATALINK SOCKET ADDRESS
#endif
}

/* ========================================================================= */

static qse_aio_devaddr_t* sckaddr_to_devaddr (qse_aio_dev_sck_t* dev, const qse_aio_sckaddr_t* sckaddr, qse_aio_devaddr_t* devaddr)
{
	if (sckaddr)
	{
		qse_aio_scklen_t len;

		qse_aio_getsckaddrinfo (dev->aio, sckaddr, &len, QSE_NULL);
		devaddr->ptr = (void*)sckaddr;
		devaddr->len = len;
		return devaddr;
	}

	return QSE_NULL;
}

static QSE_INLINE qse_aio_sckaddr_t* devaddr_to_sckaddr (qse_aio_dev_sck_t* dev, const qse_aio_devaddr_t* devaddr, qse_aio_sckaddr_t* sckaddr)
{
	return (qse_aio_sckaddr_t*)devaddr->ptr;
}

/* ========================================================================= */

#define IS_STATEFUL(sck) ((sck)->dev_capa & QSE_AIO_DEV_CAPA_STREAM)

struct sck_type_map_t
{
	int domain;
	int type;
	int proto;
	int extra_dev_capa;
};

#define PROTO_ETHARP  QSE_CONST_HTON16(QSE_AIO_ETHHDR_PROTO_ARP)

static struct sck_type_map_t sck_type_map[] =
{
	/* QSE_AIO_DEV_SCK_TCP4 */
	{ AF_INET,    SOCK_STREAM,    0,              QSE_AIO_DEV_CAPA_STREAM  | QSE_AIO_DEV_CAPA_OUT_QUEUED },

	/* QSE_AIO_DEV_SCK_TCP6 */
	{ AF_INET6,   SOCK_STREAM,    0,              QSE_AIO_DEV_CAPA_STREAM  | QSE_AIO_DEV_CAPA_OUT_QUEUED },

	/* QSE_AIO_DEV_SCK_UPD4 */
	{ AF_INET,    SOCK_DGRAM,     0,              0 },

	/* QSE_AIO_DEV_SCK_UDP6 */
	{ AF_INET6,   SOCK_DGRAM,     0,              0 },


#if defined(AF_PACKET) && (QSE_SIZEOF_STRUCT_SOCKADDR_LL > 0)
	/* QSE_AIO_DEV_SCK_ARP - Ethernet type is 2 bytes long. Protocol must be specified in the network byte order */
	{ AF_PACKET,  SOCK_RAW,       PROTO_ETHARP,   0 },

	/* QSE_AIO_DEV_SCK_DGRAM */
	{ AF_PACKET,  SOCK_DGRAM,     PROTO_ETHARP,   0 },

#elif defined(AF_LINK) && (QSE_SIZEOF_STRUCT_SOCKADDR_DL > 0)
	/* QSE_AIO_DEV_SCK_ARP */
	{ AF_LINK,  SOCK_RAW,         PROTO_ETHARP,   0 },

	/* QSE_AIO_DEV_SCK_DGRAM */
	{ AF_LINK,  SOCK_DGRAM,       PROTO_ETHARP,   0 },
#else
	{ -1,       0,                0,              0 },
	{ -1,       0,                0,              0 },
#endif

	/* QSE_AIO_DEV_SCK_ICMP4 - IP protocol field is 1 byte only. no byte order conversion is needed */
	{ AF_INET,    SOCK_RAW,       IPPROTO_ICMP,   0 },

	/* QSE_AIO_DEV_SCK_ICMP6 - IP protocol field is 1 byte only. no byte order conversion is needed */
	{ AF_INET6,   SOCK_RAW,       IPPROTO_ICMP,   0 }
};

/* ======================================================================== */

static void connect_timedout (qse_aio_t* aio, const qse_ntime_t* now, qse_aio_tmrjob_t* job)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)job->ctx;

	QSE_ASSERT (IS_STATEFUL(rdev));

	if (rdev->state & QSE_AIO_DEV_SCK_CONNECTING)
	{
		/* the state check for QSE_AIO_DEV_TCP_CONNECTING is actually redundant
		 * as it must not be fired  after it gets connected. the timer job 
		 * doesn't need to be deleted when it gets connected for this check 
		 * here. this libarary, however, deletes the job when it gets 
		 * connected. */
		qse_aio_dev_sck_halt (rdev);
	}
}

static void ssl_accept_timedout (qse_aio_t* aio, const qse_ntime_t* now, qse_aio_tmrjob_t* job)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)job->ctx;

	QSE_ASSERT (IS_STATEFUL(rdev));

	if (rdev->state & QSE_AIO_DEV_SCK_ACCEPTING_SSL)
	{
		qse_aio_dev_sck_halt(rdev);
	}
}

static void ssl_connect_timedout (qse_aio_t* aio, const qse_ntime_t* now, qse_aio_tmrjob_t* job)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)job->ctx;

	QSE_ASSERT (IS_STATEFUL(rdev));

	if (rdev->state & QSE_AIO_DEV_SCK_CONNECTING_SSL)
	{
		qse_aio_dev_sck_halt(rdev);
	}
}

static int schedule_timer_job_at (qse_aio_dev_sck_t* dev, const qse_ntime_t* fire_at, qse_aio_tmrjob_handler_t handler)
{
	qse_aio_tmrjob_t tmrjob;

	QSE_MEMSET (&tmrjob, 0, QSE_SIZEOF(tmrjob));
	tmrjob.ctx = dev;
	tmrjob.when = *fire_at;

	tmrjob.handler = handler;
	tmrjob.idxptr = &dev->tmrjob_index;

	QSE_ASSERT (dev->tmrjob_index == QSE_AIO_TMRIDX_INVALID);
	dev->tmrjob_index = qse_aio_instmrjob (dev->aio, &tmrjob);
	return dev->tmrjob_index == QSE_AIO_TMRIDX_INVALID? -1: 0;
}

static int schedule_timer_job_after (qse_aio_dev_sck_t* dev, const qse_ntime_t* fire_after, qse_aio_tmrjob_handler_t handler)
{
	qse_ntime_t fire_at;

	QSE_ASSERT (qse_ispostime(fire_after));

	qse_gettime (&fire_at);
	qse_addtime (&fire_at, fire_after, &fire_at);

	return schedule_timer_job_at (dev, &fire_at, handler);
}

/* ======================================================================== */

static int dev_sck_make (qse_aio_dev_t* dev, void* ctx)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;
	qse_aio_dev_sck_make_t* arg = (qse_aio_dev_sck_make_t*)ctx;

	QSE_ASSERT (arg->type >= 0 && arg->type < QSE_COUNTOF(sck_type_map));

	if (sck_type_map[arg->type].domain <= -1)
	{
		dev->aio->errnum = QSE_AIO_ENOIMPL;
		return -1;
	}

	rdev->sck = qse_aio_openasyncsck (dev->aio, sck_type_map[arg->type].domain, sck_type_map[arg->type].type, sck_type_map[arg->type].proto);
	if (rdev->sck == QSE_AIO_SCKHND_INVALID) goto oops;

	rdev->dev_capa = QSE_AIO_DEV_CAPA_IN | QSE_AIO_DEV_CAPA_OUT | sck_type_map[arg->type].extra_dev_capa;
	rdev->on_write = arg->on_write;
	rdev->on_read = arg->on_read;
	rdev->on_disconnect = arg->on_disconnect;
	rdev->type = arg->type;
	rdev->tmrjob_index = QSE_AIO_TMRIDX_INVALID;

	return 0;

oops:
	if (rdev->sck != QSE_AIO_SCKHND_INVALID)
	{
		qse_aio_closeasyncsck (rdev->aio, rdev->sck);
		rdev->sck = QSE_AIO_SCKHND_INVALID;
	}
	return -1;
}

static int dev_sck_make_client (qse_aio_dev_t* dev, void* ctx)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;
	qse_aio_syshnd_t* sck = (qse_aio_syshnd_t*)ctx;

	/* nothing special is done here except setting the sock et handle.
	 * most of the initialization is done by the listening socket device
	 * after a client socket has been created. */

	rdev->sck = *sck;
	rdev->tmrjob_index = QSE_AIO_TMRIDX_INVALID;

	if (qse_aio_makesckasync (rdev->aio, rdev->sck) <= -1) return -1;
#if defined(FD_CLOEXEC)
	{
		int flags = fcntl (rdev->sck, F_GETFD, 0);
		if (fcntl (rdev->sck, F_SETFD, flags | FD_CLOEXEC) == -1)
		{
			rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
			return -1;
		}
	}
#endif

	return 0;
}

static int dev_sck_kill (qse_aio_dev_t* dev, int force)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;

	if (IS_STATEFUL(rdev))
	{
		/*if (QSE_AIO_DEV_SCK_GET_PROGRESS(rdev))
		{*/
			/* for QSE_AIO_DEV_SCK_CONNECTING, QSE_AIO_DEV_SCK_CONNECTING_SSL, and QSE_AIO_DEV_ACCEPTING_SSL
			 * on_disconnect() is called without corresponding on_connect() */
			if (rdev->on_disconnect) rdev->on_disconnect (rdev);
		/*}*/

		if (rdev->tmrjob_index != QSE_AIO_TMRIDX_INVALID)
		{
			qse_aio_deltmrjob (dev->aio, rdev->tmrjob_index);
			QSE_ASSERT (rdev->tmrjob_index == QSE_AIO_TMRIDX_INVALID);
		}
	}
	else
	{
		QSE_ASSERT (rdev->state == 0);
		QSE_ASSERT (rdev->tmrjob_index == QSE_AIO_TMRIDX_INVALID);

		if (rdev->on_disconnect) rdev->on_disconnect (rdev);
	}

#if defined(USE_SSL)
	if (rdev->ssl)
	{
		SSL_shutdown ((SSL*)rdev->ssl); /* is this needed? */
		SSL_free ((SSL*)rdev->ssl);
		rdev->ssl = QSE_NULL;
	}
	if (!(rdev->state & (QSE_AIO_DEV_SCK_ACCEPTED | QSE_AIO_DEV_SCK_ACCEPTING_SSL)) && rdev->ssl_ctx)
	{
		SSL_CTX_free ((SSL_CTX*)rdev->ssl_ctx);
		rdev->ssl_ctx = QSE_NULL;
	}
#endif

	if (rdev->sck != QSE_AIO_SCKHND_INVALID) 
	{
		qse_aio_closeasyncsck (rdev->aio, rdev->sck);
		rdev->sck = QSE_AIO_SCKHND_INVALID;
	}

	return 0;
}

static qse_aio_syshnd_t dev_sck_getsyshnd (qse_aio_dev_t* dev)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;
	return (qse_aio_syshnd_t)rdev->sck;
}

static int dev_sck_read_stateful (qse_aio_dev_t* dev, void* buf, qse_aio_iolen_t* len, qse_aio_devaddr_t* srcaddr)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;

#if defined(USE_SSL)
	if (rdev->ssl)
	{
		int x;

		x = SSL_read ((SSL*)rdev->ssl, buf, *len);
		if (x <= -1)
		{
			int err = SSL_get_error ((SSL*)rdev->ssl, x);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) return 0;
			rdev->aio->errnum = QSE_AIO_ESYSERR;
			return -1;
		}

		*len = x;
	}
	else
	{
#endif
		ssize_t x;

		x = recv (rdev->sck, buf, *len, 0);
		if (x == -1)
		{
			if (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN) return 0;  /* no data available */
			if (errno == EINTR) return 0;
			rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
			return -1;
		}

		*len = x;
#if defined(USE_SSL)
	}
#endif
	return 1;
}

static int dev_sck_read_stateless (qse_aio_dev_t* dev, void* buf, qse_aio_iolen_t* len, qse_aio_devaddr_t* srcaddr)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;
	qse_aio_scklen_t srcaddrlen;
	ssize_t x;

	srcaddrlen = QSE_SIZEOF(rdev->remoteaddr);
	x = recvfrom (rdev->sck, buf, *len, 0, (struct sockaddr*)&rdev->remoteaddr, &srcaddrlen);
	if (x <= -1)
	{
		if (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN) return 0;  /* no data available */
		if (errno == EINTR) return 0;
		rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}

	srcaddr->ptr = &rdev->remoteaddr;
	srcaddr->len = srcaddrlen;

	*len = x;
	return 1;
}


static int dev_sck_write_stateful (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t* len, const qse_aio_devaddr_t* dstaddr)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;

#if defined(USE_SSL)
	if (rdev->ssl)
	{
		int x;

		if (*len <= 0)
		{
			/* it's a writing finish indicator. close the writing end of
			 * the socket, probably leaving it in the half-closed state */
			if (SSL_shutdown ((SSL*)rdev->ssl) == -1)
			{
				rdev->aio->errnum = QSE_AIO_ESYSERR;
				return -1;
			}

			return 1;
		}

		x = SSL_write ((SSL*)rdev->ssl, data, *len);
		if (x <= -1)
		{
			int err = SSL_get_error ((SSL*)rdev->ssl, x);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) return 0;
			rdev->aio->errnum = QSE_AIO_ESYSERR;
			return -1;
		}

		*len = x;
	}
	else
	{
#endif
		ssize_t x;
		int flags = 0;

		if (*len <= 0)
		{
			/* it's a writing finish indicator. close the writing end of
			 * the socket, probably leaving it in the half-closed state */
			if (shutdown (rdev->sck, SHUT_WR) == -1)
			{
				rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
				return -1;
			}

			return 1;
		}

		/* TODO: flags MSG_DONTROUTE, MSG_DONTWAIT, MSG_MORE, MSG_OOB, MSG_NOSIGNAL */
	#if defined(MSG_NOSIGNAL)
		flags |= MSG_NOSIGNAL;
	#endif
		x = send (rdev->sck, data, *len, flags);
		if (x == -1) 
		{
			if (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN) return 0;  /* no data can be written */
			if (errno == EINTR) return 0;
			rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
			return -1;
		}

		*len = x;
#if defined(USE_SSL)
	}
#endif
	return 1;
}

static int dev_sck_write_stateless (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t* len, const qse_aio_devaddr_t* dstaddr)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;
	ssize_t x;

	x = sendto (rdev->sck, data, *len, 0, dstaddr->ptr, dstaddr->len);
	if (x <= -1) 
	{
		if (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN) return 0;  /* no data can be written */
		if (errno == EINTR) return 0;
		rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}

	*len = x;
	return 1;
}

#if defined(USE_SSL)

static int do_ssl (qse_aio_dev_sck_t* dev, int (*ssl_func)(SSL*))
{
	int ret;
	int watcher_cmd;
	int watcher_events;

	QSE_ASSERT (dev->ssl_ctx);

	if (!dev->ssl)
	{
		SSL* ssl;

		ssl = SSL_new (dev->ssl_ctx);
		if (!ssl)
		{
			dev->aio->errnum = QSE_AIO_ESYSERR;
			return -1;
		}

		if (SSL_set_fd (ssl, dev->sck) == 0)
		{
			dev->aio->errnum = QSE_AIO_ESYSERR;
			return -1;
		}

		SSL_set_read_ahead (ssl, 0);

		dev->ssl = ssl;
	}

	watcher_cmd = QSE_AIO_DEV_WATCH_RENEW;
	watcher_events = 0;

	ret = ssl_func ((SSL*)dev->ssl);
	if (ret <= 0)
	{
		int err = SSL_get_error (dev->ssl, ret);
		if (err == SSL_ERROR_WANT_READ)
		{
			/* handshaking isn't complete */
			ret = 0;
		}
		else if (err == SSL_ERROR_WANT_WRITE)
		{
			/* handshaking isn't complete */
			watcher_cmd = QSE_AIO_DEV_WATCH_UPDATE;
			watcher_events = QSE_AIO_DEV_EVENT_IN | QSE_AIO_DEV_EVENT_OUT;
			ret = 0;
		}
		else
		{
			dev->aio->errnum = QSE_AIO_ESYSERR;
			ret = -1;
		}
	}
	else
	{
		ret = 1; /* accepted */
	}

	if (qse_aio_dev_watch ((qse_aio_dev_t*)dev, watcher_cmd, watcher_events) <= -1)
	{
		qse_aio_stop (dev->aio, QSE_AIO_STOPREQ_WATCHER_ERROR);
		ret = -1;
	}

	return ret;
}

static QSE_INLINE int connect_ssl (qse_aio_dev_sck_t* dev)
{
	return do_ssl (dev, SSL_connect);
}

static QSE_INLINE int accept_ssl (qse_aio_dev_sck_t* dev)
{
	return do_ssl (dev, SSL_accept);
}
#endif

static int dev_sck_ioctl (qse_aio_dev_t* dev, int cmd, void* arg)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;

	switch (cmd)
	{
		case QSE_AIO_DEV_SCK_BIND:
		{
			qse_aio_dev_sck_bind_t* bnd = (qse_aio_dev_sck_bind_t*)arg;
			struct sockaddr* sa = (struct sockaddr*)&bnd->localaddr;
			qse_aio_scklen_t sl;
			qse_aio_sckfam_t fam;
			int x;
		#if defined(USE_SSL)
			SSL_CTX* ssl_ctx = QSE_NULL;
		#endif
			if (QSE_AIO_DEV_SCK_GET_PROGRESS(rdev))
			{
				/* can't bind again */
				rdev->aio->errnum = QSE_AIO_EPERM;
				return -1;
			}

			if (bnd->options & QSE_AIO_DEV_SCK_BIND_BROADCAST)
			{
				int v = 1;
				if (setsockopt (rdev->sck, SOL_SOCKET, SO_BROADCAST, &v, QSE_SIZEOF(v)) == -1)
				{
					rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
					return -1;
				}
			}

			if (bnd->options & QSE_AIO_DEV_SCK_BIND_REUSEADDR)
			{
			#if defined(SO_REUSEADDR)
				int v = 1;
				if (setsockopt (rdev->sck, SOL_SOCKET, SO_REUSEADDR, &v, QSE_SIZEOF(v)) == -1)
				{
					rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
					return -1;
				}
			#else
				rdev->aio->errnum = QSE_AIO_ENOIMPL;
				return -1;
			#endif
			}

			if (bnd->options & QSE_AIO_DEV_SCK_BIND_REUSEPORT)
			{
			#if defined(SO_REUSEPORT)
				int v = 1;
				if (setsockopt (rdev->sck, SOL_SOCKET, SO_REUSEPORT, &v, QSE_SIZEOF(v)) == -1)
				{
					rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
					return -1;
				}
			#else
				rdev->aio->errnum = QSE_AIO_ENOIMPL;
				return -1;
			#endif
			}

			if (bnd->options & QSE_AIO_DEV_SCK_BIND_TRANSPARENT)
			{
			#if defined(IP_TRANSPARENT)
				int v = 1;
				if (setsockopt (rdev->sck, SOL_IP, IP_TRANSPARENT, &v, QSE_SIZEOF(v)) == -1)
				{
					rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
					return -1;
				}
			#else
				rdev->aio->errnum = QSE_AIO_ENOIMPL;
				return -1;
			#endif
			}

		#if defined(USE_SSL)
			if (rdev->ssl_ctx)
			{
			#if defined(USE_SSL)
				SSL_CTX_free (rdev->ssl_ctx);
			#endif
				rdev->ssl_ctx = QSE_NULL;

				if (rdev->ssl)
				{
			#if defined(USE_SSL)
					SSL_free (rdev->ssl);
			#endif
					rdev->ssl = QSE_NULL;
				}
			}
		#endif

			if (bnd->options & QSE_AIO_DEV_SCK_BIND_SSL)
			{
			#if defined(USE_SSL)
				if (!bnd->ssl_certfile || !bnd->ssl_keyfile)
				{
					rdev->aio->errnum = QSE_AIO_EINVAL;
					return -1;
				}

				ssl_ctx = SSL_CTX_new (SSLv23_server_method());
				if (!ssl_ctx)
				{
					rdev->aio->errnum = QSE_AIO_ESYSERR;
					return -1;
				}

				if (SSL_CTX_use_certificate_file (ssl_ctx, bnd->ssl_certfile, SSL_FILETYPE_PEM) == 0 ||
				    SSL_CTX_use_PrivateKey_file (ssl_ctx, bnd->ssl_keyfile, SSL_FILETYPE_PEM) == 0 ||
				    SSL_CTX_check_private_key (ssl_ctx) == 0  /*||
				    SSL_CTX_use_certificate_chain_file (ssl_ctx, bnd->chainfile) == 0*/)
				{
					SSL_CTX_free (ssl_ctx);
					rdev->aio->errnum = QSE_AIO_ESYSERR;
					return -1;
				}

				SSL_CTX_set_read_ahead (ssl_ctx, 0);
				SSL_CTX_set_mode (ssl_ctx, SSL_CTX_get_mode(ssl_ctx) | 
				                           /*SSL_MODE_ENABLE_PARTIAL_WRITE |*/
				                           SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

				SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2); /* no outdated SSLv2 by default */

				rdev->tmout = bnd->accept_tmout;
			#else
				rdev->aio->errnum = QSE_AIO_ENOIMPL;
				return -1;
			#endif
			}

			if (qse_aio_getsckaddrinfo (dev->aio, &bnd->localaddr, &sl, &fam) <= -1) return -1;

			x = bind (rdev->sck, sa, sl);
			if (x == -1)
			{
				rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
			#if defined(USE_SSL)
				if (ssl_ctx) SSL_CTX_free (ssl_ctx);
			#endif
				return -1;
			}

			rdev->localaddr = bnd->localaddr;

		#if defined(USE_SSL)
			rdev->ssl_ctx = ssl_ctx;
		#endif

			return 0;
		}

		case QSE_AIO_DEV_SCK_CONNECT:
		{
			qse_aio_dev_sck_connect_t* conn = (qse_aio_dev_sck_connect_t*)arg;
			struct sockaddr* sa = (struct sockaddr*)&conn->remoteaddr;
			qse_aio_scklen_t sl;
			qse_aio_sckaddr_t localaddr;
			int x;
		#if defined(USE_SSL)
			SSL_CTX* ssl_ctx = QSE_NULL;
		#endif

			if (QSE_AIO_DEV_SCK_GET_PROGRESS(rdev))
			{
				/* can't connect again */
				rdev->aio->errnum = QSE_AIO_EPERM;
				return -1;
			}

			if (!IS_STATEFUL(rdev)) 
			{
				dev->aio->errnum = QSE_AIO_ENOCAPA;
				return -1;
			}

			if (sa->sa_family == AF_INET) sl = QSE_SIZEOF(struct sockaddr_in);
			else if (sa->sa_family == AF_INET6) sl = QSE_SIZEOF(struct sockaddr_in6);
			else 
			{
				dev->aio->errnum = QSE_AIO_EINVAL;
				return -1;
			}

		#if defined(USE_SSL)
			if (rdev->ssl_ctx)
			{
				if (rdev->ssl)
				{
					SSL_free (rdev->ssl);
					rdev->ssl = QSE_NULL;
				}

				SSL_CTX_free (rdev->ssl_ctx);
				rdev->ssl_ctx = QSE_NULL;
			}

			if (conn->options & QSE_AIO_DEV_SCK_CONNECT_SSL)
			{
				ssl_ctx = SSL_CTX_new(SSLv23_client_method());
				if (!ssl_ctx)
				{
					rdev->aio->errnum = QSE_AIO_ESYSERR;
					return -1;
				}

				SSL_CTX_set_read_ahead (ssl_ctx, 0);
				SSL_CTX_set_mode (ssl_ctx, SSL_CTX_get_mode(ssl_ctx) | 
				                           /* SSL_MODE_ENABLE_PARTIAL_WRITE | */
				                           SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
			}
		#endif
/*{
int flags = fcntl (rdev->sck, F_GETFL);
fcntl (rdev->sck, F_SETFL, flags & ~O_NONBLOCK);
}*/

			/* the socket is already non-blocking */
			x = connect (rdev->sck, sa, sl);
/*{
int flags = fcntl (rdev->sck, F_GETFL);
fcntl (rdev->sck, F_SETFL, flags | O_NONBLOCK);
}*/
			if (x == -1)
			{
				if (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN)
				{
					if (qse_aio_dev_watch ((qse_aio_dev_t*)rdev, QSE_AIO_DEV_WATCH_UPDATE, QSE_AIO_DEV_EVENT_IN | QSE_AIO_DEV_EVENT_OUT) <= -1)
					{
						/* watcher update failure. it's critical */
						qse_aio_stop (rdev->aio, QSE_AIO_STOPREQ_WATCHER_ERROR);
						goto oops_connect;
					}
					else
					{
						qse_inittime (&rdev->tmout, 0, 0); /* just in case */

						if (qse_ispostime(&conn->connect_tmout))
						{
							if (schedule_timer_job_after (rdev, &conn->connect_tmout, connect_timedout) <= -1) 
							{
								goto oops_connect;
							}
							else
							{
								/* update rdev->tmout to the deadline of the connect timeout job */
								QSE_ASSERT (rdev->tmrjob_index != QSE_AIO_TMRIDX_INVALID);
								qse_aio_gettmrjobdeadline (rdev->aio, rdev->tmrjob_index, &rdev->tmout);
							}
						}

						rdev->remoteaddr = conn->remoteaddr;
						rdev->on_connect = conn->on_connect;
					#if defined(USE_SSL)
						rdev->ssl_ctx = ssl_ctx;
					#endif
						QSE_AIO_DEV_SCK_SET_PROGRESS (rdev, QSE_AIO_DEV_SCK_CONNECTING);
						return 0;
					}
				}

				rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);

			oops_connect:
				if (qse_aio_dev_watch ((qse_aio_dev_t*)rdev, QSE_AIO_DEV_WATCH_UPDATE, QSE_AIO_DEV_EVENT_IN) <= -1)
				{
					/* watcher update failure. it's critical */
					qse_aio_stop (rdev->aio, QSE_AIO_STOPREQ_WATCHER_ERROR);
				}

			#if defined(USE_SSL)
				if (ssl_ctx) SSL_CTX_free (ssl_ctx);
			#endif
				return -1;
			}
			else
			{
				/* connected immediately */
				rdev->remoteaddr = conn->remoteaddr;
				rdev->on_connect = conn->on_connect;

				sl = QSE_SIZEOF(localaddr);
				if (getsockname (rdev->sck, (struct sockaddr*)&localaddr, &sl) == 0) rdev->localaddr = localaddr;

			#if defined(USE_SSL)
				if (ssl_ctx)
				{
					int x;
					rdev->ssl_ctx = ssl_ctx;

					x = connect_ssl (rdev);
					if (x <= -1) 
					{
						SSL_CTX_free (rdev->ssl_ctx);
						rdev->ssl_ctx = QSE_NULL;

						QSE_ASSERT (rdev->ssl == QSE_NULL);
						return -1;
					}
					if (x == 0) 
					{
						QSE_ASSERT (rdev->tmrjob_index == QSE_AIO_TMRIDX_INVALID);
						qse_inittime (&rdev->tmout, 0, 0); /* just in case */

						/* it's ok to use conn->connect_tmout for ssl-connect as
						 * the underlying socket connection has been established immediately */
						if (qse_ispostime(&conn->connect_tmout))
						{
							if (schedule_timer_job_after (rdev, &conn->connect_tmout, ssl_connect_timedout) <= -1) 
							{
								/* no device halting in spite of failure.
								 * let the caller handle this after having 
								 * checked the return code as it is an IOCTL call. */
								SSL_CTX_free (rdev->ssl_ctx);
								rdev->ssl_ctx = QSE_NULL;

								QSE_ASSERT (rdev->ssl == QSE_NULL);
								return -1;
							}
							else
							{
								/* update rdev->tmout to the deadline of the connect timeout job */
								QSE_ASSERT (rdev->tmrjob_index != QSE_AIO_TMRIDX_INVALID);
								qse_aio_gettmrjobdeadline (rdev->aio, rdev->tmrjob_index, &rdev->tmout);
							}
						}

						QSE_AIO_DEV_SCK_SET_PROGRESS (rdev, QSE_AIO_DEV_SCK_CONNECTING_SSL);
					}
					else 
					{
						goto ssl_connected;
					}
				}
				else
				{
				ssl_connected:
			#endif
					QSE_AIO_DEV_SCK_SET_PROGRESS (rdev, QSE_AIO_DEV_SCK_CONNECTED);
					if (rdev->on_connect (rdev) <= -1) return -1;
			#if defined(USE_SSL)
				}
			#endif
				return 0;
			}
		}

		case QSE_AIO_DEV_SCK_LISTEN:
		{
			qse_aio_dev_sck_listen_t* lstn = (qse_aio_dev_sck_listen_t*)arg;
			int x;

			if (QSE_AIO_DEV_SCK_GET_PROGRESS(rdev))
			{
				/* can't listen again */
				rdev->aio->errnum = QSE_AIO_EPERM;
				return -1;
			}

			if (!IS_STATEFUL(rdev)) 
			{
				dev->aio->errnum = QSE_AIO_ENOCAPA;
				return -1;
			}

			x = listen (rdev->sck, lstn->backlogs);
			if (x == -1) 
			{
				rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
				return -1;
			}

			QSE_AIO_DEV_SCK_SET_PROGRESS (rdev, QSE_AIO_DEV_SCK_LISTENING);
			rdev->on_connect = lstn->on_connect;
			return 0;
		}
	}

	return 0;
}

static qse_aio_dev_mth_t dev_sck_methods_stateless = 
{
	dev_sck_make,
	dev_sck_kill,
	dev_sck_getsyshnd,

	dev_sck_read_stateless,
	dev_sck_write_stateless,
	dev_sck_ioctl,     /* ioctl */
};


static qse_aio_dev_mth_t dev_sck_methods_stateful = 
{
	dev_sck_make,
	dev_sck_kill,
	dev_sck_getsyshnd,

	dev_sck_read_stateful,
	dev_sck_write_stateful,
	dev_sck_ioctl,     /* ioctl */
};

static qse_aio_dev_mth_t dev_mth_clisck =
{
	dev_sck_make_client,
	dev_sck_kill,
	dev_sck_getsyshnd,

	dev_sck_read_stateful,
	dev_sck_write_stateful,
	dev_sck_ioctl
};
/* ========================================================================= */

static int harvest_outgoing_connection (qse_aio_dev_sck_t* rdev)
{
	int errcode;
	qse_aio_scklen_t len;

	QSE_ASSERT (!(rdev->state & QSE_AIO_DEV_SCK_CONNECTED));

	len = QSE_SIZEOF(errcode);
	if (getsockopt (rdev->sck, SOL_SOCKET, SO_ERROR, (char*)&errcode, &len) == -1)
	{
		rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}
	else if (errcode == 0)
	{
		qse_aio_sckaddr_t localaddr;
		qse_aio_scklen_t addrlen;

		/* connected */

		if (rdev->tmrjob_index != QSE_AIO_TMRIDX_INVALID)
		{
			qse_aio_deltmrjob (rdev->aio, rdev->tmrjob_index);
			QSE_ASSERT (rdev->tmrjob_index == QSE_AIO_TMRIDX_INVALID);
		}

		addrlen = QSE_SIZEOF(localaddr);
		if (getsockname (rdev->sck, (struct sockaddr*)&localaddr, &addrlen) == 0) rdev->localaddr = localaddr;

		if (qse_aio_dev_watch ((qse_aio_dev_t*)rdev, QSE_AIO_DEV_WATCH_RENEW, 0) <= -1) 
		{
			/* watcher update failure. it's critical */
			qse_aio_stop (rdev->aio, QSE_AIO_STOPREQ_WATCHER_ERROR);
			return -1;
		}

	#if defined(USE_SSL)
		if (rdev->ssl_ctx)
		{
			int x;
			QSE_ASSERT (!rdev->ssl); /* must not be SSL-connected yet */

			x = connect_ssl (rdev);
			if (x <= -1) return -1;
			if (x == 0)
			{
				/* underlying socket connected but not SSL-connected */
				QSE_AIO_DEV_SCK_SET_PROGRESS (rdev, QSE_AIO_DEV_SCK_CONNECTING_SSL);

				QSE_ASSERT (rdev->tmrjob_index == QSE_AIO_TMRIDX_INVALID);

				/* rdev->tmout has been set to the deadline of the connect task
				 * when the CONNECT IOCTL command has been executed. use the 
				 * same deadline here */
				if (qse_ispostime(&rdev->tmout) &&
				    schedule_timer_job_at (rdev, &rdev->tmout, ssl_connect_timedout) <= -1)
				{
					qse_aio_dev_halt ((qse_aio_dev_t*)rdev);
				}

				return 0;
			}
			else
			{
				goto ssl_connected;
			}
		}
		else
		{
		ssl_connected:
	#endif
			QSE_AIO_DEV_SCK_SET_PROGRESS (rdev, QSE_AIO_DEV_SCK_CONNECTED);
			if (rdev->on_connect (rdev) <= -1) return -1;
	#if defined(USE_SSL)
		}
	#endif

		return 0;
	}
	else if (errcode == EINPROGRESS || errcode == EWOULDBLOCK)
	{
		/* still in progress */
		return 0;
	}
	else
	{
		rdev->aio->errnum = qse_aio_syserrtoerrnum(errcode);
		return -1;
	}
}

static int accept_incoming_connection (qse_aio_dev_sck_t* rdev)
{
	qse_aio_sckhnd_t clisck;
	qse_aio_sckaddr_t remoteaddr;
	qse_aio_scklen_t addrlen;
	qse_aio_dev_sck_t* clidev;

	/* this is a server(lisening) socket */

	addrlen = QSE_SIZEOF(remoteaddr);
	clisck = accept (rdev->sck, (struct sockaddr*)&remoteaddr, &addrlen);
	if (clisck == QSE_AIO_SCKHND_INVALID)
	{
		if (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN) return 0;
		if (errno == EINTR) return 0; /* if interrupted by a signal, treat it as if it's EINPROGRESS */

		rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}

	/* use rdev->dev_size when instantiating a client sck device
	 * instead of QSE_SIZEOF(qse_aio_dev_sck_t). therefore, the 
	 * extension area as big as that of the master sck device
	 * is created in the client sck device */
	clidev = (qse_aio_dev_sck_t*)qse_aio_makedev (rdev->aio, rdev->dev_size, &dev_mth_clisck, rdev->dev_evcb, &clisck); 
	if (!clidev) 
	{
		close (clisck);
		return -1;
	}

	QSE_ASSERT (clidev->sck == clisck);

	clidev->dev_capa |= QSE_AIO_DEV_CAPA_IN | QSE_AIO_DEV_CAPA_OUT | QSE_AIO_DEV_CAPA_STREAM | QSE_AIO_DEV_CAPA_OUT_QUEUED;
	clidev->remoteaddr = remoteaddr;

	addrlen = QSE_SIZEOF(clidev->localaddr);
	if (getsockname(clisck, (struct sockaddr*)&clidev->localaddr, &addrlen) == -1) clidev->localaddr = rdev->localaddr;

#if defined(SO_ORIGINAL_DST)
	/* if REDIRECT is used, SO_ORIGINAL_DST returns the original
	 * destination address. When REDIRECT is not used, it returnes
	 * the address of the local socket. In this case, it should
	 * be same as the result of getsockname(). */
	addrlen = QSE_SIZEOF(clidev->orgdstaddr);
	if (getsockopt (clisck, SOL_IP, SO_ORIGINAL_DST, &clidev->orgdstaddr, &addrlen) == -1) clidev->orgdstaddr = rdev->localaddr;
#else
	clidev->orgdstaddr = rdev->localaddr;
#endif

	if (!qse_aio_equalsckaddrs (rdev->aio, &clidev->orgdstaddr, &clidev->localaddr))
	{
		clidev->state |= QSE_AIO_DEV_SCK_INTERCEPTED;
	}
	else if (qse_aio_getsckaddrport (&clidev->localaddr) != qse_aio_getsckaddrport(&rdev->localaddr))
	{
		/* When TPROXY is used, getsockname() and SO_ORIGNAL_DST return
		 * the same addresses. however, the port number may be different
		 * as a typical TPROXY rule is set to change the port number.
		 * However, this check is fragile if the server port number is
		 * set to 0.
		 *
		 * Take note that the above assumption gets wrong if the TPROXY
		 * rule doesn't change the port number. so it won't be able
		 * to handle such a TPROXYed packet without port transformation. */
		clidev->state |= QSE_AIO_DEV_SCK_INTERCEPTED;
	}
	#if 0
	else if ((clidev->initial_ifindex = resolve_ifindex (fd, clidev->localaddr)) <= -1)
	{
		/* the local_address is not one of a local address.
		 * it's probably proxied. */
		clidev->state |= QSE_AIO_DEV_SCK_INTERCEPTED;
	}
	#endif

	/* inherit some event handlers from the parent.
	 * you can still change them inside the on_connect handler */
	clidev->on_connect = rdev->on_connect;
	clidev->on_disconnect = rdev->on_disconnect; 
	clidev->on_write = rdev->on_write;
	clidev->on_read = rdev->on_read;

	QSE_ASSERT (clidev->tmrjob_index == QSE_AIO_TMRIDX_INVALID);

	if (rdev->ssl_ctx)
	{
		QSE_AIO_DEV_SCK_SET_PROGRESS (clidev, QSE_AIO_DEV_SCK_ACCEPTING_SSL);
		QSE_ASSERT (clidev->state & QSE_AIO_DEV_SCK_ACCEPTING_SSL);
		/* actual SSL acceptance must be completed in the client device */

		/* let the client device know the SSL context to use */
		clidev->ssl_ctx = rdev->ssl_ctx;

		if (qse_ispostime(&rdev->tmout) &&
		    schedule_timer_job_after (clidev, &rdev->tmout, ssl_accept_timedout) <= -1)
		{
			/* TODO: call a warning/error callback */
			/* timer job scheduling failed. halt the device */
			qse_aio_dev_halt ((qse_aio_dev_t*)clidev);
		}
	}
	else
	{
		QSE_AIO_DEV_SCK_SET_PROGRESS (clidev, QSE_AIO_DEV_SCK_ACCEPTED);
		if (clidev->on_connect(clidev) <= -1) qse_aio_dev_sck_halt (clidev);
	}

	return 0;
}

static int dev_evcb_sck_ready_stateful (qse_aio_dev_t* dev, int events)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;

	if (events & QSE_AIO_DEV_EVENT_ERR)
	{
		int errcode;
		qse_aio_scklen_t len;

		len = QSE_SIZEOF(errcode);
		if (getsockopt (rdev->sck, SOL_SOCKET, SO_ERROR, (char*)&errcode, &len) == -1)
		{
			/* the error number is set to the socket error code.
			 * errno resulting from getsockopt() doesn't reflect the actual
			 * socket error. so errno is not used to set the error number.
			 * instead, the generic device error QSE_AIO_EDEVERRR is used */
			rdev->aio->errnum = QSE_AIO_EDEVERR;
		}
		else
		{
			rdev->aio->errnum = qse_aio_syserrtoerrnum (errcode);
		}
		return -1;
	}

	/* this socket can connect */
	switch (QSE_AIO_DEV_SCK_GET_PROGRESS(rdev))
	{
		case QSE_AIO_DEV_SCK_CONNECTING:
			if (events & QSE_AIO_DEV_EVENT_HUP)
			{
				/* device hang-up */
				rdev->aio->errnum = QSE_AIO_EDEVHUP;
				return -1;
			}
			else if (events & (QSE_AIO_DEV_EVENT_PRI | QSE_AIO_DEV_EVENT_IN))
			{
				/* invalid event masks. generic device error */
				rdev->aio->errnum = QSE_AIO_EDEVERR;
				return -1;
			}
			else if (events & QSE_AIO_DEV_EVENT_OUT)
			{
				/* when connected, the socket becomes writable */
				return harvest_outgoing_connection (rdev);
			}
			else
			{
				return 0; /* success but don't invoke on_read() */ 
			}

		case QSE_AIO_DEV_SCK_CONNECTING_SSL:
		#if defined(USE_SSL)
			if (events & QSE_AIO_DEV_EVENT_HUP)
			{
				/* device hang-up */
				rdev->aio->errnum = QSE_AIO_EDEVHUP;
				return -1;
			}
			else if (events & QSE_AIO_DEV_EVENT_PRI)
			{
				/* invalid event masks. generic device error */
				rdev->aio->errnum = QSE_AIO_EDEVERR;
				return -1;
			}
			else if (events & (QSE_AIO_DEV_EVENT_IN | QSE_AIO_DEV_EVENT_OUT))
			{
				int x;

				x = connect_ssl (rdev);
				if (x <= -1) return -1;
				if (x == 0) return 0; /* not SSL-Connected */

				if (rdev->tmrjob_index != QSE_AIO_TMRIDX_INVALID)
				{
					qse_aio_deltmrjob (rdev->aio, rdev->tmrjob_index);
					rdev->tmrjob_index = QSE_AIO_TMRIDX_INVALID;
				}

				QSE_AIO_DEV_SCK_SET_PROGRESS (rdev, QSE_AIO_DEV_SCK_CONNECTED);
				if (rdev->on_connect (rdev) <= -1) return -1;
				return 0;
			}
			else
			{
				return 0; /* success. no actual I/O yet */
			}
		#else
			rdev->aio->errnum = QSE_AIO_EINTERN;
			return -1;
		#endif

		case QSE_AIO_DEV_SCK_LISTENING:

			if (events & QSE_AIO_DEV_EVENT_HUP)
			{
				/* device hang-up */
				rdev->aio->errnum = QSE_AIO_EDEVHUP;
				return -1;
			}
			else if (events & (QSE_AIO_DEV_EVENT_PRI | QSE_AIO_DEV_EVENT_OUT))
			{
				rdev->aio->errnum = QSE_AIO_EDEVERR;
				return -1;
			}
			else if (events & QSE_AIO_DEV_EVENT_IN)
			{
				return accept_incoming_connection (rdev);
			}
			else
			{
				return 0; /* success but don't invoke on_read() */ 
			}

		case QSE_AIO_DEV_SCK_ACCEPTING_SSL:
		#if defined(USE_SSL)
			if (events & QSE_AIO_DEV_EVENT_HUP)
			{
				/* device hang-up */
				rdev->aio->errnum = QSE_AIO_EDEVHUP;
				return -1;
			}
			else if (events & QSE_AIO_DEV_EVENT_PRI)
			{
				/* invalid event masks. generic device error */
				rdev->aio->errnum = QSE_AIO_EDEVERR;
				return -1;
			}
			else if (events & (QSE_AIO_DEV_EVENT_IN | QSE_AIO_DEV_EVENT_OUT))
			{
				int x;
				x = accept_ssl (rdev);
				if (x <= -1) return -1;
				if (x <= 0) return 0; /* not SSL-accepted yet */

				if (rdev->tmrjob_index != QSE_AIO_TMRIDX_INVALID)
				{
					qse_aio_deltmrjob (rdev->aio, rdev->tmrjob_index);
					rdev->tmrjob_index = QSE_AIO_TMRIDX_INVALID;
				}

				QSE_AIO_DEV_SCK_SET_PROGRESS (rdev, QSE_AIO_DEV_SCK_ACCEPTED);
				if (rdev->on_connect(rdev) <= -1) qse_aio_dev_sck_halt (rdev);

				return 0;
			}
			else
			{
				return 0; /* no reading or writing yet */
			}
		#else
			rdev->aio->errnum = QSE_AIO_EINTERN;
			return -1;
		#endif


		default:
			if (events & QSE_AIO_DEV_EVENT_HUP)
			{
				if (events & (QSE_AIO_DEV_EVENT_PRI | QSE_AIO_DEV_EVENT_IN | QSE_AIO_DEV_EVENT_OUT)) 
				{
					/* probably half-open? */
					return 1;
				}

				rdev->aio->errnum = QSE_AIO_EDEVHUP;
				return -1;
			}

			return 1; /* the device is ok. carry on reading or writing */
	}
}

static int dev_evcb_sck_ready_stateless (qse_aio_dev_t* dev, int events)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;

	if (events & QSE_AIO_DEV_EVENT_ERR)
	{
		int errcode;
		qse_aio_scklen_t len;

		len = QSE_SIZEOF(errcode);
		if (getsockopt (rdev->sck, SOL_SOCKET, SO_ERROR, (char*)&errcode, &len) == -1)
		{
			/* the error number is set to the socket error code.
			 * errno resulting from getsockopt() doesn't reflect the actual
			 * socket error. so errno is not used to set the error number.
			 * instead, the generic device error QSE_AIO_EDEVERRR is used */
			rdev->aio->errnum = QSE_AIO_EDEVERR;
		}
		else
		{
			rdev->aio->errnum = qse_aio_syserrtoerrnum (errcode);
		}
		return -1;
	}
	else if (events & QSE_AIO_DEV_EVENT_HUP)
	{
		rdev->aio->errnum = QSE_AIO_EDEVHUP;
		return -1;
	}

	return 1; /* the device is ok. carry on reading or writing */
}

static int dev_evcb_sck_on_read_stateful (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t dlen, const qse_aio_devaddr_t* srcaddr)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;
	return rdev->on_read (rdev, data, dlen, QSE_NULL);
}

static int dev_evcb_sck_on_write_stateful (qse_aio_dev_t* dev, qse_aio_iolen_t wrlen, void* wrctx, const qse_aio_devaddr_t* dstaddr)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;
	return rdev->on_write (rdev, wrlen, wrctx, QSE_NULL);
}

static int dev_evcb_sck_on_read_stateless (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t dlen, const qse_aio_devaddr_t* srcaddr)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;
	return rdev->on_read (rdev, data, dlen, srcaddr->ptr);
}

static int dev_evcb_sck_on_write_stateless (qse_aio_dev_t* dev, qse_aio_iolen_t wrlen, void* wrctx, const qse_aio_devaddr_t* dstaddr)
{
	qse_aio_dev_sck_t* rdev = (qse_aio_dev_sck_t*)dev;
	return rdev->on_write (rdev, wrlen, wrctx, dstaddr->ptr);
}

static qse_aio_dev_evcb_t dev_sck_event_callbacks_stateful =
{
	dev_evcb_sck_ready_stateful,
	dev_evcb_sck_on_read_stateful,
	dev_evcb_sck_on_write_stateful
};

static qse_aio_dev_evcb_t dev_sck_event_callbacks_stateless =
{
	dev_evcb_sck_ready_stateless,
	dev_evcb_sck_on_read_stateless,
	dev_evcb_sck_on_write_stateless
};

/* ========================================================================= */

qse_aio_dev_sck_t* qse_aio_dev_sck_make (qse_aio_t* aio, qse_size_t xtnsize, const qse_aio_dev_sck_make_t* info)
{
	qse_aio_dev_sck_t* rdev;

	if (info->type < 0 && info->type >= QSE_COUNTOF(sck_type_map))
	{
		aio->errnum = QSE_AIO_EINVAL;
		return QSE_NULL;
	}

	if (sck_type_map[info->type].extra_dev_capa & QSE_AIO_DEV_CAPA_STREAM) /* can't use the IS_STATEFUL() macro yet */
	{
		rdev = (qse_aio_dev_sck_t*)qse_aio_makedev (
			aio, QSE_SIZEOF(qse_aio_dev_sck_t) + xtnsize, 
			&dev_sck_methods_stateful, &dev_sck_event_callbacks_stateful, (void*)info);
	}
	else
	{
		rdev = (qse_aio_dev_sck_t*)qse_aio_makedev (
			aio, QSE_SIZEOF(qse_aio_dev_sck_t) + xtnsize,
			&dev_sck_methods_stateless, &dev_sck_event_callbacks_stateless, (void*)info);
	}

	return rdev;
}

int qse_aio_dev_sck_bind (qse_aio_dev_sck_t* dev, qse_aio_dev_sck_bind_t* info)
{
	return qse_aio_dev_ioctl ((qse_aio_dev_t*)dev, QSE_AIO_DEV_SCK_BIND, info);
}

int qse_aio_dev_sck_connect (qse_aio_dev_sck_t* dev, qse_aio_dev_sck_connect_t* info)
{
	return qse_aio_dev_ioctl ((qse_aio_dev_t*)dev, QSE_AIO_DEV_SCK_CONNECT, info);
}

int qse_aio_dev_sck_listen (qse_aio_dev_sck_t* dev, qse_aio_dev_sck_listen_t* info)
{
	return qse_aio_dev_ioctl ((qse_aio_dev_t*)dev, QSE_AIO_DEV_SCK_LISTEN, info);
}

int qse_aio_dev_sck_write (qse_aio_dev_sck_t* dev, const void* data, qse_aio_iolen_t dlen, void* wrctx, const qse_aio_sckaddr_t* dstaddr)
{
	qse_aio_devaddr_t devaddr;
	return qse_aio_dev_write ((qse_aio_dev_t*)dev, data, dlen, wrctx, sckaddr_to_devaddr(dev, dstaddr, &devaddr));
}

int qse_aio_dev_sck_timedwrite (qse_aio_dev_sck_t* dev, const void* data, qse_aio_iolen_t dlen, const qse_ntime_t* tmout, void* wrctx, const qse_aio_sckaddr_t* dstaddr)
{
	qse_aio_devaddr_t devaddr;
	return qse_aio_dev_timedwrite ((qse_aio_dev_t*)dev, data, dlen, tmout, wrctx, sckaddr_to_devaddr(dev, dstaddr, &devaddr));
}



/* ========================================================================= */

qse_uint16_t qse_aio_checksumip (const void* hdr, qse_size_t len)
{
	qse_uint32_t sum = 0;
	qse_uint16_t *ptr = (qse_uint16_t*)hdr;

	
	while (len > 1)
	{
		sum += *ptr++;
		if (sum & 0x80000000)
		sum = (sum & 0xFFFF) + (sum >> 16);
		len -= 2;
	}
 
	while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);

	return (qse_uint16_t)~sum;
}
