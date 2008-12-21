/*
 * $Id: err.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

static const qse_char_t* __geterrstr (int errnum)
{
	static const qse_char_t* __errstr[] = 
	{
		QSE_T("no error"),
		QSE_T("out of memory"),
		QSE_T("exit"),
		QSE_T("end of source"),
		QSE_T("unexpected end of string"),
		QSE_T("input not attached"),
		QSE_T("input"),
		QSE_T("output not attached"),
		QSE_T("output"),
		QSE_T("syntax"),
		QSE_T("right parenthesis expected"),
		QSE_T("bad arguments"),
		QSE_T("too few arguments"),
		QSE_T("too many arguments"),
		QSE_T("undefined function '%s'"),
		QSE_T("bad function"),
		QSE_T("duplicate formal"),
		QSE_T("bad symbol"),
		QSE_T("undefined symbol '%s'"),
		QSE_T("empty body"),
		QSE_T("bad value"),
		QSE_T("divide by zero")
	};

	if (errnum >= 0 && errnum < QSE_COUNTOF(__errstr)) 
	{
		return __errstr[errnum];
	}

	return QSE_T("unknown error");
}

void qse_lsp_geterror (
	qse_lsp_t* lsp, int* errnum, const qse_char_t** errmsg)
{
	if (errnum != QSE_NULL) *errnum = lsp->errnum;
	if (errmsg != QSE_NULL) *errmsg = lsp->errmsg;
}

void qse_lsp_seterror (
	qse_lsp_t* lsp, int errnum,
	const qse_char_t** errarg, qse_size_t argcnt)
{
	const qse_char_t* errfmt;

	QSE_ASSERT (argcnt <= 5);

	lsp->errnum = errnum;
	errfmt = __geterrstr (errnum);

	switch (argcnt)
	{
		case 0:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				QSE_COUNTOF(lsp->errmsg), 
				errfmt);
			return;

		case 1:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				QSE_COUNTOF(lsp->errmsg), 
				errfmt,
				errarg[0]);
			return;

		case 2:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				QSE_COUNTOF(lsp->errmsg), 
				errfmt,
				errarg[0],
				errarg[1]);
			return;

		case 3:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				QSE_COUNTOF(lsp->errmsg), 
				errfmt,
				errarg[0],
				errarg[1],
				errarg[2]);
			return;

		case 4:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				QSE_COUNTOF(lsp->errmsg), 
				errfmt,
				errarg[0],
				errarg[1],
				errarg[2],
				errarg[3]);
			return;

		case 5:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				QSE_COUNTOF(lsp->errmsg), 
				errfmt,
				errarg[0],
				errarg[1],
				errarg[2],
				errarg[3],
				errarg[4]);
			return;
	}
}


