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

#ifndef _QSE_CMN_HEAPMMGR_HPP_
#define _QSE_CMN_HEAPMMGR_HPP_

#include <qse/cmn/Mmgr.hpp>
#include <qse/cmn/Mmged.hpp>
#include <qse/cmn/xma.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/// The HeapMmgr class implements a memory management interface that
/// handles a memory heap of a given size. Notably, it is a memory manager 
/// managed by another memory manager. 
///
/// \code
///   QSE::HeapMmgr heap_mmgr (QSE::Mmgr::getDFL(), 30000);
///   QSE::LinkedList<int> int_list (&heap_mmgr);
///   int_list.append (10);
///   int_list.append (20);
/// \endcode

class QSE_EXPORT HeapMmgr: public Mmgr, public Mmged
{
public:
	/// The constructor function accepts an memory manager \a mmgr that 
	/// is used to create a heap of the size \a heap_size. 
	HeapMmgr (Mmgr* mmgr, qse_size_t heap_size);

	/// The destructor function frees the heap. Memory areas returned by
	/// allocate(), reallocate(), allocMem(), reallocMem() are invalidated
	/// all at once.
	~HeapMmgr ();

	void* allocMem (qse_size_t n);
	void* reallocMem (void* ptr, qse_size_t n);
	void freeMem (void* ptr);

	// the library does not provide a stock instance of this class
	//static HeapMmgr* getInstance ();

protected:
	qse_xma_t* xma;
	qse_size_t heap_size;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
