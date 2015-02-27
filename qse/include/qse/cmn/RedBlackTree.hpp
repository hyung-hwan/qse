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

#ifndef _QSE_CMN_REDBLACKTREE_HPP_
#define _QSE_CMN_REDBLACKTREE_HPP_

#include <qse/Types.hpp>
#include <qse/cmn/Mpool.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <typename T> 
class RedBlackTreeNode
{
public:
	friend class RedBlackTree<T>;
	typedef RedBlackTreeNode<T> SelfType;

	enum
	{
		RED,
		BLACK
	};

	T value; // you can use this variable or accessor functions below

protected:
	SelfType* parent;
	SelfType* child[2]; // left and right

public:
	T& getValue () { return this->value; }
	const T& getValue () const { return this->value; }
	void setValue (const T& v) { this->value = v; }
};

template<typename T>
struct RedBlackTreeComparator
{
	int operator() (const T& v1, const T& v2) const
	{
		return (v1 > v2)? 1: 
		       (v1 < v2)? -1: 0;
	}
};

template <typename T, typename COMPARATOR = RedBlackTreeComparator<T> >
class RedBlackTree: public Mmged
{
public:
	typedef RedBlackTree<T,COMPARATOR> SelfType;

	typedef RedBlackTreeHasher<T> DefaultHasher;
	typedef RedBlackTreeComparator<T> DefaultComparator;

	RedBlackTree
		Mmgr* mmgr = QSE_NULL,
		qse_size_t mpb_size = 0): Mmged(mmgr)
	{
	}

	RedBlackTree (const RedBlackTree& rbt)
	{
	}

	~RedBlackTree ()
	{
	}

	RedBlackTree& operator= (const RedBlackTree& rbt)
	{
		/* TODO */
		return *this;
	}

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
		return this->node_count <= 0;
	}

public:

	Node* inject (const T& datum, int mode, bool* injected = QSE_NULL)
	{
		return QSE_NULL;
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

#if 0
	Node* update (const T& datum)
	{
		Node* node = this->find_node (datum);
		if (node) node->value = datum;
		return node;
	}
#endif

protected:
	Mpool      mp;
	COMPARATOR comparator;

	Node       xnil; // internal node to present nil 
	Node*      root; // root node.
	qse_size_t node_count;
};


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
