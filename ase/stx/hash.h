/*
 * $Id: hash.h,v 1.3 2005-05-12 15:25:06 bacon Exp $
 */

#ifndef _XP_STX_HASH_H_
#define _XP_STX_HASH_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C"
#endif

xp_stx_word_t xp_stx_new_symbol_link (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t value);
xp_stx_word_t xp_stx_hash_lookup (
	xp_stx_t* stx, xp_stx_word_t table,
	xp_stx_word_t hash, xp_stx_word_t key);
void xp_stx_hash_insert (
	xp_stx_t* stx, xp_stx_word_t table,
	xp_stx_word_t hash, xp_stx_word_t key, xp_stx_word_t value);
void xp_stx_hash_traverse (
	xp_stx_t* stx, xp_stx_word_t table, 
	void (*func) (xp_stx_t*,xp_stx_word_t));


#ifdef __cplusplus
}
#endif

#endif
