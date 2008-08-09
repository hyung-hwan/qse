/*
 * $Id$
 *
 * {License}
 */

#include <ase/cmn/sll.h>
#include "mem.h"

struct ase_sll_t
{
	ase_mmgr_t* mmgr;

	ase_sll_copier_t copier;
	ase_sll_freeer_t freeer;

	ase_size_t size;
	ase_sll_node_t* head;
	ase_sll_node_t* tail;
};

void* ase_sll_copyinline (ase_sll_t* sll, void* dptr, ase_size_t dlen)
{
	/* this is a dummy copier */
	return ASE_NULL;
}

ase_sll_t* ase_sll_open (ase_mmgr_t* mmgr)
{
	return ase_sll_openx (mmgr, 0, ASE_NULL);
}

ase_sll_t* ase_sll_openx (ase_mmgr_t* mmgr, ase_size_t extension, ase_fuser_t fuser)
{
	ase_sll_t* sll;

	sll = ASE_MALLOC (mmgr, ASE_SIZEOF(ase_sll_t) + extension);
	if (sll == ASE_NULL) return ASE_NULL;

	ASE_MEMSET (sll, 0, ASE_SIZEOF(ase_sll_t) + extension);
	if (fuser != ASE_NULL) mmgr = fuser (mmgr, sll + 1);
	sll->mmgr = mmgr;

	return sll;
}

void ase_sll_close (ase_sll_t* sll)
{
	ase_sll_clear (sll);
	ASE_FREE (sll->mmgr, sll);
}

void ase_sll_clear (ase_sll_t* sll)
{
	while (sll->head != ASE_NULL)
	{
		ase_sll_node_t* h = sll->head;
		sll->head = h->next;

		if (sll->freeer != ASE_NULL)
		{
			sll->freeer (sll, h->data.ptr, h->data.len);
		}
	
		ASE_FREE (sll->mmgr, h);
	}	

	sll->tail = ASE_NULL;
}

void* ase_sll_getextension (ase_sll_t* sll)
{
	return sll + 1;
}

ase_size_t ase_sll_getsize (ase_sll_t* sll)
{
	return sll->size;
}

ase_sll_node_t* ase_sll_gethead (ase_sll_t* sll)
{
	return sll->head;
}

ase_sll_node_t* ase_sll_gettail (ase_sll_t* sll)
{
	return sll->tail;
}

void ass_sll_setcopier (ase_sll_t* sll, ase_sll_copier_t copier)
{
	sll->copier = copier;
}

void ase_sll_setfreeer (ase_sll_t* sll, ase_sll_freeer_t freeer)
{
	sll->freeer = freeer;
}

static ase_sll_node_t* alloc_node (ase_sll_t* sll, void* data, ase_size_t size)
{
	ase_sll_node_t* n;

	if (sll->copier == ASE_NULL)
	{
		n = ASE_MALLOC (sll->mmgr, ASE_SIZEOF(ase_sll_node_t));
		if (n == ASE_NULL) return ASE_NULL;
		n->data.ptr = data;
	}
	else if (sll->copier == ASE_SLL_COPIER_INLINE)
	{
		n = ASE_MALLOC (sll->mmgr, ASE_SIZEOF(ase_sll_node_t) + size);
		if (n == ASE_NULL) return ASE_NULL;

		ASE_MEMCPY (n + 1, data, size);
		n->data.ptr = n + 1;
	}
	else
	{
		n = ASE_MALLOC (sll->mmgr, ASE_SIZEOF(ase_sll_node_t));
		if (n == ASE_NULL) return ASE_NULL;
		n->data.ptr = sll->copier (sll, data, size);
	}

	n->data.len = size; 
	n->next = ASE_NULL;	
}

ase_sll_node_t* ase_sll_prepend (ase_sll_t* sll, void* data, ase_size_t size)
{
	ase_sll_node_t* n = alloc_node (sll, data, size);
	if (n == ASE_NULL) return ASE_NULL;

	n->next = sll->head;

	sll->head = n;
	if (sll->tail == ASE_NULL) sll->tail = n;
	sll->size++;

	return n;
}

ase_sll_node_t* ase_sll_append (ase_sll_t* sll, void* data, ase_size_t size)
{
	ase_sll_node_t* n = alloc_node (sll, data, size);
	if (n == ASE_NULL) return ASE_NULL;

	if (sll->tail != ASE_NULL) sll->tail->next = n;
	sll->tail = n;
	if (sll->head == ASE_NULL) sll->head = n;
	sll->size++;

	return n;
}

ase_sll_node_t* ase_sll_prepend (ase_sll_t* sll, void* data, ase_size_t size)
{
	return ase_sll_insert (sll, sll->head, data, size);
}


ase_sll_node_t* ase_sll_insert (
	ase_sll_t* sll, ase_sll_node_t* pos, void* data, ase_size_t size)
{
	ase_sll_node_t* n = alloc_node (sll, data, size);
	if (n == ASE_NULL) return ASE_NULL;

	if (pos == ASE_NULL)
	{
		/* insert at the end */
	}
	else
	{
		/* insert in front of the positional node */
		n->next = pos->next;
		pos->next = n;

		if (pos == sll->head) sll->head = n;
		/* TODO: update head & tail properly */
	}
	return n;
}

void ase_sll_walk (ase_sll_t* sll, ase_sll_walker_t walker, void* arg)
{
	ase_sll_node_t* n = sll->head;

	while (n != ASE_NULL)
	{
		walker (sll, n, arg);
		n = n->next;
	}
}
