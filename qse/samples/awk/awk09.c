#include <qse/awk/stdawk.h>
#include <qse/si/sio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/path.h>
#include "awk00.h"

static const qse_char_t* src = QSE_T(
	"BEGIN { print basename(\"/etc/passwd\");  }"
);

static int fnc_basename (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* a0, * rv;
	qse_char_t* ptr;
	qse_size_t len;

	/* note that this implementation doesn't care if the parameter
	 * contains a null character like "/etc/p\0asswd" */

	/* get the value of the first parameter */
	a0 = qse_awk_rtx_getarg (rtx, 0);	
	if (qse_awk_rtx_getvaltype (rtx, a0) == QSE_AWK_VAL_STR)
	{
		/* if it is a string value, don't duplicate the value */
		ptr = ((qse_awk_val_str_t*)a0)->val.ptr;
		len = ((qse_awk_val_str_t*)a0)->val.len;

		/* make a string value with the base name  */
		rv = qse_awk_rtx_makestrvalwithstr (rtx, qse_basename (ptr));
	}
	else
	{
		/* if it is a string value, convert the value to a string 
		 * with duplication  */
		ptr = qse_awk_rtx_valtostrdup (rtx, a0, &len);	
		if (ptr == QSE_NULL) return -1; 

		/* make a string value with the base name  */
		rv = qse_awk_rtx_makestrvalwithstr (rtx, qse_basename (ptr));

		/* free the duplicated string */
		qse_awk_rtx_freemem (rtx, ptr);
	}

	if (rv == QSE_NULL) return -1;

	/* set the return value that basename() will return */
	qse_awk_rtx_setretval (rtx, rv);

	/* implemenation success */
	return 0;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_parsestd_t psin[2];
	qse_awk_val_t* rtv;
	int ret = -1;
	qse_awk_fnc_spec_t spec;

	/* create an awk object */
	awk = qse_awk_openstd (0, QSE_NULL);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

	/* add a built-in function basename() */
	qse_memset (&spec, 0, QSE_SIZEOF(spec));
	spec.arg.min = 1; /* limit the number of arguments to 1 */
	spec.arg.max = 1;
	spec.impl = fnc_basename; /* specify the actual implementation */
	if (qse_awk_addfnc (awk, QSE_T("basename"), &spec) == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_geterrmsg(awk));
		goto oops;
	}

	/* prepare an awk script to parse */
	psin[0].type = QSE_AWK_PARSESTD_STR;
	psin[0].u.str.ptr = src;
	psin[0].u.str.len = qse_strlen(src);
	psin[1].type = QSE_AWK_PARSESTD_NULL;

	/* parse the script */
	if (qse_awk_parsestd (awk, psin, QSE_NULL) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_geterrmsg(awk));
		goto oops;
	}

	/* create a runtime context */
	rtx = qse_awk_rtx_openstd (
		awk, 
		0,
		QSE_T("awk09"), 
		QSE_NULL, /* stdin */
		QSE_NULL, /* stdout */
		QSE_NULL  /* default cmgr */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_geterrmsg(awk));
		goto oops;
	}
	
	/* execute the script over the standard data streams */
	rtv = qse_awk_rtx_loop (rtx);
	if (rtv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_rtx_geterrmsg(rtx));
		goto oops;
	}

	/* clear the return value */
	qse_awk_rtx_refdownval (rtx, rtv);

	ret = 0;

oops:
	/* destroy a runtime context */
	if (rtx) qse_awk_rtx_close (rtx);

	/* destroy the processor */
	if (awk) qse_awk_close (awk);

	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int x;
	qse_open_stdsios ();
	init_awk_sample_locale ();
	x = qse_run_main (argc, argv, awk_main);
	qse_close_stdsios ();
	return x;
}

