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

	StrBaseData (Mmgr* mmgr, qse_size_t capacity, const CHAR_TYPE* str, qse_size_t offset, qse_size_t size): 
		buffer (QSE_NULL) // set buffer to QSE_NULL here in case operator new rasises an exception
	{
		if (capacity < size) capacity = size;

		// I assume that CHAR_TYPE is a plain type.
		this->buffer = (CHAR_TYPE*)::operator new ((capacity + 1) * QSE_SIZEOF(CHAR_TYPE), mmgr);
		// So no constructor calling is needed.
		//for (qse_size_t i = 0; i < capacity + 1; i++)
		//{
		//	new (mmgr, &this->buffer[i]) CHAR_TYPE ();
		//}

		this->capacity = capacity;
		this->size = this->opset.copy (this->buffer, str + offset, size);
		QSE_ASSERT (this->size == size);
	}

	StrBaseData (Mmgr* mmgr, qse_size_t capacity, CHAR_TYPE c, qse_size_t size): 
		buffer (QSE_NULL) // set buffer to QSE_NULL here in case operator new rasises an exception
	{
		if (capacity < size) capacity = size;
		this->buffer = (CHAR_TYPE*)::operator new ((capacity + 1) * QSE_SIZEOF(CHAR_TYPE), mmgr);

		this->capacity = capacity;
		this->buffer[size] = NULL_CHAR;
		this->size = size;
		while (size > 0) this->buffer[--size] = c;
	}

	void dispose (Mmgr* mmgr)
	{
		QSE_ASSERT (this->buffer != QSE_NULL);
		::operator delete (this->buffer, mmgr);
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
		return new(mmgr) SelfType (mmgr, this->capacity, this->buffer, 0, this->size);
	}

	SelfType* copy (Mmgr* mmgr, qse_size_t capacity)
	{
		return new(mmgr) SelfType (mmgr, capacity, this->buffer, 0, this->size);
	}

#if 0
	void growBy (Mmgr* mmgr, qse_size_t inc)
	{
		if (inc > 0)
		{
			qse_size_t newcapa = this->capacity + inc;
			CHAR_TYPE* tmp = ::operator new ((newcapa + 1) * QSE_SIZEOF(CHAR_TYPE), mmgr);

			qse_size_t cursize = this->size;
			this->size = this->opset.copy (tmp, this->buffer, cursize);
			QSE_ASSERT (this->size == cursize);

			::operator delete (this->buffer, mmgr);

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
	qse_size_t operator() (qse_size_t current, qse_size_t desired) const
	{
		qse_size_t new_size;

		new_size = (current < 5000)?   (current + current):
		           (current < 50000)?  (current + (current / 2)):
		           (current < 100000)? (current + (current / 4)):
		           (current < 150000)? (current + (current / 8)):
		                               (current + (current / 16));

		if (new_size < desired) new_size = desired;
		return new_size;
	}
};

template <typename CHAR_TYPE, CHAR_TYPE NULL_CHAR, typename OPSET, typename RESIZER = StrBaseResizer>
class StrBase: public Mmged, public Hashable
{
public:
	enum 
	{
		DEFAULT_CAPACITY = 128,
		INVALID_INDEX = ~(qse_size_t)0
	};

/*
	class GrowthPolicy 
	{
	public:
		enum Type
		{
			ABSOLUTE,
			PERCENT
		};

		GrowthPolicy (Type type = ABSOLUTE, qse_size_t value = 0): type (type), value (value) {}

		Type type;
		qse_size_t value;
	};
*/

	typedef StrBase<CHAR_TYPE,NULL_CHAR,OPSET,RESIZER> SelfType;
	typedef StrBaseData<CHAR_TYPE,NULL_CHAR,OPSET,RESIZER> StringItem;

	/// The StrBase() function creates an empty string with the default memory manager.
	StrBase (): Mmged(QSE_NULL)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(DEFAULT_CAPACITY), QSE_NULL, 0, 0);
		this->ref_item ();
	}

	/// The StrBase() function creates an empty string with a memory manager \a mmgr.
	StrBase (Mmgr* mmgr): Mmged(mmgr)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(DEFAULT_CAPACITY), QSE_NULL, 0, 0);
		this->ref_item ();
	}

	StrBase (qse_size_t capacity): Mmged(QSE_NULL)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(capacity), QSE_NULL, 0, 0);
		this->ref_item ();
	}

	StrBase (Mmgr* mmgr, qse_size_t capacity): Mmged(mmgr)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(capacity), QSE_NULL, 0, 0);
		this->ref_item ();
	}

	StrBase (const CHAR_TYPE* str): Mmged(QSE_NULL)
	{
		qse_size_t len = this->opset.getLength(str);
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(len), str, 0, len);
		this->ref_item ();
	}

	StrBase (Mmgr* mmgr, const CHAR_TYPE* str): Mmged(mmgr)
	{
		qse_size_t len = this->_opset.getLength(str);
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(len), str, 0, len);
		this->ref_item ();
	}

	StrBase (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size): Mmged(QSE_NULL)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(size), str, offset, size);
		this->ref_item ();
	}

	StrBase (Mmgr* mmgr, const CHAR_TYPE* str, qse_size_t offset, qse_size_t size): Mmged(mmgr)
	{
		this->_item = new(this->getMmgr()) StringItem (this->getMmgr(), this->round_capacity(size), str, offset, size);
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

#if 0
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
		this->insert (0, &c, 0, 1);
		return *this;
	}
#endif

protected:
	void ref_item () const
	{
		this->_item->ref ();
	}

	void deref_item (StringItem* sd) const
	{
		if (sd->deref () <= 0)
		{
			sd->dispose (this->getMmgr());
			sd->~StringItem ();
			::operator delete (sd, this->getMmgr());
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
		return this->_opset.compare (this->_item->buffer, this->_item_size, str) == 0;
	}

	bool operator!= (const CHAR_TYPE* str) const
	{
		return this->_opset.compare (this->_item->buffer, this->_item_size, str) != 0;
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

	//
	// TODO: comparison, hash, trim, case-converting, etc
	// utf-8 encoding/decoding
	//
	void insert (qse_size_t index, const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		if (size <= 0) return;
		if (index >= this->_item->size) index = this->_item->size;
	
		//
		// When the same instance is inserted as in n.insert(index, n) which
		// finally calls n.insert(index. n.this->_item->buffer, 0, n.this->_item->size),
		// if n is not shared and should be copied, calling deref to it 
		// immediately after it's copied will destroy n.data refered to by
		// str/offset/size. So the deref must be called after copying is
		// done.
		//
	
		StringItem* old_item = QSE_NULL;
		qse_size_t new_size = this->_item->size + size;

		if (this->_item->isShared()) 
		{
			StringItem* t;

			if (new_size > this->_item->capacity) 
				t = this->_item->copy (this->getMmgr(), this->adjust_new_capacity(new_size));
			else 
				t = this->_item->copy (this->getMmgr());

			old_item = this->_item;
			this->_item = t;
			this->ref_item ();
		}
		else if (new_size > this->_item->capacity) 
		{
			StringItem* t = this->_item->copy (this->getMmgr(), this->adjust_new_capacity(new_size));
			old_item = this->_item;
			this->_item = t;
			this->ref_item ();;
		}

		CHAR_TYPE* p = this->_item->buffer + index;
		this->_opset.move (p + size, p, this->_item->size - index);
		this->_opset.move (p, str + offset, size);
		this->_item->buffer[new_size] = NULL_CHAR;
		this->_item->size = new_size;

		if (old_item) this->deref_item (old_item);
	}

	void insert (qse_size_t index, const CHAR_TYPE* str)
	{
		this->insert (index, str, 0, this->_opset.getLength(str));
	}

	void insert (qse_size_t index, const CHAR_TYPE c)
	{
		this->insert (index, &c, 0, 1);
	}

	void insert (qse_size_t index, const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= str._item->size);
		this->insert (index, str._item->buffer, offset, size);
	}

	void insert (qse_size_t index, const SelfType& str)
	{
		this->insert (index, str._item->buffer, 0, str._item->size);
	}

	void prepend (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		this->insert (0, str, offset, size);
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

	void append (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		this->insert (this->_item->size, str, offset, size);
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
		this->insert (this->_item->size, str._item->buffer, 0, str._item->size);
		return *this;
	}

	SelfType& operator+= (const CHAR_TYPE* str)
	{
		this->insert (this->_item->size, str);
		return *this;
	}

	SelfType& operator+= (const CHAR_TYPE c)
	{
		this->insert (this->_item->size, &c, 0, 1);
		return *this;
	}

#if 0
	void appendFormat (const CHAR_TYPE* fmt, ...)
	{
		/*
		int n;
		if (this->_item->isShared()) 
		{
			StringItem* t = this->_item->copy ();
			this->_item->deref (); this->_item = t; this->_item->ref ();
		}
		qse_va_start (ap, fmt);
		while ((n = SelfType::opset.vsprintf (&this->_item->buffer[this->_item->size], this->_item->capacity - this->_item->size, fmt, ap)) <= -1)
		{
			this->_item->growBy (calc_new_inc_for_growth (0));
			qse_va_end (ap);
			qse_va_start (ap, fmt);
		}
		qse_va_end (ap);
		this->_item->size += n;
		*/
		qse_va_list ap;
		qse_va_start (ap, fmt);
		this->appendFormat (fmt, ap);
		qse_va_end (ap);
	}

	void appendFormat (const CHAR_TYPE* fmt, qse_va_list ap)
	{
		int n;
		qse_va_list save_ap;

		if (this->_item->isShared()) 
		{
			StringItem* t = this->_item->copy ();
			this->_item->deref (); this->_item = t; this->_item->ref ();
		}

		qse_va_copy (save_ap, ap);
		while ((n = SelfType::opset.vsprintf (&this->_item->buffer[this->_item->size], this->_item->capacity - this->_item->size, fmt, ap)) <= -1)
		{
			this->_item->growBy (calc_new_inc_for_growth (0));
			qse_va_copy (ap, save_ap);
		}
		
		this->_item->size += n;
	}

#endif

	
	void update (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		this->clear ();
		this->insert (0, str, offset, size);
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
	void update (qse_size_t offset, qse_size_t size, const CHAR_TYPE* str, qse_size_t soffset, qse_size_t ssize)
	{
		this->remove (offset, size);
		this->insert (offset, str, soffset, ssize);
	}

	void update (qse_size_t offset, qse_size_t size, const CHAR_TYPE* str)
	{
		this->remove (offset, size);
		this->insert (offset, str, 0, this->_opset.getLength(str));
	}

	void update (qse_size_t offset, qse_size_t size, const SelfType& str, qse_size_t soffset, qse_size_t ssize)
	{
		this->remove (offset, size);
		this->insert (offset, str, soffset, ssize);
	}

	void update (qse_size_t offset, qse_size_t size, const SelfType& str)
	{
		this->remove (offset, size);
		this->insert (offset, str);
	}

	void remove (qse_size_t offset, qse_size_t size)
	{
		if (size <= 0) return;
		if (offset >= this->_item->size) return;
		if (size > this->_item->size - offset) size = this->_item->size - offset;

		if (this->_item->isShared()) this->possess_data ();

		CHAR_TYPE* p = this->_item->buffer + offset;

		// +1 for the terminating null.
		this->_opset.move (p, p + size, this->_item->size - offset - size + 1);
		this->_item->size -= size;
	}

	void remvoe ()
	{
		this->remove (0, this->_item->size);
	}

	void clear ()
	{
		this->remove (0, this->_item->size);
	}


	void invert (qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= this->_item->size);
	
		if (this->_item->isShared())  this->possess_data ();

		CHAR_TYPE c;
		qse_size_t i = offset + size;
		for (qse_size_t j = offset; j < --i; j++) 
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

	SelfType getSubstring (qse_size_t offset) const
	{
		QSE_ASSERT (offset < this->_item->size);
		return SelfType (this->_item->buffer, offset, this->_item->size - offset);
	}

	SelfType getSubstring (qse_size_t offset, qse_size_t size) const
	{
		QSE_ASSERT (offset + size <= this->_item->size);
		return SelfType (this->_item->buffer, offset, size);
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

	/// The replace() function finds a substring \a str1 and replace it by
	/// a new string \a str2.
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

#if 0
	bool beginsWith (const CHAR_TYPE* str) const
	{
		qse_size_t idx = 0;
		while (*str != NULL_CHAR) 
		{
			if (idx >= this->_item->size) return false;
			if (this->_item->buffer[idx] != *str) return false;
			idx++; str++;
		}
		return true;
	}

	bool beginsWith (const CHAR_TYPE* str, const qse_size_t len) const
	{
		const CHAR_TYPE* end = str + len;
		qse_size_t idx = 0;

		while (str < end) 
		{
			if (idx >= this->_item->size) return false;
			if (this->_item->buffer[idx] != *str) return false;
			idx++; str++;
		}
		return true;
	}

	qse_size_t touppercase ()
	{
		if (this->_item->isShared()) this->possess_data();
		return SelfType::touppercase (this->_item->buffer);
	}

	qse_size_t tolowercase ()
	{
		if (this->_item->isShared()) this->possess_data ();
		return SelfType::tolowercase (this->_item->buffer);
	}

	qse_size_t trim ()
	{
		if (this->_item->isShared()) this->possess_data ();
		this->_item->size = SelfType::trim (this->_item->buffer);
		return this->_item->size;
	}

#endif

protected:
	mutable StringItem* _item;
	OPSET _opset;
	RESIZER _resizer;

private:
	qse_size_t round_capacity (qse_size_t n) 
	{
		if (n == 0) n = 1;
		return (n + (qse_size_t)DEFAULT_CAPACITY - 1) & 
		       ~((qse_size_t)DEFAULT_CAPACITY - (qse_size_t)1);
	}

#if 0
	qse_size_t calc_new_inc_for_growth (qse_size_t desired_inc)
	{
		qse_size_t inc ;

		
		/*
		switch (this->growth_policy.type)
		{
			case GrowthPolicy::PERCENT:
				inc = (this->_item->size * this->growth_policy.value) / 100;
				break;

			case GrowthPolicy::ABSOLUTE:
				inc = this->growth_policy.value;
				break;

			default:
				inc = DEFAULT_CAPACITY;
				break;
		}
		*/

		if (inc <= 0) inc = 1;
		if (inc < desired_inc) inc = desired_inc;
		return this->round_capacity (inc);
	}
#endif

	qse_size_t adjust_new_capacity (qse_size_t new_desired_capacity)
	{
		return this->round_capacity(this->_resizer(this->_item->capacity, new_desired_capacity));
	}

};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////


#endif
