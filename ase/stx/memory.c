/*
 * $Id: memory.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <ase/stx/memory.h>
#include <ase/stx/misc.h>

ase_stx_memory_t* ase_stx_memory_open (
	ase_stx_memory_t* mem, ase_word_t capacity)
{
	ase_stx_object_t** slots;
	ase_word_t n;

	ase_assert (capacity > 0);
	if (mem == ASE_NULL) {
		mem = (ase_stx_memory_t*)ase_malloc(ase_sizeof(ase_stx_memory_t));
		if (mem == ASE_NULL) return ASE_NULL;
		mem->__dynamic = ase_true;
	}
	else mem->__dynamic = ase_false;

	slots = (ase_stx_object_t**)ase_malloc (
		capacity * ase_sizeof(ase_stx_object_t*));
	if (slots == ASE_NULL) {
		if (mem->__dynamic) ase_free (mem);
		mem = ASE_NULL;
	}

	mem->capacity = capacity;
	mem->slots = slots;

	/* weave the free slot list */
	mem->free = &slots[0];
	for (n = 0; n < capacity - 1; n++) {
		mem->slots[n] = (ase_stx_object_t*)&mem->slots[n + 1];
	}
	mem->slots[n] = ASE_NULL;

	return mem;
}

void ase_stx_memory_close (ase_stx_memory_t* mem)
{
	/* TODO: free all linked objects...	 */

	ase_free (mem->slots);
	mem->capacity = 0;
	mem->slots = ASE_NULL;
	mem->free = ASE_NULL;
	if (mem->__dynamic) ase_free (mem);
}

void ase_stx_memory_gc (ase_stx_memory_t* mem)
{
	/* TODO: implement this function */
}

ase_word_t ase_stx_memory_alloc (ase_stx_memory_t* mem, ase_word_t nbytes)
{
	ase_stx_object_t** slot;
	ase_stx_object_t* object;

	/* find the free object slot */
	if (mem->free == ASE_NULL) {
		ase_stx_memory_gc (mem);
		if (mem->free == ASE_NULL) return mem->capacity;;
	}

	object = (ase_stx_object_t*)ase_malloc (nbytes);
	if (object == ASE_NULL) {
		ase_stx_memory_gc (mem);
		object = (ase_stx_object_t*)ase_malloc (nbytes);
		/*if (object == ASE_NULL) return mem->capacity;*/
if (object == ASE_NULL) {
ase_assert (ASE_T("MEMORY ALLOCATION ERROR\n") == ASE_NULL);
exit (1);
}
	}

	slot = mem->free;
	mem->free = (ase_stx_object_t**)*slot;
	*slot = object;

	return (ase_word_t)(slot - mem->slots);
}

void ase_stx_memory_dealloc (ase_stx_memory_t* mem, ase_word_t object_index)
{
	/* 
	 * THIS IS PRIMITIVE LOW-LEVEL DEALLOC. THIS WILL NOT 
	 * DEALLOCATE MEMORY ALLOCATED FOR ITS INSTANCE VARIABLES.
	 */

	ase_free (mem->slots[object_index]);
	mem->slots[object_index] = (ase_stx_object_t*)mem->free;
	mem->free = &mem->slots[object_index];
}

