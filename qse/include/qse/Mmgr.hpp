/*
 * $Id: Sed.hpp 127 2009-05-07 13:15:04Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_MMGR_HPP_
#define _QSE_MMGR_HPP_

#include <qse/Types.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/**
 * The Mmgr class defines a memory manager interface that can be inherited
 * by a class in need of a memory manager as defined in the primitive 
 * qse_mmgr_t type. Using the class over the primitive type enables you to
 * write code in more object-oriented fashion. An inheriting class should 
 * implement three pure virtual functions.
 */ 
class Mmgr: public Types, public qse_mmgr_t
{
public:
	/** defines an alias type to qse_mmgr_t */
	typedef qse_mmgr_t mmgr_t;

	/**
	 * The Mmgr() function builds a memory manager composed of bridge
	 * functions connecting itself with it.
	 */
	Mmgr () 
	{
		this->alloc = alloc_mem;
		this->realloc = realloc_mem;
		this->free = free_mem;
		this->udd = this;
	}

	/**
	 * The ~Mmgr() function finalizes a memory manager.
	 */
	virtual ~Mmgr () {}

protected:
	/** 
	 * The allocMem() function allocates a chunk of memory of the 
	 * size \a n and return the pointer to the beginning of the chunk.
	 * If it fails to allocate memory, it should return QSE_NULL.
	 */
	virtual void* allocMem (
		qse_size_t n /**< the size of allocate in bytes */
	) = 0;

	/**
	 * The reallocMem() function resizes a chunk of memory previously
	 * allocated with the allocMem() function. When resized, the contents
	 * of the surviving part of a memory chunk is preserved. If it fails to
	 * resize memory, it should return QSE_NULL.
	 */
	virtual void* reallocMem (
		void* ptr,   /**< a pointer to a memory chunk to resize */
		qse_size_t n /**< new size in bytes */
	) = 0;

	/**
	 * The freeMem() function frees a chunk of memory allocated with
	 * the allocMem() function or resized with the reallocMem() function.
	 */
	virtual void freeMem (
		void* ptr /**< a pointer to a memory chunk to free */
	) = 0;

protected:
	/**
	 * a bridge function from the qse_mmgr_t type the allocMem() function.
	 */
	static void* alloc_mem (void* udd, qse_size_t n) 
	{
		return ((Mmgr*)udd)->allocMem (n);
	}

	/**
	 * a bridge function from the qse_mmgr_t type the reallocMem() function.
	 */
	static void* realloc_mem (void* udd, void* ptr, qse_size_t n) 
	{
		return ((Mmgr*)udd)->reallocMem (ptr, n);
	}

	/**
	 * a bridge function from the qse_mmgr_t type the freeMem() function.
	 */
	static void  free_mem (void* udd, void* ptr) 
	{
		return ((Mmgr*)udd)->freeMem (ptr);
	}
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
