/*
 * $Id$
 */

#include "stx.h"
#include <qse/cmn/str.h>

/* Implements global symbol table */

struct qse_stx_symtab_t
{
	qse_stx_objhdr_t h;
	qse_word_t tally;
	qse_word_t slot[1];	
};

typedef struct qse_stx_symtab_t qse_stx_symtab_t;

qse_word_t qse_stx_newsymbol (
	qse_stx_t* stx, qse_word_t tabref, const qse_char_t* name)
{
	qse_stx_symtab_t* tabptr;
	qse_word_t symref;
	qse_stx_charobjptr_t symptr;
	qse_word_t capa, hash, count;

#if 0
	/* the table must have at least one slot excluding the tally field */
	QSE_ASSERT (OBJSIZE(stx,tabref) > 1);
#endif


	capa = OBJSIZE(stx,tabref) - 1; /* exclude the tally field */
	hash = qse_stx_hashstr (stx, name) % capa;

	tabptr = (qse_stx_symtab_t*) PTRBYREF (stx, tabref);
	for (count = 0; count < capa; count++)
	{
		symref = tabptr->slot[hash];
		if (symref == stx->ref.nil) break; /* not found */

		QSE_ASSERT (OBJTYPE(stx,symref) == CHAROBJ);
		symptr = (qse_stx_charobjptr_t) PTRBYREF (stx, symref);

		if (qse_strcmp (name, symptr->fld) == 0) return symref;
			
		hash = (hash + 1) % capa;
	}

	if (tabptr->tally >= capa)
	{
#if 0
/* TODO: write this part....
		if (grow (stx, tab) <= -1) return -1;
		/* refresh tally */
		tally = QSE_STX_REFTOINT(QSE_STX_WORDAT(stx,tab,QSE_STX_SET_TALLY));
#endif
	}

	symref = qse_stx_alloccharobj (stx, name, qse_strlen(name));
	if (symref != stx->ref.nil) 
	{
		OBJCLASS(stx,symref) = stx->ref.class_symbol;
		tabptr->slot[hash] = symref;
	}
	return symref;
}

#if 0
#include "stx.h"

static int __grow_symtab (qse_stx_t* stx)
{
	qse_word_t capa, ncapa, i, j;
	qse_word_t* nspace;

	capa = stx->symtab.capa;
	ncapa = capa << 1; /* double the capacity */

/* TODO: allocate symbol table from stx->mem......... */
	nspace = (qse_word_t*) QSE_MMGR_ALLOC (
		stx->mmgr, ncapa * QSE_SIZEOF(*nspace)
	);
	if (nspace == QSE_NULL) 
	{
		/* TODO: handle memory error */
		qse_stx_seterrnum (stx, QSE_STX_ENOMEM);
		return -1;
	}

	for (i = 0; i < capa; i++) 
	{
		qse_word_t x = stx->symtab.slot[i];
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

	stx->symtab.capa = ncapa;	
	QSE_MMGR_FREE (stx->mmgr, stx->symtab.slot);
	stx->symtab.slot = nspace;

	return 0;
}

qse_word_t qse_stx_newsym (qse_stx_t* stx, const qse_char_t* name)
{
	return qse_stx_newsymwithlen (stx, name, qse_strlen(name));
}

qse_word_t qse_stx_newsymwithlen (qse_stx_t* stx, const qse_char_t* name, qse_word_t len)
{
	qse_word_t capa, hash, index, size, x;

	capa = stx->symtab.capa;
	size = stx->symtab.size;

	if (capa <= size + 1) 
	{
		if (__grow_symtab (stx) <= -1)
		{
/* TODO: .... */
		}
		capa = stx->symtab.capa;
	}

	hash = qse_stx_strxhash(name,len);
	index = hash % stx->symtab.capa;

	while (1) 
	{
		x = stx->symtab.slot[index];
		if (x == stx->nil) 
		{
			/* insert a new item into an empty slot */
			x = qse_stx_alloc_char_objectx (stx, name, len);
			QSE_STX_CLASS(stx,x) = stx->class_symbol;
			stx->symtab.slot[index] = x;
			stx->symtab.size++;
			break;
		}

		if (qse_strxncmp (name, len, QSE_STX_DATA(stx,x), QSE_STX_SIZE(stx,x)) == 0) break;

		index = (index % stx->symtab.capa) + 1;
	}

	return x;
}

void qse_stx_traverse_symbol_table (
	qse_stx_t* stx, void (*func) (qse_stx_t*,qse_word_t,void*), void* data)
{
	qse_word_t index, x;

	for (index = 0; index < stx->symtab.capa; index++) 
	{
		x = stx->symtab.slot[index];
		if (x != stx->nil) func (stx, x, data);
	}
}
#endif
