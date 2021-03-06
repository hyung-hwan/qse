/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "awk-prv.h"
#include <qse/cmn/alg.h>
#include <qse/cmn/fmt.h>

static int fnc_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_fflush (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_int (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_typename (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_isnil (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_ismap (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_asort (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
static int fnc_asorti (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);

#define A_MAX QSE_TYPE_MAX(int)

/* Argument Specifier 
 *
 * Each character in the specifier indicates how a parameter
 * of the corresponding postion should be passed to a function.
 *
 * - v: value. pass it after normal evaluation.
 * - r: pass a variable by reference
 * - x: regular expression as it is. not evaluated as /rex/ ~ $0.
 * 
 * NOTE: If min is greater than max, the specifier indicate the 
 * name of the module  where the function is located.
 */
static qse_awk_fnc_t sysfnctab[] = 
{
	/* io functions */
	{ {QSE_T("close"),    5}, 0, { {1,     2, QSE_NULL},        fnc_close,  QSE_AWK_RIO }, QSE_NULL},
	{ {QSE_T("fflush"),   6}, 0, { {0,     1, QSE_NULL},        fnc_fflush,  QSE_AWK_RIO }, QSE_NULL},

	/* type info/conversion */
	{ {QSE_T("int"),      3}, 0, { {1,     1, QSE_NULL},        fnc_int,              0 }, QSE_NULL},
	{ {QSE_T("isnil"),    5}, 0, { {1,     1, QSE_NULL},        fnc_isnil,            0 }, QSE_NULL},
	{ {QSE_T("ismap"),    5}, 0, { {1,     1, QSE_NULL},        fnc_ismap,            0 }, QSE_NULL},
	{ {QSE_T("typename"), 8}, 0, { {1,     1, QSE_NULL},        fnc_typename,         0 }, QSE_NULL},

	/* map(array) sort */
	{ {QSE_T("asort"),    5}, 0, { {1,     3, QSE_T("rrv")},    fnc_asort,            0 }, QSE_NULL},
	{ {QSE_T("asorti"),   6}, 0, { {1,     3, QSE_T("rrv")},    fnc_asorti,           0 }, QSE_NULL},
 
	/* string functions */
	{ {QSE_T("gsub"),     4}, 0, { {2,     3, QSE_T("xvr")},    qse_awk_fnc_gsub,     0 }, QSE_NULL},
	{ {QSE_T("index"),    5}, 0, { {2,     3, QSE_NULL},        qse_awk_fnc_index,    0 }, QSE_NULL},
	{ {QSE_T("length"),   6}, 1, { {0,     1, QSE_NULL},        qse_awk_fnc_length,   0 }, QSE_NULL},
	{ {QSE_T("match"),    5}, 0, { {2,     4, QSE_T("vxvr")},   qse_awk_fnc_match,    0 }, QSE_NULL},
	{ {QSE_T("split"),    5}, 0, { {2,     3, QSE_T("vrx")},    qse_awk_fnc_split,    0 }, QSE_NULL},
	{ {QSE_T("sprintf"),  7}, 0, { {1, A_MAX, QSE_NULL},        qse_awk_fnc_sprintf,  0 }, QSE_NULL},
	{ {QSE_T("sub"),      3}, 0, { {2,     3, QSE_T("xvr")},    qse_awk_fnc_sub,      0 }, QSE_NULL},
	{ {QSE_T("substr"),   6}, 0, { {2,     3, QSE_NULL},        qse_awk_fnc_substr,   0 }, QSE_NULL},
	{ {QSE_T("tolower"),  7}, 0, { {1,     1, QSE_NULL},        qse_awk_fnc_tolower,  0 }, QSE_NULL},
	{ {QSE_T("toupper"),  7}, 0, { {1,     1, QSE_NULL},        qse_awk_fnc_toupper,  0 }, QSE_NULL},

	/* math functions */
	{ {QSE_T("sin"),      3}, 0, { {A_MAX, 0, QSE_T("math") },  QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("cos"),      3}, 0, { {A_MAX, 0, QSE_T("math") },  QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("tan"),      3}, 0, { {A_MAX, 0, QSE_T("math") },  QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("atan"),     4}, 0, { {A_MAX, 0, QSE_T("math") },  QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("atan2"),    5}, 0, { {A_MAX, 0, QSE_T("math") },  QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("log"),      3}, 0, { {A_MAX, 0, QSE_T("math") },  QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("log10"),    5}, 0, { {A_MAX, 0, QSE_T("math") },  QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("exp"),      3}, 0, { {A_MAX, 0, QSE_T("math") },  QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("sqrt"),     4}, 0, { {A_MAX, 0, QSE_T("math") },  QSE_NULL,             0 }, QSE_NULL},

	/* time functions */
	{ {QSE_T("mktime"),   6}, 0, { {A_MAX, 0, QSE_T("sys") },   QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("strftime"), 8}, 0, { {A_MAX, 0, QSE_T("sys") },   QSE_NULL,             0 }, QSE_NULL},
	{ {QSE_T("systime"),  7}, 0, { {A_MAX, 0, QSE_T("sys") },   QSE_NULL,             0 }, QSE_NULL}
};

static qse_awk_fnc_t* add_fnc (qse_awk_t* awk, const qse_char_t* name, const qse_awk_fnc_spec_t* spec)
{
	qse_awk_fnc_t* fnc;
	qse_size_t fnc_size;
	qse_size_t speclen;
	qse_cstr_t ncs;

	ncs.ptr = (qse_char_t*)name;
	ncs.len = qse_strlen(name);
	if (ncs.len <= 0)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	/* Note it doesn't check if it conflicts with a keyword.
	 * such a function registered won't take effect because
	 * the word is treated as a keyword */

	if (qse_awk_findfncwithcstr(awk, &ncs) != QSE_NULL)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EEXIST, &ncs);
		return QSE_NULL;
	}

	speclen = spec->arg.spec? qse_strlen(spec->arg.spec): 0;

	fnc_size = QSE_SIZEOF(*fnc) + (ncs.len + 1 + speclen + 1) * QSE_SIZEOF(qse_char_t);
	fnc = (qse_awk_fnc_t*)qse_awk_callocmem(awk, fnc_size);
	if (fnc)
	{
		qse_char_t* tmp;

		tmp = (qse_char_t*)(fnc + 1);
		fnc->name.len = qse_strcpy(tmp, ncs.ptr);
		fnc->name.ptr = tmp;

		fnc->spec = *spec;
		if (spec->arg.spec)
		{
			tmp = fnc->name.ptr + fnc->name.len + 1;
			qse_strcpy (tmp, spec->arg.spec); 
			fnc->spec.arg.spec = tmp;
		}

		if (qse_htb_insert(awk->fnc.user, (qse_char_t*)ncs.ptr, ncs.len, fnc, 0) == QSE_NULL)
		{
			qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
			qse_awk_freemem (awk, fnc);
			fnc = QSE_NULL;
		}
	}

	return fnc;
}

qse_awk_fnc_t* qse_awk_addfncwithmbs (qse_awk_t* awk, const qse_mchar_t* name, const qse_awk_fnc_mspec_t* spec)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return add_fnc(awk, name, spec);
#else
	qse_wcstr_t wcs;
	qse_awk_fnc_t* fnc;
	qse_awk_fnc_spec_t wspec;

	QSE_STATIC_ASSERT (QSE_SIZEOF(*spec) == QSE_SIZEOF(wspec));

	QSE_MEMCPY (&wspec, spec, QSE_SIZEOF(wspec));
	if (spec->arg.spec)
	{
		wcs.ptr = qse_awk_mbstowcsdup(awk, spec->arg.spec, &wcs.len);
		if (!wcs.ptr) return QSE_NULL;
		wspec.arg.spec = wcs.ptr;
	}

	wcs.ptr = qse_awk_mbstowcsdup(awk, name, &wcs.len);
	if (!wcs.ptr) 
	{
		if (wspec.arg.spec) qse_awk_freemem (awk, (qse_wchar_t*)wspec.arg.spec);
		return QSE_NULL;
	}

	fnc = add_fnc(awk, wcs.ptr, &wspec);
	qse_awk_freemem (awk, wcs.ptr);
	if (wspec.arg.spec) qse_awk_freemem (awk, (qse_wchar_t*)wspec.arg.spec);
	return fnc;
#endif
}

qse_awk_fnc_t* qse_awk_addfncwithwcs (qse_awk_t* awk, const qse_wchar_t* name, const qse_awk_fnc_wspec_t* spec)
{
#if defined(QSE_CHAR_IS_MCHAR)
	qse_mcstr_t mbs;
	qse_awk_fnc_t* fnc;
	qse_awk_fnc_spec_t mspec;

	QSE_STATIC_ASSERT (QSE_SIZEOF(*spec) == QSE_SIZEOF(mspec));

	QSE_MEMCPY (&mspec, spec, QSE_SIZEOF(mspec));
	if (spec->arg.spec)
	{
		mbs.ptr = qse_awk_wcstombsdup(awk, spec->arg.spec, &mbs.len);
		if (!mbs.ptr) return QSE_NULL;
		mspec.arg.spec = mbs.ptr;
	}

	mbs.ptr = qse_awk_wcstombsdup(awk, name, &mbs.len);
	if (!mbs.ptr)
	{
		if (mspec.arg.spec) qse_awk_freemem (awk, (qse_mchar_t*)mspec.arg.spec);
		return QSE_NULL;
	}

	fnc = add_fnc(awk, mbs.ptr, &mspec);
	qse_awk_freemem (awk, mbs.ptr);
	if (mspec.arg.spec) qse_awk_freemem (awk, (qse_mchar_t*)mspec.arg.spec);
	return fnc;
#else
	return add_fnc(awk, name, spec);
#endif
}

int qse_awk_delfncwithmbs (qse_awk_t* awk, const qse_mchar_t* name)
{
	qse_mcstr_t ncs;
	qse_wcstr_t wcs;

	ncs.ptr = (qse_mchar_t*)name;
	ncs.len = qse_mbslen(name);

#if defined(QSE_CHAR_IS_MCHAR)
	if (qse_htb_delete(awk->fnc.user, ncs.ptr, ncs.len) <= -1)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOENT, &ncs);
		return -1;
	}
#else
	wcs.ptr = qse_awk_mbstowcsdup(awk, ncs.ptr, &wcs.len);
	if (!wcs.ptr) return -1;
	if (qse_htb_delete(awk->fnc.user, wcs.ptr, wcs.len) <= -1)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOENT, &wcs);
		qse_awk_freemem (awk, wcs.ptr);
		return -1;
	}
	qse_awk_freemem (awk, wcs.ptr);
#endif

	return 0;
}

int qse_awk_delfncwithwcs (qse_awk_t* awk, const qse_wchar_t* name)
{
	qse_wcstr_t ncs;
	qse_mcstr_t mbs;

	ncs.ptr = (qse_wchar_t*)name;
	ncs.len = qse_wcslen(name);

#if defined(QSE_CHAR_IS_MCHAR)
	mbs.ptr = qse_awk_wcstombsdup(awk, ncs.ptr, &mbs.len);
	if (!mbs.ptr) return -1;
	if (qse_htb_delete(awk->fnc.user, mbs.ptr, mbs.len) <= -1)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOENT, &mbs);
		qse_awk_freemem (awk, mbs.ptr);
		return -1;
	}
	qse_awk_freemem (awk, mbs.ptr);
#else
	if (qse_htb_delete(awk->fnc.user, ncs.ptr, ncs.len) <= -1)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOENT, &ncs);
		return -1;
	}
#endif

	return 0;
}

void qse_awk_clrfnc (qse_awk_t* awk)
{
	qse_htb_clear (awk->fnc.user);
}

static qse_awk_fnc_t* find_fnc (qse_awk_t* awk, const qse_cstr_t* name)
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

	pair = qse_htb_search(awk->fnc.user, name->ptr, name->len);
	if (pair)
	{
		qse_awk_fnc_t* fnc;
		fnc = (qse_awk_fnc_t*)QSE_HTB_VPTR(pair);
		if ((awk->opt.trait & fnc->spec.trait) == fnc->spec.trait) return fnc;
	}

	qse_awk_seterrnum (awk, QSE_AWK_ENOENT, name);
	return QSE_NULL;
}

qse_awk_fnc_t* qse_awk_findfncwithmcstr (qse_awk_t* awk, const qse_mcstr_t* name)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return find_fnc(awk, name);
#else
	qse_wcstr_t wcs;
	qse_awk_fnc_t* fnc;

	wcs.ptr = qse_awk_mbsntowcsdup(awk, name->ptr, name->len, &wcs.len);
	if (!wcs.ptr) return QSE_NULL;
	fnc = find_fnc(awk, &wcs);
	qse_awk_freemem (awk, wcs.ptr);
	return fnc;
#endif
}

qse_awk_fnc_t* qse_awk_findfncwithwcstr (qse_awk_t* awk, const qse_wcstr_t* name)
{
#if defined(QSE_CHAR_IS_MCHAR)
	qse_mcstr_t mbs;
	qse_awk_fnc_t* fnc;

	mbs.ptr = qse_awk_wcsntombsdup(awk, name->ptr, name->len, &mbs.len);
	if (!mbs.ptr) return QSE_NULL;
	fnc = find_fnc(awk, &mbs);
	qse_awk_freemem (awk, mbs.ptr);
	return fnc;
#else
	return find_fnc(awk, name);
#endif
}


static int fnc_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* v, * a0, * a1 = QSE_NULL;
	int n;

	qse_char_t* name, * opt = QSE_NULL;
	qse_size_t len, optlen = 0;
       
	nargs = qse_awk_rtx_getnargs(rtx);
	QSE_ASSERT (nargs == 1 || nargs == 2);

	a0 = qse_awk_rtx_getarg (rtx, 0);
	if (nargs >= 2) a1 = qse_awk_rtx_getarg(rtx, 1);
	QSE_ASSERT (a0 != QSE_NULL);

	name = qse_awk_rtx_getvalstr(rtx, a0, &len);
	if (name == QSE_NULL) return -1;

	if (a1)
	{
		opt = qse_awk_rtx_getvalstr(rtx, a1, &optlen);
		if (opt == QSE_NULL) 
		{
			qse_awk_rtx_freevalstr(rtx, a0, name);
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
		if (optlen != 1 || (opt[0] != QSE_T('r') && opt[0] != QSE_T('w')))
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
		if (a0->type != QSE_AWK_VAL_STR) qse_awk_rtx_freemem (rtx, name);
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

static int flush_io (qse_awk_rtx_t* rtx, int rio, const qse_char_t* name, int n)
{
	int n2;

	if (rtx->rio.handler[rio] != QSE_NULL)
	{
		n2 = qse_awk_rtx_flushio (rtx, rio, name);
		if (n2 <= -1)
		{
			/*
			if (rtx->errinf.num == QSE_AWK_EIOIMPL) n = -1;
			else if (rtx->errinf.num == QSE_AWK_EIONMNF) 
			{
				if (n != 0) n = -2;
			}
			else n = -99; 
			*/	
			if (rtx->errinf.num == QSE_AWK_EIONMNF) 
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

static int index_or_rindex (qse_awk_rtx_t* rtx, int rindex)
{
	/* this is similar to the built-in index() function but doesn't
	 * care about IGNORECASE. */
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_awk_int_t idx, boundary = 1;

	nargs = qse_awk_rtx_getnargs(rtx);
	a0 = qse_awk_rtx_getarg(rtx, 0);
	a1 = qse_awk_rtx_getarg(rtx, 1);

	/*
	index ("abc", "d", 3);
	rindex ("abcdefabcdx", "cd", 8);
	*/

	if (nargs >= 3) 
	{
		qse_awk_val_t* a2;
		int n;

		a2 = qse_awk_rtx_getarg(rtx, 2);
		n = qse_awk_rtx_valtoint(rtx, a2, &boundary);
		if (n <= -1) return -1;
	}

	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		qse_mchar_t* str0, * str1, * ptr;
		qse_size_t len0, len1;

		str0 = ((qse_awk_val_mbs_t*)a0)->val.ptr;
		len0 = ((qse_awk_val_mbs_t*)a0)->val.len;

		str1 = qse_awk_rtx_getvalmbs(rtx, a1, &len1);
		if (!str1) return -1;

		if (nargs < 3)
		{
			boundary = rindex? len0: 1;
		}
		else
		{
			if (boundary == 0) boundary = 1;
			else if (boundary < 0) boundary = len0 + boundary + 1;
		}

		if (boundary > len0 || boundary <= 0)
		{
			ptr = QSE_NULL;
		}
		else if (rindex)
		{
			/* 'boundary' acts as an end position */
			ptr = rtx->gbl.ignorecase?
				qse_mbsxnrcasestr(&str0[0], boundary, str1, len1):
				qse_mbsxnrstr(&str0[0], boundary, str1, len1);
		}
		else
		{
			/* 'boundary' acts as an start position */
			ptr = rtx->gbl.ignorecase?
				qse_mbsxncasestr(&str0[boundary-1], len0 -boundary + 1, str1, len1):
				qse_mbsxnstr(&str0[boundary-1], len0 - boundary + 1, str1, len1);
		}

		idx = (ptr == QSE_NULL)? 0: ((qse_awk_int_t)(ptr - str0) + 1);

		qse_awk_rtx_freevalmbs (rtx, a1, str1);
	}
	else
	{
		qse_char_t* str0, * str1, * ptr;
		qse_size_t len0, len1;

		str0 = qse_awk_rtx_getvalstr(rtx, a0, &len0);
		if (!str0) return -1;

		str1 = qse_awk_rtx_getvalstr(rtx, a1, &len1);
		if (!str1)
		{
			qse_awk_rtx_freevalstr (rtx, a0, str0);
			return -1;
		}

		if (nargs < 3)
		{
			boundary = rindex? len0: 1;
		}
		else
		{
			if (boundary == 0) boundary = 1;
			else if (boundary < 0) boundary = len0 + boundary + 1;
		}

		if (boundary > len0 || boundary <= 0)
		{
			ptr = QSE_NULL;
		}
		else if (rindex)
		{
			/* 'boundary' acts as an end position */
			ptr = rtx->gbl.ignorecase?
				qse_strxnrcasestr(&str0[0], boundary, str1, len1):
				qse_strxnrstr(&str0[0], boundary, str1, len1);
		}
		else
		{
			/* 'boundary' acts as an start position */
			ptr = rtx->gbl.ignorecase?
				qse_strxncasestr(&str0[boundary-1], len0 - boundary + 1, str1, len1):
				qse_strxnstr(&str0[boundary-1], len0 - boundary + 1, str1, len1);
		}

		idx = (ptr == QSE_NULL)? 0: ((qse_awk_int_t)(ptr - str0) + 1);

		qse_awk_rtx_freevalstr (rtx, a1, str1);
		qse_awk_rtx_freevalstr (rtx, a0, str0);
	}

	a0 = qse_awk_rtx_makeintval(rtx, idx);
	if (a0 == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, a0);
	return 0;
}

int qse_awk_fnc_index (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return index_or_rindex (rtx, 0);
}

int qse_awk_fnc_rindex (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return index_or_rindex (rtx, 1);
}

int qse_awk_fnc_length (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* v;
	qse_awk_val_type_t vtype;
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
		vtype = QSE_AWK_RTX_GETVALTYPE (rtx, v);

		switch (vtype)
		{
			case QSE_AWK_VAL_MAP:
				/* map size */
				len = QSE_HTB_SIZE(((qse_awk_val_map_t*)v)->map);
				break;

			case QSE_AWK_VAL_STR:
				/* string length */
				len = ((qse_awk_val_str_t*)v)->val.len;
				break;

			case QSE_AWK_VAL_MBS:
				len = ((qse_awk_val_mbs_t*)v)->val.len;
				break;

			default:
				/* convert to string and get length */
				str = qse_awk_rtx_valtostrdup(rtx, v, &len);
				if (!str) return -1;
				qse_awk_rtx_freemem (rtx, str);
		}
	}

	v = qse_awk_rtx_makeintval(rtx, len);
	if (!v) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

int qse_awk_fnc_substr (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a2, * r;
	qse_awk_int_t lindex, lcount;
	int n;

	nargs = qse_awk_rtx_getnargs(rtx);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = qse_awk_rtx_getarg(rtx, 0);
	a1 = qse_awk_rtx_getarg(rtx, 1);
	a2 = (nargs >= 3)? qse_awk_rtx_getarg(rtx, 2): QSE_NULL;

	n = qse_awk_rtx_valtoint(rtx, a1, &lindex);
	if (n <= -1) return -1;

	if (a2)
	{
		n = qse_awk_rtx_valtoint(rtx, a2, &lcount);
		if (n <= -1) return -1;
		if (lcount < 0) lcount = 0;
	}
	else lcount = QSE_TYPE_MAX(qse_awk_int_t);

	lindex = lindex - 1;
	if (lindex < 0) lindex = 0;

	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		qse_mchar_t* str;
		qse_size_t len;

		str = ((qse_awk_val_mbs_t*)a0)->val.ptr;
		len = ((qse_awk_val_mbs_t*)a0)->val.len;

		if (lindex >= (qse_awk_int_t)len) lindex = (qse_awk_int_t)len;
		if (lcount > (qse_awk_int_t)len - lindex) lcount = (qse_awk_int_t)len - lindex;

		r = qse_awk_rtx_makembsval(rtx, &str[lindex], (qse_size_t)lcount);
		if (!r) return -1;
	}
	else
	{
		qse_char_t* str;
		qse_size_t len;

		str = qse_awk_rtx_getvalstr(rtx, a0, &len);
		if (!str) return -1;

		if (lindex >= (qse_awk_int_t)len) lindex = (qse_awk_int_t)len;
		if (lcount > (qse_awk_int_t)len - lindex) lcount = (qse_awk_int_t)len - lindex;

		r = qse_awk_rtx_makestrval(rtx, &str[lindex], (qse_size_t)lcount);
		qse_awk_rtx_freevalstr (rtx, a0, str);
		if (!r) return -1;
	}

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

int qse_awk_fnc_split (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a2, * t1, * t2;
	qse_awk_val_type_t a1_vtype, a2_vtype, t1_vtype;

	qse_cstr_t str;
	qse_cstr_t fs;
	qse_char_t* fs_free = QSE_NULL;
	const qse_char_t* p;
	qse_size_t str_left, org_len;
	void* fs_rex = QSE_NULL; 
	void* fs_rex_free = QSE_NULL;

	qse_cstr_t tok;
	qse_awk_int_t nflds;

	qse_awk_errnum_t errnum;
	int x;

	str.ptr = QSE_NULL;
	str.len = 0;

	nargs = qse_awk_rtx_getnargs(rtx);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = qse_awk_rtx_getarg(rtx, 0);
	a1 = qse_awk_rtx_getarg(rtx, 1);
	a2 = (nargs >= 3)? qse_awk_rtx_getarg (rtx, 2): QSE_NULL;

	a1_vtype = QSE_AWK_RTX_GETVALTYPE (rtx, a1);
	QSE_ASSERT (a1_vtype == QSE_AWK_VAL_REF);

	str.ptr = qse_awk_rtx_getvalstr(rtx, a0, &str.len);
	if (str.ptr == QSE_NULL) goto oops;

	if (a2 == QSE_NULL)
	{
		/* get the value from FS */
		t1 = qse_awk_rtx_getgbl(rtx, QSE_AWK_GBL_FS);
		t1_vtype = QSE_AWK_RTX_GETVALTYPE(rtx, t1);
		if (t1_vtype == QSE_AWK_VAL_NIL)
		{
			fs.ptr = QSE_T(" ");
			fs.len = 1;
		}
		else if (t1_vtype == QSE_AWK_VAL_STR)
		{
			fs.ptr = ((qse_awk_val_str_t*)t1)->val.ptr;
			fs.len = ((qse_awk_val_str_t*)t1)->val.len;
		}
		else
		{
			fs.ptr = qse_awk_rtx_valtostrdup(rtx, t1, &fs.len);
			if (fs.ptr == QSE_NULL) goto oops;
			fs_free = (qse_char_t*)fs.ptr;
		}

		if (fs.len > 1) fs_rex = rtx->gbl.fs[rtx->gbl.ignorecase];
	}
	else 
	{
		a2_vtype = QSE_AWK_RTX_GETVALTYPE (rtx, a2);

		if (a2_vtype == QSE_AWK_VAL_REX)
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
			if (a2_vtype == QSE_AWK_VAL_STR)
			{
				fs.ptr = ((qse_awk_val_str_t*)a2)->val.ptr;
				fs.len = ((qse_awk_val_str_t*)a2)->val.len;
			}
			else
			{
				fs.ptr = qse_awk_rtx_valtostrdup(rtx, a2, &fs.len);
				if (fs.ptr == QSE_NULL) goto oops;
				fs_free = (qse_char_t*)fs.ptr;
			}

			if (fs.len > 1) 
			{
				int x;

				if (rtx->gbl.ignorecase)
					x = qse_awk_buildrex(rtx->awk, fs.ptr, fs.len, &errnum, QSE_NULL, &fs_rex);
				else
					x = qse_awk_buildrex(rtx->awk, fs.ptr, fs.len, &errnum, &fs_rex, QSE_NULL);

				if (x <= -1)
				{
					qse_awk_rtx_seterrnum (rtx, errnum, QSE_NULL);
					goto oops;
				}

				fs_rex_free = fs_rex;
			}
		}
	}

	t1 = qse_awk_rtx_makemapval(rtx);
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
			p = qse_awk_rtx_strxntok(rtx, p, str.len, fs.ptr, fs.len, &tok);
		}
		else
		{
			p = qse_awk_rtx_strxntokbyrex(rtx, str.ptr, org_len, p, str.len, fs_rex, &tok, &errnum); 
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
		key_len = qse_awk_inttostr (rtx->awk, ++nflds, 10, QSE_NULL, key_buf, QSE_COUNTOF(key_buf));
		QSE_ASSERT (key_len != (qse_size_t)-1);

		if (qse_awk_rtx_setmapvalfld(rtx, t1, key_buf, key_len, t2) == QSE_NULL)
		{
			qse_awk_rtx_refupval (rtx, t2);
			qse_awk_rtx_refdownval (rtx, t2);
			goto oops;
		}

		str.len = str_left - (p - str.ptr);
	}

	/*if (str_free) qse_awk_rtx_freemem (rtx, str_free);*/
	qse_awk_rtx_freevalstr (rtx, a0, str.ptr);

	if (fs_free) qse_awk_rtx_freemem (rtx, fs_free);

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
	/*if (str_free) qse_awk_rtx_freemem (rtx, str_free);*/
	if (str.ptr) qse_awk_rtx_freevalstr (rtx, a0, str.ptr);

	if (fs_free) qse_awk_rtx_freemem (rtx, fs_free);

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

	nargs = qse_awk_rtx_getnargs(rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (rtx, 0);
	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		qse_mcstr_t str;
		str.ptr = qse_awk_rtx_getvalmbs(rtx, a0, &str.len);
		if (!str.ptr) return -1;
		r = qse_awk_rtx_makembsvalwithmcstr(rtx, &str);
		qse_awk_rtx_freevalmbs (rtx, a0, str.ptr);
		if (!r) return -1;
		str.ptr = ((qse_awk_val_mbs_t*)r)->val.ptr;
		str.len = ((qse_awk_val_mbs_t*)r)->val.len;
		for (i = 0; i < str.len; i++) str.ptr[i] = QSE_AWK_TOMLOWER(rtx->awk, str.ptr[i]);
	}
	else
	{
		qse_cstr_t str;
		str.ptr = qse_awk_rtx_getvalstr(rtx, a0, &str.len);
		if (!str.ptr) return -1;
		r = qse_awk_rtx_makestrvalwithcstr(rtx, &str);
		qse_awk_rtx_freevalstr (rtx, a0, str.ptr);
		if (!r) return -1;
		str.ptr = ((qse_awk_val_str_t*)r)->val.ptr;
		str.len = ((qse_awk_val_str_t*)r)->val.len;
		for (i = 0; i < str.len; i++) str.ptr[i] = QSE_AWK_TOLOWER(rtx->awk, str.ptr[i]);
	}
	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

int qse_awk_fnc_toupper (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_size_t i;
	qse_awk_val_t* a0, * r;

	nargs = qse_awk_rtx_getnargs(rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (rtx, 0);
	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		qse_mcstr_t str;
		str.ptr = qse_awk_rtx_getvalmbs(rtx, a0, &str.len);
		if (!str.ptr) return -1;
		r = qse_awk_rtx_makembsvalwithmcstr(rtx, &str);
		qse_awk_rtx_freevalmbs (rtx, a0, str.ptr);
		if (!r) return -1;
		str.ptr = ((qse_awk_val_mbs_t*)r)->val.ptr;
		str.len = ((qse_awk_val_mbs_t*)r)->val.len;
		for (i = 0; i < str.len; i++) str.ptr[i] = QSE_AWK_TOMUPPER(rtx->awk, str.ptr[i]);
	}
	else
	{
		qse_cstr_t str;
		str.ptr = qse_awk_rtx_getvalstr(rtx, a0, &str.len);
		if (!str.ptr) return -1;
		r = qse_awk_rtx_makestrvalwithcstr(rtx, &str);
		qse_awk_rtx_freevalstr (rtx, a0, str.ptr);
		if (!r) return -1;
		str.ptr = ((qse_awk_val_str_t*)r)->val.ptr;
		str.len = ((qse_awk_val_str_t*)r)->val.len;
		for (i = 0; i < str.len; i++) str.ptr[i] = QSE_AWK_TOUPPER(rtx->awk, str.ptr[i]);
	}
	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int __substitute (qse_awk_rtx_t* rtx, qse_awk_int_t max_count)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a2, * v;
	qse_awk_val_type_t a0_vtype;

	qse_cstr_t s0, s2;
	qse_cstr_t s1;
	const qse_char_t* s2_end;

	qse_char_t* s0_free = QSE_NULL;
	qse_char_t* s2_free = QSE_NULL;

	void* rex = QSE_NULL;
	void* rex_free = QSE_NULL;

	qse_str_t new;
	int new_inited = 0;

	qse_cstr_t mat, pmat, cur;

	qse_awk_int_t sub_count;

	s1.ptr = QSE_NULL;
	s1.len = 0;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = qse_awk_rtx_getarg(rtx, 0);
	a1 = qse_awk_rtx_getarg(rtx, 1);
	a2 = (nargs >= 3)? qse_awk_rtx_getarg(rtx, 2): QSE_NULL;

	a0_vtype = QSE_AWK_RTX_GETVALTYPE (rtx, a0);
	QSE_ASSERT (a2 == QSE_NULL || QSE_AWK_RTX_GETVALTYPE(rtx, a2) == QSE_AWK_VAL_REF);

	if (a0_vtype == QSE_AWK_VAL_REX)
	{
		rex = ((qse_awk_val_rex_t*)a0)->code[rtx->gbl.ignorecase];
	}
	else if (a0_vtype == QSE_AWK_VAL_STR)
	{
		s0.ptr = ((qse_awk_val_str_t*)a0)->val.ptr;
		s0.len = ((qse_awk_val_str_t*)a0)->val.len;
	}
	else
	{
		s0.ptr = qse_awk_rtx_valtostrdup (rtx, a0, &s0.len);
		if (s0.ptr == QSE_NULL) goto oops;
		s0_free = (qse_char_t*)s0.ptr;
	}

	s1.ptr = qse_awk_rtx_getvalstr (rtx, a1, &s1.len);
	if (s1.ptr == QSE_NULL) goto oops;

	if (a2 == QSE_NULL)
	{
		/* is this correct? any needs to use inrec.d0? */
		s2.ptr = QSE_STR_PTR(&rtx->inrec.line);
		s2.len = QSE_STR_LEN(&rtx->inrec.line);
	}
	else
	{
		s2.ptr = qse_awk_rtx_valtostrdup (rtx, a2, &s2.len);
		if (s2.ptr == QSE_NULL) goto oops;
		s2_free = (qse_char_t*)s2.ptr;
	}

	if (qse_str_init (&new, qse_awk_rtx_getmmgr(rtx), s2.len) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		goto oops;
	}
	new_inited = 1;

	if (a0_vtype != QSE_AWK_VAL_REX)
	{
		qse_awk_errnum_t errnum;
		int x;

		if (rtx->gbl.ignorecase)
			x = qse_awk_buildrex (rtx->awk, s0.ptr, s0.len, &errnum, QSE_NULL, &rex);
		else
			x = qse_awk_buildrex (rtx->awk, s0.ptr, s0.len, &errnum, &rex, QSE_NULL);

		if (x <= -1)
		{
			qse_awk_rtx_seterrnum (rtx, errnum, QSE_NULL);
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
				rtx->awk, rex, rtx->gbl.ignorecase, 
				&s2, &cur, &mat, QSE_NULL, &errnum
			);
		}
		else n = 0;

		if (n <= -1)
		{
			qse_awk_rtx_seterrnum (rtx, errnum, QSE_NULL);
			goto oops;
		}

		if (n == 0) 
		{ 
			/* no more match found */
			if (qse_str_ncat (
				&new, cur.ptr, cur.len) == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				goto oops;
			}
			break;
		}

		if (mat.len == 0 && pmat.ptr != QSE_NULL && mat.ptr == pmat.ptr + pmat.len)
		{
			/* match length is 0 and the match is still at the
			 * end of the previous match */
			goto skip_one_char;
		}

		if (qse_str_ncat(&new, cur.ptr, mat.ptr - cur.ptr) == (qse_size_t)-1)
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			goto oops;
		}

		for (i = 0; i < s1.len; i++)
		{
			if ((i+1) < s1.len && s1.ptr[i] == QSE_T('\\') && s1.ptr[i+1] == QSE_T('&'))
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
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
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

			if (cur.ptr < s2_end)
			{
				m = qse_str_ncat (&new, cur.ptr, 1);
				if (m == (qse_size_t)-1)
				{
					qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
					goto oops;
				}
			}

			cur.ptr++; cur.len--;
		}
	}

	if (rex_free)
	{
		if (rtx->gbl.ignorecase)
			qse_awk_freerex (rtx->awk, QSE_NULL, rex_free); 
		else
			qse_awk_freerex (rtx->awk, rex_free, QSE_NULL); 
		rex_free = QSE_NULL;
	}

	if (sub_count > 0)
	{
		if (a2 == QSE_NULL)
		{
			int n;
			n = qse_awk_rtx_setrec(rtx, 0, QSE_STR_XSTR(&new));
			if (n <= -1) goto oops;
		}
		else 
		{
			int n;

			v = qse_awk_rtx_makestrvalwithcstr(rtx, QSE_STR_XSTR(&new));
			if (v == QSE_NULL) goto oops;
			qse_awk_rtx_refupval (rtx, v);
			n = qse_awk_rtx_setrefval(rtx, (qse_awk_val_ref_t*)a2, v);
			qse_awk_rtx_refdownval (rtx, v);
			if (n <= -1) goto oops;
		}
	}

	qse_str_fini (&new);

	if (s2_free) qse_awk_rtx_freemem (rtx, s2_free);

	qse_awk_rtx_freevalstr (rtx, a1, s1.ptr);

	if (s0_free) qse_awk_rtx_freemem (rtx, s0_free);

	v = qse_awk_rtx_makeintval (rtx, sub_count);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;

oops:
	if (rex_free) 
	{
		if (rtx->gbl.ignorecase)
			qse_awk_freerex (rtx->awk, QSE_NULL, rex_free);
		else	
			qse_awk_freerex (rtx->awk, rex_free, QSE_NULL); 
	}
	if (new_inited) qse_str_fini (&new);
	if (s2_free) qse_awk_rtx_freemem (rtx, s2_free);
	if (s1.ptr) qse_awk_rtx_freevalstr (rtx, a1, s1.ptr);
	if (s0_free) qse_awk_rtx_freemem (rtx, s0_free);
	return -1;
}

int qse_awk_fnc_gsub (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return __substitute(rtx, 0);
}

int qse_awk_fnc_sub (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return __substitute(rtx, 1);
}

int qse_awk_fnc_match (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1, * a3;
	qse_char_t* str0;
	qse_size_t len0;
	qse_awk_int_t idx, start = 1;
	qse_awk_val_t* x0 = QSE_NULL, * x1 = QSE_NULL, * x2 = QSE_NULL;
	int n;
	qse_cstr_t mat, submat[9];
	qse_str_t* tmpbuf = QSE_NULL;

	nargs = qse_awk_rtx_getnargs(rtx);
	QSE_ASSERT (nargs >= 2 && nargs <= 4);
	
	a0 = qse_awk_rtx_getarg(rtx, 0);
	a1 = qse_awk_rtx_getarg(rtx, 1);

	if (nargs >= 3) 
	{
		qse_awk_val_t* a2;

		a2 = qse_awk_rtx_getarg(rtx, 2);
		/* if the 3rd parameter is not an array,
		 * it is treated as a match start index */
		n = qse_awk_rtx_valtoint(rtx, a2, &start);
		if (n <= -1) return -1;

		if (nargs >= 4) a3 = qse_awk_rtx_getarg(rtx, 3);
	}

#if 0
	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		str0 = ((qse_awk_val_mbs_t*)a0)->val.ptr;
		len0 = ((qse_awk_val_mbs_t*)a0)->val.len;
	}
#endif

	str0 = qse_awk_rtx_getvalstr(rtx, a0, &len0);
	if (str0 == QSE_NULL) return -1;

	if (start == 0) start = 1;
	else if (start < 0) start = len0 + start + 1;

	QSE_MEMSET (submat, 0, QSE_SIZEOF(submat));
	if (start > len0 || start <= 0) n = 0;
	else
	{
		qse_cstr_t tmp;

		/*TODO: must use str0,len0? */
		tmp.ptr = str0 + start - 1;
		tmp.len = len0 - start + 1;

		n = qse_awk_rtx_matchrex(rtx, a1, &tmp, &tmp, &mat, (nargs >= 4? submat: QSE_NULL));
		if (n <= -1) 
		{
			qse_awk_rtx_freevalstr (rtx, a0, str0);
			return -1;
		}
	}

	qse_awk_rtx_freevalstr (rtx, a0, str0);

	/* RSTART: 0 on no match */
	idx = (n == 0)? 0: ((qse_awk_int_t)(mat.ptr - str0) + 1);

	x0 = qse_awk_rtx_makeintval(rtx, idx);
	if (!x0) goto oops;
	qse_awk_rtx_refupval (rtx, x0);

	/* RLENGTH: -1 on no match */
	x1 = qse_awk_rtx_makeintval(rtx, ((n == 0)? (qse_awk_int_t)-1: (qse_awk_int_t)mat.len));
	if (!x1) goto oops;
	qse_awk_rtx_refupval (rtx, x1);

	if (nargs >= 4)
	{
		const qse_cstr_t* subsep;
		qse_awk_int_t submatcount;
		qse_size_t i, xlen;
		qse_awk_val_t* tv;

		tmpbuf = qse_str_open(qse_awk_rtx_getmmgr(rtx), 0, 64);
		if (!tmpbuf) goto oops;

		x2 = qse_awk_rtx_makemapval(rtx);
		if (!x2) goto oops;
		qse_awk_rtx_refupval (rtx, x2);

		submatcount =0;
		subsep = qse_awk_rtx_getsubsep (rtx);
		for (i = 0; i < QSE_COUNTOF(submat); i++)
		{
			if (!submat[i].ptr) break;

			submatcount++;

			if (qse_str_fmt(tmpbuf, QSE_T("%d"), (int)submatcount) == (qse_size_t)-1 ||
			    qse_str_ncat(tmpbuf, subsep->ptr, subsep->len) == (qse_size_t)-1) goto oops;
			xlen = QSE_STR_LEN(tmpbuf);
			if (qse_str_ncat(tmpbuf, QSE_T("start"), 5) == (qse_size_t)-1) goto oops;

			tv = qse_awk_rtx_makeintval (rtx, submat[i].ptr - str0 + 1);
			if (!tv) goto oops;
			if (!qse_awk_rtx_setmapvalfld (rtx, x2, QSE_STR_PTR(tmpbuf), QSE_STR_LEN(tmpbuf), tv))
			{
				qse_awk_rtx_refupval (rtx, tv);
				qse_awk_rtx_refdownval (rtx, tv);
				goto oops;
			}

			if (qse_str_setlen(tmpbuf, xlen) == (qse_size_t)-1 ||
			    qse_str_ncat(tmpbuf, QSE_T("length"), 6) == (qse_size_t)-1) goto oops;

			tv = qse_awk_rtx_makeintval(rtx, submat[i].len);
			if (!tv) goto oops;
			if (!qse_awk_rtx_setmapvalfld(rtx, x2, QSE_STR_PTR(tmpbuf), QSE_STR_LEN(tmpbuf), tv))
			{
				qse_awk_rtx_refupval (rtx, tv);
				qse_awk_rtx_refdownval (rtx, tv);
				goto oops;
			}
		}
		/* the caller of this function must be able to get the submatch count by
		 * dividing the array size by 2 */

		if (qse_awk_rtx_setrefval(rtx, (qse_awk_val_ref_t*)a3, x2) <= -1) goto oops;
	}

	if (qse_awk_rtx_setgbl(rtx, QSE_AWK_GBL_RSTART, x0) <= -1 ||
	    qse_awk_rtx_setgbl(rtx, QSE_AWK_GBL_RLENGTH, x1) <= -1)
	{
		goto oops;
	}

	qse_awk_rtx_setretval (rtx, x0);

	if (tmpbuf) qse_str_close (tmpbuf);
	if (x2) qse_awk_rtx_refdownval (rtx, x2);
	qse_awk_rtx_refdownval (rtx, x1);
	qse_awk_rtx_refdownval (rtx, x0);
	return 0;

oops:
	if (tmpbuf) qse_str_close (tmpbuf);
	if (x2) qse_awk_rtx_refdownval (rtx, x2);
	if (x1) qse_awk_rtx_refdownval (rtx, x1);
	if (x0) qse_awk_rtx_refdownval (rtx, x0);
	return -1;
}

int qse_awk_fnc_sprintf (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs > 0);

	a0 = qse_awk_rtx_getarg(rtx, 0);
	if (QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MBS)
	{
		qse_mbs_t out, fbu;
		int out_inited = 0, fbu_inited = 0;
		qse_mcstr_t cs0;
		qse_mcstr_t x;

		if (qse_mbs_init(&out, qse_awk_rtx_getmmgr(rtx), 256) <= -1)
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			goto oops_mbs;
		}
		out_inited = 1;

		if (qse_mbs_init(&fbu, qse_awk_rtx_getmmgr(rtx), 256) <= -1)
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			goto oops_mbs;
		}
		fbu_inited = 1;

		cs0.ptr = qse_awk_rtx_getvalmbs(rtx, a0, &cs0.len);
		if (cs0.ptr == QSE_NULL) goto oops_mbs;

		x.ptr = qse_awk_rtx_formatmbs(rtx, &out, &fbu, cs0.ptr, cs0.len, nargs, QSE_NULL, &x.len);
		qse_awk_rtx_freevalmbs (rtx, a0, cs0.ptr);
		if (!x.ptr) goto oops_mbs;
		
		a0 = qse_awk_rtx_makembsvalwithmcstr(rtx, &x);
		if (a0 == QSE_NULL) goto oops_mbs;

		qse_mbs_fini (&fbu);
		qse_mbs_fini (&out);
		qse_awk_rtx_setretval (rtx, a0);
		return 0;

	oops_mbs:
		if (fbu_inited) qse_mbs_fini (&fbu);
		if (out_inited) qse_mbs_fini (&out);
		return -1;
	}
	else
	{
		qse_str_t out, fbu;
		int out_inited = 0, fbu_inited = 0;
		qse_cstr_t cs0;
		qse_cstr_t x;

		if (qse_str_init(&out, qse_awk_rtx_getmmgr(rtx), 256) <= -1)
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			goto oops;
		}
		out_inited = 1;

		if (qse_str_init(&fbu, qse_awk_rtx_getmmgr(rtx), 256) <= -1)
		{
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			goto oops;
		}
		fbu_inited = 1;

		cs0.ptr = qse_awk_rtx_getvalstr(rtx, a0, &cs0.len);
		if (cs0.ptr == QSE_NULL) goto oops;

		x.ptr = qse_awk_rtx_format(rtx, &out, &fbu, cs0.ptr, cs0.len, nargs, QSE_NULL, &x.len);
		qse_awk_rtx_freevalstr (rtx, a0, cs0.ptr);
		if (!x.ptr) goto oops;
		
		a0 = qse_awk_rtx_makestrvalwithcstr(rtx, &x);
		if (a0 == QSE_NULL) goto oops;

		qse_str_fini (&fbu);
		qse_str_fini (&out);
		qse_awk_rtx_setretval (rtx, a0);
		return 0;

	oops:
		if (fbu_inited) qse_str_fini (&fbu);
		if (out_inited) qse_str_fini (&out);
		return -1;
	}
}

static int fnc_int (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_awk_int_t lv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs(rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg(rtx, 0);

	n = qse_awk_rtx_valtoint(rtx, a0, &lv);
	if (n <= -1) return -1;

	r = qse_awk_rtx_makeintval(rtx, lv);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_typename (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* a0;
	qse_awk_val_t* r;
	const qse_char_t* name;

	a0 = qse_awk_rtx_getarg(rtx, 0);
	name = qse_awk_rtx_getvaltypename(rtx, a0);

	r = qse_awk_rtx_makestrval(rtx, name, qse_strlen(name));
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_isnil (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* a0;
	qse_awk_val_t* r;

	a0 = qse_awk_rtx_getarg(rtx, 0);

	r = qse_awk_rtx_makeintval(rtx, QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_NIL);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_ismap (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* a0;
	qse_awk_val_t* r;

	a0 = qse_awk_rtx_getarg(rtx, 0);

	r = qse_awk_rtx_makeintval(rtx, QSE_AWK_RTX_GETVALTYPE(rtx, a0) == QSE_AWK_VAL_MAP);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static QSE_INLINE int asort_compare (const void* x1, const void* x2, void* ctx, int* cv)
{
	int n;
	if (qse_awk_rtx_cmpval((qse_awk_rtx_t*)ctx, *(qse_awk_val_t**)x1, *(qse_awk_val_t**)x2, &n) <= -1) return -1;
	*cv = n;
	return 0;
}

struct cud_t
{
	qse_awk_rtx_t* rtx;
	qse_awk_fun_t* fun;
};

static QSE_INLINE int asort_compare_ud (const void* x1, const void* x2, void* ctx, int* cv)
{
	struct cud_t* cud = (struct cud_t*)ctx;
	qse_awk_val_t* r, * args[2];
	qse_awk_int_t rv;

	args[0] = *(qse_awk_val_t**)x1;
	args[1] = *(qse_awk_val_t**)x2;
	r = qse_awk_rtx_callfun(cud->rtx, cud->fun, args, 2); 
	if (!r) return -1;
	if (qse_awk_rtx_valtoint(cud->rtx, r,  &rv) <= -1) return -1;
	*cv = rv;
	return 0;
}

static QSE_INLINE int __fnc_asort (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi, int sort_keys)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a0_val, * a1, * a2;
	qse_awk_val_type_t a0_type, v_type;
	qse_awk_val_t* r, * rmap = QSE_NULL;
	qse_awk_int_t rv = 0; /* as if no element in the map */
	qse_awk_val_map_itr_t itr;
	qse_awk_fun_t* fun = QSE_NULL;
	qse_size_t msz, i;
	qse_awk_val_t** va;
	int x;

	nargs = qse_awk_rtx_getnargs(rtx);

	a0 = qse_awk_rtx_getarg(rtx, 0);
	a0_type = QSE_AWK_RTX_GETVALTYPE(rtx, a0);
	QSE_ASSERT (a0_type == QSE_AWK_VAL_REF);

	v_type = qse_awk_rtx_getrefvaltype(rtx, (qse_awk_val_ref_t*)a0);
	if (v_type != QSE_AWK_VAL_MAP)
	{
		if (v_type == QSE_AWK_VAL_NIL)
		{
			/* treat it as an empty value */
			goto done;
		}

		qse_awk_rtx_seterrfmt (rtx, QSE_AWK_ENOTMAP, QSE_NULL, QSE_T("source not a map"));
		return -1;
	}

	a0_val = qse_awk_rtx_getrefval(rtx, (qse_awk_val_ref_t*)a0);
	QSE_ASSERT (QSE_AWK_RTX_GETVALTYPE(rtx, a0_val) == QSE_AWK_VAL_MAP);

	if (nargs >= 2)
	{
		a1 = qse_awk_rtx_getarg(rtx, 1); /* destination map */
		
		if (nargs >= 3)
		{
			a2 = qse_awk_rtx_getarg(rtx, 2);
			if (QSE_AWK_RTX_GETVALTYPE(rtx, a2) != QSE_AWK_VAL_FUN)
			{
				qse_awk_rtx_seterrfmt (rtx, QSE_AWK_EINVAL, QSE_NULL, QSE_T("comparator not a function"));
				return -1;
			}

			fun = ((qse_awk_val_fun_t*)a2)->fun;
			if (fun->nargs < 2) 
			{
				/* the comparison accepts less than 2 arguments */
				qse_awk_rtx_seterrfmt (rtx, QSE_AWK_EINVAL, QSE_NULL, QSE_T("%.*s not accepting 2 arguments"), (int)fun->name.len, fun->name.ptr);
				return -1;
			}
		}
	}
	else
	{
		a1 = a0; /* let a0 be the destination. a0 is both source and destination */
	}

	if (!qse_awk_rtx_getfirstmapvalitr(rtx, a0_val, &itr)) goto done; /* map empty */

	msz = qse_htb_getsize(((qse_awk_val_map_t*)a0_val)->map);
	QSE_ASSERT (msz > 0);

	va = (qse_awk_val_t**)qse_awk_rtx_allocmem(rtx, msz * QSE_SIZEOF(*va));
	if (!va) return -1;
	i = 0;
	if (sort_keys)
	{
		do
		{
			const qse_cstr_t* key = QSE_AWK_VAL_MAP_ITR_KEY(&itr);
			va[i] = qse_awk_rtx_makestrvalwithcstr(rtx, key);
			if (!va[i]) 
			{
				while (i > 0)
				{
					--i;
					qse_awk_rtx_refdownval (rtx, va[i]);
				}
				qse_awk_rtx_freemem (rtx, va);
				return -1;
			}
			qse_awk_rtx_refupval (rtx, va[i]);
			i++;
		}
		while (qse_awk_rtx_getnextmapvalitr(rtx, a0_val, &itr));
	}
	else
	{
		do
		{
			va[i] = (qse_awk_val_t*)QSE_AWK_VAL_MAP_ITR_VAL(&itr);
			qse_awk_rtx_refupval (rtx, va[i]);
			i++;
		}
		while (qse_awk_rtx_getnextmapvalitr(rtx, a0_val, &itr));
	}

	if (fun)
	{
		struct cud_t cud;
		cud.rtx = rtx;
		cud.fun = fun;
		x = qse_qsortx(va, msz, QSE_SIZEOF(*va), asort_compare_ud, &cud);
	}
	else
	{
		x = qse_qsortx(va, msz, QSE_SIZEOF(*va), asort_compare, rtx);
	}

	if (x <= -1 || !(rmap = qse_awk_rtx_makemapval(rtx)))
	{
		for (i = 0; i < msz; i++) qse_awk_rtx_refdownval (rtx, va[i]);
		qse_awk_rtx_freemem (rtx, va);
		return -1;
	}

	for (i = 0; i < msz; i++)
	{
		qse_char_t ridx[128]; /* TODO: make it dynamic? can overflow? */
		qse_size_t ridx_len;

		ridx_len = qse_fmtuintmax (
			ridx,
			QSE_COUNTOF(ridx),
			i,
			10 | QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL,
			-1,
			QSE_T('\0'),
			QSE_NULL
		);

		if (qse_awk_rtx_setmapvalfld(rtx, rmap, ridx, ridx_len, va[i]) == QSE_NULL)
		{
			/* decrement the reference count of the values not added to the map */
			do
			{
				qse_awk_rtx_refdownval (rtx, va[i]);
				i++;
			}
			while (i < msz);
			qse_awk_rtx_freeval (rtx, rmap, 0); /* this derefs the elements added. */
			qse_awk_rtx_freemem (rtx, va);
			return -1;
		}

		qse_awk_rtx_refdownval (rtx, va[i]); /* deref it as it has been added to the map */
	}

	rv = msz;
	qse_awk_rtx_freemem (rtx, va);

done:
	r = qse_awk_rtx_makeintval(rtx, rv);
	if (!r) return -1;

	if (rmap)
	{
		/* rmap can be NULL when a jump has been made for an empty source 
		 * at the beginning of this fucntion */
		qse_awk_rtx_refupval (rtx, rmap);
		x = qse_awk_rtx_setrefval (rtx, (qse_awk_val_ref_t*)a1, rmap);
		qse_awk_rtx_refdownval (rtx, rmap);
		if (x <= -1) 
		{
			qse_awk_rtx_freeval (rtx, r, 0);
			return -1;
		}
	}

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_asort (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return __fnc_asort(rtx, fi, 0);
}

static int fnc_asorti (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return __fnc_asort(rtx, fi, 1);
}
