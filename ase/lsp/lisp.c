/*
 * $Id: lisp.c,v 1.5 2005-02-05 05:30:25 bacon Exp $
 */

#include <xp/lisp/lisp.h>
#include <xp/c/stdlib.h>
#include <xp/c/assert.h>

xp_lisp_t* xp_lisp_new (xp_size_t mem_ubound, xp_size_t mem_ubound_inc)
{
	xp_lisp_t* lsp;

	lsp = (xp_lisp_t*)xp_malloc(sizeof(xp_lisp_t));
	if (lsp == XP_NULL) return lsp;

	lsp->token = xp_lisp_token_new (256);
	if (lsp->token == XP_NULL) {
		xp_free (lsp);
		return XP_NULL;
	}

	lsp->error              = XP_LISP_ERR_NONE;
	//lsp->opt_undef_symbol = 1;
	lsp->opt_undef_symbol   = 0;

	lsp->curc               = XP_EOF;
	lsp->creader            = XP_NULL;
	lsp->creader_extra      = XP_NULL;
	lsp->creader_just_set   = 0;
	lsp->outstream          = xp_stdout;

	lsp->mem = xp_lisp_mem_new (mem_ubound, mem_ubound_inc);
	if (lsp->mem == XP_NULL) {
		xp_lisp_token_free (lsp->token);
		free (lsp);
		return XP_NULL;
	}

	if (xp_lisp_add_prims (lsp->mem) == -1) {
		xp_lisp_mem_free (lsp->mem);
		xp_lisp_token_free (lsp->token);
		free (lsp);
		return XP_NULL;
	}

	return lsp;
}

void xp_lisp_free (xp_lisp_t* lsp)
{
	xp_assert (lsp != XP_NULL);

	xp_lisp_mem_free (lsp->mem);
	xp_lisp_token_free (lsp->token);
	free (lsp);
}

int xp_lisp_error (xp_lisp_t* lsp, xp_lisp_char* buf, xp_size_t size)
{
	if (buf != XP_NULL || size == 0) return lsp->error;

	// TODO:...
	/*
	switch (lsp->error) {

	default:
		xp_lisp_copy_string (buf, size, "unknown error");	
	}
	*/

	return lsp->error;
}
