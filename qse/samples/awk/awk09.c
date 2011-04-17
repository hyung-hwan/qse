/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/awk/std.h>
#include <qse/cmn/stdio.h>

/* this sample produces 8 text files containing multiplication char */

const qse_char_t* src = QSE_T(
	"BEGIN {"
	"    printf \"\"; # work around the empty OFILENAME when printed\n"
	"                 # for the first time in the loop below\n"
	"                 # this happens because the console file is opened\n"
	"                 # and OFILENAME is set when the first console output\n"
	"                 # operation is peformed\n"
	"	for (i=2;i<=9;i++)"
	"	{"
	"         print \"OFILENAME:\" OFILENAME;"
	"		for (j=1;j<=9;j++)"
	"			print i \"*\" j \"=\" i * j;"
	"		nextofile;"
	"	}"
	"}"
);

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_val_t* retv;
	qse_awk_parsestd_in_t psin;
	int ret = -1;

	const qse_char_t* output_files[] = 
	{
		QSE_T("awk09.out.2"),
		QSE_T("awk09.out.3"),
		QSE_T("awk09.out.4"),
		QSE_T("awk09.out.5"),
		QSE_T("awk09.out.6"),
		QSE_T("awk09.out.7"),
		QSE_T("awk09.out.8"),
		QSE_T("awk09.out.9"),
		QSE_NULL
	};

	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

	qse_awk_setoption (awk, qse_awk_getoption(awk) | QSE_AWK_NEXTOFILE);

	psin.type = QSE_AWK_PARSESTD_CP;
	psin.u.cp = src;

	if (qse_awk_parsestd (awk, &psin, QSE_NULL) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}

	rtx = qse_awk_rtx_openstd (
		awk, 
		0,
		QSE_T("awk09"),
		QSE_NULL, /* stdin */
		output_files
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}
	
	retv = qse_awk_rtx_loop (rtx);
	if (retv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		goto oops;
	}

	qse_awk_rtx_refdownval (rtx, retv);
	ret = 0;

oops:
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	if (awk != QSE_NULL) qse_awk_close (awk);
	return ret;
}

