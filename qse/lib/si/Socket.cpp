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

#include <qse/si/Socket.hpp>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include "../cmn/mem-prv.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h> // strerror

#if defined(HAVE_NET_IF_H)
#	include <net/if.h>
#endif

#if defined(HAVE_SYS_IOCTL_H)
#	include <sys/ioctl.h>
#endif

#if defined(HAVE_IFADDRS_H)
#	include <ifaddrs.h>
#endif

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

#include "../cmn/syserr.h"
IMPLEMENT_SYSERR_TO_ERRNUM (Socket::ErrorNumber, Socket::)

Socket::Socket () QSE_CPP_NOEXCEPT: handle(QSE_INVALID_SCKHND), domain(-1)
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
	int fcntl_v = 0;

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
	if (traits & Socket::T_NONBLOCK) type |= SOCK_NONBLOCK;
	if (traits & Socket::T_CLOEXEC) type |= SOCK_CLOEXEC;
open_socket:
#endif
	x = ::socket(domain, type, protocol);
	if (x == -1)
	{
	#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
		if (errno == EINVAL && (type & (SOCK_NONBLOCK | SOCK_CLOEXEC)))
		{
			type &= ~(SOCK_NONBLOCK | SOCK_CLOEXEC);
			goto open_socket;
		}
	#endif
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}
	else
	{
	#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
		if (type & (SOCK_NONBLOCK | SOCK_CLOEXEC)) goto done;
	#endif
	}

	if (traits)
	{
		fcntl_v = ::fcntl(x, F_GETFL, 0);
		if (fcntl_v == -1)
		{
		fcntl_failure:
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
			::close (x);
			return -1;
		}

		if (traits & Socket::T_NONBLOCK) fcntl_v |= O_NONBLOCK;
		else fcntl_v &= ~O_NONBLOCK;

	#if defined(FD_CLOEXEC)
		if (traits & Socket::T_CLOEXEC) fcntl_v |= FD_CLOEXEC;
		else fcntl_v &= ~FD_CLOEXEC;
	#endif

		if (::fcntl(x, F_SETFL, fcntl_v) == -1) goto fcntl_failure;
	}

done:
	this->close (); // close the existing handle if open.
	this->handle = x;

	// while it seems to be possible to get the domain value from a socket
	// descriptor with getsockopt(SO_DOMAIN), the method doesn't seem universal.
	//
	//   SO_DOMAIN (since Linux 2.6.32)
	//     Retrieves the socket domain as an  integer,  returning  a  value
	//     such  as  AF_INET6.   See  socket(2)  for  details.  This socket
	//     option is read-only.
	//
	// let me just store the information in the class 

	this->domain = domain;
	return 0;
}

void Socket::close () QSE_CPP_NOEXCEPT
{
	if (this->handle != QSE_INVALID_SCKHND)
	{
		qse_close_sck (this->handle);
		this->handle = QSE_INVALID_SCKHND;
		this->domain = -1;
	}
}

int Socket::getSockName (SocketAddress& addr) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));
	qse_sck_len_t len = addr.getAddrCapa();
	int n = ::getsockname(this->handle, (struct sockaddr*)addr.getAddrPtr(), &len);
	if (n == -1) this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
	return n;
}

int Socket::getPeerName (SocketAddress& addr) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));
	qse_sck_len_t len = addr.getAddrCapa();
	int n = ::getpeername(this->handle, (struct sockaddr*)addr.getAddrPtr(), &len);
	if (n == -1) this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
	return n;
}

int Socket::getOption (int level, int optname, void* optval, qse_sck_len_t* optlen) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));
	int n = ::getsockopt(this->handle, level, optname, (char*)optval, optlen);
	if (n == -1) this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
	return n;
}

int Socket::setOption (int level, int optname, const void* optval, qse_sck_len_t optlen) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));
	int n = ::setsockopt(this->handle, level, optname, (const char*)optval, optlen);
	if (n == -1) this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
	return n;
}

int Socket::setDebug (int n) QSE_CPP_NOEXCEPT
{
	return this->setOption(SOL_SOCKET, SO_DEBUG, (char*)&n, QSE_SIZEOF(n));
};

int Socket::setReuseAddr (int n) QSE_CPP_NOEXCEPT
{
	return this->setOption(SOL_SOCKET, SO_REUSEADDR, (char*)&n, QSE_SIZEOF(n));
}

int Socket::setReusePort (int n) QSE_CPP_NOEXCEPT
{
#if defined(SO_REUSEPORT)
	return this->setOption(SOL_SOCKET, SO_REUSEPORT, (char*)&n, QSE_SIZEOF(n));
#else
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif
}

int Socket::setKeepAlive (int n, int keepidle, int keepintvl, int keepcnt) QSE_CPP_NOEXCEPT
{
	if (this->setOption(SOL_SOCKET, SO_KEEPALIVE, (char*)&n, QSE_SIZEOF(n)) <= -1) return -1;

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
	return this->setOption(SOL_SOCKET, SO_BROADCAST, (char*)&n, QSE_SIZEOF(n));
}

int Socket::setSendBuf (unsigned int size)  QSE_CPP_NOEXCEPT
{
	return this->setOption(SOL_SOCKET, SO_SNDBUF, (char*)&size, QSE_SIZEOF(size));
}

int Socket::setRecvBuf (unsigned int size) QSE_CPP_NOEXCEPT
{
	return this->setOption(SOL_SOCKET, SO_RCVBUF, (char*)&size, QSE_SIZEOF(size));
}

int Socket::setLingerOn (int sec) QSE_CPP_NOEXCEPT
{
	struct linger lng;
	lng.l_onoff = 1;
	lng.l_linger = sec;
	return this->setOption(SOL_SOCKET, SO_LINGER, (char*)&lng, QSE_SIZEOF(lng));
}

int Socket::setLingerOff () QSE_CPP_NOEXCEPT
{
	struct linger lng;
	lng.l_onoff = 0;
	lng.l_linger = 0;
	return this->setOption(SOL_SOCKET, SO_LINGER, (char*)&lng, QSE_SIZEOF(lng));
}

int Socket::setTcpNodelay (int n) QSE_CPP_NOEXCEPT
{
#if defined(TCP_NODELAY)
	return this->setOption(IPPROTO_TCP, TCP_NODELAY, (char*)&n, QSE_SIZEOF(n));
#else
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif
}

int Socket::setOobInline (int n) QSE_CPP_NOEXCEPT
{
#if defined(SO_OOBINLINE)
	return this->setOption(SOL_SOCKET, SO_OOBINLINE, (char*)&n, QSE_SIZEOF(n));
#else
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif
}

int Socket::setIpv6Only (int n) QSE_CPP_NOEXCEPT
{
#if defined(IPV6_V6ONLY)
	return this->setOption(IPPROTO_IPV6, IPV6_V6ONLY, (char*)&n, QSE_SIZEOF(n));
#else
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif
}

int Socket::setNonBlock (int n) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	if (qse_set_sck_nonblock(this->handle, n) <= -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return 0;
}

int Socket::shutdown (int how) QSE_CPP_NOEXCEPT
{
	if (this->handle != QSE_INVALID_SCKHND)
	{
		// i put this guard to allow multiple calls to shutdown().
		if (::shutdown(this->handle, how) == -1)
		{
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
			return -1;
		}
	}

	return 0;
}

int Socket::connect (const SocketAddress& target) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	if (::connect(this->handle, (struct sockaddr*)target.getAddrPtr(), target.getAddrSize()) == -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return 0;
}

int Socket::initConnectNB (const SocketAddress& target) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	if (qse_init_sck_conn(this->handle, target.getAddrPtr()) <= -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return 0;
}

int Socket::finiConnectNB () QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	if (qse_fini_sck_conn(this->handle) <= -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return 0;
}


int Socket::bind (const SocketAddress& target) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	if (::bind(this->handle, (struct sockaddr*)target.getAddrPtr(), target.getAddrSize()) == -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return 0;
}

int Socket::bindToIfceAddr (const qse_mchar_t* ifce, qse_uint16_t port) QSE_CPP_NOEXCEPT
{
	SocketAddress addr;
	if (this->getIfceAddress(ifce, &addr) <= -1) return -1;
	return this->bind(addr);
}

int Socket::bindToIfceAddr (const qse_wchar_t* ifce, qse_uint16_t port) QSE_CPP_NOEXCEPT
{
	SocketAddress addr;
	if (this->getIfceAddress(ifce, &addr) <= -1) return -1;
	return this->bind(addr);
}

int Socket::bindToIfce (const qse_mchar_t* ifce) QSE_CPP_NOEXCEPT
{
#if defined(SO_BINDTODEVICE)
	if (!ifce)
	{
		return this->setOption (SOL_SOCKET, SO_BINDTODEVICE, QSE_NULL, 0);
	}
	else
	{
		struct ifreq ifr;
		QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
		qse_size_t mlen = qse_mbsxcpy (ifr.ifr_name, QSE_COUNTOF(ifr.ifr_name), ifce);
		if (ifce[mlen] != QSE_MT('\0'))
		{
			this->setErrorNumber (E_EINVAL);
			return -1;
		}
		return this->setOption (SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, QSE_SIZEOF(ifr));
	}
#else
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif
}

int Socket::bindToIfce (const qse_wchar_t* ifce) QSE_CPP_NOEXCEPT
{
#if defined(SO_BINDTODEVICE)
	if (!ifce)
	{
		return this->setOption(SOL_SOCKET, SO_BINDTODEVICE, QSE_NULL, 0);
	}
	else
	{
		struct ifreq ifr;
		QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	
		qse_size_t wlen, mlen = QSE_COUNTOF(ifr.ifr_name);
		if (qse_wcstombs(ifce, &wlen, ifr.ifr_name, &mlen) <= -1 || ifce[wlen] != QSE_WT('\0')) 
		{
			this->setErrorNumber (E_EINVAL);
			return -1;
		}
		return this->setOption(SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, QSE_SIZEOF(ifr));
	}
#else
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif
}

int Socket::listen (int backlog) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	if (::listen(this->handle, backlog) == -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return 0;
}

int Socket::accept (Socket* newsck, SocketAddress* newaddr, int traits) QSE_CPP_NOEXCEPT
{
	int newfd, flag_v;
	qse_sklen_t addrlen;

	QSE_ASSERT (qse_is_sck_valid(this->handle));

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
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
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
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	if (traits)
	{
		flag_v = ::fcntl(newfd, F_GETFL, 0);
		if (flag_v == -1)
		{
		fcntl_failure:
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
			::close (newfd);
			return -1;
		}
		else
		{
			if (traits & Socket::T_NONBLOCK) flag_v |= O_NONBLOCK;
			else flag_v &= ~O_NONBLOCK;

		#if defined(FD_CLOEXEC)
			if (traits & Socket::T_CLOEXEC) flag_v |= FD_CLOEXEC;
			else flag_v &= ~FD_CLOEXEC;
		#endif
			
			if (::fcntl(newfd, F_SETFL, flag_v) == -1) goto fcntl_failure;
		}
	}

accept_done:
	newsck->handle = newfd;
	return 0;
}

qse_ssize_t Socket::send (const void* buf, qse_size_t len) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	qse_ssize_t n = ::send(this->handle, buf, len, 0);
	if (n == -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return n; 
}

qse_ssize_t Socket::send (const void* buf, qse_size_t len, const SocketAddress& dstaddr) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	qse_ssize_t n = ::sendto(this->handle, buf, len, 0, (struct sockaddr*)dstaddr.getAddrPtr(), dstaddr.getAddrSize());
	if (n == -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return n; 
}

qse_ssize_t Socket::send (const void* buf, qse_size_t len, const SocketAddress& dstaddr, const SocketAddress& srcaddr) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	qse_ioptl_t iov;
	iov.ptr = (void*)buf;
	iov.len = len;
	return this->send(&iov, 1, dstaddr, srcaddr);
}


qse_ssize_t Socket::send (const qse_ioptl_t* iov, int count) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

#if defined(HAVE_SENDMSG) || defined(HAVE_WRITEV)
	ssize_t nwritten;

#if defined(HAVE_SENDMSG)
	struct msghdr msg;
	QSE_MEMSET (&msg, 0, QSE_SIZEOF(msg));
	msg.msg_iov = (struct iovec*)iov;
	msg.msg_iovlen = count;
	nwritten = ::sendmsg(this->handle, &msg, 0);
#else
	nwritten = ::writev(this->handle, (const struct iovec*)iov, count);
#endif
	if (nwritten <= -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return nwritten;
#else
	// TODO: combine to a single buffer .... use sendto.... 
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif
}

qse_ssize_t Socket::send (const qse_ioptl_t* iov, int count, const SocketAddress& dstaddr) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

#if defined(HAVE_SENDMSG)
	ssize_t nwritten;

	struct msghdr msg;
	QSE_MEMSET (&msg, 0, QSE_SIZEOF(msg));
	msg.msg_name = (void*)dstaddr.getAddrPtr();
	msg.msg_namelen = dstaddr.getAddrSize();
	msg.msg_iov = (struct iovec*)iov;
	msg.msg_iovlen = count;
	nwritten = ::sendmsg(this->handle, &msg, 0);
	if (nwritten <= -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return nwritten;
#else
	// TODO: combine to a single buffer .... use sendto.... 
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif
}

qse_ssize_t Socket::send (const qse_ioptl_t* iov, int count, const SocketAddress& dstaddr, const SocketAddress& srcaddr) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

#if defined(HAVE_SENDMSG)
	ssize_t nwritten;

	struct msghdr msg;
	QSE_MEMSET (&msg, 0, QSE_SIZEOF(msg));
	msg.msg_name = (void*)dstaddr.getAddrPtr();
	msg.msg_namelen = dstaddr.getAddrSize();
	msg.msg_iov = (struct iovec*)iov;
	msg.msg_iovlen = count;

	switch (srcaddr.getFamily())
	{
	#if defined(AF_INET)
		case AF_INET:
		{
		#if defined(IP_PKTINFO)
			qse_uint8_t cmsgbuf[CMSG_SPACE(QSE_SIZEOF(struct in_pktinfo))];
			msg.msg_control = cmsgbuf;
			msg.msg_controllen = CMSG_LEN(QSE_SIZEOF(struct in_pktinfo));

			struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
			cmsg->cmsg_level = IPPROTO_IP;
			cmsg->cmsg_type = IP_PKTINFO;
			cmsg->cmsg_len = CMSG_LEN(QSE_SIZEOF(struct in_pktinfo));

			struct in_pktinfo* pi = (struct in_pktinfo*)CMSG_DATA(cmsg);
			//QSE_MEMSET(pi, 0, QSE_SIZEOF(*pi));
			pi->ipi_addr = *(struct in_addr*)srcaddr.getIp4addr();
			pi->ipi_ifindex = 0; // let the kernel choose it

			break;
		#elif defined(IP_SENDSRCADDR)
			qse_uint8_t cmsgbuf[CMSG_SPACE(QSE_SIZEOF(struct in_addr))];
			msg.msg_control = cmsgbuf;
			msg.msg_controllen = CMSG_LEN(QSE_SIZEOF(struct in_addr));

			struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
			cmsg->cmsg_level = IPPROTO_IP;
			cmsg->cmsg_type = IP_SENDSRCADDR;
			cmsg->cmsg_len = CMSG_LEN(QSE_SIZEOF(struct in_addr));

			struct in_addr* pi = (struct in_addr*)CMSG_DATA(cmsg);
			*pi = *(struct in_addr*)srcaddr.getIp6addr();
		#else
			this->setErrorNumber (E_ENOIMPL);
			return -1;
		#endif
		}
	#endif
	#if defined(AF_INET6)
		case AF_INET6:
		{
			qse_uint8_t cmsgbuf[CMSG_SPACE(QSE_SIZEOF(struct in6_pktinfo))];
			msg.msg_control = cmsgbuf;
			msg.msg_controllen = CMSG_LEN(QSE_SIZEOF(struct in6_pktinfo));;

			struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
			cmsg->cmsg_level = IPPROTO_IPV6;
			cmsg->cmsg_type = IPV6_PKTINFO;
			cmsg->cmsg_len = CMSG_LEN(QSE_SIZEOF(struct in6_pktinfo));;

			struct in6_pktinfo* pi = (struct in6_pktinfo*)CMSG_DATA(cmsg);
			//QSE_MEMSET(pi, 0, QSE_SIZEOF(*pi));
			pi->ipi6_addr = *(struct in6_addr*)srcaddr.getIp6addr();
			pi->ipi6_ifindex = 0; // let the kernel choose it

			break;
		}
	#endif
	}

	nwritten = ::sendmsg(this->handle, &msg, 0);
	if (nwritten <= -1)
	{
		this->setErrorNumber (syserr_to_errnum(errno));
		return -1;
	}

	return nwritten;
#else
	// TODO: combine to a single buffer .... use sendto.... 
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif

}

int Socket::sendx (const void* buf, qse_size_t len, qse_size_t* total_sent) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	qse_size_t pos = 0;

	while (pos < len)
	{
		qse_ssize_t n = ::send(this->handle, (char*)buf + pos, len - pos, 0);
		if (n <= -1)
		{
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
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
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	qse_size_t pos = 0;

	while (pos < len)
	{
		qse_ssize_t n = ::sendto(this->handle, (char*)buf + pos, len - pos, 0, (struct sockaddr*)dstaddr.getAddrPtr(), dstaddr.getAddrSize());
		if (n == -1)
		{
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
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
	QSE_ASSERT (qse_is_sck_valid(this->handle));

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
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
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
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
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

int Socket::sendx (qse_ioptl_t* iov, int count, const SocketAddress& dstaddr, qse_size_t* total_sent) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

#if defined(HAVE_SENDMSG)
	int index = 0;
	qse_size_t total = 0;
	int backup_index = -1;
	qse_ioptl_t backup;

	struct msghdr msg;
	QSE_MEMSET (&msg, 0, QSE_SIZEOF(msg));
	msg.msg_name = (void*)dstaddr.getAddrPtr();
	msg.msg_namelen = dstaddr.getAddrSize();

	while (1)
	{
		ssize_t nwritten;

		msg.msg_iov = (struct iovec*)&iov[index];
		msg.msg_iovlen = count - index;
		nwritten = ::sendmsg(this->handle, &msg, 0);
		if (nwritten <= -1)
		{
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
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
		nwritten = ::sendto(this->handle, (qse_uint8_t*)v->ptr + pos, rem, 0);
		if (nwritten <= -1) 
		{
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
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
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	qse_ssize_t n = ::recv(this->handle, buf, len, 0);
	if (n == -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return n; 
}

qse_ssize_t Socket::receive (void* buf, qse_size_t len, SocketAddress& srcaddr) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (qse_is_sck_valid(this->handle));

	qse_sklen_t addrlen = srcaddr.getAddrCapa();
	qse_ssize_t n = ::recvfrom(this->handle, buf, len, 0, (struct sockaddr*)srcaddr.getAddrPtr(), &addrlen);
	if (n == -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	return n; 
}

int Socket::joinMulticastGroup (const SocketAddress& mcaddr, const SocketAddress& ifaddr) QSE_CPP_NOEXCEPT
{
	int family = mcaddr.getFamily(); // ((struct sockaddr*)mcaddr.getAddrPtr())->sa_family

	if (family != ifaddr.getFamily())
	{
		this->setErrorNumber (E_EINVAL);
		return -1;
	}

	switch (family)
	{
	#if defined(AF_INET)
		case AF_INET:
		{
			
			struct ip_mreq mreq;
			QSE_MEMSET (&mreq, 0, QSE_SIZEOF(mreq));

			mreq.imr_multiaddr = *(struct in_addr*)mcaddr.getIp4addr();
			mreq.imr_interface = *(struct in_addr*)ifaddr.getIp4addr();
			return ::setsockopt(this->handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, QSE_SIZEOF(mreq));
		}
	#endif

	#if defined(AF_INET6)
		case AF_INET6:
		{
			struct ipv6_mreq mreq;
			QSE_MEMSET (&mreq, 0, QSE_SIZEOF(mreq));

			mreq.ipv6mr_multiaddr = *(struct in6_addr*)mcaddr.getIp6addr();
			mreq.ipv6mr_interface = ifaddr.getScopeId();
			return ::setsockopt(this->handle, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, QSE_SIZEOF(mreq));
		}
	#endif
	}

	this->setErrorNumber (E_ENOIMPL);
	return -1;
}

int Socket::leaveMulticastGroup (const SocketAddress& mcaddr, const SocketAddress& ifaddr) QSE_CPP_NOEXCEPT
{
	int family = mcaddr.getFamily(); // ((struct sockaddr*)mcaddr.getAddrPtr())->sa_family

	if (family != ifaddr.getFamily())
	{
		this->setErrorNumber (E_EINVAL);
		return -1;
	}

	switch (family)
	{
	#if defined(AF_INET)
		case AF_INET:
		{
			struct ip_mreq mreq;
			QSE_MEMSET (&mreq, 0, QSE_SIZEOF(mreq));

			mreq.imr_multiaddr = *(struct in_addr*)mcaddr.getIp4addr();
			mreq.imr_interface = *(struct in_addr*)ifaddr.getIp4addr();
			return ::setsockopt(this->handle, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, QSE_SIZEOF(mreq));
		}
	#endif

	#if defined(AF_INET6)
		case AF_INET6:
		{
			struct ipv6_mreq mreq;
			QSE_MEMSET (&mreq, 0, QSE_SIZEOF(mreq));

			mreq.ipv6mr_multiaddr = *(struct in6_addr*)mcaddr.getIp6addr();
			mreq.ipv6mr_interface = ifaddr.getScopeId();
			return ::setsockopt(this->handle, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, QSE_SIZEOF(mreq));
		}
	#endif
	}

	this->setErrorNumber (E_ENOIMPL);
	return -1;
}

int Socket::getIfceIndex (const qse_mchar_t* name) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_index(name, qse_mbslen(name), false);
}

int Socket::getIfceIndex (const qse_wchar_t* name) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_index(name, qse_wcslen(name), false);
}

int Socket::getIfceIndex (const qse_mchar_t* name, qse_size_t len) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_index(name, len, false);
}

int Socket::getIfceIndex (const qse_wchar_t* name, qse_size_t len) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_index(name, len, false);
}

int Socket::get_ifce_index (const void* name, qse_size_t len, bool wchar)
{
#if defined(SIOCGIFINDEX)
	struct ifreq ifr;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));

	if (wchar)
	{
		qse_size_t wlen = len, mlen = QSE_COUNTOF(ifr.ifr_name) - 1;
		if (qse_wcsntombsn((const qse_wchar_t*)name, &wlen, ifr.ifr_name, &mlen) <= -1 || wlen != len)
		{
			this->setErrorNumber (E_EINVAL);
			return -1;
		}
		ifr.ifr_name[mlen] = QSE_MT('\0');
	}
	else
	{
		qse_size_t mlen = qse_mbsxncpy(ifr.ifr_name, QSE_COUNTOF(ifr.ifr_name), (const qse_mchar_t*)name, len);
		if (mlen != len)
		{
			this->setErrorNumber (E_EINVAL);
			return -1;
		}
	}

	if (::ioctl(this->handle, SIOCGIFINDEX, &ifr) == -1) 
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		return -1;
	}

	#if defined(ifr_ifindex)
	return ifr.ifr_ifindex;
	#else
	return ifr.ifr_index;
	#endif
#else
	this->setErrorNumber (E_ENOIMPL);
	return -1;
#endif
}

int Socket::getIfceAddress (const qse_mchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFADDR, name, qse_mbslen(name), false, addr);
}

int Socket::getIfceAddress (const qse_wchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFADDR, name, qse_wcslen(name), true, addr);
}

int Socket::getIfceNetmask(const qse_mchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFNETMASK, name, qse_mbslen(name), false, addr);
}

int Socket::getIfceNetmask (const qse_wchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFNETMASK, name, qse_wcslen(name), true, addr);
}

int Socket::getIfceBroadcast(const qse_mchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFBRDADDR, name, qse_mbslen(name), false, addr);
}

int Socket::getIfceBroadcast (const qse_wchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFBRDADDR, name, qse_wcslen(name), true, addr);
}

int Socket::getIfceAddress (const qse_mchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFADDR, name, len, false, addr);
}

int Socket::getIfceAddress (const qse_wchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFADDR, name, len, true, addr);
}

int Socket::getIfceNetmask(const qse_mchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFNETMASK, name, len, false, addr);
}

int Socket::getIfceNetmask (const qse_wchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFNETMASK, name, len, true, addr);
}

int Socket::getIfceBroadcast(const qse_mchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFBRDADDR, name, len, false, addr);
}

int Socket::getIfceBroadcast (const qse_wchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT
{
	return this->get_ifce_address(SIOCGIFBRDADDR, name, len, true, addr);
}

int Socket::get_ifce_address (int cmd, const void* name, qse_size_t len, bool wchar, SocketAddress* addr)
{
	struct ifreq ifr;

	QSE_MEMSET (&ifr, 0, QSE_SIZEOF(ifr));
	if (wchar)
	{
		qse_size_t wlen = len, mlen = QSE_COUNTOF(ifr.ifr_name) - 1;
		if (qse_wcsntombsn((const qse_wchar_t*)name, &wlen, ifr.ifr_name, &mlen) <= -1 || wlen != len)
		{
			this->setErrorNumber (E_EINVAL);
			return -1;
		}
		ifr.ifr_name[mlen] = QSE_MT('\0');
	}
	else
	{
		qse_size_t mlen = qse_mbsxncpy(ifr.ifr_name, QSE_COUNTOF(ifr.ifr_name), (const qse_mchar_t*)name, len);
		if (mlen != len)
		{
			this->setErrorNumber (E_EINVAL);
			return -1;
		}
	}

#if defined(HAVE_GETIFADDRS)
	struct ifaddrs* ifa;
	if (::getifaddrs(&ifa) == 0)
	{
		for (struct ifaddrs* ife = ifa; ife; ife = ife->ifa_next)
		{
			if (qse_mbscmp(ifr.ifr_name, ife->ifa_name) != 0) continue;

			struct sockaddr* sa = QSE_NULL;

			switch (cmd)
			{
				case SIOCGIFADDR:
					sa = ife->ifa_addr;
					break;

				case SIOCGIFNETMASK:
					sa = ife->ifa_netmask;
					break;

				case SIOCGIFBRDADDR:
					sa = ife->ifa_broadaddr;
					break;

				default: 
					break;
			}

			if (!sa || !sa->sa_data) continue;
			if (sa->sa_family != this->domain) continue; /* skip an address that doesn't match the socket's domain */

			*addr = SocketAddress((const qse_skad_t*)sa);
			freeifaddrs (ifa);
			return 0;
		}

		freeifaddrs (ifa);
	}
#endif

	if (::ioctl(this->handle, cmd, &ifr) == -1) 
	{
		this->setErrorNumber (syserr_to_errnum(errno));
		return -1;
	}

	struct sockaddr* sa = (struct sockaddr*)&ifr.ifr_addr;
	if (sa->sa_family != this->domain)
	{
		this->setErrorNumber (E_ENOENT);
		return -1;
	}

	*addr = SocketAddress((const qse_skad_t*)&ifr.ifr_addr);
	return 0;
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
