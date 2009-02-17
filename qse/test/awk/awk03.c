/****S* AWK/Calling Functions
 * DESCRIPTION
 *  This program demonstrates how to use qse_awk_rtx_call().
 * SOURCE
 */

#include <qse/awk/awk.h>
#include <qse/utl/stdio.h>

static const qse_char_t* src = QSE_T(
	"function init() { a = 20; }"
	"function main() { a++; }"
	"function fini() { print a; }"
);

static const qse_char_t* f[] = 
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
	int ret, i;

	/* create a main processor */
	awk = qse_awk_opensimple ();
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: cannot open awk\n"));
		ret = -1; goto oops;
	}

	/* don't allow BEGIN, END, pattern-action blocks */
	qse_awk_setoption (awk, qse_awk_getoption(awk) & ~QSE_AWK_PABLOCK);

	ret = qse_awk_parsesimple (
		awk,
		QSE_AWK_PARSE_STRING, src, /* parse AWK source in a string */
		QSE_NULL /* no parse output */
	);
	if (ret == -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}

	/* create a runtime context */
	rtx = qse_awk_rtx_opensimple (
		awk, 
		QSE_NULL /* no console files */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_geterrmsg(awk));
		ret = -1; goto oops;
	}
	
	/* invoke functions as indicated in the array f */
	for (i = 0; i < QSE_COUNTOF(f); i++)
	{
		ret = qse_awk_rtx_call (rtx, f[i], QSE_NULL, 0);
		if (ret == -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
				qse_awk_rtx_geterrmsg(rtx));
			goto oops;
		}
	}	

oops:
	/* destroy a runtime context */
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	/* destroy the processor */
	if (awk != QSE_NULL) qse_awk_close (awk);
	return ret;
}
/******/
