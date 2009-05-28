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

