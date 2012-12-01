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

#ifndef _QSE_CMN_HTON_H_
#define _QSE_CMN_HTON_H_

#include <qse/types.h>
#include <qse/macros.h>

#ifdef __cplusplus
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

#ifdef __cplusplus
}
#endif

#endif
