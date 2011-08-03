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
    License along with QSE. If not, see <htrd://www.gnu.org/licenses/>.
 */

#include <qse/net/htre.h>
#include "../cmn/mem.h"

qse_htre_t* qse_htre_init (qse_htre_t* re, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (re, 0, QSE_SIZEOF(*re));
	re->mmgr = mmgr;	

	if (qse_htb_init (&re->hdrtab, mmgr, 60, 70, 1, 1) == QSE_NULL)
	{
		return QSE_NULL;
	}

	qse_mbs_init (&re->content, mmgr, 0);
	qse_mbs_init (&re->qpath_or_smesg, mmgr, 0);
	qse_mbs_init (&re->qparam, mmgr, 0);

	return re;
}

void qse_htre_fini (qse_htre_t* re)
{
	qse_mbs_fini (&re->qparam);
	qse_mbs_fini (&re->qpath_or_smesg);
	qse_mbs_fini (&re->content);
	qse_htb_fini (&re->hdrtab);
}

void qse_htre_clear (qse_htre_t* re)
{
	QSE_MEMSET (&re->version, 0, QSE_SIZEOF(re->version));
	QSE_MEMSET (&re->attr, 0, QSE_SIZEOF(re->attr));

	qse_htb_clear (&re->hdrtab);

	qse_mbs_clear (&re->content);
	qse_mbs_clear (&re->qpath_or_smesg);
	qse_mbs_clear (&re->qparam);

	re->discard = 0;
}

int qse_htre_setstrfromcstr (
	qse_htre_t* re, qse_mbs_t* str, const qse_mcstr_t* cstr)
{
	return (qse_mbs_ncpy (str, cstr->ptr, cstr->len) == (qse_size_t)-1)? -1: 0;
}

int qse_htre_setstrfromxstr (
	qse_htre_t* re, qse_mbs_t* str, const qse_mxstr_t* xstr)
{
	return (qse_mbs_ncpy (str, xstr->ptr, xstr->len) == (qse_size_t)-1)? -1: 0;
}

const qse_mchar_t* qse_htre_getheaderval (
	qse_htre_t* re, const qse_mchar_t* name)
{
	qse_htb_pair_t* pair;
	pair = qse_htb_search (&re->hdrtab, name, qse_mbslen(name));
	if (pair == QSE_NULL) return QSE_NULL;
	return QSE_HTB_VPTR(pair);
}

struct header_walker_ctx_t
{
	qse_htre_t* re;
	qse_htre_header_walker_t walker;
	void* ctx;
	int ret;
};

static qse_htb_walk_t walk_headers (qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
{
	struct header_walker_ctx_t* hwctx = (struct header_walker_ctx_t*)ctx;
	if (hwctx->walker (hwctx->re, QSE_HTB_KPTR(pair), QSE_HTB_VPTR(pair), hwctx->ctx) <= -1) 
	{
		hwctx->ret = -1;
		return QSE_HTB_WALK_STOP;
	}
     return QSE_HTB_WALK_FORWARD;
}

int qse_htre_walkheaders (
	qse_htre_t* re, qse_htre_header_walker_t walker, void* ctx)
{
	struct header_walker_ctx_t hwctx;
	hwctx.re = re;
	hwctx.walker = walker;
	hwctx.ctx = ctx;
	hwctx.ret = 0;
	qse_htb_walk (&re->hdrtab, walk_headers, &hwctx);
	return hwctx.ret;
}
	
