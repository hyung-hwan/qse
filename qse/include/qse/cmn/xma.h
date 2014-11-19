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

#ifndef _QSE_CMN_XMA_H_
#define _QSE_CMN_XMA_H_

/** @file
 * This file defines an extravagant memory allocator. Why? It may be so.
 * The memory allocator allows you to maintain memory blocks from a
 * larger memory chunk allocated with an outer memory allocator.
 * Typically, an outer memory allocator is a standard memory allocator
 * like malloc(). You can isolate memory blocks into a particular chunk.
 *
 * See the example below. Note it omits error handling.
 *
 * @code
 * #include <qse/cmn/xma.h>
 * #include <qse/cmn/stdio.h>
 * int main ()
 * {
 *   qse_xma_t* xma;
 *   void* ptr1, * ptr2;
 *
 *   // create a new memory allocator obtaining a 100K byte zone 
 *   // with the default memory allocator
 *   xma = qse_xma_open (QSE_NULL, 0, 100000L); 
 *
 *   ptr1 = qse_xma_alloc (xma, 5000); // allocate a 5K block from the zone
 *   ptr2 = qse_xma_alloc (xma, 1000); // allocate a 1K block from the zone
 *   ptr1 = qse_xma_realloc (xma, ptr1, 6000); // resize the 5K block to 6K.
 *
 *   qse_xma_dump (xma, qse_fprintf, QSE_STDOUT); // dump memory blocks 
 *
 *   // the following two lines are not actually needed as the allocator
 *   // is closed after them.
 *   qse_xma_free (xma, ptr2); // dispose of the 1K block
 *   qse_xma_free (xma, ptr1); // dispose of the 6K block
 *
 *   qse_xma_close (xma); //  destroy the memory allocator
 *   return 0;
 * }
 * @endcode
 */
#include <qse/types.h>
#include <qse/macros.h>

#ifdef QSE_BUILD_DEBUG
#	define QSE_XMA_ENABLE_STAT
#endif

/** @struct qse_xma_t
 * The qse_xma_t type defines a simple memory allocator over a memory zone.
 * It can obtain a relatively large zone of memory and manage it.
 */
typedef struct qse_xma_t qse_xma_t;

/**
 * The qse_xma_blk_t type defines a memory block allocated.
 */
typedef struct qse_xma_blk_t qse_xma_blk_t;

#define QSE_XMA_FIXED 32
#define QSE_XMA_SIZE_BITS ((QSE_SIZEOF_SIZE_T*8)-1)

struct qse_xma_t
{
	qse_mmgr_t* mmgr;

	/** pointer to the first memory block */
	qse_xma_blk_t* head; 

	/** pointer array to free memory blocks */
	qse_xma_blk_t* xfree[QSE_XMA_FIXED + QSE_XMA_SIZE_BITS + 1]; 

	/** pre-computed value for fast xfree index calculation */
	qse_size_t     bdec;

#ifdef QSE_XMA_ENABLE_STAT
	struct
	{
		qse_size_t total;
		qse_size_t alloc;
		qse_size_t avail;
		qse_size_t nused;
		qse_size_t nfree;
	} stat;
#endif
};

/**
 * The qse_xma_dumper_t type defines a printf-like output function
 * for qse_xma_dump().
 */
typedef int (*qse_xma_dumper_t) (
	void*             ctx,
	const qse_char_t* fmt,
	...
);

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_xma_open() function creates a memory allocator. It obtains a memory
 * zone of the @a zonesize bytes with the memory manager @a mmgr. It also makes
 * available the extension area of the @a xtnsize bytes that you can get the
 * pointer to with qse_xma_getxtn().
 *
 * @return pointer to a memory allocator on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_xma_t* qse_xma_open (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize, /**< extension size in bytes */
	qse_size_t  zonesize /**< zone size in bytes */
);

/**
 * The qse_xma_close() function destroys a memory allocator. It also frees
 * the memory zone obtained, which invalidates the memory blocks within 
 * the zone. Call this function to destroy a memory allocator created with
 * qse_xma_open().
 */
QSE_EXPORT void qse_xma_close (
	qse_xma_t* xma /**< memory allocator */
);

QSE_EXPORT qse_mmgr_t* qse_xma_getmmgr (
	qse_xma_t* xma
);

QSE_EXPORT void* qse_xma_getxtn (
	qse_xma_t* xma
);

/**
 * The qse_xma_init() initializes a memory allocator. If you have the qse_xma_t
 * structure statically declared or already allocated, you may pass the pointer
 * to this function instead of calling qse_xma_open(). It obtains a memory zone
 * of @a zonesize bytes with the memory manager @a mmgr. Unlike qse_xma_open(),
 * it does not accept the extension size, thus not creating an extention area.
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_xma_init (
	qse_xma_t*  xma,     /**< memory allocator */
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  zonesize /**< zone size in bytes */
);

/**
 * The qse_xma_fini() function finalizes a memory allocator. Call this 
 * function to finalize a memory allocator initialized with qse_xma_init().
 */
QSE_EXPORT void qse_xma_fini (
	qse_xma_t* xma /**< memory allocator */
);

/**
 * The qse_xma_alloc() function allocates @a size bytes.
 * @return pointer to a memory block on success, #QSE_NULL on failure
 */
QSE_EXPORT void* qse_xma_alloc (
	qse_xma_t* xma, /**< memory allocator */
	qse_size_t size /**< size in bytes */
);

QSE_EXPORT void* qse_xma_calloc (
	qse_xma_t* xma,
	qse_size_t size
);

/**
 * The qse_xma_alloc() function resizes the memory block @a b to @a size bytes.
 * @return pointer to a resized memory block on success, #QSE_NULL on failure
 */
QSE_EXPORT void* qse_xma_realloc (
	qse_xma_t* xma,  /**< memory allocator */
	void*      b,    /**< memory block */
	qse_size_t size  /**< new size in bytes */
);

/**
 * The qse_xma_alloc() function frees the memory block @a b.
 */
QSE_EXPORT void qse_xma_free (
	qse_xma_t* xma, /**< memory allocator */
	void*      b    /**< memory block */
);

/**
 * The qse_xma_dump() function dumps the contents of the memory zone
 * with the output function @a dumper provided. The debug build shows
 * more statistical counters.
 */
QSE_EXPORT void qse_xma_dump (
	qse_xma_t*       xma,    /**< memory allocator */
	qse_xma_dumper_t dumper, /**< output function */
	void*            ctx     /**< first parameter to output function */
);

#if defined(__cplusplus)
}
#endif

#endif
