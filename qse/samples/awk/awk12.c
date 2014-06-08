#include <qse/awk/stdawk.h>
#include <qse/cmn/sio.h>

/* this sample produces 8 text files containing multiplication chart. */

const qse_char_t* src = QSE_T(
	"BEGIN {"
	"	for (i=2;i<=9;i++)"
	"	{"
	"		print \"OFILENAME:\" OFILENAME;"
	"		for (j=1;j<=9;j++)"
	"			print i \"*\" j \"=\" i * j;"
	"		nextofile;"
	"	}"
	"}"
);

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_val_t* retv;
	qse_awk_parsestd_t psin[2];
	int ret = -1, opt;

	const qse_char_t* output_files[] = 
	{
		QSE_T("awk12.out.2"),
		QSE_T("awk12.out.3"),
		QSE_T("awk12.out.4"),
		QSE_T("awk12.out.5"),
		QSE_T("awk12.out.6"),
		QSE_T("awk12.out.7"),
		QSE_T("awk12.out.8"),
		QSE_T("awk12.out.9"),
		QSE_NULL
	};

	qse_openstdsios ();

	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

	qse_awk_getopt (awk, QSE_AWK_TRAIT, &opt);
	opt |= QSE_AWK_NEXTOFILE;
	qse_awk_setopt (awk, QSE_AWK_TRAIT, &opt);

	psin[0].type = QSE_AWK_PARSESTD_STR;
	psin[0].u.str.ptr = src;
	psin[0].u.str.len = qse_strlen(src);
	psin[1].type = QSE_AWK_PARSESTD_NULL;

	if (qse_awk_parsestd (awk, psin, QSE_NULL) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}

	rtx = qse_awk_rtx_openstd (
		awk, 
		0,
		QSE_T("awk12"),
		QSE_NULL, /* stdin */
		output_files,
		QSE_NULL  /* default cmgr */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}
	
	retv = qse_awk_rtx_loop (rtx);
	if (retv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		goto oops;
	}

	qse_awk_rtx_refdownval (rtx, retv);
	ret = 0;

oops:
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	if (awk != QSE_NULL) qse_awk_close (awk);

	qse_closestdsios ();
	return ret;
}

