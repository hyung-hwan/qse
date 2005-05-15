/*
 * $Id: hash.c,v 1.9 2005-05-15 18:37:00 bacon Exp $
 */

#include <xp/stx/hash.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>
#include <xp/bas/assert.h>

#define _SYMBOL_LINK_DIMENSION  3
#define _SYMBOL_LINK_LINK       0
#define _SYMBOL_LINK_KEY        1
#define _SYMBOL_LINK_VALUE      2

xp_stx_word_t xp_stx_new_symbol_link (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t value)
{
	xp_stx_word_t x;

	x = xp_stx_alloc_object (stx, _SYMBOL_LINK_DIMENSION);	
	XP_STX_CLASS(stx,x) = stx->class_symbol_link;
	/* XP_STX_AT(stx,x,_SYMBOL_LINK_LINK) = stx->nil; */
	XP_STX_AT(stx,x,_SYMBOL_LINK_KEY) = key;
	XP_STX_AT(stx,x,_SYMBOL_LINK_VALUE) = value;
	
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
		if (XP_STX_AT(stx,link,_SYMBOL_LINK_KEY) == key) return link;
		link = XP_STX_AT(stx,link,_SYMBOL_LINK_LINK);
	}

	return stx->nil; /* not found */
}

xp_stx_word_t xp_stx_hash_lookup_symbol (
	xp_stx_t* stx, xp_stx_word_t table, 
	xp_stx_word_t hash, xp_stx_char_t* key_str)
{
	xp_stx_word_t link, key;

	xp_assert (XP_STX_TYPE(stx,table) == XP_STX_INDEXED);

	hash = hash % XP_STX_SIZE(stx,table);
	link = XP_STX_AT(stx,table,hash);

	while (link != stx->nil) {
		key = XP_STX_AT(stx,link,_SYMBOL_LINK_KEY);

		if (XP_STX_CLASS(stx,key) == stx->class_symbol &&
		    xp_stx_strxcmp (
		    	&XP_STX_CHARAT(stx,key,0), 
		    	XP_STX_SIZE(stx,key), key_str) == 0) {
			return link;
		}
		link = XP_STX_AT(stx,link,_SYMBOL_LINK_LINK);
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
				XP_STX_AT(stx,link,_SYMBOL_LINK_VALUE) = value;
				break;		
			}

			next = XP_STX_AT(stx,link,_SYMBOL_LINK_LINK);
			if (next == stx->nil) {
				XP_STX_AT(stx,link,_SYMBOL_LINK_LINK) = 
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
			func (stx,link);
			link = XP_STX_AT(stx,link,_SYMBOL_LINK_LINK);
		}
	}
}


xp_stx_word_t xp_stx_new_symbol (
	xp_stx_t* stx, const xp_stx_char_t* name)
{
	xp_stx_word_t x, hash;

	hash = xp_stx_strhash(name);
	x = xp_stx_hash_lookup_symbol(stx, stx->symbol_table, hash, name);
	if (x == stx->nil) {
		x = xp_stx_alloc_string_object (stx, name);
		XP_STX_CLASS(stx,x) = stx->class_symbol;
		xp_stx_hash_insert (stx, stx->symbol_table, hash, x, stx->nil);
	}
	else x = XP_STX_AT(stx,x,_SYMBOL_LINK_KEY);

	return x;
}

xp_stx_word_t xp_stx_new_symbol_pp (
	xp_stx_t* stx, const xp_stx_char_t* name, 
	const xp_stx_char_t* prefix, const xp_stx_char_t* postfix)
{
	xp_stx_word_t x, hash;

	hash = xp_stx_strhash(name);

	x = xp_stx_hash_lookup_symbol(stx, stx->symbol_table, hash, name);
	if (x == stx->nil) {
		x = xp_stx_allocn_string_object (stx, prefix, name, postfix, XP_NULL);
		XP_STX_CLASS(stx,x) = stx->class_symbol;
		xp_stx_hash_insert (stx, stx->symbol_table, hash, x, stx->nil);
	}
	else x = XP_STX_AT(stx,x,_SYMBOL_LINK_KEY);

	return x;
}

