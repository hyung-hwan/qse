/*
 * $Id: token.c,v 1.9 2005-06-12 16:22:03 bacon Exp $
 */

#include <xp/stx/token.h>
#include <xp/stx/misc.h>

xp_stx_token_t* xp_stx_token_open (
	xp_stx_token_t* token, xp_word_t capacity)
{
	if (token == XP_NULL) {
		token = (xp_stx_token_t*)
			xp_malloc (xp_sizeof(xp_stx_token_t));
		if (token == XP_NULL) return XP_NULL;
		token->__malloced = xp_true;
	}
	else token->__malloced = xp_false;
	
	if (xp_stx_name_open(&token->name, capacity) == XP_NULL) {
		if (token->__malloced) xp_free (token);
		return XP_NULL;
	}

	/*
	token->ivalue    = 0;
	token->fvalue    = .0;
	*/
	token->type      = XP_STX_TOKEN_END;
	return token;
}

void xp_stx_token_close (xp_stx_token_t* token)
{
	xp_stx_name_close (&token->name);
	if (token->__malloced) xp_free (token);
}

int xp_stx_token_addc (xp_stx_token_t* token, xp_cint_t c)
{
	return xp_stx_name_addc (&token->name, c);
}

int xp_stx_token_adds (xp_stx_token_t* token, const xp_char_t* s)
{
	return xp_stx_name_adds (&token->name, s);
}

void xp_stx_token_clear (xp_stx_token_t* token)
{
	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/

	token->type = XP_STX_TOKEN_END;
	xp_stx_name_clear (&token->name);
}

xp_char_t* xp_stx_token_yield (xp_stx_token_t* token, xp_word_t capacity)
{
	xp_char_t* p;

	p = xp_stx_name_yield (&token->name, capacity);
	if (p == XP_NULL) return XP_NULL;

	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/
	token->type = XP_STX_TOKEN_END;
	return p;
}

int xp_stx_token_compare_name (xp_stx_token_t* token, const xp_char_t* str)
{
	return xp_stx_name_compare (&token->name, str);
}
