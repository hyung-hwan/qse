/*
 * $Id: name.h,v 1.2 2005-09-18 10:18:35 bacon Exp $
 */

#ifndef _XP_LSP_NAME_H_
#define _XP_LSP_NAME_H_

#include <xp/types.h>
#include <xp/macros.h>

struct xp_lsp_name_t 
{
	xp_word_t capacity;
	xp_word_t size;
	xp_char_t* buffer;
	xp_char_t static_buffer[128];
	xp_bool_t __malloced;
};

typedef struct xp_lsp_name_t xp_lsp_name_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_lsp_name_t* xp_lsp_name_open (
	xp_lsp_name_t* name, xp_word_t capacity);
void xp_lsp_name_close (xp_lsp_name_t* name);

int xp_lsp_name_addc (xp_lsp_name_t* name, xp_cint_t c);
int xp_lsp_name_adds (xp_lsp_name_t* name, const xp_char_t* s);
void xp_lsp_name_clear (xp_lsp_name_t* name);
xp_char_t* xp_lsp_name_yield (xp_lsp_name_t* name, xp_word_t capacity);
int xp_lsp_name_compare (xp_lsp_name_t* name, const xp_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
