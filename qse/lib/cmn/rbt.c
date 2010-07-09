/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/rbt.h>
#include "mem.h"

#define IS_NIL(rbt,x) ((x) == &((rbt)->nil))

#define KTOB(htb,len) ((len)*(htb)->scale[QSE_RBT_KEY])

#define LEFT 0
#define RIGHT 1

#define left child[LEFT]
#define right child[RIGHT]

#define rotate_left(rbt,pivot) rotate(rbt,pivot,1);
#define rotate_right(rbt,pivot) rotate(rbt,pivot,0);

QSE_IMPLEMENT_COMMON_FUNCTIONS (rbt)

static QSE_INLINE void init_node (
	qse_rbt_t* rbt, qse_rbt_node_t* node, int color, 
	int key, int value)
{
	QSE_MEMSET (node, 0, QSE_SIZEOF(*node)); 

	node->color = color;
	node->right = &rbt->nil;
	node->left = &rbt->nil;

	node->key = key;
	node->value = value;
}

static QSE_INLINE int comp_key (
	qse_rbt_t* rbt,
	const void* kptr1, qse_size_t klen1,
	const void* kptr2, qse_size_t klen2)
{
	qse_size_t min;
	int n, nn;

	if (klen1 < klen2)
	{
		min = klen1;
		nn = -1;
	}
	else
	{
		min = klen2;
		nn = (klen1 == klen2)? 0: 1;
	}

	n = QSE_MEMCMP (kptr1, kptr2, KTOB(rbt,min));
	if (n == 0) n = nn;
	return n;
}

qse_rbt_t* qse_rbt_open (qse_mmgr_t* mmgr, qse_size_t ext)
{
	qse_rbt_t* rbt;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	rbt = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_rbt_t) + ext);
	if (rbt == QSE_NULL) return QSE_NULL;

	if (qse_rbt_init (rbt, mmgr) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, rbt);
		return QSE_NULL;
	}

	return rbt;
}

void qse_rbt_close (qse_rbt_t* rbt)
{
	qse_rbt_fini (rbt);
	QSE_MMGR_FREE (rbt->mmgr, rbt);
}

qse_rbt_t* qse_rbt_init (qse_rbt_t* rbt, qse_mmgr_t* mmgr)
{
	/* do not zero out the extension */
	QSE_MEMSET (rbt, 0, QSE_SIZEOF(*rbt));

	rbt->mmgr = mmgr;
	rbt->size = 0;

	rbt->scale[QSE_RBT_KEY] = 1;
	rbt->scale[QSE_RBT_VAL] = 1;

	rbt->comper = comp_key;
	rbt->copier[QSE_RBT_KEY] = QSE_RBT_COPIER_SIMPLE;
	rbt->copier[QSE_RBT_VAL] = QSE_RBT_COPIER_SIMPLE;

	init_node (rbt, &rbt->nil, QSE_RBT_BLACK, 0, 0);
	rbt->root = &rbt->nil;
	return rbt;
}

void qse_rbt_fini (qse_rbt_t* rbt)
{
	qse_rbt_clear (rbt);
}

qse_rbt_node_t* qse_rbt_search (qse_rbt_t* rbt, int key)
{
	/* TODO: enhance this. ues comper... etc */

	qse_rbt_node_t* node = rbt->root;

	while (!IS_NIL(rbt,node))
	{
#if 0
		int n = rbt->comper (rbt, kptr, klen, node->kptr, node->klen);
		if (n == 0) return node;

		if (n > 0) node = node->right;
		else /* if (n < 0) */ node = node->left;
#endif
		if (key == node->key) return node;

		if (key > node->key) node = node->right;
		else /* if (key < node->key) */ node = node->left;
	}

	return QSE_NULL;
}

#if 0
static void rotate_left (qse_rbt_t* rbt, qse_rbt_node_t* pivot)
{
	/* move the pivot node down to the poistion of the pivot's original 
	 * left child(x). move the pivot's right child(y) to the pivot's original 
	 * position. as 'c1' is between 'y' and 'pivot', move it to the right 
	 * of the new pivot position.
	 *
	 *       parent                   parent 
	 *        | | (left or right?)      | |
	 *       pivot                      y
	 *       /  \                     /  \
	 *     x     y    =====>      pivot   c2
	 *          / \               /  \
	 *         c1  c2            x   c1
	 *
	 * the actual implementation here resolves the pivot's relationship to
	 * its parent by comparaing pointers as it is not known if the pivot node
	 * is the left child or the right child of its parent, 
	 */

	qse_rbt_node_t* parent, * y, * c1;

	QSE_ASSERT (pivot != QSE_NULL && pivot->right != QSE_NULL);

	parent = pivot->parent;
	y = pivot->right;
	c1 = y->left;

	y->parent = parent;
	if (parent)
	{
		if (parent->left == pivot)
		{
			parent->left = y;
		}
		else
		{
			QSE_ASSERT (parent->right == pivot);
			parent->right = y;
		}
	}
	else
	{
		QSE_ASSERT (rbt->root == pivot);
		rbt->root = y;
	}

	y->left = pivot;
	if (!IS_NIL(rbt,pivot)) pivot->parent = y;

	pivot->right = c1;
	if (!IS_NIL(rbt,c1)) c1->parent = pivot;
}

static void rotate_right (qse_rbt_t* rbt, qse_rbt_node_t* pivot)
{
	/* move the pivot node down to the poistion of the pivot's original 
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
	 * the actual implementation here resolves the pivot's relationship to
	 * its parent by comparaing pointers as it is not known if the pivot node
	 * is the left child or the right child of its parent, 
	 */

	qse_rbt_node_t* parent, * x, * c2;

	QSE_ASSERT (pivot != QSE_NULL);

	parent = pivot->parent;
	x = pivot->left;
	c2 = x->right;

	x->parent = parent;
	if (parent)
	{
		if (parent->left == pivot)
		{
			/* pivot is the left child of its parent */
			parent->left = x;
		}
		else
		{
			/* pivot is the right child of its parent */
			QSE_ASSERT (parent->right == pivot);
			parent->right = x;
		}
	}
	else
	{
		/* pivot is the root node */
		QSE_ASSERT (rbt->root == pivot);
		rbt->root = x;
	}

	x->right = pivot;
	if (!IS_NIL(rbt,pivot)) pivot->parent = x;

	pivot->left = c2;
	if (!IS_NIL(rbt,c2)) c2->parent = pivot;
}
#endif

static void rotate (qse_rbt_t* rbt, qse_rbt_node_t* pivot, int leftwise)
{
	/*
	 * == leftwise rotation
	 * move the pivot node down to the poistion of the pivot's original 
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
	 * move the pivot node down to the poistion of the pivot's original 
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
	 * its parent by comparaing pointers as it is not known if the pivot node
	 * is the left child or the right child of its parent, 
	 */

	qse_rbt_node_t* parent, * z, * c;
	int cid1, cid2;

	QSE_ASSERT (pivot != QSE_NULL);

	if (leftwise)
	{
		cid1 = RIGHT;
		cid2 = LEFT;
	}
	else
	{
		cid1 = LEFT;
		cid2 = RIGHT;
	}

	parent = pivot->parent;
	/* y for leftwise rotation, x for rightwise rotation */
	z = pivot->child[cid1]; 
	/* c1 for leftwise rotation, c1 for rightwise rotation */
	c = z->child[cid2]; 

	z->parent = parent;
	if (parent)
	{
		if (parent->left == pivot)
		{
			parent->left = z;
		}
		else
		{
			QSE_ASSERT (parent->right == pivot);
			parent->right = z;
		}
	}
	else
	{
		QSE_ASSERT (rbt->root == pivot);
		rbt->root = z;
	}

	z->child[cid2] = pivot;
	if (!IS_NIL(rbt,pivot)) pivot->parent = z;

	pivot->child[cid1] = c;
	if (!IS_NIL(rbt,c)) c->parent = pivot;
}

static void adjust (qse_rbt_t* rbt, qse_rbt_node_t* node)
{
	while (node != rbt->root)
	{
		qse_rbt_node_t* tmp, * tmp2;
		int leftwise;

		qse_rbt_node_t* xpar = node->parent;
		if (xpar->color == QSE_RBT_BLACK) break;

		QSE_ASSERT (xpar->parent != QSE_NULL);

		if (xpar == xpar->parent->child[LEFT]) 
		{
			tmp = xpar->parent->child[RIGHT];
			tmp2 = xpar->child[RIGHT];
			leftwise = 1;
		}
		else
		{
			tmp = xpar->parent->child[LEFT];
			tmp2 = xpar->child[LEFT];
			leftwise = 0;
		}

		if (tmp->color == QSE_RBT_RED)
		{
			xpar->color = QSE_RBT_BLACK;
			tmp->color = QSE_RBT_BLACK;
			xpar->parent->color = QSE_RBT_RED;	
			node = xpar->parent;
		}
		else
		{
			if (node == tmp2)
			{
				node = xpar;
				rotate (rbt, node, leftwise);
				xpar = node->parent;
			}

			xpar->color = QSE_RBT_BLACK;
			xpar->parent->color = QSE_RBT_RED;
			rotate (rbt, xpar->parent, !leftwise);
		}
	}
}

static qse_rbt_node_t* new_node (qse_rbt_t* rbt, int key, int value)
{
	qse_rbt_node_t* node;

	node = QSE_MMGR_ALLOC (rbt->mmgr, QSE_SIZEOF(*node));
	if (node == QSE_NULL) return QSE_NULL;

	init_node (rbt, node, QSE_RBT_RED, key, value);
	return node;
}

static void free_node (qse_rbt_t* rbt, qse_rbt_node_t* node)
{
	/* TODO: call destructor... */
	QSE_MMGR_FREE (rbt->mmgr, node);
}

qse_rbt_node_t* qse_rbt_insert (qse_rbt_t* rbt, int key, int value)
{
	/* TODO: enhance this. ues comper... etc */

	qse_rbt_node_t* xcur = rbt->root;
	qse_rbt_node_t* xpar = QSE_NULL;
	qse_rbt_node_t* xnew; 

	while (!IS_NIL(rbt,xcur))
	{
		if (key == xcur->key)
		{
			/* TODO: handle various cases depending on insert types. 
			 * return error. update value. */
			xcur->value = value;
			return xcur;
		}

		xpar = xcur;

		if (key > xcur->key) xcur = xcur->right;
		else /* if (key < xcur->key) */ xcur = xcur->left;
	}

	xnew = new_node (rbt, key, value);
	if (xnew == QSE_NULL) return QSE_NULL;

	if (xpar == QSE_NULL)
	{
		/* the tree contains no node */
		QSE_ASSERT (rbt->root == &rbt->nil);
		rbt->root = xnew;
	}
	else
	{
		/* perform normal binary insert */
		if (key > xpar->key)
		{
			QSE_ASSERT (xpar->right == &rbt->nil);
			xpar->right = xnew;
		}
		else
		{
			QSE_ASSERT (xpar->left == &rbt->nil);
			xpar->left = xnew;
		}

		xnew->parent = xpar;
		adjust (rbt, xnew);
	}

	rbt->root->color = QSE_RBT_BLACK;
	return xnew;
}

static void adjust_for_delete (
	qse_rbt_t* rbt, qse_rbt_node_t* node, qse_rbt_node_t* par)
{
	while (node != rbt->root && node->color == QSE_RBT_BLACK)
	{
		qse_rbt_node_t* tmp;

		if (node == par->left)
		{
			tmp = par->right;
			if (tmp->color == QSE_RBT_RED)
			{
				tmp->color = QSE_RBT_BLACK;
				par->color = QSE_RBT_RED;
				rotate_left (rbt, par);
				tmp = par->right;
			}

			if (tmp->left->color == QSE_RBT_BLACK &&
			    tmp->right->color == QSE_RBT_BLACK)
			{
				if (!IS_NIL(rbt,tmp)) tmp->color = QSE_RBT_RED;
				node = par;
				par = node->parent;
			}
			else
			{
				if (tmp->right->color == QSE_RBT_BLACK)
				{
					if (!IS_NIL(rbt,tmp->left)) 
						tmp->left->color = QSE_RBT_BLACK;
					tmp->color = QSE_RBT_RED;
					rotate_right (rbt, tmp);
					tmp = par->right;
				}

				tmp->color = par->color;
				if (!IS_NIL(rbt,par)) par->color = QSE_RBT_BLACK;
				if (tmp->right->color == QSE_RBT_RED)
					tmp->right->color = QSE_RBT_BLACK;

				rotate_left (rbt, par);
				node = rbt->root;
			}
		}
		else
		{
			QSE_ASSERT (node == par->right);
			tmp = par->left;
			if (tmp->color == QSE_RBT_RED)
			{
				tmp->color = QSE_RBT_BLACK;
				par->color = QSE_RBT_RED;
				rotate_right (rbt, par);
				tmp = par->left;
			}

			if (tmp->left->color == QSE_RBT_BLACK &&
			    tmp->right->color == QSE_RBT_BLACK)
			{
				if (!IS_NIL(rbt,tmp)) tmp->color = QSE_RBT_RED;
				node = par;
				par = node->parent;
			}
			else
			{
				if (tmp->left->color == QSE_RBT_BLACK)
				{
					if (!IS_NIL(rbt,tmp->right))
						tmp->right->color = QSE_RBT_BLACK;
					tmp->color = QSE_RBT_RED;
					rotate_left (rbt, tmp);
					tmp = par->left;
				}
				tmp->color = par->color;
				if (!IS_NIL(rbt,par)) par->color = QSE_RBT_BLACK;
				if (tmp->left->color == QSE_RBT_RED)
					tmp->left->color = QSE_RBT_BLACK;

				rotate_right (rbt, par);
				node = rbt->root;
			}
		}
	}

	node->color = QSE_RBT_BLACK;
}

static void delete_node (qse_rbt_t* rbt, qse_rbt_node_t* node)
{
	qse_rbt_node_t* x, * y, * par;

	QSE_ASSERT (node && !IS_NIL(rbt,node));

	if (IS_NIL(rbt,node->left) || IS_NIL(rbt,node->right))
	{
		y = node;
	}
	else
	{
		/* find a successor with NIL as a child */
		y = node->right;
		while (!IS_NIL(rbt,y->left)) y = y->left;
	}

	x = IS_NIL(rbt,y->left)? y->right: y->left;

	par = y->parent;
	if (!IS_NIL(rbt,x)) x->parent = par;

	if (par)
	{
		if (y == par->left)
			par->left = x;
		else
			par->right = x;
	}
	else
	{
		rbt->root = x;
	}

	if (y == node)
	{
		if (y->color == QSE_RBT_BLACK && !IS_NIL(rbt,x))
			adjust_for_delete (rbt, x, par);

		free_node (rbt, y);
	}
	else
	{
		if (y->color == QSE_RBT_BLACK && !IS_NIL(rbt,x))
			adjust_for_delete (rbt, x, par);

#if 1
		if (node->parent)
		{
			if (node->parent->left == node) node->parent->left = y;
			if (node->parent->right == node) node->parent->right = y;
		}
		else
		{
			rbt->root = y;
		}

		y->parent = node->parent;
		y->left = node->left;
		y->right = node->right;
		y->color = node->color;

		if (node->left->parent == node) node->left->parent = y;
		if (node->right->parent == node) node->right->parent = y;
#else
		*y = *node;
		if (y->parent)
		{
			if (y->parent->left == node) y->parent->left = y;
			if (y->parent->right == node) y->parent->right = y;
		}
		else
		{
			rbt->root = y;
		}

		if (y->left->parent == node) y->left->parent = y;
		if (y->right->parent == node) y->right->parent = y;
#endif

		free_node (rbt, node);
	}

	/* TODO: update tally */	
}

int qse_rbt_delete (qse_rbt_t* rbt, int key)
{
	qse_rbt_node_t* node;

	node = qse_rbt_search (rbt, key);
	if (node == QSE_NULL) return -1;

	delete_node (rbt, node);
	return 0;
}

void qse_rbt_clear (qse_rbt_t* rbt)
{
	/* TODO: improve this */
	while (!IS_NIL(rbt,rbt->root)) delete_node (rbt, rbt->root);
}

static qse_rbt_walk_t walk (
	qse_rbt_t* rbt, qse_rbt_walker_t walker,
	void* ctx, qse_rbt_node_t* node)
{
	if (!IS_NIL(rbt,node->left))
	{
		if (walk (rbt, walker, ctx, node->left) == QSE_RBT_WALK_STOP)
			return QSE_RBT_WALK_STOP;
	}

	if (walker (rbt, node, ctx) == QSE_RBT_WALK_STOP) 
		return QSE_RBT_WALK_STOP;

	if (!IS_NIL(rbt,node->right))
	{
		if (walk (rbt, walker, ctx, node->right) == QSE_RBT_WALK_STOP)
			return QSE_RBT_WALK_STOP;
	}

	return QSE_RBT_WALK_FORWARD;
}

void qse_rbt_walk (qse_rbt_t* rbt, qse_rbt_walker_t walker, void* ctx)
{
	walk (rbt, walker, ctx, rbt->root);
}
