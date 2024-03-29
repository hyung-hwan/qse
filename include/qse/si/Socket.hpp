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

#ifndef _QSE_SI_SOCKET_HPP_
#define _QSE_SI_SOCKET_HPP_

#include <qse/Types.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/cmn/Transmittable.hpp>
#include <qse/cmn/ErrorGrab.hpp>
#include <qse/si/SocketAddress.hpp>
#include <qse/si/sck.h>


/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class Socket: public Uncopyable, public Types, public Transmittable, public ErrorGrab64
{
public:
	enum Trait
	{
		T_NONBLOCK = (1 << 0),
		T_CLOEXEC  = (1 << 1)
	};

	Socket () QSE_CPP_NOEXCEPT;
	virtual ~Socket () QSE_CPP_NOEXCEPT;

	int open (int domain, int type, int protocol, int traits = 0) QSE_CPP_NOEXCEPT;
	void close () QSE_CPP_NOEXCEPT;

	int getDomain () const QSE_CPP_NOEXCEPT { return this->domain; }
	qse_sck_hnd_t getHandle() const QSE_CPP_NOEXCEPT { return this->handle; }
	bool isOpen() const QSE_CPP_NOEXCEPT { return this->handle != QSE_INVALID_SCKHND; }

	// -------------------------------------------------------------------- 

	int getSockName (SocketAddress& addr) QSE_CPP_NOEXCEPT;
	int getPeerName (SocketAddress& addr) QSE_CPP_NOEXCEPT;

	int getOption (int level, int optname, void* optval, qse_sck_len_t* optlen) QSE_CPP_NOEXCEPT;
	int setOption (int level, int optname, const void* optval, qse_sck_len_t optlen) QSE_CPP_NOEXCEPT;

	// -------------------------------------------------------------------- 
	int getSendBuf (unsigned int& n) QSE_CPP_NOEXCEPT;
	int getRecvBuf (unsigned int& n) QSE_CPP_NOEXCEPT;

	// -------------------------------------------------------------------- 
	int setDebug (int n) QSE_CPP_NOEXCEPT;
	int setReuseAddr (int n) QSE_CPP_NOEXCEPT;
	int setReusePort (int n) QSE_CPP_NOEXCEPT;
	int setKeepAlive (int n, int keepidle = 0, int keepintvl = 0, int keepcnt = 0) QSE_CPP_NOEXCEPT;
	int setBroadcast (int n) QSE_CPP_NOEXCEPT;
	int setSendBuf (unsigned int n) QSE_CPP_NOEXCEPT;
	int setRecvBuf (unsigned int n) QSE_CPP_NOEXCEPT;
	int setLingerOn (int sec) QSE_CPP_NOEXCEPT;
	int setLingerOff () QSE_CPP_NOEXCEPT;
	int setTcpNodelay (int n) QSE_CPP_NOEXCEPT;
	int setOobInline (int n) QSE_CPP_NOEXCEPT;
	int setIpv6Only (int n) QSE_CPP_NOEXCEPT;
	int setRecvPktinfo (int n) QSE_CPP_NOEXCEPT;
	int setNonBlock (int n) QSE_CPP_NOEXCEPT;

	// -------------------------------------------------------------------- 
	int shutdown (int how = 2) QSE_CPP_NOEXCEPT;
	int connect (const SocketAddress& target) QSE_CPP_NOEXCEPT;

	int initConnectNB (const SocketAddress& target) QSE_CPP_NOEXCEPT;
	int finiConnectNB () QSE_CPP_NOEXCEPT;

	// -------------------------------------------------------------------- 
	int bind (const SocketAddress& target) QSE_CPP_NOEXCEPT;
	// bind to the ip address of the interface 
	int bindToIfceAddr (const qse_mchar_t* ifce, qse_uint16_t port) QSE_CPP_NOEXCEPT;
	// bind to the ip address of the interface
	int bindToIfceAddr (const qse_wchar_t* ifce, qse_uint16_t port) QSE_CPP_NOEXCEPT;

	// bind to the interface device
	int bindToIfce (const qse_mchar_t* ifce) QSE_CPP_NOEXCEPT;
	// bind to the interface device
	int bindToIfce (const qse_wchar_t* ifce) QSE_CPP_NOEXCEPT;

	// -------------------------------------------------------------------- 

	int listen (int backlog = 128) QSE_CPP_NOEXCEPT;
	int accept (Socket* newsck, SocketAddress* newaddr, int traits = 0) QSE_CPP_NOEXCEPT;

	// -------------------------------------------------------------------- 
	// The send() functions sends data by attemping a single call to the 
	// underlying system calls
	qse_ssize_t send (const void* buf, qse_size_t len) QSE_CPP_NOEXCEPT;
	qse_ssize_t send (const void* buf, qse_size_t len, const SocketAddress& dstaddr) QSE_CPP_NOEXCEPT;
	qse_ssize_t send (const void* buf, qse_size_t len, const SocketAddress& dstaddr, const SocketAddress& srcaddr) QSE_CPP_NOEXCEPT;
	qse_ssize_t send (const qse_ioptl_t* vec, int count) QSE_CPP_NOEXCEPT;
	qse_ssize_t send (const qse_ioptl_t* vec, int count, const SocketAddress& dstaddr) QSE_CPP_NOEXCEPT;
	qse_ssize_t send (const qse_ioptl_t* vec, int count, const SocketAddress& dstaddr, const SocketAddress& srcaddr) QSE_CPP_NOEXCEPT;

	// The sendx() functions sends data as much as it can, possibly with multiple
	// underlying system calls. these are useful for stream sockets and may not
	// be useful for datagram sockets they may split the given data into more than
	// one packet.
	int sendx (const void* buf, qse_size_t len, qse_size_t* total_sent = QSE_NULL) QSE_CPP_NOEXCEPT;
	int sendx (const void* buf, qse_size_t len, const SocketAddress& dstaddr, qse_size_t* total_sent = QSE_NULL) QSE_CPP_NOEXCEPT;
	int sendx (qse_ioptl_t* vec, int count, qse_size_t* total_sent = QSE_NULL) QSE_CPP_NOEXCEPT;
	int sendx (qse_ioptl_t* vec, int count, const SocketAddress& dstaddr, qse_size_t* total_sent = QSE_NULL) QSE_CPP_NOEXCEPT;

	qse_ssize_t receive (void* buf, qse_size_t len) QSE_CPP_NOEXCEPT;
	qse_ssize_t receive (void* buf, qse_size_t len, SocketAddress& srcaddr) QSE_CPP_NOEXCEPT;
	qse_ssize_t receive (void* buf, qse_size_t len, SocketAddress& srcaddr, int& ifindex, SocketAddress* dstaddr = QSE_NULL) QSE_CPP_NOEXCEPT;

	// -------------------------------------------------------------------- 

	int joinMulticastGroup (const SocketAddress& mcaddr, const SocketAddress& ifaddr) QSE_CPP_NOEXCEPT;
	int leaveMulticastGroup (const SocketAddress& mcaddr, const SocketAddress& ifaddr) QSE_CPP_NOEXCEPT;

	// -------------------------------------------------------------------- 

	// utility functions to retrieve network configuration information.
	int getIfceIndex (const qse_mchar_t* name) QSE_CPP_NOEXCEPT;
	int getIfceIndex (const qse_wchar_t* name) QSE_CPP_NOEXCEPT;

	int getIfceIndex (const qse_mchar_t* name, qse_size_t len) QSE_CPP_NOEXCEPT;
	int getIfceIndex (const qse_wchar_t* name, qse_size_t len) QSE_CPP_NOEXCEPT;

	// -------------------------------------------------------------------- 
	// the following 24 functions are provided for backward compatibility.
	// it is limited to a single address and they may suffer race condition.
	// for example, you call getIfceAddress() followed by getIfceNetmask().
	// the network configuration information may change in between.
	// the address/netmask pair may not be the valid fixed combination.
	int getIfceAddress (const qse_mchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceAddress (const qse_wchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceNetmask (const qse_mchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceNetmask (const qse_wchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceBroadcast (const qse_mchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceBroadcast (const qse_wchar_t* name, SocketAddress* addr) QSE_CPP_NOEXCEPT;

	int getIfceAddress (const qse_mchar_t* name, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceAddress (const qse_wchar_t* name, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceNetmask (const qse_mchar_t* name, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceNetmask (const qse_wchar_t* name, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceBroadcast (const qse_mchar_t* name, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceBroadcast (const qse_wchar_t* name, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;

	int getIfceAddress (const qse_mchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceAddress (const qse_wchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceNetmask (const qse_mchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceNetmask (const qse_wchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceBroadcast (const qse_mchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT;
	int getIfceBroadcast (const qse_wchar_t* name, qse_size_t len, SocketAddress* addr) QSE_CPP_NOEXCEPT;

	int getIfceAddress (const qse_mchar_t* name, qse_size_t len, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceAddress (const qse_wchar_t* name, qse_size_t len, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceNetmask (const qse_mchar_t* name, qse_size_t len, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceNetmask (const qse_wchar_t* name, qse_size_t len, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceBroadcast (const qse_mchar_t* name, qse_size_t len, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	int getIfceBroadcast (const qse_wchar_t* name, qse_size_t len, SocketAddress* addr, int family) QSE_CPP_NOEXCEPT;
	// -------------------------------------------------------------------- 

protected:
	qse_sck_hnd_t handle;
	int domain;
	qse_uint16_t cached_binding_port;

	int get_ifce_index (const void* name, qse_size_t len, bool wchar);
	int get_ifce_address (int cmd, const void* name, qse_size_t len, bool wchar, SocketAddress* addr, int family);
};


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////


#endif
