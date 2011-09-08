/*
 * $Id: err.c 562 2011-09-07 15:36:08Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include "sed.h"
#include "../cmn/mem.h"

const qse_char_t* qse_sed_dflerrstr (qse_sed_t* sed, qse_sed_errnum_t errnum)
{
	static const qse_char_t* errstr[] =
 	{
		QSE_T("no error"),
		QSE_T("insufficient memory"),
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

