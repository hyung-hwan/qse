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

#ifndef _QSE_CMN_MMGR_HPP_
#define _QSE_CMN_MMGR_HPP_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/Exception.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

///
/// The Mmgr class defines a memory manager interface that can be inherited
/// by a class in need of a memory manager as defined in the primitive 
/// #qse_mmgr_t type. Using the class over the primitive type enables you to
/// write code in more object-oriented fashion. An inheriting class should 
/// implement three pure virtual functions.
///
/// You are free to call allocMem(), reallocMem(), and freeMem() in C++ context
/// where no exception raising is desired. If you want an exception to be 
/// raised upon memory allocation errors, you can call allocate(), reallocate(),
/// dispose() instead.
/// 
/// 
class QSE_EXPORT Mmgr: public qse_mmgr_t
{
public:
	/// defines an alias type to #qse_mmgr_t 
	typedef qse_mmgr_t mmgr_t;

	QSE_EXCEPTION (MemoryError);

protected:
	bool raise_exception;

public:
	///
	/// The Mmgr() function builds a memory manager composed of bridge
	/// functions connecting itself with it.
	///
	Mmgr ()
	{
		// NOTE:
		//  the #qse_mmgr_t interface is not affected by raise_exception
		//  because direct calls to the virtual functions that don't raise
		//  exceptions are made via bridge functions below.
		this->alloc = alloc_mem;
		this->realloc = realloc_mem;
		this->free = free_mem;
		this->ctx = this;
	}

	///
	/// The ~Mmgr() function finalizes a memory manager.
	///
	virtual ~Mmgr () {}

	///
	/// The allocate() function calls allocMem() for memory
	/// allocation. if it fails, it raise an exception if it's
	/// configured to do so.
	///
	void* allocate (qse_size_t n, bool raise_exception = true)
	{
		void* xptr = this->allocMem (n);
		if (!xptr && raise_exception) QSE_THROW (MemoryError);
		return xptr;
	}

	///
	/// The callocate() function allocates memory like allocate() and 
	/// clears the memory before returning.
	///
	void* callocate (qse_size_t n, bool raise_exception = true);

	///
	/// The reallocate() function calls reallocMem() for memory
	/// reallocation. if it fails, it raise an exception if it's
	/// configured to do so.
	///
	void* reallocate (void* ptr, qse_size_t n, bool raise_exception = true)
	{
		void* xptr = this->reallocMem (ptr, n);
		if (!xptr && raise_exception) QSE_THROW (MemoryError);
		return xptr;
	}

	///
	/// The dispose() function calls freeMem() for memory disposal.
	///
	void dispose (void* ptr)
	{
		this->freeMem (ptr);
	}

//protected:
	/// 
	/// The allocMem() function allocates a chunk of memory of the 
	/// size \a n and return the pointer to the beginning of the chunk.
	/// If it fails to allocate memory, it should return #QSE_NULL.
	///
	virtual void* allocMem (
		qse_size_t n ///< size of memory chunk to allocate in bytes 
	) = 0;

	///
	/// The reallocMem() function resizes a chunk of memory previously
	/// allocated with the allocMem() function. When resized, the contents
	/// of the surviving part of a memory chunk is preserved. If it fails to
	/// resize memory, it should return QSE_NULL.
	///
	virtual void* reallocMem (
		void* ptr, ///< pointer to memory chunk to resize
		qse_size_t n   ///< new size in bytes
	) = 0;

	///
	/// The freeMem() function frees a chunk of memory allocated with
	/// the allocMem() function or resized with the reallocMem() function.
	///
	virtual void freeMem (
		void* ptr ///< pointer to memory chunk to free 
	) = 0;

protected:
	///
	/// bridge function from the #qse_mmgr_t type the allocMem() function.
	///
	static void* alloc_mem (mmgr_t* mmgr, qse_size_t n);

	///
	/// bridge function from the #qse_mmgr_t type the reallocMem() function.
	///
	static void* realloc_mem (mmgr_t* mmgr, void* ptr, qse_size_t n);

	///
	/// bridge function from the #qse_mmgr_t type the freeMem() function.
	///
	static void  free_mem (mmgr_t* mmgr, void* ptr);

public:
	static Mmgr* getDFL ();
	static void setDFL (Mmgr* mmgr);

protected:
	static Mmgr* dfl_mmgr;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

void* operator new (qse_size_t size, QSE::Mmgr* mmgr);
void operator delete (void* ptr, QSE::Mmgr* mmgr);

void* operator new (qse_size_t size, QSE::Mmgr* mmgr, void* existing_ptr);

#if 0
// i found no way to delete an array allocated with
// the placement new operator. if the array element is an instance
// of a class, the pointer returned by the new operator call
// may not be the actual pointer allocated. some compilers
// seem to put some information about the array at the 
// beginning of the allocated memory and return the pointer
// off a few bytes from the beginning.
void* operator new[] (qse_size_t size, QSE::Mmgr* mmgr);
void operator delete[] (void* ptr, QSE::Mmgr* mmgr);
#endif

#endif
