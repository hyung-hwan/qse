/*
 * $Id: array.c,v 1.11 2006-10-24 04:22:39 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

ase_lsp_array_t* ase_lsp_array_new (ase_size_t capacity)
{
	ase_lsp_array_t* array;

	ase_assert (capacity > 0);
	array = (ase_lsp_array_t*) ase_malloc (sizeof(ase_lsp_array_t));
	if (array == ASE_NULL) return ASE_NULL;

	array->buffer = (void**) ase_malloc (capacity + 1);
	if (array->buffer == ASE_NULL) {
		free (array);
		return ASE_NULL;
	}

	array->size      = 0;
	array->capacity  = capacity;
	array->buffer[0] = ASE_NULL;
	return array;
}

void ase_lsp_array_free (ase_lsp_array_t* array)
{
	while (array->size > 0) 
		free (array->buffer[--array->size]);
	ase_assert (array->size == 0);

	free (array->buffer);
	free (array);
}

int ase_lsp_array_add_item (ase_lsp_array_t* array, void* item)
{
	if (array->size >= array->capacity) {
		void* new_buffer = (void**)realloc (
			array->buffer, array->capacity * 2 + 1);
		if (new_buffer == ASE_NULL) return -1;
		array->buffer   = new_buffer;
		array->capacity = array->capacity * 2;	
	}

	array->buffer[array->size++] = item;
	array->buffer[array->size]   = ASE_NULL;
	return 0;
}

int ase_lsp_array_insert (ase_lsp_array_t* array, ase_size_t index, void* value)
{
	ase_size_t i;

	if (index >= array->capacity) {
		void* new_buffer = (void**)realloc (
			array->buffer, array->capacity * 2 + 1);
		if (new_buffer == ASE_NULL) return -1;
		array->buffer   = new_buffer;
		array->capacity = array->capacity * 2;	
	}

	for (i = array->size; i > index; i--) {
		array->buffer[i] = array->buffer[i - 1];
	}
	array->buffer[index] = value;
	array->size = (index > array->size)? index + 1: array->size + 1;

	return 0;
}

void ase_lsp_array_delete (ase_lsp_array_t* array, ase_size_t index)
{
	ase_assert (index < array->size);

}

void ase_lsp_array_clear (ase_lsp_array_t* array)
{
	while (array->size > 0) 
		free (array->buffer[--array->size]);
	ase_assert (array->size == 0);
	array->buffer[0] = ASE_NULL;
}

void** ase_lsp_array_yield (ase_lsp_array_t* array, ase_size_t capacity)
{
	void** old_buffer, ** new_buffer;
   
	new_buffer = (void**) ase_malloc (capacity + 1);
	if (new_buffer == ASE_NULL) return ASE_NULL;

	old_buffer = array->buffer;
	array->buffer    = new_buffer;
	array->size      = 0;
	array->capacity  = capacity;
	array->buffer[0] = ASE_NULL;

	return old_buffer;
}
