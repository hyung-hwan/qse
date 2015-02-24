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

#ifndef _QSE_CMN_MPOOL_HPP_
#define _QSE_CMN_MPOOL_HPP_

#include <qse/Uncopyable.hpp>
#include <qse/cmn/Mmged.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

//
// allocator for fixed-size data
//

class QSE_EXPORT Mpool: public Uncopyable, public Mmged
{
public:
	enum 
	{
		DEFAULT_BLOCK_SIZE = 128
	};

	Mpool (
		Mmgr* mmgr,
		qse_size_t datum_size, 
		qse_size_t block_size = DEFAULT_BLOCK_SIZE);
	~Mpool ();

	void* allocate ();
	void  dispose (void* ptr);
	void  dispose ();

	bool isEnabled () const
	{
		return this->datum_size > 0 && this->block_size > 0;
	}

	bool isDisabled () const
	{
		return this->datum_size <= 0 || this->block_size <= 0;
	}

	qse_size_t getDatumSize () const
	{
		return this->datum_size;
	}

	qse_size_t getBlockSize () const
	{
		return this->block_size;
	}

	void setBlockSize (qse_size_t blockSize) 
	{
		this->block_size = blockSize;
	}

	int swap (Mpool& mpool);

#if defined(QSE_DEBUG_MPOOL)
	qse_size_t  nalloc;
	qse_size_t  navail;
#endif

protected:
	struct Block 
	{
		Block*  next;
		//qse_uint8_t data[0];
	};

	struct Chain 
	{
		Chain* next;
	};

	// NOTE: whenever you add new member variables, make sure to
	//       update the swap() function accordingly
	Block* mp_blocks;
	Chain* free_list;
	qse_size_t datum_size;
	qse_size_t block_size;

	Block* add_block ();
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
////////////////////////////////

void* operator new (qse_size_t size, QSE::Mpool* mp);
void operator delete (void* ptr, QSE::Mpool* mp);

#endif

