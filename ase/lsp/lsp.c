/*
 * $Id: lsp.c,v 1.3 2005-09-18 03:57:26 bacon Exp $
 */

#include <xp/lsp/lsp.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>

xp_lsp_t* xp_lsp_open (xp_lsp_t* lsp, 
	xp_size_t mem_ubound, xp_size_t mem_ubound_inc)
{
	if (lsp == XP_NULL) {
		lsp = (xp_lsp_t*)xp_malloc(sizeofxp_lsp_t));
		if (lsp == XP_NULL) return lsp;
		lsp->__malloced = xp_true;
	}
	else lsp->__malloced = xp_false;

	lsp->token = xp_lsp_token_new (256);
	if (lsp->token == XP_NULL) {
		xp_free (lsp);
		return XP_NULL;
	}

	lsp->error              = XP_LISP_ERR_NONE;
	//lsp->opt_undef_symbol = 1;
	lsp->opt_undef_symbol   = 0;

	lsp->curc               = XP_CHAR_EOF;
	lsp->creader            = XP_NULL;
	lsp->creader_extra      = XP_NULL;
	lsp->creader_just_set   = 0;
	lsp->outstream          = xp_stdout;

	lsp->mem = xp_lsp_mem_new (mem_ubound, mem_ubound_inc);
	if (lsp->mem == XP_NULL) {
		xp_lsp_token_free (lsp->token);
		if (lsp->__malloced) xp_free (lsp);
		return XP_NULL;
	}

	if (xp_lsp_add_prims (lsp->mem) == -1) {
		xp_lsp_mem_free (lsp->mem);
		xp_lsp_token_free (lsp->token);
		if (lsp->__malloced) xp_free (lsp);
		return XP_NULL;
	}

	return lsp;
}

void xp_lsp_close (xp_lsp_t* lsp)
{
	xp_assert (lsp != XP_NULL);
	xp_lsp_mem_free (lsp->mem);
	xp_lsp_token_free (lsp->token);
	if (lsp->__malloced) xp_free (lsp);
}

intxp_lsp_error xp_lsp_t* lsp, xp_char_t* buf, xp_size_t size)
{
	if (buf != XP_NULL || size == 0) return lsp->error;

	// TODO:...
	/*
	switch (lsp->error) {

	default:
	xp_lsp_copy_string (buf, size, "unknown error");	
	}
	*/

	return lsp->error;
}
