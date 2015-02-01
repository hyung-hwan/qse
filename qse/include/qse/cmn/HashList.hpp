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

#ifndef _QSE_CMN_HASHLIST_HPP_
#define _QSE_CMN_HASHLIST_HPP_

#include <qse/Hashable.hpp>
#include <qse/cmn/LinkedList.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template<typename T>
struct HashListHasher
{
	qse_size_t operator() (const T& v) const
	{
		return v.hashCode();
	}
};

template<typename T>
struct HashListComparator
{
	bool operator() (const T& v1, const T& v2) const
	{
		return v1 == v2;
	}
};

template <typename T, typename MPOOL = Mpool, typename HASHER = HashListHasher<T>, typename COMPARATOR = HashListComparator<T> >
class HashList: public Mmged
{
public:
	typedef LinkedList<T,MPOOL> DatumList;
	typedef typename DatumList::Node Node;
	typedef HashList<T,MPOOL,HASHER,COMPARATOR> SelfType;

	HashList (
		Mmgr* mmgr = QSE_NULL,
		qse_size_t node_capacity = 10, 
		qse_size_t load_factor = 75, 
		qse_size_t mpb_size = 0): Mmged(mmgr) /*: datum_list (mpb_size)*/
	{
		this->nodes = QSE_NULL;
		this->node_capacity = 0;
		this->datum_list = QSE_NULL;

		try 
		{
			qse_size_t total_count = node_capacity << 1;
			//this->nodes = new Node*[total_count];
			this->nodes = (Node**)this->getMmgr()->allocMem (QSE_SIZEOF(Node*) * total_count);

			this->node_capacity = node_capacity;
			for (qse_size_t i = 0; i < total_count; i++) 
			{
				this->nodes[i] = QSE_NULL;
			}

			this->datum_list = new(this->getMmgr()) DatumList (this->getMmgr(), mpb_size);
		}
		catch (...) 
		{
			if (this->nodes) 
			{
				//delete[] this->nodes;
				this->getMmgr()->freeMem (this->nodes);
				this->node_capacity = 0;
				this->nodes = QSE_NULL;
			}

			if (this->datum_list) 
			{
				this->delete_datum_list ();
				this->datum_list = QSE_NULL;
			}

			throw;
		}

		this->load_factor = load_factor;
		this->threshold   = node_capacity * load_factor / 100;
	}

	HashList (const SelfType& list): Mmged (list)/*: datum_list (list.datum_list.getMPBlockSize()) */
	{
		this->nodes = QSE_NULL;
		this->node_capacity = 0;
		this->datum_list = QSE_NULL;

		try 
		{
			qse_size_t total_count = list.node_capacity << 1;
			//this->nodes = new Node*[total_count];
			this->nodes = (Node**)this->getMmgr()->allocMem (QSE_SIZEOF(Node*) * total_count);

			this->node_capacity = list.node_capacity;
			for (qse_size_t i = 0; i < total_count; i++)
			{
				this->nodes[i] = QSE_NULL;
			}

			this->datum_list = new(list.getMmgr()) DatumList (list.getMmgr(), list.datum_list->getMPBlockSize());
		}
		catch (...) 
		{
			if (this->nodes)
			{
				//delete[] this->nodes;
				this->getMmgr()->freeMem (this->nodes);
				this->node_capacity = 0;
				this->nodes = QSE_NULL;
			}
			if (this->datum_list)
			{
				this->delete_datum_list ();
				this->datum_list = QSE_NULL;
			}

			throw;
		}

		this->load_factor = list.load_factor;
		this->threshold   = list.threshold;

		for (qse_size_t i = 0; i < list.node_capacity; i++) 
		{
			qse_size_t head = i << 1;
			qse_size_t tail = head + 1;

			Node* np = list.nodes[head];
			if (!np) continue;
			
			do 
			{
				this->copy_datum (np, this->node_capacity, this->nodes, this->datum_list);
				if (np == list.nodes[tail]) break;
				np = np->getNextNode ();
			} 
			while (1);
		}
	}

	~HashList ()
	{
		this->clear ();
		if (this->nodes) this->getMmgr()->freeMem (this->nodes); //delete[] this->nodes;
		if (this->datum_list) this->delete_datum_list ();
	}

	SelfType& operator= (const SelfType& list)
	{
		this->clear ();

		for (qse_size_t i = 0; i < list.node_capacity; i++)
		{
			qse_size_t head = i << 1;
			qse_size_t tail = head + 1;

			Node* np = list.nodes[head];
			if (np == QSE_NULL) continue;
			
			do 
			{
				copy_datum (np, this->node_capacity, this->nodes, this->datum_list);
				if (np == list.nodes[tail]) break;
				np = np->getNextNode ();
			} 
			while (1);
		}

		return *this;
	}

protected:
	Node* insert_value (const T& datum, bool overwrite = true)
	{
		qse_size_t hc, head, tail;
		Node* np;

		hc = this->hasher(datum) % this->node_capacity;
		head = hc << 1; tail = head + 1;

		np = this->nodes[head];
		if (np) 
		{
			do 
			{
				T& t = np->value;
				if (this->comparator(datum, t)) 
				{
					if (!overwrite) return QSE_NULL;
					t = datum;
					return np;
				}

				if (np == this->nodes[tail]) break;
				np = np->getNextNode ();
			}
			while (1); 
		}

		if (datum_list->getSize() >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(datum) % this->node_capacity;
			head = hc << 1; tail = head + 1;
		}

		if (nodes[head] == QSE_NULL) 
		{
			this->nodes[head] = this->datum_list->insertValue (QSE_NULL, datum);
			this->nodes[tail] = this->nodes[head];
		}
		else 
		{
			this->nodes[head] = datum_list->insertValue (this->nodes[head], datum);
		}

		return this->nodes[head];
	}

public:
	Node* insert (const T& datum)
	{
		return this->insert_value (datum, false);
	}

	Node* upsert (const T& datum)
	{
		return this->insert_value (datum, true);
	}

protected:
	const Node* find_node (const T& datum) const
	{
		qse_size_t hc, head, tail;
		Node* np;

		hc = this->hasher(datum) % this->node_capacity;
		head = hc << 1; tail = head + 1;

		np = this->nodes[head];
		if (np) 
		{
			do 
			{
				T& t = np->value;
				if (datum == t) return np;
				if (np == this->nodes[tail]) break;
				np = np->getNextNode ();
			}
			while (1);
		}

		return QSE_NULL;
	}

public:
	Node* findNode (const T& datum)
	{
		return (Node*)this->find_node (datum);
	}

	const Node* findNode (const T& datum) const
	{
		return this->find_node (datum);
	}

	T* findValue (const T& datum)
	{
		Node* b = this->findNode (datum);
		if (!b) return QSE_NULL;
		return &b->value;
	}

	const T* findValue (const T& datum) const
	{
		const Node* b = this->findNode (datum);
		if (!b) return QSE_NULL;
		return &b->value;
	}

	bool isEmpty () const 
	{
		return this->datum_list->isEmpty();
	}

	bool contains (const T& datum) const
	{
		return this->findNode (datum) != QSE_NULL;
	}

	int remove (const T& datum)
	{
		qse_size_t hc, head, tail;
		Node* np;

		hc = this->hasher(datum) % this->node_capacity;
		head = hc << 1; tail = head + 1;

		np = this->nodes[head];
		if (np) 
		{
			do 
			{
				T& t = np->value;
				if (this->comparator(datum, t)) 
				{
					if (this->nodes[head] == this->nodes[tail])
					{
						QSE_ASSERT (np == this->nodes[head]);
						this->nodes[head] = this->nodes[tail] = QSE_NULL;
					}
					else if (np == this->nodes[head])
					{
						this->nodes[head] = np->getNextNode();
					}
					else if (np == this->nodes[tail])
					{ 
						this->nodes[tail] = np->getPrevNode();
					}

					this->datum_list->remove (np);
					return 0;
				}

				if (np == this->nodes[tail]) break;
				np = np->getNextNode ();
			} 
			while (1);
		}

		return -1;
	}

	void clear ()
	{
		for (qse_size_t i = 0; i < (this->node_capacity << 1); i++) 
		{
			this->nodes[i] = QSE_NULL;
		}
		if (this->datum_list) this->datum_list->clear ();
	}

	Node* getHeadNode () const
	{
		return this->datum_list->getHeadNode();
	}

	Node* getTaileNode () const
	{
		return this->datum_list->getTailNode();
	}

	typedef int (SelfType::*TraverseCallback) (Node* start, Node* cur);

	void traverse (TraverseCallback callback, Node* start)
	{
		Node* cur, *prev, * next;

		cur = start;
		while (cur) 
		{
			prev = cur->getPrevNode ();
			next = cur->getNextNode ();

			int n = (this->*callback) (start, cur);

			if (n > 0) cur = next;
			else if (n < 0) cur = prev;
			else break;
		}
	}

	qse_size_t getCapacity() const
	{
		return this->node_capacity;
	}

	qse_size_t getSize () const
	{
		return this->datum_list->getSize();
	}

protected:
	mutable qse_size_t node_capacity;
	mutable Node** nodes;
	mutable DatumList* datum_list;
	mutable qse_size_t threshold;
	qse_size_t load_factor;
	HASHER hasher;
	COMPARATOR comparator;

	void rehash () 
	{
		// Move nodes around instead of values to prevent
		// existing values from being copied over and destroyed.
		// this incurs less number of memory allocations also.
		SelfType temp (this->getMmgr(), this->node_capacity << 1, this->load_factor, this->datum_list->getMPBlockSize());
		Node* p = this->datum_list->getHeadNode();
		while (p)
		{
			Node* next = p->getNextNode();

			// p->value must be a unique value in the existing hashed list.
			// i can safely skip checking existing values.

			// detach the existing node.
			Node* pp = this->datum_list->yield (p, true);

			// get the hash code using the new capacity
			qse_size_t hc, head, tail;
			hc = this->hasher(pp->value) % temp.node_capacity;
			head = hc << 1; tail = head + 1;

			// insert the detached node to the new temporary list
			if (temp.nodes[head]) 
			{
				temp.nodes[head] = temp.datum_list->insertNode (temp.nodes[head], pp);
			}
			else 
			{
				temp.nodes[head] = temp.datum_list->insertNode (QSE_NULL, pp);
				temp.nodes[tail] = temp.nodes[head];
			}

			p = next;
		}

		// all nodes must have been popped out.
		QSE_ASSERT (this->datum_list->getSize() <= 0);

		// clear the node pointers by force as no nodes exist in the old list.
		for (qse_size_t i = 0; i < (this->node_capacity << 1); i++) 
		{
			this->nodes[i] = QSE_NULL;
		}

		// swap the contents
		qse_size_t temp_capa = temp.node_capacity;
		Node** temp_nodes = temp.nodes;
		DatumList* temp_datum_list = temp.datum_list;
		qse_size_t temp_threshold = temp.threshold;
		qse_size_t temp_load_factor = temp.load_factor;

		temp.node_capacity = this->node_capacity;
		temp.nodes = this->nodes;
		temp.datum_list = this->datum_list;
		temp.threshold = this->threshold;
		temp.load_factor = this->load_factor;

		this->node_capacity = temp_capa;
		this->nodes = temp_nodes;
		this->datum_list = temp_datum_list;
		this->threshold = temp_threshold;
		this->load_factor = temp_load_factor;
	}

	void copy_datum (
		Node* np, qse_size_t new_node_capa,
		Node** new_nodes, DatumList* new_datum_list) const
	{
		T& t = np->value;

		qse_size_t hc = this->hasher(t) % new_node_capa;
		qse_size_t head = hc << 1;
		qse_size_t tail = head + 1;

		if (new_nodes[head] == QSE_NULL) 
		{
			new_nodes[head] = new_datum_list->insertValue (QSE_NULL, t);
			new_nodes[tail] = new_nodes[head];
		}
		else 
		{
			new_nodes[head] = new_datum_list->insertValue (new_nodes[head], t);
		}
	}

private:
	void delete_datum_list ()
	{
		this->datum_list->~DatumList();
		::operator delete (this->datum_list, this->getMmgr());
	}
};


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
