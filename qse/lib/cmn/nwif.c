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

#if defined(_SCO_DS)
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


int qse_nwifmbstoindex (const qse_mchar_t* ptr, unsigned int* index)
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

#elif defined(SIOCGIFINDEX)
	int h, x;
	qse_size_t len;
	struct ifreq ifr;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return -1;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	len = qse_mbsxcpy (ifr.ifr_name, QSE_COUNTOF(ifr.ifr_name), ptr);
	if (ptr[len] != QSE_MT('\0')) return -1; /* name too long */

	x = ioctl (h, SIOCGIFINDEX, &ifr);
	QSE_CLOSE (h);

	if (x >= 0)
	{
	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
		*index = ifr.ifr_ifindex;
	#else
		*index = ifr.ifr_index;
	#endif
	}

	return x;

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t len;
	unsigned int tmpidx;

	len = qse_mbsxcpy (tmp, QSE_COUNTOF(tmp), ptr);
	if (ptr[len] != QSE_MT('\0')) return -1; /* name too long */

	tmpidx = if_nametoindex (tmp);
	if (tmpidx == 0) return -1;
	*index = tmpidx;
	return 0;

#elif defined(_SCO_DS)

	struct ifconf ifc;
	int num, i;

	if (get_sco_ifconf (&ifc) <= -1) return -1;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	for (i = 0; i < num; i++)
	{
		if (qse_mbscmp (ptr, ifc.ifc_req[i].ifr_name) == 0) 
		{
			free_sco_ifconf (&ifc);
			*index = i + 1;
			return 0;
		}
	}

	free_sco_ifconf (&ifc);
	return -1;

#else
	return -1;
#endif
}

int qse_nwifmbsntoindex (const qse_mchar_t* ptr, qse_size_t len, unsigned int* index)
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

#elif defined(SIOCGIFINDEX)
	int h, x;
	struct ifreq ifr;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return -1;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	if (qse_mbsxncpy (ifr.ifr_name, QSE_COUNTOF(ifr.ifr_name), ptr, len) < len) return -1; /* name too long */

	x = ioctl (h, SIOCGIFINDEX, &ifr);
	QSE_CLOSE (h);

	if (x >= 0)
	{
	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
		*index = ifr.ifr_ifindex;
	#else
		*index = ifr.ifr_index;
	#endif
	}

	return x;

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	unsigned int tmpidx;

	if (qse_mbsxncpy (tmp, QSE_COUNTOF(tmp), ptr, len) < len) return -1;

	tmpidx = if_nametoindex (tmp);
	if (tmpidx == 0) return -1;
	*index = tmpidx;
	return 0;

#elif defined(_SCO_DS)

	struct ifconf ifc;
	int num, i;

	if (get_sco_ifconf (&ifc) <= -1) return -1;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	for (i = 0; i < num; i++)
	{
		if (qse_mbsxcmp (ptr, len, ifc.ifc_req[i].ifr_name) == 0) 
		{
			free_sco_ifconf (&ifc);
			*index = i + 1;
			return 0;
		}
	}

	free_sco_ifconf (&ifc);
	return -1;

#else
	return -1;
#endif
}

int qse_nwifwcstoindex (const qse_wchar_t* ptr, unsigned int* index)
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

#elif defined(SIOCGIFINDEX)
	int h, x;
	struct ifreq ifr;
	qse_size_t wl, ml;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return -1;

	ml = QSE_COUNTOF(ifr.ifr_name);
	if (qse_wcstombs (ptr, &wl, ifr.ifr_name, &ml) <= -1) return -1;

	x = ioctl (h, SIOCGIFINDEX, &ifr);
	QSE_CLOSE (h);

	if (x >= 0)
	{
	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
		*index = ifr.ifr_ifindex;
	#else
		*index = ifr.ifr_index;
	#endif
	}

	return x;

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;
	unsigned int tmpidx;

	ml = QSE_COUNTOF(tmp);
	if (qse_wcstombs (ptr, &wl, tmp, &ml) <= -1) return -1;

	tmpidx = if_nametoindex (tmp);
	if (tmpidx == 0) return -1;
	*index = tmpidx;
	return 0;

#elif defined(SIOCGIFCONF) && (defined(SIOCGIFANUM) || defined(SIOCGIFNUM))

	struct ifconf ifc;
	int num, i;
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;

	ml = QSE_COUNTOF(tmp);
	if (qse_wcstombs (ptr, &wl, tmp, &ml) <= -1) return -1;

	if (get_sco_ifconf (&ifc) <= -1) return -1;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	for (i = 0; i < num; i++)
	{
		if (qse_mbscmp (tmp, ifc.ifc_req[i].ifr_name) == 0) 
		{
			free_sco_ifconf (&ifc);
			*index = i + 1;
			return 0;
		}
	}

	free_sco_ifconf (&ifc);
	return -1;

#else
	return -1;
#endif
}

int qse_nwifwcsntoindex (const qse_wchar_t* ptr, qse_size_t len, unsigned int* index)
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

#elif defined(SIOCGIFINDEX)
	int h, x;
	struct ifreq ifr;
	qse_size_t wl, ml;

	h = socket (AF_INET, SOCK_DGRAM, 0); 
	if (h <= -1) return -1;

	wl = len; ml = QSE_COUNTOF(ifr.ifr_name) - 1;
	if (qse_wcsntombsn (ptr, &wl, ifr.ifr_name, &ml) <= -1) return -1;
	ifr.ifr_name[ml] = QSE_MT('\0');

	x = ioctl (h, SIOCGIFINDEX, &ifr);
	QSE_CLOSE (h);

	if (x >= 0)
	{
	#if defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
		*index = ifr.ifr_ifindex;
	#else
		*index = ifr.ifr_index;
	#endif
	}

	return x;

#elif defined(HAVE_IF_NAMETOINDEX)
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;
	unsigned int tmpidx;

	wl = len; ml = QSE_COUNTOF(tmp) - 1;
	if (qse_wcsntombsn (ptr, &wl, tmp, &ml) <= -1) return -1;
	tmp[ml] = QSE_MT('\0');

	tmpidx = if_nametoindex (tmp);
	if (tmpidx == 0) return -1;
	*index = tmpidx;
	return 0;

#elif defined(SIOCGIFCONF) && (defined(SIOCGIFANUM) || defined(SIOCGIFNUM))
	struct ifconf ifc;
	int num, i;
	qse_mchar_t tmp[IF_NAMESIZE + 1];
	qse_size_t wl, ml;

	wl = len; ml = QSE_COUNTOF(tmp) - 1;
	if (qse_wcsntombsn (ptr, &wl, tmp, &ml) <= -1) return -1;
	tmp[ml] = QSE_MT('\0');

	if (get_sco_ifconf (&ifc) <= -1) return -1;

	num = ifc.ifc_len / QSE_SIZEOF(struct ifreq);
	for (i = 0; i < num; i++)
	{
		if (qse_mbscmp (tmp, ifc.ifc_req[i].ifr_name) == 0) 
		{
			free_sco_ifconf (&ifc);
			*index = i + 1;
			return 0;
		}
	}

	free_sco_ifconf (&ifc);
	return -1;
#else
	return -1;
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
