/*
 * $Id: val.c,v 1.85 2006-11-16 04:44:16 bacon Exp $
 */

#include <ase/awk/awk_i.h>

static ase_char_t* __str_to_str (
	ase_awk_run_t* run, const ase_char_t* str, ase_size_t str_len,
	int opt, ase_awk_str_t* buf, ase_size_t* len);
static ase_char_t* __val_int_to_str (
	ase_awk_run_t* run, ase_awk_val_int_t* v,
	int opt, ase_awk_str_t* buf, ase_size_t* len);
static ase_char_t* __val_real_to_str (
	ase_awk_run_t* run, ase_awk_val_real_t* v,
	int opt, ase_awk_str_t* buf, ase_size_t* len);

static ase_awk_val_nil_t __awk_nil = { ASE_AWK_VAL_NIL, 0 };
ase_awk_val_t* ase_awk_val_nil = (ase_awk_val_t*)&__awk_nil;

static ase_awk_val_str_t __awk_zls = { ASE_AWK_VAL_STR, 0, ASE_T(""), 0 };
ase_awk_val_t* ase_awk_val_zls = (ase_awk_val_t*)&__awk_zls; 

static ase_awk_val_int_t __awk_int[] =
{
	{ ASE_AWK_VAL_INT, 0, -1 },
	{ ASE_AWK_VAL_INT, 0,  0 },
	{ ASE_AWK_VAL_INT, 0,  1 },
	{ ASE_AWK_VAL_INT, 0,  2 },
	{ ASE_AWK_VAL_INT, 0,  3 },
	{ ASE_AWK_VAL_INT, 0,  4 },
	{ ASE_AWK_VAL_INT, 0,  5 },
	{ ASE_AWK_VAL_INT, 0,  6 },
	{ ASE_AWK_VAL_INT, 0,  7 },
	{ ASE_AWK_VAL_INT, 0,  8 },
	{ ASE_AWK_VAL_INT, 0,  9 },
};

ase_awk_val_t* ase_awk_val_zero = (ase_awk_val_t*)&__awk_int[1];
ase_awk_val_t* ase_awk_val_one = (ase_awk_val_t*)&__awk_int[2];

ase_awk_val_t* ase_awk_makeintval (ase_awk_run_t* run, ase_long_t v)
{
	ase_awk_val_int_t* val;

	if (v >= __awk_int[0].val && 
	    v <= __awk_int[ase_countof(__awk_int)-1].val)
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
			run->awk, ase_sizeof(ase_awk_val_int_t));
		if (val == ASE_NULL) return ASE_NULL;
	}

	val->type = ASE_AWK_VAL_INT;
	val->ref = 0;
	val->val = v;
	val->nde = ASE_NULL;

/*xp_printf (ASE_T("makeintval => %p\n"), val);*/
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
			run->awk, ase_sizeof(ase_awk_val_real_t));
		if (val == ASE_NULL) return ASE_NULL;
	}

	val->type = ASE_AWK_VAL_REAL;
	val->ref = 0;
	val->val = v;
	val->nde = ASE_NULL;

/*xp_printf (ASE_T("makerealval => %p\n"), val);*/
	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makestrval0 (ase_awk_run_t* run, const ase_char_t* str)
{
	return ase_awk_makestrval (run, str, ase_awk_strlen(str));
}

ase_awk_val_t* ase_awk_makestrval (
	ase_awk_run_t* run, const ase_char_t* str, ase_size_t len)
{
	ase_awk_val_str_t* val;

	val = (ase_awk_val_str_t*) ASE_AWK_MALLOC (
		run->awk, ase_sizeof(ase_awk_val_str_t));
	if (val == ASE_NULL) return ASE_NULL;

	val->type = ASE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len;
	val->buf = ase_awk_strxdup (run->awk, str, len);
	if (val->buf == ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, val);
		return ASE_NULL;
	}

/*xp_printf (ASE_T("makestrval => %p\n"), val);*/
	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makestrval2 (
	ase_awk_run_t* run,
	const ase_char_t* str1, ase_size_t len1, 
	const ase_char_t* str2, ase_size_t len2)
{
	ase_awk_val_str_t* val;

	val = (ase_awk_val_str_t*) ASE_AWK_MALLOC (
		run->awk, ase_sizeof(ase_awk_val_str_t));
	if (val == ASE_NULL) return ASE_NULL;

	val->type = ASE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len1 + len2;
	val->buf = ase_awk_strxdup2 (run->awk, str1, len1, str2, len2);
	if (val->buf == ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, val);
		return ASE_NULL;
	}

/*xp_printf (ASE_T("makestrval2 => %p\n"), val);*/
	return (ase_awk_val_t*)val;
}

ase_awk_val_t* ase_awk_makerexval (
	ase_awk_run_t* run, const ase_char_t* buf, ase_size_t len, void* code)
{
	ase_awk_val_rex_t* val;

	val = (ase_awk_val_rex_t*) ASE_AWK_MALLOC (
		run->awk, ase_sizeof(ase_awk_val_rex_t));
	if (val == ASE_NULL) return ASE_NULL;

	val->type = ASE_AWK_VAL_REX;
	val->ref = 0;
	val->len = len;
	val->buf = ase_awk_strxdup (run->awk, buf, len);
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

	ASE_AWK_MEMCPY (run->awk, val->code, code, ASE_AWK_REX_LEN(code));
	return (ase_awk_val_t*)val;
}

static void __free_map_val (void* run, void* v)
{
/*
xp_printf (ASE_T("refdown in map free..."));
ase_awk_dprintval (v);
xp_printf (ASE_T("\n"));
*/
	ase_awk_refdownval (run, v);
}

ase_awk_val_t* ase_awk_makemapval (ase_awk_run_t* run)
{
	ase_awk_val_map_t* val;

	val = (ase_awk_val_map_t*) ASE_AWK_MALLOC (
		run->awk, ase_sizeof(ase_awk_val_map_t));
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
			run->awk, ase_sizeof(ase_awk_val_ref_t));
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
	       val == ase_awk_val_nil || val == ase_awk_val_zls || 
	       val == ase_awk_val_zero || val == ase_awk_val_one || 
	       (val >= (ase_awk_val_t*)&__awk_int[0] &&
	        val <= (ase_awk_val_t*)&__awk_int[ase_countof(__awk_int)-1]);
}

void ase_awk_freeval (ase_awk_run_t* run, ase_awk_val_t* val, ase_bool_t cache)
{
	if (ase_awk_isbuiltinval(val)) return;

/*xp_printf (ASE_T("freeing [cache=%d] ... "), cache);
ase_awk_dprintval (val);
xp_printf (ASE_T("\n"));*/
	if (val->type == ASE_AWK_VAL_NIL)
	{
		ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_INT)
	{
		if (cache == ase_true &&
		    run->icache_count < ase_countof(run->icache))
		{
			run->icache[run->icache_count++] = 
				(ase_awk_val_int_t*)val;	
		}
		else ASE_AWK_FREE (run->awk, val);
	}
	else if (val->type == ASE_AWK_VAL_REAL)
	{
		if (cache == ase_true &&
		    run->rcache_count < ase_countof(run->rcache))
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
		    run->fcache_count < ase_countof(run->fcache))
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

void ase_awk_refupval (ase_awk_val_t* val)
{
	if (ase_awk_isbuiltinval(val)) return;
/*
xp_printf (ASE_T("ref up "));
ase_awk_dprintval (val);
xp_printf (ASE_T("\n"));
*/
	val->ref++;
}

void ase_awk_refdownval (ase_awk_run_t* run, ase_awk_val_t* val)
{
	if (ase_awk_isbuiltinval(val)) return;

/*
xp_printf (ASE_T("%p, %p, %p\n"), ase_awk_val_nil, &__awk_nil, val);
xp_printf (ASE_T("ref down [count=>%d]\n"), (int)val->ref);
ase_awk_dprintval (val);
xp_printf (ASE_T("\n"));
*/

	ASE_AWK_ASSERTX (run->awk, val->ref > 0, 
		"the reference count of a value should be greater than zero for it to be decremented. check the source code for any bugs");

	val->ref--;
	if (val->ref <= 0) 
	{
/*
xp_printf (ASE_T("**FREEING ["));
ase_awk_dprintval (val);
xp_printf (ASE_T("]\n"));
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
	int opt, ase_awk_str_t* buf, ase_size_t* len)
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

/* TODO: process more value types */
	run->awk->syscas.dprintf (
		ASE_T("ERROR: WRONG VALUE TYPE [%d] in ase_awk_valtostr\n"), 
		v->type);

	run->errnum = ASE_AWK_EVALTYPE;
	return ASE_NULL;
}

static ase_char_t* __str_to_str (
	ase_awk_run_t* run, const ase_char_t* str, ase_size_t str_len,
	int opt, ase_awk_str_t* buf, ase_size_t* len)
{
	if (buf == ASE_NULL)
	{
		ase_char_t* tmp;
		tmp = ase_awk_strxdup (run->awk, str, str_len);
		if (tmp == ASE_NULL) 
		{
			run->errnum = ASE_AWK_ENOMEM;
			return ASE_NULL;
		}

		if (len != ASE_NULL) *len = str_len;
		return tmp;
	}
	else
	{
		ase_size_t n;

		if (opt & ASE_AWK_VALTOSTR_CLEAR) ase_awk_str_clear (buf);
		n = ase_awk_str_ncat (buf, str, str_len);
		if (n == (ase_size_t)-1)
		{
			run->errnum = ASE_AWK_ENOMEM;
			return ASE_NULL;
		}

		if (len != ASE_NULL) *len = ASE_AWK_STR_LEN(buf);
		return ASE_AWK_STR_BUF(buf);
	}
}

static ase_char_t* __val_int_to_str (
	ase_awk_run_t* run, ase_awk_val_int_t* v,
	int opt, ase_awk_str_t* buf, ase_size_t* len)
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
				run->awk, 2 * ase_sizeof(ase_char_t));
			if (tmp == ASE_NULL)
			{
				run->errnum = ASE_AWK_ENOMEM;
				return ASE_NULL;
			}

			tmp[0] = ASE_T('0');
			tmp[1] = ASE_T('\0');
			if (len != ASE_NULL) *len = 1;
			return tmp;
		}
		else
		{
			if (opt & ASE_AWK_VALTOSTR_CLEAR) ase_awk_str_clear (buf);
			if (ase_awk_str_cat (buf, ASE_T("0")) == (ase_size_t)-1)
			{
				run->errnum = ASE_AWK_ENOMEM;
				return ASE_NULL;
			}

			if (len != ASE_NULL) *len = ASE_AWK_STR_LEN(buf);
			return ASE_AWK_STR_BUF(buf);
		}
	}

	/* non-zero values */
	if (t < 0) { t = -t; l++; }
	while (t > 0) { l++; t /= 10; }

	if (buf == ASE_NULL)
	{
		tmp = ASE_AWK_MALLOC (
			run->awk, (l + 1) * ase_sizeof(ase_char_t));
		if (tmp == ASE_NULL)
		{
			run->errnum = ASE_AWK_ENOMEM;
			return ASE_NULL;
		}

		tmp[l] = ASE_T('\0');
		if (len != ASE_NULL) *len = l;
	}
	else
	{
		/* clear the buffer */
		if (opt & ASE_AWK_VALTOSTR_CLEAR) ase_awk_str_clear (buf);

		tmp = ASE_AWK_STR_BUF(buf) + ASE_AWK_STR_LEN(buf);

		/* extend the buffer */
		if (ase_awk_str_nccat (
			buf, ASE_T(' '), l) == (ase_size_t)-1)
		{
			run->errnum = ASE_AWK_ENOMEM;
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
		tmp = ASE_AWK_STR_BUF(buf);
		if (len != ASE_NULL) *len = ASE_AWK_STR_LEN(buf);
	}

	return tmp;
}

static ase_char_t* __val_real_to_str (
	ase_awk_run_t* run, ase_awk_val_real_t* v,
	int opt, ase_awk_str_t* buf, ase_size_t* len)
{
/* TODO: change the code */
	ase_char_t tbuf[256], * tmp;
	ase_size_t tmp_len;

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

/* TODO: need to use awk's own version of sprintf so that it would have
 *       problems with handling long double or double... */
/* TODO: does it need to check if a null character is included in convfmt??? */
/* TODO: check if convfmt contains more that one format specifier */
	//run->awk->syscas.sprintf (tbuf, ase_countof(tbuf), tmp, (double)v->val);
	tmp = ase_awk_sprintf (run, tmp, tmp_len, 
		(ase_size_t)-1, (ase_awk_nde_t*)v, &tmp_len);
	if (tmp == ASE_NULL) return ASE_NULL;

	if (buf == ASE_NULL) 
	{
		//tmp = ase_awk_strdup (run->awk, tbuf);
		tmp = ase_awk_strxdup (run->awk, tmp, tmp_len);
		if (tmp == ASE_NULL) 
		{
			run->errnum = ASE_AWK_ENOMEM;
			return ASE_NULL;
		}

		if (len != ASE_NULL) *len = ase_awk_strlen(tmp);
	}
	else
	{
		if (opt & ASE_AWK_VALTOSTR_CLEAR) ase_awk_str_clear (buf);

		//if (ase_awk_str_cat (buf, tbuf) == (ase_size_t)-1)
		if (ase_awk_str_ncat (buf, tmp, tmp_len) == (ase_size_t)-1)
		{
			run->errnum = ASE_AWK_ENOMEM;
			return ASE_NULL;
		}

		tmp = ASE_AWK_STR_BUF(buf);
		if (len != ASE_NULL) *len = ASE_AWK_STR_LEN(buf);
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
	}

	run->awk->syscas.dprintf (
		ASE_T("ERROR: WRONG VALUE TYPE [%d] in ase_awk_valtonum\n"), 
		v->type);

	run->errnum = ASE_AWK_EVALTYPE;
	return -1; /* error */
}

#define __DPRINTF run->awk->syscas.dprintf

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
		#if defined(__BORLANDC__) || defined(_MSC_VER)
			__DPRINTF (ASE_T("%I64d"), 
				(__int64)((ase_awk_nde_int_t*)val)->val);
		#elif defined(vax) || defined(__vax) || defined(_SCO_DS)
			__DPRINTF (ASE_T("%ld"), 
				(long)((ase_awk_val_int_t*)val)->val);
		#else
			__DPRINTF (ASE_T("%lld"), 
				(long long)((ase_awk_val_int_t*)val)->val);
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
