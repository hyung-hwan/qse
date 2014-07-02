/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_URI_H_
#define _QSE_CMN_URI_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_muri_t qse_muri_t;
typedef struct qse_wuri_t qse_wuri_t;

struct qse_muri_t
{
	qse_mxstr_t scheme;
	struct
	{
		qse_mxstr_t user;
		qse_mxstr_t pass;
	} auth;
	qse_mxstr_t host;
	qse_mxstr_t port;
	qse_mxstr_t path;
	qse_mxstr_t query;
	qse_mxstr_t frag;
};

struct qse_wuri_t
{
	qse_wxstr_t scheme;
	struct
	{
		qse_wxstr_t user;
		qse_wxstr_t pass;
	} auth;
	qse_wxstr_t host;
	qse_wxstr_t port;
	qse_wxstr_t path;
	qse_wxstr_t query;
	qse_wxstr_t frag;
};

enum qse_mbstouri_flag_t
{
	QSE_MBSTOURI_NOAUTH  = (1 << 0),
	QSE_MBSTOURI_NOQUERY = (1 << 1),
	QSE_MBSTOURI_NOFRAG  = (1 << 2)
};

enum qse_wcstouri_flag_t
{
	QSE_WCSTOURI_NOAUTH  = (1 << 0),
	QSE_WCSTOURI_NOQUERY = (1 << 1),
	QSE_WCSTOURI_NOFRAG  = (1 << 2)
};

#if defined(QSE_CHAR_IS_MCHAR)
#	define QSE_STRTOURI_NOAUTH  QSE_MBSTOURI_NOAUTH
#	define QSE_STRTOURI_NOQUERY QSE_MBSTOURI_NOQUERY
#	define QSE_STRTOURI_NOFRAG  QSE_MBSTOURI_NOFRAG
	typedef qse_muri_t qse_uri_t;
#else
#	define QSE_STRTOURI_NOAUTH  QSE_WCSTOURI_NOAUTH
#	define QSE_STRTOURI_NOQUERY QSE_WCSTOURI_NOQUERY
#	define QSE_STRTOURI_NOFRAG  QSE_WCSTOURI_NOFRAG
	typedef qse_wuri_t qse_uri_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT int qse_mbstouri (
	const qse_mchar_t* str,
	qse_muri_t*        uri,
	int                flags
);

QSE_EXPORT int qse_wcstouri (
	const qse_wchar_t* str,
	qse_wuri_t*        uri,
	int                flags
);

#if defined(QSE_CHAR_IS_MCHAR)
	#define qse_strtouri(str,uri,flags) qse_mbstouri(str,uri,flags)
#else
	#define qse_strtouri(str,uri,flags) qse_wcstouri(str,uri,flags)
#endif

#ifdef __cplusplus
}
#endif

#endif

