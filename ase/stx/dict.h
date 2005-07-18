/*
 * $Id: dict.h,v 1.1 2005-07-18 11:53:01 bacon Exp $
 */

#ifndef _XP_STX_DICT_H_
#define _XP_STX_DICT_H_

#include <xp/stx/stx.h>

#define XP_STX_ASSOCIATION_SIZE  2
#define XP_STX_ASSOCIATION_KEY   0
#define XP_STX_ASSOCIATION_VALUE 1

struct xp_stx_association_t
{
	xp_stx_objhdr_t header;
	xp_word_t key;
	xp_word_t value;
};

typedef struct xp_stx_association_t xp_stx_association_t;

#ifdef __cplusplus
extern "C"
#endif

xp_word_t xp_stx_new_association (
	xp_stx_t* stx, xp_word_t key, xp_word_t value);

#if 0
xp_word_t xp_stx_hash_lookup (
	xp_stx_t* stx, xp_word_t table,
	xp_word_t hash, xp_word_t key);
xp_word_t xp_stx_hash_lookup_symbol (
	xp_stx_t* stx, xp_word_t table, const xp_char_t* name);
void xp_stx_hash_insert (
	xp_stx_t* stx, xp_word_t table,
	xp_word_t hash, xp_word_t key, xp_word_t value);
void xp_stx_hash_traverse (
	xp_stx_t* stx, xp_word_t table, 
	void (*func) (xp_stx_t*,xp_word_t,void*), void* data);
#endif

#ifdef __cplusplus
}
#endif

#endif
