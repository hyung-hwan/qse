/*
 * $Id: token.c,v 1.8 2005-04-24 07:48:16 bacon Exp $
 */

#include <xp/lisp/token.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>

xp_lisp_token_t* xp_lisp_token_new (xp_size_t capacity)
{
	xp_lisp_token_t* token;

	xp_assert (capacity > 0);

	token = (xp_lisp_token_t*)xp_malloc (sizeof(xp_lisp_token_t));
	if (token == XP_NULL) return XP_NULL;

	token->buffer = (xp_char_t*)xp_malloc ((capacity + 1) * sizeof(xp_char_t));
	if (token->buffer == XP_NULL) {
		xp_free (token);
		return XP_NULL;
	}

	token->ivalue    = 0;
	token->fvalue    = .0;

	token->size      = 0;
	token->capacity  = capacity;
	token->buffer[0] = XP_CHAR('\0');

	return token;
}

void xp_lisp_token_free (xp_lisp_token_t* token)
{
	xp_free (token->buffer);
	xp_free (token);
}

int xp_lisp_token_addc (xp_lisp_token_t* token, xp_cint_t c)
{
	if (token->size >= token->capacity) {
		// double the capacity.
		xp_char_t* new_buffer = (xp_char_t*)realloc (
			token->buffer, (token->capacity * 2 + 1) * sizeof(xp_char_t));
		if (new_buffer == XP_NULL) return -1;
		token->buffer   = new_buffer;
		token->capacity = token->capacity * 2;	
	}

	token->buffer[token->size++] = c;
	token->buffer[token->size]   = XP_CHAR('\0');
	return 0;
}

void xp_lisp_token_clear (xp_lisp_token_t* token)
{
	token->ivalue    = 0;
	token->fvalue    = .0;

	token->size      = 0;
	token->buffer[0] = XP_CHAR('\0');
}

xp_char_t* xp_lisp_token_transfer (xp_lisp_token_t* token, xp_size_t capacity)
{
	xp_char_t* old_buffer, * new_buffer;
   
	new_buffer = (xp_char_t*)xp_malloc((capacity + 1) * sizeof(xp_char_t));
	if (new_buffer == XP_NULL) return XP_NULL;

	old_buffer = token->buffer;
	token->buffer    = new_buffer;
	token->size      = 0;
	token->capacity  = capacity;
	token->buffer[0] = XP_CHAR('\0');

	return old_buffer;
}

int xp_lisp_token_compare (xp_lisp_token_t* token, const xp_char_t* str)
{
	xp_char_t* p = token->buffer;
	xp_size_t index = 0;

	while (index < token->size) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == XP_CHAR('\0'))? 0: -1;
}
