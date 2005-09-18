/*
 * $Id: token.h,v 1.8 2005-09-18 08:10:50 bacon Exp $
 */

#ifndef _XP_LSP_TOKEN_H_
#define _XP_LSP_TOKEN_H_

#include <xp/lsp/lsp.h>
#include <xp/lsp/name.h>

enum 
{
	XP_LSP_TOKEN_END
};

struct xp_lsp_token_t 
{
	int type;

	xp_lsp_int_t ivalue;
	xp_lsp_real_t fvalue;

	xp_lsp_name_t name;
	xp_bool_t __malloced;
};

typedef struct xp_lsp_token_t xp_lsp_token_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_lsp_token_t* xp_lsp_token_open (
	xp_lsp_token_t* token, xp_word_t capacity);
void xp_lsp_token_close (xp_lsp_token_t* token);

int xp_lsp_token_addc (xp_lsp_token_t* token, xp_cint_t c);
int xp_lsp_token_adds (xp_lsp_token_t* token, const xp_char_t* s);
void xp_lsp_token_clear (xp_lsp_token_t* token);
xp_char_t* xp_lsp_token_yield (xp_lsp_token_t* token, xp_word_t capacity);
int xp_lsp_token_compare_name (xp_lsp_token_t* token, const xp_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
