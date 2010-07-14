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

QSE_IMPLEMENT_COMMON_FUNCTIONS (rbt)

#define rbt_t    qse_rbt_t
#define pair_t   qse_rbt_pair_t
#define id_t     qse_rbt_id_t
#define copier_t qse_rbt_copier_t
#define freeer_t qse_rbt_freeer_t
#define comper_t qse_rbt_comper_t
#define keeper_t qse_rbt_keeper_t
#define walker_t qse_rbt_walker_t

#define KPTR(p)  QSE_RBT_KPTR(p)
#define KLEN(p)  QSE_RBT_KLEN(p)
#define VPTR(p)  QSE_RBT_VPTR(p)
#define VLEN(p)  QSE_RBT_VLEN(p)

#define SIZEOF(x) QSE_SIZEOF(x)
#define size_t    qse_size_t
#define byte_t    qse_byte_t
#define mmgr_t    qse_mmgr_t

#define KTOB(rbt,len) ((len)*(rbt)->scale[QSE_RBT_KEY])
#define VTOB(rbt,len) ((len)*(rbt)->scale[QSE_RBT_VAL])

#define UPSERT 1
#define UPDATE 2
#define ENSERT 3
#define INSERT 4

#define IS_NIL(rbt,x) ((x) == &((rbt)->nil))
#define LEFT 0
#define RIGHT 1
#define left child[LEFT]
#define right child[RIGHT]
#define rotate_left(rbt,pivot) rotate(rbt,pivot,1);
#define rotate_right(rbt,pivot) rotate(rbt,pivot,0);

static QSE_INLINE int comp_key (
	qse_rbt_t* rbt,
	const void* kptr1, size_t klen1,
	const void* kptr2, size_t klen2)
{
	size_t min;
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

static pair_t* alloc_pair (rbt_t* rbt, 
	void* kptr, size_t klen, void* vptr, size_t vlen)
{
	pair_t* n;

	copier_t kcop = rbt->copier[QSE_RBT_KEY];
	copier_t vcop = rbt->copier[QSE_RBT_VAL];

	size_t as = SIZEOF(pair_t);
	if (kcop == QSE_RBT_COPIER_INLINE) as += KTOB(rbt,klen);
	if (vcop == QSE_RBT_COPIER_INLINE) as += VTOB(rbt,vlen);

	n = (pair_t*) QSE_MMGR_ALLOC (rbt->mmgr, as);
	if (n == QSE_NULL) return QSE_NULL;

	n->color = QSE_RBT_RED;
	n->parent = QSE_NULL;
	n->child[LEFT] = &rbt->nil;
	n->child[RIGHT] = &rbt->nil;

	KLEN(n) = klen;
	if (kcop == QSE_RBT_COPIER_SIMPLE)
	{
		KPTR(n) = kptr;
	}
	else if (kcop == QSE_RBT_COPIER_INLINE)
	{
		KPTR(n) = n + 1;
		QSE_MEMCPY (KPTR(n), kptr, KTOB(rbt,klen));
	}
	else 
	{
		KPTR(n) = kcop (rbt, kptr, klen);
		if (KPTR(n) == QSE_NULL)
		{
			QSE_MMGR_FREE (rbt->mmgr, n);		
			return QSE_NULL;
		}
	}

	VLEN(n) = vlen;
	if (vcop == QSE_RBT_COPIER_SIMPLE)
	{
		VPTR(n) = vptr;
	}
	else if (vcop == QSE_RBT_COPIER_INLINE)
	{
		VPTR(n) = n + 1;
		if (kcop == QSE_RBT_COPIER_INLINE) 
			VPTR(n) = (byte_t*)VPTR(n) + KTOB(rbt,klen);
		QSE_MEMCPY (VPTR(n), vptr, VTOB(rbt,vlen));
	}
	else 
	{
		VPTR(n) = vcop (rbt, vptr, vlen);
		if (VPTR(n) != QSE_NULL)
		{
			if (rbt->freeer[QSE_RBT_KEY] != QSE_NULL)
				rbt->freeer[QSE_RBT_KEY] (rbt, KPTR(n), KLEN(n));
			QSE_MMGR_FREE (rbt->mmgr, n);		
			return QSE_NULL;
		}
	}

	return n;
}

static void free_pair (rbt_t* rbt, pair_t* pair)
{
	if (rbt->freeer[QSE_RBT_KEY] != QSE_NULL) 
		rbt->freeer[QSE_RBT_KEY] (rbt, KPTR(pair), KLEN(pair));
	if (rbt->freeer[QSE_RBT_VAL] != QSE_NULL)
		rbt->freeer[QSE_RBT_VAL] (rbt, VPTR(pair), VLEN(pair));
	QSE_MMGR_FREE (rbt->mmgr, pair);
}

rbt_t* qse_rbt_open (mmgr_t* mmgr, size_t ext)
{
	rbt_t* rbt;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	rbt = (rbt_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(rbt_t) + ext);
	if (rbt == QSE_NULL) return QSE_NULL;

	if (qse_rbt_init (rbt, mmgr) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, rbt);
		return QSE_NULL;
	}

	return rbt;
}

void qse_rbt_close (rbt_t* rbt)
{
	qse_rbt_fini (rbt);
	QSE_MMGR_FREE (rbt->mmgr, rbt);
}

rbt_t* qse_rbt_init (rbt_t* rbt, mmgr_t* mmgr)
{
	/* do not zero out the extension */
	QSE_MEMSET (rbt, 0, SIZEOF(*rbt));
	rbt->mmgr = mmgr;

	rbt->scale[QSE_RBT_KEY] = 1;
	rbt->scale[QSE_RBT_VAL] = 1;
	rbt->size = 0;

	rbt->comper = comp_key;
	rbt->copier[QSE_RBT_KEY] = QSE_RBT_COPIER_SIMPLE;
	rbt->copier[QSE_RBT_VAL] = QSE_RBT_COPIER_SIMPLE;

	/*
	rbt->freeer[QSE_RBT_KEY] = QSE_NULL;
	rbt->freeer[QSE_RBT_VAL] = QSE_NULL;
	rbt->keeper = QSE_NULL;
	*/
	
	/* self-initializing nil */
	QSE_MEMSET(&rbt->nil, 0, QSE_SIZEOF(rbt->nil));
	rbt->nil.color = QSE_RBT_BLACK;
	rbt->nil.left = &rbt->nil;
	rbt->nil.right = &rbt->nil;

	/* root is set to nil initially */
	rbt->root = &rbt->nil;

	return rbt;
}

void qse_rbt_fini (rbt_t* rbt)
{
	qse_rbt_clear (rbt);
}

int qse_rbt_getscale (rbt_t* rbt, id_t id)
{
	QSE_ASSERTX (id == QSE_RBT_KEY || id == QSE_RBT_VAL,
		"The ID should be either QSE_RBT_KEY or QSE_RBT_VAL");
	return rbt->scale[id];
}

void qse_rbt_setscale (rbt_t* rbt, id_t id, int scale)
{
	QSE_ASSERTX (id == QSE_RBT_KEY || id == QSE_RBT_VAL,
		"The ID should be either QSE_RBT_KEY or QSE_RBT_VAL");

	QSE_ASSERTX (scale > 0 && scale <= QSE_TYPE_MAX(qse_byte_t), 
		"The scale should be larger than 0 and less than or equal to the maximum value that the qse_byte_t type can hold");

	if (scale <= 0) scale = 1;
	if (scale > QSE_TYPE_MAX(qse_byte_t)) scale = QSE_TYPE_MAX(qse_byte_t);

	rbt->scale[id] = scale;
}

copier_t qse_rbt_getcopier (rbt_t* rbt, id_t id)
{
	QSE_ASSERTX (id == QSE_RBT_KEY || id == QSE_RBT_VAL,
		"The ID should be either QSE_RBT_KEY or QSE_RBT_VAL");
	return rbt->copier[id];
}

void qse_rbt_setcopier (rbt_t* rbt, id_t id, copier_t copier)
{
	QSE_ASSERTX (id == QSE_RBT_KEY || id == QSE_RBT_VAL,
		"The ID should be either QSE_RBT_KEY or QSE_RBT_VAL");
	if (copier == QSE_NULL) copier = QSE_RBT_COPIER_SIMPLE;
	rbt->copier[id] = copier;
}

freeer_t qse_rbt_getfreeer (rbt_t* rbt, id_t id)
{
	QSE_ASSERTX (id == QSE_RBT_KEY || id == QSE_RBT_VAL,
		"The ID should be either QSE_RBT_KEY or QSE_RBT_VAL");
	return rbt->freeer[id];
}

void qse_rbt_setfreeer (rbt_t* rbt, id_t id, freeer_t freeer)
{
	QSE_ASSERTX (id == QSE_RBT_KEY || id == QSE_RBT_VAL,
		"The ID should be either QSE_RBT_KEY or QSE_RBT_VAL");
	rbt->freeer[id] = freeer;
}

comper_t qse_rbt_getcomper (rbt_t* rbt)
{
	return rbt->comper;
}

void qse_rbt_setcomper (rbt_t* rbt, comper_t comper)
{
	if (comper == QSE_NULL) comper = comp_key;
	rbt->comper = comper;
}

keeper_t qse_rbt_getkeeper (rbt_t* rbt)
{
	return rbt->keeper;
}

void qse_rbt_setkeeper (rbt_t* rbt, keeper_t keeper)
{
	rbt->keeper = keeper;
}

size_t qse_rbt_getsize (rbt_t* rbt)
{
	return rbt->size;
}

pair_t* qse_rbt_search (rbt_t* rbt, const void* kptr, size_t klen)
{
	pair_t* pair = rbt->root;

	while (!IS_NIL(rbt,pair))
	{
		int n = rbt->comper (rbt, kptr, klen, pair->kptr, pair->klen);
		if (n == 0) return pair;

		if (n > 0) pair = pair->right;
		else /* if (n < 0) */ pair = pair->left;
	}

	return QSE_NULL;
}

static void rotate (rbt_t* rbt, pair_t* pivot, int leftwise)
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

	pair_t* parent, * z, * c;
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

static void adjust (rbt_t* rbt, pair_t* pair)
{
	while (pair != rbt->root)
	{
		pair_t* tmp, * tmp2, * xpar;
		int leftwise;

		xpar = pair->parent;
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
			pair = xpar->parent;
		}
		else
		{
			if (pair == tmp2)
			{
				pair = xpar;
				rotate (rbt, pair, leftwise);
				xpar = pair->parent;
			}

			xpar->color = QSE_RBT_BLACK;
			xpar->parent->color = QSE_RBT_RED;
			rotate (rbt, xpar->parent, !leftwise);
		}
	}
}

static pair_t* change_pair_val (
	rbt_t* rbt, pair_t* pair, void* vptr, size_t vlen)
{
	if (VPTR(pair) == vptr && VLEN(pair) == vlen) 
	{
		/* if the old value and the new value are the same,
		 * it just calls the handler for this condition. 
		 * No value replacement occurs. */
		if (rbt->keeper != QSE_NULL)
		{
			rbt->keeper (rbt, vptr, vlen);
		}
	}
	else
	{
		copier_t vcop = rbt->copier[QSE_RBT_VAL];
		void* ovptr = VPTR(pair);
		size_t ovlen = VLEN(pair);

		/* place the new value according to the copier */
		if (vcop == QSE_RBT_COPIER_SIMPLE)
		{
			VPTR(pair) = vptr;
			VLEN(pair) = vlen;
		}
		else if (vcop == QSE_RBT_COPIER_INLINE)
		{
			if (ovlen == vlen)
			{
				QSE_MEMCPY (VPTR(pair), vptr, VTOB(rbt,vlen));
			}
			else
			{
				/* need to reconstruct the pair */
				pair_t* p = alloc_pair (rbt, 
					KPTR(pair), KLEN(pair),
					vptr, vlen);
				if (p == QSE_NULL) return QSE_NULL;

				p->color = pair->color;
				p->left = pair->left;
				p->right = pair->right;
				p->parent = pair->parent;

				if (pair->parent)
				{
					if (pair->parent->left == pair)
					{
						pair->parent->left = p;
					}
					else 
					{
						QSE_ASSERT (parent->parent->right == pair);
						pair->parent->right = p;
					}
				}
				if (!IS_NIL(rbt,pair->left)) pair->left->parent = p;
				if (!IS_NIL(rbt,pair->right)) pair->right->parent = p;

				free_pair (rbt, pair);
				return p;
			}
		}
		else 
		{
			void* nvptr = vcop (rbt, vptr, vlen);
			if (nvptr == QSE_NULL) return QSE_NULL;
			VPTR(pair) = nvptr;
			VLEN(pair) = vlen;
		}

		/* free up the old value */
		if (rbt->freeer[QSE_RBT_VAL] != QSE_NULL) 
		{
			rbt->freeer[QSE_RBT_VAL] (rbt, ovptr, ovlen);
		}
	}

	return pair;
}

static pair_t* insert (
	rbt_t* rbt, void* kptr, size_t klen, void* vptr, size_t vlen, int opt)
{
	/* TODO: enhance this. ues comper... etc */

	pair_t* xcur = rbt->root;
	pair_t* xpar = QSE_NULL;
	pair_t* xnew; 

	while (!IS_NIL(rbt,xcur))
	{
		int n = rbt->comper (rbt, kptr, klen, xcur->kptr, xcur->klen);
		if (n == 0) 
		{
			/* TODO: handle various cases depending on insert types.
			 * return error. update value. */
			switch (opt)
			{
				case UPSERT:
				case UPDATE:
					return change_pair_val (rbt, xcur, vptr, vlen);

				case ENSERT:
					/* return existing pair */
					return xcur; 

				case INSERT:
					/* return failure */
					return QSE_NULL;
			}
		}

		xpar = xcur;

		if (n > 0) xcur = xcur->right;
		else /* if (n < 0) */ xcur = xcur->left;
	}

	if (opt == UPDATE) return QSE_NULL;

	xnew = alloc_pair (rbt, kptr, klen, vptr, vlen);
	if (xnew == QSE_NULL) return QSE_NULL;

	if (xpar == QSE_NULL)
	{
		/* the tree contains no pair */
		QSE_ASSERT (rbt->root == &rbt->nil);
		rbt->root = xnew;
	}
	else
	{
		/* perform normal binary insert */
		int n = rbt->comper (rbt, kptr, klen, xpar->kptr, xpar->klen);
		if (n > 0)
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

pair_t* qse_rbt_upsert (
	rbt_t* rbt, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, UPSERT);
}

pair_t* qse_rbt_ensert (
	rbt_t* rbt, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, ENSERT);
}

pair_t* qse_rbt_insert (
	rbt_t* rbt, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, INSERT);
}


pair_t* qse_rbt_update (
	rbt_t* rbt, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, UPDATE);
}

static void adjust_for_delete (rbt_t* rbt, pair_t* pair, pair_t* par)
{
	while (pair != rbt->root && pair->color == QSE_RBT_BLACK)
	{
		pair_t* tmp;

		if (pair == par->left)
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
				pair = par;
				par = pair->parent;
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
				pair = rbt->root;
			}
		}
		else
		{
			QSE_ASSERT (pair == par->right);
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
				pair = par;
				par = pair->parent;
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
				pair = rbt->root;
			}
		}
	}

	pair->color = QSE_RBT_BLACK;
}

static void delete_pair (rbt_t* rbt, pair_t* pair)
{
	pair_t* x, * y, * par;

	QSE_ASSERT (pair && !IS_NIL(rbt,pair));

	if (IS_NIL(rbt,pair->left) || IS_NIL(rbt,pair->right))
	{
		y = pair;
	}
	else
	{
		/* find a successor with NIL as a child */
		y = pair->right;
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

	if (y == pair)
	{
		if (y->color == QSE_RBT_BLACK && !IS_NIL(rbt,x))
			adjust_for_delete (rbt, x, par);

		free_pair (rbt, y);
	}
	else
	{
		if (y->color == QSE_RBT_BLACK && !IS_NIL(rbt,x))
			adjust_for_delete (rbt, x, par);

#if 1
		if (pair->parent)
		{
			if (pair->parent->left == pair) pair->parent->left = y;
			if (pair->parent->right == pair) pair->parent->right = y;
		}
		else
		{
			rbt->root = y;
		}

		y->parent = pair->parent;
		y->left = pair->left;
		y->right = pair->right;
		y->color = pair->color;

		if (pair->left->parent == pair) pair->left->parent = y;
		if (pair->right->parent == pair) pair->right->parent = y;
#else
		*y = *pair;
		if (y->parent)
		{
			if (y->parent->left == pair) y->parent->left = y;
			if (y->parent->right == pair) y->parent->right = y;
		}
		else
		{
			rbt->root = y;
		}

		if (y->left->parent == pair) y->left->parent = y;
		if (y->right->parent == pair) y->right->parent = y;
#endif

		free_pair (rbt, pair);
	}

	rbt->size--;
}

int qse_rbt_delete (rbt_t* rbt, const void* kptr, size_t klen)
{
	pair_t* pair;

	pair = qse_rbt_search (rbt, kptr, klen);
	if (pair == QSE_NULL) return -1;

	delete_pair (rbt, pair);
	return 0;
}

void qse_rbt_clear (rbt_t* rbt)
{
	/* TODO: improve this */
	while (!IS_NIL(rbt,rbt->root)) delete_pair (rbt, rbt->root);
}

static QSE_INLINE void walk (rbt_t* rbt, walker_t walker, void* ctx, int l, int r)
{
	pair_t* xcur = rbt->root;
	pair_t* prev = rbt->root->parent;

	while (xcur && !IS_NIL(rbt,xcur))
	{
		if (prev == xcur->parent)
		{
			/* the previous node is the parent of the current node.
			 * it indicates that we're going down to the child[l] */
			if (!IS_NIL(rbt,xcur->child[l]))
			{
				/* go to the child[l] child */
				prev = xcur;
				xcur = xcur->child[l];
			}
			else
			{
				if (walker (rbt, xcur, ctx) == QSE_RBT_WALK_STOP) break;

				if (!IS_NIL(rbt,xcur->child[r]))
				{
					/* go down to the right node if exists */
					prev = xcur;
					xcur = xcur->child[r];
				}
				else
				{
					/* otherwise, move up to the parent */
					prev = xcur;
					xcur = xcur->parent;	
				}
			}
		}
		else if (prev == xcur->child[l])
		{
			/* the left child has been already traversed */

			if (walker (rbt, xcur, ctx) == QSE_RBT_WALK_STOP) break;

			if (!IS_NIL(rbt,xcur->child[r]))
			{
				/* go down to the right node if it exists */ 
				prev = xcur;
				xcur = xcur->child[r];	
			}
			else
			{
				/* otherwise, move up to the parent */
				prev = xcur;
				xcur = xcur->parent;	
			}
		}
		else
		{
			/* both the left child and the right child have beem traversed */
			QSE_ASSERT (prev == xcur->child[r]);
			/* just move up to the parent */
			prev = xcur;
			xcur = xcur->parent;
		}
	}
}

void qse_rbt_walk (rbt_t* rbt, walker_t walker, void* ctx)
{
	walk (rbt, walker, ctx, LEFT, RIGHT);
}

void qse_rbt_rwalk (rbt_t* rbt, walker_t walker, void* ctx)
{
	walk (rbt, walker, ctx, RIGHT, LEFT);
}
