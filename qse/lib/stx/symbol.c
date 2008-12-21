/*
 * $Id: symbol.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/symbol.h>
#include <qse/stx/object.h>
#include <qse/stx/misc.h>

static void __grow_symtab (qse_stx_t* stx)
{
	qse_word_t capa, ncapa, i, j;
	qse_word_t* nspace;

	capa = stx->symtab.capacity;
	ncapa = capa << 1;

	nspace = (qse_word_t*)qse_malloc(qse_sizeof(qse_word_t) * ncapa);
	if (nspace == QSE_NULL) 
	{
		/* TODO: handle memory error */
	}

	for (i = 0; i < capa; i++) 
	{
		qse_word_t x = stx->symtab.datum[i];
		if (x == stx->nil) continue;

		j = qse_stx_strxhash (
			QSE_STX_DATA(stx,x), QSE_STX_SIZE(stx,x)) % ncapa;

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
	qse_free (stx->symtab.datum);
	stx->symtab.datum = nspace;
}

qse_word_t qse_stx_new_symbol (qse_stx_t* stx, const qse_char_t* name)
{
	return qse_stx_new_symbolx (stx, name, qse_strlen(name));
}

qse_word_t qse_stx_new_symbolx (
	qse_stx_t* stx, const qse_char_t* name, qse_word_t len)
{
	qse_word_t capa, hash, index, size, x;

	capa = stx->symtab.capacity;
	size = stx->symtab.size;

	if (capa <= size + 1) 
	{
		__grow_symtab (stx);
		capa = stx->symtab.capacity;
	}

	hash = qse_stx_strxhash(name,len);
	index = hash % stx->symtab.capacity;

	while (1) 
	{
		x = stx->symtab.datum[index];
		if (x == stx->nil) 
		{
			/* insert a new item into an empty slot */
			x = qse_stx_alloc_char_objectx (stx, name, len);
			QSE_STX_CLASS(stx,x) = stx->class_symbol;
			stx->symtab.datum[index] = x;
			stx->symtab.size++;
			break;
		}

		if (qse_strxncmp(name, len, 
			QSE_STX_DATA(stx,x), QSE_STX_SIZE(stx,x)) == 0) break;

		index = (index % stx->symtab.capacity) + 1;
	}

	return x;
}

void qse_stx_traverse_symbol_table (
	qse_stx_t* stx, void (*func) (qse_stx_t*,qse_word_t,void*), void* data)
{
	qse_word_t index, x;

	for (index = 0; index < stx->symtab.capacity; index++) 
	{
		x = stx->symtab.datum[index];
		if (x != stx->nil) func (stx, x, data);
	}
}

