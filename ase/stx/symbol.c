/*
 * $Id: symbol.c,v 1.12 2005-07-05 06:28:03 bacon Exp $
 */

#include <xp/stx/symbol.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>

xp_word_t xp_stx_new_symlink (xp_stx_t* stx, xp_word_t sym)
{
	xp_word_t x;

	x = xp_stx_alloc_word_object (stx, XP_NULL, XP_STX_SYMLINK_SIZE);
	XP_STX_CLASS(stx,x) = stx->class_symlink;
	XP_STX_WORDAT(stx,x,XP_STX_SYMLINK_LINK) = stx->nil;
	XP_STX_WORDAT(stx,x,XP_STX_SYMLINK_SYMBOL) = sym;

	return x;
}

xp_word_t xp_stx_new_symbol (xp_stx_t* stx, const xp_char_t* name)
{
	xp_word_t x, hash, table, link, next;

	table = stx->symbol_table;
	hash = xp_stx_strhash(name) % XP_STX_SIZE(stx,table);
	link = XP_STX_WORDAT(stx,table,hash);

	if (link == stx->nil) {
		x = xp_stx_alloc_char_object (stx, name);
		XP_STX_CLASS(stx,x) = stx->class_symbol;
		XP_STX_WORDAT(stx,table,hash) = xp_stx_new_symlink(stx,x);
	}
	else {
		do {
			x = XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_SYMBOL);
			xp_assert (XP_STX_CLASS(stx,x) == stx->class_symbol);

			if (xp_strxcmp (
				&XP_STX_CHARAT(stx,x,0),
				XP_STX_SIZE(stx,x), name) == 0) return x;

			next = XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_LINK);
			if (next == stx->nil) {
				x = xp_stx_alloc_char_object (stx, name);
				XP_STX_CLASS(stx,x) = stx->class_symbol;
				XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_LINK) = 
					xp_stx_new_symlink(stx,x);
				break;
			}

			link = next;
		} while (1);
	}
		
	return x;
}

xp_word_t xp_stx_new_symbolx (
	xp_stx_t* stx, const xp_char_t* name, xp_word_t len)
{
	xp_word_t x, hash, table, link, next;

	table = stx->symbol_table;
	hash = xp_stx_strhash(name) % XP_STX_SIZE(stx,table);
	link = XP_STX_WORDAT(stx,table,hash);

	if (link == stx->nil) {
		x = xp_stx_alloc_char_objectx (stx, name, len);
		XP_STX_CLASS(stx,x) = stx->class_symbol;
		XP_STX_WORDAT(stx,table,hash) = xp_stx_new_symlink(stx,x);
	}
	else {
		do {
			x = XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_SYMBOL);
			xp_assert (XP_STX_CLASS(stx,x) == stx->class_symbol);

			if (xp_strxcmp (
				&XP_STX_CHARAT(stx,x,0),
				XP_STX_SIZE(stx,x), name) == 0) return x;

			next = XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_LINK);
			if (next == stx->nil) {
				x = xp_stx_alloc_char_objectx (stx, name, len);
				XP_STX_CLASS(stx,x) = stx->class_symbol;
				XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_LINK) = 
					xp_stx_new_symlink(stx,x);
				break;
			}

			link = next;
		} while (1);
	}
		
	return x;
}

xp_word_t xp_stx_new_symbol_pp (
	xp_stx_t* stx, const xp_char_t* name,
	const xp_char_t* prefix, const xp_char_t* postfix)
{
	xp_word_t x, hash, table, link, next;

	table = stx->symbol_table;
	hash = xp_stx_strhash(name) % XP_STX_SIZE(stx,table);
	link = XP_STX_WORDAT(stx,table,hash);

	if (link == stx->nil) {
		x = xp_stx_allocn_char_object (stx, prefix, name, postfix);
		XP_STX_CLASS(stx,x) = stx->class_symbol;
		XP_STX_WORDAT(stx,table,hash) = xp_stx_new_symlink(stx,x);
	}
	else {
		do {
			x = XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_SYMBOL);
			xp_assert (XP_STX_CLASS(stx,x) == stx->class_symbol);

			if (xp_strxcmp (
				&XP_STX_CHARAT(stx,x,0),
				XP_STX_SIZE(stx,x), name) == 0) return x;

			next = XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_LINK);
			if (next == stx->nil) {
				x = xp_stx_allocn_char_object (stx, prefix, name, postfix);
				XP_STX_CLASS(stx,x) = stx->class_symbol;
				XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_LINK) = 
					xp_stx_new_symlink(stx,x);
				break;
			}

			link = next;
		} while (1);
	}
	
		
	return x;
}

void xp_stx_traverse_symbol_table (
	xp_stx_t* stx, void (*func) (xp_stx_t*,xp_word_t))
{
	xp_word_t link;
	xp_word_t size;
	xp_word_t table;

	table = stx->symbol_table;
	size = XP_STX_SIZE(stx,table);
	
	while (size-- > 0) {
		link = XP_STX_WORDAT(stx,table,size);

		while (link != stx->nil) {
			func (stx,XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_SYMBOL));
			link = XP_STX_WORDAT(stx,link,XP_STX_SYMLINK_LINK);
		}
	}
}
