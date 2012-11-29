/*
 * $Id: awk04.c 441 2011-04-22 14:28:43Z hyunghwan.chung $
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
#include <qse/awk/std.h>
#include <qse/cmn/stdio.h>

static const qse_char_t* src = QSE_T(
	"function dump(x) { OFS=\"=\"; for (k in x) print k, x[k]; x[\"f99\"]=\"os2\"; return x; }"
);

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_parsestd_t psin;
	qse_awk_val_t* rtv = QSE_NULL;
	qse_awk_val_t* arg = QSE_NULL;
	int ret, i, opt;
	struct 
	{
		const qse_char_t* kptr;
		qse_size_t klen;
		const qse_char_t* vptr;
	} xxx[] =
	{
		{ QSE_T("f0"), 2, QSE_T("linux") },
		{ QSE_T("f1"), 2, QSE_T("openvms") },
		{ QSE_T("f2"), 2, QSE_T("hpux") }
	};

	/* create a main processor */
	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: cannot open awk\n"));
		ret = -1; goto oops;
	}

	qse_awk_getopt (awk, QSE_AWK_TRAIT, &opt);
	/* don't allow BEGIN, END, pattern-action blocks */
	opt &= ~QSE_AWK_PABLOCK;
	/* can assign a map to a variable */
	opt |= QSE_AWK_FLEXMAP;
	qse_awk_setopt (awk, QSE_AWK_TRAIT, &opt);

	psin.type = QSE_AWK_PARSESTD_STR;
	psin.u.str.ptr = src;
	psin.u.str.len = qse_strlen(src);

	ret = qse_awk_parsestd (awk, &psin, QSE_NULL);
	if (ret == -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}

	/* create a runtime context */
	rtx = qse_awk_rtx_openstd (
		awk, 
		0,
		QSE_T("awk10"),
		QSE_NULL, /* stdin */
		QSE_NULL, /* stdout */
		QSE_NULL  /* default cmgr */
	
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_geterrmsg(awk));
		ret = -1; goto oops;
	}
	
	/* prepare a argument to be a map */
	arg = qse_awk_rtx_makemapval (rtx);
	if (arg == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}
	qse_awk_rtx_refupval (rtx, arg);

	/* insert some key/value pairs into the map */
	for (i = 0; i < QSE_COUNTOF(xxx); i++)
	{
		qse_awk_val_t* v, * fv;

		fv = qse_awk_rtx_makestrvalwithstr (rtx, xxx[i].vptr);
		if (fv == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
				qse_awk_rtx_geterrmsg(rtx));
			ret = -1; goto oops;
		}
		qse_awk_rtx_refupval (rtx, fv);
		v = qse_awk_rtx_setmapvalfld (rtx, arg, xxx[i].kptr, xxx[i].klen, fv);
		qse_awk_rtx_refdownval (rtx, fv);
		if (v == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
				qse_awk_rtx_geterrmsg(rtx));
			ret = -1; goto oops;
		}
	}
	
	/* invoke the dump function */
	rtv = qse_awk_rtx_call (rtx, QSE_T("dump"), &arg, 1);
	if (rtv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	/* print the return value */
	if (rtv->type == QSE_AWK_VAL_MAP)
	{
		qse_awk_val_map_itr_t itr;
		qse_awk_val_map_itr_t* iptr;

		iptr = qse_awk_rtx_getfirstmapvalitr (rtx, rtv, &itr);
		while (iptr)
		{
			qse_xstr_t str;

			str.ptr = qse_awk_rtx_valtostrdup (
				rtx, QSE_AWK_VAL_MAP_ITR_VAL(iptr), &str.len);
			if (str.ptr == QSE_NULL)
			{
				qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
					qse_awk_rtx_geterrmsg(rtx));
				ret = -1; goto oops;
			}
	
			qse_printf (QSE_T("ret [%.*s]=[%.*s]\n"), 
				(int)QSE_AWK_VAL_MAP_ITR_KEY(iptr)->len, 
				QSE_AWK_VAL_MAP_ITR_KEY(iptr)->ptr,
				(int)str.len, str.ptr
			);
			qse_awk_rtx_freemem (rtx, str.ptr);
			
			iptr = qse_awk_rtx_getnextmapvalitr (rtx, rtv, &itr);
		}
	}
	else
	{
		qse_xstr_t str;

		str.ptr = qse_awk_rtx_valtostrdup (rtx, rtv, &str.len);
		if (str.ptr == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
				qse_awk_rtx_geterrmsg(rtx));
			ret = -1; goto oops;
		}
	
		qse_printf (QSE_T("ret [%.*s]\n"), (int)str.len, str.ptr);
		qse_awk_rtx_freemem (rtx, str.ptr);
	}

oops:
	/* clear the return value */
	if (rtv) qse_awk_rtx_refdownval (rtx, rtv);

	/* dereference the argument */
	if (arg) qse_awk_rtx_refdownval (rtx, arg);

	/* destroy a runtime context */
	if (rtx) qse_awk_rtx_close (rtx);
	/* destroy the processor */
	if (awk) qse_awk_close (awk);
	return ret;
}
