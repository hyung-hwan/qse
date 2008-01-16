/*
 * $Id: val.c,v 1.14 2007/11/10 15:21:40 bacon Exp $
 *
 * {License}
 */

#include <ase/awk/awk_i.h>

#ifdef DEBUG_VAL
#include <ase/utl/stdio.h>
#endif

static ase_char_t* str_to_str (
	ase_awk_run_t* run, const ase_char_t* str, ase_size_t str_len,
	int opt, ase_str_t* buf, ase_size_t* len);

static ase_char_t* val_int_to_str (
	ase_awk_run_t* run, ase_awk_val_int_t* v,
	int opt, ase_str_t* buf, ase_size_t* len);

static ase_char_t* val_real_to_str (
	ase_awk_run_t* run, ase_awk_val_real_t* v,
	int opt, ase_str_t* buf, ase_size_t* len);

static ase_awk_val_nil_t awk_nil = { ASE_AWK_VAL_NIL, 0 };
static ase_awk_val_str_t awk_zls = { ASE_AWK_VAL_STR, 0, ASE_T(""), 0 };

ase_awk_val_t* ase_awk_val_nil = (ase_awk_val_t*)&awk_nil;
ase_awk_val_t* ase_awk_val_zls = (ase_awk_val_t*)&awk_zls; 

static ase_awk_val_int_t awk_int[] =
{
	{ ASE_AWK_VAL_INT, 0, -1, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  0, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  1, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  2, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  3, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  4, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  5, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  6, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  7, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  8, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0,  9, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 10, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 11, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 12, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 13, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 14, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 15, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 16, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 17, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 18, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 19, ASE_NULL },
	{ ASE_AWK_VAL_INT, 0, 20, ASE_NULL }
};

ase_awk_val_t* ase_awk_val_negone = (ase_awk_val_t*)&awk_int[0];
ase_awk_val_t* ase_awk_val_zero = (ase_awk_val_t*)&awk_int[1];
ase_awk_val_t* ase_awk_val_one = (ase_awk_val_t*)&awk_int[2];

ase_awk_val_t* ase_awk_makeintval (ase_awk_run_t* run, ase_long_t v)
{
	ase_awk_val_int_t* val;

	if (v >= awk_int[0].val && 
	    v <= awk_int[ASE_COUNTOF(awk_int)-1].val)
	{
		return (ase_awk_val_t*)&awk_int[v-awk_int[0].val];
	}

	/*
	if (run->icache_count > 0)
	{
		val = run->icache[--run->icache_count];
	}
	else
	{
		val = (ase_awk_val_int_t*) ASE_AWK_MALLOC (
			run->awk, ASE_SIZEOF(ase_awk_val_int_t));
		if (val == ASE_NULL) 
		{
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return ASE_NULL;
		}
	}
	*/
	ase_awk_val_chunk_t* p = run->ichunk;
	ase_awk_val_int_t* f = run->ifree;

	if (f = ASE_NULL)
	{
		ase_awk_val_int_t* x;
		ase_size_t i;

		p = ASE_AWK_MALLOC (run->awk, 
			ASE_SIZEOF(ase_awk_val_chunk_t)+
			ASE_SIZEOF(ase_awk_val_int_t)*100);
		if (p == ASE_NULL)
		{
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return ASE_NULL;
		}

		x = (ase_awk_val_int_t*)(p + 1);
		for (i = 0; i < 100-1; i++) 
			x[i].nde = (ase_awk_nde_int_t*)&x[i+1];
		x[i].nde = ASE_NULL;

		run->ifree = &f[0];
		p->next = run->ichunk;
	}

	val = run->ifree;
	run->ifree = (ase_awk_val_int_t*)run->ifree->nde;

	val->type = ASE_AWK_VAL_INT;
	val->ref = 0;
	val->val = v;
	val->nde = ASE_NULL;

#ifdef DEBUG_VAL
	ase_dprintf (ASE_T("makeintval => %ld [%p]\n"), (long)v, val);
#endif
	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makerealval (ase_awk_run_t* run, ase_real_t v)
{
	ase_awk_val_real_t* val;

	if (run->rcache_count > 0)
	{
		val = run->rcache[--run->rcache_count];
	}
	else
	{
		val = (ase_awk_val_real_t*) ASE_AWK_MALLOC (
			run->awk, ASE_SIZEOF(ase_awk_val_real_t));
		if (val == ASE_NULL)
		{
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return ASE_NULL;
		}
	}

	val->type = ASE_AWK_VAL_REAL;
	val->ref = 0;
	val->val = v;
	val->nde = ASE_NULL;

#ifdef DEBUG_VAL
	ase_dprintf (ASE_T("makerealval => %Lf [%p]\n"), (double)v, val);
#endif
	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makestrval0 (ase_awk_run_t* run, const ase_char_t* str)
{
	return ase_awk_makestrval (run, str, ase_strlen(str));
}

ase_awk_val_t* ase_awk_makestrval (
	ase_awk_run_t* run, const ase_char_t* str, ase_size_t len)
{
	ase_awk_val_str_t* val;
	ase_size_t rlen = len;

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
	
	val = (ase_awk_val_str_t*) ASE_AWK_MALLOC (
		run->awk, 
		ASE_SIZEOF(ase_awk_val_str_t) + 
		(rlen+1)*ASE_SIZEOF(ase_char_t));
	if (val == ASE_NULL) 
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return ASE_NULL;
	}

init:
	val->type = ASE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len;
	val->buf = (ase_char_t*)(val + 1);
	/*ase_strxncpy (val->buf, len+1, str, len);*/
	ase_strncpy (val->buf, str, len);

#ifdef DEBUG_VAL
	ase_dprintf (ASE_T("makestrval => %p\n"), val);
#endif
	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makestrval_nodup (
	ase_awk_run_t* run, ase_char_t* str, ase_size_t len)
{
	ase_awk_val_str_t* val;

	val = (ase_awk_val_str_t*) ASE_AWK_MALLOC (
		run->awk, ASE_SIZEOF(ase_awk_val_str_t));
	if (val == ASE_NULL) 
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return ASE_NULL;
	}

	val->type = ASE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len;
	val->buf = str;
	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makestrval2 (
	ase_awk_run_t* run,
	const ase_char_t* str1, ase_size_t len1, 
	const ase_char_t* str2, ase_size_t len2)
{
	ase_awk_val_str_t* val;
	ase_size_t rlen = len1 + len2;

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

	val = (ase_awk_val_str_t*) ASE_AWK_MALLOC (
		run->awk, 
		ASE_SIZEOF(ase_awk_val_str_t) +
		(rlen+1)*ASE_SIZEOF(ase_char_t));
	if (val == ASE_NULL) 
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return ASE_NULL;
	}

init:
	val->type = ASE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len1 + len2;
	val->buf = (ase_char_t*)(val + 1);
	/*ase_strxncpy (val->buf, len1+1, str1, len1);
	ase_strxncpy (val->buf[len1], len2+1, str2, len2);*/
	ase_strncpy (val->buf, str1, len1);
	ase_strncpy (&val->buf[len1], str2, len2);

#ifdef DEBUG_VAL
	ase_dprintf (ASE_T("makestrval2 => %p\n"), val);
#endif
	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makerexval (
	ase_awk_run_t* run, const ase_char_t* buf, ase_size_t len, void* code)
{
	ase_awk_val_rex_t* val;

	val = (ase_awk_val_rex_t*) ASE_AWK_MALLOC (
		run->awk, ASE_SIZEOF(ase_awk_val_rex_t));
	if (val == ASE_NULL) return ASE_NULL;

	val->type = ASE_AWK_VAL_REX;
	val->ref = 0;
	val->len = len;
	val->buf = ase_awk_strxdup (run->awk, buf, len);
	if (val->buf == ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, val);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return ASE_NULL;
	}

	val->code = ASE_AWK_MALLOC (run->awk, ASE_AWK_REX_LEN(code));
	if (val->code == ASE_NULL)
	{
		ASE_AWK_FREE (run->awk, val->buf);
		ASE_AWK_FREE (run->awk, val);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return ASE_NULL;
	}

	ase_memcpy (val->code, code, ASE_AWK_REX_LEN(code));
	return (ase_awk_val_t*)val;
}

static void free_mapval (void* run, void* v)
{
#ifdef DEBUG_VAL
	ase_dprintf (ASE_T("refdown in map free..."));
	ase_awk_dprintval (run, v);
	ase_dprintf (ASE_T("\n"));
#endif

	ase_awk_refdownval (run, v);
}

static void same_mapval (void* run, void* v)
{
#ifdef DEBUG_VAL
	ase_dprintf (ASE_T("refdown nofree in map free..."));
	ase_awk_dprintval (run, v);
	ase_dprintf (ASE_T("\n"));
#endif
	ase_awk_refdownval_nofree (run, v);
}

ase_awk_val_t* ase_awk_makemapval (ase_awk_run_t* run)
{
	ase_awk_val_map_t* val;

	val = (ase_awk_val_map_t*) ASE_AWK_MALLOC (
		run->awk, ASE_SIZEOF(ase_awk_val_map_t));
	if (val == ASE_NULL) 
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return ASE_NULL;
	}

	val->type = ASE_AWK_VAL_MAP;
	val->ref = 0;
	val->map = ase_awk_map_open (
		run, 256, 70, free_mapval, same_mapval, run->awk);
	if (val->map == ASE_NULL)
	{
		ASE_AWK_FREE (run->awk, val);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return ASE_NULL;
	}

	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makerefval (ase_awk_run_t* run, int id, ase_awk_val_t** adr)
{
	ase_awk_val_ref_t* val;

	if (run->fcache_count > 0)
	{
		val = run->fcache[--run->fcache_count];
	}
	else
	{
		val = (ase_awk_val_ref_t*) ASE_AWK_MALLOC (
			run->awk, ASE_SIZEOF(ase_awk_val_ref_t));
		if (val == ASE_NULL)
		{
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return ASE_NULL;
		}
	}

	val->type = ASE_AWK_VAL_REF;
	val->ref = 0;
	val->id = id;
	val->adr = adr;

	return (ase_awk_val_t*)val;
}

ase_bool_t ase_awk_isstaticval (ase_awk_val_t* val)
{
	return val == ASE_NULL || 
	       val == ase_awk_val_nil || 
	       val == ase_awk_val_zls || 
	       val == ase_awk_val_zero || 
	       val == ase_awk_val_one || 
	       (val >= (ase_awk_val_t*)&awk_int[0] &&
	        val <= (ase_awk_val_t*)&awk_int[ASE_COUNTOF(awk_int)-1]);
}

void ase_awk_freeval (ase_awk_run_t* run, ase_awk_val_t* val, ase_bool_t cache)
{
	if (ase_awk_isstaticval(val)) return;

#ifdef DEBUG_VAL
	ase_dprintf (ASE_T("freeing [cache=%d] ... "), cache);
	ase_awk_dprintval (run, val);
	ase_dprintf (ASE_T("\n"));
#endif

	if (val->type == ASE_AWK_VAL_NIL)
	{
		ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_INT)
	{
		/*
		if (cache && run->icache_count < ASE_COUNTOF(run->icache))
		{
			run->icache[run->icache_count++] = 
				(ase_awk_val_int_t*)val;	
		}
		else ASE_AWK_FREE (run->awk, val);
		*/
		if (cache)
		{
			
			((ase_awk_val_int_t*)val)->nde = 
				(ase_awk_nde_int_t*)run->ifree;
			run->ifree = val;
		}
		else ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_REAL)
	{
		if (cache && run->rcache_count < ASE_COUNTOF(run->rcache))
		{
			run->rcache[run->rcache_count++] = 
				(ase_awk_val_real_t*)val;	
		}
		else ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_STR)
	{
		/*
		if (cache)
		{
			ase_awk_val_str_t* v = (ase_awk_val_str_t*)val;
			if (v->len <= 32 && 
			    run->scache32_count<ASE_COUNTOF(run->scache32))
			{
				run->scache32[run->scache32_count++] = v;
			}
			else if (v->len <= 64 && 
			         run->scache64_count<ASE_COUNTOF(run->scache64))
			{
				run->scache64[run->scache64_count++] = v;
			}
			else ASE_AWK_FREE (run->awk, val);
		}
		else*/ ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_REX)
	{
		ASE_AWK_FREE (run->awk, ((ase_awk_val_rex_t*)val)->buf);
		ase_awk_freerex (run->awk, ((ase_awk_val_rex_t*)val)->code);
		ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_MAP)
	{
		ase_awk_map_close (((ase_awk_val_map_t*)val)->map);
		ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_REF)
	{
		if (cache && run->fcache_count < ASE_COUNTOF(run->fcache))
		{
			run->fcache[run->fcache_count++] = 
				(ase_awk_val_ref_t*)val;	
		}
		else ASE_AWK_FREE (run->awk, val);
	}
	else
	{
		ASE_ASSERTX (
			!"should never happen - invalid value type",
			"the type of a value should be one of ASE_AWK_VAL_XXX's defined in val.h");
	}
}

void ase_awk_refupval (ase_awk_run_t* run, ase_awk_val_t* val)
{
	if (ase_awk_isstaticval(val)) return;

#ifdef DEBUG_VAL
	ase_dprintf (ASE_T("ref up [ptr=%p] [count=%d] "), val, (int)val->ref);
	ase_awk_dprintval (run, val);
	ase_dprintf (ASE_T("\n"));
#endif

	val->ref++;
}

void ase_awk_refdownval (ase_awk_run_t* run, ase_awk_val_t* val)
{
	if (ase_awk_isstaticval(val)) return;

#ifdef DEBUG_VAL
	ase_dprintf (ASE_T("ref down [ptr=%p] [count=%d]\n"), val, (int)val->ref);
	ase_awk_dprintval (run, val);
	ase_dprintf (ASE_T("\n"));
#endif

	ASE_ASSERTX (val->ref > 0, 
		"the reference count of a value should be greater than zero for it to be decremented. check the source code for any bugs");

	val->ref--;
	if (val->ref <= 0) 
	{
		ase_awk_freeval(run, val, ase_true);
	}
}

void ase_awk_refdownval_nofree (ase_awk_run_t* run, ase_awk_val_t* val)
{
	if (ase_awk_isstaticval(val)) return;

	ASE_ASSERTX (val->ref > 0,
		"the reference count of a value should be greater than zero for it to be decremented. check the source code for any bugs");
	val->ref--;
}

ase_bool_t ase_awk_valtobool (ase_awk_run_t* run, ase_awk_val_t* val)
{
	if (val == ASE_NULL) return ase_false;

	switch (val->type)
	{
		case ASE_AWK_VAL_NIL:
			return ase_false;
		case ASE_AWK_VAL_INT:
			return ((ase_awk_val_int_t*)val)->val != 0;
		case ASE_AWK_VAL_REAL:
			return ((ase_awk_val_real_t*)val)->val != 0.0;
		case ASE_AWK_VAL_STR:
			return ((ase_awk_val_str_t*)val)->len > 0;
		case ASE_AWK_VAL_REX: /* TODO: is this correct? */
			return ((ase_awk_val_rex_t*)val)->len > 0;
		case ASE_AWK_VAL_MAP:
			return ase_false; /* TODO: is this correct? */
		case ASE_AWK_VAL_REF:
			return ase_false; /* TODO: is this correct? */
	}

	ASE_ASSERTX (
		!"should never happen - invalid value type",
		"the type of a value should be one of ASE_AWK_VAL_XXX's defined in val.h");
	return ase_false;
}

ase_char_t* ase_awk_valtostr (
	ase_awk_run_t* run, ase_awk_val_t* v,
	int opt, ase_str_t* buf, ase_size_t* len)
{
	if (v->type == ASE_AWK_VAL_NIL)
	{
		return str_to_str (run, ASE_T(""), 0, opt, buf, len);
	}

	if (v->type == ASE_AWK_VAL_INT)
	{
		ase_awk_val_int_t* vi = (ase_awk_val_int_t*)v;

		/*
		if (vi->nde != ASE_NULL && vi->nde->str != ASE_NULL)
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

	if (v->type == ASE_AWK_VAL_REAL)
	{
		ase_awk_val_real_t* vr = (ase_awk_val_real_t*)v;

		/*
		if (vr->nde != ASE_NULL && vr->nde->str != ASE_NULL)
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

	if (v->type == ASE_AWK_VAL_STR) 
	{
		ase_awk_val_str_t* vs = (ase_awk_val_str_t*)v;

		return str_to_str (
			run, vs->buf, vs->len, opt, buf, len);
	}

#ifdef DEBUG_VAL
	ase_dprintf (
		ASE_T("ERROR: WRONG VALUE TYPE [%d] in ase_awk_valtostr\n"), 
		v->type);
#endif

	ase_awk_setrunerror (run, ASE_AWK_EVALTYPE, 0, ASE_NULL, 0);
	return ASE_NULL;
}

static ase_char_t* str_to_str (
	ase_awk_run_t* run, const ase_char_t* str, ase_size_t str_len,
	int opt, ase_str_t* buf, ase_size_t* len)
{
	if (buf == ASE_NULL)
	{
		ase_char_t* tmp;
		tmp = ase_awk_strxdup (run->awk, str, str_len);
		if (tmp == ASE_NULL) 
		{
			ase_awk_setrunerror (
				run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
			return ASE_NULL;
		}

		if (len != ASE_NULL) *len = str_len;
		return tmp;
	}
	else
	{
		ase_size_t n;

		if (opt & ASE_AWK_VALTOSTR_CLEAR) ase_str_clear (buf);
		n = ase_str_ncat (buf, str, str_len);
		if (n == (ase_size_t)-1)
		{
			ase_awk_setrunerror (
				run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
			return ASE_NULL;
		}

		if (len != ASE_NULL) *len = ASE_STR_LEN(buf);
		return ASE_STR_BUF(buf);
	}
}

static ase_char_t* val_int_to_str (
	ase_awk_run_t* run, ase_awk_val_int_t* v,
	int opt, ase_str_t* buf, ase_size_t* len)
{
	ase_char_t* tmp;
	ase_long_t t;
	ase_size_t l = 0;

	t = v->val; 
	if (t == 0)
	{
		/* handle zero */
		if (buf == ASE_NULL)
		{
			tmp = ASE_AWK_MALLOC (
				run->awk, 2 * ASE_SIZEOF(ase_char_t));
			if (tmp == ASE_NULL)
			{
				ase_awk_setrunerror (
					run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
				return ASE_NULL;
			}

			tmp[0] = ASE_T('0');
			tmp[1] = ASE_T('\0');
			if (len != ASE_NULL) *len = 1;
			return tmp;
		}
		else
		{
			if (opt & ASE_AWK_VALTOSTR_CLEAR) ase_str_clear (buf);
			if (ase_str_cat (buf, ASE_T("0")) == (ase_size_t)-1)
			{
				ase_awk_setrunerror (
					run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
				return ASE_NULL;
			}

			if (len != ASE_NULL) *len = ASE_STR_LEN(buf);
			return ASE_STR_BUF(buf);
		}
	}

	/* non-zero values */
	if (t < 0) { t = -t; l++; }
	while (t > 0) { l++; t /= 10; }

	if (buf == ASE_NULL)
	{
		tmp = ASE_AWK_MALLOC (
			run->awk, (l + 1) * ASE_SIZEOF(ase_char_t));
		if (tmp == ASE_NULL)
		{
			ase_awk_setrunerror (
				run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
			return ASE_NULL;
		}

		tmp[l] = ASE_T('\0');
		if (len != ASE_NULL) *len = l;
	}
	else
	{
		/* clear the buffer */
		if (opt & ASE_AWK_VALTOSTR_CLEAR) ase_str_clear (buf);

		tmp = ASE_STR_BUF(buf) + ASE_STR_LEN(buf);

		/* extend the buffer */
		if (ase_str_nccat (
			buf, ASE_T(' '), l) == (ase_size_t)-1)
		{
			ase_awk_setrunerror (
				run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
			return ASE_NULL;
		}
	}

	t = v->val; 
	if (t < 0) t = -t;

	while (t > 0) 
	{
		tmp[--l] = (ase_char_t)(t % 10) + ASE_T('0');
		t /= 10;
	}

	if (v->val < 0) tmp[--l] = ASE_T('-');

	if (buf != ASE_NULL) 
	{
		tmp = ASE_STR_BUF(buf);
		if (len != ASE_NULL) *len = ASE_STR_LEN(buf);
	}

	return tmp;
}

static ase_char_t* val_real_to_str (
	ase_awk_run_t* run, ase_awk_val_real_t* v,
	int opt, ase_str_t* buf, ase_size_t* len)
{
	ase_char_t* tmp;
	ase_size_t tmp_len;
	ase_str_t out, fbu;

	if (opt & ASE_AWK_VALTOSTR_PRINT)
	{
		tmp = run->global.ofmt.ptr;
		tmp_len = run->global.ofmt.len;
	}
	else
	{
		tmp = run->global.convfmt.ptr;
		tmp_len = run->global.convfmt.len;
	}

	if (ase_str_open (&out, 256, &run->awk->prmfns.mmgr) == ASE_NULL)
	{
		ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
		return ASE_NULL;
	}

	if (ase_str_open (&fbu, 256, &run->awk->prmfns.mmgr) == ASE_NULL)
	{
		ase_str_close (&out);
		ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
		return ASE_NULL;
	}

	tmp = ase_awk_format (run, &out, &fbu, tmp, tmp_len, 
		(ase_size_t)-1, (ase_awk_nde_t*)v, &tmp_len);
	if (tmp == ASE_NULL) 
	{
		ase_str_close (&fbu);
		ase_str_close (&out);
		return ASE_NULL;
	}

	if (buf == ASE_NULL) 
	{
		ase_str_close (&fbu);
		ase_str_forfeit (&out);
		if (len != ASE_NULL) *len = tmp_len;
	}
	else
	{
		if (opt & ASE_AWK_VALTOSTR_CLEAR) ase_str_clear (buf);

		if (ase_str_ncat (buf, tmp, tmp_len) == (ase_size_t)-1)
		{
			ase_str_close (&fbu);
			ase_str_close (&out);
			ase_awk_setrunerror (
				run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
			return ASE_NULL;
		}

		tmp = ASE_STR_BUF(buf);
		if (len != ASE_NULL) *len = ASE_STR_LEN(buf);

		ase_str_close (&fbu);
		ase_str_close (&out);
	}

	return tmp;
}

int ase_awk_valtonum (
	ase_awk_run_t* run, ase_awk_val_t* v, ase_long_t* l, ase_real_t* r)
{
	if (v->type == ASE_AWK_VAL_NIL) 
	{
		*l = 0;
		return 0;
	}

	if (v->type == ASE_AWK_VAL_INT)
	{
		*l = ((ase_awk_val_int_t*)v)->val;
		return 0; /* long */
	}

	if (v->type == ASE_AWK_VAL_REAL)
	{
		*r = ((ase_awk_val_real_t*)v)->val;
		return 1; /* real */
	}

	if (v->type == ASE_AWK_VAL_STR)
	{
		return ase_awk_strtonum (run,
			((ase_awk_val_str_t*)v)->buf, 
			((ase_awk_val_str_t*)v)->len, l, r);
	}

#ifdef DEBUG_VAL
	ase_dprintf (
		ASE_T("ERROR: WRONG VALUE TYPE [%d] in ase_awk_valtonum\n"), 
		v->type);
#endif

	ase_awk_setrunerror (run, ASE_AWK_EVALTYPE, 0, ASE_NULL, 0);
	return -1; /* error */
}

int ase_awk_strtonum (
	ase_awk_run_t* run, const ase_char_t* ptr, ase_size_t len, 
	ase_long_t* l, ase_real_t* r)
{
	const ase_char_t* endptr;

	*l = ase_awk_strxtolong (run->awk, ptr, len, 0, &endptr);
	if (*endptr == ASE_T('.') ||
	    *endptr == ASE_T('E') ||
	    *endptr == ASE_T('e'))
	{
		*r = ase_awk_strxtoreal (run->awk, ptr, len, ASE_NULL);
		/* TODO: need to check if it is a valid number using 
		 *       endptr for strxtoreal? */
		return 1; /* real */
	}

	/* TODO: do should i handle strings ending with invalid number 
	 *       characters like "123xx" or "dkdkdkd"? */
	return 0; /* long */
}

#define DPRINTF run->awk->prmfns.misc.dprintf
#define DCUSTOM run->awk->prmfns.misc.custom_data

static int print_pair (ase_awk_pair_t* pair, void* arg)
{
	ase_awk_run_t* run = (ase_awk_run_t*)arg;

	DPRINTF (DCUSTOM, ASE_T(" %s=>"), pair->key);	
	ase_awk_dprintval ((ase_awk_run_t*)arg, pair->val);
	DPRINTF (DCUSTOM, ASE_T(" "));
	return 0;
}

void ase_awk_dprintval (ase_awk_run_t* run, ase_awk_val_t* val)
{
	/* TODO: better value printing ... */

	switch (val->type)
	{
		case ASE_AWK_VAL_NIL:
			DPRINTF (DCUSTOM, ASE_T("nil"));
		       	break;

		case ASE_AWK_VAL_INT:
		#if ASE_SIZEOF_LONG_LONG > 0
			DPRINTF (DCUSTOM, ASE_T("%lld"), 
				(long long)((ase_awk_val_int_t*)val)->val);
		#elif ASE_SIZEOF___INT64 > 0
			DPRINTF (DCUSTOM, ASE_T("%I64d"), 
				(__int64)((ase_awk_val_int_t*)val)->val);
		#elif ASE_SIZEOF_LONG > 0
			DPRINTF (DCUSTOM, ASE_T("%ld"), 
				(long)((ase_awk_val_int_t*)val)->val);
		#elif ASE_SIZEOF_INT > 0
			DPRINTF (DCUSTOM, ASE_T("%d"), 
				(int)((ase_awk_val_int_t*)val)->val);
		#else
			#error unsupported size
		#endif
			break;

		case ASE_AWK_VAL_REAL:
			DPRINTF (DCUSTOM, ASE_T("%Lf"), 
				(long double)((ase_awk_val_real_t*)val)->val);
			break;

		case ASE_AWK_VAL_STR:
			DPRINTF (DCUSTOM, ASE_T("%s"), ((ase_awk_val_str_t*)val)->buf);
			break;

		case ASE_AWK_VAL_REX:
			DPRINTF (DCUSTOM, ASE_T("REX[%s]"), ((ase_awk_val_rex_t*)val)->buf);
			break;

		case ASE_AWK_VAL_MAP:
			DPRINTF (DCUSTOM, ASE_T("MAP["));
			ase_awk_map_walk (((ase_awk_val_map_t*)val)->map, print_pair, run);
			DPRINTF (DCUSTOM, ASE_T("]"));
			break;
	
		case ASE_AWK_VAL_REF:
			DPRINTF (DCUSTOM, ASE_T("REF[id=%d,val="), ((ase_awk_val_ref_t*)val)->id);
			ase_awk_dprintval (run, *((ase_awk_val_ref_t*)val)->adr);
			DPRINTF (DCUSTOM, ASE_T("]"));
			break;

		default:
			DPRINTF (DCUSTOM, ASE_T("**** INTERNAL ERROR - INVALID VALUE TYPE ****\n"));
	}
}
