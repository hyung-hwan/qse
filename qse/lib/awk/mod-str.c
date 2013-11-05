/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#include "mod-str.h"
#include <qse/cmn/str.h>
#include "../cmn/mem.h"

static int fnc_normspace (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	/* normalize spaces 
	 * - trim leading and trailing spaces
	 * - replace a series of spaces to a single space
	 */
	qse_xstr_t path;
	qse_awk_val_t* retv;

	path.ptr = qse_awk_rtx_valtostrdup (
		rtx, qse_awk_rtx_getarg(rtx, 0), &path.len);
	if (path.ptr)
	{
		path.len = qse_strxpac (path.ptr, path.len);
		retv = qse_awk_rtx_makestrval (rtx, path.ptr, path.len);
		qse_awk_rtx_freemem (rtx, path.ptr);
		if (retv) qse_awk_rtx_setretval (rtx, retv);
	}

	return 0;
}

static int trim (qse_awk_rtx_t* rtx, int flags)
{
	qse_xstr_t path;
	qse_char_t* npath;
	qse_awk_val_t* retv;

	path.ptr = qse_awk_rtx_valtostrdup (
		rtx, qse_awk_rtx_getarg(rtx, 0), &path.len);
	if (path.ptr)
	{
		npath = qse_strxtrmx (path.ptr, &path.len, flags);
		retv = qse_awk_rtx_makestrval (rtx, npath, path.len);
		qse_awk_rtx_freemem (rtx, path.ptr);
		if (retv) qse_awk_rtx_setretval (rtx, retv);
	}

	return 0;
}

static int fnc_trim (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return trim (rtx, QSE_STRTRMX_LEFT | QSE_STRTRMX_RIGHT);
}
static int fnc_ltrim (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return trim (rtx, QSE_STRTRMX_LEFT);
}
static int fnc_rtrim (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return trim (rtx, QSE_STRTRMX_RIGHT);
}

static int index_or_rindex (qse_awk_rtx_t* rtx, int rindex)
{
	/* this is similar to the built-in index() function but doesn't
	 * care about IGNORECASE. */
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_char_t* str0, * str1, * ptr;
	qse_size_t len0, len1;
	qse_awk_int_t idx, boundary = 1;

	nargs = qse_awk_rtx_getnargs (rtx);
	a0 = qse_awk_rtx_getarg (rtx, 0);
	a1 = qse_awk_rtx_getarg (rtx, 1);

	/*
	str::index ("abc", "d", 3);
	str::rindex ("abcdefabcdx", "cd", 8);
	*/

	if (nargs >= 3) 
	{
		qse_awk_val_t* a2;
		int n;

		a2 = qse_awk_rtx_getarg (rtx, 2);
		n = qse_awk_rtx_valtoint (rtx, a2, &boundary);
		if (n <= -1) return -1;
	}

	if (a0->type == QSE_AWK_VAL_STR)
	{
		str0 = ((qse_awk_val_str_t*)a0)->val.ptr;
		len0 = ((qse_awk_val_str_t*)a0)->val.len;
	}
	else
	{
		str0 = qse_awk_rtx_valtostrdup (rtx, a0, &len0);
		if (str0 == QSE_NULL) return -1;
	}

	if (a1->type == QSE_AWK_VAL_STR)
	{
		str1 = ((qse_awk_val_str_t*)a1)->val.ptr;
		len1 = ((qse_awk_val_str_t*)a1)->val.len;
	}
	else
	{
		str1 = qse_awk_rtx_valtostrdup (rtx, a1, &len1);
		if (str1 == QSE_NULL)
		{
			if (a0->type != QSE_AWK_VAL_STR) 
				qse_awk_rtx_freemem (rtx, str0);
			return -1;
		}
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

	if (rindex)
	{
		/* 'boundary' acts as an end position */
		ptr = (boundary > len0 || boundary <= 0)? 
			QSE_NULL: qse_strxnrstr (&str0[0], boundary, str1, len1);
	}
	else
	{
		/* 'boundary' acts as a start position */
		ptr = (boundary > len0 || boundary <= 0)? 
			QSE_NULL: qse_strxnstr (&str0[boundary-1], len0-boundary+1, str1, len1);
	}

	idx = (ptr == QSE_NULL)? 0: ((qse_awk_int_t)(ptr-str0) + 1);

	if (a0->type != QSE_AWK_VAL_STR) qse_awk_rtx_freemem (rtx, str0);
	if (a1->type != QSE_AWK_VAL_STR) qse_awk_rtx_freemem (rtx, str1);

	a0 = qse_awk_rtx_makeintval (rtx, idx);
	if (a0 == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, a0);
	return 0;
}

static int fnc_index (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return index_or_rindex (rtx, 0);
}
static int fnc_rindex (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	return index_or_rindex (rtx, 1);
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
	{ QSE_T("index"),     { { 2, 3, QSE_NULL }, fnc_index,     0 } },
	{ QSE_T("ltrim"),     { { 1, 1, QSE_NULL }, fnc_ltrim,     0 } },
	{ QSE_T("normspace"), { { 1, 1, QSE_NULL }, fnc_normspace, 0 } },
	{ QSE_T("rindex"),    { { 2, 3, QSE_NULL }, fnc_rindex,    0 } },
	{ QSE_T("rtrim"),     { { 1, 1, QSE_NULL }, fnc_rtrim,     0 } },
	{ QSE_T("trim"),      { { 1, 1, QSE_NULL }, fnc_trim,      0 } }
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

#if 0
	left = 0; right = QSE_COUNTOF(inttab) - 1;
	while (left <= right)
	{
		mid = (left + right) / 2;

		n = qse_strcmp (inttab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_INT;
			sym->u.in = inttab[mid].info;
			return 0;
		}
     }
#endif

	ea.ptr = name;
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
	/* TODO: 
	for (each pid for rtx) kill (pid, SIGKILL);
	for (each pid for rtx) waitpid (pid, QSE_NULL, 0);
	*/
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	/* TODO: anything */
}

int qse_awk_mod_str (qse_awk_mod_t* mod, qse_awk_t* awk)
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

