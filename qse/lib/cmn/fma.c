/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/fma.h>
#include "mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (fma)

qse_fma_t* qse_fma_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, 
	qse_size_t blksize, qse_size_t maxblks, qse_size_t maxcnks)
{
	qse_fma_t* fma;

	fma = (qse_fma_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*fma) + xtnsize);
	if (fma == QSE_NULL) return QSE_NULL;

	if (qse_fma_init (fma, mmgr, blksize, maxblks, maxcnks) <= -1)
	{
		QSE_MMGR_FREE (mmgr, fma);
		return QSE_NULL;
	}

	QSE_MEMSET (fma + 1, 0, xtnsize);
	return fma;
}

void qse_fma_close (qse_fma_t* fma)
{
	qse_fma_fini (fma);
	QSE_MMGR_FREE (fma->mmgr, fma);
}

int qse_fma_init (
	qse_fma_t* fma, qse_mmgr_t* mmgr,
	qse_size_t blksize, qse_size_t maxblks, qse_size_t maxcnks)
{
	QSE_MEMSET (fma, 0, QSE_SIZEOF(*fma));
	fma->mmgr = mmgr;
	
	if (blksize < QSE_SIZEOF(qse_fma_blk_t)) 
		blksize = QSE_SIZEOF(qse_fma_blk_t);
	if (maxblks <= 0) maxblks = 1;

	fma->blksize = blksize;
	fma->maxblks = maxblks;
	fma->maxcnks = maxcnks;

	return 0;
}

void qse_fma_fini (qse_fma_t* fma)
{
	/* destroys the chunks allocated */
	while (fma->cnkhead)
	{
		qse_fma_cnk_t* next = fma->cnkhead->next;
		QSE_MMGR_FREE (fma->mmgr, fma->cnkhead);
		fma->cnkhead = next;
	}
}

static QSE_INLINE qse_fma_cnk_t* add_chunk (qse_fma_t* fma)
{
	qse_fma_cnk_t* cnk; 
	qse_fma_blk_t* blk;
	qse_size_t i;

	/* check if there are too many chunks */
	if (fma->maxcnks && fma->numcnks >= fma->maxcnks) return QSE_NULL;

	/* allocate a chunk */
	cnk = (qse_fma_cnk_t*) QSE_MMGR_ALLOC (fma->mmgr, 
		QSE_SIZEOF(*cnk) + fma->blksize * fma->maxblks);
	if (cnk == QSE_NULL) return QSE_NULL;

	/* weave the blocks in the chunk to the free block list */
	fma->freeblk = (qse_fma_blk_t*)(cnk + 1);
	blk = fma->freeblk;
	for (i = 1; i < fma->maxblks; i++)
	{
		blk->next = (qse_fma_blk_t*)((qse_byte_t*)blk + fma->blksize);
		blk = blk->next;
	}
	blk->next = QSE_NULL;

	/* weave the chunk to the chunk list */
	cnk->next = fma->cnkhead;
	fma->cnkhead = cnk;
	fma->numcnks++;

	return cnk;
}

void* qse_fma_alloc (qse_fma_t* fma, qse_size_t size)
{
	void* blk;

	QSE_ASSERTX (size <= fma->blksize, 
		"You must not request a block larger than the fixed size set in the allocator. Use a generic allocator instead"
	);

	if (size > fma->blksize) return QSE_NULL;

	if ((blk = fma->freeblk) == QSE_NULL)
	{
		if (add_chunk (fma) == QSE_NULL) return QSE_NULL;
		blk = fma->freeblk;
	}
	fma->freeblk = fma->freeblk->next;
	return blk;
}

void* qse_fma_calloc (qse_fma_t* fma, qse_size_t size)
{
	void* ptr = qse_fma_alloc (fma, size);
	if (size) QSE_MEMSET (ptr, 0, size);
	return ptr;
}

void* qse_fma_realloc (qse_fma_t* fma, void* blk, qse_size_t size)
{
	if (blk)
	{
		QSE_ASSERTX (size <= fma->blksize, 
			"A block can be enlarged with a fixed-size block allocator. Use a generic allocator instead"
		);

		if (size > fma->blksize) return QSE_NULL;
		return blk;
	}

	return qse_fma_alloc (fma, size);
}

void qse_fma_free (qse_fma_t* fma, void* blk)
{
	((qse_fma_blk_t*)blk)->next = fma->freeblk;
	fma->freeblk = blk;
}
