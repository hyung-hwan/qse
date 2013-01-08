#include <qse/awk/std.h>
#include <qse/cmn/str.h>
#include <qse/cmn/stdio.h>

/* i'll print records with the second field grater than 4. 
 * at the end, we'll print the number of records seen so far */
static const qse_char_t* script = 
	QSE_T("$2 > 4 { print $0; } END { print NR; }");

/* the console input string */
static const qse_char_t* conin = 
	QSE_T("Beth 4.00 0\nDan  3.74 0\nKathy     4.00 10\nMark 5.00 20\nMary 5.50 22\nSusie     4.25 18\n");
static qse_size_t coninpos = 0;

/* the console output is stored into this buffer */
static qse_char_t conout[10000];
static qse_size_t conoutpos = 0;

/* this is the console I/O handler */
static qse_ssize_t handle_console (
     qse_awk_rtx_t*      rtx,
     qse_awk_rio_cmd_t   cmd,
     qse_awk_rio_arg_t*  arg,
     qse_char_t*         data,
     qse_size_t          count)
{
	/* this function is called separately for the console input and console
	 * output. however, since i don't maintain underlying resources like
	 * file handles, i don't really check if it's input or output.
	 * you can check the value of #qse_awk_rio_mode_t in the arg->mode 
	 * field if you want to tell. 
	 */
	switch (cmd)
	{
		case QSE_AWK_RIO_OPEN:
			/* 0 for success, -1 for failure. */
			return 0;

		case QSE_AWK_RIO_CLOSE:
			/* 0 for success, -1 for failure. */
			return 0;

		case QSE_AWK_RIO_READ:
		{
			qse_ssize_t len = 0;

			while (conin[coninpos] && len < count) 
				data[len++] = conin[coninpos++];

			/* 0 for EOF, -1 for failure. 
			   positive numbers for the number of characters read */
			return len;
		}

		case QSE_AWK_RIO_WRITE:
			conoutpos += qse_strxncpy (
				&conout[conoutpos], QSE_COUNTOF(conout) - conoutpos, 
				data, count);
			/* 0 for EOF, -1 for failure. 
			   positive numbers for the number of characters written */
			return count;
		
		case QSE_AWK_RIO_FLUSH:
			/* 0 for success, -1 for failure. */
			return 0;

		case QSE_AWK_RIO_NEXT:
			/* 0 for success, -1 for failure. */
			return 0;
	}

	/* this part will never be reached */
	return -1;
}


int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_val_t* retv;
	qse_awk_parsestd_t psin[2];
	qse_awk_rio_t rio;
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
	
	/* retrieve the I/O handlers created by qse_awk_rtx_openstd() */
	qse_awk_rtx_getrio (rtx, &rio);
	/* override the console handler */
	rio.console = handle_console;
	/* update the I/O handlers */
	qse_awk_rtx_setrio (rtx, &rio);

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

	qse_printf (QSE_T("Console Output:\n================\n%.*s\n"), (int)conoutpos, conout);
	return ret;
}
