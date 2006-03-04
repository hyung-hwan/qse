/*
 * $Id: val.c,v 1.1 2006-03-04 15:54:37 bacon Exp $
 */

#include <xp/awk/awk.h>

xp_awk_val_t* xp_awk_makeintval (xp_long_t v)
{
	xp_awk_val_int_t* val;

	val = xp_malloc (xp_sizeof(xp_awk_val_int_t));
	if (val == XP_NULL) return XP_NULL;

	val->val = v;
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

void xp_awk_printval (xp_awk_val_t* val)
{
// TODO: better value printing......................
	switch (val->type)
	{
	case XP_AWK_VAL_INT:
		xp_printf (XP_TEXT("%lld"), 
			(long long)((xp_awk_val_int_t*)val)->val);
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
		break;
	}
}
