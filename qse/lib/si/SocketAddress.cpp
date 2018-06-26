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


#include <qse/si/SocketAddress.hpp>
#include "../cmn/mem-prv.h"

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h> /* sockaddr_in6 */
#	include <windows.h>
#	undef AF_UNIX
#	if defined(__WATCOMC__) && (__WATCOMC__ < 1200)
		/* the header files shipped with watcom 11 doesn't contain
		 * proper inet6 support. note using the compiler version
		 * in the contidional isn't that good idea since you 
		 * can use newer header files with this old compiler.
		 * never mind it for the time being.
		 */
#		undef AF_INET6
#	endif
#elif defined(__OS2__)
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
	/* though AF_INET6 is defined, there is no support
	 * for it. so undefine it */
#	undef AF_INET6
#	undef AF_UNIX
#	pragma library("tcpip32.lib")
#elif defined(__DOS__)
#	include <tcp.h> /* watt-32 */
#	undef AF_UNIX
#else
#	if defined(HAVE_SYS_TYPES_H)
#		include <sys/types.h>
#	endif
#	include <sys/socket.h>
#	include <netinet/in.h>
#	if defined(HAVE_SYS_UN_H)
#		include <sys/un.h>
#	endif

#	if defined(QSE_SIZEOF_STRUCT_SOCKADDR_IN6) && (QSE_SIZEOF_STRUCT_SOCKADDR_IN6 <= 0)
#		undef AF_INET6
#	endif

#	if defined(QSE_SIZEOF_STRUCT_SOCKADDR_UN) && (QSE_SIZEOF_STRUCT_SOCKADDR_UN <= 0)
#		undef AF_UNIX
#	endif

#endif


#if defined(AF_INET)
#	define FAMILY(x) (((struct sockaddr_in*)(x))->sin_family)
#elif defined(AF_INET6)
#	define FAMILY(x) (((struct sockaddr_in6*)(x))->sin6_family)
#elif defined(AF_UNIX)
#	define FAMILY(x) (((struct sockaddr_un*)(x))->sun_family)
#else
#	define FAMILY(x) (-1)
#endif

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////


SocketAddress::SocketAddress () QSE_CPP_NOEXCEPT
{
	QSE_MEMSET (&this->skad, 0, QSE_SIZEOF(this->skad));
}

SocketAddress::SocketAddress (int family) QSE_CPP_NOEXCEPT
{
	QSE_MEMSET (&this->skad, 0, QSE_SIZEOF(this->skad));
	FAMILY(&this->skad) = family;
}

SocketAddress::SocketAddress (const qse_skad_t* skad) QSE_CPP_NOEXCEPT
{
	this->set (skad);
}

SocketAddress::SocketAddress (const qse_nwad_t* nwad) QSE_CPP_NOEXCEPT
{
	this->set (nwad);
}

int SocketAddress::getFamily () const QSE_CPP_NOEXCEPT
{
	return FAMILY(&this->skad);
	//return qse_skadfamily (&this->skad);
}

void SocketAddress::setIpaddr (const qse_ip4ad_t* ipaddr) QSE_CPP_NOEXCEPT
{
#if defined(AF_INET)
	if (FAMILY(&this->skad) == AF_INET)
	{
		struct sockaddr_in* v4 = (struct sockaddr_in*)&this->skad;
		QSE_MEMCPY (&v4->sin_addr, ipaddr, QSE_SIZEOF(*ipaddr));
	}
#endif
}

void SocketAddress::setIpaddr (const qse_ip6ad_t* ipaddr) QSE_CPP_NOEXCEPT
{
#if defined(AF_INET6)
	if (FAMILY(&this->skad) == AF_INET6)
	{
		struct sockaddr_in6* v6 = (struct sockaddr_in6*)&this->skad;
		QSE_MEMCPY (&v6->sin6_addr, ipaddr, QSE_SIZEOF(*ipaddr));
	}
#endif
}

qse_uint16_t SocketAddress::getPort () const QSE_CPP_NOEXCEPT
{
	switch (FAMILY(&this->skad))
	{
#if defined(AF_INET)
		case AF_INET:
		{
			struct sockaddr_in* v4 = (struct sockaddr_in*)&this->skad;
			return v4->sin_port;
		}
#endif

#if defined(AF_INET6)
		case AF_INET6:
		{
			struct sockaddr_in6* v6 = (struct sockaddr_in6*)&this->skad;
			return v6->sin6_port;
		}
#endif
	}

	return 0;
}

void SocketAddress::setPort (qse_uint16_t port) QSE_CPP_NOEXCEPT
{
	switch (FAMILY(&this->skad))
	{
#if defined(AF_INET)
		case AF_INET:
		{
			struct sockaddr_in* v4 = (struct sockaddr_in*)&this->skad;
			v4->sin_port = port;
			break;
		}
#endif

#if defined(AF_INET6)
		case AF_INET6:
		{
			struct sockaddr_in6* v6 = (struct sockaddr_in6*)&this->skad;
			v6->sin6_port = port;
			break;
		}
#endif
	}
}

int SocketAddress::set (const qse_skad_t* skad) QSE_CPP_NOEXCEPT
{
	this->skad = *skad;
	return 0;
}

int SocketAddress::set (const qse_nwad_t* nwad) QSE_CPP_NOEXCEPT
{
	return qse_nwadtoskad(nwad, &this->skad);
}


int SocketAddress::set (const qse_mchar_t* str) QSE_CPP_NOEXCEPT
{
	qse_nwad_t nwad;
	if (qse_mbstonwad(str, &nwad) <= -1) return -1;
	return qse_nwadtoskad(&nwad, &this->skad);
}

int SocketAddress::set (const qse_wchar_t* str) QSE_CPP_NOEXCEPT
{
	qse_nwad_t nwad;
	if (qse_wcstonwad(str, &nwad) <= -1) return -1;
	return qse_nwadtoskad(&nwad, &this->skad);
}

int SocketAddress::set (const qse_mchar_t* str, qse_size_t len) QSE_CPP_NOEXCEPT
{
	qse_nwad_t nwad;
	if (qse_mbsntonwad(str, len, &nwad) <= -1) return -1;
	return qse_nwadtoskad(&nwad, &this->skad);
}

int SocketAddress::set (const qse_wchar_t* str, qse_size_t len) QSE_CPP_NOEXCEPT
{
	qse_nwad_t nwad;
	if (qse_wcsntonwad(str, len, &nwad) <= -1) return -1;
	return qse_nwadtoskad(&nwad, &this->skad);
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
