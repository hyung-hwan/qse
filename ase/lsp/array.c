/*
 * $Id: array.c,v 1.2 2005-02-04 16:00:37 bacon Exp $
 */

#include <xp/lisp/array.h>
#include <xp/c/stdlib.h>
#include <xp/c/assert.h>

xp_lisp_array_t* xp_lisp_array_new (xp_size_t capacity)
{
	xp_lisp_array_t* array;

	xp_assert (capacity > 0);
	array = (xp_lisp_array_t*)malloc (sizeof(xp_lisp_array_t));
	if (array == XP_NULL) return XP_NULL;

	array->buffer = (void**)malloc (capacity + 1);
	if (array->buffer == XP_NULL) {
		free (array);
		return XP_NULL;
	}

	array->size      = 0;
	array->capacity  = capacity;
	array->buffer[0] = XP_NULL;
	return array;
}

void xp_lisp_array_free (xp_lisp_array_t* array)
{
	while (array->size > 0) 
		free (array->buffer[--array->size]);
	xp_assert (array->size == 0);

	free (array->buffer);
	free (array);
}

int xp_lisp_array_add_item (xp_lisp_array_t* array, void* item)
{
	if (array->size >= array->capacity) {
		void* new_buffer = (void**)realloc (
			array->buffer, array->capacity * 2 + 1);
		if (new_buffer == XP_NULL) return -1;
		array->buffer   = new_buffer;
		array->capacity = array->capacity * 2;	
	}

	array->buffer[array->size++] = item;
	array->buffer[array->size]   = XP_NULL;
	return 0;
}

int xp_lisp_array_insert (xp_lisp_array_t* array, xp_size_t index, void* value)
{
	xp_size_t i;

	if (index >= array->capacity) {
		void* new_buffer = (void**)realloc (
			array->buffer, array->capacity * 2 + 1);
		if (new_buffer == XP_NULL) return -1;
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

void xp_lisp_array_delete (xp_lisp_array_t* array, xp_size_t index)
{
	xp_assert (index < array->size);

}

void xp_lisp_array_clear (xp_lisp_array_t* array)
{
	while (array->size > 0) 
		free (array->buffer[--array->size]);
	xp_assert (array->size == 0);
	array->buffer[0] = XP_NULL;
}

void** xp_lisp_array_transfer (xp_lisp_array_t* array, xp_size_t capacity)
{
	void** old_buffer, ** new_buffer;
   
	new_buffer = (void**)malloc(capacity + 1);
	if (new_buffer == XP_NULL) return XP_NULL;

	old_buffer = array->buffer;
	array->buffer    = new_buffer;
	array->size      = 0;
	array->capacity  = capacity;
	array->buffer[0] = XP_NULL;

	return old_buffer;
}
