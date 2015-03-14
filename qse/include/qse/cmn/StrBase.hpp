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

template <typename CHAR_TYPE, CHAR_TYPE NULL_CHAR, typename OPSET> class StrBase;

template <typename CHAR_TYPE, CHAR_TYPE NULL_CHAR, typename OPSET>
class StrBaseData: public RefCounted
{
protected:
	friend class StrBase<CHAR_TYPE,NULL_CHAR,OPSET>;

	typedef StrBaseData<CHAR_TYPE,NULL_CHAR,OPSET> SelfType;

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
		return new SelfType (mmgr, this->capacity, this->buffer, 0, this->size);
	}

	SelfType* copy (Mmgr* mmgr, qse_size_t capacity)
	{
		return new SelfType (mmgr, capacity, this->buffer, 0, this->size);
	}

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

	CHAR_TYPE* buffer;
	qse_size_t capacity;
	qse_size_t size;
	OPSET      opset;
};

struct StrBaseNewSize
{
	qse_size_t operator () (qse_size_t old_size, qse_size_t desired_size) const
	{
		return desired_size;
	}
};

template <typename CHAR_TYPE, CHAR_TYPE NULL_CHAR, typename OPSET = StrBaseNewSize>
class StrBase: public Mmged, public Hashable
{
public:
	enum 
	{
		DEFAULT_CAPACITY = 128,
		INVALID_INDEX = ~(qse_size_t)0
	};

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

	typedef StrBase<CHAR_TYPE,NULL_CHAR,OPSET> SelfType;
	typedef StrBaseData<CHAR_TYPE,NULL_CHAR,OPSET> StringData;

	/// The StrBase() function creates an empty string with the default memory manager.
	StrBase (): Mmged(QSE_NULL)
	{
		this->data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(DEFAULT_CAPACITY), QSE_NULL, 0, 0);
		this->ref_data ();
	}

	/// The StrBase() function creates an empty string with a memory manager \a mmgr.
	StrBase (Mmgr* mmgr): Mmged(mmgr)
	{
		this->data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(DEFAULT_CAPACITY), QSE_NULL, 0, 0);
		this->ref_data ();
	}

	StrBase (qse_size_t capacity): Mmged(QSE_NULL)
	{
		this->data = data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(capacity), QSE_NULL, 0, 0);
		this->ref_data ();
	}

	StrBase (Mmgr* mmgr, qse_size_t capacity): Mmged(mmgr)
	{
		this->data = data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(capacity), QSE_NULL, 0, 0);
		this->ref_data ();
	}

	StrBase (const CHAR_TYPE* str): Mmged(QSE_NULL)
	{
		qse_size_t len = this->opset.length(str);
		this->data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(len), str, 0, len);
		this->ref_data ();
	}

	StrBase (Mmgr* mmgr, const CHAR_TYPE* str): Mmged(mmgr)
	{
		qse_size_t len = this->opset.length(str);
		this->data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(len), str, 0, len);
		this->ref_data ();
	}

	StrBase (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size): Mmged(QSE_NULL)
	{
		this->data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(size), str, offset, size);
		this->ref_data ();
	}

	StrBase (Mmgr* mmgr, const CHAR_TYPE* str, qse_size_t offset, qse_size_t size): Mmged(mmgr)
	{
		this->data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(size), str, offset, size);
		this->ref_data ();
	}

	StrBase (CHAR_TYPE c, qse_size_t size): Mmged(QSE_NULL)
	{
		this->data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(size), c, size);
		this->ref_data ();
	}

	StrBase (Mmgr* mmgr, CHAR_TYPE c, qse_size_t size): Mmged(mmgr)
	{
		this->data = new(this->getMmgr()) StringData (this->getMmgr(), this->round_capacity(size), c, size);
		this->ref_data ();
	}

	StrBase (const SelfType& str): Mmged(str)
	{
		this->data = str.data;
		this->ref_data ();
	}

	~StrBase () 
	{
		this->deref_data ();
	}

	SelfType& operator= (const SelfType& str)
	{
		if (this->data != str.data) 
		{
			this->deref_data ();

			// the data to be reference could be allocated using the
			// memory manager of str. and it may be freed or resized by
			// this. so the inner memory manager must be switched.
			this->setMmgr (str.getMmgr()); // copy over mmgr.

			this->data = str.data;
			this->ref_data ();
		}
		return *this;
	}

#if 0
	SelfType& operator= (const CHAR_TYPE* str)
	{
		if (this->data->buffer != str)
		{
			this->remove ();
			this->insert (0, str);
		}
		return *this;
	}

	SelfType& operator= (const CHAR_TYPE c)
	{
		this->remove ();
		this->insert (0, &c, 0, 1);
		return *this;
	}
#endif

protected:
	void ref_data () const
	{
		this->data->ref ();
	}

	void deref_data (StringData* sd) const
	{
		if (sd->deref () <= 0)
		{
			sd->dispose (this->getMmgr());
			sd->~StringData ();
			::operator delete (sd, this->getMmgr());
		}
	}

	void deref_data () const
	{
		this->deref_data (this->data);
	}

	void possess_data () const
	{
		StringData* t = this->data->copy (this->getMmgr());
		this->deref_data ();
		this->data = t;
		this->ref_data ();
	}

public:
	const GrowthPolicy& growthPolicy () const
	{
		return this->growth_policy;
	}

	///
	/// The setGrowthPolicy() function sets how to grow the buffer capacity
	/// when more space is needed. 
	///
	/// The sample below doubles the capacity of the current buffer when necessary.
	/// \code
	///   xp::bas::String x;
	///   x.setGrowthPolicy (xp::bas::String::GrowthPolicy (xp::bas::String::GrowthPolicy::PERCENT, 100));
	///   for (int i = 0; i < 2000; i+=3)
	///   {
	///       x.appendFormat (QSE_T("%d %d %d "), i+1, i+2, i+3);
	///   }
	/// \endcode
	void setGrowthPolicy (const GrowthPolicy& pol)
	{
		this->growth_policy = pol;
	}

	qse_size_t getSize () const 
	{
		return this->data->size;
	}

	qse_size_t length () const 
	{
		return this->data->size;
	}

	qse_size_t getCapacity () const 
	{
		return this->data->capacity;
	}

	operator const CHAR_TYPE* () const 
	{
		return this->data->buffer;
	}

	const CHAR_TYPE* getBuffer() const
	{
		return this->data->buffer;
	}

	qse_size_t getHashCode () const
	{
		// keep this in sync with getHashCode of BasePtrString<CHAR_TYPE>
		return Hashable::getHashCode (
			this->data->buffer, this->data->size * QSE_SIZEOF(CHAR_TYPE));
	}

	

#if 0
	SelfType& operator+= (const SelfType& str)
	{
		this->insert (this->data->size, str->data->buffer, 0, str->data->size);
		return *this;
	}

	SelfType& operator+= (const CHAR_TYPE* str)
	{
		this->insert (this->data->size, str);
		return *this;
	}

	SelfType& operator+= (const CHAR_TYPE c)
	{
		this->insert (this->data->size, &c, 0, 1);
		return *this;
	}
#endif

	bool operator== (const SelfType& str) const
	{
		if (this->data->size != str.data->size) return false;
		return this->opset.compare(this->data->buffer, str.data->buffer, this->data->size) == 0;
	}

	bool operator!= (const SelfType& str) const
	{
		return !this->operator== (str);
	}

	bool operator== (const CHAR_TYPE* str) const
	{
		return this->opset.compare (this->data->buffer, this->data_size, str) == 0;
	}
	bool operator!= (const CHAR_TYPE* str) const
	{
		return this->opset.compare (this->data->buffer, this->data_size, str) != 0;
	}


	const CHAR_TYPE& operator[] (qse_size_t index)
	{
		QSE_ASSERT (index < this->data->size);

		if (this->data->isShared()) this->possess_data ();
		return this->data->buffer[index];
	}

	CHAR_TYPE& getCharAt (qse_size_t index) 
	{
		QSE_ASSERT (index < this->data->size);
		return this->data->buffer[index];
	}

	const CHAR_TYPE& getCharAt (qse_size_t index) const 
	{
		QSE_ASSERT (index < this->data->size);
		return this->data->buffer[index];
	}

	void setCharAt (qse_size_t index, CHAR_TYPE c)
	{
		QSE_ASSERT (index < this->data->size);
		if (this->data->isShared()) this->possess_data ();
		this->data->buffer[index] = c;
	}

#if 0
	//
	// TODO: comparison, hash, trim, case-converting, etc
	// utf-8 encoding/decoding
	//
	void insert (qse_size_t index, const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		if (size <= 0) return;
		if (index >= this->data->size) index = this->data->size;
	
		//
		// When the same instance is inserted as in n.insert(index, n) which
		// finally calls n.insert(index. n.this->data->buffer, 0, n.this->data->size),
		// if n is not shared and should be copied, calling deref to it 
		// immediately after it's copied will destroy n.data refered to by
		// str/offset/size. So the deref must be called after copying is
		// done.
		//
	
		StringData* old_data = QSE_NULL;
		qse_size_t new_size = this->data->size + size;
	
		if (this->data->isShared()) 
		{
			StringData* t;
			if (new_size > this->data->capacity) 
				t = this->data->copy (this->data->capacity + calc_new_inc_for_growth(new_size - this->data->capacity));
			else 
				t = this->data->copy ();
			//this->data->deref (); this->data = t; this->data->ref ();
			old_data = data;
			this->data = t;
			this->ref_data ();
		}
		else if (new_size > this->data->capacity) 
		{
			StringData* t = this->data->copy (this->data->capacity + calc_new_inc_for_growth(new_size - this->data->capacity));
			//this->data->deref (); this->data = t; this->data->ref ();
			old_data = data;
			this->data = t;
			this->ref_data ();;
		}
		
		CHAR_TYPE* p = this->data->buffer + index;
		qse_memmove (p + size, p, (this->data->size - index) * QSE_SIZEOF(CHAR_TYPE));
		qse_memcpy (p, str + offset, size * QSE_SIZEOF(CHAR_TYPE));
	
		this->data->size = new_size;
		this->data->buffer[new_size] = NULL_CHAR;
	
		if (old_data) this->deref_data (old_data);
	}

	void insert (qse_size_t index, const CHAR_TYPE* str)
	{
		this->insert (index, str, 0, SelfType::lengthOf(str));
	}

	void insert (qse_size_t index, const CHAR_TYPE c)
	{
		this->insert (index, &c, 0, 1);
	}

	void insert (qse_size_t index, const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= str->data->size);
		this->insert (index, str->data->buffer, offset, size);
	}

	void insert (qse_size_t index, const SelfType& str)
	{
		this->insert (index, str->data->buffer, 0, str->data->size);
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
		this->insert (this->data->size, str, offset, size);
	}

	void append (const CHAR_TYPE* str)
	{
		this->insert (this->data->size, str);
	}

	void append (const CHAR_TYPE c)
	{
		this->insert (this->data->size, c);
	}

	void append (const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		this->insert (this->data->size, str, offset, size);
	}

	void append (const SelfType& str)
	{
		this->insert (this->data->size, str);
	}

	void appendFormat (const CHAR_TYPE* fmt, ...)
	{
		/*
		int n;
		if (this->data->isShared()) 
		{
			StringData* t = this->data->copy ();
			this->data->deref (); this->data = t; this->data->ref ();
		}
		qse_va_start (ap, fmt);
		while ((n = SelfType::opset.vsprintf (&this->data->buffer[this->data->size], this->data->capacity - this->data->size, fmt, ap)) <= -1)
		{
			this->data->growBy (calc_new_inc_for_growth (0));
			qse_va_end (ap);
			qse_va_start (ap, fmt);
		}
		qse_va_end (ap);
		this->data->size += n;
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

		if (this->data->isShared()) 
		{
			StringData* t = this->data->copy ();
			this->data->deref (); this->data = t; this->data->ref ();
		}

		qse_va_copy (save_ap, ap);
		while ((n = SelfType::opset.vsprintf (&this->data->buffer[this->data->size], this->data->capacity - this->data->size, fmt, ap)) <= -1)
		{
			this->data->growBy (calc_new_inc_for_growth (0));
			qse_va_copy (ap, save_ap);
		}
		
		this->data->size += n;
	}

	void set (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		this->remove ();
		this->insert (0, str, offset, size);	
	}
	void set (const CHAR_TYPE* str)
	{
		this->remove ();
		this->insert (0, str);
	}
	void set (const CHAR_TYPE c)
	{
		this->remove ();
		this->insert (0, c);
	}
	void set (const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		this->remove ();
		this->insert (0, str, offset, size);
	}
	void set (const SelfType& str)
	{
		operator= (str);
	}

	void remove (qse_size_t offset, qse_size_t size)
	{
		if (size <= 0) return;
		if (offset >= this->data->size) return;
		if (size > this->data->size - offset) size = this->data->size - offset;

		if (this->data->isShared()) 
		{
			StringData* t = this->data->copy ();
			this->data->deref (); this->data = t; this->data->ref ();
		}

		CHAR_TYPE* p = this->data->buffer + offset;
		qse_memcpy (p, p + size, (this->data->size - offset - size + 1) * QSE_SIZEOF(CHAR_TYPE));
		this->data->size -= size;	
	}
	void remove ()
	{
		this->remove (0, this->data->size);
	}

	void invert (qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= this->data->size);
	
		if (this->data->isShared()) 
		{
			StringData* t = this->data->copy ();
			this->data->deref (); this->data = t; this->data->ref ();
		}
	
		CHAR_TYPE c;
		qse_size_t i = offset + size;
		for (qse_size_t j = offset; j < --i; j++) 
		{
			c = this->data->buffer[j];	
			this->data->buffer[j] = this->data->buffer[i];
			this->data->buffer[i] = c;
		}
	}
	void invert ()
	{
		this->invert (0, this->data->size);
	}

	void replace (qse_size_t offset, qse_size_t size, const CHAR_TYPE* str, qse_size_t soffset, qse_size_t ssize)
	{
		this->remove (offset, size);	
		this->insert (offset, str, soffset, ssize);
	}
	void replace (qse_size_t offset, qse_size_t size, const CHAR_TYPE* str)
	{
		this->remove (offset, size);
		this->insert (offset, str, 0, SelfType::lengthOf(str));
	}
	void replace (qse_size_t offset, qse_size_t size, const SelfType& str, qse_size_t soffset, qse_size_t ssize)
	{
		this->remove (offset, size);	
		this->insert (offset, str, soffset, ssize);
	}
	void replace (qse_size_t offset, qse_size_t size, const SelfType& str)
	{
		this->remove (offset, size);
		this->insert (offset, str);
	}

	void replaceAll (qse_size_t index, const CHAR_TYPE* str1, const CHAR_TYPE* str2)
	{
		qse_size_t len1 = SelfType::lengthOf(str1);
		qse_size_t len2 = SelfType::lengthOf(str2);
		while ((index = this->indexOf(index, str1, 0, len1)) != INVALID_INDEX) 
		{
			this->replace (index, len1, str2, 0, len2);
			index += len2;
		}
	}
	void replaceAll (const CHAR_TYPE* str1, const CHAR_TYPE* str2)
	{
		this->replaceAll (0, str1, str2);
	}
	void replaceAll (qse_size_t index, const SelfType& str1, const SelfType& str2)
	{
		while ((index = this->indexOf(index, str1)) != INVALID_INDEX) 
		{
			this->replace (index, str1.data->data->size, str2);
			index += str2.data->data->size;
		}
	}
	void replaceAll (const SelfType& str1, const SelfType& str2)
	{
		this->replaceAll (0, str1, str2);
	}

	SelfType substring (qse_size_t offset)
	{
		QSE_ASSERT (offset < this->data->size);
		return SelfType (this->data->buffer, offset, this->data->size - offset);
	}
	SelfType substring (qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= this->data->size);
		return SelfType (this->data->buffer, offset, size);
	}

	qse_size_t indexOf (qse_size_t index,
		const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		if (size == 0) return index;
		if (size > this->data->size) return INVALID_INDEX;
		if (index >= this->data->size) return INVALID_INDEX;
	
		/*
		CHAR_TYPE first = str[offset];
		qse_size_t i = index;
		qse_size_t max = this->data->size - size;
		CHAR_TYPE* p = this->data->buffer;
	
	loop_indexOf:
		while (i <= max && p[i] != first) i++;
		if (i > max) return INVALID_INDEX;
	
		qse_size_t j = i + 1;
		qse_size_t end = j + size - 1;
		qse_size_t k = offset + 1;
		while (j < end) {
			if (p[j++] != str[k++]) {
				i++;
				goto loop_indexOf;
			}
		}
		return i;
		*/
	
		CHAR_TYPE first = str[offset];
		CHAR_TYPE* s1 = this->data->buffer + index;
		CHAR_TYPE* e1 = this->data->buffer + this->data->size - size;
		CHAR_TYPE* p1 = s1;
	
	loop_indexOf:
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
				goto loop_indexOf;
			}
		}

		return p1 - this->data->buffer;
	}
	qse_size_t indexOf (qse_size_t index, const CHAR_TYPE* str)
	{
		return indexOf (index, str, 0, SelfType::lengthOf(str));
	}
	qse_size_t indexOf (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		return indexOf (0, str, offset, size);
	}
	qse_size_t indexOf (const CHAR_TYPE* str)
	{
		return indexOf (0, str);
	}
	qse_size_t indexOf (qse_size_t index, const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= str->data->size);
		return this->indexOf (index, str->data->buffer, offset, size);
	}
	qse_size_t indexOf (qse_size_t index, const SelfType& str)
	{
		return this->indexOf (index, str->data->buffer, 0, str->data->size);
	}
	qse_size_t indexOf (const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= str->data->size);
		return this->indexOf (0, str->data->buffer, offset, size);
	}
	qse_size_t indexOf (const SelfType& str)
	{
		return this->indexOf (0, str->data->buffer, 0, str->data->size);
	}
	qse_size_t indexOf (qse_size_t index, CHAR_TYPE c)
	{
		if (index >= this->data->size) return INVALID_INDEX;
	
		CHAR_TYPE* s = this->data->buffer + index;
		CHAR_TYPE* e = this->data->buffer + this->data->size;
	
		for (CHAR_TYPE* p = s; p < e; p++) 
		{
			if (*p == c) return p - s;
		}
	
		return INVALID_INDEX;
	}
	qse_size_t indexOf (CHAR_TYPE c)
	{
		return indexOf (0, c);
	}

	qse_size_t lastIndexOf (qse_size_t index,
		const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		if (size == 0) return index;
		if (size > this->data->size) return INVALID_INDEX;
		if (index >= this->data->size) index = this->data->size - 1;
	
		qse_size_t strLast = offset + size - 1;
		CHAR_TYPE last = str[strLast];
		qse_size_t min = size - 1;
		qse_size_t i = min + index;
		CHAR_TYPE* p = this->data->buffer;
	
	loop_lastIndexOf:
		while (i >= min && p[i] != last) i--;
		if (i < min) return INVALID_INDEX;
	
		qse_size_t j = i - 1;
		qse_size_t start = j - size + 1;
		qse_size_t k = strLast - 1;
		while (j > start) 
		{
			if (p[j--] != str[k--]) 
			{
				i--;
				goto loop_lastIndexOf;
			}
		}
		return start + 1;
	}
	qse_size_t lastIndexOf (qse_size_t index, const CHAR_TYPE* str)
	{
		return this->lastIndexOf (index, str, 0, SelfType::lengthOf(str));
	}
	qse_size_t lastIndexOf (const CHAR_TYPE* str, qse_size_t offset, qse_size_t size)
	{
		return this->lastIndexOf (this->data->size - 1, str, offset, size);
	}
	qse_size_t lastIndexOf (const CHAR_TYPE* str)
	{
		return this->lastIndexOf (this->data->size - 1, str);
	}
	qse_size_t lastIndexOf (qse_size_t index, const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= str->data->size);
		return this->lastIndexOf (index, str->data->buffer, offset, size);
	}
	qse_size_t lastIndexOf (qse_size_t index, const SelfType& str)
	{
		return this->lastIndexOf (index, str->data->buffer, 0, str->data->size);
	}
	qse_size_t lastIndexOf (const SelfType& str, qse_size_t offset, qse_size_t size)
	{
		QSE_ASSERT (offset + size <= str->data->size);
		return this->lastIndexOf (this->data->size - 1, str->data->buffer, offset, size);
	}
	qse_size_t lastIndexOf (const SelfType& str)
	{
		return this->lastIndexOf (this->data->size - 1, str->data->buffer, 0, str->data->size);
	}
	qse_size_t lastIndexOf (qse_size_t index, CHAR_TYPE c)
	{
		if (index >= this->data->size) index = this->data->size - 1;

		CHAR_TYPE* s = this->data->buffer;
		CHAR_TYPE* e = this->data->buffer + index;

		for (CHAR_TYPE* p = e; p >= s; p--) 
		{
			if (*p == c) return p - s;
		}

		return INVALID_INDEX;
	}
	qse_size_t lastIndexOf (CHAR_TYPE c)
	{
		return this->lastIndexOf (this->data->size - 1, c);
	}

	bool beginsWith (const CHAR_TYPE* str) const
	{
		qse_size_t idx = 0;
		while (*str != NULL_CHAR) {
			if (idx >= this->data->size) return false;
			if (this->data->buffer[idx] != *str) return false;
			idx++; str++;
		}
		return true;
	}

	bool beginsWith (const CHAR_TYPE* str, const qse_size_t len) const
	{
		const CHAR_TYPE* end = str + len;
		qse_size_t idx = 0;

		while (str < end) {
			if (idx >= this->data->size) return false;
			if (this->data->buffer[idx] != *str) return false;
			idx++; str++;
		}
		return true;
	}

	qse_size_t touppercase ()
	{
		if (this->data->isShared()) 
	{
			StringData* t = this->data->copy ();
			this->data->deref (); this->data = t; this->data->ref ();
		}

		return touppercase (this->data->buffer);
	}

	qse_size_t tolowercase ()
	{
		if (this->data->isShared()) 
		{
			StringData* t = this->data->copy ();
			this->data->deref (); this->data = t; this->data->ref ();
		}
		return tolowercase (this->data->buffer);
	}

	qse_size_t trim ()
	{
		if (this->data->isShared()) 
		{
			StringData* t = this->data->copy ();
			this->data->deref (); this->data = t; this->data->ref ();
		}

		this->data->size = SelfType::trim (this->data->buffer);
		return this->data->size;
	}

#endif

protected:
	mutable StringData* data;
	GrowthPolicy growth_policy;
	OPSET opset;

private:
	static qse_size_t round_capacity (qse_size_t n) 
	{
		if (n == 0) n = 1;
		return 
			(n + (qse_size_t)DEFAULT_CAPACITY - 1) & 
			~((qse_size_t)DEFAULT_CAPACITY - (qse_size_t)1);
	}

	qse_size_t calc_new_inc_for_growth (qse_size_t desired_inc)
	{
		qse_size_t inc ;
		switch (this->growth_policy.type)
		{
			case GrowthPolicy::PERCENT:
				inc = (this->data->size * this->growth_policy.value) / 100;
				break;

			case GrowthPolicy::ABSOLUTE:
				inc = this->growth_policy.value;
				break;

			default:
				inc = DEFAULT_CAPACITY;
				break;
		}	

		if (inc <= 0) inc = 1;
		if (inc < desired_inc) inc = desired_inc;
		return round_capacity (inc);
	}

};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////


#endif
