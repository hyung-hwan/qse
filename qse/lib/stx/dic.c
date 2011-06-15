/*
 * $Id$
 */

#include "stx.h"
#include <qse/cmn/str.h>

/* NOTE:
 * The code here implements SystemDictionary whose key is always a symbol.
 * Dictionary, on the contrary, can accept any object as a key.
 */

struct qse_stx_assoc_t
{
	qse_stx_objhdr_t h;
	qse_word_t       key;
	qse_word_t       value;
};
typedef struct qse_stx_assoc_t qse_stx_assoc_t;

struct qse_stx_dic_t
{
	qse_stx_objhdr_t h;
	qse_word_t       tally;

	/* variable part begins here */
	qse_word_t       slot[1];
};
typedef struct qse_stx_dic_t qse_stx_dic_t;

static qse_word_t new_assoc (
	qse_stx_t* stx, qse_word_t key, qse_word_t value)
{
	qse_word_t x;

	x = qse_stx_allocwordobj (
		stx, QSE_NULL, QSE_STX_ASSOC_SIZE, QSE_NULL, 0);
	if (x == stx->ref.nil) return stx->ref.nil;

	OBJCLASS(stx,x) = stx->ref.class_association;
	WORDAT(stx,x,QSE_STX_ASSOC_KEY) = key;
	WORDAT(stx,x,QSE_STX_ASSOC_VALUE) = value;

	return x;
}

static qse_word_t expand (qse_stx_t* stx, qse_word_t dic)
{
	qse_word_t newref, size, index, assoc;
	
	/* WARNING:
	 * if this assertion fails, adjust the initial size of the 
	 * system dicionary. i don't want this function to be called
	 * during the bootstrapping.
	 */
	QSE_ASSERT (stx->ref.class_systemdictionary != stx->ref.nil);
	QSE_ASSERT (REFISIDX(stx,dic));
	QSE_ASSERT (OBJCLASS(stx,dic) == stx->ref.class_systemdictionary);

	size = OBJSIZE(stx,dic);
	newref = qse_stx_instantiate (stx, 
		OBJCLASS(stx,dic), QSE_NULL, QSE_NULL, (size - 1) * 2); 
	if (newref == stx->ref.nil) return stx->ref.nil;
	WORDAT(stx,newref,QSE_STX_DIC_TALLY) = INTTOREF (stx, 0);		

	for (index = 1; index < size; index++) 
	{
		assoc = WORDAT(stx,dic,index);
		if (assoc == stx->ref.nil) continue;

		if (qse_stx_putdic (stx, newref, 
			WORDAT(stx,assoc,QSE_STX_ASSOC_KEY),
			WORDAT(stx,assoc,QSE_STX_ASSOC_VALUE)) == stx->ref.nil)
		{
			return stx->ref.nil;
		}
	}
	
	/* TODO: explore if dic can be immediately destroyed. */
	qse_stx_swapmem (stx, REFTOIDX(stx,dic), REFTOIDX(stx,newref));

	return dic;
}

static qse_word_t find_slot (
	qse_stx_t* stx, qse_word_t dic, qse_word_t key)
{
	qse_word_t capa, hash;
	qse_stx_dic_t* dicptr;

	/* ensure that dic is a system dictionary */
	QSE_ASSERT (REFISIDX(stx,dic));
	QSE_ASSERT (OBJTYPE(stx,dic) == WORDOBJ);
	QSE_ASSERT (dic == stx->ref.sysdic ||
	            OBJCLASS(stx,key) == stx->ref.class_systemdictionary);

	/* ensure that the key is a symbol */
	QSE_ASSERT (REFISIDX(stx,key));
	QSE_ASSERT (OBJCLASS(stx,key) == stx->ref.class_symbol);
	QSE_ASSERT (OBJTYPE(stx,key) == CHAROBJ);

	capa = OBJSIZE(stx,dic) - 1; /* exclude the tally field */
	hash = qse_stx_hashobj (stx, key) % capa;

	dicptr = (qse_stx_dic_t*)PTRBYREF(stx,dic);

	do
	{
		qse_word_t assoc, sym;

		assoc = dicptr->slot[hash];
		if (assoc == stx->ref.nil) break; /* not found */

		sym = WORDAT (stx, assoc, QSE_STX_ASSOC_KEY);

		QSE_ASSERT (REFISIDX(stx,sym));
		QSE_ASSERT (OBJCLASS(stx,sym) == stx->ref.class_symbol);
		QSE_ASSERT (OBJTYPE(stx,sym) == CHAROBJ);

		if (qse_strxncmp(
			&CHARAT(stx,key,0), OBJSIZE(stx,key),
			&CHARAT(stx,sym,0), OBJSIZE(stx,sym)) == 0) break;

		hash = (hash + 1) % capa;
	}
	while (1);

	/* Include the tally back when returning the association index.
	 * you can access the association with WORDAT() by using this index. */
	return hash + 1; 
}

/* look up a system dictionary by a null-terminated string */
qse_word_t qse_stx_lookupdic (
	qse_stx_t* stx, qse_word_t dic, const qse_char_t* skey)
{
	qse_word_t capa, hash;
	qse_stx_dic_t* dicptr;

	QSE_ASSERT (REFISIDX(stx,dic));
	QSE_ASSERT (OBJTYPE(stx,dic) == WORDOBJ);
	QSE_ASSERT (dic == stx->ref.sysdic || 
	            OBJCLASS(stx,dic) == stx->ref.class_systemdictionary);

	capa = OBJSIZE(stx,dic) - 1; /* exclude the tally field */
	hash = qse_stx_hashstr (stx, skey) % capa;

	dicptr = (qse_stx_dic_t*)PTRBYREF(stx,dic);

	do
	{
		qse_word_t assoc, keyref;

		assoc = dicptr->slot[hash];
		if (assoc == stx->ref.nil) break; /* not found */

		keyref = WORDAT(stx,assoc,QSE_STX_ASSOC_KEY);

		QSE_ASSERT (REFISIDX(stx,keyref));
		QSE_ASSERT (OBJCLASS(stx,keyref) == stx->ref.class_symbol);
		QSE_ASSERT (OBJTYPE(stx,keyref) == CHAROBJ);

		if (qse_strxcmp (
			&CHARAT(stx,keyref,0), OBJSIZE(stx,keyref),
			skey) == 0) break;
			
		hash = (hash + 1) % capa;
	}
	while (1);

	return dicptr->slot[hash];
}

qse_word_t qse_stx_getdic (qse_stx_t* stx, qse_word_t dic, qse_word_t key)
{
	return WORDAT (stx, dic, find_slot (stx, dic, key));
}

qse_word_t qse_stx_putdic (
	qse_stx_t* stx, qse_word_t dic, qse_word_t key, qse_word_t value)
{
	qse_word_t slot, capa, tally, assoc;
	qse_stx_dic_t* dicptr;

	/* the dicionary must have at least one slot excluding tally */
	QSE_ASSERT (OBJSIZE(stx,dic) > 1);

	capa = OBJSIZE(stx,dic) - 1;
	dicptr = (qse_stx_dic_t*)PTRBYREF(stx,dic);

	tally = REFTOINT(stx,WORDAT(stx,dic,QSE_STX_DIC_TALLY));

	slot = find_slot (stx, dic, key);
	assoc = WORDAT(stx,dic,slot);

	if (assoc == stx->ref.nil) 
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
			if (expand (stx, dic) == stx->ref.nil) return stx->ref.nil;
		
			/* refresh tally */
			tally = REFTOINT(stx,WORDAT(stx,dic,QSE_STX_DIC_TALLY));
		}

		assoc = new_assoc (stx, key, value);
		if (assoc == stx->ref.nil) return stx->ref.nil;

		WORDAT(stx,dic,slot) = assoc;
		WORDAT(stx,dic,QSE_STX_DIC_TALLY) = INTTOREF(stx,tally + 1);
	}
	else 
	{
		/* found the key. change the value */
		WORDAT(stx,assoc,QSE_STX_ASSOC_VALUE) = value;
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
