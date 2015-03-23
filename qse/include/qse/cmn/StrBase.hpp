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

#ifndef _QSE_CMN_STRBASE_HPP_
#define _QSE_CMN_STRBASE_HPP_


#include <qse/Hashable.hpp>
#include <qse/Growable.hpp>
#include <qse/RefCounted.hpp>
#include <qse/cmn/Mmged.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename CHAR_TYPE, CHAR_TYPE NULL_CHAR, typename OPSET, typename RESIZER> class StrBase;

template <typename CHAR_TYPE, CHAR_TYPE NULL_CHAR, typename OPSET, typename RESIZER>
class StrBaseData: public RefCounted
{
protected:
	friend class StrBase<CHAR_TYPE,NULL_CHAR,OPSET,RESIZER>;

	typedef StrBaseData<CHAR_TYPE,NULL_CHAR,OPSET,RESIZER> SelfType;

	StrBaseData (Mmgr* mmgr, qse_size_t capacity, const CHAR_TYPE* str, qse_size_t size): 
		buffer (QSE_NULL) // set buffer to QSE_NULL here in case operator new rasises an exception
	{
		if (capacity < size) capacity = size;

		// I assume that CHAR_TYPE is a plain type.
		//this->buffer = (CHAR_TYPE*)::operator new ((capacity + 1) * QSE_SIZEOF(CHAR_TYPE), mmgr);
		this->buffer = (CHAR_TYPE*)mmgr->allocate((capacity + 1) * QSE_SIZEOF(CHAR_TYPE));
		// So no constructor calling is needed.
		//for (qse_size_t i = 0; i < capacity + 1; i++)
		//{
		//	new (mmgr, &this->buffer[i]) CHAR_TYPE ();
		//}

		this->capacity = capacity;
		this->size = this->opset.copy (this->buffer, str, size);
		QSE_ASSERT (this->size == size);
	}

	StrBaseData (Mmgr* mmgr, qse_size_t capacity, CHAR_TYPE c, qse_size_t size): 
		buffer (QSE_NULL) // set buffer to QSE_NULL here in case operator new rasises an exception
	{
		if (capacity < size) capacity = size;
		//this->buffer = (CHAR_TYPE*)::operator new ((capacity + 1) * QSE_SIZEOF(CHAR_TYPE), mmgr);
		this->buffer = (CHAR_TYPE*)mmgr->allocate((capacity + 1) * QSE_SIZEOF(CHAR_TYPE));

		this->capacity = capacity;
		this->buffer[size] = NULL_CHAR;
		this->size = size;
		while (size > 0) this->buffer[--size] = c;
	}

	void shatter (Mmgr* mmgr)
	{
		QSE_ASSERT (this->buffer != QSE_NULL);
		mmgr->dispose (this->buffer); //::operator_delete (this->buffer, mmgr);
		this->buffer = QSE_NULL;
	}

public:
	~StrBaseData () 
	{
		QSE_ASSERT (this->buffer == QSE_NULL);
	}

protected:
	SelfType* copy (Mmgr* mmgr)
	{
		return new(mmgr) SelfType (mmgr, this->capacity, this->buffer, this->size);
	}

	SelfType* copy (Mmgr* mmgr, qse_size_t capacity)
	{
		return new(mmgr) SelfType (mmgr, capacity, this->buffer, this->size);
	}

#if 0
	void growBy (Mmgr* mmgr, qse_size_t inc)
	{
		if (inc > 0)
		{
			qse_size_t newcapa = this->capacity + inc;
			//CHAR_TYPE* tmp = ::operator new ((newcapa + 1) * QSE_SIZEOF(CHAR_TYPE), mmgr);
			CHAR_TYPE* tmp = (CHAR_TYPE*)mmgr->allocate((newcapa + 1) * QSE_SIZEOF(CHAR_TYPE));

			qse_size_t cursize = this->size;
			this->size = this->opset.copy (tmp, this->buffer, cursize);
			QSE_ASSERT (this->size == cursize);

			//::operator_delete (this->buffer, mmgr);
			mmgr->dispose (this->buffer);

			this->buffer = tmp;
			this->capacity = newcapa;
		}
	}
#endif

	CHAR_TYPE* buffer;
	qse_size_t capacity;
	qse_size_t size;
	OPSET      opset;
};

struct StrBaseResizer
{
	qse_size_t operator() (qse_size_t current, qse_size_t desired, const GrowthPolicy* gp) const
	{
		qse_size_t new_size;

		if (gp)
		{
			new_size = gp->getNewSize (current);
		}
		else
		{
			new_size = (current < 5000)?   (current + current):
					 (current < 50000)?  (current + (current / 2)):
					 (current < 100000)? (current + (current / 4)):
					 (current < 150000)? (current + (current / 8)):
					                     (current + (current / 16));
		}

		if (new_size < desired) new_size = desired;
		return new_size;
	}
};

template <typename CHAR_TYPE, CHAR_TYPE NULL_CHAR, typename OPSET, typename RESIZER = StrBaseResizer>
class StrBase: public Mmged, public Hashable, public Growable
{
public:
	enum 
	{
		DEFAULT_CAPACITY = 128,
		INVALID_INDEX = ~(qse_size_t)0
	};

	typedef StrBase<CHAR_TYPE,NULL_CHAR,OPSET,RESIZER> SelfType;
	typedef StrBaseData<CHAR_TYPE,NULL_CHAR,OPSET,RESIZER> StringItem;

	/// The StrBase() function creates an empty string with the default memory manager.
	StrBase (): Mmged(QSE_NULL)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(DEFAULT_CAPACITY), (const CHAR_TYPE*)QSE_NULL, 0);
		this->ref_item ();
	}

	/// The StrBase() function creates an empty string with a memory manager \a mmgr.
	StrBase (Mmgr* mmgr): Mmged(mmgr)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(DEFAULT_CAPACITY), (const CHAR_TYPE*)QSE_NULL, 0);
		this->ref_item ();
	}

	StrBase (qse_size_t capacity): Mmged(QSE_NULL)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(capacity), (const CHAR_TYPE*)QSE_NULL, 0);
		this->ref_item ();
	}

	StrBase (Mmgr* mmgr, qse_size_t capacity): Mmged(mmgr)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(capacity), (const CHAR_TYPE*)QSE_NULL, 0);
		this->ref_item ();
	}

	StrBase (const CHAR_TYPE* str): Mmged(QSE_NULL)
	{
		qse_size_t len = this->_opset.getLength(str);
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(len), str, len);
		this->ref_item ();
	}

	StrBase (Mmgr* mmgr, const CHAR_TYPE* str): Mmged(mmgr)
	{
		qse_size_t len = this->_opset.getLength(str);
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(len), str, len);
		this->ref_item ();
	}

	StrBase (const CHAR_TYPE* str, qse_size_t size): Mmged(QSE_NULL)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(size), str, size);
		this->ref_item ();
	}

	StrBase (Mmgr* mmgr, const CHAR_TYPE* str, qse_size_t size): Mmged(mmgr)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(size), str, size);
		this->ref_item ();
	}

	StrBase (CHAR_TYPE c, qse_size_t size): Mmged(QSE_NULL)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(size), c, size);
		this->ref_item ();
	}

	StrBase (Mmgr* mmgr, CHAR_TYPE c, qse_size_t size): Mmged(mmgr)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(size), c, size);
		this->ref_item ();
	}

	StrBase (const SelfType& str): Mmged(str)
	{
		this->_item = str._item;
		this->ref_item ();
	}

	~StrBase () 
	{
		this->deref_item ();
	}

	SelfType& operator= (const SelfType& str)
	{
		if (this->_item != str._item) 
		{
			this->deref_item ();

			// the data to be reference could be allocated using the
			// memory manager of str. and it may be freed or resized by
			// this. so the inner memory manager must be switched.
			this->setMmgr (str.getMmgr()); // copy over mmgr.

			this->_item = str._item;
			this->ref_item ();
		}
		return *this;
	}

	SelfType& operator= (const CHAR_TYPE* str)
	{
		if (this->_item->buffer != str)
		{
			this->clear ();
			this->insert (0, str);
		}
		return *this;
	}

	SelfType& operator= (const CHAR_TYPE c)
	{
		this->clear ();
		this->insert (0, &c, 1);
		return *this;
	}

protected:
	void ref_item () const
	{
		this->_item->ref ();
	}

	void deref_item (StringItem* sd) const
	{
		if (sd->deref () <= 0)
		{
			// i can't pass an argument to the destructor.
			// destroy the actual contents via the shatter() funtion
			// call the destructor for completeness only
			sd->shatter (this->getMmgr()); 

			// delete the object allocated using the place new 
			QSE_CPP_CALL_DESTRUCTOR (sd, StringItem);
			QSE_CPP_CALL_PLACEMENT_DELETE1 (sd, this->getMmgr());
		}
	}

	void deref_item () const
	{
		this->deref_item (this->_item);
	}

	void possess_data () const
	{
		StringItem* t = this->_item->copy (this->getMmgr());
		this->deref_item ();
		this->_item = t;
		this->ref_item ();
	}

	void possess_data (qse_size_t capacity) const
	{
		StringItem* t = this->_item->copy (this->getMmgr(), capacity);
		this->deref_item ();
		this->_item = t;
		this->ref_item ();
	}

	void force_size (qse_size_t size) const
	{
		// for internal use only.
		QSE_ASSERT (size < this->getCapacity());
		this->_item->size = size;
	}

	qse_size_t round_capacity (qse_size_t n) 
	{
		if (n == 0) n = 1;
		return (n + (qse_size_t)DEFAULT_CAPACITY - 1) & 
		       ~((qse_size_t)DEFAULT_CAPACITY - (qse_size_t)1);
	}

public:

	qse_size_t getSize () const 
	{
		return this->_item->size;
	}

	qse_size_t getLength () const 
	{
		return this->_item->size;
	}

	qse_size_t getCapacity () const 
	{
		return this->_item->capacity;
	}

	operator const CHAR_TYPE* () const 
	{
		return this->_item->buffer;
	}

	const CHAR_TYPE* getBuffer() const
	{
		return this->_item->buffer;
	}

	qse_size_t getHashCode () const
	{
		// keep this in sync with getHashCode of BasePtrString<CHAR_TYPE>
		return Hashable::getHashCode (
			this->_item->buffer, this->_item->size * QSE_SIZEOF(CHAR_TYPE));
	}

	bool operator== (const SelfType& str) const
	{
		if (this->_item->size != str._item->size) return false;
		return this->_opset.compare(this->_item->buffer, str._item->buffer, this->_item->size) == 0;
	}

	bool operator!= (const SelfType& str) const
	{
		return !this->operator== (str);
	}

	bool operator== (const CHAR_TYPE* str) const
	{
		return this->_opset.compare (this->_item->buffer, this->_item->size, str) == 0;
	}

	bool operator!= (const CHAR_TYPE* str) const
	{
		return this->_opset.compare (this->_item->buffer, this->_item->size, str) != 0;
	}

	// i don't want the caller to be able to change the character
	// at the given index. i don't provide non-const operator[].
	const CHAR_TYPE& operator[] (qse_size_t index) const
	{
		QSE_ASSERT (index < this->_item->size);
		//if (this->_item->isShared()) this->possess_data ();
		return this->_item->buffer[index];
	}

	const CHAR_TYPE& getCharAt (qse_size_t index) const 
	{
		QSE_ASSERT (index < this->_item->size);
		//if (this->_item->isShared()) this->possess_data ();
		return this->_item->buffer[index];
	}

	void setCharAt (qse_size_t index, CHAR_TYPE c)
	{
		QSE_ASSERT (index < this->_item->size);
		if (this->_item->isShared()) this->possess_data ();
		this->_item->buffer[index] = c;
	}

	SelfType getSubstring (qse_size_t index) const
	{
		QSE_ASSERT (index < this->_item->size);
		return SelfType (this->_item->buffer + index, this->_item->size - index);
	}

	SelfType getSubstring (qse_size_t index, qse_size_t size) const
	{
		QSE_ASSERT (index + size <= this->_item->size);
		return SelfType (this->_item->buffer + index, size);
	}

	//
	// TODO: comparison, hash, trim, case-converting, etc
	// utf-8 encoding/decoding
	//
	void insert (qse_size_t index, const CHAR_TYPE* str, qse_size_t size)
	{
		if (size <= 0) return;
		if (index >= this->_item->size) index = this->_item->size;
	
		//
		// When the same instance is inserted as in n.insert(index, n) which
		// finally calls n.insert(index. n.this->_item->buffer, 0, n.this->_item->size),
		// if n is not shared and should be copied, calling deref to it 
		// immediately after it's copied will destroy n.data refered to by
		// str/size. So the deref must be called after copying is done.
		//
	
		StringItem* old_item = QSE_NULL;
		qse_size_t new_size = this->_item->size + size;

		if (this->_item->isShared()) 
		{
			StringItem* t;

			if (new_size > this->_item->capacity) 
				t = this->_item->copy (this->getMmgr(), this->adjust_desired_capacity(new_size));
			else 
				t = this->_item->copy (this->getMmgr());

			old_item = this->_item;
			this->_item = t;
			this->ref_item ();
		}
		else if (new_size > this->_item->capacity) 
		{
			StringItem* t = this->_item->copy (this->getMmgr(), this->adjust_desired_capacity(new_size));
			old_item = this->_item;
			this->_item = t;
			this->ref_item ();;
		}

		CHAR_TYPE* p = this->_item->buffer + index;
		this->_opset.move (p + size, p, this->_item->size - index);
		this->_opset.move (p, str, size);
		this->_item->buffer[new_size] = NULL_CHAR;
		this->_item->size = new_size;

		if (old_item) this->deref_item (old_item);
	}

	void insert (qse_size_t index, const CHAR_TYPE* str)
	{
		this->insert (index, str, this->_opset.getLength(str));
	}

	void insert (qse_size_t index, const CHAR_TYPE c)
	{
		this->insert (index, &c, 1);
	}

	void insert (qse_size_t index, const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= str._item->size);
		this->insert (index, str._item->buffer + offset, size);
	}

	void insert (qse_size_t index, const SelfType& str)
	{
		this->insert (index, str._item->buffer, str._item->size);
	}

	void prepend (const CHAR_TYPE* str, qse_size_t size)
	{
		this->insert (0, str, size);
	}

	void prepend (const CHAR_TYPE* str)
	{
		this->insert (0, str);
	}

	void prepend (const CHAR_TYPE c)
	{
		this->insert (0, c);
	}

	void prepend (const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		this->insert (0, str, offset, size);
	}

	void prepend (const SelfType& str)
	{
		this->insert (0, str);
	}

	void append (const CHAR_TYPE* str, qse_size_t size)
	{
		this->insert (this->_item->size, str, size);
	}

	void append (const CHAR_TYPE* str)
	{
		this->insert (this->_item->size, str);
	}

	void append (const CHAR_TYPE c)
	{
		this->insert (this->_item->size, c);
	}

	void append (const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		this->insert (this->_item->size, str, offset, size);
	}

	void append (const SelfType& str)
	{
		this->insert (this->_item->size, str);
	}

	SelfType& operator+= (const SelfType& str)
	{
		this->insert (this->_item->size, str._item->buffer, str._item->size);
		return *this;
	}

	SelfType& operator+= (const CHAR_TYPE* str)
	{
		this->insert (this->_item->size, str);
		return *this;
	}

	SelfType& operator+= (const CHAR_TYPE c)
	{
		this->insert (this->_item->size, &c, 1);
		return *this;
	}

	void update (const CHAR_TYPE* str, qse_size_t size)
	{
		this->clear ();
		this->insert (0, str, size);
	}

	/// The update() function updates the entire string by copying a new 
	/// null-terminated string pointed to by \a str.
	void update (const CHAR_TYPE* str)
	{
		this->clear ();
		this->insert (0, str);
	}

	void update (const CHAR_TYPE c)
	{
		this->clear ();
		this->insert (0, c);
	}

	void update (const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		this->clear ();
		this->insert (0, str, offset, size);
	}

	void update (const SelfType& str)
	{
		this->operator= (str);
	}

	/// The update() function replaces a \a size substring staring from the \a offset
	/// with a new \a ssize string pointed to by \a str starign from the \a soffset.
	void update (qse_size_t index, qse_size_t size, const CHAR_TYPE* str, qse_size_t ssize)
	{
		this->remove (index, size);
		this->insert (index, str, ssize);
	}

	void update (qse_size_t index, qse_size_t size, const CHAR_TYPE* str)
	{
		this->remove (index, size);
		this->insert (index, str, this->_opset.getLength(str));
	}

	void update (qse_size_t index, qse_size_t size, const SelfType& str, qse_size_t soffset, qse_size_t ssize)
	{
		this->remove (index, size);
		this->insert (index, str, soffset, ssize);
	}

	void update (qse_size_t index, qse_size_t size, const SelfType& str)
	{
		this->remove (index, size);
		this->insert (index, str);
	}

	void remove (qse_size_t index, qse_size_t size)
	{
		if (size <= 0) return;
		if (index >= this->_item->size) return;
		if (size > this->_item->size - index) size = this->_item->size - index;

		if (this->_item->isShared()) this->possess_data ();

		CHAR_TYPE* p = this->_item->buffer + index;

		// +1 for the terminating null.
		this->_opset.move (p, p + size, this->_item->size - index - size + 1);
		this->_item->size -= size;
	}

	void clear ()
	{
		this->remove (0, this->_item->size);
	}

	/// The compact() function compacts the internal buffer to the length of
	/// the actual string.
	void compact ()
	{
		if (this->getSize() < this->getCapacity())
		{
			// call possess_data() regardless of this->_item->isShared()
			this->possess_data (this->getSize());
		}
	}

	void invert (qse_size_t index, qse_size_t size)
	{
		QSE_ASSERT (index + size <= this->_item->size);
	
		if (this->_item->isShared())  this->possess_data ();

		CHAR_TYPE c;
		qse_size_t i = index + size;
		for (qse_size_t j = index; j < --i; j++) 
		{
			c = this->_item->buffer[j];
			this->_item->buffer[j] = this->_item->buffer[i];
			this->_item->buffer[i] = c;
		}
	}

	void invert ()
	{
		this->invert (0, this->_item->size);
	}

	qse_size_t findIndex (qse_size_t index, const CHAR_TYPE* str, qse_size_t offset, qse_size_t size) const
	{
		if (size == 0) return index;
		if (size > this->_item->size) return INVALID_INDEX;
		if (index >= this->_item->size) return INVALID_INDEX;
	
		/*
		CHAR_TYPE first = str[offset];
		qse_size_t i = index;
		qse_size_t max = this->_item->size - size;
		CHAR_TYPE* p = this->_item->buffer;
	
	loop_findIndex:
		while (i <= max && p[i] != first) i++;
		if (i > max) return INVALID_INDEX;

		qse_size_t j = i + 1;
		qse_size_t end = j + size - 1;
		qse_size_t k = offset + 1;
		while (j < end) {
			if (p[j++] != str[k++]) {
				i++;
				goto loop_findIndex;
			}
		}
		return i;
		*/
	
		CHAR_TYPE first = str[offset];
		CHAR_TYPE* s1 = this->_item->buffer + index;
		CHAR_TYPE* e1 = this->_item->buffer + this->_item->size - size;
		CHAR_TYPE* p1 = s1;
	
	loop_findIndex:
		while (p1 <= e1 && *p1 != first) p1++;
		if (p1 > e1) return INVALID_INDEX;
	
		const CHAR_TYPE* s2 = str + offset + 1;
	
		CHAR_TYPE* p2 = p1 + 1;
		CHAR_TYPE* e2 = p2 + size - 1;
	
		while (p2 < e2) 
		{
			if (*p2++ != *s2++) 
			{
				p1++;
				goto loop_findIndex;
			}
		}

		return p1 - this->_item->buffer;
	}

	qse_size_t findIndex (qse_size_t index, const CHAR_TYPE* str) const
	{
		return this->findIndex (index, str, 0, this->_opset.getLength(str));
	}

	qse_size_t findIndex (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size) const
	{
		return this->findIndex (0, str, offset, size);
	}

	qse_size_t findIndex (const CHAR_TYPE* str) const
	{
		return this->findIndex (0, str);
	}

	qse_size_t findIndex (qse_size_t index, const SelfType& str, qse_size_t offset, qse_size_t size) const
	{
		QSE_ASSERT (offset + size <= str._item->size);
		return this->findIndex (index, str._item->buffer, offset, size);
	}

	qse_size_t findIndex (qse_size_t index, const SelfType& str) const
	{
		return this->findIndex (index, str._item->buffer, 0, str._item->size);
	}

	qse_size_t findIndex (const SelfType& str, qse_size_t offset, qse_size_t size) const
	{
		QSE_ASSERT (offset + size <= str._item->size);
		return this->findIndex (0, str._item->buffer, offset, size);
	}

	qse_size_t findIndex (const SelfType& str) const
	{
		return this->findIndex (0, str._item->buffer, 0, str._item->size);
	}

	qse_size_t findIndex (qse_size_t index, CHAR_TYPE c) const
	{
		if (index >= this->_item->size) return INVALID_INDEX;

		CHAR_TYPE* s = this->_item->buffer + index;
		CHAR_TYPE* e = this->_item->buffer + this->_item->size;
	
		for (CHAR_TYPE* p = s; p < e; p++) 
		{
			if (*p == c) return p - s;
		}
	
		return INVALID_INDEX;
	}

	qse_size_t findIndex (CHAR_TYPE c) const
	{
		return this->findIndex (0, c);
	}

	qse_size_t findLastIndex (qse_size_t index, const CHAR_TYPE* str, qse_size_t offset, qse_size_t size) const
	{
		if (size == 0) return index;
		if (size > this->_item->size) return INVALID_INDEX;
		if (index >= this->_item->size) index = this->_item->size - 1;

		qse_size_t str_last = offset + size - 1;
		CHAR_TYPE last = str[str_last];
		qse_size_t min = size - 1;
		qse_size_t i = min + index;
		CHAR_TYPE* p = this->_item->buffer;

	loop_findLastIndex:
		while (i >= min && p[i] != last) 
		{
			if (i <= min) return INVALID_INDEX;
			i--;
		}

		qse_size_t j = i - 1;
		qse_size_t start = j - size + 1;
		qse_size_t k = str_last - 1;
		while (j > start) 
		{
			if (p[j--] != str[k--]) 
			{
				i--;
				goto loop_findLastIndex;
			}
		}
		return start + 1;
	}

	qse_size_t findLastIndex (qse_size_t index, const CHAR_TYPE* str) const
	{
		return this->findLastIndex (index, str, 0, this->_opset.getLength(str));
	}

	qse_size_t findLastIndex (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size) const
	{
		return this->findLastIndex (this->_item->size - 1, str, offset, size);
	}

	qse_size_t findLastIndex (const CHAR_TYPE* str) const
	{
		return this->findLastIndex (this->_item->size - 1, str);
	}

	qse_size_t findLastIndex (qse_size_t index, const SelfType& str, qse_size_t offset, qse_size_t size) const
	{
		QSE_ASSERT (offset + size <= str._item->size);
		return this->findLastIndex (index, str._item->buffer, offset, size);
	}

	qse_size_t findLastIndex (qse_size_t index, const SelfType& str) const
	{
		return this->findLastIndex (index, str._item->buffer, 0, str._item->size);
	}

	qse_size_t findLastIndex (const SelfType& str, qse_size_t offset, qse_size_t size) const
	{
		QSE_ASSERT (offset + size <= str._item->size);
		return this->findLastIndex (this->_item->size - 1, str._item->buffer, offset, size);
	}

	qse_size_t findLastIndex (const SelfType& str) const
	{
		return this->findLastIndex (this->_item->size - 1, str._item->buffer, 0, str._item->size);
	}

	qse_size_t findLastIndex (qse_size_t index, CHAR_TYPE c) const
	{
		if (index >= this->_item->size) index = this->_item->size - 1;

		CHAR_TYPE* s = this->_item->buffer;
		CHAR_TYPE* e = this->_item->buffer + index;

		for (CHAR_TYPE* p = e; p >= s; p--) 
		{
			if (*p == c) return p - s;
		}

		return INVALID_INDEX;
	}

	qse_size_t findLastIndex (CHAR_TYPE c) const
	{
		return this->findLastIndex (this->_item->size - 1, c);
	}

	/// The replace() function finds all occurrences of a substring \a str1 
	/// and replace them by a new string \a str2.
	void replace (qse_size_t index, const CHAR_TYPE* str1, const CHAR_TYPE* str2)
	{
		qse_size_t len1 = this->_opset.getLength(str1);
		qse_size_t len2 = this->_opset.getLength(str2);
		while ((index = this->findIndex(index, str1, 0, len1)) != INVALID_INDEX) 
		{
			this->update (index, len1, str2, 0, len2);
			index += len2;
		}
	}

	void replace (const CHAR_TYPE* str1, const CHAR_TYPE* str2)
	{
		this->replace (0, str1, str2);
	}

	void replace (qse_size_t index, const SelfType& str1, const SelfType& str2)
	{
		while ((index = this->findIndex(index, str1)) != INVALID_INDEX) 
		{
			this->update (index, str1.data->data->size, str2);
			index += str2.data->data->size;
		}
	}

	void replace (const SelfType& str1, const SelfType& str2)
	{
		this->replace (0, str1, str2);
	}

	bool beginsWith (const CHAR_TYPE* str) const
	{
		return this->_opset.beginsWith(this->_item->buffer, this->_item->size, str);
	}

	bool beginsWith (const CHAR_TYPE* str, const qse_size_t len) const
	{
		return this->_opset.beginsWith(this->_item->buffer, this->_item->size, str, len);
	}

	bool endsWith (const CHAR_TYPE* str) const
	{
		return this->_opset.endsWith(this->_item->buffer, this->_item->size, str);
	}

	bool endsWith (const CHAR_TYPE* str, const qse_size_t len) const
	{
		return this->_opset.endsWith(this->_item->buffer, this->_item->size, str, len);
	}

	void trim ()
	{
		if (this->_item->isShared()) this->possess_data ();
		this->_item->size = this->_opset.trim(this->_item->buffer, this->_item->size, true, true);
	}

	void trimLeft ()
	{
		if (this->_item->isShared()) this->possess_data ();
		this->_item->size = this->_opset.trim(this->_item->buffer, this->_item->size, true, false);
	}

	void trimRight ()
	{
		if (this->_item->isShared()) this->possess_data ();
		this->_item->size = this->_opset.trim(this->_item->buffer, this->_item->size, false, true);
	}

protected:
	mutable StringItem* _item;
	OPSET _opset;
	RESIZER _resizer;


private:
	qse_size_t adjust_desired_capacity (qse_size_t new_desired_capacity)
	{
		qse_size_t new_capacity = this->_resizer(this->_item->capacity, new_desired_capacity, this->getGrowthPolicy());
		new_capacity = this->round_capacity(new_capacity);
		return new_capacity;
	}

};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////


#endif
