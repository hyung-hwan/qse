/*
 * $Id: symbol.c,v 1.19 2005-08-03 16:00:01 bacon Exp $
 */

#include <xp/stx/symbol.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>

xp_word_t xp_stx_new_symlink (xp_stx_t* stx, xp_word_t symbol)
{
	xp_word_t x;

	x = xp_stx_alloc_word_object(
		stx, XP_NULL, XP_STX_SYMLINK_SIZE, XP_NULL, 0);
	XP_STX_CLASS(stx,x) = stx->class_symlink;
	XP_STX_WORD_AT(stx,x,XP_STX_SYMLINK_LINK) = stx->nil;
	XP_STX_WORD_AT(stx,x,XP_STX_SYMLINK_SYMBOL) = symbol;

	return x;
}

xp_word_t xp_stx_new_symbol (xp_stx_t* stx, const xp_char_t* name)
{
	return xp_stx_new_symbolx (stx, name, xp_strlen(name));
}

xp_word_t xp_stx_new_symbolx (
	xp_stx_t* stx, const xp_char_t* name, xp_word_t len)
{
	xp_word_t x, hash, table, link, next;

	table = stx->symbol_table;
	hash = xp_stx_strxhash(name,len) % XP_STX_SIZE(stx,table);
	link = XP_STX_WORD_AT(stx,table,hash);

	if (link == stx->nil) {
		x = xp_stx_alloc_char_objectx (stx, name, len);
		XP_STX_CLASS(stx,x) = stx->class_symbol;
		XP_STX_WORD_AT(stx,table,hash) = xp_stx_new_symlink(stx,x);
	}
	else {
		do {
			x = XP_STX_WORD_AT(stx,link,XP_STX_SYMLINK_SYMBOL);
			xp_assert (xp_stx_classof(stx,x) == stx->class_symbol);

			if (xp_strxcmp ( 
				XP_STX_DATA(stx,x),
				XP_STX_SIZE(stx,x), name) == 0) return x;

			next = XP_STX_WORD_AT(stx,link,XP_STX_SYMLINK_LINK);
			if (next == stx->nil) {
				x = xp_stx_alloc_char_objectx (stx, name, len);
				XP_STX_CLASS(stx,x) = stx->class_symbol;
				XP_STX_WORD_AT(stx,link,XP_STX_SYMLINK_LINK) = 
					xp_stx_new_symlink(stx,x);
				break;
			}

			link = next;
		} while (1);
	}
		
	return x;
}

void xp_stx_traverse_symbol_table (
	xp_stx_t* stx, void (*func) (xp_stx_t*,xp_word_t,void*), void* data)
{
	xp_word_t link;
	xp_word_t size;
	xp_word_t table;

	table = stx->symbol_table;
	size = XP_STX_SIZE(stx,table);
	
	while (size-- > 0) {
		link = XP_STX_WORD_AT(stx,table,size);

		while (link != stx->nil) {
			func (stx, XP_STX_WORD_AT(stx,link,XP_STX_SYMLINK_SYMBOL), data);
			link = XP_STX_WORD_AT(stx,link,XP_STX_SYMLINK_LINK);
		}
	}
}

xp_word_t xp_stx_new_symbolx (
	xp_stx_t* stx, const xp_char_t* name, xp_word_t len)
{
	xp_word_t capa, hash, index;

	capa = stx->symtab.capacity;
	size = stx->symtab.size;

	if (capa <= size + 1) {
		__grow_symtab (stx);
	}

	hash = xp_stx_strxhash(name,len);
	index = hash % stx->symtab.capacity;

	while (1) {
		symbol = stx->symtab.datum[index];
		if (symbol != stx->nil) break;

		if (xp_strxncmp(name, len, 
			XP_STX_DATA(stx,symbol), XP_STX_SIZE(stx,symbol)) == 0) break;

		index = index % stx->symtabl.capacity + 1;
	}
}
