/*
 * $Id: array.h,v 1.6 2006-10-22 13:10:45 bacon Exp $
 */

#ifndef _SSE_LSP_ARRAY_H_
#define _SSE_LSP_ARRAY_H_

#include <sse/types.h>

struct sse_lsp_array_t 
{
	void** buffer;
	sse_size_t size;
	sse_size_t capacity;
};

typedef struct sse_lsp_array_t sse_lsp_array_t;

#ifdef __cplusplus
extern "C" {
#endif

sse_lsp_array_t* sse_lsp_array_new (sse_size_t capacity);
void sse_lsp_array_free (sse_lsp_array_t* array);
int sse_lsp_array_add_item (sse_lsp_array_t* array, void* item);
int sse_lsp_array_insert (sse_lsp_array_t* array, sse_size_t index, void* value);
void sse_lsp_array_delete (sse_lsp_array_t* array, sse_size_t index);
void sse_lsp_array_clear (sse_lsp_array_t* array);
void** sse_lsp_array_yield (sse_lsp_array_t* array, sse_size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
