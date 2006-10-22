/*
 * $Id: val.c,v 1.74 2006-10-22 11:34:53 bacon Exp $
 */

#include <sse/awk/awk_i.h>

static sse_char_t* __str_to_str (
	sse_awk_run_t* run, const sse_char_t* str, sse_size_t str_len,
	int opt, sse_awk_str_t* buf, sse_size_t* len);
static sse_char_t* __val_int_to_str (
	sse_awk_run_t* run, sse_awk_val_int_t* v,
	int opt, sse_awk_str_t* buf, sse_size_t* len);
static sse_char_t* __val_real_to_str (
	sse_awk_run_t* run, sse_awk_val_real_t* v,
	int opt, sse_awk_str_t* buf, sse_size_t* len);

static sse_awk_val_nil_t __awk_nil = { SSE_AWK_VAL_NIL, 0 };
sse_awk_val_t* sse_awk_val_nil = (sse_awk_val_t*)&__awk_nil;

static sse_awk_val_str_t __awk_zls = { SSE_AWK_VAL_STR, 0, SSE_T(""), 0 };
sse_awk_val_t* sse_awk_val_zls = (sse_awk_val_t*)&__awk_zls; 

static sse_awk_val_int_t __awk_int[] =
{
	{ SSE_AWK_VAL_INT, 0, -1 },
	{ SSE_AWK_VAL_INT, 0,  0 },
	{ SSE_AWK_VAL_INT, 0,  1 },
	{ SSE_AWK_VAL_INT, 0,  2 },
	{ SSE_AWK_VAL_INT, 0,  3 },
	{ SSE_AWK_VAL_INT, 0,  4 },
	{ SSE_AWK_VAL_INT, 0,  5 },
	{ SSE_AWK_VAL_INT, 0,  6 },
	{ SSE_AWK_VAL_INT, 0,  7 },
	{ SSE_AWK_VAL_INT, 0,  8 },
	{ SSE_AWK_VAL_INT, 0,  9 },
};

sse_awk_val_t* sse_awk_val_zero = (sse_awk_val_t*)&__awk_int[1];
sse_awk_val_t* sse_awk_val_one = (sse_awk_val_t*)&__awk_int[2];

sse_awk_val_t* sse_awk_makeintval (sse_awk_run_t* run, sse_long_t v)
{
	sse_awk_val_int_t* val;

	if (v >= __awk_int[0].val && 
	    v <= __awk_int[sse_countof(__awk_int)-1].val)
	{
		return (sse_awk_val_t*)&__awk_int[v-__awk_int[0].val];
	}

	if (run->icache_count > 0)
	{
		val = run->icache[--run->icache_count];
	}
	else
	{
		val = (sse_awk_val_int_t*) SSE_AWK_MALLOC (
			run->awk, sse_sizeof(sse_awk_val_int_t));
		if (val == SSE_NULL) return SSE_NULL;
	}

	val->type = SSE_AWK_VAL_INT;
	val->ref = 0;
	val->val = v;
	val->nde = SSE_NULL;

/*sse_printf (SSE_T("makeintval => %p\n"), val);*/
	return (sse_awk_val_t*)val;
}

sse_awk_val_t* sse_awk_makerealval (sse_awk_run_t* run, sse_real_t v)
{
	sse_awk_val_real_t* val;

	if (run->rcache_count > 0)
	{
		val = run->rcache[--run->rcache_count];
	}
	else
	{
		val = (sse_awk_val_real_t*) SSE_AWK_MALLOC (
			run->awk, sse_sizeof(sse_awk_val_real_t));
		if (val == SSE_NULL) return SSE_NULL;
	}

	val->type = SSE_AWK_VAL_REAL;
	val->ref = 0;
	val->val = v;
	val->nde = SSE_NULL;

/*sse_printf (SSE_T("makerealval => %p\n"), val);*/
	return (sse_awk_val_t*)val;
}

sse_awk_val_t* sse_awk_makestrval0 (sse_awk_run_t* run, const sse_char_t* str)
{
	return sse_awk_makestrval (run, str, sse_awk_strlen(str));
}

sse_awk_val_t* sse_awk_makestrval (
	sse_awk_run_t* run, const sse_char_t* str, sse_size_t len)
{
	sse_awk_val_str_t* val;

	val = (sse_awk_val_str_t*) SSE_AWK_MALLOC (
		run->awk, sse_sizeof(sse_awk_val_str_t));
	if (val == SSE_NULL) return SSE_NULL;

	val->type = SSE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len;
	val->buf = sse_awk_strxdup (run->awk, str, len);
	if (val->buf == SSE_NULL) 
	{
		SSE_AWK_FREE (run->awk, val);
		return SSE_NULL;
	}

/*sse_printf (SSE_T("makestrval => %p\n"), val);*/
	return (sse_awk_val_t*)val;
}

sse_awk_val_t* sse_awk_makestrval2 (
	sse_awk_run_t* run,
	const sse_char_t* str1, sse_size_t len1, 
	const sse_char_t* str2, sse_size_t len2)
{
	sse_awk_val_str_t* val;

	val = (sse_awk_val_str_t*) SSE_AWK_MALLOC (
		run->awk, sse_sizeof(sse_awk_val_str_t));
	if (val == SSE_NULL) return SSE_NULL;

	val->type = SSE_AWK_VAL_STR;
	val->ref = 0;
	val->len = len1 + len2;
	val->buf = sse_awk_strxdup2 (run->awk, str1, len1, str2, len2);
	if (val->buf == SSE_NULL) 
	{
		SSE_AWK_FREE (run->awk, val);
		return SSE_NULL;
	}

/*sse_printf (SSE_T("makestrval2 => %p\n"), val);*/
	return (sse_awk_val_t*)val;
}

sse_awk_val_t* sse_awk_makerexval (
	sse_awk_run_t* run, const sse_char_t* buf, sse_size_t len, void* code)
{
	sse_awk_val_rex_t* val;

	val = (sse_awk_val_rex_t*) SSE_AWK_MALLOC (
		run->awk, sse_sizeof(sse_awk_val_rex_t));
	if (val == SSE_NULL) return SSE_NULL;

	val->type = SSE_AWK_VAL_REX;
	val->ref = 0;
	val->len = len;
	val->buf = sse_awk_strxdup (run->awk, buf, len);
	if (val->buf == SSE_NULL) 
	{
		SSE_AWK_FREE (run->awk, val);
		return SSE_NULL;
	}

	val->code = SSE_AWK_MALLOC (run->awk, SSE_AWK_REX_LEN(code));
	if (val->code == SSE_NULL)
	{
		SSE_AWK_FREE (run->awk, val->buf);
		SSE_AWK_FREE (run->awk, val);
		return SSE_NULL;
	}

	SSE_AWK_MEMCPY (run->awk, val->code, code, SSE_AWK_REX_LEN(code));
	return (sse_awk_val_t*)val;
}

static void __free_map_val (void* run, void* v)
{
/*
sse_printf (SSE_T("refdown in map free..."));
sse_awk_printval (v);
sse_printf (SSE_T("\n"));
*/
	sse_awk_refdownval (run, v);
}

sse_awk_val_t* sse_awk_makemapval (sse_awk_run_t* run)
{
	sse_awk_val_map_t* val;

	val = (sse_awk_val_map_t*) SSE_AWK_MALLOC (
		run->awk, sse_sizeof(sse_awk_val_map_t));
	if (val == SSE_NULL) return SSE_NULL;

	val->type = SSE_AWK_VAL_MAP;
	val->ref = 0;
	val->map = sse_awk_map_open (
		SSE_NULL, run, 256, __free_map_val, run->awk);
	if (val->map == SSE_NULL)
	{
		SSE_AWK_FREE (run->awk, val);
		return SSE_NULL;
	}

	return (sse_awk_val_t*)val;
}

sse_awk_val_t* sse_awk_makerefval (sse_awk_run_t* run, int id, sse_awk_val_t** adr)
{
	sse_awk_val_ref_t* val;

	if (run->fcache_count > 0)
	{
		val = run->fcache[--run->fcache_count];
	}
	else
	{
		val = (sse_awk_val_ref_t*) SSE_AWK_MALLOC (
			run->awk, sse_sizeof(sse_awk_val_ref_t));
		if (val == SSE_NULL) return SSE_NULL;
	}

	val->type = SSE_AWK_VAL_REF;
	val->ref = 0;
	val->id = id;
	val->adr = adr;

	return (sse_awk_val_t*)val;
}

sse_bool_t sse_awk_isbuiltinval (sse_awk_val_t* val)
{
	return val == SSE_NULL || 
	       val == sse_awk_val_nil || val == sse_awk_val_zls || 
	       val == sse_awk_val_zero || val == sse_awk_val_one || 
	       (val >= (sse_awk_val_t*)&__awk_int[0] &&
	        val <= (sse_awk_val_t*)&__awk_int[sse_countof(__awk_int)-1]);
}

void sse_awk_freeval (sse_awk_run_t* run, sse_awk_val_t* val, sse_bool_t cache)
{
	if (sse_awk_isbuiltinval(val)) return;

/*sse_printf (SSE_T("freeing [cache=%d] ... "), cache);
sse_awk_printval (val);
sse_printf (SSE_T("\n"));*/
	if (val->type == SSE_AWK_VAL_NIL)
	{
		SSE_AWK_FREE (run->awk, val);
	}
	else if (val->type == SSE_AWK_VAL_INT)
	{
		if (cache == sse_true &&
		    run->icache_count < sse_countof(run->icache))
		{
			run->icache[run->icache_count++] = 
				(sse_awk_val_int_t*)val;	
		}
		else SSE_AWK_FREE (run->awk, val);
	}
	else if (val->type == SSE_AWK_VAL_REAL)
	{
		if (cache == sse_true &&
		    run->rcache_count < sse_countof(run->rcache))
		{
			run->rcache[run->rcache_count++] = 
				(sse_awk_val_real_t*)val;	
		}
		else SSE_AWK_FREE (run->awk, val);
	}
	else if (val->type == SSE_AWK_VAL_STR)
	{
		SSE_AWK_FREE (run->awk, ((sse_awk_val_str_t*)val)->buf);
		SSE_AWK_FREE (run->awk, val);
	}
	else if (val->type == SSE_AWK_VAL_REX)
	{
		SSE_AWK_FREE (run->awk, ((sse_awk_val_rex_t*)val)->buf);
		sse_awk_freerex (run->awk, ((sse_awk_val_rex_t*)val)->code);
		SSE_AWK_FREE (run->awk, val);
	}
	else if (val->type == SSE_AWK_VAL_MAP)
	{
		sse_awk_map_close (((sse_awk_val_map_t*)val)->map);
		SSE_AWK_FREE (run->awk, val);
	}
	else if (val->type == SSE_AWK_VAL_REF)
	{
		if (cache == sse_true &&
		    run->fcache_count < sse_countof(run->fcache))
		{
			run->fcache[run->fcache_count++] = 
				(sse_awk_val_ref_t*)val;	
		}
		else SSE_AWK_FREE (run->awk, val);
	}
	else
	{
		sse_awk_assert (run->awk, 
			!"should never happen - invalid value type");
	}
}

void sse_awk_refupval (sse_awk_val_t* val)
{
	if (sse_awk_isbuiltinval(val)) return;
/*
sse_printf (SSE_T("ref up "));
sse_awk_printval (val);
sse_printf (SSE_T("\n"));
*/
	val->ref++;
}

void sse_awk_refdownval (sse_awk_run_t* run, sse_awk_val_t* val)
{
	if (sse_awk_isbuiltinval(val)) return;

/*
sse_printf (SSE_T("%p, %p, %p\n"), sse_awk_val_nil, &__awk_nil, val);
sse_printf (SSE_T("ref down [count=>%d]\n"), (int)val->ref);
sse_awk_printval (val);
sse_printf (SSE_T("\n"));
*/

	sse_awk_assert (run->awk, val->ref > 0);
	val->ref--;
	if (val->ref <= 0) 
	{
/*
sse_printf (SSE_T("**FREEING ["));
sse_awk_printval (val);
sse_printf (SSE_T("]\n"));
*/
		sse_awk_freeval(run, val, sse_true);
	}
}

void sse_awk_refdownval_nofree (sse_awk_run_t* run, sse_awk_val_t* val)
{
	if (sse_awk_isbuiltinval(val)) return;

	sse_awk_assert (run->awk, val->ref > 0);
	val->ref--;
}

sse_bool_t sse_awk_valtobool (sse_awk_run_t* run, sse_awk_val_t* val)
{
	if (val == SSE_NULL) return sse_false;

	switch (val->type)
	{
	case SSE_AWK_VAL_NIL:
		return sse_false;
	case SSE_AWK_VAL_INT:
		return ((sse_awk_val_int_t*)val)->val != 0;
	case SSE_AWK_VAL_REAL:
		return ((sse_awk_val_real_t*)val)->val != 0.0;
	case SSE_AWK_VAL_STR:
		return ((sse_awk_val_str_t*)val)->len > 0;
	case SSE_AWK_VAL_REX: /* TODO: is this correct? */
		return ((sse_awk_val_rex_t*)val)->len > 0;
	case SSE_AWK_VAL_MAP:
		return sse_false; /* TODO: is this correct? */
	case SSE_AWK_VAL_REF:
		return sse_false; /* TODO: is this correct? */
	}

	sse_awk_assert (run->awk, !"should never happen - invalid value type");
	return sse_false;
}

sse_char_t* sse_awk_valtostr (
	sse_awk_run_t* run, sse_awk_val_t* v,
	int opt, sse_awk_str_t* buf, sse_size_t* len)
{
	if (v->type == SSE_AWK_VAL_NIL)
	{
		return __str_to_str (run, SSE_T(""), 0, opt, buf, len);
	}

	if (v->type == SSE_AWK_VAL_INT)
	{
		sse_awk_val_int_t* vi = (sse_awk_val_int_t*)v;

		/*
		if (vi->nde != SSE_NULL && vi->nde->str != SSE_NULL)
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

	if (v->type == SSE_AWK_VAL_REAL)
	{
		sse_awk_val_real_t* vr = (sse_awk_val_real_t*)v;

		/*
		if (vr->nde != SSE_NULL && vr->nde->str != SSE_NULL)
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

	if (v->type == SSE_AWK_VAL_STR) 
	{
		sse_awk_val_str_t* vs = (sse_awk_val_str_t*)v;

		return __str_to_str (
			run, vs->buf, vs->len, opt, buf, len);
	}

/* TODO: process more value types */

sse_printf (SSE_T("*** ERROR: WRONG VALUE TYPE [%d] in sse_awk_valtostr v=> %p***\n"), v->type, v);
	run->errnum = SSE_AWK_EVALTYPE;
	return SSE_NULL;
}

static sse_char_t* __str_to_str (
	sse_awk_run_t* run, const sse_char_t* str, sse_size_t str_len,
	int opt, sse_awk_str_t* buf, sse_size_t* len)
{
	if (buf == SSE_NULL)
	{
		sse_char_t* tmp;
		tmp = sse_awk_strxdup (run->awk, str, str_len);
		if (tmp == SSE_NULL) 
		{
			run->errnum = SSE_AWK_ENOMEM;
			return SSE_NULL;
		}

		if (len != SSE_NULL) *len = str_len;
		return tmp;
	}
	else
	{
		sse_size_t n;

		if (opt & SSE_AWK_VALTOSTR_CLEAR) sse_awk_str_clear (buf);
		n = sse_awk_str_ncat (buf, str, str_len);
		if (n == (sse_size_t)-1)
		{
			run->errnum = SSE_AWK_ENOMEM;
			return SSE_NULL;
		}

		if (len != SSE_NULL) *len = SSE_AWK_STR_LEN(buf);
		return SSE_AWK_STR_BUF(buf);
	}
}

static sse_char_t* __val_int_to_str (
	sse_awk_run_t* run, sse_awk_val_int_t* v,
	int opt, sse_awk_str_t* buf, sse_size_t* len)
{
	sse_char_t* tmp;
	sse_long_t t;
	sse_size_t l = 0;

	t = v->val; 
	if (t == 0)
	{
		/* handle zero */
		if (buf == SSE_NULL)
		{
			tmp = SSE_AWK_MALLOC (
				run->awk, 2 * sse_sizeof(sse_char_t));
			if (tmp == SSE_NULL)
			{
				run->errnum = SSE_AWK_ENOMEM;
				return SSE_NULL;
			}

			tmp[0] = SSE_T('0');
			tmp[1] = SSE_T('\0');
			if (len != SSE_NULL) *len = 1;
			return tmp;
		}
		else
		{
			if (opt & SSE_AWK_VALTOSTR_CLEAR) sse_awk_str_clear (buf);
			if (sse_awk_str_cat (buf, SSE_T("0")) == (sse_size_t)-1)
			{
				run->errnum = SSE_AWK_ENOMEM;
				return SSE_NULL;
			}

			if (len != SSE_NULL) *len = SSE_AWK_STR_LEN(buf);
			return SSE_AWK_STR_BUF(buf);
		}
	}

	/* non-zero values */
	if (t < 0) { t = -t; l++; }
	while (t > 0) { l++; t /= 10; }

	if (buf == SSE_NULL)
	{
		tmp = SSE_AWK_MALLOC (
			run->awk, (l + 1) * sse_sizeof(sse_char_t));
		if (tmp == SSE_NULL)
		{
			run->errnum = SSE_AWK_ENOMEM;
			return SSE_NULL;
		}

		tmp[l] = SSE_T('\0');
		if (len != SSE_NULL) *len = l;
	}
	else
	{
		/* clear the buffer */
		if (opt & SSE_AWK_VALTOSTR_CLEAR) sse_awk_str_clear (buf);

		tmp = SSE_AWK_STR_BUF(buf) + SSE_AWK_STR_LEN(buf);

		/* extend the buffer */
		if (sse_awk_str_nccat (
			buf, SSE_T(' '), l) == (sse_size_t)-1)
		{
			run->errnum = SSE_AWK_ENOMEM;
			return SSE_NULL;
		}
	}

	t = v->val; 
	if (t < 0) t = -t;

	while (t > 0) 
	{
		tmp[--l] = (sse_char_t)(t % 10) + SSE_T('0');
		t /= 10;
	}

	if (v->val < 0) tmp[--l] = SSE_T('-');

	if (buf != SSE_NULL) 
	{
		tmp = SSE_AWK_STR_BUF(buf);
		if (len != SSE_NULL) *len = SSE_AWK_STR_LEN(buf);
	}

	return tmp;
}

static sse_char_t* __val_real_to_str (
	sse_awk_run_t* run, sse_awk_val_real_t* v,
	int opt, sse_awk_str_t* buf, sse_size_t* len)
{
/* TODO: change the code */
	sse_char_t tbuf[256], * tmp;

	tmp = (opt & SSE_AWK_VALTOSTR_PRINT)? 
		run->global.ofmt.ptr: run->global.convfmt.ptr;

/* TODO: need to use awk's own version of sprintf so that it would have
 *       problems with handling long double or double... */
/* TODO: does it need to check if a null character is included in convfmt??? */
	run->awk->syscas.sprintf (tbuf, sse_countof(tbuf), tmp, (double)v->val); 
	if (buf == SSE_NULL) 
	{
		tmp = sse_awk_strdup (run->awk, tbuf);
		if (tmp == SSE_NULL) 
		{
			run->errnum = SSE_AWK_ENOMEM;
			return SSE_NULL;
		}

		if (len != SSE_NULL) *len = sse_awk_strlen(tmp);
	}
	else
	{
		if (opt & SSE_AWK_VALTOSTR_CLEAR) sse_awk_str_clear (buf);

		if (sse_awk_str_cat (buf, tbuf) == (sse_size_t)-1)
		{
			run->errnum = SSE_AWK_ENOMEM;
			return SSE_NULL;
		}

		tmp = SSE_AWK_STR_BUF(buf);
		if (len != SSE_NULL) *len = SSE_AWK_STR_LEN(buf);
	}

	return tmp;
}

int sse_awk_valtonum (
	sse_awk_run_t* run, sse_awk_val_t* v, sse_long_t* l, sse_real_t* r)
{
	if (v->type == SSE_AWK_VAL_NIL) 
	{
		*l = 0;
		return 0;
	}

	if (v->type == SSE_AWK_VAL_INT)
	{
		*l = ((sse_awk_val_int_t*)v)->val;
		return 0; /* long */
	}

	if (v->type == SSE_AWK_VAL_REAL)
	{
		*r = ((sse_awk_val_real_t*)v)->val;
		return 1; /* real */
	}

	if (v->type == SSE_AWK_VAL_STR)
	{
		const sse_char_t* endptr;

		*l = sse_awk_strxtolong (run->awk, 
			((sse_awk_val_str_t*)v)->buf, 
			((sse_awk_val_str_t*)v)->len, 0, &endptr);
		if (*endptr == SSE_T('.') ||
		    *endptr == SSE_T('E') ||
		    *endptr == SSE_T('e'))
		{
			*r = sse_awk_strxtoreal (run->awk, 
				((sse_awk_val_str_t*)v)->buf,
				((sse_awk_val_str_t*)v)->len, SSE_NULL);
/* TODO: need to check if it is a valid number using endptr for strxtoreal? */
			return 1; /* real */
		}
/* TODO: do should i handle strings ending with invalid number characters like "123xx" or "dkdkdkd"? */
		return 0; /* long */
	}

sse_printf (SSE_T("*** ERROR: WRONG VALUE TYPE [%d] in sse_awk_valtonum v=> %p***\n"), v->type, v);
	run->errnum = SSE_AWK_EVALTYPE;
	return -1; /* error */
}

static int __print_pair (sse_awk_pair_t* pair, void* arg)
{
	sse_printf (SSE_T(" %s=>"), pair->key);	
	sse_awk_printval (pair->val);
	sse_printf (SSE_T(" "));
	return 0;
}

void sse_awk_printval (sse_awk_val_t* val)
{
/* TODO: better value printing...................... */
	switch (val->type)
	{
	case SSE_AWK_VAL_NIL:
		sse_printf (SSE_T("nil"));
	       	break;

	case SSE_AWK_VAL_INT:
#if defined(__LCC__)
		sse_printf (SSE_T("%lld"), 
			(long long)((sse_awk_val_int_t*)val)->val);
#elif defined(__BORLANDC__) || defined(_MSC_VER)
		sse_printf (SSE_T("%I64d"), 
			(__int64)((sse_awk_nde_int_t*)val)->val);
#elif defined(vax) || defined(__vax) || defined(_SCO_DS)
		sse_printf (SSE_T("%ld"), 
			(long)((sse_awk_val_int_t*)val)->val);
#else
		sse_printf (SSE_T("%lld"), 
			(long long)((sse_awk_val_int_t*)val)->val);
#endif
		break;

	case SSE_AWK_VAL_REAL:
		sse_printf (SSE_T("%Lf"), 
			(long double)((sse_awk_val_real_t*)val)->val);
		break;

	case SSE_AWK_VAL_STR:
		sse_printf (SSE_T("%s"), ((sse_awk_val_str_t*)val)->buf);
		break;

	case SSE_AWK_VAL_REX:
		sse_printf (SSE_T("REX[%s]"), ((sse_awk_val_rex_t*)val)->buf);
		break;

	case SSE_AWK_VAL_MAP:
		sse_printf (SSE_T("MAP["));
		sse_awk_map_walk (((sse_awk_val_map_t*)val)->map, __print_pair, SSE_NULL);
		sse_printf (SSE_T("]"));
		break;
	
	case SSE_AWK_VAL_REF:
		sse_printf (SSE_T("REF[id=%d,val="), ((sse_awk_val_ref_t*)val)->id);
		sse_awk_printval (*((sse_awk_val_ref_t*)val)->adr);
		sse_printf (SSE_T("]"));
		break;

	default:
		sse_printf (SSE_T("**** INTERNAL ERROR - INVALID VALUE TYPE ****\n"));
	}
}
