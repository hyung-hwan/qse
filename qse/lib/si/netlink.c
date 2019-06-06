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


#if defined(__linux)

/* copied from musl */

#include <qse/si/netlink.h>
#include <qse/si/sck.h>
#include "../cmn/mem-prv.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

/* linux/netlink.h */

#define NETLINK_ROUTE 0

#define NLM_F_REQUEST	1
#define NLM_F_MULTI	2
#define NLM_F_ACK	4

#define NLM_F_ROOT	0x100
#define NLM_F_MATCH	0x200
#define NLM_F_ATOMIC	0x400
#define NLM_F_DUMP	(NLM_F_ROOT|NLM_F_MATCH)

#define NLMSG_NOOP	0x1
#define NLMSG_ERROR	0x2
#define NLMSG_DONE	0x3
#define NLMSG_OVERRUN	0x4

/* linux/rtnetlink.h */

#define RTM_NEWLINK	16
#define RTM_GETLINK	18
#define RTM_NEWADDR	20
#define RTM_GETADDR	22

struct rtattr 
{
	unsigned short int rta_len;
	unsigned short int rta_type;
};

struct rtgenmsg 
{
	unsigned char rtgen_family;
};

struct ifinfomsg 
{
	unsigned char ifi_family;
	unsigned char __ifi_pad;
	unsigned short int ifi_type;
	int ifi_index;
	unsigned int ifi_flags;
	unsigned int ifi_change;
};

/* linux/if_link.h */

#define IFLA_ADDRESS	1
#define IFLA_BROADCAST	2
#define IFLA_IFNAME	3
#define IFLA_STATS	7

/* linux/if_addr.h */

struct ifaddrmsg 
{
	qse_uint8_t ifa_family;
	qse_uint8_t ifa_prefixlen;
	qse_uint8_t ifa_flags;
	qse_uint8_t ifa_scope;
	qse_uint32_t ifa_index;
};

#define IFA_ADDRESS   1
#define IFA_LOCAL     2
#define IFA_LABEL     3
#define IFA_BROADCAST 4

/* musl */

#define NETLINK_ALIGN(len)	(((len)+3) & ~3)
#define NLMSG_DATA(nlh)		((void*)((char*)(nlh)+QSE_SIZEOF(struct qse_nlmsg_hdr_t)))
#define NLMSG_DATALEN(nlh)	((nlh)->nlmsg_len-QSE_SIZEOF(struct qse_nlmsg_hdr_t))
#define NLMSG_DATAEND(nlh)	((char*)(nlh)+(nlh)->nlmsg_len)
#define NLMSG_NEXT(nlh)		(struct qse_nlmsg_hdr_t*)((char*)(nlh)+NETLINK_ALIGN((nlh)->nlmsg_len))
#define NLMSG_OK(nlh,end)	((char*)(end)-(char*)(nlh) >= QSE_SIZEOF(struct qse_nlmsg_hdr_t))

#define RTA_DATA(rta)		((void*)((char*)(rta)+QSE_SIZEOF(struct rtattr)))
#define RTA_DATALEN(rta)	((rta)->rta_len-QSE_SIZEOF(struct rtattr))
#define RTA_DATAEND(rta)	((char*)(rta)+(rta)->rta_len)
#define RTA_NEXT(rta)		(struct rtattr*)((char*)(rta)+NETLINK_ALIGN((rta)->rta_len))
#define RTA_OK(nlh,end)		((char*)(end)-(char*)(rta) >= QSE_SIZEOF(struct rtattr))

#define NLMSG_RTA(nlh,len)	((void*)((char*)(nlh)+QSE_SIZEOF(struct qse_nlmsg_hdr_t)+NETLINK_ALIGN(len)))
#define NLMSG_RTAOK(rta,nlh)	RTA_OK(rta,NLMSG_DATAEND(nlh))

static int enumerate_netlink(int fd, unsigned int seq, int type, int af, qse_nlenum_cb_t cb, void *ctx)
{
	struct qse_nlmsg_hdr_t *h;
	union 
	{
		qse_uint8_t buf[8192]; /* TODO: is this large enough? */
		struct 
		{
			struct qse_nlmsg_hdr_t nlh;
			struct rtgenmsg g;
		} req;
		struct qse_nlmsg_hdr_t reply;
	} u;
	int r;

	QSE_MEMSET(&u.req, 0, QSE_SIZEOF(u.req));
	u.req.nlh.nlmsg_len = QSE_SIZEOF(u.req);
	u.req.nlh.nlmsg_type = type;
	u.req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
	u.req.nlh.nlmsg_seq = seq;
	u.req.g.rtgen_family = af;
	r = send(fd, &u.req, QSE_SIZEOF(u.req), 0);
	if (r == -1) return -1;

	while (1) 
	{
		r = recv(fd, u.buf, QSE_SIZEOF(u.buf), MSG_DONTWAIT);
		if (r <= 0) return -1;

		for (h = &u.reply; NLMSG_OK(h, (void*)&u.buf[r]); h = NLMSG_NEXT(h)) 
		{
			if (h->nlmsg_type == NLMSG_DONE) return 0;
			if (h->nlmsg_type == NLMSG_ERROR) return -1;
			if (cb(h, ctx) <= -1) return -1;
		}
	}

	return 0;
}

int qse_nlenum_route (int link_af, int addr_af, qse_nlenum_cb_t cb, void *ctx)
{
	int fd, rc;

#if defined(SOCK_CLOEXEC)
	fd = socket(PF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
	if (fd <= -1) return -1;
#else
	fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd <= -1) return -1;
	else
	{
	#if defined(FD_CLOEXEC)
		int flag = fcntl(fd, F_GETFD);
		if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);
	#endif
	}
#endif

	rc = enumerate_netlink(fd, 1, RTM_GETLINK, link_af, cb, ctx);
	if (rc >= 0) rc = enumerate_netlink(fd, 2, RTM_GETADDR, addr_af, cb, ctx);

	qse_close_sck (fd);
	return rc;
}

#if 0
#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <syscall.h>
#include <net/if.h>
#include <netinet/in.h>


#define IFADDRS_HASH_SIZE 64

/* getifaddrs() reports hardware addresses with PF_PACKET that implies
 * struct sockaddr_ll.  But e.g. Infiniband socket address length is
 * longer than sockaddr_ll.ssl_addr[8] can hold. Use this hack struct
 * to extend ssl_addr - callers should be able to still use it. */
struct sockaddr_ll_hack 
{
	unsigned short int sll_family, sll_protocol;
	int sll_ifindex;
	unsigned short int sll_hatype;
	unsigned char sll_pkttype, sll_halen;
	unsigned char sll_addr[24];
};

union sockany 
{
	struct sockaddr sa;
	struct sockaddr_ll_hack ll;
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
};

struct ifaddrs_storage 
{
	struct ifaddrs ifa;
	struct ifaddrs_storage *hash_next;
	union sockany addr, netmask, ifu;
	unsigned int index;
	char name[IFNAMSIZ+1];
};

struct ifaddrs_ctx 
{
	struct ifaddrs_storage* first;
	struct ifaddrs_storage* last;
	struct ifaddrs_storage* hash[IFADDRS_HASH_SIZE];
};

void freeifaddrs(struct ifaddrs *ifp)
{
	struct ifaddrs *n;
	while (ifp) {
		n = ifp->ifa_next;
		free(ifp);
		ifp = n;
	}
}

static void copy_addr(struct sockaddr **r, int af, union sockany *sa, void *addr, size_t addrlen, int ifindex)
{
	qse_uint8_t *dst;
	int len;

	switch (af) {
	case AF_INET:
		dst = (qse_uint8_t*) &sa->v4.sin_addr;
		len = 4;
		break;
	case AF_INET6:
		dst = (qse_uint8_t*) &sa->v6.sin6_addr;
		len = 16;
		if (IN6_IS_ADDR_LINKLOCAL(addr) || IN6_IS_ADDR_MC_LINKLOCAL(addr))
			sa->v6.sin6_scope_id = ifindex;
		break;
	default:
		return;
	}
	if (addrlen < len) return;
	sa->sa.sa_family = af;
	QSE_MEMCPY(dst, addr, len);
	*r = &sa->sa;
}

static void gen_netmask(struct sockaddr **r, int af, union sockany *sa, int prefixlen)
{
	qse_uint8_t addr[16] = {0};
	int i;

	if (prefixlen > 8*sizeof(addr)) prefixlen = 8*sizeof(addr);
	i = prefixlen / 8;
	QSE_MEMSET(addr, 0xff, i);
	if (i < sizeof(addr)) addr[i++] = 0xff << (8 - (prefixlen % 8));
	copy_addr(r, af, sa, addr, sizeof(addr), 0);
}

static void copy_lladdr(struct sockaddr **r, union sockany *sa, void *addr, size_t addrlen, int ifindex, unsigned short int hatype)
{
	if (addrlen > sizeof(sa->ll.sll_addr)) return;
	sa->ll.sll_family = AF_PACKET;
	sa->ll.sll_ifindex = ifindex;
	sa->ll.sll_hatype = hatype;
	sa->ll.sll_halen = addrlen;
	QSE_MEMCPY(sa->ll.sll_addr, addr, addrlen);
	*r = &sa->sa;
}

static int netlink_msg_to_ifaddr (qse_nlmsg_hdr_t* h, void* pctx)
{
	struct ifaddrs_ctx* ctx = pctx;
	struct ifaddrs_storage *ifs, *ifs0;
	struct ifinfomsg *ifi = NLMSG_DATA(h);
	struct ifaddrmsg *ifa = NLMSG_DATA(h);
	struct rtattr *rta;
	int stats_len = 0;

	if (h->nlmsg_type == RTM_NEWLINK) 
	{
		for (rta = NLMSG_RTA(h, sizeof(*ifi)); NLMSG_RTAOK(rta, h); rta = RTA_NEXT(rta)) 
		{
			if (rta->rta_type != IFLA_STATS) continue;
			stats_len = RTA_DATALEN(rta);
			break;
		}
	} 
	else
	{
		for (ifs0 = ctx->hash[ifa->ifa_index % IFADDRS_HASH_SIZE]; ifs0; ifs0 = ifs0->hash_next)
		{
			if (ifs0->index == ifa->ifa_index) break;
		}
		if (!ifs0) return 0;
	}

	ifs = calloc(1, sizeof(struct ifaddrs_storage) + stats_len);
	if (ifs == 0) return -1;

	if (h->nlmsg_type == RTM_NEWLINK)
	{
		ifs->index = ifi->ifi_index;
		ifs->ifa.ifa_flags = ifi->ifi_flags;

		for (rta = NLMSG_RTA(h, sizeof(*ifi)); NLMSG_RTAOK(rta, h); rta = RTA_NEXT(rta))
		{
			switch (rta->rta_type) {
			case IFLA_IFNAME:
				if (RTA_DATALEN(rta) < sizeof(ifs->name)) {
					QSE_MEMCPY(ifs->name, RTA_DATA(rta), RTA_DATALEN(rta));
					ifs->ifa.ifa_name = ifs->name;
				}
				break;
			case IFLA_ADDRESS:
				copy_lladdr(&ifs->ifa.ifa_addr, &ifs->addr, RTA_DATA(rta), RTA_DATALEN(rta), ifi->ifi_index, ifi->ifi_type);
				break;
			case IFLA_BROADCAST:
				copy_lladdr(&ifs->ifa.ifa_broadaddr, &ifs->ifu, RTA_DATA(rta), RTA_DATALEN(rta), ifi->ifi_index, ifi->ifi_type);
				break;
			case IFLA_STATS:
				ifs->ifa.ifa_data = (void*)(ifs+1);
				QSE_MEMCPY(ifs->ifa.ifa_data, RTA_DATA(rta), RTA_DATALEN(rta));
				break;
			}
		}
		if (ifs->ifa.ifa_name) {
			unsigned int bucket = ifs->index % IFADDRS_HASH_SIZE;
			ifs->hash_next = ctx->hash[bucket];
			ctx->hash[bucket] = ifs;
		}
	}
	else 
	{
		ifs->ifa.ifa_name = ifs0->ifa.ifa_name;
		ifs->ifa.ifa_flags = ifs0->ifa.ifa_flags;
		for (rta = NLMSG_RTA(h, sizeof(*ifa)); NLMSG_RTAOK(rta, h); rta = RTA_NEXT(rta)) {
			switch (rta->rta_type) {
			case IFA_ADDRESS:
				/* If ifa_addr is already set we, received an IFA_LOCAL before
				 * so treat this as destination address */
				if (ifs->ifa.ifa_addr)
					copy_addr(&ifs->ifa.ifa_dstaddr, ifa->ifa_family, &ifs->ifu, RTA_DATA(rta), RTA_DATALEN(rta), ifa->ifa_index);
				else
					copy_addr(&ifs->ifa.ifa_addr, ifa->ifa_family, &ifs->addr, RTA_DATA(rta), RTA_DATALEN(rta), ifa->ifa_index);
				break;
			case IFA_BROADCAST:
				copy_addr(&ifs->ifa.ifa_broadaddr, ifa->ifa_family, &ifs->ifu, RTA_DATA(rta), RTA_DATALEN(rta), ifa->ifa_index);
				break;
			case IFA_LOCAL:
				/* If ifa_addr is set and we get IFA_LOCAL, assume we have
				 * a point-to-point network. Move address to correct field. */
				if (ifs->ifa.ifa_addr) {
					ifs->ifu = ifs->addr;
					ifs->ifa.ifa_dstaddr = &ifs->ifu.sa;
					QSE_MEMSET(&ifs->addr, 0, sizeof(ifs->addr));
				}
				copy_addr(&ifs->ifa.ifa_addr, ifa->ifa_family, &ifs->addr, RTA_DATA(rta), RTA_DATALEN(rta), ifa->ifa_index);
				break;
			case IFA_LABEL:
				if (RTA_DATALEN(rta) < sizeof(ifs->name)) {
					QSE_MEMCPY(ifs->name, RTA_DATA(rta), RTA_DATALEN(rta));
					ifs->ifa.ifa_name = ifs->name;
				}
				break;
			}
		}
		if (ifs->ifa.ifa_addr)
			gen_netmask(&ifs->ifa.ifa_netmask, ifa->ifa_family, &ifs->netmask, ifa->ifa_prefixlen);
	}

	if (ifs->ifa.ifa_name) 
	{
		if (!ctx->first) ctx->first = ifs;
		if (ctx->last) ctx->last->ifa.ifa_next = &ifs->ifa;
		ctx->last = ifs;
	} 
	else 
	{
		free(ifs);
	}
	return 0;
}

int getifaddrs (struct ifaddrs **ifap)
{
	struct ifaddrs_ctx _ctx, *ctx = &_ctx;
	int r;
	QSE_MEMSET(ctx, 0, sizeof *ctx);
	r = qse_nlenum_route(AF_UNSPEC, AF_UNSPEC, netlink_msg_to_ifaddr, ctx);
	if (r == 0) *ifap = &ctx->first->ifa;
	else freeifaddrs(&ctx->first->ifa);
	return r;
}
#endif



#endif /* defined(__linux) */
