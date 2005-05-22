/*
 * $Id: token.c,v 1.1 2005-05-22 10:32:37 bacon Exp $
 */

#include <xp/stx/token.h>
#include <xp/stx/misc.h>

xp_stx_token_t* xp_stx_token_new (
	xp_stx_token_t* token, xp_stx_word_t capacity)
{
	xp_stx_assert (capacity > 0);

	if (token == XP_NULL) {
		token = (xp_stx_token_t*)
			xp_stx_malloc (sizeof(xp_stx_token_t));
		if (token == XP_NULL) return XP_NULL;
		token->__malloced = xp_true;
	}
	else token->__malloced = xp_false;
	
	token->buffer = (xp_char_t*)
		xp_stx_malloc ((capacity + 1) * sizeof(xp_char_t));
	if (token->buffer == XP_NULL) {
		if (token->__malloced) xp_stx_free (token);
		return XP_NULL;
	}

	/*
	token->ivalue    = 0;
	token->fvalue    = .0;
	*/

	token->size      = 0;
	token->capacity  = capacity;
	token->buffer[0] = XP_STX_CHAR('\0');

	return token;
}

void xp_stx_token_close (xp_stx_token_t* token)
{
	xp_stx_free (token->buffer);
	if (token->__malloced) xp_stx_free (token);
}

int xp_stx_token_addc (xp_stx_token_t* token, xp_cint_t c)
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
	token->buffer[token->size]   = XP_STX_CHAR('\0');
	return 0;
}

void xp_stx_token_clear (xp_stx_token_t* token)
{
	token->ivalue    = 0;
	token->fvalue    = .0;

	token->size      = 0;
	token->buffer[0] = XP_STX_CHAR('\0');
}

xp_char_t* xp_stx_token_transfer (xp_stx_token_t* token, xp_size_t capacity)
{
	xp_char_t* old_buffer, * new_buffer;
   
	new_buffer = (xp_char_t*)xp_malloc((capacity + 1) * sizeof(xp_char_t));
	if (new_buffer == XP_NULL) return XP_NULL;

	old_buffer = token->buffer;
	token->buffer    = new_buffer;
	token->size      = 0;
	token->capacity  = capacity;
	token->buffer[0] = XP_STX_CHAR('\0');

	return old_buffer;
}

int xp_stx_token_compare (xp_stx_token_t* token, const xp_char_t* str)
{
	xp_char_t* p = token->buffer;
	xp_size_t index = 0;

	while (index < token->size) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == XP_STX_CHAR('\0'))? 0: -1;
}
