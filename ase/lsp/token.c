/*
 * $Id: token.c,v 1.14 2006-10-23 14:49:16 bacon Exp $
 */

#include <sse/lsp/lsp_i.h>

sse_lsp_token_t* sse_lsp_token_open (
	sse_lsp_token_t* token, sse_word_t capacity)
{
	if (token == SSE_NULL) 
	{
		token = (sse_lsp_token_t*)
			sse_malloc (sse_sizeof(sse_lsp_token_t));
		if (token == SSE_NULL) return SSE_NULL;
		token->__dynamic = sse_true;
	}
	else token->__dynamic = sse_false;
	
	if (sse_lsp_name_open(&token->name, capacity) == SSE_NULL) {
		if (token->__dynamic) sse_free (token);
		return SSE_NULL;
	}

	/*
	token->ivalue    = 0;
	token->fvalue    = .0;
	*/
	token->type = SSE_LSP_TOKEN_END;
	return token;
}

void sse_lsp_token_close (sse_lsp_token_t* token)
{
	sse_lsp_name_close (&token->name);
	if (token->__dynamic) sse_free (token);
}

int sse_lsp_token_addc (sse_lsp_token_t* token, sse_cint_t c)
{
	return sse_lsp_name_addc (&token->name, c);
}

int sse_lsp_token_adds (sse_lsp_token_t* token, const sse_char_t* s)
{
	return sse_lsp_name_adds (&token->name, s);
}

void sse_lsp_token_clear (sse_lsp_token_t* token)
{
	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/

	token->type = SSE_LSP_TOKEN_END;
	sse_lsp_name_clear (&token->name);
}

sse_char_t* sse_lsp_token_yield (sse_lsp_token_t* token, sse_word_t capacity)
{
	sse_char_t* p;

	p = sse_lsp_name_yield (&token->name, capacity);
	if (p == SSE_NULL) return SSE_NULL;

	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/
	token->type = SSE_LSP_TOKEN_END;
	return p;
}

int sse_lsp_token_compare_name (sse_lsp_token_t* token, const sse_char_t* str)
{
	return sse_lsp_name_compare (&token->name, str);
}
