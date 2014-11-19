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

#include <qse/cmn/hton.h>


/* --------------------------------------------------------------- */

#if defined(QSE_HAVE_UINT16_T)

qse_uint16_t qse_ntoh16 (qse_uint16_t x)
{
#if defined(QSE_ENDIAN_BIG)
	return x;
#elif defined(QSE_ENDIAN_LITTLE)
	qse_uint8_t* c = (qse_uint8_t*)&x;
	return (qse_uint16_t)(
		((qse_uint16_t)c[0] << 8) |
		((qse_uint16_t)c[1] << 0));
#else
#	error Unknown endian
#endif
}

qse_uint16_t qse_hton16 (qse_uint16_t x)
{
#if defined(QSE_ENDIAN_BIG)
	return x;
#elif defined(QSE_ENDIAN_LITTLE)
	qse_uint8_t* c = (qse_uint8_t*)&x;
	return (qse_uint16_t)(
		((qse_uint16_t)c[0] << 8) |
		((qse_uint16_t)c[1] << 0));
#else
#	error Unknown endian
#endif
}

#endif

/* --------------------------------------------------------------- */

#if defined(QSE_HAVE_UINT32_T)

qse_uint32_t qse_ntoh32 (qse_uint32_t x)
{
#if defined(QSE_ENDIAN_BIG)
	return x;
#elif defined(QSE_ENDIAN_LITTLE)
	qse_uint8_t* c = (qse_uint8_t*)&x;
	return (qse_uint32_t)(
		((qse_uint32_t)c[0] << 24) |
		((qse_uint32_t)c[1] << 16) |
		((qse_uint32_t)c[2] << 8) | 
		((qse_uint32_t)c[3] << 0));
#else
#	error Unknown endian
#endif
}

qse_uint32_t qse_hton32 (qse_uint32_t x)
{
#if defined(QSE_ENDIAN_BIG)
	return x;
#elif defined(QSE_ENDIAN_LITTLE)
	qse_uint8_t* c = (qse_uint8_t*)&x;
	return (qse_uint32_t)(
		((qse_uint32_t)c[0] << 24) |
		((qse_uint32_t)c[1] << 16) |
		((qse_uint32_t)c[2] << 8) | 
		((qse_uint32_t)c[3] << 0));
#else
#	error Unknown endian
#endif
}
#endif

/* --------------------------------------------------------------- */

#if defined(QSE_HAVE_UINT64_T)

qse_uint64_t qse_ntoh64 (qse_uint64_t x)
{
#if defined(QSE_ENDIAN_BIG)
	return x;
#elif defined(QSE_ENDIAN_LITTLE)
	qse_uint8_t* c = (qse_uint8_t*)&x;
	return (qse_uint64_t)(
		((qse_uint64_t)c[0] << 56) | 
		((qse_uint64_t)c[1] << 48) | 
		((qse_uint64_t)c[2] << 40) |
		((qse_uint64_t)c[3] << 32) |
		((qse_uint64_t)c[4] << 24) |
		((qse_uint64_t)c[5] << 16) |
		((qse_uint64_t)c[6] << 8)  |
		((qse_uint64_t)c[7] << 0));
#else
#	error Unknown endian
#endif
}

qse_uint64_t qse_hton64 (qse_uint64_t x)
{
#if defined(QSE_ENDIAN_BIG)
	return x;
#elif defined(QSE_ENDIAN_LITTLE)
	qse_uint8_t* c = (qse_uint8_t*)&x;
	return (qse_uint64_t)(
		((qse_uint64_t)c[0] << 56) | 
		((qse_uint64_t)c[1] << 48) | 
		((qse_uint64_t)c[2] << 40) |
		((qse_uint64_t)c[3] << 32) |
		((qse_uint64_t)c[4] << 24) |
		((qse_uint64_t)c[5] << 16) |
		((qse_uint64_t)c[6] << 8)  |
		((qse_uint64_t)c[7] << 0));
#else
#	error Unknown endian
#endif
}

#endif

/* --------------------------------------------------------------- */

#if defined(QSE_HAVE_UINT128_T)

qse_uint128_t qse_ntoh128 (qse_uint128_t x)
{
#if defined(QSE_ENDIAN_BIG)
	return x;
#elif defined(QSE_ENDIAN_LITTLE)
	qse_uint8_t* c = (qse_uint8_t*)&x;
	return (qse_uint128_t)(
		((qse_uint128_t)c[0]  << 120) | 
		((qse_uint128_t)c[1]  << 112) | 
		((qse_uint128_t)c[2]  << 104) |
		((qse_uint128_t)c[3]  << 96) |
		((qse_uint128_t)c[4]  << 88) |
		((qse_uint128_t)c[5]  << 80) |
		((qse_uint128_t)c[6]  << 72) |
		((qse_uint128_t)c[7]  << 64) |
		((qse_uint128_t)c[8]  << 56) | 
		((qse_uint128_t)c[9]  << 48) | 
		((qse_uint128_t)c[10] << 40) |
		((qse_uint128_t)c[11] << 32) |
		((qse_uint128_t)c[12] << 24) |
		((qse_uint128_t)c[13] << 16) |
		((qse_uint128_t)c[14] << 8)  |
		((qse_uint128_t)c[15] << 0));
#else
#	error Unknown endian
#endif
}

qse_uint128_t qse_hton128 (qse_uint128_t x)
{
#if defined(QSE_ENDIAN_BIG)
	return x;
#elif defined(QSE_ENDIAN_LITTLE)
	qse_uint8_t* c = (qse_uint8_t*)&x;
	return (qse_uint128_t)(
		((qse_uint128_t)c[0]  << 120) | 
		((qse_uint128_t)c[1]  << 112) | 
		((qse_uint128_t)c[2]  << 104) |
		((qse_uint128_t)c[3]  << 96) |
		((qse_uint128_t)c[4]  << 88) |
		((qse_uint128_t)c[5]  << 80) |
		((qse_uint128_t)c[6]  << 72) |
		((qse_uint128_t)c[7]  << 64) |
		((qse_uint128_t)c[8]  << 56) | 
		((qse_uint128_t)c[9]  << 48) | 
		((qse_uint128_t)c[10] << 40) |
		((qse_uint128_t)c[11] << 32) |
		((qse_uint128_t)c[12] << 24) |
		((qse_uint128_t)c[13] << 16) |
		((qse_uint128_t)c[14] << 8)  |
		((qse_uint128_t)c[15] << 0));
#else
#	error Unknown endian
#endif
}

#endif

/* --------------------------------------------------------------- */
