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

#ifndef _QSE_CMN_FMA_H_
#define _QSE_CMN_FMA_H_

/** @file
 * This file defines a fixed-size block memory allocator. As the block size
 * is known in advance, it achieves block allocation with little overhead.
 *
 * <pre>
 *  chunk head(cnkhead)   
 *   |                                              chunk
 *   |    +---------------------------------------------+
 *   +--> |     |   f1     |   f2      |      |         |
 *        +--|---------^------|----^--------------------+
 *           |         |      |    |
 *           |         +------+    +---+    +--------------+   chunk
 *           |                         |    |              |
 *           |       +-----------------|----V--------------|-------+
 *           +--->   |     |         |   f3    |         |  f4     |
 *                   +---------------------------------------^-----+
 *                                                           |
 *                                           free block head (freeblk)
 * </pre>
 *
 * The diagram above assumes that f1, f2, f3, and f4 are free blocks. 
 * The chaining order depends on the allocation and deallocation order.
 *
 * See #qse_fma_t for more information. Use #qse_xma_t for variable-size block
 * allocation.
 */

#include <qse/types.h>
#include <qse/macros.h>

/** @struct qse_fma_cnk_t
 * The qse_fma_cnk_t type defines a memory chunk header to hold memory blocks.
 * The chunks added are maintained in a singly-linked list
 */
typedef struct qse_fma_cnk_t qse_fma_cnk_t;
struct qse_fma_cnk_t
{
	qse_fma_cnk_t* next; /**< point to the next chunk */
};

/** @struct qse_fma_blk_t
 * The qse_fma_blk_t type defines a memory block header to weave free blocks
 * into a singly-linked list.
 */
typedef struct qse_fma_blk_t qse_fma_blk_t;
struct qse_fma_blk_t
{
	qse_fma_blk_t* next; /**< point to the next block */
};

/** @struct qse_fma_t
 * The qse_fma_t type defines a fixed-size block memory allocator.
 * See the example below. Note that it omits error handling.
 * @code
 *  qse_fma_t* fma;
 *  int* ptr1, * ptr2;
 *
 *  // create a memory allocator for integer blocks up to 50.
 *  fma = qse_fma_open (QSE_NULL, 0, sizeof(int), 10, 5);
 *
 *  // allocate two integer blocks
 *  ptr1 = (int*) qse_fma_alloc (fma);
 *  ptr2 = (int*) qse_fma_alloc (fma);
 *
 *  *ptr1 = 20; *ptr2 = 99;
 *
 *  // free the two blocks.
 *  qse_fma_free (fma, ptr1);
 *  qse_fma_free (fma, ptr2);
 *
 *  // destroy the memory allocator
 *  qse_fma_close (fma);
 * @endcode
 */
typedef struct qse_fma_t qse_fma_t;
struct qse_fma_t
{
	QSE_DEFINE_COMMON_FIELDS (fma)

	qse_size_t blksize; /**< block size */
	qse_size_t maxblks; /**< maximum blocks in a chunk */
	qse_size_t maxcnks; /**< maximum chunks */

	qse_size_t     numcnks; /**< current numbers of chunks */
	qse_fma_cnk_t* cnkhead; /**< point to the first chunk added */
	qse_fma_blk_t* freeblk; /**< point to the first free block */
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (fma)

/**
 * The qse_fma_open() function creates a memory allocator with an outer
 * memory manager.
 */
qse_fma_t* qse_fma_open (
	qse_mmgr_t* mmgr,   /**< outer memory manager */
	qse_size_t xtnsize, /**< extension size in bytes */
     qse_size_t blksize, /**< block size in bytes */
	qse_size_t maxblks, /**< maximum numbers of blocks in a chunk */
	qse_size_t maxcnks  /**< maximum numbers of chunks */
);

/**
 * The qse_fma_close() function destroys an memory allocator.
 */
void qse_fma_close (
	qse_fma_t* fma      /**< memory allocator */
);

/**
 * The qse_fma_init() function initializes an memory allocator.
 */
qse_fma_t* qse_fma_init (
     qse_fma_t* fma,     /**< memory allocator */
	qse_mmgr_t* mmgr,   /**< outer memory manager */
     qse_size_t blksize, /**< block size in bytes */
	qse_size_t maxblks, /**< maximum numbers of blocks in a chunk */
	qse_size_t maxcnks  /**< maximum numbers of chunks */
);

/**
 * The qse_fma_fini() function finalizes an memory allocator.
 */
void qse_fma_fini (
	qse_fma_t* fma      /**< memory allocator */
);

/**
 * The qse_fma_alloc() function allocates a block.
 * @return block pointer on success, QSE_NULL on failure
 */
void* qse_fma_alloc (
	qse_fma_t* fma      /**< memory allocator */
);

/**
 * The qse_fma_alloc() function deallocates a block.
 */
void qse_fma_free (
	qse_fma_t* fma,     /**< memory allocator */
	void*      blk      /**< memory block to free */
);

#ifdef __cplusplus
}
#endif

#endif
