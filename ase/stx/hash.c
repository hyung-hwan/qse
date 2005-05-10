/*
 * $Id: hash.c,v 1.3 2005-05-10 06:02:19 bacon Exp $
 */

#include <xp/stx/hash.h>

xp_stx_word_t xp_stx_new_link (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t value)
{
	xp_stx_word_t x;

	x = xp_stx_alloc_object (3);	
	XP_STX_CLASS(stx,x) = hash_lookup(stx, XP_STX_TEXT("Link"));
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
	xp_stx_word_t harr, link, next;

	xp_assert (XP_STX_TYPE(stx,table) == XP_STX_INDEXED);

	harr = XP_STX_AT(stx,table,0);
	hash = link % XP_STX_SIZE(stx,table);
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
	hash = link % XP_STX_SIZE(stx,table);
	link = XP_STX_AT(stx,harr,hash);

	if (link == stx->nil) {
		new = xp_stx_new_link (stx, key, value);
		XP_STX_AT(stx,harr,hash) = new;
	}
	else {
		for (;;) {
			if (XP_STX_AT(stx,link,0) == key) {
				XP_STX_AT(stx,link,1) = value;
				break;		
			}

			next = XP_STX_AT(stx,link,2);
			if (next == stx->nil) {
				new = xp_stx_new_link (stx, key, value);
				XP_STX_AT(stx,link,2) = new;
				break;
			}

			link = next;
		}
	}
}
