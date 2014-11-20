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
#	include <tcp.h> /* watt-32 */

#elif defined(HAVE_T_CONNECT) && !defined(HAVE_CONNECT) && defined(HAVE_TIUSER_H)

#	include "syscall.h"
#	include <tiuser.h>
#	define USE_TLI

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
	return handle >= 0;

#elif defined(USE_TLI)
	return handle >= 0;

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
	close_s (handle);

#elif defined(USE_TLI)
	t_close (handle);

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
	shutdown (handle, how_v[how]);

#elif defined(USE_TLI)
	/* Is this correct? */
	switch (how)
	{
		case QSE_SHUTSCKHND_R:
			t_rcvrel (handle);
			break;
		case QSE_SHUTSCKHND_W:
			t_sndrel (handle);
			break;
		case QSE_SHUTSCKHND_RW:
			t_rcvrel (handle);
			t_sndrel (handle);
			break;
	}

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
	close_s (sck->handle)
#else
	QSE_CLOSE (sck->handle);
#endif
}


qse_ssize_t qse_recvsocket ()

qse_ssize_t recvfromsocket ()

qse_ssize_t sendsocket ()
qse_ssize_t sendtosocket ()
#endif



