/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
 * The qse_pma_t type defines a pool-base block allocator.
 */
typedef struct qse_pma_t qse_pma_t;

struct qse_pma_t
{
	QSE_DEFINE_COMMON_FIELDS (pma)

	qse_pma_blk_t* blocks;
	qse_pma_blk_t* current;

	char *ptr;
	qse_size_t n;
	int failed;
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (pma)

qse_pma_t* qse_pma_open (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize  /**< extension size in bytes */
);

void qse_pma_close (
	qse_pma_t* pma /**< memory allocator */
);

int qse_pma_init (
	qse_pma_t*  pma,     /**< memory allocator */
	qse_mmgr_t* mmgr     /**< memory manager */
);

void qse_pma_fini (
	qse_pma_t* pma /**< memory allocator */
);


void* qse_pma_alloc (
	qse_pma_t* pma,
	qse_size_t size
);	

void* qse_pma_calloc (
	qse_pma_t* pma,
	qse_size_t size
);	


void* qse_pma_realloc (
	qse_pma_t* pma,  /**< memory allocator */
	void*      blk,  /**< memory block */
	qse_size_t size  /**< new size in bytes */
);

void qse_pma_free (
	qse_pma_t* pma, /**< memory allocator */
	void*      blk  /**< memory block */
);

#endif
