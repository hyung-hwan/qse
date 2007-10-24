/*
 * $Id: func.c,v 1.11 2007/10/21 13:58:47 bacon Exp $
 *
 * {License}
 */

#include <ase/awk/awk_i.h>

static int bfn_close   (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_fflush  (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_index   (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_length  (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_substr  (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_split   (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_tolower (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_toupper (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_gsub    (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_sub     (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_match   (ase_awk_run_t*, const ase_char_t*, ase_size_t);
static int bfn_sprintf (ase_awk_run_t*, const ase_char_t*, ase_size_t);

#undef MAX
#define MAX ASE_TYPE_UNSIGNED_MAX(ase_size_t)

static ase_awk_bfn_t __sys_bfn[] = 
{
	/* io functions */
	{ {ASE_T("close"),   5}, ASE_AWK_EXTIO, {1, 1, ASE_NULL}, bfn_close},
	{ {ASE_T("fflush"),  6}, ASE_AWK_EXTIO, {0, 1, ASE_NULL}, bfn_fflush},

	/* string functions */
	{ {ASE_T("index"),   5}, 0,  {2,   2, ASE_NULL},     bfn_index},
	{ {ASE_T("substr"),  6}, 0,  {2,   3, ASE_NULL},     bfn_substr},
	{ {ASE_T("length"),  6}, 0,  {1,   1, ASE_NULL},     bfn_length},
	{ {ASE_T("split"),   5}, 0,  {2,   3, ASE_T("vrv")}, bfn_split},
	{ {ASE_T("tolower"), 7}, 0,  {1,   1, ASE_NULL},     bfn_tolower},
	{ {ASE_T("toupper"), 7}, 0,  {1,   1, ASE_NULL},     bfn_toupper},
	{ {ASE_T("gsub"),    4}, 0,  {2,   3, ASE_T("xvr")}, bfn_gsub},
	{ {ASE_T("sub"),     3}, 0,  {2,   3, ASE_T("xvr")}, bfn_sub},
	{ {ASE_T("match"),   5}, 0,  {2,   2, ASE_T("vx")},  bfn_match},
	{ {ASE_T("sprintf"), 7}, 0,  {1, MAX, ASE_NULL},     bfn_sprintf},

	{ {ASE_NULL,         0}, 0,  {0,   0, ASE_NULL},     ASE_NULL}
};

void* ase_awk_addfunc (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t name_len, 
	int when_valid, ase_size_t min_args, ase_size_t max_args, 
	const ase_char_t* arg_spec, 
	int (*handler)(ase_awk_run_t*,const ase_char_t*,ase_size_t))
{
	ase_awk_bfn_t* p;

	/* TODO: make function table hash-accessable */

	if (name_len <= 0)
	{
		ase_awk_seterror (awk, ASE_AWK_EINVAL, 0, ASE_NULL, 0);
		return ASE_NULL;
	}

	if (ase_awk_getbfn (awk, name, name_len) != ASE_NULL)
	{
		ase_cstr_t errarg;

		errarg.ptr = name;
		errarg.len = name_len;

		ase_awk_seterror (awk, ASE_AWK_EEXIST, 0, &errarg, 1);
		return ASE_NULL;
	}

	p = (ase_awk_bfn_t*) ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_bfn_t));
	if (p == ASE_NULL) 
	{
		ase_awk_seterror (awk, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
		return ASE_NULL;
	}

	p->name.ptr = ase_strxdup (name, name_len, &awk->prmfns.mmgr);  
	if (p->name.ptr == ASE_NULL)
	{
		ASE_AWK_FREE (awk, p);
		ase_awk_seterror (awk, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
		return ASE_NULL;
	}

	p->name.len = name_len;
	p->valid = when_valid;
	p->arg.min = min_args;
	p->arg.max = max_args;
	if (arg_spec == ASE_NULL) p->arg.spec = ASE_NULL;
	else
	{
		p->arg.spec = ase_strdup (arg_spec, &awk->prmfns.mmgr);
		if (p->arg.spec == ASE_NULL)
		{
			ASE_AWK_FREE (awk, p->name.ptr);
			ASE_AWK_FREE (awk, p);
			ase_awk_seterror (awk, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
			return ASE_NULL;
		}
	}
	p->handler = handler;

	p->next = awk->bfn.user;
	awk->bfn.user = p;

	return p;
}

int ase_awk_delfunc (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t name_len)
{
	ase_awk_bfn_t* p, * pp = ASE_NULL;
	ase_cstr_t errarg;

	for (p = awk->bfn.user; p != ASE_NULL; p = p->next)
	{
		if (ase_strxncmp (
			p->name.ptr, p->name.len, name, name_len) == 0)
		{
			if (pp == ASE_NULL)
				awk->bfn.user = p->next;
			else pp->next = p->next;

			if (p->arg.spec != ASE_NULL)
				ASE_AWK_FREE (awk, p->arg.spec);
			ASE_AWK_FREE (awk, p->name.ptr);
			ASE_AWK_FREE (awk, p);
			return 0;
		}

		pp = p;
	}

	errarg.ptr = name;
	errarg.len = name_len;

	ase_awk_seterror (awk, ASE_AWK_ENOENT, 0, &errarg, 1);
	return -1;
}

void ase_awk_clrbfn (ase_awk_t* awk)
{
	ase_awk_bfn_t* p, * np;

	p = awk->bfn.user;
	while (p != ASE_NULL)
	{
		np = p->next;
		if (p->arg.spec != ASE_NULL)
			ASE_AWK_FREE (awk, p->arg.spec);
		ASE_AWK_FREE (awk, p->name.ptr);
		ASE_AWK_FREE (awk, p);
		p = np;
	}

	awk->bfn.user = ASE_NULL;
}

ase_awk_bfn_t* ase_awk_getbfn (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len)
{
	ase_awk_bfn_t* p;
	ase_awk_pair_t* pair;
	const ase_char_t* k;
	ase_size_t l;

	for (p = __sys_bfn; p->name.ptr != ASE_NULL; p++)
	{
		if (p->valid != 0 && 
		    (awk->option & p->valid) == 0) continue;

		pair = ase_awk_map_get (awk->kwtab, p->name.ptr, p->name.len);
		if (pair != ASE_NULL)
		{
			k = ((ase_cstr_t*)(pair->val))->ptr;
			l = ((ase_cstr_t*)(pair->val))->len;
		}
		else
		{
			k = p->name.ptr;
			l = p->name.len;
		}

		if (ase_strxncmp (k, l, name, len) == 0) return p;
	}

	for (p = awk->bfn.user; p != ASE_NULL; p = p->next)
	{
		if (p->valid != 0 && 
		    (awk->option & p->valid) == 0) continue;

		pair = ase_awk_map_get (awk->kwtab, p->name.ptr, p->name.len);
		if (pair != ASE_NULL)
		{
			k = ((ase_cstr_t*)(pair->val))->ptr;
			l = ((ase_cstr_t*)(pair->val))->len;
		}
		else
		{
			k = p->name.ptr;
			l = p->name.len;
		}

		if (ase_strxncmp (k, l, name, len) == 0) return p;
	}

	return ASE_NULL;
}

static int bfn_close (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* v, * a0;
	int n;

	ase_char_t* name;
	ase_size_t len;
       
	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 1);
/* TODO: support close (xxx, "to"/"from") like gawk */

	a0 = ase_awk_getarg (run, 0);
	ASE_ASSERT (a0 != ASE_NULL);

	if (a0->type == ASE_AWK_VAL_STR)
	{
		name = ((ase_awk_val_str_t*)a0)->buf;
		len = ((ase_awk_val_str_t*)a0)->len;
	}
	else
	{
		name = ase_awk_valtostr (
			run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (name == ASE_NULL) return -1;
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
		if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, name);
		n = -1;
		goto skip_close;
	}

	while (len > 0)
	{
		if (name[--len] == ASE_T('\0'))
		{
			/* the name contains a null string. 
			 * make close return -1 */
			if (a0->type != ASE_AWK_VAL_STR) 
				ASE_AWK_FREE (run->awk, name);
			n = -1;
			goto skip_close;
		}
	}	

	n = ase_awk_closeextio (run, name);
	/*
	if (n == -1 && run->errnum != ASE_AWK_EIONONE)
	{
		if (a0->type != ASE_AWK_VAL_STR) 
			ASE_AWK_FREE (run->awk, name);
		return -1;
	}
	*/

	if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, name);

skip_close:
	v = ase_awk_makeintval (run, (ase_long_t)n);
	if (v == ASE_NULL)
	{
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_awk_setretval (run, v);
	return 0;
}

static int __flush_extio (
	ase_awk_run_t* run, int extio, const ase_char_t* name, int n)
{
	int n2;

	if (run->extio.handler[extio] != ASE_NULL)
	{
		n2 = ase_awk_flushextio (run, extio, name);
		if (n2 == -1)
		{
			/*
			if (run->errnum == ASE_AWK_EIOIMPL) n = -1;
			else if (run->errnum == ASE_AWK_EIONONE) 
			{
				if (n != 0) n = -2;
			}
			else n = -99; 
			*/	
			if (run->errnum == ASE_AWK_EIONONE) 
			{
				if (n != 0) n = -2;
			}
			else n = -1;
		}
		else if (n != -1) n = 0;
	}

	return n;
}

static int bfn_fflush (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* a0;
	ase_char_t* str0;
	ase_size_t len0;
	int n;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 0 || nargs == 1);

	if (nargs == 0)
	{
		/* flush the console output.
		 * fflush() should return -1 on errors */
		n = ase_awk_flushextio (run, ASE_AWK_OUT_CONSOLE, ASE_T(""));
	}
	else
	{
		ase_char_t* ptr, * end;

		a0 = ase_awk_getarg (run, 0);
		if (a0->type == ASE_AWK_VAL_STR)
		{
			str0 = ((ase_awk_val_str_t*)a0)->buf;
			len0 = ((ase_awk_val_str_t*)a0)->len;
		}
		else
		{
			str0 = ase_awk_valtostr (
				run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len0);
			if (str0 == ASE_NULL) return -1;

		}

		/* the target name contains a null character.
		 * make fflush return -1 */
		ptr = str0; end = str0 + len0;
		while (ptr < end)
		{
			if (*ptr == ASE_T('\0')) 
			{
				if (a0->type != ASE_AWK_VAL_STR) 
					ASE_AWK_FREE (run->awk, str0);
				n = -1;
				goto skip_flush;
			}

			ptr++;
		}

		/* flush the given extio */
		n = __flush_extio (
			run, ASE_AWK_EXTIO_FILE, 
			((len0 == 0)? ASE_NULL: str0), 1);
		/*if (n == -99) return -1;*/
		n = __flush_extio (
			run, ASE_AWK_EXTIO_PIPE,
			((len0 == 0)? ASE_NULL: str0), n);
		/*if (n == -99) return -1;*/
		n = __flush_extio (
			run, ASE_AWK_EXTIO_COPROC,
			((len0 == 0)? ASE_NULL: str0), n);
		/*if (n == -99) return -1;*/

		/* if n remains 1, no ip handlers have been defined for
		 * file, pipe, and coproc. so make fflush return -1. 
		 * if n is -2, no such named io has been found at all 
		 * if n is -1, the io handler has returned an error */
		if (n != 0) n = -1;

		if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str0);
	}

skip_flush:
	a0 = ase_awk_makeintval (run, (ase_long_t)n);
	if (a0 == ASE_NULL)
	{
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_awk_setretval (run, a0);
	return 0;
}

static int bfn_index (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* a0, * a1;
	ase_char_t* str0, * str1, * ptr;
	ase_size_t len0, len1;
	ase_long_t idx;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 2);
	
	a0 = ase_awk_getarg (run, 0);
	a1 = ase_awk_getarg (run, 1);

	if (a0->type == ASE_AWK_VAL_STR)
	{
		str0 = ((ase_awk_val_str_t*)a0)->buf;
		len0 = ((ase_awk_val_str_t*)a0)->len;
	}
	else
	{
		str0 = ase_awk_valtostr (
			run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len0);
		if (str0 == ASE_NULL) return -1;
	}

	if (a1->type == ASE_AWK_VAL_STR)
	{
		str1 = ((ase_awk_val_str_t*)a1)->buf;
		len1 = ((ase_awk_val_str_t*)a1)->len;
	}
	else
	{
		str1 = ase_awk_valtostr (
			run, a1, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len1);
		if (str1 == ASE_NULL)
		{
			if (a0->type != ASE_AWK_VAL_STR) 
				ASE_AWK_FREE (run->awk, str0);
			return -1;
		}
	}

	ptr = ase_strxnstr (str0, len0, str1, len1);
	idx = (ptr == ASE_NULL)? -1: (ase_long_t)(ptr - str0);

	if (ase_awk_getoption(run->awk) & ASE_AWK_BASEONE) idx = idx + 1;

	if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str0);
	if (a1->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str1);

	a0 = ase_awk_makeintval (run, idx);
	if (a0 == ASE_NULL)
	{
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_awk_setretval (run, a0);
	return 0;
}

static int bfn_length (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* v;
	ase_char_t* str;
	ase_size_t len;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 1);
	
	v = ase_awk_getarg (run, 0);
	if (v->type == ASE_AWK_VAL_STR)
	{
		len = ((ase_awk_val_str_t*)v)->len;
	}
	else
	{
		str = ase_awk_valtostr (
			run, v, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (str == ASE_NULL) return -1;
		ASE_AWK_FREE (run->awk, str);
	}

	v = ase_awk_makeintval (run, len);
	if (v == ASE_NULL)
	{
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_awk_setretval (run, v);
	return 0;
}

static int bfn_substr (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* a0, * a1, * a2, * r;
	ase_char_t* str;
	ase_size_t len;
	ase_long_t lindex, lcount;
	ase_real_t rindex, rcount;
	int n;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = ase_awk_getarg (run, 0);
	a1 = ase_awk_getarg (run, 1);
	a2 = (nargs >= 3)? ase_awk_getarg (run, 2): ASE_NULL;

	if (a0->type == ASE_AWK_VAL_STR)
	{
		str = ((ase_awk_val_str_t*)a0)->buf;
		len = ((ase_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = ase_awk_valtostr (
			run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (str == ASE_NULL) return -1;
	}

	n = ase_awk_valtonum (run, a1, &lindex, &rindex);
	if (n == -1) 
	{
		if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str);
		return -1;
	}
	if (n == 1) lindex = (ase_long_t)rindex;

	if (a2 == ASE_NULL) lcount = (ase_long_t)len;
	else 
	{
		n = ase_awk_valtonum (run, a2, &lcount, &rcount);
		if (n == -1) 
		{
			if (a0->type != ASE_AWK_VAL_STR) 
				ASE_AWK_FREE (run->awk, str);
			return -1;
		}
		if (n == 1) lcount = (ase_long_t)rcount;
	}

	if (ase_awk_getoption(run->awk) & ASE_AWK_BASEONE) lindex = lindex - 1;
	if (lindex >= len) lindex = len;
	else if (lindex < 0) lindex = 0;

	if (lcount < 0) lcount = 0;
	else if (lcount > len - lindex) lcount = len - lindex;

	r = ase_awk_makestrval (run, &str[lindex], (ase_size_t)lcount);
	if (r == ASE_NULL)
	{
		if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str);
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str);
	ase_awk_setretval (run, r);
	return 0;
}

static int bfn_split (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* a0, * a1, * a2, * t1, * t2, ** a1_ref;
	ase_char_t* str, * str_free, * p, * tok;
	ase_size_t str_len, str_left, tok_len;
	ase_long_t sta, num;
	ase_char_t key[ASE_SIZEOF(ase_long_t)*8+2];
	ase_size_t key_len;
	ase_char_t* fs_ptr, * fs_free;
	ase_size_t fs_len;
	void* fs_rex = ASE_NULL; 
	void* fs_rex_free = ASE_NULL;
	int errnum;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = ase_awk_getarg (run, 0);
	a1 = ase_awk_getarg (run, 1);
	a2 = (nargs >= 3)? ase_awk_getarg (run, 2): ASE_NULL;

	ASE_ASSERT (a1->type == ASE_AWK_VAL_REF);

	if (((ase_awk_val_ref_t*)a1)->id >= ASE_AWK_VAL_REF_NAMEDIDX &&
	    ((ase_awk_val_ref_t*)a1)->id <= ASE_AWK_VAL_REF_ARGIDX)
	{
		/* an indexed value should not be assigned another map */
		ase_awk_setrunerrnum (run, ASE_AWK_EIDXVALASSMAP);
		return -1;
	}

	if (((ase_awk_val_ref_t*)a1)->id == ASE_AWK_VAL_REF_POS)
	{
		/* a positional should not be assigned a map */
		ase_awk_setrunerrnum (run, ASE_AWK_EPOSVALASSMAP);
		return -1;
	}

	a1_ref = (ase_awk_val_t**)((ase_awk_val_ref_t*)a1)->adr;
	if ((*a1_ref)->type != ASE_AWK_VAL_NIL &&
	    (*a1_ref)->type != ASE_AWK_VAL_MAP)
	{
		/* cannot change a scalar value to a map */
		ase_awk_setrunerrnum (run, ASE_AWK_ESCALARTOMAP);
		return -1;
	}

	if (a0->type == ASE_AWK_VAL_STR)
	{
		str = ((ase_awk_val_str_t*)a0)->buf;
		str_len = ((ase_awk_val_str_t*)a0)->len;
		str_free = ASE_NULL;
	}
	else 
	{
		str = ase_awk_valtostr (
			run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &str_len);
		if (str == ASE_NULL) return -1;
		str_free = str;
	}

	if (a2 == ASE_NULL)
	{
		/* get the value from FS */
		t1 = ase_awk_getglobal (run, ASE_AWK_GLOBAL_FS);
		if (t1->type == ASE_AWK_VAL_NIL)
		{
			fs_ptr = ASE_T(" ");
			fs_len = 1;
			fs_free = ASE_NULL;
		}
		else if (t1->type == ASE_AWK_VAL_STR)
		{
			fs_ptr = ((ase_awk_val_str_t*)t1)->buf;
			fs_len = ((ase_awk_val_str_t*)t1)->len;
			fs_free = ASE_NULL;
		}
		else
		{
			fs_ptr = ase_awk_valtostr (
				run, t1, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &fs_len);
			if (fs_ptr == ASE_NULL)
			{
				if (str_free != ASE_NULL) 
					ASE_AWK_FREE (run->awk, str_free);
				return -1;
			}
			fs_free = fs_ptr;
		}

		if (fs_len > 1) 
		{
			fs_rex = run->global.fs;
			fs_rex_free = ASE_NULL;
		}
	}
	else
	{
		if (a2->type == ASE_AWK_VAL_STR)
		{
			fs_ptr = ((ase_awk_val_str_t*)a2)->buf;
			fs_len = ((ase_awk_val_str_t*)a2)->len;
			fs_free = ASE_NULL;
		}
		else
		{
			fs_ptr = ase_awk_valtostr (
				run, a2, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &fs_len);
			if (fs_ptr == ASE_NULL)
			{
				if (str_free != ASE_NULL) 
					ASE_AWK_FREE (run->awk, str_free);
				return -1;
			}
			fs_free = fs_ptr;
		}

		if (fs_len > 1) 
		{
			fs_rex = ase_awk_buildrex (
				run->awk, fs_ptr, fs_len, &errnum);
			if (fs_rex == ASE_NULL)
			{
				if (str_free != ASE_NULL) 
					ASE_AWK_FREE (run->awk, str_free);
				if (fs_free != ASE_NULL) 
					ASE_AWK_FREE (run->awk, fs_free);
				ase_awk_setrunerrnum (run, errnum);
				return -1;
			}
			fs_rex_free = fs_rex;
		}
	}

	t1 = ase_awk_makemapval (run);
	if (t1 == ASE_NULL)
	{
		if (str_free != ASE_NULL) 
			ASE_AWK_FREE (run->awk, str_free);
		if (fs_free != ASE_NULL) 
			ASE_AWK_FREE (run->awk, fs_free);
		if (fs_rex_free != ASE_NULL) 
			ase_awk_freerex (run->awk, fs_rex_free);
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_awk_refdownval (run, *a1_ref);
	*a1_ref = t1;
	ase_awk_refupval (run, *a1_ref);

	p = str; str_left = str_len; 
	sta = (ase_awk_getoption(run->awk) & ASE_AWK_BASEONE)? 1: 0;
	num = sta;

	while (p != ASE_NULL)
	{
		if (fs_len <= 1)
		{
			p = ase_awk_strxntok (run, 
				p, str_len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = ase_awk_strxntokbyrex (run, p, str_len, 
				fs_rex, &tok, &tok_len, &errnum); 
			if (p == ASE_NULL && errnum != ASE_AWK_ENOERR)
			{
				if (str_free != ASE_NULL) 
					ASE_AWK_FREE (run->awk, str_free);
				if (fs_free != ASE_NULL) 
					ASE_AWK_FREE (run->awk, fs_free);
				if (fs_rex_free != ASE_NULL) 
					ase_awk_freerex (run->awk, fs_rex_free);
				ase_awk_setrunerrnum (run, errnum);
				return -1;
			}
		}

		if (num == 0 && p == ASE_NULL && tok_len == 0) 
		{
			/* no field at all*/
			break; 
		}	

		ASE_ASSERT (
			(tok != ASE_NULL && tok_len > 0) || tok_len == 0);

		/* create the field string */
		t2 = ase_awk_makestrval (run, tok, tok_len);
		if (t2 == ASE_NULL)
		{
			if (str_free != ASE_NULL)
				ASE_AWK_FREE (run->awk, str_free);
			if (fs_free != ASE_NULL)
				ASE_AWK_FREE (run->awk, fs_free);
			if (fs_rex_free != ASE_NULL)
				ase_awk_freerex (run->awk, fs_rex_free);
			/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
			return -1;
		}

		/* put it into the map */
		key_len = ase_awk_longtostr (
			num, 10, ASE_NULL, key, ASE_COUNTOF(key));
		ASE_ASSERT (key_len != (ase_size_t)-1);

		/* don't forget to update the reference count when you 
		 * handle the assignment-like situation.  anyway, it is 
		 * incremented in advance as if the assignment was successful.
		 * it is decremented if the assignement fails. */
		ase_awk_refupval (run, t2);

		if (ase_awk_map_putx (
			((ase_awk_val_map_t*)t1)->map, 
			key, key_len, t2, ASE_NULL) == -1)
		{
			ase_awk_refdownval (run, t2);

			if (str_free != ASE_NULL) 
				ASE_AWK_FREE (run->awk, str_free);
			if (fs_free != ASE_NULL) 
				ASE_AWK_FREE (run->awk, fs_free);
			if (fs_rex_free != ASE_NULL)
				ase_awk_freerex (run->awk, fs_rex_free);
			ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
			return -1;
		}

		num++;
		str_len = str_left - (p - str);
	}

	if (str_free != ASE_NULL) ASE_AWK_FREE (run->awk, str_free);
	if (fs_free != ASE_NULL) ASE_AWK_FREE (run->awk, fs_free);
	if (fs_rex_free != ASE_NULL) ase_awk_freerex (run->awk, fs_rex_free);

	if (sta == 1) num--;

	t1 = ase_awk_makeintval (run, num);
	if (t1 == ASE_NULL)
	{
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_awk_setretval (run, t1);
	return 0;
}

static int bfn_tolower (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_char_t* str;
	ase_size_t len, i;
	ase_awk_val_t* a0, * r;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 1);

	a0 = ase_awk_getarg (run, 0);

	if (a0->type == ASE_AWK_VAL_STR)
	{
		str = ((ase_awk_val_str_t*)a0)->buf;
		len = ((ase_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = ase_awk_valtostr (
			run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (str == ASE_NULL) return -1;
	}

	for (i = 0; i < len; i++) str[i] = ASE_AWK_TOLOWER (run->awk, str[i]);	

	r = ase_awk_makestrval (run, str, len);
	if (r == ASE_NULL)
	{
		if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str);
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str);
	ase_awk_setretval (run, r);
	return 0;
}

static int bfn_toupper (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_char_t* str;
	ase_size_t len, i;
	ase_awk_val_t* a0, * r;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 1);

	a0 = ase_awk_getarg (run, 0);

	if (a0->type == ASE_AWK_VAL_STR)
	{
		str = ((ase_awk_val_str_t*)a0)->buf;
		len = ((ase_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = ase_awk_valtostr (
			run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (str == ASE_NULL) return -1;
	}

	for (i = 0; i < len; i++) str[i] = ASE_AWK_TOUPPER (run->awk, str[i]);	

	r = ase_awk_makestrval (run, str, len);
	if (r == ASE_NULL)
	{
		if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str);
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str);
	ase_awk_setretval (run, r);
	return 0;
}

static int __substitute (ase_awk_run_t* run, ase_long_t max_count)
{
	ase_size_t nargs;
	ase_awk_val_t* a0, * a1, * a2, ** a2_ref, * v;
	ase_char_t* a0_ptr, * a1_ptr, * a2_ptr;
	ase_size_t a0_len, a1_len, a2_len;
	ase_char_t* a0_ptr_free = ASE_NULL;
	ase_char_t* a1_ptr_free = ASE_NULL;
	ase_char_t* a2_ptr_free = ASE_NULL;
	void* rex = ASE_NULL;
	int opt, n;
	const ase_char_t* cur_ptr, * mat_ptr;
	ase_size_t cur_len, mat_len, i, m;
	ase_str_t new;
	ase_long_t sub_count;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = ase_awk_getarg (run, 0);
	a1 = ase_awk_getarg (run, 1);
	a2 = (nargs >= 3)? ase_awk_getarg (run, 2): ASE_NULL;

	ASE_ASSERT (a2 == ASE_NULL || a2->type == ASE_AWK_VAL_REF);

#define FREE_A_PTRS(awk) \
	do { \
		if (a2_ptr_free != ASE_NULL) ASE_AWK_FREE (awk, a2_ptr_free); \
		if (a1_ptr_free != ASE_NULL) ASE_AWK_FREE (awk, a1_ptr_free); \
		if (a0_ptr_free != ASE_NULL) ASE_AWK_FREE (awk, a0_ptr_free); \
	} while (0)
#define FREE_A0_REX(awk,rex) \
	do { \
		if (a0->type != ASE_AWK_VAL_REX) ase_awk_freerex (awk, rex); \
	} while (0)

	if (a0->type == ASE_AWK_VAL_REX)
	{
		rex = ((ase_awk_val_rex_t*)a0)->code;
	}
	else if (a0->type == ASE_AWK_VAL_STR)
	{
		a0_ptr = ((ase_awk_val_str_t*)a0)->buf;
		a0_len = ((ase_awk_val_str_t*)a0)->len;
	}
	else
	{
		a0_ptr = ase_awk_valtostr (
			run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &a0_len);
		if (a0_ptr == ASE_NULL) 
		{
			FREE_A_PTRS (run->awk);
			return -1;
		}	
		a0_ptr_free = a0_ptr;
	}

	if (a1->type == ASE_AWK_VAL_STR)
	{
		a1_ptr = ((ase_awk_val_str_t*)a1)->buf;
		a1_len = ((ase_awk_val_str_t*)a1)->len;
	}
	else
	{
		a1_ptr = ase_awk_valtostr (
			run, a1, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &a1_len);
		if (a1_ptr == ASE_NULL) 
		{
			FREE_A_PTRS (run->awk);
			return -1;
		}
		a1_ptr_free = a1_ptr;
	}

	if (a2 == ASE_NULL)
	{
		/* is this correct? any needs to use inrec.d0? */
		a2_ptr = ASE_STR_BUF(&run->inrec.line);
		a2_len = ASE_STR_LEN(&run->inrec.line);
	}
	else if (((ase_awk_val_ref_t*)a2)->id == ASE_AWK_VAL_REF_POS)
	{
		ase_size_t idx;
	       
		idx = (ase_size_t)((ase_awk_val_ref_t*)a2)->adr;
		if (idx == 0)
		{
			a2_ptr = ASE_STR_BUF(&run->inrec.line);
			a2_len = ASE_STR_LEN(&run->inrec.line);
		}
		else if (idx <= run->inrec.nflds)
		{
			a2_ptr = run->inrec.flds[idx-1].ptr;
			a2_len = run->inrec.flds[idx-1].len;
		}
		else
		{
			a2_ptr = ASE_T("");
			a2_len = 0;
		}
	}
	else
	{
		a2_ref = (ase_awk_val_t**)((ase_awk_val_ref_t*)a2)->adr;

		if ((*a2_ref)->type == ASE_AWK_VAL_MAP)
		{
			FREE_A_PTRS (run->awk);
			/* a map is not allowed as the third parameter */
			ase_awk_setrunerrnum (run, ASE_AWK_EMAPNOTALLOWED);
			return -1;
		}

		if ((*a2_ref)->type == ASE_AWK_VAL_STR)
		{
			a2_ptr = ((ase_awk_val_str_t*)(*a2_ref))->buf;
			a2_len = ((ase_awk_val_str_t*)(*a2_ref))->len;
		}
		else
		{
			a2_ptr = ase_awk_valtostr (
				run, *a2_ref, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &a2_len);
			if (a2_ptr == ASE_NULL) 
			{
				FREE_A_PTRS (run->awk);
				return -1;
			}
			a2_ptr_free = a2_ptr;
		}
	}

	if (ase_str_open (&new, a2_len, &run->awk->prmfns.mmgr) == ASE_NULL)
	{
		FREE_A_PTRS (run->awk);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != ASE_AWK_VAL_REX)
	{
		rex = ase_awk_buildrex (run->awk, a0_ptr, a0_len, &run->errnum);
		if (rex == ASE_NULL)
		{
			ase_str_close (&new);
			FREE_A_PTRS (run->awk);
			return -1;
		}
	}

	opt = (run->global.ignorecase)? ASE_AWK_REX_IGNORECASE: 0;
	cur_ptr = a2_ptr;
	cur_len = a2_len;
	sub_count = 0;

	while (1)
	{
		if (max_count == 0 || sub_count < max_count)
		{
			n = ase_awk_matchrex (
				run->awk, rex, opt, cur_ptr, cur_len,
				&mat_ptr, &mat_len, &run->errnum);
		}
		else n = 0;

		if (n == -1)
		{
			FREE_A0_REX (run->awk, rex);
			ase_str_close (&new);
			FREE_A_PTRS (run->awk);
			return -1;
		}

		if (n == 0) 
		{ 
			/* no more match found */
			if (ase_str_ncat (
				&new, cur_ptr, cur_len) == (ase_size_t)-1)
			{
				FREE_A0_REX (run->awk, rex);
				ase_str_close (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
			break;
		}

		if (ase_str_ncat (
			&new, cur_ptr, mat_ptr - cur_ptr) == (ase_size_t)-1)
		{
			FREE_A0_REX (run->awk, rex);
			ase_str_close (&new);
			FREE_A_PTRS (run->awk);
			return -1;
		}

		for (i = 0; i < a1_len; i++)
		{
			if ((i+1) < a1_len && 
			    a1_ptr[i] == ASE_T('\\') && 
			    a1_ptr[i+1] == ASE_T('&'))
			{
				m = ase_str_ccat (&new, ASE_T('&'));
				i++;
			}
			else if (a1_ptr[i] == ASE_T('&'))
			{
				m = ase_str_ncat (&new, mat_ptr, mat_len);
			}
			else 
			{
				m = ase_str_ccat (&new, a1_ptr[i]);
			}

			if (m == (ase_size_t)-1)
			{
				FREE_A0_REX (run->awk, rex);
				ase_str_close (&new);
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
		if (a2 == ASE_NULL)
		{
			if (ase_awk_setrec (run, 0,
				ASE_STR_BUF(&new), ASE_STR_LEN(&new)) == -1)
			{
				ase_str_close (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
		}
		else if (((ase_awk_val_ref_t*)a2)->id == ASE_AWK_VAL_REF_POS)
		{
			int n;

			n = ase_awk_setrec (
				run, (ase_size_t)((ase_awk_val_ref_t*)a2)->adr,
				ASE_STR_BUF(&new), ASE_STR_LEN(&new));

			if (n == -1)
			{
				ase_str_close (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
		}
		else
		{
			v = ase_awk_makestrval (run,
				ASE_STR_BUF(&new), ASE_STR_LEN(&new));
			if (v == ASE_NULL)
			{
				ase_str_close (&new);
				FREE_A_PTRS (run->awk);
				/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
				return -1;
			}

			ase_awk_refdownval (run, *a2_ref);
			*a2_ref = v;
			ase_awk_refupval (run, *a2_ref);
		}
	}

	ase_str_close (&new);
	FREE_A_PTRS (run->awk);

#undef FREE_A0_REX
#undef FREE_A_PTRS

	v = ase_awk_makeintval (run, sub_count);
	if (v == ASE_NULL)
	{
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_awk_setretval (run, v);
	return 0;
}

static int bfn_gsub (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return __substitute (run, 0);
}

static int bfn_sub (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return __substitute (run, 1);
}

static int bfn_match (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* a0, * a1;
	ase_char_t* str0, * str1;
	ase_size_t len0, len1;
	ase_long_t idx;
	void* rex;
	int opt, n;
	const ase_char_t* mat_ptr;
	ase_size_t mat_len;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 2);
	
	a0 = ase_awk_getarg (run, 0);
	a1 = ase_awk_getarg (run, 1);

	if (a0->type == ASE_AWK_VAL_STR)
	{
		str0 = ((ase_awk_val_str_t*)a0)->buf;
		len0 = ((ase_awk_val_str_t*)a0)->len;
	}
	else
	{
		str0 = ase_awk_valtostr (
			run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len0);
		if (str0 == ASE_NULL) return -1;
	}

	if (a1->type == ASE_AWK_VAL_REX)
	{
		rex = ((ase_awk_val_rex_t*)a1)->code;
	}
	else 
	{
		if (a1->type == ASE_AWK_VAL_STR)
		{
			str1 = ((ase_awk_val_str_t*)a1)->buf;
			len1 = ((ase_awk_val_str_t*)a1)->len;
		}
		else
		{
			str1 = ase_awk_valtostr (
				run, a1, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len1);
			if (str1 == ASE_NULL)
			{
				if (a0->type != ASE_AWK_VAL_STR) 
					ASE_AWK_FREE (run->awk, str0);
				return -1;
			}
		}

		rex = ase_awk_buildrex (run->awk, str1, len1, &run->errnum);
		if (rex == ASE_NULL)
		{
			if (a0->type != ASE_AWK_VAL_STR) 
				ASE_AWK_FREE (run->awk, str0);
			return -1;
		}

		if (a1->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str1);
	}

	opt = (run->global.ignorecase)? ASE_AWK_REX_IGNORECASE: 0;
	n = ase_awk_matchrex (
		run->awk, rex, opt, str0, len0,
		&mat_ptr, &mat_len, &run->errnum);

	if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str0);
	if (a1->type != ASE_AWK_VAL_REX) ase_awk_freerex (run->awk, rex);

	if (n == -1) return -1;

	idx = (n == 0)? -1: (ase_long_t)(mat_ptr - str0);
	if (ase_awk_getoption(run->awk) & ASE_AWK_BASEONE) idx = idx + 1;

	a0 = ase_awk_makeintval (run, idx);
	if (a0 == ASE_NULL)
	{
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_awk_refupval (run, a0);

	a1 = ase_awk_makeintval (run, 
		((n == 0)? (ase_long_t)-1: (ase_long_t)mat_len));
	if (a1 == ASE_NULL)
	{
		ase_awk_refdownval (run, a0);
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_awk_refupval (run, a1);

	if (ase_awk_setglobal (run, ASE_AWK_GLOBAL_RSTART, a0) == -1)
	{
		ase_awk_refdownval (run, a1);
		ase_awk_refdownval (run, a0);
		return -1;
	}

	if (ase_awk_setglobal (run, ASE_AWK_GLOBAL_RLENGTH, a1) == -1)
	{
		ase_awk_refdownval (run, a1);
		ase_awk_refdownval (run, a0);
		return -1;
	}

	ase_awk_setretval (run, a0);

	ase_awk_refdownval (run, a1);
	ase_awk_refdownval (run, a0);
	return 0;
}

static int bfn_sprintf (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{	
	ase_size_t nargs;
	ase_awk_val_t* a0;
	ase_char_t* str0, * ptr;
	ase_size_t len0, len;
	ase_str_t out, fbu;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs > 0);

	if (ase_str_open (&out, 256, &run->awk->prmfns.mmgr) == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}
	if (ase_str_open (&fbu, 256, &run->awk->prmfns.mmgr) == ASE_NULL)
	{
		ase_str_close (&out);
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	a0 = ase_awk_getarg (run, 0);
	if (a0->type == ASE_AWK_VAL_STR)
	{
		str0 = ((ase_awk_val_str_t*)a0)->buf;
		len0 = ((ase_awk_val_str_t*)a0)->len;
	}
	else
	{
		str0 = ase_awk_valtostr (
			run, a0, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len0);
		if (str0 == ASE_NULL) 
		{
			ase_str_close (&fbu);
			ase_str_close (&out);
			return -1;
		}
	}

	ptr = ase_awk_format (run, 
		&out, &fbu, str0, len0, nargs, ASE_NULL, &len);
	if (a0->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str0);
	if (ptr == ASE_NULL) 
	{
		ase_str_close (&fbu);
		ase_str_close (&out);
		return -1;
	}
	
	a0 = ase_awk_makestrval_nodup (run, ptr, len);
	if (a0 == ASE_NULL) 
	{
		ase_str_close (&fbu);
		ase_str_close (&out);
		/*ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);*/
		return -1;
	}

	ase_str_close (&fbu);
	ase_str_forfeit (&out);
	ase_awk_setretval (run, a0);
	return 0;
}
