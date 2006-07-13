/*
 * $Id: func.c,v 1.7 2006-07-13 03:10:35 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/string.h>
#include <xp/bas/assert.h>
#include <xp/bas/str.h>
#endif

static int __bfn_close (void* run);

/* TODO: move it under the awk structure... */
static xp_awk_bfn_t __bfn[] = 
{
	{ XP_T("system"), (1 << 5),     1, 1, XP_NULL },
	{ XP_T("close"),  XP_AWK_EXTIO, 1, 1, __bfn_close },
	{ XP_NULL,        0,            0, 0, XP_NULL }
};

xp_awk_bfn_t* xp_awk_addbfn (
	xp_awk_t* awk, const xp_char_t* name, int when_valid,
	xp_size_t min_args, xp_size_t max_args, int (*handler)(void*))
{
	/* TODO */
}

xp_awk_bfn_t* xp_awk_getbfn (xp_awk_t* awk, const xp_char_t* name)
{
	xp_awk_bfn_t* p;

	for (p = __bfn; p->name != XP_NULL; p++)
	{
		if (p->valid != 0 && 
		    (awk->opt.parse & p->valid) == 0) continue;

		if (xp_strcmp (p->name, name) == 0) return p;
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
/* TODO: support close (xxx, "to"/"from") like gawk */

	if (xp_str_open (&buf, 256) == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	if (xp_awk_valtostr (xp_awk_getarg(run, 0), &errnum, &buf, XP_NULL) == XP_NULL)
	{
		xp_str_close (&buf);
		xp_awk_seterrnum (run, errnum);
		return -1;
	}

	n = xp_awk_closeextio (run, XP_STR_BUF(&buf), &errnum);
	if (n == -1 && errnum != XP_AWK_ENOERR)
	{
		xp_str_close (&buf);
		xp_awk_seterrnum (run, errnum);
		return -1;
	}

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
