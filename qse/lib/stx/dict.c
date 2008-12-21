/*
 * $Id: dict.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/dict.h>
#include <qse/stx/object.h>
#include <qse/stx/misc.h>

/* NOTE:
 * The code here implements SystemDictionary whose key is always a symbol.
 * Dictionary, on the contrary, can accept any object as a key.
 */

qse_word_t __new_association (
	qse_stx_t* stx, qse_word_t key, qse_word_t value)
{
	qse_word_t x;
#ifdef __GNUC__
	qse_word_t data[2] = { key, value };
#else
	qse_word_t data[2];
	data[0] = key;
	data[1] = value;
#endif
	x = qse_stx_alloc_word_object (
		stx, data, QSE_STX_ASSOCIATION_SIZE, QSE_NULL, 0);
	QSE_STX_CLASS(stx,x) = stx->class_association;
	return x;
}

static qse_word_t __dict_find_slot (
	qse_stx_t* stx, qse_word_t dict, qse_word_t key)
{
	qse_word_t size, hash, index, assoc, symbol;
	qse_stx_word_object_t* dict_obj;

	qse_assert (!QSE_STX_IS_SMALLINT(dict) &&
	           QSE_STX_IS_WORD_OBJECT(stx, dict));
	qse_assert (dict == stx->smalltalk || 
	           qse_stx_classof(stx,dict) == stx->class_system_dictionary);
	qse_assert (qse_stx_classof(stx,key) == stx->class_symbol);

	size = QSE_STX_SIZE(stx,dict);
	hash = qse_stx_hash_object(stx, key); 

	/* consider tally, the only instance variable of a system dictionary */
	index = hash % (size - 1) + 1;

	dict_obj = QSE_STX_WORD_OBJECT(stx,dict);

	while (1) {
		assoc = dict_obj->data[index];
		if (assoc == stx->nil) break;

		symbol = QSE_STX_WORD_AT(stx,assoc,QSE_STX_ASSOCIATION_KEY);
		qse_assert (qse_stx_classof(stx,symbol) == stx->class_symbol);

		/* NOTE:
		 * shallow comparison is enough for identity check 
		 * because a symbol can just be a key of a system dictionary
		 */
		if (qse_strxncmp(
			QSE_STX_DATA(stx,key), QSE_STX_SIZE(stx,key),
			QSE_STX_DATA(stx,symbol), QSE_STX_SIZE(stx,symbol)) == 0) break;

		/* consider tally here too */	
		index = index % (size - 1) + 1;
	}

	return index;
}

static void __grow_dict (qse_stx_t* stx, qse_word_t dict)
{
	qse_word_t new, size, index, assoc;
	
	/* WARNING:
	 * if this assertion fails, adjust the initial size of the 
	 * system dictionary. i don't want this function to be called
	 * during the bootstrapping.
	 */
	qse_assert (stx->class_system_dictionary != stx->nil);
	qse_assert (qse_stx_classof(stx,dict) == stx->class_system_dictionary);

	size = QSE_STX_SIZE(stx,dict);
	new = qse_stx_instantiate (stx, 
		QSE_STX_CLASS(stx,dict), QSE_NULL, QSE_NULL, (size - 1) * 2); 
	QSE_STX_WORD_AT(stx,new,0) = QSE_STX_TO_SMALLINT(0);		

	for (index = 1; index < size; index++) {
		assoc = QSE_STX_WORD_AT(stx,dict,index);
		if (assoc == stx->nil) continue;

		qse_stx_dict_put (stx, new, 
			QSE_STX_WORD_AT(stx,assoc,QSE_STX_ASSOCIATION_KEY),
			QSE_STX_WORD_AT(stx,assoc,QSE_STX_ASSOCIATION_VALUE));
	}
	
	/* TODO: explore if dict can be immediately destroyed. */

	qse_assert (qse_sizeof(qse_stx_object_t*) == qse_sizeof(qse_uint_t));
	QSE_SWAP (QSE_STX_OBJECT(stx,dict),
	         QSE_STX_OBJECT(stx,new),
	         qse_stx_object_t*, qse_uint_t);
}

qse_word_t qse_stx_dict_lookup (
	qse_stx_t* stx, qse_word_t dict, const qse_char_t* key)
{
	qse_word_t size, hash, index, assoc, symbol;
	qse_stx_word_object_t* dict_obj;

	qse_assert (!QSE_STX_IS_SMALLINT(dict) &&
	           QSE_STX_IS_WORD_OBJECT(stx, dict));
	qse_assert (dict == stx->smalltalk || 
	           qse_stx_classof(stx,dict) == stx->class_system_dictionary);

	size = QSE_STX_SIZE(stx,dict);
	hash = qse_stx_hash(key, qse_strlen(key) * qse_sizeof(qse_char_t));

	/* consider tally, the only instance variable of a system dictionary */
	index = hash % (size - 1) + 1;

	dict_obj = QSE_STX_WORD_OBJECT(stx,dict);

	while (1) {
		assoc = dict_obj->data[index];
		if (assoc == stx->nil) break;

		symbol = QSE_STX_WORD_AT(stx,assoc,QSE_STX_ASSOCIATION_KEY);
		qse_assert (qse_stx_classof(stx,symbol) == stx->class_symbol);

		if (qse_strxcmp (QSE_STX_DATA(stx,symbol),
			QSE_STX_SIZE(stx,symbol), key) == 0) break;

		/* consider tally here too */	
		index = index % (size - 1) + 1;
	}

	return QSE_STX_WORD_AT(stx,dict,index);
}

qse_word_t qse_stx_dict_get (qse_stx_t* stx, qse_word_t dict, qse_word_t key)
{
	return QSE_STX_WORD_AT(stx,dict,__dict_find_slot(stx, dict, key));
}

qse_word_t qse_stx_dict_put (
	qse_stx_t* stx, qse_word_t dict, qse_word_t key, qse_word_t value)
{
	qse_word_t slot, capa, tally, assoc;

	/* the dictionary must have at least one slot excluding tally */
	qse_assert (QSE_STX_SIZE(stx,dict) > 1);

	capa = QSE_STX_SIZE(stx,dict) - 1;
	tally = QSE_STX_FROM_SMALLINT(QSE_STX_WORD_AT(stx,dict,0));
	if (capa <= tally + 1) {
		__grow_dict (stx, dict);
		/* refresh tally */
		tally = QSE_STX_FROM_SMALLINT(QSE_STX_WORD_AT(stx,dict,0));
	}

	slot = __dict_find_slot (stx, dict, key);

	assoc = QSE_STX_WORD_AT(stx,dict,slot);
	if (assoc == stx->nil) {
		QSE_STX_WORD_AT(stx,dict,slot) = 
			__new_association (stx, key, value);
		QSE_STX_WORD_AT(stx,dict,0) = QSE_STX_TO_SMALLINT(tally + 1);
	}
	else QSE_STX_WORD_AT(stx,assoc,QSE_STX_ASSOCIATION_VALUE) = value;

	return QSE_STX_WORD_AT(stx,dict,slot);
}

void qse_stx_dict_traverse (
	qse_stx_t* stx, qse_word_t dict, 
	void (*func) (qse_stx_t*,qse_word_t,void*), void* data)
{
	qse_word_t index, assoc;
	qse_word_t size = QSE_STX_SIZE(stx,dict);
	
	for (index = 1; index < size; index++) {
		assoc = QSE_STX_WORD_AT(stx,dict,index);
		if (assoc == stx->nil) continue;
		func (stx, assoc, data);
	}
}

