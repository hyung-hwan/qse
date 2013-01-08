#include <qse/awk/std.h>
#include <qse/cmn/stdio.h>

static const qse_char_t* script = QSE_T("BEGIN { print \"hello, world\"; }");

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_val_t* retv;
	qse_awk_parsestd_t psin[2];
	int ret = -1;

	/* create an awk object */
	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

	/* prepare a script to parse */
	psin[0].type = QSE_AWK_PARSESTD_STR;
	psin[0].u.str.ptr = script;
	psin[0].u.str.len = qse_strlen(script);
	psin[1].type = QSE_AWK_PARSESTD_NULL;

	/* parse a script in a string */
	if (qse_awk_parsestd (awk, psin, QSE_NULL) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}

	/* open a runtime context */
	rtx = qse_awk_rtx_openstd (
		awk, 
		0,
		QSE_T("awk01"),
		QSE_NULL, /* stdin */
		QSE_NULL, /* stdout */               
		QSE_NULL  /* default cmgr */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}
	
	/* execute pattern-action blocks */
	retv = qse_awk_rtx_loop (rtx);
	if (retv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		goto oops;
	}

	/* decrement the reference count of the return value */
	qse_awk_rtx_refdownval (rtx, retv);
	ret = 0;

oops:
	/* destroy the runtime context */
	if (rtx) qse_awk_rtx_close (rtx);

	/* destroy the awk object */
	if (awk) qse_awk_close (awk);

	return ret;
}
