/*
 * $Id: error.c,v 1.1 2005-09-19 12:04:00 bacon Exp $
 */

#include <xp/lsp/lsp.h>
#include <xp/bas/string.h>

static const xp_char_t* __errstr[] = 
{
	XP_TEXT("no error"),
	XP_TEXT("abort"),
	XP_TEXT("end"),
	XP_TEXT("memory"),
	XP_TEXT("input not attached"),
	XP_TEXT("input"),
	XP_TEXT("output not attached"),
	XP_TEXT("output"),
	XP_TEXT("syntax"),
	XP_TEXT("bad arguments"),
	XP_TEXT("wrong arguments"),
	XP_TEXT("too few arguments"),
	XP_TEXT("too many arguments"),
	XP_TEXT("undefined function"),
	XP_TEXT("bad function"),
	XP_TEXT("duplicate formal"),
	XP_TEXT("bad symbol"),
	XP_TEXT("undefined symbol"),
	XP_TEXT("empty body"),
	XP_TEXT("bad value")
};

int xp_lsp_error (xp_lsp_t* lsp, xp_char_t* buf, xp_size_t size)
{
        if (buf == XP_NULL || size == 0) return lsp->errnum;
        xp_strxcpy (buf, size, __errstr[lsp->errnum]);
        return lsp->errnum;
}

