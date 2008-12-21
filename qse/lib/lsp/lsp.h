/*
 * $Id: lsp_i.h 332 2008-08-18 11:21:48Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LIB_LSP_LSP_H_
#define _QSE_LIB_LSP_LSP_H_

#include "../cmn/mem.h"
#include "../cmn/chr.h"
#include <qse/cmn/str.h>

#include <qse/lsp/lsp.h>
#include "env.h"
#include "obj.h"
#include "mem.h"
#include "misc.h"
#include "prim.h"
#include "name.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

#define QSE_LSP_ALLOC(lsp,size)       QSE_MMGR_ALLOC(&(lsp)->prmfns.mmgr,size)
#define QSE_LSP_REALLOC(lsp,ptr,size) QSE_MMGR_REALLOC(&(lsp)->prmfns.mmgr,ptr,size)
#define QSE_LSP_FREE(lsp,ptr)         QSE_MMGR_FREE(&(lsp)->prmfns.mmgr,ptr)

#define QSE_LSP_ISUPPER(lsp,c)  QSE_CCLS_ISUPPER(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISLOWER(lsp,c)  QSE_CCLS_ISLOWER(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISALPHA(lsp,c)  QSE_CCLS_ISALPHA(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISDIGIT(lsp,c)  QSE_CCLS_ISDIGIT(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISXDIGIT(lsp,c) QSE_CCLS_ISXDIGIT(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISALNUM(lsp,c)  QSE_CCLS_ISALNUM(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISSPACE(lsp,c)  QSE_CCLS_ISSPACE(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISPRINT(lsp,c)  QSE_CCLS_ISPRINT(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISGRAPH(lsp,c)  QSE_CCLS_ISGRAPH(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISCNTRL(lsp,c)  QSE_CCLS_ISCNTRL(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_ISPUNCT(lsp,c)  QSE_CCLS_ISPUNCT(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_TOUPPER(lsp,c)  QSE_CCLS_TOUPPER(&(lsp)->prmfns.ccls,c)
#define QSE_LSP_TOLOWER(lsp,c)  QSE_CCLS_TOLOWER(&(lsp)->prmfns.ccls,c)

struct qse_lsp_t 
{
	qse_lsp_prmfns_t prmfns;
	/* user-specified data */
	void* assoc_data;

	/* error */
	int errnum;
	qse_char_t errmsg[256];

	/* options */
	int opt_undef_symbol;

	/* for read */
	qse_cint_t curc;
	struct
	{
		int type;
		qse_long_t ival;
		qse_real_t rval;
		qse_lsp_name_t name;
	} token;

	/* io functions */
	qse_lsp_io_t input_func;
	qse_lsp_io_t output_func;
	void* input_arg;
	void* output_arg;

	/* security options */
	qse_size_t max_eval_depth;
	qse_size_t cur_eval_depth;

	/* memory manager */
	qse_lsp_mem_t* mem;
};

#endif
