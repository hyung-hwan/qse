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

#include "mod-str.h"
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include "../cmn/mem.h"
#include "fnc.h"

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

static fnctab_t fnctab[] =
{
	/* keep this table sorted for binary search in query(). */
	{ QSE_T("atan"),    { { 1, 1, QSE_NULL },     qse_awk_fnc_atan,     0 } },
	{ QSE_T("atan2"),   { { 2, 2, QSE_NULL },     qse_awk_fnc_atan2,    0 } },
	{ QSE_T("cos"),     { { 1, 1, QSE_NULL },     qse_awk_fnc_cos,      0 } },
	{ QSE_T("exp"),     { { 1, 1, QSE_NULL },     qse_awk_fnc_exp,      0 } },
	{ QSE_T("log"),     { { 1, 1, QSE_NULL },     qse_awk_fnc_log,      0 } },
	{ QSE_T("log10"),   { { 1, 1, QSE_NULL },     qse_awk_fnc_log10,    0 } },
	{ QSE_T("sin"),     { { 1, 1, QSE_NULL },     qse_awk_fnc_sin,      0 } },
	{ QSE_T("sqrt"),    { { 1, 1, QSE_NULL },     qse_awk_fnc_sqrt,     0 } },
	{ QSE_T("tan"),     { { 1, 1, QSE_NULL },     qse_awk_fnc_tan,      0 } },

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

int qse_awk_mod_math (qse_awk_mod_t* mod, qse_awk_t* awk)
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

