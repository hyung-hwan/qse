/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

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

#include "mod-sed.h"
#include <qse/sed/stdsed.h>
#include "../cmn/mem.h"

#if 0
static int fnc_errno (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	uctx_list_t* list;
	qse_awk_val_t* retv;

	list = rtx_to_list (rtx, fi);

	retv = qse_awk_rtx_makeintval (rtx, list->errnum);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static qse_char_t* errmsg[] =
{
	QSE_T("no error"),
	QSE_T("out of memory"),
	QSE_T("invalid data"),
	QSE_T("not found"),
	QSE_T("I/O error"),
	QSE_T("parse error"),
	QSE_T("duplicate data"),
	QSE_T("unknown error")
};

static int fnc_errstr (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	qse_awk_int_t errnum;

/*
	list = rtx_to_list (rtx, fi);

	if (qse_awk_rtx_getnargs (rtx) <= 0 ||
	    qse_awk_rtx_valtoint (rtx, qse_awk_rtx_getarg (rtx, 0), &errnum) <= -1)
	{
		errnum = list->errnum;
	}
*/

	ret = qse_awk_rtx_valtoint (rtx, qse_awk_rtx_getarg (rtx, 0), &id);
	if (ret <= -1) errnum = -1;


	if (errnum < 0 || errnum >= QSE_COUNTOF(errmsg)) errnum = QSE_COUNTOF(errmsg) - 1;

	retv = qse_awk_rtx_makestrvalwithstr (rtx, errmsg[errnum]);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}
#endif

static int fnc_file_to_file (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_sed_t* sed = QSE_NULL;
	qse_awk_val_t* retv;
	qse_awk_val_t* a[3];
	qse_cstr_t xstr[3];
	int i = 0, ret = 0;

	/* result = sed::file_to_file ("s/ABC/123/g", input_file, output_file [, option_string]) */

	sed = qse_sed_openstdwithmmgr (qse_awk_rtx_getmmgr(rtx), 0);
	if (sed == QSE_NULL) 
	{
		ret = -2;
		goto oops;
	}

/* TODO qse_set_opt (TRAIT) using the optional parameter */

	for (i = 0; i < 3; i++)
	{
		a[i] = qse_awk_rtx_getarg (rtx, i);
		xstr[i].ptr = qse_awk_rtx_getvalstr (rtx, a[i], &xstr[i].len);
		if (xstr[i].ptr == QSE_NULL) 
		{
			ret = -2;
			goto oops;
		}
	}

	if (qse_sed_compstdxstr (sed, &xstr[0]) <= -1) 
	{
		ret = -3; /* compile error */
		goto oops;
	}

	if (qse_sed_execstdfile (sed, xstr[1].ptr, xstr[2].ptr, QSE_NULL) <= -1) 
	{
		ret = -4;
		goto oops;
	}

oops:
	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) retv = qse_awk_rtx_makeintval (rtx, -1);

	while (i > 0)
	{
		--i;
		qse_awk_rtx_freevalstr (rtx, a[i], xstr[i].ptr);
	}

	if (sed) qse_sed_close (sed);
	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_str_to_str (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_sed_t* sed = QSE_NULL;
	qse_awk_val_t* retv;
	qse_awk_val_t* a[2], * tmp;
	qse_cstr_t xstr[2];
	qse_cstr_t outstr;
	int i = 0, ret = 0, n;

	sed = qse_sed_openstdwithmmgr (qse_awk_rtx_getmmgr(rtx), 0);
	if (sed == QSE_NULL) 
	{
		ret = -2;
		goto oops;
	}

/* TODO qse_set_opt (TRAIT) using the optional parameter */

	for (i = 0; i < 2; i++)
	{
		a[i] = qse_awk_rtx_getarg (rtx, i);
		xstr[i].ptr = qse_awk_rtx_getvalstr (rtx, a[i], &xstr[i].len);
		if (xstr[i].ptr == QSE_NULL) 
		{
			ret = -2;
			goto oops;
		}
	}

	if (qse_sed_compstdxstr (sed, &xstr[0]) <= -1) 
	{
		ret = -3; /* compile error */
		goto oops;
	}

	if (qse_sed_execstdxstr (sed, &xstr[1], &outstr, QSE_NULL) <= -1) 
	{
		ret = -4;
		goto oops;
	}

	tmp = qse_awk_rtx_makestrvalwithxstr (rtx, &outstr);
	qse_sed_freemem (sed, outstr.ptr);

	if (!tmp)
	{
		ret = -1;
		goto oops;
	}

	qse_awk_rtx_refupval (rtx, tmp);
	n = qse_awk_rtx_setrefval (rtx, (qse_awk_val_ref_t*)qse_awk_rtx_getarg (rtx, 2), tmp);
	qse_awk_rtx_refdownval (rtx, tmp);
	if (n <= -1)
	{
		ret = -5;
		goto oops;
	}

oops:
	retv = qse_awk_rtx_makeintval (rtx, ret);
	if (retv == QSE_NULL) retv = qse_awk_rtx_makeintval (rtx, -1);

	while (i > 0)
	{
		--i;
		qse_awk_rtx_freevalstr (rtx, a[i], xstr[i].ptr);
	}

	if (sed) qse_sed_close (sed);
	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

static fnctab_t fnctab[] =
{
	/* keep this table sorted for binary search in query(). */
	{ QSE_T("file_to_file"),  { { 3, 3, QSE_NULL },    fnc_file_to_file,  0 } },
	{ QSE_T("str_to_str"),    { { 3, 3, QSE_T("vvr")}, fnc_str_to_str,    0 } }
};

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
	int left, right, mid, n;

	left = 0; right = QSE_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = (left + right) / 2;

		n = qse_strcmp (fnctab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_FNC;
			sym->u.fnc = fnctab[mid].info;
			return 0;
		}
	}

	ea.ptr = (qse_char_t*)name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}

/* TODO: proper resource management */

static int init (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	return 0;
}

static void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	/* TODO: anything */
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	/* TODO: anything */
}

int qse_awk_mod_sed (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;
	/*
	mod->ctx...
	 */

	return 0;
}

