/*
 * $Id: symbol.h,v 1.3 2007/04/30 08:32:41 bacon Exp $
 */

#ifndef _ASE_STX_SYMBOL_H_
#define _ASE_STX_SYMBOL_H_

#include <ase/stx/stx.h>

#define ASE_STX_SYMLINK_SIZE   2
#define ASE_STX_SYMLINK_LINK   0
#define ASE_STX_SYMLINK_SYMBOL 1

struct ase_stx_symlink_t
{
	ase_stx_objhdr_t header;
	ase_word_t link;
	ase_word_t symbol;
};

typedef struct ase_stx_symlink_t ase_stx_symlink_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_word_t ase_stx_new_symbol_link (ase_stx_t* stx, ase_word_t sym);

ase_word_t ase_stx_new_symbol (
	ase_stx_t* stx, const ase_char_t* name);
ase_word_t ase_stx_new_symbolx (
	ase_stx_t* stx, const ase_char_t* name, ase_word_t len);
void ase_stx_traverse_symbol_table (
        ase_stx_t* stx, void (*func) (ase_stx_t*,ase_word_t,void*), void* data);

#ifdef __cplusplus
}
#endif

#endif
