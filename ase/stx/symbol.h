/*
 * $Id: symbol.h,v 1.5 2005-05-23 15:51:03 bacon Exp $
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
	xp_stx_word_t link;
	xp_stx_word_t symbol;
};

typedef struct xp_stx_symlink_t xp_stx_symlink_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_word_t xp_stx_new_symbol_link (xp_stx_t* stx, xp_stx_word_t sym);

xp_stx_word_t xp_stx_new_symbol (
	xp_stx_t* stx, const xp_stx_char_t* name);
xp_stx_word_t xp_stx_new_symbolx (
	xp_stx_t* stx, const xp_stx_char_t* name, xp_stx_word_t len);

xp_stx_word_t xp_stx_new_symbol_pp (
	xp_stx_t* stx, const xp_stx_char_t* name,
	const xp_stx_char_t* prefix, const xp_stx_char_t* postfix);
void xp_stx_traverse_symbol_table (
        xp_stx_t* stx, void (*func) (xp_stx_t*,xp_stx_word_t));

#ifdef __cplusplus
}
#endif

#endif
