/*
 * $Id: awk03.c 523 2011-07-25 15:42:35Z hyunghwan.chung $
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
	"function init() { a = 20; return a; }"
	"function main() { return ++a; }"
	"function fini() { print \"a in fini() =>\", ++a; return a; }"
);

static const qse_char_t* fnc[] = 
{
	QSE_T("init"),
	QSE_T("main"),
	QSE_T("main"),
	QSE_T("main"),
	QSE_T("main"),
	QSE_T("fini"),
};

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;

	qse_awk_parsestd_t psin;

	int ret, i;

	/* create a main processor */
	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: cannot open awk\n"));
		ret = -1; goto oops;
	}

	/* don't allow BEGIN, END, pattern-action blocks */
	qse_awk_setoption (awk, qse_awk_getoption(awk) & ~QSE_AWK_PABLOCK);

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
		QSE_T("awk03"),
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
	
	/* invoke functions as indicated in the array fnc */
	for (i = 0; i < QSE_COUNTOF(fnc); i++)
	{
		qse_awk_val_t* v;
		qse_char_t* str;	
		qse_size_t len;

		v = qse_awk_rtx_call (rtx, fnc[i], QSE_NULL, 0);
		if (v == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
				qse_awk_rtx_geterrmsg(rtx));
			ret = -1; goto oops;
		}

		str = qse_awk_rtx_valtocpldup (rtx, v, &len);
		if (str == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
				qse_awk_rtx_geterrmsg(rtx));

			qse_awk_rtx_refdownval (rtx, v);
			ret = -1; goto oops;
		}

		qse_printf (QSE_T("return: [%.*s]\n"), (int)len, str);
		qse_awk_rtx_freemem (rtx, str);

		/* clear the return value */
		qse_awk_rtx_refdownval (rtx, v);
	}	

oops:
	/* destroy a runtime context */
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	/* destroy the processor */
	if (awk != QSE_NULL) qse_awk_close (awk);
	return ret;
}
