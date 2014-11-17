/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#define rbt_t           qse_rbt_t
#define pair_t          qse_rbt_pair_t
#define id_t            qse_rbt_id_t
#define copier_t        qse_rbt_copier_t
#define freeer_t        qse_rbt_freeer_t
#define comper_t        qse_rbt_comper_t
#define keeper_t        qse_rbt_keeper_t
#define walker_t        qse_rbt_walker_t
#define cbserter_t      qse_rbt_cbserter_t
#define style_t        qse_rbt_style_t
#define style_kind_t   qse_rbt_style_kind_t

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

#define IS_NIL(rbt,x) ((x) == &((rbt)->xnil))
#define LEFT 0
#define RIGHT 1
#define left child[LEFT]
#define right child[RIGHT]
#define rotate_left(rbt,pivot) rotate(rbt,pivot,1);
#define rotate_right(rbt,pivot) rotate(rbt,pivot,0);

QSE_INLINE pair_t* qse_rbt_allocpair (
	rbt_t* rbt, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	pair_t* n;

	copier_t kcop = rbt->style->copier[QSE_RBT_KEY];
	copier_t vcop = rbt->style->copier[QSE_RBT_VAL];

	size_t as = SIZEOF(pair_t);
	if (kcop == QSE_RBT_COPIER_INLINE) as += KTOB(rbt,klen);
	if (vcop == QSE_RBT_COPIER_INLINE) as += VTOB(rbt,vlen);

	n = (pair_t*) QSE_MMGR_ALLOC (rbt->mmgr, as);
	if (n == QSE_NULL) return QSE_NULL;

	n->color = QSE_RBT_RED;
	n->parent = QSE_NULL;
	n->child[LEFT] = &rbt->xnil;
	n->child[RIGHT] = &rbt->xnil;

	KLEN(n) = klen;
	if (kcop == QSE_RBT_COPIER_SIMPLE)
	{
		KPTR(n) = kptr;
	}
	else if (kcop == QSE_RBT_COPIER_INLINE)
	{
		KPTR(n) = n + 1;
		if (kptr) QSE_MEMCPY (KPTR(n), kptr, KTOB(rbt,klen));
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
		if (vptr) QSE_MEMCPY (VPTR(n), vptr, VTOB(rbt,vlen));
	}
	else 
	{
		VPTR(n) = vcop (rbt, vptr, vlen);
		if (VPTR(n) != QSE_NULL)
		{
			if (rbt->style->freeer[QSE_RBT_KEY] != QSE_NULL)
				rbt->style->freeer[QSE_RBT_KEY] (rbt, KPTR(n), KLEN(n));
			QSE_MMGR_FREE (rbt->mmgr, n);		
			return QSE_NULL;
		}
	}

	return n;
}

QSE_INLINE void qse_rbt_freepair (rbt_t* rbt, pair_t* pair)
{
	if (rbt->style->freeer[QSE_RBT_KEY] != QSE_NULL) 
		rbt->style->freeer[QSE_RBT_KEY] (rbt, KPTR(pair), KLEN(pair));
	if (rbt->style->freeer[QSE_RBT_VAL] != QSE_NULL)
		rbt->style->freeer[QSE_RBT_VAL] (rbt, VPTR(pair), VLEN(pair));
	QSE_MMGR_FREE (rbt->mmgr, pair);
}

static style_t style[] =
{
	{
		{
			QSE_RBT_COPIER_DEFAULT,
			QSE_RBT_COPIER_DEFAULT
		},
		{
			QSE_RBT_FREEER_DEFAULT,
			QSE_RBT_FREEER_DEFAULT
		},
		QSE_RBT_COMPER_DEFAULT,
		QSE_RBT_KEEPER_DEFAULT
	},

	{
		{
			QSE_RBT_COPIER_INLINE,
			QSE_RBT_COPIER_INLINE
		},
		{
			QSE_RBT_FREEER_DEFAULT,
			QSE_RBT_FREEER_DEFAULT
		},
		QSE_RBT_COMPER_DEFAULT,
		QSE_RBT_KEEPER_DEFAULT
	},

	{
		{
			QSE_RBT_COPIER_INLINE,
			QSE_RBT_COPIER_DEFAULT
		},
		{
			QSE_RBT_FREEER_DEFAULT,
			QSE_RBT_FREEER_DEFAULT
		},
		QSE_RBT_COMPER_DEFAULT,
		QSE_RBT_KEEPER_DEFAULT
	},

	{
		{
			QSE_RBT_COPIER_DEFAULT,
			QSE_RBT_COPIER_INLINE
		},
		{
			QSE_RBT_FREEER_DEFAULT,
			QSE_RBT_FREEER_DEFAULT
		},
		QSE_RBT_COMPER_DEFAULT,
		QSE_RBT_KEEPER_DEFAULT
	}
};

const style_t* qse_getrbtstyle (style_kind_t kind)
{
	return &style[kind];
}

rbt_t* qse_rbt_open (mmgr_t* mmgr, size_t xtnsize, int kscale, int vscale)
{
	rbt_t* rbt;

	rbt = (rbt_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(rbt_t) + xtnsize);
	if (rbt == QSE_NULL) return QSE_NULL;

	if (qse_rbt_init (rbt, mmgr, kscale, vscale) <= -1)
	{
		QSE_MMGR_FREE (mmgr, rbt);
		return QSE_NULL;
	}

	QSE_MEMSET (rbt + 1, 0, xtnsize);
	return rbt;
}

void qse_rbt_close (rbt_t* rbt)
{
	qse_rbt_fini (rbt);
	QSE_MMGR_FREE (rbt->mmgr, rbt);
}

int qse_rbt_init (rbt_t* rbt, mmgr_t* mmgr, int kscale, int vscale)
{
	/* do not zero out the extension */
	QSE_MEMSET (rbt, 0, SIZEOF(*rbt));
	rbt->mmgr = mmgr;

	rbt->scale[QSE_RBT_KEY] = (kscale < 1)? 1: kscale;
	rbt->scale[QSE_RBT_VAL] = (vscale < 1)? 1: vscale;
	rbt->size = 0;

	rbt->style = &style[0];
	
	/* self-initializing nil */
	QSE_MEMSET(&rbt->xnil, 0, QSE_SIZEOF(rbt->xnil));
	rbt->xnil.color = QSE_RBT_BLACK;
	rbt->xnil.left = &rbt->xnil;
	rbt->xnil.right = &rbt->xnil;

	/* root is set to nil initially */
	rbt->root = &rbt->xnil;

	return 0;
}

void qse_rbt_fini (rbt_t* rbt)
{
	qse_rbt_clear (rbt);
}

qse_mmgr_t* qse_rbt_getmmgr (qse_rbt_t* rbt)
{
	return rbt->mmgr;
}

void* qse_rbt_getxtn (qse_rbt_t* rbt)
{
	return QSE_XTN (rbt);
}

const style_t* qse_rbt_getstyle (const rbt_t* rbt)
{
	return rbt->style;
}

void qse_rbt_setstyle (rbt_t* rbt, const style_t* style)
{
	QSE_ASSERT (style != QSE_NULL);
	rbt->style = style;
}

size_t qse_rbt_getsize (const rbt_t* rbt)
{
	return rbt->size;
}

pair_t* qse_rbt_search (const rbt_t* rbt, const void* kptr, size_t klen)
{
	pair_t* pair = rbt->root;

	while (!IS_NIL(rbt,pair))
	{
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(pair), KLEN(pair));
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
		pair_t* tmp, * tmp2, * x_par;
		int leftwise;

		x_par = pair->parent;
		if (x_par->color == QSE_RBT_BLACK) break;

		QSE_ASSERT (x_par->parent != QSE_NULL);

		if (x_par == x_par->parent->child[LEFT]) 
		{
			tmp = x_par->parent->child[RIGHT];
			tmp2 = x_par->child[RIGHT];
			leftwise = 1;
		}
		else
		{
			tmp = x_par->parent->child[LEFT];
			tmp2 = x_par->child[LEFT];
			leftwise = 0;
		}

		if (tmp->color == QSE_RBT_RED)
		{
			x_par->color = QSE_RBT_BLACK;
			tmp->color = QSE_RBT_BLACK;
			x_par->parent->color = QSE_RBT_RED;	
			pair = x_par->parent;
		}
		else
		{
			if (pair == tmp2)
			{
				pair = x_par;
				rotate (rbt, pair, leftwise);
				x_par = pair->parent;
			}

			x_par->color = QSE_RBT_BLACK;
			x_par->parent->color = QSE_RBT_RED;
			rotate (rbt, x_par->parent, !leftwise);
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
		if (rbt->style->keeper != QSE_NULL)
		{
			rbt->style->keeper (rbt, vptr, vlen);
		}
	}
	else
	{
		copier_t vcop = rbt->style->copier[QSE_RBT_VAL];
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
				if (vptr) QSE_MEMCPY (VPTR(pair), vptr, VTOB(rbt,vlen));
			}
			else
			{
				/* need to reconstruct the pair */
				pair_t* p = qse_rbt_allocpair (rbt, 
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
						QSE_ASSERT (pair->parent->right == pair);
						pair->parent->right = p;
					}
				}
				if (!IS_NIL(rbt,pair->left)) pair->left->parent = p;
				if (!IS_NIL(rbt,pair->right)) pair->right->parent = p;

				if (pair == rbt->root) rbt->root = p;

				qse_rbt_freepair (rbt, pair);
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
		if (rbt->style->freeer[QSE_RBT_VAL] != QSE_NULL) 
		{
			rbt->style->freeer[QSE_RBT_VAL] (rbt, ovptr, ovlen);
		}
	}

	return pair;
}

static pair_t* insert (
	rbt_t* rbt, void* kptr, size_t klen, void* vptr, size_t vlen, int opt)
{
	pair_t* x_cur = rbt->root;
	pair_t* x_par = QSE_NULL;
	pair_t* x_new; 

	while (!IS_NIL(rbt,x_cur))
	{
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_cur), KLEN(x_cur));
		if (n == 0) 
		{
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
		}

		x_par = x_cur;

		if (n > 0) x_cur = x_cur->right;
		else /* if (n < 0) */ x_cur = x_cur->left;
	}

	if (opt == UPDATE) return QSE_NULL;

	x_new = qse_rbt_allocpair (rbt, kptr, klen, vptr, vlen);
	if (x_new == QSE_NULL) return QSE_NULL;

	if (x_par == QSE_NULL)
	{
		/* the tree contains no pair */
		QSE_ASSERT (rbt->root == &rbt->xnil);
		rbt->root = x_new;
	}
	else
	{
		/* perform normal binary insert */
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_par), KLEN(x_par));
		if (n > 0)
		{
			QSE_ASSERT (x_par->right == &rbt->xnil);
			x_par->right = x_new;
		}
		else
		{
			QSE_ASSERT (x_par->left == &rbt->xnil);
			x_par->left = x_new;
		}

		x_new->parent = x_par;
		adjust (rbt, x_new);
	}

	rbt->root->color = QSE_RBT_BLACK;
	rbt->size++;
	return x_new;
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

pair_t* qse_rbt_cbsert (
	rbt_t* rbt, void* kptr, size_t klen, cbserter_t cbserter, void* ctx)
{
	pair_t* x_cur = rbt->root;
	pair_t* x_par = QSE_NULL;
	pair_t* x_new; 

	while (!IS_NIL(rbt,x_cur))
	{
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_cur), KLEN(x_cur));
		if (n == 0) 
		{
			/* back up the contents of the current pair 
			 * in case it is reallocated */
			pair_t tmp;

			tmp = *x_cur;	 

			/* call the callback function to manipulate the pair */
			x_new = cbserter (rbt, x_cur, kptr, klen, ctx);
			if (x_new == QSE_NULL)
			{
				/* error returned by the callback function */
				return QSE_NULL;
			}

			if (x_new != x_cur)
			{
				/* the current pair has been reallocated, which implicitly
				 * means the previous contents were wiped out. so the contents
				 * backed up will be used for restoration/migration */

				x_new->color = tmp.color;
				x_new->left = tmp.left;
				x_new->right = tmp.right;
				x_new->parent = tmp.parent;

				if (tmp.parent)
				{
					if (tmp.parent->left == x_cur)
					{
						tmp.parent->left = x_new;
					}
					else 
					{
						QSE_ASSERT (tmp.parent->right == x_cur);
						tmp.parent->right = x_new;
					}
				}
				if (!IS_NIL(rbt,tmp.left)) tmp.left->parent = x_new;
				if (!IS_NIL(rbt,tmp.right)) tmp.right->parent = x_new;

				if (x_cur == rbt->root) rbt->root = x_new;
			}

			return x_new;
		}

		x_par = x_cur;

		if (n > 0) x_cur = x_cur->right;
		else /* if (n < 0) */ x_cur = x_cur->left;
	}

	x_new = cbserter (rbt, QSE_NULL, kptr, klen, ctx);
	if (x_new == QSE_NULL) return QSE_NULL;

	if (x_par == QSE_NULL)
	{
		/* the tree contains no pair */
		QSE_ASSERT (rbt->root == &rbt->xnil);
		rbt->root = x_new;
	}
	else
	{
		/* perform normal binary insert */
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_par), KLEN(x_par));
		if (n > 0)
		{
			QSE_ASSERT (x_par->right == &rbt->xnil);
			x_par->right = x_new;
		}
		else
		{
			QSE_ASSERT (x_par->left == &rbt->xnil);
			x_par->left = x_new;
		}

		x_new->parent = x_par;
		adjust (rbt, x_new);
	}

	rbt->root->color = QSE_RBT_BLACK;
	rbt->size++;
	return x_new;
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

		qse_rbt_freepair (rbt, y);
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

		qse_rbt_freepair (rbt, pair);
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

#if 0
static QSE_INLINE qse_rbt_walk_t walk_recursively (
	rbt_t* rbt, walker_t walker, void* ctx, qse_rbt_pair_t* pair)
{
	if (!IS_NIL(rbt,pair->left))
	{
		if (walk_recursively (rbt, walker, ctx, pair->left) == QSE_RBT_WALK_STOP)
			return QSE_RBT_WALK_STOP;
	}

	if (walker (rbt, pair, ctx) == QSE_RBT_WALK_STOP) return QSE_RBT_WALK_STOP;

	if (!IS_NIL(rbt,pair->right))
	{
		if (walk_recursively (rbt, walker, ctx, pair->right) == QSE_RBT_WALK_STOP)
			return QSE_RBT_WALK_STOP;
	}

	return QSE_RBT_WALK_FORWARD;
}
#endif

static QSE_INLINE void walk (
	rbt_t* rbt, walker_t walker, void* ctx, int l, int r)
{
	pair_t* x_cur = rbt->root;
	pair_t* prev = rbt->root->parent;

	while (x_cur && !IS_NIL(rbt,x_cur))
	{
		if (prev == x_cur->parent)
		{
			/* the previous node is the parent of the current node.
			 * it indicates that we're going down to the child[l] */
			if (!IS_NIL(rbt,x_cur->child[l]))
			{
				/* go to the child[l] child */
				prev = x_cur;
				x_cur = x_cur->child[l];
			}
			else
			{
				if (walker (rbt, x_cur, ctx) == QSE_RBT_WALK_STOP) break;

				if (!IS_NIL(rbt,x_cur->child[r]))
				{
					/* go down to the right node if exists */
					prev = x_cur;
					x_cur = x_cur->child[r];
				}
				else
				{
					/* otherwise, move up to the parent */
					prev = x_cur;
					x_cur = x_cur->parent;	
				}
			}
		}
		else if (prev == x_cur->child[l])
		{
			/* the left child has been already traversed */

			if (walker (rbt, x_cur, ctx) == QSE_RBT_WALK_STOP) break;

			if (!IS_NIL(rbt,x_cur->child[r]))
			{
				/* go down to the right node if it exists */ 
				prev = x_cur;
				x_cur = x_cur->child[r];	
			}
			else
			{
				/* otherwise, move up to the parent */
				prev = x_cur;
				x_cur = x_cur->parent;	
			}
		}
		else
		{
			/* both the left child and the right child have been traversed */
			QSE_ASSERT (prev == x_cur->child[r]);
			/* just move up to the parent */
			prev = x_cur;
			x_cur = x_cur->parent;
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

int qse_rbt_dflcomp (
	const qse_rbt_t* rbt,
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

