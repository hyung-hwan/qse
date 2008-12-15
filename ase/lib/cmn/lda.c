/*
 * $Id: lda.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/lda.h>
#include "mem.h"

#define lda_t    ase_lda_t
#define node_t   ase_lda_node_t
#define copier_t ase_lda_copier_t
#define freeer_t ase_lda_freeer_t
#define comper_t ase_lda_comper_t
#define sizer_t  ase_lda_sizer_t
#define keeper_t ase_lda_keeper_t
#define walker_t ase_lda_walker_t

#define mmgr_t   ase_mmgr_t
#define size_t   ase_size_t

#define TOB(lda,len) ((len)*(lda)->scale)
#define DPTR(node)   ((node)->dptr)
#define DLEN(node)   ((node)->dlen)
#define INVALID      ASE_LDA_INVALID

static int comp_data (lda_t* lda, 
	const void* dptr1, size_t dlen1, 
	const void* dptr2, size_t dlen2)
{
	/*
	if (dlen1 == dlen2) return ASE_MEMCMP (dptr1, dptr2, TOB(lda,dlen1));
	return 1;
	*/

	size_t min = (dlen1 < dlen2)? dlen1: dlen2;
	int n = ASE_MEMCMP (dptr1, dptr2, TOB(lda,min));
	if (n == 0 && dlen1 != dlen2) 
	{
		n = (dlen1 > dlen2)? 1: -1;
	}

	return n;
}

static node_t* alloc_node (lda_t* lda, void* dptr, size_t dlen)
{
	node_t* n;

	if (lda->copier == ASE_LDA_COPIER_SIMPLE)
	{
		n = ASE_MMGR_ALLOC (lda->mmgr, ASE_SIZEOF(node_t));
		if (n == ASE_NULL) return ASE_NULL;
		DPTR(n) = dptr;
	}
	else if (lda->copier == ASE_LDA_COPIER_INLINE)
	{
		n = ASE_MMGR_ALLOC (lda->mmgr, 
			ASE_SIZEOF(node_t) + TOB(lda,dlen));
		if (n == ASE_NULL) return ASE_NULL;

		ASE_MEMCPY (n + 1, dptr, TOB(lda,dlen));
		DPTR(n) = n + 1;
	}
	else
	{
		n = ASE_MMGR_ALLOC (lda->mmgr, ASE_SIZEOF(node_t));
		if (n == ASE_NULL) return ASE_NULL;
		DPTR(n) = lda->copier (lda, dptr, dlen);
		if (DPTR(n) == ASE_NULL) 
		{
			ASE_MMGR_FREE (lda->mmgr, n);
			return ASE_NULL;
		}
	}

	DLEN(n) = dlen; 

	return n;
}

lda_t* ase_lda_open (mmgr_t* mmgr, size_t ext, size_t capa)
{
	lda_t* lda;

	if (mmgr == ASE_NULL) 
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

        lda = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(lda_t) + ext);
        if (lda == ASE_NULL) return ASE_NULL;

        if (ase_lda_init (lda, mmgr, capa) == ASE_NULL)
	{
		ASE_MMGR_FREE (mmgr, lda);
		return ASE_NULL;
	}

	return lda;
}

void ase_lda_close (lda_t* lda)
{
	ase_lda_fini (lda);
	ASE_MMGR_FREE (lda->mmgr, lda);
}

lda_t* ase_lda_init (lda_t* lda, mmgr_t* mmgr, size_t capa)
{
	ASE_MEMSET (lda, 0, ASE_SIZEOF(*lda));

	lda->mmgr = mmgr;
	lda->size = 0;
	lda->capa = 0;
	lda->node = ASE_NULL;

	lda->copier = ASE_LDA_COPIER_SIMPLE;
	lda->comper = comp_data;

	if (ase_lda_setcapa (lda, capa) == ASE_NULL) return ASE_NULL;
	return lda;
}

void ase_lda_fini (lda_t* lda)
{
	ase_lda_clear (lda);

	if (lda->node != ASE_NULL) 
	{
		ASE_MMGR_FREE (lda->mmgr, lda->node);
		lda->node = ASE_NULL;
		lda->capa = 0;
	}
}

void* ase_lda_getextension (lda_t* lda)
{
	return lda + 1;
}

mmgr_t* ase_lda_getmmgr (lda_t* lda)
{
	return lda->mmgr;
}

void ase_lda_setmmgr (lda_t* lda, mmgr_t* mmgr)
{
	lda->mmgr = mmgr;
}

int ase_lda_getscale (lda_t* lda)
{
	return lda->scale;
}

void ase_lda_setscale (lda_t* lda, int scale)
{
	ASE_ASSERTX (scale > 0 && scale <= ASE_TYPE_MAX(ase_byte_t), 
		"The scale should be larger than 0 and less than or equal to the maximum value that the ase_byte_t type can hold");

	if (scale <= 0) scale = 1;
	if (scale > ASE_TYPE_MAX(ase_byte_t)) scale = ASE_TYPE_MAX(ase_byte_t);

	lda->scale = scale;
}

copier_t ase_lda_getcopier (lda_t* lda)
{
	return lda->copier;
}

void ase_lda_setcopier (lda_t* lda, copier_t copier)
{
	if (copier == ASE_NULL) copier = ASE_LDA_COPIER_SIMPLE;
	lda->copier = copier;
}

freeer_t ase_lda_getfreeer (lda_t* lda)
{
	return lda->freeer;
}

void ase_lda_setfreeer (lda_t* lda, freeer_t freeer)
{
	lda->freeer = freeer;
}

comper_t ase_lda_getcomper (lda_t* lda)
{
	return lda->comper;
}

void ase_lda_setcomper (lda_t* lda, comper_t comper)
{
	if (comper == ASE_NULL) comper = comp_data;
	lda->comper = comper;
}

keeper_t ase_lda_getkeeper (lda_t* lda)
{
        return lda->keeper;
}

void ase_lda_setkeeper (lda_t* lda, keeper_t keeper)
{
        lda->keeper = keeper;
}

sizer_t ase_lda_getsizer (lda_t* lda)
{
        return lda->sizer;
}

void ase_lda_setsizer (lda_t* lda, sizer_t sizer)
{
        lda->sizer = sizer;
}

size_t ase_lda_getsize (lda_t* lda)
{
	return lda->size;
}

size_t ase_lda_getcapa (lda_t* lda)
{
	return lda->capa;
}

lda_t* ase_lda_setcapa (lda_t* lda, size_t capa)
{
	void* tmp;

	if (lda->size > capa) 
	{
		ase_lda_delete (lda, capa, lda->size - capa);
		ASE_ASSERT (lda->size <= capa);
	}

	if (capa > 0) 
	{
		if (lda->mmgr->realloc != ASE_NULL && lda->node != ASE_NULL)
		{
			tmp = (ase_lda_node_t**)ASE_MMGR_REALLOC (
				lda->mmgr, lda->node, ASE_SIZEOF(*lda->node)*capa);
			if (tmp == ASE_NULL) return ASE_NULL;
		}
		else
		{
			tmp = (ase_lda_node_t**) ASE_MMGR_ALLOC (
				lda->mmgr, ASE_SIZEOF(*lda->node)*capa);
			if (tmp == ASE_NULL) return ASE_NULL;

			if (lda->node != ASE_NULL)
			{
				size_t x;
				x = (capa > lda->capa)? lda->capa: capa;
				ASE_MEMCPY (tmp, lda->node, 
					ASE_SIZEOF(*lda->node) * x);
				ASE_MMGR_FREE (lda->mmgr, lda->node);
			}
		}
	}
	else 
	{
		if (lda->node != ASE_NULL) 
		{
			ase_lda_clear (lda);
			ASE_MMGR_FREE (lda->mmgr, lda->node);
		}

		tmp = ASE_NULL;
	}

	lda->node = tmp;
	lda->capa = capa;
	
	return lda;
}

size_t ase_lda_search (lda_t* lda, size_t pos, const void* dptr, size_t dlen)
{
	size_t i;

	for (i = pos; i < lda->size; i++) 
	{
		if (lda->node[i] == ASE_NULL) continue;

		if (lda->comper (lda, 
			DPTR(lda->node[i]), DLEN(lda->node[i]),
			dptr, dlen) == 0) return i;
	}

	return INVALID;
}

size_t ase_lda_rsearch (lda_t* lda, size_t pos, const void* dptr, size_t dlen)
{
	size_t i;

	if (lda->size > 0)
	{
		if (pos >= lda->size) pos = lda->size - 1;

		for (i = pos + 1; i-- > 0; ) 
		{
			if (lda->node[i] == ASE_NULL) continue;

			if (lda->comper (lda, 
				DPTR(lda->node[i]), DLEN(lda->node[i]),
				dptr, dlen) == 0) return i;
		}
	}

	return INVALID;
}

size_t ase_lda_upsert (lda_t* lda, size_t pos, void* dptr, size_t dlen)
{
	if (pos < lda->size) return ase_lda_update (lda, pos, dptr, dlen);
	return ase_lda_insert (lda, pos, dptr, dlen);
}

size_t ase_lda_insert (lda_t* lda, size_t pos, void* dptr, size_t dlen)
{
	size_t i;
	node_t* node;

	/* allocate the node first */
	node = alloc_node (lda, dptr, dlen);
	if (node == ASE_NULL) return INVALID;

	/* do resizeing if necessary. 
	 * resizing is performed after node allocation because that way, it 
	 * doesn't modify lda on any errors */
	if (pos >= lda->capa || lda->size >= lda->capa) 
	{
		size_t capa;

		if (lda->sizer)
		{
			capa = (pos >= lda->size)? (pos + 1): (lda->size + 1);
			capa = lda->sizer (lda, capa);
		}
		else
		{
			if (lda->capa <= 0) 
			{
				ASE_ASSERT (lda->size <= 0);
				capa = (pos < 16)? 16: (pos + 1);
			}
			else 
			{
				size_t bound = (pos >= lda->size)? pos: lda->size;
				do { capa = lda->capa * 2; } while (capa <= bound);
			}
		}
		
		if (ase_lda_setcapa(lda,capa) == ASE_NULL) 
		{
			if (lda->freeer) 
				lda->freeer (lda, DPTR(node), DLEN(node));
			ASE_MMGR_FREE (lda->mmgr, node);
			return INVALID;
		}
	}

	if (pos >= lda->capa || lda->size >= lda->capa) 
	{
		/* the buffer is not still enough after resizing */
		if (lda->freeer) 
			lda->freeer (lda, DPTR(node), DLEN(node));
		ASE_MMGR_FREE (lda->mmgr, node);
		return INVALID;
	}

	/* fill in the gap with ASE_NULL */
	for (i = lda->size; i < pos; i++) lda->node[i] = ASE_NULL;

	/* shift values to the next cell */
	for (i = lda->size; i > pos; i--) lda->node[i] = lda->node[i-1];

	/*  set the value */
	lda->node[pos] = node;

	if (pos > lda->size) lda->size = pos + 1;
	else lda->size++;

	return pos;
}

size_t ase_lda_update (lda_t* lda, size_t pos, void* dptr, size_t dlen)
{
	node_t* c;

	if (pos >= lda->size) return INVALID;

	c = lda->node[pos];
	if (c == ASE_NULL)
	{
		/* no previous data */
		lda->node[pos] = alloc_node (lda, dptr, dlen);
		if (lda->node[pos] == ASE_NULL) return INVALID;
	}
	else
	{
		if (dptr == DPTR(c) && dlen == DLEN(c))
		{
			/* updated to the same data */
			if (lda->keeper) lda->keeper (lda, dptr, dlen);	
		}
		else
		{
			/* updated to different data */
			node_t* node = alloc_node (lda, dptr, dlen);
			if (node == ASE_NULL) return INVALID;

			if (lda->freeer != ASE_NULL)
				lda->freeer (lda, DPTR(c), DLEN(c));
			ASE_MMGR_FREE (lda->mmgr, c);

			lda->node[pos] = node;
		}
	}

	return pos;
}

size_t ase_lda_delete (lda_t* lda, size_t index, size_t count)
{
	size_t i;

	if (index >= lda->size) return 0;
	if (count > lda->size - index) count = lda->size - index;

	i = index;

	for (i = index; i < index + count; i++)
	{
		node_t* c = lda->node[i];

		if (c != ASE_NULL)
		{
			if (lda->freeer != ASE_NULL)
				lda->freeer (lda, DPTR(c), DLEN(c));
			ASE_MMGR_FREE (lda->mmgr, c);

			lda->node[i] = ASE_NULL;
		}
	}

	for (i = index + count; i < lda->size; i++)
	{
		lda->node[i-count] = lda->node[i];
	}
	lda->node[lda->size-1] = ASE_NULL;

	lda->size -= count;
	return count;
}

size_t ase_lda_uplete (lda_t* lda, size_t index, size_t count)
{
	size_t i;

	if (index >= lda->size) return 0;
	if (count > lda->size - index) count = lda->size - index;

	i = index;

	for (i = index; i < index + count; i++)
	{
		node_t* c = lda->node[i];

		if (c != ASE_NULL)
		{
			if (lda->freeer != ASE_NULL)
				lda->freeer (lda, DPTR(c), DLEN(c));
			ASE_MMGR_FREE (lda->mmgr, c);

			lda->node[i] = ASE_NULL;
		}
	}

	return count;
}

void ase_lda_clear (lda_t* lda)
{
	size_t i;

	for (i = 0; i < lda->size; i++) 
	{
		node_t* c = lda->node[i];
		if (c != ASE_NULL)
		{
			if (lda->freeer)
				lda->freeer (lda, DPTR(c), DLEN(c));
			ASE_MMGR_FREE (lda->mmgr, c);
			lda->node[i] = ASE_NULL;
		}
	}

	lda->size = 0;
}

void ase_lda_walk (lda_t* lda, walker_t walker, void* arg)
{
	ase_lda_walk_t w = ASE_LDA_WALK_FORWARD;
	size_t i = 0;

	while (1)	
	{
		if (lda->node[i] != ASE_NULL) 
                	w = walker (lda, i, arg);

		if (w == ASE_LDA_WALK_STOP) return;

		if (w == ASE_LDA_WALK_FORWARD) 
		{
			i++;
			if (i >= lda->size) return;
		}
		if (w == ASE_LDA_WALK_BACKWARD) 
		{
			if (i <= 0) return;
			i--;
		}
	}
}

void ase_lda_rwalk (lda_t* lda, walker_t walker, void* arg)
{
	ase_lda_walk_t w = ASE_LDA_WALK_BACKWARD;
	size_t i;

	if (lda->size <= 0) return;
	i = lda->size - 1;

	while (1)	
	{
		if (lda->node[i] != ASE_NULL) 
                	w = walker (lda, i, arg);

		if (w == ASE_LDA_WALK_STOP) return;

		if (w == ASE_LDA_WALK_FORWARD) 
		{
			i++;
			if (i >= lda->size) return;
		}
		if (w == ASE_LDA_WALK_BACKWARD) 
		{
			if (i <= 0) return;
			i--;
		}
	}
}

