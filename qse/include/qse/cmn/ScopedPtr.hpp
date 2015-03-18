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

#ifndef _QSE_CMN_SCOPEDPTR_HPP_
#define _QSE_CMN_SCOPEDPTR_HPP_

/// \file
/// Provides the ScopedPtr template class.

#include <qse/Uncopyable.hpp>
#include <qse/cmn/Mmgr.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename T>
struct ScopedPtrDeleter
{
	void operator() (T* ptr, void* arg)
	{
		delete ptr;
	}
};

template <typename T>
struct ScopedPtrArrayDeleter
{
	void operator() (T* ptr, void* arg)
	{
		delete[] ptr;
	}
};

template <typename T>
struct ScopedPtrMmgrDeleter
{
	void operator() (T* ptr, void* arg)
	{
		ptr->~T ();
		::operator delete (ptr, (QSE::Mmgr*)arg);
	}
};

/// The ScopedPtr class is a template class that destroys the object the
/// pointer points to when its destructor is called. You can use this class
/// to free a certain resource associated to the pointer when it goes out
/// of the current scope.
///
/// \code
/// #include <stdio.h>
/// #include <qse/cmn/ScopedPtr.hpp>
/// #include <qse/cmn/HeapMmgr.hpp>
/// 
/// 
/// class X
/// {
/// public:
///     X() { printf ("X constructured\n"); }
///     ~X() { printf ("X destructed\n"); }
/// };
/// 
/// 
/// int main ()
/// {
///     QSE::HeapMmgr heap_mmgr (QSE::Mmgr::getDFL(), 30000);
/// 
///     {   
///         QSE::ScopedPtr<X> x1 (new X);
///         QSE::ScopedPtr<X,QSE::ScopedPtrArrayDeleter<X> > x3 (new X[10]); 
///         QSE::ScopedPtr<X,QSE::ScopedPtrMmgrDeleter<X> > x2 (new(&heap_mmgr) X, &heap_mmgr);
///     }   
/// 
///     return 0;
/// }
/// \endcode
///

template<typename T, typename DELETER = ScopedPtrDeleter<T> >
class QSE_EXPORT ScopedPtr: public Uncopyable
{
public:
	typedef ScopedPtr<T,DELETER> SelfType;

	typedef ScopedPtrDeleter<T> DefaultDeleter;

	ScopedPtr (T* ptr = (T*)QSE_NULL, void* darg = (void*)QSE_NULL): _ptr (ptr), _darg (darg)
	{
	}

	~ScopedPtr () 
	{
		if (this->_ptr) 
		{
			this->_deleter (this->_ptr, this->_darg);
		}
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

	T* release () 
	{
		T* t = this->_ptr;
		this->_ptr = (T*)QSE_NULL;
		this->_darg = QSE_NULL;
		return t;
	}

	void reset (T* ptr = (T*)QSE_NULL, void* darg = (T*)QSE_NULL) 
	{
		if (this->_ptr) 
		{
			this->_deleter (this->_ptr, this->_darg);
		}

		this->_ptr = ptr;
		this->_darg = darg;
	}

private:
	T* _ptr;
	void* _darg;
	DELETER _deleter;
}; 

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
