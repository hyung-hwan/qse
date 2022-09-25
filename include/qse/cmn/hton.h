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

#ifndef _QSE_CMN_HTON_H_
#define _QSE_CMN_HTON_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_CONST_BSWAP16(x) \
	((qse_uint16_t)((((qse_uint16_t)(x) & ((qse_uint16_t)0xff << 0)) << 8) | \
	                (((qse_uint16_t)(x) & ((qse_uint16_t)0xff << 8)) >> 8)))

#define QSE_CONST_BSWAP32(x) \
	((qse_uint32_t)((((qse_uint32_t)(x) & ((qse_uint32_t)0xff <<  0)) << 24) | \
	                (((qse_uint32_t)(x) & ((qse_uint32_t)0xff <<  8)) <<  8) | \
	                (((qse_uint32_t)(x) & ((qse_uint32_t)0xff << 16)) >>  8) | \
	                (((qse_uint32_t)(x) & ((qse_uint32_t)0xff << 24)) >> 24)))

#if defined(QSE_HAVE_UINT64_T)
#define QSE_CONST_BSWAP64(x) \
	((qse_uint64_t)((((qse_uint64_t)(x) & ((qse_uint64_t)0xff <<  0)) << 56) | \
	                (((qse_uint64_t)(x) & ((qse_uint64_t)0xff <<  8)) << 40) | \
	                (((qse_uint64_t)(x) & ((qse_uint64_t)0xff << 16)) << 24) | \
	                (((qse_uint64_t)(x) & ((qse_uint64_t)0xff << 24)) <<  8) | \
	                (((qse_uint64_t)(x) & ((qse_uint64_t)0xff << 32)) >>  8) | \
	                (((qse_uint64_t)(x) & ((qse_uint64_t)0xff << 40)) >> 24) | \
	                (((qse_uint64_t)(x) & ((qse_uint64_t)0xff << 48)) >> 40) | \
	                (((qse_uint64_t)(x) & ((qse_uint64_t)0xff << 56)) >> 56)))
#endif

#if defined(QSE_HAVE_UINT128_T)
#define QSE_CONST_BSWAP128(x) \
	((qse_uint128_t)((((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 0)) << 120) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 8)) << 104) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 16)) << 88) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 24)) << 72) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 32)) << 56) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 40)) << 40) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 48)) << 24) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 56)) << 8) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 64)) >> 8) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 72)) >> 24) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 80)) >> 40) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 88)) >> 56) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 96)) >> 72) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 104)) >> 88) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 112)) >> 104) | \
	                 (((qse_uint128_t)(x) & ((qse_uint128_t)0xff << 120)) >> 120)))
#endif

#if defined(QSE_ENDIAN_LITTLE)

#	if defined(QSE_HAVE_UINT16_T)
#	define QSE_CONST_NTOH16(x) QSE_CONST_BSWAP16(x)
#	define QSE_CONST_HTON16(x) QSE_CONST_BSWAP16(x)
#	define QSE_CONST_HTOBE16(x) QSE_CONST_BSWAP16(x)
#	define QSE_CONST_HTOLE16(x) (x)
#	define QSE_CONST_BE16TOH(x) QSE_CONST_BSWAP16(x)
#	define QSE_CONST_LE16TOH(x) (x)
#	endif

#	if defined(QSE_HAVE_UINT32_T)
#	define QSE_CONST_NTOH32(x) QSE_CONST_BSWAP32(x)
#	define QSE_CONST_HTON32(x) QSE_CONST_BSWAP32(x)
#	define QSE_CONST_HTOBE32(x) QSE_CONST_BSWAP32(x)
#	define QSE_CONST_HTOLE32(x) (x)
#	define QSE_CONST_BE32TOH(x) QSE_CONST_BSWAP32(x)
#	define QSE_CONST_LE32TOH(x) (x)
#	endif

#	if defined(QSE_HAVE_UINT64_T)
#	define QSE_CONST_NTOH64(x) QSE_CONST_BSWAP64(x)
#	define QSE_CONST_HTON64(x) QSE_CONST_BSWAP64(x)
#	define QSE_CONST_HTOBE64(x) QSE_CONST_BSWAP64(x)
#	define QSE_CONST_HTOLE64(x) (x)
#	define QSE_CONST_BE64TOH(x) QSE_CONST_BSWAP64(x)
#	define QSE_CONST_LE64TOH(x) (x)
#	endif

#	if defined(QSE_HAVE_UINT128_T)
#	define QSE_CONST_NTOH128(x) QSE_CONST_BSWAP128(x)
#	define QSE_CONST_HTON128(x) QSE_CONST_BSWAP128(x)
#	define QSE_CONST_HTOBE128(x) QSE_CONST_BSWAP128(x)
#	define QSE_CONST_HTOLE128(x) (x)
#	define QSE_CONST_BE128TOH(x) QSE_CONST_BSWAP128(x)
#	define QSE_CONST_LE128TOH(x) (x)
#endif

#elif defined(QSE_ENDIAN_BIG)

#	if defined(QSE_HAVE_UINT16_T)
#	define QSE_CONST_NTOH16(x) (x)
#	define QSE_CONST_HTON16(x) (x)
#	define QSE_CONST_HTOBE16(x) (x)
#	define QSE_CONST_HTOLE16(x) QSE_CONST_BSWAP16(x)
#	define QSE_CONST_BE16TOH(x) (x)
#	define QSE_CONST_LE16TOH(x) QSE_CONST_BSWAP16(x)
#	endif

#	if defined(QSE_HAVE_UINT32_T)
#	define QSE_CONST_NTOH32(x) (x)
#	define QSE_CONST_HTON32(x) (x)
#	define QSE_CONST_HTOBE32(x) (x)
#	define QSE_CONST_HTOLE32(x) QSE_CONST_BSWAP32(x)
#	define QSE_CONST_BE32TOH(x) (x)
#	define QSE_CONST_LE32TOH(x) QSE_CONST_BSWAP32(x)
#	endif

#	if defined(QSE_HAVE_UINT64_T)
#	define QSE_CONST_NTOH64(x) (x)
#	define QSE_CONST_HTON64(x) (x)
#	define QSE_CONST_HTOBE64(x) (x)
#	define QSE_CONST_HTOLE64(x) QSE_CONST_BSWAP64(x)
#	define QSE_CONST_BE64TOH(x) (x)
#	define QSE_CONST_LE64TOH(x) QSE_CONST_BSWAP64(x)
#	endif

#	if defined(QSE_HAVE_UINT128_T)
#	define QSE_CONST_NTOH128(x) (x)
#	define QSE_CONST_HTON128(x) (x)
#	define QSE_CONST_HTOBE128(x) (x)
#	define QSE_CONST_HTOLE128(x) QSE_CONST_BSWAP128(x)
#	define QSE_CONST_BE128TOH(x) (x)
#	define QSE_CONST_LE128TOH(x) QSE_CONST_BSWAP128(x)
#	endif

#else
#	error UNKNOWN ENDIAN
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(QSE_HAVE_INLINE)

#if defined(QSE_HAVE_UINT16_T)
static QSE_INLINE qse_uint16_t qse_bswap16 (qse_uint16_t x)
{
#if defined(QSE_HAVE_BUILTIN_BSWAP16)
	return __builtin_bswap16(x);
#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64) || defined(__i386) || defined(i386))
	__asm__ volatile ("xchgb %b0, %h0" : "=Q"(x): "0"(x));
	return x;
#else
	return (x << 8) | (x >> 8);
#endif
}
#endif

#if defined(QSE_HAVE_UINT32_T)
static QSE_INLINE qse_uint32_t qse_bswap32 (qse_uint32_t x)
{
#if defined(QSE_HAVE_BUILTIN_BSWAP32)
	return __builtin_bswap32(x);
#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64) || defined(__i386) || defined(i386))
	__asm__ volatile ("bswapl %0" : "=r"(x) : "0"(x));
	return x;
#else
	return ((x >> 24)) | 
	       ((x >>  8) & ((qse_uint32_t)0xff << 8)) | 
	       ((x <<  8) & ((qse_uint32_t)0xff << 16)) | 
	       ((x << 24));
#endif
}
#endif

#if defined(QSE_HAVE_UINT64_T)
static QSE_INLINE qse_uint64_t qse_bswap64 (qse_uint64_t x)
{
#if defined(QSE_HAVE_BUILTIN_BSWAP64)
	return __builtin_bswap64(x);
#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64))
	__asm__ volatile ("bswapq %0" : "=r"(x) : "0"(x));
	return x;
#else
	return ((x >> 56)) | 
	       ((x >> 40) & ((qse_uint64_t)0xff << 8)) | 
	       ((x >> 24) & ((qse_uint64_t)0xff << 16)) | 
	       ((x >>  8) & ((qse_uint64_t)0xff << 24)) | 
	       ((x <<  8) & ((qse_uint64_t)0xff << 32)) | 
	       ((x << 24) & ((qse_uint64_t)0xff << 40)) | 
	       ((x << 40) & ((qse_uint64_t)0xff << 48)) | 
	       ((x << 56));
#endif
}
#endif

#if defined(QSE_HAVE_UINT128_T)
static QSE_INLINE qse_uint128_t qse_bswap128 (qse_uint128_t x)
{
#if defined(QSE_HAVE_BUILTIN_BSWAP128)
	return __builtin_bswap128(x);
#else
	return ((x >> 120)) | 
	       ((x >> 104) & ((qse_uint128_t)0xff << 8)) |
	       ((x >>  88) & ((qse_uint128_t)0xff << 16)) |
	       ((x >>  72) & ((qse_uint128_t)0xff << 24)) |
	       ((x >>  56) & ((qse_uint128_t)0xff << 32)) |
	       ((x >>  40) & ((qse_uint128_t)0xff << 40)) |
	       ((x >>  24) & ((qse_uint128_t)0xff << 48)) |
	       ((x >>   8) & ((qse_uint128_t)0xff << 56)) |
	       ((x <<   8) & ((qse_uint128_t)0xff << 64)) |
	       ((x <<  24) & ((qse_uint128_t)0xff << 72)) |
	       ((x <<  40) & ((qse_uint128_t)0xff << 80)) |
	       ((x <<  56) & ((qse_uint128_t)0xff << 88)) |
	       ((x <<  72) & ((qse_uint128_t)0xff << 96)) |
	       ((x <<  88) & ((qse_uint128_t)0xff << 104)) |
	       ((x << 104) & ((qse_uint128_t)0xff << 112)) |
	       ((x << 120));
#endif
}
#endif

#else

#if defined(QSE_HAVE_UINT16_T)
#	define qse_bswap16(x) ((qse_uint16_t)(((qse_uint16_t)(x)) << 8) | (((qse_uint16_t)(x)) >> 8))
#endif

#if defined(QSE_HAVE_UINT32_T)
#	define qse_bswap32(x) ((qse_uint32_t)(((((qse_uint32_t)(x)) >> 24)) | \
	                                      ((((qse_uint32_t)(x)) >>  8) & ((qse_uint32_t)0xff << 8)) | \
	                                      ((((qse_uint32_t)(x)) <<  8) & ((qse_uint32_t)0xff << 16)) | \
	                                      ((((qse_uint32_t)(x)) << 24))))
#endif

#if defined(QSE_HAVE_UINT64_T)
#	define qse_bswap64(x) ((qse_uint64_t)(((((qse_uint64_t)(x)) >> 56)) | \
	                                      ((((qse_uint64_t)(x)) >> 40) & ((qse_uint64_t)0xff << 8)) | \
	                                      ((((qse_uint64_t)(x)) >> 24) & ((qse_uint64_t)0xff << 16)) | \
	                                      ((((qse_uint64_t)(x)) >>  8) & ((qse_uint64_t)0xff << 24)) | \
	                                      ((((qse_uint64_t)(x)) <<  8) & ((qse_uint64_t)0xff << 32)) | \
	                                      ((((qse_uint64_t)(x)) << 24) & ((qse_uint64_t)0xff << 40)) | \
	                                      ((((qse_uint64_t)(x)) << 40) & ((qse_uint64_t)0xff << 48)) | \
	                                      ((((qse_uint64_t)(x)) << 56))))
#endif

#if defined(QSE_HAVE_UINT128_T)
#	define qse_bswap128(x) ((qse_uint128_t)(((((qse_uint128_t)(x)) >> 120)) |  \
	                                        ((((qse_uint128_t)(x)) >> 104) & ((qse_uint128_t)0xff << 8)) | \
	                                        ((((qse_uint128_t)(x)) >>  88) & ((qse_uint128_t)0xff << 16)) | \
	                                        ((((qse_uint128_t)(x)) >>  72) & ((qse_uint128_t)0xff << 24)) | \
	                                        ((((qse_uint128_t)(x)) >>  56) & ((qse_uint128_t)0xff << 32)) | \
	                                        ((((qse_uint128_t)(x)) >>  40) & ((qse_uint128_t)0xff << 40)) | \
	                                        ((((qse_uint128_t)(x)) >>  24) & ((qse_uint128_t)0xff << 48)) | \
	                                        ((((qse_uint128_t)(x)) >>   8) & ((qse_uint128_t)0xff << 56)) | \
	                                        ((((qse_uint128_t)(x)) <<   8) & ((qse_uint128_t)0xff << 64)) | \
	                                        ((((qse_uint128_t)(x)) <<  24) & ((qse_uint128_t)0xff << 72)) | \
	                                        ((((qse_uint128_t)(x)) <<  40) & ((qse_uint128_t)0xff << 80)) | \
	                                        ((((qse_uint128_t)(x)) <<  56) & ((qse_uint128_t)0xff << 88)) | \
	                                        ((((qse_uint128_t)(x)) <<  72) & ((qse_uint128_t)0xff << 96)) | \
	                                        ((((qse_uint128_t)(x)) <<  88) & ((qse_uint128_t)0xff << 104)) | \
	                                        ((((qse_uint128_t)(x)) << 104) & ((qse_uint128_t)0xff << 112)) | \
	                                        ((((qse_uint128_t)(x)) << 120))))
#endif

#endif /* QSE_HAVE_INLINE */


#if defined(QSE_ENDIAN_LITTLE)

#	if defined(QSE_HAVE_UINT16_T)
#	define qse_hton16(x) qse_bswap16(x)
#	define qse_ntoh16(x) qse_bswap16(x)
#	define qse_htobe16(x) qse_bswap16(x)
#	define qse_be16toh(x) qse_bswap16(x)
#	define qse_htole16(x) ((qse_uint16_t)(x))
#	define qse_le16toh(x) ((qse_uint16_t)(x))
#	endif

#	if defined(QSE_HAVE_UINT32_T)
#	define qse_hton32(x) qse_bswap32(x)
#	define qse_ntoh32(x) qse_bswap32(x)
#	define qse_htobe32(x) qse_bswap32(x)
#	define qse_be32toh(x) qse_bswap32(x)
#	define qse_htole32(x) ((qse_uint32_t)(x))
#	define qse_le32toh(x) ((qse_uint32_t)(x))
#	endif

#	if defined(QSE_HAVE_UINT64_T)
#	define qse_hton64(x) qse_bswap64(x)
#	define qse_ntoh64(x) qse_bswap64(x)
#	define qse_htobe64(x) qse_bswap64(x)
#	define qse_be64toh(x) qse_bswap64(x)
#	define qse_htole64(x) ((qse_uint64_t)(x))
#	define qse_le64toh(x) ((qse_uint64_t)(x))
#	endif

#	if defined(QSE_HAVE_UINT128_T)

#	define qse_hton128(x) qse_bswap128(x)
#	define qse_ntoh128(x) qse_bswap128(x)
#	define qse_htobe128(x) qse_bswap128(x)
#	define qse_be128toh(x) qse_bswap128(x)
#	define qse_htole128(x) ((qse_uint128_t)(x))
#	define qse_le128toh(x) ((qse_uint128_t)(x))
#	endif

#elif defined(QSE_ENDIAN_BIG)

#	if defined(QSE_HAVE_UINT16_T)
#	define qse_hton16(x) ((qse_uint16_t)(x))
#	define qse_ntoh16(x) ((qse_uint16_t)(x))
#	define qse_htobe16(x) ((qse_uint16_t)(x))
#	define qse_be16toh(x) ((qse_uint16_t)(x))
#	define qse_htole16(x) qse_bswap16(x)
#	define qse_le16toh(x) qse_bswap16(x)
#	endif

#	if defined(QSE_HAVE_UINT32_T)
#	define qse_hton32(x) ((qse_uint32_t)(x))
#	define qse_ntoh32(x) ((qse_uint32_t)(x))
#	define qse_htobe32(x) ((qse_uint32_t)(x))
#	define qse_be32toh(x) ((qse_uint32_t)(x))
#	define qse_htole32(x) qse_bswap32(x)
#	define qse_le32toh(x) qse_bswap32(x)
#	endif

#	if defined(QSE_HAVE_UINT64_T)
#	define qse_hton64(x) ((qse_uint64_t)(x))
#	define qse_ntoh64(x) ((qse_uint64_t)(x))
#	define qse_htobe64(x) ((qse_uint64_t)(x))
#	define qse_be64toh(x) ((qse_uint64_t)(x))
#	define qse_htole64(x) qse_bswap64(x)
#	define qse_le64toh(x) qse_bswap64(x)
#	endif

#	if defined(QSE_HAVE_UINT128_T)
#	define qse_hton128(x) ((qse_uint128_t)(x))
#	define qse_ntoh128(x) ((qse_uint128_t)(x))
#	define qse_htobe128(x) ((qse_uint128_t)(x))
#	define qse_be128toh(x) ((qse_uint128_t)(x))
#	define qse_htole128(x) qse_bswap128(x)
#	define qse_le128toh(x) qse_bswap128(x)
#	endif

#else
#	error UNKNOWN ENDIAN
#endif

#if defined(__cplusplus)
}
#endif

#endif
