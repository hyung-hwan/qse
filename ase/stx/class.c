/*
 * $Id: class.c,v 1.9 2005-06-08 16:00:51 bacon Exp $
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

