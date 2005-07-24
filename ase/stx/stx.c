/*
 * $Id: stx.c,v 1.35 2005-07-24 16:50:53 bacon Exp $
 */

#include <xp/stx/stx.h>
#include <xp/stx/memory.h>
#include <xp/stx/misc.h>

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_word_t capacity)
{
	xp_word_t i;

	if (stx == XP_NULL) {
		stx = (xp_stx_t*)xp_malloc (xp_sizeof(stx));
		if (stx == XP_NULL) return XP_NULL;
		stx->__malloced = xp_true;
	}
	else stx->__malloced = xp_false;

	if (xp_stx_memory_open (&stx->memory, capacity) == XP_NULL) {
		if (stx->__malloced) xp_free (stx);
		return XP_NULL;
	}

	stx->symtab.size = 0;
	stx->symtab.capacity = 256;
	stx->symtab.data = (xp_word_t*)xp_malloc (
		xp_sizeof(xp_word_t) * stx->symtab.capacity);
	if (stx->symtab.data == XP_NULL) {
		xp_stx_memory_close (&stx->memory);
		if (stx->__malloced) xp_free (stx);
		return XP_NULL;
	}

	stx->nil = XP_STX_NIL;
	stx->true = XP_STX_TRUE;
	stx->false = XP_STX_FALSE;

	stx->symbol_table = XP_STX_NIL;
	stx->smalltalk = XP_STX_NIL;

	stx->class_symlink = XP_STX_NIL;
	stx->class_symbol = XP_STX_NIL;
	stx->class_metaclass = XP_STX_NIL;
	stx->class_association = XP_STX_NIL;

	stx->class_object = XP_STX_NIL;
	stx->class_class = XP_STX_NIL;
	stx->class_bytearray = XP_STX_NIL;
	stx->class_array = XP_STX_NIL;
	stx->class_string = XP_STX_NIL;
	stx->class_system_dictionary = XP_STX_NIL;
	stx->class_method = XP_STX_NIL;
	stx->class_smallinteger = XP_STX_NIL;

	for (i = 0; i < stx->symtab.capacity; i++) {
		stx->symtab.data[i] = stx->nil;
	}
	
	stx->__wantabort = xp_false;
	return stx;
}

void xp_stx_close (xp_stx_t* stx)
{
	xp_free (stx->symtab.data);
	xp_stx_memory_close (&stx->memory);
	if (stx->__malloced) xp_free (stx);
}

