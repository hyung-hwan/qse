/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#ifdef __cplusplus
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

#ifdef __cplusplus
}
#endif

#endif
