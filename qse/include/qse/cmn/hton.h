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

#ifndef _QSE_CMN_HTON_H_
#define _QSE_CMN_HTON_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_CONST_SWAP16(x) \
	((qse_uint16_t)((((qse_uint16_t)(x) & (qse_uint16_t)0x00ffU) << 8) | \
	                (((qse_uint16_t)(x) & (qse_uint16_t)0xff00U) >> 8) ))

#define QSE_CONST_SWAP32(x) \
	((qse_uint32_t)((((qse_uint32_t)(x) & (qse_uint32_t)0x000000ffUL) << 24) | \
	                (((qse_uint32_t)(x) & (qse_uint32_t)0x0000ff00UL) <<  8) | \
	                (((qse_uint32_t)(x) & (qse_uint32_t)0x00ff0000UL) >>  8) | \
	                (((qse_uint32_t)(x) & (qse_uint32_t)0xff000000UL) >> 24) ))

#if defined(QSE_ENDIAN_LITTLE)
#       define QSE_CONST_NTOH16(x) QSE_CONST_SWAP16(x)
#       define QSE_CONST_HTON16(x) QSE_CONST_SWAP16(x)
#       define QSE_CONST_NTOH32(x) QSE_CONST_SWAP32(x)
#       define QSE_CONST_HTON32(x) QSE_CONST_SWAP32(x)
#elif defined(QSE_ENDIAN_BIG)
#       define QSE_CONST_NTOH16(x) (x)
#       define QSE_CONST_HTON16(x) (x)
#       define QSE_CONST_NTOH32(x) (x)
#       define QSE_CONST_HTON32(x) (x)
#else
#       error UNKNOWN ENDIAN
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(QSE_HAVE_UINT16_T)
QSE_EXPORT qse_uint16_t qse_ntoh16 (
	qse_uint16_t x
);

QSE_EXPORT qse_uint16_t qse_hton16 (
	qse_uint16_t x
);
#endif

#if defined(QSE_HAVE_UINT32_T)
QSE_EXPORT qse_uint32_t qse_ntoh32 (
	qse_uint32_t x
);

QSE_EXPORT qse_uint32_t qse_hton32 (
	qse_uint32_t x
);
#endif

#if defined(QSE_HAVE_UINT64_T)
QSE_EXPORT qse_uint64_t qse_ntoh64 (
	qse_uint64_t x
);

QSE_EXPORT qse_uint64_t qse_hton64 (
	qse_uint64_t x
);
#endif

#if defined(QSE_HAVE_UINT128_T)
QSE_EXPORT qse_uint128_t qse_ntoh128 (
	qse_uint128_t x
);

QSE_EXPORT qse_uint128_t qse_hton128 (
	qse_uint128_t x
);
#endif

#if defined(__cplusplus)
}
#endif

#endif
