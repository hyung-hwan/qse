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

template <typename T, typename COMPARATOR> class RedBlackTree;

template <typename T, typename COMPARATOR> 
class RedBlackTreeNode
{
public:
	friend class RedBlackTree<T,COMPARATOR>;
	typedef RedBlackTreeNode<T,COMPARATOR> SelfType;

	enum Color
	{
		RED,
		BLACK
	};

	enum Child
	{
		LEFT,
		RIGHT
	};

	T value; // you can use this variable or accessor functions below

protected:
	Color color;
	SelfType* parent;
	SelfType* child[2]; // left and right

	RedBlackTreeNode()
	{
		// no initialization. make sure to initialize member variables later
	}

	RedBlackTreeNode(const T& value, Color color, SelfType* parent, SelfType* left, SelfType* right):
		value (value), color (color), parent (parent)
	{
		this->child[LEFT] = left;
		this->child[RIGHT] = right;
	}

public:
	T& getValue () { return this->value; }
	const T& getValue () const { return this->value; }
	void setValue (const T& v) { this->value = v; }

	bool isBlack () const  { return this->color == BLACK; }
	bool isRed () const { return this->color == RED; }
	void setBlack () { this->color = BLACK; }
	void setRed () { this->color = RED; }

	SelfType* getParent () { return this->parent; }
	const SelfType* getParent () const { return this->parent; }

	SelfType* getLeft () { return this->child[LEFT]; }
	const SelfType* getLeft () const { return this->child[LEFT]; }

	SelfType* getRight () { return this->child[RIGHT]; }
	const SelfType* getRight () const { return this->child[RIGHT]; }

	void setParent (SelfType* node) { this->parent = node; }
	void setLeft (SelfType* node) { this->child[LEFT] = node; }
	void setRight (SelfType* node) { this->child[RIGHT] = node; }

	void setAll (Color color, SelfType* parent, SelfType* left, SelfType* right)
	{
		this->color = color;
		this->parent = parent;
		this->child[LEFT] = left;
		this->child[RIGHT] = right;
	}
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
	typedef RedBlackTreeNode<T,COMPARATOR> Node;

	typedef RedBlackTreeComparator<T> DefaultComparator;

	RedBlackTree (Mmgr* mmgr = QSE_NULL, qse_size_t mpb_size = 0): Mmged(mmgr),  mp (mmgr, QSE_SIZEOF(Node), mpb_size), node_count (0)
	{
		// initialize nil
		this->nil = new(&this->mp) Node();
		this->nil->setAll (Node::BLACK, this->nil, this->nil, this->nil);

		// set root to nil
		this->root = this->nil;
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

	bool isNil (Node* node) const
	{
		return node == this->nil;
	}

	bool notNil (Node* node) const
	{
		return node != this->nil;
	}

	Node* getRoot () 
	{
		return this->root;
	}

	const Node* getRoot () const
	{
		return this->root;
	}

protected:
	Node* find_node (const T& datum) const
	{
		Node* node = this->root;

		// normal binary tree search
		while (notNil (node))
		{
			int n = this->comparator (datum, node->value);
			if (n == 0) return node;

			if (n > 0) node = node->getRight();
			else /* if (n < 0) */ node = node->getLeft();
		}

		return QSE_NULL;
	}

	void rotate (Node* pivot, bool leftwise)
	{
		/*
		 * == leftwise rotation
		 * move the pivot pair down to the poistion of the pivot's original 
		 * left child(x). move the pivot's right child(y) to the pivot's original 
		 * position. as 'c1' is between 'y' and 'pivot', move it to the right 
		 * of the new pivot position.
		 *       parent                   parent 
		 *        | | (left or right?)      | |
		 *       pivot                      y
		 *       /  \                     /  \
		 *     x     y    =====>      pivot   c2
		 *          / \               /  \
		 *         c1  c2            x   c1
		 *
		 * == rightwise rotation
		 * move the pivot pair down to the poistion of the pivot's original 
		 * right child(y). move the pivot's left child(x) to the pivot's original 
		 * position. as 'c2' is between 'x' and 'pivot', move it to the left 
		 * of the new pivot position.
		 *
		 *       parent                   parent 
		 *        | | (left or right?)      | |
		 *       pivot                      x
		 *       /  \                     /  \
		 *     x     y    =====>        c1   pivot          
		 *    / \                            /  \
		 *   c1  c2                         c2   y
		 *
		 *
		 * the actual implementation here resolves the pivot's relationship to
		 * its parent by comparaing pointers as it is not known if the pivot pair
		 * is the left child or the right child of its parent, 
		 */

		Node* parent, * z, * c;

		QSE_ASSERT (pivot != QSE_NULL);

		parent = pivot->getParent();
		if (leftwise)
		{
			// y for leftwise rotation
			z = pivot->getRight();
			// c1 for leftwise rotation
			c = z->getLeft();
		}
		else
		{
			// x for rightwise rotation
			z = pivot->getLeft();
			// c2 for rightwise rotation
			c = z->getRight();
		}

		z->setParent (parent);
		if (notNil (parent))
		{
			if (parent->getLeft() == pivot)
			{
				parent->setLeft (z);
			}
			else
			{
				QSE_ASSERT (parent->getRight() == pivot);
				parent->setRight (z);
			}
		}
		else
		{
			QSE_ASSERT (this->root == pivot);
			this->root = z;
		}

		if (leftwise)
		{
			z->setLeft (pivot);
			pivot->setRight (c);
		}
		else
		{
			z->setRight (pivot);
			pivot->setLeft (c);
		}

		if (notNil(pivot)) pivot->setParent (z);
		if (notNil(c)) c->setParent (pivot);
	}

	void adjust (Node* node)
	{
		while (node != this->root)
		{
			Node* tmp, * tmp2, * x_par, * x_par_par;
			bool leftwise;

			x_par = node->getParent ();
			if (x_par->isBlack()) break;

			QSE_ASSERT (notNil (x_par->parent));

			x_par_par = x_par->getParent ();
			if (x_par == x_par_par->getLeft ()) 
			{
				tmp = x_par_par->getRight ();
				tmp2 = x_par->getRight ();
				leftwise = true;
			}
			else
			{
				tmp = x_par_par->getLeft ();
				tmp2 = x_par->getLeft ();
				leftwise = false;
			}

			if (tmp->isRed ())
			{
				x_par->setBlack ();
				tmp->setBlack ();
				x_par_par->setRed ();
				node = x_par_par;
			}
			else
			{
				if (node == tmp2)
				{
					node = x_par;
					this->rotate (node, leftwise);
					x_par = node->getParent();
					x_par_par = x_par->getParent();
				}

				x_par->setBlack();
				x_par_par->setRed();
				this->rotate (x_par_par, !leftwise);
			}
		}
	}


public:
	Node* search (const T& datum)
	{
		return this->find_node (datum);
	}

	const Node* search (const T& datum) const
	{
		return this->find_node (datum);
	}

	Node* inject (const T& datum, int mode, bool* injected = QSE_NULL)
	{
		Node* x_cur = this->root;
		Node* x_par = this->nil;

		while (notNil (x_cur))
		{
			int n = this->comparator (datum, x_cur->value);
			if (n == 0) 
			{
			#if 0
				switch (opt)
				{
					case UPSERT:
					case UPDATE:
						return change_pair_val (rbt, x_cur, vptr, vlen);

					case ENSERT:
						/* return existing pair */
						return x_cur; 

					case INSERT:
						/* return failure */
						return QSE_NULL;
				}
			#endif
			}

			x_par = x_cur;

			if (n > 0) x_cur = x_cur->getRight ();
			else /* if (n < 0) */ x_cur = x_cur->getLeft ();
		}

		//if (opt == UPDATE) return QSE_NULL;

		Node* x_new = new(&this->mp) Node (datum, Node::RED, this->nil, this->nil, this->nil);
		if (isNil (x_par))
		{
			QSE_ASSERT (isNil (this->root));
			this->root = x_new;
		}
		else
		{
			int n = this->comparator (datum, x_par->value);
			if (n > 0)
			{
				QSE_ASSERT (isNil (x_par->getRight ()));
				x_par->setRight (x_new);
			}
			else
			{
				QSE_ASSERT (isNil (x_par->getLeft ()));
				x_par->setLeft (x_new);
			}

			x_new->setParent (x_par);
			this->adjust (x_new);
		}

		this->root->setBlack ();
		this->node_count++;

		return x_new;
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

	void dump (Node* node)
	{
		printf ("%d %d\n", node->value.getX(), node->value.getY());
		if (notNil(node->getLeft())) dump (node->getLeft());
		if (notNil(node->getRight())) dump (node->getRight());
	}

protected:
	Mpool      mp;
	COMPARATOR comparator;

	qse_size_t node_count;
	Node*      nil; // internal node to present nil 
	Node*      root; // root node.

};


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
