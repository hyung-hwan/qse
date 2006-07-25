/*
 * $Id: val.c,v 1.45 2006-07-25 16:41:40 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#endif

static xp_awk_val_nil_t __awk_nil = { XP_AWK_VAL_NIL, 0 };
xp_awk_val_t* xp_awk_val_nil = (xp_awk_val_t*)&__awk_nil;

static xp_awk_val_str_t __awk_zls = { XP_AWK_VAL_STR, 0, XP_T(""), 0 };
xp_awk_val_t* xp_awk_val_zls = (xp_awk_val_t*)&__awk_zls; 

static xp_awk_val_int_t __awk_int[] =
{
	{ XP_AWK_VAL_INT, 0, -1 },
	{ XP_AWK_VAL_INT, 0,  0 },
	{ XP_AWK_VAL_INT, 0,  1 },
	{ XP_AWK_VAL_INT, 0,  2 },
	{ XP_AWK_VAL_INT, 0,  3 },
	{ XP_AWK_VAL_INT, 0,  4 },
	{ XP_AWK_VAL_INT, 0,  5 },
	{ XP_AWK_VAL_INT, 0,  6 },
	{ XP_AWK_VAL_INT, 0,  7 },
	{ XP_AWK_VAL_INT, 0,  8 },
	{ XP_AWK_VAL_INT, 0,  9 },
};

xp_awk_val_t* xp_awk_val_zero = (xp_awk_val_t*)&__awk_int[1];
xp_awk_val_t* xp_awk_val_one = (xp_awk_val_t*)&__awk_int[2];

xp_awk_val_t* xp_awk_makeintval (xp_awk_run_t* run, xp_long_t v)
{
	xp_awk_val_int_t* val;

	if (v >= __awk_int[0].val && 
	    v <= __awk_int[xp_countof(__awk_int)-1].val)
	{
		return (xp_awk_val_t*)&__awk_int[v-__awk_int[0].val];
	}

	if (run->icache_count > 0)
	{
		val = run->icache[--run->icache_count];
	}
	else
	{
		val = (xp_awk_val_int_t*)
			xp_malloc (xp_sizeof(xp_awk_val_int_t));
		if (val == XP_NULL) return XP_NULL;
	}

	val->type = XP_AWK_VAL_INT;
	val->ref = 0;
	val->val = v;

/*xp_printf (XP_T("makeintval => %p\n"), val);*/
	return (xp_awk_val_t*)val;
}

xp_awk_val_t* xp_awk_makerealval (xp_awk_run_t* run, xp_real_t v)
{
	xp_awk_val_real_t* val;

	if (run->rcache_count > 0)
	{
		val = run->rcache[--run->rcache_count];
	}
	else
	{
		val = (xp_awk_val_real_t*)
			xp_malloc (xp_sizeof(xp_awk_val_real_t));
		if (val == XP_NULL) return XP_NULL;
	}

	val->type = XP_AWK_VAL_REAL;
	val->ref = 0;
	val->val = v;

/*xp_printf (XP_T("makerealval => %p\n"), val);*/
	return (xp_awk_val_t*)val;
}

xp_awk_val_t* xp_awk_makestrval0 (const xp_char_t* str)
{
	return xp_awk_makestrval (str, xp_strlen(str));
}

xp_awk_val_t* xp_awk_makestrval (const xp_char_t* str, xp_size_t len)
{
	xp_awk_val_str_t* val;

	val = (xp_awk_val_str_t*) xp_malloc (xp_sizeof(xp_awk_val_str_t));
	if (val == XP_NULL) return XP_NULL;

	val->type = XP_AWK_VAL_STR;
	val->ref = 0;
	val->len = len;
	val->buf = xp_strxdup (str, len);
	if (val->buf == XP_NULL) 
	{
		xp_free (val);
		return XP_NULL;
	}

/*xp_printf (XP_T("makestrval => %p\n"), val);*/
	return (xp_awk_val_t*)val;
}

xp_awk_val_t* xp_awk_makestrval2 (
	const xp_char_t* str1, xp_size_t len1, 
	const xp_char_t* str2, xp_size_t len2)
{
	xp_awk_val_str_t* val;

	val = (xp_awk_val_str_t*) xp_malloc (xp_sizeof(xp_awk_val_str_t));
	if (val == XP_NULL) return XP_NULL;

	val->type = XP_AWK_VAL_STR;
	val->ref = 0;
	val->len = len1 + len2;
	val->buf = xp_strxdup2 (str1, len1, str2, len2);
	if (val->buf == XP_NULL) 
	{
		xp_free (val);
		return XP_NULL;
	}

/*xp_printf (XP_T("makestrval2 => %p\n"), val);*/
	return (xp_awk_val_t*)val;
}

xp_awk_val_t* xp_awk_makerexval (const xp_byte_t* buf, xp_size_t len)
{
	xp_awk_val_rex_t* val;

	val = (xp_awk_val_rex_t*) xp_malloc (xp_sizeof(xp_awk_val_rex_t));
	if (val == XP_NULL) return XP_NULL;

	val->type = XP_AWK_VAL_REX;
	val->ref = 0;
	val->len = len;
	val->buf = xp_malloc (len);
	if (val->buf == XP_NULL) 
	{
		xp_free (val);
		return XP_NULL;
	}

	xp_memcpy (val->buf, buf, len);
	return (xp_awk_val_t*)val;
}

static void __free_map_val (void* run, void* v)
{
/*
xp_printf (XP_T("refdown in map free..."));
xp_awk_printval (v);
xp_printf (XP_T("\n"));
*/
	xp_awk_refdownval (run, v);
}

xp_awk_val_t* xp_awk_makemapval (xp_awk_run_t* run)
{
	xp_awk_val_map_t* val;

	val = (xp_awk_val_map_t*) xp_malloc (xp_sizeof(xp_awk_val_map_t));
	if (val == XP_NULL) return XP_NULL;

	val->type = XP_AWK_VAL_MAP;
	val->ref = 0;
	val->map = xp_awk_map_open (XP_NULL, run, 256, __free_map_val);
	if (val->map == XP_NULL)
	{
		xp_free (val);
		return XP_NULL;
	}

	return (xp_awk_val_t*)val;
}


xp_bool_t xp_awk_isbuiltinval (xp_awk_val_t* val)
{
	return val == XP_NULL || 
	       val == xp_awk_val_nil || val == xp_awk_val_zls || 
	       val == xp_awk_val_zero || val == xp_awk_val_one || 
	       (val >= (xp_awk_val_t*)&__awk_int[0] &&
	        val <= (xp_awk_val_t*)&__awk_int[xp_countof(__awk_int)-1]);
}

void xp_awk_freeval (xp_awk_run_t* run, xp_awk_val_t* val, xp_bool_t cache)
{
	if (xp_awk_isbuiltinval(val)) return;

/*xp_printf (XP_T("freeing [cache=%d] ... "), cache);
xp_awk_printval (val);
xp_printf (XP_T("\n"));*/
	switch (val->type)
	{
	case XP_AWK_VAL_NIL:
		xp_free (val);
		return;

	case XP_AWK_VAL_INT:
		if (cache == xp_true &&
		    run->icache_count < xp_countof(run->icache))
		{
			run->icache[run->icache_count++] = 
				(xp_awk_val_int_t*)val;	
		}
		else xp_free (val);
		return;

	case XP_AWK_VAL_REAL:
		if (cache == xp_true &&
		    run->rcache_count < xp_countof(run->rcache))
		{
			run->rcache[run->rcache_count++] = 
				(xp_awk_val_real_t*)val;	
		}
		else xp_free (val);
		return;

	case XP_AWK_VAL_STR:
		xp_free (((xp_awk_val_str_t*)val)->buf);
		xp_free (val);
		return;

	case XP_AWK_VAL_REX:
		xp_free (((xp_awk_val_rex_t*)val)->buf);
		xp_free (val);
		return;

	case XP_AWK_VAL_MAP:
		xp_awk_map_close (((xp_awk_val_map_t*)val)->map);
		xp_free (val);
		return;
	}

	xp_assert (!"should never happen - invalid value type");
}

void xp_awk_refupval (xp_awk_val_t* val)
{
	if (xp_awk_isbuiltinval(val)) return;
/*
xp_printf (XP_T("ref up "));
xp_awk_printval (val);
xp_printf (XP_T("\n"));
*/
	val->ref++;
}

void xp_awk_refdownval (xp_awk_run_t* run, xp_awk_val_t* val)
{
	if (xp_awk_isbuiltinval(val)) return;

/*
xp_printf (XP_T("%p, %p, %p\n"), xp_awk_val_nil, &__awk_nil, val);
xp_printf (XP_T("ref down [count=>%d]\n"), (int)val->ref);
xp_awk_printval (val);
xp_printf (XP_T("\n"));
*/

	xp_assert (val->ref > 0);
	val->ref--;
	if (val->ref <= 0) 
	{
/*
xp_printf (XP_T("**FREEING ["));
xp_awk_printval (val);
xp_printf (XP_T("]\n"));
*/
		xp_awk_freeval(run, val, xp_true);
	}
}

void xp_awk_refdownval_nofree (xp_awk_run_t* run, xp_awk_val_t* val)
{
	if (xp_awk_isbuiltinval(val)) return;

	xp_assert (val->ref > 0);
	val->ref--;
}

xp_bool_t xp_awk_valtobool (xp_awk_val_t* val)
{
	if (val == XP_NULL) return xp_false;

	switch (val->type)
	{
	case XP_AWK_VAL_NIL:
		return xp_false;
	case XP_AWK_VAL_INT:
		return ((xp_awk_val_int_t*)val)->val != 0;
	case XP_AWK_VAL_REAL:
		return ((xp_awk_val_real_t*)val)->val != 0.0;
	case XP_AWK_VAL_STR:
		return ((xp_awk_val_str_t*)val)->len > 0;
	case XP_AWK_VAL_REX: /* TODO: is this correct? */
		return ((xp_awk_val_rex_t*)val)->len > 0;
	case XP_AWK_VAL_MAP:
		return xp_false; /* TODO: is this correct? */
	}

	xp_assert (!"should never happen - invalid value type");
	return xp_false;
}

xp_char_t* xp_awk_valtostr (
	xp_awk_val_t* v, int* errnum, xp_str_t* buf, xp_size_t* len)
{
	if (v->type == XP_AWK_VAL_NIL)
	{
		if (buf == XP_NULL) 
		{
			xp_char_t* tmp;
			tmp = xp_strdup (XP_T(""));
			if (tmp == XP_NULL) 
			{
				*errnum = XP_AWK_ENOMEM;
				return XP_NULL;
			}

			if (len != XP_NULL) *len = 0;
			return tmp;
		}
		else
		{
			xp_str_clear (buf);
			if (len != XP_NULL) *len = XP_STR_LEN(buf);
			return XP_STR_BUF(buf);
		}
	}

	if (v->type == XP_AWK_VAL_INT)
	{
		xp_char_t* tmp;
		xp_long_t t;
		xp_size_t l = 0;

		t = ((xp_awk_val_int_t*)v)->val; 
		if (t == 0)
		{
			/* handle zero */
			if (buf == XP_NULL)
			{
				tmp = xp_malloc (2 * xp_sizeof(xp_char_t));
				if (tmp == XP_NULL)
				{
					*errnum = XP_AWK_ENOMEM;
					return XP_NULL;
				}

				tmp[0] = XP_T('0');
				tmp[1] = XP_T('\0');
				if (len != XP_NULL) *len = 1;
				return tmp;
			}
			else
			{
				xp_str_clear (buf);
				if (xp_str_cat (buf, XP_T("0")) == (xp_size_t)-1)
				{
					*errnum = XP_AWK_ENOMEM;
					return XP_NULL;
				}

				if (len != XP_NULL) *len = XP_STR_LEN(buf);
				return XP_STR_BUF(buf);
			}
		}

		/* non-zero values */
		if (t < 0) { t = -t; l++; }
		while (t > 0) { l++; t /= 10; }

		if (buf == XP_NULL)
		{
			tmp = xp_malloc ((l + 1) * xp_sizeof(xp_char_t));
			if (tmp == XP_NULL)
			{
				*errnum = XP_AWK_ENOMEM;
				return XP_NULL;
			}

			tmp[l] = XP_T('\0');
		}
		else
		{
			/* clear the buffer */
			xp_str_clear (buf);

			tmp = XP_STR_BUF(buf);

			/* extend the buffer */
			if (xp_str_nccat (
				buf, XP_T(' '), l) == (xp_size_t)-1)
			{
				*errnum = XP_AWK_ENOMEM;
				return XP_NULL;
			}
		}

		if (len != XP_NULL) *len = l;

		t = ((xp_awk_val_int_t*)v)->val; 
		if (t < 0) t = -t;

		while (t > 0) 
		{
			tmp[--l] = (xp_char_t)(t % 10) + XP_T('0');
			t /= 10;
		}

		if (((xp_awk_val_int_t*)v)->val < 0) tmp[--l] = XP_T('-');

		return tmp;
	}

	if (v->type == XP_AWK_VAL_REAL)
	{
		/* TODO: change the code */

		xp_char_t tbuf[256], * tmp;

	#if (XP_SIZEOF_LONG_DOUBLE != 0)
		xp_sprintf (
			tbuf, xp_countof(tbuf), XP_T("%Lf"), 
			(long double)((xp_awk_val_real_t*)v)->val); 
	#elif (XP_SIZEOF_DOUBLE != 0)
		xp_sprintf (
			tbuf, xp_countof(tbuf), XP_T("%f"), 
			(double)((xp_awk_val_real_t*)v)->val); 
	#else
		#error unsupported floating-point data type
	#endif

		if (buf == XP_NULL) 
		{
			tmp = xp_strdup (tbuf);
			if (tmp == XP_NULL) 
			{
				*errnum = XP_AWK_ENOMEM;
				return XP_NULL;
			}

			if (len != XP_NULL) *len = xp_strlen(tmp);
		}
		else
		{
			xp_str_clear (buf);
			if (xp_str_cat (buf, tbuf) == (xp_size_t)-1)
			{
				*errnum = XP_AWK_ENOMEM;
				return XP_NULL;
			}

			tmp = XP_STR_BUF(buf);
			if (len != XP_NULL) *len = XP_STR_LEN(buf);
		}

		return tmp;
	}

	if (v->type == XP_AWK_VAL_STR) 
	{
		xp_char_t* tmp;

		if (buf == XP_NULL)
		{
			tmp = xp_strxdup (
				((xp_awk_val_str_t*)v)->buf, 
				((xp_awk_val_str_t*)v)->len);

			if (tmp == XP_NULL) *errnum = XP_AWK_ENOMEM;
		}
		else
		{
			xp_str_clear (buf);
			tmp = XP_STR_BUF(buf);

			if (xp_str_ncat (buf, 
				((xp_awk_val_str_t*)v)->buf, 
				((xp_awk_val_str_t*)v)->len) == (xp_size_t)-1)
			{
				*errnum = XP_AWK_ENOMEM;
				tmp = XP_NULL;
			}
		}

		if (len != XP_NULL) *len = ((xp_awk_val_str_t*)v)->len;
		return tmp;
	}

/* TODO: process more value types */

xp_printf (XP_T("*** ERROR: WRONG VALUE TYPE [%d] in xp_awk_valtostr v=> %p***\n"), v->type, v);
	*errnum = XP_AWK_EVALTYPE;
	return XP_NULL;
}

int xp_awk_valtonum (xp_awk_val_t* v, xp_long_t* l, xp_real_t* r)
{
	if (v->type == XP_AWK_VAL_NIL) 
	{
		*l = 0;
		return 0;
	}

	if (v->type == XP_AWK_VAL_INT)
	{
		*l = ((xp_awk_val_int_t*)v)->val;
		return 0; /* long */
	}

	if (v->type == XP_AWK_VAL_REAL)
	{
		*r = ((xp_awk_val_real_t*)v)->val;
		return 1; /* real */
	}

	if (v->type == XP_AWK_VAL_STR)
	{
		const xp_char_t* endptr;

		/* don't care about val->len */
		*l = xp_awk_strtolong (((xp_awk_val_str_t*)v)->buf, 0, &endptr);
	
		if (*endptr == XP_T('.') ||
		    *endptr == XP_T('E') ||
		    *endptr == XP_T('e'))
		{
			*r = xp_awk_strtoreal (((xp_awk_val_str_t*)v)->buf);
			return 1; /* real */
		}
	
		return 0; /* long */
	}

xp_printf (XP_T("*** ERROR: WRONG VALUE TYPE [%d] in xp_awk_valtonum v=> %p***\n"), v->type, v);
	return -1; /* error */
}

static int __print_pair (xp_awk_pair_t* pair, void* arg)
{
	xp_printf (XP_T(" %s=>"), pair->key);	
	xp_awk_printval (pair->val);
	xp_printf (XP_T(" "));
	return 0;
}

void xp_awk_printval (xp_awk_val_t* val)
{
/* TODO: better value printing...................... */
	switch (val->type)
	{
	case XP_AWK_VAL_NIL:
		xp_printf (XP_T("nil"));
	       	break;

	case XP_AWK_VAL_INT:
#if defined(__LCC__)
		xp_printf (XP_T("%lld"), 
			(long long)((xp_awk_val_int_t*)val)->val);
#elif defined(__BORLANDC__) || defined(_MSC_VER)
		xp_printf (XP_T("%I64d"), 
			(__int64)((xp_awk_nde_int_t*)val)->val);
#elif defined(vax) || defined(__vax) || defined(_SCO_DS)
		xp_printf (XP_T("%ld"), 
			(long)((xp_awk_val_int_t*)val)->val);
#else
		xp_printf (XP_T("%lld"), 
			(long long)((xp_awk_val_int_t*)val)->val);
#endif
		break;

	case XP_AWK_VAL_REAL:
		xp_printf (XP_T("%Lf"), 
			(long double)((xp_awk_val_real_t*)val)->val);
		break;

	case XP_AWK_VAL_STR:
		xp_printf (XP_T("%s"), ((xp_awk_val_str_t*)val)->buf);
		break;

	case XP_AWK_VAL_REX:
		xp_printf (XP_T("REX[%s]"), ((xp_awk_val_rex_t*)val)->buf);
		break;

	case XP_AWK_VAL_MAP:
		xp_printf (XP_T("MAP["));
		xp_awk_map_walk (((xp_awk_val_map_t*)val)->map, __print_pair, XP_NULL);
		xp_printf (XP_T("]"));
		break;

	default:
		xp_assert (!"should never happen - invalid value type");
		xp_printf (XP_T("**** INTERNAL ERROR - INVALID VALUE TYPE ****\n"));
	}
}
