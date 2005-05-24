/*
 * $Id: class.c,v 1.6 2005-05-24 03:29:43 bacon Exp $
 */

#include <xp/stx/class.h>
#include <xp/stx/symbol.h>
#include <xp/stx/object.h>
#include <xp/stx/hash.h>
#include <xp/stx/misc.h>

xp_stx_word_t xp_stx_new_class (xp_stx_t* stx, const xp_stx_char_t* name)
{
	xp_stx_word_t meta, class;
	xp_stx_word_t class_name;

	meta = xp_stx_alloc_word_object (stx, XP_STX_METACLASS_SIZE);
	XP_STX_CLASS(stx,meta) = stx->class_metaclass;
	XP_STX_AT(stx,meta,XP_STX_METACLASS_SPEC) = 
		XP_STX_TO_SMALLINT((XP_STX_METACLASS_SIZE << 1) | 0x00);
	
	class = xp_stx_alloc_word_object (stx, XP_STX_CLASS_SIZE);
	XP_STX_CLASS(stx,class) = meta;

	class_name = xp_stx_new_symbol (stx, name);
	XP_STX_AT(stx,class,XP_STX_CLASS_NAME) = class_name;

	xp_stx_hash_insert (stx, stx->smalltalk, 
		xp_stx_hash_char_object(stx, class_name),
		class_name, class);

	return class;
}

xp_stx_word_t xp_stx_new_metaclass (xp_stx_t* stx, xp_stx_word_t class)
{
	meta = xp_stx_alloc_word_object (stx, XP_STX_METACLASS_SIZE);
	meta_obj = meta;

	if (class == stx->class_class) {
		/*
		 * "Object class superclass" returns "Class"
		 * whereas "Class subclasses" just returns an empty array
		 * because "Object class" is not registered as a subclass of 
		 * "Class".
		 */
		meta_obj->superclass = stx->class_class;
	}
	else {
		meta_obj->superclass = (class, SUPERCLASS)->class;
	}

	meta_obj->name = class_obj->name;
	meta_obj->instance_class = class;

	return meta;
}

xp_stx_word_t xp_stx_lookup_class (xp_stx_t* stx, const xp_stx_char_t* name)
{
	xp_stx_word_t link, meta, value;

	link = xp_stx_hash_lookup_symbol (stx, stx->smalltalk, name);
	if (link == stx->nil) return stx->nil;

	value = XP_STX_AT(stx,link,XP_STX_PAIRLINK_VALUE);

	meta = XP_STX_CLASS(stx,value);
	if (XP_STX_CLASS(stx,meta) != stx->class_metaclass) return stx->nil;

	return value;
}

