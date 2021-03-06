/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#include "sed-prv.h"
#include "../cmn/mem-prv.h"

const qse_char_t* qse_sed_dflerrstr (qse_sed_t* sed, qse_sed_errnum_t errnum)
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
		QSE_T("command '${0}' not recognized"),
		QSE_T("command code missing"),
		QSE_T("command '${0}' incomplete"),
		QSE_T("regular expression '${0}' incomplete"),
		QSE_T("failed to compile regular expression '${0}'"),
		QSE_T("failed to match regular expression"),
		QSE_T("address 1 prohibited for '${0}'"),
		QSE_T("address 1 missing or invalid"),
		QSE_T("address 2 prohibited for '${0}'"),
		QSE_T("address 2 missing or invalid"),
		QSE_T("newline expected"),
		QSE_T("backslash expected"),
		QSE_T("backslash used as delimiter"),
		QSE_T("garbage after backslash"),
		QSE_T("semicolon expected"),
		QSE_T("empty label name"),
		QSE_T("duplicate label name '${0}'"),
		QSE_T("label '${0}' not found"),
		QSE_T("empty file name"),
		QSE_T("illegal file name"),
		QSE_T("strings in translation set not the same length"),
		QSE_T("group brackets not balanced"),
		QSE_T("group nesting too deep"),
		QSE_T("multiple occurrence specifiers"),
		QSE_T("occurrence specifier zero"),
		QSE_T("occurrence specifier too large"),
		QSE_T("no previous regular expression"),
		QSE_T("cut selector not valid"),
		QSE_T("I/O error with file '${0}'"),
		QSE_T("error returned by user I/O handler")
	};

	return (errnum >= 0 && errnum < QSE_COUNTOF(errstr))?
		errstr[errnum]: QSE_T("unknown error");
}

qse_sed_errstr_t qse_sed_geterrstr (qse_sed_t* sed)
{
	return sed->errstr;
}

void qse_sed_seterrstr (qse_sed_t* sed, qse_sed_errstr_t errstr)
{
	sed->errstr = errstr;
}

qse_sed_errnum_t qse_sed_geterrnum (qse_sed_t* sed)
{
	return sed->errnum;
}

const qse_sed_loc_t* qse_sed_geterrloc (qse_sed_t* sed)
{
	return &sed->errloc;
}

const qse_char_t* qse_sed_geterrmsg (qse_sed_t* sed)
{
	return (sed->errmsg[0] == QSE_T('\0'))?
		qse_sed_geterrstr(sed)(sed,sed->errnum): sed->errmsg;
}

void qse_sed_geterror (
	qse_sed_t* sed, qse_sed_errnum_t* errnum, 
	const qse_char_t** errmsg, qse_sed_loc_t* errloc)
{
	if (errnum != QSE_NULL) *errnum = sed->errnum;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (sed->errmsg[0] == QSE_T('\0'))?
			qse_sed_geterrstr(sed)(sed,sed->errnum):
			sed->errmsg;
	}
	if (errloc != QSE_NULL) *errloc = sed->errloc;
}

void qse_sed_seterrnum (
	qse_sed_t* sed, qse_sed_errnum_t errnum, const qse_cstr_t* errarg)
{
	qse_sed_seterror (sed, errnum, errarg, QSE_NULL);
}

void qse_sed_seterrmsg (
	qse_sed_t* sed, qse_sed_errnum_t errnum,
	const qse_char_t* errmsg, const qse_sed_loc_t* errloc)
{
	sed->errnum = errnum;
	qse_strxcpy (sed->errmsg, QSE_COUNTOF(sed->errmsg), errmsg);
	if (errloc != QSE_NULL) sed->errloc = *errloc;
	else QSE_MEMSET (&sed->errloc, 0, QSE_SIZEOF(sed->errloc));
}

void qse_sed_seterror (
	qse_sed_t* sed, qse_sed_errnum_t errnum,
	const qse_cstr_t* errarg, const qse_sed_loc_t* errloc)
{
	const qse_char_t* errfmt;

	sed->errnum = errnum;

	errfmt = qse_sed_geterrstr(sed)(sed,sed->errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (sed->errmsg, QSE_COUNTOF(sed->errmsg), errfmt, errarg);

	if (errloc != QSE_NULL) sed->errloc = *errloc;
	else QSE_MEMSET (&sed->errloc, 0, QSE_SIZEOF(sed->errloc));
}

