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

#ifndef _QSE_NET_HTRE_H_
#define _QSE_NET_HTRE_H_

#include <qse/net/http.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/str.h>

/* header and contents of request/response */
typedef struct qse_htre_t qse_htre_t;
struct qse_htre_t 
{
	qse_mmgr_t* mmgr;

	/* version */
	qse_http_version_t version;

	int qmethod_or_sstatus; 
	qse_htob_t qpath_or_smesg;
	qse_htob_t qparamstr;

	/* special attributes derived from the header */
	struct
	{
		int chunked;		
		int content_length;
		int connection_close;
		int expect_continue;
	} attr;

	/* header table */
	qse_htb_t hdrtab;
	
	/* content octets */
	qse_htob_t content;

	/* if set, the rest of the contents are discarded */
	int discard;
};

#define qse_htre_getversion(re) (&((re)->version))
#define qse_htre_getmajorversion(re) ((re)->version.major)
#define qse_htre_getminorversion(re) ((re)->version.minor)
#define qse_htre_setversion(re,v) QSE_BLOCK((re)->version = *(v);)

#define qse_htre_getqmethod(re) ((re)->qmethod_or_sstatus)
#define qse_htre_setqmethod(re,v) QSE_BLOCK((re)->qmethod_or_sstatus=(v);)

#define qse_htre_getsstatus(re) ((re)->qmethod_or_sstatus)
#define qse_htre_setsstatus(re,v) QSE_BLOCK((re)->qmethod_or_sstatus=(v);)

#define qse_htre_getqpath(re) (&(re)->qpath_or_smesg)
#define qse_htre_getqpathptr(re) QSE_MBS_PTR(&(re)->qpath_or_smesg)
#define qse_htre_getqpathlen(re) QSE_MBS_LEN(&(re)->qpath_or_smesg)

#define qse_htre_getqparamstr(re) (&(re)->qparamstr)
#define qse_htre_getqparamstrptr(re) QSE_MBS_PTR(&(re)->qparamstr)
#define qse_htre_getqparamstrlen(re) QSE_MBS_LEN(&(re)->qparamstr)

#define qse_htre_getsmessage(re) (&(re)->qpath_or_smesg)
#define qse_htre_getsmessageptr(re) QSE_MBS_PTR(&(re)->qpath_or_smesg)
#define qse_htre_getsmessagelen(re) QSE_MBS_LEN(&(re)->qpath_or_smesg)

#define qse_htre_setqpath(re,v) qse_htre_setbuf((re),qse_htre_getqpath(re),(v))
#define qse_htre_setsmessage(re,v) qse_htre_setbuf((re),qse_htre_getsmessage(re),(v))

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

int qse_htre_setbuf (
	qse_htre_t* re,
	qse_htob_t* buf,
	const qse_mcstr_t* str
);

void qse_htre_getbuf (
	qse_htre_t* re,
	const qse_htob_t* buf,
	qse_mcstr_t* str
);
	
int qse_htre_setqparamstr (
	qse_htre_t* re,
	const qse_mcstr_t* str
);

#ifdef __cplusplus
}
#endif

#endif
