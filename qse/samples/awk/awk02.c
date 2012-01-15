/*
 * $Id: awk02.c 441 2011-04-22 14:28:43Z hyunghwan.chung $ 
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
#include <qse/cmn/mem.h>
#include <qse/cmn/stdio.h>

static const qse_char_t* src = QSE_T(
	"BEGIN {"
	"	for (i=2;i<=9;i++)"
	"	{"
	"		for (j=1;j<=9;j++)"
	"			print i \"*\" j \"=\" i * j;"
	"		print \"---------------------\";"
	"	}"
	"}"
);

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_val_t* retv;

	qse_awk_parsestd_t psin;
	qse_awk_parsestd_t psout;

	int ret;

	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		ret = -1; goto oops;
	}

	psin.type = QSE_AWK_PARSESTD_STR;
	psin.u.str.ptr = src;
	psin.u.str.len = qse_strlen(src);

	psout.type = QSE_AWK_PARSESTD_STR;
	/* ps.out.u.str.ptr and ps.out.u.str.len are set when qse_awk_parsestd() 
	 * returns success */

	ret = qse_awk_parsestd (awk, &psin, &psout);
	if (ret <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		ret = -1; goto oops;
	}

	qse_printf (QSE_T("DEPARSED SOURCE:\n%s\n"), psout.u.str.ptr);
	qse_printf (QSE_T("=================================\n"));
	qse_fflush (QSE_STDOUT);

	QSE_MMGR_FREE (qse_awk_getmmgr(awk), psout.u.str.ptr);

	rtx = qse_awk_rtx_openstd (
		awk, 
		0,
		QSE_T("awk02"),
		QSE_NULL,  /* stdin */
		QSE_NULL,  /* stdout */
		QSE_NULL   /* default cmgr */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		ret = -1; goto oops;
	}
	
	retv = qse_awk_rtx_loop (rtx);
	if (retv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	qse_awk_rtx_refdownval (rtx, retv);
	ret = 0;

oops:
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	if (awk != QSE_NULL) qse_awk_close (awk);
	return -1;
}

