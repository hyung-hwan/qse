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

/// \file
/// Provides the RedBlackTree class.

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

	// You must take extreme care not to screw up the whole tree by
	// overriding the 'value' variable with a randome value.
	T value; // you can use this variable or accessor functions below

protected:
	Color color;
	SelfType* up;
	SelfType* left; // left child
	SelfType* right; // right child

	RedBlackTreeNode(): color (BLACK), up (this), left (this), right (this)
	{
		// no initialization on 'value' in this constructor.
	}

	RedBlackTreeNode(const T& value, Color color, SelfType* up, SelfType* left, SelfType* right):
		value (value), color (color), up (up), left (left), right (right)
	{
		QSE_ASSERT (up != this);
		QSE_ASSERT (left != this);
		QSE_ASSERT (right != this);
	}

public:
	T& getValue () { return this->value; }
	const T& getValue () const { return this->value; }
	void setValue (const T& v) { this->value = v; }

	bool isNil () const
	{
		return this->up == this; // && this->left == this && this->right == this;
	}

	bool notNil () const
	{
		return !this->isNil ();
	}

	bool isBlack () const  { return this->color == BLACK; }
	bool isRed () const { return this->color == RED; }

	SelfType* getUpNode () { return this->up; }
	const SelfType* getUpNode () const { return this->up; }

	
	SelfType* getLeftNode () { return this->left; }
	const SelfType* getLeftNode () const { return this->left; }

	SelfType* getRightNode () { return this->right; }
	const SelfType* getRightNode () const { return this->right; }

	//void setBlack () { this->color = BLACK; }
	//void setRed () { this->color = RED; }
	//void setUpNode (SelfType* node) { this->up = node; }
	//void setLeftNode (SelfType* node) { this->left = node; }
	//void setRightNode (SelfType* node) { this->right = node; }
};

template <typename T>
struct RedBlackTreeComparator
{
	int operator() (const T& v1, const T& v2) const
	{
		if (v1 > v2) return 1;
		if (v1 < v2) return -1;
		QSE_ASSERT (v1 == v2);
		return 0;
	}
};

template <typename T, typename COMPARATOR, typename GET_NODE, typename GET_T>
class RedBlackTreeIterator
{
public:
	typedef RedBlackTreeNode<T,COMPARATOR> Node;
	typedef RedBlackTreeIterator<T,COMPARATOR,GET_NODE,GET_T> SelfType;

	typedef RedBlackTreeComparator<T> DefaultComparator;

	typedef Node* (Node::*GetChild) ();

	enum Mode
	{
		ASCENDING,
		DESCENDING
	};

	RedBlackTreeIterator (): 
		pending_action (0), current (QSE_NULL), previous (QSE_NULL),
		get_left (QSE_NULL), get_right (QSE_NULL) 
	{
	}

	RedBlackTreeIterator (Node* root, Mode mode): pending_action (0), current (root)
	{
		QSE_ASSERT (root != QSE_NULL);

		this->previous = root->getUpNode();
		if (mode == DESCENDING)
		{
			this->get_left = &Node::getRightNode;
			this->get_right = &Node::getLeftNode;
		}
		else 
		{
			this->get_left = &Node::getLeftNode;
			this->get_right = &Node::getRightNode;
		}

		this->__move_to_next_node ();
	}

protected:
	void __move_to_next_node ()
	{
		QSE_ASSERT (this->current != QSE_NULL);

		while (this->current->notNil())
		{
			if (this->previous == this->current->getUpNode())
			{
				/* the previous node is the parent of the current node.
				 * it indicates that we're going down to the getChild(l) */
				if ((this->current->*this->get_left)()->notNil())
				{
					/* go to the left child */
					this->previous = this->current;
					this->current = (this->current->*this->get_left)();
				}
				else
				{
					this->pending_action = 1;
					break;
				}
			}
			else if (this->previous == (this->current->*this->get_left)())
			{
				/* the left child has been already traversed */
				this->pending_action = 2;
				break;
			}
			else
			{
				/* both the left child and the right child have been traversed */
				QSE_ASSERT (this->previous == (this->current->*this->get_right)());
				/* just move up to the parent */
				this->previous = this->current;
				this->current = this->current->getUpNode();
			}
		}
	}

	void move_to_next_node ()
	{
		if (pending_action ==  1)
		{
			if ((this->current->*this->get_right)()->notNil())
			{
				/* go down to the right node if exists */
				this->previous = this->current;
				this->current = (this->current->*this->get_right)();
			}
			else
			{
				/* otherwise, move up to the parent */
				this->previous = this->current;
				this->current = this->current->getUpNode();
			}
		}
		else if (pending_action == 2)
		{
			if ((this->current->*this->get_right)()->notNil())
			{
				/* go down to the right node if it exists */ 
				this->previous = this->current;
				this->current = (this->current->*this->get_right)();
			}
			else
			{
				/* otherwise, move up to the parent */
				this->previous = this->current;
				this->current = this->current->getUpNode();
			}
		}

		this->__move_to_next_node ();
	}

public:
	SelfType& operator++ () // prefix increment
	{
		this->move_to_next_node ();
		return *this;
	}

	SelfType operator++ (int) // postfix increment
	{
		SelfType saved (*this);
		this->move_to_next_node ();
		return saved;
	}

	// no operator--

	bool operator== (const SelfType& it) const
	{
		QSE_ASSERT (this->current != QSE_NULL);
		return this->current == it.current &&
		       this->previous == it.previous;
	}

	bool operator!= (const SelfType& it) const
	{
		QSE_ASSERT (this->current != QSE_NULL);
		return this->current != it.current ||
		       this->previous != it.previous;
	}

	bool isLegit() const
	{
		return current->notNil();
	}

	GET_T& operator* () // dereference
	{
		return this->current->getValue();
	}

	GET_T& getValue ()
	{
		return this->current->getValue();
	}

	// no setValue().

	GET_NODE* getNode ()
	{
		return this->current;
	}

protected:
	int pending_action;
	Node* current;
	Node* previous;
	//Node* (Node::*get_left) ();
	//Node* (Node::*get_right) ();
	GetChild get_left;
	GetChild get_right;
};


/// The RedBlackTree class implements the red-black tree data structure.
///
///   A node is either red or black.
///   The root is black.
///   All leaves (NIL) are black. (All leaves are same color as the root.)
///   Every red node must have two black child nodes.
///   Every path from a given node to any of its descendant NIL nodes contains the same number of black nodes.
///
/// \sa RedBlackTable, qse_rbt_t
///
template <typename T, typename COMPARATOR = RedBlackTreeComparator<T> >
class RedBlackTree: public Mmged
{
public:
	typedef RedBlackTree<T,COMPARATOR> SelfType;
	typedef RedBlackTreeNode<T,COMPARATOR> Node;
	typedef RedBlackTreeIterator<T,COMPARATOR,Node,T> Iterator;
	typedef RedBlackTreeIterator<T,COMPARATOR,const Node,const T> ConstIterator;

	typedef RedBlackTreeComparator<T> DefaultComparator;

private:
	void init_tree ()
	{
	#if defined(QSE_REDBLACKTREE_ALLOCATE_NIL)
		// create a nil object. note it doesn't go into the memory pool.
		// the nil node allocated inside the memory pool makes implementation
		// of this->clear (true) difficult as disposal of memory pool
		// also deallocates the nil node.
		this->nil = new(this->getMmgr()) Node();
	#else
		this->nil = &this->xnil;
	#endif

		// set root to nil
		this->root = this->nil;
	}

public:
	RedBlackTree (qse_size_t mpb_size = 0):
		Mmged(QSE_NULL), mp(QSE_NULL, QSE_SIZEOF(Node), mpb_size), node_count(0)
	{
		this->init_tree ();
	}

	RedBlackTree (Mmgr* mmgr, qse_size_t mpb_size = 0):
		Mmged(mmgr), mp(mmgr, QSE_SIZEOF(Node), mpb_size), node_count(0)
	{
		this->init_tree ();
	}

	RedBlackTree (const SelfType& rbt): 
		Mmged(rbt.getMmgr()),
		mp(rbt.getMmgr(), rbt.mp.getDatumSize(), rbt.mp.getBlockSize()),
		node_count(0)
	{
	#if defined(QSE_REDBLACKTREE_ALLOCATE_NIL)
		// create a nil object. note it doesn't go into the memory pool.
		// the nil node allocated inside the memory pool makes implementation
		// of this->clear (true) difficult as disposal of memory pool
		// also deallocates the nil node.
		this->nil = new(this->getMmgr()) Node();
	#else
		this->nil = &this->xnil;
	#endif

		// set root to nil
		this->root = this->nil;

		// TODO: do the level-order traversal to minimize rebalancing.
		Iterator it = rbt.getIterator();
		while (it.isLegit())
		{
			this->insert (*it);
			++it;
		}
	}

	~RedBlackTree ()
	{
		this->clear (true);

	#if defined(QSE_REDBLACKTREE_ALLOCATE_NIL)
		// destroy the nil node.
		QSE_CPP_CALL_DESTRUCTOR (this->nil, Node);
		QSE_CPP_CALL_PLACEMENT_DELETE1 (this->nil, this->getMmgr());
	#else
		// do nothing
	#endif
	}

	SelfType& operator= (const SelfType& rbt)
	{
		if (this != &rbt)
		{
			this->clear (false);

			// TODO: do the level-order traversal to minimize rebalancing.
			Iterator it = rbt.getIterator();
			while (it.isLegit())
			{
				this->insert (*it);
				++it;
			}
		}

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

	/// The getRootNode() function gets the pointer to the root node.
	/// If no node exists in the tree, it returns #QSE_NULL.
	Node* getRootNode ()
	{
		return this->root->isNil()? QSE_NULL: this->root;
	}

	const Node* getRootNode () const
	{
		return this->root->isNil()? QSE_NULL: this->root;
	}

protected:
	void dispose_node (Node* node)
	{
		QSE_CPP_CALL_DESTRUCTOR (node, Node);
		QSE_CPP_CALL_PLACEMENT_DELETE1 (node, &this->mp);
	}

	Node* find_node (const T& datum) const
	{
		Node* node = this->root;

		// normal binary tree search
		while (node->notNil())
		{
			int n = this->comparator (datum, node->value);
			if (n == 0) return node;

			if (n > 0) node = node->right;
			else /* if (n < 0) */ node = node->left;
		}

		return QSE_NULL;
	}

	template <typename MT, typename MCOMPARATOR>
	Node* heterofind_node (const MT& datum) const
	{
		MCOMPARATOR mcomparator;
		Node* node = this->root;

		// normal binary tree search
		while (node->notNil())
		{
			int n = mcomparator (datum, node->value);
			if (n == 0) return node;

			if (n > 0) node = node->right;
			else /* if (n < 0) */ node = node->left;
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

		Node* up, * z, * c;

		QSE_ASSERT (pivot != QSE_NULL);

		up = pivot->up;
		if (leftwise)
		{
			// y for leftwise rotation
			z = pivot->right;
			// c1 for leftwise rotation
			c = z->left;
		}
		else
		{
			// x for rightwise rotation
			z = pivot->left;
			// c2 for rightwise rotation
			c = z->right;
		}

		z->up = up;
		if (up->notNil())
		{
			if (up->left == pivot)
			{
				up->left = z;
			}
			else
			{
				QSE_ASSERT (up->right == pivot);
				up->right = z;
			}
		}
		else
		{
			QSE_ASSERT (this->root == pivot);
			this->root = z;
		}

		if (leftwise)
		{
			z->left = pivot;
			pivot->right = c;
		}
		else
		{
			z->right = pivot;
			pivot->left = c;
		}

		if (pivot->notNil()) pivot->up = z;
		if (c->notNil()) c->up = pivot;
	}

	void rotate_left (Node* pivot)
	{
		this->rotate (pivot, true);
	}

	void rotate_right (Node* pivot)
	{
		this->rotate (pivot, false);
	}

	void rebalance_for_injection (Node* node)
	{
		while (node != this->root)
		{
			Node* tmp, * tmp2, * x_par, * x_grand_par;
			bool leftwise;

			x_par = node->up;
			if (x_par->color == Node::BLACK) break;

			QSE_ASSERT (x_par->up->notNil());

			x_grand_par = x_par->up;
			if (x_par == x_grand_par->left)
			{
				tmp = x_grand_par->right;
				tmp2 = x_par->right;
				leftwise = true;
			}
			else
			{
				tmp = x_grand_par->left;
				tmp2 = x_par->left;
				leftwise = false;
			}

			if (tmp->color == Node::RED)
			{
				x_par->color = Node::BLACK;
				tmp->color = Node::BLACK;
				x_grand_par->color = Node::RED;
				node = x_grand_par;
			}
			else
			{
				if (node == tmp2)
				{
					node = x_par;
					this->rotate (node, leftwise);
					x_par = node->up;
					x_grand_par = x_par->up;
				}

				x_par->color = Node::BLACK;
				x_grand_par->color = Node::RED;
				this->rotate (x_grand_par, !leftwise);
			}
		}
	}

	void rebalance_for_removal (Node* node, Node* par)
	{
		while (node != this->root && node->color == Node::BLACK)
		{
			Node* tmp;

			if (node == par->left)
			{
				tmp = par->right;
				if (tmp->color == Node::RED)
				{
					tmp->color = Node::BLACK;
					par->color = Node::RED;
					this->rotate_left (par);
					tmp = par->right;
				}

				if (tmp->left->color == Node::BLACK &&
				    tmp->right->color == Node::BLACK)
				{
					if (tmp->notNil()) tmp->color = Node::RED;
					node = par;
					par = node->up;
				}
				else
				{
					if (tmp->right->color == Node::BLACK)
					{
						if (tmp->left->notNil())
							tmp->left->color = Node::BLACK;
						tmp->color = Node::RED;
						this->rotate_right (tmp);
						tmp = par->right;
					}

					tmp->color = par->color;
					if (par->notNil()) par->color = Node::BLACK;
					if (tmp->right->color == Node::RED)
						tmp->right->color = Node::BLACK;

					this->rotate_left (par);
					node = this->root;
				}
			}
			else
			{
				QSE_ASSERT (node == par->right);
				tmp = par->left;
				if (tmp->color == Node::RED)
				{
					tmp->color = Node::BLACK;
					par->color = Node::RED;
					this->rotate_right (par);
					tmp = par->left;
				}

				if (tmp->left->color == Node::BLACK &&
				    tmp->right->color == Node::BLACK)
				{
					if (tmp->notNil()) tmp->color = Node::RED;
					node = par;
					par = node->up;
				}
				else
				{
					if (tmp->left->color == Node::BLACK)
					{
						if (tmp->right->notNil())
							tmp->right->color = Node::BLACK;
						tmp->color = Node::RED;
						this->rotate_left (tmp);
						tmp = par->left;
					}
					tmp->color = par->color;
					if (par->notNil()) par->color = Node::BLACK;
					if (tmp->left->color == Node::RED)
						tmp->left->color = Node::BLACK;

					this->rotate_right (par);
					node = this->root;
				}
			}
		}

		node->color = Node::BLACK;
	}

	void remove_node (Node* node)
	{
		Node* x, * y, * par;

		QSE_ASSERT (node && node->notNil());

		if (node->left->isNil() || node->right->isNil())
		{
			y = node;
		}
		else
		{
			/* find a successor with NIL as a child */
			y = node->right;
			while (y->left->notNil()) y = y->left;
		}

		x = (y->left->isNil())? y->right: y->left;

		par = y->up;
		if (x->notNil()) x->up = par;

		if (par->notNil()) // if (par)
		{
			if (y == par->left)
				par->left = x;
			else
				par->right = x;
		}
		else
		{
			this->root = x;
		}

		if (y == node)
		{
			if (y->color == Node::BLACK && x->notNil())
				this->rebalance_for_removal (x, par); 

			this->dispose_node (y);
		}
		else
		{
			if (y->color == Node::BLACK && x->notNil())
				this->rebalance_for_removal (x, par);

			if (node->up->notNil()) //if (node->up)
			{
				if (node->up->left == node) node->up->left = y;
				if (node->up->right == node) node->up->right = y;
			}
			else
			{
				this->root = y;
			}

			y->up = node->up;
			y->left = node->left;
			y->right = node->right;
			y->color = node->color;

			if (node->left->up == node) node->left->up = y;
			if (node->right->up == node) node->right->up = y;

			this->dispose_node (node);
		}

		this->node_count--;
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

	template <typename MT, typename MCOMPARATOR>
	Node* heterofindNode (const MT& datum)
	{
		return this->heterofind_node<MT,MCOMPARATOR> (datum);
	}

	template <typename MT, typename MCOMPARATOR>
	const Node* heterofindNode (const MT& datum) const
	{
		return this->heterofind_node<MT,MCOMPARATOR> (datum);
	}

	template <typename MT, typename MCOMPARATOR>
	T* heterofindValue(const MT& datum)
	{
		Node* b = this->heterofind_node<MT,MCOMPARATOR> (datum);
		if (!b) return QSE_NULL;
		return &b->value;
	}

	template <typename MT, typename MCOMPARATOR>
	const T* heterofindValue(const MT& datum) const
	{
		Node* b = this->heterofind_node<MT,MCOMPARATOR> (datum);
		if (!b) return QSE_NULL;
		return &b->value;
	}

	Node* search (const T& datum)
	{
		return this->find_node (datum);
	}

	const Node* search (const T& datum) const
	{
		return this->find_node (datum);
	}

	template <typename MT, typename MCOMPARATOR>
	Node* heterosearch (const MT& datum)
	{
		return this->heterofind_node<MT,MCOMPARATOR> (datum);
	}

	template <typename MT, typename MCOMPARATOR>
	const Node* heterosearch (const MT& datum) const
	{
		return this->heterofind_node<MT,MCOMPARATOR> (datum);
	}

	/// The inject() function inserts a \a datum if no existing datum
	/// is found to be equal using the comparator. The \a mode argument
	/// determines what action to take when an equal datum is found.
	/// - -1: failure
	/// -  0: do nothing
	/// -  1: overwrite the existing datum
	///
	/// if \a injected is not #QSE_NULL, it is set to true when \a datum
	/// has been inserted newly and false when an equal datum has been
	/// found.
	///
	/// The function returns the poniter to the node inserted or 
	/// affected. It return #QSE_NULL if mode is set to -1 and a duplicate
	/// item has been found.
	Node* inject (const T& datum, int mode, bool* injected = QSE_NULL)
	{
		Node* x_cur = this->root;
		Node* x_par = this->nil;

		while (x_cur->notNil())
		{
			int n = this->comparator (datum, x_cur->value);
			if (n == 0)
			{
				if (injected) *injected = false;
				if (mode <= -1) return QSE_NULL; // return failure
				if (mode >= 1) x_cur->value = datum;
				return x_cur;
			}

			x_par = x_cur;

			if (n > 0) x_cur = x_cur->right;
			else /* if (n < 0) */ x_cur = x_cur->left;
		}

		Node* x_new = new(&this->mp) Node (datum, Node::RED, this->nil, this->nil, this->nil);
		if (x_par->isNil())
		{
			QSE_ASSERT (this->root->isNil());
			this->root = x_new;
		}
		else
		{
			int n = this->comparator (datum, x_par->value);
			if (n > 0)
			{
				QSE_ASSERT (x_par->right->isNil());
				x_par->right = x_new;
			}
			else
			{
				QSE_ASSERT (x_par->left->isNil());
				x_par->left = x_new;
			}

			x_new->up = x_par;
			this->rebalance_for_injection (x_new);
		}

		this->root->color = Node::BLACK;
		this->node_count++;

		if (injected) *injected = true; // indicate that a new node has been injected
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

	Node* update (const T& datum)
	{
		Node* node = this->find_node (datum);
		if (node) node->value = datum;
		return node;
	}

	int remove (const T& datum)
	{
		Node* node = this->find_node (datum);
		if (node == QSE_NULL) return -1;

		this->remove_node (node);
		return 0;
	}

	template <typename MT, typename MCOMPARATOR>
	int heteroremove (const MT& datum)
	{
		Node* node = this->QSE_CPP_TEMPLATE_QUALIFIER heterofind_node<MT,MCOMPARATOR> (datum);
		if (node == QSE_NULL) return -1;

		this->remove_node (node);
		return 0;
	}

	void clear (bool clear_mpool = false)
	{
		while (this->root->notNil()) this->remove_node (this->root);
		QSE_ASSERT (this->root = this->nil);
		QSE_ASSERT (this->node_count == 0);

		if (clear_mpool) this->mp.dispose ();
	}

	Iterator getIterator (typename Iterator::Mode mode = Iterator::ASCENDING) const
	{
		return Iterator (this->root, mode);
	}

	ConstIterator getConstIterator (typename ConstIterator::Mode mode = ConstIterator::ASCENDING) const
	{
		return ConstIterator (this->root, mode);
	}

protected:
	Mpool      mp;
	COMPARATOR comparator;

	qse_size_t node_count;
#if defined(QSE_REDBLACKTREE_ALLOCATE_NIL)
	// nothing. let the constructor allocate it to this->nil.
#else
	// use a statically declared nil object.
	Node       xnil;
#endif

	Node*      nil; // internal node to present nil
	Node*      root; // root node.
};


/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
