/*
 * $Id: token.h,v 1.12 2006-10-22 13:10:46 bacon Exp $
 */

#ifndef _SSE_LSP_TOKEN_H_
#define _SSE_LSP_TOKEN_H_

#include <sse/lsp/types.h>
#include <sse/lsp/name.h>

enum 
{
	SSE_LSP_TOKEN_END
};

struct sse_lsp_token_t 
{
	int type;

	sse_lsp_int_t ivalue;
	sse_lsp_real_t rvalue;

	sse_lsp_name_t name;
	sse_bool_t __dynamic;
};

typedef struct sse_lsp_token_t sse_lsp_token_t;

#ifdef __cplusplus
extern "C" {
#endif

sse_lsp_token_t* sse_lsp_token_open (
	sse_lsp_token_t* token, sse_word_t capacity);
void sse_lsp_token_close (sse_lsp_token_t* token);

int sse_lsp_token_addc (sse_lsp_token_t* token, sse_cint_t c);
int sse_lsp_token_adds (sse_lsp_token_t* token, const sse_char_t* s);
void sse_lsp_token_clear (sse_lsp_token_t* token);
sse_char_t* sse_lsp_token_yield (sse_lsp_token_t* token, sse_word_t capacity);
int sse_lsp_token_compare_name (sse_lsp_token_t* token, const sse_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
