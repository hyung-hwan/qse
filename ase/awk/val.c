/*
 * $Id: val.c,v 1.6 2006-03-15 15:34:59 bacon Exp $
 */

#include <xp/awk/awk.h>

#ifndef __STAND_ALONE
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#endif

static xp_awk_val_nil_t __awk_nil = { XP_AWK_VAL_NIL };
xp_awk_val_t* xp_awk_val_nil = (xp_awk_val_t*)&__awk_nil;

xp_awk_val_t* xp_awk_makeintval (xp_long_t v)
{
	xp_awk_val_int_t* val;

	val = (xp_awk_val_int_t*)xp_malloc(xp_sizeof(xp_awk_val_int_t));
	if (val == XP_NULL) return XP_NULL;

	val->type = XP_AWK_VAL_INT;
	val->val = v;

	return (xp_awk_val_t*)val;
}

xp_awk_val_t* xp_awk_makerealval (xp_real_t v)
{
	xp_awk_val_real_t* val;

	val = (xp_awk_val_real_t*)xp_malloc(xp_sizeof(xp_awk_val_real_t));
	if (val == XP_NULL) return XP_NULL;

	val->type = XP_AWK_VAL_REAL;
	val->val = v;

	return (xp_awk_val_t*)val;
}

xp_awk_val_t* xp_awk_makestrval (const xp_char_t* str, xp_size_t len)
{
	xp_awk_val_str_t* val;

	val = (xp_awk_val_str_t*)xp_malloc(xp_sizeof(xp_awk_val_str_t));
	if (val == XP_NULL) return XP_NULL;

	val->type = XP_AWK_VAL_STR;
	val->len = len;
	val->buf = xp_strxdup (str, len);
	if (val->buf == XP_NULL) {
		xp_free (val);
		return XP_NULL;
	}

	return (xp_awk_val_t*)val;
}

void xp_awk_freeval (xp_awk_val_t* val)
{
	switch (val->type)
	{
	case XP_AWK_VAL_STR:
		xp_free (((xp_awk_val_str_t*)val)->buf);
	default:
		xp_free (val);
	}
}

xp_awk_val_t* xp_awk_cloneval (xp_awk_val_t* val)
{
	if (val == XP_NULL) return xp_awk_val_nil;

	switch (val->type)
	{
	case XP_AWK_VAL_NIL:
		return xp_awk_val_nil;
	case XP_AWK_VAL_INT:
		return xp_awk_makeintval (((xp_awk_val_int_t*)val)->val);
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
