/*
 * $Id: misc.h,v 1.3 2007/04/30 06:09:46 bacon Exp $
 *
 * {License}
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

#ifdef __cplusplus
}
#endif

#endif

