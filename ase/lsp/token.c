/*
 * $Id: token.c,v 1.4 2005-02-05 05:18:20 bacon Exp $
 */

#include "token.h"
#include <stdlib.h>

xp_lisp_token_t* xp_lisp_token_new (xp_size_t capacity)
{
	xp_lisp_token_t* token;

	xp_assert (capacity > 0);

	token = (xp_lisp_token_t*)malloc (sizeof(xp_lisp_token_t));
	if (token == XP_NULL) return XP_NULL;

	token->buffer = (xp_lisp_char*)malloc ((capacity + 1) * sizeof(xp_lisp_char));
	if (token->buffer == XP_NULL) {
		free (token);
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
	free (token->buffer);
	free (token);
}

int xp_lisp_token_addc (xp_lisp_token_t* token, xp_lisp_cint c)
{
	if (token->size >= token->capacity) {
		// double the capacity.
		xp_lisp_char* new_buffer = (xp_lisp_char*)realloc (
			token->buffer, (token->capacity * 2 + 1) * sizeof(xp_lisp_char));
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

xp_lisp_char* xp_lisp_token_transfer (xp_lisp_token_t* token, xp_size_t capacity)
{
	xp_lisp_char* old_buffer, * new_buffer;
   
	new_buffer = (xp_lisp_char*)malloc((capacity + 1) * sizeof(xp_lisp_char));
	if (new_buffer == XP_NULL) return XP_NULL;

	old_buffer = token->buffer;
	token->buffer    = new_buffer;
	token->size      = 0;
	token->capacity  = capacity;
	token->buffer[0] = XP_CHAR('\0');

	return old_buffer;
}

int xp_lisp_token_compare (xp_lisp_token_t* token, const xp_lisp_char* str)
{
	xp_lisp_char* p = token->buffer;
	xp_size_t index = 0;

	while (index < token->size) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == XP_CHAR('\0'))? 0: -1;
}
