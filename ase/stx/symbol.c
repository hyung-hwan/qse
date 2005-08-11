/*
 * $Id: symbol.c,v 1.21 2005-08-11 09:57:54 bacon Exp $
 */

#include <xp/stx/symbol.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>

static void __grow_symtab (xp_stx_t* stx)
{
	xp_word_t capa, ncapa, i, j;
	xp_word_t* nspace;

	capa = stx->symtab.capacity;
	ncapa = capa << 1;

	nspace = (xp_word_t*)xp_malloc(xp_sizeof(xp_word_t) * ncapa);
	if (nspace == XP_NULL) {
		/* TODO: handle memory error */
	}

	for (i = 0; i < capa; i++) {
		xp_word_t x = stx->symtab.datum[i];
		if (x == stx->nil) continue;

		j = xp_stx_strxhash (
			XP_STX_DATA(stx,x), XP_STX_SIZE(stx,x)) % ncapa;

		while (1) {
			if (nspace[j] == stx->nil) {
				nspace[j] = x;
				break;
			}
			j = (j % ncapa) + 1;
		}
	}

	stx->symtab.capacity = ncapa;	
	xp_free (stx->symtab.datum);
	stx->symtab.datum = nspace;
}

xp_word_t xp_stx_new_symbol (xp_stx_t* stx, const xp_char_t* name)
{
	return xp_stx_new_symbolx (stx, name, xp_strlen(name));
}

xp_word_t xp_stx_new_symbolx (
	xp_stx_t* stx, const xp_char_t* name, xp_word_t len)
{
	xp_word_t capa, hash, index, size, x;

	capa = stx->symtab.capacity;
	size = stx->symtab.size;

	if (capa <= size + 1) {
		__grow_symtab (stx);
		capa = stx->symtab.capacity;
	}

	hash = xp_stx_strxhash(name,len);
	index = hash % stx->symtab.capacity;

	while (1) {
		x = stx->symtab.datum[index];
		if (x == stx->nil) {
			/* insert a new item into an empty slot */
			x = xp_stx_alloc_char_objectx (stx, name, len);
			XP_STX_CLASS(stx,x) = stx->class_symbol;
			stx->symtab.datum[index] = x;
			stx->symtab.size++;
			break;
		}

		if (xp_strxncmp(name, len, 
			XP_STX_DATA(stx,x), XP_STX_SIZE(stx,x)) == 0) break;

		index = (index % stx->symtab.capacity) + 1;
	}

	return x;
}

void xp_stx_traverse_symbol_table (
	xp_stx_t* stx, void (*func) (xp_stx_t*,xp_word_t,void*), void* data)
{
	xp_word_t index, x;

	for (index = 0; index < stx->symtab.capacity; index++) {
		x = stx->symtab.datum[index];
		if (x != stx->nil) func (stx, x, data);
	}
}

