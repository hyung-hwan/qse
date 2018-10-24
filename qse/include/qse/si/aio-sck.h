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

#ifndef _QSE_SI_AIO_SCK_H_
#define _QSE_SI_AIO_SCK_H_

#include <qse/si/aio.h>

/* ========================================================================= */
/* TOOD: move these to a separte file */

#define QSE_AIO_ETHHDR_PROTO_IP4   0x0800
#define QSE_AIO_ETHHDR_PROTO_ARP   0x0806
#define QSE_AIO_ETHHDR_PROTO_8021Q 0x8100 /* 802.1Q VLAN */
#define QSE_AIO_ETHHDR_PROTO_IP6   0x86DD


#define QSE_AIO_ARPHDR_OPCODE_REQUEST 1
#define QSE_AIO_ARPHDR_OPCODE_REPLY   2

#define QSE_AIO_ARPHDR_HTYPE_ETH 0x0001
#define QSE_AIO_ARPHDR_PTYPE_IP4 0x0800

#define QSE_AIO_ETHADDR_LEN 6
#define QSE_AIO_IP4ADDR_LEN 4
#define QSE_AIO_IP6ADDR_LEN 16 


#if defined(__GNUC__)
#	define QSE_AIO_PACKED __attribute__((__packed__))

#else
#	define QSE_AIO_PACKED 
#	QSE_AIO_PACK_PUSH pack(push)
#	QSE_AIO_PACK_PUSH pack(push)
#	QSE_AIO_PACK(x) pack(x)
#endif


#if defined(__GNUC__)
	/* nothing */
#else
	#pragma pack(push)
	#pragma pack(1)
#endif
struct QSE_AIO_PACKED qse_aio_ethaddr_t
{
	qse_uint8_t v[QSE_AIO_ETHADDR_LEN]; 
};
typedef struct qse_aio_ethaddr_t qse_aio_ethaddr_t;

struct QSE_AIO_PACKED qse_aio_ip4addr_t
{
	qse_uint8_t v[QSE_AIO_IP4ADDR_LEN];
};
typedef struct qse_aio_ip4addr_t qse_aio_ip4addr_t;

struct QSE_AIO_PACKED qse_aio_ip6addr_t
{
	qse_uint8_t v[QSE_AIO_IP6ADDR_LEN]; 
};
typedef struct qse_aio_ip6addr_t qse_aio_ip6addr_t;

struct QSE_AIO_PACKED qse_aio_ethhdr_t
{
	qse_uint8_t  dest[QSE_AIO_ETHADDR_LEN];
	qse_uint8_t  source[QSE_AIO_ETHADDR_LEN];
	qse_uint16_t proto;
};
typedef struct qse_aio_ethhdr_t qse_aio_ethhdr_t;

struct QSE_AIO_PACKED qse_aio_arphdr_t
{
	qse_uint16_t htype;   /* hardware type (ethernet: 0x0001) */
	qse_uint16_t ptype;   /* protocol type (ipv4: 0x0800) */
	qse_uint8_t  hlen;    /* hardware address length (ethernet: 6) */
	qse_uint8_t  plen;    /* protocol address length (ipv4 :4) */
	qse_uint16_t opcode;  /* operation code */
};
typedef struct qse_aio_arphdr_t qse_aio_arphdr_t;

/* arp payload for ipv4 over ethernet */
struct QSE_AIO_PACKED qse_aio_etharp_t
{
	qse_uint8_t sha[QSE_AIO_ETHADDR_LEN];   /* source hardware address */
	qse_uint8_t spa[QSE_AIO_IP4ADDR_LEN];   /* source protocol address */
	qse_uint8_t tha[QSE_AIO_ETHADDR_LEN];   /* target hardware address */
	qse_uint8_t tpa[QSE_AIO_IP4ADDR_LEN];   /* target protocol address */
};
typedef struct qse_aio_etharp_t qse_aio_etharp_t;

struct QSE_AIO_PACKED qse_aio_etharp_pkt_t
{
	qse_aio_ethhdr_t ethhdr;
	qse_aio_arphdr_t arphdr;
	qse_aio_etharp_t arppld;
};
typedef struct qse_aio_etharp_pkt_t qse_aio_etharp_pkt_t;


struct qse_aio_iphdr_t
{
#if defined(QSE_ENDIAN_LITTLE)
	qse_uint8_t ihl:4;
	qse_uint8_t version:4;
#elif defined(QSE_ENDIAN_BIG)
	qse_uint8_t version:4;
	qse_uint8_t ihl:4;
#else
#	UNSUPPORTED ENDIAN
#endif
	qse_int8_t tos;
	qse_int16_t tot_len;
	qse_int16_t id;
	qse_int16_t frag_off;
	qse_int8_t ttl;
	qse_int8_t protocol;
	qse_int16_t check;
	qse_int32_t saddr;
	qse_int32_t daddr;
	/*The options start here. */
};
typedef struct qse_aio_iphdr_t qse_aio_iphdr_t;


struct QSE_AIO_PACKED qse_aio_icmphdr_t 
{
	qse_uint8_t type; /* message type */
	qse_uint8_t code; /* subcode */
	qse_uint16_t checksum;
	union
	{
		struct
		{
			qse_uint16_t id;
			qse_uint16_t seq;
		} echo;

		qse_uint32_t gateway;

		struct
		{
			qse_uint16_t frag_unused;
			qse_uint16_t mtu;
		} frag; /* path mut discovery */
	} u;
};
typedef struct qse_aio_icmphdr_t qse_aio_icmphdr_t;

#if defined(__GNUC__)
	/* nothing */
#else
	#pragma pack(pop)
#endif

/* ICMP types */
#define QSE_AIO_ICMP_ECHO_REPLY        0
#define QSE_AIO_ICMP_UNREACH           3 /* destination unreachable */
#define QSE_AIO_ICMP_SOURCE_QUENCE     4
#define QSE_AIO_ICMP_REDIRECT          5
#define QSE_AIO_ICMP_ECHO_REQUEST      8
#define QSE_AIO_ICMP_TIME_EXCEEDED     11
#define QSE_AIO_ICMP_PARAM_PROBLEM     12
#define QSE_AIO_ICMP_TIMESTAMP_REQUEST 13
#define QSE_AIO_ICMP_TIMESTAMP_REPLY   14
#define QSE_AIO_ICMP_INFO_REQUEST      15
#define QSE_AIO_ICMP_INFO_REPLY        16
#define QSE_AIO_ICMP_ADDR_MASK_REQUEST 17
#define QSE_AIO_ICMP_ADDR_MASK_REPLY   18

/* Subcode for QSE_AIO_ICMP_UNREACH */
#define QSE_AIO_ICMP_UNREACH_NET          0
#define QSE_AIO_ICMP_UNREACH_HOST         1
#define QSE_AIO_ICMP_UNREACH_PROTOCOL     2
#define QSE_AIO_ICMP_UNREACH_PORT         3
#define QSE_AIO_ICMP_UNREACH_FRAG_NEEDED  4

/* Subcode for QSE_AIO_ICMP_REDIRECT */
#define QSE_AIO_ICMP_REDIRECT_NET      0
#define QSE_AIO_ICMP_REDIRECT_HOST     1
#define QSE_AIO_ICMP_REDIRECT_NETTOS   2
#define QSE_AIO_ICMP_REDIRECT_HOSTTOS  3

/* Subcode for QSE_AIO_ICMP_TIME_EXCEEDED */
#define QSE_AIO_ICMP_TIME_EXCEEDED_TTL       0
#define QSE_AIO_ICMP_TIME_EXCEEDED_FRAGTIME  1

/* ========================================================================= */

typedef int qse_aio_sckfam_t;

struct qse_aio_sckaddr_t
{
	qse_aio_sckfam_t family;
	qse_uint8_t data[128]; /* TODO: use the actual sockaddr size */
};
typedef struct qse_aio_sckaddr_t qse_aio_sckaddr_t;

#if (QSE_SIZEOF_SOCKLEN_T == QSE_SIZEOF_INT)
	#if defined(QSE_AIO_SOCKLEN_T_IS_SIGNED)
		typedef int qse_aio_scklen_t;
	#else
		typedef unsigned int qse_aio_scklen_t;
	#endif
#elif (QSE_SIZEOF_SOCKLEN_T == QSE_SIZEOF_LONG)
	#if defined(QSE_AIO_SOCKLEN_T_IS_SIGNED)
		typedef long qse_aio_scklen_t;
	#else
		typedef unsigned long qse_aio_scklen_t;
	#endif
#else
	typedef int qse_aio_scklen_t;
#endif

#if defined(_WIN32)
#	define QSE_AIO_IOCP_KEY 1
	/*
	typedef HANDLE qse_aio_syshnd_t;
	typedef SOCKET qse_aio_sckhnd_t;
#	define QSE_AIO_SCKHND_INVALID (INVALID_SOCKET)
	*/

	typedef qse_uintptr_t qse_aio_sckhnd_t;
#	define QSE_AIO_SCKHND_INVALID (~(qse_aio_sck_hnd_t)0)

#else
	typedef int qse_aio_sckhnd_t;
#	define QSE_AIO_SCKHND_INVALID (-1)
	
#endif


/* ========================================================================= */

enum qse_aio_dev_sck_ioctl_cmd_t
{
	QSE_AIO_DEV_SCK_BIND, 
	QSE_AIO_DEV_SCK_CONNECT,
	QSE_AIO_DEV_SCK_LISTEN
};
typedef enum qse_aio_dev_sck_ioctl_cmd_t qse_aio_dev_sck_ioctl_cmd_t;


#define QSE_AIO_DEV_SCK_SET_PROGRESS(dev,bit) do { \
	(dev)->state &= ~QSE_AIO_DEV_SCK_ALL_PROGRESS_BITS; \
	(dev)->state |= (bit); \
} while(0)

#define QSE_AIO_DEV_SCK_GET_PROGRESS(dev) ((dev)->state & QSE_AIO_DEV_SCK_ALL_PROGRESS_BITS)

enum qse_aio_dev_sck_state_t
{
	/* the following items(progress bits) are mutually exclusive */
	QSE_AIO_DEV_SCK_CONNECTING     = (1 << 0),
	QSE_AIO_DEV_SCK_CONNECTING_SSL = (1 << 1),
	QSE_AIO_DEV_SCK_CONNECTED      = (1 << 2),
	QSE_AIO_DEV_SCK_LISTENING      = (1 << 3),
	QSE_AIO_DEV_SCK_ACCEPTING_SSL  = (1 << 4),
	QSE_AIO_DEV_SCK_ACCEPTED       = (1 << 5),

	/* the following items can be bitwise-ORed with an exclusive item above */
	QSE_AIO_DEV_SCK_INTERCEPTED    = (1 << 15),


	/* convenience bit masks */
	QSE_AIO_DEV_SCK_ALL_PROGRESS_BITS = (QSE_AIO_DEV_SCK_CONNECTING |
	                                  QSE_AIO_DEV_SCK_CONNECTING_SSL |
	                                  QSE_AIO_DEV_SCK_CONNECTED |
	                                  QSE_AIO_DEV_SCK_LISTENING |
	                                  QSE_AIO_DEV_SCK_ACCEPTING_SSL |
	                                  QSE_AIO_DEV_SCK_ACCEPTED)
};
typedef enum qse_aio_dev_sck_state_t qse_aio_dev_sck_state_t;

typedef struct qse_aio_dev_sck_t qse_aio_dev_sck_t;

typedef int (*qse_aio_dev_sck_on_read_t) (
	qse_aio_dev_sck_t*       dev,
	const void*           data,
	qse_aio_iolen_t          dlen,
	const qse_aio_sckaddr_t* srcaddr
);

typedef int (*qse_aio_dev_sck_on_write_t) (
	qse_aio_dev_sck_t*       dev,
	qse_aio_iolen_t          wrlen,
	void*                 wrctx,
	const qse_aio_sckaddr_t* dstaddr
);

typedef void (*qse_aio_dev_sck_on_disconnect_t) (
	qse_aio_dev_sck_t* dev
);

typedef int (*qse_aio_dev_sck_on_connect_t) (
	qse_aio_dev_sck_t* dev
);

enum qse_aio_dev_sck_type_t
{
	QSE_AIO_DEV_SCK_TCP4,
	QSE_AIO_DEV_SCK_TCP6,
	QSE_AIO_DEV_SCK_UPD4,
	QSE_AIO_DEV_SCK_UDP6,

	/* ARP at the ethernet layer */
	QSE_AIO_DEV_SCK_ARP,
	QSE_AIO_DEV_SCK_ARP_DGRAM,

	/* ICMP at the IPv4 layer */
	QSE_AIO_DEV_SCK_ICMP4,

	/* ICMP at the IPv6 layer */
	QSE_AIO_DEV_SCK_ICMP6

#if 0
	QSE_AIO_DEV_SCK_RAW,  /* raw L2-level packet */
#endif
};
typedef enum qse_aio_dev_sck_type_t qse_aio_dev_sck_type_t;

typedef struct qse_aio_dev_sck_make_t qse_aio_dev_sck_make_t;
struct qse_aio_dev_sck_make_t
{
	qse_aio_dev_sck_type_t type;
	qse_aio_dev_sck_on_write_t on_write;
	qse_aio_dev_sck_on_read_t on_read;
	qse_aio_dev_sck_on_disconnect_t on_disconnect;
};

enum qse_aio_dev_sck_bind_option_t
{
	QSE_AIO_DEV_SCK_BIND_BROADCAST   = (1 << 0),
	QSE_AIO_DEV_SCK_BIND_REUSEADDR   = (1 << 1),
	QSE_AIO_DEV_SCK_BIND_REUSEPORT   = (1 << 2),
	QSE_AIO_DEV_SCK_BIND_TRANSPARENT = (1 << 3),

/* TODO: more options --- SO_RCVBUF, SO_SNDBUF, SO_RCVTIMEO, SO_SNDTIMEO, SO_KEEPALIVE */
/*   BINDTODEVICE??? */

	QSE_AIO_DEV_SCK_BIND_SSL         = (1 << 15)
};
typedef enum qse_aio_dev_sck_bind_option_t qse_aio_dev_sck_bind_option_t;

typedef struct qse_aio_dev_sck_bind_t qse_aio_dev_sck_bind_t;
struct qse_aio_dev_sck_bind_t
{
	int options;
	qse_aio_sckaddr_t localaddr;
	/* TODO: add device name for BIND_TO_DEVICE */

	const qse_mchar_t* ssl_certfile;
	const qse_mchar_t* ssl_keyfile;
	qse_ntime_t accept_tmout;
};

enum qse_aio_def_sck_connect_option_t
{
	QSE_AIO_DEV_SCK_CONNECT_SSL = (1 << 15)
};
typedef enum qse_aio_dev_sck_connect_option_t qse_aio_dev_sck_connect_option_t;

typedef struct qse_aio_dev_sck_connect_t qse_aio_dev_sck_connect_t;
struct qse_aio_dev_sck_connect_t
{
	int options;
	qse_aio_sckaddr_t remoteaddr;
	qse_ntime_t connect_tmout;
	qse_aio_dev_sck_on_connect_t on_connect;
};

typedef struct qse_aio_dev_sck_listen_t qse_aio_dev_sck_listen_t;
struct qse_aio_dev_sck_listen_t
{
	int backlogs;
	qse_aio_dev_sck_on_connect_t on_connect; /* optional, but new connections are dropped immediately without this */
};

typedef struct qse_aio_dev_sck_accept_t qse_aio_dev_sck_accept_t;
struct qse_aio_dev_sck_accept_t
{
	qse_aio_syshnd_t  sck;
/* TODO: add timeout */
	qse_aio_sckaddr_t remoteaddr;
};

struct qse_aio_dev_sck_t
{
	QSE_AIO_DEV_HEADERS;

	qse_aio_dev_sck_type_t type;
	qse_aio_sckhnd_t sck;

	int state;

	/* remote peer address for a stateful stream socket. valid if one of the 
	 * followings is set in state:
	 *   QSE_AIO_DEV_TCP_ACCEPTING_SSL
	 *   QSE_AIO_DEV_TCP_ACCEPTED
	 *   QSE_AIO_DEV_TCP_CONNECTED
	 *   QSE_AIO_DEV_TCP_CONNECTING
	 *   QSE_AIO_DEV_TCP_CONNECTING_SSL
	 *
	 * also used as a placeholder to store source address for
	 * a stateless socket */
	qse_aio_sckaddr_t remoteaddr; 

	/* local socket address */
	qse_aio_sckaddr_t localaddr;

	/* original destination address */
	qse_aio_sckaddr_t orgdstaddr;

	qse_aio_dev_sck_on_write_t on_write;
	qse_aio_dev_sck_on_read_t on_read;

	/* return 0 on succes, -1 on failure.
	 * called on a new tcp device for an accepted client or
	 *        on a tcp device conntected to a remote server */
	qse_aio_dev_sck_on_connect_t on_connect;
	qse_aio_dev_sck_on_disconnect_t on_disconnect;

	/* timer job index for handling
	 *  - connect() timeout for a connecting socket.
	 *  - SSL_accept() timeout for a socket accepting SSL */
	qse_aio_tmridx_t tmrjob_index;

	/* connect timeout, ssl-connect timeout, ssl-accept timeout.
	 * it denotes timeout duration under some circumstances
	 * or an absolute expiry time under some other circumstances. */
	qse_ntime_t tmout;

	void* ssl_ctx;
	void* ssl;

};

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT qse_aio_sckhnd_t qse_aio_openasyncsck (
	qse_aio_t* aio,
	int     domain, 
	int     type,
	int     proto
);

QSE_EXPORT void qse_aio_closeasyncsck (
	qse_aio_t*       aio,
	qse_aio_sckhnd_t sck
);

QSE_EXPORT int qse_aio_makesckasync (
	qse_aio_t*       aio,
	qse_aio_sckhnd_t sck
);

QSE_EXPORT int qse_aio_getsckaddrinfo (
	qse_aio_t*                aio,
	const qse_aio_sckaddr_t*  addr,
	qse_aio_scklen_t*         len,
	qse_aio_sckfam_t*         family
);

/*
 * The qse_aio_getsckaddrport() function returns the port number of a socket
 * address in the host byte order. If the address doesn't support the port
 * number, it returns 0.
 */
QSE_EXPORT qse_uint16_t qse_aio_getsckaddrport (
	const qse_aio_sckaddr_t* addr
);

/*
 * The qse_aio_getsckaddrifindex() function returns an interface number.
 * If the address doesn't support the interface number, it returns 0. */
QSE_EXPORT int qse_aio_getsckaddrifindex (
	const qse_aio_sckaddr_t* addr
);


QSE_EXPORT void qse_aio_sckaddr_initforip4 (
	qse_aio_sckaddr_t* sckaddr,
	qse_uint16_t   port,
	qse_aio_ip4addr_t* ip4addr
);

QSE_EXPORT void qse_aio_sckaddr_initforip6 (
	qse_aio_sckaddr_t* sckaddr,
	qse_uint16_t   port,
	qse_aio_ip6addr_t* ip6addr
);

QSE_EXPORT void qse_aio_sckaddr_initforeth (
	qse_aio_sckaddr_t* sckaddr,
	int                ifindex,
	qse_aio_ethaddr_t* ethaddr
);

/* ========================================================================= */

QSE_EXPORT qse_aio_dev_sck_t* qse_aio_dev_sck_make (
	qse_aio_t*                    aio,
	qse_size_t                    xtnsize,
	const qse_aio_dev_sck_make_t* info
);

QSE_EXPORT int qse_aio_dev_sck_bind (
	qse_aio_dev_sck_t*         dev,
	qse_aio_dev_sck_bind_t*    info
);

QSE_EXPORT int qse_aio_dev_sck_connect (
	qse_aio_dev_sck_t*         dev,
	qse_aio_dev_sck_connect_t* info
);

QSE_EXPORT int qse_aio_dev_sck_listen (
	qse_aio_dev_sck_t*         dev,
	qse_aio_dev_sck_listen_t*  info
);

QSE_EXPORT int qse_aio_dev_sck_write (
	qse_aio_dev_sck_t*        dev,
	const void*               data,
	qse_aio_iolen_t           len,
	void*                     wrctx,
	const qse_aio_sckaddr_t*  dstaddr
);

QSE_EXPORT int qse_aio_dev_sck_timedwrite (
	qse_aio_dev_sck_t*        dev,
	const void*               data,
	qse_aio_iolen_t           len,
	const qse_ntime_t*        tmout,
	void*                     wrctx,
	const qse_aio_sckaddr_t*  dstaddr
);

#if defined(QSE_AIO_HAVE_INLINE)

static QSE_INLINE void qse_aio_dev_sck_halt (qse_aio_dev_sck_t* sck)
{
	qse_aio_dev_halt ((qse_aio_dev_t*)sck);
}

static QSE_INLINE int qse_aio_dev_sck_read (qse_aio_dev_sck_t* sck, int enabled)
{
	return qse_aio_dev_read ((qse_aio_dev_t*)sck, enabled);
}

#else

#define qse_aio_dev_sck_halt(sck) qse_aio_dev_halt((qse_aio_dev_t*)sck)
#define qse_aio_dev_sck_read(sck,enabled) qse_aio_dev_read((qse_aio_dev_t*)sck, enabled)

#endif


QSE_EXPORT qse_uint16_t qse_aio_checksumip (
	const void* hdr,
	qse_size_t  len
);


#ifdef __cplusplus
}
#endif




#endif
