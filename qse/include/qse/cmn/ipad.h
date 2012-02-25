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

#ifndef _QSE_CMN_IPAD_H_
#define _QSE_CMN_IPAD_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_ipad_t  qse_ipad_t;
typedef struct qse_ipad4_t qse_ipad4_t;
typedef struct qse_ipad6_t qse_ipad6_t;

#include <qse/pack1.h>
struct qse_ipad4_t
{
	qse_uint32_t value;
};
struct qse_ipad6_t
{
	qse_uint8_t value[16];
};
#include <qse/unpack.h>

struct qse_ipad_t
{
	enum
	{
		QSE_IPAD_V4,
		QSE_IPAD_V6
	} type;

	union
	{
		qse_ipad4_t v4;
		qse_ipad6_t v6;
	} u;
};

#ifdef __cplusplus
extern "C" {
#endif

int qse_mbstoipad4 (
	const qse_mchar_t* mbs,
	qse_ipad4_t*       ipad
);

int qse_mbsntoipad4 (
	const qse_mchar_t* mbs,
	qse_size_t         len
);

int qse_ipad4tombs (
	const qse_ipad4_t* ipad,
	qse_mchar_t*       mbs,
	qse_size_t         len
);

int qse_wcstoipad4 (
	const qse_wchar_t* wcs,
	qse_ipad4_t*       ipad
);

int qse_wcstoipad4 (
	const qse_wchar_t* wcs,
	qse_ipad4_t*       ipad
);

int qse_ipad4towcs (
	const qse_ipad4_t* ipad,
	qse_wchar_t*       wcs,
	qse_size_t         len
);

#ifdef __cplusplus
}
#endif

#endif
