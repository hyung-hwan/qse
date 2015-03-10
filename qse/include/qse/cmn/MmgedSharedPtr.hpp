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

#ifndef _QSE_CMN_MMGEDSHAREDPTR_HPP_
#define _QSE_CMN_MMGEDSHAREDPTR_HPP_

#include <qse/cmn/Mmged.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename T>
struct MmgedSharedPtrDeleter
{
	void operator() (T* ptr, void* arg)
	{
		delete ptr;
	}
};

template <typename T>
struct MmgedSharedPtrArrayDeleter
{
	void operator() (T* ptr, void* arg)
	{
		delete[] ptr;
	}
};

///
/// The MmgedSharedPtr class is similar to SharedPtr except that i
/// accepts a pointer to a memory manager to allocate the space for
/// shared reference count.
///
template<typename T, typename DELETER = MmgedSharedPtrDeleter<T> >
class QSE_EXPORT MmgedSharedPtr: public Mmged
{
public:
	typedef MmgedSharedPtr<T,DELETER> SelfType;

	typedef MmgedSharedPtrDeleter<T> DefaultDeleter;

	MmgedSharedPtr (T* ptr = (T*)QSE_NULL, void* darg = (void*)QSE_NULL): Mmged(QSE_NULL), _ptr(ptr), _darg(darg)
	{
		this->_ref = new(this->getMmgr()) qse_size_t;
		(*this->_ref) = 1;
	}

	MmgedSharedPtr (Mmgr* mmgr, T* ptr = (T*)QSE_NULL, void* darg = (void*)QSE_NULL): Mmged(mmgr), _ptr(ptr), _darg(darg)
	{
		this->_ref = new(this->getMmgr()) qse_size_t;
		(*this->_ref) = 1;
	}

	MmgedSharedPtr (const SelfType& ptr): Mmged(ptr), _ref(ptr._ref), _ptr (ptr._ptr), _darg (ptr._darg)
	{
		(*this->_ref)++;
	}

	~MmgedSharedPtr () 
	{
		(*this->_ref)--;
		if (*this->_ref <= 0)
		{
			if (this->_ptr) this->deleter (this->_ptr, this->_darg);
			// no destructor as *this->_ref is a plain type.
			::operator delete (this->_ref, this->getMmgr());
		}
	}

	SelfType& operator= (const SelfType& ptr)
	{
		if (this != &ptr)
		{
			(*this->_ref)--;
			if (*this->_ref <= 0)
			{
				if (this->_ptr) this->deleter (this->_ptr, this->_darg);
				// no destructor as *this->_ref is a plain type.
				::operator delete (this->_ref, this->getMmgr());
			}

			this->mmgr = ptr.getMmgr(); // memory manager must be copied
			this->_ptr = ptr._ptr;
			this->_darg = ptr._darg;
			this->_ref = ptr._ref;
			(*this->_ref)++;
		}

		return *this;
	}

	T& operator* ()
	{
		QSE_ASSERT (this->_ptr != (T*)QSE_NULL);
		return *this->_ptr;
	}

	const T& operator* () const 
	{
		QSE_ASSERT (this->_ptr != (T*)QSE_NULL);
		return *this->_ptr;
	}

	T* operator-> () 
	{
		QSE_ASSERT (this->_ptr != (T*)QSE_NULL);
		return this->_ptr;
	}

	const T* operator-> () const 
	{
		QSE_ASSERT (this->_ptr != (T*)QSE_NULL);
		return this->_ptr;
	}

	bool operator! () const 
	{
		return this->_ptr == (T*)QSE_NULL;
	}

	T& operator[] (qse_size_t idx) 
	{
		QSE_ASSERT (this->_ptr != (T*)QSE_NULL);
		return this->_ptr[idx];
	}

	T* get () 
	{
		return this->_ptr;
	}

	const T* get () const
	{
		return this->_ptr;
	}

protected:
	qse_size_t* _ref;
	T*          _ptr;
	void*       _darg;
	DELETER     deleter;
}; 

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
