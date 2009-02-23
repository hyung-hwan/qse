/*
 * $Id: awk04.c 76 2009-02-22 14:18:06Z hyunghwan.chung $
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

/****S* AWK/Calling Functions
 * DESCRIPTION
 *  This program demonstrates how to use qse_awk_rtx_call().
 *  It parses the program stored in the string src and calls the functions
 *  stated in the array fnc. If no errors occur, it should print 24.
 * SOURCE
 */

#include <qse/awk/awk.h>
#include <qse/awk/std.h>
#include <qse/utl/stdio.h>

static const qse_char_t* src = QSE_T(
	"function pow(x,y) { return x ** y; }"
);

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_parsestd_in_t psin;
	qse_char_t buf[1000];
	qse_size_t bufsize;
	qse_awk_val_t* v;
	qse_awk_val_t* arg[2] = { QSE_NULL, QSE_NULL };
	int ret, i;

	/* create a main processor */
	awk = qse_awk_openstd ();
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: cannot open awk\n"));
		ret = -1; goto oops;
	}

	/* don't allow BEGIN, END, pattern-action blocks */
	qse_awk_setoption (awk, qse_awk_getoption(awk) & ~QSE_AWK_PABLOCK);

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
		QSE_NULL,                  /* no console input */
		QSE_AWK_RTX_OPENSTD_STDIO  /* stdout for console output */
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

	v = qse_awk_rtx_call (rtx, QSE_T("pow"), arg, 2);
	if (v == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	bufsize = QSE_COUNTOF(buf);
	qse_awk_rtx_valtostr (rtx, v, 
		QSE_AWK_RTX_VALTOSTR_FIXED, buf, &bufsize);
	qse_printf (QSE_T("[%.*s]\n"), (int)bufsize, buf);

	/* clear the return value */
	qse_awk_rtx_refdownval (rtx, v);

oops:
	/* dereference all arguments */
	for (i = 0; i < QSE_COUNTOF(arg); i++)
	{
		if (arg[i] != QSE_NULL) 
			qse_awk_rtx_refdownval (rtx, arg[i]);
	}

	/* destroy a runtime context */
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	/* destroy the processor */
	if (awk != QSE_NULL) qse_awk_close (awk);
	return ret;
}
/******/
