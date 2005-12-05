/*
 * $Id: mem.c,v 1.2 2005-12-05 15:11:29 bacon Exp $
 */

#include <xp/sce/mem.h>
#include <xp/sce/misc.h>

xp_sce_mem_t* xp_sce_mem_open (
	xp_sce_mem_t* mem, xp_word_t capacity)
{
	xp_sce_obj_t** slots;
	xp_word_t n;

	xp_assert (capacity > 0);
	if (mem == XP_NULL) {
		mem = (xp_sce_mem_t*)xp_malloc(xp_sizeof(xp_sce_mem_t));
		if (mem == XP_NULL) return XP_NULL;
		mem->__dynamic = xp_true;
	}
	else mem->__dynamic = xp_false;

	slots = (xp_sce_obj_t**)xp_malloc (
		capacity * xp_sizeof(xp_sce_obj_t*));
	if (slots == XP_NULL) {
		if (mem->__dynamic) xp_free (mem);
		mem = XP_NULL;
	}

	mem->capacity = capacity;
	mem->slots = slots;

	/* weave the free slot list */
	mem->free = &slots[0];
	for (n = 0; n < capacity - 1; n++) {
		mem->slots[n] = (xp_sce_obj_t*)&mem->slots[n + 1];
	}
	mem->slots[n] = XP_NULL;

	return mem;
}

void xp_sce_mem_close (xp_sce_mem_t* mem)
{
	/* TODO: free all linked objs...	 */

	xp_free (mem->slots);
	mem->capacity = 0;
	mem->slots = XP_NULL;
	mem->free = XP_NULL;
	if (mem->__dynamic) xp_free (mem);
}

void xp_sce_mem_gc (xp_sce_mem_t* mem)
{
	/* TODO: implement this function */
}

xp_word_t xp_sce_mem_alloc (xp_sce_mem_t* mem, xp_word_t nbytes)
{
	xp_sce_obj_t** slot;
	xp_sce_obj_t* obj;

	/* find the free obj slot */
	if (mem->free == XP_NULL) {
		xp_sce_mem_gc (mem);
		if (mem->free == XP_NULL) return mem->capacity;;
	}

	obj = (xp_sce_obj_t*)xp_malloc (nbytes);
	if (obj == XP_NULL) {
		xp_sce_mem_gc (mem);
		obj = (xp_sce_obj_t*)xp_malloc (nbytes);
		/*if (obj == XP_NULL) return mem->capacity;*/
if (obj == XP_NULL) {
xp_assert (XP_TEXT("MEMORY ALLOCATION ERROR\n") == XP_NULL);
exit (1);
}
	}

	slot = mem->free;
	mem->free = (xp_sce_obj_t**)*slot;
	*slot = obj;

	return (xp_word_t)(slot - mem->slots);
}

void xp_sce_mem_dealloc (xp_sce_mem_t* mem, xp_word_t obj_index)
{
	/* 
	 * THIS IS PRIMITIVE LOW-LEVEL DEALLOC. THIS WILL NOT 
	 * DEALLOCATE MEMORY ALLOCATED FOR ITS INSTANCE VARIABLES.
	 */

	xp_free (mem->slots[obj_index]);
	mem->slots[obj_index] = (xp_sce_obj_t*)mem->free;
	mem->free = &mem->slots[obj_index];
}

