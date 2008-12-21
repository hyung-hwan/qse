/*
 * $Id: misc.h 117 2008-03-03 11:20:05Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LSP_MISC_H_
#define _QSE_LSP_MISC_H_

#ifndef _QSE_LSP_LSP_H_
#error Never include this file directly. Include <qse/lsp/lsp.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

void* qse_lsp_memcpy (void* dst, const void* src, qse_size_t n);
void* qse_lsp_memset (void* dst, int val, qse_size_t n);

#ifdef __cplusplus
}
#endif

#endif

