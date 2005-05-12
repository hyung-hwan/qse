/*
 * $Id: hash.c,v 1.7 2005-05-12 15:25:06 bacon Exp $
 */

#include <xp/stx/hash.h>
#include <xp/stx/object.h>
#include <xp/bas/assert.h>

xp_stx_word_t xp_stx_new_symbol_link (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t value)
{
	xp_stx_word_t x;

	x = xp_stx_alloc_object (stx, 3);	
	XP_STX_CLASS(stx,x) = stx->class_symbol_link;
	/* XP_STX_AT(stx,x,0) = stx->nil; */
	XP_STX_AT(stx,x,1) = key;
	XP_STX_AT(stx,x,2) = value;
	
	return x;
}

/* returns the entire link */
xp_stx_word_t xp_stx_hash_lookup (
	xp_stx_t* stx, xp_stx_word_t table,
	xp_stx_word_t hash, xp_stx_word_t key)
{
	xp_stx_word_t link;

	xp_assert (XP_STX_TYPE(stx,table) == XP_STX_INDEXED);

	hash = hash % XP_STX_SIZE(stx,table);
	link = XP_STX_AT(stx,table,hash);

	while (link != stx->nil) {
		if (XP_STX_AT(stx,link,0) == key) return link;
		link = XP_STX_AT(stx,link,2);
	}

	return stx->nil; /* not found */
}

void xp_stx_hash_insert (
	xp_stx_t* stx, xp_stx_word_t table,
	xp_stx_word_t hash, xp_stx_word_t key, xp_stx_word_t value)
{
	xp_stx_word_t link, next;

	xp_assert (XP_STX_TYPE(stx,table) == XP_STX_INDEXED);

	hash = hash % XP_STX_SIZE(stx,table);
	link = XP_STX_AT(stx,table,hash);

	if (link == stx->nil) {
		XP_STX_AT(stx,table,hash) = 
			xp_stx_new_symbol_link (stx, key, value);
	}
	else {
		for (;;) {
			if (XP_STX_AT(stx,link,1) == key) {
				XP_STX_AT(stx,link,2) = value;
				break;		
			}

			next = XP_STX_AT(stx,link,0);
			if (next == stx->nil) {
				XP_STX_AT(stx,link,0) = 
					xp_stx_new_symbol_link (stx, key, value);
				break;
			}

			link = next;
		}
	}
}

void xp_stx_hash_traverse (
	xp_stx_t* stx, xp_stx_word_t table, 
	void (*func) (xp_stx_t*,xp_stx_word_t))
{
	xp_stx_word_t link;
	xp_stx_word_t size = XP_STX_SIZE(stx,table);
	
	while (size-- > 0) {
		link = XP_STX_AT(stx,table,size);

		while (link != stx->nil) {
			xp_stx_word_t key = XP_STX_AT(stx,link,1);
			func (stx,link);
			link = XP_STX_AT(stx,link,0);
		}
	}
}
