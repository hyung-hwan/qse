/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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
		qse_xma_blk_t* prev; /**< link to the previous adjacent block */
		qse_xma_blk_t* next; /**< link to the next adjacent block */
	} b;
};

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

#if QSE_SIZEOF_SIZE_T >= 128
#	error qse_size_t too large. unsupported platform
#endif

#if QSE_SIZEOF_SIZE_T >= 64
	if ((n & (~(qse_size_t)0 << (BITS-128))) == 0) { x -= 256; n <<= 256; }
#endif
#if QSE_SIZEOF_SIZE_T >= 32
	if ((n & (~(qse_size_t)0 << (BITS-128))) == 0) { x -= 128; n <<= 128; }
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

qse_xma_t* qse_xma_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_size_t zonesize)
{
	qse_xma_t* xma;

	xma = (qse_xma_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*xma) + xtnsize);
	if (xma == QSE_NULL) return QSE_NULL;

	if (qse_xma_init (xma, mmgr, zonesize) <= -1)
	{
		QSE_MMGR_FREE (mmgr, xma);
		return QSE_NULL;
	}

	QSE_MEMSET (QSE_XTN(xma), 0, xtnsize);
	return xma;
}

void qse_xma_close (qse_xma_t* xma)
{
	qse_xma_fini (xma);
	QSE_MMGR_FREE (xma->mmgr, xma);
}

int qse_xma_init (qse_xma_t* xma, qse_mmgr_t* mmgr, qse_size_t zonesize)
{
	qse_xma_blk_t* free;
	qse_size_t xfi;

	/* round 'zonesize' to be the multiples of ALIGN */
	zonesize = ((zonesize + ALIGN - 1) / ALIGN) * ALIGN;

	/* adjust 'zonesize' to be large enough to hold a single smallest block */
	if (zonesize < MINBLKLEN) zonesize = MINBLKLEN;

	/* allocate a memory chunk to use for actual memory allocation */
	free = QSE_MMGR_ALLOC (mmgr, zonesize);
	if (free == QSE_NULL) return -1;
	
	/* initialize the header part of the free chunk */
	free->avail = 1;
	free->size = zonesize - HDRSIZE; /* size excluding the block header */
	free->f.prev = QSE_NULL;
	free->f.next = QSE_NULL;
	free->b.next = QSE_NULL;
	free->b.prev = QSE_NULL;

	QSE_MEMSET (xma, 0, QSE_SIZEOF(*xma));
	xma->mmgr = mmgr;
	xma->bdec = szlog2(FIXED * ALIGN); /* precalculate the decrement value */

	/* at this point, the 'free' chunk is a only block available */

	/* get the free block index */
	xfi = getxfi(xma,free->size);
	/* locate it into an apporopriate slot */
	xma->xfree[xfi] = free; 
	/* let it be the head, which is natural with only a block */
	xma->head = free;

	/* initialize some statistical variables */
#if defined(QSE_XMA_ENABLE_STAT)
	xma->stat.total = zonesize;
	xma->stat.alloc = 0;
	xma->stat.avail = zonesize - HDRSIZE;
	xma->stat.nfree = 1;
	xma->stat.nused = 0;
#endif
	
	return 0;
}

void qse_xma_fini (qse_xma_t* xma)
{
	/* the head must point to the free chunk allocated in init().
	 * let's deallocate it */
	QSE_MMGR_FREE (xma->mmgr, xma->head);
}

qse_mmgr_t* qse_xma_getmmgr (qse_xma_t* xma)
{
	return xma->mmgr;
}

void* qse_xma_getxtn (qse_xma_t* xma)
{
	return QSE_XTN (xma);
}

static QSE_INLINE void attach_to_freelist (qse_xma_t* xma, qse_xma_blk_t* b)
{
	/* 
	 * attach a block to a free list 
	 */

	/* get the free list index for the block size */
	qse_size_t xfi = getxfi(xma,b->size); 

	/* let it be the head of the free list doubly-linked */
	b->f.prev = QSE_NULL; 
	b->f.next = xma->xfree[xfi];
	if (xma->xfree[xfi]) xma->xfree[xfi]->f.prev = b;
	xma->xfree[xfi] = b;		
}

static QSE_INLINE void detach_from_freelist (qse_xma_t* xma, qse_xma_blk_t* b)
{
	/*
 	 * detach a block from a free list 
 	 */
	qse_xma_blk_t* p, * n;

	/* alias the previous and the next with short variable names */
	p = b->f.prev;
	n = b->f.next;

	if (p)
	{
		/* the previous item exists. let its 'next' pointer point to 
		 * the block's next item. */
		p->f.next = n;
	}
	else 
	{
		/* the previous item does not exist. the block is the first
 		 * item in the free list. */

		qse_size_t xfi = getxfi(xma,b->size);
		QSE_ASSERT (b == xma->xfree[xfi]);
		/* let's update the free list head */
		xma->xfree[xfi] = n;
	}

	/* let the 'prev' pointer of the block's next item point to the 
	 * block's previous item */
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
			qse_size_t rem;

			detach_from_freelist (xma, free);

			rem = free->size - size;
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

#if defined(QSE_XMA_ENABLE_STAT)
				xma->stat.avail -= HDRSIZE;
#endif
			}
#if defined(QSE_XMA_ENABLE_STAT)
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

#if defined(QSE_XMA_ENABLE_STAT)
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

#if defined(QSE_XMA_ENABLE_STAT)
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
				if (free) break;
			}
			if (free == QSE_NULL) return QSE_NULL;
		}
	}

	return SYS_TO_USR(free);
}

static void* _realloc_merge (qse_xma_t* xma, void* b, qse_size_t size)
{
	qse_xma_blk_t* blk = USR_TO_SYS(b);

	/* rounds up 'size' to be multiples of ALIGN */ 
	size = ((size + ALIGN - 1) / ALIGN) * ALIGN;

	if (size > blk->size)
	{
		/* 
		 * grow the current block
		 */
		qse_size_t req;
		qse_xma_blk_t* n;
		qse_size_t rem;
		
		req = size - blk->size;

		n = blk->b.next;

		/* check if the next adjacent block is available */
		if (!n || !n->avail || req > n->size) return QSE_NULL; /* no! */

		/* let's merge the current block with the next block */
		detach_from_freelist (xma, n);

		rem = (HDRSIZE + n->size) - req;
		if (rem >= MINBLKLEN)
		{
			/* 	
			 * the remaining part of the next block is large enough 
			 * to hold a block. break the next block.
			 */

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

#if defined(QSE_XMA_ENABLE_STAT)
			xma->stat.alloc += req;
			xma->stat.avail -= req; /* req + HDRSIZE(tmp) - HDRSIZE(n) */
#endif
		}
		else
		{
			/* the remaining part of the next block is negligible.
			 * utilize the whole block by merging to the resizing block */
			blk->size += HDRSIZE + n->size;
			blk->b.next = n->b.next;
			if (n->b.next) n->b.next->b.prev = blk;

#if defined(QSE_XMA_ENABLE_STAT)
			xma->stat.nfree--;
			xma->stat.alloc += HDRSIZE + n->size;
			xma->stat.avail -= n->size;
#endif
		}
	}
	else if (size < blk->size)
	{
		/* 
		 * shrink the block 
		 */

		qse_size_t rem = blk->size - size;
		if (rem >= MINBLKLEN) 
		{
			qse_xma_blk_t* tmp;
			qse_xma_blk_t* n = blk->b.next;

			/* the leftover is large enough to hold a block
			 * of minimum size. split the current block. 
			 * let 'tmp' point to the leftover. */
			tmp = (qse_xma_blk_t*)(((qse_byte_t*)(blk + 1)) + size);
			tmp->avail = 1;

			if (n && n->avail)
			{
				/* merge with the next block */
				detach_from_freelist (xma, n);

				tmp->b.next = n->b.next;
				tmp->b.prev = blk;
				if (n->b.next) n->b.next->b.prev = tmp;
				blk->b.next = tmp;
				blk->size = size;

				tmp->size = rem - HDRSIZE + HDRSIZE + n->size;

#if defined(QSE_XMA_ENABLE_STAT)
				xma->stat.alloc -= rem;
				/* rem - HDRSIZE(tmp) + HDRSIZE(n) */
				xma->stat.avail += rem;
#endif
			}
			else
			{
				/* link 'tmp' to the block list */
				tmp->b.next = n;
				tmp->b.prev = blk;
				if (n) n->b.prev = tmp;
				blk->b.next = tmp;
				blk->size = size;

				tmp->size = rem - HDRSIZE;

#if defined(QSE_XMA_ENABLE_STAT)
				xma->stat.nfree++;
				xma->stat.alloc -= rem;
				xma->stat.avail += tmp->size;
#endif
			}

			/* add 'tmp' to the free list */
			attach_to_freelist (xma, tmp);
		}
	}

	return b;
}

void* qse_xma_calloc (qse_xma_t* xma, qse_size_t size)
{
	void* ptr = qse_xma_alloc (xma, size);
	if (ptr) QSE_MEMSET (ptr, 0, size);
	return ptr;
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

#if defined(QSE_XMA_ENABLE_STAT)
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

#if defined(QSE_XMA_ENABLE_STAT)
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

#if defined(QSE_XMA_ENABLE_STAT)
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

#if defined(QSE_XMA_ENABLE_STAT)
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

#if defined(QSE_XMA_ENABLE_STAT)
		xma->stat.nfree++;
		xma->stat.avail += blk->size;
#endif
	}
}

void qse_xma_dump (qse_xma_t* xma, qse_xma_dumper_t dumper, void* ctx)
{
	qse_xma_blk_t* tmp;
	qse_ulong_t fsum, asum; 
#if defined(QSE_XMA_ENABLE_STAT)
	qse_ulong_t isum;
#endif

	dumper (ctx, QSE_T("<XMA DUMP>\n"));

#if defined(QSE_XMA_ENABLE_STAT)
	dumper (ctx, QSE_T("== statistics ==\n"));
#if (QSE_SIZEOF_SIZE_T == QSE_SIZEOF_LONG)
	dumper (ctx, QSE_T("total = %lu\n"), (unsigned long)xma->stat.total);
	dumper (ctx, QSE_T("alloc = %lu\n"), (unsigned long)xma->stat.alloc);
	dumper (ctx, QSE_T("avail = %lu\n"), (unsigned long)xma->stat.avail);
#elif (QSE_SIZEOF_SIZE_T == QSE_SIZEOF_LONG_LONG)
	dumper (ctx, QSE_T("total = %llu\n"), (unsigned long long)xma->stat.total);
	dumper (ctx, QSE_T("alloc = %llu\n"), (unsigned long long)xma->stat.alloc);
	dumper (ctx, QSE_T("avail = %llu\n"), (unsigned long long)xma->stat.avail);
#elif (QSE_SIZEOF_SIZE_T == QSE_SIZEOF_INT)
	dumper (ctx, QSE_T("total = %u\n"), (unsigned int)xma->stat.total);
	dumper (ctx, QSE_T("alloc = %u\n"), (unsigned int)xma->stat.alloc);
	dumper (ctx, QSE_T("avail = %u\n"), (unsigned int)xma->stat.avail);
#else
#	error weird size of qse_size_t. unsupported platform
#endif
#endif

	dumper (ctx, QSE_T("== blocks ==\n"));
	dumper (ctx, QSE_T(" size               avail address\n"));
	for (tmp = xma->head, fsum = 0, asum = 0; tmp; tmp = tmp->b.next)
	{
#if (QSE_SIZEOF_SIZE_T == QSE_SIZEOF_LONG)
		dumper (ctx, QSE_T(" %-18lu %-5u %p\n"), 
			(unsigned long)tmp->size, (unsigned int)tmp->avail, tmp
		);
#elif (QSE_SIZEOF_SIZE_T == QSE_SIZEOF_LONG_LONG)
		dumper (ctx, QSE_T(" %-18llu %-5u %p\n"), 
			(unsigned long long)tmp->size, (unsigned int)tmp->avail, tmp
		);
#elif (QSE_SIZEOF_SIZE_T == QSE_SIZEOF_INT)
		dumper (ctx, QSE_T(" %-18u %-5u %p\n"), 
			(unsigned int)tmp->size, (unsigned int)tmp->avail, tmp
		);
#else
#	error weird size of qse_size_t. unsupported platform
#endif
		if (tmp->avail) fsum += tmp->size;
		else asum += tmp->size;
	}

#if defined(QSE_XMA_ENABLE_STAT)
	isum = (xma->stat.nfree + xma->stat.nused) * HDRSIZE;
#endif

	dumper (ctx, QSE_T("---------------------------------------\n"));
#if (QSE_SIZEOF_ULONG_T == QSE_SIZEOF_LONG)
	dumper (ctx, QSE_T("Allocated blocks: %18lu bytes\n"), (unsigned long)asum);
	dumper (ctx, QSE_T("Available blocks: %18lu bytes\n"), (unsigned long)fsum);
#elif (QSE_SIZEOF_ULONG_T == QSE_SIZEOF_LONG_LONG)
	dumper (ctx, QSE_T("Allocated blocks: %18llu bytes\n"), (unsigned long long)asum);
	dumper (ctx, QSE_T("Available blocks: %18llu bytes\n"), (unsigned long long)fsum);
#elif (QSE_SIZEOF_ULONG_T == QSE_SIZEOF_INT)
	dumper (ctx, QSE_T("Allocated blocks: %18u bytes\n"), (unsigned int)asum);
	dumper (ctx, QSE_T("Available blocks: %18u bytes\n"), (unsigned int)fsum);
#else
#	error weird size of qse_ulong_t. unsupported platform
#endif

#if defined(QSE_XMA_ENABLE_STAT)
#if (QSE_SIZEOF_ULONG_T == QSE_SIZEOF_LONG)
	dumper (ctx, QSE_T("Internal use    : %18lu bytes\n"), (unsigned long)isum);
	dumper (ctx, QSE_T("Total           : %18lu bytes\n"), (unsigned long)(asum + fsum + isum));
#elif (QSE_SIZEOF_ULONG_T == QSE_SIZEOF_LONG_LONG)
	dumper (ctx, QSE_T("Internal use    : %18llu bytes\n"), (unsigned long long)isum);
	dumper (ctx, QSE_T("Total           : %18llu bytes\n"), (unsigned long long)(asum + fsum + isum));
#elif (QSE_SIZEOF_ULONG_T == QSE_SIZEOF_INT)
	dumper (ctx, QSE_T("Internal use    : %18u bytes\n"), (unsigned int)isum);
	dumper (ctx, QSE_T("Total           : %18u bytes\n"), (unsigned int)(asum + fsum + isum));
#else
#	error weird size of qse_ulong_t. unsupported platform
#endif
#endif

#if defined(QSE_XMA_ENABLE_STAT)
	QSE_ASSERT (asum == xma->stat.alloc);
	QSE_ASSERT (fsum == xma->stat.avail);
	QSE_ASSERT (isum == xma->stat.total - (xma->stat.alloc + xma->stat.avail));
	QSE_ASSERT (asum + fsum + isum == xma->stat.total);
#endif
}

