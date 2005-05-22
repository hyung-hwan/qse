/*
 * $Id: hash.h,v 1.8 2005-05-22 15:03:20 bacon Exp $
 */

#ifndef _XP_STX_HASH_H_
#define _XP_STX_HASH_H_

#include <xp/stx/stx.h>

#define XP_STX_PAIRLINK_SIZE     3
#define XP_STX_PAIRLINK_LINK     0
#define XP_STX_PAIRLINK_KEY      1
#define XP_STX_PAIRLINK_VALUE    2

struct xp_stx_pairlink_t
{
	xp_stx_objhdr_t header;
	xp_stx_word_t link;
	xp_stx_word_t key;
	xp_stx_word_t value;
};

typedef struct xp_stx_pairlink_t xp_stx_pairlink_t;

#ifdef __cplusplus
extern "C"
#endif

/* hash table manipulation */
xp_stx_word_t xp_stx_new_plink (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t value);
xp_stx_word_t xp_stx_hash_lookup (
	xp_stx_t* stx, xp_stx_word_t table,
	xp_stx_word_t hash, xp_stx_word_t key);
xp_stx_word_t xp_stx_hash_lookup_symbol (
	xp_stx_t* stx, xp_stx_word_t table, const xp_stx_char_t* name);
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
