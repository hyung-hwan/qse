/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "awk-prv.h"
#include <qse/cmn/mbwc.h>

const qse_char_t* qse_awk_dflerrstr (qse_awk_t* awk, qse_awk_errnum_t errnum)
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
		QSE_T("operation not allowed"),
		QSE_T("not supported"),
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
		QSE_T("invalid mbs character '${0}'"),
		QSE_T("left brace expected in place of '${0}'"),
		QSE_T("left parenthesis expected in place of '${0}'"),
		QSE_T("right parenthesis expected in place of '${0}'"),
		QSE_T("right bracket expected in place of '${0}'"),
		QSE_T("comma expected in place of '${0}'"),
		QSE_T("semicolon expected in place of '${0}'"),
		QSE_T("colon expected in place of '${0}'"),
		QSE_T("integer literal expected in place of '${0}'"),
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

		QSE_T("stack error"),
		QSE_T("divide by zero"),
		QSE_T("invalid operand"),
		QSE_T("wrong position index"),
		QSE_T("too few arguments"),
		QSE_T("too many arguments"),
		QSE_T("function '${0}' not found"),
		QSE_T("non-function value in '${0}'"),
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

qse_awk_errstr_t qse_awk_geterrstr (qse_awk_t* awk)
{
	return awk->errstr;
}

void qse_awk_seterrstr (qse_awk_t* awk, qse_awk_errstr_t errstr)
{
	awk->errstr = errstr;
}

qse_awk_errnum_t qse_awk_geterrnum (qse_awk_t* awk)
{
	return awk->errinf.num;
}

const qse_awk_loc_t* qse_awk_geterrloc (qse_awk_t* awk)
{
	return &awk->errinf.loc;
}

const qse_mchar_t* qse_awk_geterrmsgasmbs (qse_awk_t* awk)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return (awk->errinf.msg[0] == QSE_T('\0'))?
		qse_awk_geterrstr(awk)(awk,awk->errinf.num): awk->errinf.msg;
#else
	const qse_char_t* msg;
	qse_size_t wcslen, mbslen;

	msg = (awk->errinf.msg[0] == QSE_T('\0'))?
		qse_awk_geterrstr(awk)(awk,awk->errinf.num): awk->errinf.msg;

	mbslen = QSE_COUNTOF(awk->merrmsg);
	qse_wcstombswithcmgr(msg, &wcslen, awk->merrmsg, &mbslen, qse_awk_getcmgr(awk));

	return awk->merrmsg;
#endif
}

const qse_wchar_t* qse_awk_geterrmsgaswcs (qse_awk_t* awk)
{
#if defined(QSE_CHAR_IS_MCHAR)
	const qse_char_t* msg;
	qse_size_t wcslen, mbslen;

	msg = (awk->errinf.msg[0] == QSE_T('\0'))?
		qse_awk_geterrstr(awk)(awk,awk->errinf.num): awk->errinf.msg;

	wcslen = QSE_COUNTOF(awk->werrmsg);
	qse_mbstowcsallwithcmgr (msg, &mbslen, awk->werrmsg, &wcslen, qse_awk_getcmgr(awk));

	return awk->werrmsg;
#else
	return (awk->errinf.msg[0] == QSE_T('\0'))?
		qse_awk_geterrstr(awk)(awk,awk->errinf.num): awk->errinf.msg;
#endif
	
}

void qse_awk_geterrinf (qse_awk_t* awk, qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (errinf, &awk->errinf, QSE_SIZEOF(*errinf));
	if (errinf->msg[0] == QSE_T('\0'))
	{
		qse_strxcpy (errinf->msg, QSE_COUNTOF(errinf->msg),
			qse_awk_geterrstr(awk)(awk,awk->errinf.num));
	}
}

void qse_awk_geterror (qse_awk_t* awk, qse_awk_errnum_t* errnum, const qse_char_t** errmsg, qse_awk_loc_t* errloc)
{
	if (errnum) *errnum = awk->errinf.num;
	if (errmsg) 
	{
		*errmsg = (awk->errinf.msg[0] == QSE_T('\0'))?
			qse_awk_geterrstr(awk)(awk,awk->errinf.num):
			awk->errinf.msg;
	}
	if (errloc) *errloc = awk->errinf.loc;
}

const qse_char_t* qse_awk_backuperrmsg (qse_awk_t* awk)
{
	qse_strxcpy (awk->errmsg_backup, QSE_COUNTOF(awk->errmsg_backup), qse_awk_geterrmsg(awk));
	return awk->errmsg_backup;
}

void qse_awk_seterrnum (qse_awk_t* awk, qse_awk_errnum_t errnum, const qse_cstr_t* errarg)
{
	qse_awk_seterror (awk, errnum, errarg, QSE_NULL);
}

void qse_awk_seterrinf (qse_awk_t* awk, const qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (&awk->errinf, errinf, QSE_SIZEOF(*errinf));
}

void qse_awk_seterrfmt (qse_awk_t* awk, qse_awk_errnum_t errnum, qse_awk_loc_t* errloc, const qse_char_t* errfmt, ...)
{
	va_list ap;

	QSE_MEMSET (&awk->errinf, 0, QSE_SIZEOF(awk->errinf));
	awk->errinf.num = errnum;

	va_start (ap, errfmt);
	qse_strxvfmt (awk->errinf.msg, QSE_COUNTOF(awk->errinf.msg), errfmt, ap);
	va_end (ap);

	if (errloc) awk->errinf.loc = *errloc;
}

void qse_awk_seterror (qse_awk_t* awk, qse_awk_errnum_t errnum, const qse_cstr_t* errarg,	const qse_awk_loc_t* errloc)
{
	const qse_char_t* errfmt;

	QSE_MEMSET (&awk->errinf, 0, QSE_SIZEOF(awk->errinf));
	awk->errinf.num = errnum;

	errfmt = qse_awk_geterrstr(awk)(awk,errnum);
	QSE_ASSERT (errfmt != QSE_NULL);
	qse_strxfncpy (awk->errinf.msg, QSE_COUNTOF(awk->errinf.msg), errfmt, errarg);

	if (errloc != QSE_NULL) awk->errinf.loc = *errloc;
}


qse_awk_errnum_t qse_awk_rtx_geterrnum (qse_awk_rtx_t* rtx)
{
	return rtx->errinf.num;
}

const qse_awk_loc_t* qse_awk_rtx_geterrloc (qse_awk_rtx_t* rtx)
{
	return &rtx->errinf.loc;
}

const qse_mchar_t* qse_awk_rtx_geterrmsgasmbs (qse_awk_rtx_t* rtx)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return (rtx->errinf.msg[0] == QSE_T('\0')) ?
		qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num): rtx->errinf.msg;
#else
	const qse_char_t* msg;
	qse_size_t wcslen, mbslen;

	msg = (rtx->errinf.msg[0] == QSE_T('\0')) ?
		qse_awk_geterrstr(rtx->awk)(rtx->awk, rtx->errinf.num): rtx->errinf.msg;

	mbslen = QSE_COUNTOF(rtx->merrmsg);
	qse_wcstombswithcmgr(msg, &wcslen, rtx->merrmsg, &mbslen, qse_awk_rtx_getcmgr(rtx));

	return rtx->merrmsg;
#endif
}

const qse_wchar_t* qse_awk_rtx_geterrmsgaswcs (qse_awk_rtx_t* rtx)
{
#if defined(QSE_CHAR_IS_MCHAR)
	const qse_char_t* msg;
	qse_size_t wcslen, mbslen;

	msg = (rtx->errinf.msg[0] == QSE_T('\0')) ?
		qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num): rtx->errinf.msg;

	wcslen = QSE_COUNTOF(rtx->werrmsg);
	qse_mbstowcsallwithcmgr (msg, &mbslen, rtx->werrmsg, &wcslen, qse_awk_rtx_getcmgr(rtx));

	return rtx->werrmsg;
#else
	return (rtx->errinf.msg[0] == QSE_T('\0')) ?
		qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num): rtx->errinf.msg;
#endif
}

void qse_awk_rtx_geterrinf (qse_awk_rtx_t* rtx, qse_awk_errinf_t* errinf)
{
	QSE_MEMCPY (errinf, &rtx->errinf, QSE_SIZEOF(*errinf));
	if (errinf->msg[0] == QSE_T('\0'))
	{
		qse_strxcpy (errinf->msg, QSE_COUNTOF(errinf->msg),
			qse_awk_geterrstr(rtx->awk)(rtx->awk,rtx->errinf.num));
	}
}

void qse_awk_rtx_geterror (
	qse_awk_rtx_t* rtx, qse_awk_errnum_t* errnum, 
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

const qse_char_t* qse_awk_rtx_backuperrmsg (qse_awk_rtx_t* rtx)
{
	qse_strxcpy (rtx->errmsg_backup, QSE_COUNTOF(rtx->errmsg_backup), qse_awk_rtx_geterrmsg(rtx));
	return rtx->errmsg_backup;
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

void qse_awk_rtx_seterrfmt (qse_awk_rtx_t* rtx, qse_awk_errnum_t errnum, const qse_awk_loc_t* errloc, const qse_char_t* errfmt, ...)
{
	va_list ap;

	QSE_MEMSET (&rtx->errinf, 0, QSE_SIZEOF(rtx->errinf));
	rtx->errinf.num = errnum;

	va_start (ap, errfmt);
	qse_strxvfmt (rtx->errinf.msg, QSE_COUNTOF(rtx->errinf.msg), errfmt, ap);
	va_end (ap);

	if (errloc) rtx->errinf.loc = *errloc;
}


void qse_awk_rtx_seterror (qse_awk_rtx_t* rtx, qse_awk_errnum_t errnum, const qse_cstr_t* errarg, const qse_awk_loc_t* errloc)
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

