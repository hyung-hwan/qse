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

#ifndef _QSE_SI_SOCKETADDRESS_HPP_
#define _QSE_SI_SOCKETADDRESS_HPP_

#include <qse/Types.hpp>
#include <qse/si/nwad.h>
#include <qse/cmn/String.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class SocketAddress
{
public:
	enum
	{
		MAX_IP4AD_STR_LEN = 15, // nnn.nnn.nnn.nnn
		MAX_IP6AD_STR_LEN = 45, // XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX, XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:nnn.nnn.nnn.nnn 
		MAX_IP4AD_PORT_STR_LEN = 21, // nnn.nnn.nnn.nnn:AAAAA 
		MAX_IP6AD_PORT_STR_LEN = 53 // [....]:AAAAA
	};

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

	void setIp4addr (qse_uint32_t ipaddr) QSE_CPP_NOEXCEPT;
	void setIp4addr (const qse_ip4ad_t* ipaddr) QSE_CPP_NOEXCEPT;
	void setIp6addr (const qse_ip6ad_t* ipaddr) QSE_CPP_NOEXCEPT;

	const qse_ip4ad_t* getIp4addr () const QSE_CPP_NOEXCEPT;
	const qse_ip6ad_t* getIp6addr () const QSE_CPP_NOEXCEPT;

	qse_uint16_t getPort() const QSE_CPP_NOEXCEPT; // in network-byte order
	void setPort (qse_uint16_t port) QSE_CPP_NOEXCEPT; // in network-byte order

	qse_uint32_t getScopeId () const QSE_CPP_NOEXCEPT; // in network-byte order
	void setScopeId (qse_uint32_t scope_id) QSE_CPP_NOEXCEPT; // in network-byte order

	// set address with ip address strings
	int set (const qse_skad_t* skad) QSE_CPP_NOEXCEPT; 
	int set (const qse_nwad_t* nwad) QSE_CPP_NOEXCEPT;
	int set (const qse_mchar_t* str) QSE_CPP_NOEXCEPT;
	int set (const qse_wchar_t* str) QSE_CPP_NOEXCEPT;
	int set (const qse_mchar_t* str, qse_size_t len) QSE_CPP_NOEXCEPT;
	int set (const qse_wchar_t* str, qse_size_t len) QSE_CPP_NOEXCEPT;

	// set address with ip address strings or host/service names
	int resolve (const qse_mchar_t* service, const qse_mchar_t* host, int family, int type) QSE_CPP_NOEXCEPT;
	int resolve (const qse_wchar_t* service, const qse_wchar_t* host, int family, int type) QSE_CPP_NOEXCEPT;
	int resolve (const qse_mchar_t* service, const qse_mchar_t* host, int type) QSE_CPP_NOEXCEPT { return this->resolve(service, host, this->getFamily(), type); }
	int resolve (const qse_wchar_t* service, const qse_wchar_t* host, int type) QSE_CPP_NOEXCEPT { return this->resolve(service, host, this->getFamily(), type); }

	bool isLoopBack () const QSE_CPP_NOEXCEPT;
	bool isLinkLocal() const QSE_CPP_NOEXCEPT;
	bool isSiteLocal() const QSE_CPP_NOEXCEPT;
	bool isMulticast() const QSE_CPP_NOEXCEPT;
	bool isV4Mapped() const QSE_CPP_NOEXCEPT;
	
	//TODO: bool isInIpSubnet (const qse_ip4ad_t* addr, int prefix) const  QSE_CPP_NOEXCEPT;
	//TODO: bool isInIpSubnet (const qse_ip6ad_t* addr, int prefix) const  QSE_CPP_NOEXCEPT;
	bool isInIpSubnet (const qse_nwad_t* addr, int prefix) const  QSE_CPP_NOEXCEPT;
	//TODO: bool isInIpSubnet (const qse_skad_t* addr, int prefix) const  QSE_CPP_NOEXCEPT;

	qse_mchar_t* toStrBuf (qse_mchar_t* buf, qse_size_t len) const QSE_CPP_NOEXCEPT;
	qse_wchar_t* toStrBuf (qse_wchar_t* buf, qse_size_t len) const QSE_CPP_NOEXCEPT;

	QSE::MbString toMbString (QSE::Mmgr* mmgr = QSE_NULL) const;
	QSE::WcString toWcString (QSE::Mmgr* mmgr = QSE_NULL) const;
	QSE::String toString (QSE::Mmgr* mmgr = QSE_NULL) const;

	static qse_mchar_t* ip4addrToStrBuf (const qse_ip4ad_t* ipaddr, qse_mchar_t* buf, qse_size_t len);
	static qse_mchar_t* ip4addrToStrBuf (qse_uint32_t ipaddr, qse_mchar_t* buf, qse_size_t len);
	static qse_mchar_t* ip6addrToStrBuf (const qse_ip6ad_t* ipaddr, qse_mchar_t* buf, qse_size_t len);
	
	static qse_wchar_t* ip4addrToStrBuf (const qse_ip4ad_t* ipaddr, qse_wchar_t* buf, qse_size_t len);
	static qse_wchar_t* ip4addrToStrBuf (qse_uint32_t ipaddr, qse_wchar_t* buf, qse_size_t len);
	static qse_wchar_t* ip6addrToStrBuf (const qse_ip6ad_t* ipaddr, qse_wchar_t* buf, qse_size_t len);

protected:
	qse_skad_t skad;
};


#if 0
class IfceAddress
{
	QSE::String   name;
	unsigned int  flags;
	SocketAddress addr;
	SocketAddress netmask;
	SocketAddress broadcast; // also used as dstaddr for a point-to-point interface
};
#endif

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
