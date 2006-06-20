/*
 * $Id: func.c,v 1.2 2006-06-20 15:27:50 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/string.h>
#endif

static int __bfn_close (void* run);

static xp_awk_bfn_t __bfn[] = 
{
	{ XP_T("system"), 0,            1, 1, XP_NULL },
	{ XP_T("close"),  XP_AWK_EXTIO, 1, 2, __bfn_close },
	{ XP_NULL,        0,            0, 0, XP_NULL }
};

xp_awk_bfn_t* xp_awk_getbfn (const xp_char_t* name)
{
	xp_awk_bfn_t* p = __bfn;

	while (p->name != XP_NULL)
	{
		if (xp_strcmp (p->name, name) == 0) return p;
		p++;
	}

	return XP_NULL;
}

static int __bfn_close (void* run)
{
	xp_size_t nargs, i;
       
	nargs = xp_awk_getnargs (run);
	for (i = 0; i < nargs; i++)
	{
		xp_printf (XP_T("arg %d => "), (int)i);
		xp_awk_printval (xp_awk_getarg (run, i));
		xp_printf (XP_T("\n"));
	}

	xp_awk_setretval (run, xp_awk_makeintval(run,10));
	return 0;
}
