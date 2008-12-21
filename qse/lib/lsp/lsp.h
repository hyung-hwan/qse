/*
 * $Id: lsp_i.h 332 2008-08-18 11:21:48Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_LIB_LSP_LSP_H_
#define _ASE_LIB_LSP_LSP_H_

#include "../cmn/mem.h"
#include "../cmn/chr.h"
#include <ase/cmn/str.h>

#include <ase/lsp/lsp.h>
#include "env.h"
#include "obj.h"
#include "mem.h"
#include "misc.h"
#include "prim.h"
#include "name.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

#define ASE_LSP_ALLOC(lsp,size)       ASE_MMGR_ALLOC(&(lsp)->prmfns.mmgr,size)
#define ASE_LSP_REALLOC(lsp,ptr,size) ASE_MMGR_REALLOC(&(lsp)->prmfns.mmgr,ptr,size)
#define ASE_LSP_FREE(lsp,ptr)         ASE_MMGR_FREE(&(lsp)->prmfns.mmgr,ptr)

#define ASE_LSP_ISUPPER(lsp,c)  ASE_CCLS_ISUPPER(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISLOWER(lsp,c)  ASE_CCLS_ISLOWER(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISALPHA(lsp,c)  ASE_CCLS_ISALPHA(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISDIGIT(lsp,c)  ASE_CCLS_ISDIGIT(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISXDIGIT(lsp,c) ASE_CCLS_ISXDIGIT(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISALNUM(lsp,c)  ASE_CCLS_ISALNUM(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISSPACE(lsp,c)  ASE_CCLS_ISSPACE(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISPRINT(lsp,c)  ASE_CCLS_ISPRINT(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISGRAPH(lsp,c)  ASE_CCLS_ISGRAPH(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISCNTRL(lsp,c)  ASE_CCLS_ISCNTRL(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_ISPUNCT(lsp,c)  ASE_CCLS_ISPUNCT(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_TOUPPER(lsp,c)  ASE_CCLS_TOUPPER(&(lsp)->prmfns.ccls,c)
#define ASE_LSP_TOLOWER(lsp,c)  ASE_CCLS_TOLOWER(&(lsp)->prmfns.ccls,c)

struct ase_lsp_t 
{
	ase_lsp_prmfns_t prmfns;
	/* user-specified data */
	void* assoc_data;

	/* error */
	int errnum;
	ase_char_t errmsg[256];

	/* options */
	int opt_undef_symbol;

	/* for read */
	ase_cint_t curc;
	struct
	{
		int type;
		ase_long_t ival;
		ase_real_t rval;
		ase_lsp_name_t name;
	} token;

	/* io functions */
	ase_lsp_io_t input_func;
	ase_lsp_io_t output_func;
	void* input_arg;
	void* output_arg;

	/* security options */
	ase_size_t max_eval_depth;
	ase_size_t cur_eval_depth;

	/* memory manager */
	ase_lsp_mem_t* mem;
};

#endif
