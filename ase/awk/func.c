/*
 * $Id: func.c,v 1.1 2006-06-16 14:31:42 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/string.h>
#endif

enum
{
	BFN_SYSTEM,
	BFN_CLOSE
};

static xp_awk_bfn_t __bfn[] = 
{
	{ XP_T("system"),  BFN_SYSTEM,   0, 1, 1, XP_NULL },
	{ XP_T("close"),   BFN_CLOSE,    0, 1, 2, XP_NULL },
	{ XP_NULL,         0,            0, 0, 0, XP_NULL }
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
