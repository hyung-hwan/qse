/*
 * $Id: token.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _ASE_STX_TOKEN_H_
#define _ASE_STX_TOKEN_H_

#include <ase/stx/stx.h>
#include <ase/stx/name.h>

enum
{
	ASE_STX_TOKEN_END,
	ASE_STX_TOKEN_CHARLIT,
	ASE_STX_TOKEN_STRLIT,
	ASE_STX_TOKEN_SYMLIT,
	ASE_STX_TOKEN_NUMLIT,
	ASE_STX_TOKEN_IDENT,
	ASE_STX_TOKEN_BINARY,
	ASE_STX_TOKEN_KEYWORD,
	ASE_STX_TOKEN_PRIMITIVE,
	ASE_STX_TOKEN_ASSIGN,
	ASE_STX_TOKEN_COLON,
	ASE_STX_TOKEN_RETURN,
	ASE_STX_TOKEN_LBRACKET,
	ASE_STX_TOKEN_RBRACKET,
	ASE_STX_TOKEN_LPAREN,
	ASE_STX_TOKEN_RPAREN,
	ASE_STX_TOKEN_APAREN,
	ASE_STX_TOKEN_PERIOD,
	ASE_STX_TOKEN_SEMICOLON
};

struct ase_stx_token_t 
{
	int type;

	/*
	ase_stx_int_t   ivalue;
	ase_stx_real_t  fvalue;
	*/
	ase_stx_name_t name;
	ase_bool_t __dynamic;
};

typedef struct ase_stx_token_t ase_stx_token_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_stx_token_t* ase_stx_token_open (
	ase_stx_token_t* token, ase_word_t capacity);
void ase_stx_token_close (ase_stx_token_t* token);

int ase_stx_token_addc (ase_stx_token_t* token, ase_cint_t c);
int ase_stx_token_adds (ase_stx_token_t* token, const ase_char_t* s);
void ase_stx_token_clear (ase_stx_token_t* token);
ase_char_t* ase_stx_token_yield (ase_stx_token_t* token, ase_word_t capacity);
int ase_stx_token_compare_name (ase_stx_token_t* token, const ase_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
