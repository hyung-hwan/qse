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

#ifndef _QSE_CMN_SHAREDPTR_HPP_
#define _QSE_CMN_SHAREDPTR_HPP_

/// \file
/// Provides the SharedPtr template class.

#include <qse/cmn/Mmged.hpp>
#include <qse/RefCounted.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename T>
struct SharedPtrDeleter
{
	void operator() (T* ptr, void* arg)
	{
		delete ptr;
	}
};

template <typename T>
struct SharedPtrArrayDeleter
{
	void operator() (T* ptr, void* arg)
	{
		delete[] ptr;
	}
};

template <typename T>
struct SharedPtrMmgrDeleter
{
	void operator() (T* ptr, void* arg)
	{
		QSE_CPP_CALL_DESTRUCTOR (ptr, T);
		QSE_CPP_CALL_PLACEMENT_DELETE1 (ptr, (QSE::Mmgr*)arg);
	}
};

///
/// The SharedPtr class provides a smart pointer that can be shared
/// using reference counting.
///
template<typename T, typename DELETER = SharedPtrDeleter<T> >
class QSE_EXPORT SharedPtr: public Mmged
{
public:
	typedef SharedPtr<T,DELETER> SelfType;

	typedef SharedPtrDeleter<T> DefaultDeleter;

	SharedPtr (T* ptr = (T*)QSE_NULL, void* darg = (void*)QSE_NULL): Mmged(QSE_NULL)
	{
		this->_item = new(this->getMmgr()) item_t;
		this->_item->ptr = ptr;
		this->_item->darg = darg;

		this->_item->ref ();
	}

	SharedPtr (Mmgr* mmgr, T* ptr = (T*)QSE_NULL, void* darg = (void*)QSE_NULL): Mmged(mmgr)
	{
		this->_item = new(this->getMmgr()) item_t;
		this->_item->ptr = ptr;
		this->_item->darg = darg;

		this->_item->ref ();
	}

	SharedPtr (const SelfType& sp): Mmged(sp), _item(sp._item) 
	{
		this->_item->ref ();
	}

	~SharedPtr () 
	{
		if (this->_item->deref() <= 0)
		{
			// reference count reached 0.
			if (this->_item->ptr) this->_item->deleter (this->_item->ptr, this->_item->darg);

			QSE_CPP_CALL_DESTRUCTOR (this->_item, item_t);
			QSE_CPP_CALL_PLACEMENT_DELETE1 (this->_item, this->getMmgr());
		}
	}

	SelfType& operator= (const SelfType& sp)
	{
		if (this != &sp)
		{
			if (this->_item->deref() <= 0)
			{
				if (this->_item->ptr) this->_item->deleter (this->_item->ptr, this->_item->darg);

				QSE_CPP_CALL_DESTRUCTOR (this->_item, item_t);
				QSE_CPP_CALL_PLACEMENT_DELETE1 (this->_item, this->getMmgr());
			}

			// must copy the memory manager pointer as the item
			// to be copied is allocated using the memory manager of sp.
			this->setMmgr (sp.getMmgr()); // copy over mmgr

			this->_item = sp._item;
			this->_item->ref ();
		}

		return *this;
	}

	T& operator* ()
	{
		QSE_ASSERT (this->_item->ptr != (T*)QSE_NULL);
		return *this->_item->ptr;
	}

	const T& operator* () const 
	{
		QSE_ASSERT (this->_item->ptr != (T*)QSE_NULL);
		return *this->_item->ptr;
	}

	T* operator-> () 
	{
		QSE_ASSERT (this->_item->ptr != (T*)QSE_NULL);
		return this->_item->ptr;
	}

	const T* operator-> () const 
	{
		QSE_ASSERT (this->_item->ptr != (T*)QSE_NULL);
		return this->_item->ptr;
	}

	bool operator! () const 
	{
		return this->_item->ptr == (T*)QSE_NULL;
	}

	T& operator[] (qse_size_t idx) 
	{
		QSE_ASSERT (this->_item->ptr != (T*)QSE_NULL);
		return this->_item->ptr[idx];
	}

	T* getPtr () 
	{
		return this->_item->ptr;
	}

	const T* getPtr () const
	{
		return this->_item->ptr;
	}

private:
	struct item_t: public RefCounted
	{
		T*         ptr;
		void*      darg;
		DELETER    deleter;
	};

	item_t* _item;
}; 

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
