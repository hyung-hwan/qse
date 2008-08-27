/*
 * $Id$
 *
 * {License}
 */

#include <ase/cmn/dll.h>
#include "mem.h"

ase_dll_t* ase_dll_open (
	ase_mmgr_t* mmgr, ase_size_t ext, void (*init) (ase_dll_t*))
{
	ase_dll_t* dll;

	if (mmgr == ASE_NULL) 
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

	dll = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(ase_dll_t) + ext);
	if (dll == ASE_NULL) return ASE_NULL;

	ASE_MEMSET (dll, 0, ASE_SIZEOF(ase_dll_t) + ext);
	dll->mmgr = mmgr;

	if (init) init (dll);

	return dll;
}

void ase_dll_close (ase_dll_t* dll)
{
	ase_dll_clear (dll);
	ASE_MMGR_FREE (dll->mmgr, dll);
}

void ase_dll_clear (ase_dll_t* dll)
{
	while (dll->head != ASE_NULL) ase_dll_delete (dll, dll->head);
	ASE_ASSERT (dll->tail == ASE_NULL);
}

void* ase_dll_getextension (ase_dll_t* dll)
{
	return dll + 1;
}

ase_mmgr_t* ase_dll_getmmgr (ase_dll_t* dll)
{
        return dll->mmgr;
}

void ase_dll_setmmgr (ase_dll_t* dll, ase_mmgr_t* mmgr)
{
	dll->mmgr = mmgr;
}

ase_size_t ase_dll_getsize (ase_dll_t* dll)
{
	return dll->size;
}

ase_dll_node_t* ase_dll_gethead (ase_dll_t* dll)
{
	return dll->head;
}

ase_dll_node_t* ase_dll_gettail (ase_dll_t* dll)
{
	return dll->tail;
}

ase_dll_copier_t ase_dll_getcopier (ase_dll_t* dll)
{
	return dll->copier;
}

void ase_dll_setcopier (ase_dll_t* dll, ase_dll_copier_t copier)
{
	dll->copier = copier;
}

ase_dll_freeer_t ase_dll_getfreeer (ase_dll_t* dll)
{
	return dll->freeer;
}

void ase_dll_setfreeer (ase_dll_t* dll, ase_dll_freeer_t freeer)
{
	dll->freeer = freeer;
}

static ase_dll_node_t* alloc_node (ase_dll_t* dll, void* dptr, ase_size_t dlen)
{
	ase_dll_node_t* n;

	if (dll->copier == ASE_NULL)
	{
		n = ASE_MMGR_ALLOC (dll->mmgr, ASE_SIZEOF(ase_dll_node_t));
		if (n == ASE_NULL) return ASE_NULL;
		n->dptr = dptr;
	}
	else if (dll->copier == ASE_DLL_COPIER_INLINE)
	{
		n = ASE_MMGR_ALLOC (dll->mmgr, ASE_SIZEOF(ase_dll_node_t) + dlen);
		if (n == ASE_NULL) return ASE_NULL;

		ASE_MEMCPY (n + 1, dptr, dlen);
		n->dptr = n + 1;
	}
	else
	{
		n = ASE_MMGR_ALLOC (dll->mmgr, ASE_SIZEOF(ase_dll_node_t));
		if (n == ASE_NULL) return ASE_NULL;
		n->dptr = dll->copier (dll, dptr, dlen);
		if (n->dptr == ASE_NULL)
		{
			ASE_MMGR_FREE (dll->mmgr, n);
			return ASE_NULL;
		}
	}

	n->dlen = dlen; 
	n->next = ASE_NULL;	
	n->prev = ASE_NULL;

	return n;
}

ase_dll_node_t* ase_dll_insert (
	ase_dll_t* dll, ase_dll_node_t* pos, void* dptr, ase_size_t dlen)
{
	ase_dll_node_t* n = alloc_node (dll, dptr, dlen);
	if (n == ASE_NULL) return ASE_NULL;

	if (pos == ASE_NULL)
	{
		/* insert at the end */
		if (dll->head == ASE_NULL)
		{
			ASE_ASSERT (dll->tail == ASE_NULL);
			dll->head = n;
		}
		else dll->tail->next = n;

		dll->tail = n;
	}
	else
	{
		/* insert in front of the positional node */
		n->next = pos;
		if (pos == dll->head) dll->head = n;
		else
		{
			/* take note of performance penalty */
			ase_dll_node_t* n2 = dll->head;
			while (n2->next != pos) n2 = n2->next;
			n2->next = n;
		}
	}

	dll->size++;
	return n;
}

ase_dll_node_t* ase_dll_pushhead (ase_dll_t* dll, void* data, ase_size_t size)
{
	return ase_dll_insert (dll, dll->head, data, size);
}

ase_dll_node_t* ase_dll_pushtail (ase_dll_t* dll, void* data, ase_size_t size)
{
	return ase_dll_insert (dll, ASE_NULL, data, size);
}

void ase_dll_delete (ase_dll_t* dll, ase_dll_node_t* pos)
{
	if (pos == ASE_NULL) return; /* not a valid node */

	if (pos == dll->head)
	{
		/* it is simple to delete the head node */
		dll->head = pos->next;
		if (dll->head == ASE_NULL) dll->tail = ASE_NULL;
	}
	else 
	{
		/* but deletion of other nodes has significant performance
		 * penalty as it has look for the predecessor of the 
		 * target node */
		ase_dll_node_t* n2 = dll->head;
		while (n2->next != pos) n2 = n2->next;

		n2->next = pos->next;

		/* update the tail node if necessary */
		if (pos == dll->tail) dll->tail = n2;
	}

	if (dll->freeer != ASE_NULL)
	{
		/* free the actual data */
		dll->freeer (dll, pos->dptr, pos->dlen);
	}

	/* free the node */
	ASE_MMGR_FREE (dll->mmgr, pos);

	/* decrement the number of elements */
	dll->size--;
}

void ase_dll_pophead (ase_dll_t* dll)
{
	ase_dll_delete (dll, dll->head);
}

void ase_dll_poptail (ase_dll_t* dll)
{
	ase_dll_delete (dll, dll->tail);
}

void ase_dll_walk (ase_dll_t* dll, ase_dll_walker_t walker, void* arg)
{
	ase_dll_node_t* n = dll->head;

	while (n != ASE_NULL)
	{
		if (walker(dll,n,arg) == ASE_DLL_WALK_STOP) return;
		n = n->next;
	}
}

void* ase_dll_copyinline (ase_dll_t* dll, void* dptr, ase_size_t dlen)
{
	/* this is a dummy copier */
	return ASE_NULL;
}

