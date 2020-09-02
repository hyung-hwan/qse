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


#include <qse/si/SocketAddress.hpp>
#include <qse/cmn/mbwc.h>
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
#	include <netdb.h>

#	if defined(QSE_SIZEOF_STRUCT_SOCKADDR_IN6) && (QSE_SIZEOF_STRUCT_SOCKADDR_IN6 <= 0)
#		undef AF_INET6
#	endif

#	if defined(QSE_SIZEOF_STRUCT_SOCKADDR_UN) && (QSE_SIZEOF_STRUCT_SOCKADDR_UN <= 0)
#		undef AF_UNIX
#	endif

#endif


#define FAMILY(x) (((struct sockaddr*)(x))->sa_family)

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

void SocketAddress::setIp4addr (const qse_ip4ad_t* ipaddr) QSE_CPP_NOEXCEPT
{
#if defined(AF_INET)
	if (FAMILY(&this->skad) == AF_INET)
	{
		struct sockaddr_in* v4 = (struct sockaddr_in*)&this->skad;
		QSE_MEMCPY (&v4->sin_addr, ipaddr, QSE_SIZEOF(*ipaddr));
	}
#endif
}

void SocketAddress::setIp4addr (const qse_uint32_t ipaddr) QSE_CPP_NOEXCEPT
{
#if defined(AF_INET)
	if (FAMILY(&this->skad) == AF_INET)
	{
		struct sockaddr_in* v4 = (struct sockaddr_in*)&this->skad;
		v4->sin_addr.s_addr = ipaddr;
	}
#endif
}

void SocketAddress::setIp6addr (const qse_ip6ad_t* ipaddr) QSE_CPP_NOEXCEPT
{
#if defined(AF_INET6)
	if (FAMILY(&this->skad) == AF_INET6)
	{
		struct sockaddr_in6* v6 = (struct sockaddr_in6*)&this->skad;
		QSE_MEMCPY (&v6->sin6_addr, ipaddr, QSE_SIZEOF(*ipaddr));
	}
#endif
}

const qse_ip4ad_t* SocketAddress::getIp4addr () const QSE_CPP_NOEXCEPT
{
#if defined(AF_INET)
	if (FAMILY(&this->skad) == AF_INET)
	{
		struct sockaddr_in* v4 = (struct sockaddr_in*)&this->skad;
		return (const qse_ip4ad_t*)&v4->sin_addr;
	}
#endif
	return QSE_NULL;
}

const qse_ip6ad_t* SocketAddress::getIp6addr () const QSE_CPP_NOEXCEPT
{
#if defined(AF_INET6)
	if (FAMILY(&this->skad) == AF_INET6)
	{
		struct sockaddr_in6* v6 = (struct sockaddr_in6*)&this->skad;
		return (const qse_ip6ad_t*)&v6->sin6_addr;
	}
#endif
	return QSE_NULL;
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

qse_uint32_t SocketAddress::getScopeId () const QSE_CPP_NOEXCEPT
{
	switch (FAMILY(&this->skad))
	{
	#if defined(AF_INET6)
		case AF_INET6:
		{
			struct sockaddr_in6* v6 = (struct sockaddr_in6*)&this->skad;
			return v6->sin6_scope_id;
		}
	#endif
	}

	return 0;
}

void SocketAddress::setScopeId (qse_uint32_t scope_id) QSE_CPP_NOEXCEPT
{
	switch (FAMILY(&this->skad))
	{
	#if defined(AF_INET6)
		case AF_INET6:
		{
			struct sockaddr_in6* v6 = (struct sockaddr_in6*)&this->skad;
			v6->sin6_scope_id = scope_id;
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

/* 
 * NOTICE: 
 *   When host is "", the address is resolved to localhost.
 *   When host is QSE_NULL, the address is resolved to INADDR_ANY 
 *   or IN6ADDR_ANY_INIT depending on the address family.
 */
int SocketAddress::resolve (const qse_mchar_t* service, const qse_mchar_t* host, int family, int type) QSE_CPP_NOEXCEPT
{
	struct addrinfo hints;
	struct addrinfo* info, * p;
	int x;
 
	QSE_ASSERT (family == QSE_AF_UNSPEC || family == QSE_AF_INET || family == QSE_AF_INET6);
 
	QSE_MEMSET (&hints, 0, QSE_SIZEOF(hints));
	hints.ai_family = family;
	hints.ai_socktype = type;
	if (!host) hints.ai_flags = AI_PASSIVE; 
	else if (host[0] == '\0') host = QSE_NULL;

	x = ::getaddrinfo(host, service, &hints, &info);
	if (x != 0) return -1;
	
	for (p = info; p; p = p->ai_next) 
	{
		if (family != QSE_AF_UNSPEC && p->ai_family != family) continue;
		if (type != 0 && p->ai_socktype != type) continue;
		if (QSE_SIZEOF(this->skad) < p->ai_addrlen) continue;
 
		QSE_MEMCPY (&this->skad, p->ai_addr, p->ai_addrlen);
		break;
	}
	::freeaddrinfo (info);
	return 0;
}

int SocketAddress::resolve (const qse_wchar_t* service, const qse_wchar_t* host, int family, int type) QSE_CPP_NOEXCEPT
{
	struct addrinfo hints;
	struct addrinfo* info, * p;
	int x;
 
	QSE_ASSERT (family == QSE_AF_UNSPEC || family == QSE_AF_INET || family == QSE_AF_INET6);
 
	QSE_MEMSET (&hints, 0, QSE_SIZEOF(hints));
	hints.ai_family = family;
	hints.ai_socktype = type;
	if (!host) hints.ai_flags = AI_PASSIVE; 
	else if (host[0] == '\0') host = QSE_NULL;

	qse_mchar_t mb_host[NI_MAXHOST + 1];
	qse_mchar_t mb_service[NI_MAXSERV + 1];
	qse_mchar_t* p_host = QSE_NULL;
	qse_mchar_t* p_service = QSE_NULL;
 
	if (host) 
	{
		qse_size_t wcslen, mbslen = QSE_COUNTOF(mb_host);
		if (qse_wcstombs(host, &wcslen, mb_host, &mbslen) <= -1)  return -1;
		p_host = mb_host;
	}
	if (service) 
	{
		qse_size_t wcslen, mbslen = QSE_COUNTOF(mb_service);

		if (qse_wcstombs(service, &wcslen, mb_service, &mbslen) <= -1) return -1;
		p_service = mb_service;
	}

	x = ::getaddrinfo(p_host, p_service, &hints, &info);	
	if (x != 0) return -1;
	
	for (p = info; p; p = p->ai_next) 
	{
		if (family != QSE_AF_UNSPEC && p->ai_family != family) continue;
		if (type != 0 && p->ai_socktype != type) continue;
		if (QSE_SIZEOF(this->skad) < p->ai_addrlen) continue;
 
		QSE_MEMCPY (&this->skad, p->ai_addr, p->ai_addrlen);
		break;
	}
	::freeaddrinfo (info);
	return 0;
}


bool SocketAddress::isLoopBack () const QSE_CPP_NOEXCEPT
{
	switch (FAMILY(&this->skad))
	{
		case AF_INET:
		{
			struct sockaddr_in* v4 = (struct sockaddr_in*)&this->skad;
			return v4->sin_addr.s_addr == QSE_CONST_HTON32(0x7F000001);
		}
	
		case AF_INET6:
		{
			struct sockaddr_in6* v6 = (struct sockaddr_in6*)&this->skad;
			qse_uint32_t* x = (qse_uint32_t*)v6->sin6_addr.s6_addr; // TODO: is this alignment safe? 
			return x[0] == 0 && x[1] == 0 && x[2] == 0 && x[3] == 1;
		}
	}

	return false;
}

qse_wchar_t* SocketAddress::toStrBuf (qse_wchar_t* buf, qse_size_t len) const QSE_CPP_NOEXCEPT
{
	qse_nwad_t nwad;
	qse_skadtonwad (&this->skad, &nwad);
	qse_nwadtowcs (&nwad, buf, len, QSE_NWADTOWCS_ALL);
	return buf;
}

qse_mchar_t* SocketAddress::toStrBuf (qse_mchar_t* buf, qse_size_t len) const QSE_CPP_NOEXCEPT
{
	qse_nwad_t nwad;
	qse_skadtonwad (&this->skad, &nwad);
	qse_nwadtombs (&nwad, buf, len, QSE_NWADTOWCS_ALL);
	return buf;
}

QSE::MbString SocketAddress::toMbString (QSE::Mmgr* mmgr) const
{
	QSE::MbString buf(256, mmgr);
	qse_nwad_t nwad;
	qse_skadtonwad (&this->skad, &nwad);
	qse_size_t n = qse_nwadtombs(&nwad, (qse_mchar_t*)buf.getBuffer(), buf.getCapacity(), QSE_NWADTOMBS_ALL);
	buf.truncate (n);
	return buf;
}

QSE::WcString SocketAddress::toWcString (QSE::Mmgr* mmgr) const
{
	QSE::WcString buf(256, mmgr);
	qse_nwad_t nwad;
	qse_skadtonwad (&this->skad, &nwad);
	qse_size_t n = qse_nwadtowcs(&nwad, (qse_wchar_t*)buf.getBuffer(), buf.getCapacity(), QSE_NWADTOWCS_ALL);
	buf.truncate (n);
	return buf;
}

QSE::String SocketAddress::toString (QSE::Mmgr* mmgr) const
{
	QSE::String buf(256, mmgr);
	qse_nwad_t nwad;
	qse_skadtonwad (&this->skad, &nwad);
	qse_size_t n = qse_nwadtostr(&nwad, (qse_char_t*)buf.getBuffer(), buf.getCapacity(), QSE_NWADTOSTR_ALL);
	buf.truncate (n);
	return buf;
}

qse_mchar_t* SocketAddress::ip4addrToStrBuf (const qse_ip4ad_t* ipaddr, qse_mchar_t* buf, qse_size_t len)
{
	qse_nwad_t nwad;
	qse_initnwadwithip4ad (&nwad, 0, ipaddr);
	qse_nwadtombs (&nwad, buf, len, QSE_NWADTOMBS_ALL);
	return buf;
}

qse_mchar_t* SocketAddress::ip4addrToStrBuf (qse_uint32_t ipaddr, qse_mchar_t* buf, qse_size_t len)
{
	qse_nwad_t nwad;
	qse_initnwadwithip4ad (&nwad, 0, (qse_ip4ad_t*)&ipaddr);
	qse_nwadtombs (&nwad, buf, len, QSE_NWADTOMBS_ALL);
	return buf;
}

qse_mchar_t* SocketAddress::ip6addrToStrBuf (const qse_ip6ad_t* ipaddr, qse_mchar_t* buf, qse_size_t len)
{
	qse_nwad_t nwad;
	qse_initnwadwithip6ad (&nwad, 0, ipaddr, 0);
	qse_nwadtombs (&nwad, buf, len, QSE_NWADTOMBS_ALL);
	return buf;
}

qse_wchar_t* SocketAddress::ip4addrToStrBuf (const qse_ip4ad_t* ipaddr, qse_wchar_t* buf, qse_size_t len)
{
	qse_nwad_t nwad;
	qse_initnwadwithip4ad (&nwad, 0, ipaddr);
	qse_nwadtowcs (&nwad, buf, len, QSE_NWADTOWCS_ALL);
	return buf;
}

qse_wchar_t* SocketAddress::ip4addrToStrBuf (qse_uint32_t ipaddr, qse_wchar_t* buf, qse_size_t len)
{
	qse_nwad_t nwad;
	qse_initnwadwithip4ad (&nwad, 0, (qse_ip4ad_t*)&ipaddr);
	qse_nwadtowcs (&nwad, buf, len, QSE_NWADTOWCS_ALL);
	return buf;
}

qse_wchar_t* SocketAddress::ip6addrToStrBuf (const qse_ip6ad_t* ipaddr, qse_wchar_t* buf, qse_size_t len)
{
	qse_nwad_t nwad;
	qse_initnwadwithip6ad (&nwad, 0, ipaddr, 0);
	qse_nwadtowcs (&nwad, buf, len, QSE_NWADTOWCS_ALL);
	return buf;
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
