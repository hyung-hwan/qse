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

Socket::Socket () QSE_CPP_NOEXCEPT: handle(QSE_INVALID_SCKHND), errcode(E_NOERR)
{
	this->errmsg[0] = QSE_T('\0');
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

void Socket::setError (ErrorCode error_code, const qse_char_t* fmt, ...)
{
	static const qse_char_t* errstr[] = 
	{
		QSE_T("no error"),
		QSE_T("insufficient memory"),
		QSE_T("invalid parameter"),
		QSE_T("in progress"),
		QSE_T("socket not open"),
		QSE_T("system error")
	};

	this->errcode = error_code;
	if (fmt)
	{
		va_list ap;
		va_start (ap, fmt);
		qse_strxvfmt (this->errmsg, QSE_COUNTOF(errmsg), fmt, ap);
		va_end (ap);
	}
	else
	{
		qse_strxcpy (this->errmsg, QSE_COUNTOF(errmsg), errstr[error_code]);
	}
}

int Socket::open (int domain, int type, int protocol, int traits) QSE_CPP_NOEXCEPT
{
	int x;
	bool fcntl_v = 0;

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
	if (traits & T_NONBLOCK) type |= SOCK_NONBLOCK;
	if (traits & T_CLOEXEC) type |= SOCK_CLOEXEC;
open_socket:
#endif
	x = ::socket (domain, type, protocol);
	if (x == -1)
	{
	#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
		if (errno == EINVAL && (type & (SOCK_NONBLOCK | SOCK_CLOEXEC)))
		{
			type &= ~(SOCK_NONBLOCK | SOCK_CLOEXEC);
			if (traits & T_NONBLOCK ) fcntl_v |= O_NONBLOCK;
			if (traits & T_CLOEXEC) fcntl_v |= O_CLOEXEC;
			goto open_socket;
		}
	#endif
		this->set_error_with_syserr (errno);
		return -1;
	}

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
	// do nothing
#else
	if (traits & T_NONBLOCK ) fcntl_v |= O_NONBLOCK;
	if (traits & T_CLOEXEC) fcntl_v |= O_CLOEXEC;
#endif

	if (fcntl_v)
	{
		int fl = fcntl(x, F_GETFL, 0);
		if (fl == -1)
		{
		fcntl_failure:
			this->set_error_with_syserr (errno);
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

int Socket::connect (const SocketAddress& target) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	if (::connect(this->handle, (struct sockaddr*)target.getAddrPtr(), target.getAddrSize()) == -1)
	{
		this->set_error_with_syserr (errno);
		return -1;
	}

	return 0;
}

int Socket::bind (const SocketAddress& target) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	if (::bind(this->handle, (struct sockaddr*)target.getAddrPtr(), target.getAddrSize()) == -1)
	{
		this->set_error_with_syserr (errno);
		return -1;
	}

	return 0;
}

int Socket::accept (Socket* newsck, SocketAddress* newaddr, int traits) QSE_CPP_NOEXCEPT
{
	int n;

	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

#if 0
	qse_socklen_t len = newaddr->getAddrSize();

	if ((n = ::accept4 (this->handle, newaddr->getAddrPtr(), &len)) == -1)
	{
		this->set_error_with_syserr (errno);
		return -1;
	}

	newsck->handle = n;
#endif
	return 0;
}

qse_ssize_t Socket::send (const void* buf, qse_size_t len) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	qse_ssize_t n = ::send(this->handle, buf, len, 0);
	if (n == -1)
	{
		this->set_error_with_syserr (errno);
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
		this->set_error_with_syserr (errno);
		return -1;
	}

	return n; 
}

qse_ssize_t Socket::receive (void* buf, qse_size_t len) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (this->handle != QSE_INVALID_SCKHND);

	qse_ssize_t n = ::recv(this->handle, buf, len, 0);
	if (n == -1)
	{
		this->set_error_with_syserr (errno);
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
		this->set_error_with_syserr (errno);
		return -1;
	}

	return n; 
}



void Socket::set_error_with_syserr (int syserr)
{
	qse_mchar_t buf[128];
	ErrorCode errcode;

	switch (errno)
	{
		case EINVAL:
			errcode = this->E_INVAL;
			break;

		case ENOMEM:
			errcode = this->E_NOMEM;
			break;

		case EINPROGRESS:
			errcode = this->E_INPROG;
			break;

// TODO: translate more system error codes

		default:
			strerror_r(errno, buf, QSE_COUNTOF(buf));
			this->setError (this->E_SYSERR, QSE_T("%hs"), buf);
			return;
	}

	this->setError (errcode);
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
