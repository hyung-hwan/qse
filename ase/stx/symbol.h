/*
 * $Id: symbol.h,v 1.7 2005-07-07 07:45:05 bacon Exp $
 */

#ifndef _XP_STX_SYMBOL_H_
#define _XP_STX_SYMBOL_H_

#include <xp/stx/stx.h>

#define XP_STX_SYMLINK_SIZE   2
#define XP_STX_SYMLINK_LINK   0
#define XP_STX_SYMLINK_SYMBOL 1

struct xp_stx_symlink_t
{
	xp_stx_objhdr_t header;
	xp_word_t link;
	xp_word_t symbol;
};

typedef struct xp_stx_symlink_t xp_stx_symlink_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_new_symbol_link (xp_stx_t* stx, xp_word_t sym);

xp_word_t xp_stx_new_symbol (
	xp_stx_t* stx, const xp_char_t* name);
xp_word_t xp_stx_new_symbolx (
	xp_stx_t* stx, const xp_char_t* name, xp_word_t len);

xp_word_t xp_stx_new_symbol_pp (
	xp_stx_t* stx, const xp_char_t* name,
	const xp_char_t* prefix, const xp_char_t* postfix);
void xp_stx_traverse_symbol_table (
        xp_stx_t* stx, void (*func) (xp_stx_t*,xp_word_t,void*), void* data);

#ifdef __cplusplus
}
#endif

#endif
