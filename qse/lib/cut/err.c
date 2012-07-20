/*
 * $Id: err.c 287 2009-09-15 10:01:02Z baconevi $
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

#include "cut.h"
#include "../cmn/mem.h"

const qse_char_t* qse_cut_dflerrstr (qse_cut_t* cut, qse_cut_errnum_t errnum)
{
	static const qse_char_t* errstr[] =
 	{
		QSE_T("no error"),
		QSE_T("insufficient memory"),
		QSE_T("invalid parameter or data"),
		QSE_T("selector not valid"),
		QSE_T("io error with file '${0}'"),
		QSE_T("error returned by user io handler")
	};

	return (errnum >= 0 && errnum < QSE_COUNTOF(errstr))?
		errstr[errnum]: QSE_T("unknown error");
}

qse_cut_errstr_t qse_cut_geterrstr (qse_cut_t* cut)
{
	return cut->errstr;
}

void qse_cut_seterrstr (qse_cut_t* cut, qse_cut_errstr_t errstr)
{
	cut->errstr = errstr;
}

qse_cut_errnum_t qse_cut_geterrnum (qse_cut_t* cut)
{
	return cut->errnum;
}

const qse_char_t* qse_cut_geterrmsg (qse_cut_t* cut)
{
	return (cut->errmsg[0] == QSE_T('\0'))?
		qse_cut_geterrstr(cut)(cut,cut->errnum): cut->errmsg;
}

void qse_cut_geterror (
	qse_cut_t* cut, qse_cut_errnum_t* errnum, const qse_char_t** errmsg)
{
	if (errnum != QSE_NULL) *errnum = cut->errnum;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (cut->errmsg[0] == QSE_T('\0'))?
			qse_cut_geterrstr(cut)(cut,cut->errnum):
			cut->errmsg;
	}
}

void qse_cut_seterrnum (
	qse_cut_t* cut, qse_cut_errnum_t errnum, const qse_cstr_t* errarg)
{
	qse_cut_seterror (cut, errnum, errarg);
}

void qse_cut_seterrmsg (
	qse_cut_t* cut, qse_cut_errnum_t errnum, const qse_char_t* errmsg)
{
	cut->errnum = errnum;
	qse_strxcpy (cut->errmsg, QSE_COUNTOF(cut->errmsg), errmsg);
}

void qse_cut_seterror (
	qse_cut_t* cut, qse_cut_errnum_t errnum, const qse_cstr_t* errarg)
{
	const qse_char_t* errfmt;

	cut->errnum = errnum;

	errfmt = qse_cut_geterrstr(cut)(cut,cut->errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (cut->errmsg, QSE_COUNTOF(cut->errmsg), errfmt, errarg);
}

