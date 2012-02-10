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

enum qse_htre_state_t
{
	QSE_HTRE_DISCARDED = (1 << 0), /** content has been discarded */
	QSE_HTRE_COMPLETED = (1 << 1)  /** complete content has been seen */
};

typedef int (*qse_htre_concb_t) (
	qse_htre_t*        re,
	const qse_mchar_t* ptr,
	qse_size_t         len,
	void*              ctx
);

struct qse_htre_t 
{
	qse_mmgr_t* mmgr;

	/* version */
	qse_http_version_t version;

	int qmethod_or_sstatus; 
	qse_mbs_t qpath_or_smesg;
	qse_mbs_t qparam;

	/* special attributes derived from the header */
	struct
	{
		int chunked;		
		int content_length_set;
		qse_size_t content_length;
		int keepalive;
		const qse_mchar_t* expect;
	} attr;

	/* header table */
	qse_htb_t hdrtab;
	
	/* content octets */
	qse_mbs_t content;

	/* content callback */
	qse_htre_concb_t concb;
	void* concb_ctx;

	/* ORed of qse_htre_state_t */
	int state;
};

#define qse_htre_getversion(re) (&((re)->version))
#define qse_htre_getmajorversion(re) ((re)->version.major)
#define qse_htre_getminorversion(re) ((re)->version.minor)
#define qse_htre_setversion(re,v) QSE_BLOCK((re)->version = *(v);)

#define qse_htre_getqmethod(re) ((re)->qmethod_or_sstatus)
#define qse_htre_setqmethod(re,v) QSE_BLOCK((re)->qmethod_or_sstatus=(v);)

#define qse_htre_getsstatus(re) ((re)->qmethod_or_sstatus)
#define qse_htre_setsstatus(re,v) QSE_BLOCK((re)->qmethod_or_sstatus=(v);)

#define qse_htre_getqpath(re)     (&(re)->qpath_or_smesg)
#define qse_htre_getqpathxstr(re) QSE_MBS_XSTR(&(re)->qpath_or_smesg)
#define qse_htre_getqpathcstr(re) QSE_MBS_CSTR(&(re)->qpath_or_smesg)
#define qse_htre_getqpathptr(re)  QSE_MBS_PTR(&(re)->qpath_or_smesg)
#define qse_htre_getqpathlen(re)  QSE_MBS_LEN(&(re)->qpath_or_smesg)

#define qse_htre_getqparam(re)     (&(re)->qparam)
#define qse_htre_getqparamxstr(re) QSE_MBS_XSTR(&(re)->qparam)
#define qse_htre_getqparamcstr(re) QSE_MBS_CSTR(&(re)->qparam)
#define qse_htre_getqparamptr(re)  QSE_MBS_PTR(&(re)->qparam)
#define qse_htre_getqparamlen(re)  QSE_MBS_LEN(&(re)->qparam)

#define qse_htre_getsmessage(re)     (&(re)->qpath_or_smesg)
#define qse_htre_getsmessagexstr(re) QSE_MBS_XSTR(&(re)->qpath_or_smesg)
#define qse_htre_getsmessagecstr(re) QSE_MBS_CSTR(&(re)->qpath_or_smesg)
#define qse_htre_getsmessageptr(re)  QSE_MBS_PTR(&(re)->qpath_or_smesg)
#define qse_htre_getsmessagelen(re)  QSE_MBS_LEN(&(re)->qpath_or_smesg)

#define qse_htre_getcontent(re)     (&(re)->content)
#define qse_htre_getcontentxstr(re) QSE_MBS_XSTR(&(re)->content)
#define qse_htre_getcontentcstr(re) QSE_MBS_CSTR(&(re)->content)
#define qse_htre_getcontentptr(re)  QSE_MBS_PTR(&(re)->content)
#define qse_htre_getcontentlen(re)  QSE_MBS_LEN(&(re)->content)

#define qse_htre_setqpathfromcstr(re,v) \
	qse_htre_setstrfromcstr((re),qse_htre_getqpath(re),(v))
#define qse_htre_setqpathfromxstr(re,v) \
	qse_htre_setstrfromxstr((re),qse_htre_getqpath(re),(v))

#define qse_htre_setqparamfromcstr(re,v) \
	qse_htre_setstrfromcstr((re),qse_htre_getqparam(re),(v))
#define qse_htre_setqparamfromxstr(re,v) \
	qse_htre_setstrfromxstr((re),qse_htre_getqparam(re),(v))

#define qse_htre_setsmessagefromcstr(re,v) \
	qse_htre_setstrfromcstr((re),qse_htre_getsmessage(re),(v))
#define qse_htre_setsmessagefromxstr(re,v) \
	qse_htre_setstrfromxstr((re),qse_htre_getsmessage(re),(v))

typedef int (*qse_htre_header_walker_t) (
	qse_htre_t*        re,
	const qse_mchar_t* key,
	const qse_mchar_t* val,
	void*              ctx
);

#ifdef __cplusplus
extern "C" {
#endif

int qse_htre_init (
	qse_htre_t* re,
	qse_mmgr_t* mmgr
);

void qse_htre_fini (
	qse_htre_t* re
);

void qse_htre_clear (
	qse_htre_t* re
);

int qse_htre_setstrfromcstr (
	qse_htre_t*        re,
	qse_mbs_t*         str,
	const qse_mcstr_t* cstr
);

int qse_htre_setstrfromxstr (
	qse_htre_t*        re,
	qse_mbs_t*         str,
	const qse_mxstr_t* xstr
);

const qse_mchar_t* qse_htre_getheaderval (
	const qse_htre_t*  re, 
	const qse_mchar_t* key
);

int qse_htre_walkheaders (
	qse_htre_t*              re,
	qse_htre_header_walker_t walker,
	void*                    ctx
);

/**
 * The qse_htre_addcontent() function adds a content semgnet pointed to by
 * @a ptr of @a len bytes to the content buffer. If @a re is already completed
 * or discarded, this function returns 0 without adding the segment to the 
 * content buffer. 
 * @return 1 on success, -1 on failure, 0 if adding is skipped.
 */
int qse_htre_addcontent (
	qse_htre_t*        re,
	const qse_mchar_t* ptr,
	qse_size_t         len
);

void qse_htre_completecontent (
	qse_htre_t*      re
);

void qse_htre_discardcontent (
	qse_htre_t*      re
);

void qse_htre_unsetconcb (
	qse_htre_t*      re
);

void qse_htre_setconcb (
	qse_htre_t*      re,
	qse_htre_concb_t concb, 
	void*            ctx
);

const qse_mchar_t* qse_htre_getqmethodname (
	const qse_htre_t* re
);

#ifdef __cplusplus
}
#endif

#endif
