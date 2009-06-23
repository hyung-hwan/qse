/*
 * $Id: err.c 207 2009-06-22 13:01:28Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "sed.h"

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
		QSE_T("io error with file '${0}'"),
		QSE_T("error returned by user io handler")
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

qse_size_t qse_sed_geterrlin (qse_sed_t* sed)
{
	return sed->errlin;
}

const qse_char_t* qse_sed_geterrmsg (qse_sed_t* sed)
{
	return (sed->errmsg[0] == QSE_T('\0'))?
		qse_sed_geterrstr(sed)(sed,sed->errnum): sed->errmsg;
}

void qse_sed_geterror (
	qse_sed_t* sed, qse_sed_errnum_t* errnum, 
	qse_size_t* errlin, const qse_char_t** errmsg)
{
	if (errnum != QSE_NULL) *errnum = sed->errnum;
	if (errlin != QSE_NULL) *errlin = sed->errlin;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (sed->errmsg[0] == QSE_T('\0'))?
			qse_sed_geterrstr(sed)(sed,sed->errnum):
			sed->errmsg;
	}
}

void qse_sed_seterrnum (qse_sed_t* sed, qse_sed_errnum_t errnum)
{
	sed->errnum = errnum;
	sed->errlin = 0;
	sed->errmsg[0] = QSE_T('\0');
}

void qse_sed_seterrmsg (
	qse_sed_t* sed, qse_sed_errnum_t errnum,
	qse_size_t errlin, const qse_char_t* errmsg)
{
	sed->errnum = errnum;
	sed->errlin = errlin;
	qse_strxcpy (sed->errmsg, QSE_COUNTOF(sed->errmsg), errmsg);
}

void qse_sed_seterror (
	qse_sed_t* sed, qse_sed_errnum_t errnum,
	qse_size_t errlin, const qse_cstr_t* errarg)
{
	const qse_char_t* errfmt;

	sed->errnum = errnum;
	sed->errlin = errlin;

	errfmt = qse_sed_geterrstr(sed)(sed,sed->errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (sed->errmsg, QSE_COUNTOF(sed->errmsg), errfmt, errarg);
}

