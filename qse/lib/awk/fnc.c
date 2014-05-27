/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "awk.h"
 
static int fnc_close   (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_fflush  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_index   (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);

static int fnc_sin     (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_cos     (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_tan     (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_atan    (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_atan2   (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_log     (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_log10   (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_exp     (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_sqrt    (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_int     (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);

#define A_MAX QSE_TYPE_MAX(int)

/* Argument Specifier 
 *
 * Each character in the specifier indicates how a parameter
 * of the corresponding postion should be passed to a function.
 *
 * - v: value. pass it after normal evaluation.
 * - r: pass a variable by reference
 * - x: regular expression as it it. not evaluated as /rex/ ~ $0.
 *
 * If the first character of the specifer is 'R', all
 * parameters are passed by reference regarless of the remaining
 * chracters.
 */
static qse_awk_fnc_t sysfnctab[] = 
{
	/* io functions */
	{ {QSE_T("close"),   5}, 0, { {1,     2, QSE_NULL},     fnc_close,   QSE_AWK_RIO }, QSE_NULL},
	{ {QSE_T("fflush"),  6}, 0, { {0,     1, QSE_NULL},     fnc_fflush,  QSE_AWK_RIO }, QSE_NULL},

	/* string functions */
	{ {QSE_T("index"),   5}, 0, { {2,     3, QSE_NULL},     fnc_index,            0 }, QSE_NULL},
	{ {QSE_T("substr"),  6}, 0, { {2,     3, QSE_NULL},     qse_awk_fnc_substr,   0 }, QSE_NULL},
	{ {QSE_T("length"),  6}, 1, { {0,     1, QSE_NULL},     qse_awk_fnc_length,   0 }, QSE_NULL},
	{ {QSE_T("split"),   5}, 0, { {2,     3, QSE_T("vrx")}, qse_awk_fnc_split,    0 }, QSE_NULL},
	{ {QSE_T("tolower"), 7}, 0, { {1,     1, QSE_NULL},     qse_awk_fnc_tolower,  0 }, QSE_NULL},
	{ {QSE_T("toupper"), 7}, 0, { {1,     1, QSE_NULL},     qse_awk_fnc_toupper,  0 }, QSE_NULL},
	{ {QSE_T("gsub"),    4}, 0, { {2,     3, QSE_T("xvr")}, qse_awk_fnc_gsub,     0 }, QSE_NULL},
	{ {QSE_T("sub"),     3}, 0, { {2,     3, QSE_T("xvr")}, qse_awk_fnc_sub,      0 }, QSE_NULL},
	{ {QSE_T("match"),   5}, 0, { {2,     3, QSE_T("vxv")}, qse_awk_fnc_match,    0 }, QSE_NULL},
	{ {QSE_T("sprintf"), 7}, 0, { {1, A_MAX, QSE_NULL},     qse_awk_fnc_sprintf,  0 }, QSE_NULL},

	/* math functions */
	{ {QSE_T("sin"),     3}, 0, { {1,     1, QSE_NULL},     fnc_sin,              0 }, QSE_NULL},
	{ {QSE_T("cos"),     3}, 0, { {1,     1, QSE_NULL},     fnc_cos,              0 }, QSE_NULL},
	{ {QSE_T("tan"),     3}, 0, { {1,     1, QSE_NULL},     fnc_tan,              0 }, QSE_NULL},
	{ {QSE_T("atan"),    4}, 0, { {1,     1, QSE_NULL},     fnc_atan,             0 }, QSE_NULL},
	{ {QSE_T("atan2"),   5}, 0, { {2,     2, QSE_NULL},     fnc_atan2,            0 }, QSE_NULL},
	{ {QSE_T("log"),     3}, 0, { {1,     1, QSE_NULL},     fnc_log,              0 }, QSE_NULL},
	{ {QSE_T("log10"),   5}, 0, { {1,     1, QSE_NULL},     fnc_log10,            0 }, QSE_NULL},
	{ {QSE_T("exp"),     3}, 0, { {1,     1, QSE_NULL},     fnc_exp,              0 }, QSE_NULL},
	{ {QSE_T("sqrt"),    4}, 0, { {1,     1, QSE_NULL},     fnc_sqrt,             0 }, QSE_NULL},
	{ {QSE_T("int"),     3}, 0, { {1,     1, QSE_NULL},     fnc_int,              0 }, QSE_NULL}
};

qse_awk_fnc_t* qse_awk_addfnc (qse_awk_t* awk, const qse_char_t* name, const qse_awk_fnc_spec_t* spec)
{
	qse_awk_fnc_t* fnc;
	qse_size_t fnc_size;
	qse_size_t speclen;
	qse_cstr_t ncs;

	ncs.ptr = name;
	ncs.len = qse_strlen (name);

	if (ncs.len <= 0)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	/* Note it doesn't check if it conflicts with a keyword.
	 * such a function registered won't take effect because
	 * the word is treated as a keyword */

	if (qse_awk_findfnc (awk, &ncs) != QSE_NULL)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EEXIST, &ncs);
		return QSE_NULL;
	}

	speclen = spec->arg.spec? qse_strlen(spec->arg.spec): 0;

	fnc_size = QSE_SIZEOF(*fnc) + (ncs.len + 1 + speclen + 1) * QSE_SIZEOF(qse_char_t);
	fnc = (qse_awk_fnc_t*) qse_awk_callocmem (awk, fnc_size);
	if (fnc)
	{
		qse_char_t* tmp;

		tmp = (qse_char_t*)(fnc + 1);
		fnc->name.len = qse_strcpy (tmp, ncs.ptr);
		fnc->name.ptr = tmp;

		fnc->spec = *spec;
		if (spec->arg.spec)
		{
			tmp = fnc->name.ptr + fnc->name.len + 1;
			qse_strcpy (tmp, spec->arg.spec); 
			fnc->spec.arg.spec = tmp;
		}

		if (qse_htb_insert (awk->fnc.user,
			(qse_char_t*)ncs.ptr, ncs.len, fnc, 0) == QSE_NULL)
		{
			qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
			QSE_AWK_FREE (awk, fnc); fnc = QSE_NULL;
		}
	}

	return fnc;
}

int qse_awk_delfnc (qse_awk_t* awk, const qse_char_t* name)
{
	qse_cstr_t ncs;

	ncs.ptr = name;
	ncs.len = qse_strlen (name);

	if (qse_htb_delete (awk->fnc.user, ncs.ptr, ncs.len) <= -1)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOENT, &ncs);
		return -1;
	}

	return 0;
}

void qse_awk_clrfnc (qse_awk_t* awk)
{
	qse_htb_clear (awk->fnc.user);
}

qse_awk_fnc_t* qse_awk_findfnc (qse_awk_t* awk, const qse_cstr_t* name)
{
	qse_htb_pair_t* pair;
	int i;

	/* search the system function table 
	 * though some optimization like binary search can
	 * speed up the search, i don't do that since this
	 * function is called durting parse-time only. 
	 */
	for (i = 0; i < QSE_COUNTOF(sysfnctab); i++)
	{
		if ((awk->opt.trait & sysfnctab[i].spec.trait) != sysfnctab[i].spec.trait) continue;

		if (qse_strxncmp (
			sysfnctab[i].name.ptr, sysfnctab[i].name.len,
			name->ptr, name->len) == 0) return &sysfnctab[i];
	}

	pair = qse_htb_search (awk->fnc.user, name->ptr, name->len);
	if (pair)
	{
		qse_awk_fnc_t* fnc;
		fnc = (qse_awk_fnc_t*)QSE_HTB_VPTR(pair);
		if ((awk->opt.trait & fnc->spec.trait) == fnc->spec.trait) return fnc;
	}

	return QSE_NULL;
}

static int fnc_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* v, * a0, * a1 = QSE_NULL;
	int n;

	qse_char_t* name, * opt = QSE_NULL;
	qse_size_t len, optlen = 0;
       
	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1 || nargs == 2);

	a0 = qse_awk_rtx_getarg (rtx, 0);
	if (nargs >= 2) a1 = qse_awk_rtx_getarg (rtx, 1);
	QSE_ASSERT (a0 != QSE_NULL);

	name = qse_awk_rtx_getvalstr (rtx, a0, &len);
	if (name == QSE_NULL) return -1;

	if (a1)
	{
		opt = qse_awk_rtx_getvalstr (rtx, a1, &optlen);
		if (opt == QSE_NULL) 
		{
			qse_awk_rtx_freevalstr (rtx, a0, name);
			return -1;
		}
	}

	if (len == 0)
	{
		/* getline or print doesn't allow an empty string for the 
		 * input or output file name. so close should not allow 
		 * it either.  
		 * another reason for this is if close is called explicitly 
		 * with an empty string, it may close the console that uses 
		 * an empty string for its identification because closeio
		 * closes any ios that match the name given unlike 
		 * closeio_read or closeio_write. */ 
		n = -1;
		goto skip_close;
	}

	while (len > 0)
	{
		if (name[--len] == QSE_T('\0'))
		{
			/* the name contains a null charater. 
			 * make close return -1 */
			n = -1;
			goto skip_close;
		}
	}	

	if (opt)
	{
		if (optlen != 1 || 
		    (opt[0] != QSE_T('r') && opt[0] != QSE_T('w')))
		{
			n = -1;
			goto skip_close;
		}
	}

	n = qse_awk_rtx_closeio (rtx, name, opt);
	/* failure to close is not a critical error. instead, that is
	 * flagged by the return value of close(). 
	if (n <= -1 && rtx->errinf.num != QSE_AWK_EIONMNF)
	{
		if (a0->type != QSE_AWK_VAL_STR) 
			QSE_AWK_FREE (rtx->awk, name);
		return -1;
	}
	*/

skip_close:
	if (a1) qse_awk_rtx_freevalstr (rtx, a1, opt);
	qse_awk_rtx_freevalstr (rtx, a0, name);

	v = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)n);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

static int flush_io (
	qse_awk_rtx_t* run, int rio, const qse_char_t* name, int n)
{
	int n2;

	if (run->rio.handler[rio] != QSE_NULL)
	{
		n2 = qse_awk_rtx_flushio (run, rio, name);
		if (n2 <= -1)
		{
			/*
			if (run->errinf.num == QSE_AWK_EIOIMPL) n = -1;
			else if (run->errinf.num == QSE_AWK_EIONMNF) 
			{
				if (n != 0) n = -2;
			}
			else n = -99; 
			*/	
			if (run->errinf.num == QSE_AWK_EIONMNF) 
			{
				if (n != 0) n = -2;
			}
			else n = -1;
		}
		else if (n != -1) n = 0;
	}

	return n;
}

static int fnc_fflush (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * v;
	qse_char_t* str0;
	qse_size_t len0;
	int n;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 0 || nargs == 1);

	if (nargs == 0)
	{
		/* fflush() flushes the console output.
		 * fflush() should return -1 on errors.
		 *
		 * if no previous console output statement is seen,
		 * this function won't be able to find the entry.
		 * so it returns -1;
		 *
		 * BEGIN { flush(); } # flush() returns -1
		 * BEGIN { print 1; flush(); } # flush() returns 0
		 */
		n = qse_awk_rtx_flushio (rtx, QSE_AWK_OUT_CONSOLE, QSE_T(""));
	}
	else
	{
		qse_char_t* ptr, * end;

		a0 = qse_awk_rtx_getarg (rtx, 0);
		str0 = qse_awk_rtx_getvalstr (rtx, a0, &len0);
		if (str0 == QSE_NULL) return -1;

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

		/* flush the given rio.
		 *
		 * flush("") flushes all output streams regardless of names.
		 * pass QSE_NULL for the name in that case so that the
		 * callee matches any streams. 
		 *
		 * fflush() doesn't specify the type of output streams
		 * so it attemps to flush all types of output streams.
		 * 
		 * though not useful, it's possible to have multiple
		 * streams with the same name but of different types.
		 * 
		 *  BEGIN { 
		 *    print 1 | "/tmp/x"; 
		 *    print 1 > "/tmp/x";
		 *    fflush("/tmp/x"); 
		 *  }
		 */

		n = flush_io (
			rtx, QSE_AWK_OUT_FILE, 
			((len0 == 0)? QSE_NULL: str0), 1);
		/*if (n == -99) return -1;*/
		n = flush_io (
			rtx, QSE_AWK_OUT_APFILE, 
			((len0 == 0)? QSE_NULL: str0), n);
		/*if (n == -99) return -1;*/
		n = flush_io (
			rtx, QSE_AWK_OUT_PIPE,
			((len0 == 0)? QSE_NULL: str0), n);
		/*if (n == -99) return -1;*/
		n = flush_io (
			rtx, QSE_AWK_OUT_RWPIPE,
			((len0 == 0)? QSE_NULL: str0), n);
		/*if (n == -99) return -1;*/

		/* if n remains 1, no io handlers have been defined for
		 * file, pipe, and rwpipe. so make fflush return -1. 
		 * if n is -2, no such named io has been found at all 
		 * if n is -1, the io handler has returned an error */
		if (n != 0) n = -1;

	skip_flush:
		qse_awk_rtx_freevalstr (rtx, a0, str0);
	}

	v = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)n);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

static int fnc_index (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * v;
	qse_char_t* str0, * str1, * ptr;
	qse_size_t len0, len1;
	qse_awk_int_t idx, start = 1;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);
	
	a0 = qse_awk_rtx_getarg (rtx, 0);
	a1 = qse_awk_rtx_getarg (rtx, 1);

	if (nargs >= 3) 
	{
		qse_awk_val_t* a2;
		int n;

		a2 = qse_awk_rtx_getarg (rtx, 2);
		n = qse_awk_rtx_valtoint (rtx, a2, &start);
		if (n <= -1) return -1;
	}

	str0 = qse_awk_rtx_getvalstr (rtx, a0, &len0);
	if (str0 == QSE_NULL) return -1;

	str1 = qse_awk_rtx_getvalstr (rtx, a1, &len1);
	if (str1 == QSE_NULL)
	{
		qse_awk_rtx_freevalstr (rtx, a0, str0);
		return -1;
	}

	if (start == 0) start = 1;
	else if (start < 0) start = len0 + start + 1;

	ptr = (start > len0 || start <= 0)? QSE_NULL:
	      (rtx->gbl.ignorecase)?
	          qse_strxncasestr (&str0[start-1], len0-start+1, str1, len1):
	          qse_strxnstr (&str0[start-1], len0-start+1, str1, len1);

	idx = (ptr == QSE_NULL)? 0: ((qse_awk_int_t)(ptr-str0) + 1);

	qse_awk_rtx_freevalstr (rtx, a1, str1);
	qse_awk_rtx_freevalstr (rtx, a0, str0);

	v = qse_awk_rtx_makeintval (rtx, idx);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

int qse_awk_fnc_length (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* v;
	qse_char_t* str;
	qse_size_t len;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs >= 0 && nargs <= 1);
	
	if (nargs == 0)
	{
		/* get the length of $0 */
		len = QSE_STR_LEN(&rtx->inrec.line);
	}
	else
	{
		v = qse_awk_rtx_getarg (rtx, 0);
		if (v->type == QSE_AWK_VAL_MAP)
		{
			/* map size */
			len = QSE_HTB_SIZE(((qse_awk_val_map_t*)v)->map);
		}
		else if (v->type == QSE_AWK_VAL_STR)
		{
			/* string length */
			len = ((qse_awk_val_str_t*)v)->val.len;
		}
		else
		{
			/* convert to string and get length */
			str = qse_awk_rtx_valtostrdup (rtx, v, &len);
			if (str == QSE_NULL) return -1;
			QSE_AWK_FREE (rtx->awk, str);
		}
	}

	v = qse_awk_rtx_makeintval (rtx, len);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

int qse_awk_fnc_substr (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a2, * r;
	qse_char_t* str;
	qse_size_t len;
	qse_awk_int_t lindex, lcount;
	int n;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = qse_awk_rtx_getarg (rtx, 0);
	a1 = qse_awk_rtx_getarg (rtx, 1);
	a2 = (nargs >= 3)? qse_awk_rtx_getarg (rtx, 2): QSE_NULL;

	str = qse_awk_rtx_getvalstr (rtx, a0, &len);
	if (str == QSE_NULL) return -1;

	n = qse_awk_rtx_valtoint (rtx, a1, &lindex);
	if (n <= -1) 
	{
		qse_awk_rtx_freevalstr (rtx, a0, str);
		return -1;
	}

	if (a2 == QSE_NULL) lcount = (qse_awk_int_t)len;
	else 
	{
		n = qse_awk_rtx_valtoint (rtx, a2, &lcount);
		if (n <= -1) 
		{
			qse_awk_rtx_freevalstr (rtx, a0, str);
			return -1;
		}
	}

	lindex = lindex - 1;
	if (lindex >= (qse_awk_int_t)len) lindex = (qse_awk_int_t)len;
	else if (lindex < 0) lindex = 0;

	if (lcount < 0) lcount = 0;
	else if (lcount > (qse_awk_int_t)len - lindex) 
	{
		lcount = (qse_awk_int_t)len - lindex;
	}

	r = qse_awk_rtx_makestrval (rtx, &str[lindex], (qse_size_t)lcount);
	if (r == QSE_NULL)
	{
		qse_awk_rtx_freevalstr (rtx, a0, str);
		return -1;
	}

	qse_awk_rtx_freevalstr (rtx, a0, str);
	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

int qse_awk_fnc_split (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a2, * t1, * t2;

	qse_cstr_t str, fs;
	qse_char_t* str_free = QSE_NULL, * fs_free = QSE_NULL;
	const qse_char_t* p;
	qse_size_t str_left, org_len;
	void* fs_rex = QSE_NULL; 
	void* fs_rex_free = QSE_NULL;

	qse_cstr_t tok;
	qse_awk_int_t nflds;

	qse_awk_errnum_t errnum;
	int x;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = qse_awk_rtx_getarg (rtx, 0);
	a1 = qse_awk_rtx_getarg (rtx, 1);
	a2 = (nargs >= 3)? qse_awk_rtx_getarg (rtx, 2): QSE_NULL;

	QSE_ASSERT (a1->type == QSE_AWK_VAL_REF);

	if (a0->type == QSE_AWK_VAL_STR)
	{
		str.ptr = ((qse_awk_val_str_t*)a0)->val.ptr;
		str.len = ((qse_awk_val_str_t*)a0)->val.len;
	}
	else 
	{
		str.ptr = qse_awk_rtx_valtostrdup (rtx, a0, &str.len);
		if (str.ptr == QSE_NULL) return -1;
		str_free = (qse_char_t*)str.ptr;
	}

	if (a2 == QSE_NULL)
	{
		/* get the value from FS */
		t1 = qse_awk_rtx_getgbl (rtx, QSE_AWK_GBL_FS);
		if (t1->type == QSE_AWK_VAL_NIL)
		{
			fs.ptr = QSE_T(" ");
			fs.len = 1;
		}
		else if (t1->type == QSE_AWK_VAL_STR)
		{
			fs.ptr = ((qse_awk_val_str_t*)t1)->val.ptr;
			fs.len = ((qse_awk_val_str_t*)t1)->val.len;
		}
		else
		{
			fs.ptr = qse_awk_rtx_valtostrdup (rtx, t1, &fs.len);
			if (fs.ptr == QSE_NULL) goto oops;
			fs_free = (qse_char_t*)fs.ptr;
		}

		if (fs.len > 1) fs_rex = rtx->gbl.fs[rtx->gbl.ignorecase];
	}
	else if (a2->type == QSE_AWK_VAL_REX)
	{
		/* the third parameter is a regular expression */
		fs_rex = ((qse_awk_val_rex_t*)a2)->code[rtx->gbl.ignorecase];

		/* make the loop below to take fs_rex by 
		 * setting fs_len greater than 1*/
		fs.ptr = QSE_NULL;
		fs.len = 2;
	}
	else 
	{
		if (a2->type == QSE_AWK_VAL_STR)
		{
			fs.ptr = ((qse_awk_val_str_t*)a2)->val.ptr;
			fs.len = ((qse_awk_val_str_t*)a2)->val.len;
		}
		else
		{
			fs.ptr = qse_awk_rtx_valtostrdup (rtx, a2, &fs.len);
			if (fs.ptr == QSE_NULL) goto oops;
			fs_free = (qse_char_t*)fs.ptr;
		}

		if (fs.len > 1) 
		{
			int x;

			if (rtx->gbl.ignorecase)
				x = qse_awk_buildrex (rtx->awk, fs.ptr, fs.len, &errnum, QSE_NULL, &fs_rex);
			else
				x = qse_awk_buildrex (rtx->awk, fs.ptr, fs.len, &errnum, &fs_rex, QSE_NULL);

			if (x <= -1)
			{
				qse_awk_rtx_seterrnum (rtx, errnum, QSE_NULL);
				goto oops;
			}

			fs_rex_free = fs_rex;
		}
	}

	t1 = qse_awk_rtx_makemapval (rtx);
	if (t1 == QSE_NULL) goto oops;

	qse_awk_rtx_refupval (rtx, t1);
	x = qse_awk_rtx_setrefval (rtx, (qse_awk_val_ref_t*)a1, t1);
	qse_awk_rtx_refdownval (rtx, t1);
	if (x <= -1) goto oops;

	/* fill the map with actual values */
	p = str.ptr; str_left = str.len; org_len = str.len;
	nflds = 0;

	while (p != QSE_NULL)
	{
		qse_char_t key_buf[QSE_SIZEOF(qse_awk_int_t)*8+2];
		qse_size_t key_len;

		if (fs.len <= 1)
		{
			p = qse_awk_rtx_strxntok (rtx, 
				p, str.len, fs.ptr, fs.len, &tok);
		}
		else
		{
			p = qse_awk_rtx_strxntokbyrex (
				rtx, str.ptr, org_len, p, str.len, 
				fs_rex, &tok, &errnum
			); 
			if (p == QSE_NULL && errnum != QSE_AWK_ENOERR)
			{
				qse_awk_rtx_seterrnum (rtx, errnum, QSE_NULL);
				goto oops;
			}
		}

		if (nflds == 0 && p == QSE_NULL && tok.len == 0) 
		{
			/* no field at all*/
			break; 
		}	

		QSE_ASSERT ((tok.ptr != QSE_NULL && tok.len > 0) || tok.len == 0);

		/* create the field string - however, the split function must
		 * create a numeric string if the string is a number */
		/*t2 = qse_awk_rtx_makestrvalwithcstr (rtx, &tok);*/
		t2 = qse_awk_rtx_makenstrvalwithcstr (rtx, &tok);
		if (t2 == QSE_NULL) goto oops;

		/* put it into the map */
		key_len = qse_awk_inttostr (
			rtx->awk, ++nflds, 10, QSE_NULL, key_buf, QSE_COUNTOF(key_buf));
		QSE_ASSERT (key_len != (qse_size_t)-1);

		if (qse_awk_rtx_setmapvalfld (
			rtx, t1, key_buf, key_len, t2) == QSE_NULL)
		{
			qse_awk_rtx_refupval (rtx, t2);
			qse_awk_rtx_refdownval (rtx, t2);
			goto oops;
		}

		str.len = str_left - (p - str.ptr);
	}

	if (str_free) QSE_AWK_FREE (rtx->awk, str_free);
	if (fs_free) QSE_AWK_FREE (rtx->awk, fs_free);
	if (fs_rex_free) 
	{
		if (rtx->gbl.ignorecase)
			qse_awk_freerex (rtx->awk, QSE_NULL, fs_rex_free);
		else	
			qse_awk_freerex (rtx->awk, fs_rex_free, QSE_NULL);
	}

	/*nflds--;*/

	t1 = qse_awk_rtx_makeintval (rtx, nflds);
	if (t1 == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, t1);
	return 0;

oops:
	if (str_free) QSE_AWK_FREE (rtx->awk, str_free);
	if (fs_free) QSE_AWK_FREE (rtx->awk, fs_free);
	if (fs_rex_free) 
	{
		if (rtx->gbl.ignorecase)
			qse_awk_freerex (rtx->awk, QSE_NULL, fs_rex_free);
		else	
			qse_awk_freerex (rtx->awk, fs_rex_free, QSE_NULL);
	}
	return -1;
}

int qse_awk_fnc_tolower (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_size_t i;
	qse_awk_val_t* a0, * r;
	qse_xstr_t str;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (rtx, 0);

	str.ptr = qse_awk_rtx_getvalstr (rtx, a0, &str.len);
	if (str.ptr == QSE_NULL) return -1;

	for (i = 0; i < str.len; i++) str.ptr[i] = QSE_AWK_TOLOWER (rtx->awk, str.ptr[i]);	

	r = qse_awk_rtx_makestrvalwithcstr (rtx, (qse_cstr_t*)&str);
	if (r == QSE_NULL)
	{
		qse_awk_rtx_freevalstr (rtx, a0, str.ptr);
		return -1;
	}

	qse_awk_rtx_freevalstr (rtx, a0, str.ptr);
	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

int qse_awk_fnc_toupper (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_size_t i;
	qse_awk_val_t* a0, * r;
	qse_xstr_t str;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (rtx, 0);

	str.ptr = qse_awk_rtx_getvalstr (rtx, a0, &str.len);
	if (str.ptr == QSE_NULL) return -1;

	for (i = 0; i < str.len; i++) str.ptr[i] = QSE_AWK_TOUPPER (rtx->awk, str.ptr[i]);	

	r = qse_awk_rtx_makestrvalwithcstr (rtx, (qse_cstr_t*)&str);
	if (r == QSE_NULL)
	{
		qse_awk_rtx_freevalstr (rtx, a0, str.ptr);
		return -1;
	}

	qse_awk_rtx_freevalstr (rtx, a0, str.ptr);
	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int __substitute (qse_awk_rtx_t* run, qse_awk_int_t max_count)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a2, * v;

	qse_cstr_t s0, s1, s2;
	const qse_char_t* s2_end;

	qse_char_t* s0_free = QSE_NULL;
	qse_char_t* s1_free = QSE_NULL;
	qse_char_t* s2_free = QSE_NULL;

	void* rex = QSE_NULL;
	void* rex_free = QSE_NULL;

	qse_str_t new;
	int new_inited = 0;

	qse_cstr_t mat, pmat, cur;

	qse_awk_int_t sub_count;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = qse_awk_rtx_getarg (run, 0);
	a1 = qse_awk_rtx_getarg (run, 1);
	a2 = (nargs >= 3)? qse_awk_rtx_getarg (run, 2): QSE_NULL;

	QSE_ASSERT (a2 == QSE_NULL || a2->type == QSE_AWK_VAL_REF);

	if (a0->type == QSE_AWK_VAL_REX)
	{
		rex = ((qse_awk_val_rex_t*)a0)->code[run->gbl.ignorecase];
	}
	else if (a0->type == QSE_AWK_VAL_STR)
	{
		s0.ptr = ((qse_awk_val_str_t*)a0)->val.ptr;
		s0.len = ((qse_awk_val_str_t*)a0)->val.len;
	}
	else
	{
		s0.ptr = qse_awk_rtx_valtostrdup (run, a0, &s0.len);
		if (s0.ptr == QSE_NULL) goto oops;
		s0_free = (qse_char_t*)s0.ptr;
	}

	if (a1->type == QSE_AWK_VAL_STR)
	{
		s1.ptr = ((qse_awk_val_str_t*)a1)->val.ptr;
		s1.len = ((qse_awk_val_str_t*)a1)->val.len;
	}
	else
	{
		s1.ptr = qse_awk_rtx_valtostrdup (run, a1, &s1.len);
		if (s1.ptr == QSE_NULL) goto oops;
		s1_free = (qse_char_t*)s1.ptr;
	}

	if (a2 == QSE_NULL)
	{
		/* is this correct? any needs to use inrec.d0? */
		s2.ptr = QSE_STR_PTR(&run->inrec.line);
		s2.len = QSE_STR_LEN(&run->inrec.line);
	}
	else
	{
		s2.ptr = qse_awk_rtx_valtostrdup (run, a2, &s2.len);
		if (s2.ptr == QSE_NULL) goto oops;
		s2_free = (qse_char_t*)s2.ptr;
	}

	if (qse_str_init (&new, run->awk->mmgr, s2.len) <= -1)
	{
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
		goto oops;
	}
	new_inited = 1;

	if (a0->type != QSE_AWK_VAL_REX)
	{
		qse_awk_errnum_t errnum;
		int x;

		if (run->gbl.ignorecase)
			x = qse_awk_buildrex (run->awk, s0.ptr, s0.len, &errnum, QSE_NULL, &rex);
		else
			x = qse_awk_buildrex (run->awk, s0.ptr, s0.len, &errnum, &rex, QSE_NULL);

		if (x <= -1)
		{
			qse_awk_rtx_seterrnum (run, errnum, QSE_NULL);
			goto oops;
		}

		rex_free = rex;
	}

	s2_end = s2.ptr + s2.len;
	cur.ptr = s2.ptr;
	cur.len = s2.len;
	sub_count = 0;

	pmat.ptr = QSE_NULL;
	pmat.len = 0;

	/* perform test when cur_ptr == s2_end also because
	 * end of string($) needs to be tested */
	while (cur.ptr <= s2_end)
	{
		qse_awk_errnum_t errnum;
		int n;
		qse_size_t m, i;

		if (max_count == 0 || sub_count < max_count)
		{
			n = qse_awk_matchrex (
				run->awk, rex, run->gbl.ignorecase, 
				&s2, &cur, &mat, &errnum
			);
		}
		else n = 0;

		if (n <= -1)
		{
			qse_awk_rtx_seterrnum (run, errnum, QSE_NULL);
			goto oops;
		}

		if (n == 0) 
		{ 
			/* no more match found */
			if (qse_str_ncat (
				&new, cur.ptr, cur.len) == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				goto oops;
			}
			break;
		}

		if (mat.len == 0 &&
		    pmat.ptr != QSE_NULL &&
		    mat.ptr == pmat.ptr + pmat.len)
		{
			/* match length is 0 and the match is still at the
			 * end of the previous match */
			goto skip_one_char;
		}

		if (qse_str_ncat (
			&new, cur.ptr, mat.ptr - cur.ptr) == (qse_size_t)-1)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
			goto oops;
		}

		for (i = 0; i < s1.len; i++)
		{
			if ((i+1) < s1.len && 
			    s1.ptr[i] == QSE_T('\\') && 
			    s1.ptr[i+1] == QSE_T('&'))
			{
				m = qse_str_ccat (&new, QSE_T('&'));
				i++;
			}
			else if (s1.ptr[i] == QSE_T('&'))
			{
				m = qse_str_ncat (&new, mat.ptr, mat.len);
			}
			else 
			{
				m = qse_str_ccat (&new, s1.ptr[i]);
			}

			if (m == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				goto oops;
			}
		}

		sub_count++;
		cur.len = cur.len - ((mat.ptr - cur.ptr) + mat.len);
		cur.ptr = mat.ptr + mat.len;

		pmat = mat;

		if (mat.len == 0)
		{
		skip_one_char:
			/* special treatment is needed if match length is 0 */

			m = qse_str_ncat (&new, cur.ptr, 1);
			if (m == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				goto oops;
			}

			cur.ptr++; cur.len--;
		}
	}

	if (rex_free)
	{
		if (run->gbl.ignorecase)
			qse_awk_freerex (run->awk, QSE_NULL, rex_free); 
		else
			qse_awk_freerex (run->awk, rex_free, QSE_NULL); 
		rex_free = QSE_NULL;
	}

	if (sub_count > 0)
	{
		if (a2 == QSE_NULL)
		{
			int n;
			n = qse_awk_rtx_setrec (run, 0, QSE_STR_CSTR(&new));
			if (n <= -1) goto oops;
		}
		else 
		{
			int n;

			v = qse_awk_rtx_makestrvalwithcstr (run, QSE_STR_CSTR(&new));
			if (v == QSE_NULL) goto oops;
			qse_awk_rtx_refupval (run, v);
			n = qse_awk_rtx_setrefval (run, (qse_awk_val_ref_t*)a2, v);
			qse_awk_rtx_refdownval (run, v);
			if (n <= -1) goto oops;
		}
	}

	qse_str_fini (&new);

	if (s2_free) QSE_AWK_FREE (run->awk, s2_free);
	if (s1_free) QSE_AWK_FREE (run->awk, s1_free);
	if (s0_free) QSE_AWK_FREE (run->awk, s0_free);

	v = qse_awk_rtx_makeintval (run, sub_count);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (run, v);
	return 0;

oops:
	if (rex_free) 
	{
		if (run->gbl.ignorecase)
			qse_awk_freerex (run->awk, QSE_NULL, rex_free);
		else	
			qse_awk_freerex (run->awk, rex_free, QSE_NULL); 
	}
	if (new_inited) qse_str_fini (&new);
	if (s2_free) QSE_AWK_FREE (run->awk, s2_free);
	if (s1_free) QSE_AWK_FREE (run->awk, s1_free);
	if (s0_free) QSE_AWK_FREE (run->awk, s0_free);
	return -1;
}

int qse_awk_fnc_gsub (qse_awk_rtx_t* run, const qse_awk_fnc_info_t* fi)
{
	return __substitute (run, 0);
}

int qse_awk_fnc_sub (qse_awk_rtx_t* run, const qse_awk_fnc_info_t* fi)
{
	return __substitute (run, 1);
}

int qse_awk_fnc_match (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_char_t* str0;
	qse_size_t len0;
	qse_awk_int_t idx, start = 1;
	int n;
	qse_cstr_t mat;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);
	
	a0 = qse_awk_rtx_getarg (rtx, 0);
	a1 = qse_awk_rtx_getarg (rtx, 1);

	if (nargs >= 3) 
	{
		qse_awk_val_t* a2;

		a2 = qse_awk_rtx_getarg (rtx, 2);
#if 0
		if (a2->type == QSE_AWK_VAL_MAP)
		{
			/* if the 3rd paramater is an array,
			 * it is a placeholder to store parenthesized 
			 * subexpressions */

			/* TODO: please implement this... */
			start = 0;
		}
		else
#endif
		{
			/* if the 3rd parameter is not an array,
			 * it is treated as a match start index */
			n = qse_awk_rtx_valtoint (rtx, a2, &start);
			if (n <= -1) return -1;
		}
	}

	str0 = qse_awk_rtx_getvalstr (rtx, a0, &len0);
	if (str0 == QSE_NULL) return -1;

	if (start == 0) start = 1;
	else if (start < 0) start = len0 + start + 1;

	if (start > len0 || start <= 0) n = 0;
	else
	{
		qse_cstr_t tmp;

		/*TODO: must use str0,len0? */
		tmp.ptr = str0 + start - 1;
		tmp.len = len0 - start + 1;

		n = qse_awk_rtx_matchrex (rtx, a1, &tmp, &tmp, &mat);
		if (n <= -1) return -1;
	}

	qse_awk_rtx_freevalstr (rtx, a0, str0);

	idx = (n == 0)? 0: ((qse_awk_int_t)(mat.ptr-str0) + 1);

	a0 = qse_awk_rtx_makeintval (rtx, idx);
	if (a0 == QSE_NULL) return -1;

	qse_awk_rtx_refupval (rtx, a0);

	a1 = qse_awk_rtx_makeintval (rtx, 
		((n == 0)? (qse_awk_int_t)-1: (qse_awk_int_t)mat.len));
	if (a1 == QSE_NULL)
	{
		qse_awk_rtx_refdownval (rtx, a0);
		return -1;
	}

	qse_awk_rtx_refupval (rtx, a1);

	if (qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_RSTART, a0) <= -1)
	{
		qse_awk_rtx_refdownval (rtx, a1);
		qse_awk_rtx_refdownval (rtx, a0);
		return -1;
	}

	if (qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_RLENGTH, a1) <= -1)
	{
		qse_awk_rtx_refdownval (rtx, a1);
		qse_awk_rtx_refdownval (rtx, a0);
		return -1;
	}

	qse_awk_rtx_setretval (rtx, a0);

	qse_awk_rtx_refdownval (rtx, a1);
	qse_awk_rtx_refdownval (rtx, a0);
	return 0;
}

int qse_awk_fnc_sprintf (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{	
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_str_t out, fbu;
	int out_inited = 0, fbu_inited = 0;
	qse_xstr_t cs0;
	qse_xstr_t x;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs > 0);

	if (qse_str_init (&out, rtx->awk->mmgr, 256) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		goto oops;
	}
	out_inited = 1;

	if (qse_str_init (&fbu, rtx->awk->mmgr, 256) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		goto oops;
	}
	fbu_inited = 1;

	a0 = qse_awk_rtx_getarg (rtx, 0);
	cs0.ptr = qse_awk_rtx_getvalstr (rtx, a0, &cs0.len);
	if (cs0.ptr == QSE_NULL) goto oops;

	x.ptr = qse_awk_rtx_format (rtx, 
		&out, &fbu, cs0.ptr, cs0.len, nargs, QSE_NULL, &x.len);
	qse_awk_rtx_freevalstr (rtx, a0, cs0.ptr);
	if (x.ptr == QSE_NULL) goto oops;
	
	a0 = qse_awk_rtx_makestrvalwithcstr (rtx, (qse_cstr_t*)&x);
	if (a0 == QSE_NULL)  goto oops;

	qse_str_fini (&fbu);
	qse_str_fini (&out);
	qse_awk_rtx_setretval (rtx, a0);
	return 0;

oops:
	if (fbu_inited) qse_str_fini (&fbu);
	if (out_inited) qse_str_fini (&out);
	return -1;
}

static int fnc_math_1 (
	qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi, qse_awk_math1_t f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_awk_flt_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (rtx, 0);

	n = qse_awk_rtx_valtoflt (rtx, a0, &rv);
	if (n <= -1) return -1;

	r = qse_awk_rtx_makefltval (rtx, f (rtx->awk, rv));
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_math_2 (
	qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi, qse_awk_math2_t f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_awk_flt_t rv0, rv1;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 2);

	a0 = qse_awk_rtx_getarg (rtx, 0);
	a1 = qse_awk_rtx_getarg (rtx, 1);

	n = qse_awk_rtx_valtoflt (rtx, a0, &rv0);
	if (n <= -1) return -1;

	n = qse_awk_rtx_valtoflt (rtx, a1, &rv1);
	if (n <= -1) return -1;

	r = qse_awk_rtx_makefltval (rtx, f (rtx->awk, rv0, rv1));
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_sin (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, rtx->awk->prm.math.sin);
}
static int fnc_cos (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, rtx->awk->prm.math.cos);
}
static int fnc_tan (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, rtx->awk->prm.math.tan);
}
static int fnc_atan (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, rtx->awk->prm.math.atan);
}
static int fnc_atan2 (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_2 (rtx, fi, rtx->awk->prm.math.atan2);
}
static int fnc_log (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, rtx->awk->prm.math.log);
}
static int fnc_log10 (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, rtx->awk->prm.math.log10);
}
static int fnc_exp (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, rtx->awk->prm.math.exp);
}
static int fnc_sqrt (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return fnc_math_1 (rtx, fi, rtx->awk->prm.math.sqrt);
}

static int fnc_int (qse_awk_rtx_t* run, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_awk_int_t lv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (run, 0);

	n = qse_awk_rtx_valtoint (run, a0, &lv);
	if (n <= -1) return -1;

	r = qse_awk_rtx_makeintval (run, lv);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (run, r);
	return 0;
}
