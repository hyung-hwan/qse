/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/awk/stdawk.h>
#include <qse/cmn/sio.h>

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

	qse_openstdsios ();

	awk = qse_awk_openstd (0, QSE_NULL);
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

	qse_closestdsios ();
	return ret;
}

