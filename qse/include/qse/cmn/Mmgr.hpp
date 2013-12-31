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

#ifndef _QSE_CMN_MMGR_HPP_
#define _QSE_CMN_MMGR_HPP_

#include <qse/Types.hpp>

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
class QSE_EXPORT Mmgr: public Types, public qse_mmgr_t
{
public:
	/// defines an alias type to #qse_mmgr_t 
	typedef qse_mmgr_t mmgr_t;

	///
	/// The Mmgr() function builds a memory manager composed of bridge
	/// functions connecting itself with it.
	///
	Mmgr () 
	{
		this->alloc = alloc_mem;
		this->realloc = realloc_mem;
		this->free = free_mem;
		this->ctx = this;
	}

	///
	/// The ~Mmgr() function finalizes a memory manager.
	///
	virtual ~Mmgr () {}

protected:
	/// 
	/// The allocMem() function allocates a chunk of memory of the 
	/// size @a n and return the pointer to the beginning of the chunk.
	/// If it fails to allocate memory, it should return QSE_NULL.
	///
	virtual void* allocMem (
		size_t n ///< size of memory chunk to allocate in bytes 
	) = 0;

	///
	/// The reallocMem() function resizes a chunk of memory previously
	/// allocated with the allocMem() function. When resized, the contents
	/// of the surviving part of a memory chunk is preserved. If it fails to
	/// resize memory, it should return QSE_NULL.
	///
	virtual void* reallocMem (
		void* ptr, ///< pointer to memory chunk to resize
		size_t n   ///< new size in bytes
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
	static void* alloc_mem (void* ctx, size_t n);

	///
	/// bridge function from the #qse_mmgr_t type the reallocMem() function.
	///
	static void* realloc_mem (void* ctx, void* ptr, size_t n);

	///
	/// bridge function from the #qse_mmgr_t type the freeMem() function.
	///
	static void  free_mem (void* ctx, void* ptr);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
