/*
 * $Id: lsp.h 332 2008-08-18 11:21:48Z baconevi $
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

#ifndef _QSE_LIB_LSP_LSP_H_
#define _QSE_LIB_LSP_LSP_H_

#include "../cmn/mem.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>

#include <qse/lsp/lsp.h>
#include "env.h"
#include "obj.h"
#include "mem.h"
#include "misc.h"
#include "prim.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

#define QSE_LSP_ALLOC(lsp,size)       QSE_MMGR_ALLOC((lsp)->mmgr,size)
#define QSE_LSP_REALLOC(lsp,ptr,size) QSE_MMGR_REALLOC((lsp)->mmgr,ptr,size)
#define QSE_LSP_FREE(lsp,ptr)         QSE_MMGR_FREE((lsp)->mmgr,ptr)

#define QSE_LSP_ISUPPER(lsp,c)  QSE_ISUPPER(c)
#define QSE_LSP_ISLOWER(lsp,c)  QSE_ISLOWER(c)
#define QSE_LSP_ISALPHA(lsp,c)  QSE_ISALPHA(c)
#define QSE_LSP_ISDIGIT(lsp,c)  QSE_ISDIGIT(c)
#define QSE_LSP_ISXDIGIT(lsp,c) QSE_ISXDIGIT(c)
#define QSE_LSP_ISALNUM(lsp,c)  QSE_ISALNUM(c)
#define QSE_LSP_ISSPACE(lsp,c)  QSE_ISSPACE(c)
#define QSE_LSP_ISPRINT(lsp,c)  QSE_ISPRINT(c)
#define QSE_LSP_ISGRAPH(lsp,c)  QSE_ISGRAPH(c)
#define QSE_LSP_ISCNTRL(lsp,c)  QSE_ISCNTRL(c)
#define QSE_LSP_ISPUNCT(lsp,c)  QSE_ISPUNCT(c)
#define QSE_LSP_TOUPPER(lsp,c)  QSE_TOUPPER(c)
#define QSE_LSP_TOLOWER(lsp,c)  QSE_TOLOWER(c)

struct qse_lsp_t 
{
	QSE_DEFINE_COMMON_FIELDS (lsp)

	qse_lsp_prm_t prm;

	qse_lsp_errstr_t errstr; /**< error string getter */
	qse_lsp_errnum_t errnum; /**< stores an error number */
	qse_char_t errmsg[128];  /**< error message holder */
	qse_lsp_loc_t errloc;    /**< location of the last error */

	/* options */
	int opt_undef_symbol;

	/* for read */
	qse_cint_t curc; 
	qse_lsp_loc_t curloc;

	struct
	{
		int           type;
		qse_lsp_loc_t loc;
		qse_long_t    ival;
		qse_real_t    rval;
		qse_str_t     name;
	} token;

	/* io function */
	struct
	{
		qse_lsp_io_t fns;

		struct
		{
			qse_lsp_io_arg_t in;
			qse_lsp_io_arg_t out;
		} arg;
	} io;

	/* security options */
	qse_size_t max_eval_depth;
	qse_size_t cur_eval_depth;

	/* memory manager */
	qse_lsp_mem_t* mem;
};

#ifdef __cplusplus
extern "C" {
#endif

const qse_char_t* qse_lsp_dflerrstr (qse_lsp_t* lsp, qse_lsp_errnum_t errnum);

#ifdef __cplusplus
}
#endif
#endif
