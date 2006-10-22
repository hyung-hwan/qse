/*
 * $Id: error.c,v 1.3 2006-10-22 13:10:45 bacon Exp $
 */

#include <sse/lsp/lsp.h>
#include <sse/bas/string.h>

static const sse_char_t* __errstr[] = 
{
	SSE_TEXT("no error"),
	SSE_TEXT("abort"),
	SSE_TEXT("end"),
	SSE_TEXT("memory"),
	SSE_TEXT("input not attached"),
	SSE_TEXT("input"),
	SSE_TEXT("output not attached"),
	SSE_TEXT("output"),
	SSE_TEXT("syntax"),
	SSE_TEXT("bad arguments"),
	SSE_TEXT("wrong arguments"),
	SSE_TEXT("too few arguments"),
	SSE_TEXT("too many arguments"),
	SSE_TEXT("undefined function"),
	SSE_TEXT("bad function"),
	SSE_TEXT("duplicate formal"),
	SSE_TEXT("bad symbol"),
	SSE_TEXT("undefined symbol"),
	SSE_TEXT("empty body"),
	SSE_TEXT("bad value"),
	SSE_TEXT("divide by zero")
};

int sse_lsp_error (sse_lsp_t* lsp, sse_char_t* buf, sse_size_t size)
{
        if (buf == SSE_NULL || size == 0) return lsp->errnum;
        sse_strxcpy (buf, size, __errstr[lsp->errnum]);
        return lsp->errnum;
}

