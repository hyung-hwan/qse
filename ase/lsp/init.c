/*
 * $Id: init.c,v 1.10 2005-09-21 11:52:36 bacon Exp $
 */

#include <xp/lsp/lsp.h>
#include <xp/lsp/prim.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>

static int __add_builtin_prims (xp_lsp_t* lsp);

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
	lsp->opt_undef_symbol = 1;
	//lsp->opt_undef_symbol = 0;

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

	if (__add_builtin_prims(lsp) == -1) {
		xp_lsp_mem_free (lsp->mem);
		xp_lsp_token_close (&lsp->token);
		if (lsp->__malloced) xp_free (lsp);
		return XP_NULL;
	}

	lsp->max_eval_depth = 0; // TODO: put restriction here....
	lsp->cur_eval_depth = 0;

	return lsp;
}

void xp_lsp_close (xp_lsp_t* lsp)
{
	xp_assert (lsp != XP_NULL);
	xp_lsp_mem_free (lsp->mem);
	xp_lsp_token_close (&lsp->token);
	if (lsp->__malloced) xp_free (lsp);
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
	lsp->curc = XP_CHAR_EOF;
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
		lsp->curc = XP_CHAR_EOF;
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

static int __add_builtin_prims (xp_lsp_t* lsp)
{

#define ADD_PRIM(mem,name,prim) \
	if (xp_lsp_add_prim(mem,name,prim) == -1) return -1;

	ADD_PRIM (lsp, XP_TEXT("abort"), xp_lsp_prim_abort);
	ADD_PRIM (lsp, XP_TEXT("eval"),  xp_lsp_prim_eval);
	ADD_PRIM (lsp, XP_TEXT("prog1"), xp_lsp_prim_prog1);
	ADD_PRIM (lsp, XP_TEXT("progn"), xp_lsp_prim_progn);
	ADD_PRIM (lsp, XP_TEXT("gc"),    xp_lsp_prim_gc);

	ADD_PRIM (lsp, XP_TEXT("cond"),  xp_lsp_prim_cond);
	ADD_PRIM (lsp, XP_TEXT("if"),    xp_lsp_prim_if);
	ADD_PRIM (lsp, XP_TEXT("while"), xp_lsp_prim_while);

	ADD_PRIM (lsp, XP_TEXT("car"),   xp_lsp_prim_car);
	ADD_PRIM (lsp, XP_TEXT("cdr"),   xp_lsp_prim_cdr);
	ADD_PRIM (lsp, XP_TEXT("cons"),  xp_lsp_prim_cons);
	ADD_PRIM (lsp, XP_TEXT("set"),   xp_lsp_prim_set);
	ADD_PRIM (lsp, XP_TEXT("setq"),  xp_lsp_prim_setq);
	ADD_PRIM (lsp, XP_TEXT("quote"), xp_lsp_prim_quote);
	ADD_PRIM (lsp, XP_TEXT("defun"), xp_lsp_prim_defun);
	ADD_PRIM (lsp, XP_TEXT("demac"), xp_lsp_prim_demac);
	ADD_PRIM (lsp, XP_TEXT("let"),   xp_lsp_prim_let);
	ADD_PRIM (lsp, XP_TEXT("let*"),  xp_lsp_prim_letx);

	ADD_PRIM (lsp, XP_TEXT("="),     xp_lsp_prim_eq);
	ADD_PRIM (lsp, XP_TEXT("/="),    xp_lsp_prim_ne);
	ADD_PRIM (lsp, XP_TEXT(">"),     xp_lsp_prim_gt);
	ADD_PRIM (lsp, XP_TEXT("<"),     xp_lsp_prim_lt);
	ADD_PRIM (lsp, XP_TEXT(">="),    xp_lsp_prim_ge);
	ADD_PRIM (lsp, XP_TEXT("<="),    xp_lsp_prim_le);

	ADD_PRIM (lsp, XP_TEXT("+"),     xp_lsp_prim_plus);
	ADD_PRIM (lsp, XP_TEXT("-"),     xp_lsp_prim_minus);
	ADD_PRIM (lsp, XP_TEXT("*"),     xp_lsp_prim_multiply);
	ADD_PRIM (lsp, XP_TEXT("/"),     xp_lsp_prim_divide);
	ADD_PRIM (lsp, XP_TEXT("%"),     xp_lsp_prim_modulus);

	return 0;
}
