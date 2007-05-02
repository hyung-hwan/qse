/*
 * $Id: symbol.c,v 1.3 2007/04/30 08:32:41 bacon Exp $
 */

#include <ase/stx/symbol.h>
#include <ase/stx/object.h>
#include <ase/stx/misc.h>

static void __grow_symtab (ase_stx_t* stx)
{
	ase_word_t capa, ncapa, i, j;
	ase_word_t* nspace;

	capa = stx->symtab.capacity;
	ncapa = capa << 1;

	nspace = (ase_word_t*)ase_malloc(ase_sizeof(ase_word_t) * ncapa);
	if (nspace == ASE_NULL) 
	{
		/* TODO: handle memory error */
	}

	for (i = 0; i < capa; i++) 
	{
		ase_word_t x = stx->symtab.datum[i];
		if (x == stx->nil) continue;

		j = ase_stx_strxhash (
			ASE_STX_DATA(stx,x), ASE_STX_SIZE(stx,x)) % ncapa;

		while (1) 
		{
			if (nspace[j] == stx->nil) 
			{
				nspace[j] = x;
				break;
			}
			j = (j % ncapa) + 1;
		}
	}

	stx->symtab.capacity = ncapa;	
	ase_free (stx->symtab.datum);
	stx->symtab.datum = nspace;
}

ase_word_t ase_stx_new_symbol (ase_stx_t* stx, const ase_char_t* name)
{
	return ase_stx_new_symbolx (stx, name, ase_strlen(name));
}

ase_word_t ase_stx_new_symbolx (
	ase_stx_t* stx, const ase_char_t* name, ase_word_t len)
{
	ase_word_t capa, hash, index, size, x;

	capa = stx->symtab.capacity;
	size = stx->symtab.size;

	if (capa <= size + 1) 
	{
		__grow_symtab (stx);
		capa = stx->symtab.capacity;
	}

	hash = ase_stx_strxhash(name,len);
	index = hash % stx->symtab.capacity;

	while (1) 
	{
		x = stx->symtab.datum[index];
		if (x == stx->nil) 
		{
			/* insert a new item into an empty slot */
			x = ase_stx_alloc_char_objectx (stx, name, len);
			ASE_STX_CLASS(stx,x) = stx->class_symbol;
			stx->symtab.datum[index] = x;
			stx->symtab.size++;
			break;
		}

		if (ase_strxncmp(name, len, 
			ASE_STX_DATA(stx,x), ASE_STX_SIZE(stx,x)) == 0) break;

		index = (index % stx->symtab.capacity) + 1;
	}

	return x;
}

void ase_stx_traverse_symbol_table (
	ase_stx_t* stx, void (*func) (ase_stx_t*,ase_word_t,void*), void* data)
{
	ase_word_t index, x;

	for (index = 0; index < stx->symtab.capacity; index++) 
	{
		x = stx->symtab.datum[index];
		if (x != stx->nil) func (stx, x, data);
	}
}

