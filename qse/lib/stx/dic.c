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

static qse_word_t find_slot (
	qse_stx_t* stx, qse_word_t dic, qse_word_t key)
{
	qse_word_t size, hash, index, assoc, symbol;
	qse_stx_dic_t* ptr;

	/* ensure that dic is a system dictionary */
	QSE_ASSERT (REFISIDX(stx,dic));
	QSE_ASSERT (OBJTYPE(stx,dic) == WORDOBJ);
	QSE_ASSERT (dic == stx->ref.sysdic ||
	            OBJCLASS(stx,key) == stx->ref.class_system_dictionary);

	/* ensure that the key is a symbol */
	QSE_ASSERT (REFISIDX(stx,key));
	QSE_ASSERT (OBJCLASS(stx,key) == stx->ref.class_symbol);

	size = OBJSIZE (stx, dic);
	hash = qse_stx_hashobj (stx, key); 

	/* consider tally, the only instance variable of a system dicionary */
	index = hash % (size - 1);

	ptr = (qse_stx_dic_t*) REFTOIDX (stx, dic);

	while (1) 
	{
		assoc = ptr->slot[index];
		if (assoc == stx->ref.nil) break;

		symbol = WORDAT (stx, assoc, QSE_STX_ASSOC_KEY);

		QSE_ASSERT (REFISIDX(stx,symbol));
		QSE_ASSERT (OBJCLASS(stx,symbol) == stx->ref.class_symbol);

		/* NOTE:
		 * shallow comparison is enough for identity check 
		 * because a symbol can just be a key of a system dicionary
		 */
		if (qse_strxncmp(
			QSE_STX_DATA(stx,key), OBJSIZE(stx,key),
			QSE_STX_DATA(stx,symbol), OBJSIZE(stx,symbol)) == 0) break;

		/* consider tally here too */	
		index = index % (size - 1);
	}

	return index;
}

static void grow_dic (qse_stx_t* stx, qse_word_t dic)
{
	qse_word_t new, size, index, assoc;
	
	/* WARNING:
	 * if this assertion fails, adjust the initial size of the 
	 * system dicionary. i don't want this function to be called
	 * during the bootstrapping.
	 */
	QSE_ASSERT (stx->ref.class_system_dictionary != stx->ref.nil);
	QSE_ASSERT (qse_stx_classof(stx,dic) == stx->ref.class_system_dicionary);

	size = OBJSIZE(stx,dic);
	new = qse_stx_instantiate (stx, 
		OBJCLASS(stx,dic), QSE_NULL, QSE_NULL, (size - 1) * 2); 
	WORDAT(stx,new,0) = QSE_STX_TO_SMALLINT(0);		

	for (index = 1; index < size; index++) 
	{
		assoc = WORDAT(stx,dic,index);
		if (assoc == stx->nil) continue;

		qse_stx_putdic (stx, new, 
			WORDAT(stx,assoc,QSE_STX_ASSOC_KEY),
			WORDAT(stx,assoc,QSE_STX_ASSOC_VALUE));
	}
	
	/* TODO: explore if dic can be immediately destroyed. */

	QSE_ASSERT (qse_sizeof(qse_stx_object_t*) == qse_sizeof(qse_uint_t));

	QSE_SWAP (
		QSE_STX_OBJPTR(stx,dic),
		QSE_STX_OBJPTR(stx,new),
		qse_stx_object_t*,
		qse_uint_t
	);
}

qse_word_t qse_stx_lookupdic (
	qse_stx_t* stx, qse_word_t dic, const qse_char_t* key)
{
	qse_word_t size, hash, index, assoc, symbol;
	qse_stx_word_object_t* ptr;

	QSE_ASSERT (!QSE_STX_ISSMALLINT(dic) &&
	           QSE_STX_ISWORDOBJECT(stx, dic));
	QSE_ASSERT (dic == stx->smalltalk || 
	           qse_stx_classof(stx,dic) == stx->class_system_dicionary);

	size = OBJSIZE(stx,dic);
	hash = qse_stx_hash(key, qse_strlen(key) * qse_sizeof(qse_char_t));

	/* consider tally, the only instance variable of a system dicionary */
	index = hash % (size - 1);

	ptr = QSE_STX_WORD_OBJECT(stx,dic);

	while (1) 
	{
		assoc = ptr->slot[index];
		if (assoc == stx->ref.nil) break;

		symbol = WORDAT(stx,assoc,QSE_STX_ASSOC_KEY);
		QSE_ASSERT (qse_stx_classof(stx,symbol) == stx->class_symbol);

		if (qse_strxcmp (
			QSE_STX_DATA(stx,symbol), OBJSIZE(stx,symbol), key) == 0) break;

		/* consider tally here too */	
		index = index % (size - 1);
	}

	return WORDAT(stx,dic,index);
}

#if 0
qse_word_t qse_stx_getdic (qse_stx_t* stx, qse_word_t dic, qse_word_t key)
{
	return WORDAT (stx, dic, find_slot(stx, dic, key));
}

qse_word_t qse_stx_putdic (
	qse_stx_t* stx, qse_word_t dic, qse_word_t key, qse_word_t val)
{
	qse_word_t slot, capa, tally, assoc;

	/* the dicionary must have at least one slot excluding tally */
	QSE_ASSERT (OBJSIZE(stx,dic) > 1);

	capa = OBJSIZE(stx,dic) - 1;
	tally = REFTOINT(WORDAT(stx,dic,0));
	if (capa <= tally + 1) 
	{
		if (grow_dic (stx, dic) == stx->ref.nil) return stx->ref.nil;
		
		/* refresh tally */
		tally = REFTOINT (stx, WORDAT (stx, dic, QSE_STX_DIC_TALLY));
	}

	slot = find_slot (stx, dic, key);

	assoc = WORDAT (stx, dic, slot);
	if (assoc == stx->ref.nil) 
	{
		assoc = new_assoc (stx, key, value);
		if (assoc == stx->ref.nil) return stx->ref.nil;

		WORDAT (stx, dic, slot) = assoc;
		WORDAT (stx, dic, QSE_STX_DIC_TALLY) = INTTOREF (stx, tally + 1);
	}
	else WORDAT (stx, assoc, QSE_STX_ASSOC_VALUE) = value;

	return WORDAT(stx,dic,slot);
}

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
