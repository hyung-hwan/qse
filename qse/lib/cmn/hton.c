/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
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
