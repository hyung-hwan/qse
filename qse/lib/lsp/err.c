/*
 * $Id: err.c 337 2008-08-20 09:17:25Z baconevi $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include "lsp.h"

const qse_char_t* qse_lsp_dflerrstr (qse_lsp_t* lsp, qse_lsp_errnum_t errnum)
{
	static const qse_char_t* errstr[] = 
	{
		QSE_T("no error"),
		QSE_T("out of memory"),
		QSE_T("exit"),
		QSE_T("end of source"),
		QSE_T("unexpected end of string"),
		QSE_T("input not attached"),
		QSE_T("input"),
		QSE_T("output not attached"),
		QSE_T("output"),

		QSE_T("internal error"),
		QSE_T("syntax"),
		QSE_T("list too deep"),
		QSE_T("right parenthesis expected"),
		QSE_T("bad arguments"),
		QSE_T("too few arguments"),
		QSE_T("too many arguments"),
		QSE_T("undefined function '${0}'"),
		QSE_T("bad function"),
		QSE_T("duplicate formal"),
		QSE_T("bad symbol"),
		QSE_T("undefined symbol '${0}'"),
		QSE_T("empty body"),
		QSE_T("bad value"),
		QSE_T("divide by zero")
	};

	return (errnum >= 0 && errnum < QSE_COUNTOF(errstr))?
		errstr[errnum]: QSE_T("unknown error");
}

qse_lsp_errstr_t qse_lsp_geterrstr (qse_lsp_t* lsp)
{
	return lsp->errstr;
}

void qse_lsp_seterrstr (qse_lsp_t* lsp, qse_lsp_errstr_t errstr)
{
	lsp->errstr = errstr;
}

qse_lsp_errnum_t qse_lsp_geterrnum (qse_lsp_t* lsp)
{
	return lsp->errnum;
}

const qse_lsp_loc_t* qse_lsp_geterrloc (qse_lsp_t* lsp)
{
	return &lsp->errloc;
}

const qse_char_t* qse_lsp_geterrmsg (qse_lsp_t* lsp)
{
	return (lsp->errmsg[0] == QSE_T('\0'))?
		qse_lsp_geterrstr(lsp)(lsp,lsp->errnum): lsp->errmsg;
}

void qse_lsp_geterror (
	qse_lsp_t* lsp, qse_lsp_errnum_t* errnum, 
	const qse_char_t** errmsg, qse_lsp_loc_t* errloc)
{
	if (errnum != QSE_NULL) *errnum = lsp->errnum;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (lsp->errmsg[0] == QSE_T('\0'))?
			qse_lsp_geterrstr(lsp)(lsp,lsp->errnum):
			lsp->errmsg;
	}
	if (errloc != QSE_NULL) *errloc = lsp->errloc;
}

void qse_lsp_seterrnum (
	qse_lsp_t* lsp, qse_lsp_errnum_t errnum, const qse_cstr_t* errarg)
{
	qse_lsp_seterror (lsp, errnum, errarg, QSE_NULL);
}

void qse_lsp_seterrmsg (
	qse_lsp_t* lsp, qse_lsp_errnum_t errnum,
	const qse_char_t* errmsg, const qse_lsp_loc_t* errloc)
{
	lsp->errnum = errnum;
	qse_strxcpy (lsp->errmsg, QSE_COUNTOF(lsp->errmsg), errmsg);
	if (errloc != QSE_NULL) lsp->errloc = *errloc;
	else QSE_MEMSET (&lsp->errloc, 0, QSE_SIZEOF(lsp->errloc));
}

void qse_lsp_seterror (
	qse_lsp_t* lsp, qse_lsp_errnum_t errnum,
	const qse_cstr_t* errarg, const qse_lsp_loc_t* errloc)
{
	const qse_char_t* errfmt;

	lsp->errnum = errnum;

	errfmt = qse_lsp_geterrstr(lsp)(lsp,lsp->errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (lsp->errmsg, QSE_COUNTOF(lsp->errmsg), errfmt, errarg);

	if (errloc != QSE_NULL) lsp->errloc = *errloc;
	else QSE_MEMSET (&lsp->errloc, 0, QSE_SIZEOF(lsp->errloc));
}

