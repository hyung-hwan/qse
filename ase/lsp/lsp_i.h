/*
 * $Id: lsp_i.h,v 1.6 2007-02-01 08:49:52 bacon Exp $
 */

#ifndef _ASE_LSP_LSPI_H_
#define _ASE_LSP_LSPI_H_

#include <ase/lsp/lsp.h>
#include <ase/lsp/env.h>
#include <ase/lsp/obj.h>
#include <ase/lsp/mem.h>
#include <ase/lsp/misc.h>
#include <ase/lsp/prim.h>
#include <ase/lsp/name.h>

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>

	#define ASE_LSP_MALLOC(lsp,size) malloc (size)
	#define ASE_LSP_REALLOC(lsp,ptr,size) realloc (ptr, size)
	#define ASE_LSP_FREE(lsp,ptr) free (ptr)
#else
	#define ASE_LSP_MALLOC(lsp,size) \
		(lsp)->prmfns.malloc (size, (lsp)->prmfns.custom_data)
	#define ASE_LSP_REALLOC(lsp,ptr,size) \
		(lsp)->prmfns.realloc (ptr, size, (lsp)->prmfns.custom_data)
	#define ASE_LSP_FREE(lsp,ptr) \
		(lsp)->prmfns.free (ptr, (lsp)->prmfns.custom_data)
#endif

#define ASE_LSP_ISUPPER(lsp,c)  (lsp)->prmfns.is_upper(c)
#define ASE_LSP_ISLOWER(lsp,c)  (lsp)->prmfns.is_lower(c)
#define ASE_LSP_ISALPHA(lsp,c)  (lsp)->prmfns.is_alpha(c)
#define ASE_LSP_ISDIGIT(lsp,c)  (lsp)->prmfns.is_digit(c)
#define ASE_LSP_ISXDIGIT(lsp,c) (lsp)->prmfns.is_xdigit(c)
#define ASE_LSP_ISALNUM(lsp,c)  (lsp)->prmfns.is_alnum(c)
#define ASE_LSP_ISSPACE(lsp,c)  (lsp)->prmfns.is_space(c)
#define ASE_LSP_ISPRINT(lsp,c)  (lsp)->prmfns.is_print(c)
#define ASE_LSP_ISGRAPH(lsp,c)  (lsp)->prmfns.is_graph(c)
#define ASE_LSP_ISCNTRL(lsp,c)  (lsp)->prmfns.is_cntrl(c)
#define ASE_LSP_ISPUNCT(lsp,c)  (lsp)->prmfns.is_punct(c)
#define ASE_LSP_TOUPPER(lsp,c)  (lsp)->prmfns.to_upper(c)
#define ASE_LSP_TOLOWER(lsp,c)  (lsp)->prmfns.to_lower(c)

#define ASE_LSP_MEMCPY(lsp,dst,src,len) (lsp)->prmfns.memcpy (dst, src, len)
#define ASE_LSP_MEMSET(lsp,dst,val,len) (lsp)->prmfns.memset (dst, val, len)

struct ase_lsp_t 
{
	ase_lsp_prmfns_t prmfns;

	/* error number */
	int errnum;
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
