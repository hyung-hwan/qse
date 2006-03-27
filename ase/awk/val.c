/*
 * $Id: val.c,v 1.11 2006-03-27 11:43:17 bacon Exp $
 */

#include <xp/awk/awk.h>

#ifndef __STAND_ALONE
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#endif

static xp_awk_val_nil_t __awk_nil = { XP_AWK_VAL_NIL, 0 };
xp_awk_val_t* xp_awk_val_nil = (xp_awk_val_t*)&__awk_nil;

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

xp_awk_val_t* xp_awk_makeintval (xp_awk_t* awk, xp_long_t v)
{
	xp_awk_val_int_t* val;

	if (v >= __awk_int[0].val && 
	    v <= __awk_int[xp_countof(__awk_int)-1].val)
	{
		return (xp_awk_val_t*)&__awk_int[v-__awk_int[0].val];
	}

	if (awk->run.icache_count > 0)
	{
		val = awk->run.icache[--awk->run.icache_count];
	}
	else
	{
		val = (xp_awk_val_int_t*)xp_malloc(xp_sizeof(xp_awk_val_int_t));
		if (val == XP_NULL) return XP_NULL;
	}

	val->type = XP_AWK_VAL_INT;
	val->ref = 0;
	val->val = v;

	return (xp_awk_val_t*)val;
}

xp_awk_val_t* xp_awk_makerealval (xp_real_t v)
{
	xp_awk_val_real_t* val;

	val = (xp_awk_val_real_t*)xp_malloc(xp_sizeof(xp_awk_val_real_t));
	if (val == XP_NULL) return XP_NULL;

	val->type = XP_AWK_VAL_REAL;
	val->ref = 0;
	val->val = v;

	return (xp_awk_val_t*)val;
}

xp_awk_val_t* xp_awk_makestrval (const xp_char_t* str, xp_size_t len)
{
	xp_awk_val_str_t* val;

	val = (xp_awk_val_str_t*)xp_malloc(xp_sizeof(xp_awk_val_str_t));
	if (val == XP_NULL) return XP_NULL;

	val->type = XP_AWK_VAL_STR;
	val->ref = 0;
	val->len = len;
	val->buf = xp_strxdup (str, len);
	if (val->buf == XP_NULL) {
		xp_free (val);
		return XP_NULL;
	}

	return (xp_awk_val_t*)val;
}

xp_bool_t xp_awk_isbuiltinval (xp_awk_val_t* val)
{
	return val == XP_NULL || val == xp_awk_val_nil ||
	       (val >= (xp_awk_val_t*)&__awk_int[0] &&
	        val <= (xp_awk_val_t*)&__awk_int[xp_countof(__awk_int)-1]);
}

void xp_awk_freeval (xp_awk_t* awk, xp_awk_val_t* val)
{
	if (xp_awk_isbuiltinval(val)) return;

	switch (val->type)
	{
	case XP_AWK_VAL_NIL:
		xp_free (val);
		break;

	case XP_AWK_VAL_INT:
		if (awk->run.icache_count < xp_countof(awk->run.icache))
		{
			awk->run.icache[awk->run.icache_count++] = 
				(xp_awk_val_int_t*)val;	
		}
		else
		{
			xp_free (val);
		}
		break;

	case XP_AWK_VAL_REAL:
		xp_free (val);
		break;

	case XP_AWK_VAL_STR:
		xp_free (((xp_awk_val_str_t*)val)->buf);
		xp_free (val);
		break;
	}

	/* should never reach here */
}

void xp_awk_refupval (xp_awk_val_t* val)
{
	if (xp_awk_isbuiltinval(val)) return;
/*
xp_printf (XP_TEXT("ref up "));
xp_awk_printval (val);
xp_printf (XP_TEXT("\n"));
*/
	val->ref++;
}

void xp_awk_refdownval (xp_awk_t* awk, xp_awk_val_t* val)
{
	if (xp_awk_isbuiltinval(val)) return;

/*
xp_printf (XP_TEXT("ref down [count=>%d]\n"), val->ref);
xp_awk_printval (val);
xp_printf (XP_TEXT("\n"));
*/

	xp_assert (val->ref > 0);
	val->ref--;
	if (val->ref <= 0) 
	{
/*
xp_printf (XP_TEXT("**FREEING "));
xp_awk_printval (val);
xp_printf (XP_TEXT("\n"));
*/
		xp_awk_freeval(awk, val);
	}
}

void xp_awk_refdownval_nofree (xp_awk_t* awk, xp_awk_val_t* val)
{
	if (xp_awk_isbuiltinval(val)) return;

	xp_assert (val->ref > 0);
	val->ref--;
}

xp_awk_val_t* xp_awk_cloneval (xp_awk_t* awk, xp_awk_val_t* val)
{
	if (val == XP_NULL) return xp_awk_val_nil;

	switch (val->type)
	{
	case XP_AWK_VAL_NIL:
		return xp_awk_val_nil;
	case XP_AWK_VAL_INT:
		return xp_awk_makeintval (awk, ((xp_awk_val_int_t*)val)->val);
	case XP_AWK_VAL_REAL:
		return xp_awk_makerealval (((xp_awk_val_real_t*)val)->val);
	case XP_AWK_VAL_STR:
		return xp_awk_makestrval (
			((xp_awk_val_str_t*)val)->buf,
			((xp_awk_val_str_t*)val)->len);
	}

	return XP_NULL;
}

xp_bool_t xp_awk_isvaltrue (xp_awk_val_t* val)
{
	if (val == XP_NULL) return xp_false;

	switch (val->type)
	{
	case XP_AWK_VAL_NIL:
		return xp_false;
	case XP_AWK_VAL_INT:
		return (((xp_awk_val_int_t*)val)->val == 0)? xp_false: xp_true;
	case XP_AWK_VAL_REAL:
		return (((xp_awk_val_real_t*)val)->val == 0.0)? xp_false: xp_true;
	case XP_AWK_VAL_STR:
		/* TODO: decide what to do */
		return (((xp_awk_val_str_t*)val)->len == 0)? xp_false: xp_true;
	}

	/* this should never happen */
	return xp_false;
}

void xp_awk_printval (xp_awk_val_t* val)
{
// TODO: better value printing......................
	switch (val->type)
	{
	case XP_AWK_VAL_NIL:
		xp_printf (XP_TEXT("nil"));
	       	break;

	case XP_AWK_VAL_INT:
#if defined(__LCC__)
		xp_printf (XP_TEXT("%lld"), 
			(long long)((xp_awk_val_int_t*)val)->val);
#elif defined(__BORLANDC__) || defined(_MSC_VER)
		xp_printf (XP_TEXT("%I64d"), 
			(__int64)((xp_awk_nde_int_t*)val)->val);
#elif defined(vax) || defined(__vax)
		xp_printf (XP_TEXT("%ld"), 
			(long)((xp_awk_val_int_t*)val)->val);
#else
		xp_printf (XP_TEXT("%lld"), 
			(long long)((xp_awk_val_int_t*)val)->val);
#endif
		break;

	case XP_AWK_VAL_REAL:
		xp_printf (XP_TEXT("%lf"), 
			(long double)((xp_awk_val_real_t*)val)->val);
		break;

	case XP_AWK_VAL_STR:
		xp_printf (XP_TEXT("%s"), ((xp_awk_val_str_t*)val)->buf);
		break;

	default:
		xp_printf (XP_TEXT("**** INTERNAL ERROR - UNKNOWN VALUE TYPE ****\n"));
	}
}
