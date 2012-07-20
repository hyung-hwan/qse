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

#include "stx.h"
#include "../cmn/mem.h"
#include <qse/cmn/str.h>

const qse_char_t* qse_stx_dflerrstr (qse_stx_t* stx, qse_stx_errnum_t errnum)
{
	static const qse_char_t* errstr[] = 
	{
		QSE_T("no error"),

		QSE_T("out of memory"),
		QSE_T("internal error"),

		QSE_T("exit"),
		QSE_T("end of source"),

		QSE_T("I/O error"),
		QSE_T("unexpected end of string"),
		QSE_T("bad sharp expression"),
		QSE_T("wrong use of dot"),
		QSE_T("left parenthesis expected"),
		QSE_T("right parenthesis expected"),
		QSE_T("list too deep"),

		QSE_T("bad variable"),
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

qse_stx_errstr_t qse_stx_geterrstr (qse_stx_t* stx)
{
	return stx->err.str;
}

void qse_stx_seterrstr (qse_stx_t* stx, qse_stx_errstr_t errstr)
{
	stx->err.str = errstr;
}

qse_stx_errnum_t qse_stx_geterrnum (qse_stx_t* stx)
{
	return stx->err.num;
}

const qse_stx_loc_t* qse_stx_geterrloc (qse_stx_t* stx)
{
	return &stx->err.loc;
}

const qse_char_t* qse_stx_geterrmsg (qse_stx_t* stx)
{
	return (stx->err.msg[0] == QSE_T('\0'))?
		qse_stx_geterrstr(stx)(stx,stx->err.num): stx->err.msg;
}

void qse_stx_geterror (
	qse_stx_t* stx, qse_stx_errnum_t* errnum, 
	const qse_char_t** errmsg, qse_stx_loc_t* errloc)
{
	if (errnum != QSE_NULL) *errnum = stx->err.num;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (stx->err.msg[0] == QSE_T('\0'))?
			qse_stx_geterrstr(stx)(stx,stx->err.num):
			stx->err.msg;
	}
	if (errloc != QSE_NULL) *errloc = stx->err.loc;
}

void qse_stx_seterrnum (
	qse_stx_t* stx, qse_stx_errnum_t errnum, const qse_cstr_t* errarg)
{
	qse_stx_seterror (stx, errnum, errarg, QSE_NULL);
}

void qse_stx_seterrmsg (
	qse_stx_t* stx, qse_stx_errnum_t errnum,
	const qse_char_t* errmsg, const qse_stx_loc_t* errloc)
{
	stx->err.num = errnum;
	qse_strxcpy (stx->err.msg, QSE_COUNTOF(stx->err.msg), errmsg);
	if (errloc != QSE_NULL) stx->err.loc = *errloc;
	else QSE_MEMSET (&stx->err.loc, 0, QSE_SIZEOF(stx->err.loc));
}

void qse_stx_seterror (
	qse_stx_t* stx, qse_stx_errnum_t errnum,
	const qse_cstr_t* errarg, const qse_stx_loc_t* errloc)
{
	const qse_char_t* errfmt;

	stx->err.num = errnum;

	errfmt = qse_stx_geterrstr(stx)(stx,stx->err.num);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (stx->err.msg, QSE_COUNTOF(stx->err.msg), errfmt, errarg);

	if (errloc != QSE_NULL) stx->err.loc = *errloc;
	else QSE_MEMSET (&stx->err.loc, 0, QSE_SIZEOF(stx->err.loc));
}

