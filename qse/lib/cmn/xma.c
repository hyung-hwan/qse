/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/cmn/xma.h>
#include "mem.h"

#define ALIGN QSE_SIZEOF(qse_size_t)
#define HDRSIZE QSE_SIZEOF(qse_xma_blk_t)

#define SYS_TO_USR(_) (((qse_xma_blk_t*)_) + 1)
#define USR_TO_SYS(_) (((qse_xma_blk_t*)_) - 1)

#define XFIMAX(mmp) (QSE_COUNTOF(mmp->xfree)-1)

struct qse_xma_blk_t 
{
	qse_size_t avail: 1;
	qse_size_t size: (sizeof(qse_size_t)*8)-1; /**< block size */
	qse_xma_blk_t* link; /**< link to the next free block */
	qse_xma_blk_t* blink; /**< link to the previous free block */

	qse_xma_blk_t* back; /**< previous block */
	qse_xma_blk_t* fore; /**< next block */
};

qse_xma_t* qse_xma_open (
	qse_mmgr_t* mmgr, qse_size_t ext, qse_size_t size)
{
	qse_xma_t* mmp;
	qse_xma_blk_t* free;

	if (size < HDRSIZE + ALIGN) size = HDRSIZE + ALIGN;

	mmp = (qse_xma_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*mmp) + ext + size);
	if (mmp == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (mmp, 0, QSE_SIZEOF(*mmp));
	
	/* make the entire region available. the allocatable block begins
	 * past the mmp structure */
	free = (qse_xma_blk_t*)(mmp + 1);
	free->size = size - HDRSIZE; /* size excluding the block header */
	free->link = QSE_NULL;
	free->fore = QSE_NULL;
	free->back = QSE_NULL;
	free->avail = 1;

	/* the entire region is a free block */
	mmp->xfree[XFIMAX(mmp)] = free;
	mmp->head = free;

	/* initialize some statistical variables */
	mmp->stat.total = size;
	mmp->stat.alloc = 0;
	mmp->stat.avail = size - HDRSIZE;
	mmp->stat.nfree = 1;
	mmp->stat.nused = 0;

	return mmp;
}

void qse_xma_close (qse_xma_t* mmp)
{
	QSE_MMGR_FREE (mmp->mmgr, mmp);
}

static QSE_INLINE void attach_block_to_freelist (qse_xma_t* mmp, qse_xma_blk_t* b)
{
	qse_size_t xfi = (b->size / ALIGN) - 1;
	if (xfi > XFIMAX(mmp)) xfi = XFIMAX(mmp);

	b->blink = QSE_NULL;
	b->link = mmp->xfree[xfi];
	if (mmp->xfree[xfi]) mmp->xfree[xfi]->blink = b;
	mmp->xfree[xfi] = b;		
}

static QSE_INLINE void detach_block_from_freelist (qse_xma_t* mmp, qse_xma_blk_t* b)
{
	qse_xma_blk_t* x, * y;

	x = b->blink;
	y = b->link;

	if (x) x->link = y;
	else 
	{
		qse_size_t xfi = (b->size / ALIGN) - 1;
		if (xfi > XFIMAX(mmp)) xfi = XFIMAX(mmp);
		mmp->xfree[xfi] = y;
	}
	if (y) y->blink = x;
}

static qse_xma_blk_t* alloc_from_freelist (
	qse_xma_t* mmp, qse_size_t xfi, qse_size_t size)
{
	qse_xma_blk_t* free;

	for (free = mmp->xfree[xfi]; free; free = free->link)
	{
		if (free->size >= size)
		{
			qse_size_t rem;

			rem  = free->size - size;
			if (rem > HDRSIZE)
			{
				qse_xma_blk_t* tmp;

				/* the remaining part is large enough to hold 
				 * another block. let's split it */
				tmp = (qse_xma_blk_t*)(((qse_byte_t*)(free + 1)) + size);

				tmp->avail = 1;
				tmp->size = rem - HDRSIZE;
				tmp->fore = free->fore;
				tmp->back = free;
				if (free->fore) free->fore->back = tmp;
				free->fore = tmp;
				free->size = size;

				tmp->link = free->link;
				free->link = tmp;

				mmp->stat.avail -= HDRSIZE;
			}
			else
			{
				/* decrement the number of free blocks as the current
				 * block is allocated as a whole without being split */
				mmp->stat.nfree--;
			}


			if (free->link) free->link->blink = free->blink;
			if (free->blink) free->blink->link = free->link;
			else
			{
				QSE_ASSERT (mmp->xfree[xfi] == free);
				mmp->xfree[xfi] = free->link;
			}

			free->avail = 0;
			/*
			free->link = QSE_NULL;
			free->blink = QSE_NULL;
			*/

			mmp->stat.nused++;
			mmp->stat.alloc += free->size;
			mmp->stat.avail -= free->size;
			return free;
		}
	}

	return QSE_NULL;
}

void* qse_xma_alloc (qse_xma_t* mmp, qse_size_t size)
{
	qse_xma_blk_t* free;
	qse_size_t xfi;

	if (size <= 0) size = 1;

	/* round up 'size' to the multiples of ALIGN */
	/*size = (size + ALIGN - 1) & ~(ALIGN - 1); */
	size = ((size + ALIGN - 1) / ALIGN) * ALIGN;

	QSE_ASSERT (size >= ALIGN);
	xfi = (size / ALIGN) - 1;
	if (xfi > XFIMAX(mmp)) xfi = XFIMAX(mmp);

	if (xfi < XFIMAX(mmp) && mmp->xfree[xfi])
	{
		/* try the best fit */
printf ("FOUND A BEST FIT...............\n");
		free = mmp->xfree[xfi];

		QSE_ASSERT (free->avail != 0);
		QSE_ASSERT (free->size == size);

		detach_block_from_freelist (mmp, free);
		free->avail = 0;

		mmp->stat.nfree--;
		mmp->stat.nused++;
		mmp->stat.alloc += free->size;
		mmp->stat.avail -= free->size;
	}
	else
	{
		/* traverses the chain of free blocks */
		free = alloc_from_freelist (mmp, XFIMAX(mmp), size);
		if (free == QSE_NULL)
		{
			for (++xfi; xfi < XFIMAX(mmp) - 1; xfi++)
			{
				free = alloc_from_freelist (mmp, xfi, size);
				if (free != QSE_NULL) break;
			}

			if (free == QSE_NULL) return QSE_NULL;
		}
	}

	return SYS_TO_USR(free);
}

void qse_xma_free (qse_xma_t* mmp, void* b)
{
	qse_xma_blk_t* blk = USR_TO_SYS(b);

	/*QSE_ASSERT (blk->link == QSE_NULL);*/

	/* update statistical variables */
	mmp->stat.nused--;
	mmp->stat.alloc -= blk->size;

	if ((blk->back && blk->back->avail) &&
	    (blk->fore && blk->fore->avail))
	{
		/*
		 * Merge the block with surrounding blocks
		 *
		 *                    blk 
		 *           +-----+   |   +-----+     +------+
		 *           |     V   v   |     v     |      V
		 * +------------+------------+------------+------------+
		 * |     X      |            |     Y      |     Z      |
		 * +------------+------------+------------+------------+
		 *           ^     |     ^      |     ^      |
		 *           +-----+     +------+     +------+
		 *
		 *           
		 *      +-----------------------------------+
		 *      |                                   V
		 * +--------------------------------------+------------+
		 * |     X                                |     Z      |
		 * +--------------------------------------+------------+
		 *      ^                                   |
		 *      +-----------------------------------+
		 */
		qse_xma_blk_t* x = blk->back;
		qse_xma_blk_t* y = blk->fore;
		qse_xma_blk_t* z = y->fore;
		qse_size_t bs = blk->size + HDRSIZE + y->size + HDRSIZE;

		detach_block_from_freelist (mmp, x);
		detach_block_from_freelist (mmp, y);

		x->size += bs;
		x->fore = z;
		if (z) z->back = x;	

		attach_block_to_freelist (mmp, x);

		mmp->stat.nfree--;
		mmp->stat.avail += bs;
	}
	else if (blk->fore && blk->fore->avail)
	{
		/*
		 * Merge the block with the next block
		 *
		 *   blk
		 *    |      +-----+     +------+
		 *    v      |     v     |      V
		 * +------------+------------+------------+
		 * |            |     X      |     Y      |
		 * +------------+------------+------------+
		 *          ^      |     ^      |
		 *          +------+     +------+
		 *
		 *   blk
		 *    |      +------------------+
		 *    v      |                  V
		 * +-------------------------+------------+
		 * |                         |     Y      |
		 * +-------------------------+------------+
		 *          ^                   |
		 *          +-------------------+
		 */
		qse_xma_blk_t* x = blk->fore;
		qse_xma_blk_t* y = x->fore;
		qse_size_t bs = x->size + HDRSIZE;

		/* detach x from the free list */
		detach_block_from_freelist (mmp, x);

		/* update the block availability */
		blk->avail = 1;
		/* update the block size. HDRSIZE for the header space in x */
		blk->size += bs;

		/* update the backward link of Y */
		if (y) y->back = blk;
		/* update the forward link of the block being freed */
		blk->fore = y;

		/* attach blk to the free list */
		attach_block_to_freelist (mmp, blk);

		mmp->stat.avail += bs;
	}
	else if (blk->back && blk->back->avail)
	{
		/*
		 * Merge the block with the previous block 
		 *
		 *                   blk
		 *          +-----+   |    +-----+  
		 *          |     V   v    |     v  
		 * +------------+------------+------------+
		 * |     X      |            |     Y      |
		 * +------------+------------+------------+
		 *         ^      |      ^      | 
		 *         +------+      +------+   
		 *
		 *               
		 *          +---------------------+  
		 *          |                     v  
		 * +-------------------------+------------+
		 * |     X                   |     Y      |
		 * +-------------------------+------------+
		 *         ^                    | 
		 *         +--------------------+   
		 *
		 */
		qse_xma_blk_t* x = blk->back;
		qse_xma_blk_t* y = blk->fore;
		qse_size_t bs = blk->size + HDRSIZE;

		detach_block_from_freelist (mmp, x);

		x->size += blk->size + HDRSIZE;
		x->fore = y;
		if (y) y->back = x;

		attach_block_to_freelist (mmp, x);

		mmp->stat.avail += bs;
	}
	else
	{
		blk->avail = 1;
		attach_block_to_freelist (mmp, blk);

		mmp->stat.nfree++;
		mmp->stat.avail += blk->size;
	}
}

void qse_xma_dump (qse_xma_t* mmp)
{
	qse_xma_blk_t* tmp;
	unsigned long long fsum, asum, isum;

	printf ("<MMP DUMP>\n");
	printf ("== statistics ==\n");
	printf ("total = %llu\n", (unsigned long long)mmp->stat.total);
	printf ("alloc = %llu\n", (unsigned long long)mmp->stat.alloc);
	printf ("avail = %llu\n", (unsigned long long)mmp->stat.avail);

	printf ("== blocks ==\n");
	printf (" size               avail address\n");
	for (tmp = mmp->head, fsum = 0, asum = 0; tmp; tmp = tmp->fore)
	{
		printf (" %-18llu %-5d %p\n", (unsigned long long)tmp->size, tmp->avail, tmp);
		if (tmp->avail) fsum += tmp->size;
		else asum += tmp->size;
	}

	isum = (mmp->stat.nfree + mmp->stat.nused) * HDRSIZE;

	printf ("---------------------------------------\n");
	printf ("Allocated blocks: %18llu bytes\n", asum);
	printf ("Available blocks: %18llu bytes\n", fsum);
	printf ("Internal use    : %18llu bytes\n", isum);
	printf ("Total           : %18llu bytes\n", asum + fsum + isum);
}

#if 0
int main ()
{
	int i;
	void* ptr[100];

	qse_xma_t* mmp = qse_xma_open (100000L);
	if (mmp == QSE_NULL) 
	{
		printf ("cannot open mmp\n");
		return -1;
	}

	for (i = 0; i < 100; i++)
	{
		int sz = (i + 1) * 10;
		/*int sz = 10240;*/
		ptr[i] = qse_xma_alloc (mmp, sz);
		if (ptr[i] == QSE_NULL) 
		{
			printf ("failed to alloc %d\n", sz);
			break;
		}
		printf ("%d %p\n", sz, ptr[i]);
	}

	for (--i; i > 0; i-= 3)
	{
		if (i >= 0) qse_xma_free (mmp, ptr[i]);
	}

/*
	qse_xma_free (mmp, ptr[0]);
	qse_xma_free (mmp, ptr[1]);
	qse_xma_free (mmp, ptr[2]);
*/

	{
		void* x, * y;

		printf ("%p\n", qse_xma_alloc (mmp, 5000));
		printf ("%p\n", qse_xma_alloc (mmp, 1000));
		printf ("%p\n", (x = qse_xma_alloc (mmp, 10)));
		printf ("%p\n", (y = qse_xma_alloc (mmp, 40)));

		if (x) qse_xma_free (mmp, x);
		if (y) qse_xma_free (mmp, y);
		printf ("%p\n", (x = qse_xma_alloc (mmp, 10)));
		printf ("%p\n", (y = qse_xma_alloc (mmp, 40)));
	}
	qse_xma_dump (mmp);

	qse_xma_close (mmp);
	return 0;
}
#endif
