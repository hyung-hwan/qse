/*
 * $Id: memory.c,v 1.10 2005-05-19 16:41:10 bacon Exp $
 */

#include <xp/stx/memory.h>
#include <xp/stx/misc.h>

xp_stx_memory_t* xp_stx_memory_open (
	xp_stx_memory_t* mem, xp_stx_word_t capacity)
{
	xp_stx_object_t** slots;
	xp_stx_word_t n;

	xp_stx_assert (capacity > 0);
	if (mem == XP_NULL) {
		mem = (xp_stx_memory_t*)xp_stx_malloc(xp_sizeof(xp_stx_memory_t));
		if (mem == XP_NULL) return XP_NULL;
		mem->__malloced = xp_true;
	}
	else mem->__malloced = xp_false;

	slots = (xp_stx_object_t**)xp_stx_malloc (
		capacity * xp_sizeof(xp_stx_object_t*));
	if (slots == XP_NULL) {
		if (mem->__malloced) xp_stx_free (mem);
		mem = XP_NULL;
	}

	mem->capacity = capacity;
	mem->slots = slots;

	/* weave the free slot list */
	mem->free = &slots[0];
	for (n = 0; n < capacity - 1; n++) {
		mem->slots[n] = (xp_stx_object_t*)&mem->slots[n + 1];
	}
	mem->slots[n] = XP_NULL;

	return mem;
}

void xp_stx_memory_close (xp_stx_memory_t* mem)
{
	/* TODO: free all linked objects...	 */

	xp_stx_free (mem->slots);
	mem->capacity = 0;
	mem->slots = XP_NULL;
	mem->free = XP_NULL;
	if (mem->__malloced) xp_stx_free (mem);
}

void xp_stx_memory_gc (xp_stx_memory_t* mem)
{
	/* TODO: implement this function */
}

xp_stx_word_t xp_stx_memory_alloc (xp_stx_memory_t* mem, xp_stx_word_t nbytes)
{
	xp_stx_object_t** slot;
	xp_stx_object_t* object;

	/* find the free object slot */
	if (mem->free == XP_NULL) {
		xp_stx_memory_gc (mem);
		if (mem->free == XP_NULL) return mem->capacity;;
	}

	object = (xp_stx_object_t*)xp_stx_malloc (nbytes);
	if (object == XP_NULL) {
		xp_stx_memory_gc (mem);
		object = (xp_stx_object_t*)xp_stx_malloc (nbytes);
		if (object == XP_NULL) return mem->capacity;
	}

	slot = mem->free;
	mem->free = (xp_stx_object_t**)*slot;
	*slot = object;

	return (xp_stx_word_t)(slot - mem->slots);
}

void xp_stx_memory_dealloc (xp_stx_memory_t* mem, xp_stx_word_t object_index)
{
	/* 
	 * THIS IS PRIMITIVE LOW-LEVEL DEALLOC. THIS WILL NOT 
	 * DEALLOCATE MEMORY ALLOCATED FOR ITS INSTANCE VARIABLES.
	 */

	xp_stx_free (mem->slots[object_index]);
	mem->slots[object_index] = (xp_stx_object_t*)mem->free;
	mem->free = &mem->slots[object_index];
}

