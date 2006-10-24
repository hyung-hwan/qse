/*
 * $Id: misc.h,v 1.2 2006-10-24 04:22:39 bacon Exp $
 */

#ifndef _ASE_LSP_MISC_H_
#define _ASE_LSP_MISC_H_

#ifndef _ASE_LSP_LSP_H_
#error Never include this file directly. Include <ase/lsp/lsp.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

void* ase_lsp_memcpy (void* dst, const void* src, ase_size_t n);
void* ase_lsp_memset (void* dst, int val, ase_size_t n);

int ase_lsp_abort (ase_lsp_t* lsp, 
	const ase_char_t* expr, const ase_char_t* file, int line);

#ifdef __cplusplus
}
#endif

#endif

