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

#include <qse/Hashable.hpp>
#include <qse/cmn/LinkedList.hpp>
#include <qse/cmn/Pair.hpp>

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
	bool operator() (const T& v1, const T& v2) const
	{
		return v1 == v2;
	}
};

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

template <typename K, typename V, typename HASHER = HashTableHasher<K>, typename COMPARATOR = HashTableComparator<K>, typename RESIZER = HashTableResizer>
class HashTable: public Mmged
{
public:
	typedef Pair<K,V> Entry;
	typedef LinkedList<Entry> Bucket;
	typedef HashTable<K,V,HASHER,COMPARATOR,RESIZER> SelfType;

protected:
	Bucket** allocate_bucket (Mmgr* mm, qse_size_t bs) const
	{
		Bucket** b = QSE_NULL;

		try
		{
			b = (Bucket**) mm->callocate (QSE_SIZEOF(*b) * bs);
			for (qse_size_t i = 0; i < bs; i++)
			{
				b[i] = new(mm) Bucket (mm);
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
	HashTable (Mmgr* mmgr, qse_size_t bucket_size = 10, qse_size_t load_factor = 75): Mmged(mmgr)
	{
		this->buckets = this->allocate_bucket (this->getMmgr(), bucket_size);
		this->bucket_size = bucket_size;
		this->entry_count = 0;
		this->load_factor = load_factor;
		this->threshold   = bucket_size * load_factor / 100;
	}

	HashTable (const SelfType& table): Mmged (table)
	{
		this->buckets = this->allocate_bucket (this->getMmgr(), table.bucket_size);
		this->bucket_size = table.bucket_size;
		this->entry_count = 0;
		this->load_factor = table.load_factor;
		this->threshold   = table.bucket_size * table.load_factor / 100;

		for (qse_size_t i = 0; i < table.bucket_size; i++) 
		{
			Bucket* b = table.buckets[i];
			typename Bucket::Node* np;
			for (np = b->getHeadNode(); np; np = np->getNext())
			{
				Entry& e = np->value;
				qse_size_t hc = this->hasher(e.key) % this->bucket_size;
				this->buckets[hc]->append (e);
				this->entry_count++;
			}
		}

		// doesn't need to rehash in the copy constructor.
		//if (entry_count >= threshold) this->rehash ();
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
			typename Bucket::Node* np;
			for (np = b->getHeadNode(); np; np = np->getNext()) 
			{
				Entry& e = np->value;
				qse_size_t hc = this->hasher(e.key) % this->bucket_size;
				this->buckets[hc]->append (e);
				entry_count++;
			}
		}

		if (this->entry_count >= this->threshold) this->rehash ();
		return *this;
	}

	qse_size_t getSize () const
	{
		return this->entry_count;
	}

	bool isEmpty () const
	{
		return this->entry_count == 0;
	}

	qse_size_t getBucketSize () const
	{
		return this->bucket_size;
	}

	Bucket& getBucket (qse_size_t index) const
	{
		QSE_ASSERT (index < this->bucket_size);
		return this->buckets[index];
	}

#if 0
	V& operator[] (const K& key)
	{
		qse_size_t hc = this->hasher(key) % this->bucket_size;
	
		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext())
		{
			Entry& e = np->value;
			if (this->comparator (key, e.key)) return e.value;
		}

		if (entry_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % bucket_size;
		}

		// insert a new key if the key is not found
		Entry& e2 = this->buckets[hc]->append (Entry(key));
		entry_count++;
		return e2.value;
	}

	const V& operator[] (const K& key) const
	{
		qse_size_t hc = this->hasher(key) % this->bucket_size;

		Entry* pair = this->find_pair (key, hc);
		if (pair) return pair->value;

		// insert a new key if the key is not found
		Entry& new_pair = this->buckets[hc]->append (Entry(key));
		this->entry_count++;
		return new_pair.value;
	}
#endif

	//
	// NOTE: getConstWithCustomKey() and getWithCustomKey() would
	//       not need to have different names if compilers were smarter.
	//

	template <typename MK, typename MHASHER>
	const V* getConstWithCustomKey (const MK& key) const
	{
		MHASHER h;
		qse_size_t hc = h(key) % bucket_size;
	
		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			const Entry& e = np->value;
			if (key == e.key) return &e.value;
		}

		return QSE_NULL;
	}

	template <typename MK, typename MHASHER>
	V* getWithCustomKey (const MK& key) const
	{
		MHASHER h;
		qse_size_t hc = h(key) % bucket_size;

		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext())
		{
			Entry& e = np->value;
			if (key == e.key) return &e.value;
		}

		return QSE_NULL;
	}

protected:
	Entry* find_pair (const K& key, qse_size_t hc)
	{
		//qse_size_t hc = this->hasher(key) % this->bucket_size;
	
		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext())
		{
			Entry& e = np->value;
			if (this->comparator (key, e.key)) return &e;
		}

		return QSE_NULL;
	}

public:
	Entry* findPair (const K& key)
	{
		return this->find_pair (key, this->hasher(key) % this->bucket_size);
	}

	const Entry* findPair (const K& key) const
	{
		return this->find_pair (key, this->hasher(key) % this->bucket_size);
	}

	V* findValue (const K& key)
	{
		Entry* pair = this->findPair (key);
		if (pair) return &pair->value;
		return QSE_NULL;
	}

	const V* findValue (const K& key) const
	{
		const Entry* pair = this->findPair (key);
		if (pair) return &pair->value;
		return QSE_NULL;
	}

	V* findValue (const K& key, qse_size_t* hash_code, typename Bucket::Node** node)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;
	
		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (this->comparator (key, e.key))
			{
				*hash_code = hc;
				*node = np;
				return &e.value;
			}
		}

		return QSE_NULL;
	}

	const V* findValue (const K& key, qse_size_t* hash_code, typename Bucket::Node** node) const
	{
		qse_size_t hc = this->hasher(key) % bucket_size;
	
		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (this->comparator (key, e.key))
			{
				*hash_code = hc;
				*node = np;
				return &e.value;
			}
		}

		return QSE_NULL;
	}

	int put (const K& key, const V& value)
	{
		upsert (key, value);
		return 0;
	}

	int putNew (const K& key, const V& value)
	{
		return (insertNew(key,value) == QSE_NULL)? -1: 0;
	}

	Entry* insert (const K& key)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;

		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (this->comparator (key, e.key)) return &e;
		}

		if (entry_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % bucket_size;
		}

		Entry& e = this->buckets[hc]->append (Entry(key));
		this->entry_count++;
		return &e;
	}

	Entry* insert (const K& key, const V& value)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;

		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (this->comparator (key, e.key)) return &e;
		}

		if (entry_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % bucket_size;
		}

		Entry& e = this->buckets[hc]->append (Entry(key,value));
		entry_count++;
		return &e;
	}

#if 0
	Entry* upsert (const K& Key)
	{
		qse_size_t hc = this->hasher(key) % this->bucket_size;

		Entry* pair = this->find_pair (key, hc);
		if (pair) 
		{
			// don't change the existing value.
			return pair
		}

		if (entry_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % this->bucket_size;
		}

		// insert a new key if the key is not found
		Entry& new_pair = this->buckets[hc]->append (Entry(key));
		this->entry_count++;
		return &new_pair
	}
#endif

	Entry* upsert (const K& key, const V& value)
	{
		qse_size_t hc = this->hasher(key) % this->bucket_size;

		Entry* pair = this->find_pair (key, hc);
		if (pair) 
		{
			pair->value = value;
			return pair;
		}

		if (entry_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % this->bucket_size;
		}

		// insert a new key if the key is not found
		Entry& new_pair = this->buckets[hc]->append (Entry(key,value));
		this->entry_count++;
		return &new_pair;

		/*
		qse_size_t hc = this->hasher(key) % this->bucket_size;

		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (key == e.key) 
			{
				e.value = value;
				return &e.value;
			}
		}

		if (entry_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % bucket_size;
		}

		Entry& e = this->buckets[hc]->append (Entry(key,value));
		this->entry_count++;
		return &e.value;
		*/
	}


#if 0
	V* insertNew (const K& key)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;

		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (key == e.key) return QSE_NULL;
		}

		if (entry_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % bucket_size;
		}

		Entry& e = this->buckets[hc]->append (Entry(key));
		entry_count++;
		return &e.value;
	}

	V* insertNew (const K& key, const V& value)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;

		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (key == e.key) return QSE_NULL;
		}

		if (entry_count >= threshold) 
		{
			this->rehash ();
			hc = this->hasher(key) % bucket_size;
		}

		Entry& e = this->buckets[hc]->append (Entry(key, value));
		entry_count++;
		return &e.value;
	}
#endif


	template <typename MK, typename MHASHER>
	int removeWithCustomKey (const MK& key)
	{
		MHASHER h;
		qse_size_t hc = h(key) % bucket_size;

		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (key == e.key) 
			{
				this->buckets[hc]->remove (np);
				this->entry_count--;
				return 0;
			}
		}

		return -1;
	}

	int remove (const K& key)
	{
		qse_size_t hc = this->hasher(key) % bucket_size;

		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (key == e.key) 
			{
				this->buckets[hc]->remove (np);
				this->entry_count--;
				return 0;
			}
		}

		return -1;
	}

	int remove (qse_size_t hc, typename Bucket::Node* np)
	{
		//
		// WARNING: this method should be used with extra care.
		//
		this->buckets[hc]->remove (np);
		this->entry_count--;
		return 0;
	}

#if 0
	qse_size_t removeValue (const V& value)
	{
		qse_size_t count = 0;

		for (qse_size_t i = 0; i < this->bucket_size; i++) 
		{
			typename Bucket::Node* np, * np2;
			np = this->buckets[i]->getHeadNode();
			while (np != QSE_NULL) 
			{
				Entry& e = np->value;
				np2 = np->getNext ();
				if (value == e.value) 
				{
					this->remove (i, np);
					count++;
				}
				np = np2;
			}
		}

		return count;
	}
#endif

	bool containsKey (const K& key) const
	{
		qse_size_t hc = this->hasher(key) % bucket_size;
	
		typename Bucket::Node* np;
		for (np = this->buckets[hc]->getHeadNode(); np; np = np->getNext()) 
		{
			Entry& e = np->value;
			if (key == e.key) return true;
		}

		return false;
	}

	bool containsValue (const K& value) const
	{
		for (qse_size_t i = 0; i < bucket_size; i++) 
		{
			typename Bucket::Node* np;
			for (np = this->buckets[i]->getHeadNode(); np; np = np->getNext()) 
			{
				Entry& e = np->value;
				if (value == e.value) return true;
			}
		}

		return false;
	}

	void clear (qse_size_t new_bucket_size = 0)
	{
		for (qse_size_t i = 0; i < bucket_size; i++) this->buckets[i]->clear ();
		this->entry_count = 0;

		if (new_bucket_size > 0)
		{
			Bucket** tmp = this->allocate_bucket (this->getMmgr(), new_bucket_size);
			this->dispose_bucket (this->getMmgr(), this->buckets, this->bucket_size);

			this->buckets = tmp;
			this->bucket_size = new_bucket_size;
			this->threshold   = this->bucket_size * this->load_factor / 100;
		}
	}

	typedef int (SelfType::*TraverseCallback) 
		(const Entry& entry, void* user_data) const;

	int traverse (TraverseCallback callback, void* user_data) const
	{
		for (qse_size_t i = 0; i < this->bucket_size; i++) 
		{
			typename Bucket::Node* np;
			for (np = this->buckets[i]->getHeadNode(); np; np = np->getNext()) 
			{
				const Entry& e = np->value;
				if ((this->*callback)(e,user_data) == -1) return -1;
			}
		}

		return 0;
	}

	typedef int (*StaticTraverseCallback) 
		(SelfType* table, Entry& entry, void* user_data);

	int traverse (StaticTraverseCallback callback, void* user_data) 
	{
		for (qse_size_t i = 0; i < this->bucket_size; i++) 
		{
			typename Bucket::Node* np;
			for (np = this->buckets[i]->getHeadNode(); np; np = np->getNext()) 
			{
				Entry& e = np->value;
				if (callback(this,e,user_data) == -1) return -1;
			}
		}

		return 0;
	}
	
protected:
	mutable qse_size_t  entry_count;
	mutable qse_size_t  bucket_size;
	mutable Bucket**    buckets;
	mutable qse_size_t  threshold;
	qse_size_t load_factor;
	HASHER hasher;
	COMPARATOR comparator;
	RESIZER resizer;

	void rehash () const
	{
		qse_size_t new_bucket_size = this->resizer (this->bucket_size);
		Bucket** new_buckets = this->allocate_bucket (this->getMmgr(), new_bucket_size);

		try 
		{
			for (qse_size_t i = 0; i < this->bucket_size; i++) 
			{
				/*
				typename Bucket::Node* np;
				for (np = this->buckets[i]->getHeadNode(); np; np = np->getNext()) 
				{
					const Entry& e = np->value;
					qse_size_t hc = e.key.hashCode() % new_bucket_size;
					new_this->buckets[hc]->append (e);
				}
				*/

				// this approach save redundant memory allocation
				// and retains the previous pointers before rehashing.
				// if the bucket uses a memory pool, this would not
				// work. fortunately, the hash table doesn't use it
				// for a bucket.
				typename Bucket::Node* np = this->buckets[i]->getHeadNode();
				while (np)
				{
					typename Bucket::Node* next = np->getNext();
					const Entry& e = np->value;
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

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
