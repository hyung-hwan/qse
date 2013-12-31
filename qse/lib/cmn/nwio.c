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

#include <qse/cmn/nwio.h>
#include <qse/cmn/time.h>
#include "mem.h"

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h> /* sockaddr_in6 */
#	include <windows.h>
#	define  USE_SELECT
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
#	include <sys/time.h>
#	define  USE_SELECT
#endif

enum
{
	STATUS_UDP_CONNECT    = (1 << 0),
	STATUS_TMOUT_R_PRESET = (1 << 1),
	STATUS_TMOUT_W_PRESET = (1 << 2)
};

static qse_ssize_t socket_output (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size);
static qse_ssize_t socket_input (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size);

#define TMOUT_ENABLED(tmout) (tmout.sec >= 0 && tmout.nsec >= 0)

#if defined(_WIN32)
static qse_nwio_errnum_t skerr_to_errnum (DWORD e)
{
	switch (e)
	{
		case WSA_NOT_ENOUGH_MEMORY:
			return QSE_NWIO_ENOMEM;

		case WSA_INVALID_PARAMETER:
		case WSA_INVALID_HANDLE:
			return QSE_NWIO_EINVAL;

		case WSAEACCES:
			return QSE_NWIO_EACCES;

		case WSAEINTR:
			return QSE_NWIO_EINTR;

		case WSAECONNREFUSED:
		case WSAENETUNREACH:
		case WSAEHOSTUNREACH:
		case WSAEHOSTDOWN:
			return QSE_NWIO_ECONN;

		default:
			return QSE_NWIO_ESYSERR;
	}
}
#elif defined(__OS2__)
static qse_nwio_errnum_t skerr_to_errnum (int e)
{
	switch (e)
	{
	#if defined(SOCENOMEM)
		case SOCENOMEM:
			return QSE_NWIO_ENOMEM;
	#endif

		case SOCEINVAL:
			return QSE_NWIO_EINVAL;

		case SOCEACCES:
			return QSE_NWIO_EACCES;

	#if defined(SOCENOENT)
		case SOCENOENT:
			return QSE_NWIO_ENOENT;
	#endif

	#if defined(SOCEXIST)
		case SOCEEXIST:
			return QSE_NWIO_EEXIST;
	#endif
	
		case SOCEINTR:
			return QSE_NWIO_EINTR;

		case SOCEPIPE:
			return QSE_NWIO_EPIPE;

		case SOCECONNREFUSED:
		case SOCENETUNREACH:
		case SOCEHOSTUNREACH:
		case SOCEHOSTDOWN:
			return QSE_NWIO_ECONN;

		default:
			return QSE_NWIO_ESYSERR;
	}
}

#elif defined(__DOS__)
static qse_nwio_errnum_t skerr_to_errnum (int e)
{
	/* TODO: */
	return QSE_NWIO_ESYSERR;
}
#else
static qse_nwio_errnum_t skerr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_NWIO_ENOMEM;

		case EINVAL:
			return QSE_NWIO_EINVAL;

		case EACCES:
			return QSE_NWIO_EACCES;

		case ENOENT:
			return QSE_NWIO_ENOENT;

		case EEXIST:
			return QSE_NWIO_EEXIST;
	
		case EINTR:
			return QSE_NWIO_EINTR;

		case EPIPE:
			return QSE_NWIO_EPIPE;

		case EAGAIN:
			return QSE_NWIO_EAGAIN;

#if defined(ECONNREFUSED) || defined(ENETUNREACH) || defined(EHOSTUNREACH) || defined(EHOSTDOWN)
	#if defined(ECONNREFUSED) 
		case ECONNREFUSED:
	#endif
	#if defined(ENETUNREACH) 
		case ENETUNREACH:
	#endif
	#if defined(EHOSTUNREACH) 
		case EHOSTUNREACH:
	#endif
	#if defined(EHOSTDOWN) 
		case EHOSTDOWN:
	#endif
			return QSE_NWIO_ECONN;
#endif

		default:
			return QSE_NWIO_ESYSERR;
	}
}
#endif

static qse_nwio_errnum_t tio_errnum_to_nwio_errnum (qse_tio_t* tio)
{
	switch (tio->errnum)
	{
		case QSE_TIO_ENOMEM:
			return QSE_NWIO_ENOMEM;
		case QSE_TIO_EINVAL:
			return QSE_NWIO_EINVAL;
		case QSE_TIO_EACCES:
			return QSE_NWIO_EACCES;
		case QSE_TIO_ENOENT:
			return QSE_NWIO_ENOENT;
		case QSE_TIO_EILSEQ:
			return QSE_NWIO_EILSEQ;
		case QSE_TIO_EICSEQ:
			return QSE_NWIO_EICSEQ;
		case QSE_TIO_EILCHR:
			return QSE_NWIO_EILCHR;
		default:
			return QSE_NWIO_EOTHER;
	}
}

static int wait_for_data (qse_nwio_t* nwio, const qse_ntime_t* tmout, int what)
{
	int xret;

#if defined(USE_SELECT)
	fd_set fds[2];
	struct timeval tv;

	FD_ZERO (&fds[0]);
	FD_ZERO (&fds[1]);

	FD_SET (nwio->handle, &fds[what]);

	tv.tv_sec = tmout->sec;
	tv.tv_usec = QSE_NSEC_TO_USEC (tmout->nsec);

	xret = select (nwio->handle + 1, &fds[0], &fds[1], QSE_NULL, &tv);
	#if defined(_WIN32)
	if (xret == SOCKET_ERROR)
	#else
	if (xret <= -1)
	#endif
	{	
	#if defined(_WIN32)
		nwio->errnum = skerr_to_errnum (WSAGetLastError());
	#elif defined(__OS2__)
		nwio->errnum = skerr_to_errnum (sock_errno());
	#else		
		nwio->errnum = skerr_to_errnum (errno);
	#endif
		return -1;
	}
	else if (xret == 0)
	{
		nwio->errnum = QSE_NWIO_ETMOUT;
		return -1;
	}
	return 0;

#elif defined(__OS2__)
	int count[2] = { 0, 0 };
	long tmout_msecs; 
	
	count[what]++;

	tmout_msecs = QSE_SECNSEC_TO_MSEC (tmout->sec, tmout->nsec);
	xret = os2_select (&nwio->handle, count[0], count[1], 0, tmout_msecs);
	if (xret <= -1)
	{	
		nwio->errnum = skerr_to_errnum (sock_errno());
		return -1;
	}
	else if (xret == 0)
	{
		nwio->errnum = QSE_NWIO_ETMOUT;
		return -1;
	}
	return 0;

#else
	nwio->errnum = QSE_NWIO_ENOIMPL;
	return -1;
#endif

}

qse_nwio_t* qse_nwio_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, const qse_nwad_t* nwad, 
	int flags, const qse_nwio_tmout_t* tmout)
{
	qse_nwio_t* nwio;

	nwio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_nwio_t) + xtnsize);
	if (nwio == QSE_NULL) return QSE_NULL;

	if (qse_nwio_init (nwio, mmgr, nwad, flags, tmout) <= -1)
	{
		QSE_MMGR_FREE (mmgr, nwio);
		return QSE_NULL;
	}

	QSE_MEMSET (nwio + 1, 0, xtnsize);
	return nwio;
}

void qse_nwio_close (qse_nwio_t* nwio)
{
	qse_nwio_fini (nwio);
	QSE_MMGR_FREE (nwio->mmgr, nwio);
}

static int preset_tmout (qse_nwio_t* nwio)
{
#if defined(SO_RCVTIMEO) && defined(SO_SNDTIMEO)
	#if defined(_WIN32)
	DWORD tv;
	#elif defined(__OS2__)
	long	tv;
	#else
	struct timeval tv;
	#endif

	if (TMOUT_ENABLED(nwio->tmout.r))
	{
	#if defined(_WIN32) || defined(__OS2__)
		tv = QSE_SEC_TO_MSEC(nwio->tmout.r.sec) + QSE_NSEC_TO_MSEC (nwio->tmout.r.nsec);
	#else
		tv.tv_sec = nwio->tmout.r.sec;
		tv.tv_usec = QSE_NSEC_TO_USEC (nwio->tmout.r.nsec);
	#endif

		if (setsockopt (nwio->handle, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, QSE_SIZEOF(tv)) <= -1)
		{
	#if defined(_WIN32)
			nwio->errnum = skerr_to_errnum (WSAGetLastError());
	#elif defined(__OS2__)
			nwio->errnum = skerr_to_errnum (sock_errno());
	#else
			nwio->errnum = skerr_to_errnum (errno);
	#endif
			return -1; /* tried to set but failed */
		}

		nwio->status |= STATUS_TMOUT_R_PRESET;
	}

	if (TMOUT_ENABLED(nwio->tmout.w))
	{
	#if defined(_WIN32) || defined(__OS2__)
		tv = QSE_SEC_TO_MSEC(nwio->tmout.w.sec) + QSE_NSEC_TO_MSEC (nwio->tmout.w.nsec);
	#else
		tv.tv_sec = nwio->tmout.w.sec;
		tv.tv_usec = QSE_NSEC_TO_USEC (nwio->tmout.w.nsec);
	#endif
		if (setsockopt (nwio->handle, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, QSE_SIZEOF(tv)) <= -1)
		{
	#if defined(_WIN32)
			nwio->errnum = skerr_to_errnum (WSAGetLastError());
	#elif defined(__OS2__)
			nwio->errnum = skerr_to_errnum (sock_errno());
	#else
			nwio->errnum = skerr_to_errnum (errno);
	#endif
			return -1; /* tried to set but failed */
		}

		nwio->status |= STATUS_TMOUT_W_PRESET;
	}

	return 1; /* set successfully - don't need a multiplexer */
#endif

	return 0; /* no measn to set it */
}

int qse_nwio_init (
	qse_nwio_t* nwio, qse_mmgr_t* mmgr, const qse_nwad_t* nwad, 
	int flags, const qse_nwio_tmout_t* tmout)
{
	qse_skad_t addr;
#if defined(HAVE_SOCKLEN_T)
	socklen_t addrlen;
#else
	int addrlen;
#endif
	int family, type, tmp;

	QSE_MEMSET (nwio, 0, QSE_SIZEOF(*nwio));
	nwio->mmgr = mmgr;
	nwio->flags = flags;
	nwio->errnum = QSE_NWIO_ENOERR;
	if (tmout) nwio->tmout = *tmout;
	else
	{
		nwio->tmout.r.sec = -1;
		nwio->tmout.w.sec = -1;
		nwio->tmout.c.sec = -1;
		nwio->tmout.a.sec = -1;
	}
	
	tmp = qse_nwadtoskad (nwad, &addr);
	if (tmp <= -1) 
	{
		nwio->errnum = QSE_NWIO_EINVAL;
		return -1;
	}
	addrlen = tmp;


#if defined(SOCK_STREAM) && defined(SOCK_DGRAM)
	if (flags & QSE_NWIO_TCP) type = SOCK_STREAM;
	else if (flags & QSE_NWIO_UDP) type = SOCK_DGRAM;
	else
#endif
	{
		nwio->errnum = QSE_NWIO_EINVAL;
		return -1;
	}

	family = qse_skadfamily (&addr);

#if defined(_WIN32)
	nwio->handle = socket (family, type, 0);
	if (nwio->handle == INVALID_SOCKET)
	{
		nwio->errnum = skerr_to_errnum (WSAGetLastError());
		goto oops;
	}

	if ((flags & QSE_NWIO_TCP) && (flags & QSE_NWIO_KEEPALIVE))
	{
		int optval = 1;
		setsockopt (nwio->handle, SOL_SOCKET, SO_KEEPALIVE, (void*)&optval, QSE_SIZEOF(optval));
	}

	if (flags & QSE_NWIO_PASSIVE)
	{
		qse_nwio_hnd_t handle;

		if (flags & QSE_NWIO_REUSEADDR)
		{
			int optval = 1;
			setsockopt (nwio->handle, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, QSE_SIZEOF(optval));
		}

		if (bind (nwio->handle, (struct sockaddr*)&addr, addrlen) == SOCKET_ERROR)
		{
			nwio->errnum = skerr_to_errnum (WSAGetLastError());
			goto oops;
		}

		if (flags & QSE_NWIO_TCP)
		{
			if (listen (nwio->handle, 10) == SOCKET_ERROR)
			{
				nwio->errnum = skerr_to_errnum (WSAGetLastError());
				goto oops;
			}

			if (TMOUT_ENABLED(nwio->tmout.a) &&
			    wait_for_data (nwio, &nwio->tmout.a, 0) <= -1) goto oops;

			handle = accept (nwio->handle, (struct sockaddr*)&addr, &addrlen);
			if (handle == INVALID_SOCKET)
			{
				nwio->errnum = skerr_to_errnum (WSAGetLastError());
				goto oops;
			}

			closesocket (nwio->handle);
			nwio->handle = handle;
		}
		else if (flags & QSE_NWIO_UDP)
		{
			nwio->status |= STATUS_UDP_CONNECT;
		}
	}
	else
	{
		int xret;

		if (TMOUT_ENABLED(nwio->tmout.c) && (flags & QSE_NWIO_TCP))
		{
			unsigned long cmd = 1;

			if (ioctlsocket(nwio->handle, FIONBIO, &cmd) == SOCKET_ERROR) 
			{
				nwio->errnum = skerr_to_errnum (WSAGetLastError());
				goto oops;
			}
		}

		xret = connect (nwio->handle, (struct sockaddr*)&addr, addrlen);

		if (TMOUT_ENABLED(nwio->tmout.c) && (flags & QSE_NWIO_TCP))
		{
			unsigned long cmd = 0;
			
			if ((xret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) ||
			    ioctlsocket (nwio->handle, FIONBIO, &cmd) == SOCKET_ERROR)
			{
				nwio->errnum = skerr_to_errnum (WSAGetLastError());
				goto oops;
			}

			if (wait_for_data (nwio, &nwio->tmout.c, 1) <= -1) goto oops;
			else 
			{
				int xlen;
				DWORD xerr;

				xlen = QSE_SIZEOF(xerr);
				if (getsockopt (nwio->handle, SOL_SOCKET, SO_ERROR, (char*)&xerr, &xlen) == SOCKET_ERROR)
				{
					nwio->errnum = skerr_to_errnum (WSAGetLastError());
					goto oops;
				}
				else if (xerr != 0)
				{
					nwio->errnum = skerr_to_errnum (xerr);
					goto oops;
				}
			}
		}
		else
		{
			if (xret == SOCKET_ERROR)
			{
				nwio->errnum = skerr_to_errnum (WSAGetLastError());
				goto oops;
			}
		}
	}

#elif defined(__OS2__)
	nwio->handle = socket (family, type, 0);
	if (nwio->handle <= -1)
	{
		nwio->errnum = skerr_to_errnum (sock_errno());
		goto oops;
	}

	if ((flags & QSE_NWIO_TCP) && (flags & QSE_NWIO_KEEPALIVE))
	{
		int optval = 1;
		setsockopt (nwio->handle, SOL_SOCKET, SO_KEEPALIVE, (void*)&optval, QSE_SIZEOF(optval));
	}

	if (flags & QSE_NWIO_PASSIVE)
	{
		qse_nwio_hnd_t handle;

		if (flags & QSE_NWIO_REUSEADDR)
		{
			int optval = 1;
			setsockopt (nwio->handle, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, QSE_SIZEOF(optval));
		}

		if (bind (nwio->handle, (struct sockaddr*)&addr, addrlen) <= -1)
		{
			nwio->errnum = skerr_to_errnum (sock_errno());
			goto oops;
		}

		if (flags & QSE_NWIO_TCP)
		{
			if (listen (nwio->handle, 10) <= -1)
			{
				nwio->errnum = skerr_to_errnum (sock_errno());
				goto oops;
			}

			if (TMOUT_ENABLED(nwio->tmout.a) &&
			    wait_for_data (nwio, &nwio->tmout.a, 0) <= -1) goto oops;

			handle = accept (nwio->handle, (struct sockaddr*)&addr, &addrlen);
			if (handle <= -1)
			{
				nwio->errnum = skerr_to_errnum (sock_errno());
				goto oops;
			}

			soclose (nwio->handle);
			nwio->handle = handle;
		}
		else if (flags & QSE_NWIO_UDP)
		{
			nwio->status |= STATUS_UDP_CONNECT;
		}
	}
	else
	{
		int xret;

		if (TMOUT_ENABLED(nwio->tmout.c) && (flags & QSE_NWIO_TCP))
		{
			int noblk = 1;

			if (ioctl (nwio->handle, FIONBIO, (void*)&noblk, QSE_SIZEOF(noblk)) <= -1)
			{
				nwio->errnum = skerr_to_errnum (sock_errno());
				goto oops;
			}
		}

		xret = connect (nwio->handle, (struct sockaddr*)&addr, addrlen);

		if (TMOUT_ENABLED(nwio->tmout.c) && (flags & QSE_NWIO_TCP))
		{
			int noblk = 0;
			
			if ((xret <= -1 && sock_errno() != SOCEINPROGRESS) ||
			    ioctl (nwio->handle, FIONBIO, (void*)&noblk, QSE_SIZEOF(noblk)) <= -1)
			{
				nwio->errnum = skerr_to_errnum (sock_errno());
				goto oops;
			}

			if (wait_for_data (nwio, &nwio->tmout.c, 1) <= -1) goto oops;
			else 
			{
				int xlen, xerr;

				xlen = QSE_SIZEOF(xerr);
				if (getsockopt (nwio->handle, SOL_SOCKET, SO_ERROR, (char*)&xerr, &xlen) <= -1)
				{
					nwio->errnum = skerr_to_errnum (sock_errno());
					goto oops;
				}
				else if (xerr != 0)
				{
					nwio->errnum = skerr_to_errnum (xerr);
					goto oops;
				}
			}
		}
		else
		{
			if (xret <= -1)
			{
				nwio->errnum = skerr_to_errnum (sock_errno());
				goto oops;
			}
		}
	}

#elif defined(__DOS__)
	nwio->errnum = QSE_NWIO_ENOIMPL;
	return -1;

#else
	#if defined(SOCK_CLOEXEC)
	nwio->handle = socket (family, type | SOCK_CLOEXEC, 0);
	#else
	nwio->handle = socket (family, type, 0);
	#endif
	if (nwio->handle <= -1)
	{
		nwio->errnum = skerr_to_errnum (errno);
		goto oops;
	}

	#if !defined(SOCK_CLOEXEC) && defined(FD_CLOEXEC)
	{ 
		int tmp = fcntl (nwio->handle, F_GETFD);
		if (tmp >= 0) fcntl (nwio->handle, F_SETFD, tmp | FD_CLOEXEC);
	}
	#endif

	if ((flags & QSE_NWIO_TCP) && (flags & QSE_NWIO_KEEPALIVE))
	{
		int optval = 1;
		setsockopt (nwio->handle, SOL_SOCKET, SO_KEEPALIVE, (void*)&optval, QSE_SIZEOF(optval));
	}

	if (flags & QSE_NWIO_PASSIVE)
	{
		qse_nwio_hnd_t handle;

	#if defined(SO_REUSEADDR)
		if (flags & QSE_NWIO_REUSEADDR)
		{
			int optval = 1;
			setsockopt (nwio->handle, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, QSE_SIZEOF(optval));
		}
	#endif

		if (bind (nwio->handle, (struct sockaddr*)&addr, addrlen) <= -1)
		{
			nwio->errnum = skerr_to_errnum (errno);
			goto oops;
		}

		if (flags & QSE_NWIO_TCP)
		{
			if (listen (nwio->handle, 10) <= -1)
			{
				nwio->errnum = skerr_to_errnum (errno);
				goto oops;
			}

			if (TMOUT_ENABLED(nwio->tmout.a) &&
			    wait_for_data (nwio, &nwio->tmout.a, 0) <= -1) goto oops;

			handle = accept (nwio->handle, (struct sockaddr*)&addr, &addrlen);
			if (handle <= -1)
			{
				nwio->errnum = skerr_to_errnum (errno);
				goto oops;
			}

			QSE_CLOSE (nwio->handle);
			nwio->handle = handle;
		}
		else if (flags & QSE_NWIO_UDP)
		{
			nwio->status |= STATUS_UDP_CONNECT;
		}
	}
	else
	{
		int xret;
		
		if (TMOUT_ENABLED(nwio->tmout.c) && (flags & QSE_NWIO_TCP))
		{
			int orgfl;

			orgfl = fcntl (nwio->handle, F_GETFL, 0);
			if (orgfl <= -1 ||
			    fcntl (nwio->handle, F_SETFL, orgfl | O_NONBLOCK) <= -1)
			{
				nwio->errnum = skerr_to_errnum (errno);
				goto oops;
			}
		
			xret = connect (nwio->handle, (struct sockaddr*)&addr, addrlen);
		
			if ((xret <= -1 && errno != EINPROGRESS) ||
			    fcntl (nwio->handle, F_SETFL, orgfl) <= -1) 
			{
				nwio->errnum = skerr_to_errnum (errno);
				goto oops;
			}

			if (wait_for_data (nwio, &nwio->tmout.c, 1) <= -1) goto oops;
			else 
			{
			#if defined(HAVE_SOCKLEN_T)
				socklen_t xlen;
			#else
				int xlen;
			#endif
				xlen = QSE_SIZEOF(xret);
				if (getsockopt (nwio->handle, SOL_SOCKET, SO_ERROR, (char*)&xret, &xlen) <= -1)
				{
					nwio->errnum = skerr_to_errnum (errno);
					goto oops;
				}
				else if (xret != 0)
				{
					nwio->errnum = skerr_to_errnum (xret);
					goto oops;
				}
			}
		}
		else
		{
			xret = connect (nwio->handle, (struct sockaddr*)&addr, addrlen);
			if (xret <= -1)
			{	
				nwio->errnum = skerr_to_errnum (errno);
				goto oops;
			}
		}
	}
#endif

	if (flags & QSE_NWIO_TEXT)
	{
		int topt = 0;

		if (flags & QSE_NWIO_IGNOREMBWCERR) topt |= QSE_TIO_IGNOREMBWCERR;
		if (flags & QSE_NWIO_NOAUTOFLUSH) topt |= QSE_TIO_NOAUTOFLUSH;

		nwio->tio = qse_tio_open (mmgr, QSE_SIZEOF(qse_nwio_t*), topt);
		if (nwio->tio == QSE_NULL)
		{
			nwio->errnum = QSE_NWIO_ENOMEM;
			goto oops;
		}

		/* store the back-reference to nwio in the extension area.*/
		*(qse_nwio_t**)QSE_XTN(nwio->tio) = nwio;

		if (qse_tio_attachin (nwio->tio, socket_input, QSE_NULL, 4096) <= -1 ||
		    qse_tio_attachout (nwio->tio, socket_output, QSE_NULL, 4096) <= -1)
		{
			if (nwio->errnum == QSE_NWIO_ENOERR) 
				nwio->errnum = tio_errnum_to_nwio_errnum (nwio->tio);
			goto oops;
		}
	}

	preset_tmout (nwio);
	return 0;

oops:
	if (nwio->tio) 
	{
		qse_tio_close (nwio->tio);
		nwio->tio = QSE_NULL;
	}

#if defined(_WIN32)
	if (nwio->handle != INVALID_SOCKET) closesocket (nwio->handle);

#elif defined(__OS2__)
	if (nwio->handle >= 0) soclose (nwio->handle);

#elif defined(__DOS__)
	/* TODO: */

#else
	if (nwio->handle >= 0) QSE_CLOSE (nwio->handle);
#endif
	return -1;
}

void qse_nwio_fini (qse_nwio_t* nwio)
{
	/*if (qse_nwio_flush (nwio) <= -1) return -1;*/
	qse_nwio_flush (nwio);
	if (nwio->tio)
	{
		qse_tio_close (nwio->tio);
		nwio->tio = QSE_NULL;
	}

#if defined(_WIN32)
	closesocket (nwio->handle);
#elif defined(__OS2__)
	/* TODO: */
#elif defined(__DOS__)
	/* TODO: */
#else
	QSE_CLOSE (nwio->handle);
#endif
}

qse_mmgr_t* qse_nwio_getmmgr (qse_nwio_t* nwio)
{
	return nwio->mmgr;
}

void* qse_nwio_getxtn (qse_nwio_t* nwio)
{
	return QSE_XTN (nwio);
}

qse_nwio_errnum_t qse_nwio_geterrnum (const qse_nwio_t* nwio)
{
	return nwio->errnum;
}

qse_cmgr_t* qse_nwio_getcmgr (qse_nwio_t* nwio)
{
	return nwio->tio? qse_tio_getcmgr (nwio->tio): QSE_NULL;
}

void qse_nwio_setcmgr (qse_nwio_t* nwio, qse_cmgr_t* cmgr)
{
	if (nwio->tio) qse_tio_setcmgr (nwio->tio, cmgr);
}

qse_nwio_hnd_t qse_nwio_gethandle (const qse_nwio_t* nwio)
{
	return nwio->handle;
}

qse_ubi_t qse_nwio_gethandleasubi (const qse_nwio_t* nwio)
{
	qse_ubi_t ubi;

#if defined(_WIN32)
	ubi.intptr = nwio->handle;
#elif defined(__OS2__)
	ubi.i = nwio->handle;
#elif defined(__DOS__)
	ubi.i = nwio->handle;
#else
	ubi.i = nwio->handle;
#endif
	return ubi;
}

qse_ssize_t qse_nwio_flush (qse_nwio_t* nwio)
{
	qse_ssize_t n;

	if (nwio->tio)
	{
		nwio->errnum = QSE_NWIO_ENOERR;
		n = qse_tio_flush (nwio->tio);
		if (n <= -1 && nwio->errnum == QSE_NWIO_ENOERR) 
			nwio->errnum = tio_errnum_to_nwio_errnum (nwio->tio);
	}
	else n = 0;

	return n;
}

void qse_nwio_purge (qse_nwio_t* nwio)
{
	if (nwio->tio) qse_tio_purge (nwio->tio);
}

/* ---------------------------------------------------------- */

static qse_ssize_t nwio_read (qse_nwio_t* nwio, void* buf, qse_size_t size)
{
#if defined(_WIN32)
	int count;
#elif defined(__OS2__)
	int n;
#elif defined(__DOS__)
	int n;
#else
	qse_ssize_t n;
#endif

#if defined(_WIN32)
	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(int);

	if (nwio->status & STATUS_UDP_CONNECT)
	{
		qse_skad_t addr;
		int addrlen;

		addrlen = QSE_SIZEOF(addr);

		if (TMOUT_ENABLED(nwio->tmout.a) &&
		    wait_for_data (nwio, &nwio->tmout.a, 0) <= -1) return -1;

		count = recvfrom (
			nwio->handle, buf, size, 0, 
			(struct sockaddr*)&addr, &addrlen);
		if (count == SOCKET_ERROR) 
		{
			nwio->errnum = skerr_to_errnum (WSAGetLastError());
		}
		else if (count >= 1)
		{
			/* for udp, it just creates a stream with the
			 * first sender */
			if (connect (nwio->handle, (struct sockaddr*)&addr, addrlen) <= -1)
			{
				nwio->errnum = skerr_to_errnum (WSAGetLastError());
				return -1;			
			}
			nwio->status &= ~STATUS_UDP_CONNECT;
		}
	}
	else
	{
		if (!(nwio->status & STATUS_TMOUT_R_PRESET) && 
		    TMOUT_ENABLED(nwio->tmout.r) &&
		    wait_for_data (nwio, &nwio->tmout.r, 0) <= -1) return -1;

		count = recv (nwio->handle, buf, size, 0);
		if (count == SOCKET_ERROR) nwio->errnum = skerr_to_errnum (WSAGetLastError());
	}

	return count;

#elif defined(__OS2__)
	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(int);

	if (nwio->status & STATUS_UDP_CONNECT)
	{
		qse_skad_t addr;
		int addrlen;

		addrlen = QSE_SIZEOF(addr);

		if (TMOUT_ENABLED(nwio->tmout.a) &&
		    wait_for_data (nwio, &nwio->tmout.a, 0) <= -1) return -1;

		n = recvfrom (
			nwio->handle, buf, size, 0, 
			(struct sockaddr*)&addr, &addrlen);
		if (n <= -1) 
		{
			nwio->errnum = skerr_to_errnum (sock_errno());
		}
		else if (n >= 1)
		{
			/* for udp, it just creates a stream with the
			 * first sender */
			if (connect (nwio->handle, (struct sockaddr*)&addr, addrlen) <= -1)
			{
				nwio->errnum = skerr_to_errnum (sock_errno());
				return -1;			
			}
			nwio->status &= ~STATUS_UDP_CONNECT;
		}
	}
	else
	{
		if (!(nwio->status & STATUS_TMOUT_R_PRESET) && 
		    TMOUT_ENABLED(nwio->tmout.r) &&
		    wait_for_data (nwio, &nwio->tmout.r, 0) <= -1) return -1;

		n = recv (nwio->handle, buf, size, 0);
		if (n <= -1) nwio->errnum = skerr_to_errnum (sock_errno());
	}

	return n;

#elif defined(__DOS__)

	nwio->errnum = QSE_NWIO_ENOIMPL;
	return -1;

#else

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);

reread:
	if (nwio->status & STATUS_UDP_CONNECT)
	{
		qse_skad_t addr;
#if defined(HAVE_SOCKLEN_T)
		socklen_t addrlen;
#else
		int addrlen;
#endif

		addrlen = QSE_SIZEOF(addr);

		/* it's similar to accept for tcp because i'm expecting 
		 * the first sender and call connect() to it below just
		 * like the 'nc' utility does.
		 * so i treat this recvfrom() as if it is accept(). 
		 */
		if (TMOUT_ENABLED(nwio->tmout.a) &&
		    wait_for_data (nwio, &nwio->tmout.a, 0) <= -1) return -1;

		n = recvfrom (
			nwio->handle, buf, size, 0, 
			(struct sockaddr*)&addr, &addrlen);
		if (n <= -1) 
		{
			if (errno == EINTR)
			{
				if (nwio->flags & QSE_NWIO_READNORETRY) 
					nwio->errnum = QSE_NWIO_EINTR;
				else goto reread;
			}
			else
			{
				nwio->errnum = skerr_to_errnum (errno);
			}
		}
		else if (n >= 1)
		{
			/* for udp, it just creates a stream with the
			 * first sender */
			if (connect (nwio->handle, (struct sockaddr*)&addr, addrlen) <= -1)
			{
				nwio->errnum = skerr_to_errnum (errno);
				return -1;			
			}
			nwio->status &= ~STATUS_UDP_CONNECT;
		}
	}
	else
	{
		if (!(nwio->status & STATUS_TMOUT_R_PRESET) && 
		    TMOUT_ENABLED(nwio->tmout.r) &&
		    wait_for_data (nwio, &nwio->tmout.r, 0) <= -1) return -1;

		n = recv (nwio->handle, buf, size, 0);
		if (n <= -1) 
		{
			if (errno == EINTR)
			{
				if (nwio->flags & QSE_NWIO_READNORETRY) 
					nwio->errnum = QSE_NWIO_EINTR;
				else goto reread;
			}
			else
			{
				nwio->errnum = skerr_to_errnum (errno);
			}
		}
	}

	return n;
#endif
}

qse_ssize_t qse_nwio_read (qse_nwio_t* nwio, void* buf, qse_size_t size)
{
	if (nwio->tio == QSE_NULL) 
		return nwio_read (nwio, buf, size);
	else
	{
		qse_ssize_t n;

		nwio->errnum = QSE_NWIO_ENOERR;
		n = qse_tio_read (nwio->tio, buf, size);
		if (n <= -1 && nwio->errnum == QSE_NWIO_ENOERR) 
			nwio->errnum = tio_errnum_to_nwio_errnum (nwio->tio);

		return n;
	}
}

static qse_ssize_t nwio_write (qse_nwio_t* nwio, const void* data, qse_size_t size)
{
#if defined(_WIN32)
	int count;
#elif defined(__OS2__)
	int n;
#elif defined(__DOS__)
	int n;
#else
	qse_ssize_t n;
#endif

#if defined(_WIN32)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(int);

	if (!(nwio->status & STATUS_TMOUT_W_PRESET) && 
	    TMOUT_ENABLED(nwio->tmout.w) &&
	    wait_for_data (nwio, &nwio->tmout.w, 1) <= -1) return -1;

	count = send (nwio->handle, data, size, 0);
	if (count == SOCKET_ERROR) nwio->errnum = skerr_to_errnum (WSAGetLastError());
	return count;

#elif defined(__OS2__)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(int);

	if (!(nwio->status & STATUS_TMOUT_W_PRESET) && 
	    TMOUT_ENABLED(nwio->tmout.w) &&
	    wait_for_data (nwio, &nwio->tmout.w, 1) <= -1) return -1;

	n = send (nwio->handle, data, size, 0);
	if (n <= -1) nwio->errnum = skerr_to_errnum (sock_errno());
	return n;

#elif defined(__DOS__)

	nwio->errnum = QSE_NWIO_ENOIMPL;
	return -1;

#else

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);

rewrite:
	if (!(nwio->status & STATUS_TMOUT_W_PRESET) && 
	    TMOUT_ENABLED(nwio->tmout.w) &&
	    wait_for_data (nwio, &nwio->tmout.w, 1) <= -1) return -1;

	n = send (nwio->handle, data, size, 0);
	if (n <= -1) 
	{
		if (errno == EINTR)
		{
			if (nwio->flags & QSE_NWIO_WRITENORETRY)
				nwio->errnum = QSE_NWIO_EINTR;
			else goto rewrite;
		}
		else
		{
			nwio->errnum = skerr_to_errnum (errno);
		}
	}
	return n;

#endif
}

qse_ssize_t qse_nwio_write (qse_nwio_t* nwio, const void* data, qse_size_t size)
{
	if (nwio->tio == QSE_NULL)
	{
		return nwio_write (nwio, data, size);
	}
	else
	{
		qse_ssize_t n;

		nwio->errnum = QSE_NWIO_ENOERR;	
		n = qse_tio_write (nwio->tio, data, size);
		if (n <= -1 && nwio->errnum == QSE_NWIO_ENOERR) 
			nwio->errnum = tio_errnum_to_nwio_errnum (nwio->tio);

		return n;
	}
}

/* ---------------------------------------------------------- */

static qse_ssize_t socket_input (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size)
{
	if (cmd == QSE_TIO_DATA) 
	{
		qse_nwio_t* nwio;

		nwio = *(qse_nwio_t**)QSE_XTN(tio);
		QSE_ASSERT (nwio != QSE_NULL);

		return nwio_read (nwio, buf, size);
	}

	return 0;
}

static qse_ssize_t socket_output (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size)
{
	if (cmd == QSE_TIO_DATA) 
	{
		qse_nwio_t* nwio;

		nwio = *(qse_nwio_t**)QSE_XTN(tio);
		QSE_ASSERT (nwio != QSE_NULL);

		return nwio_write (nwio, buf, size);
	}

	return 0;
}
