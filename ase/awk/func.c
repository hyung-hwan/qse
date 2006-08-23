/*
 * $Id: func.c,v 1.29 2006-08-23 15:45:26 bacon Exp $
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
	#include <math.h>
#endif

static int __bfn_close (xp_awk_t* awk, void* run);
static int __bfn_fflush (xp_awk_t* awk, void* run);
static int __bfn_index (xp_awk_t* awk, void* run);
static int __bfn_length (xp_awk_t* awk, void* run);
static int __bfn_substr (xp_awk_t* awk, void* run);
static int __bfn_split (xp_awk_t* awk, void* run);
static int __bfn_tolower (xp_awk_t* awk, void* run);
static int __bfn_toupper (xp_awk_t* awk, void* run);
static int __bfn_system (xp_awk_t* awk, void* run);
static int __bfn_sin (xp_awk_t* awk, void* run);

/* TODO: move it under the awk structure... */
static xp_awk_bfn_t __sys_bfn[] = 
{
	/* io functions */
	{ XP_T("close"),   5, XP_AWK_EXTIO, 1,  1,  XP_NULL, __bfn_close },
	{ XP_T("fflush"),  6, XP_AWK_EXTIO, 0,  1,  XP_NULL, __bfn_fflush },

	/* string functions */
	{ XP_T("index"),   5, 0,            2,  2,  XP_NULL, __bfn_index },
	{ XP_T("length"),  6, 0,            1,  1,  XP_NULL, __bfn_length },
	{ XP_T("substr"),  6, 0,            2,  3,  XP_NULL, __bfn_substr },
	{ XP_T("split"),   5, 0,            2,  3,  XP_T("vrv"), __bfn_split },
	{ XP_T("tolower"), 7, 0,            1,  1,  XP_NULL, __bfn_tolower },
	{ XP_T("toupper"), 7, 0,            1,  1,  XP_NULL, __bfn_toupper },

	/* TODO: remove these two functions */
	{ XP_T("system"),  6, 0,            1,  1,  XP_NULL, __bfn_system },
	{ XP_T("sin"),     3, 0,            1,  1,  XP_NULL, __bfn_sin },

	{ XP_NULL,         0, 0,            0,  0,  XP_NULL, XP_NULL }
};

xp_awk_bfn_t* xp_awk_addbfn (
	xp_awk_t* awk, const xp_char_t* name, int when_valid, 
	xp_size_t min_args, xp_size_t max_args, const xp_char_t* arg_spec,
	int (*handler)(xp_awk_t*,void*))
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
	p->arg_spec = arg_spec;
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

static int __bfn_close (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
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
		name = xp_awk_valtostr (a0, &errnum, xp_true, XP_NULL, &len);
		if (name == XP_NULL)
		{
			xp_awk_seterrnum (run, errnum);
			return -1;
		}
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
		if (a0->type != XP_AWK_VAL_STR) xp_free (name);
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
			if (a0->type != XP_AWK_VAL_STR) xp_free (name);
			n = -1;
			/* TODO: need to set ERRNO??? */
			goto skip_close;
		}
	}	

	n = xp_awk_closeextio (run, name, &errnum);
	if (n == -1 && errnum != XP_AWK_EIOHANDLER)
	{
		if (a0->type != XP_AWK_VAL_STR) xp_free (name);
		xp_awk_seterrnum (run, errnum);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_STR) xp_free (name);

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

static int __bfn_fflush (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_awk_val_t* a0;
	xp_char_t* str0;
	xp_size_t len0;
	int errnum, n, n2;
       
	nargs = xp_awk_getnargs (run);
	xp_assert (nargs >= 0 && nargs <= 1);

	if (nargs == 0)
	{
		/* flush the console output */
		n = xp_awk_flushextio (run, 
			XP_AWK_OUT_CONSOLE, XP_T(""), &errnum);
		if (n == -1 && 
		    errnum != XP_AWK_EIOHANDLER && 
		    errnum != XP_AWK_ENOSUCHIO)
		{
			xp_awk_seterrnum (run, errnum);
			return -1;
		}
	}
	else
	{
		xp_char_t* ptr, * end;

		a0 = xp_awk_getarg (run, 0);
		if (a0->type == XP_AWK_VAL_STR)
		{
			str0 = ((xp_awk_val_str_t*)a0)->buf;
			len0 = ((xp_awk_val_str_t*)a0)->len;
		}
		else
		{
			str0 = xp_awk_valtostr (
				a0, &errnum, xp_true, XP_NULL, &len0);
			if (str0 == XP_NULL)
			{
				xp_awk_seterrnum (run, errnum);
				return -1;
			}

		}

		/* the target name contains a null character.
		 * make fflush return -1 and set ERRNO accordingly */
		ptr = str0; end = str0 + len0;
		while (ptr < end)
		{
			if (*ptr == XP_T('\0')) 
			{
				if (a0->type != XP_AWK_VAL_STR) xp_free (str0);
				n = -1;
				goto skip_flush;
			}

			ptr++;
		}

		/* flush the given extio */
		n = 1;

		if (((xp_awk_run_t*)run)->extio.handler[XP_AWK_EXTIO_FILE] != XP_NULL)
		{
			n2 = xp_awk_flushextio (
				run, XP_AWK_OUT_FILE, 
				((len0 == 0)? XP_NULL: str0), &errnum);
			if (n2 == -1)
			{
				if (errnum == XP_AWK_EIOHANDLER) n = -1;
				else if (errnum == XP_AWK_ENOSUCHIO) 
				{
					if (n != 0) n = -2;
				}
				else
				{
					xp_awk_seterrnum (run, errnum);
					return -1;
				}
			}
			else if (n != -1) n = 0;
		}

		if (((xp_awk_run_t*)run)->extio.handler[XP_AWK_EXTIO_PIPE] != XP_NULL)
		{
			n2 = xp_awk_flushextio (
				run, XP_AWK_OUT_PIPE, 
				((len0 == 0)? XP_NULL: str0), &errnum);
			if (n2 == -1)
			{
				if (errnum == XP_AWK_EIOHANDLER) n = -1;
				else if (errnum == XP_AWK_ENOSUCHIO) 
				{
					if (n != 0) n = -2;
				}
				else
				{
					xp_awk_seterrnum (run, errnum);
					return -1;
				}
			}
			else if (n != -1) n = 0;
		}

		if (((xp_awk_run_t*)run)->extio.handler[XP_AWK_EXTIO_COPROC] != XP_NULL)
		{
			n2 = xp_awk_flushextio (
				run, XP_AWK_OUT_COPROC, 
				((len0 == 0)? XP_NULL: str0), &errnum);
			if (n2 == -1)
			{
				if (errnum == XP_AWK_EIOHANDLER) n = -1;
				else if (errnum == XP_AWK_ENOSUCHIO) 
				{
					if (n != 0) n = -2;
				}
				else
				{
					xp_awk_seterrnum (run, errnum);
					return -1;
				}
			}
			else if (n != -1) n = 0;
		}

		/* if n remains 1, no ip handlers have been defined for
		 * file, pipe, and coproc. so make fflush return -1. 
		 * if n is -2, no such named io has been found at all 
		 * if n is -1, the io handler has returned an error */
		if (n != 0) n = -1;

		if (a0->type != XP_AWK_VAL_STR) xp_free (str0);
	}

skip_flush:
	a0 = xp_awk_makeintval (run, (xp_long_t)n);
	if (a0 == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_setretval (run, a0);
	return 0;
}

static int __bfn_index (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_awk_val_t* a0, * a1;
	xp_char_t* str0, * str1, * ptr;
	xp_size_t len0, len1;
	xp_long_t idx;
	int errnum;

	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 2);
	
	a0 = xp_awk_getarg (run, 0);
	a1 = xp_awk_getarg (run, 1);

	if (a0->type == XP_AWK_VAL_STR)
	{
		str0 = ((xp_awk_val_str_t*)a0)->buf;
		len0 = ((xp_awk_val_str_t*)a0)->len;
	}
	else
	{
		str0 = xp_awk_valtostr (a0, &errnum, xp_true, XP_NULL, &len0);
		if (str0 == XP_NULL)
		{
			xp_awk_seterrnum (run, errnum);
			return -1;
		}
	}

	if (a1->type == XP_AWK_VAL_STR)
	{
		str1 = ((xp_awk_val_str_t*)a1)->buf;
		len1 = ((xp_awk_val_str_t*)a1)->len;
	}
	else
	{
		str1 = xp_awk_valtostr (a1, &errnum, xp_true, XP_NULL, &len1);
		if (str1 == XP_NULL)
		{
			xp_awk_seterrnum (run, errnum);
			if (a0->type != XP_AWK_VAL_STR) xp_free (str0);
			return -1;
		}
	}

	ptr = xp_strxnstr (str0, len0, str1, len1);
	idx = (ptr == XP_NULL)? -1: (xp_long_t)(ptr - str0);

	if (xp_awk_getopt(awk) & XP_AWK_STRINDEXONE) idx = idx + 1;

	if (a0->type != XP_AWK_VAL_STR) xp_free (str0);
	if (a1->type != XP_AWK_VAL_STR) xp_free (str1);

	a0 = xp_awk_makeintval (run, idx);
	if (a0 == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_setretval (run, a0);
	return 0;
}

static int __bfn_length (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_awk_val_t* v;
	xp_char_t* str;
	xp_size_t len;
	int errnum;

	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 1);
	
	v = xp_awk_getarg (run, 0);
	if (v->type == XP_AWK_VAL_STR)
	{
		len = ((xp_awk_val_str_t*)v)->len;
	}
	else
	{
		str = xp_awk_valtostr (v, &errnum, xp_true, XP_NULL, &len);
		if (str == XP_NULL)
		{
			xp_awk_seterrnum (run, errnum);
			return -1;
		}

		xp_free (str);
	}

	v = xp_awk_makeintval (run, len);
	if (v == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_setretval (run, v);
	return 0;
}

static int __bfn_substr (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_awk_val_t* a0, * a1, * a2, * r;
	xp_char_t* str;
	xp_size_t len;
	xp_long_t lindex, lcount;
	xp_real_t rindex, rcount;
	int errnum, n;

	nargs = xp_awk_getnargs (run);
	xp_assert (nargs >= 2 && nargs <= 3);

	a0 = xp_awk_getarg (run, 0);
	a1 = xp_awk_getarg (run, 1);
	a2 = (nargs >= 3)? xp_awk_getarg (run, 2): XP_NULL;

	if (a0->type == XP_AWK_VAL_STR)
	{
		str = ((xp_awk_val_str_t*)a0)->buf;
		len = ((xp_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = xp_awk_valtostr (a0, &errnum, xp_true, XP_NULL, &len);
		if (str == XP_NULL)
		{
			xp_awk_seterrnum (run, errnum);
			return -1;
		}
	}

	n = xp_awk_valtonum (a1, &lindex, &rindex);
	if (n == -1) 
	{
		if (a0->type != XP_AWK_VAL_STR) xp_free (str);
		xp_awk_seterrnum (run, XP_AWK_EVALTYPE);
		return -1;
	}
	if (n == 1) lindex = (xp_long_t)rindex;

	if (a2 == XP_NULL) lcount = (xp_long_t)len;
	else 
	{
		n = xp_awk_valtonum (a2, &lcount, &rcount);
		if (n == -1) 
		{
			if (a0->type != XP_AWK_VAL_STR) xp_free (str);
			xp_awk_seterrnum (run, XP_AWK_EVALTYPE);
			return -1;
		}
		if (n == 1) lcount = (xp_long_t)rcount;
	}

	if (xp_awk_getopt(awk) & XP_AWK_STRINDEXONE) lindex = lindex - 1;
	if (lindex >= len) lindex = len;
	else if (lindex < 0) lindex = 0;

	if (lcount < 0) lcount = 0;
	else if (lcount > len - lindex) lcount = len - lindex;

	r = xp_awk_makestrval (&str[lindex], (xp_size_t)lcount);
	if (r == XP_NULL)
	{
		if (a0->type != XP_AWK_VAL_STR) xp_free (str);
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_STR) xp_free (str);
	xp_awk_setretval (run, r);
	return 0;
}

static int __bfn_split (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_awk_val_t* a0, * a1, * a2, * t1, * t2, ** a1r;
	xp_char_t* str, * p, * tok;
	xp_size_t len, left, tok_len;
	xp_long_t num;
	int errnum;
	xp_char_t key[32];

	nargs = xp_awk_getnargs (run);
	xp_assert (nargs >= 2 && nargs <= 3);

	a0 = xp_awk_getarg (run, 0);
	a1 = xp_awk_getarg (run, 1);
	a2 = (nargs >= 3)? xp_awk_getarg (run, 2): XP_NULL;

	xp_assert (a1->type == XP_AWK_VAL_REF);

	if (((xp_awk_val_ref_t*)a1)->id >= XP_AWK_VAL_REF_NAMEDIDX &&
	    ((xp_awk_val_ref_t*)a1)->id <= XP_AWK_VAL_REF_ARGIDX)
	{
		/* an indexed value should not be assigned another map */
		xp_awk_seterrnum (run, XP_AWK_EIDXVALASSMAP);
		return -1;
	}

	a1r = (xp_awk_val_t**)((xp_awk_val_ref_t*)a1)->adr;
	if ((*a1r)->type != XP_AWK_VAL_NIL &&
	    (*a1r)->type != XP_AWK_VAL_MAP)
	{
		/* cannot change a scalar value to a map */
		xp_awk_seterrnum (run, XP_AWK_ESCALARTOMAP);
		return -1;
	}

	if (a0->type == XP_AWK_VAL_STR)
	{
		str = ((xp_awk_val_str_t*)a0)->buf;
		len = ((xp_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = xp_awk_valtostr (a0, &errnum, xp_true, XP_NULL, &len);
		if (str == XP_NULL)
		{
			xp_awk_seterrnum (run, errnum);
			return -1;
		}
	}

	t1 = xp_awk_makemapval (run);
	if (t1 == XP_NULL)
	{
		if (a0->type != XP_AWK_VAL_STR) xp_free (str);
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_refdownval (run, *a1r);
	*a1r = t1;
	xp_awk_refupval (*a1r);

	p = str; left = len; num = 0;
	while (p != XP_NULL)
	{
/* TODO: use FS when a2 is missing. apply a difference scheme */
		p = xp_strxtok (p, left, XP_T(" \t"), &tok, &tok_len);

		if (num == 0 && p == XP_NULL && tok_len == 0) 
		{
			/* no field at all*/
			break; 
		}	

		xp_assert ((tok != XP_NULL && tok_len > 0) || tok_len == 0);

		/* create the field string */
		t2 = xp_awk_makestrval (tok, tok_len);
		if (t2 == XP_NULL)
		{
			if (a0->type != XP_AWK_VAL_STR) xp_free (str);
			xp_awk_seterrnum (run, XP_AWK_ENOMEM);
			return -1;
		}

		/* put it into the map */
/* TODO: remove dependency on xp_sprintf */
	#if defined(__LCC__)
		xp_sprintf (key, xp_countof(key), XP_T("%lld"), (long long)num);
	#elif defined(__BORLANDC__) || defined(_MSC_VER)
		xp_sprintf (key, xp_countof(key), XP_T("%I64d"), (__int64)num);
	#elif defined(vax) || defined(__vax) || defined(_SCO_DS)
		xp_sprintf (key, xp_countof(key), XP_T("%ld"), (long)num);
	#else
		xp_sprintf (key, xp_countof(key), XP_T("%lld"), (long long)num);
	#endif

		if (xp_awk_map_putx (
			((xp_awk_val_map_t*)t1)->map, 
			key, xp_strlen(key), t2, XP_NULL) == -1)
		{
			if (a0->type != XP_AWK_VAL_STR) xp_free (str);
			xp_awk_seterrnum (run, XP_AWK_ENOMEM);
			return -1;
		}

		/* don't forget to update the reference count 
		 * when you handle the assignment-like situation
		 * with the internal data structures */
		xp_awk_refupval (t2);

		num++;
		len = len - (p - str);
	}

	if (a0->type != XP_AWK_VAL_STR) xp_free (str);

	t1 = xp_awk_makeintval (run, num);
	if (t1 == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_setretval (run, t1);
	return 0;
}

static int __bfn_tolower (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_char_t* str;
	xp_size_t len, i;
	xp_awk_val_t* a0, * r;
	int errnum;

	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 1);

	a0 = xp_awk_getarg (run, 0);

	if (a0->type == XP_AWK_VAL_STR)
	{
		str = ((xp_awk_val_str_t*)a0)->buf;
		len = ((xp_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = xp_awk_valtostr (a0, &errnum, xp_true, XP_NULL, &len);
		if (str == XP_NULL)
		{
			xp_awk_seterrnum (run, errnum);
			return -1;
		}
	}

	for (i = 0; i < len; i++) str[i] = xp_tolower(str[i]);	

	r = xp_awk_makestrval (str, len);
	if (r == XP_NULL)
	{
		if (a0->type != XP_AWK_VAL_STR) xp_free (str);
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_STR) xp_free (str);
	xp_awk_setretval (run, r);
	return 0;
}

static int __bfn_toupper (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_char_t* str;
	xp_size_t len, i;
	xp_awk_val_t* a0, * r;
	int errnum;

	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 1);

	a0 = xp_awk_getarg (run, 0);

	if (a0->type == XP_AWK_VAL_STR)
	{
		str = ((xp_awk_val_str_t*)a0)->buf;
		len = ((xp_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = xp_awk_valtostr (a0, &errnum, xp_true, XP_NULL, &len);
		if (str == XP_NULL)
		{
			xp_awk_seterrnum (run, errnum);
			return -1;
		}
	}

	for (i = 0; i < len; i++) str[i] = xp_toupper(str[i]);	

	r = xp_awk_makestrval (str, len);
	if (r == XP_NULL)
	{
		if (a0->type != XP_AWK_VAL_STR) xp_free (str);
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_STR) xp_free (str);
	xp_awk_setretval (run, r);
	return 0;
}

static int __bfn_system (xp_awk_t* awk, void* run)
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
		xp_awk_seterrnum (run, errnum);
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
static int __bfn_sin (xp_awk_t* awk, void* run)
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
