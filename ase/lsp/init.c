/*
 * $Id: init.c,v 1.3 2005-09-18 13:06:43 bacon Exp $
 */

#include <xp/lsp/lsp.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>

xp_lsp_t* xp_lsp_open (xp_lsp_t* lsp, 
	xp_size_t mem_ubound, xp_size_t mem_ubound_inc)
{
	if (lsp == XP_NULL) {
		lsp = (xp_lsp_t*)xp_malloc(xp_sizeof(xp_lsp_t));
		if (lsp == XP_NULL) return lsp;
		lsp->__malloced = xp_true;
	}
	else lsp->__malloced = xp_false;

	if (xp_lsp_token_open(&lsp->token, 0) == XP_NULL) {
		if (lsp->__malloced) xp_free (lsp);
		return XP_NULL;
	}

	lsp->errnum = XP_LSP_ERR_NONE;
	//lsp->opt_undef_symbol = 1;
	lsp->opt_undef_symbol = 0;

	lsp->curc = XP_CHAR_EOF;
	lsp->input_func = XP_NULL;
	lsp->output_func = XP_NULL;
	lsp->input_arg = XP_NULL;
	lsp->output_arg = XP_NULL;

	lsp->mem = xp_lsp_mem_new (mem_ubound, mem_ubound_inc);
	if (lsp->mem == XP_NULL) {
		xp_lsp_token_close (&lsp->token);
		if (lsp->__malloced) xp_free (lsp);
		return XP_NULL;
	}

	if (xp_lsp_add_prims (lsp->mem) == -1) {
		xp_lsp_mem_free (lsp->mem);
		xp_lsp_token_close (&lsp->token);
		if (lsp->__malloced) xp_free (lsp);
		return XP_NULL;
	}

	return lsp;
}

void xp_lsp_close (xp_lsp_t* lsp)
{
	xp_assert (lsp != XP_NULL);
	xp_lsp_mem_free (lsp->mem);
	xp_lsp_token_close (&lsp->token);
	if (lsp->__malloced) xp_free (lsp);
}

int xp_lsp_error (xp_lsp_t* lsp, xp_char_t* buf, xp_size_t size)
{
	if (buf != XP_NULL || size == 0) return lsp->errnum;

	// TODO:...
	/*
	switch (lsp->errnum) {

	default:
	xp_lsp_copy_string (buf, size, "unknown error");	
	}
	*/

	return lsp->errnum;
}

int xp_lsp_attach_input (xp_lsp_t* lsp, xp_lsp_io_t input, void* arg)
{
	if (xp_lsp_detach_input(lsp) == -1) return -1;

	xp_assert (lsp->input_func == XP_NULL);

	if (input(XP_LSP_IO_OPEN, arg, XP_NULL, 0) == -1) {
		/* TODO: set error number */
		return -1;
	}
	lsp->input_func = input;
	lsp->input_arg = arg;
	return 0;
}

int xp_lsp_detach_input (xp_lsp_t* lsp)
{
	if (lsp->input_func != XP_NULL) {
		if (lsp->input_func(XP_LSP_IO_CLOSE, lsp->input_arg, XP_NULL, 0) == -1) {
			/* TODO: set error number */
			return -1;
		}
		lsp->input_func = XP_NULL;
		lsp->input_arg = XP_NULL;
	}

	return 0;
}

int xp_lsp_attach_output (xp_lsp_t* lsp, xp_lsp_io_t output, void* arg)
{
	if (xp_lsp_detach_output(lsp) == -1) return -1;

	xp_assert (lsp->output_func == XP_NULL);

	if (output(XP_LSP_IO_OPEN, arg, XP_NULL, 0) == -1) {
		/* TODO: set error number */
		return -1;
	}
	lsp->output_func = output;
	lsp->output_arg = arg;
	return 0;
}

int xp_lsp_detach_output (xp_lsp_t* lsp)
{
	if (lsp->output_func != XP_NULL) {
		if (lsp->output_func(XP_LSP_IO_CLOSE, lsp->output_arg, XP_NULL, 0) == -1) {
			/* TODO: set error number */
			return -1;
		}
		lsp->output_func = XP_NULL;
		lsp->output_arg = XP_NULL;
	}

	return 0;
}
