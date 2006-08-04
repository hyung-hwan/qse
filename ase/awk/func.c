/*
 * $Id: func.c,v 1.17 2006-08-04 17:36:40 bacon Exp $
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

	{ XP_T("close"),   5,  XP_AWK_EXTIO,  1,  1,  __bfn_close },
	{ XP_T("system"),  6,  0,             1,  1,  __bfn_system },

	{ XP_T("sin"),     3,  0,             1,  1,  __bfn_sin },

	{ XP_NULL,         0,  0,             0,  0,  XP_NULL }
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
		    (awk->option & p->valid) == 0) continue;

		if (xp_strcmp (p->name, name) == 0) return p;
	}

/* TODO: */
	for (p = awk->bfn.user; p != XP_NULL; p = p->next)
	{
		if (p->valid != 0 && 
		    (awk->option & p->valid) == 0) continue;

		if (xp_strcmp (p->name, name) == 0) return p;
	}

	return XP_NULL;
}

static int __bfn_close (void* run)
{
	xp_size_t nargs;
	xp_str_t buf;
	xp_awk_val_t* v, * a0;
	int errnum, n;

	xp_char_t* name;
	xp_size_t len;
       
	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 1);
/* TODO: support close (xxx, "to"/"from") like gawk */

	a0 = xp_awk_getarg(run, 0);
	xp_assert (a0 != XP_NULL);

	if (a0->type == XP_AWK_VAL_STR)
	{
		name = ((xp_awk_val_str_t*)a0)->buf;
		len = ((xp_awk_val_str_t*)a0)->len;
	}
	else
	{
		if (xp_str_open (&buf, 256) == XP_NULL)
		{
			xp_awk_seterrnum (run, XP_AWK_ENOMEM);
			return -1;
		}

		if (xp_awk_valtostr (
			a0, &errnum, xp_true, &buf, XP_NULL) == XP_NULL)
		{
			xp_str_close (&buf);
			xp_awk_seterrnum (run, errnum);
			return -1;
		}

		name = XP_STR_BUF(&buf);
		len = XP_STR_LEN(&buf);
	}

	if (len == 0)
	{
		/* getline or print doesn't allow an emptry for the 
		 * input or output file name. so close should not allow 
		 * it either.  
		 * another reason for this is if close is called explicitly 
		 * with an empty string, it may close the console that uses 
		 * an empty string for its identification because closeextio
		 * closes any extios that match the name given unlike 
		 * closeextio_read or closeextio_write. */ 
		if (a0->type != XP_AWK_VAL_STR) xp_str_close (&buf);
		n = -1;
		/* TODO: need to set ERRNO??? */
		goto skip_close;
	}

	while (len > 0)
	{
		if (name[--len] == XP_T('\0'))
		{
			/* the name contains a null string. 
			 * make close return -1 */
			if (a0->type != XP_AWK_VAL_STR) xp_str_close (&buf);
			n = -1;
			/* TODO: need to set ERRNO??? */
			goto skip_close;
		}
	}	

	n = xp_awk_closeextio (run, name, &errnum);
	if (n == -1 && errnum != XP_AWK_ENOERR)
	{
		if (a0->type != XP_AWK_VAL_STR) xp_str_close (&buf);
		xp_awk_seterrnum (run, errnum);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_STR) xp_str_close (&buf);

skip_close:
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
		xp_awk_getarg(run, 0), &errnum, xp_true, XP_NULL, XP_NULL);
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
