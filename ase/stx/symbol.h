/*
 * $Id: symbol.h,v 1.2 2005-05-18 04:01:51 bacon Exp $
 */

#ifndef _XP_STX_SYMBOL_H_
#define _XP_STX_SYMBOL_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_word_t xp_stx_new_symbol_link (xp_stx_t* stx, xp_stx_word_t sym);
xp_stx_word_t xp_stx_new_symbol (xp_stx_t* stx, const xp_stx_char_t* name);
xp_stx_word_t xp_stx_new_symbol_pp (
	xp_stx_t* stx, const xp_stx_char_t* name,
	const xp_stx_char_t* prefix, const xp_stx_char_t* postfix);
void xp_stx_traverse_symbol_table (
        xp_stx_t* stx, void (*func) (xp_stx_t*,xp_stx_word_t));


#ifdef __cplusplus
}
#endif

#endif
