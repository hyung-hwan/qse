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

#include "awk.h"

const qse_char_t* qse_awk_dflerrstr (const qse_awk_t* awk, qse_awk_errnum_t errnum)
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
		QSE_T("access denied"),
		QSE_T("not supported"),
		QSE_T("operation not allowed"),
		QSE_T("'${0}' not found"),
		QSE_T("'${0}' already exists"),
		QSE_T("I/O error"),

		QSE_T("cannot open '${0}'"),
		QSE_T("cannot read '${0}'"),
		QSE_T("cannot write '${0}'"),
		QSE_T("cannot close '${0}'"),
		
		QSE_T("block nested too deeply"),
		QSE_T("expression nested too deeply"),

		QSE_T("invalid character '${0}'"),
		QSE_T("invalid digit '${0}'"),

		QSE_T("unexpected end of input"),
		QSE_T("comment not closed properly"),
		QSE_T("string or regular expression not closed"),
		QSE_T("left brace expected in place of '${0}'"),
		QSE_T("left parenthesis expected in place of '${0}'"),
		QSE_T("right parenthesis expected in place of '${0}'"),
		QSE_T("right bracket expected in place of '${0}'"),
		QSE_T("comma expected in place of '${0}'"),
		QSE_T("semicolon expected in place of '${0}'"),
		QSE_T("colon expected in place of '${0}'"),
		QSE_T("statement not ending with a semicolon"),
		QSE_T("keyword 'in' expected in place of '${0}'"),
		QSE_T("right-hand side of 'in' not a variable"),
		QSE_T("expression not recognized around '${0}'"),

		QSE_T("keyword 'function' expected in place of '${0}'"),
		QSE_T("keyword 'while' expected in place of '${0}'"),
		QSE_T("invalid assignment statement"),
		QSE_T("identifier expected in place of '${0}'"),
		QSE_T("'${0}' not a valid function name"),
		QSE_T("BEGIN not followed by left bracket on the same line"),
		QSE_T("END not followed by left bracket on the same line"),
		QSE_T("keyword '${0}' redefined"),
		QSE_T("intrinsic function '${0}' redefined"),
		QSE_T("function '${0}' redefined"),
		QSE_T("global variable '${0}' redefined"),
		QSE_T("parameter '${0}' redefined"),
		QSE_T("variable '${0}' redefined"),
		QSE_T("duplicate parameter name '${0}'"),
		QSE_T("duplicate global variable '${0}'"),
		QSE_T("duplicate local variable '${0}'"),
		QSE_T("'${0}' not a valid parameter name"),
		QSE_T("'${0}' not a valid variable name"),
		QSE_T("variable name missing"),
		QSE_T("undefined identifier '${0}'"),
		QSE_T("l-value required"),
		QSE_T("too many global variables"),
		QSE_T("too many local variables"),
		QSE_T("too many parameters"),
		QSE_T("too many segments"),
		QSE_T("segment '${0}' too long"),
		QSE_T("bad argument"),
		QSE_T("no argument provided"),
		QSE_T("'break' outside a loop"),
		QSE_T("'continue' outside a loop"),
		QSE_T("'next' illegal in the BEGIN block"),
		QSE_T("'next' illegal in the END block"),
		QSE_T("'nextfile' illegal in the BEGIN block"),
		QSE_T("'nextfile' illegal in the END block"),
		QSE_T("both prefix and postfix increment/decrement operator present"),
		QSE_T("illegal operand for increment/decrement operator"),
		QSE_T("'@include' not followed by a string"),
		QSE_T("include level too deep"),
		QSE_T("'${0}' not recognized"),
		QSE_T("@ not followed by a valid word"),

		QSE_T("divide by zero"),
		QSE_T("invalid operand"),
		QSE_T("wrong position index"),
		QSE_T("too few arguments"),
		QSE_T("too many arguments"),
		QSE_T("function '${0}' not found"),
		QSE_T("'${0}' not deletable"),
		QSE_T("value not a map"),
		QSE_T("right-hand side of the 'in' operator not a map"),
		QSE_T("right-hand side of the 'in' operator not a map nor nil"),
		QSE_T("value not referenceable"),
		QSE_T("cannot return a map"),                      /* EMAPRET */
		QSE_T("cannot assign a map to a positional"),      /* EMAPTOPOS */
		QSE_T("cannot assign a map to an indexed variable"),/* EMAPTOIDX */
		QSE_T("cannot assign a map to a variable '${0}'"), /* EMAPTONVAR */
		QSE_T("cannot change a map to a scalar"),          /* EMAPTOSCALAR */
		QSE_T("cannot change a scalar to a map"),          /* ESCALARTOMAP */
		QSE_T("cannot change a map '${0}' to another map"),/* ENMAPTOMAP */
		QSE_T("cannot change a map '${0}' to a scalar"),   /* ENMAPTOSCALAR */
		QSE_T("cannot change a scalar '${0}' to a map"),   /* ENSCALARTOMAP */
		QSE_T("invalid value to convert to a string"),
		QSE_T("invalid value to convert to a number"),
		QSE_T("invalid value to a character"),
		QSE_T("invalid value for hashing"),
		QSE_T("'next' called from BEGIN block"),
		QSE_T("'next' called from END block"),
		QSE_T("'nextfile' called from BEGIN block"),
		QSE_T("'nextfile' called from END block"),
		QSE_T("intrinsic function handler for '${0}' failed"),
		QSE_T("wrong implementation of user-defined I/O handler"),
		QSE_T("I/O handler returned an error"),
		QSE_T("no such I/O name found"),
		QSE_T("I/O name empty"),
		QSE_T("I/O name '${0}' containing '\\0'"),
		QSE_T("not sufficient arguments to formatting sequence"),
		QSE_T("recursion detected in format conversion"),
		QSE_T("invalid character in CONVFMT"),
		QSE_T("invalid character in OFMT"),

		QSE_T("failed to build regular expression"),
		QSE_T("failed to match regular expression"),
		QSE_T("recursion too deep in regular expression"),
		QSE_T("right parenthesis expected in regular expression"),
		QSE_T("right bracket expected in regular expression"),
		QSE_T("right brace expected in regular expression"),
		QSE_T("colon expected in regular expression"),
		QSE_T("invalid character range in regular expression"),
		QSE_T("invalid character class in regular expression"),
		QSE_T("invalid occurrence bound in regular expression"),
		QSE_T("special character at wrong position"),
		QSE_T("premature end of regular expression")
	};

	return (errnum >= 0 && errnum < QSE_COUNTOF(errstr))?
		errstr[errnum]: QSE_T("unknown error");
}

qse_awk_errstr_t qse_awk_geterrstr (const qse_awk_t* awk)
{
	return awk->errstr;
}

void qse_awk_seterrstr (qse_awk_t* awk, qse_awk_errstr_t errstr)
{
	awk->errstr = errstr;
}

qse_awk_errnum_t qse_awk_geterrnum (const qse_awk_t* awk)
{
	return awk->errinf.num;
}

const qse_awk_loc_t* qse_awk_geterrloc (const qse_awk_t* awk)
{
	return &awk->errinf.loc;
}

const qse_char_t* qse_awk_geterrmsg (const qse_awk_t* awk)
{
	return (awk->errinf.msg[0] == QSE_T('\0'))?
		qse_awk_geterrstr(awk)(awk,awk->errinf.num): awk->errinf.msg;
}

void qse_awk_geterrinf (const qse_awk_t* awk, qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (errinf, &awk->errinf, QSE_SIZEOF(*errinf));
	if (errinf->msg[0] == QSE_T('\0'))
	{
		qse_strxcpy (errinf->msg, QSE_COUNTOF(errinf->msg),
			qse_awk_geterrstr(awk)(awk,awk->errinf.num));
	}
}

void qse_awk_geterror (
	const qse_awk_t* awk, qse_awk_errnum_t* errnum, 
	const qse_char_t** errmsg, qse_awk_loc_t* errloc)
{
	if (errnum != QSE_NULL) *errnum = awk->errinf.num;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (awk->errinf.msg[0] == QSE_T('\0'))?
			qse_awk_geterrstr(awk)(awk,awk->errinf.num):
			awk->errinf.msg;
	}
	if (errloc != QSE_NULL) *errloc = awk->errinf.loc;
}

void qse_awk_seterrnum (
	qse_awk_t* awk, qse_awk_errnum_t errnum, const qse_cstr_t* errarg)
{
	qse_awk_seterror (awk, errnum, errarg, QSE_NULL);
}

void qse_awk_seterrinf (qse_awk_t* awk, const qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (&awk->errinf, errinf, QSE_SIZEOF(*errinf));
}

void qse_awk_seterror (
	qse_awk_t* awk, qse_awk_errnum_t errnum, const qse_cstr_t* errarg,
	const qse_awk_loc_t* errloc)
{
	const qse_char_t* errfmt;

	QSE_MEMSET (&awk->errinf, 0, QSE_SIZEOF(awk->errinf));
	awk->errinf.num = errnum;

	errfmt = qse_awk_geterrstr(awk)(awk,errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (
		awk->errinf.msg, QSE_COUNTOF(awk->errinf.msg),
		errfmt, errarg
	);

	if (errloc != QSE_NULL) awk->errinf.loc = *errloc;
}

qse_awk_errnum_t qse_awk_rtx_geterrnum (const qse_awk_rtx_t* rtx)
{
	return rtx->errinf.num;
}

const qse_awk_loc_t* qse_awk_rtx_geterrloc (const qse_awk_rtx_t* rtx)
{
	return &rtx->errinf.loc;
}

const qse_char_t* qse_awk_rtx_geterrmsg (const qse_awk_rtx_t* rtx)
{
	return (rtx->errinf.msg[0] == QSE_T('\0')) ?
		qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num): rtx->errinf.msg;
}

void qse_awk_rtx_geterrinf (const qse_awk_rtx_t* rtx, qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (errinf, &rtx->errinf, QSE_SIZEOF(*errinf));
	if (errinf->msg[0] == QSE_T('\0'))
	{
		qse_strxcpy (errinf->msg, QSE_COUNTOF(errinf->msg),
			qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num));
	}
}

void qse_awk_rtx_geterror (
	const qse_awk_rtx_t* rtx, qse_awk_errnum_t* errnum, 
	const qse_char_t** errmsg, qse_awk_loc_t* errloc)
{
	if (errnum != QSE_NULL) *errnum = rtx->errinf.num;
	if (errloc != QSE_NULL) *errloc = rtx->errinf.loc;
	if (errmsg != QSE_NULL) 
	{
		*errmsg = (rtx->errinf.msg[0] == QSE_T('\0'))?
			qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num): rtx->errinf.msg;
	}
}

void qse_awk_rtx_seterrnum (
	qse_awk_rtx_t* rtx, qse_awk_errnum_t errnum, const qse_cstr_t* errarg)
{
	qse_awk_rtx_seterror (rtx, errnum, errarg, 0);
}

void qse_awk_rtx_seterrinf (qse_awk_rtx_t* rtx, const qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (&rtx->errinf, errinf, QSE_SIZEOF(*errinf));
}

void qse_awk_rtx_seterror (
	qse_awk_rtx_t* rtx, qse_awk_errnum_t errnum, const qse_cstr_t* errarg,
	const qse_awk_loc_t* errloc)
{
	const qse_char_t* errfmt;

	QSE_MEMSET (&rtx->errinf, 0, QSE_SIZEOF(rtx->errinf));
	rtx->errinf.num = errnum;

	errfmt = qse_awk_geterrstr(rtx->awk)(rtx->awk,errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (
		rtx->errinf.msg, QSE_COUNTOF(rtx->errinf.msg),
		errfmt, errarg
	);

	if (errloc != QSE_NULL) rtx->errinf.loc = *errloc;
}

