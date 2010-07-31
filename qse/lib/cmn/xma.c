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
#define MINBLKLEN (HDRSIZE + ALIGN)

#define SYS_TO_USR(_) (((qse_xma_blk_t*)_) + 1)
#define USR_TO_SYS(_) (((qse_xma_blk_t*)_) - 1)

/*
 * the xfree array is divided into three region
 * 0 ....................... FIXED ......................... XFIMAX-1 ... XFIMAX
 * |  small fixed-size chains |     large  chains                | huge chain |
 */
#define FIXED QSE_XMA_FIXED
#define XFIMAX(xma) (QSE_COUNTOF(xma->xfree)-1)

struct qse_xma_blk_t 
{
	qse_size_t avail: 1;
	qse_size_t size: QSE_XMA_SIZE_BITS;/**< block size */

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

static QSE_INLINE_ALWAYS qse_size_t szlog2 (qse_size_t n) 
{
	/*
	 * 2**x = n;
	 * x = log2(n);
	 * -------------------------------------------
	 * 	unsigned int x = 0;
	 * 	while((n >> x) > 1) ++x;
	 * 	return x;
	 */

#define BITS (QSE_SIZEOF_SIZE_T * 8)
	int x = BITS - 1;

#if QSE_SIZEOF_SIZE_T >= 32
#	error qse_size_t too large. unsupported platform
#endif

#if QSE_SIZEOF_SIZE_T >= 16
	if ((n & (~(qse_size_t)0 << (BITS-64))) == 0) { x -= 64; n <<= 64; }
#endif
#if QSE_SIZEOF_SIZE_T >= 8
	if ((n & (~(qse_size_t)0 << (BITS-32))) == 0) { x -= 32; n <<= 32; }
#endif
#if QSE_SIZEOF_SIZE_T >= 4 
	if ((n & (~(qse_size_t)0 << (BITS-16))) == 0) { x -= 16; n <<= 16; }
#endif
#if QSE_SIZEOF_SIZE_T >= 2
	if ((n & (~(qse_size_t)0 << (BITS-8))) == 0) { x -= 8; n <<= 8; }
#endif
#if QSE_SIZEOF_SIZE_T >= 1
	if ((n & (~(qse_size_t)0 << (BITS-4))) == 0) { x -= 4; n <<= 4; }
#endif
	if ((n & (~(qse_size_t)0 << (BITS-2))) == 0) { x -= 2; n <<= 2; }
	if ((n & (~(qse_size_t)0 << (BITS-1))) == 0) { x -= 1; }

	return x;
#undef BITS
}

static QSE_INLINE_ALWAYS qse_size_t getxfi (qse_xma_t* xma, qse_size_t size) 
{
	qse_size_t xfi = ((size) / ALIGN) - 1;
	if (xfi >= FIXED) xfi = szlog2(size) - (xma)->bdec + FIXED;
	if (xfi > XFIMAX(xma)) xfi = XFIMAX(xma);
	return xfi;
}

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
	qse_size_t xfi;

	size = ((size + ALIGN - 1) / ALIGN) * ALIGN;
	/* adjust 'size' to be large enough to hold a single smallest block */
	if (size < MINBLKLEN) size = MINBLKLEN;

	/* allocate a memory chunk to use for actual memory allocation */
	free = QSE_MMGR_ALLOC (mmgr, size);
	if (free == QSE_NULL) return QSE_NULL;
	
	/* initialize the header part of the free chunk */
	free->avail = 1;
	free->size = size - HDRSIZE; /* size excluding the block header */
	free->f.prev = QSE_NULL;
	free->f.next = QSE_NULL;
	free->b.next = QSE_NULL;
	free->b.prev = QSE_NULL;

	QSE_MEMSET (xma, 0, QSE_SIZEOF(*xma));
	xma->mmgr = mmgr;
	xma->bdec = szlog2(FIXED*ALIGN); /* precalculate the decrement value */

	/* the entire chunk is a free block */
	xfi = getxfi(xma,free->size);
	xma->xfree[xfi] = free; /* locate it at the right slot */
	xma->head = free; /* store it for furture reference */

	/* initialize some statistical variables */
#ifdef QSE_XMA_ENABLE_STAT
	xma->stat.total = size;
	xma->stat.alloc = 0;
	xma->stat.avail = size - HDRSIZE;
	xma->stat.nfree = 1;
	xma->stat.nused = 0;
#endif
	
	return xma;
}

void qse_xma_fini (qse_xma_t* xma)
{
	QSE_MMGR_FREE (xma->mmgr, xma->head);
}

static QSE_INLINE void attach_to_freelist (qse_xma_t* xma, qse_xma_blk_t* b)
{
	qse_size_t xfi = getxfi(xma,b->size);

	b->f.prev = QSE_NULL;
	b->f.next = xma->xfree[xfi];
	if (xma->xfree[xfi]) xma->xfree[xfi]->f.prev = b;
	xma->xfree[xfi] = b;		
}

static QSE_INLINE void detach_from_freelist (qse_xma_t* xma, qse_xma_blk_t* b)
{
	qse_xma_blk_t* p, * n;

	p = b->f.prev;
	n = b->f.next;

	if (p)
	{
		p->f.next = n;
	}
	else 
	{
		qse_size_t xfi = getxfi(xma,b->size);
		QSE_ASSERT (b == xma->xfree[xfi]);
		xma->xfree[xfi] = n;
	}
	if (n) n->f.prev = p;
}

static qse_xma_blk_t* alloc_from_freelist (
	qse_xma_t* xma, qse_size_t xfi, qse_size_t size)
{
	qse_xma_blk_t* free;

	for (free = xma->xfree[xfi]; free; free = free->f.next)
	{
		if (free->size >= size)
		{
			detach_from_freelist (xma, free);

			qse_size_t rem = free->size - size;
			if (rem >= MINBLKLEN)
			{
				qse_xma_blk_t* tmp;

				/* the remaining part is large enough to hold 
				 * another block. let's split it 
				 */

				/* shrink the size of the 'free' block */
				free->size = size;

				/* let 'tmp' point to the remaining part */
				tmp = (qse_xma_blk_t*)(((qse_byte_t*)(free + 1)) + size);

				/* initialize some fields */
				tmp->avail = 1;
				tmp->size = rem - HDRSIZE;

				/* link 'tmp' to the block list */
				tmp->b.next = free->b.next;
				tmp->b.prev = free;
				if (free->b.next) free->b.next->b.prev = tmp;
				free->b.next = tmp;

				/* add the remaining part to the free list */
				attach_to_freelist (xma, tmp);

#ifdef QSE_XMA_ENABLE_STAT
				xma->stat.avail -= HDRSIZE;
#endif
			}
#ifdef QSE_XMA_ENABLE_STAT
			else
			{
				/* decrement the number of free blocks as the current
				 * block is allocated as a whole without being split */
				xma->stat.nfree--;
			}
#endif

			free->avail = 0;
			/*
			free->f.next = QSE_NULL;
			free->f.prev = QSE_NULL;
			*/

#ifdef QSE_XMA_ENABLE_STAT
			xma->stat.nused++;
			xma->stat.alloc += free->size;
			xma->stat.avail -= free->size;
#endif
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
	xfi = getxfi(xma,size);

	/*if (xfi < XFIMAX(xma) && xma->xfree[xfi])*/
	if (xfi < FIXED && xma->xfree[xfi])
	{
		/* try the best fit */
		free = xma->xfree[xfi];

		QSE_ASSERT (free->avail != 0);
		QSE_ASSERT (free->size == size);

		detach_from_freelist (xma, free);
		free->avail = 0;

#ifdef QSE_XMA_ENABLE_STAT
		xma->stat.nfree--;
		xma->stat.nused++;
		xma->stat.alloc += free->size;
		xma->stat.avail -= free->size;
#endif
	}
	else if (xfi == XFIMAX(xma))
	{
		/* huge block */
		free = alloc_from_freelist (xma, XFIMAX(xma), size);
		if (free == QSE_NULL) return QSE_NULL;
	}
	else
	{
		if (xfi >= FIXED)
		{
			/* get the block from its own large chain */
			free = alloc_from_freelist (xma, xfi, size);
			if (free == QSE_NULL)
			{
				/* borrow a large block from the huge block chain */
				free = alloc_from_freelist (xma, XFIMAX(xma), size);
			}
		}
		else
		{
			/* borrow a small block from the huge block chain */
			free = alloc_from_freelist (xma, XFIMAX(xma), size);
			if (free == QSE_NULL) xfi = FIXED - 1;
		}

		if (free == QSE_NULL)
		{
			/* try each large block chain left */
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

static void* _realloc_merge (qse_xma_t* xma, void* b, qse_size_t size)
{
	qse_xma_blk_t* blk = USR_TO_SYS(b);

	if (size > blk->size)
	{
		qse_size_t req = size - blk->size;
		qse_xma_blk_t* n;
		qse_size_t rem;
		
		n = blk->b.next;
		if (!n || !n->avail || req > n->size) return QSE_NULL;

		/* merge the current block with the next block 
		 * if it is available */

		detach_from_freelist (xma, n);

		rem = (HDRSIZE + n->size) - req;
		if (rem >= MINBLKLEN)
		{
			qse_xma_blk_t* tmp;
			/* store n->b.next in case 'tmp' begins somewhere 
			 * in the header part of n */
			qse_xma_blk_t* nn = n->b.next; 

			tmp = (qse_xma_blk_t*)(((qse_byte_t*)n) + req);

			tmp->avail = 1;
			tmp->size = rem - HDRSIZE;
			attach_to_freelist (xma, tmp);

			blk->size += req;

			tmp->b.next = nn;
			if (nn) nn->b.prev = tmp;

			blk->b.next = tmp;
			tmp->b.prev = blk;

#ifdef QSE_XMA_ENABLE_STAT
			xma->stat.alloc += req;
			xma->stat.avail -= req; /* req + HDRSIZE(tmp) - HDRSIZE(n) */
#endif
		}
		else
		{
			blk->size += HDRSIZE + n->size;
			blk->b.next = n->b.next;
			if (n->b.next) n->b.next->b.prev = blk;

#ifdef QSE_XMA_ENABLE_STAT
			xma->stat.nfree--;
			xma->stat.alloc += HDRSIZE + n->size;
			xma->stat.avail -= n->size;
#endif
		}
		
#if 0
		qse_size_t total = 0;
		qse_xma_blk_t* x = QSE_NULL;

		/* find continuous blocks available to accomodate
 		 * additional space required */
		for (n = blk->b.next; n && n->avail; n = n->b.next)
		{
			total += n->size + HDRSIZE;
			if (req <= total) 
			{
				x = n;
				break;
			}
			n = n->b.next;	
		}

		if (!x) 
		{
			/* no such blocks. return failure */
			return QSE_NULL;
		}

		for (n = blk->b.next; n != x; n = n->b.next)
		{
			detach_from_freelist (xma, n);
#ifdef QSE_XMA_ENABLE_STAT
			xma->stat.nfree--;
#endif
		}

		detach_from_freelist (xma, x);

		rem = total - req;
		if (rem >= MINBLKLEN)
		{
			qse_xma_blk_t* tmp;

			tmp = (qse_xma_blk_t*)(((qse_byte_t*)(blk->b.next + 1)) + req);
			tmp->avail = 1;
			tmp->size = rem - HDRSIZE;

			attach_to_freelist (xma, tmp);

			blk->size += req;

			tmp->b.next = x->b.next;
			if (x->b.next) x->b.next->b.prev = tmp;

			blk->b.next = tmp;
			tmp->b.prev = blk;

#ifdef QSE_XMA_ENABLE_STAT
			xma->stat.alloc += req;
			xma->stat.avail -= req + HDRSIZE;
#endif
		}
		else
		{
			blk->size += total;
			blk->b.next = x->b.next;
			if (x->b.next) x->b.next->b.prev = blk;

#ifdef QSE_XMA_ENABLE_STAT
			xma->stat.nfree--;
			xma->stat.alloc += total;
			xma->stat.avail -= total;
#endif
		}
#endif
	}
	else if (size < blk->size)
	{
		qse_size_t rem = blk->size - size;
		if (rem >= MINBLKLEN) 
		{
			qse_xma_blk_t* tmp;

			/* the leftover is large enough to hold a block
			 * of minimum size. split the current block. 
			 * let 'tmp' point to the leftover. */
			tmp = (qse_xma_blk_t*)(((qse_byte_t*)(blk + 1)) + size);
			tmp->avail = 1;
			tmp->size = rem - HDRSIZE;

			/* link 'tmp' to the block list */
			tmp->b.next = blk->b.next;
			tmp->b.prev = blk;
			if (blk->b.next) blk->b.next->b.prev = tmp;
			blk->b.next = tmp;
			blk->size = size;

			/* add 'tmp' to the free list */
			attach_to_freelist (xma, tmp);

/* TODO: if the next block is free. need to merge tmp with that..... */
/* TODO: if the next block is free. need to merge tmp with that..... */
/* TODO: if the next block is free. need to merge tmp with that..... */
/* TODO: if the next block is free. need to merge tmp with that..... */
/* TODO: if the next block is free. need to merge tmp with that..... */
/* TODO: if the next block is free. need to merge tmp with that..... */
/* TODO: if the next block is free. need to merge tmp with that..... */

#ifdef QSE_XMA_ENABLE_STAT
			xma->stat.nfree++;
			xma->stat.alloc -= rem;
			xma->stat.avail += tmp->size;
#endif
		}
	}

	return b;
}

void* qse_xma_realloc (qse_xma_t* xma, void* b, qse_size_t size)
{
	void* n;

	if (b == QSE_NULL) 
	{
		/* 'realloc' with NULL is the same as 'alloc' */
		n = qse_xma_alloc (xma, size);
	}
	else
	{
		/* try reallocation by merging the adjacent continuous blocks */
		n = _realloc_merge (xma, b, size);
		if (n == QSE_NULL)
		{
			/* reallocation by merging failed. fall back to the slow
			 * allocation-copy-free scheme */
			n = qse_xma_alloc (xma, size);
			if (n)
			{
				QSE_MEMCPY (n, b, size);
				qse_xma_free (xma, b);
			}
		}
	}

	return n;
}

void qse_xma_free (qse_xma_t* xma, void* b)
{
	qse_xma_blk_t* blk = USR_TO_SYS(b);

	/*QSE_ASSERT (blk->f.next == QSE_NULL);*/

#ifdef QSE_XMA_ENABLE_STAT
	/* update statistical variables */
	xma->stat.nused--;
	xma->stat.alloc -= blk->size;
#endif

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
		qse_size_t ns = HDRSIZE + blk->size + HDRSIZE;
		qse_size_t bs = ns + y->size;

		detach_from_freelist (xma, x);
		detach_from_freelist (xma, y);

		x->size += bs;
		x->b.next = z;
		if (z) z->b.prev = x;	

		attach_to_freelist (xma, x);

#ifdef QSE_XMA_ENABLE_STAT
		xma->stat.nfree--;
		xma->stat.avail += ns;
#endif
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

#ifdef QSE_XMA_ENABLE_STAT
		xma->stat.avail += blk->size + HDRSIZE;
#endif

		/* detach x from the free list */
		detach_from_freelist (xma, x);

		/* update the block availability */
		blk->avail = 1;
		/* update the block size. HDRSIZE for the header space in x */
		blk->size += HDRSIZE + x->size;

		/* update the backward link of Y */
		if (y) y->b.prev = blk;
		/* update the forward link of the block being freed */
		blk->b.next = y;

		/* attach blk to the free list */
		attach_to_freelist (xma, blk);

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

#ifdef QSE_XMA_ENABLE_STAT
		xma->stat.avail += HDRSIZE + blk->size;
#endif

		detach_from_freelist (xma, x);

		x->size += HDRSIZE + blk->size;
		x->b.next = y;
		if (y) y->b.prev = x;

		attach_to_freelist (xma, x);
	}
	else
	{
		blk->avail = 1;
		attach_to_freelist (xma, blk);

#ifdef QSE_XMA_ENABLE_STAT
		xma->stat.nfree++;
		xma->stat.avail += blk->size;
#endif
	}
}

void qse_xma_dump (qse_xma_t* xma)
{
	qse_xma_blk_t* tmp;
	unsigned long long fsum, asum; 
#ifdef QSE_XMA_ENABLE_STAT
	unsigned long long isum;
#endif

	qse_printf (QSE_T("<XMA DUMP>\n"));
#ifdef QSE_XMA_ENABLE_STAT
	qse_printf (QSE_T("== statistics ==\n"));
	qse_printf (QSE_T("total = %llu\n"), (unsigned long long)xma->stat.total);
	qse_printf (QSE_T("alloc = %llu\n"), (unsigned long long)xma->stat.alloc);
	qse_printf (QSE_T("avail = %llu\n"), (unsigned long long)xma->stat.avail);
#endif

	qse_printf (QSE_T("== blocks ==\n"));
	qse_printf (QSE_T(" size               avail address\n"));
	for (tmp = xma->head, fsum = 0, asum = 0; tmp; tmp = tmp->b.next)
	{
		qse_printf (QSE_T(" %-18llu %-5d %p\n"), (unsigned long long)tmp->size, tmp->avail, tmp);
		if (tmp->avail) fsum += tmp->size;
		else asum += tmp->size;
	}

#ifdef QSE_XMA_ENABLE_STAT
	isum = (xma->stat.nfree + xma->stat.nused) * HDRSIZE;
#endif

	qse_printf (QSE_T("---------------------------------------\n"));
	qse_printf (QSE_T("Allocated blocks: %18llu bytes\n"), asum);
	qse_printf (QSE_T("Available blocks: %18llu bytes\n"), fsum);
#ifdef QSE_XMA_ENABLE_STAT
	qse_printf (QSE_T("Internal use    : %18llu bytes\n"), isum);
	qse_printf (QSE_T("Total           : %18llu bytes\n"), asum + fsum + isum);
#endif
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
