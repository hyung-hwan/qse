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

#include <qse/si/Socket.hpp>
#include <qse/cmn/str.h>
#include "../cmn/mem-prv.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

#include "../cmn/syserr.h"
IMPLEMENT_SYSERR_TO_ERRNUM (Socket::ErrorCode, Socket::)

Socket::Socket () QSE_CPP_NOEXCEPT: handle(QSE_INVALID_SCKHND), errcode(E_ENOERR)
{
}

Socket::~Socket () QSE_CPP_NOEXCEPT
{
	this->close ();
}

#if 0
int Socket::fdopen (int handle) QSE_CPP_NOEXCEPT
{
	this->handle = handle;
}
#endif

int Socket::open (int domain, int type, int protocol, int traits) QSE_CPP_NOEXCEPT
{
	int x;
	bool fcntl_v = 0;

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
	if (traits & Socket::T_NONBLOCK) type |= SOCK_NONBLOCK;
	if (traits & Socket::T_CLOEXEC) type |= SOCK_CLOEXEC;
open_socket:
#endif
	x = ::socket (domain, type, protocol);
	if (x == -1)
	{
	#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
		if (errno == EINVAL && (type & (SOCK_NONBLOCK | SOCK_CLOEXEC)))
		{
			type &= ~(SOCK_NONBLOCK | SOCK_CLOEXEC);
			if (traits & Socket::T_NONBLOCK ) fcntl_v |= O_NONBLOCK;
			#if defined(O_CLOEXEC)
			if (traits & Socket::T_CLOEXEC) fcntl_v |= O_CLOEXEC;
			#endif
			goto open_socket;
		}
	#endif
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
	// do nothing
#else
	if (traits & Socket::T_NONBLOCK ) fcntl_v |= O_NONBLOCK;
	#if defined(O_CLOEXEC)
	if (traits & O_CLOEXEC) fcntl_v |= O_CLOEXEC;
	#endif
#endif

	if (fcntl_v)
	{
		int fl = fcntl(x, F_GETFL, 0);
		if (fl == -1)
		{
		fcntl_failure:
			this->setErrorCode (syserr_to_errnum(errno));
			::close (x);
			x = -1;
		}
		else
		{
			if (fcntl(x, F_SETFL, fl | fcntl_v) == -1) goto fcntl_failure;
		}
	}

	this->close ();
	this->handle = x;
	return 0;
}

void Socket::close () QSE_CPP_NOEXCEPT
{
	if (this->handle != QSE_INVALID_SCKHND)
	{
		qse_closesckhnd (this->handle);
		this->handle = QSE_INVALID_SCKHND;
	}
}

int Socket::getOption (int level, int optname, void* optval, qse_sck_len_t* optlen) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);
	int n = ::getsockopt (this->handle, level, optname, (char*)optval, optlen);
	if (n == -1) this->setErrorCode (syserr_to_errnum(errno));
	return n;
}

int Socket::setOption (int level, int optname, const void* optval, qse_sck_len_t optlen) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);
	int n = ::setsockopt (this->handle, level, optname, (const char*)optval, optlen);
	if (n == -1) this->setErrorCode (syserr_to_errnum(errno));
	return n;
}

int Socket::setDebug (int n) QSE_CPP_NOEXCEPT
{
	return this->setOption (SOL_SOCKET, SO_DEBUG, (char*)&n, QSE_SIZEOF(n));
};

int Socket::setReuseAddr (int n) QSE_CPP_NOEXCEPT
{
	return this->setOption (SOL_SOCKET, SO_REUSEADDR, (char*)&n, QSE_SIZEOF(n));
}

int Socket::setReusePort (int n) QSE_CPP_NOEXCEPT
{
#if defined(SO_REUSEPORT)
	return this->setOption (SOL_SOCKET, SO_REUSEPORT, (char*)&n, QSE_SIZEOF(n));
#else
	this->setErrorCode (E_ENOIMPL);
	return -1;
#endif
}

int Socket::setKeepAlive (int n, int keepidle, int keepintvl, int keepcnt) QSE_CPP_NOEXCEPT
{
	if (this->setOption (SOL_SOCKET, SO_KEEPALIVE, (char*)&n, QSE_SIZEOF(n)) <= -1) return -1;

	// the following values are just hints. 
	// i don't care about success and failure
#if defined(TCP_KEEPIDLE) && defined(SOL_TCP)
	if (keepidle > 0) this->setOption (SOL_TCP, TCP_KEEPIDLE, (char*)&keepidle, QSE_SIZEOF(keepidle));
#endif
#if defined(TCP_KEEPINTVL) && defined(SOL_TCP)
	if (keepintvl > 0) this->setOption (SOL_TCP, TCP_KEEPINTVL, (char*)&keepintvl, QSE_SIZEOF(keepintvl));
#endif
#if defined(TCP_KEEPCNT) && defined(SOL_TCP)
	if (keepcnt > 0) this->setOption (SOL_TCP, TCP_KEEPCNT, (char*)&keepcnt, QSE_SIZEOF(keepcnt));
#endif
	return 0;
}

int Socket::setBroadcast (int n) QSE_CPP_NOEXCEPT
{
	return this->setOption (SOL_SOCKET, SO_BROADCAST, (char*)&n, QSE_SIZEOF(n));
}

int Socket::setSendBuf (unsigned int size)  QSE_CPP_NOEXCEPT
{
	return this->setOption (SOL_SOCKET, SO_SNDBUF, (char*)&size, QSE_SIZEOF(size));
}

int Socket::setRecvBuf (unsigned int size) QSE_CPP_NOEXCEPT
{
	return this->setOption (SOL_SOCKET, SO_RCVBUF, (char*)&size, QSE_SIZEOF(size));
}

int Socket::setLingerOn (int sec) QSE_CPP_NOEXCEPT
{
	struct linger lng;
	lng.l_onoff = 1;
	lng.l_linger = sec;
	return this->setOption (SOL_SOCKET, SO_LINGER, (char*)&lng, QSE_SIZEOF(lng));
}

int Socket::setLingerOff () QSE_CPP_NOEXCEPT
{
	struct linger lng;
	lng.l_onoff = 0;
	lng.l_linger = 0;
	return this->setOption (SOL_SOCKET, SO_LINGER, (char*)&lng, QSE_SIZEOF(lng));
}

int Socket::setTcpNodelay (int n) QSE_CPP_NOEXCEPT
{
#if defined(TCP_NODELAY)
	return this->setOption (IPPROTO_TCP, TCP_NODELAY, (char*)&n, QSE_SIZEOF(n));
#else
	this->setErrorCode (E_ENOIMPL);
	return -1;
#endif
}

int Socket::setOobInline (int n) QSE_CPP_NOEXCEPT
{
#if defined(SO_OOBINLINE)
	return this->setOption (SOL_SOCKET, SO_OOBINLINE, (char*)&n, QSE_SIZEOF(n));
#else
	this->setErrorCode (E_ENOIMPL);
	return -1;
#endif
}

int Socket::setIpv6Only (int n) QSE_CPP_NOEXCEPT
{
#if defined(IPV6_V6ONLY)
	return this->setOption (IPPROTO_IPV6, IPV6_V6ONLY, (char*)&n, QSE_SIZEOF(n));
#else
	this->setErrorCode (E_ENOIMPL);
	return -1;
#endif
}

int Socket::shutdown (int how) QSE_CPP_NOEXCEPT
{
	if (this->handle != QSE_INVALID_SCKHND)
	{
		// i put this guard to allow multiple calls to shutdown().
		if (::shutdown(this->handle, how) == -1)
		{
			this->setErrorCode (syserr_to_errnum(errno));
			return -1;
		}
	}

	return 0;
}

int Socket::connect (const SocketAddress& target) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	if (::connect(this->handle, (struct sockaddr*)target.getAddrPtr(), target.getAddrSize()) == -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

	return 0;
}

int Socket::bind (const SocketAddress& target) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	if (::bind(this->handle, (struct sockaddr*)target.getAddrPtr(), target.getAddrSize()) == -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

	return 0;
}

int Socket::listen (int backlog) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	if (::listen(this->handle, backlog) == -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

	return 0;
}

int Socket::accept (Socket* newsck, SocketAddress* newaddr, int traits) QSE_CPP_NOEXCEPT
{
	int newfd, flag_v;
	qse_sklen_t addrlen;

	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC) && defined(HAVE_ACCEPT4)

	flag_v = 0;
	if (traits & Socket::T_NONBLOCK ) flag_v |= SOCK_NONBLOCK;
	if (traits & Socket::T_CLOEXEC) flag_v |= SOCK_CLOEXEC;

	addrlen = newaddr->getAddrCapa();
	newfd = ::accept4(this->handle, (struct sockaddr*)newaddr->getAddrPtr(), &addrlen, flag_v);
	if (newfd == -1)
	{
		if (errno != ENOSYS)
		{
			this->setErrorCode (syserr_to_errnum(errno));
			return -1;
		}

		// go on for the normal 3-parameter accept
	}
	else
	{
		goto accept_done;
	}
#endif

	addrlen = newaddr->getAddrCapa();
	newfd = ::accept(this->handle, (struct sockaddr*)newaddr->getAddrPtr(), &addrlen);
	if (newfd == -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

	flag_v = 0;
	if (traits & Socket::T_NONBLOCK ) flag_v |= O_NONBLOCK;
	#if defined(O_CLOEXEC)
	if (traits & Socket::T_CLOEXEC) flag_v |= O_CLOEXEC;
	#endif

	if (flag_v)
	{
		int fl = ::fcntl(newfd, F_GETFL, 0);
		if (fl == -1)
		{
		fcntl_failure:
			this->setErrorCode (syserr_to_errnum(errno));
			::close (newfd);
			return -1;
		}
		else
		{
			if (::fcntl(newfd, F_SETFL, fl | flag_v) == -1) goto fcntl_failure;
		}
	}

accept_done:
	newsck->handle = newfd;
	return 0;
}

qse_ssize_t Socket::send (const void* buf, qse_size_t len) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	qse_ssize_t n = ::send(this->handle, buf, len, 0);
	if (n == -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

	return n; 
}

qse_ssize_t Socket::send (const void* buf, qse_size_t len, const SocketAddress& dstaddr) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	qse_ssize_t n = ::sendto(this->handle, buf, len, 0, (struct sockaddr*)dstaddr.getAddrPtr(), dstaddr.getAddrSize());
	if (n == -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

	return n; 
}

int Socket::sendx (const void* buf, qse_size_t len, qse_size_t* total_sent) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	qse_size_t pos = 0;

	while (pos < len)
	{
		qse_ssize_t n = ::send(this->handle, (char*)buf + pos, len - pos, 0);
		if (n <= -1)
		{
			this->setErrorCode (syserr_to_errnum(errno));
			if (total_sent) *total_sent = pos;
			return -1;
		}

		pos += n;
	}

	if (total_sent) *total_sent = pos;
	return 0; 
}

int Socket::sendx (const void* buf, qse_size_t len, const SocketAddress& dstaddr, qse_size_t* total_sent) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	qse_size_t pos = 0;

	while (pos < len)
	{
		qse_ssize_t n = ::sendto(this->handle, (char*)buf + pos, len - pos, 0, (struct sockaddr*)dstaddr.getAddrPtr(), dstaddr.getAddrSize());
		if (n == -1)
		{
			this->setErrorCode (syserr_to_errnum(errno));
			if (total_sent) *total_sent = pos;
			return -1;
		}

		pos += n;
	}

	if (total_sent) *total_sent = pos;
	return 0; 
}

int Socket::sendx (qse_ioptl_t* iov, int count, qse_size_t* total_sent) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

#if defined(HAVE_SENDMSG) || defined(HAVE_WRITEV)
	int index = 0;
	qse_size_t total = 0;
	int backup_index = -1;
	qse_ioptl_t backup;

	#if defined(HAVE_SENDMSG)
	struct msghdr msg;
	QSE_MEMSET (&msg, 0, QSE_SIZEOF(msg));
	#endif

	while (1)
	{
		ssize_t nwritten;

	#if defined(HAVE_SENDMSG)
		msg.msg_iov = (struct iovec*)&iov[index];
		msg.msg_iovlen = count - index;
		nwritten = ::sendmsg(this->handle, &msg, 0);
	#else
		nwritten = ::writev(this->handle, (const struct iovec*)&iov[index], count - index);
	#endif
		if (nwritten <= -1)
		{
			this->setErrorCode (syserr_to_errnum(errno));
			if (backup_index >= 0) iov[backup_index] = backup;
			if (total_sent) *total_sent = total; 
			return -1;
		}

		total += nwritten;

		while (index < count && (qse_size_t)nwritten >= iov[index].len)
			nwritten -= iov[index++].len;

		if (index == count) break;

		if (backup_index != index)
		{
			if (backup_index >= 0) iov[backup_index] = backup;
			backup = iov[index];
			backup_index = index;
		}

		iov[index].ptr = (void*)((qse_uint8_t*)iov[index].ptr + nwritten);
		iov[index].len -= nwritten;
	}

	if (backup_index >= 0) iov[backup_index] = backup;
	if (total_sent) *total_sent = total; 
	return 0;

#else
	qse_ioptl_t* v, * ve;
	qse_size_t total = 0, pos, rem;
	ssize_t nwritten;

	v = iov;
	ve = v + count;

	while (v < ve)
	{
		if (v->len <= 0) 
		{
			v++;
			continue;
		}

		pos = 0;
		rem = v->len;
	write_again:
		nwritten = ::send(this->handle, (qse_uint8_t*)v->ptr + pos, rem, 0);
		if (nwritten <= -1) 
		{
			this->setErrorCode (syserr_to_errnum(errno));
			if (total_sent) *total_sent = total; 
			return -1;
		}

		total += nwritten;
		if ((qse_size_t)nwritten < rem)
		{
			pos += nwritten;
			rem -= nwritten;
			goto write_again;
		}

		v++;
	}

	if (total_sent) *total_sent = total; 
	return 0;
#endif
}

qse_ssize_t Socket::receive (void* buf, qse_size_t len) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	qse_ssize_t n = ::recv(this->handle, buf, len, 0);
	if (n == -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

	return n; 
}

qse_ssize_t Socket::receive (void* buf, qse_size_t len, SocketAddress& srcaddr) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	qse_sklen_t addrlen = srcaddr.getAddrCapa();
	qse_ssize_t n = ::recvfrom(this->handle, buf, len, 0, (struct sockaddr*)srcaddr.getAddrPtr(), &addrlen);
	if (n == -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

	return n; 
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
