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

#ifndef _QSE_CMN_BINARYHEAP_HPP_
#define _QSE_CMN_BINARYHEAP_HPP_

///
/// This file provides an array-based binary heap.
/// In the heap, each node is greater than or equal to its BinaryHeap
///
/// #include <qse/cmn/BinaryHeap.hpp>
/// #include <stdio.h>
///
/// struct IntComparator
/// {
///         bool operator() (int v1, int v2) const
///         {
///                 //return !(v1 > v2);
///                 return v1 > v2;
///         }
/// };
/// 
/// int main (int argc, char* argv[])
/// {
///         QSE::BinaryHeap<int,IntComparator> heap;
/// 
///         heap.insert (70);
///         heap.insert (90);
///         heap.insert (10);
///         heap.insert (5);
///         heap.insert (88);
///         heap.insert (87);
///         heap.insert (300);
///         heap.insert (91);
///         heap.insert (100);
///         heap.insert (200);
/// 
///         while (heap.getSize() > 0)
///         {
///                 printf ("%d\n", heap.getRootValue());
///                 heap.remove (0);
///         }
/// 
///         return 0;
/// }
/// 

#include <qse/Types.hpp>
#include <qse/cmn/Mpool.hpp>


/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename T>
struct BinaryHeapComparator
{
	// this can be used to build a max heap
	bool operator() (const T& v1, const T& v2) const
	{
		return v1 > v2;
	}
};

template<typename T>
struct BinaryHeapAssigner
{
	// The assignment proxy is used to get the value informed of its position
	// within the heap. This default implmentation, however, doesn't utilize
	// the position (index).
	T& operator() (T& v1, const T& v2, xp_size_t index) const
	{
		v1 = v2;
		return v1;
	}
};

struct BinaryHeapResizer
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


#define QSE_BINARY_HEAP_UP(x)     (((x) - 1) / 2)
#define QSE_BINARY_HEAP_LEFT(x)   ((x) * 2 + 1)
#define QSE_BINARY_HEAP_RIGHT(x)  ((x) * 2 + 2)

template <typename T, typename COMPARATOR = BinaryHeapComparator<T>, typename ASSIGNER = BinaryHeapAssigner<T>, RESIZER = TeeeHeapResizer >
class BinaryHeap: public Mmged
{
public:
	typedef BinaryHeap<T,COMPARATOR,ASSIGNER,RESIZER> SelfType;

	enum
	{
		DEFAULT_CAPACITY = 10,
		MIN_CAPACITY = 1
	};

	BinaryHeap (Mmgr* mmgr = QSE_NULL,
	            qse_size_t capacity = DEFAULT_CAPACITY, 
	            qse_size_t mpb_size = 0):
		Mmged (mmgr),
		mp (mmgr, QSE_SIZEOF(Node), mpb_size)
	{
		if (capacity < MIN_CAPACITY) capacity = MIN_CAPACITY;
		this->capacity = capacity;
		this->count = 0;

		this->buffer = (T*)::operator new (this->capacity * QSE_SIZEOF(*this->buffer), &this->mp);
		for (qse_size_t i = 0; i < this->capacity; i++)
		{
			
		}
	}

	BinaryHeap (const SelfType& heap):
		Mmged (heap.getMmgr()),
		mp (heap.getMmgr(), heap.mp.getDatumSize(), heap.mp.getBlockSize()),
		capacity (heap.capacity), count (0)
	{
		// TODO: copy data items.
	}

	~BinaryHeap ()
	{
		for (qse_size_t i = this->count; i > 0; )
		{
			--i;
			this->buffer[i].~T ();
		}

		::operator delete (this->buffer, &this->mp);
	}

	SelfType& operator= (const SelfType& heap)
	{
		this->clear ();
		// TODO: copy data items
		return *this;
	}

	~BinaryHeap
	Mpool& getMpool ()
	{
		return this->mp;
	}

	const Mpool& getMpool () const
	{
		return this->mp;
	}

	qse_size_t getCapacity () const
	{
		return this->capacity;
	}

	qse_size_t getSize () const
	{
		return this->count;
	}

	bool isEmpty () const
	{
		return this->count <= 0;
	}

	

	Node* insert (const T& value)
	{
#if 0
		qse_size_t index = this->data_count;

		// add the item at the back of the array
		// i don't use Tree<T>::insert() for this->assign().
		//Tree<T>::insert (index, value);
		Tree<T>::setSize (index + 1);
		this->assign (this->data_buffer[index], value, index);

		// move the item up to the top if it's greater than the up item	
		return sift_up (index);
#endif
	}

#if 0
	qse_size_t update (qse_size_t index, const T& value)
	{
		T old = this->data_buffer[index];

		//this->data_buffer[index] = value;
		this->assign (this->data_buffer[index], value, index);

		return (this->greater_than (value, old))? sift_up (index): sift_down (index);
	}
#endif

	void remove_node (qse_size_t index)
	{
		QSE_ASSERT (index < this->data_count);

#if 0
		// copy the last item to the position to remove 
		// note that this->assign() isn't called for temporary assignment.
		T old = this->data_buffer[index];

		//this->data_buffer[index] = this->data_buffer[this->data_count - 1];
		this->assign (this->data_buffer[index], this->data_buffer[this->data_count - 1], index);

		// delete the last item
		Tree<T>::remove (this->data_count - 1);
		
		// relocate the item
		(this->greater_than (this->data_buffer[index], old))? sift_up (index): sift_down (index);
#endif
	}

	void remove ()
	{
		/* TODO: remove root node */
	}

	void clear ()
	{
		while (this->root->notNil()) this->remove_node (this->root);
		QSE_ASSERT (this->root = this->nil);
		QSE_ASSERT (this->node_count == 0);
	}

protected:
	Node* sift_up (qse_size_t index)
	{
#if 0
		qse_size_t up;

		up = QSE_ARRAY_HEAP_PARENT (index);
		if (index > 0 && this->greater_than (this->data_buffer[index], this->data_buffer[up]))
		{
			// note that this->assign() isn't called for temporary assignment.
			T item = this->data_buffer[index];

			do 
			{
				//this->data_buffer[index] = this->data_buffer[up];
				this->assign (this->data_buffer[index], this->data_buffer[up], index);

				index = up;	
				up = QSE_ARRAY_HEAP_PARENT (up);
			}
			while (index > 0 && this->greater_than (item, this->data_buffer[up]));

			//this->data_buffer[index] = item;
			this->assign (this->data_buffer[index], item, index);
		}

		return index;
#endif
		return QSE_NULL;
	}

	Node* sift_down (qse_size_t index)
	{
#if 0
		qse_size_t half_data_count = this->data_count / 2;
		
		if (index < half_data_count)
		{
			// if at least 1 child is under the 'index' position
			// perform sifting

			// note that this->assign() isn't called for temporary assignment.
			T item = this->data_buffer[index];
			T item = this->data_buffer[index];

			do
			{
				qse_size_t left, right, greater;
	
				left = QSE_ARRAY_HEAP_LEFT (index);
				right = QSE_ARRAY_HEAP_RIGHT (index);
	
				// choose the larger one between 2 BinaryHeap 
				if (right < this->data_count && 
				    this->greater_than (this->data_buffer[right], this->data_buffer[left]))
				{
					// if the right child exists and 
					// the right item is greater than the left item
					greater = right;
				}
				else
				{
					greater = left;
				}
	
				if (this->greater_than (item, this->data_buffer[greater])) break;
	
				//this->data_buffer[index] = this->data_buffer[greater];
				this->assign (this->data_buffer[index], this->data_buffer[greater], index);
				index = greater;
			}
			while (index < half_data_count);

			//this->data_buffer[index] = item;
			this->assign (this->data_buffer[index], item, index);
		}

		return index;
#endif
		return QSE_NULL;
	}

protected:
	Mpool      mp;
	COMPARATOR greater_than;
	ASSIGNER   assigner;
	RESIZER    resizer;

	qse_size_t capacity;
	qse_size_t count;
	T* buffer;
};


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
