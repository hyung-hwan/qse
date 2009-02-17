#include <qse/awk/awk.h>
#include <qse/utl/stdio.h>

const qse_char_t* src = QSE_T(
	"function init() { a = 20; }"
	"function main() { a++; }"
	"function fini() { output (a); }"
	"function output(x) { print x; }"
);

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

	rtx = qse_awk_rtx_opensimple (
		awk, 
		QSE_NULL /* no console files */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}
	
	qse_awk_rtx_call (rtx, QSE_T("init"), QSE_NULL, 0);
	qse_awk_rtx_call (rtx, QSE_T("main"), QSE_NULL, 0);
	qse_awk_rtx_call (rtx, QSE_T("main"), QSE_NULL, 0);
	qse_awk_rtx_call (rtx, QSE_T("main"), QSE_NULL, 0);
	qse_awk_rtx_call (rtx, QSE_T("fini"), QSE_NULL, 0);

oops:
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	if (awk != QSE_NULL) qse_awk_close (awk);
	return -1;
}
