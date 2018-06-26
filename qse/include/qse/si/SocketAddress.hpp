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

#ifndef _QSE_SI_SOCKETADDRESS_HPP_
#define _QSE_SI_SOCKETADDRESS_HPP_

#include <qse/Types.hpp>
#include <qse/si/nwad.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class SocketAddress
{
public:
	SocketAddress () QSE_CPP_NOEXCEPT;
	SocketAddress (int family) QSE_CPP_NOEXCEPT;
	SocketAddress (const qse_skad_t* skad) QSE_CPP_NOEXCEPT;
	SocketAddress (const qse_nwad_t* nwad) QSE_CPP_NOEXCEPT;

	int getFamily () const QSE_CPP_NOEXCEPT;

	qse_skad_t* getAddrPtr() QSE_CPP_NOEXCEPT
	{
		return &this->skad;
	}

	const qse_skad_t* getAddrPtr() const QSE_CPP_NOEXCEPT
	{
		return &this->skad;
	}

	int getAddrSize () const QSE_CPP_NOEXCEPT
	{
		return qse_skadsize(&this->skad);
	}

	int getAddrCapa() const QSE_CPP_NOEXCEPT
	{
		return QSE_SIZEOF(this->skad);
	}

	void setIpaddr (const qse_ip4ad_t* ipaddr) QSE_CPP_NOEXCEPT;
	void setIpaddr (const qse_ip6ad_t* ipaddr) QSE_CPP_NOEXCEPT;

	qse_uint16_t getPort() const QSE_CPP_NOEXCEPT; // in network-byte order
	void setPort (qse_uint16_t port) QSE_CPP_NOEXCEPT; // in network-byte order

	int set (const qse_skad_t* skad) QSE_CPP_NOEXCEPT; 
	int set (const qse_nwad_t* nwad) QSE_CPP_NOEXCEPT;
	int set (const qse_mchar_t* str) QSE_CPP_NOEXCEPT;
	int set (const qse_wchar_t* str) QSE_CPP_NOEXCEPT;
	int set (const qse_mchar_t* str, qse_size_t len) QSE_CPP_NOEXCEPT;
	int set (const qse_wchar_t* str, qse_size_t len) QSE_CPP_NOEXCEPT;


protected:
	qse_skad_t skad;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
