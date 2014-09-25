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
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/sck.h>

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h> /* sockaddr_in6 */
#	include <windows.h>
#elif defined(__OS2__)
#	if defined(TCPV40HDRS)
#		define  BSD_SELECT
#	endif
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <sys/ioctl.h>
#	include <nerrno.h>
#	if defined(TCPV40HDRS)
#		define  USE_SELECT
#		include <sys/select.h>
#	else
#		include <unistd.h>
#	endif
#elif defined(__DOS__)
 	/* TODO:  consider watt-32 */
#else
#	include "syscall.h"
#	include <sys/socket.h>
#	include <netinet/in.h>
#	if defined(HAVE_NETINET_SCTP_H)
#		include <netinet/sctp.h>
#	endif
#endif


#if !defined(SHUT_RD)
#	define SHUT_RD 0
#endif

#if !defined(SHUT_WR)
#	define SHUT_WR 1
#endif

#if !defined(SHUT_RDWR)
#	define SHUT_RDWR 2
#endif

QSE_INLINE int qse_isvalidsckhnd (qse_sck_hnd_t handle)
{
#if defined(_WIN32)
	return handle != QSE_INVALID_SCKHND;

#elif defined(__OS2__)
	return handle >= 0;

#elif defined(__DOS__)
	/* TODO: */
	return 0;
#else
	return handle >= 0;
#endif
}

QSE_INLINE void qse_closesckhnd (qse_sck_hnd_t handle)
{
#if defined(_WIN32)
	closesocket (handle);
#elif defined(__OS2__)
	soclose (handle);
#elif defined(__DOS__)
	/* TODO: */
#else
	QSE_CLOSE (handle);
#endif
}

QSE_INLINE void qse_shutsckhnd (qse_sck_hnd_t handle, qse_shutsckhnd_how_t how)
{
	static int how_v[] = { SHUT_RD, SHUT_WR, SHUT_RDWR };

#if defined(_WIN32)
	shutdown (handle, how_v[how]);
#elif defined(__OS2__)
	shutdown (handle, how_v[how]);
#elif defined(__DOS__)
	/* TODO: */
#else
	shutdown (handle, how_v[how]);
#endif
}


#if 0
qse_sck_hnd_t


int qse_sck_open (qse_mmgr_t* mmgr, qse_sck_type_t type)
{
}

void qse_sck_close (qse_sck_t* sck)
{
}

int qse_sck_init (qse_sck_t* sck, qse_mmgr_t* mmgr, qse_sck_type_t type)
{
	int domain, type, proto = 0;

	switch (type)
	{
		case QSE_SCK_TCP4:
			domain = AF_INET;
			type = SOCK_STREAM;
			break;

		case QSE_SCK_TCP6:
			domain = AF_INET6;
			type = SOCK_STREAM;
			break;

		case QSE_SCK_UDP4:
			domain = AF_INET;
			type = SOCK_DGRAM;
			break;

		case QSE_SCK_UDP6:
			domain = AF_INET6;
			type = SOCK_DGRAM;
			break;

		case QSE_SCK_SCTP4:
			domain = AF_INET;
			type = SCOK_SEQPACKET;
			proto = IPPROTO_SCTP;
			break;

		case QSE_SCK_SCTP6:
			domain = AF_INET6;
			type = SCOK_SEQPACKET;
			proto = IPPROTO_SCTP;
			break;

		case QSE_SCK_SCTP4:
			domain = AF_INET;
			type = SCOK_STREAM;
			proto = IPPROTO_SCTP;
			break;

		case QSE_SCK_SCTP6:
			domain = AF_INET6;
			type = SCOK_STREAM;
			proto = IPPROTO_SCTP;
			break;

#if 0
		case QSE_SCK_RAW4:
			domain = AF_INET;
			type = SOCK_RAW;
			break;

		case QSE_SCK_RAW6:
			domain = AF_INET6;
			type = SOCK_RAW;
			break;

		case QSE_SCK_PACKET:
			domain = AF_PACKET;
			type = SOCK_RAW;
			proto = qse_hton16(ETH_P_ALL);
			break;

		case QSE_SCK_PACKET:
			domain = AF_PACKET;
			type = SOCK_DGRAM; /* cooked packet with the link level header removed */
			proto = qse_hton16(ETH_P_ALL);
			break;

		case QSE_SCK_ARP:
			domain = AF_PACKET;
			type = SOCK_RAW;
			proto = qse_hton16(ETH_P_ARP);
			proto = 
#endif
	}

	sck->handle = socket (domain, type, proto);
}

void qse_sck_fini (qse_sck_t* sck)
{
#if defined(_WIN32)
	closesocket (sck->handle);
#elif defined(__OS2__)
	soclose (sck->handle);
#elif defined(__DOS__)
	/* TODO: */
#else
	QSE_CLOSE (sck->handle);
#endif
}


qse_ssize_t qse_recvsocket ()

qse_ssize_t recvfromsocket ()

qse_ssize_t sendsocket ()
qse_ssize_t sendtosocket ()
#endif



