/*
 * $Id: lsp.c 337 2008-08-20 09:17:25Z baconevi $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "lsp.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (lsp)

static qse_lsp_t* qse_lsp_init (
	qse_lsp_t* lsp, qse_mmgr_t* mmgr, const qse_lsp_prm_t* prm,
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc);
static void qse_lsp_fini (qse_lsp_t* lsp);

static int __add_builtin_prims (qse_lsp_t* lsp);

qse_lsp_t* qse_lsp_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, const qse_lsp_prm_t* prm, 
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc)
{
	qse_lsp_t* lsp;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	lsp = (qse_lsp_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_lsp_t) + xtnsize);
	if (lsp == QSE_NULL) return QSE_NULL;

	if (qse_lsp_init (lsp, mmgr, prm, mem_ubound, mem_ubound_inc) == QSE_NULL)
	{
		QSE_MMGR_FREE (lsp->mmgr, lsp);
		return QSE_NULL;
	}

	return lsp;
}

void qse_lsp_close (qse_lsp_t* lsp)
{
	qse_lsp_fini (lsp);
	QSE_LSP_FREE (lsp, lsp);
}

static qse_lsp_t* qse_lsp_init (
	qse_lsp_t* lsp, qse_mmgr_t* mmgr, const qse_lsp_prm_t* prm,
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc)
{
	QSE_MEMSET (lsp, 0, QSE_SIZEOF(*lsp));

	lsp->mmgr = mmgr;
	lsp->errstr = qse_lsp_dflerrstr;
	lsp->prm = *prm;

	if (qse_str_init(&lsp->token.name, mmgr, 256) == QSE_NULL) 
	{
		QSE_LSP_FREE (lsp, lsp);
		return QSE_NULL;
	}

	lsp->errnum = QSE_LSP_ENOERR;
	lsp->errmsg[0] = QSE_T('\0');
	lsp->opt_undef_symbol = 1;
	/*lsp->opt_undef_symbol = 0;*/

	lsp->curc = QSE_CHAR_EOF;
	lsp->curloc.line = 1;
	lsp->curloc.colm = 0;

	lsp->mem = qse_lsp_openmem (lsp, mem_ubound, mem_ubound_inc);
	if (lsp->mem == QSE_NULL) 
	{
		qse_str_fini (&lsp->token.name);
		QSE_LSP_FREE (lsp, lsp);
		return QSE_NULL;
	}

	if (__add_builtin_prims(lsp) == -1) 
	{
		qse_lsp_closemem (lsp->mem);
		qse_str_fini (&lsp->token.name);
		QSE_LSP_FREE (lsp, lsp);
		return QSE_NULL;
	}

	lsp->max_eval_depth = 0; /* TODO: put restriction here.... */
	lsp->cur_eval_depth = 0;

	return lsp;
}

static void qse_lsp_fini (qse_lsp_t* lsp)
{
	qse_lsp_closemem (lsp->mem);
	qse_str_fini (&lsp->token.name);
}

void qse_lsp_detachio (qse_lsp_t* lsp)
{
	if (lsp->io.fns.out)
	{
		lsp->io.fns.out (lsp, QSE_LSP_IO_CLOSE, &lsp->io.arg.out, QSE_NULL, 0);
		lsp->io.fns.out = QSE_NULL;
	}
	if (lsp->io.fns.in)
	{
		lsp->io.fns.in (lsp, QSE_LSP_IO_CLOSE, &lsp->io.arg.in, QSE_NULL, 0);
		lsp->io.fns.in = QSE_NULL;
		lsp->curc = QSE_CHAR_EOF; /* TODO: needed??? */
	}
}

int qse_lsp_attachio (qse_lsp_t* lsp, qse_lsp_io_t* io)
{
	qse_lsp_detachio(lsp);

	QSE_ASSERT (lsp->io.fns.in == QSE_NULL);
	QSE_ASSERT (lsp->io.fns.out == QSE_NULL);

	if (io->in (lsp, QSE_LSP_IO_OPEN, &lsp->io.arg.in, QSE_NULL, 0) <= -1)
	{
		/* TODO: error code if error not set... */
		return -1;
	}

	if (io->out (lsp, QSE_LSP_IO_OPEN, &lsp->io.arg.out, QSE_NULL, 0) <= -1)
	{
		/* TODO: error code if error not set... */
		io->in (lsp, QSE_LSP_IO_CLOSE, &lsp->io.arg.in, QSE_NULL, 0);
		return -1;
	}

	lsp->io.fns = *io;
	lsp->curc = QSE_CHAR_EOF;
	lsp->curloc.line = 1;
	lsp->curloc.colm = 0;

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
	ADD_PRIM (lsp, QSE_T("macro"),  5, qse_lsp_prim_demac,  3, MAX_ARGS);
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
