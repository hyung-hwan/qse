/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "awk-prv.h"
#include <qse/cmn/mbwc.h>
#include <qse/hash.h>

#ifdef DEBUG_VAL
#include <qse/cmn/sio.h>
#endif

#define CHUNKSIZE QSE_AWK_VAL_CHUNK_SIZE

static qse_awk_val_nil_t awk_nil = { QSE_AWK_VAL_NIL, 0, 1, 0, 0 };
static qse_awk_val_str_t awk_zls = { QSE_AWK_VAL_STR, 0, 1, 0, 0, { QSE_T(""), 0 } };
static qse_awk_val_mbs_t awk_zlm = { QSE_AWK_VAL_MBS, 0, 1, 0, 0, { QSE_MT(""), 0 } };

qse_awk_val_t* qse_awk_val_nil = (qse_awk_val_t*)&awk_nil;
qse_awk_val_t* qse_awk_val_zls = (qse_awk_val_t*)&awk_zls; 
qse_awk_val_t* qse_awk_val_zlm = (qse_awk_val_t*)&awk_zlm;

qse_awk_val_t* qse_get_awk_nil_val (void)
{
	return (qse_awk_val_t*)&awk_nil;
}

int qse_awk_rtx_isnilval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	return val == (qse_awk_val_t*)&awk_nil || (QSE_AWK_VTR_IS_POINTER(val) && val->v_type == QSE_AWK_VAL_NIL);
}

qse_awk_val_t* qse_awk_rtx_makenilval (qse_awk_rtx_t* rtx)
{
	return (qse_awk_val_t*)&awk_nil;
}

qse_awk_val_t* qse_awk_rtx_makeintval (qse_awk_rtx_t* rtx, qse_awk_int_t v)
{
	qse_awk_val_int_t* val;

	if (QSE_AWK_IN_QUICKINT_RANGE(v)) return QSE_AWK_QUICKINT_TO_VTR(v);

	if (!rtx->vmgr.ifree)
	{
		qse_awk_val_ichunk_t* c;
		/*qse_awk_val_int_t* x;*/
		qse_size_t i;

		/* use qse_awk_val_ichunk structure to avoid
		 * any alignment issues on platforms requiring
		 * aligned memory access - using the code commented out
		 * will cause a fault on such a platform */

		/* c = qse_awk_rtx_allocmem(rtx, QSE_SIZEOF(qse_awk_val_chunk_t) + QSE_SIZEOF(qse_awk_val_int_t)*CHUNKSIZE); */
		c = qse_awk_rtx_allocmem(rtx, QSE_SIZEOF(qse_awk_val_ichunk_t));
		if (!c) return QSE_NULL;
		
		c->next = rtx->vmgr.ichunk;
		/*run->vmgr.ichunk = c;*/
		rtx->vmgr.ichunk = (qse_awk_val_chunk_t*)c;

		/*x = (qse_awk_val_int_t*)(c + 1);
		for (i = 0; i < CHUNKSIZE-1; i++) 
			x[i].nde = (qse_awk_nde_int_t*)&x[i+1];
		x[i].nde = QSE_NULL;

		run->vmgr.ifree = x;
		*/

		for (i = 0; i < CHUNKSIZE-1; i++)
			c->slot[i].nde = (qse_awk_nde_int_t*)&c->slot[i+1];
		c->slot[i].nde = QSE_NULL;

		rtx->vmgr.ifree = &c->slot[0];
	}

	val = rtx->vmgr.ifree;
	rtx->vmgr.ifree = (qse_awk_val_int_t*)val->nde;

	val->v_type = QSE_AWK_VAL_INT;
	val->ref = 0;
	val->stat = 0;
	val->nstr = 0;
	val->fcb = 0;
	val->i_val = v;
	val->nde = QSE_NULL;

#ifdef DEBUG_VAL
	qse_errputstrf (QSE_T("makeintval => %jd [%p]\n"), (qse_intmax_t)v, val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makefltval (qse_awk_rtx_t* rtx, qse_awk_flt_t v)
{
	qse_awk_val_flt_t* val;

	if (rtx->vmgr.rfree == QSE_NULL)
	{
		qse_awk_val_rchunk_t* c;
		/*qse_awk_val_flt_t* x;*/
		qse_size_t i;

		/* c = qse_awk_rtx_allocmem (rtx, QSE_SIZEOF(qse_awk_val_chunk_t) + QSE_SIZEOF(qse_awk_val_flt_t) * CHUNKSIZE); */
		c = qse_awk_rtx_allocmem(rtx, QSE_SIZEOF(qse_awk_val_rchunk_t));
		if (!c) return QSE_NULL;

		c->next = rtx->vmgr.rchunk;
		/*run->vmgr.rchunk = c;*/
		rtx->vmgr.rchunk = (qse_awk_val_chunk_t*)c;

		/*
		x = (qse_awk_val_flt_t*)(c + 1);
		for (i = 0; i < CHUNKSIZE-1; i++) 
			x[i].nde = (qse_awk_nde_flt_t*)&x[i+1];
		x[i].nde = QSE_NULL;

		run->vmgr.rfree = x;
		*/

		for (i = 0; i < CHUNKSIZE-1; i++)
			c->slot[i].nde = (qse_awk_nde_flt_t*)&c->slot[i+1];
		c->slot[i].nde = QSE_NULL;

		rtx->vmgr.rfree = &c->slot[0];
	}

	val = rtx->vmgr.rfree;
	rtx->vmgr.rfree = (qse_awk_val_flt_t*)val->nde;

	val->v_type = QSE_AWK_VAL_FLT;
	val->ref = 0;
	val->stat = 0;
	val->nstr = 0;
	val->fcb = 0;
	val->val = v;
	val->nde = QSE_NULL;

#ifdef DEBUG_VAL
	qse_errputstrf (QSE_T("makefltval => %Lf [%p]\n"), (double)v, val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithcstr (qse_awk_rtx_t* rtx, const qse_cstr_t* str)
{
	qse_awk_val_str_t* val = QSE_NULL;
	qse_size_t rlen = str->len;
#ifdef ENABLE_FEATURE_SCACHE
	qse_size_t i;
#endif

	if (rlen <= 0) return qse_awk_val_zls;

#ifdef ENABLE_FEATURE_SCACHE
	i = rlen / FEATURE_SCACHE_BLOCK_UNIT;
	if (i < QSE_COUNTOF(rtx->scache_count))
	{
		rlen = (i + 1) * FEATURE_SCACHE_BLOCK_UNIT - 1;
		if (rtx->scache_count[i] > 0)
		{
			val = rtx->scache[i][--rtx->scache_count[i]];
			goto init;
		}
	}
#endif

	val = (qse_awk_val_str_t*)qse_awk_rtx_callocmem(rtx, QSE_SIZEOF(qse_awk_val_str_t) + (rlen + 1) * QSE_SIZEOF(qse_char_t));
	if (!val) return QSE_NULL;

#ifdef ENABLE_FEATURE_SCACHE
init:
#endif
	val->v_type = QSE_AWK_VAL_STR;
	val->ref = 0;
	val->stat = 0;
	val->nstr = 0;
	val->fcb = 0;
	val->val.len = str->len;
	val->val.ptr = (qse_char_t*)(val + 1);
	if (str->ptr) qse_strncpy (val->val.ptr, str->ptr, str->len);

#ifdef DEBUG_VAL
	qse_errputstrf (QSE_T("makestrval => %p\n"), val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithmbs (qse_awk_rtx_t* rtx, const qse_mchar_t* mbs)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return qse_awk_rtx_makestrval(rtx, mbs, qse_mbslen(mbs));
#else
	qse_awk_val_t* v;
	qse_wcstr_t tmp;

	tmp.ptr = qse_mbstowcsalldupwithcmgr(mbs, &tmp.len, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
	if (tmp.ptr == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	v = qse_awk_rtx_makestrvalwithcstr (rtx, &tmp);
	qse_awk_rtx_freemem (rtx, tmp.ptr);
	return v;
#endif
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithwcs (qse_awk_rtx_t* rtx, const qse_wchar_t* wcs)
{
#if defined(QSE_CHAR_IS_MCHAR)
	qse_awk_val_t* v;
	qse_mcstr_t tmp;

	tmp.ptr = qse_wcstombsdupwithcmgr(wcs, &tmp.len, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
	if (tmp.ptr == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	v = qse_awk_rtx_makestrvalwithcstr (rtx, &tmp);
	qse_awk_rtx_freemem (rtx, tmp.ptr);
	return v;
#else
	return qse_awk_rtx_makestrval (rtx, wcs, qse_wcslen(wcs));
#endif
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithstr (qse_awk_rtx_t* rtx, const qse_char_t* str)
{
	return qse_awk_rtx_makestrval(rtx, str, qse_strlen(str));
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithmcstr (qse_awk_rtx_t* rtx, const qse_mcstr_t* mxstr)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return qse_awk_rtx_makestrvalwithcstr(rtx, mxstr);
#else
	qse_awk_val_t* v;
	qse_wcstr_t tmp;
	qse_size_t mbslen;

	mbslen = mxstr->len;
	tmp.ptr = qse_mbsntowcsalldupwithcmgr(mxstr->ptr, &mbslen, &tmp.len, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
	if (tmp.ptr == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	v = qse_awk_rtx_makestrvalwithcstr(rtx, &tmp);
	qse_awk_rtx_freemem (rtx, tmp.ptr);
	return v;
#endif
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithwcstr (qse_awk_rtx_t* rtx, const qse_wcstr_t* wxstr)
{
#if defined(QSE_CHAR_IS_MCHAR)
	qse_awk_val_t* v;
	qse_mcstr_t tmp;
	qse_size_t wcslen;

	wcslen = wxstr->len;
	tmp.ptr = qse_wcsntombsdupwithcmgr(wxstr->ptr, wcslen, &tmp.len, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
	if (tmp.ptr == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	v = qse_awk_rtx_makestrvalwithcstr(rtx, &tmp);
	qse_awk_rtx_freemem (rtx, tmp.ptr);
	return v;
#else
	return qse_awk_rtx_makestrvalwithcstr(rtx, wxstr);
#endif
}

qse_awk_val_t* qse_awk_rtx_makestrval (qse_awk_rtx_t* rtx, const qse_char_t* str, qse_size_t len)
{
	qse_cstr_t xstr;
	xstr.ptr = (qse_char_t*)str;
	xstr.len = len;
	return qse_awk_rtx_makestrvalwithcstr(rtx, &xstr);
}

qse_awk_val_t* qse_awk_rtx_makestrval2 (qse_awk_rtx_t* rtx, const qse_char_t* str1, qse_size_t len1, const qse_char_t* str2, qse_size_t len2)
{
	qse_awk_val_str_t* val;
	qse_size_t rlen = len1 + len2;

#ifdef ENABLE_FEATURE_SCACHE
	int i;

	i = rlen / FEATURE_SCACHE_BLOCK_UNIT;
	if (i < QSE_COUNTOF(rtx->scache_count))
	{
		rlen = (i + 1) * FEATURE_SCACHE_BLOCK_UNIT - 1;
		if (rtx->scache_count[i] > 0)
		{
			val = rtx->scache[i][--rtx->scache_count[i]];
			goto init;
		}
	}
#endif

	val = (qse_awk_val_str_t*)qse_awk_rtx_callocmem(rtx, QSE_SIZEOF(qse_awk_val_str_t) + ((rlen + 1) * QSE_SIZEOF(qse_char_t)));
	if (!val) return QSE_NULL;

#ifdef ENABLE_FEATURE_SCACHE
init:
#endif
	val->v_type = QSE_AWK_VAL_STR;
	val->ref = 0;
	val->stat = 0;
	val->nstr = 0;
	val->fcb = 0;
	val->val.len = len1 + len2;
	val->val.ptr = (qse_char_t*)(val + 1);
	qse_strncpy (val->val.ptr, str1, len1);
	qse_strncpy (&val->val.ptr[len1], str2, len2);

#ifdef DEBUG_VAL
	qse_errputstrf (QSE_T("makestrval2 => %p\n"), val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makenstrvalwithstr (qse_awk_rtx_t* rtx, const qse_char_t* str)
{
	qse_cstr_t xstr;
	xstr.ptr = (qse_char_t*)str;
	xstr.len = qse_strlen(str);
	return qse_awk_rtx_makenstrvalwithcstr (rtx, &xstr);
}

qse_awk_val_t* qse_awk_rtx_makenstrvalwithcstr (qse_awk_rtx_t* rtx, const qse_cstr_t* str)
{
	int x;
	qse_awk_val_t* v;
	qse_awk_int_t l;
	qse_awk_flt_t r;

	x = qse_awk_rtx_strtonum(rtx, QSE_AWK_RTX_STRTONUM_MAKE_OPTION(1, 0), str->ptr, str->len, &l, &r);
	v = qse_awk_rtx_makestrvalwithcstr(rtx, str);

	if (v == QSE_NULL) return QSE_NULL;

	if (x >= 0) 
	{
		/* set the numeric string flag if a string
		 * can be converted to a number */
		QSE_ASSERT (x == 0 || x == 1);
		v->nstr = x + 1; /* long -> 1, real -> 2 */
	}

	return v;
}

qse_awk_val_t* qse_awk_rtx_makembsval (qse_awk_rtx_t* rtx, const qse_mchar_t* ptr, qse_size_t len)
{
	qse_awk_val_mbs_t* val = QSE_NULL;
	qse_size_t xsz;

	if (len <= 0) return qse_awk_val_zlm;

	xsz = len * QSE_SIZEOF(*ptr);

	val = (qse_awk_val_mbs_t*)qse_awk_rtx_callocmem(rtx, QSE_SIZEOF(qse_awk_val_mbs_t) + xsz + QSE_SIZEOF(*ptr));
	if (!val) return QSE_NULL;

	val->v_type = QSE_AWK_VAL_MBS;
	val->ref = 0;
	val->stat = 0;
	val->nstr = 0;
	val->fcb = 0;
	val->val.len = len;
	val->val.ptr = (qse_mchar_t*)(val + 1);
	if (ptr) QSE_MEMCPY (val->val.ptr, ptr, xsz);
	val->val.ptr[len] = QSE_MT('\0');

	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makembsvalwithmcstr (qse_awk_rtx_t* rtx, const qse_mcstr_t* mxstr)
{
	return qse_awk_rtx_makembsval(rtx, mxstr->ptr, mxstr->len);
}

qse_awk_val_t* qse_awk_rtx_makerexval (qse_awk_rtx_t* rtx, const qse_cstr_t* str, void* code[2])
{
	qse_awk_val_rex_t* val;
	qse_size_t totsz;

	/* the regular expression value holds:
	 * - header
	 * - a raw string plus with added a terminating '\0'
	 * the total size is just large enough for all these.
	 */
	totsz = QSE_SIZEOF(*val) + (QSE_SIZEOF(*str->ptr) * (str->len + 1));
	val = (qse_awk_val_rex_t*)qse_awk_rtx_callocmem(rtx, totsz);
	if (!val) return QSE_NULL;

	val->v_type = QSE_AWK_VAL_REX;
	val->ref = 0;
	val->stat = 0;
	val->nstr = 0;
	val->fcb = 0;
	val->str.len = str->len;

	val->str.ptr = (qse_char_t*)(val + 1);
	qse_strncpy (val->str.ptr, str->ptr, str->len);

	val->code[0] = code[0];
	val->code[1] = code[1];

	return (qse_awk_val_t*)val;
}

static void free_mapval (qse_htb_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_t* rtx = *(qse_awk_rtx_t**)QSE_XTN(map);

#ifdef DEBUG_VAL
	qse_errputstrf (QSE_T("refdown in map free..."));
	qse_awk_dprintval (rtx, dptr);
	qse_errputstrf (QSE_T("\n"));
#endif

	qse_awk_rtx_refdownval (rtx, dptr);
}

static void same_mapval (qse_htb_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_t* run = *(qse_awk_rtx_t**)QSE_XTN(map);
#ifdef DEBUG_VAL
	qse_errputstrf (QSE_T("refdown nofree in map free..."));
	qse_awk_dprintval (run, dptr);
	qse_errputstrf (QSE_T("\n"));
#endif
	qse_awk_rtx_refdownval_nofree (run, dptr);
}

qse_awk_val_t* qse_awk_rtx_makemapval (qse_awk_rtx_t* rtx)
{
	static qse_htb_style_t style =
	{
	/* the key is copied inline into a pair and is freed when the pair
	 * is destroyed. not setting copier for a value means that the pointer 
	 * to the data allocated somewhere else is remembered in a pair. but 
	 * freeing the actual value is handled by free_mapval and same_mapval */
		{
			QSE_HTB_COPIER_INLINE,
			QSE_HTB_COPIER_DEFAULT
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			free_mapval
		},
		QSE_HTB_COMPER_DEFAULT,
		same_mapval,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	};
	qse_awk_val_map_t* val;

	/* CHECK */
	/* 
	val = (qse_awk_val_map_t*)qse_awk_rtx_callocmem(rtx, QSE_SIZEOF(qse_awk_val_map_t));
	if (!val) return QSE_NULL;

	val->type = QSE_AWK_VAL_MAP;
	val->ref = 0;
	val->stat = 0;
	val->nstr = 0;
	val->fcb = 0;
	val->map = qse_htb_open(run, 256, 70, free_mapval, same_mapval, qse_awk_rtx_getmmgr(run));
	if (val->map == QSE_NULL)
	{
		qse_awk_rtx_freemem (rtx, val);
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}
	*/

	val = (qse_awk_val_map_t*)qse_awk_rtx_callocmem(rtx, QSE_SIZEOF(qse_awk_val_map_t) + QSE_SIZEOF(qse_htb_t) + QSE_SIZEOF(rtx));
	if (!val) return QSE_NULL;

	val->v_type = QSE_AWK_VAL_MAP;
	val->ref = 0;
	val->stat = 0;
	val->nstr = 0;
	val->fcb = 0;
	val->map = (qse_htb_t*)(val + 1);

	if (qse_htb_init(val->map, qse_awk_rtx_getmmgr(rtx), 256, 70, QSE_SIZEOF(qse_char_t), 1) <= -1)
	{
		qse_awk_rtx_freemem (rtx, val);
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}
	*(qse_awk_rtx_t**)QSE_XTN(val->map) = rtx;
	qse_htb_setstyle (val->map, &style);
	/* END CHECK */

	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makemapvalwithdata (qse_awk_rtx_t* rtx, qse_awk_val_map_data_t data[])
{
	qse_awk_val_t* map, * tmp;
	qse_awk_val_map_data_t* p;

	map = qse_awk_rtx_makemapval(rtx);
	if (!map) return QSE_NULL;

	for (p = data; p->key.ptr; p++)
	{
		switch (p->type)
		{
			case QSE_AWK_VAL_MAP_DATA_INT:
				tmp = qse_awk_rtx_makeintval(rtx, *(qse_awk_int_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_FLT:
				tmp = qse_awk_rtx_makefltval(rtx, *(qse_awk_flt_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_STR:
				tmp = qse_awk_rtx_makestrvalwithstr(rtx, (qse_char_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_MBS:
				tmp = qse_awk_rtx_makestrvalwithmbs(rtx, (qse_mchar_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_WCS:
				tmp = qse_awk_rtx_makestrvalwithwcs(rtx, (qse_wchar_t*)p->vptr);
				break;
					
			case QSE_AWK_VAL_MAP_DATA_XSTR:
			case QSE_AWK_VAL_MAP_DATA_CSTR:
				tmp = qse_awk_rtx_makestrvalwithcstr(rtx, (qse_cstr_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_MXSTR:
			case QSE_AWK_VAL_MAP_DATA_MCSTR:
				tmp = qse_awk_rtx_makestrvalwithmcstr(rtx, (qse_mcstr_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_WXSTR:
			case QSE_AWK_VAL_MAP_DATA_WCSTR:
				tmp = qse_awk_rtx_makestrvalwithwcstr(rtx, (qse_wcstr_t*)p->vptr);
				break;

			default:
				tmp = QSE_NULL;
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
				break;
		}

		if (tmp == QSE_NULL || qse_awk_rtx_setmapvalfld (rtx, map, p->key.ptr, p->key.len, tmp) == QSE_NULL)
		{
			if (tmp) qse_awk_rtx_freeval (rtx, tmp, 1);
			qse_awk_rtx_freeval (rtx, map, 1);
			return QSE_NULL;
		}
	}

	return map;
}

qse_awk_val_t* qse_awk_rtx_setmapvalfld (
	qse_awk_rtx_t* rtx, qse_awk_val_t* map, 
	const qse_char_t* kptr, qse_size_t klen, qse_awk_val_t* v)
{
	QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE (rtx, map) == QSE_AWK_VAL_MAP);

	if (qse_htb_upsert (
		((qse_awk_val_map_t*)map)->map,
		(qse_char_t*)kptr, klen, v, 0) == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	/* the value is passed in by an external party. we can't refup()
	 * and refdown() the value if htb_upsert() fails. that way, the value
	 * can be destroyed if it was passed with the reference count of 0.
	 * so we increment the reference count when htb_upsert() is complete */
	qse_awk_rtx_refupval (rtx, v);

	return v;
}

qse_awk_val_t* qse_awk_rtx_getmapvalfld (
	qse_awk_rtx_t* rtx, qse_awk_val_t* map, 
	const qse_char_t* kptr, qse_size_t klen)
{
	qse_htb_pair_t* pair;

	QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE (rtx, map) == QSE_AWK_VAL_MAP);

	pair = qse_htb_search(((qse_awk_val_map_t*)map)->map, kptr, klen);
	if (pair == QSE_NULL)
	{
		/* the given key is not found in the map.
		 * we return NULL here as this function is called by 
		 * a user unlike the awk statement accessing the map key.
		 * so you can easily determine if the key is found by
		 * checking the error number.
		 */
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	return QSE_HTB_VPTR(pair);
}

qse_awk_val_map_itr_t* qse_awk_rtx_getfirstmapvalitr (
	qse_awk_rtx_t* rtx, qse_awk_val_t* map, qse_awk_val_map_itr_t* itr)
{
	QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE (rtx, map) == QSE_AWK_VAL_MAP);
	itr->pair = qse_htb_getfirstpair (((qse_awk_val_map_t*)map)->map, &itr->buckno);
	return itr->pair? itr: QSE_NULL;
}

qse_awk_val_map_itr_t* qse_awk_rtx_getnextmapvalitr (
	qse_awk_rtx_t* rtx, qse_awk_val_t* map, qse_awk_val_map_itr_t* itr)
{
	QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE (rtx, map) == QSE_AWK_VAL_MAP);
	itr->pair = qse_htb_getnextpair (((qse_awk_val_map_t*)map)->map, itr->pair, &itr->buckno);
	return itr->pair? itr: QSE_NULL;
}

qse_awk_val_t* qse_awk_rtx_makerefval (qse_awk_rtx_t* rtx, int id, qse_awk_val_t** adr)
{
	qse_awk_val_ref_t* val;

	if (rtx->rcache_count > 0)
	{
		val = rtx->rcache[--rtx->rcache_count];
	}
	else
	{
		val = (qse_awk_val_ref_t*)qse_awk_rtx_callocmem(rtx, QSE_SIZEOF(*val));
		if (!val) return QSE_NULL;
	}

	QSE_AWK_RTX_INIT_REF_VAL (val, id, adr, 0);
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makefunval (qse_awk_rtx_t* rtx, const qse_awk_fun_t* fun)
{
	qse_awk_val_fun_t* val;

	val = (qse_awk_val_fun_t*)qse_awk_rtx_callocmem(rtx, QSE_SIZEOF(*val));
	if (!val) return QSE_NULL;

	val->v_type = QSE_AWK_VAL_FUN;
	val->ref = 0;
	val->stat = 0;
	val->nstr = 0;
	val->fcb = 0;
	val->fun = (qse_awk_fun_t*)fun;

	return (qse_awk_val_t*)val;
}

int QSE_INLINE qse_awk_rtx_isstaticval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	return QSE_AWK_VTR_IS_POINTER(val) && IS_STATICVAL(val);
}

int qse_awk_rtx_getvaltype (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	return QSE_AWK_RTX_GETVALTYPE(rtx, val);
}

const qse_char_t* qse_awk_rtx_getvaltypename(qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	static const qse_char_t* __val_type_name[] =
	{
		/* synchronize this table with enum qse_awk_val_type_t in awk.h */
		QSE_T("nil"),
		QSE_T("int"),
		QSE_T("flt"),
		QSE_T("str"),
		QSE_T("mbs"),
		QSE_T("fun"),
		QSE_T("map"),
		QSE_T("rex"),
		QSE_T("ref")
	};

	return __val_type_name[QSE_AWK_RTX_GETVALTYPE(rtx, val)];
}


int qse_awk_rtx_getintfromval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	return QSE_AWK_RTX_GETINTFROMVAL(rtx, val);
}

void qse_awk_rtx_freeval (qse_awk_rtx_t* rtx, qse_awk_val_t* val, int cache)
{
	qse_awk_val_type_t vtype;

	if (QSE_AWK_VTR_IS_POINTER(val))
	{
		if (IS_STATICVAL(val)) return;

	#ifdef DEBUG_VAL
		qse_errputstrf (QSE_T("freeing [cache=%d] ... "), cache);
		qse_awk_dprintval (rtx, val);
		qse_errputstrf (QSE_T("\n"));
	#endif

		vtype = QSE_AWK_RTX_GETVALTYPE (rtx, val);
		switch (vtype)
		{
			case QSE_AWK_VAL_NIL:
			{
				qse_awk_rtx_freemem (rtx, val);
				break;
			}
			
			case QSE_AWK_VAL_INT:
			{
				((qse_awk_val_int_t*)val)->nde = (qse_awk_nde_int_t*)rtx->vmgr.ifree;
				rtx->vmgr.ifree = (qse_awk_val_int_t*)val;
				break;
			}

			case QSE_AWK_VAL_FLT:
			{
				((qse_awk_val_flt_t*)val)->nde = (qse_awk_nde_flt_t*)rtx->vmgr.rfree;
				rtx->vmgr.rfree = (qse_awk_val_flt_t*)val;
				break;
			}

			case QSE_AWK_VAL_STR:
			{
			#ifdef ENABLE_FEATURE_SCACHE
				if (cache)
				{
					qse_awk_val_str_t* v = (qse_awk_val_str_t*)val;
					int i;
		
					i = v->val.len / FEATURE_SCACHE_BLOCK_UNIT;
					if (i < QSE_COUNTOF(rtx->scache_count) &&
					    rtx->scache_count[i] < QSE_COUNTOF(rtx->scache[i]))
					{
						rtx->scache[i][rtx->scache_count[i]++] = v;
						v->nstr = 0;
					}
					else qse_awk_rtx_freemem (rtx, val);
				}
				else 
			#endif
					qse_awk_rtx_freemem (rtx, val);

				break;
			}

			case QSE_AWK_VAL_MBS:
				qse_awk_rtx_freemem (rtx, val);
				break;

			case QSE_AWK_VAL_REX:
			{
				/* don't free ptr as it is inlined to val
				qse_awk_rtx_freemem (rtx, ((qse_awk_val_rex_t*)val)->ptr);
				 */
			
				/* code is just a pointer to a regular expression stored
				 * in parse tree nodes. so don't free it.
				qse_awk_freerex (rtx->awk, ((qse_awk_val_rex_t*)val)->code[0], ((qse_awk_val_rex_t*)val)->code[1]);
				 */

				qse_awk_rtx_freemem (rtx, val);
				break;
			}

			case QSE_AWK_VAL_FUN:
				qse_awk_rtx_freemem (rtx, val);
				break;

			case QSE_AWK_VAL_MAP:
			{
				qse_htb_fini (((qse_awk_val_map_t*)val)->map);
				qse_awk_rtx_freemem (rtx, val);
				break;
			}

			case QSE_AWK_VAL_REF:
			{
				if (cache && rtx->rcache_count < QSE_COUNTOF(rtx->rcache))
				{
					rtx->rcache[rtx->rcache_count++] = (qse_awk_val_ref_t*)val;	
				}
				else qse_awk_rtx_freemem (rtx, val);
				break;
			}

		}
	}
}

void qse_awk_rtx_refupval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	if (QSE_AWK_VTR_IS_POINTER(val))
	{
		if (IS_STATICVAL(val)) return;

	#ifdef DEBUG_VAL
		qse_errputstrf (QSE_T("ref up [ptr=%p] [count=%d] "), val, (int)val->ref);
		qse_awk_dprintval (rtx, val);
		qse_errputstrf (QSE_T("\n"));
	#endif
		val->ref++;
	}
}

void qse_awk_rtx_refdownval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	if (QSE_AWK_VTR_IS_POINTER(val))
	{
		if (IS_STATICVAL(val)) return;

	#ifdef DEBUG_VAL
		qse_errputstrf (QSE_T("ref down [ptr=%p] [count=%d]\n"), val, (int)val->ref);
		qse_awk_dprintval (rtx, val);
		qse_errputstrf (QSE_T("\n"));
	#endif

		QSE_ASSERTX (val->ref > 0, 
			"the reference count of a value should be greater than zero for it to be decremented. check the source code for any bugs");

		val->ref--;
		if (val->ref <= 0) 
		{
			qse_awk_rtx_freeval(rtx, val, 1);
		}
	}
}

void qse_awk_rtx_refdownval_nofree (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	if (QSE_AWK_VTR_IS_POINTER(val))
	{
		if (IS_STATICVAL(val)) return;
	
		QSE_ASSERTX (val->ref > 0,
			"the reference count of a value should be greater than zero for it to be decremented. check the source code for any bugs");

		val->ref--;
	}
}

void qse_awk_rtx_freevalchunk (qse_awk_rtx_t* rtx, qse_awk_val_chunk_t* chunk)
{
	while (chunk != QSE_NULL)
	{
		qse_awk_val_chunk_t* next = chunk->next;
		qse_awk_rtx_freemem (rtx, chunk);
		chunk = next;
	}
}

static int val_ref_to_bool (qse_awk_rtx_t* rtx, const qse_awk_val_ref_t* ref)
{
	switch (ref->id)
	{
		case QSE_AWK_VAL_REF_POS:
		{
			qse_size_t idx;

			idx = (qse_size_t)ref->adr;
			if (idx == 0)
			{
				return QSE_STR_LEN(&rtx->inrec.line) > 0;
			}
			else if (idx <= rtx->inrec.nflds)
			{
				return rtx->inrec.flds[idx-1].len > 0;
			}
			else
			{
				/* the index is greater than the number of records.
				 * it's an empty string. so false */
				return 0;
			}
		}
		case QSE_AWK_VAL_REF_GBL:
		{
			qse_size_t idx;
			idx = (qse_size_t)ref->adr;
			return qse_awk_rtx_valtobool(rtx, RTX_STACK_GBL (rtx, idx));
		}

		default:
		{
			qse_awk_val_t** xref = (qse_awk_val_t**)ref->adr;

			/* A reference value is not able to point to another 
			 * refernce value for the way values are represented
			 * in QSEAWK */
			QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE (rtx, *xref)!= QSE_AWK_VAL_REF); 

			/* make a recursive call back to the caller */
			return qse_awk_rtx_valtobool(rtx, *xref);
		}
	}
}

int qse_awk_rtx_valtobool (qse_awk_rtx_t* rtx, const qse_awk_val_t* val)
{
	qse_awk_val_type_t vtype;

	if (val == QSE_NULL) return 0;

	vtype = QSE_AWK_RTX_GETVALTYPE(rtx, val);
	switch (vtype)
	{
		case QSE_AWK_VAL_NIL:
			return 0;
		case QSE_AWK_VAL_INT:
			return QSE_AWK_RTX_GETINTFROMVAL(rtx, val) != 0;
		case QSE_AWK_VAL_FLT:
			return ((qse_awk_val_flt_t*)val)->val != 0.0;
		case QSE_AWK_VAL_STR:
			return ((qse_awk_val_str_t*)val)->val.len > 0;
		case QSE_AWK_VAL_MBS:
			return ((qse_awk_val_mbs_t*)val)->val.len > 0;
		case QSE_AWK_VAL_REX: /* TODO: is this correct? */
			return ((qse_awk_val_rex_t*)val)->str.len > 0;
		case QSE_AWK_VAL_FUN:
			/* return always true */
			return 1;
		case QSE_AWK_VAL_MAP:
			/* true if the map size is greater than 0. false if not */
			return QSE_HTB_SIZE(((qse_awk_val_map_t*)val)->map) > 0;
		case QSE_AWK_VAL_REF:
			return val_ref_to_bool(rtx, (qse_awk_val_ref_t*)val);
	}

	QSE_ASSERTX (
		!"should never happen - invalid value type",
		"the type of a value should be one of QSE_AWK_VAL_XXX's defined in awk-prv.h");
	return 0;
}

static int str_to_str (qse_awk_rtx_t* rtx, const qse_char_t* str, qse_size_t str_len, qse_awk_rtx_valtostr_out_t* out)
{
	int type = out->type & ~QSE_AWK_RTX_VALTOSTR_PRINT;

	switch (type)
	{
		case QSE_AWK_RTX_VALTOSTR_CPL:
		{
			out->u.cpl.len = str_len;
			out->u.cpl.ptr = (qse_char_t*)str;
			return 0;
		}

		case QSE_AWK_RTX_VALTOSTR_CPLCPY:
		{
			if (str_len >= out->u.cplcpy.len)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
				/*out->u.cplcpy.len = str_len + 1;*/ /* set the required length */
				return -1;
			}

			out->u.cplcpy.len = qse_strncpy(out->u.cplcpy.ptr, str, str_len);
			return 0;
		}

		case QSE_AWK_RTX_VALTOSTR_CPLDUP:
		{
			qse_char_t* tmp;

			tmp = QSE_AWK_STRXDUP(rtx->awk, str, str_len);
			if (!tmp) 
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}

			out->u.cpldup.ptr = tmp;
			out->u.cpldup.len = str_len;
			return 0;
		}

		case QSE_AWK_RTX_VALTOSTR_STRP:
		{
			qse_size_t n;

			qse_str_clear (out->u.strp);
			n = qse_str_ncat(out->u.strp, str, str_len);
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}
			return 0;
		}

		case QSE_AWK_RTX_VALTOSTR_STRPCAT:
		{
			qse_size_t n;

			n = qse_str_ncat(out->u.strpcat, str, str_len);
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}
			return 0;
		}
	}

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
	return -1;
}

#if defined(QSE_CHAR_IS_MCHAR)
#	define mbs_to_str(rtx,str,str_len,out) str_to_str(rtx,str,str_len,out)
#else
static int mbs_to_str (qse_awk_rtx_t* rtx, const qse_mchar_t* str, qse_size_t str_len, qse_awk_rtx_valtostr_out_t* out)
{
	int type = out->type & ~QSE_AWK_RTX_VALTOSTR_PRINT;

	switch (type)
	{
		case QSE_AWK_RTX_VALTOSTR_CPL:
			/* conversion is required. i can't simply return it. let CPL
			 * behave like CPLCPY. fall thru */
		case QSE_AWK_RTX_VALTOSTR_CPLCPY:
		{
			qse_size_t mbslen, wcslen;

			mbslen = str_len;
			wcslen = out->u.cplcpy.len;
			/*if (qse_mbsntowcsnallwithcmgr(str, &mbslen, out->u.cplcpy.ptr, &wcslen, qse_findcmgrbyid(QSE_CMGR_MB8)) <= -1 || wcslen >= out->u.cplcpy.len)*/
			if (qse_mbsntowcsnallwithcmgr(str, &mbslen, out->u.cplcpy.ptr, &wcslen, qse_awk_rtx_getcmgr(rtx)) <= -1 || wcslen >= out->u.cplcpy.len)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL); /* TODO: change error code */
				return -1;
			}

			out->u.cplcpy.ptr[wcslen] = QSE_T('\0');
			out->u.cplcpy.len = wcslen;

			return 0;
		}

		case QSE_AWK_RTX_VALTOSTR_CPLDUP:
		{
			qse_char_t* tmp;
			qse_size_t mbslen, wcslen;

			mbslen = str_len;
			/*tmp = qse_mbsntowcsalldupwithcmgr(str, &mbslen, &wcslen, qse_awk_rtx_getmmgr(rtx), qse_findcmgrbyid(QSE_CMGR_MB8));*/
			tmp = qse_mbsntowcsalldupwithcmgr(str, &mbslen, &wcslen, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
			if (!tmp) 
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}

			out->u.cpldup.ptr = tmp;
			out->u.cpldup.len = wcslen;
			return 0;
		}

		case QSE_AWK_RTX_VALTOSTR_STRP:
		{
			qse_size_t n;

			qse_str_clear (out->u.strp);
			/*n = qse_str_ncatmbs(out->u.strp, str, str_len, qse_findcmgrbyid(QSE_CMGR_MB8));*/
			n = qse_str_ncatmbs(out->u.strp, str, str_len, qse_awk_rtx_getcmgr(rtx));
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}
			return 0;
		}

		case QSE_AWK_RTX_VALTOSTR_STRPCAT:
		{
			qse_size_t n;

			/*n = qse_str_ncatmbs(out->u.strpcat, str, str_len, qse_findcmgrbyid(QSE_CMGR_MB8));*/
			n = qse_str_ncatmbs(out->u.strpcat, str, str_len, qse_awk_rtx_getcmgr(rtx));
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}
			return 0;
		}
	}

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
	return -1;
}
#endif


static int val_int_to_str (qse_awk_rtx_t* rtx, const qse_awk_val_int_t* v, qse_awk_rtx_valtostr_out_t* out)
{
	qse_char_t* tmp;
	qse_size_t rlen = 0;
	int type = out->type & ~QSE_AWK_RTX_VALTOSTR_PRINT;
	qse_awk_int_t orgval = QSE_AWK_RTX_GETINTFROMVAL (rtx, v);
	qse_awk_uint_t t;

	if (orgval == 0) rlen++;
	else
	{
		/* non-zero values */
		if (orgval < 0) 
		{
			t = orgval * -1; rlen++; 
		}
		else t = orgval;
		while (t > 0) { rlen++; t /= 10; }
	}

	switch (type)
	{
		case QSE_AWK_RTX_VALTOSTR_CPL:
			/* CPL and CPLCP behave the same for int_t.
			 * i just fall through assuming that cplcpy 
			 * and cpl are the same type. the following
			 * assertion at least ensure that they have
			 * the same size. */ 
			QSE_ASSERT (QSE_SIZEOF(out->u.cpl) == QSE_SIZEOF(out->u.cplcpy));

		case QSE_AWK_RTX_VALTOSTR_CPLCPY:
			if (rlen >= out->u.cplcpy.len)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
				/* store the buffer size needed */
				out->u.cplcpy.len = rlen + 1; 
				return -1;
			}

			tmp = out->u.cplcpy.ptr;
			tmp[rlen] = QSE_T('\0');
			out->u.cplcpy.len = rlen;
			break;

		case QSE_AWK_RTX_VALTOSTR_CPLDUP:
			tmp = qse_awk_rtx_allocmem(rtx, (rlen + 1) * QSE_SIZEOF(qse_char_t));
			if (!tmp) return -1;

			tmp[rlen] = QSE_T('\0');
			out->u.cpldup.ptr = tmp;
			out->u.cpldup.len = rlen;
			break;

		case QSE_AWK_RTX_VALTOSTR_STRP:
		{
			qse_size_t n;

			qse_str_clear (out->u.strp);
			QSE_ASSERT (QSE_STR_LEN(out->u.strp) == 0);

			/* point to the beginning of the buffer */
			tmp = QSE_STR_PTR(out->u.strp);

			/* extend the buffer */
			n = qse_str_nccat(out->u.strp, QSE_T(' '), rlen);
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}
			break;
		}

		case QSE_AWK_RTX_VALTOSTR_STRPCAT:
		{
			qse_size_t n;

			/* point to the insertion point */
			tmp = QSE_STR_PTR(out->u.strpcat) + QSE_STR_LEN(out->u.strpcat);

			/* extend the buffer */
			n = qse_str_nccat(out->u.strpcat, QSE_T(' '), rlen);
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}
			break;
		}

		default:
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
			return -1;
		}
	}

	if (orgval == 0) tmp[0] = QSE_T('0'); 
	else
	{
		t = (orgval < 0)? (orgval * -1): orgval;

		/* fill in the buffer with digits */
		while (t > 0) 
		{
			tmp[--rlen] = (qse_char_t)(t % 10) + QSE_T('0');
			t /= 10;
		}

		/* insert the negative sign if necessary */
		if (orgval < 0) tmp[--rlen] = QSE_T('-');
	}

	return 0;
}

static int val_flt_to_str (qse_awk_rtx_t* rtx, const qse_awk_val_flt_t* v, qse_awk_rtx_valtostr_out_t* out)
{
	qse_char_t* tmp;
	qse_size_t tmp_len;
	qse_str_t buf, fbu;
	int buf_inited = 0, fbu_inited = 0;
	int type = out->type & ~QSE_AWK_RTX_VALTOSTR_PRINT;

	if (out->type & QSE_AWK_RTX_VALTOSTR_PRINT)
	{
		tmp = rtx->gbl.ofmt.ptr;
		tmp_len = rtx->gbl.ofmt.len;
	}
	else
	{
		tmp = rtx->gbl.convfmt.ptr;
		tmp_len = rtx->gbl.convfmt.len;
	}

	if (qse_str_init(&buf, qse_awk_rtx_getmmgr(rtx), 256) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}
	buf_inited = 1;

	if (qse_str_init(&fbu, qse_awk_rtx_getmmgr(rtx), 256) <= -1)
	{
		qse_str_fini (&buf);
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}
	fbu_inited = 1;

	tmp = qse_awk_rtx_format(rtx, &buf, &fbu, tmp, tmp_len, (qse_size_t)-1, (qse_awk_nde_t*)v, &tmp_len);
	if (tmp == QSE_NULL) goto oops;

	switch (type)
	{
		case QSE_AWK_RTX_VALTOSTR_CPL:
			/* CPL and CPLCP behave the same for flt_t.
			 * i just fall through assuming that cplcpy 
			 * and cpl are the same type. the following
			 * assertion at least ensure that they have
			 * the same size. */ 
			QSE_ASSERT (QSE_SIZEOF(out->u.cpl) == QSE_SIZEOF(out->u.cplcpy));

		case QSE_AWK_RTX_VALTOSTR_CPLCPY:
			if (out->u.cplcpy.len <= tmp_len)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
				/* store the buffer size required */
				out->u.cplcpy.len = tmp_len + 1; 
				goto oops;
			}

			qse_strncpy (out->u.cplcpy.ptr, tmp, tmp_len);
			out->u.cplcpy.len = tmp_len;
			break;

		case QSE_AWK_RTX_VALTOSTR_CPLDUP:
		{
			qse_str_yield (&buf, QSE_NULL, 0);
			out->u.cpldup.ptr = tmp;
			out->u.cpldup.len = tmp_len;
			break;
		}

		case QSE_AWK_RTX_VALTOSTR_STRP:
		{
			qse_size_t n;

			qse_str_clear (out->u.strp);

			n = qse_str_ncat (out->u.strp, tmp, tmp_len);
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				goto oops;
			}
			break;
		}

		case QSE_AWK_RTX_VALTOSTR_STRPCAT:
		{
			qse_size_t n;
	
			n = qse_str_ncat (out->u.strpcat, tmp, tmp_len);
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				goto oops;
			}
			break;
		}

		default:
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
			goto oops;
		}
	}

	qse_str_fini (&fbu);
	qse_str_fini (&buf);
	return 0;

oops:
	if (fbu_inited) qse_str_fini (&fbu);
	if (buf_inited) qse_str_fini (&buf);
	return -1;
}

static int val_ref_to_str (qse_awk_rtx_t* rtx, const qse_awk_val_ref_t* ref, qse_awk_rtx_valtostr_out_t* out)
{
	switch (ref->id)
	{
		case QSE_AWK_VAL_REF_POS:
		{
			qse_size_t idx;
	       
			/* special case when the reference value is 
			 * pointing to the positional */

			idx = (qse_size_t)ref->adr;
			if (idx == 0)
			{
				return str_to_str(
					rtx,
					QSE_STR_PTR(&rtx->inrec.line),
					QSE_STR_LEN(&rtx->inrec.line),
					out
				);
			}
			else if (idx <= rtx->inrec.nflds)
			{
				return str_to_str(
					rtx,
					rtx->inrec.flds[idx-1].ptr,
					rtx->inrec.flds[idx-1].len,
					out
				);
			}
			else
			{
				return str_to_str(rtx, QSE_T(""), 0, out);
			}
		}

		case QSE_AWK_VAL_REF_GBL:
		{
			qse_size_t idx = (qse_size_t)ref->adr;
			return qse_awk_rtx_valtostr (rtx, RTX_STACK_GBL (rtx, idx), out);
		}

		default:
		{
			qse_awk_val_t** xref = (qse_awk_val_t**)ref->adr;

			/* A reference value is not able to point to another 
			 * refernce value for the way values are represented
			 * in QSEAWK */
			QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE (rtx, *xref) != QSE_AWK_VAL_REF); 

			/* make a recursive call back to the caller */
			return qse_awk_rtx_valtostr (rtx, *xref, out);
		}
	}
}

int qse_awk_rtx_valtostr (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_awk_rtx_valtostr_out_t* out)
{
	qse_awk_val_type_t vtype = QSE_AWK_RTX_GETVALTYPE(rtx, v);

	switch (vtype)
	{
		case QSE_AWK_VAL_NIL:
		{
			return str_to_str(rtx, QSE_T(""), 0, out);
		}

		case QSE_AWK_VAL_INT:
		{
			return val_int_to_str(rtx, (qse_awk_val_int_t*)v, out);
		}

		case QSE_AWK_VAL_FLT:
		{
			return val_flt_to_str(rtx, (qse_awk_val_flt_t*)v, out);
		}

		case QSE_AWK_VAL_STR:
		{
			qse_awk_val_str_t* vs = (qse_awk_val_str_t*)v;
			return str_to_str(rtx, vs->val.ptr, vs->val.len, out);
		}

		case QSE_AWK_VAL_MBS:
		{
			qse_awk_val_mbs_t* vs = (qse_awk_val_mbs_t*)v;
		#if defined(QSE_CHAR_IS_MCHAR)
			return str_to_str(rtx, vs->val.ptr, vs->val.len, out);
		#else
			return mbs_to_str(rtx, vs->val.ptr, vs->val.len, out);
		#endif
		}

		case QSE_AWK_VAL_FUN:
		{
			return str_to_str(rtx, ((qse_awk_val_fun_t*)v)->fun->name.ptr, ((qse_awk_val_fun_t*)v)->fun->name.len, out);
		}

		case QSE_AWK_VAL_MAP:
		{
			if (rtx->awk->opt.trait & QSE_AWK_FLEXMAP)
			{
				return str_to_str(rtx, QSE_T("#MAP"), 4, out);
			}
			break;
		}

		case QSE_AWK_VAL_REF:
		{
			return val_ref_to_str(rtx, (qse_awk_val_ref_t*)v, out);
		}
	}


#ifdef DEBUG_VAL
	qse_errputstrf (
		QSE_T(">>WRONG VALUE TYPE [%d] in qse_awk_rtx_valtostr\n"), 
		v->type
	);
#endif
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EVALTOSTR, QSE_NULL);
	return -1;
}

qse_mchar_t* qse_awk_rtx_valtombsdupwithcmgr (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_size_t* len, qse_cmgr_t* cmgr)
{
	qse_mchar_t* mbs;
	qse_awk_val_type_t vtype;

	vtype = QSE_AWK_RTX_GETVALTYPE(rtx,v);

	switch (vtype)
	{
		case QSE_AWK_VAL_MBS:
		{
			mbs = qse_mbsxdup(((qse_awk_val_mbs_t*)v)->val.ptr, ((qse_awk_val_mbs_t*)v)->val.len, qse_awk_rtx_getmmgr(rtx));
			if (!mbs) 
			{
				qse_awk_rtx_seterror (rtx, QSE_AWK_ENOMEM, QSE_NULL, QSE_NULL);
				return QSE_NULL;
			}

			if (len) *len = ((qse_awk_val_mbs_t*)v)->val.len;
			break;
		}
		
		case QSE_AWK_VAL_STR:
		{
		#if defined(QSE_CHAR_IS_MCHAR)
			mbs = qse_strxdup(((qse_awk_val_str_t*)v)->val.ptr, ((qse_awk_val_str_t*)v)->val.len, qse_awk_rtx_getmmgr(rtx));
			if (!mbs) 
			{
				qse_awk_rtx_seterror (rtx, QSE_AWK_ENOMEM, QSE_NULL, QSE_NULL);
				return QSE_NULL;
			}

			if (len) *len = ((qse_awk_val_str_t*)v)->val.len;
		#else
			qse_size_t mbslen, wcslen;
			wcslen = ((qse_awk_val_str_t*)v)->val.len;
			mbs = qse_wcsntombsdupwithcmgr(((qse_awk_val_str_t*)v)->val.ptr, wcslen, &mbslen, qse_awk_rtx_getmmgr(rtx), cmgr);
			if (!mbs) 
			{
				qse_awk_rtx_seterror (rtx, QSE_AWK_ENOMEM, QSE_NULL, QSE_NULL);
				return QSE_NULL;
			}

			if (len) *len = mbslen;
		#endif
			break;
		}

		default:
		{
		#if defined(QSE_CHAR_IS_MCHAR)
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr(rtx, v, &out) <= -1) return QSE_NULL;

			mbs = out.u.cpldup.ptr;
			if (len) *len = out.u.cpldup.len;
		#else
			qse_size_t mbslen;
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr(rtx, v, &out) <= -1) return QSE_NULL;

			mbs = qse_wcsntombsdupwithcmgr(out.u.cpldup.ptr, out.u.cpldup.len, &mbslen, qse_awk_rtx_getmmgr(rtx), cmgr);
			qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
			if (!mbs)
			{
				qse_awk_rtx_seterror (rtx, QSE_AWK_ENOMEM, QSE_NULL, QSE_NULL);
				return QSE_NULL;
			}

			if (len) *len = mbslen;
		#endif
			break;
		}
	}

	return mbs;
}

qse_wchar_t* qse_awk_rtx_valtowcsdupwithcmgr (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_size_t* len, qse_cmgr_t* cmgr)
{
	qse_wchar_t* wcs;
	qse_awk_val_type_t vtype;

	vtype = QSE_AWK_RTX_GETVALTYPE(rtx,v);

	switch (vtype)
	{
		case QSE_AWK_VAL_MBS:
		{
			qse_size_t mbslen, wcslen;
			mbslen = ((qse_awk_val_mbs_t*)v)->val.len;
			wcs = qse_mbsntowcsalldupwithcmgr(((qse_awk_val_mbs_t*)v)->val.ptr, &mbslen, &wcslen, qse_awk_rtx_getmmgr(rtx), cmgr);
			if (!wcs) 
			{
				qse_awk_rtx_seterror (rtx, QSE_AWK_ENOMEM, QSE_NULL, QSE_NULL);
				return QSE_NULL;
			}

			if (len) *len = wcslen;
			break;
		}

		case QSE_AWK_VAL_STR:
		{
		#if defined(QSE_CHAR_IS_MCHAR)
			qse_size_t wcslen, mbslen;
			mbslen = ((qse_awk_val_str_t*)v)->val.len;
			wcs = qse_mbsntowcsalldupwithcmgr(((qse_awk_val_str_t*)v)->val.ptr, &mbslen, &wcslen, qse_awk_rtx_getmmgr(rtx), cmgr);
		#else
			wcs = qse_strxdup(((qse_awk_val_str_t*)v)->val.ptr, ((qse_awk_val_str_t*)v)->val.len, qse_awk_rtx_getmmgr(rtx));
		#endif
			if (!wcs) 
			{
				qse_awk_rtx_seterror (rtx, QSE_AWK_ENOMEM, QSE_NULL, QSE_NULL);
				return QSE_NULL;
			}

		#if defined(QSE_CHAR_IS_MCHAR)
			if (len) *len = wcslen;
		#else
			if (len) *len = ((qse_awk_val_str_t*)v)->val.len;
		#endif
			break;
		}

		default:
		{
		#if defined(QSE_CHAR_IS_MCHAR)
			qse_size_t wcslen;
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr(rtx, v, &out) <= -1) return QSE_NULL;

			wcs = qse_mbsntowcsalldupwithcmgr(out.u.cpldup.ptr, &out.u.cpldup.len, &wcslen, qse_awk_rtx_getmmgr(rtx), cmgr);
			qse_awk_rtx_freemem (rtx, out.u.cpldup.ptr);
			if (!wcs)
			{
				qse_awk_rtx_seterror (rtx, QSE_AWK_ENOMEM, QSE_NULL, QSE_NULL);
				return QSE_NULL;
			}

			if (len) *len = wcslen;
		#else
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr(rtx, v, &out) <= -1) return QSE_NULL;

			wcs = out.u.cpldup.ptr;
			if (len) *len = out.u.cpldup.len;
		#endif
			break;
		}
	}
	return wcs;
}

qse_char_t* qse_awk_rtx_getvalstrwithcmgr (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_size_t* len, qse_cmgr_t* cmgr)
{
	if (QSE_AWK_RTX_GETVALTYPE(rtx, v) == QSE_AWK_VAL_STR)
	{
		if (len) *len = ((qse_awk_val_str_t*)v)->val.len;
		return ((qse_awk_val_str_t*)v)->val.ptr;
	}
	else
	{
		return qse_awk_rtx_valtostrdupwithcmgr(rtx, v, len, cmgr);
	}
}

void qse_awk_rtx_freevalstr (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_char_t* str)
{
	if (QSE_AWK_RTX_GETVALTYPE(rtx, v) != QSE_AWK_VAL_STR ||
	    str != ((qse_awk_val_str_t*)v)->val.ptr)
	{
		qse_awk_rtx_freemem (rtx, str);
	}
}

qse_mchar_t* qse_awk_rtx_getvalmbswithcmgr (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_size_t* len, qse_cmgr_t* cmgr)
{
	if (QSE_AWK_RTX_GETVALTYPE(rtx, v) == QSE_AWK_VAL_MBS)
	{
		if (len) *len = ((qse_awk_val_mbs_t*)v)->val.len;
		return ((qse_awk_val_mbs_t*)v)->val.ptr;
	}
	else
	{
		return qse_awk_rtx_valtombsdupwithcmgr(rtx, v, len, cmgr);
	}
}

void qse_awk_rtx_freevalmbs (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_mchar_t* str)
{
	if (QSE_AWK_RTX_GETVALTYPE(rtx, v) != QSE_AWK_VAL_MBS ||
	    str != ((qse_awk_val_mbs_t*)v)->val.ptr)
	{
		qse_awk_rtx_freemem (rtx, str);
	}
}

static int val_ref_to_num (qse_awk_rtx_t* rtx, const qse_awk_val_ref_t* ref, qse_awk_int_t* l, qse_awk_flt_t* r)
{
	switch (ref->id)
	{
		case QSE_AWK_VAL_REF_POS:
		{
			qse_size_t idx;
	       
			idx = (qse_size_t)ref->adr;
			if (idx == 0)
			{
				return qse_awk_rtx_strtonum(
					rtx, 
					QSE_AWK_RTX_STRTONUM_MAKE_OPTION(0, 0),
					QSE_STR_PTR(&rtx->inrec.line),
					QSE_STR_LEN(&rtx->inrec.line),
					l, r
				);
			}
			else if (idx <= rtx->inrec.nflds)
			{
				return qse_awk_rtx_strtonum(
					rtx,
					QSE_AWK_RTX_STRTONUM_MAKE_OPTION(0, 0),
					rtx->inrec.flds[idx-1].ptr,
					rtx->inrec.flds[idx-1].len,
					l, r
				);
			}
			else
			{
				return qse_awk_rtx_strtonum(
					rtx, QSE_AWK_RTX_STRTONUM_MAKE_OPTION(0, 0), QSE_T(""), 0, l, r
				);
			}
		}

		case QSE_AWK_VAL_REF_GBL:
		{
			qse_size_t idx = (qse_size_t)ref->adr;
			return qse_awk_rtx_valtonum (rtx, RTX_STACK_GBL (rtx, idx), l, r);
		}

		default:
		{
			qse_awk_val_t** xref = (qse_awk_val_t**)ref->adr;

			/* A reference value is not able to point to another 
			 * refernce value for the way values are represented
			 * in QSEAWK */
			QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE(rtx, *xref) != QSE_AWK_VAL_REF); 

			/* make a recursive call back to the caller */
			return qse_awk_rtx_valtonum(rtx, *xref, l, r);
		}
	}
}


int qse_awk_rtx_valtonum (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_awk_int_t* l, qse_awk_flt_t* r)
{
	qse_awk_val_type_t vtype = QSE_AWK_RTX_GETVALTYPE (rtx, v);

	switch (vtype)
	{
		case QSE_AWK_VAL_NIL:
		{
			*l = 0;
			return 0;
		}

		case QSE_AWK_VAL_INT:
		{
			*l = QSE_AWK_RTX_GETINTFROMVAL (rtx, v);
			return 0; /* long */
		}

		case QSE_AWK_VAL_FLT:
		{
			*r = ((qse_awk_val_flt_t*)v)->val;
			return 1; /* real */
		}

		case QSE_AWK_VAL_STR:
		{
			return qse_awk_rtx_strtonum(
				rtx, 
				QSE_AWK_RTX_STRTONUM_MAKE_OPTION(0, 0),
				((qse_awk_val_str_t*)v)->val.ptr,
				((qse_awk_val_str_t*)v)->val.len,
				l, r
			);
		}

		case QSE_AWK_VAL_MBS:
		{
			return qse_awk_rtx_mbstonum(
				rtx, 
				QSE_AWK_RTX_STRTONUM_MAKE_OPTION(0, 0),
				((qse_awk_val_mbs_t*)v)->val.ptr,
				((qse_awk_val_mbs_t*)v)->val.len,
				l, r
			);
		}

		case QSE_AWK_VAL_FUN:
		{
			/* unable to convert a function to a number */
			break;
		}

		case QSE_AWK_VAL_MAP:
		{
			if (rtx->awk->opt.trait & QSE_AWK_FLEXMAP)
			{
				*l = QSE_HTB_SIZE(((qse_awk_val_map_t*)v)->map);
				return 0; /* long */
			}
			break;
		}

		case QSE_AWK_VAL_REF:
		{
			return val_ref_to_num(rtx, (qse_awk_val_ref_t*)v, l, r);
		}
	}

#ifdef DEBUG_VAL
	qse_errputstrf (
		QSE_T(">>WRONG VALUE TYPE [%d] in qse_awk_rtx_valtonum()\n"),
		v->type
	);
#endif

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EVALTONUM, QSE_NULL);
	return -1; /* error */
}

int qse_awk_rtx_valtoint (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_awk_int_t* l)
{
	int n;
	qse_awk_flt_t r;

	n = qse_awk_rtx_valtonum(rtx, v, l, &r);
	if (n == 1) 
	{
		*l = (qse_awk_int_t)r;
		n = 0;
	}

	return n;
}

int qse_awk_rtx_valtoflt (qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_awk_flt_t* r)
{
	int n;
	qse_awk_int_t l;

	n = qse_awk_rtx_valtonum(rtx, v, &l, r);
	if (n == 0) *r = (qse_awk_flt_t)l;
	else if (n == 1) n = 0;

	return n;
}

/* ========================================================================== */
#undef awk_rtx_strtonum
#undef awk_strxtoint
#undef awk_strxtoflt
#undef char_t
#undef AWK_ISDIGIT
#undef _T

#define awk_rtx_strtonum qse_awk_rtx_mbstonum
#define awk_strxtoint qse_awk_mbsxtoint
#define awk_strxtoflt qse_awk_mbsxtoflt
#define char_t qse_mchar_t
#define AWK_ISDIGIT QSE_AWK_ISMDIGIT
#define _T QSE_MT
#include "val-imp.h"

/* ------------------------------------------------------------------------- */
#undef awk_rtx_strtonum
#undef awk_strxtoint
#undef awk_strxtoflt
#undef char_t
#undef AWK_ISDIGIT
#undef _T
/* ------------------------------------------------------------------------- */

#define awk_rtx_strtonum qse_awk_rtx_wcstonum
#define awk_strxtoint qse_awk_wcsxtoint
#define awk_strxtoflt qse_awk_wcsxtoflt
#define char_t qse_wchar_t
#define AWK_ISDIGIT QSE_AWK_ISWDIGIT
#define _T QSE_WT
#include "val-imp.h"

#undef awk_rtx_strtonum
#undef awk_strxtoint
#undef awk_strxtoflt
#undef char_t
#undef AWK_ISDIGIT
#undef _T
/* ========================================================================== */

static QSE_INLINE qse_awk_uint_t hash (qse_uint8_t* ptr, qse_size_t len)
{
	qse_awk_uint_t h;
	QSE_HASH_BYTES (h, ptr, len);
	return h;
}

qse_awk_int_t qse_awk_rtx_hashval (qse_awk_rtx_t* rtx, qse_awk_val_t* v)
{
	qse_awk_val_type_t vtype = QSE_AWK_RTX_GETVALTYPE (rtx, v);
	qse_awk_int_t hv;

	switch (vtype)
	{
		case QSE_AWK_VAL_NIL:
			hv = 0;
			break;

		case QSE_AWK_VAL_INT:
		{
			qse_awk_int_t tmp = QSE_AWK_RTX_GETINTFROMVAL(rtx, v);
			/*hv = ((qse_awk_val_int_t*)v)->val;*/
			hv = (qse_awk_int_t)hash((qse_uint8_t*)&tmp, QSE_SIZEOF(tmp));
			break;
		}

		case QSE_AWK_VAL_FLT:
		{
			qse_awk_val_flt_t* dv = (qse_awk_val_flt_t*)v;
			hv = (qse_awk_int_t)hash((qse_uint8_t*)&dv->val, QSE_SIZEOF(dv->val));
			break;
		}

		case QSE_AWK_VAL_STR:
		{
			qse_awk_val_str_t* dv = (qse_awk_val_str_t*)v;
			hv = (qse_awk_int_t)hash((qse_uint8_t*)dv->val.ptr, dv->val.len * QSE_SIZEOF(*dv->val.ptr));
			break;
		}

		case QSE_AWK_VAL_MBS:
		{
			qse_awk_val_mbs_t* dv = (qse_awk_val_mbs_t*)v;
			hv = (qse_awk_int_t)hash((qse_uint8_t*)dv->val.ptr, dv->val.len * QSE_SIZEOF(*dv->val.ptr));
			break;
		}

		default:

#ifdef DEBUG_VAL
			qse_errputstrf (
				QSE_T(">>WRONG VALUE TYPE [%d] in qse_awk_rtx_hashval()\n"), 
				v->type
			);
#endif
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_EHASHVAL, QSE_NULL);
			return -1;
	}

	/* turn off the sign bit */
	return hv  & ~(((qse_awk_uint_t)1) << ((QSE_SIZEOF(qse_awk_uint_t) * 8) - 1));
}

qse_awk_val_type_t qse_awk_rtx_getrefvaltype (qse_awk_rtx_t* rtx, qse_awk_val_ref_t* ref)
{
	/* return the type of the value that the reference points to */
	switch (ref->id)
	{
		case QSE_AWK_VAL_REF_POS:
		{
			return QSE_AWK_VAL_STR;
		}
		case QSE_AWK_VAL_REF_GBL:
		{
			qse_size_t idx;
			qse_awk_val_t* v;
			idx = (qse_size_t)ref->adr;
			v = RTX_STACK_GBL(rtx, idx);
			return QSE_AWK_RTX_GETVALTYPE(rtx, v);
		}

		default:
		{
			qse_awk_val_t** xref = (qse_awk_val_t**)ref->adr;
			qse_awk_val_t* v;

			/* A reference value is not able to point to another 
			 * refernce value for the way values are represented
			 * in QSEAWK */
			v = *xref;
			QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE(rtx, v) != QSE_AWK_VAL_REF); 
			return QSE_AWK_RTX_GETVALTYPE(rtx, v);
		}
	}
}

qse_awk_val_t* qse_awk_rtx_getrefval (qse_awk_rtx_t* rtx, qse_awk_val_ref_t* ref)
{
	switch (ref->id)
	{
		case QSE_AWK_VAL_REF_POS:
		{
			/* a positional doesn't contain a value. you should use qse_awk_rtx_valtoXXX()
			 * like qse_awk_rtx_valtostr(), qse_awk_rtx_valtoint() */
			return QSE_NULL;
		}

		case QSE_AWK_VAL_REF_GBL:
		{
			qse_size_t idx;
			idx = (qse_size_t)ref->adr;
			return RTX_STACK_GBL(rtx, idx);
		}

		default:
		{
			qse_awk_val_t** xref = (qse_awk_val_t**)ref->adr;
			/* A reference value is not able to point to another 
			 * refernce value for the way values are represented
			 * in QSEAWK */
			QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE (rtx, *xref)!= QSE_AWK_VAL_REF); 
			return *xref;
		}
	}
}

int qse_awk_rtx_setrefval (qse_awk_rtx_t* rtx, qse_awk_val_ref_t* ref, qse_awk_val_t* val)
{
	qse_awk_val_type_t vtype = QSE_AWK_RTX_GETVALTYPE (rtx, val);

	if (vtype == QSE_AWK_VAL_REX || vtype == QSE_AWK_VAL_REF)
	{
		/* though it is possible that an intrinsic function handler
		 * can accept a regular expression withtout evaluation when 'x'
		 * is specified for the parameter, this function doesn't allow
		 * regular expression to be set to a reference variable to
		 * avoid potential chaos. the nature of performing '/rex/ ~ $0' 
		 * for a regular expression without the match operator
		 * makes it difficult to be implemented. */
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
		return -1;
	}

	switch (ref->id)
	{
		case QSE_AWK_VAL_REF_POS:
		{
			switch (vtype)
			{
				case QSE_AWK_VAL_MAP:
					/* a map is assigned to a positional. this is disallowed. */
					qse_awk_rtx_seterrnum (rtx, QSE_AWK_EMAPTOPOS, QSE_NULL);
					return -1;

				case QSE_AWK_VAL_STR:
				{
					int x;
					/* handle this separately from the default case
					 * for no duplication. jumping to the default case
					 * and callingqse_awk_rtx_valtostrdup() would also work, anyway. */
					qse_awk_rtx_refupval (rtx, val);
					x = qse_awk_rtx_setrec(rtx, (qse_size_t)ref->adr, &((qse_awk_val_str_t*)val)->val);
					qse_awk_rtx_refdownval (rtx, val);
					return x;
				}

				case QSE_AWK_VAL_MBS:
				#if defined(QSE_CHAR_IS_MCHAR)
				{
					/* same as str in the mchar mode */
					int x;
					qse_awk_rtx_refupval (rtx, val);
					x = qse_awk_rtx_setrec(rtx, (qse_size_t)ref->adr, &((qse_awk_val_mbs_t*)val)->val);
					qse_awk_rtx_refdownval (rtx, val);
					return x;
				}
				#endif
					/* fall thru otherwise */

				default:
				{
					qse_cstr_t str;
					int x;

					str.ptr = qse_awk_rtx_valtostrdup(rtx, val, &str.len);
					qse_awk_rtx_refupval (rtx, val);
					x = qse_awk_rtx_setrec(rtx, (qse_size_t)ref->adr, &str);
					qse_awk_rtx_refdownval (rtx, val);
					qse_awk_rtx_freemem (rtx, str.ptr);
					return x;
				}
			}
		}

		case QSE_AWK_VAL_REF_GBL:
			/* ref->adr is the index to the global variables, not a real pointer address for QSE_AWK_VAL_REF_GBL */
			return qse_awk_rtx_setgbl(rtx, (int)ref->adr, val);

		case QSE_AWK_VAL_REF_NAMEDIDX:
		case QSE_AWK_VAL_REF_GBLIDX:
		case QSE_AWK_VAL_REF_LCLIDX:
		case QSE_AWK_VAL_REF_ARGIDX:
			if (vtype == QSE_AWK_VAL_MAP)
			{
				/* an indexed variable cannot be assigned a map. 
				 * in other cases, it falls down to the default case. */
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EMAPTOIDX, QSE_NULL);
				return -1;
			}
			/* fall through */

		default:
		{
			qse_awk_val_t** rref;
			qse_awk_val_type_t rref_vtype;

			rref = (qse_awk_val_t**)ref->adr; /* old value pointer */
			rref_vtype = QSE_AWK_RTX_GETVALTYPE(rtx, *rref); /* old value type */
			if (vtype == QSE_AWK_VAL_MAP)
			{
				/* new value: map, old value: nil or map => ok */
				if (rref_vtype != QSE_AWK_VAL_NIL && rref_vtype != QSE_AWK_VAL_MAP)
				{
					if (!(rtx->awk->opt.trait & QSE_AWK_FLEXMAP))
					{
						/* cannot change a scalar value to a map */
						qse_awk_rtx_seterrnum (rtx, QSE_AWK_ESCALARTOMAP, QSE_NULL);
						return -1;
					}
				}
			}
			else
			{
				/* new value: scalar, old value: nil or scalar => ok */
				if (rref_vtype == QSE_AWK_VAL_MAP)
				{
					if (!(rtx->awk->opt.trait & QSE_AWK_FLEXMAP))
					{
						qse_awk_rtx_seterrnum (rtx, QSE_AWK_EMAPTOSCALAR, QSE_NULL);
						return -1;
					}
				}
			}
			
			if (*rref != val)
			{
				/* if the new value is not the same as the old value */
				qse_awk_rtx_refdownval (rtx, *rref);
				*rref = val;
				qse_awk_rtx_refupval (rtx, *rref);
			}
			return 0;
		}
	}
}

#if 0

#define qse_errputstrf qse_errputstrf

static qse_htb_walk_t print_pair (
	qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	qse_awk_rtx_t* run = (qse_awk_rtx_t*)arg;

	QSE_ASSERT (run == *(qse_awk_rtx_t**)QSE_XTN(map));

	qse_errputstrf (QSE_T(" %.*s=>"),
		(int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair));
	qse_awk_dprintval ((qse_awk_rtx_t*)arg, QSE_HTB_VPTR(pair));
	qse_errputstrf (QSE_T(" "));

	return QSE_HTB_WALK_FORWARD;
}

void qse_awk_dprintval (qse_awk_rtx_t* run, qse_awk_val_t* val)
{
	/* TODO: better value printing ... */

	switch (val->type)
	{
		case QSE_AWK_VAL_NIL:
			qse_errputstrf (QSE_T("nil"));
		       	break;

		case QSE_AWK_VAL_INT:
			qse_errputstrf (QSE_T("%jd"), 
				(qse_intmax_t)((qse_awk_val_int_t*)val)->val);
			break;

		case QSE_AWK_VAL_FLT:
			qse_errputstrf (QSE_T("%jf"), 
				(qse_fltmax_t)((qse_awk_val_flt_t*)val)->val);
			break;

		case QSE_AWK_VAL_STR:
			qse_errputstrf (QSE_T("%s"), ((qse_awk_val_str_t*)val)->ptr);
			break;

		case QSE_AWK_VAL_MBS:
			qse_errputstrf (QSE_T("%hs"), ((qse_awk_val_mbs_t*)val)->ptr);
			break;

		case QSE_AWK_VAL_REX:
			qse_errputstrf (QSE_T("/%s/"), ((qse_awk_val_rex_t*)val)->ptr);
			break;

		case QSE_AWK_VAL_FUN:
			qse_errputstrf (QSE_T("%.*s"), (int)((qse_awk_val_fun_t*)val)->fun->name.len, ((qse_awk_val_fun_t*)val)->fun->name.ptr);
			break;

		case QSE_AWK_VAL_MAP:
			qse_errputstrf (QSE_T("MAP["));
			qse_htb_walk (((qse_awk_val_map_t*)val)->map, print_pair, run);
			qse_errputstrf (QSE_T("]"));
			break;

		case QSE_AWK_VAL_REF:
			qse_errputstrf (QSE_T("REF[id=%d,val="), ((qse_awk_val_ref_t*)val)->id);
			qse_awk_dprintval (run, *((qse_awk_val_ref_t*)val)->adr);
			qse_errputstrf (QSE_T("]"));
			break;

		default:
			qse_errputstrf (QSE_T("**** INTERNAL ERROR - INVALID VALUE TYPE ****\n"));
	}
}

#endif
