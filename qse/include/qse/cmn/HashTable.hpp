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

#include <qse/cmn/Association.hpp>
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
struct HashTableEqualer
{
	// it must return true if two values are equal
	bool operator() (const T& v1, const T& v2) const
	{
		return v1 == v2;
	}
};

typedef HashListResizer HashTableResizer;

template <typename K, typename V, typename HASHER = HashTableHasher<K>, typename EQUALER = HashTableEqualer<K>, typename RESIZER = HashTableResizer>
class HashTable: public Mmged
{
public:
	typedef Association<K,V> Pair;
	typedef HashTable<K,V,HASHER,EQUALER,RESIZER> SelfType;

	typedef HashTableHasher<K> DefaultHasher;
	typedef HashTableEqualer<K> DefaultEqualer;
	typedef HashTableResizer DefaultResizer;

	struct PairHasher
	{
		qse_size_t operator() (const Pair& p) const
		{
			HASHER hasher;
			return hasher (p.key);
		}
	};

	struct PairEqualer
	{
		qse_size_t operator() (const Pair& p1, const Pair& p2) const
		{
			EQUALER equaler;
			return equaler (p1.key, p2.key);
		}
	};

	struct PairHeteroEqualer
	{
		qse_size_t operator() (const K& p1, const Pair& p2) const
		{
			EQUALER equaler;
			return equaler (p1, p2.key);
		}
	};

	template <typename MK, typename MEQUALER>
	struct MHeteroEqualer
	{
		qse_size_t operator() (const MK& p1, const Pair& p2) const
		{
			MEQUALER mequaler;
			return mequaler (p1, p2.key);
		}
	};

	typedef HashList<Pair,PairHasher,PairEqualer,RESIZER> PairList;
	typedef typename PairList::Node PairNode;
	typedef typename PairList::Iterator Iterator;
	typedef typename PairList::ConstIterator ConstIterator;

	enum
	{
		DEFAULT_CAPACITY = PairList::DEFAULT_CAPACITY,
		DEFAULT_LOAD_FACTOR = PairList::DEFAULT_LOAD_FACTOR,

		MIN_CAPACITY = PairList::MIN_CAPACITY,
		MIN_LOAD_FACTOR = PairList::MIN_LOAD_FACTOR
	};

	HashTable (Mmgr* mmgr = QSE_NULL, 
	           qse_size_t capacity = DEFAULT_CAPACITY,
	           qse_size_t load_factor = DEFAULT_LOAD_FACTOR,
	           qse_size_t mpb_size = 0):
		Mmged(mmgr), pair_list (mmgr, capacity, load_factor, mpb_size)
	{
	}

	HashTable (const SelfType& table): Mmged (table), pair_list (table.pair_list)
	{
	}

	SelfType& operator= (const SelfType& table)
	{
		if (this != &table)
		{
			this->pair_list = table.pair_list;
		}
		return *this;
	}

	Mpool& getMpool () 
	{
		return this->pair_list.getMpool ();
	}

	const Mpool& getMpool () const
	{
		return this->pair_list.getMpool ();
	}

	qse_size_t getCapacity() const
	{
		return this->pair_list.getCapacity ();
	}

	qse_size_t getSize() const
	{
		return this->pair_list.getSize ();
	}

	bool isEmpty () const 
	{
		return this->pair_list.isEmpty ();
	}

	PairNode* getHeadNode ()
	{
		return this->pair_list.getHeadNode ();
	}

	const PairNode* getHeadNode () const
	{
		return this->pair_list.getHeadNode ();
	}

	PairNode* getTailNode ()
	{
		return this->pair_list.getTailNode ();
	}

	const PairNode* getTailNode () const
	{
		return this->pair_list.getTailNode ();
	}

	Pair* inject (const K& key, const V& value, int mode, bool* injected = QSE_NULL)
	{
		PairNode* node = this->pair_list.inject (Pair(key, value), mode, injected);
		if (!node) return QSE_NULL;
		return &node->value;
	}

	Pair* insert (const K& key, const V& value)
	{
		PairNode* node = this->pair_list.insert (Pair(key, value));
		if (!node) return QSE_NULL;
		return &node->value;
	}

	Pair* ensert (const K& key, const V& value)
	{
		PairNode* node = this->pair_list.ensert (Pair(key, value));
		if (!node) return QSE_NULL;
		return &node->value;
	}

	Pair* upsert (const K& key, const V& value)
	{
		//PairNode* node = this->pair_list.upsert (Pair(key, value));
		//if (!node) return QSE_NULL;
		//return &node->value;

		// Don't call pair_list.upsert() to make sure that the 'key' object
		// itself remains identical after potential update operation. 
		// pair_list.upsert() changes the Pair object as a whole. so this
		// trick is required.
		bool injected;
		PairNode* node = this->pair_list.inject (Pair(key, value), 0, &injected);
		QSE_ASSERT (node != QSE_NULL);
		Pair& pair = node->value;
		if (injected) pair.value = value;
		return &pair;
	}

	Pair* update (const K& key, const V& value)
	{
		//PairNode* node = this->pair_list.update (Pair(key, value));
		//if (!node) return QSE_NULL;
		//return &node->value;

		PairNode* node = this->pair_list.template heterofindNode<K,HASHER,PairHeteroEqualer> (key);
		if (!node) return QSE_NULL;
		Pair& pair = node->value;
		pair.value = value;
		return &pair;
	}

	Pair* search (const K& key)
	{
		//PairNode* node = this->pair_list.update (Pair(key));
		PairNode* node = this->pair_list.template heterofindNode<K,HASHER,PairHeteroEqualer> (key);
		if (!node) return QSE_NULL;
		return &node->value;
	}

	int remove (const K& key)
	{
		//return this->pair_list.remove (Pair(key));
		return this->pair_list.template heteroremove<K,HASHER,PairHeteroEqualer> (key);
	}

	template <typename MK, typename MHASHER, typename MEQUALER>
	Pair* heterosearch (const MK& key)
	{
		typedef MHeteroEqualer<MK,MEQUALER> MEqualer;
		PairNode* node = this->pair_list.template heterosearch<MK,MHASHER,MEqualer> (key);
		if (!node) return QSE_NULL;
		return &node->value;
	}

	template <typename MK, typename MHASHER, typename MEQUALER>
	const Pair* heterosearch (const MK& key) const
	{
		typedef MHeteroEqualer<MK,MEQUALER> MEqualer;
		PairNode* node = this->pair_list.template heterosearch<MK,MHASHER,MEqualer> (key);
		if (!node) return QSE_NULL;
		return &node->value;
	}

	template <typename MK, typename MHASHER, typename MEQUALER>
	int heteroremove (const MK& key)
	{
		typedef MHeteroEqualer<MK,MEQUALER> MEqualer;
		return this->pair_list.template heteroremove<MK,MHASHER,MEqualer> (key);
	}

	void clear (bool clear_mpool = false)
	{
		return this->pair_list.clear (clear_mpool);
	}

	Iterator getIterator (qse_size_t index = 0)
	{
		return this->pair_list.getIterator (index);
	}

	ConstIterator getConstIterator (qse_size_t index = 0) const
	{
		return this->pair_list.getConstIterator (index);
	}

protected:
	PairList pair_list;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
