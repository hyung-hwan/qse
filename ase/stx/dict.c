/*
 * $Id: dict.c,v 1.3 2005-07-19 15:00:09 bacon Exp $
 */

#include <xp/stx/dict.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>

xp_word_t __new_association (
	xp_stx_t* stx, xp_word_t key, xp_word_t value)
{
	xp_word_t x;
#ifdef __GNUC__
	xp_word_t data[2] = { key, value };
#else
	xp_word_t data[2];
	data[0] = key;
	data[1] = value;
#endif

	x = xp_stx_alloc_word_object (
		stx, data, XP_STX_ASSOCIATION_SIZE, XP_NULL, 0);
	XP_STX_CLASS(stx,x) = stx->class_association;
	return x;
}

static xp_word_t __dict_find_slot (
	xp_stx_t* stx, xp_word_t dict, xp_word_t key)
{
	xp_word_t size, hash, index, assoc, symbol;
	xp_stx_word_object_t* dict_obj;

	xp_assert (XP_STX_IS_WORD_OBJECT(stx, dict));
	xp_assert (dict == stx->smalltalk || 
		XP_STX_CLASS(stx,dict) == stx->class_system_dictionary);
	xp_assert (XP_STX_CLASS(stx,key) == stx->class_symbol);

	size = XP_STX_SIZE(stx,dict);
	hash = xp_stx_hash_object(stx, key); 
xp_printf (XP_TEXT("find_slot %s %u\n"), XP_STX_DATA(stx,key), hash);

	/* consider tally, the only instance variable of a system dictionary */
	index = hash % (size - 1) + 1;

	dict_obj = XP_STX_WORD_OBJECT(stx,dict);

	while (1) {
		assoc = dict_obj->data[index];
		if (assoc == stx->nil) break;

		symbol = XP_STX_WORD_AT(stx,assoc,XP_STX_ASSOCIATION_KEY);
		xp_assert (XP_STX_CLASS(stx,symbol) == stx->class_symbol);

		/* 
		 * shallow comparison is enough for identity check 
		 * because only a symbol can be a key of a system dictionary
		 */
		if (xp_stx_shallow_compare_object(stx, key, symbol) == 0) break;

		/* consider tally here too */	
		index = index % (size - 1) + 1;
	}

	return index;
}

static void __dict_grow (xp_stx_t* stx, xp_word_t dict)
{
	xp_word_t new, size, index, assoc;
	
	/*
	 * if this assertion fails, adjust the initial size of the 
	 * system dictionary. i don't want this function to be called
	 * during the bootstrapping.
	 */
	xp_assert (stx->class_system_dictionary != stx->nil);
	xp_assert (XP_STX_CLASS(stx,dict) == stx->class_system_dictionary);

	size = XP_STX_SIZE(stx,dict);
	new = xp_stx_instantiate (stx, 
		XP_STX_CLASS(stx,dict), XP_NULL, XP_NULL, (size - 1) * 2); 
	XP_STX_WORD_AT(stx,new,0) = XP_STX_TO_SMALLINT(0);		

	for (index = 1; index < size; index++) {
		assoc = XP_STX_WORD_AT(stx,dict,index);
		if (assoc == stx->nil) continue;

		xp_stx_dict_put (stx, new, 
			XP_STX_WORD_AT(stx,assoc,XP_STX_ASSOCIATION_KEY),
			XP_STX_WORD_AT(stx,assoc,XP_STX_ASSOCIATION_VALUE));
	}
	
xp_printf (XP_TEXT("dictionary grown. swapped the index\n"));
	XP_SWAP ((xp_uint_t)XP_STX_OBJECT(stx,dict), 
	         (xp_uint_t)XP_STX_OBJECT(stx,new));
}

xp_word_t xp_stx_dict_lookup (
	xp_stx_t* stx, xp_word_t dict, const xp_char_t* key)
{
	xp_word_t size, hash, index, assoc, symbol;
	xp_stx_word_object_t* dict_obj;

	xp_assert (XP_STX_IS_WORD_OBJECT(stx, dict));
	xp_assert (dict == stx->smalltalk || 
		XP_STX_CLASS(stx,dict) == stx->class_system_dictionary);

	size = XP_STX_SIZE(stx,dict);
	/*hash = xp_stx_hash_object(stx, key);*/
	hash = xp_stx_hash(key, xp_strlen(key) * xp_sizeof(xp_char_t));
xp_printf (XP_TEXT("lookup hash %s %u\n"), key, hash);

	/* consider tally, the only instance variable of a system dictionary */
	index = hash % (size - 1) + 1;

	dict_obj = XP_STX_WORD_OBJECT(stx,dict);

	while (1) {
xp_printf (XP_TEXT("dict_lookup: %d\n"), index);
		assoc = dict_obj->data[index];
		if (assoc == stx->nil) break;

		symbol = XP_STX_WORD_AT(stx,assoc,XP_STX_ASSOCIATION_KEY);
		xp_assert (XP_STX_CLASS(stx,symbol) == stx->class_symbol);
		/* 
		 * note that xp_strcmp should be compatible with 
		 * character object comparison in xp_stx_shallow_compare_object.
		 * otherwise, you will be in trouble.
		 */
		if (xp_strcmp(key, XP_STX_DATA(stx,symbol)) == 0) break;

		/* consider tally here too */	
		index = index % (size - 1) + 1;
	}

xp_printf (XP_TEXT("dict_lookup: %s, %d, %d\n"), key, index, XP_STX_WORD_AT(stx,dict,index));
	return XP_STX_WORD_AT(stx,dict,index);
}

xp_word_t xp_stx_dict_get (xp_stx_t* stx, xp_word_t dict, xp_word_t key)
{
	return XP_STX_WORD_AT(stx,dict,__dict_find_slot(stx, dict, key));
}

xp_word_t xp_stx_dict_put (
	xp_stx_t* stx, xp_word_t dict, xp_word_t key, xp_word_t value)
{
	xp_word_t slot, capa, tally, assoc;

	/* the dictionary must have at least one slot excluding tally */
	xp_assert (XP_STX_SIZE(stx,dict) > 1);

	capa = XP_STX_SIZE(stx,dict) - 1;
	tally = XP_STX_FROM_SMALLINT(XP_STX_WORD_AT(stx,dict,0));
	if (capa <= tally + 1) {
		__dict_grow (stx, dict);
		/* refresh tally */
		tally = XP_STX_FROM_SMALLINT(XP_STX_WORD_AT(stx,dict,0));
	}

	slot = __dict_find_slot (stx, dict, key);

	assoc = XP_STX_WORD_AT(stx,dict,slot);
	if (assoc == stx->nil) {
		XP_STX_WORD_AT(stx,dict,slot) = 
			__new_association (stx, key, value);
		XP_STX_WORD_AT(stx,dict,0) = XP_STX_TO_SMALLINT(tally + 1);
	}
	else XP_STX_WORD_AT(stx,assoc,XP_STX_ASSOCIATION_VALUE) = value;

xp_printf (XP_TEXT("dict_put %s %d\n"), XP_STX_DATA(stx,key),slot);
	return XP_STX_WORD_AT(stx,dict,slot);
}

void xp_stx_dict_traverse (
	xp_stx_t* stx, xp_word_t dict, 
	void (*func) (xp_stx_t*,xp_word_t,void*), void* data)
{
	xp_word_t index, assoc;
	xp_word_t size = XP_STX_SIZE(stx,dict);
	
	for (index = 1; index < size; index++) {
		assoc = XP_STX_WORD_AT(stx,dict,index);
		if (assoc == stx->nil) continue;
		func (stx, assoc, data);
	}
}

