/*
 * $Id: name.c,v 1.1 2005-09-18 08:10:50 bacon Exp $
 */

#include <xp/stx/name.h>
#include <xp/stx/misc.h>

xp_stx_name_t* xp_stx_name_open (
	xp_stx_name_t* name, xp_word_t capacity)
{
	if (capacity == 0) 
		capacity = xp_countof(name->static_buffer) - 1;

	if (name == XP_NULL) {
		name = (xp_stx_name_t*)
			xp_malloc (xp_sizeof(xp_stx_name_t));
		if (name == XP_NULL) return XP_NULL;
		name->__malloced = xp_true;
	}
	else name->__malloced = xp_false;
	
	if (capacity < xp_countof(name->static_buffer)) {
		name->buffer = name->static_buffer;
	}
	else {
		name->buffer = (xp_char_t*)
			xp_malloc ((capacity + 1) * xp_sizeof(xp_char_t));
		if (name->buffer == XP_NULL) {
			if (name->__malloced) xp_free (name);
			return XP_NULL;
		}
	}

	name->size      = 0;
	name->capacity  = capacity;
	name->buffer[0] = XP_CHAR('\0');

	return name;
}

void xp_stx_name_close (xp_stx_name_t* name)
{
	if (name->capacity >= xp_countof(name->static_buffer)) {
		xp_assert (name->buffer != name->static_buffer);
		xp_free (name->buffer);
	}
	if (name->__malloced) xp_free (name);
}

int xp_stx_name_addc (xp_stx_name_t* name, xp_cint_t c)
{
	if (name->size >= name->capacity) {
		/* double the capacity. */
		xp_size_t new_capacity = name->capacity * 2;

		if (new_capacity >= xp_countof(name->static_buffer)) {
			xp_char_t* space;

			if (name->capacity < xp_countof(name->static_buffer)) {
				space = (xp_char_t*)xp_malloc (
					(new_capacity + 1) * xp_sizeof(xp_char_t));
				if (space == XP_NULL) return -1;

				/* don't need to copy up to the terminating null */
				xp_memcpy (space, name->buffer, 
					name->capacity * xp_sizeof(xp_char_t));
			}
			else {
				space = (xp_char_t*)xp_realloc (name->buffer, 
					(new_capacity + 1) * xp_sizeof(xp_char_t));
				if (space == XP_NULL) return -1;
			}

			name->buffer   = space;
		}

		name->capacity = new_capacity;
	}

	name->buffer[name->size++] = c;
	name->buffer[name->size]   = XP_CHAR('\0');
	return 0;
}

int xp_stx_name_adds (xp_stx_name_t* name, const xp_char_t* s)
{
	while (*s != XP_CHAR('\0')) {
		if (xp_stx_name_addc(name, *s) == -1) return -1;
		s++;
	}

	return 0;
}

void xp_stx_name_clear (xp_stx_name_t* name)
{
	name->size      = 0;
	name->buffer[0] = XP_CHAR('\0');
}

xp_char_t* xp_stx_name_yield (xp_stx_name_t* name, xp_word_t capacity)
{
	xp_char_t* old_buffer, * new_buffer;

	if (capacity == 0) 
		capacity = xp_countof(name->static_buffer) - 1;
   
	if (name->capacity < xp_countof(name->static_buffer)) {
		old_buffer = (xp_char_t*)
			xp_malloc((name->capacity + 1) * xp_sizeof(xp_char_t));
		if (old_buffer == XP_NULL) return XP_NULL;
		xp_memcpy (old_buffer, name->buffer, 
			(name->capacity + 1) * xp_sizeof(xp_char_t));
	}
	else old_buffer = name->buffer;

	if (capacity < xp_countof(name->static_buffer)) {
		new_buffer = name->static_buffer;
	}
	else {
		new_buffer = (xp_char_t*)
			xp_malloc((capacity + 1) * xp_sizeof(xp_char_t));
		if (new_buffer == XP_NULL) return XP_NULL;
	}

	name->buffer    = new_buffer;
	name->size      = 0;
	name->capacity  = capacity;
	name->buffer[0] = XP_CHAR('\0');

	return old_buffer;
}

int xp_stx_name_compare (xp_stx_name_t* name, const xp_char_t* str)
{
	xp_char_t* p = name->buffer;
	xp_word_t index = 0;

	while (index < name->size) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == XP_CHAR('\0'))? 0: -1;
}
