/*
 * $Id: err.c,v 1.3 2006-10-24 04:22:39 bacon Exp $
 */

#include <ase/lsp/lsp.h>

static const ase_char_t* __errstr[] = 
{
	ASE_T("no error"),
	ASE_T("out of memory"),
	ASE_T("abort"),
	ASE_T("end"),
	ASE_T("input not attached"),
	ASE_T("input"),
	ASE_T("output not attached"),
	ASE_T("output"),
	ASE_T("syntax"),
	ASE_T("bad arguments"),
	ASE_T("wrong arguments"),
	ASE_T("too few arguments"),
	ASE_T("too many arguments"),
	ASE_T("undefined function"),
	ASE_T("bad function"),
	ASE_T("duplicate formal"),
	ASE_T("bad symbol"),
	ASE_T("undefined symbol"),
	ASE_T("empty body"),
	ASE_T("bad value"),
	ASE_T("divide by zero")
};

int ase_lsp_geterrnum (ase_lsp_t* lsp)
{
	return lsp->errnum;
}

const ase_char_t* ase_lsp_geterrstr (int errnum)
{
	if (errnum >= 0 && errnum < ase_countof(__errstr)) 
	{
		return __errstr[errnum];
	}

	return ASE_T("unknown error");
}
