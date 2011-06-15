/*
 * $Id: memory.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include "stx.h"
#include "../cmn/mem.h"

int qse_stx_initmem (qse_stx_t* stx, qse_size_t capa)
{
	qse_size_t n;

	QSE_ASSERT (capa > 0);

	QSE_ASSERTX (
		stx->mem.slot == QSE_NULL, 
		"This function is for internal use. Never call this again after initialization"
	);

	stx->mem.slot = (qse_stx_objptr_t*) QSE_MMGR_ALLOC (
		stx->mmgr,
		capa * QSE_SIZEOF(*stx->mem.slot)
	);
	if (stx->mem.slot == QSE_NULL) 
	{
		qse_stx_seterrnum (stx, QSE_STX_ENOMEM, QSE_NULL);
		return -1;
	}

	stx->mem.capa = capa;

	/* weave the free slot list */
	stx->mem.free = &stx->mem.slot[0];
	for (n = 0; n < capa - 1; n++) 
	{
		stx->mem.slot[n] = (qse_stx_objptr_t)&stx->mem.slot[n + 1];
	}
	stx->mem.slot[n] = QSE_NULL;

	return 0;
}

void qse_stx_finimem (qse_stx_t* stx)
{
	/* TODO: free all linked objects...	 */

	QSE_MMGR_FREE (stx->mmgr, stx->mem.slot);
	stx->mem.capa = 0;
	stx->mem.slot = QSE_NULL;
	stx->mem.free = QSE_NULL;
}

void qse_stx_gcmem (qse_stx_t* stx)
{
	/* TODO: implement this function */
}

qse_stx_objidx_t qse_stx_allocmem (qse_stx_t* stx, qse_size_t nbytes)
{
	qse_stx_objptr_t* slot;
	qse_stx_objptr_t objptr;

	/* find the free object slot */
	if (stx->mem.free == QSE_NULL) 
	{
		qse_stx_gcmem (stx);
		if (stx->mem.free == QSE_NULL) 
		{
			/* ran out of object table slots */
/* TODO: NEED TO USE a different error code??? */
			qse_stx_seterrnum (stx, QSE_STX_ENOMEM, QSE_NULL);
			return QSE_STX_OBJIDX_INVALID;
		}
	}

/* TODO: memory allocation by region.. instead of calling individual QSE_MMGR_ALLOC 
 * compaction in gc... etc
*/
	objptr = (qse_stx_objptr_t) QSE_MMGR_ALLOC (stx->mmgr, nbytes);
	if (objptr == QSE_NULL) 
	{
		qse_stx_gcmem (stx);

		objptr = (qse_stx_objptr_t) QSE_MMGR_ALLOC (stx->mmgr, nbytes);
		if (objptr == QSE_NULL) 
		{
			/* ran out of object memory */
			qse_stx_seterrnum (stx, QSE_STX_ENOMEM, QSE_NULL);
			return QSE_STX_OBJIDX_INVALID;
		}
	}

	slot = stx->mem.free;
	stx->mem.free = (qse_stx_objptr_t*)*slot;
	*slot = objptr;

	QSE_MEMSET (objptr, 0, nbytes);
	return (qse_stx_objidx_t)(slot - stx->mem.slot);
}

void qse_stx_freemem (qse_stx_t* stx, qse_stx_objidx_t objidx)
{
	/* 
	 * THIS IS PRIMITIVE LOW-LEVEL DEALLOC. THIS WILL NOT 
	 * DEALLOCATE MEMORY ALLOCATED FOR ITS INSTANCE VARIABLES.
	 */

	QSE_MMGR_FREE (stx->mmgr, stx->mem.slot[objidx]);
	stx->mem.slot[objidx] = (qse_stx_objptr_t)stx->mem.free;
	stx->mem.free = &stx->mem.slot[objidx];
}

void qse_stx_swapmem  (qse_stx_t* stx, qse_stx_objidx_t idx1, qse_stx_objidx_t idx2)
{
	qse_stx_objptr_t tmp;	

	tmp = stx->mem.slot[idx1];
	stx->mem.slot[idx1] = stx->mem.slot[idx2];
	stx->mem.slot[idx2] = tmp;
}
