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

#ifndef _QSE_CMN_IPAD_H_
#define _QSE_CMN_IPAD_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_ipad_t  qse_ipad_t;
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
