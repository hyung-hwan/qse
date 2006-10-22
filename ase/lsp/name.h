/*
 * $Id: name.h,v 1.4 2006-10-22 13:10:46 bacon Exp $
 */

#ifndef _SSE_LSP_NAME_H_
#define _SSE_LSP_NAME_H_

#include <sse/types.h>
#include <sse/macros.h>

struct sse_lsp_name_t 
{
	sse_word_t capacity;
	sse_word_t size;
	sse_char_t* buffer;
	sse_char_t static_buffer[128];
	sse_bool_t __dynamic;
};

typedef struct sse_lsp_name_t sse_lsp_name_t;

#ifdef __cplusplus
extern "C" {
#endif

sse_lsp_name_t* sse_lsp_name_open (
	sse_lsp_name_t* name, sse_word_t capacity);
void sse_lsp_name_close (sse_lsp_name_t* name);

int sse_lsp_name_addc (sse_lsp_name_t* name, sse_cint_t c);
int sse_lsp_name_adds (sse_lsp_name_t* name, const sse_char_t* s);
void sse_lsp_name_clear (sse_lsp_name_t* name);
sse_char_t* sse_lsp_name_yield (sse_lsp_name_t* name, sse_word_t capacity);
int sse_lsp_name_compare (sse_lsp_name_t* name, const sse_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
