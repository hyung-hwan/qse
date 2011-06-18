/*
 * $Id$
 */

#include "stx.h"
#include <qse/cmn/str.h>

/* NOTE:
 * The code here implements SystemDictionary whose key is always a symbol.
 * Dictionary, on the contrary, can accept any object as a key.
 */

struct qse_stx_association_t
{
	qse_stx_objhdr_t h;
	qse_word_t       key;
	qse_word_t       value;
};
typedef struct qse_stx_association_t qse_stx_association_t;

struct qse_stx_systemdictionary_t
{
	qse_stx_objhdr_t h;
	qse_word_t       tally;

	/* variable part begins here */
	qse_word_t       slot[1];
};
typedef struct qse_stx_systemdictionary_t qse_stx_systemdictionary_t;

static qse_word_t new_association (
	qse_stx_t* stx, qse_word_t key, qse_word_t value)
{
#if 0
	qse_word_t x;

	x = qse_stx_allocwordobj (
		stx, QSE_NULL, QSE_STX_ASSOCIATION_SIZE, QSE_NULL, 0);
	if (ISNIL(stx,x)) return stx->ref.nil;

	OBJCLASS(stx,x) = stx->ref.class_association;
	WORDAT(stx,x,QSE_STX_ASSOCIATION_KEY) = key;
	WORDAT(stx,x,QSE_STX_ASSOCIATION_VALUE) = value;
	return x;
#endif
	
	qse_word_t x;

	QSE_ASSERT (REFISIDX(stx,stx->ref.class_association));
	QSE_ASSERT (!ISNIL(stx,stx->ref.class_association));

	x = qse_stx_instantiate (
		stx, stx->ref.class_association, QSE_NULL, QSE_NULL, 0);
	if (!ISNIL(stx,x))
	{
		WORDAT(stx,x,QSE_STX_ASSOCIATION_KEY) = key;
		WORDAT(stx,x,QSE_STX_ASSOCIATION_VALUE) = value;
	}
	return x;
}

static qse_word_t expand (qse_stx_t* stx, qse_word_t dic)
{
	qse_word_t oldcapa, newdic, newcapa;
	qse_stx_systemdictionary_t* oldptr, * newptr;
	
	QSE_ASSERT (REFISIDX(stx,stx->ref.class_systemdictionary));
	QSE_ASSERT (!ISNIL(stx,stx->ref.class_systemdictionary));

	QSE_ASSERTX (
		REFISIDX(stx,dic),
		"The reference is not an object index"
	);

	/* WARNING:
	 * if this assertion fails, adjust the initial size of the 
	 * system dicionary. i don't want this function to be called
	 * during the bootstrapping.
	 */
	QSE_ASSERT (OBJCLASS(stx,dic) == stx->ref.class_systemdictionary);

	/* get the current capacity excluding the tally field */
	oldcapa = OBJSIZE(stx,dic) - 1;
	
	/* instantiate a new dictionary with its capacity doubled.
	 * 1 fixed slot for the tally field is encoded is the class part. 
	 * so 'newcapa' specifies the number of slots to hold associations */
	newcapa = oldcapa * 2;
	newdic = qse_stx_instantiate (
		stx, OBJCLASS(stx,dic), 
		QSE_NULL, QSE_NULL, newcapa
	);
	if (ISNIL(stx,newdic)) return stx->ref.nil;

	/* get object pointers for easier access without using macros */
	oldptr = (qse_stx_systemdictionary_t*)PTRBYREF(stx,dic);
	newptr = (qse_stx_systemdictionary_t*)PTRBYREF(stx,newdic);
	newptr->tally = INTTOREF(stx,0);

	QSE_ASSERT (newcapa == OBJSIZE(stx,newdic)-1);

	/* reorganize the dictionary */
	while (oldcapa > 0)
	{
		qse_word_t assoc;

		assoc = oldptr->slot[--oldcapa];
		if (!ISNIL(stx,assoc))
		{
			qse_word_t index;

			index = qse_stx_hashobj (stx, WORDAT(stx,assoc,QSE_STX_ASSOCIATION_KEY)) % newcapa;
			while (!ISNIL(stx,newptr->slot[index]))
				index = (index + 1) % newcapa;
			newptr->slot[index] = assoc;
		}
	}
	
	newptr->tally = oldptr->tally;

	/* TODO: explore if dic can be immediately destroyed. */
	qse_stx_swapmem (stx, REFTOIDX(stx,dic), REFTOIDX(stx,newdic));

	return dic;
}

static qse_word_t find_basic_index (
	qse_stx_t* stx, qse_word_t dic, qse_word_t key)
{
	qse_word_t capa, index;
	qse_stx_systemdictionary_t* dicptr;

	/* ensure that dic is a system dictionary */
	QSE_ASSERT (REFISIDX(stx,dic));
	QSE_ASSERT (OBJTYPE(stx,dic) == WORDOBJ);
	QSE_ASSERT (dic == stx->ref.sysdic ||
	            OBJCLASS(stx,dic) == stx->ref.class_systemdictionary);

	/* ensure that the key is a symbol */
	QSE_ASSERT (REFISIDX(stx,key));
	QSE_ASSERT (OBJCLASS(stx,key) == stx->ref.class_symbol);
	QSE_ASSERT (OBJTYPE(stx,key) == CHAROBJ);

	capa = OBJSIZE(stx,dic) - 1; /* exclude the tally field */
	index = qse_stx_hashobj (stx, key) % capa;

	dicptr = (qse_stx_systemdictionary_t*)PTRBYREF(stx,dic);

	do
	{
		qse_word_t assoc, sym;

		assoc = dicptr->slot[index];
		if (ISNIL(stx,assoc)) break; /* not found */

		sym = WORDAT (stx, assoc, QSE_STX_ASSOCIATION_KEY);

		/* make sure that the key is a symbol */
		QSE_ASSERT (REFISIDX(stx,sym));
		QSE_ASSERT (OBJCLASS(stx,sym) == stx->ref.class_symbol);
		QSE_ASSERT (OBJTYPE(stx,sym) == CHAROBJ);

		/* check if the key matches */
		if (qse_strxncmp(
			&CHARAT(stx,key,0), OBJSIZE(stx,key),
			&CHARAT(stx,sym,0), OBJSIZE(stx,sym)) == 0) break;

		index = (index + 1) % capa;
	}
	while (1);

	/* Include the tally back when returning the association index.
	 * you can access the association with WORDAT() by using this index. */
	return index + 1; 
}

/* look up a system dictionary by a null-terminated string */
qse_word_t qse_stx_lookupdic (
	qse_stx_t* stx, qse_word_t dic, const qse_char_t* skey)
{
	qse_word_t capa, index;
	qse_stx_systemdictionary_t* dicptr;

	QSE_ASSERT (REFISIDX(stx,dic));
	QSE_ASSERT (OBJTYPE(stx,dic) == WORDOBJ);
	QSE_ASSERT (dic == stx->ref.sysdic || 
	            OBJCLASS(stx,dic) == stx->ref.class_systemdictionary);

	capa = OBJSIZE(stx,dic) - 1; /* exclude the tally field */
	index = qse_stx_hashstr (stx, skey) % capa;

	dicptr = (qse_stx_systemdictionary_t*)PTRBYREF(stx,dic);

	do
	{
		qse_word_t assoc, keyref;

		assoc = dicptr->slot[index];
		if (ISNIL(stx,assoc)) break; /* not found */

		keyref = WORDAT(stx,assoc,QSE_STX_ASSOCIATION_KEY);

		QSE_ASSERT (REFISIDX(stx,keyref));
		QSE_ASSERT (OBJCLASS(stx,keyref) == stx->ref.class_symbol);
		QSE_ASSERT (OBJTYPE(stx,keyref) == CHAROBJ);

		if (qse_strxcmp (
			&CHARAT(stx,keyref,0), OBJSIZE(stx,keyref),
			skey) == 0) break;
			
		index = (index + 1) % capa;
	}
	while (1);

	return dicptr->slot[index];
}

qse_word_t qse_stx_getdic (qse_stx_t* stx, qse_word_t dic, qse_word_t key)
{
	/* returns the association for the key. nil if it is not found */
	return WORDAT (stx, dic, find_basic_index (stx, dic, key));
}

qse_word_t qse_stx_putdic (
	qse_stx_t* stx, qse_word_t dic, qse_word_t key, qse_word_t value)
{
	qse_word_t index, capa, tally, assoc;
	qse_stx_systemdictionary_t* dicptr;

	/* the dicionary must have at least one slot excluding tally */
	QSE_ASSERT (OBJSIZE(stx,dic) > 1);

	capa = OBJSIZE(stx,dic) - 1;
	dicptr = (qse_stx_systemdictionary_t*)PTRBYREF(stx,dic);

	tally = REFTOINT(stx,dicptr->tally);
	index = find_basic_index (stx, dic, key) - 1;
	assoc = dicptr->slot[index];

	/*assoc = WORDAT(stx,dic,slot);*/

	if (ISNIL(stx,assoc))
	{
		/* the key is not found */

		if (tally + 1 >= capa) 
		{
			/* Enlarge the dictionary if there is one free slot left.
			 * The last free slot left is always maintained to be nil.
			 * The nil slot plays multiple roles.
			 *  - make sure that lookup never enters a infinite loop.
			 *  - the slot's index can be returned when no key is found.
			 */
			if (ISNIL(stx, expand (stx, dic))) return stx->ref.nil;
		
			capa = OBJSIZE(stx,dic) - 1;
			dicptr = (qse_stx_systemdictionary_t*)PTRBYREF(stx,dic);
			/* tally must remain the same after expansion */
			QSE_ASSERT (tally == REFTOINT(stx,dicptr->tally));

			/* find the key in the expanded dictionary again */
			index = find_basic_index (stx, dic, key) - 1;
			/* the basic index returned must point to nil meaning
			 * the key is not found */
			QSE_ASSERT (ISNIL(stx,dicptr->slot[index]));
		}

		assoc = new_association (stx, key, value);
		if (ISNIL(stx,assoc)) return stx->ref.nil;

		dicptr->slot[index] = assoc;
		dicptr->tally = INTTOREF(stx,tally+1);
	}
	else 
	{
		/* found the key. change the value */
		WORDAT(stx,assoc,QSE_STX_ASSOCIATION_VALUE) = value;
	}

	return assoc;
}

#if 0
void qse_stx_walkdic (
	qse_stx_t* stx, qse_word_t dic, 
	void (*func) (qse_stx_t*,qse_word_t,void*), void* data)
{
	qse_word_t index, assoc;
	qse_word_t size = OBJSIZE(stx,dic);
	
	for (index = 1; index < size; index++) 
	{
		assoc = WORDAT (stx, dic, index);
		if (assoc == stx->nil) continue;
		func (stx, assoc, data);
	}
}

#endif
