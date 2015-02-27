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
struct HashListEqualer
{
	bool operator() (const T& v1, const T& v2) const
	{
		return v1 == v2;
	}
};

struct HashListResizer
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

/// The HashList class provides a linked list where a data item can be accessed
/// using hashing. The accessor functions are similar to those of the HashTable
/// class whereas the data items are linked with each other in a linked list.
/// Extra hashing buckets maintain pointers to the first and the last node of
/// data items whose hash values are equal. Unlike the HashTable class that
/// maintains pairs of a key and a value, it stores a series of single items
/// whose key is distinuguished via the hashing function.
///
/// For the capacicity of X, it allocates (X * 2) node slots(this->nodes).
/// For a hash value of hc, this->nodes[hc * 2] points to the first node and
/// this->nodes[hc * 2 + 1] ponits to the last node.     

template <typename T, typename HASHER = HashListHasher<T>, typename EQUALER = HashListEqualer<T>, typename RESIZER = HashListResizer >
class HashList: public Mmged
{
public:
	typedef LinkedList<T,EQUALER> DatumList;
	typedef typename DatumList::Node Node;
	typedef typename DatumList::Iterator Iterator;
	typedef typename DatumList::ConstIterator ConstIterator;
	typedef HashList<T,HASHER,EQUALER,RESIZER> SelfType;

	typedef HashListHasher<T> DefaultHasher;
	typedef HashListEqualer<T> DefaultEqualer;
	typedef HashListResizer DefaultResizer;

	enum
	{
		DEFAULT_CAPACITY = 10,
		DEFAULT_LOAD_FACTOR = 75, // Load factor in percentage

		MIN_CAPACITY = 1,
		MIN_LOAD_FACTOR = 20
	};

	HashList (
		Mmgr* mmgr = QSE_NULL,
		qse_size_t node_capacity = DEFAULT_CAPACITY, 
		qse_size_t load_factor = DEFAULT_LOAD_FACTOR, 
		qse_size_t mpb_size = 0): Mmged(mmgr)
	{
		if (node_capacity < MIN_CAPACITY) node_capacity = MIN_CAPACITY;
		if (load_factor < MIN_LOAD_FACTOR) load_factor = MIN_LOAD_FACTOR;

		this->nodes = QSE_NULL;
		this->node_capacity = 0;
		this->datum_list = QSE_NULL;

		try 
		{
			qse_size_t total_count = node_capacity << 1;

			// Node* is a plain type that doesn't have any constructors and destructors.
			// it should be safe to call the memory manager bypassing the new operator.
			//this->nodes = new Node*[total_count];
			this->nodes = (Node**)this->getMmgr()->allocate (QSE_SIZEOF(Node*) * total_count);

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
				this->getMmgr()->dispose (this->nodes); //delete[] this->nodes;
				this->nodes = QSE_NULL;
				this->node_capacity = 0;
			}

			if (this->datum_list) 
			{
				this->free_datum_list ();
				this->datum_list = QSE_NULL;
			}

			throw;
		}

		this->load_factor = load_factor;
		this->threshold   = node_capacity * load_factor / 100;
	}

	HashList (const SelfType& list): Mmged (list)
	{
		this->nodes = QSE_NULL;
		this->node_capacity = 0;
		this->datum_list = QSE_NULL;

		try 
		{
			qse_size_t total_count = list.node_capacity << 1;
			//this->nodes = new Node*[total_count];
			this->nodes = (Node**)this->getMmgr()->allocate (QSE_SIZEOF(Node*) * total_count);

			this->node_capacity = list.node_capacity;
			for (qse_size_t i = 0; i < total_count; i++)
			{
				this->nodes[i] = QSE_NULL;
			}

			// placement new
			this->datum_list = new(list.getMmgr()) 
				DatumList (list.getMmgr(), list.getMpool().getBlockSize());
		}
		catch (...) 
		{
			if (this->nodes)
			{
				this->getMmgr()->dispose (this->nodes); //delete[] this->nodes;
				this->nodes = QSE_NULL;
				this->node_capacity = 0;
			}
			if (this->datum_list)
			{
				this->free_datum_list ();
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
		this->clear (true);
		if (this->nodes) this->getMmgr()->dispose (this->nodes); //delete[] this->nodes;
		if (this->datum_list) this->free_datum_list ();
	}

	SelfType& operator= (const SelfType& list)
	{
		this->clear ();

		// note that the memory pool itself is not copied.

		for (qse_size_t i = 0; i < list.node_capacity; i++)
		{
			qse_size_t head = i << 1;
			qse_size_t tail = head + 1;

			Node* np = list.nodes[head];
			if (np == QSE_NULL) continue;
			
			do 
			{
				this->copy_datum (np, this->node_capacity, this->nodes, this->datum_list);
				if (np == list.nodes[tail]) break;
				np = np->getNextNode ();
			} 
			while (1);
		}

		return *this;
	}

	Mpool& getMpool ()
	{
		return this->datum_list->getMpool ();
	}

	const Mpool& getMpool() const
	{
		return this->datum_list->getMpool ();
	}

	qse_size_t getCapacity() const
	{
		return this->node_capacity;
	}

	qse_size_t getSize () const
	{
		return this->datum_list->getSize ();
	}

	bool isEmpty () const 
	{
		return this->datum_list->isEmpty ();
	}

	Node* getHeadNode ()
	{
		return this->datum_list->getHeadNode ();
	}

	const Node* getHeadNode () const
	{
		return this->datum_list->getHeadNode ();
	}

	Node* getTailNode ()
	{
		return this->datum_list->getTailNode ();
	}

	const Node* getTailNode () const
	{
		return this->datum_list->getTailNode ();
	}

protected:
	Node* find_node (const T& datum) const
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
				if (this->equaler (datum, t)) return np;
				if (np == this->nodes[tail]) break;
				np = np->getNextNode ();
			}
			while (1);
		}

		return QSE_NULL;
	}

	template <typename MT, typename MEQUALER>
	Node* heterofind_node (const MT& datum, qse_size_t hc) const
	{
		MEQUALER mequaler;

		qse_size_t head, tail;
		Node* np;

		head = hc << 1; tail = head + 1;

		np = this->nodes[head];
		if (np) 
		{
			do 
			{
				T& t = np->value;
				if (mequaler (datum, t)) return np;
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
		return this->find_node (datum);
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

	template <typename MT, typename MHASHER, typename MEQUALER>
	Node* heterofindNode (const MT& datum)
	{
		MHASHER hash;
		return this->heterofind_node<MT,MEQUALER> (datum, hash(datum) % this->node_capacity);
	}

	template <typename MT, typename MHASHER, typename MEQUALER>
	const Node* heterofindNode (const MT& datum) const
	{
		MHASHER hash;
		return this->heterofind_node<MT,MEQUALER> (datum, hash(datum) % this->node_capacity);
	}

	template <typename MT, typename MHASHER, typename MEQUALER>
	T* heterofindValue(const MT& datum)
	{
		MHASHER hash;
		Node* b = this->heterofind_node<MT,MEQUALER> (datum, hash(datum) % this->node_capacity);
		if (!b) return QSE_NULL;
		return &b->value;
	}

	template <typename MT, typename MHASHER, typename MEQUALER>
	const T* heterofindValue(const MT& datum) const
	{
		MHASHER hash;
		Node* b = this->heterofind_node<MT,MEQUALER> (datum, hash(datum) % this->node_capacity);
		if (!b) return QSE_NULL;
		return &b->value;
	}

	/// The search() function returns the pointer to the existing node
	/// containing the equal value to \a datum. If no node is found, it
	/// return #QSE_NULL.
	Node* search (const T& datum)
	{
		return this->find_node (datum);
	}

	/// The search() function returns the pointer to the existing node
	/// containing the equal value to \a datum. If no node is found, it
	/// return #QSE_NULL.
	const Node* search (const T& datum) const
	{
		return this->find_node (datum);
	}

	template <typename MT, typename MHASHER, typename MEQUALER>
	Node* heterosearch (const MT& datum)
	{
		MHASHER hash;
		return this->heterofind_node<MT,MEQUALER> (datum, hash(datum) % this->node_capacity);
	}

	template <typename MT, typename MHASHER, typename MEQUALER>
	const Node* heterosearch (const MT& datum) const
	{
		MHASHER hash;
		return this->heterofind_node<MT,MEQUALER> (datum, hash(datum) % this->node_capacity);
	}

	Node* inject (const T& datum, int mode, bool* injected = QSE_NULL)
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
				if (this->equaler (datum, t)) 
				{
					if (injected) *injected = false;
					if (mode <= -1) return QSE_NULL; // failure
					if (mode >= 1) t = datum; // overwrite
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
			this->nodes[head] = this->datum_list->insert ((Node*)QSE_NULL, datum);
			this->nodes[tail] = this->nodes[head];
		}
		else 
		{
			this->nodes[head] = this->datum_list->insert (this->nodes[head], datum);
		}

		if (injected) *injected = true;
		return this->nodes[head];
	}

	Node* insert (const T& datum)
	{
		return this->inject (datum, -1, QSE_NULL);
	}

	Node* ensert (const T& datum)
	{
		return this->inject (datum, 0, QSE_NULL);
	}

	Node* upsert (const T& datum)
	{
		return this->inject (datum, 1, QSE_NULL);
	}

	Node* update (const T& datum)
	{
		Node* node = this->find_node (datum);
		if (node) node->value = datum;
		return node;
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
				if (this->equaler (datum, t)) 
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

	template <typename MT, typename MHASHER, typename MEQUALER>
	int heteroremove (const MT& datum)
	{
		MHASHER hash;
		qse_size_t hc = hash(datum) % this->node_capacity;

		Node* np = this->heterofind_node<MT,MEQUALER> (datum, hc);
		if (np)
		{
			qse_size_t head, tail;

			head = hc << 1; tail = head + 1;

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

		return -1;
	}

	void clear (bool clear_mpool = false)
	{
		for (qse_size_t i = 0; i < (this->node_capacity << 1); i++) 
		{
			this->nodes[i] = QSE_NULL;
		}
		this->datum_list->clear (clear_mpool);
	}

	/// The getIterator() function returns an interator.
	///
	/// \code
	///  struct IntHasher 
	///  {
	///      qse_size_t operator() (int v) { return v; }
	///  };
	///  typedef QSE::HashList<int,QSE::Mpool,IntHasher> IntList;
	///
	///  IntList hl;
	///  IntList::Iterator it;
	///
	///  hl.insert (10);
	///  hl.insert (150);
	///  hl.insert (200);
	///  for (it = hl.getIterator(); it.isLegit(); it++)
	///  {
	///      printf ("%d\n", *it);
	///  }
	/// \endcode
	Iterator getIterator (qse_size_t index = 0)
	{
		return this->datum_list->getIterator (index);
	}

	ConstIterator getConstIterator (qse_size_t index = 0) const
	{
		return this->datum_list->getConstIterator (index);
	}

protected:
	mutable qse_size_t node_capacity;
	mutable Node**     nodes;
	mutable DatumList* datum_list;
	mutable qse_size_t threshold;
	qse_size_t         load_factor;
	HASHER             hasher;
	EQUALER            equaler;
	RESIZER            resizer;

	void rehash () 
	{
		// Move nodes around instead of values to prevent
		// existing values from being copied over and destroyed.
		// this incurs less number of memory allocations also.
		Mpool& mpool = this->getMpool();

		// Using the memory pool block size of 0 is OK because the nodes
		// to be inserted are yielded off the original list and inserted
		// without new allocation.
		//SelfType temp (this->getMmgr(), this->resizer(this->node_capacity), this->load_factor, mpool.getBlockSize());
		SelfType temp (this->getMmgr(), this->resizer(this->node_capacity), this->load_factor, 0);
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
				temp.nodes[head] = temp.datum_list->insertNode ((Node*)QSE_NULL, pp);
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

		// swapping the memory pool is a critical thing to do
		// especially when the memory pooling is enabled. the datum node in
		// 'temp' is actual allocated inside 'mpool' not inside temp.getMpool().
		// it is because yield() has been used for insertion into 'temp'.
		mpool.swap (temp.getMpool());

		// swap the actual contents
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
			new_nodes[head] = new_datum_list->insert ((Node*)QSE_NULL, t);
			new_nodes[tail] = new_nodes[head];
		}
		else 
		{
			new_nodes[head] = new_datum_list->insert (new_nodes[head], t);
		}
	}

private:
	void free_datum_list ()
	{
		// destruction in response to 'placement new'

		// call the destructor
		this->datum_list->~DatumList();
		// free the memory
		::operator delete (this->datum_list, this->getMmgr());
	}
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
