/*
 * $Id: class.c,v 1.11 2005-06-30 12:07:02 bacon Exp $
 */

#include <xp/stx/class.h>
#include <xp/stx/symbol.h>
#include <xp/stx/object.h>
#include <xp/stx/hash.h>
#include <xp/stx/misc.h>

xp_word_t xp_stx_new_class (xp_stx_t* stx, const xp_char_t* name)
{
	xp_word_t meta, class;
	xp_word_t class_name;

	meta = xp_stx_alloc_word_object (stx, XP_STX_METACLASS_SIZE);
	XP_STX_CLASS(stx,meta) = stx->class_metaclass;
	/* the spec of the metaclass must be the spec of its
	 * instance. so the XP_STX_CLASS_SIZE is set */
	XP_STX_WORDAT(stx,meta,XP_STX_METACLASS_SPEC) = 
		XP_STX_TO_SMALLINT((XP_STX_CLASS_SIZE << 1) | 0x00);
	
	/* the spec of the class is set later in __create_builtin_classes */
	class = xp_stx_alloc_word_object (stx, XP_STX_CLASS_SIZE);
	XP_STX_CLASS(stx,class) = meta;
	class_name = xp_stx_new_symbol (stx, name);
	XP_STX_WORDAT(stx,class,XP_STX_CLASS_NAME) = class_name;

	xp_stx_hash_insert (stx, stx->smalltalk, 
		xp_stx_hash_char_object(stx, class_name),
		class_name, class);

	return class;
}

xp_word_t xp_stx_lookup_class (xp_stx_t* stx, const xp_char_t* name)
{
	xp_word_t link, meta, value;

	link = xp_stx_hash_lookup_symbol (stx, stx->smalltalk, name);
	if (link == stx->nil) return stx->nil;

	value = XP_STX_WORDAT(stx,link,XP_STX_PAIRLINK_VALUE);

	meta = XP_STX_CLASS(stx,value);
	if (XP_STX_CLASS(stx,meta) != stx->class_metaclass) return stx->nil;

	return value;
}

int xp_stx_get_instance_variable_index (
	xp_stx_t* stx, xp_word_t class_index, 
	const xp_char_t* name, xp_word_t* index)
{
	xp_word_t i, size, index_super = 0;
	xp_stx_class_t* class_obj;
	/*xp_stx_word_object_t* array;*/
	xp_stx_char_object_t* string;
	const xp_char_t* iname;

	class_obj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx, class_index);
	xp_assert (class_obj != XP_NULL);

	if (class_obj->superclass != stx->nil) {
		if (xp_stx_get_instance_variable_index (
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
		/*
		size = XP_STX_SIZE(stx, class_obj->variables);
		array = XP_STX_WORD_OBJECT(stx, class_obj->variables);

		for (i = 0; i < size; i++) {
			iname = &XP_STX_CHARAT(stx, array->data[i], 0);
			if (xp_strcmp(iname, name) == 0) {
				*index = i + index_super;
				return 0;
			}
		}
		*index = size + index_super;

		*/
		if (class_obj->variables != stx->nil) {
			string = XP_STX_CHAR_OBJECT(stx, class_obj->variables);
			if (xp_stx_strword (string->data, name, index) != XP_NULL) {
				*index += index_super;
				return 0;
			}
			*index = size + index_super;
		}
		else *index = index_super;
	}

	return -1;
}
