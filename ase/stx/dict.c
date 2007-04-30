/*
 * $Id: dict.c,v 1.1.1.1 2007/03/28 14:05:28 bacon Exp $
 */

#include <ase/stx/dict.h>
#include <ase/stx/object.h>
#include <ase/stx/misc.h>

/* NOTE:
 * The code here implements SystemDictionary whose key is always a symbol.
 * Dictionary, on the contrary, can accept any object as a key.
 */

ase_word_t __new_association (
	ase_stx_t* stx, ase_word_t key, ase_word_t value)
{
	ase_word_t x;
#ifdef __GNUC__
	ase_word_t data[2] = { key, value };
#else
	ase_word_t data[2];
	data[0] = key;
	data[1] = value;
#endif
	x = ase_stx_alloc_word_object (
		stx, data, ASE_STX_ASSOCIATION_SIZE, ASE_NULL, 0);
	ASE_STX_CLASS(stx,x) = stx->class_association;
	return x;
}

static ase_word_t __dict_find_slot (
	ase_stx_t* stx, ase_word_t dict, ase_word_t key)
{
	ase_word_t size, hash, index, assoc, symbol;
	ase_stx_word_object_t* dict_obj;

	ase_assert (!ASE_STX_IS_SMALLINT(dict) &&
	           ASE_STX_IS_WORD_OBJECT(stx, dict));
	ase_assert (dict == stx->smalltalk || 
	           ase_stx_classof(stx,dict) == stx->class_system_dictionary);
	ase_assert (ase_stx_classof(stx,key) == stx->class_symbol);

	size = ASE_STX_SIZE(stx,dict);
	hash = ase_stx_hash_object(stx, key); 

	/* consider tally, the only instance variable of a system dictionary */
	index = hash % (size - 1) + 1;

	dict_obj = ASE_STX_WORD_OBJECT(stx,dict);

	while (1) {
		assoc = dict_obj->data[index];
		if (assoc == stx->nil) break;

		symbol = ASE_STX_WORD_AT(stx,assoc,ASE_STX_ASSOCIATION_KEY);
		ase_assert (ase_stx_classof(stx,symbol) == stx->class_symbol);

		/* NOTE:
		 * shallow comparison is enough for identity check 
		 * because a symbol can just be a key of a system dictionary
		 */
		if (ase_strxncmp(
			ASE_STX_DATA(stx,key), ASE_STX_SIZE(stx,key),
			ASE_STX_DATA(stx,symbol), ASE_STX_SIZE(stx,symbol)) == 0) break;

		/* consider tally here too */	
		index = index % (size - 1) + 1;
	}

	return index;
}

static void __grow_dict (ase_stx_t* stx, ase_word_t dict)
{
	ase_word_t new, size, index, assoc;
	
	/* WARNING:
	 * if this assertion fails, adjust the initial size of the 
	 * system dictionary. i don't want this function to be called
	 * during the bootstrapping.
	 */
	ase_assert (stx->class_system_dictionary != stx->nil);
	ase_assert (ase_stx_classof(stx,dict) == stx->class_system_dictionary);

	size = ASE_STX_SIZE(stx,dict);
	new = ase_stx_instantiate (stx, 
		ASE_STX_CLASS(stx,dict), ASE_NULL, ASE_NULL, (size - 1) * 2); 
	ASE_STX_WORD_AT(stx,new,0) = ASE_STX_TO_SMALLINT(0);		

	for (index = 1; index < size; index++) {
		assoc = ASE_STX_WORD_AT(stx,dict,index);
		if (assoc == stx->nil) continue;

		ase_stx_dict_put (stx, new, 
			ASE_STX_WORD_AT(stx,assoc,ASE_STX_ASSOCIATION_KEY),
			ASE_STX_WORD_AT(stx,assoc,ASE_STX_ASSOCIATION_VALUE));
	}
	
	/* TODO: explore if dict can be immediately destroyed. */

	ase_assert (ase_sizeof(ase_stx_object_t*) == ase_sizeof(ase_uint_t));
	ASE_SWAP (ASE_STX_OBJECT(stx,dict),
	         ASE_STX_OBJECT(stx,new),
	         ase_stx_object_t*, ase_uint_t);
}

ase_word_t ase_stx_dict_lookup (
	ase_stx_t* stx, ase_word_t dict, const ase_char_t* key)
{
	ase_word_t size, hash, index, assoc, symbol;
	ase_stx_word_object_t* dict_obj;

	ase_assert (!ASE_STX_IS_SMALLINT(dict) &&
	           ASE_STX_IS_WORD_OBJECT(stx, dict));
	ase_assert (dict == stx->smalltalk || 
	           ase_stx_classof(stx,dict) == stx->class_system_dictionary);

	size = ASE_STX_SIZE(stx,dict);
	hash = ase_stx_hash(key, ase_strlen(key) * ase_sizeof(ase_char_t));

	/* consider tally, the only instance variable of a system dictionary */
	index = hash % (size - 1) + 1;

	dict_obj = ASE_STX_WORD_OBJECT(stx,dict);

	while (1) {
		assoc = dict_obj->data[index];
		if (assoc == stx->nil) break;

		symbol = ASE_STX_WORD_AT(stx,assoc,ASE_STX_ASSOCIATION_KEY);
		ase_assert (ase_stx_classof(stx,symbol) == stx->class_symbol);

		if (ase_strxcmp (ASE_STX_DATA(stx,symbol),
			ASE_STX_SIZE(stx,symbol), key) == 0) break;

		/* consider tally here too */	
		index = index % (size - 1) + 1;
	}

	return ASE_STX_WORD_AT(stx,dict,index);
}

ase_word_t ase_stx_dict_get (ase_stx_t* stx, ase_word_t dict, ase_word_t key)
{
	return ASE_STX_WORD_AT(stx,dict,__dict_find_slot(stx, dict, key));
}

ase_word_t ase_stx_dict_put (
	ase_stx_t* stx, ase_word_t dict, ase_word_t key, ase_word_t value)
{
	ase_word_t slot, capa, tally, assoc;

	/* the dictionary must have at least one slot excluding tally */
	ase_assert (ASE_STX_SIZE(stx,dict) > 1);

	capa = ASE_STX_SIZE(stx,dict) - 1;
	tally = ASE_STX_FROM_SMALLINT(ASE_STX_WORD_AT(stx,dict,0));
	if (capa <= tally + 1) {
		__grow_dict (stx, dict);
		/* refresh tally */
		tally = ASE_STX_FROM_SMALLINT(ASE_STX_WORD_AT(stx,dict,0));
	}

	slot = __dict_find_slot (stx, dict, key);

	assoc = ASE_STX_WORD_AT(stx,dict,slot);
	if (assoc == stx->nil) {
		ASE_STX_WORD_AT(stx,dict,slot) = 
			__new_association (stx, key, value);
		ASE_STX_WORD_AT(stx,dict,0) = ASE_STX_TO_SMALLINT(tally + 1);
	}
	else ASE_STX_WORD_AT(stx,assoc,ASE_STX_ASSOCIATION_VALUE) = value;

	return ASE_STX_WORD_AT(stx,dict,slot);
}

void ase_stx_dict_traverse (
	ase_stx_t* stx, ase_word_t dict, 
	void (*func) (ase_stx_t*,ase_word_t,void*), void* data)
{
	ase_word_t index, assoc;
	ase_word_t size = ASE_STX_SIZE(stx,dict);
	
	for (index = 1; index < size; index++) {
		assoc = ASE_STX_WORD_AT(stx,dict,index);
		if (assoc == stx->nil) continue;
		func (stx, assoc, data);
	}
}

