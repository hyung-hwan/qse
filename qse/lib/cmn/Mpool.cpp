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

#include <qse/cmn/Mpool.hpp>


// TODO: can i use QSE_MMGR_XXXXX instead of ::new and ::delete???

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

Mpool::Mpool (qse_size_t datum_size, qse_size_t block_size)
{
	if (datum_size > 0 && datum_size < QSE_SIZEOF(void*))
		datum_size = QSE_SIZEOF(void*);

	this->mp_blocks  = QSE_NULL;
	this->free_list  = QSE_NULL;
	this->datum_size = datum_size;
	this->block_size = block_size;
#if defined(QSE_DEBUG_MPOOL)
	this->navail = 0;
	this->nalloc = 0;
#endif
}
Mpool::~Mpool ()
{
	this->dispose ();
}

void* Mpool::allocate ()
{
	if (this->datum_size <= 0 || this->block_size <= 0) return QSE_NULL;

	void* ptr = this->free_list;
	if (!ptr) 
	{
		this->add_block ();
		ptr = this->free_list;
	}
	this->free_list = this->free_list->next;
#if defined(QSE_DEBUG_MPOOL)
	this->navail--;
#endif
	return ptr;
}

void Mpool::dispose (void* ptr)
{
	((Chain*)ptr)->next = this->free_list;
	this->free_list = (Chain*)ptr;
#if defined(QSE_DEBUG_MPOOL)
	this->navail++;
#endif
}

void Mpool::dispose ()
{
	Block* block = this->mp_blocks;
	while (block) 
	{
		Block* next = block->next;
		::delete[] block;
		block = next;
	}

	this->free_list = QSE_NULL;
	this->mp_blocks = QSE_NULL;
#if defined(QSE_DEBUG_MPOOL)
	this->navail = 0;
	this->nalloc = 0;
#endif
}

void Mpool::add_block ()
{
	QSE_ASSERT (this->datum_size > 0 && this->block_size > 0);

	Block* block = (Block*)::new qse_uint8_t[
		QSE_SIZEOF(Block) + this->block_size * this->datum_size];

	//this->free_list = (Chain*)block->data;
	this->free_list = (Chain*)(block + 1);
	Chain* ptr = this->free_list;
	for (qse_size_t i = 0; i < this->block_size - 1; i++) 
	{
		Chain* next = (Chain*)((qse_uint8_t*)ptr + this->datum_size);
		ptr->next = next;
		ptr = next;
	}
	ptr->next = QSE_NULL;

	block->next  = this->mp_blocks;
	this->mp_blocks = block;
#if defined(QSE_DEBUG_MPOOL)
	this->navail += this->block_size;
	this->nalloc += this->block_size;
#endif
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
