/*
 * $Id: token.c,v 1.11 2007-03-22 11:19:28 bacon Exp $
 */

#include <ase/stx/token.h>
#include <ase/stx/misc.h>

ase_stx_token_t* ase_stx_token_open (
	ase_stx_token_t* token, ase_word_t capacity)
{
	if (token == ASE_NULL) {
		token = (ase_stx_token_t*)
			ase_malloc (ase_sizeof(ase_stx_token_t));
		if (token == ASE_NULL) return ASE_NULL;
		token->__dynamic = ase_true;
	}
	else token->__dynamic = ase_false;
	
	if (ase_stx_name_open(&token->name, capacity) == ASE_NULL) {
		if (token->__dynamic) ase_free (token);
		return ASE_NULL;
	}

	/*
	token->ivalue    = 0;
	token->fvalue    = .0;
	*/
	token->type      = ASE_STX_TOKEN_END;
	return token;
}

void ase_stx_token_close (ase_stx_token_t* token)
{
	ase_stx_name_close (&token->name);
	if (token->__dynamic) ase_free (token);
}

int ase_stx_token_addc (ase_stx_token_t* token, ase_cint_t c)
{
	return ase_stx_name_addc (&token->name, c);
}

int ase_stx_token_adds (ase_stx_token_t* token, const ase_char_t* s)
{
	return ase_stx_name_adds (&token->name, s);
}

void ase_stx_token_clear (ase_stx_token_t* token)
{
	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/

	token->type = ASE_STX_TOKEN_END;
	ase_stx_name_clear (&token->name);
}

ase_char_t* ase_stx_token_yield (ase_stx_token_t* token, ase_word_t capacity)
{
	ase_char_t* p;

	p = ase_stx_name_yield (&token->name, capacity);
	if (p == ASE_NULL) return ASE_NULL;

	/*
	token->ivalue = 0;
	token->fvalue = .0;
	*/
	token->type = ASE_STX_TOKEN_END;
	return p;
}

int ase_stx_token_compare_name (ase_stx_token_t* token, const ase_char_t* str)
{
	return ase_stx_name_compare (&token->name, str);
}
