/*
 * $Id: hash.c,v 1.13 2005-05-19 15:04:21 bacon Exp $
 */

#include <xp/stx/hash.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>
#include <xp/bas/assert.h>

xp_stx_word_t xp_stx_new_pairlink (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t value)
{
	xp_stx_word_t x;

	x = xp_stx_alloc_object (stx, XP_STX_PAIRLINK_SIZE);	
	XP_STX_CLASS(stx,x) = stx->class_pairlink;
	/* XP_STX_AT(stx,x,XP_STX_PAIRLINK_LINK) = stx->nil; */
	XP_STX_AT(stx,x,XP_STX_PAIRLINK_KEY) = key;
	XP_STX_AT(stx,x,XP_STX_PAIRLINK_VALUE) = value;
	
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
		if (XP_STX_AT(stx,link,XP_STX_PAIRLINK_KEY) == key) return link;
		link = XP_STX_AT(stx,link,XP_STX_PAIRLINK_LINK);
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
			xp_stx_new_pairlink (stx, key, value);
	}
	else {
		for (;;) {
			if (XP_STX_AT(stx,link,1) == key) {
				XP_STX_AT(stx,link,XP_STX_PAIRLINK_VALUE) = value;
				break;		
			}

			next = XP_STX_AT(stx,link,XP_STX_PAIRLINK_LINK);
			if (next == stx->nil) {
				XP_STX_AT(stx,link,XP_STX_PAIRLINK_LINK) = 
					xp_stx_new_pairlink (stx, key, value);
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
			func (stx,link);
			link = XP_STX_AT(stx,link,XP_STX_PAIRLINK_LINK);
		}
	}
}

