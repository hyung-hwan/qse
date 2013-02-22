/*
 * $Id: awk01.c 441 2011-04-22 14:28:43Z hyunghwan.chung $ 
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/awk/stdawk.h>
#include <qse/cmn/stdio.h>

const qse_char_t* src = QSE_T("BEGIN { print \"hello, world\" | \"dir\"; }");

struct rtx_xtn_t
{
	qse_awk_rio_impl_t old_pipe_handler;
};

static qse_ssize_t new_pipe_handler (
	qse_awk_rtx_t* rtx, qse_awk_rio_cmd_t cmd, qse_awk_rio_arg_t* riod,
	qse_char_t* data, qse_size_t size)
{
	struct rtx_xtn_t* xtn;
	xtn = qse_awk_rtx_getxtnstd (rtx);

	if (cmd == QSE_AWK_RIO_OPEN)
		qse_fprintf (QSE_STDERR, QSE_T("LOG: Executing [%s] for piping\n"), riod->name);

	return xtn->old_pipe_handler (rtx, cmd, riod, data, size);
}

static void extend_pipe_handler (qse_awk_rtx_t* rtx)
{
	/* this function simply demonstrates how to extend
	 * runtime I/O handlers provided by qse_awk_rtx_openstd() */

	struct rtx_xtn_t* xtn;
	qse_awk_rio_t rio;

	xtn = qse_awk_rtx_getxtnstd (rtx);

	/* get the previous handler functions */
	qse_awk_rtx_getrio (rtx, &rio); 

	/* remember the old pipe handler function */
	xtn->old_pipe_handler = rio.pipe;

	/* change the pipe handler to a new one */
	rio.pipe = new_pipe_handler;

	/* changes the handlers with a new set */
	qse_awk_rtx_setrio (rtx, &rio);
}

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_val_t* retv;
	qse_awk_parsestd_t psin[2];
	int ret = -1;

	awk = qse_awk_openstd (0);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		goto oops;
	}

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
		QSE_SIZEOF(struct rtx_xtn_t),
		QSE_T("awk11"),
		QSE_NULL, /* stdin */
		QSE_NULL,  /* stdout */               
		QSE_NULL  /* default cmgr */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		goto oops;
	}

	extend_pipe_handler (rtx);
	
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
	return ret;
}

