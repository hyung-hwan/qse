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

#include <qse/awk/awk.h>
#include <qse/awk/stdawk.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/sio.h>

static const qse_char_t* src = QSE_T(
	"BEGIN {"
	"	for (i=2;i<=9;i++)"
	"	{"
	"		for (j=1;j<=9;j++)"
	"			print i \"*\" j \"=\" i * j;"
	"		print \"---------------------\";"
	"	}"
	"}"
);

int main ()
{
	qse_awk_t* awk = QSE_NULL;
	qse_awk_rtx_t* rtx = QSE_NULL;
	qse_awk_val_t* retv;

	qse_awk_parsestd_t psin[2];
	qse_awk_parsestd_t psout;

	int ret;

	qse_openstdsios ();

	awk = qse_awk_openstd (0, QSE_NULL);
	if (awk == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open awk\n"));
		ret = -1; goto oops;
	}

	psin[0].type = QSE_AWK_PARSESTD_STR;
	psin[0].u.str.ptr = src;
	psin[0].u.str.len = qse_strlen(src);
	psin[1].type = QSE_AWK_PARSESTD_NULL;

	psout.type = QSE_AWK_PARSESTD_STR;
	/* ps.out.u.str.ptr and ps.out.u.str.len are set when qse_awk_parsestd() 
	 * returns success */

	ret = qse_awk_parsestd (awk, psin, &psout);
	if (ret <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		ret = -1; goto oops;
	}

	qse_printf (QSE_T("DEPARSED SOURCE:\n%s\n"), psout.u.str.ptr);
	qse_printf (QSE_T("=================================\n"));
	qse_fflush (QSE_STDOUT);

	QSE_MMGR_FREE (qse_awk_getmmgr(awk), psout.u.str.ptr);

	rtx = qse_awk_rtx_openstd (
		awk, 
		0,
		QSE_T("awk02"),
		QSE_NULL,  /* stdin */
		QSE_NULL,  /* stdout */
		QSE_NULL   /* default cmgr */
	);
	if (rtx == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_geterrmsg(awk));
		ret = -1; goto oops;
	}
	
	retv = qse_awk_rtx_loop (rtx);
	if (retv == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), 
			qse_awk_rtx_geterrmsg(rtx));
		ret = -1; goto oops;
	}

	qse_awk_rtx_refdownval (rtx, retv);
	ret = 0;

oops:
	if (rtx != QSE_NULL) qse_awk_rtx_close (rtx);
	if (awk != QSE_NULL) qse_awk_close (awk);

	qse_closestdsios ();
	return -1;
}

