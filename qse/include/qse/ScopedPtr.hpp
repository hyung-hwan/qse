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

#ifndef _QSE_SCOPEDPTR_HPP_
#define _QSE_SCOPEDPTR_HPP_

#include <qse/Uncopyable.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class ScopedPtrType 
{
public:
	enum Value 
	{
		SINGLETON = 0,
		ARRAY     = 1
	};
};

template<class T, ScopedPtrType::Value type = ScopedPtrType::SINGLETON>
class QSE_EXPORT ScopedPtr: public Uncopyable
{
public:
	ScopedPtr (T* p = (T*)QSE_NULL) 
	{
		this->_ptr = p;
	}

	~ScopedPtr () 
	{
		if (this->_ptr) 
		{
			if (type == ScopedPtrType::SINGLETON) delete this->_ptr;
			else delete[] this->_ptr;
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

	int operator! () const 
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
		return t;
	}

	void reset (T* p = (T*)QSE_NULL) 
	{
		if (this->_ptr) 
		{
			if (type == ScopedPtrType::SINGLETON) delete this->_ptr;
			else delete[] this->_ptr;
		}

		this->_ptr = p;
	}

protected:
	T* _ptr;
}; 

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
