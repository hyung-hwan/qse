#include <qse/awk/stdawk.h>
#include <qse/cmn/sio.h>
#include <qse/cmn/main.h>
#include "awk00.h"

static const qse_char_t* src = QSE_T(
	"BEGIN { G0 = 10; G1 = \"hello, world\"; G2 = sin(90); match (\"abcdefg\", /[c-f]+/); }"
);

struct ginfo_t
{
	int g[3];
};

typedef struct ginfo_t ginfo_t;

static int awk_main (int argc, qse_char_t* argv[])
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_parsestd_t psin[2];
	qse_char_t* str;
	qse_size_t len;
	qse_awk_val_t* rtv;
	int ret = -1, i;
	ginfo_t* ginfo;

	/* create an awk object */
	awk = qse_awk_openstd (QSE_SIZEOF(*ginfo));
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

	/* add global variables G1, G2, and G3. store the IDs to the extension 
	 * area. the extension area is used for demonstration. there is no special
	 * need to use it when adding global variables. */
	ginfo = qse_awk_getxtnstd (awk);
	for (i = 0; i < QSE_COUNTOF(ginfo->g); i++)
	{
		qse_char_t name[] = QSE_T("GX");
		name[1] = QSE_T('0') + i; 
		ginfo->g[i] = qse_awk_addgbl (awk, name);
		if (ginfo->g[i] <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_geterrmsg(awk));
			goto oops;
		}
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
		QSE_T("awk08"), 
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

	/* get the values of built-in global variables
	 * and print them */
	for (i = 0; i < QSE_COUNTOF(ginfo->g); i++)
	{
		str = qse_awk_rtx_valtostrdup (rtx, qse_awk_rtx_getgbl (rtx, ginfo->g[i]), &len);
		if (str == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_rtx_geterrmsg(rtx));
			goto oops;
		}
		qse_printf (QSE_T("G%d => %.*s\n"), i, (int)len, str);
		qse_awk_rtx_freemem (rtx, str);
	}

	/* get the value of RLENGTH and print it */
	str = qse_awk_rtx_valtostrdup (rtx, qse_awk_rtx_getgbl (rtx, QSE_AWK_GBL_RLENGTH), &len);
	if (str == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_awk_rtx_geterrmsg(rtx));
		goto oops;
	}
	qse_printf (QSE_T("RLENGTH => %.*s\n"), (int)len, str);
	qse_awk_rtx_freemem (rtx, str);

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
	qse_openstdsios ();
	init_awk_sample_locale ();
	x = qse_runmain (argc, argv, awk_main);
	qse_closestdsios ();
	return x;
}

