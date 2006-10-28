/*
 * $Id: lsp.c,v 1.12 2006-10-28 16:24:40 bacon Exp $
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

#define ADD_PRIM(mem,name,prim) \
	if (ase_lsp_add_prim(mem,name,prim) == -1) return -1;

	ADD_PRIM (lsp, ASE_T("abort"), ase_lsp_prim_abort);
	ADD_PRIM (lsp, ASE_T("eval"),  ase_lsp_prim_eval);
	ADD_PRIM (lsp, ASE_T("prog1"), ase_lsp_prim_prog1);
	ADD_PRIM (lsp, ASE_T("progn"), ase_lsp_prim_progn);
	ADD_PRIM (lsp, ASE_T("gc"),    ase_lsp_prim_gc);

	ADD_PRIM (lsp, ASE_T("cond"),  ase_lsp_prim_cond);
	ADD_PRIM (lsp, ASE_T("if"),    ase_lsp_prim_if);
	ADD_PRIM (lsp, ASE_T("while"), ase_lsp_prim_while);

	ADD_PRIM (lsp, ASE_T("car"),   ase_lsp_prim_car);
	ADD_PRIM (lsp, ASE_T("cdr"),   ase_lsp_prim_cdr);
	ADD_PRIM (lsp, ASE_T("cons"),  ase_lsp_prim_cons);
	ADD_PRIM (lsp, ASE_T("set"),   ase_lsp_prim_set);
	ADD_PRIM (lsp, ASE_T("setq"),  ase_lsp_prim_setq);
	ADD_PRIM (lsp, ASE_T("quote"), ase_lsp_prim_quote);
	ADD_PRIM (lsp, ASE_T("defun"), ase_lsp_prim_defun);
	ADD_PRIM (lsp, ASE_T("demac"), ase_lsp_prim_demac);
	ADD_PRIM (lsp, ASE_T("let"),   ase_lsp_prim_let);
	ADD_PRIM (lsp, ASE_T("let*"),  ase_lsp_prim_letx);

	ADD_PRIM (lsp, ASE_T("="),     ase_lsp_prim_eq);
	ADD_PRIM (lsp, ASE_T("/="),    ase_lsp_prim_ne);
	ADD_PRIM (lsp, ASE_T(">"),     ase_lsp_prim_gt);
	ADD_PRIM (lsp, ASE_T("<"),     ase_lsp_prim_lt);
	ADD_PRIM (lsp, ASE_T(">="),    ase_lsp_prim_ge);
	ADD_PRIM (lsp, ASE_T("<="),    ase_lsp_prim_le);

	ADD_PRIM (lsp, ASE_T("+"),     ase_lsp_prim_plus);
	ADD_PRIM (lsp, ASE_T("-"),     ase_lsp_prim_minus);
	ADD_PRIM (lsp, ASE_T("*"),     ase_lsp_prim_multiply);
	ADD_PRIM (lsp, ASE_T("/"),     ase_lsp_prim_divide);
	ADD_PRIM (lsp, ASE_T("%"),     ase_lsp_prim_modulus);

	return 0;
}


