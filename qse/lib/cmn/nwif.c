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
#	if !defined(IF_NAMESIZE)
#		define IF_NAMESIZE 63
#	endif
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

	return (x <= -1)? 0u: ifr.ifr_ifindex;

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t len;

	len = qse_mbsxcpy (tmp, QSE_COUNTOF(tmp), ptr);
	if (ptr[len] != QSE_MT('\0')) return 0u; /* name too long */
	return if_nametoindex (tmp);

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

	return (x <= -1)? 0u: ifr.ifr_ifindex;

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	if (qse_mbsxncpy (tmp, QSE_COUNTOF(tmp), ptr, len) < len) return 0u; /* name too long */
	return if_nametoindex (tmp);

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
	if (qse_wcstombs (ptr, &wl, ifr.ifr_name, &ml) <= -1) return 0;

	x = ioctl (h, SIOCGIFINDEX, &ifr);
	QSE_CLOSE (h);

	return (x <= -1)? 0u: ifr.ifr_ifindex;

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;

	ml = QSE_COUNTOF(tmp);
	if (qse_wcstombs (ptr, &wl, tmp, &ml) <= -1) return 0;

	return if_nametoindex (tmp);

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

	return (x <= -1)? 0u: ifr.ifr_ifindex;

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;

	wl = len; ml = QSE_COUNTOF(tmp) - 1;
	if (qse_wcsntombsn (ptr, &wl, tmp, &ml) <= -1) return 0;
	tmp[ml] = QSE_MT('\0');
	return if_nametoindex (tmp);

#else
	return 0u;
#endif
}

qse_size_t qse_nwifindextombs (unsigned int index, qse_mchar_t* buf, qse_size_t len)
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

#elif defined(SIOCGIFNAME)

	int h, x;
	struct ifreq ifr;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return 0u;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	ifr.ifr_ifindex = index;
	
	x = ioctl (h, SIOCGIFNAME, &ifr);
	QSE_CLOSE (h);

	return (x <= -1)? 0: qse_mbsxcpy (buf, len, ifr.ifr_name);

#elif defined(HAVE_IF_INDEXTONAME)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	if (if_indextoname (index, tmp) == QSE_NULL) return 0;
	return qse_mbsxcpy (buf, len, tmp);
#else
	return 0;
#endif
}

qse_size_t qse_nwifindextowcs (unsigned int index, qse_wchar_t* buf, qse_size_t len)
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

#elif defined(SIOCGIFNAME)

	int h, x;
	struct ifreq ifr;
	qse_size_t wl, ml;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return 0u;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	ifr.ifr_ifindex = index;
	
	x = ioctl (h, SIOCGIFNAME, &ifr);
	QSE_CLOSE (h);

	if (x <= -1) return 0;

	wl = len;
	x = qse_mbstowcs (ifr.ifr_name, &ml, buf, &wl);
	if (x == -2 && wl > 1) buf[wl - 1] = QSE_WT('\0');
	else if (x != 0) return 0;
	return wl;

#elif defined(HAVE_IF_INDEXTONAME)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t ml, wl;
	int x;

	if (if_indextoname (index, tmp) == QSE_NULL) return 0;
	wl = len;
	x = qse_mbstowcs (tmp, &ml, buf, &wl);
	if (x == -2 && wl > 1) buf[wl - 1] = QSE_WT('\0');
	else if (x != 0) return 0;
	return wl;
#else
	return 0;
#endif
}
