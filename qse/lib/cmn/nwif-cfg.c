/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

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

#include <qse/cmn/nwif.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/sio.h>
#include "mem.h"

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h>
/*#	include <iphlpapi.h> */
#elif defined(__OS2__)
#	if defined(TCPV40HDRS)
#		define BSD_SELECT
#	endif
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <sys/ioctl.h>
#	include <nerrno.h>
#	if defined(TCPV40HDRS)
#		define USE_SELECT
#		include <sys/select.h>
#	else
#		include <unistd.h>
#	endif
#elif defined(__DOS__)
	/* TODO: */
#else
#	include "syscall.h"
#	include <sys/socket.h>
#	include <netinet/in.h>
#	if defined(HAVE_SYS_IOCTL_H)
#		include <sys/ioctl.h>
#	endif
#	if defined(HAVE_NET_IF_H)
#		include <net/if.h>
#	endif
#	if defined(HAVE_NET_IF_DL_H)
#		include <net/if_dl.h>
#	endif
#	if defined(HAVE_SYS_SOCKIO_H)
#		include <sys/sockio.h>
#	endif
#	if defined(HAVE_IFADDRS_H)
#		include <ifaddrs.h>
#	endif
#	if defined(HAVE_SYS_SYSCTL_H)
#		include <sys/sysctl.h>
#	endif
#	if defined(HAVE_SYS_STROPTS_H)
#		include <sys/stropts.h> /* stream options */
#	endif
#	if defined(HAVE_SYS_MACSTAT_H)
#		include <sys/macstat.h>
#	endif
#	if defined(HAVE_LINUX_ETHTOOL_H)
#		include <linux/ethtool.h>
#	endif
#	if defined(HAVE_LINUX_SOCKIOS_H)
#		include <linux/sockios.h>
#	endif
#endif

/*

#if defined(HAVE_NET_IF_DL_H)
#	include <net/if_dl.h>
#endif
 #ifndef SIOCGLIFHWADDR
#define SIOCGLIFHWADDR  _IOWR('i', 192, struct lifreq)
 #endif
 
    struct lifreq lif;

    memset(&lif, 0, sizeof(lif));
    strlcpy(lif.lifr_name, ifname, sizeof(lif.lifr_name));

    if (ioctl(sock, SIOCGLIFHWADDR, &lif) != -1) {
        struct sockaddr_dl *sp;
        sp = (struct sockaddr_dl *)&lif.lifr_addr;
        memcpy(buf, &sp->sdl_data[0], sp->sdl_alen);
        return sp->sdl_alen;
    }
*/


#if 0
#if defined(SIOCGLIFCONF) && defined(SIOCGLIFNUM) && \
    defined(HAVE_STRUCT_LIFCONF) && defined(HAVE_STRUCT_LIFREQ)
static int get_nwifs (qse_mmgr_t* mmgr, int s, int f, qse_xptl_t* nwifs)
{
	struct lifnum ifn;
	struct lifconf ifc;
	qse_xptl_t b;
	qse_size_t ifcount;

	ifcount = 0;

	b.ptr = QSE_NULL;
	b.len = 0;

	do
	{
		ifn.lifn_family = f;
		ifn.lifn_flags  = 0;
		if (ioctl (s, SIOCGLIFNUM, &ifn) <= -1) goto oops;

		if (b.ptr)
		{
			/* b.ptr won't be QSE_NULL when retrying */
			if (ifn.lifn_count <= ifcount) break;
		}
		
		/* +1 for extra space to leave empty
		 * if SIOCGLIFCONF returns the same number of
		 * intefaces as SIOCLIFNUM */
		b.len = (ifn.lifn_count + 1) * QSE_SIZEOF(struct lifreq);
		b.ptr = QSE_MMGR_ALLOC (mmgr, b.len);
		if (b.ptr == QSE_NULL) goto oops;

		ifc.lifc_family = f;
		ifc.lifc_flags = 0;
		ifc.lifc_len = b.len;
		ifc.lifc_buf = b.ptr;	

		if (ioctl (s, SIOCGLIFCONF, &ifc) <= -1) goto oops;

		ifcount = ifc.lifc_len / QSE_SIZEOF(struct lifreq);
	}
	while (ifcount > ifn.lifn_count); 
	/* the while condition above is for checking if
	 * the buffer got full. when it's full, there is a chance
	 * that there are more interfaces. */
		
	nwifs->ptr = b.ptr;
	nwifs->len = ifcount;
	return 0;

oops:
	if (b.ptr) QSE_MMGR_FREE (mmgr, b.ptr);
	return -1;
}
#endif
#endif

#if 0
static void free_nwifcfg (qse_mmgr_t* mmgr, qse_nwifcfg_node_t* cfg)
{
	qse_nwifcfg_node_t* cur;

	while (cfg)
	{
		cur = cfg;
		cfg = cur->next;
		if (cur->name) QSE_MMGR_FREE (mmgr, cur->name);
		QSE_MMGR_FREE (mmgr, cur);
	}
}

int qse_getnwifcfg (qse_nwifcfg_t* cfg)
{
#if defined(SIOCGLIFCONF)
	struct lifreq* ifr, ifrbuf;
	qse_size_t i;
	int s, f;
	int s4 = -1;
	int s6 = -1;
	qse_xptl_t nwifs = { QSE_NULL, 0 };
	qse_nwifcfg_node_t* head = QSE_NULL;
	qse_nwifcfg_node_t* tmp;

	QSE_ASSERT (cfg->mmgr != QSE_NULL);

#if defined(AF_INET) || defined(AF_INET6)
	#if defined(AF_INET)
	s4 = socket (AF_INET, SOCK_DGRAM, 0);
	if (s4 <= -1) goto oops;
	#endif

	#if defined(AF_INET6)
	s6 = socket (AF_INET6, SOCK_DGRAM, 0);
	if (s6 <= -1) goto oops;
	#endif
#else
	/* no implementation */
	goto oops;
#endif
	
	if (get_nwifs (cfg->mmgr, s4, AF_UNSPEC, &nwifs) <= -1) goto oops;

	ifr = nwifs.ptr;
	for (i = 0; i < nwifs.len; i++, ifr++)
	{
		f = ifr->lifr_addr.ss_family;

		if (f == AF_INET) s = s4;
		else if (f == AF_INET6) s = s6;
		else continue;

		tmp = QSE_MMGR_ALLOC (cfg->mmgr, QSE_SIZEOF(*tmp));
		if (tmp == QSE_NULL) goto oops;

		QSE_MEMSET (tmp, 0, QSE_SIZEOF(*tmp));
		tmp->next = head;
		head = tmp;

#if defined(QSE_CHAR_IS_MCHAR)
		head->name = qse_mbsdup (ifr->lifr_name, cfg->mmgr);
#else
		head->name = qse_mbstowcsdup (ifr->lifr_name, QSE_NULL, cfg->mmgr);
#endif
		if (head->name == QSE_NULL) goto oops;

		qse_skadtonwad (&ifr->lifr_addr, &head->addr);

		qse_mbsxcpy (ifrbuf.lifr_name, QSE_SIZEOF(ifrbuf.lifr_name), ifr->lifr_name);
		if (ioctl (s, SIOCGLIFFLAGS, &ifrbuf) <= -1) goto oops;
		if (ifrbuf.lifr_flags & IFF_UP) head->flags |= QSE_NWIFCFG_UP;
		if (ifrbuf.lifr_flags & IFF_BROADCAST) 
		{
			if (ioctl (s, SIOCGLIFBRDADDR, &ifrbuf) <= -1) goto oops;
			qse_skadtonwad (&ifrbuf.lifr_addr, &head->bcast);
			head->flags |= QSE_NWIFCFG_BCAST;
		}
		if (ifrbuf.lifr_flags & IFF_POINTOPOINT) 
		{
			if (ioctl (s, SIOCGLIFDSTADDR, &ifrbuf) <= -1) goto oops;
			qse_skadtonwad (&ifrbuf.lifr_addr, &head->ptop);
			head->flags |= QSE_NWIFCFG_PTOP;
		}

		if (ioctl (s, SIOCGLIFINDEX, &ifrbuf) <= -1) goto oops;		
		head->index = ifrbuf.lifr_index;

		if (ioctl (s, SIOCGLIFNETMASK, &ifrbuf) <= -1) goto oops;
		qse_skadtonwad (&ifrbuf.lifr_addr, &head->mask);
	}

	QSE_MMGR_FREE (cfg->mmgr, nwifs.ptr);
	close (s6);
	close (s4);

	cfg->list = head;
	return 0;

oops:
	if (head) free_nwifcfg (cfg->mmgr, head);
	if (nwifs.ptr) QSE_MMGR_FREE (cfg->mmgr, nwifs.ptr);
	if (s6 >= 0) close (s6);
	if (s4 >= 0) close (s4);
	return -1;
#else

	/* TODO  */
	return QSE_NULL;
#endif
}

void qse_freenwifcfg (qse_nwifcfg_t* cfg)
{
	free_nwifcfg (cfg->mmgr, cfg->list);
	cfg->list = QSE_NULL;
}

#endif

#if defined(__linux)
static void read_proc_net_if_inet6 (qse_nwifcfg_t* cfg, struct ifreq* ifr)
{
	/*
     *
	 * # cat /proc/net/if_inet6
	 * 00000000000000000000000000000001 01 80 10 80 lo
	 * +------------------------------+ ++ ++ ++ ++ ++
	 * |                                |  |  |  |  |
	 * 1                                2  3  4  5  6
	 * 
	 * 1. IPv6 address displayed in 32 hexadecimal chars without colons as separator
	 * 2. Netlink device number (interface index) in hexadecimal (see “ip addr” , too)
	 * 3. Prefix length in hexadecimal
	 * 4. Scope value (see kernel source “ include/net/ipv6.h” and “net/ipv6/addrconf.c” for more)
	 * 5. Interface flags (see “include/linux/rtnetlink.h” and “net/ipv6/addrconf.c” for more)
	 * 6. Device name
	 */

	qse_sio_t* sio;
	qse_mchar_t line[128];
	qse_mchar_t* ptr, * ptr2;
	qse_ssize_t len;
	qse_mcstr_t tok[6];
	int count, index;

	/* TODO */

	sio = qse_sio_open (QSE_MMGR_GETDFL(), 0, 
		QSE_T("/proc/net/if_inet6"), QSE_SIO_IGNOREMBWCERR | QSE_SIO_READ); 
	if (sio)
	{
		
		while (1)
		{
			len = qse_sio_getmbs (sio, line, QSE_COUNTOF(line));
			if (len <= 0) break;

			count = 0;
			ptr = line;

			while (ptr && count < 6)
			{
				ptr2 = qse_mbsxtok (ptr, len, QSE_MT(" \t"), &tok[count]);

				len -= ptr2 - ptr;
				ptr = ptr2;
				count++;
			}

			if (count >= 6)
			{
				index = qse_mbsxtoi (tok[1].ptr, tok[1].len, 16);
				if (index == cfg->index)
				{
					int ti;

					if (qse_mbshextobin (tok[0].ptr, tok[0].len, cfg->addr.u.in6.addr.value, QSE_COUNTOF(cfg->addr.u.in6.addr.value)) <= -1) break;

					/* tok[3] is the scope type, not the actual scope. 
					 * i leave this code for reference only.
					cfg->addr.u.in6.scope = qse_mbsxtoi (tok[3].ptr, tok[3].len, 16); */
			

					cfg->addr.type = QSE_NWAD_IN6;

					ti = qse_mbsxtoi (tok[2].ptr, tok[0].len, 16);
					qse_prefixtoip6ad (ti, &cfg->mask.u.in6.addr);

					cfg->mask.type = QSE_NWAD_IN6;
					goto done;
				}
			}
		}

	done:
		qse_sio_close (sio);
	}
}
#endif

static int get_nwifcfg (int s, qse_nwifcfg_t* cfg, struct ifreq* ifr)
{
#if defined(_WIN32)
	return -1;

#elif defined(__OS2__)

	return -1;

#elif defined(__DOS__)

	return -1;


#elif defined(SIOCGLIFADDR) && defined(SIOCGLIFINDEX) && \
    defined(HAVE_STRUCT_LIFCONF) && defined(HAVE_STRUCT_LIFREQ)
	/* opensolaris */
	struct lifreq lifrbuf;
	qse_size_t ml, wl;
	
	QSE_MEMSET (&lifrbuf, 0, QSE_SIZEOF(lifrbuf));

	qse_mbsxcpy (lifrbuf.lifr_name, QSE_SIZEOF(lifrbuf.lifr_name), ifr->ifr_name);

	if (ioctl (s, SIOCGLIFINDEX, &lifrbuf) <= -1) return -1;		
	cfg->index = lifrbuf.lifr_index;

	if (ioctl (s, SIOCGLIFFLAGS, &lifrbuf) <= -1) return -1;
	cfg->flags = 0;
	if (lifrbuf.lifr_flags & IFF_UP) cfg->flags |= QSE_NWIFCFG_UP;
	if (lifrbuf.lifr_flags & IFF_RUNNING) cfg->flags |= QSE_NWIFCFG_RUNNING;
	if (lifrbuf.lifr_flags & IFF_BROADCAST) cfg->flags |= QSE_NWIFCFG_BCAST;
	if (lifrbuf.lifr_flags & IFF_POINTOPOINT) cfg->flags |= QSE_NWIFCFG_PTOP;

	if (ioctl (s, SIOCGLIFMTU, &lifrbuf) <= -1) return -1;
	cfg->mtu = lifrbuf.lifr_mtu;
	
	qse_clearnwad (&cfg->addr, QSE_NWAD_NX);
	qse_clearnwad (&cfg->mask, QSE_NWAD_NX);
	qse_clearnwad (&cfg->bcast, QSE_NWAD_NX);
	qse_clearnwad (&cfg->ptop, QSE_NWAD_NX);
	QSE_MEMSET (cfg->ethw, 0, QSE_SIZEOF(cfg->ethw));

	if (ioctl (s, SIOCGLIFADDR, &lifrbuf) >= 0) 
		qse_skadtonwad (&lifrbuf.lifr_addr, &cfg->addr);

	if (ioctl (s, SIOCGLIFNETMASK, &lifrbuf) >= 0) 
		qse_skadtonwad (&lifrbuf.lifr_addr, &cfg->mask);

	if ((cfg->flags & QSE_NWIFCFG_BCAST) &&
	    ioctl (s, SIOCGLIFBRDADDR, &lifrbuf) >= 0)
	{
		qse_skadtonwad (&lifrbuf.lifr_broadaddr, &cfg->bcast);
	}
	if ((cfg->flags & QSE_NWIFCFG_PTOP) &&
	    ioctl (s, SIOCGLIFDSTADDR, &lifrbuf) >= 0)
	{
		qse_skadtonwad (&lifrbuf.lifr_dstaddr, &cfg->ptop);
	}

	#if defined(SIOCGENADDR)
	{
		if (ioctl (s, SIOCGENADDR, ifr) >= 0 && 
		    QSE_SIZEOF(ifr->ifr_enaddr) >= QSE_SIZEOF(cfg->ethw))
		{
			QSE_MEMCPY (cfg->ethw, ifr->ifr_enaddr, QSE_SIZEOF(cfg->ethw));
		}
		/* TODO: try DLPI if SIOCGENADDR fails... */
	}
	#endif

	return 0;

#elif defined(SIOCGLIFADDR) && defined(HAVE_STRUCT_IF_LADDRREQ) && !defined(SIOCGLIFINDEX)
	/* freebsd */
	qse_size_t ml, wl;
	
	#if defined(SIOCGIFINDEX)
	if (ioctl (s, SIOCGIFINDEX, ifr) <= -1) return -1;
	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
	cfg->index = ifr->ifr_ifindex;
	#else
	cfg->index = ifr->ifr_index;
	#endif
	#else
	cfg->index = 0;
	#endif

	if (ioctl (s, SIOCGIFFLAGS, ifr) <= -1) return -1;
	cfg->flags = 0;
	if (ifr->ifr_flags & IFF_UP) cfg->flags |= QSE_NWIFCFG_UP;
	if (ifr->ifr_flags & IFF_RUNNING) cfg->flags |= QSE_NWIFCFG_RUNNING;
	if (ifr->ifr_flags & IFF_BROADCAST) cfg->flags |= QSE_NWIFCFG_BCAST;
	if (ifr->ifr_flags & IFF_POINTOPOINT) cfg->flags |= QSE_NWIFCFG_PTOP;

	if (ioctl (s, SIOCGIFMTU, ifr) <= -1) return -1;
	#if defined(HAVE_STRUCT_IFREQ_IFR_MTU)
	cfg->mtu = ifr->ifr_mtu;
	#else
	/* well, this is a bit dirty. but since all these are unions, 
	 * the name must not really matter. some OSes just omitts defining
	 * the MTU field */
	cfg->mtu = ifr->ifr_metric; 
	#endif
	
	qse_clearnwad (&cfg->addr, QSE_NWAD_NX);
	qse_clearnwad (&cfg->mask, QSE_NWAD_NX);
	qse_clearnwad (&cfg->bcast, QSE_NWAD_NX);
	qse_clearnwad (&cfg->ptop, QSE_NWAD_NX);
	QSE_MEMSET (cfg->ethw, 0, QSE_SIZEOF(cfg->ethw));

	if (cfg->type == QSE_NWIFCFG_IN6)
	{
		struct if_laddrreq iflrbuf;
		QSE_MEMSET (&iflrbuf, 0, QSE_SIZEOF(iflrbuf));
		qse_mbsxcpy (iflrbuf.iflr_name, QSE_SIZEOF(iflrbuf.iflr_name), ifr->ifr_name);

		if (ioctl (s, SIOCGLIFADDR, &iflrbuf) >= 0) 
		{
			qse_skadtonwad (&iflrbuf.addr, &cfg->addr);

			cfg->mask.type = QSE_NWAD_IN6;
			qse_prefixtoip6ad (iflrbuf.prefixlen, &cfg->mask.u.in6.addr);

			if (cfg->flags & QSE_NWIFCFG_PTOP)
				qse_skadtonwad (&iflrbuf.dstaddr, &cfg->ptop);
		}
	}
	else
	{
		if (ioctl (s, SIOCGIFADDR, ifr) >= 0)
			qse_skadtonwad (&ifr->ifr_addr, &cfg->addr);

		if (ioctl (s, SIOCGIFNETMASK, ifr) >= 0)
			qse_skadtonwad (&ifr->ifr_addr, &cfg->mask);

		if ((cfg->flags & QSE_NWIFCFG_BCAST) &&
		    ioctl (s, SIOCGIFBRDADDR, ifr) >= 0) 
		{
			qse_skadtonwad (&ifr->ifr_broadaddr, &cfg->bcast);
		}

		if ((cfg->flags & QSE_NWIFCFG_PTOP) &&
		    ioctl (s, SIOCGIFDSTADDR, ifr) >= 0) 
		{
			qse_skadtonwad (&ifr->ifr_dstaddr, &cfg->ptop);
		}
	}

	#if defined(CTL_NET) && defined(AF_ROUTE) && defined(AF_LINK)
	{
		int mib[6];
		size_t len;

		mib[0] = CTL_NET;
		mib[1] = AF_ROUTE;
		mib[2] = 0;
		mib[3] = AF_LINK;
		mib[4] = NET_RT_IFLIST;
		mib[5] = cfg->index;
		if (sysctl (mib, QSE_COUNTOF(mib), QSE_NULL, &len, QSE_NULL, 0) >= 0)
		{
			qse_mmgr_t* mmgr = QSE_MMGR_GETDFL();
			void* buf;

			buf = QSE_MMGR_ALLOC (mmgr, len);
			if (buf)
			{
				if (sysctl (mib, QSE_COUNTOF(mib), buf, &len, QSE_NULL, 0) >= 0)
				{
					struct sockaddr_dl* sadl;
					sadl = ((struct if_msghdr*)buf + 1);

					/* i don't really care if it's really ethernet
					 * so long as the data is long enough */
					if (sadl->sdl_alen >= QSE_COUNTOF(cfg->ethw))
						QSE_MEMCPY (cfg->ethw, LLADDR(sadl), QSE_SIZEOF(cfg->ethw));
				}

				QSE_MMGR_FREE (mmgr, buf);
			}
		}
	}
	#endif

	return 0;

#elif defined(SIOCGIFADDR)

	#if defined(SIOCGIFINDEX)
	if (ioctl (s, SIOCGIFINDEX, ifr) <= -1) return -1;

	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
	cfg->index = ifr->ifr_ifindex;
	#else
	cfg->index = ifr->ifr_index;
	#endif

	#else
	cfg->index = 0;
	#endif

	if (ioctl (s, SIOCGIFFLAGS, ifr) <= -1) return -1;
	cfg->flags = 0;
	if (ifr->ifr_flags & IFF_UP) cfg->flags |= QSE_NWIFCFG_UP;
	if (ifr->ifr_flags & IFF_RUNNING) cfg->flags |= QSE_NWIFCFG_RUNNING;
	if (ifr->ifr_flags & IFF_BROADCAST) cfg->flags |= QSE_NWIFCFG_BCAST;
	if (ifr->ifr_flags & IFF_POINTOPOINT) cfg->flags |= QSE_NWIFCFG_PTOP;

	if (ioctl (s, SIOCGIFMTU, ifr) <= -1) return -1;
	#if defined(HAVE_STRUCT_IFREQ_IFR_MTU)
	cfg->mtu = ifr->ifr_mtu;
	#else
	/* well, this is a bit dirty. but since all these are unions, 
	 * the name must not really matter. SCO just omits defining
	 * the MTU field, and uses ifr_metric instead */
	cfg->mtu = ifr->ifr_metric; 
	#endif

	qse_clearnwad (&cfg->addr, QSE_NWAD_NX);
	qse_clearnwad (&cfg->mask, QSE_NWAD_NX);
	qse_clearnwad (&cfg->bcast, QSE_NWAD_NX);
	qse_clearnwad (&cfg->ptop, QSE_NWAD_NX);
	QSE_MEMSET (cfg->ethw, 0, QSE_SIZEOF(cfg->ethw));
	
	if (ioctl (s, SIOCGIFADDR, ifr) >= 0)
		qse_skadtonwad (&ifr->ifr_addr, &cfg->addr);

	if (ioctl (s, SIOCGIFNETMASK, ifr) >= 0)
		qse_skadtonwad (&ifr->ifr_addr, &cfg->mask);

	#if defined(__linux)
	if (cfg->addr.type == QSE_NWAD_NX && cfg->mask.type == QSE_NWAD_NX && cfg->type == QSE_NWIFCFG_IN6)
	{
		/* access /proc/net/if_inet6 */
		read_proc_net_if_inet6 (cfg, ifr);
	}
	#endif

	if ((cfg->flags & QSE_NWIFCFG_BCAST) &&
	    ioctl (s, SIOCGIFBRDADDR, ifr) >= 0)
	{
		qse_skadtonwad (&ifr->ifr_broadaddr, &cfg->bcast);
	}

	if ((cfg->flags & QSE_NWIFCFG_PTOP) &&
	    ioctl (s, SIOCGIFDSTADDR, ifr) >= 0)
	{
		qse_skadtonwad (&ifr->ifr_dstaddr, &cfg->ptop);
	}

	#if defined(SIOCGIFHWADDR)
	if (ioctl (s, SIOCGIFHWADDR, ifr) >= 0)
	{ 
		QSE_MEMCPY (cfg->ethw, ifr->ifr_hwaddr.sa_data, QSE_SIZEOF(cfg->ethw));
	}
	#elif defined(MACIOC_GETADDR)
	{
		/* sco openserver
		 * use the streams interface to get the hardware address. 
		 */
		int strfd;
		/*qse_mchar_t devname[QSE_COUNTOF(ifr->ifr_name) + 5 + 1] = QSE_MT("/dev/");*/
		qse_mchar_t devname[QSE_COUNTOF(ifr->ifr_name) + 5 + 1];

		qse_mbscpy (devname, QSE_MT("/dev/"));
		qse_mbscpy (&devname[5], ifr->ifr_name);
		if ((strfd = QSE_OPEN (devname, O_RDONLY, 0)) >= 0)
		{
			qse_uint8_t buf[QSE_SIZEOF(cfg->ethw)];
			struct strioctl strioc;

			strioc.ic_cmd = MACIOC_GETADDR;
			strioc.ic_timout = -1;
			strioc.ic_len = QSE_SIZEOF (buf);
			strioc.ic_dp = buf;
			if (ioctl (strfd, I_STR, (char *) &strioc) >= 0) 
				QSE_MEMCPY (cfg->ethw, buf, QSE_SIZEOF(cfg->ethw));

			QSE_CLOSE (strfd);
		}
	}
	#endif

	return 0;
#else

	/* TODO  */
	return -1;
#endif
}

static void get_moreinfo (int s, qse_nwifcfg_t* cfg, struct ifreq* ifr)
{
#if defined(ETHTOOL_GLINK)
	{
		/* get link status */
		struct ethtool_value ev;

		QSE_MEMSET (&ev, 0, QSE_SIZEOF(ev));
		ev.cmd= ETHTOOL_GLINK;
		ifr->ifr_data = &ev;
		if (ioctl (s, SIOCETHTOOL,ifr) >= 0)
			cfg->flags |= ev.data? QSE_NWIFCFG_LINKUP: QSE_NWIFCFG_LINKDOWN;
	}
#endif

#if 0

#if defined(ETHTOOL_GSTATS)
	{
		/* get link statistics */
		struct ethtool_drvinfo drvinfo;


		drvinfo.cmd = ETHTOOL_GDRVINFO;
		ifr->ifr_data = &drvinfo;
		if (ioctl (s, SIOCETHTOOL, ifr) >= 0)
		{
			struct ethtool_stats *stats;
			qse_uint8_t buf[1000]; /* TODO: make this dynamic according to drvinfo.n_stats */

			stats = buf;
			stats->cmd = ETHTOOL_GSTATS;
			stats->n_stats = drvinfo.n_stats * QSE_SIZEOF(stats->data[0]);
			ifr->ifr_data = (caddr_t) stats;
			if (ioctl (s, SIOCETHTOOL, ifr) >= 0)
			{
for (i = 0; i  < drvinfo.n_stats; i++)
{
	qse_printf (QSE_T(">>> %llu \n"), stats->data[i]);
}
			}
		}
	}
#endif
#endif
}

/* TOOD: consider how to handle multiple IPv6 addresses on a single interfce.
 *       consider how to get IPv4 addresses on an aliased interface? so mutliple ipv4 addresses */

int qse_getnwifcfg (qse_nwifcfg_t* cfg)
{
#if defined(_WIN32)
	/* TODO */
	return -1;
#elif defined(__OS2__)
	/* TODO */
	return -1;
#elif defined(__DOS__)
	/* TODO */
	return -1;
#else
	int x = -1, s = -1;
	struct ifreq ifr;
	qse_size_t ml, wl;

	if (cfg->type == QSE_NWIFCFG_IN4)
	{
	#if defined(AF_INET)
		s = socket (AF_INET, SOCK_DGRAM, 0);
	#endif
	}
	else if (cfg->type == QSE_NWIFCFG_IN6)
	{
	#if defined(AF_INET6)
		s = socket (AF_INET6, SOCK_DGRAM, 0);
	#endif
	}
	if (s <= -1) return -1;
	
	if (cfg->name[0] == QSE_T('\0')&& cfg->index >= 1)
	{
/* TODO: support lookup by ifindex */
	}

	QSE_MEMSET (&ifr, 0, sizeof(ifr));
	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxcpy (ifr.ifr_name, QSE_SIZEOF(ifr.ifr_name), cfg->name);
	#else
	ml = QSE_COUNTOF(ifr.ifr_name);
	if (qse_wcstombs (cfg->name, &wl, ifr.ifr_name, &ml) <= -1)  goto oops;
	#endif

	x = get_nwifcfg (s, cfg, &ifr);

	if (x >= 0) get_moreinfo (s, cfg, &ifr);

oops:
	QSE_CLOSE (s);
	return x;
#endif
}
