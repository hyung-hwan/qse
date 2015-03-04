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

#ifndef _QSE_CMN_ARRAY_HPP_
#define _QSE_CMN_ARRAY_HPP_

#include <qse/Types.hpp>
#include <qse/cmn/Mpool.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

struct ArrayResizer
{
	qse_size_t operator() (qse_size_t current) const
	{
		return (current < 5000)?   (current + current):
		       (current < 50000)?  (current + (current / 2)):
		       (current < 100000)? (current + (current / 4)):
		       (current < 150000)? (current + (current / 8)):
		                           (current + (current / 16));
	}
};

template <typename T, typename RESIZER = ArrayResizer>
class Array: public Mmged
{
public:
	typedef Array<T,RESIZER> SelfType;

	enum 
	{
		DEFAULT_CAPACITY = 128,
		INVALID_INDEX = ~(qse_size_t)0
	};

	Array (Mmgr* mmgr = QSE_NULL,
	            qse_size_t capacity = DEFAULT_CAPACITY, 
	            qse_size_t mpb_size = 0):
		Mmged (mmgr),
		mp (mmgr, QSE_SIZEOF(T), mpb_size)
	{
		if (capacity <= 0) 
		{
			this->buffer = QSE_NULL;
			this->capacity = 0;
		}
		else 
		{
			//this->buffer = new T[capacity];
			this->buffer = (T*)::operator new (capacity * QSE_SIZEOF(*this->buffer), &this->mp);
			this->capacity = capacity;
		}

		this->count  = 0;
	}

#if 0
	Array (const SelfType& array)
	{
		if (array.buffer == QSE_NULL) 
		{
			this->buffer = QSE_NULL;
			this->capacity = 0;
			this->grow_factor = array.grow_factor;
			this->count  = 0;
		}
		else 
		{
			T* tmp = QSE_NULL;
			QSE_ASSERT (array.capacity > 0 && array.grow_factor > 0);

			try {  
				tmp = new T[array.capacity];
				for (qse_size_t i = 0; i < array.this->count; i++) {
					tmp[i] = array.buffer[i];
				}
			}
			catch (...) {
				// just in case where the assignment throws an 
				// exception. when T is a class type, the operator = 
				// for the class may throw an exception.
				if (tmp != QSE_NULL) delete[] tmp;
				throw;
			}

			this->buffer = tmp;
			this->capacity = array.capacity;
			this->grow_factor = array.grow_factor;
			this->count  = array.this->count;
		}
	}
#endif

	~Array ()
	{
		if (this->buffer)
		{
			for (qse_size_t i = this->count; i > 0; )
			{
				--i;
				this->buffer[i].~T ();
			}

			::operator delete (this->buffer, &this->mp);
		}
	}

#if 0
	SelfType& operator= (const SelfType& array)
	{
		setSize (array.this->count);
		for (qse_size_t i = 0; i < array.this->count; i++) {
			this->buffer[i] = array.buffer[i];
		}
		return *this;
	}
#endif

	/*
	Array<T>& operator+= (const T& value)
	{
		addDatum (value);
		return *this;
	}
	Array<T>& operator+ (const T& value) const
	{
		Array<T> array (*this);
		array.addDatum (value);
		return array;
	}
	*/

	bool isEmpty () const
	{
		return this->count == 0;
	}

	qse_size_t getSize () const
	{
		return this->count;
	}

	qse_size_t getCapacity () const
	{
		return this->capacity;
	}
	
	operator T* ()
	{
		return this->buffer;
	}

	operator const T* () const
	{
		return this->buffer;
	}

	T* getBuffer ()
	{
		return this->buffer;
	}

	const T* getBuffer () const
	{
		return this->buffer;
	}

	T& operator[] (qse_size_t index)
	{
		QSE_ASSERT (index < this->count);
		return this->buffer[index];
	}

	const T& operator[] (qse_size_t index) const
	{
		QSE_ASSERT (index < this->count);
		return this->buffer[index];
	}

	T& get (qse_size_t index)
	{
		QSE_ASSERT (index < this->count);
		return this->buffer[index];
	}

	const T& get (qse_size_t index) const
	{
		QSE_ASSERT (index < this->count);
		return this->buffer[index];
	}

	void set (qse_size_t index, const T& value)
	{
		QSE_ASSERT (index < this->count);
		this->buffer[index] = value;
	}

protected:
	void set_item (qse_size_t index, const T& value)
	{
		if (index >= this->count)
			new((QSE::Mpool*)QSE_NULL, &this->buffer[index]) T(value);
		else
			this->buffer[index] = value;
	}

public:
	qse_size_t insert (qse_size_t index, const T& value)
	{
		if (index >= this->capacity) 
		{
			qse_size_t new_capa = this->resizer (this->capacity);

			if (index < new_capa)
				this->setCapacity (new_capa);
			else
				this->setCapacity (index + 1);
		}
		else if (this->count >= this->capacity) 
		{
			qse_size_t new_capa = this->resizer (this->capacity);
			this->setCapacity (new_capa);
		}

		for (qse_size_t i = this->count; i > index; i--) 
		{
			//this->buffer[i] = this->buffer[i - 1];
			this->set_item (i, this->buffer[i - 1]);
		}

		//this->buffer[index] = value;
		this->set_item (index, value);
		if (index > this->count) this->count = index + 1;
		else this->count++;

		return index;
	}
	
	void remove (qse_size_t index)
	{
		this->remove (index, index);
	}

	void remove (qse_size_t from_index, qse_size_t to_index)
	{
		QSE_ASSERT (from_index < this->count);
		QSE_ASSERT (to_index < this->count);

		qse_size_t j = from_index;
		qse_size_t i = to_index + 1;
		while (i < this->count) 
		{
			this->buffer[j++] = this->buffer[i++];
		}
		this->count -= to_index - from_index + 1;
	}

#if 0
	qse_size_t addDatum (const T& value)
	{
		return insert (this->count, value);
	}

	qse_size_t removeDatum (const T& value)
	{
		qse_size_t i = 0, sz = this->size();
		while (i < this->count) 
		{
			if (value == this->buffer[i]) 
			{
				remove (i);
				break;
			}
			i++;
		}

		return sz - this->size();
	}

	qse_size_t removeDatums (const T& value)
	{
		qse_size_t i = 0, sz = this->size();

		while (i < this->count) 
		{
			if (value == this->buffer[i]) remove (i);
			else i++;
		}

		return sz - this->size();
	}
#endif


	void clear ()
	{
		setSize (0);
	}

	void trimToSize ()
	{
		setCapacity (this->size);
	}

	void setSize (qse_size_t size)
	{
		if (size > this->capacity) this->setCapacity (size);
		QSE_ASSERT (size <= this->capacity);
		this->count = size;
	}

	void setCapacity (qse_size_t capacity)
	{
		if (capacity <= 0) 
		{
			if (this->buffer != QSE_NULL) 
				delete[] this->buffer;
			this->buffer = QSE_NULL;
			this->capacity = 0;
			this->count  = 0;
		}
		else 
		{
			T* tmp = QSE_NULL;
			qse_size_t cnt = this->count;

			try 
			{
				tmp = new T[capacity];
				if (cnt > capacity) cnt = capacity;
				for (qse_size_t i = 0; i < cnt; i++) 
				{
					tmp[i] = this->buffer[i];
				}
			}
			catch (...) 
			{
				if (tmp != QSE_NULL) delete[] tmp;
				throw;
			}

			if (this->buffer != QSE_NULL) 
				delete[] this->buffer;
			this->buffer = tmp;
			this->capacity = capacity;
			this->count = cnt;
		}
	}

#if 0
	qse_size_t indexOf (const T& value) const
	{
		for (qse_size_t i = 0; i < this->count; i++) 
		{
			if (this->buffer[i] == value) return i;
		}
		return INVALID_INDEX;
	}
	
	qse_size_t indexOf (const T& value, qse_size_t index) const
	{
		for (qse_size_t i = index; i < this->count; i++) 
		{
			if (this->buffer[i] == value) return i;
		}
		return INVALID_INDEX;
	}
	
	qse_size_t lastIndexOf (const T& value) const
	{
		for (qse_size_t i = this->count; i > 0; ) 
		{
			if (this->buffer[--i] == value) return i;
		}	
		return INVALID_INDEX;
	}
	
	qse_size_t lastIndexOf (const T& value, qse_size_t index) const
	{
		for (qse_size_t i = index + 1; i > 0; ) 
		{
			if (this->buffer[--i] == value) return i;
		}
		return INVALID_INDEX;
	}
#endif

	void rotate (int dir, qse_size_t n)
	{
		qse_size_t first, last, cnt, index, nk;
		T c;

		if (dir == 0) return this->count;
		if ((n %= this->count) == 0) return this->count;

		if (dir > 0) n = this->count - n;
		first = 0; nk = this->count - n; cnt = 0; 

		while (cnt < n) 
		{
			last = first + nk;
			index = first;
			c = this->buffer[first];
			while (1) 
			{
				cnt++;
				while (index < nk) 
				{
					this->buffer[index] = this->buffer[index + n];
					index += n;
				}
				if (index == last) break;
				this->buffer[index] = this->buffer[index - nk];
				index -= nk;
			}
			this->buffer[last] = c; first++;
		}
	}

protected:
	Mpool      mp;
	RESIZER    resizer;

	qse_size_t count;
	qse_size_t capacity;
	T* buffer;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif


