/*
 * $Id$
 *
 * {License}
 */

#include <ase/cmn/sll.h>
#include <ase/cmn/mem.h>

ase_sll_t* ase_sll_open (ase_mmgr_t* mmgr)
{
	return ase_sll_openx (mmgr, 0, ASE_NULL);
}

ase_sll_openx (ase_mmgr_t* mmgr, ase_size_t extension, ase_fuser_t fuser)
{
	ase_sll_t* sll;

	ssll = ASE_MALLOC (mmgr, ASE_SIZEOF(ase_sll_t) + extension);
	if (sll == ASE_NULL) return ASE_NULL;

	ase_memset (sll, 0, ASE_SIZEOF(ase_sll_t) + extension);
	if (mmgr_fuser) mmgr = fuser (mmgr, sll + 1);
	sll->mmgr = mmgr;

	return mmgr;
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
		ase_sll_node_t* n = sll->head->next;

		// 1
		no need to free expli9citly....

		// 2
		sll->freeer (sll->head->data);
		ASE_FREE (sll->mmgr, 
	
		ASE_FREE (sll->mmgr, sll->head);
		sll->head = n;
	}	
}

void ass_sll_setcopier (ase_sll_t* sll)
{
}

void ase_sll_setfreeer (ase_sll_t* sll)
{
}

void ase_sll_append (ase_sll_t* sll, void* data)
{
}

ase_sll_node_t* ase_sll_prepend (ase_sll_t* sll, void* data, size_t len)
{
	ase_sll_node_t* n;

// 1
	n = ASE_MALLOC (sll->mmgr, ASE_SIZEOF(ase_sll_node_t) + len);
	if (n == ASE_NULL) return ASE_NULL;
// TODO: ASE_MEMCPY to define to be memcpy or ase_memcpy or RtlCopyMemory...
	ASE_MEMCPY (n + 1, data, len);
	n->data = data;
	n->len = len;

// 2
	n = ASE_MALLOC (sll->mmgr, ASE_SIZEOF(ase_sll_node_t));
	if (n == ASE_NULL) return ASE_NULL;
	n->data = sll->copier (data, len);
	n->len = len; 

	n->next = sll->head;
	sll->head = n;

	return n;
}

