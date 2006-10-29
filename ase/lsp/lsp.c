/*
 * $Id: lsp.c,v 1.14 2006-10-29 13:40:31 bacon Exp $
 */

#if defined(__BORLANDC__)
#pragma hdrstop
#define Library
#endif

#include <ase/lsp/lsp_i.h>

static int __add_builtin_prims (ase_lsp_t* lsp);

ase_lsp_t* ase_lsp_open (
	const ase_lsp_syscas_t* syscas, 
	ase_size_t mem_ubound, ase_size_t mem_ubound_inc)
{
	ase_lsp_t* lsp;

	if (syscas == ASE_NULL) return ASE_NULL;

	if (syscas->malloc == ASE_NULL || 
	    syscas->realloc == ASE_NULL || 
	    syscas->free == ASE_NULL) return ASE_NULL;

	if (syscas->is_upper  == ASE_NULL ||
	    syscas->is_lower  == ASE_NULL ||
	    syscas->is_alpha  == ASE_NULL ||
	    syscas->is_digit  == ASE_NULL ||
	    syscas->is_xdigit == ASE_NULL ||
	    syscas->is_alnum  == ASE_NULL ||
	    syscas->is_space  == ASE_NULL ||
	    syscas->is_print  == ASE_NULL ||
	    syscas->is_graph  == ASE_NULL ||
	    syscas->is_cntrl  == ASE_NULL ||
	    syscas->is_punct  == ASE_NULL ||
	    syscas->to_upper  == ASE_NULL ||
	    syscas->to_lower  == ASE_NULL) return ASE_NULL;

	if (syscas->sprintf == ASE_NULL || 
	    syscas->aprintf == ASE_NULL || 
	    syscas->dprintf == ASE_NULL || 
	    syscas->abort == ASE_NULL) return ASE_NULL;

#if defined(_WIN32) && defined(_DEBUG)
	lsp = (ase_lsp_t*) malloc (ase_sizeof(ase_lsp_t));
#else
	lsp = (ase_lsp_t*) syscas->malloc (
		ase_sizeof(ase_lsp_t), syscas->custom_data);
#endif
	if (lsp == ASE_NULL) return ASE_NULL;

	/* it uses the built-in ase_lsp_memset because lsp is not 
	 * fully initialized yet */
	ase_lsp_memset (lsp, 0, ase_sizeof(ase_lsp_t));

	if (syscas->memcpy == ASE_NULL)
	{
		ase_lsp_memcpy (&lsp->syscas, syscas, ase_sizeof(lsp->syscas));
		lsp->syscas.memcpy = ase_lsp_memcpy;
	}
	else syscas->memcpy (&lsp->syscas, syscas, ase_sizeof(lsp->syscas));
	if (syscas->memset == ASE_NULL) lsp->syscas.memset = ase_lsp_memset;

	if (ase_lsp_name_open(&lsp->token.name, 0, lsp) == ASE_NULL) 
	{
		ASE_LSP_FREE (lsp, lsp);
		return ASE_NULL;
	}

	lsp->errnum = ASE_LSP_ENOERR;
	lsp->opt_undef_symbol = 1;
	//lsp->opt_undef_symbol = 0;

	lsp->curc = ASE_CHAR_EOF;
	lsp->input_func = ASE_NULL;
	lsp->output_func = ASE_NULL;
	lsp->input_arg = ASE_NULL;
	lsp->output_arg = ASE_NULL;

	lsp->mem = ase_lsp_openmem (lsp, mem_ubound, mem_ubound_inc);
	if (lsp->mem == ASE_NULL) 
	{
		ase_lsp_name_close (&lsp->token.name);
		ASE_LSP_FREE (lsp, lsp);
		return ASE_NULL;
	}

	if (__add_builtin_prims(lsp) == -1) 
	{
		ase_lsp_closemem (lsp->mem);
		ase_lsp_name_close (&lsp->token.name);
		ASE_LSP_FREE (lsp, lsp);
		return ASE_NULL;
	}

	lsp->max_eval_depth = 0; // TODO: put restriction here....
	lsp->cur_eval_depth = 0;

	return lsp;
}

void ase_lsp_close (ase_lsp_t* lsp)
{
	ase_lsp_closemem (lsp->mem);
	ase_lsp_name_close (&lsp->token.name);
	ASE_LSP_FREE (lsp, lsp);
}

int ase_lsp_attach_input (ase_lsp_t* lsp, ase_lsp_io_t input, void* arg)
{
	if (ase_lsp_detach_input(lsp) == -1) return -1;

	ASE_LSP_ASSERT (lsp, lsp->input_func == ASE_NULL);

	if (input(ASE_LSP_IO_OPEN, arg, ASE_NULL, 0) == -1) {
		/* TODO: set error number */
		return -1;
	}

	lsp->input_func = input;
	lsp->input_arg = arg;
	lsp->curc = ASE_CHAR_EOF;
	return 0;
}

int ase_lsp_detach_input (ase_lsp_t* lsp)
{
	if (lsp->input_func != ASE_NULL) {
		if (lsp->input_func(ASE_LSP_IO_CLOSE, lsp->input_arg, ASE_NULL, 0) == -1) {
			/* TODO: set error number */
			return -1;
		}
		lsp->input_func = ASE_NULL;
		lsp->input_arg = ASE_NULL;
		lsp->curc = ASE_CHAR_EOF;
	}

	return 0;
}

int ase_lsp_attach_output (ase_lsp_t* lsp, ase_lsp_io_t output, void* arg)
{
	if (ase_lsp_detach_output(lsp) == -1) return -1;

	ASE_LSP_ASSERT (lsp, lsp->output_func == ASE_NULL);

	if (output(ASE_LSP_IO_OPEN, arg, ASE_NULL, 0) == -1) {
		/* TODO: set error number */
		return -1;
	}
	lsp->output_func = output;
	lsp->output_arg = arg;
	return 0;
}

int ase_lsp_detach_output (ase_lsp_t* lsp)
{
	if (lsp->output_func != ASE_NULL) {
		if (lsp->output_func(ASE_LSP_IO_CLOSE, lsp->output_arg, ASE_NULL, 0) == -1) {
			/* TODO: set error number */
			return -1;
		}
		lsp->output_func = ASE_NULL;
		lsp->output_arg = ASE_NULL;
	}

	return 0;
}

static int __add_builtin_prims (ase_lsp_t* lsp)
{

#define ADD_PRIM(mem,name,name_len,pimpl,min_args,max_args) \
	if (ase_lsp_addprim(mem,name,name_len,pimpl,min_args,max_args) == -1) return -1;

	ADD_PRIM (lsp, ASE_T("exit"),  4, ase_lsp_prim_exit,  0, 0);
	ADD_PRIM (lsp, ASE_T("eval"),  4, ase_lsp_prim_eval,  1, 1);
	ADD_PRIM (lsp, ASE_T("prog1"), 5, ase_lsp_prim_prog1, 1, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("progn"), 5, ase_lsp_prim_progn, 1, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("gc"),    2, ase_lsp_prim_gc,    0, 0);

	ADD_PRIM (lsp, ASE_T("cond"),  4, ase_lsp_prim_cond,  0, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("if"),    2, ase_lsp_prim_if,    2, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("while"), 5, ase_lsp_prim_while, 1, ASE_TYPE_MAX(ase_size_t));

	ADD_PRIM (lsp, ASE_T("car"),   3, ase_lsp_prim_car,   1, 1);
	ADD_PRIM (lsp, ASE_T("cdr"),   3, ase_lsp_prim_cdr,   1, 1);
	ADD_PRIM (lsp, ASE_T("cons"),  4, ase_lsp_prim_cons,  2, 2);
	ADD_PRIM (lsp, ASE_T("set"),   3, ase_lsp_prim_set,   2, 2);
	ADD_PRIM (lsp, ASE_T("setq"),  4, ase_lsp_prim_setq,  1, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("quote"), 5, ase_lsp_prim_quote, 1, 1);
	ADD_PRIM (lsp, ASE_T("defun"), 5, ase_lsp_prim_defun, 3, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("demac"), 5, ase_lsp_prim_demac, 3, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("let"),   3, ase_lsp_prim_let,   1, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("let*"),  4, ase_lsp_prim_letx,  1, ASE_TYPE_MAX(ase_size_t));

	ADD_PRIM (lsp, ASE_T("="),     1, ase_lsp_prim_eq,    2, 2);
	ADD_PRIM (lsp, ASE_T("/="),    2, ase_lsp_prim_ne,    2, 2);
	ADD_PRIM (lsp, ASE_T(">"),     1, ase_lsp_prim_gt,    2, 2);
	ADD_PRIM (lsp, ASE_T("<"),     1, ase_lsp_prim_lt,    2, 2);
	ADD_PRIM (lsp, ASE_T(">="),    2, ase_lsp_prim_ge,    2, 2);
	ADD_PRIM (lsp, ASE_T("<="),    2, ase_lsp_prim_le,    2, 2);

	ADD_PRIM (lsp, ASE_T("+"),     1, ase_lsp_prim_plus,  1, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("-"),     1, ase_lsp_prim_minus, 1, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("*"),     1, ase_lsp_prim_mul,   1, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("/"),     1, ase_lsp_prim_div,   1, ASE_TYPE_MAX(ase_size_t));
	ADD_PRIM (lsp, ASE_T("%"),     1, ase_lsp_prim_mod  , 1, ASE_TYPE_MAX(ase_size_t));

	return 0;
}


