/*
 * $Id: class.c,v 1.1 2005-05-22 15:03:20 bacon Exp $
 */

#include <xp/stx/class.h>
#include <xp/stx/symbol.h>
#include <xp/stx/object.h>
#include <xp/stx/hash.h>
#include <xp/stx/misc.h>

xp_stx_word_t xp_stx_new_class (xp_stx_t* stx, const xp_stx_char_t* name)
{
	xp_stx_word_t meta, class;
	xp_stx_word_t /*meta_name,*/ class_name;

	meta = xp_stx_alloc_word_object (stx, XP_STX_CLASS_SIZE);
	XP_STX_CLASS(stx,meta) = stx->class_metaclass;
	XP_STX_AT(stx,meta,XP_STX_CLASS_SPEC) = 
		XP_STX_TO_SMALLINT(XP_STX_CLASS_SIZE);
	
	class = xp_stx_alloc_word_object (stx, XP_STX_CLASS_SIZE);
	XP_STX_CLASS(stx,class) = meta;

	/*
	meta_name = xp_stx_new_symbol_pp (
		stx, name, XP_STX_TEXT(""), XP_STX_TEXT(" class"));
	XP_STX_AT(stx,meta,XP_STX_CLASS_NAME) = meta_name;
	*/
	class_name = xp_stx_new_symbol (stx, name);
	XP_STX_AT(stx,class,XP_STX_CLASS_NAME) = class_name;

	/*
	xp_stx_hash_insert (stx, stx->smalltalk, 
		xp_stx_hash_char_object(stx, meta_name),
		meta_name, meta);
	*/
	xp_stx_hash_insert (stx, stx->smalltalk, 
		xp_stx_hash_char_object(stx, class_name),
		class_name, class);

	return class;
}

xp_stx_word_t xp_stx_lookup_class (xp_stx_t* stx, const xp_stx_char_t* name)
{
	xp_stx_word_t link;

	link = xp_stx_hash_lookup_symbol (stx, stx->smalltalk, name);
	if (link == stx->nil) return stx->nil;

	return XP_STX_AT(stx,link,XP_STX_PAIRLINK_VALUE);
}

