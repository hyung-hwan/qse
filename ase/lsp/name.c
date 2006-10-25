/*
 * $Id: name.c,v 1.7 2006-10-25 14:42:40 bacon Exp $
 */

#include <ase/lsp/name.h>

ase_lsp_name_t* ase_lsp_name_open (
	ase_lsp_name_t* name, ase_size_t capacity)
{
	if (capacity == 0) 
		capacity = ase_countof(name->static_buffer) - 1;

	if (name == ASE_NULL) {
		name = (ase_lsp_name_t*)
			ase_malloc (ase_sizeof(ase_lsp_name_t));
		if (name == ASE_NULL) return ASE_NULL;
		name->__dynamic = ase_true;
	}
	else name->__dynamic = ase_false;
	
	if (capacity < ase_countof(name->static_buffer)) {
		name->buffer = name->static_buffer;
	}
	else {
		name->buffer = (ase_char_t*)
			ase_malloc ((capacity + 1) * ase_sizeof(ase_char_t));
		if (name->buffer == ASE_NULL) {
			if (name->__dynamic) ase_free (name);
			return ASE_NULL;
		}
	}

	name->size      = 0;
	name->capacity  = capacity;
	name->buffer[0] = ASE_CHAR('\0');

	return name;
}

void ase_lsp_name_close (ase_lsp_name_t* name)
{
	if (name->capacity >= ase_countof(name->static_buffer)) {
		ase_assert (name->buffer != name->static_buffer);
		ase_free (name->buffer);
	}
	if (name->__dynamic) ase_free (name);
}

int ase_lsp_name_addc (ase_lsp_name_t* name, ase_cint_t c)
{
	if (name->size >= name->capacity) {
		/* double the capacity. */
		ase_size_t new_capacity = name->capacity * 2;

		if (new_capacity >= ase_countof(name->static_buffer)) {
			ase_char_t* space;

			if (name->capacity < ase_countof(name->static_buffer)) {
				space = (ase_char_t*)ase_malloc (
					(new_capacity + 1) * ase_sizeof(ase_char_t));
				if (space == ASE_NULL) return -1;

				/* don't need to copy up to the terminating null */
				ase_memcpy (space, name->buffer, 
					name->capacity * ase_sizeof(ase_char_t));
			}
			else {
				space = (ase_char_t*)ase_realloc (name->buffer, 
					(new_capacity + 1) * ase_sizeof(ase_char_t));
				if (space == ASE_NULL) return -1;
			}

			name->buffer   = space;
		}

		name->capacity = new_capacity;
	}

	name->buffer[name->size++] = c;
	name->buffer[name->size]   = ASE_CHAR('\0');
	return 0;
}

int ase_lsp_name_adds (ase_lsp_name_t* name, const ase_char_t* s)
{
	while (*s != ASE_CHAR('\0')) {
		if (ase_lsp_name_addc(name, *s) == -1) return -1;
		s++;
	}

	return 0;
}

void ase_lsp_name_clear (ase_lsp_name_t* name)
{
	name->size      = 0;
	name->buffer[0] = ASE_CHAR('\0');
}

ase_char_t* ase_lsp_name_yield (ase_lsp_name_t* name, ase_size_t capacity)
{
	ase_char_t* old_buffer, * new_buffer;

	if (capacity == 0) 
		capacity = ase_countof(name->static_buffer) - 1;
   
	if (name->capacity < ase_countof(name->static_buffer)) {
		old_buffer = (ase_char_t*)
			ase_malloc((name->capacity + 1) * ase_sizeof(ase_char_t));
		if (old_buffer == ASE_NULL) return ASE_NULL;
		ase_memcpy (old_buffer, name->buffer, 
			(name->capacity + 1) * ase_sizeof(ase_char_t));
	}
	else old_buffer = name->buffer;

	if (capacity < ase_countof(name->static_buffer)) {
		new_buffer = name->static_buffer;
	}
	else {
		new_buffer = (ase_char_t*)
			ase_malloc((capacity + 1) * ase_sizeof(ase_char_t));
		if (new_buffer == ASE_NULL) return ASE_NULL;
	}

	name->buffer    = new_buffer;
	name->size      = 0;
	name->capacity  = capacity;
	name->buffer[0] = ASE_CHAR('\0');

	return old_buffer;
}

int ase_lsp_name_compare (ase_lsp_name_t* name, const ase_char_t* str)
{
	ase_char_t* p = name->buffer;
	ase_size_t index = 0;

	while (index < name->size) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == ASE_CHAR('\0'))? 0: -1;
}
