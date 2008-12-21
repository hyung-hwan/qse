/*
 * $Id: lsp.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#if defined(__BORLANDC__)
#pragma hdrstop
#define Library
#endif

#include "lsp.h"

static int __add_builtin_prims (qse_lsp_t* lsp);

qse_lsp_t* qse_lsp_open (
	const qse_lsp_prmfns_t* prmfns, 
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc)
{
	qse_lsp_t* lsp;

	if (prmfns == QSE_NULL) return QSE_NULL;

/*
	if (prmfns->mmgr.malloc == QSE_NULL || 
	    prmfns->mmgr.realloc == QSE_NULL || 
	    prmfns->mmgr.free == QSE_NULL) return QSE_NULL;

	if (prmfns->ccls.is_upper  == QSE_NULL ||
	    prmfns->ccls.is_lower  == QSE_NULL ||
	    prmfns->ccls.is_alpha  == QSE_NULL ||
	    prmfns->ccls.is_digit  == QSE_NULL ||
	    prmfns->ccls.is_xdigit == QSE_NULL ||
	    prmfns->ccls.is_alnum  == QSE_NULL ||
	    prmfns->ccls.is_space  == QSE_NULL ||
	    prmfns->ccls.is_print  == QSE_NULL ||
	    prmfns->ccls.is_graph  == QSE_NULL ||
	    prmfns->ccls.is_cntrl  == QSE_NULL ||
	    prmfns->ccls.is_punct  == QSE_NULL ||
	    prmfns->ccls.to_upper  == QSE_NULL ||
	    prmfns->ccls.to_lower  == QSE_NULL) return QSE_NULL;
*/

	if (prmfns->misc.sprintf == QSE_NULL || 
	    prmfns->misc.dprintf == QSE_NULL) return QSE_NULL;

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
	lsp = (qse_lsp_t*) malloc (QSE_SIZEOF(qse_lsp_t));
#else
	lsp = (qse_lsp_t*) prmfns->mmgr.alloc (
		prmfns->mmgr.data, QSE_SIZEOF(qse_lsp_t));
#endif
	if (lsp == QSE_NULL) return QSE_NULL;

	/* it uses the built-in qse_lsp_memset because lsp is not 
	 * fully initialized yet */
	QSE_MEMSET (lsp, 0, QSE_SIZEOF(qse_lsp_t));
	QSE_MEMCPY (&lsp->prmfns, prmfns, QSE_SIZEOF(lsp->prmfns));
	lsp->assoc_data = QSE_NULL;

	if (qse_lsp_name_open(&lsp->token.name, 0, lsp) == QSE_NULL) 
	{
		QSE_LSP_FREE (lsp, lsp);
		return QSE_NULL;
	}

	lsp->errnum = QSE_LSP_ENOERR;
	lsp->errmsg[0] = QSE_T('\0');
	lsp->opt_undef_symbol = 1;
	/*lsp->opt_undef_symbol = 0;*/

	lsp->curc = QSE_CHAR_EOF;
	lsp->input_func = QSE_NULL;
	lsp->output_func = QSE_NULL;
	lsp->input_arg = QSE_NULL;
	lsp->output_arg = QSE_NULL;

	lsp->mem = qse_lsp_openmem (lsp, mem_ubound, mem_ubound_inc);
	if (lsp->mem == QSE_NULL) 
	{
		qse_lsp_name_close (&lsp->token.name);
		QSE_LSP_FREE (lsp, lsp);
		return QSE_NULL;
	}

	if (__add_builtin_prims(lsp) == -1) 
	{
		qse_lsp_closemem (lsp->mem);
		qse_lsp_name_close (&lsp->token.name);
		QSE_LSP_FREE (lsp, lsp);
		return QSE_NULL;
	}

	lsp->max_eval_depth = 0; /* TODO: put restriction here.... */
	lsp->cur_eval_depth = 0;

	return lsp;
}

void qse_lsp_close (qse_lsp_t* lsp)
{
	qse_lsp_closemem (lsp->mem);
	qse_lsp_name_close (&lsp->token.name);
	QSE_LSP_FREE (lsp, lsp);
}

void qse_lsp_setassocdata (qse_lsp_t* lsp, void* data)
{
	lsp->assoc_data = data;
}

void* qse_lsp_getassocdata (qse_lsp_t* lsp)
{
	return lsp->assoc_data;
}

int qse_lsp_attinput (qse_lsp_t* lsp, qse_lsp_io_t input, void* arg)
{
	if (qse_lsp_detinput(lsp) == -1) return -1;

	QSE_ASSERT (lsp->input_func == QSE_NULL);

	if (input(QSE_LSP_IO_OPEN, arg, QSE_NULL, 0) == -1) 
	{
		/* TODO: set error number */
		return -1;
	}

	lsp->input_func = input;
	lsp->input_arg = arg;
	lsp->curc = QSE_CHAR_EOF;
	return 0;
}

int qse_lsp_detinput (qse_lsp_t* lsp)
{
	if (lsp->input_func != QSE_NULL) 
	{
		if (lsp->input_func (
			QSE_LSP_IO_CLOSE, lsp->input_arg, QSE_NULL, 0) == -1) 
		{
			/* TODO: set error number */
			return -1;
		}
		lsp->input_func = QSE_NULL;
		lsp->input_arg = QSE_NULL;
		lsp->curc = QSE_CHAR_EOF;
	}

	return 0;
}

int qse_lsp_attoutput (qse_lsp_t* lsp, qse_lsp_io_t output, void* arg)
{
	if (qse_lsp_detoutput(lsp) == -1) return -1;

	QSE_ASSERT (lsp->output_func == QSE_NULL);

	if (output(QSE_LSP_IO_OPEN, arg, QSE_NULL, 0) == -1) 
	{
		/* TODO: set error number */
		return -1;
	}
	lsp->output_func = output;
	lsp->output_arg = arg;
	return 0;
}

int qse_lsp_detoutput (qse_lsp_t* lsp)
{
	if (lsp->output_func != QSE_NULL) 
	{
		if (lsp->output_func (
			QSE_LSP_IO_CLOSE, lsp->output_arg, QSE_NULL, 0) == -1) 
		{
			/* TODO: set error number */
			return -1;
		}
		lsp->output_func = QSE_NULL;
		lsp->output_arg = QSE_NULL;
	}

	return 0;
}

static int __add_builtin_prims (qse_lsp_t* lsp)
{

#define ADD_PRIM(mem,name,name_len,pimpl,min_args,max_args) \
	if (qse_lsp_addprim(mem,name,name_len,pimpl,min_args,max_args) == -1) return -1;
#define MAX_ARGS QSE_TYPE_MAX(qse_size_t)

	ADD_PRIM (lsp, QSE_T("exit"),   4, qse_lsp_prim_exit,   0, 0);
	ADD_PRIM (lsp, QSE_T("eval"),   4, qse_lsp_prim_eval,   1, 1);
	ADD_PRIM (lsp, QSE_T("prog1"),  5, qse_lsp_prim_prog1,  1, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("progn"),  5, qse_lsp_prim_progn,  1, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("gc"),     2, qse_lsp_prim_gc,     0, 0);

	ADD_PRIM (lsp, QSE_T("cond"),   4, qse_lsp_prim_cond,   0, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("if"),     2, qse_lsp_prim_if,     2, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("while"),  5, qse_lsp_prim_while,  1, MAX_ARGS);

	ADD_PRIM (lsp, QSE_T("car"),    3, qse_lsp_prim_car,    1, 1);
	ADD_PRIM (lsp, QSE_T("cdr"),    3, qse_lsp_prim_cdr,    1, 1);
	ADD_PRIM (lsp, QSE_T("cons"),   4, qse_lsp_prim_cons,   2, 2);
	ADD_PRIM (lsp, QSE_T("length"), 6, qse_lsp_prim_length, 1, 1);

	ADD_PRIM (lsp, QSE_T("set"),    3, qse_lsp_prim_set,    2, 2);
	ADD_PRIM (lsp, QSE_T("setq"),   4, qse_lsp_prim_setq,   1, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("quote"),  5, qse_lsp_prim_quote,  1, 1);
	ADD_PRIM (lsp, QSE_T("defun"),  5, qse_lsp_prim_defun,  3, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("demac"),  5, qse_lsp_prim_demac,  3, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("let"),    3, qse_lsp_prim_let,    1, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("let*"),   4, qse_lsp_prim_letx,   1, MAX_ARGS);
	/*ADD_PRIM (lsp, QSE_T("or"),     2, qse_lsp_prim_or,     2, MAX_ARGS);*/

	ADD_PRIM (lsp, QSE_T("="),     1, qse_lsp_prim_eq,    2, 2);
	ADD_PRIM (lsp, QSE_T("/="),    2, qse_lsp_prim_ne,    2, 2);
	ADD_PRIM (lsp, QSE_T(">"),     1, qse_lsp_prim_gt,    2, 2);
	ADD_PRIM (lsp, QSE_T("<"),     1, qse_lsp_prim_lt,    2, 2);
	ADD_PRIM (lsp, QSE_T(">="),    2, qse_lsp_prim_ge,    2, 2);
	ADD_PRIM (lsp, QSE_T("<="),    2, qse_lsp_prim_le,    2, 2);

	ADD_PRIM (lsp, QSE_T("+"),     1, qse_lsp_prim_plus,  1, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("-"),     1, qse_lsp_prim_minus, 1, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("*"),     1, qse_lsp_prim_mul,   1, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("/"),     1, qse_lsp_prim_div,   1, MAX_ARGS);
	ADD_PRIM (lsp, QSE_T("%"),     1, qse_lsp_prim_mod  , 1, MAX_ARGS);

	return 0;
}
