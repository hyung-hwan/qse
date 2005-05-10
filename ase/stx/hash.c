/*
 * $Id: hash.c,v 1.4 2005-05-10 06:08:57 bacon Exp $
 */

#include <xp/stx/hash.h>
#include <xp/stx/object.h>
#include <xp/bas/assert.h>

xp_stx_word_t xp_stx_new_link (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t value)
{
	xp_stx_word_t x;

	x = xp_stx_alloc_object (stx, 3);	
	XP_STX_CLASS(stx,x) = stx->link_class;
	XP_STX_AT(stx,x,0) = key;
	XP_STX_AT(stx,x,1) = value;
	/* XP_STX_AT(stx,x,2) = stx->nil; */
	
	return x;
}

/* returns the entire link */
xp_stx_word_t xp_stx_hash_lookup (
	xp_stx_t* stx, xp_stx_word_t table,
	xp_stx_word_t hash, xp_stx_word_t key)
{
	xp_stx_word_t harr, link;

	xp_assert (XP_STX_TYPE(stx,table) == XP_STX_INDEXED);

	harr = XP_STX_AT(stx,table,0);
	hash = hash % XP_STX_SIZE(stx,table);
	link = XP_STX_AT(stx,harr,hash);

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
	xp_stx_word_t harr, link, next;

	xp_assert (XP_STX_TYPE(stx,table) == XP_STX_INDEXED);

	harr = XP_STX_AT(stx,table,0);
	hash = hash % XP_STX_SIZE(stx,table);
	link = XP_STX_AT(stx,harr,hash);

	if (link == stx->nil) {
		XP_STX_AT(stx,harr,hash) = xp_stx_new_link (stx, key, value);
	}
	else {
		for (;;) {
			if (XP_STX_AT(stx,link,0) == key) {
				XP_STX_AT(stx,link,1) = value;
				break;		
			}

			next = XP_STX_AT(stx,link,2);
			if (next == stx->nil) {
				XP_STX_AT(stx,link,2) = xp_stx_new_link (stx, key, value);
				break;
			}

			link = next;
		}
	}
}
