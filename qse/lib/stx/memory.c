/*
 * $Id: memory.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/memory.h>
#include <qse/stx/misc.h>

qse_stx_memory_t* qse_stx_memory_open (
	qse_stx_memory_t* mem, qse_word_t capacity)
{
	qse_stx_object_t** slots;
	qse_word_t n;

	qse_assert (capacity > 0);
	if (mem == QSE_NULL) {
		mem = (qse_stx_memory_t*)qse_malloc(qse_sizeof(qse_stx_memory_t));
		if (mem == QSE_NULL) return QSE_NULL;
		mem->__dynamic = qse_true;
	}
	else mem->__dynamic = qse_false;

	slots = (qse_stx_object_t**)qse_malloc (
		capacity * qse_sizeof(qse_stx_object_t*));
	if (slots == QSE_NULL) {
		if (mem->__dynamic) qse_free (mem);
		mem = QSE_NULL;
	}

	mem->capacity = capacity;
	mem->slots = slots;

	/* weave the free slot list */
	mem->free = &slots[0];
	for (n = 0; n < capacity - 1; n++) {
		mem->slots[n] = (qse_stx_object_t*)&mem->slots[n + 1];
	}
	mem->slots[n] = QSE_NULL;

	return mem;
}

void qse_stx_memory_close (qse_stx_memory_t* mem)
{
	/* TODO: free all linked objects...	 */

	qse_free (mem->slots);
	mem->capacity = 0;
	mem->slots = QSE_NULL;
	mem->free = QSE_NULL;
	if (mem->__dynamic) qse_free (mem);
}

void qse_stx_memory_gc (qse_stx_memory_t* mem)
{
	/* TODO: implement this function */
}

qse_word_t qse_stx_memory_alloc (qse_stx_memory_t* mem, qse_word_t nbytes)
{
	qse_stx_object_t** slot;
	qse_stx_object_t* object;

	/* find the free object slot */
	if (mem->free == QSE_NULL) {
		qse_stx_memory_gc (mem);
		if (mem->free == QSE_NULL) return mem->capacity;;
	}

	object = (qse_stx_object_t*)qse_malloc (nbytes);
	if (object == QSE_NULL) {
		qse_stx_memory_gc (mem);
		object = (qse_stx_object_t*)qse_malloc (nbytes);
		/*if (object == QSE_NULL) return mem->capacity;*/
if (object == QSE_NULL) {
qse_assert (QSE_T("MEMORY ALLOCATION ERROR\n") == QSE_NULL);
exit (1);
}
	}

	slot = mem->free;
	mem->free = (qse_stx_object_t**)*slot;
	*slot = object;

	return (qse_word_t)(slot - mem->slots);
}

void qse_stx_memory_dealloc (qse_stx_memory_t* mem, qse_word_t object_index)
{
	/* 
	 * THIS IS PRIMITIVE LOW-LEVEL DEALLOC. THIS WILL NOT 
	 * DEALLOCATE MEMORY ALLOCATED FOR ITS INSTANCE VARIABLES.
	 */

	qse_free (mem->slots[object_index]);
	mem->slots[object_index] = (qse_stx_object_t*)mem->free;
	mem->free = &mem->slots[object_index];
}

