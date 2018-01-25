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

#ifndef _QSE_SI_SOCKET_HPP_
#define _QSE_SI_SOCKET_HPP_

#include <qse/Types.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/si/SocketAddress.hpp>
#include <qse/si/sck.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class Socket: public Uncopyable, public Types
{
public:
	enum ErrorCode
	{
		E_NOERR,
		E_NOMEM,
		E_INVAL,
		E_NOTOPEN,
		E_SYSERR
	};

	Socket () QSE_CPP_NOEXCEPT;
	~Socket () QSE_CPP_NOEXCEPT;

	void setError (ErrorCode error_code, const qse_char_t* fmt = QSE_NULL, ...);


	int open (int domain, int type, int protocol) QSE_CPP_NOEXCEPT;
	void close () QSE_CPP_NOEXCEPT;

	int connect (const SocketAddress& target) QSE_CPP_NOEXCEPT;
	int bind (const SocketAddress& target) QSE_CPP_NOEXCEPT;
	int accept (Socket* newsck, SocketAddress* newaddr, int flags) QSE_CPP_NOEXCEPT;

	int read () QSE_CPP_NOEXCEPT;
	int write () QSE_CPP_NOEXCEPT;

protected:
	qse_sck_hnd_t handle;

	ErrorCode errcode;
	qse_char_t errmsg[128];

	void set_error_with_syserr (int syserr);
};


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////


#endif
