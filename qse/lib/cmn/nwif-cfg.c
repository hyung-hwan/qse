/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it uheadr the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/nwif.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include "mem.h"

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include <iphlpapi.h> 
#elif defined(__OS2__)
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <tcpustd.h>
#	include <sys/ioctl.h>
#	include <nerrno.h>
#	pragma library("tcpip32.lib")
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
#	if defined(HAVE_SYS_SOCKIO_H)
#		include <sys/sockio.h>
#	endif
#	if defined(HAVE_IFADDRS_H)
#		include <ifaddrs.h>
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


#if defined(SIOCGLIFCONF)
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


static int get_nwifcfg (int s, qse_nwifcfg_t* cfg)
{
#if defined(SIOCGLIFADDR)
	struct lifreq ifrbuf;
	qse_size_t ml, wl;
	
	QSE_MEMSET (&ifrbuf, 0, QSE_SIZEOF(ifrbuf));

	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxcpy (ifrbuf.lifr_name, QSE_SIZEOF(ifrbuf.lifr_name), cfg->name);
	#else
	ml = QSE_COUNTOF(ifrbuf.lifr_name);
	if (qse_wcstombs (cfg->name, &wl, ifrbuf.lifr_name, &ml) <= -1) return -1;
	#endif

	if (ioctl (s, SIOCGLIFINDEX, &ifrbuf) <= -1) return -1;		
	cfg->index = ifrbuf.lifr_index;
	
	if (ioctl (s, SIOCGLIFADDR, &ifrbuf) <= -1) return -1;
	qse_skadtonwad (&ifrbuf.lifr_addr, &cfg->addr);

	if (ioctl (s, SIOCGLIFNETMASK, &ifrbuf) <= -1) return -1;
	qse_skadtonwad (&ifrbuf.lifr_addr, &cfg->mask);

	if (ioctl (s, SIOCGLIFMTU, &ifrbuf) <= -1) return -1;
	cfg->mtu = ifrbuf.lifr_mtu;

	if (ioctl (s, SIOCGLIFFLAGS, &ifrbuf) <= -1) return -1;
	if (ifrbuf.lifr_flags & IFF_UP) cfg->flags |= QSE_NWIFCFG_UP;
	if (ifrbuf.lifr_flags & IFF_RUNNING) cfg->flags |= QSE_NWIFCFG_RUNNING;
	if (ifrbuf.lifr_flags & IFF_BROADCAST) 
	{
		if (ioctl (s, SIOCGLIFBRDADDR, &ifrbuf) <= -1) return -1;
		qse_skadtonwad (&ifrbuf.lifr_broadaddr, &cfg->bcast);
		cfg->flags |= QSE_NWIFCFG_BCAST;
	}
	if (ifrbuf.lifr_flags & IFF_POINTOPOINT) 
	{
		if (ioctl (s, SIOCGLIFDSTADDR, &ifrbuf) <= -1) return -1;
		qse_skadtonwad (&ifrbuf.lifr_dstaddr, &cfg->ptop);
		cfg->flags |= QSE_NWIFCFG_PTOP;
	}

	return 0;

#elif defined(SIOCGIFADDR)
	struct ifreq ifrbuf;
	qse_size_t ml, wl;
	
	QSE_MEMSET (&ifrbuf, 0, QSE_SIZEOF(ifrbuf));

	#if defined(QSE_CHAR_IS_MCHAR)
	qse_mbsxcpy (ifrbuf.ifr_name, QSE_SIZEOF(ifrbuf.ifr_name), cfg->name);
	#else
	ml = QSE_COUNTOF(ifrbuf.ifr_name);
	if (qse_wcstombs (cfg->name, &wl, ifrbuf.ifr_name, &ml) <= -1) return -1;
	#endif

	#if defined(SIOCGIFINDEX)
	if (ioctl (s, SIOCGIFINDEX, &ifrbuf) <= -1) return -1;		
	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
	cfg->index = ifrbuf.ifr_ifindex;
	#else
	cfg->index = ifrbuf.ifr_index;
	#endif
	#else
	cfg->index = 0;
	#endif
	
	if (ioctl (s, SIOCGIFADDR, &ifrbuf) <= -1) return -1;
	qse_skadtonwad (&ifrbuf.ifr_addr, &cfg->addr);

	if (ioctl (s, SIOCGIFNETMASK, &ifrbuf) <= -1) return -1;
	qse_skadtonwad (&ifrbuf.ifr_addr, &cfg->mask);

	if (ioctl (s, SIOCGIFMTU, &ifrbuf) <= -1) return -1;
	#if defined(HAVE_STRUCT_IFREQ_IFR_MTU)
	cfg->mtu = ifrbuf.ifr_mtu;
	#else
	/* well, this is a bit dirty. but since all these are unions, 
	 * the name must not really matter. some OSes just omitts defining
	 * the MTU field */
	cfg->mtu = ifrbuf.ifr_metric; 
	#endif

	if (ioctl (s, SIOCGIFFLAGS, &ifrbuf) <= -1) return -1;
	if (ifrbuf.ifr_flags & IFF_UP) cfg->flags |= QSE_NWIFCFG_UP;
	if (ifrbuf.ifr_flags & IFF_RUNNING) cfg->flags |= QSE_NWIFCFG_RUNNING;
	if (ifrbuf.ifr_flags & IFF_BROADCAST) 
	{
		if (ioctl (s, SIOCGIFBRDADDR, &ifrbuf) <= -1) return -1;
		qse_skadtonwad (&ifrbuf.ifr_broadaddr, &cfg->bcast);
		cfg->flags |= QSE_NWIFCFG_BCAST;
	}
	else QSE_MEMSET (&cfg->bcast, 0, QSE_SIZEOF(cfg->bcast));
	if (ifrbuf.ifr_flags & IFF_POINTOPOINT) 
	{
		if (ioctl (s, SIOCGIFDSTADDR, &ifrbuf) <= -1) return -1;
		qse_skadtonwad (&ifrbuf.ifr_dstaddr, &cfg->ptop);
		cfg->flags |= QSE_NWIFCFG_PTOP;
	}
	else QSE_MEMSET (&cfg->ptop, 0, QSE_SIZEOF(cfg->ptop));

	return 0;
#else

	/* TODO  */
	return -1;
#endif
}

int qse_getnwifcfg (qse_nwifcfg_t* cfg)
{
	int s = -1;

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
	
	return get_nwifcfg (s, cfg);
}
