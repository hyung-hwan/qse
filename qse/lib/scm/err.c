/*
 * $Id$
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

#include "scm.h"

const qse_char_t* qse_scm_dflerrstr (qse_scm_t* scm, qse_scm_errnum_t errnum)
{
	static const qse_char_t* errstr[] = 
	{
		QSE_T("no error"),
		QSE_T("out of memory"),

		QSE_T("exit"),
		QSE_T("end of source"),

		QSE_T("I/O error"),
		QSE_T("unexpected end of string"),

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

qse_scm_errstr_t qse_scm_geterrstr (qse_scm_t* scm)
{
	return scm->err.str;
}

void qse_scm_seterrstr (qse_scm_t* scm, qse_scm_errstr_t errstr)
{
	scm->err.str = errstr;
}

qse_scm_errnum_t qse_scm_geterrnum (qse_scm_t* scm)
{
	return scm->err.num;
}

const qse_scm_loc_t* qse_scm_geterrloc (qse_scm_t* scm)
{
	return &scm->err.loc;
}

const qse_char_t* qse_scm_geterrmsg (qse_scm_t* scm)
{
	return (scm->err.msg[0] == QSE_T('\0'))?
		qse_scm_geterrstr(scm)(scm,scm->err.num): scm->err.msg;
}

void qse_scm_geterror (
	qse_scm_t* scm, qse_scm_errnum_t* errnum, 
	const qse_char_t** errmsg, qse_scm_loc_t* errloc)
{
	if (errnum != QSE_NULL) *errnum = scm->err.num;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (scm->err.msg[0] == QSE_T('\0'))?
			qse_scm_geterrstr(scm)(scm,scm->err.num):
			scm->err.msg;
	}
	if (errloc != QSE_NULL) *errloc = scm->err.loc;
}

void qse_scm_seterrnum (
	qse_scm_t* scm, qse_scm_errnum_t errnum, const qse_cstr_t* errarg)
{
	qse_scm_seterror (scm, errnum, errarg, QSE_NULL);
}

void qse_scm_seterrmsg (
	qse_scm_t* scm, qse_scm_errnum_t errnum,
	const qse_char_t* errmsg, const qse_scm_loc_t* errloc)
{
	scm->err.num = errnum;
	qse_strxcpy (scm->err.msg, QSE_COUNTOF(scm->err.msg), errmsg);
	if (errloc != QSE_NULL) scm->err.loc = *errloc;
	else QSE_MEMSET (&scm->err.loc, 0, QSE_SIZEOF(scm->err.loc));
}

void qse_scm_seterror (
	qse_scm_t* scm, qse_scm_errnum_t errnum,
	const qse_cstr_t* errarg, const qse_scm_loc_t* errloc)
{
	const qse_char_t* errfmt;

	scm->err.num = errnum;

	errfmt = qse_scm_geterrstr(scm)(scm,scm->err.num);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (scm->err.msg, QSE_COUNTOF(scm->err.msg), errfmt, errarg);

	if (errloc != QSE_NULL) scm->err.loc = *errloc;
	else QSE_MEMSET (&scm->err.loc, 0, QSE_SIZEOF(scm->err.loc));
}

