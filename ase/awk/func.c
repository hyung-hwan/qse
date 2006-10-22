/*
 * $Id: func.c,v 1.67 2006-10-22 12:39:29 bacon Exp $
 */

#include <sse/awk/awk_i.h>

#ifdef _WIN32
	#include <tchar.h>
	#include <math.h>
#else
	#include <stdlib.h>
	#include <math.h>
#endif

static int __bfn_close   (sse_awk_run_t* run);
static int __bfn_fflush  (sse_awk_run_t* run);
static int __bfn_index   (sse_awk_run_t* run);
static int __bfn_length  (sse_awk_run_t* run);
static int __bfn_substr  (sse_awk_run_t* run);
static int __bfn_split   (sse_awk_run_t* run);
static int __bfn_tolower (sse_awk_run_t* run);
static int __bfn_toupper (sse_awk_run_t* run);
static int __bfn_gsub    (sse_awk_run_t* run);
static int __bfn_sub     (sse_awk_run_t* run);
static int __bfn_match   (sse_awk_run_t* run);
static int __bfn_system  (sse_awk_run_t* run);
/*static int __bfn_sin   (sse_awk_run_t* run);*/

/* TODO: move it under the awk structure... */
static sse_awk_bfn_t __sys_bfn[] = 
{
	/* io functions */
	{SSE_T("close"),   5, SSE_AWK_EXTIO, 1,  1,  SSE_NULL,     __bfn_close},
	{SSE_T("fflush"),  6, SSE_AWK_EXTIO, 0,  1,  SSE_NULL,     __bfn_fflush},

	/* string functions */
	{SSE_T("index"),   5, 0,            2,  2,  SSE_NULL,     __bfn_index},
	{SSE_T("substr"),  6, 0,            2,  3,  SSE_NULL,     __bfn_substr},
	{SSE_T("length"),  6, 0,            1,  1,  SSE_NULL,     __bfn_length},
	{SSE_T("split"),   5, 0,            2,  3,  SSE_T("vrv"), __bfn_split},
	{SSE_T("tolower"), 7, 0,            1,  1,  SSE_NULL,     __bfn_tolower},
	{SSE_T("toupper"), 7, 0,            1,  1,  SSE_NULL,     __bfn_toupper},
	{SSE_T("gsub"),    4, 0,            2,  3,  SSE_T("xvr"), __bfn_gsub},
	{SSE_T("sub"),     3, 0,            2,  3,  SSE_T("xvr"), __bfn_sub},
	{SSE_T("match"),   5, 0,            2,  2,  SSE_T("vx"),  __bfn_match},

	/* TODO: remove these two functions */
	{SSE_T("system"),  6, 0,            1,  1,  SSE_NULL,     __bfn_system},
	/*{ SSE_T("sin"),     3, 0,            1,  1,  SSE_NULL,   __bfn_sin},*/

	{SSE_NULL,         0, 0,            0,  0,  SSE_NULL,     SSE_NULL}
};

sse_awk_bfn_t* sse_awk_addbfn (
	sse_awk_t* awk, const sse_char_t* name, sse_size_t name_len, 
	int when_valid, sse_size_t min_args, sse_size_t max_args, 
	const sse_char_t* arg_spec, int (*handler)(sse_awk_run_t*))
{
	sse_awk_bfn_t* p;

/* TODO: complete this function??? */

	p = (sse_awk_bfn_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_bfn_t));
	if (p == SSE_NULL) return SSE_NULL;

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

int sse_awk_delbfn (sse_awk_t* awk, const sse_char_t* name, sse_size_t name_len)
{
	sse_awk_bfn_t* p, * pp = SSE_NULL;

	for (p = awk->bfn.user; p != SSE_NULL; p++)
	{
		if (sse_awk_strxncmp(p->name, p->name_len, name, name_len) == 0)
		{
			if (pp == SSE_NULL)
				awk->bfn.user = p->next;
			else pp->next = p->next;

			SSE_AWK_FREE (awk, p);
			return 0;
		}

		pp = p;
	}

	return -1;
}

void sse_awk_clrbfn (sse_awk_t* awk)
{
	sse_awk_bfn_t* p, * np;

	p = awk->bfn.user;
	while (p != SSE_NULL)
	{
		np = p;
		SSE_AWK_FREE (awk, p);
		p = np;
	}

	awk->bfn.user = SSE_NULL;
}

sse_awk_bfn_t* sse_awk_getbfn (
	sse_awk_t* awk, const sse_char_t* name, sse_size_t name_len)
{
	sse_awk_bfn_t* p;

	for (p = __sys_bfn; p->name != SSE_NULL; p++)
	{
		if (p->valid != 0 && 
		    (awk->option & p->valid) == 0) continue;

		if (sse_awk_strxncmp (
			p->name, p->name_len, name, name_len) == 0) return p;
	}

/* TODO: user-added builtin functions... */
	for (p = awk->bfn.user; p != SSE_NULL; p = p->next)
	{
		if (p->valid != 0 && 
		    (awk->option & p->valid) == 0) continue;

		if (sse_awk_strxncmp (
			p->name, p->name_len, name, name_len) == 0) return p;
	}

	return SSE_NULL;
}

static int __bfn_close (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_awk_val_t* v, * a0;
	int n;

	sse_char_t* name;
	sse_size_t len;
       
	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs == 1);
/* TODO: support close (xxx, "to"/"from") like gawk */

	a0 = sse_awk_getarg (run, 0);
	sse_awk_assert (run->awk, a0 != SSE_NULL);

	if (a0->type == SSE_AWK_VAL_STR)
	{
		name = ((sse_awk_val_str_t*)a0)->buf;
		len = ((sse_awk_val_str_t*)a0)->len;
	}
	else
	{
		name = sse_awk_valtostr (
			run, a0, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (name == SSE_NULL) return -1;
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
		if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, name);
		n = -1;
		/* TODO: need to set ERRNO??? */
		goto skip_close;
	}

	while (len > 0)
	{
		if (name[--len] == SSE_T('\0'))
		{
			/* the name contains a null string. 
			 * make close return -1 */
			if (a0->type != SSE_AWK_VAL_STR) 
				SSE_AWK_FREE (run->awk, name);
			n = -1;
			/* TODO: need to set ERRNO??? */
			goto skip_close;
		}
	}	

	n = sse_awk_closeextio (run, name);
	if (n == -1 && run->errnum != SSE_AWK_EIOHANDLER)
	{
		if (a0->type != SSE_AWK_VAL_STR) 
			SSE_AWK_FREE (run->awk, name);
		return -1;
	}

	if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, name);

skip_close:
	v = sse_awk_makeintval (run, n);
	if (v == SSE_NULL)
	{
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_setretval (run, v);
	return 0;
}

static int __flush_extio (
	sse_awk_run_t* run, int extio, const sse_char_t* name, int n)
{
	int n2;

	if (run->extio.handler[extio] != SSE_NULL)
	{
		n2 = sse_awk_flushextio (run, extio, name);
		if (n2 == -1)
		{
			if (run->errnum == SSE_AWK_EIOHANDLER) n = -1;
			else if (run->errnum == SSE_AWK_ENOSUCHIO) 
			{
				if (n != 0) n = -2;
			}
			else n = -99; 
		}
		else if (n != -1) n = 0;
	}

	return n;
}

static int __bfn_fflush (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_awk_val_t* a0;
	sse_char_t* str0;
	sse_size_t len0;
	int n;
       
	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs >= 0 && nargs <= 1);

	if (nargs == 0)
	{
		/* flush the console output */
		n = sse_awk_flushextio (run, SSE_AWK_OUT_CONSOLE, SSE_T(""));
		if (n == -1 && 
		    run->errnum != SSE_AWK_EIOHANDLER && 
		    run->errnum != SSE_AWK_ENOSUCHIO)
		{
			return -1;
		}

		/* fflush() should return -1 on EIOHANDLER and ENOSUCHIO */
	}
	else
	{
		sse_char_t* ptr, * end;

		a0 = sse_awk_getarg (run, 0);
		if (a0->type == SSE_AWK_VAL_STR)
		{
			str0 = ((sse_awk_val_str_t*)a0)->buf;
			len0 = ((sse_awk_val_str_t*)a0)->len;
		}
		else
		{
			str0 = sse_awk_valtostr (
				run, a0, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len0);
			if (str0 == SSE_NULL) return -1;

		}

		/* the target name contains a null character.
		 * make fflush return -1 and set ERRNO accordingly */
		ptr = str0; end = str0 + len0;
		while (ptr < end)
		{
			if (*ptr == SSE_T('\0')) 
			{
				if (a0->type != SSE_AWK_VAL_STR) 
					SSE_AWK_FREE (run->awk, str0);
				n = -1;
				goto skip_flush;
			}

			ptr++;
		}

		/* flush the given extio */
		n = 1;

		n = __flush_extio (
			run, SSE_AWK_EXTIO_FILE, 
			((len0 == 0)? SSE_NULL: str0), 1);
		if (n == -99) return -1;
		n = __flush_extio (
			run, SSE_AWK_EXTIO_PIPE,
			((len0 == 0)? SSE_NULL: str0), n);
		if (n == -99) return -1;
		n = __flush_extio (
			run, SSE_AWK_EXTIO_COPROC,
			((len0 == 0)? SSE_NULL: str0), n);
		if (n == -99) return -1;

		/* if n remains 1, no ip handlers have been defined for
		 * file, pipe, and coproc. so make fflush return -1. 
		 * if n is -2, no such named io has been found at all 
		 * if n is -1, the io handler has returned an error */
		if (n != 0) n = -1;

		if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str0);
	}

skip_flush:
	a0 = sse_awk_makeintval (run, (sse_long_t)n);
	if (a0 == SSE_NULL)
	{
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_setretval (run, a0);
	return 0;
}

static int __bfn_index (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_awk_val_t* a0, * a1;
	sse_char_t* str0, * str1, * ptr;
	sse_size_t len0, len1;
	sse_long_t idx;

	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs == 2);
	
	a0 = sse_awk_getarg (run, 0);
	a1 = sse_awk_getarg (run, 1);

	if (a0->type == SSE_AWK_VAL_STR)
	{
		str0 = ((sse_awk_val_str_t*)a0)->buf;
		len0 = ((sse_awk_val_str_t*)a0)->len;
	}
	else
	{
		str0 = sse_awk_valtostr (
			run, a0, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len0);
		if (str0 == SSE_NULL) return -1;
	}

	if (a1->type == SSE_AWK_VAL_STR)
	{
		str1 = ((sse_awk_val_str_t*)a1)->buf;
		len1 = ((sse_awk_val_str_t*)a1)->len;
	}
	else
	{
		str1 = sse_awk_valtostr (
			run, a1, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len1);
		if (str1 == SSE_NULL)
		{
			if (a0->type != SSE_AWK_VAL_STR) 
				SSE_AWK_FREE (run->awk, str0);
			return -1;
		}
	}

	ptr = sse_awk_strxnstr (str0, len0, str1, len1);
	idx = (ptr == SSE_NULL)? -1: (sse_long_t)(ptr - str0);

	if (sse_awk_getopt(run->awk) & SSE_AWK_STRINDEXONE) idx = idx + 1;

	if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str0);
	if (a1->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str1);

	a0 = sse_awk_makeintval (run, idx);
	if (a0 == SSE_NULL)
	{
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_setretval (run, a0);
	return 0;
}

static int __bfn_length (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_awk_val_t* v;
	sse_char_t* str;
	sse_size_t len;

	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs == 1);
	
	v = sse_awk_getarg (run, 0);
	if (v->type == SSE_AWK_VAL_STR)
	{
		len = ((sse_awk_val_str_t*)v)->len;
	}
	else
	{
		str = sse_awk_valtostr (
			run, v, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (str == SSE_NULL) return -1;
		SSE_AWK_FREE (run->awk, str);
	}

	v = sse_awk_makeintval (run, len);
	if (v == SSE_NULL)
	{
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_setretval (run, v);
	return 0;
}

static int __bfn_substr (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_awk_val_t* a0, * a1, * a2, * r;
	sse_char_t* str;
	sse_size_t len;
	sse_long_t lindex, lcount;
	sse_real_t rindex, rcount;
	int n;

	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs >= 2 && nargs <= 3);

	a0 = sse_awk_getarg (run, 0);
	a1 = sse_awk_getarg (run, 1);
	a2 = (nargs >= 3)? sse_awk_getarg (run, 2): SSE_NULL;

	if (a0->type == SSE_AWK_VAL_STR)
	{
		str = ((sse_awk_val_str_t*)a0)->buf;
		len = ((sse_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = sse_awk_valtostr (
			run, a0, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (str == SSE_NULL) return -1;
	}

	n = sse_awk_valtonum (run, a1, &lindex, &rindex);
	if (n == -1) 
	{
		if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str);
		return -1;
	}
	if (n == 1) lindex = (sse_long_t)rindex;

	if (a2 == SSE_NULL) lcount = (sse_long_t)len;
	else 
	{
		n = sse_awk_valtonum (run, a2, &lcount, &rcount);
		if (n == -1) 
		{
			if (a0->type != SSE_AWK_VAL_STR) 
				SSE_AWK_FREE (run->awk, str);
			return -1;
		}
		if (n == 1) lcount = (sse_long_t)rcount;
	}

	if (sse_awk_getopt(run->awk) & SSE_AWK_STRINDEXONE) lindex = lindex - 1;
	if (lindex >= len) lindex = len;
	else if (lindex < 0) lindex = 0;

	if (lcount < 0) lcount = 0;
	else if (lcount > len - lindex) lcount = len - lindex;

	r = sse_awk_makestrval (run, &str[lindex], (sse_size_t)lcount);
	if (r == SSE_NULL)
	{
		if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str);
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str);
	sse_awk_setretval (run, r);
	return 0;
}

static int __bfn_split (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_awk_val_t* a0, * a1, * a2, * t1, * t2, ** a1_ref;
	sse_char_t* str, * str_free, * p, * tok;
	sse_size_t str_len, str_left, tok_len;
	sse_long_t sta, num;
	sse_char_t key[sse_sizeof(sse_long_t)*8+2];
	sse_size_t key_len;
	sse_char_t* fs_ptr, * fs_free;
	sse_size_t fs_len;
	void* fs_rex = SSE_NULL; 
	void* fs_rex_free = SSE_NULL;
	int errnum;

	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs >= 2 && nargs <= 3);

	a0 = sse_awk_getarg (run, 0);
	a1 = sse_awk_getarg (run, 1);
	a2 = (nargs >= 3)? sse_awk_getarg (run, 2): SSE_NULL;

	sse_awk_assert (run->awk, a1->type == SSE_AWK_VAL_REF);

	if (((sse_awk_val_ref_t*)a1)->id >= SSE_AWK_VAL_REF_NAMEDIDX &&
	    ((sse_awk_val_ref_t*)a1)->id <= SSE_AWK_VAL_REF_ARGIDX)
	{
		/* an indexed value should not be assigned another map */
		sse_awk_setrunerrnum (run, SSE_AWK_EIDXVALASSMAP);
		return -1;
	}

	if (((sse_awk_val_ref_t*)a1)->id == SSE_AWK_VAL_REF_POS)
	{
		/* a positional should not be assigned a map */
		sse_awk_setrunerrnum (run, SSE_AWK_EPOSVALASSMAP);
		return -1;
	}

	a1_ref = (sse_awk_val_t**)((sse_awk_val_ref_t*)a1)->adr;
	if ((*a1_ref)->type != SSE_AWK_VAL_NIL &&
	    (*a1_ref)->type != SSE_AWK_VAL_MAP)
	{
		/* cannot change a scalar value to a map */
		sse_awk_setrunerrnum (run, SSE_AWK_ESCALARTOMAP);
		return -1;
	}

	if (a0->type == SSE_AWK_VAL_STR)
	{
		str = ((sse_awk_val_str_t*)a0)->buf;
		str_len = ((sse_awk_val_str_t*)a0)->len;
		str_free = SSE_NULL;
	}
	else 
	{
		str = sse_awk_valtostr (
			run, a0, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &str_len);
		if (str == SSE_NULL) return -1;
		str_free = str;
	}

	if (a2 == SSE_NULL)
	{
		/* get the value from FS */
		t1 = sse_awk_getglobal (run, SSE_AWK_GLOBAL_FS);
		if (t1->type == SSE_AWK_VAL_NIL)
		{
			fs_ptr = SSE_T(" ");
			fs_len = 1;
			fs_free = SSE_NULL;
		}
		else if (t1->type == SSE_AWK_VAL_STR)
		{
			fs_ptr = ((sse_awk_val_str_t*)t1)->buf;
			fs_len = ((sse_awk_val_str_t*)t1)->len;
			fs_free = SSE_NULL;
		}
		else
		{
			fs_ptr = sse_awk_valtostr (
				run, t1, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &fs_len);
			if (fs_ptr == SSE_NULL)
			{
				if (str_free != SSE_NULL) 
					SSE_AWK_FREE (run->awk, str_free);
				return -1;
			}
			fs_free = fs_ptr;
		}

		if (fs_len > 1) 
		{
			fs_rex = run->global.fs;
			fs_rex_free = SSE_NULL;
		}
	}
	else
	{
		if (a2->type == SSE_AWK_VAL_STR)
		{
			fs_ptr = ((sse_awk_val_str_t*)a2)->buf;
			fs_len = ((sse_awk_val_str_t*)a2)->len;
			fs_free = SSE_NULL;
		}
		else
		{
			fs_ptr = sse_awk_valtostr (
				run, a2, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &fs_len);
			if (fs_ptr == SSE_NULL)
			{
				if (str_free != SSE_NULL) 
					SSE_AWK_FREE (run->awk, str_free);
				return -1;
			}
			fs_free = fs_ptr;
		}

		if (fs_len > 1) 
		{
			fs_rex = sse_awk_buildrex (
				run->awk, fs_ptr, fs_len, &errnum);
			if (fs_rex == SSE_NULL)
			{
				if (str_free != SSE_NULL) 
					SSE_AWK_FREE (run->awk, str_free);
				if (fs_free != SSE_NULL) 
					SSE_AWK_FREE (run->awk, fs_free);
				sse_awk_setrunerrnum (run, errnum);
				return -1;
			}
			fs_rex_free = fs_rex;
		}
	}

	t1 = sse_awk_makemapval (run);
	if (t1 == SSE_NULL)
	{
		if (str_free != SSE_NULL) 
			SSE_AWK_FREE (run->awk, str_free);
		if (fs_free != SSE_NULL) 
			SSE_AWK_FREE (run->awk, fs_free);
		if (fs_rex_free != SSE_NULL) 
			sse_awk_freerex (run->awk, fs_rex_free);
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_refdownval (run, *a1_ref);
	*a1_ref = t1;
	sse_awk_refupval (*a1_ref);

	p = str; str_left = str_len; 
	sta = (sse_awk_getopt(run->awk) & SSE_AWK_STRINDEXONE)? 1: 0;
	num = sta;

	while (p != SSE_NULL)
	{
		if (fs_len <= 1)
		{
			p = sse_awk_strxntok (run, 
				p, str_len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = sse_awk_strxntokbyrex (run, p, str_len, 
				fs_rex, &tok, &tok_len, &errnum); 
			if (p == SSE_NULL && errnum != SSE_AWK_ENOERR)
			{
				if (str_free != SSE_NULL) 
					SSE_AWK_FREE (run->awk, str_free);
				if (fs_free != SSE_NULL) 
					SSE_AWK_FREE (run->awk, fs_free);
				if (fs_rex_free != SSE_NULL) 
					sse_awk_freerex (run->awk, fs_rex_free);
				sse_awk_setrunerrnum (run, errnum);
				return -1;
			}
		}

		if (num == 0 && p == SSE_NULL && tok_len == 0) 
		{
			/* no field at all*/
			break; 
		}	

		sse_awk_assert (run->awk, 
			(tok != SSE_NULL && tok_len > 0) || tok_len == 0);

		/* create the field string */
		t2 = sse_awk_makestrval (run, tok, tok_len);
		if (t2 == SSE_NULL)
		{
			if (str_free != SSE_NULL)
				SSE_AWK_FREE (run->awk, str_free);
			if (fs_free != SSE_NULL)
				SSE_AWK_FREE (run->awk, fs_free);
			if (fs_rex_free != SSE_NULL)
				sse_awk_freerex (run->awk, fs_rex_free);
			sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
			return -1;
		}

		/* put it into the map */
		key_len = sse_awk_longtostr (
			num, 10, SSE_NULL, key, sse_countof(key));
		sse_awk_assert (run->awk, key_len != (sse_size_t)-1);

		/* don't forget to update the reference count when you 
		 * handle the assignment-like situation.  anyway, it is 
		 * incremented in advance as if the assignment was successful.
		 * it is decremented if the assignement fails. */
		sse_awk_refupval (t2);

		if (sse_awk_map_putx (
			((sse_awk_val_map_t*)t1)->map, 
			key, key_len, t2, SSE_NULL) == -1)
		{
			sse_awk_refdownval (run, t2);

			if (str_free != SSE_NULL) 
				SSE_AWK_FREE (run->awk, str_free);
			if (fs_free != SSE_NULL) 
				SSE_AWK_FREE (run->awk, fs_free);
			if (fs_rex_free != SSE_NULL)
				sse_awk_freerex (run->awk, fs_rex_free);
			sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
			return -1;
		}

		num++;
		str_len = str_left - (p - str);
	}

	if (str_free != SSE_NULL) SSE_AWK_FREE (run->awk, str_free);
	if (fs_free != SSE_NULL) SSE_AWK_FREE (run->awk, fs_free);
	if (fs_rex_free != SSE_NULL) sse_awk_freerex (run->awk, fs_rex_free);

	if (sta == 1) num--;

	t1 = sse_awk_makeintval (run, num);
	if (t1 == SSE_NULL)
	{
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_setretval (run, t1);
	return 0;
}

static int __bfn_tolower (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_char_t* str;
	sse_size_t len, i;
	sse_awk_val_t* a0, * r;

	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs == 1);

	a0 = sse_awk_getarg (run, 0);

	if (a0->type == SSE_AWK_VAL_STR)
	{
		str = ((sse_awk_val_str_t*)a0)->buf;
		len = ((sse_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = sse_awk_valtostr (
			run, a0, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (str == SSE_NULL) return -1;
	}

	for (i = 0; i < len; i++) str[i] = SSE_AWK_TOLOWER (run->awk, str[i]);	

	r = sse_awk_makestrval (run, str, len);
	if (r == SSE_NULL)
	{
		if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str);
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str);
	sse_awk_setretval (run, r);
	return 0;
}

static int __bfn_toupper (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_char_t* str;
	sse_size_t len, i;
	sse_awk_val_t* a0, * r;

	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs == 1);

	a0 = sse_awk_getarg (run, 0);

	if (a0->type == SSE_AWK_VAL_STR)
	{
		str = ((sse_awk_val_str_t*)a0)->buf;
		len = ((sse_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = sse_awk_valtostr (
			run, a0, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (str == SSE_NULL) return -1;
	}

	for (i = 0; i < len; i++) str[i] = SSE_AWK_TOUPPER (run->awk, str[i]);	

	r = sse_awk_makestrval (run, str, len);
	if (r == SSE_NULL)
	{
		if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str);
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str);
	sse_awk_setretval (run, r);
	return 0;
}

static int __substitute (sse_awk_run_t* run, sse_long_t max_count)
{
	sse_size_t nargs;
	sse_awk_val_t* a0, * a1, * a2, ** a2_ref, * v;
	sse_char_t* a0_ptr, * a1_ptr, * a2_ptr;
	sse_size_t a0_len, a1_len, a2_len;
	sse_char_t* a0_ptr_free = SSE_NULL;
	sse_char_t* a1_ptr_free = SSE_NULL;
	sse_char_t* a2_ptr_free = SSE_NULL;
	void* rex;
	int opt, n;
	const sse_char_t* cur_ptr, * mat_ptr;
	sse_size_t cur_len, mat_len, i, m;
	sse_awk_str_t new;
	sse_long_t sub_count;

	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs >= 2 && nargs <= 3);

	a0 = sse_awk_getarg (run, 0);
	a1 = sse_awk_getarg (run, 1);
	a2 = (nargs >= 3)? sse_awk_getarg (run, 2): SSE_NULL;

	sse_awk_assert (run->awk, a2 == SSE_NULL || a2->type == SSE_AWK_VAL_REF);

#define FREE_A_PTRS(awk) \
	do { \
		if (a2_ptr_free != SSE_NULL) SSE_AWK_FREE (awk, a2_ptr_free); \
		if (a1_ptr_free != SSE_NULL) SSE_AWK_FREE (awk, a1_ptr_free); \
		if (a0_ptr_free != SSE_NULL) SSE_AWK_FREE (awk, a0_ptr_free); \
	} while (0)
#define FREE_A0_REX(awk,rex) \
	do { \
		if (a0->type != SSE_AWK_VAL_REX) sse_awk_freerex (awk, rex); \
	} while (0)

	if (a0->type == SSE_AWK_VAL_REX)
	{
		rex = ((sse_awk_val_rex_t*)a0)->code;
	}
	else if (a0->type == SSE_AWK_VAL_STR)
	{
		a0_ptr = ((sse_awk_val_str_t*)a0)->buf;
		a0_len = ((sse_awk_val_str_t*)a0)->len;
	}
	else
	{
		a0_ptr = sse_awk_valtostr (
			run, a0, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &a0_len);
		if (a0_ptr == SSE_NULL) 
		{
			FREE_A_PTRS (run->awk);
			return -1;
		}	
		a0_ptr_free = a0_ptr;
	}

	if (a1->type == SSE_AWK_VAL_STR)
	{
		a1_ptr = ((sse_awk_val_str_t*)a1)->buf;
		a1_len = ((sse_awk_val_str_t*)a1)->len;
	}
	else
	{
		a1_ptr = sse_awk_valtostr (
			run, a1, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &a1_len);
		if (a1_ptr == SSE_NULL) 
		{
			FREE_A_PTRS (run->awk);
			return -1;
		}
		a1_ptr_free = a1_ptr;
	}

	if (a2 == SSE_NULL)
	{
		/* is this correct? any needs to use inrec.d0? */
		a2_ptr = SSE_AWK_STR_BUF(&run->inrec.line);
		a2_len = SSE_AWK_STR_LEN(&run->inrec.line);
	}
	else if (((sse_awk_val_ref_t*)a2)->id == SSE_AWK_VAL_REF_POS)
	{
		sse_size_t idx;
	       
		idx = (sse_size_t)((sse_awk_val_ref_t*)a2)->adr;
		if (idx == 0)
		{
			a2_ptr = SSE_AWK_STR_BUF(&run->inrec.line);
			a2_len = SSE_AWK_STR_LEN(&run->inrec.line);
		}
		else if (idx <= run->inrec.nflds)
		{
			a2_ptr = run->inrec.flds[idx-1].ptr;
			a2_len = run->inrec.flds[idx-1].len;
		}
		else
		{
			a2_ptr = SSE_T("");
			a2_len = 0;
		}
	}
	else
	{
		a2_ref = (sse_awk_val_t**)((sse_awk_val_ref_t*)a2)->adr;

		if ((*a2_ref)->type == SSE_AWK_VAL_MAP)
		{
			FREE_A_PTRS (run->awk);
			/* a map is not allowed as the third parameter */
			sse_awk_setrunerrnum (run, SSE_AWK_EMAPNOTALLOWED);
			return -1;
		}

		if ((*a2_ref)->type == SSE_AWK_VAL_STR)
		{
			a2_ptr = ((sse_awk_val_str_t*)(*a2_ref))->buf;
			a2_len = ((sse_awk_val_str_t*)(*a2_ref))->len;
		}
		else
		{
			a2_ptr = sse_awk_valtostr (
				run, *a2_ref, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &a2_len);
			if (a2_ptr == SSE_NULL) 
			{
				FREE_A_PTRS (run->awk);
				return -1;
			}
			a2_ptr_free = a2_ptr;
		}
	}

	if (sse_awk_str_open (&new, a2_len, run->awk) == SSE_NULL)
	{
		FREE_A_PTRS (run->awk);
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != SSE_AWK_VAL_REX)
	{
		rex = sse_awk_buildrex (run->awk, a0_ptr, a0_len, &run->errnum);
		if (rex == SSE_NULL)
		{
			sse_awk_str_close (&new);
			FREE_A_PTRS (run->awk);
			return -1;
		}
	}

	opt = (run->global.ignorecase)? SSE_AWK_REX_IGNORECASE: 0;
	cur_ptr = a2_ptr;
	cur_len = a2_len;
	sub_count = 0;

	while (1)
	{
		if (max_count == 0 || sub_count < max_count)
		{
			n = sse_awk_matchrex (
				run->awk, rex, opt, cur_ptr, cur_len,
				&mat_ptr, &mat_len, &run->errnum);
		}
		else n = 0;

		if (n == -1)
		{
			FREE_A0_REX (run->awk, rex);
			sse_awk_str_close (&new);
			FREE_A_PTRS (run->awk);
			return -1;
		}

		if (n == 0) 
		{ 
			/* no more match found */
			if (sse_awk_str_ncat (
				&new, cur_ptr, cur_len) == (sse_size_t)-1)
			{
				FREE_A0_REX (run->awk, rex);
				sse_awk_str_close (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
			break;
		}

		if (sse_awk_str_ncat (
			&new, cur_ptr, mat_ptr - cur_ptr) == (sse_size_t)-1)
		{
			FREE_A0_REX (run->awk, rex);
			sse_awk_str_close (&new);
			FREE_A_PTRS (run->awk);
			return -1;
		}

		for (i = 0; i < a1_len; i++)
		{
			if ((i+1) < a1_len && 
			    a1_ptr[i] == SSE_T('\\') && 
			    a1_ptr[i+1] == SSE_T('&'))
			{
				m = sse_awk_str_ccat (&new, SSE_T('&'));
				i++;
			}
			else if (a1_ptr[i] == SSE_T('&'))
			{
				m = sse_awk_str_ncat (&new, mat_ptr, mat_len);
			}
			else 
			{
				m = sse_awk_str_ccat (&new, a1_ptr[i]);
			}

			if (m == (sse_size_t)-1)
			{
				FREE_A0_REX (run->awk, rex);
				sse_awk_str_close (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
		}

		sub_count++;
		cur_len = cur_len - ((mat_ptr - cur_ptr) + mat_len);
		cur_ptr = mat_ptr + mat_len;
	}

	FREE_A0_REX (run->awk, rex);

	if (sub_count > 0)
	{
		if (a2 == SSE_NULL)
		{
			if (sse_awk_setrec (run, 0,
				SSE_AWK_STR_BUF(&new), SSE_AWK_STR_LEN(&new)) == -1)
			{
				sse_awk_str_close (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
		}
		else if (((sse_awk_val_ref_t*)a2)->id == SSE_AWK_VAL_REF_POS)
		{
			int n;

			n = sse_awk_setrec (
				run, (sse_size_t)((sse_awk_val_ref_t*)a2)->adr,
				SSE_AWK_STR_BUF(&new), SSE_AWK_STR_LEN(&new));

			if (n == -1)
			{
				sse_awk_str_close (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
		}
		else
		{
			v = sse_awk_makestrval (run,
				SSE_AWK_STR_BUF(&new), SSE_AWK_STR_LEN(&new));
			if (v == SSE_NULL)
			{
				sse_awk_str_close (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}

			sse_awk_refdownval (run, *a2_ref);
			*a2_ref = v;
			sse_awk_refupval (*a2_ref);
		}
	}

	sse_awk_str_close (&new);
	FREE_A_PTRS (run->awk);

#undef FREE_A0_REX
#undef FREE_A_PTRS

	v = sse_awk_makeintval (run, sub_count);
	if (v == SSE_NULL)
	{
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_setretval (run, v);
	return 0;
}

static int __bfn_gsub (sse_awk_run_t* run)
{
	return __substitute (run, 0);
}

static int __bfn_sub (sse_awk_run_t* run)
{
	return __substitute (run, 1);
}

static int __bfn_match (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_awk_val_t* a0, * a1;
	sse_char_t* str0, * str1;
	sse_size_t len0, len1;
	sse_long_t idx;
	void* rex;
	int opt, n;
	const sse_char_t* mat_ptr;
	sse_size_t mat_len;

	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs == 2);
	
	a0 = sse_awk_getarg (run, 0);
	a1 = sse_awk_getarg (run, 1);

	if (a0->type == SSE_AWK_VAL_STR)
	{
		str0 = ((sse_awk_val_str_t*)a0)->buf;
		len0 = ((sse_awk_val_str_t*)a0)->len;
	}
	else
	{
		str0 = sse_awk_valtostr (
			run, a0, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len0);
		if (str0 == SSE_NULL) return -1;
	}

	if (a1->type == SSE_AWK_VAL_REX)
	{
		rex = ((sse_awk_val_rex_t*)a1)->code;
	}
	else 
	{
		if (a1->type == SSE_AWK_VAL_STR)
		{
			str1 = ((sse_awk_val_str_t*)a1)->buf;
			len1 = ((sse_awk_val_str_t*)a1)->len;
		}
		else
		{
			str1 = sse_awk_valtostr (
				run, a1, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len1);
			if (str1 == SSE_NULL)
			{
				if (a0->type != SSE_AWK_VAL_STR) 
					SSE_AWK_FREE (run->awk, str0);
				return -1;
			}
		}

		rex = sse_awk_buildrex (run->awk, str1, len1, &run->errnum);
		if (rex == SSE_NULL)
		{
			if (a0->type != SSE_AWK_VAL_STR) 
				SSE_AWK_FREE (run->awk, str0);
			return -1;
		}

		if (a1->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str1);
	}

	opt = (run->global.ignorecase)? SSE_AWK_REX_IGNORECASE: 0;
	n = sse_awk_matchrex (
		run->awk, rex, opt, str0, len0,
		&mat_ptr, &mat_len, &run->errnum);

	if (a0->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str0);
	if (a1->type != SSE_AWK_VAL_REX) sse_awk_freerex (run->awk, rex);

	if (n == -1) return -1;

	idx = (n == 0)? -1: (sse_long_t)(mat_ptr - str0);
	if (sse_awk_getopt(run->awk) & SSE_AWK_STRINDEXONE) idx = idx + 1;

	a0 = sse_awk_makeintval (run, idx);
	if (a0 == SSE_NULL)
	{
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_refupval (a0);

	a1 = sse_awk_makeintval (run, 
		((n == 0)? (sse_long_t)-1: (sse_long_t)mat_len));
	if (a1 == SSE_NULL)
	{
		sse_awk_refdownval (run, a0);
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_refupval (a1);

	if (sse_awk_setglobal (run, SSE_AWK_GLOBAL_RSTART, a0) == -1)
	{
		sse_awk_refdownval (run, a1);
		sse_awk_refdownval (run, a0);
		return -1;
	}

	if (sse_awk_setglobal (run, SSE_AWK_GLOBAL_RLENGTH, a1) == -1)
	{
		sse_awk_refdownval (run, a1);
		sse_awk_refdownval (run, a0);
		return -1;
	}

	sse_awk_setretval (run, a0);

	sse_awk_refdownval (run, a1);
	sse_awk_refdownval (run, a0);
	return 0;
}

static int __bfn_system (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_char_t* cmd;
	sse_awk_val_t* v;
	int n;
       
	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs == 1);

	cmd = sse_awk_valtostr (
		run, sse_awk_getarg(run, 0), 
		SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, SSE_NULL);
	if (cmd == SSE_NULL) return -1;

#ifdef _WIN32
	n = _tsystem (cmd);
#else
	/* TODO: support system on other platforms that win32 */
	n = -1;
#endif

	SSE_AWK_FREE (run->awk, cmd);

	v = sse_awk_makeintval (run, n);
	if (v == SSE_NULL)
	{
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_setretval (run, v);
	return 0;
}

/* math functions */
#if 0
static int __bfn_sin (sse_awk_run_t* run)
{
	sse_size_t nargs;
	sse_awk_val_t* v;
	int n;
	sse_long_t lv;
	sse_real_t rv;
       
	nargs = sse_awk_getnargs (run);
	sse_awk_assert (run->awk, nargs == 1);

	n = sse_awk_valtonum (run, sse_awk_getarg(run, 0), &lv, &rv);
	if (n == -1)
	{
		/* wrong value */
		return  -1;
	}

	if (n == 0) rv = (sse_real_t)lv;

#if (SSE_SIZEOF_REAL == SSE_SIZEOF_LONG_DOUBLE) 
	v = sse_awk_makerealval (run, (sse_real_t)sinl(rv));
#elif (SSE_SIZEOF_REAL == SSE_SIZEOF_DOUBLE)
	v = sse_awk_makerealval (run, (sse_real_t)sin(rv));
#else
	#error unsupported floating-point data type
#endif

	if (v == SSE_NULL)
	{
		sse_awk_setrunerrnum (run, SSE_AWK_ENOMEM);
		return -1;
	}

	sse_awk_setretval (run, v);
	return 0;
}
#endif
