/*
 * $Id: func.c,v 1.55 2006-10-02 14:53:44 bacon Exp $
 */

#include <xp/awk/awk_i.h>

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
static int __bfn_gsub (xp_awk_t* awk, void* run);
static int __bfn_sub (xp_awk_t* awk, void* run);
static int __substitute (xp_awk_t* awk, void* run, xp_long_t max_count);
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
	{ XP_T("gsub"),    4, 0,            2,  3,  XP_T("xvr"), __bfn_gsub },
	{ XP_T("sub"),     3, 0,            2,  3,  XP_T("xvr"), __bfn_sub },

	/* TODO: remove these two functions */
	{ XP_T("system"),  6, 0,            1,  1,  XP_NULL, __bfn_system },
	{ XP_T("sin"),     3, 0,            1,  1,  XP_NULL, __bfn_sin },

	{ XP_NULL,         0, 0,            0,  0,  XP_NULL, XP_NULL }
};

xp_awk_bfn_t* xp_awk_addbfn (
	xp_awk_t* awk, const xp_char_t* name, xp_size_t name_len, 
	int when_valid, xp_size_t min_args, xp_size_t max_args, 
	const xp_char_t* arg_spec, int (*handler)(xp_awk_t*,void*))
{
	xp_awk_bfn_t* p;

/* TODO: complete this function??? */

	p = (xp_awk_bfn_t*) XP_AWK_MALLOC (awk, xp_sizeof(xp_awk_bfn_t));
	if (p == XP_NULL) return XP_NULL;

	/* NOTE: make sure that name is a constant string */
	p->name = name;  
	p->name_len = name_len;
	p->valid = when_valid;
	p->min_args = min_args;
	p->max_args = max_args;
	p->arg_spec = arg_spec;
	p->handler = handler;

	p->next = awk->bfn.user;
	awk->bfn.user = p;

	return p;
}

int xp_awk_delbfn (xp_awk_t* awk, const xp_char_t* name, xp_size_t name_len)
{
	xp_awk_bfn_t* p, * pp = XP_NULL;

	for (p = awk->bfn.user; p != XP_NULL; p++)
	{
		if (xp_awk_strxncmp(p->name, p->name_len, name, name_len) == 0)
		{
			if (pp == XP_NULL)
				awk->bfn.user = p->next;
			else pp->next = p->next;

			XP_AWK_FREE (awk, p);
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
		XP_AWK_FREE (awk, p);
		p = np;
	}

	awk->bfn.user = XP_NULL;
}

xp_awk_bfn_t* xp_awk_getbfn (
	xp_awk_t* awk, const xp_char_t* name, xp_size_t name_len)
{
	xp_awk_bfn_t* p;

	for (p = __sys_bfn; p->name != XP_NULL; p++)
	{
		if (p->valid != 0 && 
		    (awk->option & p->valid) == 0) continue;

		if (xp_awk_strxncmp (
			p->name, p->name_len, name, name_len) == 0) return p;
	}

/* TODO: user-added builtin functions... */
	for (p = awk->bfn.user; p != XP_NULL; p = p->next)
	{
		if (p->valid != 0 && 
		    (awk->option & p->valid) == 0) continue;

		if (xp_awk_strxncmp (
			p->name, p->name_len, name, name_len) == 0) return p;
	}

	return XP_NULL;
}

static int __bfn_close (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_awk_val_t* v, * a0;
	int n;

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
		name = xp_awk_valtostr (run, a0, xp_true, XP_NULL, &len);
		if (name == XP_NULL) return -1;
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
		if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, name);
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
			if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, name);
			n = -1;
			/* TODO: need to set ERRNO??? */
			goto skip_close;
		}
	}	

	n = xp_awk_closeextio (run, name);
	if (n == -1 && ((xp_awk_run_t*)run)->errnum != XP_AWK_EIOHANDLER)
	{
		if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, name);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, name);

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

static int __flush_extio (
	xp_awk_run_t* run, int extio, const xp_char_t* name, int n)
{
	int n2;

	if (run->extio.handler[extio] != XP_NULL)
	{
		n2 = xp_awk_flushextio (run, extio, name);
		if (n2 == -1)
		{
			if (run->errnum == XP_AWK_EIOHANDLER) n = -1;
			else if (run->errnum == XP_AWK_ENOSUCHIO) 
			{
				if (n != 0) n = -2;
			}
			else n = -99; 
		}
		else if (n != -1) n = 0;
	}

	return n;
}

static int __bfn_fflush (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_awk_val_t* a0;
	xp_char_t* str0;
	xp_size_t len0;
	int n;
       
	nargs = xp_awk_getnargs (run);
	xp_assert (nargs >= 0 && nargs <= 1);

	if (nargs == 0)
	{
		/* flush the console output */
		n = xp_awk_flushextio (run, XP_AWK_OUT_CONSOLE, XP_T(""));
		if (n == -1 && 
		    ((xp_awk_run_t*)run)->errnum != XP_AWK_EIOHANDLER && 
		    ((xp_awk_run_t*)run)->errnum != XP_AWK_ENOSUCHIO)
		{
			return -1;
		}

		/* fflush() should return -1 on EIOHANDLER and ENOSUCHIO */
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
				run, a0, xp_true, XP_NULL, &len0);
			if (str0 == XP_NULL) return -1;

		}

		/* the target name contains a null character.
		 * make fflush return -1 and set ERRNO accordingly */
		ptr = str0; end = str0 + len0;
		while (ptr < end)
		{
			if (*ptr == XP_T('\0')) 
			{
				if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str0);
				n = -1;
				goto skip_flush;
			}

			ptr++;
		}

		/* flush the given extio */
		n = 1;

		n = __flush_extio (
			run, XP_AWK_EXTIO_FILE, 
			((len0 == 0)? XP_NULL: str0), 1);
		if (n == -99) return -1;
		n = __flush_extio (
			run, XP_AWK_EXTIO_PIPE,
			((len0 == 0)? XP_NULL: str0), n);
		if (n == -99) return -1;
		n = __flush_extio (
			run, XP_AWK_EXTIO_COPROC,
			((len0 == 0)? XP_NULL: str0), n);
		if (n == -99) return -1;

		/* if n remains 1, no ip handlers have been defined for
		 * file, pipe, and coproc. so make fflush return -1. 
		 * if n is -2, no such named io has been found at all 
		 * if n is -1, the io handler has returned an error */
		if (n != 0) n = -1;

		if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str0);
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
		str0 = xp_awk_valtostr (run, a0, xp_true, XP_NULL, &len0);
		if (str0 == XP_NULL) return -1;
	}

	if (a1->type == XP_AWK_VAL_STR)
	{
		str1 = ((xp_awk_val_str_t*)a1)->buf;
		len1 = ((xp_awk_val_str_t*)a1)->len;
	}
	else
	{
		str1 = xp_awk_valtostr (run, a1, xp_true, XP_NULL, &len1);
		if (str1 == XP_NULL)
		{
			if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str0);
			return -1;
		}
	}

	ptr = xp_awk_strxnstr (str0, len0, str1, len1);
	idx = (ptr == XP_NULL)? -1: (xp_long_t)(ptr - str0);

	if (xp_awk_getopt(awk) & XP_AWK_STRINDEXONE) idx = idx + 1;

	if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str0);
	if (a1->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str1);

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

	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 1);
	
	v = xp_awk_getarg (run, 0);
	if (v->type == XP_AWK_VAL_STR)
	{
		len = ((xp_awk_val_str_t*)v)->len;
	}
	else
	{
		str = xp_awk_valtostr (run, v, xp_true, XP_NULL, &len);
		if (str == XP_NULL) return -1;

		XP_AWK_FREE (awk, str);
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
	int n;

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
		str = xp_awk_valtostr (run, a0, xp_true, XP_NULL, &len);
		if (str == XP_NULL) return -1;
	}

	n = xp_awk_valtonum (run, a1, &lindex, &rindex);
	if (n == -1) 
	{
		if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str);
		return -1;
	}
	if (n == 1) lindex = (xp_long_t)rindex;

	if (a2 == XP_NULL) lcount = (xp_long_t)len;
	else 
	{
		n = xp_awk_valtonum (run, a2, &lcount, &rcount);
		if (n == -1) 
		{
			if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str);
			return -1;
		}
		if (n == 1) lcount = (xp_long_t)rcount;
	}

	if (xp_awk_getopt(awk) & XP_AWK_STRINDEXONE) lindex = lindex - 1;
	if (lindex >= len) lindex = len;
	else if (lindex < 0) lindex = 0;

	if (lcount < 0) lcount = 0;
	else if (lcount > len - lindex) lcount = len - lindex;

	r = xp_awk_makestrval (run, &str[lindex], (xp_size_t)lcount);
	if (r == XP_NULL)
	{
		if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str);
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str);
	xp_awk_setretval (run, r);
	return 0;
}

static int __bfn_split (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_awk_val_t* a0, * a1, * a2, * t1, * t2, ** a1_ref;
	xp_char_t* str, * str_free, * p, * tok;
	xp_size_t str_len, str_left, tok_len;
	xp_long_t num;
	xp_char_t key[xp_sizeof(xp_long_t)*8+2];
	xp_size_t key_len;
	xp_char_t* fs_ptr, * fs_free;
	xp_size_t fs_len;
	void* fs_rex = XP_NULL; 
	void* fs_rex_free = XP_NULL;
	int errnum;

	nargs = xp_awk_getnargs (run);
	xp_assert (nargs >= 2 && nargs <= 3);

	a0 = xp_awk_getarg (run, 0);
	a1 = xp_awk_getarg (run, 1);
	a2 = (nargs >= 3)? xp_awk_getarg (run, 2): XP_NULL;

	xp_assert (a1->type == XP_AWK_VAL_REF);

/* TODO: XP_AWK_VAL_REF_POS */
	if (((xp_awk_val_ref_t*)a1)->id >= XP_AWK_VAL_REF_NAMEDIDX &&
	    ((xp_awk_val_ref_t*)a1)->id <= XP_AWK_VAL_REF_ARGIDX)
	{
		/* an indexed value should not be assigned another map */
		xp_awk_seterrnum (run, XP_AWK_EIDXVALASSMAP);
		return -1;
	}

	a1_ref = (xp_awk_val_t**)((xp_awk_val_ref_t*)a1)->adr;
	if ((*a1_ref)->type != XP_AWK_VAL_NIL &&
	    (*a1_ref)->type != XP_AWK_VAL_MAP)
	{
		/* cannot change a scalar value to a map */
		xp_awk_seterrnum (run, XP_AWK_ESCALARTOMAP);
		return -1;
	}

	if (a0->type == XP_AWK_VAL_STR)
	{
		str = ((xp_awk_val_str_t*)a0)->buf;
		str_len = ((xp_awk_val_str_t*)a0)->len;
		str_free = XP_NULL;
	}
	else 
	{
		str = xp_awk_valtostr (run, a0, xp_true, XP_NULL, &str_len);
		if (str == XP_NULL) return -1;
		str_free = str;
	}

	if (a2 == XP_NULL)
	{
		/* get the value from FS */
		t1 = xp_awk_getglobal (run, XP_AWK_GLOBAL_FS);
		if (t1->type == XP_AWK_VAL_NIL)
		{
			fs_ptr = XP_T(" ");
			fs_len = 1;
			fs_free = XP_NULL;
		}
		else if (t1->type == XP_AWK_VAL_STR)
		{
			fs_ptr = ((xp_awk_val_str_t*)t1)->buf;
			fs_len = ((xp_awk_val_str_t*)t1)->len;
			fs_free = XP_NULL;
		}
		else
		{
			fs_ptr = xp_awk_valtostr (
				run, t1, xp_true, XP_NULL, &fs_len);
			if (fs_ptr == XP_NULL)
			{
				if (str_free != XP_NULL) 
					XP_AWK_FREE (awk, str_free);
				return -1;
			}
			fs_free = fs_ptr;
		}

		if (fs_len > 1) 
		{
			fs_rex = ((xp_awk_run_t*)run)->global.fs;
			fs_rex_free = XP_NULL;
		}
	}
	else
	{
		if (a2->type == XP_AWK_VAL_STR)
		{
			fs_ptr = ((xp_awk_val_str_t*)a2)->buf;
			fs_len = ((xp_awk_val_str_t*)a2)->len;
			fs_free = XP_NULL;
		}
		else
		{
			fs_ptr = xp_awk_valtostr (
				run, a2, xp_true, XP_NULL, &fs_len);
			if (fs_ptr == XP_NULL)
			{
				if (str_free != XP_NULL) 
					XP_AWK_FREE (awk, str_free);
				return -1;
			}
			fs_free = fs_ptr;
		}

		if (fs_len > 1) 
		{
			fs_rex = xp_awk_buildrex (awk, fs_ptr, fs_len, &errnum);
			if (fs_rex == XP_NULL)
			{
				if (str_free != XP_NULL) 
					XP_AWK_FREE (awk, str_free);
				if (fs_free != XP_NULL) 
					XP_AWK_FREE (awk, fs_free);
				xp_awk_seterrnum (run, errnum);
				return -1;
			}
			fs_rex_free = fs_rex;
		}
	}

	t1 = xp_awk_makemapval (run);
	if (t1 == XP_NULL)
	{
		if (str_free != XP_NULL) XP_AWK_FREE (awk, str_free);
		if (fs_free != XP_NULL) XP_AWK_FREE (awk, fs_free);
		if (fs_rex_free != XP_NULL) xp_awk_freerex (awk, fs_rex_free);
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_refdownval (run, *a1_ref);
	*a1_ref = t1;
	xp_awk_refupval (*a1_ref);

	p = str; str_left = str_len; num = 0;
	while (p != XP_NULL)
	{
		if (fs_len <= 1)
		{
			p = xp_awk_strxntok (run, 
				p, str_len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = xp_awk_strxntokbyrex (run, p, str_len, 
				fs_rex, &tok, &tok_len, &errnum); 
			if (p == XP_NULL && errnum != XP_AWK_ENOERR)
			{
				if (str_free != XP_NULL) 
					XP_AWK_FREE (awk, str_free);
				if (fs_free != XP_NULL) 
					XP_AWK_FREE (awk, fs_free);
				if (fs_rex_free != XP_NULL) 
					xp_awk_freerex (awk, fs_rex_free);
				xp_awk_seterrnum (run, errnum);
				return -1;
			}
		}

		if (num == 0 && p == XP_NULL && tok_len == 0) 
		{
			/* no field at all*/
			break; 
		}	

		xp_assert ((tok != XP_NULL && tok_len > 0) || tok_len == 0);

		/* create the field string */
		t2 = xp_awk_makestrval (run, tok, tok_len);
		if (t2 == XP_NULL)
		{
			if (str_free != XP_NULL) XP_AWK_FREE (awk, str_free);
			if (fs_free != XP_NULL) XP_AWK_FREE (awk, fs_free);
			if (fs_rex_free != XP_NULL) xp_awk_freerex (awk, fs_rex_free);
			xp_awk_seterrnum (run, XP_AWK_ENOMEM);
			return -1;
		}

		/* put it into the map */
		key_len = xp_awk_longtostr (
			num, 10, XP_NULL, key, xp_countof(key));
		xp_assert (key_len != (xp_size_t)-1);

		if (xp_awk_map_putx (
			((xp_awk_val_map_t*)t1)->map, 
			key, key_len, t2, XP_NULL) == -1)
		{
			if (str_free != XP_NULL) XP_AWK_FREE (awk, str_free);
			if (fs_free != XP_NULL) XP_AWK_FREE (awk, fs_free);
			if (fs_rex_free != XP_NULL) xp_awk_freerex (awk, fs_rex_free);
			xp_awk_seterrnum (run, XP_AWK_ENOMEM);
			return -1;
		}

		/* don't forget to update the reference count 
		 * when you handle the assignment-like situation
		 * with the internal data structures */
		xp_awk_refupval (t2);

		num++;
		str_len = str_left - (p - str);
	}

	if (str_free != XP_NULL) XP_AWK_FREE (awk, str_free);
	if (fs_free != XP_NULL) XP_AWK_FREE (awk, fs_free);
	if (fs_rex_free != XP_NULL) xp_awk_freerex (awk, fs_rex_free);

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
		str = xp_awk_valtostr (run, a0, xp_true, XP_NULL, &len);
		if (str == XP_NULL) return -1;
	}

	for (i = 0; i < len; i++) str[i] = XP_AWK_TOLOWER (awk, str[i]);	

	r = xp_awk_makestrval (run, str, len);
	if (r == XP_NULL)
	{
		if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str);
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str);
	xp_awk_setretval (run, r);
	return 0;
}

static int __bfn_toupper (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_char_t* str;
	xp_size_t len, i;
	xp_awk_val_t* a0, * r;

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
		str = xp_awk_valtostr (run, a0, xp_true, XP_NULL, &len);
		if (str == XP_NULL) return -1;
	}

	for (i = 0; i < len; i++) str[i] = XP_AWK_TOUPPER (awk, str[i]);	

	r = xp_awk_makestrval (run, str, len);
	if (r == XP_NULL)
	{
		if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str);
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_STR) XP_AWK_FREE (awk, str);
	xp_awk_setretval (run, r);
	return 0;
}

static int __bfn_gsub (xp_awk_t* awk, void* run)
{
	return __substitute (awk, run, 0);
}

static int __bfn_sub (xp_awk_t* awk, void* run)
{
	return __substitute (awk, run, 1);
}

static int __substitute (xp_awk_t* awk, void* run, xp_long_t max_count)
{
	xp_awk_run_t* r = run;
	xp_size_t nargs;
	xp_awk_val_t* a0, * a1, * a2, ** a2_ref, * v;
	xp_char_t* a0_ptr, * a1_ptr, * a2_ptr;
	xp_size_t a0_len, a1_len, a2_len;
	xp_char_t* a0_ptr_free = XP_NULL;
	xp_char_t* a1_ptr_free = XP_NULL;
	xp_char_t* a2_ptr_free = XP_NULL;
	void* rex;
	int opt, n;
	const xp_char_t* cur_ptr, * mat_ptr;
	xp_size_t cur_len, mat_len, i, m;
	xp_awk_str_t new;
	xp_long_t sub_count;

	nargs = xp_awk_getnargs (run);
	xp_assert (nargs >= 2 && nargs <= 3);

	a0 = xp_awk_getarg (run, 0);
	a1 = xp_awk_getarg (run, 1);
	a2 = (nargs >= 3)? xp_awk_getarg (run, 2): XP_NULL;

	xp_assert (a2 == XP_NULL || a2->type == XP_AWK_VAL_REF);

#define FREE_A_PTRS(awk) \
	do { \
		if (a2_ptr_free != XP_NULL) XP_AWK_FREE (awk, a2_ptr_free); \
		if (a1_ptr_free != XP_NULL) XP_AWK_FREE (awk, a1_ptr_free); \
		if (a0_ptr_free != XP_NULL) XP_AWK_FREE (awk, a0_ptr_free); \
	} while (0)
#define FREE_A0_REX(awk,rex) \
	do { \
		if (a0->type != XP_AWK_VAL_REX) xp_awk_freerex (awk, rex); \
	} while (0)

	if (a0->type == XP_AWK_VAL_REX)
	{
		rex = ((xp_awk_val_rex_t*)a0)->code;
	}
	else if (a0->type == XP_AWK_VAL_STR)
	{
		a0_ptr = ((xp_awk_val_str_t*)a0)->buf;
		a0_len = ((xp_awk_val_str_t*)a0)->len;
	}
	else
	{
		a0_ptr = xp_awk_valtostr (run, a0, xp_true, XP_NULL, &a0_len);
		if (a0_ptr == XP_NULL) 
		{
			FREE_A_PTRS (awk);
			return -1;
		}	
		a0_ptr_free = a0_ptr;
	}

	if (a1->type == XP_AWK_VAL_STR)
	{
		a1_ptr = ((xp_awk_val_str_t*)a1)->buf;
		a1_len = ((xp_awk_val_str_t*)a1)->len;
	}
	else
	{
		a1_ptr = xp_awk_valtostr (run, a1, xp_true, XP_NULL, &a1_len);
		if (a1_ptr == XP_NULL) 
		{
			FREE_A_PTRS (awk);
			return -1;
		}
		a1_ptr_free = a1_ptr;
	}

	if (a2 == XP_NULL)
	{
		/* is this correct? any needs to use inrec.d0? */
		a2_ptr = XP_AWK_STR_BUF(&r->inrec.line);
		a2_len = XP_AWK_STR_LEN(&r->inrec.line);
	}
	else if (((xp_awk_val_ref_t*)a2)->id == XP_AWK_VAL_REF_POS)
	{
		xp_size_t idx;
	       
		idx = (xp_size_t)((xp_awk_val_ref_t*)a2)->adr;
		if (idx == 0)
		{
			a2_ptr = XP_AWK_STR_BUF(&r->inrec.line);
			a2_len = XP_AWK_STR_LEN(&r->inrec.line);
		}
		else if (idx <= r->inrec.nflds)
		{
			a2_ptr = r->inrec.flds[idx-1].ptr;
			a2_len = r->inrec.flds[idx-1].len;
		}
		else
		{
			a2_ptr = XP_T("");
			a2_len = 0;
		}
	}
	else
	{
		a2_ref = (xp_awk_val_t**)((xp_awk_val_ref_t*)a2)->adr;

		if ((*a2_ref)->type == XP_AWK_VAL_MAP)
		{
			FREE_A_PTRS (awk);
			/* a map is not allowed as the third parameter */
			xp_awk_seterrnum (run, XP_AWK_EMAPNOTALLOWED);
			return -1;
		}

		if ((*a2_ref)->type == XP_AWK_VAL_STR)
		{
			a2_ptr = ((xp_awk_val_str_t*)(*a2_ref))->buf;
			a2_len = ((xp_awk_val_str_t*)(*a2_ref))->len;
		}
		else
		{
			a2_ptr = xp_awk_valtostr (
				r, *a2_ref, xp_true, XP_NULL, &a2_len);
			if (a2_ptr == XP_NULL) 
			{
				FREE_A_PTRS (awk);
				return -1;
			}
			a2_ptr_free = a2_ptr;
		}
	}

	if (xp_awk_str_open (&new, a2_len, awk) == XP_NULL)
	{
		FREE_A_PTRS (awk);
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != XP_AWK_VAL_REX)
	{
		rex = xp_awk_buildrex (awk, 
			a0_ptr, a0_len, &((xp_awk_run_t*)run)->errnum);
		if (rex == XP_NULL)
		{
			xp_awk_str_close (&new);
			FREE_A_PTRS (awk);
			return -1;
		}
	}

	opt = (((xp_awk_run_t*)run)->global.ignorecase)? XP_AWK_REX_IGNORECASE: 0;
	cur_ptr = a2_ptr;
	cur_len = a2_len;
	sub_count = 0;

	while (1)
	{
		if (max_count == 0 || sub_count < max_count)
		{
			n = xp_awk_matchrex (
				awk, rex, opt, cur_ptr, cur_len,
				&mat_ptr, &mat_len, 
				&((xp_awk_run_t*)run)->errnum);
		}
		else n = 0;

		if (n == -1)
		{
			FREE_A0_REX (awk, rex);
			xp_awk_str_close (&new);
			FREE_A_PTRS (awk);
			return -1;
		}

		if (n == 0) 
		{ 
			/* no more match found */
			if (xp_awk_str_ncat (
				&new, cur_ptr, cur_len) == (xp_size_t)-1)
			{
				FREE_A0_REX (awk, rex);
				xp_awk_str_close (&new);
				FREE_A_PTRS (awk);
				return -1;
			}
			break;
		}

		if (xp_awk_str_ncat (
			&new, cur_ptr, mat_ptr - cur_ptr) == (xp_size_t)-1)
		{
			FREE_A0_REX (awk, rex);
			xp_awk_str_close (&new);
			FREE_A_PTRS (awk);
			return -1;
		}

		for (i = 0; i < a1_len; i++)
		{
			if ((i+1) < a1_len && 
			    a1_ptr[i] == XP_T('\\') && 
			    a1_ptr[i+1] == XP_T('&'))
			{
				m = xp_awk_str_ccat (&new, XP_T('&'));
				i++;
			}
			else if (a1_ptr[i] == XP_T('&'))
			{
				m = xp_awk_str_ncat (&new, mat_ptr, mat_len);
			}
			else 
			{
				m = xp_awk_str_ccat (&new, a1_ptr[i]);
			}

			if (m == (xp_size_t)-1)
			{
				FREE_A0_REX (awk, rex);
				xp_awk_str_close (&new);
				FREE_A_PTRS (awk);
				return -1;
			}
		}

		sub_count++;
		cur_len = cur_len - ((mat_ptr - cur_ptr) + mat_len);
		cur_ptr = mat_ptr + mat_len;
	}

	FREE_A0_REX (awk, rex);

	if (sub_count > 0)
	{
		if (a2 == XP_NULL)
		{
			if (xp_awk_setrecord (run,
				XP_AWK_STR_BUF(&new), XP_AWK_STR_LEN(&new)) == -1)
			{
				xp_awk_str_close (&new);
				FREE_A_PTRS (awk);
				return -1;
			}
		}
		else if (((xp_awk_val_ref_t*)a2)->id == XP_AWK_VAL_REF_POS)
		{
			int n;
			xp_size_t idx;

			idx = (xp_size_t)((xp_awk_val_ref_t*)a2)->adr;
			if (idx == 0)
			{
				n = xp_awk_setrecord (run, 
					XP_AWK_STR_BUF(&new), XP_AWK_STR_LEN(&new));
			}
			else
			{
				n = xp_awk_setfield (run, idx, 
					XP_AWK_STR_BUF(&new), XP_AWK_STR_LEN(&new));
			}

			if (n == -1)
			{
				xp_awk_str_close (&new);
				FREE_A_PTRS (awk);
				return -1;
			}
		}
		else
		{
			v = xp_awk_makestrval (run,
				XP_AWK_STR_BUF(&new), XP_AWK_STR_LEN(&new));
			if (v == XP_NULL)
			{
				xp_awk_str_close (&new);
				FREE_A_PTRS (awk);
				return -1;
			}

			xp_awk_refdownval (run, *a2_ref);
			*a2_ref = v;
			xp_awk_refupval (*a2_ref);
		}
	}

	xp_awk_str_close (&new);
	FREE_A_PTRS (awk);

#undef FREE_A0_REX
#undef FREE_A_PTRS

	v = xp_awk_makeintval (run, sub_count);
	if (v == XP_NULL)
	{
		xp_awk_seterrnum (run, XP_AWK_ENOMEM);
		return -1;
	}

	xp_awk_setretval (run, v);
	return 0;
}

static int __bfn_system (xp_awk_t* awk, void* run)
{
	xp_size_t nargs;
	xp_char_t* cmd;
	xp_awk_val_t* v;
	int n;
       
	nargs = xp_awk_getnargs (run);
	xp_assert (nargs == 1);

	cmd = xp_awk_valtostr (
		run, xp_awk_getarg(run, 0), xp_true, XP_NULL, XP_NULL);
	if (cmd == XP_NULL) return -1;

#ifdef _WIN32
	n = _tsystem (cmd);
#else
	/* TODO: support system on other platforms that win32 */
	n = -1;
#endif

	XP_AWK_FREE (awk, cmd);

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

	n = xp_awk_valtonum (run, xp_awk_getarg(run, 0), &lv, &rv);
	if (n == -1)
	{
		/* wrong value */
		return  -1;
	}

	if (n == 0) rv = (xp_real_t)lv;

#if (XP_SIZEOF_REAL == XP_SIZEOF_LONG_DOUBLE)
	v = xp_awk_makerealval (run, (xp_real_t)sinl(rv));
#elif (XP_SIZEOF_REAL == XP_SIZEOF_DOUBLE)
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
