/*
 * $Id: name.c,v 1.5 2006-10-23 14:42:38 bacon Exp $
 */

#include <sse/lsp/name.h>

sse_lsp_name_t* sse_lsp_name_open (
	sse_lsp_name_t* name, sse_word_t capacity)
{
	if (capacity == 0) 
		capacity = sse_countof(name->static_buffer) - 1;

	if (name == SSE_NULL) {
		name = (sse_lsp_name_t*)
			sse_malloc (sse_sizeof(sse_lsp_name_t));
		if (name == SSE_NULL) return SSE_NULL;
		name->__dynamic = sse_true;
	}
	else name->__dynamic = sse_false;
	
	if (capacity < sse_countof(name->static_buffer)) {
		name->buffer = name->static_buffer;
	}
	else {
		name->buffer = (sse_char_t*)
			sse_malloc ((capacity + 1) * sse_sizeof(sse_char_t));
		if (name->buffer == SSE_NULL) {
			if (name->__dynamic) sse_free (name);
			return SSE_NULL;
		}
	}

	name->size      = 0;
	name->capacity  = capacity;
	name->buffer[0] = SSE_CHAR('\0');

	return name;
}

void sse_lsp_name_close (sse_lsp_name_t* name)
{
	if (name->capacity >= sse_countof(name->static_buffer)) {
		sse_assert (name->buffer != name->static_buffer);
		sse_free (name->buffer);
	}
	if (name->__dynamic) sse_free (name);
}

int sse_lsp_name_addc (sse_lsp_name_t* name, sse_cint_t c)
{
	if (name->size >= name->capacity) {
		/* double the capacity. */
		sse_size_t new_capacity = name->capacity * 2;

		if (new_capacity >= sse_countof(name->static_buffer)) {
			sse_char_t* space;

			if (name->capacity < sse_countof(name->static_buffer)) {
				space = (sse_char_t*)sse_malloc (
					(new_capacity + 1) * sse_sizeof(sse_char_t));
				if (space == SSE_NULL) return -1;

				/* don't need to copy up to the terminating null */
				sse_memcpy (space, name->buffer, 
					name->capacity * sse_sizeof(sse_char_t));
			}
			else {
				space = (sse_char_t*)sse_realloc (name->buffer, 
					(new_capacity + 1) * sse_sizeof(sse_char_t));
				if (space == SSE_NULL) return -1;
			}

			name->buffer   = space;
		}

		name->capacity = new_capacity;
	}

	name->buffer[name->size++] = c;
	name->buffer[name->size]   = SSE_CHAR('\0');
	return 0;
}

int sse_lsp_name_adds (sse_lsp_name_t* name, const sse_char_t* s)
{
	while (*s != SSE_CHAR('\0')) {
		if (sse_lsp_name_addc(name, *s) == -1) return -1;
		s++;
	}

	return 0;
}

void sse_lsp_name_clear (sse_lsp_name_t* name)
{
	name->size      = 0;
	name->buffer[0] = SSE_CHAR('\0');
}

sse_char_t* sse_lsp_name_yield (sse_lsp_name_t* name, sse_word_t capacity)
{
	sse_char_t* old_buffer, * new_buffer;

	if (capacity == 0) 
		capacity = sse_countof(name->static_buffer) - 1;
   
	if (name->capacity < sse_countof(name->static_buffer)) {
		old_buffer = (sse_char_t*)
			sse_malloc((name->capacity + 1) * sse_sizeof(sse_char_t));
		if (old_buffer == SSE_NULL) return SSE_NULL;
		sse_memcpy (old_buffer, name->buffer, 
			(name->capacity + 1) * sse_sizeof(sse_char_t));
	}
	else old_buffer = name->buffer;

	if (capacity < sse_countof(name->static_buffer)) {
		new_buffer = name->static_buffer;
	}
	else {
		new_buffer = (sse_char_t*)
			sse_malloc((capacity + 1) * sse_sizeof(sse_char_t));
		if (new_buffer == SSE_NULL) return SSE_NULL;
	}

	name->buffer    = new_buffer;
	name->size      = 0;
	name->capacity  = capacity;
	name->buffer[0] = SSE_CHAR('\0');

	return old_buffer;
}

int sse_lsp_name_compare (sse_lsp_name_t* name, const sse_char_t* str)
{
	sse_char_t* p = name->buffer;
	sse_word_t index = 0;

	while (index < name->size) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == SSE_CHAR('\0'))? 0: -1;
}
