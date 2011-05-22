/*
 * $Id: awk04.c 469 2011-05-21 16:16:18Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
	"function pow(x,y) { return x ** y; }"
);

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_parsestd_in_t psin;
	qse_char_t* str;
	qse_size_t len;
	qse_awk_val_t* rtv = QSE_NULL;
	qse_awk_val_t* arg[2] = { QSE_NULL, QSE_NULL };
	qse_awk_fun_t* fun;
	int ret, i, opt;

	/* create a main processor */
	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: cannot open awk\n"));
		ret = -1; goto oops;
	}

	opt = qse_awk_getoption(awk);

	/* don't allow BEGIN, END, pattern-action blocks */
	opt &= ~QSE_AWK_PABLOCK;
	/* enable ** */
	opt |= QSE_AWK_EXTRAOPS;

	qse_awk_setoption (awk, opt);

	psin.type = QSE_AWK_PARSESTD_CP;
	psin.u.cp = src;

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
		QSE_T("awk04"),
		QSE_NULL, /* stdin */
		QSE_NULL  /* stdout */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_geterrmsg(awk));
		ret = -1; goto oops;
	}
	
	/* invoke the pow function */
	arg[0] = qse_awk_rtx_makeintval (rtx, 50);
	if (arg[0] == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}
	qse_awk_rtx_refupval (rtx, arg[0]);

	arg[1] = qse_awk_rtx_makeintval (rtx, 3);
	if (arg[1] == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}
	qse_awk_rtx_refupval (rtx, arg[1]);

	rtv = qse_awk_rtx_call (rtx, QSE_T("pow"), arg, 2);
	if (rtv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	str = qse_awk_rtx_valtocpldup (rtx, rtv, &len);
	if (str == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	qse_printf (QSE_T("[%.*s]\n"), (int)len, str);
	qse_awk_rtx_free (rtx, str);

	
	/* call the function again using different API functions */
	fun = qse_awk_rtx_findfun (rtx, QSE_T("pow"));
	if (fun == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	rtv = qse_awk_rtx_callfun (rtx, fun, arg, 2);
	if (rtv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	str = qse_awk_rtx_valtocpldup (rtx, rtv, &len);
	if (str == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	qse_printf (QSE_T("[%.*s]\n"), (int)len, str);
	qse_awk_rtx_free (rtx, str);

	

oops:
	/* clear the return value */
	if (rtv) qse_awk_rtx_refdownval (rtx, rtv);

	/* dereference all arguments */
	for (i = 0; i < QSE_COUNTOF(arg); i++)
	{
		if (arg[i]) qse_awk_rtx_refdownval (rtx, arg[i]);
	}

	/* destroy a runtime context */
	if (rtx) qse_awk_rtx_close (rtx);
	/* destroy the processor */
	if (awk) qse_awk_close (awk);
	return ret;
}
