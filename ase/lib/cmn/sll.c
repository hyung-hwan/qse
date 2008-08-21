/*
 * $Id$
 *
 * {License}
 */

#include <ase/cmn/sll.h>
#include "mem.h"

void* ase_sll_copyinline (ase_sll_t* sll, void* dptr, ase_size_t dlen)
{
	/* this is a dummy copier */
	return ASE_NULL;
}

ase_sll_t* ase_sll_open (
	ase_mmgr_t* mmgr, ase_size_t extension, 
	void (*initializer) (ase_sll_t*))
{
	ase_sll_t* sll;

	if (mmgr == ASE_NULL) 
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

	sll = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(ase_sll_t) + extension);
	if (sll == ASE_NULL) return ASE_NULL;

	ASE_MEMSET (sll, 0, ASE_SIZEOF(ase_sll_t) + extension);
	sll->mmgr = mmgr;

	if (initializer) initializer (sll);

	return sll;
}

void ase_sll_close (ase_sll_t* sll)
{
	ase_sll_clear (sll);
	ASE_FREE (sll->mmgr, sll);
}

void ase_sll_clear (ase_sll_t* sll)
{
	while (sll->head != ASE_NULL) ase_sll_delete (sll, sll->head);
	ASE_ASSERT (sll->tail == ASE_NULL);
}

void* ase_sll_getextension (ase_sll_t* sll)
{
	return sll + 1;
}

ase_mmgr_t* ase_sll_getmmgr (ase_sll_t* sll)
{
	return sll->mmgr;
}

void ase_sll_setmmgr (ase_sll_t* sll, ase_mmgr_t* mmgr)
{
	sll->mmgr = mmgr;
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

ase_sll_copier_t ase_sll_getcopier (ase_sll_t* sll)
{
	return sll->copier;
}

void ase_sll_setcopier (ase_sll_t* sll, ase_sll_copier_t copier)
{
	sll->copier = copier;
}

ase_sll_freeer_t ase_sll_getfreeer (ase_sll_t* sll)
{
	return sll->freeer;
}

void ase_sll_setfreeer (ase_sll_t* sll, ase_sll_freeer_t freeer)
{
	sll->freeer = freeer;
}

static ase_sll_node_t* alloc_node (ase_sll_t* sll, void* dptr, ase_size_t dlen)
{
	ase_sll_node_t* n;

	if (sll->copier == ASE_NULL)
	{
		n = ASE_MALLOC (sll->mmgr, ASE_SIZEOF(ase_sll_node_t));
		if (n == ASE_NULL) return ASE_NULL;
		n->dptr = dptr;
	}
	else if (sll->copier == ASE_SLL_COPIER_INLINE)
	{
		n = ASE_MALLOC (sll->mmgr, ASE_SIZEOF(ase_sll_node_t) + dlen);
		if (n == ASE_NULL) return ASE_NULL;

		ASE_MEMCPY (n + 1, dptr, dlen);
		n->dptr = n + 1;
	}
	else
	{
		n = ASE_MALLOC (sll->mmgr, ASE_SIZEOF(ase_sll_node_t));
		if (n == ASE_NULL) return ASE_NULL;
		n->dptr = sll->copier (sll, dptr, dlen);
	}

	n->dlen = dlen; 
	n->next = ASE_NULL;	

	return n;
}

ase_sll_node_t* ase_sll_insert (
	ase_sll_t* sll, ase_sll_node_t* pos, void* dptr, ase_size_t dlen)
{
	ase_sll_node_t* n = alloc_node (sll, dptr, dlen);
	if (n == ASE_NULL) return ASE_NULL;

	if (pos == ASE_NULL)
	{
		/* insert at the end */
		if (sll->head == ASE_NULL)
		{
			ASE_ASSERT (sll->tail == ASE_NULL);
			sll->head = n;
		}
		else sll->tail->next = n;

		sll->tail = n;
	}
	else
	{
		/* insert in front of the positional node */
		n->next = pos;
		if (pos == sll->head) sll->head = n;
		else
		{
			/* take note of performance penalty */
			ase_sll_node_t* n2 = sll->head;
			while (n2->next != pos) n2 = n2->next;
			n2->next = n;
		}
	}

	sll->size++;
	return n;
}

ase_sll_node_t* ase_sll_pushhead (ase_sll_t* sll, void* data, ase_size_t size)
{
	return ase_sll_insert (sll, sll->head, data, size);
}

ase_sll_node_t* ase_sll_pushtail (ase_sll_t* sll, void* data, ase_size_t size)
{
	return ase_sll_insert (sll, ASE_NULL, data, size);
}

void ase_sll_delete (ase_sll_t* sll, ase_sll_node_t* pos)
{
	if (pos == ASE_NULL) return; /* not a valid node */

	if (pos == sll->head)
	{
		/* it is simple to delete the head node */
		sll->head = pos->next;
		if (sll->head == ASE_NULL) sll->tail = ASE_NULL;
	}
	else 
	{
		/* but deletion of other nodes has significant performance
		 * penalty as it has look for the predecessor of the 
		 * target node */
		ase_sll_node_t* n2 = sll->head;
		while (n2->next != pos) n2 = n2->next;

		n2->next = pos->next;

		/* update the tail node if necessary */
		if (pos == sll->tail) sll->tail = n2;
	}

	if (sll->freeer != ASE_NULL)
	{
		/* free the actual data */
		sll->freeer (sll, pos->dptr, pos->dlen);
	}

	/* free the node */
	ASE_FREE (sll->mmgr, pos);

	/* decrement the number of elements */
	sll->size--;
}

void ase_sll_pophead (ase_sll_t* sll)
{
	ase_sll_delete (sll, sll->head);
}

void ase_sll_poptail (ase_sll_t* sll)
{
	ase_sll_delete (sll, sll->tail);
}

void ase_sll_walk (ase_sll_t* sll, ase_sll_walker_t walker, void* arg)
{
	ase_sll_node_t* n = sll->head;

	while (n != ASE_NULL)
	{
		if (walker(sll,n,arg) == ASE_SLL_WALK_STOP) return;
		n = n->next;
	}
}
