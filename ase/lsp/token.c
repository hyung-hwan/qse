/*
 * $Id: token.c,v 1.16 2006-10-25 14:42:40 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

ase_lsp_token_t* ase_lsp_token_open (
	ase_lsp_token_t* token, ase_size_t capacity)
{
	if (token == ASE_NULL) 
	{
		token = (ase_lsp_token_t*)
			ase_malloc (ase_sizeof(ase_lsp_token_t));
		if (token == ASE_NULL) return ASE_NULL;
		token->__dynamic = ase_true;
	}
	else token->__dynamic = ase_false;
	
	if (ase_lsp_name_open(&token->name, capacity) == ASE_NULL) {
		if (token->__dynamic) ase_free (token);
		return ASE_NULL;
	}

	/*
	token->ivalue    = 0;
	token->fvalue    = .0;
	*/
	token->type = ASE_LSP_TOKEN_END;
	return token;
}

void ase_lsp_token_close (ase_lsp_token_t* token)
{
	ase_lsp_name_close (&token->name);
	if (token->__dynamic) ase_free (token);
}

int ase_lsp_token_addc (ase_lsp_token_t* token, ase_cint_t c)
{
	return ase_lsp_name_addc (&token->name, c);
}

int ase_lsp_token_adds (ase_lsp_token_t* token, const ase_char_t* s)
{
	return ase_lsp_name_adds (&token->name, s);
}

void ase_lsp_token_clear (ase_lsp_token_t* token)
{
	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/

	token->type = ASE_LSP_TOKEN_END;
	ase_lsp_name_clear (&token->name);
}

ase_char_t* ase_lsp_token_yield (ase_lsp_token_t* token, ase_size_t capacity)
{
	ase_char_t* p;

	p = ase_lsp_name_yield (&token->name, capacity);
	if (p == ASE_NULL) return ASE_NULL;

	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/
	token->type = ASE_LSP_TOKEN_END;
	return p;
}

int ase_lsp_token_compare_name (ase_lsp_token_t* token, const ase_char_t* str)
{
	return ase_lsp_name_compare (&token->name, str);
}
