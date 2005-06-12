/*
 * $Id: token.c,v 1.6 2005-06-12 14:40:35 bacon Exp $
 */

#include <xp/stx/token.h>
#include <xp/stx/misc.h>

xp_stx_token_t* xp_stx_token_open (
	xp_stx_token_t* token, xp_word_t capacity)
{
	xp_assert (capacity > 0);

	if (token == XP_NULL) {
		token = (xp_stx_token_t*)
			xp_malloc (xp_sizeof(xp_stx_token_t));
		if (token == XP_NULL) return XP_NULL;
		token->__malloced = xp_true;
	}
	else token->__malloced = xp_false;
	
	if (capacity < xp_countof(token->static_buffer)) {
		token->buffer = token->static_buffer;
	}
	else {
		token->buffer = (xp_char_t*)
			xp_malloc ((capacity + 1) * xp_sizeof(xp_char_t));
		if (token->buffer == XP_NULL) {
			if (token->__malloced) xp_free (token);
			return XP_NULL;
		}
	}

	/*
	token->ivalue    = 0;
	token->fvalue    = .0;
	*/

	token->size      = 0;
	token->capacity  = capacity;
	token->buffer[0] = XP_CHAR('\0');

	return token;
}

void xp_stx_token_close (xp_stx_token_t* token)
{
	if (token->capacity < xp_countof(token->static_buffer)) {
		xp_assert (token->buffer != token->static_buffer);
		xp_free (token->buffer);
	}
	if (token->__malloced) xp_free (token);
}

int xp_stx_token_addc (xp_stx_token_t* token, xp_cint_t c)
{
	if (token->size >= token->capacity) {
		/* double the capacity. */
		xp_size_t new_capacity = token->capacity * 2;

		if (new_capacity >= xp_countof(token->static_buffer)) {
			xp_char_t* space;

			if (token->capacity < xp_countof(token->static_buffer)) {
				space = (xp_char_t*)xp_malloc (
					(new_capacity + 1) * xp_sizeof(xp_char_t));
				if (space == XP_NULL) return -1;

				/* don't need to copy up to the terminating null */
				xp_memcpy (space, token->buffer, 
					token->capacity * xp_sizeof(xp_char_t));
			}
			else {
				space = (xp_char_t*)xp_realloc (token->buffer, 
					(new_capacity + 1) * xp_sizeof(xp_char_t));
				if (space == XP_NULL) return -1;
			}

			token->buffer   = space;
		}

		token->capacity = new_capacity;
	}

	token->buffer[token->size++] = c;
	token->buffer[token->size]   = XP_CHAR('\0');
	return 0;
}

void xp_stx_token_clear (xp_stx_token_t* token)
{
	/*
	token->ivalue    = 0;
	token->fvalue    = .0;
	*/

	token->size      = 0;
	token->buffer[0] = XP_CHAR('\0');
}

xp_char_t* xp_stx_token_yield (xp_stx_token_t* token, xp_word_t capacity)
{
	xp_char_t* old_buffer, * new_buffer;
   
	if (token->capacity < xp_countof(token->static_buffer)) {
		old_buffer = (xp_char_t*)
			xp_malloc((token->capacity + 1) * xp_sizeof(xp_char_t));
		if (old_buffer == XP_NULL) return XP_NULL;
		xp_memcpy (old_buffer, token->buffer, 
			(token->capacity + 1) * xp_sizeof(xp_char_t));
	}
	else old_buffer = token->buffer;

	if (capacity < xp_countof(token->static_buffer)) {
		new_buffer = token->static_buffer;
	}
	else {
		new_buffer = (xp_char_t*)
			xp_malloc((capacity + 1) * xp_sizeof(xp_char_t));
		if (new_buffer == XP_NULL) return XP_NULL;
	}

	token->buffer    = new_buffer;
	token->size      = 0;
	token->capacity  = capacity;
	token->buffer[0] = XP_CHAR('\0');

	return old_buffer;
}

int xp_stx_token_compare (xp_stx_token_t* token, const xp_char_t* str)
{
	xp_char_t* p = token->buffer;
	xp_word_t index = 0;

	while (index < token->size) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == XP_CHAR('\0'))? 0: -1;
}
