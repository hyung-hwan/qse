/*
 * $Id: http.h 223 2008-06-26 06:44:41Z baconevi $
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

#ifndef _QSE_NET_HTTP_H_
#define _QSE_NET_HTTP_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/str.h>
#include <qse/cmn/htb.h>

/*typedef qse_byte_t qse_htoc_t;*/
typedef qse_mchar_t qse_htoc_t;

/* octet buffer */
typedef qse_mbs_t qse_htob_t;

/* octet string */
typedef qse_mxstr_t qse_htos_t;

typedef struct qse_http_version_t qse_http_version_t;
struct qse_http_version_t
{
	short major;
	short minor;
};

enum qse_http_method_t
{
	QSE_HTTP_GET,
	QSE_HTTP_HEAD,
	QSE_HTTP_POST,
	QSE_HTTP_PUT,
	QSE_HTTP_DELETE,
	QSE_HTTP_TRACE,
	QSE_HTTP_OPTIONS,
	QSE_HTTP_CONNECT
};

typedef enum qse_http_method_t qse_http_method_t;

typedef int (*qse_scanhttpparamstr_callback_t) (
	void* ctx,
	const qse_mcstr_t* key,
	const qse_mcstr_t* val
);

#ifdef __cplusplus
extern "C" {
#endif

const qse_htoc_t* qse_gethttpmethodname (
	qse_http_method_t type
);

int qse_gethttpmethodtype (
	const qse_htoc_t* name, 
	qse_http_method_t* method
);

int qse_gethttpmethodtypefromstr (
	const qse_mcstr_t* name,
	qse_http_method_t* type
);

int qse_scanhttpparamstr (
	const qse_htoc_t*               paramstr,
	qse_scanhttpparamstr_callback_t callback,
	void*                           ctx
);

#ifdef __cplusplus
}
#endif


#endif
