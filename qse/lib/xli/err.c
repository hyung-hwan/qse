/*
 * $Id$
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

		QSE_T("semicolon expected in place of '${0}'"),
		QSE_T("left-brace or equal-sign expected in place of '${0}'"),
		QSE_T("right-brace expected in place of '${0}'"),
		QSE_T("pair value expected in place of '${0}'")
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

