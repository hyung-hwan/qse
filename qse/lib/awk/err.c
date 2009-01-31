/*
 * $Id: err.c 337 2008-08-20 09:17:25Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

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

#include "awk.h"

static const qse_char_t* __geterrstr (int errnum)
{
	static const qse_char_t* errstr[] =
 	{
		QSE_T("no error"),
		QSE_T("custom error"),

		QSE_T("invalid parameter or data"),
		QSE_T("out of memory"),
		QSE_T("not supported"),
		QSE_T("operation not allowed"),
		QSE_T("no such device"),
		QSE_T("no space left on device"),
		QSE_T("too many open files"),
		QSE_T("too many links"),
		QSE_T("resource temporarily unavailable"),
		QSE_T("'%.*s' not existing"),
		QSE_T("'%.*s' already exists"),
		QSE_T("file or data too big"),
		QSE_T("system too busy"),
		QSE_T("is a directory"),
		QSE_T("i/o error"),

		QSE_T("cannot open '%.*s'"),
		QSE_T("cannot read '%.*s'"),
		QSE_T("cannot write '%.*s'"),
		QSE_T("cannot close '%.*s'"),
		
		QSE_T("internal error that should never have happened"),
		QSE_T("general run-time error"),
		QSE_T("block nested too deeply"),
		QSE_T("expressio nested too deeply"),

		QSE_T("cannot open source input"),
		QSE_T("cannot close source input"),
		QSE_T("cannot read source input"),

		QSE_T("cannot open source output"),
		QSE_T("cannot close source output"),
		QSE_T("cannot write source output"),

		QSE_T("invalid character '%.*s'"),
		QSE_T("invalid digit '%.*s'"),
		QSE_T("cannot unget character"),

		QSE_T("unexpected end of source"),
		QSE_T("a comment not closed properly"),
		QSE_T("a string not closed with a quote"),
		QSE_T("unexpected end of a regular expression"),
		QSE_T("a left brace expected in place of '%.*s'"),
		QSE_T("a left parenthesis expected in place of '%.*s'"),
		QSE_T("a right parenthesis expected in place of '%.*s'"),
		QSE_T("a right bracket expected in place of '%.*s'"),
		QSE_T("a comma expected in place of '%.*s'"),
		QSE_T("a semicolon expected in place of '%.*s'"),
		QSE_T("a colon expected in place of '%.*s'"),
		QSE_T("statement not ending with a semicolon"),
		QSE_T("'in' expected in place of '%.*s'"),
		QSE_T("right-hand side of the 'in' operator not a variable"),
		QSE_T("invalid expression"),

		QSE_T("keyword 'function' expected in place of '%.*s'"),
		QSE_T("keyword 'while' expected in place of '%.*s'"),
		QSE_T("invalid assignment statement"),
		QSE_T("an identifier expected in place of '%.*s'"),
		QSE_T("'%.*s' not a valid function name"),
		QSE_T("BEGIN not followed by a left bracket on the same line"),
		QSE_T("END not followed by a left bracket on the same line"),
		QSE_T("duplicate BEGIN"),
		QSE_T("duplicate END"),
		QSE_T("intrinsic function '%.*s' redefined"),
		QSE_T("function '%.*s' redefined"),
		QSE_T("global variable '%.*s' redefined"),
		QSE_T("parameter '%.*s' redefined"),
		QSE_T("variable '%.*s' redefined"),
		QSE_T("duplicate parameter name '%.*s'"),
		QSE_T("duplicate global variable '%.*s'"),
		QSE_T("duplicate local variable '%.*s'"),
		QSE_T("'%.*s' not a valid parameter name"),
		QSE_T("'%.*s' not a valid variable name"),
		QSE_T("undefined identifier '%.*s'"),
		QSE_T("l-value required"),
		QSE_T("too many global variables"),
		QSE_T("too many local variables"),
		QSE_T("too many parameters"),
		QSE_T("delete statement not followed by a normal variable"),
		QSE_T("reset statement not followed by a normal variable"),
		QSE_T("break statement outside a loop"),
		QSE_T("continue statement outside a loop"),
		QSE_T("next statement illegal in the BEGIN block"),
		QSE_T("next statement illegal in the END block"),
		QSE_T("nextfile statement illegal in the BEGIN block"),
		QSE_T("nextfile statement illegal in the END block"),
		QSE_T("printf not followed by any arguments"),
		QSE_T("both prefix and postfix increment/decrement operator present"),

		QSE_T("divide by zero"),
		QSE_T("invalid operand"),
		QSE_T("wrong position index"),
		QSE_T("too few arguments"),
		QSE_T("too many arguments"),
		QSE_T("function '%.*s' not found"),
		QSE_T("variable not indexable"),
		QSE_T("variable '%.*s' not deletable"),
		QSE_T("value not a map"),
		QSE_T("right-hand side of the 'in' operator not a map"),
		QSE_T("right-hand side of the 'in' operator not a map nor nil"),
		QSE_T("value not referenceable"),
		QSE_T("value not assignable"),
		QSE_T("an indexed value cannot be assigned a map"),
		QSE_T("a positional value cannot be assigned a map"),
		QSE_T("map '%.*s' not assignable with a scalar"),
		QSE_T("cannot change a scalar value to a map"),
		QSE_T("a map is not allowed"),
		QSE_T("invalid value type"),
		QSE_T("delete statement called with a wrong target"),
		QSE_T("reset statement called with a wrong target"),
		QSE_T("next statement called from the BEGIN block"),
		QSE_T("next statement called from the END block"),
		QSE_T("nextfile statement called from the BEGIN block"),
		QSE_T("nextfile statement called from the END block"),
		QSE_T("wrong implementation of intrinsic function handler"),
		QSE_T("intrinsic function handler returned an error"),
		QSE_T("wrong implementation of user-defined io handler"),
		QSE_T("no such io name found"),
		QSE_T("i/o handler returned an error"),
		QSE_T("i/o name empty"),
		QSE_T("i/o name containing a null character"),
		QSE_T("not sufficient arguments to formatting sequence"),
		QSE_T("recursion detected in format conversion"),
		QSE_T("invalid character in CONVFMT"),
		QSE_T("invalid character in OFMT"),

		QSE_T("recursion too deep in the regular expression"),
		QSE_T("a right parenthesis expected in the regular expression"),
		QSE_T("a right bracket expected in the regular expression"),
		QSE_T("a right brace expected in the regular expression"),
		QSE_T("unbalanced parenthesis in the regular expression"),
		QSE_T("a colon expected in the regular expression"),
		QSE_T("invalid character range in the regular expression"),
		QSE_T("invalid character class in the regular expression"),
		QSE_T("invalid boundary range in the regular expression"),
		QSE_T("unexpected end of the regular expression"),
		QSE_T("garbage after the regular expression")
	};

	if (errnum >= 0 && errnum < QSE_COUNTOF(errstr)) 
	{
		return errstr[errnum];
	}

	return QSE_T("unknown error");
}

const qse_char_t* qse_awk_geterrstr (qse_awk_t* awk, int num)
{
	if (awk != QSE_NULL && 
	    awk->errstr[num] != QSE_NULL) return awk->errstr[num];
	return __geterrstr (num);
}

int qse_awk_seterrstr (qse_awk_t* awk, int num, const qse_char_t* str)
{
	qse_char_t* dup;
       
	if (str == QSE_NULL) dup = QSE_NULL;
	else
	{
		dup = QSE_AWK_STRDUP (awk, str);
		if (dup == QSE_NULL) return -1;
	}

	if (awk->errstr[num] != QSE_NULL) 
		QSE_AWK_FREE (awk, awk->errstr[num]);
	else awk->errstr[num] = dup;
	return 0;
}

int qse_awk_geterrnum (qse_awk_t* awk)
{
	return awk->errnum;
}

qse_size_t qse_awk_geterrlin (qse_awk_t* awk)
{
	return awk->errlin;
}

const qse_char_t* qse_awk_geterrmsg (qse_awk_t* awk)
{
	if (awk->errmsg[0] == QSE_T('\0')) 
		return qse_awk_geterrstr (awk, awk->errnum);
	return awk->errmsg;
}

void qse_awk_geterror (
	qse_awk_t* awk, int* errnum, 
	qse_size_t* errlin, const qse_char_t** errmsg)
{
	if (errnum != QSE_NULL) *errnum = awk->errnum;
	if (errlin != QSE_NULL) *errlin = awk->errlin;
	if (errmsg != QSE_NULL) 
	{
		if (awk->errmsg[0] == QSE_T('\0'))
			*errmsg = qse_awk_geterrstr (awk, awk->errnum);
		else
			*errmsg = awk->errmsg;
	}
}

void qse_awk_seterrnum (qse_awk_t* awk, int errnum)
{
	awk->errnum = errnum;
	awk->errlin = 0;
	awk->errmsg[0] = QSE_T('\0');
}

void qse_awk_seterrmsg (qse_awk_t* awk, 
	int errnum, qse_size_t errlin, const qse_char_t* errmsg)
{
	awk->errnum = errnum;
	awk->errlin = errlin;
	qse_strxcpy (awk->errmsg, QSE_COUNTOF(awk->errmsg), errmsg);
}

void qse_awk_seterror (
	qse_awk_t* awk, int errnum, qse_size_t errlin,
	const qse_cstr_t* errarg, qse_size_t argcnt)
{
	const qse_char_t* errfmt;
	qse_size_t fmtlen;

	QSE_ASSERT (argcnt <= 5);

	awk->errnum = errnum;
	awk->errlin = errlin;

	errfmt = qse_awk_geterrstr (awk, errnum);
	fmtlen = qse_strlen(errfmt);

	switch (argcnt)
	{
		case 0:
			awk->prmfns->sprintf (
				awk->prmfns->data,
				awk->errmsg, 
				QSE_COUNTOF(awk->errmsg), 
				errfmt);
			return;

		case 1:
		{
			qse_char_t tmp[QSE_COUNTOF(awk->errmsg)];
			qse_size_t len, tl;

			if (fmtlen < QSE_COUNTOF(awk->errmsg) &&
			    errarg[0].len + fmtlen >= QSE_COUNTOF(awk->errmsg))
			{
				len = QSE_COUNTOF(awk->errmsg) - fmtlen - 3 - 1;
				tl = qse_strxncpy (tmp, QSE_COUNTOF(tmp), errarg[0].ptr, len);
				tmp[tl] = QSE_T('.');
				tmp[tl+1] = QSE_T('.');
				tmp[tl+2] = QSE_T('.');
				len += 3;
			}
			else 
			{
				len = errarg[0].len;
				qse_strxncpy (tmp, QSE_COUNTOF(tmp), errarg[0].ptr, len);
			}

			awk->prmfns->sprintf (
				awk->prmfns->data,
				awk->errmsg, 
				QSE_COUNTOF(awk->errmsg), 
				errfmt, (int)len, tmp);
			return;
		}

		case 2:
			awk->prmfns->sprintf (
				awk->prmfns->data,
				awk->errmsg, 
				QSE_COUNTOF(awk->errmsg), 
				errfmt,
				(int)errarg[0].len, errarg[0].ptr,
				(int)errarg[1].len, errarg[1].ptr);
			return;

		case 3:
			awk->prmfns->sprintf (
				awk->prmfns->data,
				awk->errmsg, 
				QSE_COUNTOF(awk->errmsg), 
				errfmt,
				(int)errarg[0].len, errarg[0].ptr,
				(int)errarg[1].len, errarg[1].ptr,
				(int)errarg[2].len, errarg[2].ptr);
			return;

		case 4:
			awk->prmfns->sprintf (
				awk->prmfns->data,
				awk->errmsg, 
				QSE_COUNTOF(awk->errmsg), 
				errfmt,
				(int)errarg[0].len, errarg[0].ptr,
				(int)errarg[1].len, errarg[1].ptr,
				(int)errarg[2].len, errarg[2].ptr,
				(int)errarg[3].len, errarg[3].ptr);
			return;

		case 5:
			awk->prmfns->sprintf (
				awk->prmfns->data,
				awk->errmsg, 
				QSE_COUNTOF(awk->errmsg), 
				errfmt,
				(int)errarg[0].len, errarg[0].ptr,
				(int)errarg[1].len, errarg[1].ptr,
				(int)errarg[2].len, errarg[2].ptr,
				(int)errarg[3].len, errarg[3].ptr,
				(int)errarg[4].len, errarg[4].ptr);
			return;
	}
}

int qse_awk_getrunerrnum (qse_awk_rtx_t* run)
{
	return run->errnum;
}

qse_size_t qse_awk_getrunerrlin (qse_awk_rtx_t* run)
{
	return run->errlin;
}

const qse_char_t* qse_awk_getrunerrmsg (qse_awk_rtx_t* run)
{
	if (run->errmsg[0] == QSE_T('\0')) 
		return qse_awk_geterrstr (run->awk, run->errnum);

	return run->errmsg;
}

void qse_awk_setrunerrnum (qse_awk_rtx_t* run, int errnum)
{
	run->errnum = errnum;
	run->errlin = 0;
	run->errmsg[0] = QSE_T('\0');
}

void qse_awk_setrunerrmsg (qse_awk_rtx_t* run, 
	int errnum, qse_size_t errlin, const qse_char_t* errmsg)
{
	run->errnum = errnum;
	run->errlin = errlin;
	qse_strxcpy (run->errmsg, QSE_COUNTOF(run->errmsg), errmsg);
}

void qse_awk_getrunerror (
	qse_awk_rtx_t* run, int* errnum, 
	qse_size_t* errlin, const qse_char_t** errmsg)
{
	if (errnum != QSE_NULL) *errnum = run->errnum;
	if (errlin != QSE_NULL) *errlin = run->errlin;
	if (errmsg != QSE_NULL) 
	{
		if (run->errmsg[0] == QSE_T('\0'))
			*errmsg = qse_awk_geterrstr (run->awk, run->errnum);
		else
			*errmsg = run->errmsg;
	}
}

void qse_awk_setrunerror (
	qse_awk_rtx_t* run, int errnum, qse_size_t errlin,
	const qse_cstr_t* errarg, qse_size_t argcnt)
{
	const qse_char_t* errfmt;
	qse_size_t fmtlen;

	QSE_ASSERT (argcnt <= 5);

	run->errnum = errnum;
	run->errlin = errlin;

	errfmt = qse_awk_geterrstr (run->awk, errnum);
	fmtlen = qse_strlen (errfmt);

	switch (argcnt)
	{
		case 0:
			/* TODO: convert % to %% if the original % is not
			 *       the first % of the %% sequence */
			run->awk->prmfns->sprintf (
				run->awk->prmfns->data,
				run->errmsg, 
				QSE_COUNTOF(run->errmsg), 
				errfmt);
			return;

		case 1:
		{
			/* TODO: what if the argument contains a null character? 
			 *       handle the case more gracefully than now... */

			qse_char_t tmp[QSE_COUNTOF(run->errmsg)];
			qse_size_t len, tl;

			if (fmtlen < QSE_COUNTOF(run->errmsg) &&
			    errarg[0].len + fmtlen >= QSE_COUNTOF(run->errmsg))
			{
				len = QSE_COUNTOF(run->errmsg) - fmtlen - 3 - 1;
				tl = qse_strxncpy (tmp, QSE_COUNTOF(tmp), errarg[0].ptr, len);
				tmp[tl] = QSE_T('.');
				tmp[tl+1] = QSE_T('.');
				tmp[tl+2] = QSE_T('.');
				len += 3;
			}
			else 
			{
				len = errarg[0].len;
				qse_strxncpy (tmp, QSE_COUNTOF(tmp), errarg[0].ptr, len);
			}

			run->awk->prmfns->sprintf (
				run->awk->prmfns->data,
				run->errmsg, 
				QSE_COUNTOF(run->errmsg), 
				errfmt, len, tmp);
			return;
		}

		case 2:
			run->awk->prmfns->sprintf (
				run->awk->prmfns->data,
				run->errmsg, 
				QSE_COUNTOF(run->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr);
			return;

		case 3:
			run->awk->prmfns->sprintf (
				run->awk->prmfns->data,
				run->errmsg, 
				QSE_COUNTOF(run->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr,
				errarg[2].len, errarg[2].ptr);
			return;

		case 4:
			run->awk->prmfns->sprintf (
				run->awk->prmfns->data,
				run->errmsg, 
				QSE_COUNTOF(run->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr,
				errarg[2].len, errarg[2].ptr,
				errarg[3].len, errarg[3].ptr);
			return;

		case 5:
			run->awk->prmfns->sprintf (
				run->awk->prmfns->data,
				run->errmsg, 
				QSE_COUNTOF(run->errmsg), 
				errfmt,
				errarg[0].len, errarg[0].ptr,
				errarg[1].len, errarg[1].ptr,
				errarg[2].len, errarg[2].ptr,
				errarg[3].len, errarg[3].ptr,
				errarg[4].len, errarg[4].ptr);
			return;
	}
}
