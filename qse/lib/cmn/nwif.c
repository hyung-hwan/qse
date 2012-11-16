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
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/nwif.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include "mem.h"

#if defined(_WIN32)
	/* TODO: */
#elif defined(__OS2__)
	/* TODO: */
#elif defined(__DOS__)
	/* TODO: */
#else
#	include "syscall.h"
#	include <sys/socket.h>
#	if defined(HAVE_SYS_IOCTL_H)
#		include <sys/ioctl.h>
#	endif
#	if defined(HAVE_NET_IF_H)
#		include <net/if.h>
#	endif
#	if defined(HAVE_SYS_SOCKIO_H)
#		include <sys/sockio.h>
#	endif
#	if !defined(IF_NAMESIZE)
#		define IF_NAMESIZE 63
#	endif
#endif

#if !defined(SIOCGIFINDEX) && !defined(SIOCGIFNAME) && \
    !defined(HAVE_IF_NAMETOINDEX) && !defined(HAVE_IF_INDEXTONAME) && \
    defined(SIOCGIFCONF) && (defined(SIOCGIFANUM) || defined(SIOCGIFNUM))
static int get_sco_ifconf (struct ifconf* ifc)
{
	/* SCO doesn't have have any IFINDEX thing.
	 * i emultate it using IFCONF */
	int h, num;
	struct ifreq* ifr;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return -1;

	ifc->ifc_len = 0;
	ifc->ifc_buf = QSE_NULL;

	#if defined(SIOCGIFANUM)
	if (ioctl (h, SIOCGIFANUM, &num) <= -1) goto oops;
	#else
	if (ioctl (h, SIOCGIFNUM, &num) <= -1) goto oops;
	#endif

	/* sco needs reboot when you add an network interface.
	 * it should be safe not to consider the case when the interface
	 * is added after SIOCGIFANUM above. 
	 * another thing to note is that SIOCGIFCONF ends with segfault
	 * if the buffer is not large enough unlike some other OSes
	 * like opensolaris which truncates the configuration. */

	ifc->ifc_len = num * QSE_SIZEOF(*ifr);
	ifc->ifc_buf = QSE_MMGR_ALLOC (QSE_MMGR_GETDFL(), ifc->ifc_len);
	if (ifc->ifc_buf == QSE_NULL) goto oops;

	if (ioctl (h, SIOCGIFCONF, ifc) <= -1) goto oops;
	QSE_CLOSE (h); h = -1;

	return 0;

oops:
	if (ifc->ifc_buf) QSE_MMGR_FREE (QSE_MMGR_GETDFL(), ifc->ifc_buf);
	if (h >= 0) QSE_CLOSE (h);
	return -1;
}

static QSE_INLINE void free_sco_ifconf (struct ifconf* ifc)
{
	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), ifc->ifc_buf);
}
#endif


unsigned int qse_nwifmbstoindex (const qse_mchar_t* ptr)
{
#if defined(_WIN32)
	/* TODO: */
	return 0u;
#elif defined(__OS2__)
	/* TODO: */
	return 0u;
#elif defined(__DOS__)
	/* TODO: */
	return 0u;

#elif defined(SIOCGIFINDEX)
	int h, x;
	qse_size_t len;
	struct ifreq ifr;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return 0u;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	len = qse_mbsxcpy (ifr.ifr_name, QSE_COUNTOF(ifr.ifr_name), ptr);
	if (ptr[len] != QSE_MT('\0')) return 0u; /* name too long */

	x = ioctl (h, SIOCGIFINDEX, &ifr);
	QSE_CLOSE (h);

	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
	return (x <= -1)? 0u: ifr.ifr_ifindex;
	#else
	return (x <= -1)? 0u: ifr.ifr_index;
	#endif

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t len;

	len = qse_mbsxcpy (tmp, QSE_COUNTOF(tmp), ptr);
	if (ptr[len] != QSE_MT('\0')) return 0u; /* name too long */
	return if_nametoindex (tmp);

#elif defined(SIOCGIFCONF) && (defined(SIOCGIFANUM) || defined(SIOCGIFNUM))

	struct ifconf ifc;
	int num, i;

	if (get_sco_ifconf (&ifc) <= -1) return 0u;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	for (i = 0; i < num; i++)
	{
		if (qse_mbscmp (ptr, ifc.ifc_req[i].ifr_name) == 0) 
		{
			free_sco_ifconf (&ifc);
			return i + 1;
		}
	}

	free_sco_ifconf (&ifc);
	return 0u;

#else
	return 0u;
#endif
}

unsigned int qse_nwifmbsntoindex (const qse_mchar_t* ptr, qse_size_t len)
{
#if defined(_WIN32)
	/* TODO: */
	return 0u;
#elif defined(__OS2__)
	/* TODO: */
	return 0u;
#elif defined(__DOS__)
	/* TODO: */
	return 0u;

#elif defined(SIOCGIFINDEX)
	int h, x;
	struct ifreq ifr;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return 0u;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	if (qse_mbsxncpy (ifr.ifr_name, QSE_COUNTOF(ifr.ifr_name), ptr, len) < len) return 0u; /* name too long */

	x = ioctl (h, SIOCGIFINDEX, &ifr);
	QSE_CLOSE (h);

	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
	return (x <= -1)? 0u: ifr.ifr_ifindex;
	#else
	return (x <= -1)? 0u: ifr.ifr_index;
	#endif

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	if (qse_mbsxncpy (tmp, QSE_COUNTOF(tmp), ptr, len) < len) return 0u; /* name too long */
	return if_nametoindex (tmp);

#elif defined(SIOCGIFCONF) && (defined(SIOCGIFANUM) || defined(SIOCGIFNUM))

	struct ifconf ifc;
	int num, i;

	if (get_sco_ifconf (&ifc) <= -1) return 0u;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	for (i = 0; i < num; i++)
	{
		if (qse_mbsxcmp (ptr, len, ifc.ifc_req[i].ifr_name) == 0) 
		{
			free_sco_ifconf (&ifc);
			return i + 1;
		}
	}

	free_sco_ifconf (&ifc);
	return 0u;

#else
	return 0u;
#endif
}

unsigned int qse_nwifwcstoindex (const qse_wchar_t* ptr)
{
#if defined(_WIN32)
	/* TODO: */
	return 0u;
#elif defined(__OS2__)
	/* TODO: */
	return 0u;
#elif defined(__DOS__)
	/* TODO: */
	return 0u;

#elif defined(SIOCGIFINDEX)
	int h, x;
	struct ifreq ifr;
	qse_size_t wl, ml;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return 0u;

	ml = QSE_COUNTOF(ifr.ifr_name);
	if (qse_wcstombs (ptr, &wl, ifr.ifr_name, &ml) <= -1) return 0u;

	x = ioctl (h, SIOCGIFINDEX, &ifr);
	QSE_CLOSE (h);

	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
	return (x <= -1)? 0u: ifr.ifr_ifindex;
	#else
	return (x <= -1)? 0u: ifr.ifr_index;
	#endif

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;

	ml = QSE_COUNTOF(tmp);
	if (qse_wcstombs (ptr, &wl, tmp, &ml) <= -1) return 0u;

	return if_nametoindex (tmp);

#elif defined(SIOCGIFCONF) && (defined(SIOCGIFANUM) || defined(SIOCGIFNUM))

	struct ifconf ifc;
	int num, i;
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;

	ml = QSE_COUNTOF(tmp);
	if (qse_wcstombs (ptr, &wl, tmp, &ml) <= -1) return 0u;

	if (get_sco_ifconf (&ifc) <= -1) return 0u;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	for (i = 0; i < num; i++)
	{
		if (qse_mbscmp (tmp, ifc.ifc_req[i].ifr_name) == 0) 
		{
			free_sco_ifconf (&ifc);
			return i + 1;
		}
	}

	free_sco_ifconf (&ifc);
	return 0u;

#else
	return 0u;
#endif
}

unsigned int qse_nwifwcsntoindex (const qse_wchar_t* ptr, qse_size_t len)
{
#if defined(_WIN32)
	/* TODO: */
	return 0u;
#elif defined(__OS2__)
	/* TODO: */
	return 0u;
#elif defined(__DOS__)
	/* TODO: */
	return 0u;

#elif defined(SIOCGIFINDEX)
	int h, x;
	struct ifreq ifr;
	qse_size_t wl, ml;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return 0u;

	wl = len; ml = QSE_COUNTOF(ifr.ifr_name) - 1;
	if (qse_wcsntombsn (ptr, &wl, ifr.ifr_name, &ml) <= -1) return 0;
	ifr.ifr_name[ml] = QSE_MT('\0');

	x = ioctl (h, SIOCGIFINDEX, &ifr);
	QSE_CLOSE (h);

	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
	return (x <= -1)? 0u: ifr.ifr_ifindex;
	#else
	return (x <= -1)? 0u: ifr.ifr_index;
	#endif

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;

	wl = len; ml = QSE_COUNTOF(tmp) - 1;
	if (qse_wcsntombsn (ptr, &wl, tmp, &ml) <= -1) return 0u;
	tmp[ml] = QSE_MT('\0');
	return if_nametoindex (tmp);

#elif defined(SIOCGIFCONF) && (defined(SIOCGIFANUM) || defined(SIOCGIFNUM))
	struct ifconf ifc;
	int num, i;
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;

	wl = len; ml = QSE_COUNTOF(tmp) - 1;
	if (qse_wcsntombsn (ptr, &wl, tmp, &ml) <= -1) return 0u;
	tmp[ml] = QSE_MT('\0');

	if (get_sco_ifconf (&ifc) <= -1) return -1;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	for (i = 0; i < num; i++)
	{
		if (qse_mbscmp (tmp, ifc.ifc_req[i].ifr_name) == 0) 
		{
			free_sco_ifconf (&ifc);
			return i + 1;
		}
	}

	free_sco_ifconf (&ifc);
	return 0u;
#else
	return 0u;
#endif
}

/* ---------------------------------------------------------- */

int qse_nwifindextombs (unsigned int index, qse_mchar_t* buf, qse_size_t len)
{
#if defined(_WIN32)
	/* TODO: */
	return -1;
#elif defined(__OS2__)
	/* TODO: */
	return -1;
#elif defined(__DOS__)
	/* TODO: */
	return -1;

#elif defined(SIOCGIFNAME)

	int h, x;
	struct ifreq ifr;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return -1;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
	ifr.ifr_ifindex = index;
	#else
	ifr.ifr_index = index;
	#endif
	
	x = ioctl (h, SIOCGIFNAME, &ifr);
	QSE_CLOSE (h);

	return (x <= -1)? -1: qse_mbsxcpy (buf, len, ifr.ifr_name);

#elif defined(HAVE_IF_INDEXTONAME)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	if (if_indextoname (index, tmp) == QSE_NULL) return -1;
	return qse_mbsxcpy (buf, len, tmp);

#elif defined(SIOCGIFCONF) && (defined(SIOCGIFANUM) || defined(SIOCGIFNUM))

	struct ifconf ifc;
	qse_size_t ml;
	int num;

	if (index <= 0) return -1;
	if (get_sco_ifconf (&ifc) <= -1) return -1;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	if (index > num) 
	{
		free_sco_ifconf (&ifc);
		return -1;
	}

	ml = qse_mbsxcpy (buf, len, ifc.ifc_req[index - 1].ifr_name);
	free_sco_ifconf (&ifc);
	return ml;

#else
	return -1;
#endif
}

int qse_nwifindextowcs (unsigned int index, qse_wchar_t* buf, qse_size_t len)
{
#if defined(_WIN32)
	/* TODO: */
	return -1;
#elif defined(__OS2__)
	/* TODO: */
	return -1;
#elif defined(__DOS__)
	/* TODO: */
	return -1;

#elif defined(SIOCGIFNAME)

	int h, x;
	struct ifreq ifr;
	qse_size_t wl, ml;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return -1;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
	ifr.ifr_ifindex = index;
	#else
	ifr.ifr_index = index;
	#endif
	
	x = ioctl (h, SIOCGIFNAME, &ifr);
	QSE_CLOSE (h);

	if (x <= -1) return -1;

	wl = len;
	x = qse_mbstowcs (ifr.ifr_name, &ml, buf, &wl);
	if (x == -2 && wl > 1) buf[wl - 1] = QSE_WT('\0');
	else if (x != 0) return -1;
	return wl;

#elif defined(HAVE_IF_INDEXTONAME)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t ml, wl;
	int x;

	if (if_indextoname (index, tmp) == QSE_NULL) return -1;
	wl = len;
	x = qse_mbstowcs (tmp, &ml, buf, &wl);
	if (x == -2 && wl > 1) buf[wl - 1] = QSE_WT('\0');
	else if (x != 0) return -1;
	return wl;

#elif defined(SIOCGIFCONF) && (defined(SIOCGIFANUM) || defined(SIOCGIFNUM))

	struct ifconf ifc;
	qse_size_t wl, ml;
	int num, x;

	if (index <= 0) return -1;
	if (get_sco_ifconf (&ifc) <= -1) return -1;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	if (index > num) 
	{
		free_sco_ifconf (&ifc);
		return -1;
	}

	wl = len;
	x = qse_mbstowcs (ifc.ifc_req[index - 1].ifr_name, &ml, buf, &wl);
	free_sco_ifconf (&ifc);

	if (x == -2 && wl > 1) buf[wl - 1] = QSE_WT('\0');
	else if (x != 0) return -1;

	return wl;
#else
	return -1;
#endif
}
