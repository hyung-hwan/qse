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

#ifndef _QSE_CMN_IPAD_H_
#define _QSE_CMN_IPAD_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/hton.h>

typedef struct qse_ip4ad_t qse_ip4ad_t;
typedef struct qse_ip6ad_t qse_ip6ad_t;

#include <qse/pack1.h>
struct qse_ip4ad_t
{
	qse_uint32_t value;
};

struct qse_ip6ad_t
{
	qse_uint8_t value[16];
};

#define QSE_IP4AD_IS_LOOPBACK(addr) ((((addr)->value) & QSE_CONST_HTON32(0xFF000000)) == QSE_CONST_HTON32(0x7F000000))

#define QSE_IP6AD_IS_LOOPBACK(addr) \
		((addr)->value[0] == 0 && (addr)->value[1] == 0 && \
		 (addr)->value[2] == 0 && (addr)->value[3] == 0 && \
		 (addr)->value[4] == 0 && (addr)->value[5] == 0 && \
		 (addr)->value[6] == 0 && (addr)->value[7] == 0 && \
		 (addr)->value[8] == 0 && (addr)->value[9] == 0 && \
		 (addr)->value[10] == 0 && (addr)->value[11] == 0 && \
		 (addr)->value[12] == 0 && (addr)->value[13] == 0 && \
		 (addr)->value[14] == 0 && (addr)->value[15] == 1)

// FE80::/10
#define QSE_IP6AD_IS_LINK_LOCAL(addr) (this->value[0] == 0xFE && (this->value[1] & 0xC0) == 0x80)
// FEC0::/10
#define QSE_IP6AD_IS_SITE_LOCAL(addr) (this->value[0] == 0xFE && (this->value[1] & 0xC0) == 0xC0)

#define QSE_IP6AD_IS_MULTICAST(addr) ((addr)->value[0] == 0xFF)
#define QSE_IP6AD_IS_MULTICAST_LINK_LOCAL(addr) ((addr)->value[0] == 0xFF && ((addr)->value[1] & 0x0F) == 0x02)
#define QSE_IP6AD_IS_MULTICAST_SITE_LOCAL(addr) ((addr)->value[0] == 0xFF && ((addr)->value[1] & 0x0F) == 0x05)
#define QSE_IP6AD_IS_MULTICAST_ORGANIZATION(addr) ((addr)->value[0] == 0xFF && ((addr)->value[1] & 0x0F) == 0x08)
#define QSE_IP6AD_IS_MULTICAST_GLOBAL(addr) ((addr)->value[0] == 0xFF && ((addr)->value[1] & 0x0F) == 0x0E)
#define QSE_IP6AD_IS_MULTICAST_INTERFACE_LOCAL(addr) ((addr)->value[0] == 0xFF && ((addr)->value[1] & 0x0F) == 0x01)

#if defined(__cplusplus)
struct qse_ip4adxx_t: public qse_ip4ad_t
{
	qse_ip4adxx_t (qse_uint32_t v = 0)  
	{
		this->value = v;
	}

	bool operator== (const qse_ip4adxx_t& peer) const { return this->value == peer.value; }
	bool operator!= (const qse_ip4adxx_t& peer) const { return this->value != peer.value; }
	bool operator<  (const qse_ip4adxx_t& peer) const { return qse_ntoh32(this->value) < qse_ntoh32(peer.value); }
	bool operator<= (const qse_ip4adxx_t& peer) const { return qse_ntoh32(this->value) <= qse_ntoh32(peer.value); }
	bool operator>  (const qse_ip4adxx_t& peer) const { return qse_ntoh32(this->value) > qse_ntoh32(peer.value); }
	bool operator>= (const qse_ip4adxx_t& peer) const { return qse_ntoh32(this->value) >= qse_ntoh32(peer.value); }

	qse_ip4adxx_t& operator= (const qse_ip4adxx_t& v)
	{
		this->value = v.value;
		return *this;
	}
	qse_ip4adxx_t& operator+= (const qse_ip4adxx_t& v) 
	{
		this->value = qse_hton32(qse_ntoh32(this->value) + qse_ntoh32(v.value));
		return *this;
	}
	qse_ip4adxx_t& operator-= (const qse_ip4adxx_t& v) 
	{
		this->value = qse_hton32(qse_ntoh32(this->value) - qse_ntoh32(v.value));
		return *this;
	}
	qse_ip4adxx_t operator+ (const qse_ip4adxx_t& v) const
	{
		qse_ip4adxx_t x;
		x.value = qse_hton32(qse_ntoh32(this->value) + qse_ntoh32(v.value));
		return x;
	}
	qse_ip4adxx_t operator- (const qse_ip4adxx_t& v) const
	{
		qse_ip4adxx_t x;
		x.value = qse_hton32(qse_ntoh32(this->value) - qse_ntoh32(v.value));
		return x;
	}

	qse_ip4adxx_t& operator= (qse_uint32_t v)
	{
		this->value = qse_hton32(v);
		return *this;
	}
	qse_ip4adxx_t& operator+= (qse_uint32_t v) 
	{
		this->value = qse_hton32(qse_ntoh32(this->value) + v);
		return *this;
	}
	qse_ip4adxx_t& operator-= (qse_uint32_t v) 
	{
		this->value = qse_hton32(qse_ntoh32(this->value) - v);
		return *this;
	}
	qse_ip4adxx_t operator+ (qse_uint32_t v) const
	{
		qse_ip4adxx_t x;
		x.value = qse_hton32(qse_ntoh32(this->value) + v);
		return x;
	}
	qse_ip4adxx_t operator- (qse_uint32_t v) const
	{
		qse_ip4adxx_t x;
		x.value = qse_hton32(qse_ntoh32(this->value) - v);
		return x;
	}
};

struct qse_ip6adxx_t: public qse_ip6ad_t
{
	struct ad64_t
	{
		qse_uint64_t w[2];
	};

	qse_ip6adxx_t ()
	{
		register ad64_t* x = (ad64_t*)this->value;
		x->w[0] = 0;
		x->w[1] = 0;
	}

	qse_ip6adxx_t (qse_uint8_t (*value)[16])
	{
		register ad64_t* x = (ad64_t*)this->value;
		x->w[0] = ((ad64_t*)value)->w[0];
		x->w[1] = ((ad64_t*)value)->w[1];
	}

	qse_ip6adxx_t (qse_uint8_t (&value)[16])
	{
		register ad64_t* x = (ad64_t*)this->value;
		x->w[0] = ((ad64_t*)&value)->w[0];
		x->w[1] = ((ad64_t*)&value)->w[1];
	}

	bool operator== (const qse_ip4adxx_t& peer) const 
	{
		register ad64_t* x = (ad64_t*)this->value;
		register ad64_t* y = (ad64_t*)&peer.value;
		return x->w[0] == y->w[0] && x->w[1] == y->w[1];
	}

	bool operator!= (const qse_ip4adxx_t& peer) const 
	{
		register ad64_t* x = (ad64_t*)this->value;
		register ad64_t* y = (ad64_t*)&peer.value;
		return x->w[0] != y->w[0] || x->w[1] != y->w[1];
	}

	qse_ip6adxx_t& operator= (const qse_ip6adxx_t& v)
	{
		register ad64_t* x = (ad64_t*)this->value;
		x->w[0] = ((ad64_t*)&v)->w[0];
		x->w[1] = ((ad64_t*)&v)->w[1];
		return *this;
	}

	bool isZero() const
	{
		register ad64_t* x = (ad64_t*)this->value;
		return x->w[0] == 0 && x->w[1] == 0;
	}

	bool isLoopback() const
	{
		return QSE_IP6AD_IS_LOOPBACK(this);
	}

	bool isLinkLocal() const
	{
		// FE80::/10
		return QSE_IP6AD_IS_LINK_LOCAL(this);
	}

	bool isSiteLocal() const
	{
		// FEC0::/10
		return QSE_IP6AD_IS_SITE_LOCAL(this);
	}

	// multicast addresses
	// ff02:: Link Local: spans the same topological region as the corresponding unicast scope, i.e. all nodes on the same LAN.
	// ff05:: Site local: is intended to span a single site
	// ff08:: Organization scope: Intended to span multiple sizes within the same organization
	// ff0e:: Global scope, assigned by IANA.
	// ff01:: Interface local: Spans only a single interface on a node and is useful only for loopback transmission of multicast.
	bool isMulticast() const
	{
		return QSE_IP6AD_IS_MULTICAST(this);
	}

	bool isMulticastLinkLocal() const
	{
		return QSE_IP6AD_IS_MULTICAST_LINK_LOCAL(this);
	}

	bool isMulticastSiteLocal() const
	{
		return QSE_IP6AD_IS_MULTICAST_SITE_LOCAL(this);
	}

	bool isMulticastOrganization() const
	{
		return QSE_IP6AD_IS_MULTICAST_ORGANIZATION(this);
	}

	bool isMulticastGlobal() const
	{
		return QSE_IP6AD_IS_MULTICAST_GLOBAL(this);
	}

	bool isMulticastInterfaceLocal() const
	{
		return QSE_IP6AD_IS_MULTICAST_INTERFACE_LOCAL(this);
	}

	bool isV4Mapped () const
	{
		return this->value[0] == 0x00 && this->value[1] == 0x00 &&
		       this->value[2] == 0x00 && this->value[3] == 0x00 &&
		       this->value[4] == 0x00 && this->value[5] == 0x00 &&
		       this->value[6] == 0x00 && this->value[7] == 0x00 &&
		       this->value[8] == 0x00 && this->value[9] == 0x00 &&
		       this->value[10] == 0xFF && this->value[11] == 0xFF;
	}
};
#endif

#include <qse/unpack.h>




#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_mbstoip4ad (
	const qse_mchar_t* mbs,
	qse_ip4ad_t*       ipad
);

QSE_EXPORT int qse_mbsntoip4ad (
	const qse_mchar_t* mbs,
	qse_size_t         len,
	qse_ip4ad_t*       ipad
);

QSE_EXPORT int qse_wcstoip4ad (
	const qse_wchar_t* wcs,
	qse_ip4ad_t*       ipad
);

QSE_EXPORT int qse_wcsntoip4ad (
	const qse_wchar_t* wcs,
	qse_size_t         len,
	qse_ip4ad_t*       ipad
);

QSE_EXPORT qse_size_t qse_ip4adtombs (
	const qse_ip4ad_t* ipad,
	qse_mchar_t*       mbs,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_ip4adtowcs (
	const qse_ip4ad_t* ipad,
	qse_wchar_t*       wcs,
	qse_size_t         len
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strtoip4ad(ptr,ipad)      qse_mbstoip4ad(ptr,ipad)
#	define qse_strntoip4ad(ptr,len,ipad) qse_mbsntoip4ad(ptr,len,ipad)
#	define qse_ip4adtostr(ipad,ptr,len)  qse_ip4adtombs(ipad,ptr,len)
#else
#	define qse_strtoip4ad(ptr,ipad)      qse_wcstoip4ad(ptr,ipad)
#	define qse_strntoip4ad(ptr,len,ipad) qse_wcsntoip4ad(ptr,len,ipad)
#	define qse_ip4adtostr(ipad,ptr,len)  qse_ip4adtowcs(ipad,ptr,len)
#endif

QSE_EXPORT int qse_mbstoip6ad (
	const qse_mchar_t* mbs,
	qse_ip6ad_t*       ipad
);

QSE_EXPORT int qse_mbsntoip6ad (
	const qse_mchar_t* mbs,
	qse_size_t         len,
	qse_ip6ad_t*       ipad
);

QSE_EXPORT int qse_wcstoip6ad (
	const qse_wchar_t* wcs,
	qse_ip6ad_t*       ipad
);

QSE_EXPORT int qse_wcsntoip6ad (
	const qse_wchar_t* wcs,
	qse_size_t         len,
	qse_ip6ad_t*       ipad
);

QSE_EXPORT qse_size_t qse_ip6adtombs (
	const qse_ip6ad_t* ipad,
	qse_mchar_t*       mbs,
	qse_size_t         len
);

QSE_EXPORT qse_size_t qse_ip6adtowcs (
	const qse_ip6ad_t* ipad,
	qse_wchar_t*       wcs,
	qse_size_t         len
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strtoip6ad(ptr,ipad)      qse_mbstoip6ad(ptr,ipad)
#	define qse_strntoip6ad(ptr,len,ipad) qse_mbsntoip6ad(ptr,len,ipad)
#	define qse_ip6adtostr(ipad,ptr,len)  qse_ip6adtombs(ipad,ptr,len)
#else
#	define qse_strtoip6ad(ptr,ipad)      qse_wcstoip6ad(ptr,ipad)
#	define qse_strntoip6ad(ptr,len,ipad) qse_wcsntoip6ad(ptr,len,ipad)
#	define qse_ip6adtostr(ipad,ptr,len)  qse_ip6adtowcs(ipad,ptr,len)
#endif 

/*
 * The qse_prefixtoip4ad() function converts the prefix length
 * to an IPv4 address mask.  The prefix length @a prefix must be
 * between 0 and 32 inclusive.
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_prefixtoip4ad (
	int          prefix,
	qse_ip4ad_t* ipad
);

/*
 * The qse_prefixtoip4ad() function converts the prefix length
 * to an IPv6 address mask. The prefix length @a prefix must be
 * between 0 and 128 inclusive.
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_prefixtoip6ad (
	int          prefix,
	qse_ip6ad_t* ipad
);

#if defined(__cplusplus)
}
#endif

#endif
