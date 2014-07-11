#include <qse/awk/stdawk.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/main.h>
#include "awk00.h"

static const qse_char_t* src = QSE_T(
	"function dump(x) { OFS=\"=\"; for (k in x) print k, x[k]; x[\"f99\"]=\"os2\"; return x; }"
);

static int awk_main (int argc, qse_char_t* argv[])
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_parsestd_t psin[2];
	qse_awk_val_t* rtv = QSE_NULL;
	qse_awk_val_t* arg = QSE_NULL;
	int ret = -1, opt;

	/* this structure is passed to qse_awk_rtx_makemapvalwithdata() */
	qse_awk_val_map_data_t md[] =
	{
		{ { QSE_T("f0"), 2 }, QSE_AWK_VAL_MAP_DATA_STR, QSE_T("linux") },
		{ { QSE_T("f1"), 2 }, QSE_AWK_VAL_MAP_DATA_STR, QSE_T("openvms") },
		{ { QSE_T("f2"), 2 }, QSE_AWK_VAL_MAP_DATA_STR, QSE_T("hpux") },
		{ { QSE_NULL,    0 }, 0, QSE_NULL } /* last item */
	};

	/* create a standard awk object */
	awk = qse_awk_openstd (0, QSE_NULL);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

	/* get the awk's trait */
	qse_awk_getopt (awk, QSE_AWK_TRAIT, &opt);
	/* change the trait value to disallow BEGIN, END, pattern-action blocks */
	opt &= ~QSE_AWK_PABLOCK;
	/* update the trait */
	qse_awk_setopt (awk, QSE_AWK_TRAIT, &opt);

	/* prepare a script to parse */
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

	/* create a standard runtime context */
	rtx = qse_awk_rtx_openstd (
		awk, 
		0,
		QSE_T("awk06"),
		QSE_NULL, /* stdin */
		QSE_NULL, /* stdout */
		QSE_NULL  /* default cmgr */
	
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_geterrmsg(awk));
		goto oops;
	}
	
	/* create a map value to pass as an argument */
	arg = qse_awk_rtx_makemapvalwithdata (rtx, md);
	if (arg == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_rtx_geterrmsg(rtx));
		goto oops;
	}
	qse_awk_rtx_refupval (rtx, arg);
	
	/* execute the dump function in the awk script */
	rtv = qse_awk_rtx_call (rtx, QSE_T("dump"), &arg, 1);
	if (rtv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_rtx_geterrmsg(rtx));
		goto oops;
	}

	if (rtv->type == QSE_AWK_VAL_MAP)
	{
		/* if a returned value is a map, 
		 * traverse the map and print the key/value pairs. */

		qse_awk_val_map_itr_t itr;
		qse_awk_val_map_itr_t* iptr;

		/* get the iterator to the first key/value pair */
		iptr = qse_awk_rtx_getfirstmapvalitr (rtx, rtv, &itr);
		while (iptr)
		{
			qse_cstr_t str;

			/* #QSE_AWK_VAL_MAP_ITR_VAL returns the value part */
			str.ptr = qse_awk_rtx_valtostrdup (
				rtx, QSE_AWK_VAL_MAP_ITR_VAL(iptr), &str.len);
			if (str.ptr == QSE_NULL)
			{
				qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_rtx_geterrmsg(rtx));
				goto oops;
			}
	
			/* #QSE_AWK_VAL_MAP_ITR_KEY returns the key part */
			qse_printf (QSE_T("ret [%.*s]=[%.*s]\n"), 
				(int)QSE_AWK_VAL_MAP_ITR_KEY(iptr)->len, 
				QSE_AWK_VAL_MAP_ITR_KEY(iptr)->ptr,
				(int)str.len, str.ptr
			);
			qse_awk_rtx_freemem (rtx, str.ptr);
			
			/* get the iterator to the next key/value pair */
			iptr = qse_awk_rtx_getnextmapvalitr (rtx, rtv, &itr);
		}
	}
	else
	{
		/* if it is a plain value, convert it to a string
		 * and print it */
		qse_cstr_t str;

		str.ptr = qse_awk_rtx_valtostrdup (rtx, rtv, &str.len);
		if (str.ptr == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_rtx_geterrmsg(rtx));
			goto oops;
		}
	
		qse_printf (QSE_T("ret [%.*s]\n"), (int)str.len, str.ptr);
		qse_awk_rtx_freemem (rtx, str.ptr);
	}

	ret = 0;

oops:
	/* clear the return value */
	if (rtv) qse_awk_rtx_refdownval (rtx, rtv);

	/* dereference the argument */
	if (arg) qse_awk_rtx_refdownval (rtx, arg);

	/* destroy a runtime context */
	if (rtx) qse_awk_rtx_close (rtx);

	/* destroy the processor */
	if (awk) qse_awk_close (awk);

	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int x;
	qse_openstdsios ();
	init_awk_sample_locale ();
	x = qse_runmain (argc, argv, awk_main);
	qse_closestdsios ();
	return x;
}

