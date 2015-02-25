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

#ifndef _QSE_CMN_HASHTABLE_HPP_
#define _QSE_CMN_HASHTABLE_HPP_

/*#include <qse/Hashable.hpp>
#include <qse/cmn/LinkedList.hpp>*/

#include <qse/cmn/Couple.hpp>
#include <qse/cmn/HashList.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template<typename T>
struct HashTableHasher
{
	qse_size_t operator() (const T& v) const
	{
		return v.hashCode();
	}
};

template<typename T>
struct HashTableComparator
{
	// it must return true if two values are equal
	bool operator() (const T& v1, const T& v2) const
	{
		return v1 == v2;
	}
};

#if 0
struct HashTableResizer
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
#endif

typedef HashListResizer HashTableResizer;

template <typename K, typename V, typename MPOOL = Mpool, typename HASHER = HashTableHasher<K>, typename COMPARATOR = HashTableComparator<K>, typename RESIZER = HashTableResizer>
class HashTable: public Mmged
{
public:
	typedef Couple<K,V> Pair;
	typedef HashTable<K,V,MPOOL,HASHER,COMPARATOR,RESIZER> SelfType;

	typedef HashTableHasher<K> DefaultHasher;
	typedef HashTableComparator<K> DefaultComparator;
	typedef HashTableResizer DefaultResizer;


	struct PairHasher
	{
		qse_size_t operator() (const Pair& p)
		{
			HASHER hasher;
			return hasher (p.key);
		}
	};

	struct PairComparator
	{
		qse_size_t operator() (const Pair& p1, const Pair& p2)
		{
			COMPARATOR comparator;
			return comparator (p1.key, p2.key);
		}
	};

	typedef HashList<Pair,MPOOL,PairHasher,PairComparator,RESIZER> PairList;
	typedef typename PairList::Node PairNode;

	enum
	{
		DEFAULT_CAPACITY = PairList::DEFAULT_CAPACITY,
		DEFAULT_LOAD_FACTOR = PairList::DEFAULT_LOAD_FACTOR,

		MIN_CAPACITY = PairList::MIN_CAPACITY,
		MIN_LOAD_FACTOR = PairList::MIN_LOAD_FACTOR
	};

	HashTable (Mmgr* mmgr, qse_size_t capacity = DEFAULT_CAPACITY, qse_size_t load_factor = DEFAULT_LOAD_FACTOR, qse_size_t mpb_size = 0): Mmged(mmgr), pair_list (mmgr, capacity, load_factor, mpb_size)
	{
	}

	HashTable (const SelfType& table): Mmged (table), pair_list (table.pair_list)
	{
	}

	SelfType& operator= (const SelfType& table)
	{
		this->pair_list = table.pair_list;
		return *this;
	}

	Pair* insert (const K& key, const V& value)
	{
		PairNode* node = this->pair_list.insert (Pair(key, value));
		if (!node) return QSE_NULL;
		return &node->value;
	}

	Pair* upsert (const K& key, const V& value)
	{
		PairNode* node = this->pair_list.upsert (Pair(key, value));
		if (!node) return QSE_NULL;
		return &node->value;
	}

	Pair* update (const K& key, const V& value)
	{
		PairNode* node = this->pair_list.update (Pair(key, value));
		if (!node) return QSE_NULL;
		return &node->value;
	}

	Pair* search (const K& key)
	{
		// TODO: find with custom...
		PairNode* node = this->pair_list.update (Pair(key));
		if (!node) return QSE_NULL;
		return &node->value;
	}

	int remove (const K& key)
	{
		// TODO: use removeWithCustom....
		return this->pair_list.remove (Pair(key));
	}


	void clear ()
	{
		// TODO: accept new capacity.
		return this->pair_list.clear ();
	}

	qse_size_t getSize() const
	{
		return this->pair_list.getSize ();
	}

protected:
	PairList pair_list;
};

#if 0
template <typename K, typename V, typename HASHER = HashTableHasher<K>, typename COMPARATOR = HashTableComparator<K>, typename RESIZER = HashTableResizer>
class HashTable: public Mmged
{
public:
	typedef Couple<K,V> Pair;
	typedef LinkedList<Pair> Bucket;
	typedef typename LinkedList<Pair>::Node BucketNode;
	typedef HashTable<K,V,HASHER,COMPARATOR,RESIZER> SelfType;

	typedef HashTableHasher<K> DefaultHasher;
	typedef HashTableComparator<K> DefaultComparator;
	typedef HashTableResizer DefaultResizer;

	enum
	{
		DEFAULT_CAPACITY = 10,
		DEFAULT_LOAD_FACTOR = 75, // Load factor in percentage

		MIN_CAPACITY = 1,
		MIN_LOAD_FACTOR = 20
	};

#if 0
	class Iterator
	{
	public:
		Iterator (): bucket_index(0), bucket_node(QSE_NULL) {}
		Iterator (qse_size_t index, BucketNode* node): bucket_index(index), bucket_node(node) {}
		Iterator (const Iterator& it): bucket_index (it.bucket_index), bucket_node(it.bucket_node) {}

		Iterator& operator= (const Iterator& it) 
		{
			this->bucket_index = it.bucket_index;
			this->bucket_node = it.bucket_node;
			return *this;
		}

		Iterator& operator++ () // prefix increment
		{
			QSE_ASSERT (this->isLegit());
			this->bucket_node = this->bucket_node->getNext();
			if (!this->bucket_node)
			{
				while (this->bucket_index 
			}
			return *this;
		}

		Iterator operator++ (int) // postfix increment
		{
			QSE_ASSERT (this->isLegit());
			Iterator saved (*this);
			this->current = this->current->getNext(); //++(*this);
			return saved;
		}

		Iterator& operator-- () // prefix decrement
		{
			QSE_ASSERT (this->isLegit());
			this->current = this->current->getPrev();
			return *this;
		}

		Iterator operator-- (int) // postfix decrement
		{
			QSE_ASSERT (this->isLegit());
			Iterator saved (*this);
			this->current = this->current->getPrev(); //--(*this);
			return saved;
		}

		bool operator== (const Iterator& it) const
		{
			return this->bucket_index == it.bucket_index &&
			       this->bucket_node == it.bucket_node;
		}

		bool operator!= (const Iterator& it) const
		{
			return this->bucket_index != it.bucket_index ||
			       this->bucket_node != it.bucket_node;
		}

		bool isLegit () const 
		{
			// TODO: change this
			return this->bucket_node != QSE_NULL;
		}

		T& operator* () // dereference
		{
			return this->bucket_node->getValue();
		}

		const T& operator* () const // dereference
		{
			return this->bucket_node->getValue();
		}

	protected:
		SelfType* table;
		qse_size_t bucket_index;
		BucketNode* bucket_node;
	};
#endif

protected:
	Bucket** allocate_bucket (Mmgr* mm, qse_size_t bs, qse_size_t mpb_size) const
	{
		Bucket** b = QSE_NULL;

		try
		{
			b = (Bucket**) mm->callocate (QSE_SIZEOF(*b) * bs);
			for (qse_size_t i = 0; i < bs; i++)
			{
				b[i] = new(mm) Bucket (mm, mpb_size);
			}
		}
		catch (...)
		{
			if (b) 
			{
				for (qse_size_t i = bs; i > 0;)
				{
					i--;
					if (b[i])
					{
						b[i]->~Bucket ();
						::operator delete (b, mm);
					}
				}
				mm->dispose (b);
			}

			throw;
		}

		return b;
	}

	void dispose_bucket (Mmgr* mm, Bucket** b, qse_size_t bs) const
	{
		for (qse_size_t i = bs; i > 0;)
		{
			i--;
			if (b[i])
			{
				b[i]->~Bucket ();
				::operator delete (b[i], mm);
			}
		}
		mm->dispose (b);
	}

public:
	HashTable (Mmgr* mmgr, qse_size_t bucket_size = DEFAULT_CAPACITY, qse_size_t load_factor = DEFAULT_LOAD_FACTOR, qse_size_t bucket_mpb_size = 0): Mmged(mmgr)
	{
		if (bucket_size < MIN_CAPACITY) bucket_size = MIN_CAPACITY;
		if (load_factor < MIN_LOAD_FACTOR) load_factor = MIN_LOAD_FACTOR;

		this->buckets = this->allocate_bucket (this->getMmgr(), bucket_size, bucket_mpb_size);
		this->bucket_size = bucket_size;
		this->pair_count = 0;
		this->load_factor = load_factor;
		this->threshold = bucket_size * load_factor / 100;
		this->bucket_mpb_size = bucket_mpb_size;
	}

	HashTable (const SelfType& table): Mmged (table)
	{
		this->buckets = this->allocate_bucket (this->getMmgr(), table.bucket_size, table.bucket_mpb_size);
		this->bucket_size = table.bucket_size;
		this->pair_count = 0;
		this->load_factor = table.load_factor;
		this->threshold = table.bucket_size * table.load_factor / 100;
		this->bucket_mpb_size = table.bucket_mpb_size;

		for (qse_size_t i = 0; i < table.bucket_size; i++) 
		{
			Bucket* b = table.buckets[i];
			BucketNode* np;
			for (np = b->getHeadNode(); np; np = np->getNext())
			{
				Pair& e = np->value;
				qse_size_t hc = this->hasher(e.key) % this->bucket_size;
				this->buckets[hc]->append (e);
				this->pair_count++;
			}
		}

		// no rehashing is needed in the copy constructor.
		//if (this->pair_count >= threshold) this->rehash ();
	}

	~HashTable ()
	{
		this->clear ();
		this->dispose_bucket (this->getMmgr(), this->buckets, this->bucket_size);
	}

	SelfType& operator= (const SelfType& table)
	{
		this->clear ();

		for (qse_size_t i = 0; i < table.bucket_size; i++) 
		{
			Bucket* b = table.buckets[i];
			BucketNode* np;
			for (np = b->getHeadNode(); np; np = np->getNext()) 
			{
				Pair& e = np->value;
				qse_size_t hc = this->hasher(e.key) % this->bucket_size;
				this->buckets[hc]->append (e);
				this->pair_count++;
			}
		}

		if (this->pair_count >= this->threshold) this->rehash ();
		return *this;
	}

	V& operator[] (const K& key)
	{
		Pair* pair = this->upsert (key);
		QSE_ASSERT (pair != QSE_NULL);
		return pair->value;
	}

	const V& operator[] (const K& key) const
	{
		Pair* pair = this->upsert (key);
		QSE_ASSERT (pair != QSE_NULL);
		return pair->value;
	}

	qse_size_t getSize () const
	{
		return this->pair_count;
	}

	bool isEmpty () const
	{
		return this->pair_count == 0;
	}

	/// The getBucketSize() function returns the number of
	/// pair list currently existing.
	qse_size_t getBucketSize () const
	{
		return this->bucket_size;
	}

	/// The getBucket() function returns the pointer to the 
	/// list of pairs of the given hash value \a index.
	Bucket* getBucket (qse_size_t index)
	{
		QSE_ASSERT (index < this->bucket_size);
		return this->buckets[index];
	}

	/// The getBucket() function returns the pointer to the 
	/// list of pairs of the given hash value \a index.
	const Bucket* getBucket (qse_size_t index) const
	{
		QSE_ASSERT (index < this->bucket_size);
		return this->buckets[index];
	}

protected:
	BucketNode* find_pair_node (const K& key, qse_size_t hc) const
	{
		BucketNode* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext())
		{
			Pair& e = np->value;
			if (this->comparator(key, e.key)) return np;
		}
		return QSE_NULL;
	}

	Pair* find_pair (const K& key, qse_size_t hc) const
	{
		BucketNode* np = this->find_pair_node (key, hc);
		if (np) return &np->value;
		return QSE_NULL;
	}

	template <typename MK, typename MCOMPARATOR>
	BucketNode* find_pair_node_with_custom_key (const MK& key, qse_size_t hc) const
	{
		MCOMPARATOR is_equal;

		BucketNode* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Pair& e = np->value;
			if (is_equal(key, e.key)) return np;
		}

		return QSE_NULL;
	}

	template <typename MK, typename MCOMPARATOR>
	Pair* find_pair_with_custom_key (const MK& key, qse_size_t hc) const
	{
		BucketNode* np = this->find_pair_node_with_custom_key<MK,MCOMPARATOR> (key, hc);
		if (np) return &np->value;
		return QSE_NULL;
	}

public:
	Pair* findPair (const K& key)
	{
		return this->find_pair (key, this->hasher(key) % this->bucket_size);
	}

	const Pair* findPair (const K& key) const
	{
		return this->find_pair (key, this->hasher(key) % this->bucket_size);
	}

	V* findValue (const K& key)
	{
		Pair* pair = this->findPair (key);
		if (pair) return &pair->value;
		return QSE_NULL;
	}

	const V* findValue (const K& key) const
	{
		const Pair* pair = this->findPair (key);
		if (pair) return &pair->value;
		return QSE_NULL;
	}

	V* findValue (const K& key, qse_size_t* hash_code, BucketNode** node)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;
	
		BucketNode* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Pair& e = np->value;
			if (this->comparator(key, e.key))
			{
				*hash_code = hc;
				*node = np;
				return &e.value;
			}
		}

		return QSE_NULL;
	}

	const V* findValue (const K& key, qse_size_t* hash_code, BucketNode** node) const
	{
		qse_size_t hc = this->hasher(key) % bucket_size;
	
		BucketNode* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Pair& e = np->value;
			if (this->comparator(key, e.key))
			{
				*hash_code = hc;
				*node = np;
				return &e.value;
			}
		}

		return QSE_NULL;
	}

	/// \code
	///	typedef QSE::HashTable<int,int,IntHasher> IntTable;
	///	struct IntClass
	///	{
	///		IntClass (int x = 0): x (x) {}
	///		int x;
	///	};
	///	struct IntClassHasher
	///	{
	///		qse_size_t operator() (const IntClass& v) const { return v.x; }
	///	};
	///	struct IntClassComparator
	///	{
	///		bool operator() (const IntClass& v, int y) const { return v.x == y; }
	///		}
	///	};
	/// int main ()
	/// {
	///    IntTable int_list (NULL, 1000);
	///    ...
	///    IntTable::Pair* pair = int_list.findPairWithCustomKey<IntClass,IntClassHasher,IntClassComparator> (IntClass(50));
	///    ...
	/// }
	/// \endcode

	template <typename MK, typename MHASHER, typename MCOMPARATOR>
	Pair* findPairWithCustomKey (const MK& key)
	{
		MHASHER hash;
		return this->find_pair_with_custom_key<MK,MCOMPARATOR> (key, hash(key) % this->bucket_size);
	}

	template <typename MK, typename MHASHER, typename MCOMPARATOR>
	const Pair* findPairWithCustomKey (const MK& key) const
	{
		MHASHER hash;
		return this->find_pair_with_custom_key<MK,MCOMPARATOR> (key, hash(key) % this->bucket_size);
	}

	/// The search() function returns the pointer to a pair of the \a key.
	/// If no pair is found, it returns #QSE_NULL.
	Pair* search (const K& key)
	{
		return this->find_pair (key, this->hasher(key) % this->bucket_size);
	}

	/// The search() function returns the pointer to a pair of the \a key.
	/// If no pair is found, it returns #QSE_NULL.
	const Pair* search (const K& key) const
	{
		return this->find_pair (key, this->hasher(key) % this->bucket_size);
	}

	/// The insert() function inserts a new pair with a \a key with the default value.
	/// if the key is not found in the table. It returns the pointer to the 
	/// new pair inserted. If the key is found, it returns #QSE_NULL
	Pair* insert (const K& key)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;

		Pair* pair = this->find_pair (key, hc);
		if (pair) return QSE_NULL; // existing pair found.

		if (this->pair_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % bucket_size;
		}

		// insert a new pair
		BucketNode* node = this->buckets[hc]->append (Pair(key));
		this->pair_count++;
		return &node->value;
	}

	/// The insert() function inserts a new pair with a \a key with a \a value.
	/// if the key is not found in the table. It returns the pointer to the 
	/// new pair inserted. If the key is found, it returns #QSE_NULL
	Pair* insert (const K& key, const V& value)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;

		Pair* pair = this->find_pair (key, hc);
		if (pair) return QSE_NULL; // existing pair found.

		if (this->pair_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % bucket_size;
		}

		// insert a new pair
		BucketNode* node = this->buckets[hc]->append (Pair(key, value));
		this->pair_count++;
		return &node->value;
	}

	/// The update() function updates an existing pair of the \a key
	/// with a \a value and returns a pointer to the pair. If the key
	/// is not found, it returns #QSE_NULL.
	Pair* update (const K& key, const V& value)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;
		Pair* pair = this->find_pair (key, hc);
		if (pair) pair->value = value; // update the existing pair
		return pair;
	}

	/// The upsert() function inserts a new pair with a \a key and a \a value
	/// if the key is not found in the table. If it is found, it updated the
	/// existing pair with a given \a value.
	Pair* upsert (const K& key)
	{
		qse_size_t hc = this->hasher(key) % this->bucket_size;

		Pair* pair = this->find_pair (key, hc);
		if (pair) return pair; // return the existing pair

		if (this->pair_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % this->bucket_size;
		}

		// insert a new pair if the key is not found
		BucketNode* node = this->buckets[hc]->append (Pair(key));
		this->pair_count++;
		return &node->value;
	}

	/// The upsert() function inserts a new pair with a \a key and a \a value
	/// if the key is not found in the table. If it is found, it updated the
	/// existing pair with a given \a value.
	Pair* upsert (const K& key, const V& value)
	{
		qse_size_t hc = this->hasher(key) % this->bucket_size;

		Pair* pair = this->find_pair (key, hc);
		if (pair) 
		{
			// update the value of the existing pair
			pair->value = value;
			return pair;
		}

		if (this->pair_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % this->bucket_size;
		}

		// insert a new pair if the key is not found
		BucketNode* node = this->buckets[hc]->append (Pair(key, value));
		this->pair_count++;
		return &node->value;
	}

protected:
	int remove (qse_size_t hc, BucketNode* np)
	{
		//
		// WARNING: this method should be used with extra care.
		//
		this->buckets[hc]->remove (np);
		this->pair_count--;
		return 0;
	}

public:
	int remove (const K& key)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;

		BucketNode* np = this->find_pair_node (key, hc);
		if (np)
		{
			this->remove (hc, np);
			return 0;
		}

		return -1;
	}

	template <typename MK, typename MHASHER, typename MCOMPARATOR>
	int removeWithCustomKey (const MK& key)
	{
		MHASHER hash;
		qse_size_t hc = hash(key) % this->bucket_size;

		BucketNode* np = this->find_pair_node_with_custom_key<MK,MCOMPARATOR> (key, hc);
		if (np)
		{
			this->remove (hc, np);
			return 0;
		}

		return -1;
	}

	void clear (qse_size_t new_bucket_size = 0)
	{
		for (qse_size_t i = 0; i < bucket_size; i++) this->buckets[i]->clear ();
		this->pair_count = 0;

		if (new_bucket_size > 0)
		{
			Bucket** tmp = this->allocate_bucket (this->getMmgr(), new_bucket_size, this->bucket_mpb_size);
			this->dispose_bucket (this->getMmgr(), this->buckets, this->bucket_size);

			this->buckets = tmp;
			this->bucket_size = new_bucket_size;
			this->threshold   = this->bucket_size * this->load_factor / 100;
		}
	}

	typedef int (SelfType::*TraverseCallback) 
		(const Pair& entry, void* user_data) const;

	int traverse (TraverseCallback callback, void* user_data) const
	{
		for (qse_size_t i = 0; i < this->bucket_size; i++) 
		{
			BucketNode* np;
			for (np = this->buckets[i]->getHeadNode(); np; np = np->getNext()) 
			{
				const Pair& e = np->value;
				if ((this->*callback)(e,user_data) == -1) return -1;
			}
		}

		return 0;
	}

	typedef int (*StaticTraverseCallback) 
		(SelfType* table, Pair& entry, void* user_data);

	int traverse (StaticTraverseCallback callback, void* user_data) 
	{
		for (qse_size_t i = 0; i < this->bucket_size; i++) 
		{
			BucketNode* np;
			for (np = this->buckets[i]->getHeadNode(); np; np = np->getNext()) 
			{
				Pair& e = np->value;
				if (callback(this,e,user_data) == -1) return -1;
			}
		}

		return 0;
	}

	//Iterator getIterator ()
	//{
	//}

protected:
	mutable qse_size_t  pair_count;
	mutable qse_size_t  bucket_size;
	mutable Bucket**    buckets;
	mutable qse_size_t  threshold;
	qse_size_t          load_factor;
	qse_size_t          bucket_mpb_size;
	HASHER              hasher;
	COMPARATOR          comparator;
	RESIZER             resizer;

	void rehash () const
	{
		qse_size_t new_bucket_size = this->resizer (this->bucket_size);
		Bucket** new_buckets = this->allocate_bucket (this->getMmgr(), new_bucket_size, this->bucket_mpb_size);

		try 
		{
			for (qse_size_t i = 0; i < this->bucket_size; i++) 
			{
				/*
				BucketNode* np;
				for (np = this->buckets[i]->getHeadNode(); np; np = np->getNext()) 
				{
					const Pair& e = np->value;
					qse_size_t hc = e.key.hashCode() % new_bucket_size;
					new_this->buckets[hc]->append (e);
				}
				*/

				// this approach save redundant memory allocation
				// and retains the previous pointers before rehashing.
				// if the bucket uses a memory pool, this would not
				// work. fortunately, the hash table doesn't use it
				// for a bucket. ---> this is not true any more.
				//               ---> this has been broken as memory pool
				//               ---> can be activated for buckets.
				BucketNode* np = this->buckets[i]->getHeadNode();
				while (np)
				{
					BucketNode* next = np->getNext();
					const Pair& e = np->value;
					qse_size_t hc = this->hasher(e.key) % new_bucket_size;
					new_buckets[hc]->appendNode (this->buckets[i]->yield(np));
					np = next;
				}
			}
		}
		catch (...) 
		{
			this->dispose_bucket (this->getMmgr(), new_buckets, new_bucket_size);
			throw;
		}

		this->dispose_bucket (this->getMmgr(), this->buckets, this->bucket_size);

		this->buckets     = new_buckets;
		this->bucket_size = new_bucket_size;
		this->threshold   = this->load_factor * this->bucket_size / 100;
	}
};
#endif

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
