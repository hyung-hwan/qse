/*
 * $Id: array.h,v 1.7 2006-10-24 04:22:39 bacon Exp $
 */

#ifndef _ASE_LSP_ARRAY_H_
#define _ASE_LSP_ARRAY_H_

#include <ase/types.h>

struct ase_lsp_array_t 
{
	void** buffer;
	ase_size_t size;
	ase_size_t capacity;
};

typedef struct ase_lsp_array_t ase_lsp_array_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_lsp_array_t* ase_lsp_array_new (ase_size_t capacity);
void ase_lsp_array_free (ase_lsp_array_t* array);
int ase_lsp_array_add_item (ase_lsp_array_t* array, void* item);
int ase_lsp_array_insert (ase_lsp_array_t* array, ase_size_t index, void* value);
void ase_lsp_array_delete (ase_lsp_array_t* array, ase_size_t index);
void ase_lsp_array_clear (ase_lsp_array_t* array);
void** ase_lsp_array_yield (ase_lsp_array_t* array, ase_size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
