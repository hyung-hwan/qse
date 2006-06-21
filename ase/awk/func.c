/*
 * $Id: func.c,v 1.3 2006-06-21 11:44:55 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/string.h>
#include <xp/bas/str.h>
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
	xp_size_t nargs;
	xp_str_t buf;
	xp_awk_val_t* v;
	int errnum, n;
       
	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 1);

	if (xp_str_open (&buf, 256) == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	if (xp_awk_valtostr (xp_awk_getarg(run, 0), &errnum, &buf) == XP_NULL)
	{
		xp_str_close (&buf);
		xp_awk_seterrnum (run, errnum);
		return -1;
	}

	/*
	n = xp_awk_closeextio (run, XP_STR_BUF(&buf), &errnum);
	if (n == -1 && errnum != XP_AWK_ENOERR)
	{
		xp_str_close (&buf);
		xp_awk_seterrnum (run, errnum);
		return -1;
	}
	*/
n = -1;
xp_printf (XP_T("closing %s\n"), XP_STR_BUF(&buf));

	xp_str_close (&buf);

	v = xp_awk_makeintval (run, n);
	if (v == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_setretval (run, v);
	return 0;
}
