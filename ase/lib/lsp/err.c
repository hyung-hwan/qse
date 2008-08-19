/*
 * $Id: err.c 332 2008-08-18 11:21:48Z baconevi $
 *
 * {License}
 */

#include "lsp_i.h"

static const ase_char_t* __geterrstr (int errnum)
{
	static const ase_char_t* __errstr[] = 
	{
		ASE_T("no error"),
		ASE_T("out of memory"),
		ASE_T("exit"),
		ASE_T("end of source"),
		ASE_T("unexpected end of string"),
		ASE_T("input not attached"),
		ASE_T("input"),
		ASE_T("output not attached"),
		ASE_T("output"),
		ASE_T("syntax"),
		ASE_T("right parenthesis expected"),
		ASE_T("bad arguments"),
		ASE_T("too few arguments"),
		ASE_T("too many arguments"),
		ASE_T("undefined function '%s'"),
		ASE_T("bad function"),
		ASE_T("duplicate formal"),
		ASE_T("bad symbol"),
		ASE_T("undefined symbol '%s'"),
		ASE_T("empty body"),
		ASE_T("bad value"),
		ASE_T("divide by zero")
	};

	if (errnum >= 0 && errnum < ASE_COUNTOF(__errstr)) 
	{
		return __errstr[errnum];
	}

	return ASE_T("unknown error");
}

void ase_lsp_geterror (
	ase_lsp_t* lsp, int* errnum, const ase_char_t** errmsg)
{
	if (errnum != ASE_NULL) *errnum = lsp->errnum;
	if (errmsg != ASE_NULL) *errmsg = lsp->errmsg;
}

void ase_lsp_seterror (
	ase_lsp_t* lsp, int errnum,
	const ase_char_t** errarg, ase_size_t argcnt)
{
	const ase_char_t* errfmt;

	ASE_ASSERT (argcnt <= 5);

	lsp->errnum = errnum;
	errfmt = __geterrstr (errnum);

	switch (argcnt)
	{
		case 0:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				ASE_COUNTOF(lsp->errmsg), 
				errfmt);
			return;

		case 1:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				ASE_COUNTOF(lsp->errmsg), 
				errfmt,
				errarg[0]);
			return;

		case 2:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				ASE_COUNTOF(lsp->errmsg), 
				errfmt,
				errarg[0],
				errarg[1]);
			return;

		case 3:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				ASE_COUNTOF(lsp->errmsg), 
				errfmt,
				errarg[0],
				errarg[1],
				errarg[2]);
			return;

		case 4:
			lsp->prmfns.misc.sprintf (
				lsp->prmfns.misc.data,
				lsp->errmsg, 
				ASE_COUNTOF(lsp->errmsg), 
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
				ASE_COUNTOF(lsp->errmsg), 
				errfmt,
				errarg[0],
				errarg[1],
				errarg[2],
				errarg[3],
				errarg[4]);
			return;
	}
}


