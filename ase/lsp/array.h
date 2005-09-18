/*
 * $Id: array.h,v 1.5 2005-09-18 08:10:50 bacon Exp $
 */

#ifndef _XP_LSP_ARRAY_H_
#define _XP_LSP_ARRAY_H_

#include <xp/types.h>

struct xp_lsp_array_t 
{
	void** buffer;
	xp_size_t size;
	xp_size_t capacity;
};

typedef struct xp_lsp_array_t xp_lsp_array_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_lsp_array_t* xp_lsp_array_new (xp_size_t capacity);
void xp_lsp_array_free (xp_lsp_array_t* array);
int xp_lsp_array_add_item (xp_lsp_array_t* array, void* item);
int xp_lsp_array_insert (xp_lsp_array_t* array, xp_size_t index, void* value);
void xp_lsp_array_delete (xp_lsp_array_t* array, xp_size_t index);
void xp_lsp_array_clear (xp_lsp_array_t* array);
void** xp_lsp_array_yield (xp_lsp_array_t* array, xp_size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
