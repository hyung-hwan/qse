/*
 * $Id: name.h,v 1.5 2006-10-24 04:22:39 bacon Exp $
 */

#ifndef _ASE_LSP_NAME_H_
#define _ASE_LSP_NAME_H_

#include <ase/types.h>
#include <ase/macros.h>

struct ase_lsp_name_t 
{
	ase_word_t capacity;
	ase_word_t size;
	ase_char_t* buffer;
	ase_char_t static_buffer[128];
	ase_bool_t __dynamic;
};

typedef struct ase_lsp_name_t ase_lsp_name_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_name_t* ase_lsp_name_open (
	ase_lsp_name_t* name, ase_word_t capacity);
void ase_lsp_name_close (ase_lsp_name_t* name);

int ase_lsp_name_addc (ase_lsp_name_t* name, ase_cint_t c);
int ase_lsp_name_adds (ase_lsp_name_t* name, const ase_char_t* s);
void ase_lsp_name_clear (ase_lsp_name_t* name);
ase_char_t* ase_lsp_name_yield (ase_lsp_name_t* name, ase_word_t capacity);
int ase_lsp_name_compare (ase_lsp_name_t* name, const ase_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
