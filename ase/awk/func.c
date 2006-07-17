/*
 * $Id: func.c,v 1.13 2006-07-17 14:27:09 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/string.h>
#include <xp/bas/assert.h>
#include <xp/bas/str.h>
#endif

#ifdef _WIN32
	#include <tchar.h>
	#include <math.h>
#else
	#include <stdlib.h>
#endif

static int __bfn_close (void* run);
static int __bfn_system (void* run);
static int __bfn_sin (void* run);

/* TODO: move it under the awk structure... */
static xp_awk_bfn_t __sys_bfn[] = 
{
	/* ensure that this matches XP_AWK_BNF_XXX in func.h */

	{ XP_T("close"),  XP_AWK_EXTIO,  1,  1,  __bfn_close },
	{ XP_T("system"), 0,             1,  1,  __bfn_system },

	{ XP_T("sin"),    0,             1,  1,  __bfn_sin },

	{ XP_NULL,        0,             0,  0,  XP_NULL }
};

xp_awk_bfn_t* xp_awk_addbfn (
	xp_awk_t* awk, const xp_char_t* name, int when_valid,
	xp_size_t min_args, xp_size_t max_args, int (*handler)(void*))
{
	xp_awk_bfn_t* p;

/* TODO: complete this function??? */

	p = (xp_awk_bfn_t*) xp_malloc (xp_sizeof(xp_awk_bfn_t));
	if (p == XP_NULL) return XP_NULL;

	/* NOTE: make sure that name is a constant string */
	p->name = name;  
	p->valid = when_valid;
	p->min_args = min_args;
	p->max_args = max_args;
	p->handler = handler;

	p->next = awk->bfn.user;
	awk->bfn.user = p;

	return p;
}

int xp_awk_delbfn (xp_awk_t* awk, const xp_char_t* name)
{
	xp_awk_bfn_t* p, * pp = XP_NULL;

	for (p = awk->bfn.user; p != XP_NULL; p++)
	{
		if (xp_strcmp(p->name, name) == 0)
		{
			if (pp == XP_NULL)
				awk->bfn.user = p->next;
			else pp->next = p->next;

			xp_free (p);
			return 0;
		}

		pp = p;
	}

	return -1;
}

void xp_awk_clrbfn (xp_awk_t* awk)
{
	xp_awk_bfn_t* p, * np;

	p = awk->bfn.user;
	while (p != XP_NULL)
	{
		np = p;
		xp_free (p);
		p = np;
	}

	awk->bfn.user = XP_NULL;
}

xp_awk_bfn_t* xp_awk_getbfn (xp_awk_t* awk, const xp_char_t* name)
{
	xp_awk_bfn_t* p;

	for (p = __sys_bfn; p->name != XP_NULL; p++)
	{
		if (p->valid != 0 && 
		    (awk->opt.parse & p->valid) == 0) continue;

		if (xp_strcmp (p->name, name) == 0) return p;
	}

/* TODO: */
	for (p = awk->bfn.user; p != XP_NULL; p = p->next)
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

	if (xp_awk_valtostr (
		xp_awk_getarg(run, 0), &errnum, &buf, XP_NULL) == XP_NULL)
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

static int __bfn_system (void* run)
{
	xp_size_t nargs;
	xp_char_t* cmd;
	xp_awk_val_t* v;
	int errnum, n;
       
	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 1);

	cmd = xp_awk_valtostr (
		xp_awk_getarg(run, 0), &errnum, XP_NULL, XP_NULL);
	if (cmd == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

#ifdef _WIN32
	n = _tsystem (cmd);
#else
#error NOT SUPPORTED ...
#endif

	xp_free (cmd);

	v = xp_awk_makeintval (run, n);
	if (v == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_setretval (run, v);
	return 0;
}

/* math functions */
static int __bfn_sin (void* run)
{
	xp_size_t nargs;
	xp_awk_val_t* v;
	int n;
	xp_long_t lv;
	xp_real_t rv;
       
	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 1);

	n = xp_awk_valtonum (
		xp_awk_getarg(run, 0), &lv, &rv);
	if (n == -1)
	{
		/* wrong value */
		xp_awk_seterrnum (run, XP_AWK_EVALTYPE);
		return  -1;
	}

	if (n == 0) rv = (xp_real_t)lv;

#if (XP_SIZEOF_LONG_DOUBLE != 0)
	v = xp_awk_makerealval (run, (xp_real_t)sinl(rv));
#elif (XP_SIZEOF_DOUBLE != 0)
	v = xp_awk_makerealval (run, (xp_real_t)sin(rv));
#else
	#error unsupported floating-point data type
#endif
	if (v == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_setretval (run, v);
	return 0;
}
