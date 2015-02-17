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

#include <qse/cmn/HeapMmgr.hpp>
#include <qse/cmn/xma.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

struct xma_xtn_t
{
	HeapMmgr* heap;
};

HeapMmgr::HeapMmgr (Mmgr* mmgr, qse_size_t heap_size): 
	Mmgr(), Mmged(mmgr), xma(QSE_NULL), heap_size (heap_size)
{
}

HeapMmgr::~HeapMmgr ()
{
	if (this->xma) qse_xma_close (this->xma);
}

void* HeapMmgr::allocMem (qse_size_t n)
{
	if (!this->xma)
	{
		this->xma = qse_xma_open (this->getMmgr(), QSE_SIZEOF(xma_xtn_t), heap_size);
		if (!this->xma) return QSE_NULL;

		xma_xtn_t* xtn = (xma_xtn_t*)qse_xma_getxtn (this->xma);
		xtn->heap = this;
	}

	void* xptr = qse_xma_alloc (this->xma, n);
	if (!xptr) QSE_THROW (MemoryError);
	return xptr;
}

void* HeapMmgr::reallocMem (void* ptr, qse_size_t n)
{
	if (!this->xma)
	{
		this->xma = qse_xma_open (this->getMmgr(), QSE_SIZEOF(xma_xtn_t), heap_size);
		if (!this->xma) return QSE_NULL;

		xma_xtn_t* xtn = (xma_xtn_t*)qse_xma_getxtn (this->xma);
		xtn->heap = this;
	}

	void* xptr = qse_xma_realloc (this->xma, ptr, n);
	if (!xptr) QSE_THROW (MemoryError);
	return xptr;
}

void HeapMmgr::freeMem (void* ptr)
{
	if (this->xma) qse_xma_free (this->xma, ptr);
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
