/*
 * $Id: class.c,v 1.1.1.1 2007/03/28 14:05:25 bacon Exp $
 */

#include <ase/stx/class.h>
#include <ase/stx/symbol.h>
#include <ase/stx/object.h>
#include <ase/stx/dict.h>
#include <ase/stx/misc.h>

ase_word_t ase_stx_new_class (ase_stx_t* stx, const ase_char_t* name)
{
	ase_word_t meta, class;
	ase_word_t class_name;

	meta = ase_stx_alloc_word_object (
		stx, ASE_NULL, ASE_STX_METACLASS_SIZE, ASE_NULL, 0);
	ASE_STX_CLASS(stx,meta) = stx->class_metaclass;
	/* the spec of the metaclass must be the spec of its
	 * instance. so the ASE_STX_CLASS_SIZE is set */
	ASE_STX_WORD_AT(stx,meta,ASE_STX_METACLASS_SPEC) = 
		ASE_STX_TO_SMALLINT((ASE_STX_CLASS_SIZE << ASE_STX_SPEC_INDEXABLE_BITS) | ASE_STX_SPEC_NOT_INDEXABLE);
	
	/* the spec of the class is set later in __create_builtin_classes */
	class = ase_stx_alloc_word_object (
		stx, ASE_NULL, ASE_STX_CLASS_SIZE, ASE_NULL, 0);
	ASE_STX_CLASS(stx,class) = meta;
	class_name = ase_stx_new_symbol (stx, name);
	ASE_STX_WORD_AT(stx,class,ASE_STX_CLASS_NAME) = class_name;

	ase_stx_dict_put (stx, stx->smalltalk, class_name, class);
	return class;
}

ase_word_t ase_stx_lookup_class (ase_stx_t* stx, const ase_char_t* name)
{
	ase_word_t assoc, meta, value;

	assoc = ase_stx_dict_lookup (stx, stx->smalltalk, name);
	if (assoc == stx->nil) {
		return stx->nil;
	}

	value = ASE_STX_WORD_AT(stx,assoc,ASE_STX_ASSOCIATION_VALUE);
	meta = ASE_STX_CLASS(stx,value);
	if (ASE_STX_CLASS(stx,meta) != stx->class_metaclass) return stx->nil;

	return value;
}

int ase_stx_get_instance_variable_index (
	ase_stx_t* stx, ase_word_t class_index, 
	const ase_char_t* name, ase_word_t* index)
{
	ase_word_t index_super = 0;
	ase_stx_class_t* class_obj;
	ase_stx_char_object_t* string;

	class_obj = (ase_stx_class_t*)ASE_STX_OBJECT(stx, class_index);
	ase_assert (class_obj != ASE_NULL);

	if (class_obj->superclass != stx->nil) {
		if (ase_stx_get_instance_variable_index (
			stx, class_obj->superclass, name, &index_super) == 0) {
			*index = index_super;
			return 0;
		}
	}

	if (class_obj->header.class == stx->class_metaclass) {
		/* metaclass */
		/* TODO: can a metaclas have instance variables? */	
		*index = index_super;
	}
	else {
		if (class_obj->variables == stx->nil) *index = 0;
		else {
			string = ASE_STX_CHAR_OBJECT(stx, class_obj->variables);
			if (ase_stx_strword(string->data, name, index) != ASE_NULL) {
				*index += index_super;
				return 0;
			}
		}

		*index += index_super;
	}

	return -1;
}

ase_word_t ase_stx_lookup_class_variable (
	ase_stx_t* stx, ase_word_t class_index, const ase_char_t* name)
{
	ase_stx_class_t* class_obj;

	class_obj = (ase_stx_class_t*)ASE_STX_OBJECT(stx, class_index);
	ase_assert (class_obj != ASE_NULL);

	if (class_obj->superclass != stx->nil) {
		ase_word_t tmp;
		tmp = ase_stx_lookup_class_variable (
			stx, class_obj->superclass, name);
		if (tmp != stx->nil) return tmp;
	}

	/* TODO: can a metaclas have class variables? */	
	if (class_obj->header.class != stx->class_metaclass &&
	    class_obj->class_variables != stx->nil) {
		if (ase_stx_dict_lookup(stx,
			class_obj->class_variables,name) != stx->nil) return class_index;
	}

	return stx->nil;
}

ase_word_t ase_stx_lookup_method (ase_stx_t* stx, 
	ase_word_t class_index, const ase_char_t* name, ase_bool_t from_super)
{
	ase_stx_class_t* class_obj;

	class_obj = (ase_stx_class_t*)ASE_STX_OBJECT(stx, class_index);
	ase_assert (class_obj != ASE_NULL);

#if 0
	if (class_obj->header.class != stx->class_metaclass &&
	    class_obj->methods != stx->nil) {
		ase_word_t assoc;
		assoc = ase_stx_dict_lookup(stx, class_obj->methods, name);
		if (assoc != stx->nil) {
			ase_assert (ASE_STX_CLASS(stx,assoc) == stx->class_association);
			return ASE_STX_WORD_AT(stx, assoc, ASE_STX_ASSOCIATION_VALUE);
		}
	}

	if (class_obj->superclass != stx->nil) {
		ase_word_t tmp;
		tmp = ase_stx_lookup_method (
			stx, class_obj->superclass, name);
		if (tmp != stx->nil) return tmp;
	}
#endif

	while (class_index != stx->nil) {
		class_obj = (ase_stx_class_t*)ASE_STX_OBJECT(stx, class_index);

		ase_assert (class_obj != ASE_NULL);
		ase_assert (
			class_obj->header.class == stx->class_metaclass ||
			ASE_STX_CLASS(stx,class_obj->header.class) == stx->class_metaclass);

		if (from_super) {	
			from_super = ase_false;
		}
		else if (class_obj->methods != stx->nil) {
			ase_word_t assoc;
			assoc = ase_stx_dict_lookup(stx, class_obj->methods, name);
			if (assoc != stx->nil) {
				ase_assert (ASE_STX_CLASS(stx,assoc) == stx->class_association);
				return ASE_STX_WORD_AT(stx, assoc, ASE_STX_ASSOCIATION_VALUE);
			}
		}

		class_index = class_obj->superclass;
	}

	return stx->nil;
}

