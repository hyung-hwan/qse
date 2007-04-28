/*
 * $Id: stx.c,v 1.1 2007/03/28 14:05:28 bacon Exp $
 */

#include <ase/stx/stx.h>
#include <ase/stx/memory.h>
#include <ase/stx/misc.h>

ase_stx_t* ase_stx_open (ase_stx_t* stx, ase_word_t capacity)
{
	ase_word_t i;

	if (stx == ASE_NULL) {
		stx = (ase_stx_t*)ase_malloc (ase_sizeof(stx));
		if (stx == ASE_NULL) return ASE_NULL;
		stx->__dynamic = ase_true;
	}
	else stx->__dynamic = ase_false;

	if (ase_stx_memory_open (&stx->memory, capacity) == ASE_NULL) {
		if (stx->__dynamic) ase_free (stx);
		return ASE_NULL;
	}

	stx->symtab.size = 0;
	stx->symtab.capacity = 128; /* TODO: symbol table size */
	stx->symtab.datum = (ase_word_t*)ase_malloc (
		ase_sizeof(ase_word_t) * stx->symtab.capacity);
	if (stx->symtab.datum == ASE_NULL) {
		ase_stx_memory_close (&stx->memory);
		if (stx->__dynamic) ase_free (stx);
		return ASE_NULL;
	}

	stx->nil = ASE_STX_NIL;
	stx->true = ASE_STX_TRUE;
	stx->false = ASE_STX_FALSE;

	stx->smalltalk = ASE_STX_NIL;

	stx->class_symbol = ASE_STX_NIL;
	stx->class_metaclass = ASE_STX_NIL;
	stx->class_association = ASE_STX_NIL;

	stx->class_object = ASE_STX_NIL;
	stx->class_class = ASE_STX_NIL;
	stx->class_array = ASE_STX_NIL;
	stx->class_bytearray = ASE_STX_NIL;
	stx->class_string = ASE_STX_NIL;
	stx->class_character = ASE_STX_NIL;
	stx->class_context = ASE_STX_NIL;
	stx->class_system_dictionary = ASE_STX_NIL;
	stx->class_method = ASE_STX_NIL;
	stx->class_smallinteger = ASE_STX_NIL;

	for (i = 0; i < stx->symtab.capacity; i++) {
		stx->symtab.datum[i] = stx->nil;
	}
	
	stx->__wantabort = ase_false;
	return stx;
}

void ase_stx_close (ase_stx_t* stx)
{
	ase_free (stx->symtab.datum);
	ase_stx_memory_close (&stx->memory);
	if (stx->__dynamic) ase_free (stx);
}

