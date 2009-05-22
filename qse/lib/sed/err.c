/*
 * $Id: err.c 113 2009-03-25 14:53:10Z baconevi $
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

static const qse_char_t* geterrstr (int errnum)
{
	static const qse_char_t* errstr[] =
 	{
		QSE_T("no error"),
		QSE_T("out of memory"),
		QSE_T("too much text"),
		QSE_T("command ${0} not recognized"),
		QSE_T("command code missing"),
		QSE_T("command ${0} not terminated properly"),
		QSE_T("regular expression build error"),
		QSE_T("regular expression match error"),
		QSE_T("address 1 prohibited"),
		QSE_T("address 2 prohibited"),
		QSE_T("invalid step address"),
		QSE_T("a new line expected"),
		QSE_T("a backslash expected"),
		QSE_T("a backslash used as a delimiter"),
		QSE_T("garbage after a backslash"),
		QSE_T("a semicolon expected"),
		QSE_T("empty label name"),
		QSE_T("duplicate label name ${0}"),
		QSE_T("label ${0} not found"),
		QSE_T("empty file name"),
		QSE_T("illegal file name"),
		QSE_T("strings in translation set not the same length"),
		QSE_T("group brackets not balanced"),
		QSE_T("group nesting too deep"),
		QSE_T("multiple occurrence specifier"),
		QSE_T("occurrence specifier is zero"),
		QSE_T("occurrence specifier too large"),
		QSE_T("error returned by user io handler")
	};

	return (errnum >= 0 && errnum < QSE_COUNTOF(errstr))?
		errstr[errnum]: QSE_T("unknown error");
}

#if 0
const qse_char_t* qse_sed_geterrstr (qse_sed_t* sed, qse_sed_errnum_t num)
{
	if (sed != QSE_NULL && 
	    sed->errstr[num] != QSE_NULL) return sed->errstr[num];
	return geterrstr (num);
}

int qse_sed_seterrstr (
	qse_sed_t* sed, qse_sed_errnum_t num, const qse_char_t* str)
{
	qse_char_t* dup;
       
	if (str == QSE_NULL) dup = QSE_NULL;
	else
	{
		dup = QSE_AWK_STRDUP (sed, str);
		if (dup == QSE_NULL) return -1;
	}

	if (sed->errstr[num] != QSE_NULL) 
		QSE_AWK_FREE (sed, sed->errstr[num]);
	else sed->errstr[num] = dup;
	return 0;
}
#endif

int qse_sed_geterrnum (qse_sed_t* sed)
{
	return sed->errnum;
}

qse_size_t qse_sed_geterrlin (qse_sed_t* sed)
{
	return sed->errlin;
}

const qse_char_t* qse_sed_geterrmsg (qse_sed_t* sed)
{
	if (sed->errmsg[0] == QSE_T('\0')) 
		/*return qse_sed_geterrstr (sed, sed->errnum);*/
		return geterrstr (sed->errnum);
	return sed->errmsg;
}

void qse_sed_geterror (
	qse_sed_t* sed, int* errnum, 
	qse_size_t* errlin, const qse_char_t** errmsg)
{
	if (errnum != QSE_NULL) *errnum = sed->errnum;
	if (errlin != QSE_NULL) *errlin = sed->errlin;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (sed->errmsg[0] == QSE_T('\0'))?
			/*qse_sed_geterrstr (sed, sed->errnum):*/
			geterrstr (sed->errnum):
			sed->errmsg;
	}
}

void qse_sed_seterrnum (qse_sed_t* sed, int errnum)
{
	sed->errnum = errnum;
	sed->errlin = 0;
	sed->errmsg[0] = QSE_T('\0');
}

void qse_sed_seterrmsg (qse_sed_t* sed, 
	int errnum, qse_size_t errlin, const qse_char_t* errmsg)
{
	sed->errnum = errnum;
	sed->errlin = errlin;
	qse_strxcpy (sed->errmsg, QSE_COUNTOF(sed->errmsg), errmsg);
}

void qse_sed_seterror (
	qse_sed_t* sed, int errnum,
	qse_size_t errlin, const qse_cstr_t* errarg)
{
	const qse_char_t* errfmt;

	sed->errnum = errnum;
	sed->errlin = errlin;

	errfmt = /*qse_sed_geterrstr (sed, errnum);*/ geterrstr (sed->errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (sed->errmsg, QSE_COUNTOF(sed->errmsg), errfmt, errarg);
}

