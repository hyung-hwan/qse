/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_CMN_FMA_H_
#define _QSE_CMN_FMA_H_

/** @file
 * This file defines a fixed-size block memory allocator.
 * As the block size is known in advance, it achieves block allocation 
 * with little overhead.
 *
 * <pre>
 *  chunk head(cnkhead)   
 *   |
 *   |    +---------------------------------------------+
 *   +--> |     |   f1     |   f2      |      |         | chunk #2
 *        +--|---------^------|----^--------------------+
 *           |         |      |    |
 *           |         +------+    +---+    +--------------+  
 *           |                         |    |              |
 *           |       +-----------------|----V--------------|-------+
 *           +--->   |     |         |   f3    |         |  f4     | chunk #1
 *                   +---------------------------------------^-----+
 *                                                           |
 *                                           free block head (freeblk)
 * </pre>
 *
 * The diagram above assumes that f1, f2, f3, and f4 are free blocks. 
 * The chaining order depends on the allocation and deallocation order.
 *
 * See an example below. Note that it omits error handling.
 *
 * @code
 * #include <qse/cmn/fma.h>
 * int main ()
 * {
 *   qse_fma_t* fma;
 *   int* ptr1, * ptr2;
 *
 *   // create a memory allocator for integer blocks up to 50.
 *   fma = qse_fma_open (QSE_NULL, 0, sizeof(int), 10, 5);
 *
 *   // allocate two integer blocks
 *   ptr1 = (int*) qse_fma_alloc (fma, sizeof(int));
 *   ptr2 = (int*) qse_fma_alloc (fma, sizeof(int));
 *
 *   *ptr1 = 20; *ptr2 = 99;
 *
 *   // free the two blocks.
 *   qse_fma_free (fma, ptr1);
 *   qse_fma_free (fma, ptr2);
 *
 *   // destroy the memory allocator
 *   qse_fma_close (fma);
 * }
 * @endcode
 *
 * The following example shows how to use the fixed-size block
 * allocator for a dynamic data structure allocating fixed-size nodes.
 *
 * @code
 * #include <qse/cmn/fma.h>
 * #include <qse/cmn/rbt.h>
 * #include <qse/cmn/mem.h>
 * #include <qse/cmn/stdio.h>
 *
 * static qse_rbt_walk_t walk (qse_rbt_t* rbt, qse_rbt_pair_t* pair, void* ctx)
 * {
 *   qse_printf (QSE_T("key = %lld, value = %lld\n"),
 *       *(long*)QSE_RBT_KPTR(pair), *(long*)QSE_RBT_VPTR(pair));
 *   return QSE_RBT_WALK_FORWARD;
 * }
 *
 * int main ()
 * {
 *   qse_fma_t* fma;
 *   qse_rbt_t rbt; 
 *   qse_size_t blksize;
 *   long x;
 *
 *   // prepare the fixed-size block allocator into the qse_mmgr_t interface
 *   qse_mmgr_t mmgr = 
 *   {
 *      (qse_mmgr_alloc_t) qse_fma_alloc,
 *      (qse_mmgr_realloc_t) qse_fma_realloc,
 *      (qse_mmgr_free_t) qse_fma_free,
 *      QSE_NULL
 *   };
 *
 *   // the block size of a red-black tree is fixed to be:
 *	//   key size + value size + internal node size.
 *   blksize = sizeof(long) + sizeof(long) + sizeof(qse_rbt_pair_t);
 *
 *   // create a fixed-size block allocator which is created
 *   // with the default memory allocator.
 *   fma = qse_fma_open (QSE_MMGR_GETDFL(), 0, blksize, 10, 0);
 *   if (fma == QSE_NULL)
 *   {
 *      qse_printf (QSE_T("cannot open a memory allocator\n"));
 *      return -1;
 *   }
 *
 *   // complete the qse_mmgr_t interface by providing the allocator.
 *   mmgr.ctx = fma;
 *
 *   // initializes the statically declared red-black tree.
 *   // can not call qse_rbt_open() which allocates the qse_rbt_t object.
 *   // as its size differs from the block size calculated above. 
 *   if (qse_rbt_init (&rbt, &mmgr) == QSE_NULL)
 *   {
 *      qse_printf (QSE_T("cannot initialize a tree\n"));
 *      qse_fma_close (fma);
 *      return -1;
 *   }
 *
 *   // perform more initializations for keys and values.
 *   qse_rbt_setcopier (&rbt, QSE_RBT_KEY, QSE_RBT_COPIER_INLINE);
 *   qse_rbt_setcopier (&rbt, QSE_RBT_VAL, QSE_RBT_COPIER_INLINE);
 *   qse_rbt_setscale (&rbt, QSE_RBT_KEY, QSE_SIZEOF(long));
 *   qse_rbt_setscale (&rbt, QSE_RBT_VAL, QSE_SIZEOF(long));
 *
 *   // insert numbers into the red-black tree
 *   for (x = 10; x < 100; x++)
 *   {
 *      long y = x * x;
 *      if (qse_rbt_insert (&rbt, &x, 1, &y, 1) == QSE_NULL) 
 *      {
 *         qse_printf (QSE_T("failed to insert. out of memory\n"));
 *         break;
 *      }
 *   }
 *
 *   // print the tree contents 
 *   qse_rbt_walk (&rbt, walk, QSE_NULL);
 *
 *   // finalize the tree.
 *   qse_rbt_fini (&rbt);
 *
 *   // destroy the memory allocator.
 *   qse_fma_close (fma);
 *
 *   return 0;
 * }
 * @endcode
 *
 * Use #qse_xma_t for variable-size block allocation.
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
 */
typedef struct qse_fma_t qse_fma_t;
struct qse_fma_t
{
	qse_mmgr_t* mmgr; /**< memory manager */

	qse_size_t blksize; /**< block size */
	qse_size_t maxblks; /**< maximum blocks in a chunk */
	qse_size_t maxcnks; /**< maximum chunks */

	qse_size_t     numcnks; /**< current numbers of chunks */
	qse_fma_cnk_t* cnkhead; /**< point to the first chunk added */
	qse_fma_blk_t* freeblk; /**< point to the first free block */
};

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_fma_open() function creates a memory allocator with an outer
 * memory manager.
 */
QSE_EXPORT qse_fma_t* qse_fma_open (
	qse_mmgr_t* mmgr,   /**< outer memory manager */
	qse_size_t xtnsize, /**< extension size in bytes */
	qse_size_t blksize, /**< fixed block size in bytes */
	qse_size_t maxblks, /**< maximum numbers of blocks in a chunk */
	qse_size_t maxcnks  /**< maximum numbers of chunks. 0 for no limit */
);

/**
 * The qse_fma_close() function destroys an memory allocator.
 */
QSE_EXPORT void qse_fma_close (
	qse_fma_t* fma      /**< memory allocator */
);

/**
 * The qse_fma_init() function initializes an memory allocator statically 
 * declared.
 */
QSE_EXPORT int qse_fma_init (
	qse_fma_t* fma,     /**< memory allocator */
	qse_mmgr_t* mmgr,   /**< outer memory manager */
	qse_size_t blksize, /**< fixed block size in bytes */
	qse_size_t maxblks, /**< maximum numbers of blocks in a chunk */
	qse_size_t maxcnks  /**< maximum numbers of chunks. 0 for no limit */
);

/**
 * The qse_fma_fini() function finalizes an memory allocator.
 */
QSE_EXPORT void qse_fma_fini (
	qse_fma_t* fma      /**< memory allocator */
);

QSE_EXPORT qse_mmgr_t* qse_fma_getmmgr (
	qse_fma_t* fma
);

QSE_EXPORT void* qse_fma_getxtn (
	qse_fma_t* fma
);

/**
 * The qse_fma_alloc() function allocates a block of the fixed block size
 * specified during initialization regardless of the block size @a size 
 * requested so long as it is not greater than the fixed size. The function
 * fails if it is greater.
 * 
 * @return block pointer on success, #QSE_NULL on failure
 */
QSE_EXPORT void* qse_fma_alloc (
	qse_fma_t* fma,     /**< memory allocator */
	qse_size_t size     /**< block size in bytes*/
);

QSE_EXPORT void* qse_fma_calloc (
	qse_fma_t* fma,
	qse_size_t size
);

/**
 * The qse_fma_realloc() function is provided for consistency with other 
 * generic memory allocator which provides a reallocation function.
 * Block resizing is meaningless for #qse_fma_t as it deals with fixed-size
 * blocks. 
 *
 * If the @a size requested is greater than the fixed block size of the memory
 * allocator @a fma, the function fails; If the block @a blk is #QSE_NULL and 
 * the @a size requested is not greater than the fixed block size of the memory
 * allocator @a fma, it allocates a block of the fixed size; If the block 
 * @a blk is not #QSE_NULL and the @a size requested is not greater than the
 * fixed block size of the memory allocator @a fma, it returns the block @a blk.
 *
 * @return block pointer on success, #QSE_NULL on failure
 */
QSE_EXPORT void* qse_fma_realloc (
	qse_fma_t* fma,     /**< memory allocator */
	void*      blk,     /**< memory block */
	qse_size_t size     /**< block size in bytes*/
);

/**
 * The qse_fma_free() function deallocates a block.
 */
QSE_EXPORT void qse_fma_free (
	qse_fma_t* fma,     /**< memory allocator */
	void*      blk      /**< memory block to free */
);

#if defined(__cplusplus)
}
#endif

#endif
