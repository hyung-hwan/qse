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

/*
 tre-mem.h - TRE memory allocator interface

 This software is released under a BSD-style license.
 See the file LICENSE for details and copyright.

 */

#ifndef _QSE_CMN_PMA_H_
#define _QSE_CMN_PMA_H_

/** @file
 * This file defines a pool-based block allocator.
 */

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_PMA_BLOCK_SIZE 1024

typedef struct qse_pma_blk_t qse_pma_blk_t;

struct qse_pma_blk_t
{
	void *data;
	qse_pma_blk_t* next;
};

/**
 * The qse_pma_t type defines a pool-based block allocator. You can allocate
 * blocks of memories but you can't resize or free individual blocks allocated.
 * Instead, you can destroy the whole pool once you're done with all the
 * blocks allocated.
 */
typedef struct qse_pma_t qse_pma_t;

struct qse_pma_t
{
	qse_mmgr_t* mmgr;

	qse_pma_blk_t* blocks;
	qse_pma_blk_t* current;

	char *ptr;
	qse_size_t n;
	int failed;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_pma_open() function creates a pool-based memory allocator.
 */
QSE_EXPORT qse_pma_t* qse_pma_open (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize  /**< extension size in bytes */
);

/**
 * The qse_pma_close() function destroys a pool-based memory allocator.
 */
QSE_EXPORT void qse_pma_close (
	qse_pma_t* pma /**< memory allocator */
);

QSE_EXPORT int qse_pma_init (
	qse_pma_t*  pma, /**< memory allocator */
	qse_mmgr_t* mmgr /**< memory manager */
);

QSE_EXPORT void qse_pma_fini (
	qse_pma_t* pma /**< memory allocator */
);

QSE_EXPORT qse_mmgr_t* qse_pma_getmmgr (
	qse_pma_t* pma
);

QSE_EXPORT void* qse_pma_getxtn (
	qse_pma_t* pma
);

/** 
 * The qse_pma_clear() function frees all the allocated memory blocks 
 * by freeing the entire memory pool. 
 */
QSE_EXPORT void qse_pma_clear (
	qse_pma_t* pma /**< memory allocator */
);

/**
 * The qse_pma_alloc() function allocates a memory block of the @a size bytes.
 * @return pointer to a allocated block on success, #QSE_NULL on failure.
 */
QSE_EXPORT void* qse_pma_alloc (
	qse_pma_t* pma, /**< memory allocator */
	qse_size_t size /**< block size */
);	

/**
 * The qse_pma_alloc() function allocates a memory block of the @a size bytes
 * and initializes the whole block with 0.
 * @return pointer to a allocated block on success, #QSE_NULL on failure.
 */
QSE_EXPORT void* qse_pma_calloc (
	qse_pma_t* pma, /**< memory allocator */
	qse_size_t size /**< block size */
);	

/**
 * The qse_pma_free() function is provided for completeness, and doesn't
 * resize an individual block @a blk. 
 */
QSE_EXPORT void* qse_pma_realloc (
	qse_pma_t* pma,  /**< memory allocator */
	void*      blk,  /**< memory block */
	qse_size_t size  /**< new size in bytes */
);

/**
 * The qse_pma_free() function is provided for completeness, and doesn't
 * free an individual block @a blk. 
 */
QSE_EXPORT void qse_pma_free (
	qse_pma_t* pma, /**< memory allocator */
	void*      blk  /**< memory block */
);

#endif
