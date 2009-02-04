/*
 * $Id: val.c 496 2008-12-15 09:56:48Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "awk.h"

#ifdef DEBUG_VAL
#include <qse/utl/stdio.h>
#endif

#define CHUNKSIZE 100

typedef struct qse_awk_val_ichunk_t qse_awk_val_ichunk_t;
typedef struct qse_awk_val_rchunk_t qse_awk_val_rchunk_t;

struct qse_awk_val_chunk_t
{
        qse_awk_val_chunk_t* next;
};

struct qse_awk_val_ichunk_t
{
	qse_awk_val_chunk_t* next;
	/* make sure that it has the same fields as 
	   qse_awk_val_chunk_t up to this point */

	qse_awk_val_int_t slot[CHUNKSIZE];
};

struct qse_awk_val_rchunk_t
{
	qse_awk_val_chunk_t* next;
	/* make sure that it has the same fields as 
	   qse_awk_val_chunk_t up to this point */

	qse_awk_val_real_t slot[CHUNKSIZE];
};

static qse_char_t* str_to_str (
	qse_awk_rtx_t* run, const qse_char_t* str, qse_size_t str_len,
	int opt, qse_str_t* buf, qse_size_t* len);

static qse_char_t* val_int_to_str (
	qse_awk_rtx_t* run, qse_awk_val_int_t* v,
	int opt, qse_str_t* buf, qse_size_t* len);

static qse_char_t* val_real_to_str (
	qse_awk_rtx_t* run, qse_awk_val_real_t* v,
	int opt, qse_str_t* buf, qse_size_t* len);

static qse_awk_val_nil_t awk_nil = { QSE_AWK_VAL_NIL, 0 };
static qse_awk_val_str_t awk_zls = { QSE_AWK_VAL_STR, 0, QSE_T(""), 0 };

qse_awk_val_t* qse_awk_val_nil = (qse_awk_val_t*)&awk_nil;
qse_awk_val_t* qse_awk_val_zls = (qse_awk_val_t*)&awk_zls; 

static qse_awk_val_int_t awk_int[] =
{
	{ QSE_AWK_VAL_INT, 0, -1, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  0, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  1, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  2, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  3, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  4, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  5, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  6, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  7, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  8, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0,  9, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 10, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 11, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 12, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 13, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 14, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 15, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 16, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 17, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 18, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 19, QSE_NULL },
	{ QSE_AWK_VAL_INT, 0, 20, QSE_NULL }
};

qse_awk_val_t* qse_awk_val_negone = (qse_awk_val_t*)&awk_int[0];
qse_awk_val_t* qse_awk_val_zero = (qse_awk_val_t*)&awk_int[1];
qse_awk_val_t* qse_awk_val_one = (qse_awk_val_t*)&awk_int[2];

qse_awk_val_t* qse_awk_rtx_makeintval (qse_awk_rtx_t* run, qse_long_t v)
{
	qse_awk_val_int_t* val;

	if (v >= awk_int[0].val && 
	    v <= awk_int[QSE_COUNTOF(awk_int)-1].val)
	{
		return (qse_awk_val_t*)&awk_int[v-awk_int[0].val];
	}

	/*
	if (run->icache_count > 0)
	{
		val = run->icache[--run->icache_count];
	}
	else
	{
		val = (qse_awk_val_int_t*) QSE_AWK_ALLOC (
			run->awk, QSE_SIZEOF(qse_awk_val_int_t));
		if (val == QSE_NULL) 
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
			return QSE_NULL;
		}
	}
	*/

	if (run->vmgr.ifree == QSE_NULL)
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
		c = QSE_AWK_ALLOC (run->awk, QSE_SIZEOF(qse_awk_val_ichunk_t));
		if (c == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
			return QSE_NULL;
		}

		c->next = run->vmgr.ichunk;
		/*run->vmgr.ichunk = c;*/
		run->vmgr.ichunk = (qse_awk_val_chunk_t*)c;

		/*x = (qse_awk_val_int_t*)(c + 1);
		for (i = 0; i < CHUNKSIZE-1; i++) 
			x[i].nde = (qse_awk_nde_int_t*)&x[i+1];
		x[i].nde = QSE_NULL;

		run->vmgr.ifree = x;
		*/

		for (i = 0; i < CHUNKSIZE-1; i++)
			c->slot[i].nde = (qse_awk_nde_int_t*)&c->slot[i+1];
		c->slot[i].nde = QSE_NULL;

		run->vmgr.ifree = &c->slot[0];
	}

	val = run->vmgr.ifree;
	run->vmgr.ifree = (qse_awk_val_int_t*)val->nde;

	val->type = QSE_AWK_VAL_INT;
	val->ref = 0;
	val->val = v;
	val->nde = QSE_NULL;

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makeintval => %ld [%p]\n"), (long)v, val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makerealval (qse_awk_rtx_t* run, qse_real_t v)
{
	qse_awk_val_real_t* val;

	/*
	if (run->rcache_count > 0)
	{
		val = run->rcache[--run->rcache_count];
	}
	else
	{
		val = (qse_awk_val_real_t*) QSE_AWK_ALLOC (
			run->awk, QSE_SIZEOF(qse_awk_val_real_t));
		if (val == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
			return QSE_NULL;
		}
	}
	*/

	if (run->vmgr.rfree == QSE_NULL)
	{
		qse_awk_val_rchunk_t* c;
		/*qse_awk_val_real_t* x;*/
		qse_size_t i;

		/* c = QSE_AWK_ALLOC (run->awk, 
			QSE_SIZEOF(qse_awk_val_chunk_t)+
			QSE_SIZEOF(qse_awk_val_real_t)*CHUNKSIZE); */
		c = QSE_AWK_ALLOC (run->awk, QSE_SIZEOF(qse_awk_val_rchunk_t));
		if (c == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
			return QSE_NULL;
		}

		c->next = run->vmgr.rchunk;
		/*run->vmgr.rchunk = c;*/
		run->vmgr.rchunk = (qse_awk_val_chunk_t*)c;

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

		run->vmgr.rfree = &c->slot[0];
	}

	val = run->vmgr.rfree;
	run->vmgr.rfree = (qse_awk_val_real_t*)val->nde;

	val->type = QSE_AWK_VAL_REAL;
	val->ref = 0;
	val->val = v;
	val->nde = QSE_NULL;

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makerealval => %Lf [%p]\n"), (double)v, val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makestrval0 (qse_awk_rtx_t* run, const qse_char_t* str)
{
	return qse_awk_rtx_makestrval (run, str, qse_strlen(str));
}

qse_awk_val_t* qse_awk_rtx_makestrval (
	qse_awk_rtx_t* run, const qse_char_t* str, qse_size_t len)
{
	qse_awk_val_str_t* val;
	qse_size_t rlen = len;

	/*if (rlen <= 32)
	{
		if (run->scache32_count > 0)
		{
			val = run->scache32[--run->scache32_count];
			goto init;
		}
		rlen = 32;
	}
	else if (rlen <= 64)
	{
		if (run->scache64_count > 0)
		{
			val = run->scache64[--run->scache64_count];
			goto init;
		}
		rlen = 64;
	}*/
	
	val = (qse_awk_val_str_t*) QSE_AWK_ALLOC (
		run->awk, 
		QSE_SIZEOF(qse_awk_val_str_t) + 
		(rlen+1)*QSE_SIZEOF(qse_char_t));
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}
/*
init:
*/
	val->type = QSE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len;
	val->ptr = (qse_char_t*)(val + 1);
	/*qse_strxncpy (val->ptr, len+1, str, len);*/
	qse_strncpy (val->ptr, str, len);

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makestrval => %p\n"), val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makestrval_nodup (
	qse_awk_rtx_t* run, qse_char_t* str, qse_size_t len)
{
	qse_awk_val_str_t* val;

	val = (qse_awk_val_str_t*) QSE_AWK_ALLOC (
		run->awk, QSE_SIZEOF(qse_awk_val_str_t));
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}

	val->type = QSE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len;
	val->ptr = str;
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makestrval2 (
	qse_awk_rtx_t* run,
	const qse_char_t* str1, qse_size_t len1, 
	const qse_char_t* str2, qse_size_t len2)
{
	qse_awk_val_str_t* val;
	qse_size_t rlen = len1 + len2;

	/*if (rlen <= 32)
	{
		if (run->scache32_count > 0)
		{
			val = run->scache32[--run->scache32_count];
			goto init;
		}
		rlen = 32;
	}
	else if (rlen <= 64)
	{
		if (run->scache64_count > 0)
		{
			val = run->scache64[--run->scache64_count];
			goto init;
		}
		rlen = 64;
	}*/

	val = (qse_awk_val_str_t*) QSE_AWK_ALLOC (
		run->awk, 
		QSE_SIZEOF(qse_awk_val_str_t) +
		(rlen+1)*QSE_SIZEOF(qse_char_t));
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}

/*
init:
*/
	val->type = QSE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len1 + len2;
	val->ptr = (qse_char_t*)(val + 1);
	/*qse_strxncpy (val->ptr, len1+1, str1, len1);
	qse_strxncpy (val->ptr[len1], len2+1, str2, len2);*/
	qse_strncpy (val->ptr, str1, len1);
	qse_strncpy (&val->ptr[len1], str2, len2);

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("makestrval2 => %p\n"), val);
#endif
	return (qse_awk_val_t*)val;
}

qse_awk_val_t* qse_awk_rtx_makerexval (
	qse_awk_rtx_t* run, const qse_char_t* buf, qse_size_t len, void* code)
{
	qse_awk_val_rex_t* val;

	val = (qse_awk_val_rex_t*) QSE_AWK_ALLOC (
		run->awk, QSE_SIZEOF(qse_awk_val_rex_t) + 
		(QSE_SIZEOF(*buf)*len+1) + QSE_REX_LEN(code));
	if (val == QSE_NULL) return QSE_NULL;

	val->type = QSE_AWK_VAL_REX;
	val->ref = 0;
	val->len = len;
	/*
	val->ptr = QSE_AWK_STRXDUP (run->awk, buf, len);
	if (val->ptr == QSE_NULL) 
	{
		QSE_AWK_FREE (run->awk, val);
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}*/
	val->ptr = (qse_char_t*)(val + 1);
	qse_strncpy (val->ptr, buf, len);

	/*
	val->code = QSE_AWK_ALLOC (run->awk, QSE_REX_LEN(code));
	if (val->code == QSE_NULL)
	{
		QSE_AWK_FREE (run->awk, val->ptr);
		QSE_AWK_FREE (run->awk, val);
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}
	*/
	val->code = val->ptr + len + 1;
	QSE_MEMCPY (val->code, code, QSE_REX_LEN(code));

	return (qse_awk_val_t*)val;
}

/* CHECK */
/*
static void free_mapval (void* run, void* v)
{
#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("refdown in map free..."));
	qse_awk_dprintval (run, v);
	qse_dprintf (QSE_T("\n"));
#endif

	qse_awk_rtx_refdownval (run, v);
}

static void same_mapval (void* run, void* v)
{
#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("refdown nofree in map free..."));
	qse_awk_dprintval (run, v);
	qse_dprintf (QSE_T("\n"));
#endif
	qse_awk_rtx_refdownval_nofree (run, v);
}
*/
static void free_mapval (qse_map_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_t* run = *(qse_awk_rtx_t**)qse_map_getxtn(map);

#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("refdown in map free..."));
	qse_awk_dprintval (run, dptr);
	qse_dprintf (QSE_T("\n"));
#endif

	qse_awk_rtx_refdownval (run, dptr);
}

static void same_mapval (qse_map_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_t* run = *(qse_awk_rtx_t**)qse_map_getxtn(map);
#ifdef DEBUG_VAL
	qse_dprintf (QSE_T("refdown nofree in map free..."));
	qse_awk_dprintval (run, dptr);
	qse_dprintf (QSE_T("\n"));
#endif
	qse_awk_rtx_refdownval_nofree (run, dptr);
}
/* END CHECK */

qse_awk_val_t* qse_awk_rtx_makemapval (qse_awk_rtx_t* run)
{
	qse_awk_val_map_t* val;

	/* CHECK */
	/* 
	val = (qse_awk_val_map_t*) QSE_AWK_ALLOC (
		run->awk, QSE_SIZEOF(qse_awk_val_map_t) );
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}

	val->type = QSE_AWK_VAL_MAP;
	val->ref = 0;
	val->map = qse_map_open (
		run, 256, 70, free_mapval, same_mapval, run->awk->mmgr);
	if (val->map == QSE_NULL)
	{
		QSE_AWK_FREE (run->awk, val);
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}
	*/

	val = (qse_awk_val_map_t*) QSE_AWK_ALLOC (
		run->awk, 
		QSE_SIZEOF(qse_awk_val_map_t) +
		QSE_SIZEOF(qse_map_t) +
		QSE_SIZEOF(run));
	if (val == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}

	val->type = QSE_AWK_VAL_MAP;
	val->ref = 0;
	val->map = (qse_map_t*)(val + 1);

	val->map = qse_map_init (val->map, run->awk->mmgr, 256, 70);
	if (val->map == QSE_NULL)
	{
		QSE_AWK_FREE (run->awk, val);
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}
	*(qse_awk_rtx_t**)qse_map_getxtn(val->map) = run;

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

qse_awk_val_t* qse_awk_rtx_makerefval (qse_awk_rtx_t* run, int id, qse_awk_val_t** adr)
{
	qse_awk_val_ref_t* val;

	if (run->fcache_count > 0)
	{
		val = run->fcache[--run->fcache_count];
	}
	else
	{
		val = (qse_awk_val_ref_t*) QSE_AWK_ALLOC (
			run->awk, QSE_SIZEOF(qse_awk_val_ref_t));
		if (val == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM);
			return QSE_NULL;
		}
	}

	val->type = QSE_AWK_VAL_REF;
	val->ref = 0;
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

qse_bool_t qse_awk_isstaticval (qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	return IS_STATICVAL(val);
}

void qse_awk_rtx_freeval (qse_awk_rtx_t* rtx, qse_awk_val_t* val, qse_bool_t cache)
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
		/*
		if (cache && rtx->icache_count < QSE_COUNTOF(rtx->icache))
		{
			rtx->icache[rtx->icache_count++] = 
				(qse_awk_val_int_t*)val;	
		}
		else QSE_AWK_FREE (rtx->awk, val);
		*/
			
		((qse_awk_val_int_t*)val)->nde = (qse_awk_nde_int_t*)rtx->vmgr.ifree;
		rtx->vmgr.ifree = (qse_awk_val_int_t*)val;
	}
	else if (val->type == QSE_AWK_VAL_REAL)
	{
		/*
		if (cache && rtx->rcache_count < QSE_COUNTOF(rtx->rcache))
		{
			rtx->rcache[rtx->rcache_count++] = 
				(qse_awk_val_real_t*)val;	
		}
		else QSE_AWK_FREE (rtx->awk, val);
		*/
		((qse_awk_val_real_t*)val)->nde = (qse_awk_nde_real_t*)rtx->vmgr.rfree;
		rtx->vmgr.rfree = (qse_awk_val_real_t*)val;
	}
	else if (val->type == QSE_AWK_VAL_STR)
	{
		/*
		if (cache)
		{
			qse_awk_val_str_t* v = (qse_awk_val_str_t*)val;
			if (v->len <= 32 && 
			    rtx->scache32_count<QSE_COUNTOF(rtx->scache32))
			{
				rtx->scache32[rtx->scache32_count++] = v;
			}
			else if (v->len <= 64 && 
			         rtx->scache64_count<QSE_COUNTOF(rtx->scache64))
			{
				rtx->scache64[rtx->scache64_count++] = v;
			}
			else QSE_AWK_FREE (rtx->awk, val);
		}
		else*/ QSE_AWK_FREE (rtx->awk, val);
	}
	else if (val->type == QSE_AWK_VAL_REX)
	{
		/*
		QSE_AWK_FREE (rtx->awk, ((qse_awk_val_rex_t*)val)->ptr);
		QSE_AWK_FREEREX (rtx->awk, ((qse_awk_val_rex_t*)val)->code);
		*/
		QSE_AWK_FREE (rtx->awk, val);
	}
	else if (val->type == QSE_AWK_VAL_MAP)
	{
		/* CHECK */
		/* qse_map_close (((qse_awk_val_map_t*)val)->map);*/
		qse_map_fini (((qse_awk_val_map_t*)val)->map);
		/* END CHECK */
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
		"the reference count of a value should be greater than zero for it to be decremented. check the source code for any bugs");

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
		"the reference count of a value should be greater than zero for it to be decremented. check the source code for any bugs");
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

qse_char_t* qse_awk_rtx_valtostr (
	qse_awk_rtx_t* run, qse_awk_val_t* v,
	int opt, qse_str_t* buf, qse_size_t* len)
{
	if (v->type == QSE_AWK_VAL_NIL)
	{
		return str_to_str (run, QSE_T(""), 0, opt, buf, len);
	}

	if (v->type == QSE_AWK_VAL_INT)
	{
		qse_awk_val_int_t* vi = (qse_awk_val_int_t*)v;

		/*
		if (vi->nde != QSE_NULL && vi->nde->str != QSE_NULL)
		{
			return str_to_str (
				run, vi->nde->str, vi->nde->len, 
				opt, buf, len);
		}
		else
		{*/
			return val_int_to_str (run, vi, opt, buf, len);
		/*}*/
	}

	if (v->type == QSE_AWK_VAL_REAL)
	{
		qse_awk_val_real_t* vr = (qse_awk_val_real_t*)v;

		/*
		if (vr->nde != QSE_NULL && vr->nde->str != QSE_NULL)
		{
			return str_to_str (
				run, vr->nde->str, vr->nde->len, 
				opt, buf, len);
		}
		else
		{*/
			return val_real_to_str (run, vr, opt, buf, len);
		/*}*/
	}

	if (v->type == QSE_AWK_VAL_STR) 
	{
		qse_awk_val_str_t* vs = (qse_awk_val_str_t*)v;

		return str_to_str (
			run, vs->ptr, vs->len, opt, buf, len);
	}

#ifdef DEBUG_VAL
	qse_dprintf (
		QSE_T("ERROR: WRONG VALUE TYPE [%d] in qse_awk_rtx_valtostr\n"), 
		v->type);
#endif

	qse_awk_rtx_seterror (run, QSE_AWK_EVALTYPE, 0, QSE_NULL, 0);
	return QSE_NULL;
}

static qse_char_t* str_to_str (
	qse_awk_rtx_t* run, const qse_char_t* str, qse_size_t str_len,
	int opt, qse_str_t* buf, qse_size_t* len)
{
	if (buf == QSE_NULL)
	{
		qse_char_t* tmp;
		tmp = QSE_AWK_STRXDUP (run->awk, str, str_len);
		if (tmp == QSE_NULL) 
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
			return QSE_NULL;
		}

		if (len != QSE_NULL) *len = str_len;
		return tmp;
	}
	else if (opt & QSE_AWK_VALTOSTR_FIXED)
	{
		QSE_ASSERT (buf != QSE_NULL && len != QSE_NULL);

		if (str_len >= *len)
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_EINVAL, 0, QSE_NULL, 0);
			*len = str_len + 1;
			return QSE_NULL;
		}

		*len = qse_strncpy ((qse_char_t*)buf, str, str_len);
		return (qse_char_t*)buf;
	}
	else
	{
		qse_size_t n;

		if (opt & QSE_AWK_VALTOSTR_CLEAR) qse_str_clear (buf);
		n = qse_str_ncat (buf, str, str_len);
		if (n == (qse_size_t)-1)
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
			return QSE_NULL;
		}

		if (len != QSE_NULL) *len = QSE_STR_LEN(buf);
		return QSE_STR_PTR(buf);
	}
}

static qse_char_t* val_int_to_str (
	qse_awk_rtx_t* run, qse_awk_val_int_t* v,
	int opt, qse_str_t* buf, qse_size_t* len)
{
	qse_char_t* tmp;
	qse_long_t t;
	qse_size_t rlen = 0;

	t = v->val; 
	if (t == 0)
	{
		/* handle zero */
		if (buf == QSE_NULL)
		{
			tmp = QSE_AWK_ALLOC (
				run->awk, 2 * QSE_SIZEOF(qse_char_t));
			if (tmp == QSE_NULL)
			{
				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
				return QSE_NULL;
			}

			tmp[0] = QSE_T('0');
			tmp[1] = QSE_T('\0');
			if (len != QSE_NULL) *len = 1;
			return tmp;
		}
		else if (opt & QSE_AWK_VALTOSTR_FIXED)
		{
			QSE_ASSERT (buf != QSE_NULL && len != QSE_NULL);
	
			if (1 >= *len)
			{
				qse_awk_rtx_seterror (
					run, QSE_AWK_EINVAL, 0, QSE_NULL, 0);
				*len = 2; /* buffer size required */
				return QSE_NULL;
			}

			tmp = (qse_char_t*)buf;
			tmp[0] = QSE_T('0');
			tmp[1] = QSE_T('\0');
			*len = 1; /* actual length */
			return tmp;
		}
		else
		{
			if (opt & QSE_AWK_VALTOSTR_CLEAR) qse_str_clear (buf);
			if (qse_str_cat (buf, QSE_T("0")) == (qse_size_t)-1)
			{
				qse_awk_rtx_seterror (
					run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
				return QSE_NULL;
			}

			if (len != QSE_NULL) *len = QSE_STR_LEN(buf);
			return QSE_STR_PTR(buf);
		}
	}

	/* non-zero values */
	if (t < 0) { t = -t; rlen++; }
	while (t > 0) { rlen++; t /= 10; }

	if (buf == QSE_NULL)
	{
		tmp = QSE_AWK_ALLOC (
			run->awk, (rlen + 1) * QSE_SIZEOF(qse_char_t));
		if (tmp == QSE_NULL)
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
			return QSE_NULL;
		}

		tmp[rlen] = QSE_T('\0');
		if (len != QSE_NULL) *len = rlen;
	}
	else if (opt & QSE_AWK_VALTOSTR_FIXED)
	{
		QSE_ASSERT (buf != QSE_NULL && len != QSE_NULL);

		if (rlen >= *len)
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_EINVAL, 0, QSE_NULL, 0);
			*len = rlen + 1; /* buffer size required */
			return QSE_NULL;
		}

		tmp = (qse_char_t*)buf;
		tmp[rlen] = QSE_T('\0');
		*len = rlen; /* actual length */
	}
	else
	{
		/* clear the buffer */
		if (opt & QSE_AWK_VALTOSTR_CLEAR) qse_str_clear (buf);

		tmp = QSE_STR_PTR(buf) + QSE_STR_LEN(buf);

		/* extend the buffer */
		if (qse_str_nccat (
			buf, QSE_T(' '), rlen) == (qse_size_t)-1)
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
			return QSE_NULL;
		}
	}

	t = v->val; 
	if (t < 0) t = -t;

	while (t > 0) 
	{
		tmp[--rlen] = (qse_char_t)(t % 10) + QSE_T('0');
		t /= 10;
	}

	if (v->val < 0) tmp[--rlen] = QSE_T('-');

	if (buf != QSE_NULL && !(opt & QSE_AWK_VALTOSTR_FIXED))
	{
		tmp = QSE_STR_PTR(buf);
		if (len != QSE_NULL) *len = QSE_STR_LEN(buf);
	}

	return tmp;
}

static qse_char_t* val_real_to_str (
	qse_awk_rtx_t* run, qse_awk_val_real_t* v,
	int opt, qse_str_t* buf, qse_size_t* len)
{
	qse_char_t* tmp;
	qse_size_t tmp_len;
	qse_str_t out, fbu;

	if (opt & QSE_AWK_VALTOSTR_PRINT)
	{
		tmp = run->gbl.ofmt.ptr;
		tmp_len = run->gbl.ofmt.len;
	}
	else
	{
		tmp = run->gbl.convfmt.ptr;
		tmp_len = run->gbl.convfmt.len;
	}

	if (qse_str_init (&out, run->awk->mmgr, 256) == QSE_NULL)
	{
		qse_awk_rtx_seterror (run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
		return QSE_NULL;
	}

	if (qse_str_init (&fbu, run->awk->mmgr, 256) == QSE_NULL)
	{
		qse_str_fini (&out);
		qse_awk_rtx_seterror (run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
		return QSE_NULL;
	}

	tmp = qse_awk_format (run, &out, &fbu, tmp, tmp_len, 
		(qse_size_t)-1, (qse_awk_nde_t*)v, &tmp_len);
	if (tmp == QSE_NULL) 
	{
		qse_str_fini (&fbu);
		qse_str_fini (&out);
		return QSE_NULL;
	}

	if (buf == QSE_NULL) 
	{
		qse_str_fini (&fbu);

		qse_str_yield (&out, QSE_NULL, 0);
		qse_str_fini (&out);

		if (len != QSE_NULL) *len = tmp_len;
	}
	else if (opt & QSE_AWK_VALTOSTR_FIXED)
	{
		QSE_ASSERT (buf != QSE_NULL && len != QSE_NULL);

		if (tmp_len >= *len)
		{
			qse_awk_rtx_seterror (
				run, QSE_AWK_EINVAL, 0, QSE_NULL, 0);
			*len = tmp_len + 1; /* buffer size required */
			qse_str_close (&fbu);
			qse_str_close (&out);
			return QSE_NULL;
		}

		qse_strncpy ((qse_char_t*)buf, tmp, tmp_len);
		tmp = (qse_char_t*)buf;
		*len = tmp_len;

		qse_str_fini (&fbu);
		qse_str_fini (&out);
	}
	else
	{
		if (opt & QSE_AWK_VALTOSTR_CLEAR) qse_str_clear (buf);

		if (qse_str_ncat (buf, tmp, tmp_len) == (qse_size_t)-1)
		{
			qse_str_fini (&fbu);
			qse_str_fini (&out);
			qse_awk_rtx_seterror (
				run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
			return QSE_NULL;
		}

		tmp = QSE_STR_PTR(buf);
		if (len != QSE_NULL) *len = QSE_STR_LEN(buf);

		qse_str_fini (&fbu);
		qse_str_fini (&out);
	}

	return tmp;
}

int qse_awk_rtx_valtonum (
	qse_awk_rtx_t* run, qse_awk_val_t* v, qse_long_t* l, qse_real_t* r)
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
		return qse_awk_rtx_strtonum (run,
			((qse_awk_val_str_t*)v)->ptr, 
			((qse_awk_val_str_t*)v)->len, l, r);
	}

#ifdef DEBUG_VAL
	qse_dprintf (
		QSE_T("ERROR: WRONG VALUE TYPE [%d] in qse_awk_rtx_valtonum\n"), 
		v->type);
#endif

	qse_awk_rtx_seterror (run, QSE_AWK_EVALTYPE, 0, QSE_NULL, 0);
	return -1; /* error */
}

int qse_awk_rtx_strtonum (
	qse_awk_rtx_t* run, const qse_char_t* ptr, qse_size_t len, 
	qse_long_t* l, qse_real_t* r)
{
	const qse_char_t* endptr;

	*l = qse_awk_strxtolong (run->awk, ptr, len, 0, &endptr);
	if (*endptr == QSE_T('.') ||
	    *endptr == QSE_T('E') ||
	    *endptr == QSE_T('e'))
	{
		*r = qse_awk_strxtoreal (run->awk, ptr, len, QSE_NULL);
		/* TODO: need to check if it is a valid number using 
		 *       endptr for strxtoreal? */
		return 1; /* real */
	}

	/* TODO: do should i handle strings ending with invalid number 
	 *       characters like "123xx" or "dkdkdkd"? */
	return 0; /* long */
}

#if 0

#define DPRINTF run->awk->prmfns->dprintf
#define DCUSTOM run->awk->prmfns->data

static qse_map_walk_t print_pair (
	qse_map_t* map, qse_map_pair_t* pair, void* arg)
{
	qse_awk_rtx_t* run = (qse_awk_rtx_t*)arg;

	QSE_ASSERT (run == *(qse_awk_rtx_t**)qse_map_getxtn(map));

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
