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

#define XFIMAX(xma) (QSE_COUNTOF(xma->xfree)-1)

struct qse_xma_blk_t 
{
	qse_size_t avail: 1;
	qse_size_t size: (sizeof(qse_size_t)*8)-1; /**< block size */

	struct
	{
		qse_xma_blk_t* prev; /**< link to the previous free block */
		qse_xma_blk_t* next; /**< link to the next free block */
	} f;

	struct
	{
		qse_xma_blk_t* prev; /**< link to the previous block */
		qse_xma_blk_t* next; /**< link to the next block */
	} b;
};

QSE_IMPLEMENT_COMMON_FUNCTIONS (xma)

qse_xma_t* qse_xma_open (qse_mmgr_t* mmgr, qse_size_t ext, qse_size_t size)
{
	qse_xma_t* xma;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	xma = (qse_xma_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*xma) + ext);
	if (xma == QSE_NULL) return QSE_NULL;

	if (qse_xma_init (xma, mmgr, size) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, xma);
		return QSE_NULL;
	}

	return xma;
}

void qse_xma_close (qse_xma_t* xma)
{
	qse_xma_fini (xma);
	QSE_MMGR_FREE (xma->mmgr, xma);
}

qse_xma_t* qse_xma_init (qse_xma_t* xma, qse_mmgr_t* mmgr, qse_size_t size)
{
	qse_xma_blk_t* free;

	if (size < HDRSIZE + ALIGN) size = HDRSIZE + ALIGN;

	free = QSE_MMGR_ALLOC (mmgr, size);
	if (free == QSE_NULL) return QSE_NULL;
	
	/* make the entire region available. the allocatable block begins
	 * past the xma structure */
	free->size = size - HDRSIZE; /* size excluding the block header */
	free->f.prev = QSE_NULL;
	free->f.next = QSE_NULL;
	free->b.next = QSE_NULL;
	free->b.prev = QSE_NULL;
	free->avail = 1;

	QSE_MEMSET (xma, 0, QSE_SIZEOF(*xma));
	xma->mmgr = mmgr;

	/* the entire region is a free block */
	xma->xfree[XFIMAX(xma)] = free;
	xma->head = free;

	/* initialize some statistical variables */
	xma->stat.total = size;
	xma->stat.alloc = 0;
	xma->stat.avail = size - HDRSIZE;
	xma->stat.nfree = 1;
	xma->stat.nused = 0;

	return xma;
}

void qse_xma_fini (qse_xma_t* xma)
{
	QSE_MMGR_FREE (xma->mmgr, xma->head);
}

static QSE_INLINE void attach_block_to_freelist (qse_xma_t* xma, qse_xma_blk_t* b)
{
	qse_size_t xfi = (b->size / ALIGN) - 1;
	if (xfi > XFIMAX(xma)) xfi = XFIMAX(xma);

	b->f.prev = QSE_NULL;
	b->f.next = xma->xfree[xfi];
	if (xma->xfree[xfi]) xma->xfree[xfi]->f.prev = b;
	xma->xfree[xfi] = b;		
}

static QSE_INLINE void detach_block_from_freelist (qse_xma_t* xma, qse_xma_blk_t* b)
{
	qse_xma_blk_t* x, * y;

	x = b->f.prev;
	y = b->f.next;

	if (x) x->f.next = y;
	else 
	{
		qse_size_t xfi = (b->size / ALIGN) - 1;
		if (xfi > XFIMAX(xma)) xfi = XFIMAX(xma);
		xma->xfree[xfi] = y;
	}
	if (y) y->f.prev = x;
}

static qse_xma_blk_t* alloc_from_freelist (
	qse_xma_t* xma, qse_size_t xfi, qse_size_t size)
{
	qse_xma_blk_t* free;

	for (free = xma->xfree[xfi]; free; free = free->f.next)
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
				tmp->b.next = free->b.next;
				tmp->b.prev = free;
				if (free->b.next) free->b.next->b.prev = tmp;
				free->b.next = tmp;
				free->size = size;

				tmp->f.next = free->f.next;
				free->f.next = tmp;

				xma->stat.avail -= HDRSIZE;
			}
			else
			{
				/* decrement the number of free blocks as the current
				 * block is allocated as a whole without being split */
				xma->stat.nfree--;
			}


			if (free->f.next) free->f.next->f.prev = free->f.prev;
			if (free->f.prev) free->f.prev->f.next = free->f.next;
			else
			{
				QSE_ASSERT (xma->xfree[xfi] == free);
				xma->xfree[xfi] = free->f.next;
			}

			free->avail = 0;
			/*
			free->f.next = QSE_NULL;
			free->f.prev = QSE_NULL;
			*/

			xma->stat.nused++;
			xma->stat.alloc += free->size;
			xma->stat.avail -= free->size;
			return free;
		}
	}

	return QSE_NULL;
}

void* qse_xma_alloc (qse_xma_t* xma, qse_size_t size)
{
	qse_xma_blk_t* free;
	qse_size_t xfi;

	if (size <= 0) size = 1;

	/* round up 'size' to the multiples of ALIGN */
	/*size = (size + ALIGN - 1) & ~(ALIGN - 1); */
	size = ((size + ALIGN - 1) / ALIGN) * ALIGN;

	QSE_ASSERT (size >= ALIGN);
	xfi = (size / ALIGN) - 1;
	if (xfi > XFIMAX(xma)) xfi = XFIMAX(xma);

	if (xfi < XFIMAX(xma) && xma->xfree[xfi])
	{
		/* try the best fit */
//printf ("FOUND A BEST FIT...............\n");
		free = xma->xfree[xfi];

		QSE_ASSERT (free->avail != 0);
		QSE_ASSERT (free->size == size);

		detach_block_from_freelist (xma, free);
		free->avail = 0;

		xma->stat.nfree--;
		xma->stat.nused++;
		xma->stat.alloc += free->size;
		xma->stat.avail -= free->size;
	}
	else
	{
		/* traverses the chain of free blocks */
		free = alloc_from_freelist (xma, XFIMAX(xma), size);
		if (free == QSE_NULL)
		{
			for (++xfi; xfi < XFIMAX(xma) - 1; xfi++)
			{
				free = alloc_from_freelist (xma, xfi, size);
				if (free != QSE_NULL) break;
			}

			if (free == QSE_NULL) return QSE_NULL;
		}
	}

	return SYS_TO_USR(free);
}

void qse_xma_free (qse_xma_t* xma, void* b)
{
	qse_xma_blk_t* blk = USR_TO_SYS(b);

	/*QSE_ASSERT (blk->f.next == QSE_NULL);*/

	/* update statistical variables */
	xma->stat.nused--;
	xma->stat.alloc -= blk->size;

	if ((blk->b.prev && blk->b.prev->avail) &&
	    (blk->b.next && blk->b.next->avail))
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
		qse_xma_blk_t* x = blk->b.prev;
		qse_xma_blk_t* y = blk->b.next;
		qse_xma_blk_t* z = y->b.next;
		qse_size_t bs = blk->size + HDRSIZE + y->size + HDRSIZE;

		detach_block_from_freelist (xma, x);
		detach_block_from_freelist (xma, y);

		x->size += bs;
		x->b.next = z;
		if (z) z->b.prev = x;	

		attach_block_to_freelist (xma, x);

		xma->stat.nfree--;
		xma->stat.avail += bs;
	}
	else if (blk->b.next && blk->b.next->avail)
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
		qse_xma_blk_t* x = blk->b.next;
		qse_xma_blk_t* y = x->b.next;
		qse_size_t bs = x->size + HDRSIZE;

		/* detach x from the free list */
		detach_block_from_freelist (xma, x);

		/* update the block availability */
		blk->avail = 1;
		/* update the block size. HDRSIZE for the header space in x */
		blk->size += bs;

		/* update the backward link of Y */
		if (y) y->b.prev = blk;
		/* update the forward link of the block being freed */
		blk->b.next = y;

		/* attach blk to the free list */
		attach_block_to_freelist (xma, blk);

		xma->stat.avail += bs;
	}
	else if (blk->b.prev && blk->b.prev->avail)
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
		qse_xma_blk_t* x = blk->b.prev;
		qse_xma_blk_t* y = blk->b.next;
		qse_size_t bs = blk->size + HDRSIZE;

		detach_block_from_freelist (xma, x);

		x->size += blk->size + HDRSIZE;
		x->b.next = y;
		if (y) y->b.prev = x;

		attach_block_to_freelist (xma, x);

		xma->stat.avail += bs;
	}
	else
	{
		blk->avail = 1;
		attach_block_to_freelist (xma, blk);

		xma->stat.nfree++;
		xma->stat.avail += blk->size;
	}
}

void qse_xma_dump (qse_xma_t* xma)
{
	qse_xma_blk_t* tmp;
	unsigned long long fsum, asum, isum;

	printf ("<MMP DUMP>\n");
	printf ("== statistics ==\n");
	printf ("total = %llu\n", (unsigned long long)xma->stat.total);
	printf ("alloc = %llu\n", (unsigned long long)xma->stat.alloc);
	printf ("avail = %llu\n", (unsigned long long)xma->stat.avail);

	printf ("== blocks ==\n");
	printf (" size               avail address\n");
	for (tmp = xma->head, fsum = 0, asum = 0; tmp; tmp = tmp->b.next)
	{
		printf (" %-18llu %-5d %p\n", (unsigned long long)tmp->size, tmp->avail, tmp);
		if (tmp->avail) fsum += tmp->size;
		else asum += tmp->size;
	}

	isum = (xma->stat.nfree + xma->stat.nused) * HDRSIZE;

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

	qse_xma_t* xma = qse_xma_open (100000L);
	if (xma == QSE_NULL) 
	{
		printf ("cannot open xma\n");
		return -1;
	}

	for (i = 0; i < 100; i++)
	{
		int sz = (i + 1) * 10;
		/*int sz = 10240;*/
		ptr[i] = qse_xma_alloc (xma, sz);
		if (ptr[i] == QSE_NULL) 
		{
			printf ("failed to alloc %d\n", sz);
			break;
		}
		printf ("%d %p\n", sz, ptr[i]);
	}

	for (--i; i > 0; i-= 3)
	{
		if (i >= 0) qse_xma_free (xma, ptr[i]);
	}

/*
	qse_xma_free (xma, ptr[0]);
	qse_xma_free (xma, ptr[1]);
	qse_xma_free (xma, ptr[2]);
*/

	{
		void* x, * y;

		printf ("%p\n", qse_xma_alloc (xma, 5000));
		printf ("%p\n", qse_xma_alloc (xma, 1000));
		printf ("%p\n", (x = qse_xma_alloc (xma, 10)));
		printf ("%p\n", (y = qse_xma_alloc (xma, 40)));

		if (x) qse_xma_free (xma, x);
		if (y) qse_xma_free (xma, y);
		printf ("%p\n", (x = qse_xma_alloc (xma, 10)));
		printf ("%p\n", (y = qse_xma_alloc (xma, 40)));
	}
	qse_xma_dump (xma);

	qse_xma_close (xma);
	return 0;
}
#endif
