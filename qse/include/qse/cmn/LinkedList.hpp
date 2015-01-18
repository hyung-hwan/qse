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
#include <qse/cmn/Mpoolable.hpp>


/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename T, typename MPOOL> class LinkedList;

template <typename T,typename MPOOL> 
class LinkedListNode: protected Mpoolable
{
public:
	friend class LinkedList<T,MPOOL>;
	typedef LinkedListNode<T,MPOOL> Node;

	T value; // you can use this variable or accessor functions below

protected:
	Node* next;
	Node* prev;

public:
	T& getValue () { return this->value; }
	const T& getValue () const { return this->value; }
	void setValue (const T& v) { this->value = v; }

	Node* getNext () { return this->next; }
	const Node* getNext () const { return this->next; }
	Node* getNextNode () { return this->next; }
	const Node* getNextNode () const { return this->next; }

	Node* getPrev () { return this->prev; }
	const Node* getPrev () const { return this->prev; }
	Node* getPrevNode () { return this->prev; }
	const Node* getPrevNode () const { return this->prev; }

protected:
	LinkedListNode (): prev(QSE_NULL), next(QSE_NULL)  {}
	LinkedListNode (const T& v): value(v), prev(QSE_NULL), next(QSE_NULL)  {}

	void setNext (const Node* node)
	{
		this->next = node;
	}

	void setPrev (const Node* node)
	{
		this->prev = node;
	}
};

///
/// The LinkedList<T> class provides a template for a doubly-linked list.
///
template <typename T, typename MPOOL = Mpool> class LinkedList
{
public:
	typedef LinkedListNode<T,MPOOL> Node;

	enum 
	{
		INVALID_INDEX = ~(qse_size_t)0
	};

	~LinkedList () 
	{
		this->clearout ();
	}

	LinkedList (qse_size_t mpb_size = 0): mp (QSE_SIZEOF(Node), mpb_size)
	{
		this->node_count = 0;
		this->head_node = QSE_NULL;
		this->tail_node = QSE_NULL;
	}

	LinkedList (const LinkedList<T>& ll): mp (ll.mp.getDatumSize(), ll.mp.getBlockSize())
	{
		this->node_count = 0;
		this->head_node = QSE_NULL;
		this->tail_node = QSE_NULL;
		for (Node* p = ll.head_node; p != QSE_NULL; p = p->next) 
			this->append (p->value);
	}

	LinkedList<T>& operator= (const LinkedList<T>& ll) 
	{
		this->clear ();
		for (Node* p = ll.head_node; p != QSE_NULL; p = p->next)
			this->append (p->value);
		return *this;
	}
	
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

	qse_size_t getMPBlockSize() const
	{
		return this->mp.getBlockSize();
	}

	bool isMPEnabled () const
	{
		return this->mp.isEnabled();
	}

	qse_size_t getSize () const 
	{
		return this->node_count;
	}

	bool isEmpty () const 
	{
		return this->node_count == 0;
	}

	bool contains (const T& value) const
	{
		return this->findFirstNode(value) != QSE_NULL;    
	}

	// insert an externally created node.
	// may need to take extra care when using this method.
	Node* insertNode (Node* pos, Node* node);

	// create a new node to hold the value and insert it.
	Node* insertValue (Node* pos, const T& value)
	{
		Node* node = new(&mp) Node(value);
		return this->insertNode (pos, node);
	}

	T& insert (Node* node, const T& value)
	{
		return this->insertValue(node, value)->value;
	}

	T& insert (qse_size_t index, const T& value)
	{
		QSE_ASSERT (index <= node_count);

		if (index >= node_count) 
		{
			// insert it at the back
			return this->insert ((Node*)QSE_NULL, value);
		}

		return this->insert (this->getNodeAt(index), value);
	}

	T& prepend (const T& value)
	{
		// same as prependValue()
		return this->insert (this->head_node, value);
	}
	T& append (const T& value)
	{
		// same as appendValue()
		return this->insert ((Node*)QSE_NULL, value);
	}

	Node* prependNode (Node* node)
	{
		return this->insertNode (head_node, node);
	}
	Node* appendNode (Node* node)
	{
		return this->insertNode ((Node*)QSE_NULL, node);
	}

	Node* prependValue (const T& value)
	{
		return this->insertValue (this->head_node, value);
	}
	Node* appendValue (const T& value)
	{
		return this->insertValue ((Node*)QSE_NULL, value);
	}

	void prependAll (const LinkedList<T>& list)
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

	void appendAll (const LinkedList<T>& list) 
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

		// cal the deallocator
	#if defined(_MSC_VER)
		node->operator delete (node, &this->mp);
	#else
		node->dispose (node, &this->mp);
	#endif
	}

	void remove (qse_size_t index) 
	{
		QSE_ASSERT (index < node_count);

		Node* np = this->head_node; 
		while (index > 0) 
		{
			np = np->next;
			index--;
		}

		this->remove (np);
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

	Node* getHeadNode () const 
	{
		return this->head_node;
	}

	Node* getTailNode () const 
	{
		return this->tail_node;
	}

	Node* getNodeAt (qse_size_t index) const
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
			if (value == p->value) return p;
		}
		return QSE_NULL;
	}

	Node* findLastNode (const T& value) const
	{
		for (Node* p = tail_node; p; p = p->prev) 
		{
			if (value == p->value) return p;
		}
		return QSE_NULL;
	}

	Node* findFirstNode (const T& value, Node* head) const
	{
		for (Node* p = head; p; p = p->next)
		{
			if (value == p->value) return p;
		}
		return QSE_NULL;
	}

	Node* findLastNode (const T& value, Node* tail) const
	{
		for (Node* p = tail; p; p = p->prev) 
		{
			if (value == p->value) return p;
		}
		return QSE_NULL;
	}	

	qse_size_t findFirstIndex (const T& value) const
	{
		qse_size_t index = 0;
		for (Node* p = this->head_node; p; p = p->next) 
		{
			if (value == p->value) return index;
			index++;
		}
		return INVALID_INDEX;
	}

	qse_size_t findLastIndex (const T& value) const
	{
		qse_size_t index = node_count;
		for (Node* p = tail_node; p; p = p->prev) 
		{
			index--;
			if (value == p->value) return index;
		}
		return INVALID_INDEX;
	}

	void clear () 
	{
		Node* p, * saved;

		p = this->head_node;
		while (p) 
		{
			saved = p->next;

			if (this->mp.isDisabled()) delete p;
			else 
			{
				p->~Node ();
			#if defined(_MSC_VER)
				p->operator delete (p, &this->mp);
			#else
				p->dispose (p, &this->mp);
			#endif
			}

			this->node_count--;
			p = saved;
		}

		this->head_node = this->tail_node = QSE_NULL;
		QSE_ASSERT (this->node_count == 0);
	}

	void clearout ()
	{
		this->clear ();
		this->mp.dispose ();
	}

	typedef int (LinkedList<T>::*TraverseCallback) (Node* start, Node* cur);

	void traverse (TraverseCallback callback, Node* start)
	{
		Node* cur, * prev, * next;

		cur = start;
		while (cur)
		{
			prev = cur->prev;
			next = cur->next;

			int n = (this->*callback) (start, cur);

			if (n > 0) cur = next;
			else if (n < 0) cur = prev;
			else break;
		}
	}

protected:
	//Mpool       mp;
	MPOOL       mp;
	Node*       head_node;
	Node*       tail_node;
	qse_size_t  node_count;
};

template <typename T,typename MPOOL> 
inline typename LinkedList<T,MPOOL>::Node* LinkedList<T,MPOOL>::insertNode (Node* pos, Node* node)
{
	if (pos == QSE_NULL) 
	{
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
		node->next = pos;
		node->prev = pos->prev;
		if (pos->prev) pos->prev->next = node;
		else this->head_node = node;
		pos->prev = node;
	}

	this->node_count++;
	return node;
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif


