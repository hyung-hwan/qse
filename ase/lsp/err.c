/*
 * $Id: err.c,v 1.2 2006-10-23 14:42:38 bacon Exp $
 */

#include <sse/lsp/lsp.h>

static const sse_char_t* __errstr[] = 
{
	SSE_T("no error"),
	SSE_T("out of memory"),
	SSE_T("abort"),
	SSE_T("end"),
	SSE_T("input not attached"),
	SSE_T("input"),
	SSE_T("output not attached"),
	SSE_T("output"),
	SSE_T("syntax"),
	SSE_T("bad arguments"),
	SSE_T("wrong arguments"),
	SSE_T("too few arguments"),
	SSE_T("too many arguments"),
	SSE_T("undefined function"),
	SSE_T("bad function"),
	SSE_T("duplicate formal"),
	SSE_T("bad symbol"),
	SSE_T("undefined symbol"),
	SSE_T("empty body"),
	SSE_T("bad value"),
	SSE_T("divide by zero")
};

int sse_lsp_geterrnum (sse_lsp_t* lsp)
{
	return lsp->errnum;
}

const sse_char_t* sse_lsp_geterrstr (int errnum)
{
	if (errnum >= 0 && errnum < sse_countof(__errstr)) 
	{
		return __errstr[errnum];
	}

	return SSE_T("unknown error");
}
