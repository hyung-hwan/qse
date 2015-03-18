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

#ifndef _QSE_GROWABLE_HPP_
#define _QSE_GROWABLE_HPP_

/// \file 
/// Provides classes for handling size growth including buffer growth.

#include <qse/types.h>
#include <qse/macros.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

///
/// The GrowthPolicy class is an abstract class that defines the behavior of
/// growth policy.
///
class QSE_EXPORT GrowthPolicy
{
public:
	virtual ~GrowthPolicy () {}

	///
	/// A subclass must implement this function to return a new size over
	/// the existing size \a current.
	///
	virtual qse_size_t getNewSize (qse_size_t current) const = 0;
};

///
/// The PercentageGrowthPolicy class calculates a new size incremented by
/// the configured percentage over the existing size.
///
class QSE_EXPORT PercentageGrowthPolicy: public GrowthPolicy
{
public:
	PercentageGrowthPolicy (int percentage = 0): _percentage(percentage) {}

	qse_size_t getNewSize (qse_size_t current) const
	{
		// TODO: better way to handle overflow?
		qse_size_t new_size = current + ((current * this->_percentage) / 100);
		if (new_size < current) new_size = QSE_TYPE_MAX(qse_size_t);
		return new_size;
	}

protected:
	int _percentage;
};

///
/// The Growable class implements common functions to get and set growth policy.
/// The class of an object that needs to grow the buffer or something similar
/// can inherit this class and utilize the policy set. The interface is designed
/// to remember the pointer to the policy to minimize memory use. This requires
/// the policy to outlive the life of the target object set with the policy.
///
class QSE_EXPORT Growable
{
public:
	Growable (const GrowthPolicy* p = QSE_NULL): _growth_policy(p) {}

	const GrowthPolicy* getGrowthPolicy () const
	{
		return this->_growth_policy;
	}

	void setGrowthPolicy (const GrowthPolicy* p) 
	{
		this->_growth_policy = p;
	}

protected:
	const GrowthPolicy* _growth_policy;
};


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
