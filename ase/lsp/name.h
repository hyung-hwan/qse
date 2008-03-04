/*
 * $Id: name.h 117 2008-03-03 11:20:05Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_LSP_NAME_H_
#define _ASE_LSP_NAME_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

struct ase_lsp_name_t 
{
	ase_size_t  capa;
	ase_size_t  size;
	ase_char_t* buf;
	ase_char_t  static_buf[128];
	ase_lsp_t*  lsp;
	ase_bool_t __dynamic;
};

typedef struct ase_lsp_name_t ase_lsp_name_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_name_t* ase_lsp_name_open (
	ase_lsp_name_t* name, ase_size_t capa, ase_lsp_t* lsp);
void ase_lsp_name_close (ase_lsp_name_t* name);

int ase_lsp_name_addc (ase_lsp_name_t* name, ase_cint_t c);
int ase_lsp_name_adds (ase_lsp_name_t* name, const ase_char_t* s);
void ase_lsp_name_clear (ase_lsp_name_t* name);
int ase_lsp_name_compare (ase_lsp_name_t* name, const ase_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
