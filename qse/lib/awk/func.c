/*
 * $Id: func.c 499 2008-12-16 09:42:48Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "awk.h"

static int bfn_close   (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_fflush  (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_index   (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_length  (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_substr  (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_split   (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_tolower (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_toupper (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_gsub    (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_sub     (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_match   (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);
static int bfn_sprintf (qse_awk_rtx_t*, const qse_char_t*, qse_size_t);

#undef MAX
#define MAX QSE_TYPE_UNSIGNED_MAX(qse_size_t)

static qse_awk_bfn_t sys_bfn[] = 
{
	/* io functions */
	{ {QSE_T("close"),   5}, QSE_AWK_EIO, {1, 1, QSE_NULL}, bfn_close},
	{ {QSE_T("fflush"),  6}, QSE_AWK_EIO, {0, 1, QSE_NULL}, bfn_fflush},

	/* string functions */
	{ {QSE_T("index"),   5}, 0,  {2,   2, QSE_NULL},     bfn_index},
	{ {QSE_T("substr"),  6}, 0,  {2,   3, QSE_NULL},     bfn_substr},
	{ {QSE_T("length"),  6}, 0,  {1,   1, QSE_NULL},     bfn_length},
	{ {QSE_T("split"),   5}, 0,  {2,   3, QSE_T("vrv")}, bfn_split},
	{ {QSE_T("tolower"), 7}, 0,  {1,   1, QSE_NULL},     bfn_tolower},
	{ {QSE_T("toupper"), 7}, 0,  {1,   1, QSE_NULL},     bfn_toupper},
	{ {QSE_T("gsub"),    4}, 0,  {2,   3, QSE_T("xvr")}, bfn_gsub},
	{ {QSE_T("sub"),     3}, 0,  {2,   3, QSE_T("xvr")}, bfn_sub},
	{ {QSE_T("match"),   5}, 0,  {2,   2, QSE_T("vx")},  bfn_match},
	{ {QSE_T("sprintf"), 7}, 0,  {1, MAX, QSE_NULL},     bfn_sprintf},

	{ {QSE_NULL,         0}, 0,  {0,   0, QSE_NULL},     QSE_NULL}
};

void* qse_awk_addfunc (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t name_len, 
	int when_valid, qse_size_t min_args, qse_size_t max_args, 
	const qse_char_t* arg_spec, 
	int (*handler)(qse_awk_rtx_t*,const qse_char_t*,qse_size_t))
{
	qse_awk_bfn_t* bfn;
	qse_size_t spec_len;

	if (name_len <= 0)
	{
		qse_awk_seterror (awk, QSE_AWK_EINVAL, 0, QSE_NULL, 0);
		return QSE_NULL;
	}

	if (qse_awk_getbfn (awk, name, name_len) != QSE_NULL)
	{
		qse_cstr_t errarg;

		errarg.ptr = name;
		errarg.len = name_len;

		qse_awk_seterror (awk, QSE_AWK_EEXIST, 0, &errarg, 1);
		return QSE_NULL;
	}

	spec_len = (arg_spec == QSE_NULL)? 0: qse_strlen(arg_spec);

	bfn = (qse_awk_bfn_t*) QSE_AWK_ALLOC (awk, 
		QSE_SIZEOF(qse_awk_bfn_t) + 
		(name_len+1) * QSE_SIZEOF(qse_char_t) +
		(spec_len+1) * QSE_SIZEOF(qse_char_t));
	if (bfn == QSE_NULL)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}

	bfn->name.ptr = (qse_char_t*)(bfn + 1);
	bfn->name.len = name_len;
	qse_strxncpy (bfn->name.ptr, name_len+1, name, name_len);

	bfn->valid = when_valid;
	bfn->arg.min = min_args;
	bfn->arg.max = max_args;

	if (arg_spec == QSE_NULL) bfn->arg.spec = QSE_NULL;
	else
	{
		bfn->arg.spec = bfn->name.ptr + bfn->name.len + 1;
		qse_strxcpy (bfn->arg.spec, spec_len+1, arg_spec); 
	}

	bfn->handler = handler;

	if (qse_map_insert (awk->bfn.user,
		(qse_char_t*)name, name_len, bfn, 0) == QSE_NULL)
	{
		QSE_AWK_FREE (awk, bfn);
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM);
		return QSE_NULL;
	}

	return bfn;
}

int qse_awk_delfunc (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t name_len)
{
	if (qse_map_delete (awk->bfn.user, name, name_len) == -1)
	{
		qse_cstr_t errarg;

		errarg.ptr = name;
		errarg.len = name_len;

		qse_awk_seterror (awk, QSE_AWK_ENOENT, 0, &errarg, 1);
		return -1;
	}

	return 0;
}

void qse_awk_clrbfn (qse_awk_t* awk)
{
	qse_map_clear (awk->bfn.user);
}

qse_awk_bfn_t* qse_awk_getbfn (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t len)
{
	qse_awk_bfn_t* bfn;
	qse_map_pair_t* pair;
	const qse_char_t* k;
	qse_size_t l;

	/* search the system function table */
	for (bfn = sys_bfn; bfn->name.ptr != QSE_NULL; bfn++)
	{
		if (bfn->valid != 0 && 
		    (awk->option & bfn->valid) != bfn->valid) continue;

		pair = qse_map_search (
			awk->wtab, bfn->name.ptr, bfn->name.len);
		if (pair != QSE_NULL)
		{
			/* found in the customized word table */
			k = QSE_MAP_VPTR(pair);
			l = QSE_MAP_VLEN(pair);
		}
		else
		{
			k = bfn->name.ptr;
			l = bfn->name.len;
		}

		if (qse_strxncmp (k, l, name, len) == 0) return bfn;
	}

	/* NOTE: I suspect this block of code might be very fragile.
	 *       because I'm trying to support qse_awk_setword in 
	 *       a very flimsy way here. Would it be better to drop
	 *       qse_awk_setword totally? */
	pair = qse_map_search (awk->rwtab, name, len);
	if (pair != QSE_NULL)
	{
		/* the current name is a target name for
		 * one of the original word. */
		k = QSE_MAP_VPTR(pair);
		l = QSE_MAP_VLEN(pair);
	}
	else
	{
		pair = qse_map_search (awk->wtab, name, len);
		if (pair != QSE_NULL)
		{
			k = QSE_MAP_VPTR(pair);
			l = QSE_MAP_VLEN(pair);

			if (qse_strxncmp (name, len, k, l) != 0)
			{
				/* it name is not a target name but has
				 * a target name different from itself,
				 * it cannot be a intrinsic function name.
				 *
				 * For instance, name is "sin" here after
				 * qse_awk_setword ("sin", "cain") is called.
				 * If name were "cain", it would be handled
				 * in the outmost if block */

				return QSE_NULL;
			}
		}
		else
		{
			k = name;
			l = len;
		}
	}
	/* END NOTE */

	pair = qse_map_search (awk->bfn.user, k, l);
	if (pair == QSE_NULL) return QSE_NULL;

	bfn = (qse_awk_bfn_t*)QSE_MAP_VPTR(pair);
	if (bfn->valid != 0 && (awk->option & bfn->valid) == 0) return QSE_NULL;

	return bfn;
}

static int bfn_close (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* v, * a0;
	int n;

	qse_char_t* name;
	qse_size_t len;
       
	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);
/* TODO: support close (xxx, "to"/"from"/"rw"/"r"/"w"/????) */

	a0 = qse_awk_rtx_getarg (run, 0);
	QSE_ASSERT (a0 != QSE_NULL);

	if (a0->type == QSE_AWK_VAL_STR)
	{
		name = ((qse_awk_val_str_t*)a0)->ptr;
		len = ((qse_awk_val_str_t*)a0)->len;
	}
	else
	{
		name = qse_awk_valtostr (
			run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (name == QSE_NULL) return -1;
	}

	if (len == 0)
	{
		/* getline or print doesn't allow an emptry for the 
		 * input or output file name. so close should not allow 
		 * it either.  
		 * another reason for this is if close is called explicitly 
		 * with an empty string, it may close the console that uses 
		 * an empty string for its identification because closeeio
		 * closes any eios that match the name given unlike 
		 * closeeio_read or closeeio_write. */ 
		n = -1;
		goto skip_close;
	}

	while (len > 0)
	{
		if (name[--len] == QSE_T('\0'))
		{
			/* the name contains a null string. 
			 * make close return -1 */
			n = -1;
			goto skip_close;
		}
	}	

	n = qse_awk_closeeio (run, name);
	/*
	if (n == -1 && run->errnum != QSE_AWK_EIONONE)
	{
		if (a0->type != QSE_AWK_VAL_STR) 
			QSE_AWK_FREE (run->awk, name);
		return -1;
	}
	*/

skip_close:
	if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, name);

	v = qse_awk_makeintval (run, (qse_long_t)n);
	if (v == QSE_NULL)
	{
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_rtx_setretval (run, v);
	return 0;
}

static int flush_eio (
	qse_awk_rtx_t* run, int eio, const qse_char_t* name, int n)
{
	int n2;

	if (run->eio.handler[eio] != QSE_NULL)
	{
		n2 = qse_awk_flusheio (run, eio, name);
		if (n2 == -1)
		{
			/*
			if (run->errnum == QSE_AWK_EIOIMPL) n = -1;
			else if (run->errnum == QSE_AWK_EIONONE) 
			{
				if (n != 0) n = -2;
			}
			else n = -99; 
			*/	
			if (run->errnum == QSE_AWK_EIONONE) 
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
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_char_t* str0;
	qse_size_t len0;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 0 || nargs == 1);

	if (nargs == 0)
	{
		/* flush the console output.
		 * fflush() should return -1 on errors */
		n = qse_awk_flusheio (run, QSE_AWK_OUT_CONSOLE, QSE_T(""));
	}
	else
	{
		qse_char_t* ptr, * end;

		a0 = qse_awk_rtx_getarg (run, 0);
		if (a0->type == QSE_AWK_VAL_STR)
		{
			str0 = ((qse_awk_val_str_t*)a0)->ptr;
			len0 = ((qse_awk_val_str_t*)a0)->len;
		}
		else
		{
			str0 = qse_awk_valtostr (
				run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len0);
			if (str0 == QSE_NULL) return -1;

		}

		/* the target name contains a null character.
		 * make fflush return -1 */
		ptr = str0; end = str0 + len0;
		while (ptr < end)
		{
			if (*ptr == QSE_T('\0')) 
			{
				n = -1;
				goto skip_flush;
			}

			ptr++;
		}

		/* flush the given eio */
		n = flush_eio (
			run, QSE_AWK_EIO_FILE, 
			((len0 == 0)? QSE_NULL: str0), 1);
		/*if (n == -99) return -1;*/
		n = flush_eio (
			run, QSE_AWK_EIO_PIPE,
			((len0 == 0)? QSE_NULL: str0), n);
		/*if (n == -99) return -1;*/

		/* if n remains 1, no ip handlers have been defined for
		 * file, pipe, and rwpipe. so make fflush return -1. 
		 * if n is -2, no such named io has been found at all 
		 * if n is -1, the io handler has returned an error */
		if (n != 0) n = -1;

	skip_flush:
		if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str0);
	}

	a0 = qse_awk_makeintval (run, (qse_long_t)n);
	if (a0 == QSE_NULL)
	{
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_rtx_setretval (run, a0);
	return 0;
}

static int bfn_index (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_char_t* str0, * str1, * ptr;
	qse_size_t len0, len1;
	qse_long_t idx;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 2);
	
	a0 = qse_awk_rtx_getarg (run, 0);
	a1 = qse_awk_rtx_getarg (run, 1);

	if (a0->type == QSE_AWK_VAL_STR)
	{
		str0 = ((qse_awk_val_str_t*)a0)->ptr;
		len0 = ((qse_awk_val_str_t*)a0)->len;
	}
	else
	{
		str0 = qse_awk_valtostr (
			run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len0);
		if (str0 == QSE_NULL) return -1;
	}

	if (a1->type == QSE_AWK_VAL_STR)
	{
		str1 = ((qse_awk_val_str_t*)a1)->ptr;
		len1 = ((qse_awk_val_str_t*)a1)->len;
	}
	else
	{
		str1 = qse_awk_valtostr (
			run, a1, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len1);
		if (str1 == QSE_NULL)
		{
			if (a0->type != QSE_AWK_VAL_STR) 
				QSE_AWK_FREE (run->awk, str0);
			return -1;
		}
	}

	ptr = qse_strxnstr (str0, len0, str1, len1);
	idx = (ptr == QSE_NULL)? -1: (qse_long_t)(ptr - str0);

	if (qse_awk_getoption(run->awk) & QSE_AWK_BASEONE) idx = idx + 1;

	if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str0);
	if (a1->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str1);

	a0 = qse_awk_makeintval (run, idx);
	if (a0 == QSE_NULL)
	{
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_rtx_setretval (run, a0);
	return 0;
}

static int bfn_length (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* v;
	qse_char_t* str;
	qse_size_t len;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);
	
	v = qse_awk_rtx_getarg (run, 0);
	if (v->type == QSE_AWK_VAL_STR)
	{
		len = ((qse_awk_val_str_t*)v)->len;
	}
	else
	{
		str = qse_awk_valtostr (
			run, v, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (str == QSE_NULL) return -1;
		QSE_AWK_FREE (run->awk, str);
	}

	v = qse_awk_makeintval (run, len);
	if (v == QSE_NULL)
	{
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_rtx_setretval (run, v);
	return 0;
}

static int bfn_substr (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a2, * r;
	qse_char_t* str;
	qse_size_t len;
	qse_long_t lindex, lcount;
	qse_real_t rindex, rcount;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = qse_awk_rtx_getarg (run, 0);
	a1 = qse_awk_rtx_getarg (run, 1);
	a2 = (nargs >= 3)? qse_awk_rtx_getarg (run, 2): QSE_NULL;

	if (a0->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)a0)->ptr;
		len = ((qse_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = qse_awk_valtostr (
			run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (str == QSE_NULL) return -1;
	}

	n = qse_awk_valtonum (run, a1, &lindex, &rindex);
	if (n == -1) 
	{
		if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);
		return -1;
	}
	if (n == 1) lindex = (qse_long_t)rindex;

	if (a2 == QSE_NULL) lcount = (qse_long_t)len;
	else 
	{
		n = qse_awk_valtonum (run, a2, &lcount, &rcount);
		if (n == -1) 
		{
			if (a0->type != QSE_AWK_VAL_STR) 
				QSE_AWK_FREE (run->awk, str);
			return -1;
		}
		if (n == 1) lcount = (qse_long_t)rcount;
	}

	if (qse_awk_getoption(run->awk) & QSE_AWK_BASEONE) lindex = lindex - 1;
	if (lindex >= (qse_long_t)len) lindex = (qse_long_t)len;
	else if (lindex < 0) lindex = 0;

	if (lcount < 0) lcount = 0;
	else if (lcount > (qse_long_t)len - lindex) 
	{
		lcount = (qse_long_t)len - lindex;
	}

	r = qse_awk_makestrval (run, &str[lindex], (qse_size_t)lcount);
	if (r == QSE_NULL)
	{
		if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);
	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int bfn_split (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a2, * t1, * t2, ** a1_ref;
	qse_char_t* str, * str_free, * p, * tok;
	qse_size_t str_len, str_left, tok_len;
	qse_long_t sta, num;
	qse_char_t key[QSE_SIZEOF(qse_long_t)*8+2];
	qse_size_t key_len;
	qse_char_t* fs_ptr, * fs_free;
	qse_size_t fs_len;
	void* fs_rex = QSE_NULL; 
	void* fs_rex_free = QSE_NULL;
	int errnum;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = qse_awk_rtx_getarg (run, 0);
	a1 = qse_awk_rtx_getarg (run, 1);
	a2 = (nargs >= 3)? qse_awk_rtx_getarg (run, 2): QSE_NULL;

	QSE_ASSERT (a1->type == QSE_AWK_VAL_REF);

	if (((qse_awk_val_ref_t*)a1)->id >= QSE_AWK_VAL_REF_NAMEDIDX &&
	    ((qse_awk_val_ref_t*)a1)->id <= QSE_AWK_VAL_REF_ARGIDX)
	{
		/* an indexed value should not be assigned another map */
		qse_awk_setrunerrnum (run, QSE_AWK_EIDXVALASSMAP);
		return -1;
	}

	if (((qse_awk_val_ref_t*)a1)->id == QSE_AWK_VAL_REF_POS)
	{
		/* a positional should not be assigned a map */
		qse_awk_setrunerrnum (run, QSE_AWK_EPOSVALASSMAP);
		return -1;
	}

	a1_ref = (qse_awk_val_t**)((qse_awk_val_ref_t*)a1)->adr;
	if ((*a1_ref)->type != QSE_AWK_VAL_NIL &&
	    (*a1_ref)->type != QSE_AWK_VAL_MAP)
	{
		/* cannot change a scalar value to a map */
		qse_awk_setrunerrnum (run, QSE_AWK_ESCALARTOMAP);
		return -1;
	}

	if (a0->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)a0)->ptr;
		str_len = ((qse_awk_val_str_t*)a0)->len;
		str_free = QSE_NULL;
	}
	else 
	{
		str = qse_awk_valtostr (
			run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &str_len);
		if (str == QSE_NULL) return -1;
		str_free = str;
	}

	if (a2 == QSE_NULL)
	{
		/* get the value from FS */
		t1 = qse_awk_rtx_getglobal (run, QSE_AWK_GLOBAL_FS);
		if (t1->type == QSE_AWK_VAL_NIL)
		{
			fs_ptr = QSE_T(" ");
			fs_len = 1;
			fs_free = QSE_NULL;
		}
		else if (t1->type == QSE_AWK_VAL_STR)
		{
			fs_ptr = ((qse_awk_val_str_t*)t1)->ptr;
			fs_len = ((qse_awk_val_str_t*)t1)->len;
			fs_free = QSE_NULL;
		}
		else
		{
			fs_ptr = qse_awk_valtostr (
				run, t1, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &fs_len);
			if (fs_ptr == QSE_NULL)
			{
				if (str_free != QSE_NULL) 
					QSE_AWK_FREE (run->awk, str_free);
				return -1;
			}
			fs_free = fs_ptr;
		}

		if (fs_len > 1) 
		{
			fs_rex = run->global.fs;
			fs_rex_free = QSE_NULL;
		}
	}
	else
	{
		if (a2->type == QSE_AWK_VAL_STR)
		{
			fs_ptr = ((qse_awk_val_str_t*)a2)->ptr;
			fs_len = ((qse_awk_val_str_t*)a2)->len;
			fs_free = QSE_NULL;
		}
		else
		{
			fs_ptr = qse_awk_valtostr (
				run, a2, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &fs_len);
			if (fs_ptr == QSE_NULL)
			{
				if (str_free != QSE_NULL) 
					QSE_AWK_FREE (run->awk, str_free);
				return -1;
			}
			fs_free = fs_ptr;
		}

		if (fs_len > 1) 
		{
			fs_rex = QSE_AWK_BUILDREX (
				run->awk, fs_ptr, fs_len, &errnum);
			if (fs_rex == QSE_NULL)
			{
				if (str_free != QSE_NULL) 
					QSE_AWK_FREE (run->awk, str_free);
				if (fs_free != QSE_NULL) 
					QSE_AWK_FREE (run->awk, fs_free);
				qse_awk_setrunerrnum (run, errnum);
				return -1;
			}
			fs_rex_free = fs_rex;
		}
	}

	t1 = qse_awk_makemapval (run);
	if (t1 == QSE_NULL)
	{
		if (str_free != QSE_NULL) 
			QSE_AWK_FREE (run->awk, str_free);
		if (fs_free != QSE_NULL) 
			QSE_AWK_FREE (run->awk, fs_free);
		if (fs_rex_free != QSE_NULL) 
			QSE_AWK_FREEREX (run->awk, fs_rex_free);
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_refdownval (run, *a1_ref);
	*a1_ref = t1;
	qse_awk_refupval (run, *a1_ref);

	p = str; str_left = str_len; 
	sta = (qse_awk_getoption(run->awk) & QSE_AWK_BASEONE)? 1: 0;
	num = sta;

	while (p != QSE_NULL)
	{
		if (fs_len <= 1)
		{
			p = qse_awk_strxntok (run, 
				p, str_len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = qse_awk_strxntokbyrex (run, p, str_len, 
				fs_rex, &tok, &tok_len, &errnum); 
			if (p == QSE_NULL && errnum != QSE_AWK_ENOERR)
			{
				if (str_free != QSE_NULL) 
					QSE_AWK_FREE (run->awk, str_free);
				if (fs_free != QSE_NULL) 
					QSE_AWK_FREE (run->awk, fs_free);
				if (fs_rex_free != QSE_NULL) 
					QSE_AWK_FREEREX (run->awk, fs_rex_free);
				qse_awk_setrunerrnum (run, errnum);
				return -1;
			}
		}

		if (num == 0 && p == QSE_NULL && tok_len == 0) 
		{
			/* no field at all*/
			break; 
		}	

		QSE_ASSERT (
			(tok != QSE_NULL && tok_len > 0) || tok_len == 0);

		/* create the field string */
		t2 = qse_awk_makestrval (run, tok, tok_len);
		if (t2 == QSE_NULL)
		{
			if (str_free != QSE_NULL)
				QSE_AWK_FREE (run->awk, str_free);
			if (fs_free != QSE_NULL)
				QSE_AWK_FREE (run->awk, fs_free);
			if (fs_rex_free != QSE_NULL)
				QSE_AWK_FREEREX (run->awk, fs_rex_free);
			/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
			return -1;
		}

		/* put it into the map */
		key_len = qse_awk_longtostr (
			num, 10, QSE_NULL, key, QSE_COUNTOF(key));
		QSE_ASSERT (key_len != (qse_size_t)-1);

		/* don't forget to update the reference count when you 
		 * handle the assignment-like situation.  anyway, it is 
		 * incremented in advance as if the assignment was successful.
		 * it is decremented if the assignement fails. */
		qse_awk_refupval (run, t2);

		if (qse_map_insert (
			((qse_awk_val_map_t*)t1)->map, 
			key, key_len, t2, 0) == QSE_NULL)
		{
			/* assignment failed. restore the reference counter */
			qse_awk_refdownval (run, t2);

			if (str_free != QSE_NULL) 
				QSE_AWK_FREE (run->awk, str_free);
			if (fs_free != QSE_NULL) 
				QSE_AWK_FREE (run->awk, fs_free);
			if (fs_rex_free != QSE_NULL)
				QSE_AWK_FREEREX (run->awk, fs_rex_free);

			/* qse_map_insert() fails if the key exists.
			 * that can't happen here. so set the error code
			 * to ENOMEM */
			qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
			return -1;
		}

		num++;
		str_len = str_left - (p - str);
	}

	if (str_free != QSE_NULL) QSE_AWK_FREE (run->awk, str_free);
	if (fs_free != QSE_NULL) QSE_AWK_FREE (run->awk, fs_free);
	if (fs_rex_free != QSE_NULL) QSE_AWK_FREEREX (run->awk, fs_rex_free);

	if (sta == 1) num--;

	t1 = qse_awk_makeintval (run, num);
	if (t1 == QSE_NULL)
	{
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_rtx_setretval (run, t1);
	return 0;
}

static int bfn_tolower (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_char_t* str;
	qse_size_t len, i;
	qse_awk_val_t* a0, * r;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (run, 0);

	if (a0->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)a0)->ptr;
		len = ((qse_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = qse_awk_valtostr (
			run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (str == QSE_NULL) return -1;
	}

	for (i = 0; i < len; i++) str[i] = QSE_AWK_TOLOWER (run->awk, str[i]);	

	r = qse_awk_makestrval (run, str, len);
	if (r == QSE_NULL)
	{
		if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);
	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int bfn_toupper (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_char_t* str;
	qse_size_t len, i;
	qse_awk_val_t* a0, * r;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (run, 0);

	if (a0->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)a0)->ptr;
		len = ((qse_awk_val_str_t*)a0)->len;
	}
	else 
	{
		str = qse_awk_valtostr (
			run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len);
		if (str == QSE_NULL) return -1;
	}

	for (i = 0; i < len; i++) str[i] = QSE_AWK_TOUPPER (run->awk, str[i]);	

	r = qse_awk_makestrval (run, str, len);
	if (r == QSE_NULL)
	{
		if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);
	qse_awk_rtx_setretval (run, r);
	return 0;
}

static int __substitute (qse_awk_rtx_t* run, qse_long_t max_count)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a2, ** a2_ref, * v;
	qse_char_t* a0_ptr, * a1_ptr, * a2_ptr;
	qse_size_t a0_len, a1_len, a2_len;
	qse_char_t* a0_ptr_free = QSE_NULL;
	qse_char_t* a1_ptr_free = QSE_NULL;
	qse_char_t* a2_ptr_free = QSE_NULL;
	void* rex = QSE_NULL;
	int opt, n;
	const qse_char_t* cur_ptr, * mat_ptr;
	qse_size_t cur_len, mat_len, i, m;
	qse_str_t new;
	qse_long_t sub_count;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = qse_awk_rtx_getarg (run, 0);
	a1 = qse_awk_rtx_getarg (run, 1);
	a2 = (nargs >= 3)? qse_awk_rtx_getarg (run, 2): QSE_NULL;

	QSE_ASSERT (a2 == QSE_NULL || a2->type == QSE_AWK_VAL_REF);

#define FREE_A_PTRS(awk) \
	do { \
		if (a2_ptr_free != QSE_NULL) QSE_AWK_FREE (awk, a2_ptr_free); \
		if (a1_ptr_free != QSE_NULL) QSE_AWK_FREE (awk, a1_ptr_free); \
		if (a0_ptr_free != QSE_NULL) QSE_AWK_FREE (awk, a0_ptr_free); \
	} while (0)
#define FREE_A0_REX(awk,rex) \
	do { \
		if (a0->type != QSE_AWK_VAL_REX) QSE_AWK_FREEREX (awk, rex); \
	} while (0)

	if (a0->type == QSE_AWK_VAL_REX)
	{
		rex = ((qse_awk_val_rex_t*)a0)->code;
	}
	else if (a0->type == QSE_AWK_VAL_STR)
	{
		a0_ptr = ((qse_awk_val_str_t*)a0)->ptr;
		a0_len = ((qse_awk_val_str_t*)a0)->len;
	}
	else
	{
		a0_ptr = qse_awk_valtostr (
			run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &a0_len);
		if (a0_ptr == QSE_NULL) 
		{
			FREE_A_PTRS (run->awk);
			return -1;
		}	
		a0_ptr_free = a0_ptr;
	}

	if (a1->type == QSE_AWK_VAL_STR)
	{
		a1_ptr = ((qse_awk_val_str_t*)a1)->ptr;
		a1_len = ((qse_awk_val_str_t*)a1)->len;
	}
	else
	{
		a1_ptr = qse_awk_valtostr (
			run, a1, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &a1_len);
		if (a1_ptr == QSE_NULL) 
		{
			FREE_A_PTRS (run->awk);
			return -1;
		}
		a1_ptr_free = a1_ptr;
	}

	if (a2 == QSE_NULL)
	{
		/* is this correct? any needs to use inrec.d0? */
		a2_ptr = QSE_STR_PTR(&run->inrec.line);
		a2_len = QSE_STR_LEN(&run->inrec.line);
	}
	else if (((qse_awk_val_ref_t*)a2)->id == QSE_AWK_VAL_REF_POS)
	{
		qse_size_t idx;
	       
		idx = (qse_size_t)((qse_awk_val_ref_t*)a2)->adr;
		if (idx == 0)
		{
			a2_ptr = QSE_STR_PTR(&run->inrec.line);
			a2_len = QSE_STR_LEN(&run->inrec.line);
		}
		else if (idx <= run->inrec.nflds)
		{
			a2_ptr = run->inrec.flds[idx-1].ptr;
			a2_len = run->inrec.flds[idx-1].len;
		}
		else
		{
			a2_ptr = QSE_T("");
			a2_len = 0;
		}
	}
	else
	{
		a2_ref = (qse_awk_val_t**)((qse_awk_val_ref_t*)a2)->adr;

		if ((*a2_ref)->type == QSE_AWK_VAL_MAP)
		{
			FREE_A_PTRS (run->awk);
			/* a map is not allowed as the third parameter */
			qse_awk_setrunerrnum (run, QSE_AWK_EMAPNOTALLOWED);
			return -1;
		}

		if ((*a2_ref)->type == QSE_AWK_VAL_STR)
		{
			a2_ptr = ((qse_awk_val_str_t*)(*a2_ref))->ptr;
			a2_len = ((qse_awk_val_str_t*)(*a2_ref))->len;
		}
		else
		{
			a2_ptr = qse_awk_valtostr (
				run, *a2_ref, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &a2_len);
			if (a2_ptr == QSE_NULL) 
			{
				FREE_A_PTRS (run->awk);
				return -1;
			}
			a2_ptr_free = a2_ptr;
		}
	}

	if (qse_str_init (&new, run->awk->mmgr, a2_len) == QSE_NULL)
	{
		FREE_A_PTRS (run->awk);
		qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	if (a0->type != QSE_AWK_VAL_REX)
	{
		rex = QSE_AWK_BUILDREX (run->awk, a0_ptr, a0_len, &run->errnum);
		if (rex == QSE_NULL)
		{
			qse_str_fini (&new);
			FREE_A_PTRS (run->awk);
			return -1;
		}
	}

	opt = (run->global.ignorecase)? QSE_REX_IGNORECASE: 0;
	cur_ptr = a2_ptr;
	cur_len = a2_len;
	sub_count = 0;

	while (1)
	{
		if (max_count == 0 || sub_count < max_count)
		{
			n = QSE_AWK_MATCHREX (
				run->awk, rex, opt, cur_ptr, cur_len,
				&mat_ptr, &mat_len, &run->errnum);
		}
		else n = 0;

		if (n == -1)
		{
			FREE_A0_REX (run->awk, rex);
			qse_str_fini (&new);
			FREE_A_PTRS (run->awk);
			return -1;
		}

		if (n == 0) 
		{ 
			/* no more match found */
			if (qse_str_ncat (
				&new, cur_ptr, cur_len) == (qse_size_t)-1)
			{
				FREE_A0_REX (run->awk, rex);
				qse_str_fini (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
			break;
		}

		if (qse_str_ncat (
			&new, cur_ptr, mat_ptr - cur_ptr) == (qse_size_t)-1)
		{
			FREE_A0_REX (run->awk, rex);
			qse_str_fini (&new);
			FREE_A_PTRS (run->awk);
			return -1;
		}

		for (i = 0; i < a1_len; i++)
		{
			if ((i+1) < a1_len && 
			    a1_ptr[i] == QSE_T('\\') && 
			    a1_ptr[i+1] == QSE_T('&'))
			{
				m = qse_str_ccat (&new, QSE_T('&'));
				i++;
			}
			else if (a1_ptr[i] == QSE_T('&'))
			{
				m = qse_str_ncat (&new, mat_ptr, mat_len);
			}
			else 
			{
				m = qse_str_ccat (&new, a1_ptr[i]);
			}

			if (m == (qse_size_t)-1)
			{
				FREE_A0_REX (run->awk, rex);
				qse_str_fini (&new);
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
		if (a2 == QSE_NULL)
		{
			if (qse_awk_setrec (run, 0,
				QSE_STR_PTR(&new), QSE_STR_LEN(&new)) == -1)
			{
				qse_str_fini (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
		}
		else if (((qse_awk_val_ref_t*)a2)->id == QSE_AWK_VAL_REF_POS)
		{
			int n;

			n = qse_awk_setrec (
				run, (qse_size_t)((qse_awk_val_ref_t*)a2)->adr,
				QSE_STR_PTR(&new), QSE_STR_LEN(&new));

			if (n == -1)
			{
				qse_str_fini (&new);
				FREE_A_PTRS (run->awk);
				return -1;
			}
		}
		else
		{
			v = qse_awk_makestrval (run,
				QSE_STR_PTR(&new), QSE_STR_LEN(&new));
			if (v == QSE_NULL)
			{
				qse_str_fini (&new);
				FREE_A_PTRS (run->awk);
				/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
				return -1;
			}

			qse_awk_refdownval (run, *a2_ref);
			*a2_ref = v;
			qse_awk_refupval (run, *a2_ref);
		}
	}

	qse_str_fini (&new);
	FREE_A_PTRS (run->awk);

#undef FREE_A0_REX
#undef FREE_A_PTRS

	v = qse_awk_makeintval (run, sub_count);
	if (v == QSE_NULL)
	{
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_rtx_setretval (run, v);
	return 0;
}

static int bfn_gsub (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return __substitute (run, 0);
}

static int bfn_sub (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return __substitute (run, 1);
}

static int bfn_match (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_char_t* str0, * str1;
	qse_size_t len0, len1;
	qse_long_t idx;
	void* rex;
	int opt, n;
	const qse_char_t* mat_ptr;
	qse_size_t mat_len;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 2);
	
	a0 = qse_awk_rtx_getarg (run, 0);
	a1 = qse_awk_rtx_getarg (run, 1);

	if (a0->type == QSE_AWK_VAL_STR)
	{
		str0 = ((qse_awk_val_str_t*)a0)->ptr;
		len0 = ((qse_awk_val_str_t*)a0)->len;
	}
	else
	{
		str0 = qse_awk_valtostr (
			run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len0);
		if (str0 == QSE_NULL) return -1;
	}

	if (a1->type == QSE_AWK_VAL_REX)
	{
		rex = ((qse_awk_val_rex_t*)a1)->code;
	}
	else 
	{
		if (a1->type == QSE_AWK_VAL_STR)
		{
			str1 = ((qse_awk_val_str_t*)a1)->ptr;
			len1 = ((qse_awk_val_str_t*)a1)->len;
		}
		else
		{
			str1 = qse_awk_valtostr (
				run, a1, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &len1);
			if (str1 == QSE_NULL)
			{
				if (a0->type != QSE_AWK_VAL_STR) 
					QSE_AWK_FREE (run->awk, str0);
				return -1;
			}
		}

		rex = QSE_AWK_BUILDREX (run->awk, str1, len1, &run->errnum);
		if (rex == QSE_NULL)
		{
			if (a0->type != QSE_AWK_VAL_STR) 
				QSE_AWK_FREE (run->awk, str0);
			return -1;
		}

		if (a1->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str1);
	}

	opt = (run->global.ignorecase)? QSE_REX_IGNORECASE: 0;
	n = QSE_AWK_MATCHREX (
		run->awk, rex, opt, str0, len0,
		&mat_ptr, &mat_len, &run->errnum);

	if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str0);
	if (a1->type != QSE_AWK_VAL_REX) QSE_AWK_FREEREX (run->awk, rex);

	if (n == -1) return -1;

	idx = (n == 0)? -1: (qse_long_t)(mat_ptr - str0);
	if (qse_awk_getoption(run->awk) & QSE_AWK_BASEONE) idx = idx + 1;

	a0 = qse_awk_makeintval (run, idx);
	if (a0 == QSE_NULL)
	{
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_refupval (run, a0);

	a1 = qse_awk_makeintval (run, 
		((n == 0)? (qse_long_t)-1: (qse_long_t)mat_len));
	if (a1 == QSE_NULL)
	{
		qse_awk_refdownval (run, a0);
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_awk_refupval (run, a1);

	if (qse_awk_rtx_setglobal (run, QSE_AWK_GLOBAL_RSTART, a0) == -1)
	{
		qse_awk_refdownval (run, a1);
		qse_awk_refdownval (run, a0);
		return -1;
	}

	if (qse_awk_rtx_setglobal (run, QSE_AWK_GLOBAL_RLENGTH, a1) == -1)
	{
		qse_awk_refdownval (run, a1);
		qse_awk_refdownval (run, a0);
		return -1;
	}

	qse_awk_rtx_setretval (run, a0);

	qse_awk_refdownval (run, a1);
	qse_awk_refdownval (run, a0);
	return 0;
}

static int bfn_sprintf (
	qse_awk_rtx_t* run, const qse_char_t* fnm, qse_size_t fnl)
{	
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_str_t out, fbu;
	qse_xstr_t cs0;
	qse_xstr_t x;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs > 0);

	if (qse_str_init (&out, run->awk->mmgr, 256) == QSE_NULL)
	{
		qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}
	if (qse_str_init (&fbu, run->awk->mmgr, 256) == QSE_NULL)
	{
		qse_str_fini (&out);
		qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	a0 = qse_awk_rtx_getarg (run, 0);
	if (a0->type == QSE_AWK_VAL_STR)
	{
		cs0.ptr = ((qse_awk_val_str_t*)a0)->ptr;
		cs0.len = ((qse_awk_val_str_t*)a0)->len;
	}
	else
	{
		cs0.ptr = qse_awk_valtostr (
			run, a0, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &cs0.len);
		if (cs0.ptr == QSE_NULL) 
		{
			qse_str_fini (&fbu);
			qse_str_fini (&out);
			return -1;
		}
	}

	x.ptr = qse_awk_format (run, 
		&out, &fbu, cs0.ptr, cs0.len, nargs, QSE_NULL, &x.len);
	if (a0->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, cs0.ptr);
	if (x.ptr == QSE_NULL) 
	{
		qse_str_fini (&fbu);
		qse_str_fini (&out);
		return -1;
	}
	
	/*a0 = qse_awk_makestrval_nodup (run, x.ptr, x.len);*/
	a0 = qse_awk_makestrval (run, x.ptr, x.len);
	if (a0 == QSE_NULL) 
	{
		qse_str_fini (&fbu);
		qse_str_fini (&out);
		/*qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);*/
		return -1;
	}

	qse_str_fini (&fbu);
	/*qse_str_yield (&out, QSE_NULL, 0);*/
	qse_str_fini (&out);
	qse_awk_rtx_setretval (run, a0);
	return 0;
}
