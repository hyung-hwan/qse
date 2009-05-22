/*
 * $Id: awk02.c 151 2009-05-21 06:50:02Z hyunghwan.chung $ 
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/awk/awk.h>
#include <qse/awk/std.h>
#include <qse/cmn/mem.h>
#include <qse/utl/stdio.h>

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

static qse_char_t srcout[5000];

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;

	qse_awk_parsestd_in_t psin;
	qse_awk_parsestd_out_t psout;

	int ret;

	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: cannot open awk\n"));
		goto oops;
	}

	qse_memset (srcout, QSE_T(' '), QSE_COUNTOF(srcout)-1);
	srcout[QSE_COUNTOF(srcout)-1] = QSE_T('\0');

	psin.type = QSE_AWK_PARSESTD_CP;
	psin.u.cp  = src;
	psout.type = QSE_AWK_PARSESTD_CP;
	psout.u.cp  = srcout;

	ret = qse_awk_parsestd (awk, &psin, &psout);
	if (ret == -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}

	qse_printf (QSE_T("DEPARSED SOURCE:\n%s\n"), srcout);
	qse_printf (QSE_T("=================================\n"));
	qse_fflush (QSE_STDOUT);

	rtx = qse_awk_rtx_openstd (
		awk, 0,
		QSE_NULL,                 /* no console input */
		QSE_AWK_RTX_OPENSTD_STDIO /* stdout for console output */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}
	
	ret = qse_awk_rtx_loop (rtx);
	if (ret == -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		goto oops;
	}

oops:
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	if (awk != QSE_NULL) qse_awk_close (awk);
	return -1;
}

