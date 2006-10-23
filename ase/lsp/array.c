/*
 * $Id: array.c,v 1.10 2006-10-23 14:49:16 bacon Exp $
 */

#include <sse/lsp/lsp_i.h>

sse_lsp_array_t* sse_lsp_array_new (sse_size_t capacity)
{
	sse_lsp_array_t* array;

	sse_assert (capacity > 0);
	array = (sse_lsp_array_t*) sse_malloc (sizeof(sse_lsp_array_t));
	if (array == SSE_NULL) return SSE_NULL;

	array->buffer = (void**) sse_malloc (capacity + 1);
	if (array->buffer == SSE_NULL) {
		free (array);
		return SSE_NULL;
	}

	array->size      = 0;
	array->capacity  = capacity;
	array->buffer[0] = SSE_NULL;
	return array;
}

void sse_lsp_array_free (sse_lsp_array_t* array)
{
	while (array->size > 0) 
		free (array->buffer[--array->size]);
	sse_assert (array->size == 0);

	free (array->buffer);
	free (array);
}

int sse_lsp_array_add_item (sse_lsp_array_t* array, void* item)
{
	if (array->size >= array->capacity) {
		void* new_buffer = (void**)realloc (
			array->buffer, array->capacity * 2 + 1);
		if (new_buffer == SSE_NULL) return -1;
		array->buffer   = new_buffer;
		array->capacity = array->capacity * 2;	
	}

	array->buffer[array->size++] = item;
	array->buffer[array->size]   = SSE_NULL;
	return 0;
}

int sse_lsp_array_insert (sse_lsp_array_t* array, sse_size_t index, void* value)
{
	sse_size_t i;

	if (index >= array->capacity) {
		void* new_buffer = (void**)realloc (
			array->buffer, array->capacity * 2 + 1);
		if (new_buffer == SSE_NULL) return -1;
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

void sse_lsp_array_delete (sse_lsp_array_t* array, sse_size_t index)
{
	sse_assert (index < array->size);

}

void sse_lsp_array_clear (sse_lsp_array_t* array)
{
	while (array->size > 0) 
		free (array->buffer[--array->size]);
	sse_assert (array->size == 0);
	array->buffer[0] = SSE_NULL;
}

void** sse_lsp_array_yield (sse_lsp_array_t* array, sse_size_t capacity)
{
	void** old_buffer, ** new_buffer;
   
	new_buffer = (void**) sse_malloc (capacity + 1);
	if (new_buffer == SSE_NULL) return SSE_NULL;

	old_buffer = array->buffer;
	array->buffer    = new_buffer;
	array->size      = 0;
	array->capacity  = capacity;
	array->buffer[0] = SSE_NULL;

	return old_buffer;
}
