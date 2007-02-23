/*
 * $Id: val.c,v 1.110 2007-02-23 08:17:51 bacon Exp $
 *
 * {License}
 */

#include <ase/awk/awk_i.h>

static ase_char_t* __str_to_str (
	ase_awk_run_t* run, const ase_char_t* str, ase_size_t str_len,
	int opt, ase_str_t* buf, ase_size_t* len);
static ase_char_t* __val_int_to_str (
	ase_awk_run_t* run, ase_awk_val_int_t* v,
	int opt, ase_str_t* buf, ase_size_t* len);
static ase_char_t* __val_real_to_str (
	ase_awk_run_t* run, ase_awk_val_real_t* v,
	int opt, ase_str_t* buf, ase_size_t* len);

static ase_awk_val_nil_t __awk_nil = { ASE_AWK_VAL_NIL, 0 };
static ase_awk_val_str_t __awk_zls = { ASE_AWK_VAL_STR, 0, ASE_T(""), 0 };

ase_awk_val_t* ase_awk_val_nil = (ase_awk_val_t*)&__awk_nil;
ase_awk_val_t* ase_awk_val_zls = (ase_awk_val_t*)&__awk_zls; 

static ase_awk_val_int_t __awk_int[] =
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

ase_awk_val_t* ase_awk_val_negone = (ase_awk_val_t*)&__awk_int[0];
ase_awk_val_t* ase_awk_val_zero = (ase_awk_val_t*)&__awk_int[1];
ase_awk_val_t* ase_awk_val_one = (ase_awk_val_t*)&__awk_int[2];

ase_awk_val_t* ase_awk_makeintval (ase_awk_run_t* run, ase_long_t v)
{
	ase_awk_val_int_t* val;

	if (v >= __awk_int[0].val && 
	    v <= __awk_int[ASE_COUNTOF(__awk_int)-1].val)
	{
		return (ase_awk_val_t*)&__awk_int[v-__awk_int[0].val];
	}

	if (run->icache_count > 0)
	{
		val = run->icache[--run->icache_count];
	}
	else
	{
		val = (ase_awk_val_int_t*) ASE_AWK_MALLOC (
			run->awk, ASE_SIZEOF(ase_awk_val_int_t));
		if (val == ASE_NULL) return ASE_NULL;
	}

	val->type = ASE_AWK_VAL_INT;
	val->ref = 0;
	val->val = v;
	val->nde = ASE_NULL;

/*ase_printf (ASE_T("makeintval => %p\n"), val);*/
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
		if (val == ASE_NULL) return ASE_NULL;
	}

	val->type = ASE_AWK_VAL_REAL;
	val->ref = 0;
	val->val = v;
	val->nde = ASE_NULL;

/*ase_printf (ASE_T("makerealval => %p\n"), val);*/
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

	val = (ase_awk_val_str_t*) ASE_AWK_MALLOC (
		run->awk, ASE_SIZEOF(ase_awk_val_str_t));
	if (val == ASE_NULL) return ASE_NULL;

	val->type = ASE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len;
	val->buf = ase_strxdup (str, len, &run->awk->prmfns.mmgr);
	if (val->buf == ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, val);
		return ASE_NULL;
	}

/*ase_printf (ASE_T("makestrval => %p\n"), val);*/
	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makestrval_nodup (
	ase_awk_run_t* run, ase_char_t* str, ase_size_t len)
{
	ase_awk_val_str_t* val;

	val = (ase_awk_val_str_t*) ASE_AWK_MALLOC (
		run->awk, ASE_SIZEOF(ase_awk_val_str_t));
	if (val == ASE_NULL) return ASE_NULL;

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

	val = (ase_awk_val_str_t*) ASE_AWK_MALLOC (
		run->awk, ASE_SIZEOF(ase_awk_val_str_t));
	if (val == ASE_NULL) return ASE_NULL;

	val->type = ASE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len1 + len2;
	val->buf = ase_strxdup2 (str1, len1, str2, len2, &run->awk->prmfns.mmgr);
	if (val->buf == ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, val);
		return ASE_NULL;
	}

/*ase_printf (ASE_T("makestrval2 => %p\n"), val);*/
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
	val->buf = ase_strxdup (buf, len, &run->awk->prmfns.mmgr);
	if (val->buf == ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, val);
		return ASE_NULL;
	}

	val->code = ASE_AWK_MALLOC (run->awk, ASE_AWK_REX_LEN(code));
	if (val->code == ASE_NULL)
	{
		ASE_AWK_FREE (run->awk, val->buf);
		ASE_AWK_FREE (run->awk, val);
		return ASE_NULL;
	}

	ase_memcpy (val->code, code, ASE_AWK_REX_LEN(code));
	return (ase_awk_val_t*)val;
}

static void __free_map_val (void* run, void* v)
{
/*
ase_printf (ASE_T("refdown in map free..."));
ase_awk_dprintval (v);
ase_printf (ASE_T("\n"));
*/
	ase_awk_refdownval (run, v);
}

ase_awk_val_t* ase_awk_makemapval (ase_awk_run_t* run)
{
	ase_awk_val_map_t* val;

	val = (ase_awk_val_map_t*) ASE_AWK_MALLOC (
		run->awk, ASE_SIZEOF(ase_awk_val_map_t));
	if (val == ASE_NULL) return ASE_NULL;

	val->type = ASE_AWK_VAL_MAP;
	val->ref = 0;
	val->map = ase_awk_map_open (
		ASE_NULL, run, 256, __free_map_val, run->awk);
	if (val->map == ASE_NULL)
	{
		ASE_AWK_FREE (run->awk, val);
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
		if (val == ASE_NULL) return ASE_NULL;
	}

	val->type = ASE_AWK_VAL_REF;
	val->ref = 0;
	val->id = id;
	val->adr = adr;

	return (ase_awk_val_t*)val;
}

ase_bool_t ase_awk_isbuiltinval (ase_awk_val_t* val)
{
	return val == ASE_NULL || 
	       val == ase_awk_val_nil || 
	       val == ase_awk_val_zls || 
	       val == ase_awk_val_zero || 
	       val == ase_awk_val_one || 
	       (val >= (ase_awk_val_t*)&__awk_int[0] &&
	        val <= (ase_awk_val_t*)&__awk_int[ASE_COUNTOF(__awk_int)-1]);
}

void ase_awk_freeval (ase_awk_run_t* run, ase_awk_val_t* val, ase_bool_t cache)
{
	if (ase_awk_isbuiltinval(val)) return;

/*ase_printf (ASE_T("freeing [cache=%d] ... "), cache);
ase_awk_dprintval (val);
ase_printf (ASE_T("\n"));*/
	if (val->type == ASE_AWK_VAL_NIL)
	{
		ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_INT)
	{
		if (cache == ase_true &&
		    run->icache_count < ASE_COUNTOF(run->icache))
		{
			run->icache[run->icache_count++] = 
				(ase_awk_val_int_t*)val;	
		}
		else ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_REAL)
	{
		if (cache == ase_true &&
		    run->rcache_count < ASE_COUNTOF(run->rcache))
		{
			run->rcache[run->rcache_count++] = 
				(ase_awk_val_real_t*)val;	
		}
		else ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_STR)
	{
		ASE_AWK_FREE (run->awk, ((ase_awk_val_str_t*)val)->buf);
		ASE_AWK_FREE (run->awk, val);
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
		if (cache == ase_true &&
		    run->fcache_count < ASE_COUNTOF(run->fcache))
		{
			run->fcache[run->fcache_count++] = 
				(ase_awk_val_ref_t*)val;	
		}
		else ASE_AWK_FREE (run->awk, val);
	}
	else
	{
		ASE_AWK_ASSERTX (run->awk, 
			!"should never happen - invalid value type",
			"the type of a value should be one of ASE_AWK_VAL_XXX's defined in val.h");
	}
}

void ase_awk_refupval (ase_awk_run_t* run, ase_awk_val_t* val)
{
	if (ase_awk_isbuiltinval(val)) return;

/*
run->awk->prmfns.misc.dprintf (ASE_T("ref up [ptr=%p] [count=%d] "), val, (int)val->ref);
ase_awk_dprintval (run, val);
run->awk->prmfns.misc.dprintf (ASE_T("\n"));
*/

	val->ref++;
}

void ase_awk_refdownval (ase_awk_run_t* run, ase_awk_val_t* val)
{
	if (ase_awk_isbuiltinval(val)) return;

/*
run->awk->prmfns.misc.dprintf (ASE_T("ref down [ptr=%p] [count=%d]\n"), val, (int)val->ref);
ase_awk_dprintval (run, val);
run->awk->prmfns.misc.dprintf (ASE_T("\n"));
*/

	ASE_AWK_ASSERTX (run->awk, val->ref > 0, 
		"the reference count of a value should be greater than zero for it to be decremented. check the source code for any bugs");

	val->ref--;
	if (val->ref <= 0) 
	{
/*
ase_printf (ASE_T("**FREEING ["));
ase_awk_dprintval (val);
ase_printf (ASE_T("]\n"));
*/
		ase_awk_freeval(run, val, ase_true);
	}
}

void ase_awk_refdownval_nofree (ase_awk_run_t* run, ase_awk_val_t* val)
{
	if (ase_awk_isbuiltinval(val)) return;

	ASE_AWK_ASSERTX (run->awk, val->ref > 0,
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

	ASE_AWK_ASSERTX (run->awk, 
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
		return __str_to_str (run, ASE_T(""), 0, opt, buf, len);
	}

	if (v->type == ASE_AWK_VAL_INT)
	{
		ase_awk_val_int_t* vi = (ase_awk_val_int_t*)v;

		/*
		if (vi->nde != ASE_NULL && vi->nde->str != ASE_NULL)
		{
			return __str_to_str (
				run, vi->nde->str, vi->nde->len, 
				opt, buf, len);
		}
		else
		{
			*/
			return __val_int_to_str (run, vi, opt, buf, len);
		/*}*/
	}

	if (v->type == ASE_AWK_VAL_REAL)
	{
		ase_awk_val_real_t* vr = (ase_awk_val_real_t*)v;

		/*
		if (vr->nde != ASE_NULL && vr->nde->str != ASE_NULL)
		{
			return __str_to_str (
				run, vr->nde->str, vr->nde->len, 
				opt, buf, len);
		}
		else
		{*/
			return __val_real_to_str (run, vr, opt, buf, len);
		/*}*/
	}

	if (v->type == ASE_AWK_VAL_STR) 
	{
		ase_awk_val_str_t* vs = (ase_awk_val_str_t*)v;

		return __str_to_str (
			run, vs->buf, vs->len, opt, buf, len);
	}

#ifdef _DEBUG
	run->awk->prmfns.misc.dprintf (
		ASE_T("ERROR: WRONG VALUE TYPE [%d] in ase_awk_valtostr\n"), 
		v->type);
#endif

	ase_awk_setrunerror (run, ASE_AWK_EVALTYPE, 0, ASE_NULL);
	return ASE_NULL;
}

static ase_char_t* __str_to_str (
	ase_awk_run_t* run, const ase_char_t* str, ase_size_t str_len,
	int opt, ase_str_t* buf, ase_size_t* len)
{
	if (buf == ASE_NULL)
	{
		ase_char_t* tmp;
		tmp = ase_strxdup (str, str_len, &run->awk->prmfns.mmgr);
		if (tmp == ASE_NULL) 
		{
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, ASE_NULL);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, ASE_NULL);
			return ASE_NULL;
		}

		if (len != ASE_NULL) *len = ASE_STR_LEN(buf);
		return ASE_STR_BUF(buf);
	}
}

static ase_char_t* __val_int_to_str (
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
					run, ASE_AWK_ENOMEM, 0, ASE_NULL);
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
					run, ASE_AWK_ENOMEM, 0, ASE_NULL);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, ASE_NULL);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, ASE_NULL);
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

static ase_char_t* __val_real_to_str (
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
		ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, ASE_NULL);
		return ASE_NULL;
	}

	if (ase_str_open (&fbu, 256, &run->awk->prmfns.mmgr) == ASE_NULL)
	{
		ase_str_close (&out);
		ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, ASE_NULL);
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
			ase_awk_setrunerror (run, ASE_AWK_ENOMEM, 0, ASE_NULL);
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

#if 0
		const ase_char_t* endptr;

		*l = ase_awk_strxtolong (run->awk, 
			((ase_awk_val_str_t*)v)->buf, 
			((ase_awk_val_str_t*)v)->len, 0, &endptr);
		if (*endptr == ASE_T('.') ||
		    *endptr == ASE_T('E') ||
		    *endptr == ASE_T('e'))
		{
			*r = ase_awk_strxtoreal (run->awk, 
				((ase_awk_val_str_t*)v)->buf,
				((ase_awk_val_str_t*)v)->len, ASE_NULL);
/* TODO: need to check if it is a valid number using endptr for strxtoreal? */
			return 1; /* real */
		}
/* TODO: do should i handle strings ending with invalid number characters like "123xx" or "dkdkdkd"? */
		return 0; /* long */
#endif
	}

#ifdef _DEBUG
	run->awk->prmfns.misc.dprintf (
		ASE_T("ERROR: WRONG VALUE TYPE [%d] in ase_awk_valtonum\n"), 
		v->type);
#endif

	ase_awk_setrunerror (run, ASE_AWK_EVALTYPE, 0, ASE_NULL);
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
/* TODO: need to check if it is a valid number using endptr for strxtoreal? */
		return 1; /* real */
	}
/* TODO: do should i handle strings ending with invalid number characters like "123xx" or "dkdkdkd"? */
	return 0; /* long */

}

#define __DPRINTF run->awk->prmfns.misc.dprintf

static int __print_pair (ase_awk_pair_t* pair, void* arg)
{
	ase_awk_run_t* run = (ase_awk_run_t*)arg;

	__DPRINTF (ASE_T(" %s=>"), pair->key);	
	ase_awk_dprintval ((ase_awk_run_t*)arg, pair->val);
	__DPRINTF (ASE_T(" "));
	return 0;
}

void ase_awk_dprintval (ase_awk_run_t* run, ase_awk_val_t* val)
{
	/* TODO: better value printing ... */

	switch (val->type)
	{
		case ASE_AWK_VAL_NIL:
			__DPRINTF (ASE_T("nil"));
		       	break;

		case ASE_AWK_VAL_INT:
		#if ASE_SIZEOF_LONG_LONG > 0
			__DPRINTF (ASE_T("%lld"), 
				(long long)((ase_awk_val_int_t*)val)->val);
		#elif ASE_SIZEOF___INT64 > 0
			__DPRINTF (ASE_T("%I64d"), 
				(__int64)((ase_awk_val_int_t*)val)->val);
		#elif ASE_SIZEOF_LONG > 0
			__DPRINTF (ASE_T("%ld"), 
				(long)((ase_awk_val_int_t*)val)->val);
		#elif ASE_SIZEOF_INT > 0
			__DPRINTF (ASE_T("%d"), 
				(int)((ase_awk_val_int_t*)val)->val);
		#else
			#error unsupported size
		#endif
			break;

		case ASE_AWK_VAL_REAL:
			__DPRINTF (ASE_T("%Lf"), 
				(long double)((ase_awk_val_real_t*)val)->val);
			break;

		case ASE_AWK_VAL_STR:
			__DPRINTF (ASE_T("%s"), ((ase_awk_val_str_t*)val)->buf);
			break;

		case ASE_AWK_VAL_REX:
			__DPRINTF (ASE_T("REX[%s]"), ((ase_awk_val_rex_t*)val)->buf);
			break;

		case ASE_AWK_VAL_MAP:
			__DPRINTF (ASE_T("MAP["));
			ase_awk_map_walk (((ase_awk_val_map_t*)val)->map, __print_pair, run);
			__DPRINTF (ASE_T("]"));
			break;
	
		case ASE_AWK_VAL_REF:
			__DPRINTF (ASE_T("REF[id=%d,val="), ((ase_awk_val_ref_t*)val)->id);
			ase_awk_dprintval (run, *((ase_awk_val_ref_t*)val)->adr);
			__DPRINTF (ASE_T("]"));
			break;

		default:
			__DPRINTF (ASE_T("**** INTERNAL ERROR - INVALID VALUE TYPE ****\n"));
	}
}
