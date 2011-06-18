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

static qse_word_t expand (qse_stx_t* stx, qse_word_t tabref)
{
	qse_word_t oldcapa, newcapa;
	qse_word_t newtab;
	qse_stx_symtab_t* oldptr, * newptr;

	QSE_ASSERT (REFISIDX(stx,stx->ref.class_systemsymboltable));
	QSE_ASSERT (!ISNIL(stx,stx->ref.class_systemsymboltable));

	QSE_ASSERTX (
		REFISIDX(stx,tabref), 
		"The reference is not an object index"
	);

	/* This function can handle expansion of an object whose class is
	 * SystemSymbolTable. During initial bootstrapping, the class of
	 * the stock symbol table (stx->ref.symtab) may not be set properly.
	 * You must make sure that expansion is not triggered until its class
	 * is set. If this assertion fails, you must increase the value of
	 * SYMTAB_INIT_CAPA.
	 */
	QSE_ASSERT (OBJCLASS(stx,tabref) == stx->ref.class_systemsymboltable);

	/* get the current table capacity being the size of the object
	 * excluding the tally field. */
	oldcapa = OBJSIZE(stx,tabref) - 1;

	/* instantiate a new symbol table with its capacity doubled. */
	newcapa = oldcapa * 2;
	newtab = qse_stx_instantiate (
		stx, OBJCLASS(stx,tabref),
		QSE_NULL, QSE_NULL, newcapa
	); 
	if (ISNIL(stx,newtab)) return stx->ref.nil;

	oldptr = (qse_stx_symtab_t*)PTRBYREF(stx,tabref);
	newptr = (qse_stx_symtab_t*)PTRBYREF(stx,newtab);
	newptr->tally = INTTOREF (stx, 0);

	QSE_ASSERT (newcapa == OBJSIZE(stx,newtab) - 1);

	/* reorganize the symbol table */
	while (oldcapa > 0)
	{
		qse_word_t symbol;

		symbol = oldptr->slot[--oldcapa];
		if (!ISNIL(stx,symbol))
		{
			qse_word_t index;

			QSE_ASSERT (REFISIDX(stx,symbol));
			QSE_ASSERT (OBJCLASS(stx,symbol) == stx->ref.class_symbol);
			QSE_ASSERT (OBJTYPE(stx,symbol) == CHAROBJ);

			/* qse_stx_newsymbol uses qse_stx_hashstr().
			 * this function uses qse_stx_hashobj(). 
			 * both must return the same value */
			QSE_ASSERT (qse_stx_hashobj (stx, symbol) == 
			            qse_stx_hashstr (stx, &CHARAT(stx,symbol,0)));

			index = qse_stx_hashobj (stx, symbol) % newcapa;
			while (!ISNIL(stx,newptr->slot[index]))
				index = (index + 1) % newcapa;
			newptr->slot[index] = symbol;
		}
	}

	newptr->tally = oldptr->tally;
	qse_stx_swapmem (stx, REFTOIDX(stx,tabref), REFTOIDX(stx,newtab));
	return tabref;
}


static qse_word_t new_symbol (
	qse_stx_t* stx, qse_word_t tabref, const qse_char_t* name, qse_size_t len)
{
	qse_stx_symtab_t* tabptr;
	qse_word_t symref;
	qse_word_t capa, hash, index, tally;

	/* the table must have at least one slot excluding the tally field */
	QSE_ASSERT (OBJSIZE(stx,tabref) > 1);

	capa = OBJSIZE(stx,tabref) - 1; /* exclude the tally field */
	hash = qse_stx_hashstr (stx, name);
	index = hash % capa;

	tabptr = (qse_stx_symtab_t*)PTRBYREF(stx,tabref);

	do
	{
		/*symref = WORDAT (stx, tabref, index + 1);*/
		symref = tabptr->slot[index];
		if (ISNIL(stx,symref)) break; /* not found */

		QSE_ASSERT (REFISIDX(stx,symref));
		QSE_ASSERT (OBJCLASS(stx,symref) == stx->ref.class_symbol);
		QSE_ASSERT (OBJTYPE(stx,symref) == CHAROBJ);

		/*if (qse_strxcmp (
			&CHARAT(stx,symref,0), OBJSIZE(stx,symref),
			name) == 0) return symref;*/
		if (qse_strcmp (&CHARAT(stx,symref,0), name) == 0) return symref;
			
		index = (index + 1) % capa;
	}
	while (0);

	/* symbol is not found. let's create a new symbol */
	tally = REFTOINT(stx, tabptr->tally);

	/* check if the symbol table is getting full soon */
	if (tally + 1 >= capa)
	{
		/* Enlarge the symbol table before it gets full to 
		 * make sure that it has at least one free slot left
		 * after having added a new symbol. this is to help
		 * traversal end at a nil slot if no entry is found. */
		if (ISNIL (stx, expand (stx, tabref))) return stx->ref.nil;

		/* refresh the object pointer */
		tabptr = (qse_stx_symtab_t*)PTRBYREF(stx,tabref);

		/* refersh capacity and hash index */
		capa = OBJSIZE(stx,tabref) - 1; /* exclude the tally field */
		index = hash % capa;

		/* after expansion, the tally must still be the same */
		QSE_ASSERT (tally == REFTOINT (stx, tabptr->tally));
	}

	symref = qse_stx_alloccharobj (stx, name, len);
	if (!ISNIL(stx,symref))
	{
		OBJCLASS(stx,symref) = stx->ref.class_symbol;
		tabptr->tally = INTTOREF (stx, tally + 1);
		tabptr->slot[index] = symref;
	}

	return symref;
}

qse_word_t qse_stx_newsymbol (qse_stx_t* stx, const qse_char_t* name)
{
	return new_symbol (stx, stx->ref.symtab, name, qse_strlen(name));
}

qse_word_t qse_stx_newsymbolx (qse_stx_t* stx, const qse_char_t* name, qse_size_t len)
{
	return new_symbol (stx, stx->ref.symtab, name, len);
}

#if 0
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
