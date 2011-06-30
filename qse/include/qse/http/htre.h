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

#ifndef _QSE_HTTP_HTRE_H_
#define _QSE_HTTP_HTRE_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/str.h>

/*typedef qse_byte_t qse_htoc_t;*/
typedef qse_mchar_t qse_htoc_t;

/* octet buffer */
typedef qse_mbs_t qse_htob_t;

/* octet string */
typedef qse_mxstr_t qse_htos_t;

typedef struct qse_htvr_t qse_htvr_t;
struct qse_htvr_t
{
	short major;
	short minor;
};

/* header and contents of request/response */
typedef struct qse_htre_t qse_htre_t;
struct qse_htre_t 
{
	qse_mmgr_t* mmgr;

	/* version */
	qse_htvr_t version;

	union
	{
		struct
		{
			enum
			{
				QSE_HTTP_REQ_GET,
				QSE_HTTP_REQ_HEAD,
				QSE_HTTP_REQ_POST,
				QSE_HTTP_REQ_PUT,
				QSE_HTTP_REQ_DELETE,
				QSE_HTTP_REQ_TRACE,
				QSE_HTTP_REQ_OPTIONS,
				QSE_HTTP_REQ_CONNECT
			} method;
	
			qse_htos_t path;
			/* qse_htos_t args; */
		} quest;

		struct
		{
			int code;
			qse_htos_t message;
		} sponse;
	} re;

	/* special attributes derived from the header */
	struct
	{
		int chunked;		
		int content_length;
		int connection_close;
		qse_htos_t content_type;
		qse_htos_t host;
		int expect_continue;
	} attr;

	/* header table */
	qse_htb_t hdrtab;
	
	/* content octets */
	qse_htob_t content;

	/* if set, the rest of the contents are discarded */
	int discard;
};

#define qse_htre_getversion(re) &((re)->version)
#define qse_htre_setversion(re,v) QSE_BLOCK((re)->version = *(v);)
#define qse_htre_setdiscard(re,v) QSE_BLOCK((re)->discard = (v);)

#ifdef __cplusplus
extern "C" {
#endif

qse_htre_t* qse_htre_init (
	qse_htre_t* re,
	qse_mmgr_t* mmgr
);

void qse_htre_fini (
	qse_htre_t* re
);

void qse_htre_clear (
	qse_htre_t* re
);

#ifdef __cplusplus
}
#endif

#endif
