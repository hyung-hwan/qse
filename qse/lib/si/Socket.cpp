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

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

Socket::Socket () QSE_CPP_NOEXCEPT: handle(QSE_INVALID_SCKHND)
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

void Socket::setError (ErrorCode error_code, const qse_char_t* fmt, ...)
{
	static const qse_char_t* errstr[] = 
	{
		QSE_T("no error"),
		QSE_T("insufficient memory"),
		QSE_T("invalid parameter"),
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

int Socket::open (int domain, int type, int protocol) QSE_CPP_NOEXCEPT
{
	int x;

	x = ::socket (domain, type, protocol);
	if (x != -1)
	{
		this->close ();
		this->handle = x;
	}

	return x;
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
	if (this->handle == QSE_INVALID_SCKHND)
	{
		this->setError (Socket::E_NOTOPEN);
		return -1;
	}

	if (::connect(this->handle, (struct sockaddr*)target.getAddrPtr(), target.getAddrSize()) == -1)
	{
		this->set_error_with_syserr (errno);
		return -1;
	}

	return 0;
}

int Socket::bind (const SocketAddress& target) QSE_CPP_NOEXCEPT
{
	if (this->handle == QSE_INVALID_SCKHND)
	{
		this->setError (Socket::E_NOTOPEN);
		return -1;
	}

	if (::bind(this->handle, (struct sockaddr*)target.getAddrPtr(), target.getAddrSize()) == -1)
	{
		this->set_error_with_syserr (errno);
		return -1;
	}

	return 0;
}

int Socket::accept (Socket* newsck, SocketAddress* newaddr, int flags) QSE_CPP_NOEXCEPT
{
	int n;

	if (this->handle == QSE_INVALID_SCKHND)
	{
		this->setError (Socket::E_NOTOPEN);
		return -1;
	}
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
