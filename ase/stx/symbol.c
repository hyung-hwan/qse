/*
 * $Id: symbol.c,v 1.2 2005-05-18 04:01:51 bacon Exp $
 */

#include <xp/stx/symbol.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>
#include <xp/bas/assert.h>

#define SYMLINK_SIZE   2
#define SYMLINK_LINK   0
#define SYMLINK_SYMBOL 1

xp_stx_word_t xp_stx_new_symlink (xp_stx_t* stx, xp_stx_word_t sym)
{
	xp_stx_word_t x;

	x = xp_stx_alloc_object (stx, SYMLINK_SIZE);
	XP_STX_CLASS(stx,x) = stx->class_symlink;
	/*XP_STX_AT(stx,x,SYMLINK_LINK) = stx->nil;*/
	XP_STX_AT(stx,x,SYMLINK_SYMBOL) = sym;

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
		XP_STX_AT(stx,table,hash) = xp_stx_new_symlink(stx,x);
	}
	else {
		do {
			x = XP_STX_AT(stx,link,SYMLINK_SYMBOL);
			xp_assert (XP_STX_CLASS(stx,x) == stx->class_symbol);

			if (xp_stx_strxcmp (
				&XP_STX_CHARAT(stx,x,0),
				XP_STX_SIZE(stx,x), name) == 0) return x;

			next = XP_STX_AT(stx,link,SYMLINK_LINK);
			if (next == stx->nil) {
				x = xp_stx_alloc_string_object (stx, name);
				XP_STX_CLASS(stx,x) = stx->class_symbol;
				XP_STX_AT(stx,link,SYMLINK_LINK) = 
					xp_stx_new_symlink(stx,x);
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
		XP_STX_AT(stx,table,hash) = xp_stx_new_symlink(stx,x);
	}
	else {
		do {
			x = XP_STX_AT(stx,link,SYMLINK_SYMBOL);
			xp_assert (XP_STX_CLASS(stx,x) == stx->class_symbol);

			if (xp_stx_strxcmp (
				&XP_STX_CHARAT(stx,x,0),
				XP_STX_SIZE(stx,x), name) == 0) return x;

			next = XP_STX_AT(stx,link,SYMLINK_LINK);
			if (next == stx->nil) {
				x = xp_stx_allocn_string_object (stx, prefix, name, postfix);
				XP_STX_CLASS(stx,x) = stx->class_symbol;
				XP_STX_AT(stx,link,SYMLINK_LINK) = 
					xp_stx_new_symlink(stx,x);
				break;
			}

			link = next;
		} while (1);
	}
	
		
	return x;
}

void xp_stx_traverse_symbol_table (
	xp_stx_t* stx, void (*func) (xp_stx_t*,xp_stx_word_t))
{
	xp_stx_word_t link;
	xp_stx_word_t size;
	xp_stx_word_t table;

	table = stx->symbol_table;
	size = XP_STX_SIZE(stx,table);
	
	while (size-- > 0) {
		link = XP_STX_AT(stx,table,size);

		while (link != stx->nil) {
			func (stx,XP_STX_AT(stx,link,SYMLINK_SYMBOL));
			link = XP_STX_AT(stx,link,SYMLINK_LINK);
		}
	}
}
