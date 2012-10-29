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

#include "awk.h"
#include <qse/cmn/mbwc.h>

#ifdef DEBUG_VAL
#include <qse/cmn/stdio.h>
#endif

#define CHUNKSIZE QSE_AWK_VAL_CHUNK_SIZE

static qse_awk_val_nil_t awk_nil = { QSE_AWK_VAL_NIL, 0, 0 };
static qse_awk_val_str_t awk_zls = { QSE_AWK_VAL_STR, 0, 0, { QSE_T(""), 0 } };

qse_awk_val_t* qse_awk_val_nil = (qse_awk_val_t*)&awk_nil;
qse_awk_val_t* qse_awk_val_zls = (qse_awk_val_t*)&awk_zls; 

static qse_awk_val_int_t awk_int[] =
{
	{ QSE_AWK_VAL_INT, 0, 0, -1, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  0, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  1, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  2, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  3, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  4, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  5, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  6, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  7, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  8, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0,  9, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 10, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 11, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 12, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 13, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 14, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 15, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 16, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 17, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 18, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 19, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 0, 20, QSE_NULL }
};

qse_awk_val_t* qse_awk_val_negone = (qse_awk_val_t*)&awk_int[0];
qse_awk_val_t* qse_awk_val_zero = (qse_awk_val_t*)&awk_int[1];
qse_awk_val_t* qse_awk_val_one = (qse_awk_val_t*)&awk_int[2];

qse_awk_val_t* qse_awk_rtx_makenilval (qse_awk_rtx_t* rtx)
{
	return (qse_awk_val_t*)&awk_nil;
}

qse_awk_val_t* qse_awk_rtx_makeintval (qse_awk_rtx_t* rtx, qse_long_t v)
{
	qse_awk_val_int_t* val;

	if (v >= awk_int[0].val && 
	    v <= awk_int[QSE_COUNTOF(awk_int)-1].val)
	{
		return (qse_awk_val_t*)&awk_int[v-awk_int[0].val];
	}

	if (rtx->vmgr.ifree == QSE_NULL)
	{
		qse_awk_val_ichunk_t* c;
		/*qse_awk_val_int_t* x;*/
		qse_size_t i;

		/* use qse_awk_val_ichunk structure to avoid
		 * any alignment issues on platforms requiring
		 * aligned memory access - using the code commented out
		 * will cause a fault on such a platform */

		/* c = QSE_AWK_ALLOC (run->awk, 
			QSE_SIZEOF(qse_awk_val_chunk_t)+
			QSE_SIZEOF(qse_awk_val_int_t)*CHUNKSIZE); */
		c = QSE_AWK_ALLOC (rtx->awk, QSE_SIZEOF(qse_awk_val_ichunk_t));
		if (c == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			return QSE_NULL;
		}

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

	val->type = QSE_AWK_VAL_INT;
	val->ref = 0;
	val->nstr = 0;
	val->val = v;
	val->nde = QSE_NULL;

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makeintval => %ld [%p]\n"), (long)v, val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makefltval (qse_awk_rtx_t* rtx, qse_flt_t v)
{
	qse_awk_val_flt_t* val;

	if (rtx->vmgr.rfree == QSE_NULL)
	{
		qse_awk_val_rchunk_t* c;
		/*qse_awk_val_flt_t* x;*/
		qse_size_t i;

		/* c = QSE_AWK_ALLOC (run->awk, 
			QSE_SIZEOF(qse_awk_val_chunk_t)+
			QSE_SIZEOF(qse_awk_val_flt_t)*CHUNKSIZE); */
		c = QSE_AWK_ALLOC (rtx->awk, QSE_SIZEOF(qse_awk_val_rchunk_t));
		if (c == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			return QSE_NULL;
		}

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

	val->type = QSE_AWK_VAL_FLT;
	val->ref = 0;
	val->nstr = 0;
	val->val = v;
	val->nde = QSE_NULL;

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makefltval => %Lf [%p]\n"), (double)v, val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithmbs (
	qse_awk_rtx_t* rtx, const qse_mchar_t* mbs)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return qse_awk_rtx_makestrval (rtx, mbs, qse_mbslen(mbs));
#else
	qse_awk_val_t* v;
	qse_wxstr_t tmp;

	tmp.ptr = qse_mbstowcsdup (mbs, &tmp.len, rtx->awk->mmgr);
	if (tmp.ptr == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	v = qse_awk_rtx_makestrvalwithcstr (rtx, &tmp);
	QSE_AWK_FREE (rtx->awk, tmp.ptr);
	return v;
#endif
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithwcs (
	qse_awk_rtx_t* rtx, const qse_wchar_t* wcs)
{
#if defined(QSE_CHAR_IS_MCHAR)
	qse_awk_val_t* v;
	qse_mxstr_t tmp;

	tmp.ptr = qse_wcstombsdup (wcs, &tmp.len, rtx->awk->mmgr);
	if (tmp.ptr == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	v = qse_awk_rtx_makestrvalwithcstr (rtx, &tmp);
	QSE_AWK_FREE (rtx->awk, tmp.ptr);
	return v;
#else
	return qse_awk_rtx_makestrval (rtx, wcs, qse_wcslen(wcs));
#endif
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithstr (
	qse_awk_rtx_t* rtx, const qse_char_t* str)
{
	return qse_awk_rtx_makestrval (rtx, str, qse_strlen(str));
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithmcstr (
	qse_awk_rtx_t* rtx, const qse_mcstr_t* mcstr)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return qse_awk_rtx_makestrvalwithcstr (rtx, mcstr);
#else
	qse_awk_val_t* v;
	qse_wcstr_t tmp;
	qse_size_t mbslen;

	mbslen = mcstr->len;
	tmp.ptr = qse_mbsntowcsdup (mcstr->ptr, &mbslen, &tmp.len, rtx->awk->mmgr);
	if (tmp.ptr == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	v = qse_awk_rtx_makestrvalwithcstr (rtx, &tmp);
	QSE_AWK_FREE (rtx->awk, tmp.ptr);
	return v;
#endif
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithwcstr (
	qse_awk_rtx_t* rtx, const qse_wcstr_t* wcstr)
{
#if defined(QSE_CHAR_IS_MCHAR)
	qse_awk_val_t* v;
	qse_mcstr_t tmp;
	qse_size_t wcslen;

	wcslen = wcstr->len;
	tmp.ptr = qse_wcsntombsdup (wcstr->ptr, &wcslen, &tmp.len, rtx->awk->mmgr);
	if (tmp.ptr == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	v = qse_awk_rtx_makestrvalwithcstr (rtx, &tmp);
	QSE_AWK_FREE (rtx->awk, tmp.ptr);
	return v;
#else
	return qse_awk_rtx_makestrvalwithcstr (rtx, wcstr);
#endif
}

qse_awk_val_t* qse_awk_rtx_makestrvalwithcstr (
	qse_awk_rtx_t* rtx, const qse_cstr_t* str)
{
	qse_awk_val_str_t* val = QSE_NULL;
	qse_size_t rlen = str->len;

#ifdef ENABLE_FEATURE_SCACHE
	qse_size_t i;

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

	val = (qse_awk_val_str_t*) QSE_AWK_ALLOC (
		rtx->awk,
		QSE_SIZEOF(qse_awk_val_str_t) + 
		(rlen + 1) * QSE_SIZEOF(qse_char_t));
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

#ifdef ENABLE_FEATURE_SCACHE
init:
#endif
	val->type = QSE_AWK_VAL_STR;
	val->ref = 0;
	val->nstr = 0;
	val->val.len = str->len;
	val->val.ptr = (qse_char_t*)(val + 1);
	qse_strncpy (val->val.ptr, str->ptr, str->len);

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makestrval => %p\n"), val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makestrval (
	qse_awk_rtx_t* rtx, const qse_char_t* str, qse_size_t len)
{
	qse_cstr_t cstr;
	cstr.ptr = str;
	cstr.len = len;
	return qse_awk_rtx_makestrvalwithcstr (rtx, &cstr);
}

qse_awk_val_t* qse_awk_rtx_makestrval2 (
	qse_awk_rtx_t* rtx,
	const qse_char_t* str1, qse_size_t len1, 
	const qse_char_t* str2, qse_size_t len2)
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

	val = (qse_awk_val_str_t*) QSE_AWK_ALLOC (
		rtx->awk,
		QSE_SIZEOF(qse_awk_val_str_t) +
		(rlen+1)*QSE_SIZEOF(qse_char_t));
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

#ifdef ENABLE_FEATURE_SCACHE
init:
#endif
	val->type = QSE_AWK_VAL_STR;
	val->ref = 0;
	val->nstr = 0;
	val->val.len = len1 + len2;
	val->val.ptr = (qse_char_t*)(val + 1);
	qse_strncpy (val->val.ptr, str1, len1);
	qse_strncpy (&val->val.ptr[len1], str2, len2);

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makestrval2 => %p\n"), val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makenstrvalwithcstr (qse_awk_rtx_t* rtx, const qse_cstr_t* str)
{
	int x;
	qse_awk_val_t* v;
	qse_long_t l;
	qse_flt_t r;

	x = qse_awk_rtx_strtonum (rtx, 1, str->ptr, str->len, &l, &r);
	v = qse_awk_rtx_makestrvalwithcstr (rtx, str);

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

qse_awk_val_t* qse_awk_rtx_makerexval (
	qse_awk_rtx_t* rtx, const qse_char_t* buf, qse_size_t len, void* code)
{
	qse_awk_val_rex_t* val;
	qse_size_t totsz;

	/* the regular expression value holds:
	 * - header
	 * - a raw string plus with added a terminating '\0'
	 * the total size is just large enough for all these.
	 */
	totsz = QSE_SIZEOF(*val) + (QSE_SIZEOF(*buf) * (len + 1));
	val = (qse_awk_val_rex_t*) QSE_AWK_ALLOC (rtx->awk, totsz);
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	val->type = QSE_AWK_VAL_REX;
	val->ref = 0;
	val->nstr = 0;
	val->len = len;

	val->ptr = (qse_char_t*)(val + 1);
	qse_strncpy (val->ptr, buf, len);

	val->code = code;

	return (qse_awk_val_t*)val;
}

static void free_mapval (qse_htb_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_t* rtx = *(qse_awk_rtx_t**)QSE_XTN(map);

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("refdown in map free..."));
	qse_awk_dprintval (rtx, dptr);
	qse_dprintf (QSE_T("\n"));
#endif

	qse_awk_rtx_refdownval (rtx, dptr);
}

static void same_mapval (qse_htb_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_t* run = *(qse_awk_rtx_t**)QSE_XTN(map);
#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("refdown nofree in map free..."));
	qse_awk_dprintval (run, dptr);
	qse_dprintf (QSE_T("\n"));
#endif
	qse_awk_rtx_refdownval_nofree (run, dptr);
}

qse_awk_val_t* qse_awk_rtx_makemapval (qse_awk_rtx_t* rtx)
{
	static qse_htb_mancbs_t mancbs =
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
	val = (qse_awk_val_map_t*) QSE_AWK_ALLOC (
		run->awk, QSE_SIZEOF(qse_awk_val_map_t) );
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	val->type = QSE_AWK_VAL_MAP;
	val->ref = 0;
	val->nstr = 0;
	val->map = qse_htb_open (
		run, 256, 70, free_mapval, same_mapval, run->awk->mmgr);
	if (val->map == QSE_NULL)
	{
		QSE_AWK_FREE (run->awk, val);
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}
	*/

	val = (qse_awk_val_map_t*) QSE_AWK_ALLOC (
		rtx->awk,
		QSE_SIZEOF(qse_awk_val_map_t) +
		QSE_SIZEOF(qse_htb_t) +
		QSE_SIZEOF(rtx));
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	val->type = QSE_AWK_VAL_MAP;
	val->ref = 0;
	val->nstr = 0;
	val->map = (qse_htb_t*)(val + 1);

	if (qse_htb_init (
		val->map, rtx->awk->mmgr, 
		256, 70, QSE_SIZEOF(qse_char_t), 1) <= -1)
	{
		QSE_AWK_FREE (rtx->awk, val);
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}
	*(qse_awk_rtx_t**)QSE_XTN(val->map) = rtx;
	qse_htb_setmancbs (val->map, &mancbs);
	/* END CHECK */

	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makemapvalwithdata (qse_awk_rtx_t* rtx, qse_awk_val_map_data_t data[])
{
	qse_awk_val_t* map, * tmp;
	qse_awk_val_map_data_t* p;

	map = qse_awk_rtx_makemapval (rtx);
	if (map == QSE_NULL) return QSE_NULL;

	for (p = data; p->key.ptr; p++)
	{
		switch (p->type)
		{
			case QSE_AWK_VAL_MAP_DATA_INT:
				tmp = qse_awk_rtx_makeintval (rtx, *(qse_long_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_FLT:
				tmp = qse_awk_rtx_makefltval (rtx, *(qse_flt_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_STR:
				tmp = qse_awk_rtx_makestrvalwithstr (rtx, (qse_char_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_MBS:
				tmp = qse_awk_rtx_makestrvalwithmbs (rtx, (qse_mchar_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_WCS:
				tmp = qse_awk_rtx_makestrvalwithwcs (rtx, (qse_wchar_t*)p->vptr);
				break;
					
			case QSE_AWK_VAL_MAP_DATA_XSTR:
			case QSE_AWK_VAL_MAP_DATA_CSTR:
				tmp = qse_awk_rtx_makestrvalwithcstr (rtx, (qse_cstr_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_MXSTR:
			case QSE_AWK_VAL_MAP_DATA_MCSTR:
				tmp = qse_awk_rtx_makestrvalwithmcstr (rtx, (qse_mcstr_t*)p->vptr);
				break;

			case QSE_AWK_VAL_MAP_DATA_WXSTR:
			case QSE_AWK_VAL_MAP_DATA_WCSTR:
				tmp = qse_awk_rtx_makestrvalwithwcstr (rtx, (qse_wcstr_t*)p->vptr);
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
	QSE_ASSERT (map->type == QSE_AWK_VAL_MAP);

#if 0
	if (map->type != QSE_AWK_VAL_MAP)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOTIDX, QSE_NULL);
		return QSE_NULL;
	}
#endif

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

	QSE_ASSERT (map->type == QSE_AWK_VAL_MAP);

#if 0
	if (map->type != QSE_AWK_VAL_MAP)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOTIDX, QSE_NULL);
		return QSE_NULL;
	}
#endif

	pair = qse_htb_search (((qse_awk_val_map_t*)map)->map, kptr, klen);
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
	QSE_ASSERT (map->type == QSE_AWK_VAL_MAP);
	itr->pair = qse_htb_getfirstpair (
		((qse_awk_val_map_t*)map)->map, &itr->buckno);
	return itr->pair? itr: QSE_NULL;
}

qse_awk_val_map_itr_t* qse_awk_rtx_getnextmapvalitr (
	qse_awk_rtx_t* rtx, qse_awk_val_t* map, qse_awk_val_map_itr_t* itr)
{
	QSE_ASSERT (map->type == QSE_AWK_VAL_MAP);
	itr->pair = qse_htb_getnextpair (
		((qse_awk_val_map_t*)map)->map, itr->pair, &itr->buckno);
	return itr->pair? itr: QSE_NULL;
}

qse_awk_val_t* qse_awk_rtx_makerefval (
	qse_awk_rtx_t* rtx, int id, qse_awk_val_t** adr)
{
	qse_awk_val_ref_t* val;

	if (rtx->rcache_count > 0)
	{
		val = rtx->rcache[--rtx->rcache_count];
	}
	else
	{
		val = (qse_awk_val_ref_t*) QSE_AWK_ALLOC (
			rtx->awk, QSE_SIZEOF(qse_awk_val_ref_t));
		if (val == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			return QSE_NULL;
		}
	}

	val->type = QSE_AWK_VAL_REF;
	val->ref = 0;
	val->nstr = 0;
	val->id = id;
	val->adr = adr;

	return (qse_awk_val_t*)val;
}

#define IS_STATICVAL(val) \
	((val) == QSE_NULL || \
	 (val) == qse_awk_val_nil || \
	 (val) == qse_awk_val_zls || \
	 (val) == qse_awk_val_zero || \
	 (val) == qse_awk_val_one || \
	 ((val) >= (qse_awk_val_t*)&awk_int[0] && \
	  (val) <= (qse_awk_val_t*)&awk_int[QSE_COUNTOF(awk_int)-1]))

int qse_awk_rtx_isstaticval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	return IS_STATICVAL(val);
}

void qse_awk_rtx_freeval (	
	qse_awk_rtx_t* rtx, qse_awk_val_t* val, int cache)
{
	if (IS_STATICVAL(val)) return;

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("freeing [cache=%d] ... "), cache);
	qse_awk_dprintval (rtx, val);
	qse_dprintf (QSE_T("\n"));
#endif

	switch (val->type)
	{
		case QSE_AWK_VAL_NIL:
		{
			QSE_AWK_FREE (rtx->awk, val);
			break;
		}
		
		case QSE_AWK_VAL_INT:
		{
			((qse_awk_val_int_t*)val)->nde = 
				(qse_awk_nde_int_t*)rtx->vmgr.ifree;
			rtx->vmgr.ifree = (qse_awk_val_int_t*)val;
			break;
		}

		case QSE_AWK_VAL_FLT:
		{
			((qse_awk_val_flt_t*)val)->nde =
				(qse_awk_nde_flt_t*)rtx->vmgr.rfree;
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
				else QSE_AWK_FREE (rtx->awk, val);
			}
			else 
		#endif
				QSE_AWK_FREE (rtx->awk, val);

			break;
		}

		case QSE_AWK_VAL_REX:
		{
			/* don't free ptr as it is inlined to val
			QSE_AWK_FREE (rtx->awk, ((qse_awk_val_rex_t*)val)->ptr);
			 */
		
			/* code is just a pointer to a regular expression stored
			 * in parse tree nodes. so don't free it.
			QSE_AWK_FREEREX (rtx->awk, ((qse_awk_val_rex_t*)val)->code);
			 */

			QSE_AWK_FREE (rtx->awk, val);
			break;
		}

		case QSE_AWK_VAL_MAP:
		{
			qse_htb_fini (((qse_awk_val_map_t*)val)->map);
			QSE_AWK_FREE (rtx->awk, val);
			break;
		}

		case QSE_AWK_VAL_REF:
		{
			if (cache && rtx->rcache_count < QSE_COUNTOF(rtx->rcache))
			{
				rtx->rcache[rtx->rcache_count++] = (qse_awk_val_ref_t*)val;	
			}
			else QSE_AWK_FREE (rtx->awk, val);
			break;
		}
	}
}

void qse_awk_rtx_refupval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	if (IS_STATICVAL(val)) return;

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("ref up [ptr=%p] [count=%d] "), val, (int)val->ref);
	qse_awk_dprintval (rtx, val);
	qse_dprintf (QSE_T("\n"));
#endif

	val->ref++;
}

void qse_awk_rtx_refdownval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	if (IS_STATICVAL(val)) return;

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("ref down [ptr=%p] [count=%d]\n"), val, (int)val->ref);
	qse_awk_dprintval (rtx, val);
	qse_dprintf (QSE_T("\n"));
#endif

	QSE_ASSERTX (val->ref > 0, 
		"the reference count of a value should be greater than zero for it"
		" to be decremented. check the source code for any bugs");

	val->ref--;
	if (val->ref <= 0) 
	{
		qse_awk_rtx_freeval(rtx, val, 1);
	}
}

void qse_awk_rtx_refdownval_nofree (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	if (IS_STATICVAL(val)) return;

	QSE_ASSERTX (val->ref > 0,
		"the reference count of a value should be greater than zero for it"
		" to be decremented. check the source code for any bugs");
	val->ref--;
}

void qse_awk_rtx_freevalchunk (qse_awk_rtx_t* rtx, qse_awk_val_chunk_t* chunk)
{
	while (chunk != QSE_NULL)
        {
		qse_awk_val_chunk_t* next = chunk->next;
		QSE_AWK_FREE (rtx->awk, chunk);
		chunk = next;
	}
}

int qse_awk_rtx_valtobool (qse_awk_rtx_t* run, const qse_awk_val_t* val)
{
	if (val == QSE_NULL) return 0;

	switch (val->type)
	{
		case QSE_AWK_VAL_NIL:
			return 0;
		case QSE_AWK_VAL_INT:
			return ((qse_awk_val_int_t*)val)->val != 0;
		case QSE_AWK_VAL_FLT:
			return ((qse_awk_val_flt_t*)val)->val != 0.0;
		case QSE_AWK_VAL_STR:
			return ((qse_awk_val_str_t*)val)->val.len > 0;
		case QSE_AWK_VAL_REX: /* TODO: is this correct? */
			return ((qse_awk_val_rex_t*)val)->len > 0;
		case QSE_AWK_VAL_MAP:
			return 0; /* TODO: is this correct? */
		case QSE_AWK_VAL_REF:
			return 0; /* TODO: is this correct? */
	}

	QSE_ASSERTX (
		!"should never happen - invalid value type",
		"the type of a value should be one of QSE_AWK_VAL_XXX's defined in awk.h");
	return 0;
}

static int str_to_str (
	qse_awk_rtx_t* rtx, const qse_char_t* str, qse_size_t str_len,
	qse_awk_rtx_valtostr_out_t* out)
{
	int type = out->type & ~QSE_AWK_RTX_VALTOSTR_PRINT;

	switch (type)
	{
		case QSE_AWK_RTX_VALTOSTR_CPL:
		{
			out->u.cpl.len = str_len;
			out->u.cpl.ptr = str;
			return 0;
		}

		case QSE_AWK_RTX_VALTOSTR_CPLCPY:
		{
			if (str_len >= out->u.cplcpy.len)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
				out->u.cplcpy.len = str_len + 1;
				return -1;
			}

			out->u.cplcpy.len = 
				qse_strncpy (out->u.cplcpy.ptr, str, str_len);
			return 0;
		}

		case QSE_AWK_RTX_VALTOSTR_CPLDUP:
		{
			qse_char_t* tmp;

			tmp = QSE_AWK_STRXDUP (rtx->awk, str, str_len);
			if (tmp == QSE_NULL) 
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
			n = qse_str_ncat (out->u.strp, str, str_len);
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

			n = qse_str_ncat (out->u.strpcat, str, str_len);
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

static int val_int_to_str (
	qse_awk_rtx_t* rtx, const qse_awk_val_int_t* v,
	qse_awk_rtx_valtostr_out_t* out)
{
	qse_char_t* tmp;
	qse_ulong_t t;
	qse_size_t rlen = 0;
	int type = out->type & ~QSE_AWK_RTX_VALTOSTR_PRINT;

	if (v->val == 0) rlen++;
	else
	{
		/* non-zero values */
		if (v->val < 0) 
		{
			t = v->val * -1; rlen++; 
		}
		else t = v->val;
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
			tmp = QSE_AWK_ALLOC (
				rtx->awk, (rlen + 1) * QSE_SIZEOF(qse_char_t));
			if (tmp == QSE_NULL)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}

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
			n = qse_str_nccat (out->u.strp, QSE_T(' '), rlen);
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
			n = qse_str_nccat (out->u.strpcat, QSE_T(' '), rlen);
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

	if (v->val == 0) tmp[0] = QSE_T('0'); 
	else
	{
		t = (v->val < 0)? (v->val * -1): v->val;

		/* fill in the buffer with digits */
		while (t > 0) 
		{
			tmp[--rlen] = (qse_char_t)(t % 10) + QSE_T('0');
			t /= 10;
		}

		/* insert the negative sign if necessary */
		if (v->val < 0) tmp[--rlen] = QSE_T('-');
	}

	return 0;
}

static int val_flt_to_str (
	qse_awk_rtx_t* rtx, const qse_awk_val_flt_t* v,
	qse_awk_rtx_valtostr_out_t* out)
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

	if (qse_str_init (&buf, rtx->awk->mmgr, 256) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}
	buf_inited = 1;

	if (qse_str_init (&fbu, rtx->awk->mmgr, 256) <= -1)
	{
		qse_str_fini (&buf);
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}
	fbu_inited = 1;

	tmp = qse_awk_rtx_format (rtx, &buf, &fbu, tmp, tmp_len, 
		(qse_size_t)-1, (qse_awk_nde_t*)v, &tmp_len);
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

int qse_awk_rtx_valtostr (
	qse_awk_rtx_t* rtx, const qse_awk_val_t* v,
	qse_awk_rtx_valtostr_out_t* out)
{
	switch (v->type)
	{
		case QSE_AWK_VAL_NIL:
		{
			return str_to_str (rtx, QSE_T(""), 0, out);
		}

		case QSE_AWK_VAL_INT:
		{
			return val_int_to_str (
				rtx, (qse_awk_val_int_t*)v, out);
		}

		case QSE_AWK_VAL_FLT:
		{
			return val_flt_to_str (
				rtx, (qse_awk_val_flt_t*)v, out);
		}

		case QSE_AWK_VAL_STR:
		{
			qse_awk_val_str_t* vs = (qse_awk_val_str_t*)v;
			return str_to_str (rtx, vs->val.ptr, vs->val.len, out);
		}
	}

#ifdef DEBUG_VAL
	qse_dprintf (
		QSE_T(">>WRONG VALUE TYPE [%d] in qse_awk_rtx_valtostr\n"), 
		v->type
	);
#endif
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EVALTYPE, QSE_NULL);
	return -1;
}

qse_char_t* qse_awk_rtx_valtocpldup (
	qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_size_t* len)
{
	qse_awk_rtx_valtostr_out_t out;

	out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
	if (qse_awk_rtx_valtostr (rtx, v, &out) <= -1) return QSE_NULL;

	*len = out.u.cpldup.len;
	return out.u.cpldup.ptr;
}

int qse_awk_rtx_valtonum (
	qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_long_t* l, qse_flt_t* r)
{
	switch (v->type)
	{
		case QSE_AWK_VAL_NIL:
		{
			*l = 0;
			return 0;
		}

		case QSE_AWK_VAL_INT:
		{
			*l = ((qse_awk_val_int_t*)v)->val;
			return 0; /* long */
		}

		case QSE_AWK_VAL_FLT:
		{
			*r = ((qse_awk_val_flt_t*)v)->val;
			return 1; /* real */
		}

		case QSE_AWK_VAL_STR:
		{
			return qse_awk_rtx_strtonum (
				rtx, 0,
				((qse_awk_val_str_t*)v)->val.ptr,
				((qse_awk_val_str_t*)v)->val.len,
				l, r
			);
		}
	}

#ifdef DEBUG_VAL
	qse_dprintf (
		QSE_T(">>WRONG VALUE TYPE [%d] in qse_awk_rtx_valtonum()\n"),
		v->type
	);
#endif

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EVALTYPE, QSE_NULL);
	return -1; /* error */
}

int qse_awk_rtx_valtolong (
	qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_long_t* l)
{
	int n;
	qse_flt_t r;

	n = qse_awk_rtx_valtonum (rtx, v, l, &r);
	if (n == 1) 
	{
		*l = (qse_long_t)r;
		n = 0;
	}

	return n;
}

int qse_awk_rtx_valtoflt (
	qse_awk_rtx_t* rtx, const qse_awk_val_t* v, qse_flt_t* r)
{
	int n;
	qse_long_t l;

	n = qse_awk_rtx_valtonum (rtx, v, &l, r);
	if (n == 0) *r = (qse_flt_t)l;
	else if (n == 1) n = 0;

	return n;
}

int qse_awk_rtx_strtonum (
	qse_awk_rtx_t* rtx, int strict,
	const qse_char_t* ptr, qse_size_t len, 
	qse_long_t* l, qse_flt_t* r)
{
	const qse_char_t* endptr;

	*l = qse_awk_strxtolong (rtx->awk, ptr, len, 0, &endptr);
	if (endptr < ptr + len &&
	    (*endptr == QSE_T('.') ||
	     *endptr == QSE_T('E') ||
	     *endptr == QSE_T('e')))
	{
		*r = qse_awk_strxtoflt (rtx->awk, ptr, len, &endptr);
		if (strict && endptr < ptr + len) return -1;
		return 1; /* real */
	}

	if (strict && endptr < ptr + len) return -1;
	return 0; /* long */
}

static qse_ulong_t hash (qse_uint8_t* ptr, qse_size_t len)
{
	qse_ulong_t h = 5381;
	while (len > 0) h = ((h << 5) + h) + ptr[--len];
	return h;
}

qse_long_t qse_awk_rtx_hashval (qse_awk_rtx_t* rtx, qse_awk_val_t* v)
{
	qse_long_t hv;

	switch (v->type)
	{
		case QSE_AWK_VAL_NIL:
			hv = 0;
			break;

		case QSE_AWK_VAL_INT:
			/*hv = ((qse_awk_val_int_t*)v)->val;*/
			hv = (qse_long_t)hash (
				(qse_uint8_t*)&((qse_awk_val_int_t*)v)->val,
				QSE_SIZEOF(((qse_awk_val_int_t*)v)->val));
			break;

		case QSE_AWK_VAL_FLT:
			hv = (qse_long_t)hash (
				(qse_uint8_t*)&((qse_awk_val_flt_t*)v)->val,
				QSE_SIZEOF(((qse_awk_val_flt_t*)v)->val));
			break;

		case QSE_AWK_VAL_STR:
			hv = (qse_long_t)hash (
				(qse_uint8_t*)((qse_awk_val_str_t*)v)->val.ptr,
				((qse_awk_val_str_t*)v)->val.len * QSE_SIZEOF(qse_char_t));
			break;

		default:

#ifdef DEBUG_VAL
			qse_dprintf (
				QSE_T(">>WRONG VALUE TYPE [%d] in qse_awk_rtx_hashval()\n"), 
				v->type
			);
#endif

			qse_awk_rtx_seterrnum (rtx, QSE_AWK_EVALTYPE, QSE_NULL);
			return -1;
	}

	/* turn off the sign bit */
	return hv  & ~(((qse_ulong_t)1) << ((QSE_SIZEOF(qse_ulong_t) * 8) - 1));
}

#if 0

#define DPRINTF run->awk->prmfns->dprintf
#define DCUSTOM run->awk->prmfns->data

static qse_htb_walk_t print_pair (
	qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	qse_awk_rtx_t* run = (qse_awk_rtx_t*)arg;

	QSE_ASSERT (run == *(qse_awk_rtx_t**)QSE_XTN(map));

	DPRINTF (DCUSTOM, QSE_T(" %.*s=>"),
		(int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair));
	qse_awk_dprintval ((qse_awk_rtx_t*)arg, QSE_HTB_VPTR(pair));
	DPRINTF (DCUSTOM, QSE_T(" "));

	return QSE_HTB_WALK_FORWARD;
}

void qse_awk_dprintval (qse_awk_rtx_t* run, qse_awk_val_t* val)
{
	/* TODO: better value printing ... */

	switch (val->type)
	{
		case QSE_AWK_VAL_NIL:
			DPRINTF (DCUSTOM, QSE_T("nil"));
		       	break;

		case QSE_AWK_VAL_INT:
		#if QSE_SIZEOF_LONG_LONG > 0
			DPRINTF (DCUSTOM, QSE_T("%lld"), 
				(long long)((qse_awk_val_int_t*)val)->val);
		#elif QSE_SIZEOF___INT64 > 0
			DPRINTF (DCUSTOM, QSE_T("%I64d"), 
				(__int64)((qse_awk_val_int_t*)val)->val);
		#elif QSE_SIZEOF_LONG > 0
			DPRINTF (DCUSTOM, QSE_T("%ld"), 
				(long)((qse_awk_val_int_t*)val)->val);
		#elif QSE_SIZEOF_INT > 0
			DPRINTF (DCUSTOM, QSE_T("%d"), 
				(int)((qse_awk_val_int_t*)val)->val);
		#else
			#error unsupported size
		#endif
			break;

		case QSE_AWK_VAL_FLT:
		#if defined(__MINGW32__)
			DPRINTF (DCUSTOM, QSE_T("%Lf"), 
				(double)((qse_awk_val_flt_t*)val)->val);
		#else
			DPRINTF (DCUSTOM, QSE_T("%Lf"), 
				(long double)((qse_awk_val_flt_t*)val)->val);
		#endif
			break;

		case QSE_AWK_VAL_STR:
			DPRINTF (DCUSTOM, QSE_T("%s"), ((qse_awk_val_str_t*)val)->ptr);
			break;

		case QSE_AWK_VAL_REX:
			DPRINTF (DCUSTOM, QSE_T("REX[%s]"), ((qse_awk_val_rex_t*)val)->ptr);
			break;

		case QSE_AWK_VAL_MAP:
			DPRINTF (DCUSTOM, QSE_T("MAP["));
			qse_htb_walk (((qse_awk_val_map_t*)val)->map, print_pair, run);
			DPRINTF (DCUSTOM, QSE_T("]"));
			break;
	
		case QSE_AWK_VAL_REF:
			DPRINTF (DCUSTOM, QSE_T("REF[id=%d,val="), ((qse_awk_val_ref_t*)val)->id);
			qse_awk_dprintval (run, *((qse_awk_val_ref_t*)val)->adr);
			DPRINTF (DCUSTOM, QSE_T("]"));
			break;

		default:
			DPRINTF (DCUSTOM, QSE_T("**** INTERNAL ERROR - INVALID VALUE TYPE ****\n"));
	}
}

#endif
