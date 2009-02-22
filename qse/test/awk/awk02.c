/*
 * $Id: awk.c 501 2008-12-17 08:39:15Z baconevi $ 
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

/****S* AWK/Basic Loop
 * DESCRIPTION
 *  This program demonstrates how to use qse_awk_rtx_loop().
 * SOURCE
 */

#include <qse/awk/awk.h>
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
	int ret;

	awk = qse_awk_opensimple ();
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: cannot open awk\n"));
		goto oops;
	}

	qse_memset (srcout, QSE_T(' '), QSE_COUNTOF(srcout)-1);
	srcout[QSE_COUNTOF(srcout)-1] = QSE_T('\0');

	ret = qse_awk_parsesimple (
		awk,
		/* parse the source in src */
		QSE_AWK_PARSESIMPLE_STR, src, 
		/* deparse into srcout */
		QSE_AWK_PARSESIMPLE_STR, srcout
	);
	if (ret == -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}

	qse_printf (QSE_T("DEPARSED SOURCE:\n%s\n"), srcout);
	qse_printf (QSE_T("=================================\n"));
	qse_fflush (QSE_STDOUT);

	rtx = qse_awk_rtx_opensimple (
		awk, 
		QSE_NULL,             /* no console input */
		QSE_AWK_RTX_OPENSIMPLE_STDIO /* stdout for console output */
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

/******/
