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

#include "xli.h"
#include "../cmn/mem.h"

const qse_char_t* qse_xli_dflerrstr (
	const qse_xli_t* xli, qse_xli_errnum_t errnum)
{
	static const qse_char_t* errstr[] =
 	{
		QSE_T("no error"),
		QSE_T("other error"),
		QSE_T("not implemented"),
		QSE_T("subsystem error"),
		QSE_T("internal error that should never have happened"),

		QSE_T("insufficient memory"),
		QSE_T("invalid parameter or data"),
		QSE_T("'${0}' not found"),
		QSE_T("'${0}' already exists"),
		QSE_T("I/O error with file '${0}'"),
		QSE_T("error returned by user I/O handler"),

		QSE_T("syntax error"),
		QSE_T("semicolon expected in place of '${0}'"),
		QSE_T("left-brace or equal-sign expected in place of '${0}'"),
		QSE_T("right-brace expected in place of '${0}'"),
		QSE_T("pair value expected in place of '${0}'"),
		QSE_T("string not closed"),
		QSE_T("string tag not closed"),
		QSE_T("'@include' not followed by a string"),
		QSE_T("invalid character '${0}'"),
		QSE_T("invalid tag character '${0}'"),
		QSE_T("'${0}' not recognized"),
		QSE_T("@ not followed by a valid word"),
		QSE_T("invalid identifier '${0}'"),
		QSE_T("missing key after key tag"),
		QSE_T("undefined key '${0}'"),
		QSE_T("no alias for '${0}'"),
		QSE_T("illegal value for '${0}'"),
		QSE_T("no value for '${0}'"),
		QSE_T("uncomplying number of string segments for '${0}'")
	};

	return (errnum >= 0 && errnum < QSE_COUNTOF(errstr))?
		errstr[errnum]: QSE_T("unknown error");
}

qse_xli_errstr_t qse_xli_geterrstr (const qse_xli_t* xli)
{
	return xli->errstr;
}

void qse_xli_seterrstr (qse_xli_t* xli, qse_xli_errstr_t errstr)
{
	xli->errstr = errstr;
}

qse_xli_errnum_t qse_xli_geterrnum (const qse_xli_t* xli)
{
	return xli->errnum;
}

const qse_xli_loc_t* qse_xli_geterrloc (const qse_xli_t* xli)
{
	return &xli->errloc;
}

const qse_char_t* qse_xli_geterrmsg (const qse_xli_t* xli)
{
	return (xli->errmsg[0] == QSE_T('\0'))?
		qse_xli_geterrstr(xli)(xli,xli->errnum): xli->errmsg;
}

void qse_xli_geterror (
	const qse_xli_t* xli, qse_xli_errnum_t* errnum, 
	const qse_char_t** errmsg, qse_xli_loc_t* errloc)
{
	if (errnum != QSE_NULL) *errnum = xli->errnum;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (xli->errmsg[0] == QSE_T('\0'))?
			qse_xli_geterrstr(xli)(xli,xli->errnum):
			xli->errmsg;
	}
	if (errloc != QSE_NULL) *errloc = xli->errloc;
}

void qse_xli_seterrnum (
	qse_xli_t* xli, qse_xli_errnum_t errnum, const qse_cstr_t* errarg)
{
	qse_xli_seterror (xli, errnum, errarg, QSE_NULL);
}

void qse_xli_seterrmsg (
	qse_xli_t* xli, qse_xli_errnum_t errnum,
	const qse_char_t* errmsg, const qse_xli_loc_t* errloc)
{
	xli->errnum = errnum;
	qse_strxcpy (xli->errmsg, QSE_COUNTOF(xli->errmsg), errmsg);
	if (errloc != QSE_NULL) xli->errloc = *errloc;
	else QSE_MEMSET (&xli->errloc, 0, QSE_SIZEOF(xli->errloc));
}

void qse_xli_seterror (
	qse_xli_t* xli, qse_xli_errnum_t errnum,
	const qse_cstr_t* errarg, const qse_xli_loc_t* errloc)
{
	const qse_char_t* errfmt;

	xli->errnum = errnum;

	errfmt = qse_xli_geterrstr(xli)(xli,xli->errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (xli->errmsg, QSE_COUNTOF(xli->errmsg), errfmt, errarg);

	if (errloc != QSE_NULL) xli->errloc = *errloc;
	else QSE_MEMSET (&xli->errloc, 0, QSE_SIZEOF(xli->errloc));
}

