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

/*
 * This is the TRE memory allocator modified for QSE.
 * See the original license notice below.
 */

/*
 tre-mem.c - TRE memory allocator

 This software is released under a BSD-style license.
 See the file LICENSE for details and copyright.
 */

/*
 This memory allocator is for allocating small memory blocks efficiently
 in terms of memory overhead and execution speed.  The allocated blocks
 cannot be freed individually, only all at once.  There can be multiple
 allocators, though.
 */

#include <qse/cmn/pma.h>
#include "mem.h"

/* Returns number of bytes to add to (char *)ptr to make it
   properly aligned for the type. */
#define ALIGN(ptr, type) \
	((((qse_uintptr_t)ptr) % QSE_SIZEOF(type))? \
		(QSE_SIZEOF(type) - (((qse_uintptr_t)ptr) % QSE_SIZEOF(type))) : 0)


qse_pma_t* qse_pma_open (qse_mmgr_t* mmgr, qse_size_t xtnsize) 
{
	qse_pma_t* pma;

	pma = (qse_pma_t*)QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*pma) + xtnsize);
	if (pma == QSE_NULL) return QSE_NULL;

	if (qse_pma_init (pma, mmgr) <= -1)
	{
		QSE_MMGR_FREE (mmgr, pma);
		return QSE_NULL;
	}

	QSE_MEMSET (QSE_XTN(pma), 0, xtnsize);
	return pma;
}

void qse_pma_close (qse_pma_t* pma)
{
	qse_pma_fini (pma);
	QSE_MMGR_FREE (pma->mmgr, pma);
}

int qse_pma_init (qse_pma_t* pma, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (pma, 0, QSE_SIZEOF(*pma));
	pma->mmgr = mmgr;
	return 0;
}

/* Frees the memory allocator and all memory allocated with it. */
void qse_pma_fini (qse_pma_t* pma)
{
	qse_pma_clear (pma);
}

qse_mmgr_t* qse_pma_getmmgr (qse_pma_t* pma)
{
	return pma->mmgr;
}

void* qse_pma_getxtn (qse_pma_t* pma)
{
	return QSE_XTN (pma);
}

void qse_pma_clear (qse_pma_t* pma)
{
	qse_mmgr_t* mmgr = pma->mmgr;
	qse_pma_blk_t* tmp, * l = pma->blocks;

	while (l != QSE_NULL)
	{
		tmp = l->next;
		QSE_MMGR_FREE (mmgr, l);
		l = tmp;
	}
	
	QSE_MEMSET (pma, 0, QSE_SIZEOF(*pma));
	pma->mmgr = mmgr;
}
/* Returns a new memory allocator or NULL if out of memory. */

/* Allocates a block of `size' bytes from `mem'.  Returns a pointer to the
 allocated block or NULL if an underlying malloc() failed. */
void* qse_pma_alloc (qse_pma_t* pma, qse_size_t size)
{
	void *ptr;

	if (pma->failed) return QSE_NULL;

	if (pma->n < size)
	{
		/* We need more memory than is available in the current block.
		 Allocate a new block. */

		qse_pma_blk_t* l;
		int block_size;
		if (size * 8 > QSE_PMA_BLOCK_SIZE)
			block_size = size * 8;
		else
			block_size = QSE_PMA_BLOCK_SIZE;

		l = QSE_MMGR_ALLOC (pma->mmgr, QSE_SIZEOF(*l) + block_size);
		if (l == QSE_NULL)
		{
			pma->failed = 1;
			return QSE_NULL;
		}
		l->data = (void*)(l + 1);

		l->next = QSE_NULL;
		if (pma->current != QSE_NULL) pma->current->next = l;
		if (pma->blocks == QSE_NULL) pma->blocks = l;
		pma->current = l;
		pma->ptr = l->data;
		pma->n = block_size;
	}

	/* Make sure the next pointer will be aligned. */
	size += ALIGN((qse_uintptr_t)(pma->ptr + size), qse_uintptr_t);

	/* Allocate from current block. */
	ptr = pma->ptr;
	pma->ptr += size;
	pma->n -= size;

	return ptr;
}

void* qse_pma_calloc (qse_pma_t* pma, qse_size_t size)
{
	void* ptr = qse_pma_alloc (pma, size);
	if (ptr) QSE_MEMSET (ptr, 0, size);
	return ptr;
}

void* qse_pma_realloc (qse_pma_t* pma, void* blk, qse_size_t size)
{
	/* do nothing. you can't resize an individual memory chunk */
	return QSE_NULL;
}

void qse_pma_free (qse_pma_t* pma, void* blk)
{
	/* do nothing. you can't free an individual memory chunk */
}

