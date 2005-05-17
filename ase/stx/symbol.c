/*
 * $Id: symbol.c,v 1.1 2005-05-17 16:18:56 bacon Exp $
 */

#include <xp/stx/symbol.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>
#include <xp/bas/assert.h>

#define SYMBOL_LINK_DIMENSION  2
#define SYMBOL_LINK_LINK       0
#define SYMBOL_LINK_SYMBOL     1

xp_stx_word_t xp_stx_new_symbol_link (xp_stx_t* stx, xp_stx_word_t sym)
{
	xp_stx_word_t x;

	x = xp_stx_alloc_object (stx, SYMBOL_LINK_DIMENSION);
	XP_STX_CLASS(stx,x) = stx->class_symbol_link;
	/*XP_STX_AT(stx,x,SYMBOL_LINK_LINK) = stx->nil;*/
	XP_STX_AT(stx,x,SYMBOL_LINK_SYMBOL) = sym;

	return x;
}

xp_stx_word_t xp_stx_new_symbol (xp_stx_t* stx, const xp_stx_char_t* name)
{
	xp_stx_word_t x, hash, table, link, next;

	table = stx->symbol_table;
	hash = xp_stx_strhash(name) % XP_STX_SIZE(stx,table);
	link = XP_STX_AT(stx,table,hash);

	if (link == stx->nil) {
		x = xp_stx_alloc_string_object (stx, name);
		XP_STX_CLASS(stx,x) = stx->class_symbol;
		XP_STX_AT(stx,table,hash) = xp_stx_new_symbol_link(stx,x);
	}
	else {
		do {
			x = XP_STX_AT(stx,link,SYMBOL_LINK_SYMBOL);
			xp_assert (XP_STX_CLASS(stx,x) == stx->class_symbol);

			if (xp_stx_strxcmp (
				&XP_STX_CHARAT(stx,x,0),
				XP_STX_SIZE(stx,x), name) == 0) return x;

			next = XP_STX_AT(stx,link,SYMBOL_LINK_LINK);
			if (next == stx->nil) {
				x = xp_stx_alloc_string_object (stx, name);
				XP_STX_CLASS(stx,x) = stx->class_symbol;
				XP_STX_AT(stx,link,SYMBOL_LINK_LINK) = 
					xp_stx_new_symbol_link(stx,x);
				break;
			}

			link = next;
		} while (1);
	}
	
		
	return x;
}

xp_stx_word_t xp_stx_new_symbol_pp (
	xp_stx_t* stx, const xp_stx_char_t* name,
	const xp_stx_char_t* prefix, const xp_stx_char_t* postfix)
{
	xp_stx_word_t x, hash, table, link, next;

	table = stx->symbol_table;
	hash = xp_stx_strhash(name) % XP_STX_SIZE(stx,table);
	link = XP_STX_AT(stx,table,hash);

	if (link == stx->nil) {
		x = xp_stx_allocn_string_object (stx, prefix, name, postfix);
		XP_STX_CLASS(stx,x) = stx->class_symbol;
		XP_STX_AT(stx,table,hash) = xp_stx_new_symbol_link(stx,x);
	}
	else {
		do {
			x = XP_STX_AT(stx,link,SYMBOL_LINK_SYMBOL);
			xp_assert (XP_STX_CLASS(stx,x) == stx->class_symbol);

			if (xp_stx_strxcmp (
				&XP_STX_CHARAT(stx,x,0),
				XP_STX_SIZE(stx,x), name) == 0) return x;

			next = XP_STX_AT(stx,link,SYMBOL_LINK_LINK);
			if (next == stx->nil) {
				x = xp_stx_allocn_string_object (stx, prefix, name, postfix);
				XP_STX_CLASS(stx,x) = stx->class_symbol;
				XP_STX_AT(stx,link,SYMBOL_LINK_LINK) = 
					xp_stx_new_symbol_link(stx,x);
				break;
			}

			link = next;
		} while (1);
	}
	
		
	return x;
}
