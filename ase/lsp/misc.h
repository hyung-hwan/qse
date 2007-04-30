/*
 * $Id: misc.h,v 1.1.1.1 2007/03/28 14:05:24 bacon Exp $
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

