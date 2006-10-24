/*
 * $Id: token.h,v 1.13 2006-10-24 04:22:40 bacon Exp $
 */

#ifndef _ASE_LSP_TOKEN_H_
#define _ASE_LSP_TOKEN_H_

#include <ase/lsp/types.h>
#include <ase/lsp/name.h>

enum 
{
	ASE_LSP_TOKEN_END
};

struct ase_lsp_token_t 
{
	int type;

	ase_lsp_int_t ivalue;
	ase_lsp_real_t rvalue;

	ase_lsp_name_t name;
	ase_bool_t __dynamic;
};

typedef struct ase_lsp_token_t ase_lsp_token_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_token_t* ase_lsp_token_open (
	ase_lsp_token_t* token, ase_word_t capacity);
void ase_lsp_token_close (ase_lsp_token_t* token);

int ase_lsp_token_addc (ase_lsp_token_t* token, ase_cint_t c);
int ase_lsp_token_adds (ase_lsp_token_t* token, const ase_char_t* s);
void ase_lsp_token_clear (ase_lsp_token_t* token);
ase_char_t* ase_lsp_token_yield (ase_lsp_token_t* token, ase_word_t capacity);
int ase_lsp_token_compare_name (ase_lsp_token_t* token, const ase_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
