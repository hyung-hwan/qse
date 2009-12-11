/*
 * $Id: val.c 312 2009-12-10 13:03:54Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#ifdef DEBUG_VAL
#include <qse/cmn/stdio.h>
#endif

#define CHUNKSIZE QSE_AWK_VAL_CHUNK_SIZE

static qse_awk_val_nil_t awk_nil = { QSE_AWK_VAL_NIL, 0, 0 };
static qse_awk_val_str_t awk_zls = { QSE_AWK_VAL_STR, 0, 0, QSE_T(""), 0 };

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

qse_awk_val_t* qse_awk_rtx_makerealval (qse_awk_rtx_t* rtx, qse_real_t v)
{
	qse_awk_val_real_t* val;

	if (rtx->vmgr.rfree == QSE_NULL)
	{
		qse_awk_val_rchunk_t* c;
		/*qse_awk_val_real_t* x;*/
		qse_size_t i;

		/* c = QSE_AWK_ALLOC (run->awk, 
			QSE_SIZEOF(qse_awk_val_chunk_t)+
			QSE_SIZEOF(qse_awk_val_real_t)*CHUNKSIZE); */
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
		x = (qse_awk_val_real_t*)(c + 1);
		for (i = 0; i < CHUNKSIZE-1; i++) 
			x[i].nde = (qse_awk_nde_real_t*)&x[i+1];
		x[i].nde = QSE_NULL;

		run->vmgr.rfree = x;
		*/

		for (i = 0; i < CHUNKSIZE-1; i++)
			c->slot[i].nde = (qse_awk_nde_real_t*)&c->slot[i+1];
		c->slot[i].nde = QSE_NULL;

		rtx->vmgr.rfree = &c->slot[0];
	}

	val = rtx->vmgr.rfree;
	rtx->vmgr.rfree = (qse_awk_val_real_t*)val->nde;

	val->type = QSE_AWK_VAL_REAL;
	val->ref = 0;
	val->nstr = 0;
	val->val = v;
	val->nde = QSE_NULL;

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makerealval => %Lf [%p]\n"), (double)v, val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makestrval0 (
	qse_awk_rtx_t* rtx, const qse_char_t* str)
{
	return qse_awk_rtx_makestrval (rtx, str, qse_strlen(str));
}

qse_awk_val_t* qse_awk_rtx_makestrval (
	qse_awk_rtx_t* rtx, const qse_char_t* str, qse_size_t len)
{
	qse_awk_val_str_t* val = QSE_NULL;
	qse_size_t rlen = len;

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
	val->len = len;
	val->ptr = (qse_char_t*)(val + 1);
	qse_strncpy (val->ptr, str, len);

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makestrval => %p\n"), val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makestrval_nodup (
	qse_awk_rtx_t* rtx, qse_char_t* str, qse_size_t len)
{
	qse_awk_val_str_t* val;

	val = (qse_awk_val_str_t*) QSE_AWK_ALLOC (
		rtx->awk, QSE_SIZEOF(qse_awk_val_str_t));
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	val->type = QSE_AWK_VAL_STR;
	val->ref = 0;
	val->nstr = 0;
	val->len = len;
	val->ptr = str;
	return (qse_awk_val_t*)val;
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
	val->len = len1 + len2;
	val->ptr = (qse_char_t*)(val + 1);
	qse_strncpy (val->ptr, str1, len1);
	qse_strncpy (&val->ptr[len1], str2, len2);

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makestrval2 => %p\n"), val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makenstrval (
	qse_awk_rtx_t* rtx, const qse_char_t* str, qse_size_t len)
{
	int x;
	qse_awk_val_t* v;
	qse_long_t l;
	qse_real_t r;

	x = qse_awk_rtx_strtonum (rtx, 1, str, len, &l, &r);
	v = qse_awk_rtx_makestrval (rtx, str, len);

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

static void free_mapval (qse_map_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_t* rtx = *(qse_awk_rtx_t**)QSE_XTN(map);

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("refdown in map free..."));
	qse_awk_dprintval (rtx, dptr);
	qse_dprintf (QSE_T("\n"));
#endif

	qse_awk_rtx_refdownval (rtx, dptr);
}

static void same_mapval (qse_map_t* map, void* dptr, qse_size_t dlen)
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
	val->map = qse_map_open (
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
		QSE_SIZEOF(qse_map_t) +
		QSE_SIZEOF(rtx));
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	val->type = QSE_AWK_VAL_MAP;
	val->ref = 0;
	val->nstr = 0;
	val->map = (qse_map_t*)(val + 1);

	val->map = qse_map_init (val->map, rtx->awk->mmgr, 256, 70);
	if (val->map == QSE_NULL)
	{
		QSE_AWK_FREE (rtx->awk, val);
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}
	*(qse_awk_rtx_t**)QSE_XTN(val->map) = rtx;

	/* the key is copied inline into a pair and is freed when the pair
	 * is destroyed */
	qse_map_setcopier (val->map, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setscale (val->map, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	/* not setting copier for a value means that the pointer to the data 
	 * allocated somewhere else is remembered in a pair. but the freeing 
	 * the actual value is handled by free_mapval and same_mapval */
	qse_map_setfreeer (val->map, QSE_MAP_VAL, free_mapval);
	qse_map_setkeeper (val->map, same_mapval);
	/* END CHECK */

	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makerefval (
	qse_awk_rtx_t* rtx, int id, qse_awk_val_t** adr)
{
	qse_awk_val_ref_t* val;

	if (rtx->fcache_count > 0)
	{
		val = rtx->fcache[--rtx->fcache_count];
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

qse_bool_t qse_awk_rtx_isstaticval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	return IS_STATICVAL(val);
}

void qse_awk_rtx_freeval (	
	qse_awk_rtx_t* rtx, qse_awk_val_t* val, qse_bool_t cache)
{
	if (IS_STATICVAL(val)) return;

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("freeing [cache=%d] ... "), cache);
	qse_awk_dprintval (rtx, val);
	qse_dprintf (QSE_T("\n"));
#endif

	if (val->type == QSE_AWK_VAL_NIL)
	{
		QSE_AWK_FREE (rtx->awk, val);
	}
	else if (val->type == QSE_AWK_VAL_INT)
	{
		((qse_awk_val_int_t*)val)->nde = 
			(qse_awk_nde_int_t*)rtx->vmgr.ifree;
		rtx->vmgr.ifree = (qse_awk_val_int_t*)val;
	}
	else if (val->type == QSE_AWK_VAL_REAL)
	{
		((qse_awk_val_real_t*)val)->nde =
			(qse_awk_nde_real_t*)rtx->vmgr.rfree;
		rtx->vmgr.rfree = (qse_awk_val_real_t*)val;
	}
	else if (val->type == QSE_AWK_VAL_STR)
	{
#ifdef ENABLE_FEATURE_SCACHE
		if (cache)
		{
			qse_awk_val_str_t* v = (qse_awk_val_str_t*)val;
			int i;

			i = v->len / FEATURE_SCACHE_BLOCK_UNIT;
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
	}
	else if (val->type == QSE_AWK_VAL_REX)
	{
		/* don't free ptr as it is inlined to val
		QSE_AWK_FREE (rtx->awk, ((qse_awk_val_rex_t*)val)->ptr);
		 */
	
		/* code is just a pointer to a regular expression stored
		 * in parse tree nodes. so don't free it.
		QSE_AWK_FREEREX (rtx->awk, ((qse_awk_val_rex_t*)val)->code);
		 */

		QSE_AWK_FREE (rtx->awk, val);
	}
	else if (val->type == QSE_AWK_VAL_MAP)
	{
		qse_map_fini (((qse_awk_val_map_t*)val)->map);
		QSE_AWK_FREE (rtx->awk, val);
	}
	else if (val->type == QSE_AWK_VAL_REF)
	{
		if (cache && rtx->fcache_count < QSE_COUNTOF(rtx->fcache))
		{
			rtx->fcache[rtx->fcache_count++] = 
				(qse_awk_val_ref_t*)val;	
		}
		else QSE_AWK_FREE (rtx->awk, val);
	}
	else
	{
		QSE_ASSERTX (
			!"should never happen - invalid value type",
			"the type of a value should be one of QSE_AWK_VAL_XXX's defined in awk.h");
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
		qse_awk_rtx_freeval(rtx, val, QSE_TRUE);
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

qse_bool_t qse_awk_rtx_valtobool (qse_awk_rtx_t* run, qse_awk_val_t* val)
{
	if (val == QSE_NULL) return QSE_FALSE;

	switch (val->type)
	{
		case QSE_AWK_VAL_NIL:
			return QSE_FALSE;
		case QSE_AWK_VAL_INT:
			return ((qse_awk_val_int_t*)val)->val != 0;
		case QSE_AWK_VAL_REAL:
			return ((qse_awk_val_real_t*)val)->val != 0.0;
		case QSE_AWK_VAL_STR:
			return ((qse_awk_val_str_t*)val)->len > 0;
		case QSE_AWK_VAL_REX: /* TODO: is this correct? */
			return ((qse_awk_val_rex_t*)val)->len > 0;
		case QSE_AWK_VAL_MAP:
			return QSE_FALSE; /* TODO: is this correct? */
		case QSE_AWK_VAL_REF:
			return QSE_FALSE; /* TODO: is this correct? */
	}

	QSE_ASSERTX (
		!"should never happen - invalid value type",
		"the type of a value should be one of QSE_AWK_VAL_XXX's defined in awk.h");
	return QSE_FALSE;
}

static qse_char_t* str_to_str (
	qse_awk_rtx_t* rtx, const qse_char_t* str, qse_size_t str_len,
	qse_awk_rtx_valtostr_out_t* out)
{
	int type = out->type & ~QSE_AWK_RTX_VALTOSTR_PRINT;

	switch (type)
	{
		case QSE_AWK_RTX_VALTOSTR_CPL:
		{
			if (str_len >= out->u.cpl.len)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
				out->u.cpl.len = str_len + 1;
				return QSE_NULL;
			}

			out->u.cpl.len = 
				qse_strncpy (out->u.cpl.ptr, str, str_len);
			return out->u.cpl.ptr;
		}

		case QSE_AWK_RTX_VALTOSTR_CPLDUP:
		{
			qse_char_t* tmp;

			tmp = QSE_AWK_STRXDUP (rtx->awk, str, str_len);
			if (tmp == QSE_NULL) 
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return QSE_NULL;
			}

			out->u.cpldup.ptr = tmp;
			out->u.cpldup.len = str_len;
			return tmp;
		}
			
		case QSE_AWK_RTX_VALTOSTR_STRP:
		{
			qse_size_t n;

			qse_str_clear (out->u.strp);
			n = qse_str_ncat (out->u.strp, str, str_len);
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return QSE_NULL;
			}

			return QSE_STR_PTR(out->u.strp);
		}

		case QSE_AWK_RTX_VALTOSTR_STRPCAT:
		{
			qse_size_t n;

			n = qse_str_ncat (out->u.strpcat, str, str_len);
			if (n == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return QSE_NULL;
			}

			return QSE_STR_PTR(out->u.strpcat);
		}
	}

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
	return QSE_NULL;
}

static qse_char_t* val_int_to_str (
	qse_awk_rtx_t* rtx, qse_awk_val_int_t* v,
	qse_awk_rtx_valtostr_out_t* out)
{
	qse_char_t* tmp;
	qse_long_t t;
	qse_size_t rlen = 0;
	int type = out->type & ~QSE_AWK_RTX_VALTOSTR_PRINT;

	t = v->val; 
	if (t == 0)
	{
		/* handle zero */
		switch (type)
		{
			case QSE_AWK_RTX_VALTOSTR_CPL:
				if (out->u.cpl.len <= 1)
				{
					qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
					/* store the buffer size needed */
					out->u.cpl.len = 2; 
					return QSE_NULL;
				}

				out->u.cpl.len = 1; /* actual length */
				out->u.cpl.ptr[0] = QSE_T('0');
				out->u.cpl.ptr[1] = QSE_T('\0');
				return out->u.cpl.ptr;

			case QSE_AWK_RTX_VALTOSTR_CPLDUP:
				tmp = QSE_AWK_ALLOC (
					rtx->awk, 2 * QSE_SIZEOF(qse_char_t));
				if (tmp == QSE_NULL)
				{
					qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
					return QSE_NULL;
				}

				tmp[0] = QSE_T('0');
				tmp[1] = QSE_T('\0');
				
				out->u.cpldup.ptr = tmp;
				out->u.cpldup.len = 1; /* actual length */
				return out->u.cpldup.ptr;

			case QSE_AWK_RTX_VALTOSTR_STRP:
				qse_str_clear (out->u.strp);
				if (qse_str_ccat (out->u.strp, QSE_T('0')) == (qse_size_t)-1)
				{
					qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
					return QSE_NULL;
				}
				return QSE_STR_PTR(out->u.strp);

			case QSE_AWK_RTX_VALTOSTR_STRPCAT:
				if (qse_str_ccat (out->u.strpcat, QSE_T('0')) == (qse_size_t)-1)
				{
					qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
					return QSE_NULL;
				}
				return QSE_STR_PTR(out->u.strpcat);
		}

		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	/* non-zero values */
	if (t < 0) { t = -t; rlen++; }
	while (t > 0) { rlen++; t /= 10; }

	switch (type)
	{
		case QSE_AWK_RTX_VALTOSTR_CPL:
			if (rlen >= out->u.cpl.len)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
				/* store the buffer size needed */
				out->u.cpl.len = rlen + 1; 
				return QSE_NULL;
			}

			tmp = out->u.cpl.ptr;
			tmp[rlen] = QSE_T('\0');
			out->u.cpl.len = rlen;
			break;

		case QSE_AWK_RTX_VALTOSTR_CPLDUP:
			tmp = QSE_AWK_ALLOC (
				rtx->awk, (rlen + 1) * QSE_SIZEOF(qse_char_t));
			if (tmp == QSE_NULL)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return QSE_NULL;
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
				return QSE_NULL;
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
				return QSE_NULL;
			}
			break;
		}
	}

	t = v->val; 
	if (t < 0) t = -t;

	/* fill in the buffer with digits */
	while (t > 0) 
	{
		tmp[--rlen] = (qse_char_t)(t % 10) + QSE_T('0');
		t /= 10;
	}

	/* insert the negative sign if necessary */
	if (v->val < 0) tmp[--rlen] = QSE_T('-');

	if (type == QSE_AWK_RTX_VALTOSTR_STRPCAT)
	{
		/* for concatenation type, change tmp to
		 * point to the buffer */
		tmp = QSE_STR_PTR(out->u.strpcat);
	}

	return tmp;
}

static qse_char_t* val_real_to_str (
	qse_awk_rtx_t* rtx, qse_awk_val_real_t* v,
	qse_awk_rtx_valtostr_out_t* out)
{
	qse_char_t* tmp;
	qse_size_t tmp_len;
	qse_str_t buf, fbu;
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

	if (qse_str_init (&buf, rtx->awk->mmgr, 256) == QSE_NULL)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	if (qse_str_init (&fbu, rtx->awk->mmgr, 256) == QSE_NULL)
	{
		qse_str_fini (&buf);
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	tmp = qse_awk_rtx_format (rtx, &buf, &fbu, tmp, tmp_len, 
		(qse_size_t)-1, (qse_awk_nde_t*)v, &tmp_len);
	if (tmp == QSE_NULL) 
	{
		qse_str_fini (&fbu);
		qse_str_fini (&buf);
		return QSE_NULL;
	}

	switch (type)
	{
		case QSE_AWK_RTX_VALTOSTR_CPL:

			if (out->u.cpl.len <= tmp_len)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
				/* store the buffer size required */
				out->u.cpl.len = tmp_len + 1; 
				qse_str_close (&fbu);
				qse_str_close (&buf);
				return QSE_NULL;
			}

			qse_strncpy (out->u.cpl.ptr, tmp, tmp_len);
			out->u.cpl.len = tmp_len;
			tmp = out->u.cpl.ptr;

			qse_str_fini (&fbu);
			qse_str_fini (&buf);
			return tmp;

		case QSE_AWK_RTX_VALTOSTR_CPLDUP:
			qse_str_fini (&fbu);

			qse_str_yield (&buf, QSE_NULL, 0);
			qse_str_fini (&buf);

			out->u.cpldup.ptr = tmp;
			out->u.cpldup.len = tmp_len;
			return tmp;

		case QSE_AWK_RTX_VALTOSTR_STRP:
		{
			qse_size_t n;

			qse_str_clear (out->u.strp);

			n = qse_str_ncat (out->u.strp, tmp, tmp_len);
			if (n == (qse_size_t)-1)
			{
				qse_str_fini (&fbu);
				qse_str_fini (&buf);
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return QSE_NULL;
			}

			tmp = QSE_STR_PTR(out->u.strp);

			qse_str_fini (&fbu);
			qse_str_fini (&buf);
			return tmp;
		}

		case QSE_AWK_RTX_VALTOSTR_STRPCAT:
		{
			qse_size_t n;
	
			n = qse_str_ncat (out->u.strpcat, tmp, tmp_len);
			if (n == (qse_size_t)-1)
			{
				qse_str_fini (&fbu);
				qse_str_fini (&buf);
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return QSE_NULL;
			}

			tmp = QSE_STR_PTR(out->u.strpcat);

			qse_str_fini (&fbu);
			qse_str_fini (&buf);
			return tmp;
		}
	}

	qse_str_fini (&fbu);
	qse_str_fini (&buf);
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
	return QSE_NULL;
}

qse_char_t* qse_awk_rtx_valtostr (
	qse_awk_rtx_t* rtx, qse_awk_val_t* v, qse_awk_rtx_valtostr_out_t* out)
{
	switch (v->type)
	{
		case QSE_AWK_VAL_NIL:
			return str_to_str (rtx, QSE_T(""), 0, out);

		case QSE_AWK_VAL_INT:
			return val_int_to_str (
				rtx, (qse_awk_val_int_t*)v, out);

		case QSE_AWK_VAL_REAL:
			return val_real_to_str (
				rtx, (qse_awk_val_real_t*)v, out);

		case QSE_AWK_VAL_STR:
		{
			qse_awk_val_str_t* vs = (qse_awk_val_str_t*)v;
			return str_to_str (rtx, vs->ptr, vs->len, out);
		}
	}

#ifdef DEBUG_VAL
	qse_dprintf (
		QSE_T("ERROR: WRONG VALUE TYPE [%d] in qse_awk_rtx_valtostr\n"), 
		v->type);
#endif

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EVALTYPE, QSE_NULL);
	return QSE_NULL;
}

qse_char_t* qse_awk_rtx_valtocpldup (
	qse_awk_rtx_t* rtx, qse_awk_val_t* v, qse_size_t* len)
{
	qse_awk_rtx_valtostr_out_t out;

	out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
	if (qse_awk_rtx_valtostr (rtx, v, &out) == QSE_NULL) return QSE_NULL;

	*len = out.u.cpldup.len;
	return out.u.cpldup.ptr;
}

int qse_awk_rtx_valtonum (
	qse_awk_rtx_t* rtx, qse_awk_val_t* v, qse_long_t* l, qse_real_t* r)
{
	if (v->type == QSE_AWK_VAL_NIL) 
	{
		*l = 0;
		return 0;
	}

	if (v->type == QSE_AWK_VAL_INT)
	{
		*l = ((qse_awk_val_int_t*)v)->val;
		return 0; /* long */
	}

	if (v->type == QSE_AWK_VAL_REAL)
	{
		*r = ((qse_awk_val_real_t*)v)->val;
		return 1; /* real */
	}

	if (v->type == QSE_AWK_VAL_STR)
	{
		return qse_awk_rtx_strtonum (
			rtx, 0,
			((qse_awk_val_str_t*)v)->ptr, 
			((qse_awk_val_str_t*)v)->len, 
			l, r
		);
	}

#ifdef DEBUG_VAL
	qse_dprintf (
		QSE_T("ERROR: WRONG VALUE TYPE [%d] in qse_awk_rtx_valtonum\n"),
		v->type);
#endif

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EVALTYPE, QSE_NULL);
	return -1; /* error */
}

int qse_awk_rtx_strtonum (
	qse_awk_rtx_t* rtx, int strict,
	const qse_char_t* ptr, qse_size_t len, 
	qse_long_t* l, qse_real_t* r)
{
	const qse_char_t* endptr;

	*l = qse_awk_strxtolong (rtx->awk, ptr, len, 0, &endptr);
	if (endptr < ptr + len &&
	    (*endptr == QSE_T('.') ||
	     *endptr == QSE_T('E') ||
	     *endptr == QSE_T('e')))
	{
		*r = qse_awk_strxtoreal (rtx->awk, ptr, len, &endptr);
		if (strict && endptr < ptr + len) return -1;
		return 1; /* real */
	}

	if (strict && endptr < ptr + len) return -1;
	return 0; /* long */
}

#if 0

#define DPRINTF run->awk->prmfns->dprintf
#define DCUSTOM run->awk->prmfns->data

static qse_map_walk_t print_pair (
	qse_map_t* map, qse_map_pair_t* pair, void* arg)
{
	qse_awk_rtx_t* run = (qse_awk_rtx_t*)arg;

	QSE_ASSERT (run == *(qse_awk_rtx_t**)QSE_XTN(map));

	DPRINTF (DCUSTOM, QSE_T(" %.*s=>"),
		(int)QSE_MAP_KLEN(pair), QSE_MAP_KPTR(pair));
	qse_awk_dprintval ((qse_awk_rtx_t*)arg, QSE_MAP_VPTR(pair));
	DPRINTF (DCUSTOM, QSE_T(" "));

	return QSE_MAP_WALK_FORWARD;
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

		case QSE_AWK_VAL_REAL:
		#if defined(__MINGW32__)
			DPRINTF (DCUSTOM, QSE_T("%Lf"), 
				(double)((qse_awk_val_real_t*)val)->val);
		#else
			DPRINTF (DCUSTOM, QSE_T("%Lf"), 
				(long double)((qse_awk_val_real_t*)val)->val);
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
			qse_map_walk (((qse_awk_val_map_t*)val)->map, print_pair, run);
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
