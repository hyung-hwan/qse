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

#ifndef _QSE_CMN_LINKEDLIST_HPP_
#define _QSE_CMN_LINKEDLIST_HPP_

#include <qse/Types.hpp>
#include <qse/cmn/Mpool.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename T, typename COMPARATOR> class LinkedList;

template <typename T, typename COMPARATOR> 
class LinkedListNode
{
public:
	friend class LinkedList<T,COMPARATOR>;
	typedef LinkedListNode<T,COMPARATOR> SelfType;

	T value; // you can use this variable or accessor functions below

protected:
	SelfType* prev;
	SelfType* next;

public:
	T& getValue () { return this->value; }
	const T& getValue () const { return this->value; }
	void setValue (const T& v) { this->value = v; }

	SelfType* getNext () { return this->next; }
	const SelfType* getNext () const { return this->next; }
	SelfType* getNextNode () { return this->next; }
	const SelfType* getNextNode () const { return this->next; }

	SelfType* getPrev () { return this->prev; }
	const SelfType* getPrev () const { return this->prev; }
	SelfType* getPrevNode () { return this->prev; }
	const SelfType* getPrevNode () const { return this->prev; }

protected:
	LinkedListNode (): prev(QSE_NULL), next(QSE_NULL)  {}
	LinkedListNode (const T& v): value(v), prev(QSE_NULL), next(QSE_NULL)  {}

	void setNext (const SelfType* node)
	{
		this->next = node;
	}

	void setPrev (const SelfType* node)
	{
		this->prev = node;
	}
};

template <typename T, typename COMPARATOR, typename NODE, typename GET_T>
class LinkedListIterator {
public:
	friend class LinkedList<T,COMPARATOR>;
	typedef NODE Node;
	typedef LinkedListIterator<T,COMPARATOR,NODE,GET_T> SelfType;

	LinkedListIterator (): current(QSE_NULL) {}
	LinkedListIterator (Node* node): current(node) {}
	LinkedListIterator (const SelfType& it): current (it.current) {}

	SelfType& operator= (const SelfType& it) 
	{
		this->current = it.current;
		return *this;
	}

	SelfType& operator++ () // prefix increment
	{
		QSE_ASSERT (this->current != QSE_NULL);
		this->current = this->current->getNext();
		return *this;
	}

	SelfType operator++ (int) // postfix increment
	{
		QSE_ASSERT (this->current != QSE_NULL);
		SelfType saved (*this);
		this->current = this->current->getNext(); //++(*this);
		return saved;
	}

	SelfType& operator-- () // prefix decrement
	{
		QSE_ASSERT (this->current != QSE_NULL);
		this->current = this->current->getPrev();
		return *this;
	}

	SelfType operator-- (int) // postfix decrement
	{
		QSE_ASSERT (this->current != QSE_NULL);
		SelfType saved (*this);
		this->current = this->current->getPrev(); //--(*this);
		return saved;
	}

	bool operator== (const SelfType& it) const
	{
		QSE_ASSERT (this->current != QSE_NULL);
		return this->current == it.current;
	}

	bool operator!= (const SelfType& it) const
	{
		QSE_ASSERT (this->current != QSE_NULL);
		return this->current != it.current;
	}

	bool isLegit () const 
	{
		return this->current != QSE_NULL;
	}

	GET_T& operator* () // dereference
	{
		return this->current->getValue();
	}

#if 0
	T* operator-> ()
	{
		return &this->current->getValue();
	}
#endif

	GET_T& getValue ()
	{
		QSE_ASSERT (this->current != QSE_NULL);
		return this->current->getValue();
	}

	SelfType& setValue (const T& v) 
	{
		QSE_ASSERT (this->current != QSE_NULL);
		this->current->setValue (v);
		return *this;
	}

	Node* getNode ()
	{
		return this->current;
	}

protected:
	Node* current;
};

template<typename T>
struct LinkedListComparator
{
	// it must return true if two values are equal
	bool operator() (const T& v1, const T& v2) const
	{
		return v1 == v2;
	}
};

///
/// The LinkedList<T,COMPARATOR> class provides a template for a doubly-linked list.
///
template <typename T, typename COMPARATOR = LinkedListComparator<T> > class LinkedList: public Mmged
{
public:
	typedef LinkedList<T,COMPARATOR> SelfType;
	typedef LinkedListNode<T,COMPARATOR> Node;
	typedef LinkedListIterator<T,COMPARATOR,Node,T> Iterator;
	typedef LinkedListIterator<T,COMPARATOR,const Node,const T> ConstIterator;

	typedef LinkedListComparator<T> DefaultComparator;

	enum 
	{
		INVALID_INDEX = ~(qse_size_t)0
	};

	~LinkedList () 
	{
		this->clear (true);
	}

	LinkedList (Mmgr* mmgr = QSE_NULL, qse_size_t mpb_size = 0): Mmged(mmgr), mp (mmgr, QSE_SIZEOF(Node), mpb_size)
	{
		this->node_count = 0;
		this->head_node = QSE_NULL;
		this->tail_node = QSE_NULL;
	}

	LinkedList (const SelfType& ll): Mmged(ll.getMmgr()), mp (ll.getMmgr(), ll.mp.getDatumSize(), ll.mp.getBlockSize())
	{
		this->node_count = 0;
		this->head_node = QSE_NULL;
		this->tail_node = QSE_NULL;
		for (Node* p = ll.head_node; p != QSE_NULL; p = p->next) 
			this->append (p->value);
	}

	SelfType& operator= (const SelfType& ll) 
	{
		this->clear ();
		// note that the memory pool itself is not copied.
		for (Node* p = ll.head_node; p != QSE_NULL; p = p->next)
			this->append (p->value);
		return *this;
	}

#if 0
	T& operator[] (qse_size_t index)
	{
		// same as getValueAt()
		QSE_ASSERT (index < this->node_count);
		Node* np = this->getNodeAt (index);
		return np->value;
	}

	const T& operator[] (qse_size_t index) const
	{
		// same as getValueAt()
		QSE_ASSERT (index < this->node_count);
		Node* np = this->getNodeAt (index);
		return np->value;
	}
#endif

	Mpool& getMpool ()
	{
		return this->mp;
	}

	const Mpool& getMpool () const
	{
		return this->mp;
	}

	qse_size_t getSize () const 
	{
		return this->node_count;
	}

	bool isEmpty () const 
	{
		return this->node_count == 0;
	}

	/// The insertNode() function inserts an externally created \a node
	/// before the node at the given position \a pos. If \a pos is QSE_NULL,
	/// the \a node is inserted at the back of the list. You must take extra
	/// care when using this function.
	Node* insertNode (Node* pos, Node* node)
	{
		if (pos == QSE_NULL) 
		{
			// add to the back
			if (this->node_count == 0) 
			{
				QSE_ASSERT (head_node == QSE_NULL);
				QSE_ASSERT (tail_node == QSE_NULL);
				this->head_node = this->tail_node = node;
			}
			else 
			{
				node->prev = this->tail_node;
				this->tail_node->next = node;
				this->tail_node = node;
			}
		}
		else 
		{
			// insert 'node' before the node at the given position 'pos'.
			node->next = pos;
			node->prev = pos->prev;
			if (pos->prev) pos->prev->next = node;
			else this->head_node = node;
			pos->prev = node;
		}

		this->node_count++;
		return node;
	}

	/// The prependNode() function adds an externally created \a node
	/// to the front of the list.
	Node* prependNode (Node* node)
	{
		return this->insertNode (this->head_node, node);
	}

	/// The appendNode() function adds an externally created \a node
	/// to the back of the list.
	Node* appendNode (Node* node)
	{
		return this->insertNode (QSE_NULL, node);
	}

	// create a new node to hold the value and insert it.
	Node* insert (Node* pos, const T& value)
	{
		Node* node = new(&this->mp) Node(value);
		return this->insertNode (pos, node);
	}

	Node* prepend (const T& value)
	{
		return this->insert (this->head_node, value);
	}

	Node* append (const T& value)
	{
		return this->insert ((Node*)QSE_NULL, value);
	}

	void prependAll (const SelfType& list)
	{
		Node* n = list.tail_node;

		if (&list == this) 
		{
			Node* head = list.head_node;
			while (n) 
			{
				this->prepend (n->value);
				if (n == head) break;
				n = (Node*)n->prev;
			}
		}
		else 
		{
			while (n) 
			{
				this->prepend (n->value);
				n = (Node*)n->prev;
			}
		}
	}

	void appendAll (const SelfType& list) 
	{
		Node* n = list.head_node;

		if (&list == this)  
		{
			Node* tail = list.tail_node;
			while (n) 
			{
				this->append (n->value);
				if (n == tail) break;
				n = n->next;
			}
		}
		else 
		{
			while (n) 
			{
				this->append (n->value);
				n = n->next;
			}
		}
	}

	// remove a node from the list without freeing it.
	// take extra care when using this method as the node 
	// can be freed through the memory pool when the list is 
	// destructed if the memory pool is enabled.
	Node* yield (Node* node, bool clear_links = true)
	{
		QSE_ASSERT (node != QSE_NULL);
		QSE_ASSERT (this->node_count > 0);

		if (node->next)
			node->next->prev = node->prev;
		else
			this->tail_node = node->prev;

		if (node->prev)
			node->prev->next = node->next;
		else
			this->head_node = node->next;
			
		this->node_count--;

		if (clear_links)
		{
			node->next = QSE_NULL;
			node->prev = QSE_NULL;
		}

		return node;
	}

	// remove a node from the list and free it.
	void remove (Node* node)
	{
		this->yield (node, false);

		//call the destructor
		node->~Node (); 
		// free the memory
		::operator delete (node, &this->mp);
	}

	Node* yieldByValue (const T& value, bool clear_links = true)
	{
		Node* p = this->findFirstNode (value);
		if (!p) return QSE_NULL;
		return this->yield (p, clear_links);
	}

	/// \return the number of items deleted.
	qse_size_t removeByValue (const T& value)
	{
		Node* p = this->findFirstNode (value);
		if (!p) return 0;
		this->remove (p);
		return 1; 
	}

	/// \return the number of items deleted
	qse_size_t removeAllByValue (const T& value)
	{
		Node* p = this->findFirstNode (value);
		if (!p) return 0;

		qse_size_t cnt = 0;
		do 
		{
			Node* tmp = p->next;
			this->remove (p);
			cnt++;
			p = this->findFirstNode (value, tmp);
		} 
		while (p);

		return cnt;
	}

	void removeHead ()
	{
		this->remove (this->head_node);
	}

	void removeTail ()
	{
		this->remove (this->tail_node);
	}

	/// The getHeadNode() function returns the first node.
	/// \code
	/// QSE::LinkedList<int> l;
	/// QSE::LinkedList<int>::Node* np;
	/// for (np = l.getHeadNode(); np; np = np->getNextNode())
	/// {
	///     printf ("%d\n", np->value);
	/// }
	/// \endcode
	Node* getHeadNode ()
	{
		return this->head_node;
	}

	const Node* getHeadNode () const 
	{
		return this->head_node;
	}

	/// The getTailNode() function returns the last node.
	/// \code
	/// QSE::LinkedList<int> l;
	/// QSE::LinkedList<int>::Node* np;
	/// for (np = l.getTailNode(); np; np = np->getPrevNode())
	/// {
	///     printf ("%d\n", np->value);
	/// }
	/// \endcode
	Node* getTailNode ()
	{
		return this->tail_node;
	}

	const Node* getTailNode () const 
	{
		return this->tail_node;
	}

protected:
	Node* get_node_at (qse_size_t index) const
	{
		QSE_ASSERT (index < this->node_count);

		register Node* np;
		register qse_size_t cnt;

		if (index < (this->node_count >> 1)) 
		{
			for (np = this->head_node, cnt = 0; cnt < index; np = np->next, cnt++) 
			{
				QSE_ASSERT (np != QSE_NULL);
			}
		}
		else 
		{
			for (np = this->tail_node, cnt = this->node_count - 1;  cnt > index; np = np->prev, cnt--) 
			{
				QSE_ASSERT (np != QSE_NULL);
			}
		}

		return np;
	}

public:
	Node* getNodeAt (qse_size_t index) 
	{
		return this->get_node_at (index);
	}

	const Node* getNodeAt (qse_size_t index) const
	{
		return this->get_node_at (index);
	}

	T& getValueAt (qse_size_t index) 
	{
		// same as operator[]
		QSE_ASSERT (index < this->node_count);
		Node* np = this->getNodeAt (index);
		return np->value;
	}

	const T& getValueAt (qse_size_t index) const
	{
		// same as operator[]
		QSE_ASSERT (index < this->node_count);
		Node* np = this->getNodeAt (index);
		return np->value;
	}

	void setValueAt (qse_size_t index, const T& value)
	{
		QSE_ASSERT (index < this->node_count);
		Node* np = this->getNodeAt (index);
		np->value = value;
	}

	Node* findFirstNode (const T& value) const
	{
		for (Node* p = this->head_node; p; p = p->next) 
		{
			if (this->comparator (value, p->value)) return p;
		}
		return QSE_NULL;
	}

	Node* findLastNode (const T& value) const
	{
		for (Node* p = tail_node; p; p = p->prev) 
		{
			if (this->comparator (value, p->value)) return p;
		}
		return QSE_NULL;
	}

	Node* findFirstNode (const T& value, Node* head) const
	{
		for (Node* p = head; p; p = p->next)
		{
			if (this->comparator (value, p->value)) return p;
		}
		return QSE_NULL;
	}

	Node* findLastNode (const T& value, Node* tail) const
	{
		for (Node* p = tail; p; p = p->prev) 
		{
			if (this->comparator (value, p->value)) return p;
		}
		return QSE_NULL;
	}

	qse_size_t findFirstIndex (const T& value) const
	{
		qse_size_t index = 0;
		for (Node* p = this->head_node; p; p = p->next) 
		{
			if (this->comparator (value, p->value)) return index;
			index++;
		}
		return INVALID_INDEX;
	}

	qse_size_t findLastIndex (const T& value) const
	{
		qse_size_t index = this->node_count;
		for (Node* p = tail_node; p; p = p->prev) 
		{
			index--;
			if (this->comparator (value, p->value)) return index;
		}
		return INVALID_INDEX;
	}

	void clear (bool clear_mpool = false) 
	{
		Node* p, * saved;

		p = this->head_node;
		while (p) 
		{
			saved = p->next;

			// placement new/delete handling
			p->~Node (); // call the destructor
			::operator delete (p, &this->mp); // free the memory

			this->node_count--;
			p = saved;
		}

		this->head_node = this->tail_node = QSE_NULL;
		QSE_ASSERT (this->node_count == 0);

		if (clear_mpool) this->mp.dispose ();
	}

	Iterator getIterator (qse_size_t index = 0)
	{
		if (this->node_count <= 0) 
		{
			return Iterator (QSE_NULL);
		}
		else
		{
			if (index >= this->node_count) index = this->node_count - 1;
			return Iterator (this->getNodeAt(index));
		}
	}

	ConstIterator getConstIterator (qse_size_t index = 0) const
	{
		if (this->node_count <= 0) 
		{
			return ConstIterator (QSE_NULL);
		}
		else
		{
			if (index >= this->node_count) index = this->node_count - 1;
			return ConstIterator (this->getNodeAt(index));
		}
	}

	/// The reverse() function reverses the order of nodes.
	void reverse ()
	{
		if (this->node_count > 0)
		{
			Node* head = this->head_node;
			QSE_ASSERT (head != QSE_NULL);
			while (head->next)
			{
				Node* next_node = this->yield (head->next, false);
				this->insertNode (this->head_node, next_node);
			}
		}
	}

protected:
	Mpool       mp;
	COMPARATOR  comparator;
	Node*       head_node;
	Node*       tail_node;
	qse_size_t  node_count;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif


