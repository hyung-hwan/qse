/*
 * $Id: hash.h,v 1.5 2005-05-16 14:14:34 bacon Exp $
 */

#ifndef _XP_STX_HASH_H_
#define _XP_STX_HASH_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C"
#endif

/* hash table manipulation */
xp_stx_word_t xp_stx_new_symbol_link (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t value);
xp_stx_word_t xp_stx_hash_lookup (
	xp_stx_t* stx, xp_stx_word_t table,
	xp_stx_word_t hash, xp_stx_word_t key);
xp_stx_word_t xp_stx_hash_lookup_symbol (
	xp_stx_t* stx, xp_stx_word_t table,
	xp_stx_word_t hash, const xp_stx_char_t* key_str);
void xp_stx_hash_insert (
	xp_stx_t* stx, xp_stx_word_t table,
	xp_stx_word_t hash, xp_stx_word_t key, xp_stx_word_t value);
void xp_stx_hash_traverse (
	xp_stx_t* stx, xp_stx_word_t table, 
	void (*func) (xp_stx_t*,xp_stx_word_t));

xp_stx_word_t xp_stx_new_symbol (
	xp_stx_t* stx, const xp_stx_char_t* name);
xp_stx_word_t xp_stx_new_symbol_pp (
	xp_stx_t* stx, const xp_stx_char_t* name, 
	const xp_stx_char_t* prefix, const xp_stx_char_t* postfix);

#ifdef __cplusplus
}
#endif

#endif
