#include <qse/awk/std.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>
#include "awk00.h"

static const qse_char_t* src = QSE_T(
	"function pow(x,y) { return x ** y; }"
);

static int awk_main (int argc, qse_char_t* argv[])
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_parsestd_t psin[2];
	qse_char_t* str;
	qse_size_t len;
	qse_awk_val_t* rtv = QSE_NULL;
	qse_awk_val_t* arg[2] = { QSE_NULL, QSE_NULL };
	int ret, i, opt;

	/* create an awk object */
	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: cannot open awk\n"));
		ret = -1; goto oops;
	}

	/* get the awk's trait */
	qse_awk_getopt (awk, QSE_AWK_TRAIT, &opt);
	/* change the trait value to disallow BEGIN, END, pattern-action blocks */
	opt &= ~QSE_AWK_PABLOCK;
	/* update the trait */
	qse_awk_setopt (awk, QSE_AWK_TRAIT, &opt);

	/* prepare an awk script to parse */
	psin[0].type = QSE_AWK_PARSESTD_STR;
	psin[0].u.str.ptr = src;
	psin[0].u.str.len = qse_strlen(src);
	psin[1].type = QSE_AWK_PARSESTD_NULL;

	/* parse the script */
	ret = qse_awk_parsestd (awk, psin, QSE_NULL);
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
		QSE_T("awk04"), 
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
	
	/* create the first argument to the pow function to call */
	arg[0] = qse_awk_rtx_makeintval (rtx, 50);
	if (arg[0] == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}
	qse_awk_rtx_refupval (rtx, arg[0]);

	/* create the second argument to the pow function to call */
	arg[1] = qse_awk_rtx_makeintval (rtx, 3);
	if (arg[1] == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}
	qse_awk_rtx_refupval (rtx, arg[1]);

	/* call the pow function */
	rtv = qse_awk_rtx_call (rtx, QSE_T("pow"), arg, 2);
	if (rtv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	/* duplicate the return value to a string */
	str = qse_awk_rtx_valtostrdup (rtx, rtv, &len);

	/* clear the return value */
	qse_awk_rtx_refdownval (rtx, rtv);

	if (str == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("error: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	qse_printf (QSE_T("[%.*s]\n"), (int)len, str);

	/* destroy the duplicated string  */
	qse_awk_rtx_freemem (rtx, str);

oops:
	/* dereference all arguments */
	for (i = 0; i < QSE_COUNTOF(arg); i++)
	{
		if (arg[i]) qse_awk_rtx_refdownval (rtx, arg[i]);
	}

	/* destroy a runtime context */
	if (rtx) qse_awk_rtx_close (rtx);
	/* destroy the processor */
	if (awk) qse_awk_close (awk);

	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	init_awk_sample_locale ();
	return qse_runmain (argc, argv, awk_main);
}

