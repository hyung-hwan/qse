/*
 * $Id: token.c,v 1.11 2005-09-18 08:10:50 bacon Exp $
 */

#include <xp/lsp/token.h>
#include <xp/bas/memory.h>

xp_lsp_token_t* xp_lsp_token_open (
	xp_lsp_token_t* token, xp_word_t capacity)
{
	if (token == XP_NULL) {
		token = (xp_lsp_token_t*)
			xp_malloc (xp_sizeof(xp_lsp_token_t));
		if (token == XP_NULL) return XP_NULL;
		token->__malloced = xp_true;
	}
	else token->__malloced = xp_false;
	
	if (xp_lsp_name_open(&token->name, capacity) == XP_NULL) {
		if (token->__malloced) xp_free (token);
		return XP_NULL;
	}

	/*
	token->ivalue    = 0;
	token->fvalue    = .0;
	*/
	token->type      = XP_LSP_TOKEN_END;
	return token;
}

void xp_lsp_token_close (xp_lsp_token_t* token)
{
	xp_lsp_name_close (&token->name);
	if (token->__malloced) xp_free (token);
}

int xp_lsp_token_addc (xp_lsp_token_t* token, xp_cint_t c)
{
	return xp_lsp_name_addc (&token->name, c);
}

int xp_lsp_token_adds (xp_lsp_token_t* token, const xp_char_t* s)
{
	return xp_lsp_name_adds (&token->name, s);
}

void xp_lsp_token_clear (xp_lsp_token_t* token)
{
	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/

	token->type = XP_LSP_TOKEN_END;
	xp_lsp_name_clear (&token->name);
}

xp_char_t* xp_lsp_token_yield (xp_lsp_token_t* token, xp_word_t capacity)
{
	xp_char_t* p;

	p = xp_lsp_name_yield (&token->name, capacity);
	if (p == XP_NULL) return XP_NULL;

	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/
	token->type = XP_LSP_TOKEN_END;
	return p;
}

int xp_lsp_token_compare_name (xp_lsp_token_t* token, const xp_char_t* str)
{
	return xp_lsp_name_compare (&token->name, str);
}
