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

#ifndef _QSE_REFCOUNTED_HPP_
#define _QSE_REFCOUNTED_HPP_


/// \file
/// Defines a class that can be used to implement a class of a reference
/// counted object.

#include <qse/Uncopyable.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/// The RefCounted class provides common functions required to
/// implement a reference counted class.

class QSE_EXPORT RefCounted: public Uncopyable
{
protected:
	RefCounted (): _ref_count(0)
	{
	}

public:
	/// The ref() function increments the reference count and returns
	/// the incremented count.
	qse_size_t ref () const
	{
		return ++this->_ref_count;
	}

	/// The deref() function decrements the reference count and returns
	/// the decremented count. The caller should kill the callee if it 
	/// returns 0.
	qse_size_t deref () const
	{
		return --this->_ref_count;
	}

	/// The getRefCount() function returns the current reference count.
	qse_size_t getRefCount () const
	{
		return this->_ref_count;
	}

	/// The isShared() function returns true if the object is referenced
	/// more than twice and false otherwise.
	bool isShared () const
	{
		return this->_ref_count > 1;
	}

protected:
	mutable qse_size_t _ref_count;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
