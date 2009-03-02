/*
 * $Id: awk03.c 90 2009-03-01 09:58:19Z hyunghwan.chung $
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
	"function init() { a = 20; return a; }"
	"function main() { return ++a; }"
	"function fini() { print ++a; return a; }"
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

	qse_awk_parsestd_in_t psin;

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
		awk, 0,
		QSE_NULL,                  /* no console input */
		QSE_AWK_RTX_OPENSTD_STDIO  /* stdout for console output */
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
			ret = -1; goto oops;
		}

		qse_printf (QSE_T("return: [%.*s]\n"), (int)len, str);
		qse_awk_rtx_free (rtx, str);

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
/******/
