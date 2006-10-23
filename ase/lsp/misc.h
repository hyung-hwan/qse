/*
 * $Id: misc.h,v 1.1 2006-10-23 14:42:38 bacon Exp $
 */

#ifndef _SSE_LSP_MISC_H_
#define _SSE_LSP_MISC_H_

#ifndef _SSE_LSP_LSP_H_
#error Never include this file directly. Include <sse/lsp/lsp.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

void* sse_lsp_memcpy (void* dst, const void* src, sse_size_t n);
void* sse_lsp_memset (void* dst, int val, sse_size_t n);

int sse_lsp_abort (sse_lsp_t* lsp, 
	const sse_char_t* expr, const sse_char_t* file, int line);

#ifdef __cplusplus
}
#endif

#endif

