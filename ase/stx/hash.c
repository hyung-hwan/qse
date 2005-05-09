/*
 * $Id: hash.c,v 1.2 2005-05-09 15:55:04 bacon Exp $
 */

#include <xp/stx/hash.h>

int xp_stx_new_link (xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t value)
{
	xp_stx_word_t x;

	x = xp_stx_alloc_object (3);	
	XP_STX_CLASS(stx,x) = hash_lookup(stx, XP_STX_TEXT("Link"));
	XP_STX_AT(stx,x,0) = key;
	XP_STX_AT(stx,x,1) = value;
	/* XP_STX_AT(stx,x,2) = stx->nil; */
	
	return x;
}

void xp_stx_hash_insert (
	xp_stx_t* stx, xp_stx_word_t hash, 
	xp_stx_word_t key, xp_stx_word_t value)
{
	xp_stx_word_t oaha, link;

	/* the first instance variable is an open addressing hash array */
	oaha = XP_STX_AT(stx,stx->globals,0);

	hash = hash % oaha_size;
	link = XP_STX_AT(stx,oaha,hash);

	if (link == stx->nil || link == key) {
		XP_STX_AT(oaha, hash + 1
	}

	for (;;) {
		if (link == stx->nil) {
			new = xp_stx_new_link (stx, key, value);
			
		}

		/*
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
		*/
	}
}
