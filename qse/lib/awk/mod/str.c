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

#include <qse/awk/awk.h>
#include <qse/cmn/str.h>
#include "../../cmn/mem.h"

static int fnc_normspc (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
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

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

static fnctab_t fnctab[] =
{
	/* keep this table sorted for binary search in query(). */
	{ QSE_T("ltrim"),    { { 1, 1, QSE_NULL }, fnc_ltrim,    0  } },
	{ QSE_T("normspc"),  { { 1, 1, QSE_NULL }, fnc_normspc,  0  } },
	{ QSE_T("rtrim"),    { { 1, 1, QSE_NULL }, fnc_rtrim,    0  } },
	{ QSE_T("trim"),     { { 1, 1, QSE_NULL }, fnc_trim,     0  } }
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

QSE_EXPORT int load (qse_awk_mod_t* mod, qse_awk_t* awk)
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

#if defined(__DOS__)
/* kind of DllMain() for Causeway DLL */
int main (int eax) { return 0; }
#endif
