/*
 * $Id: lda.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include <qse/cmn/lda.h>
#include "mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (lda)

#define lda_t    qse_lda_t
#define slot_t   qse_lda_slot_t
#define copier_t qse_lda_copier_t
#define freeer_t qse_lda_freeer_t
#define comper_t qse_lda_comper_t
#define sizer_t  qse_lda_sizer_t
#define keeper_t qse_lda_keeper_t
#define walker_t qse_lda_walker_t

#define mmgr_t   qse_mmgr_t
#define size_t   qse_size_t

#define TOB(lda,len) ((len)*(lda)->scale)
#define DPTR(slot)   ((slot)->val.ptr)
#define DLEN(slot)   ((slot)->val.len)

static int default_comparator (lda_t* lda, 
	const void* dptr1, size_t dlen1, 
	const void* dptr2, size_t dlen2)
{
	/*
	if (dlen1 == dlen2) return QSE_MEMCMP (dptr1, dptr2, TOB(lda,dlen1));
	return 1;
	*/

	size_t min = (dlen1 < dlen2)? dlen1: dlen2;
	int n = QSE_MEMCMP (dptr1, dptr2, TOB(lda,min));
	if (n == 0 && dlen1 != dlen2) 
	{
		n = (dlen1 > dlen2)? 1: -1;
	}

	return n;
}

static QSE_INLINE slot_t* alloc_slot (lda_t* lda, void* dptr, size_t dlen)
{
	slot_t* n;

	if (lda->copier == QSE_LDA_COPIER_SIMPLE)
	{
		n = QSE_MMGR_ALLOC (lda->mmgr, QSE_SIZEOF(slot_t));
		if (n == QSE_NULL) return QSE_NULL;
		DPTR(n) = dptr;
	}
	else if (lda->copier == QSE_LDA_COPIER_INLINE)
	{
		n = QSE_MMGR_ALLOC (lda->mmgr, 
			QSE_SIZEOF(slot_t) + TOB(lda,dlen));
		if (n == QSE_NULL) return QSE_NULL;

		QSE_MEMCPY (n + 1, dptr, TOB(lda,dlen));
		DPTR(n) = n + 1;
	}
	else
	{
		n = QSE_MMGR_ALLOC (lda->mmgr, QSE_SIZEOF(slot_t));
		if (n == QSE_NULL) return QSE_NULL;
		DPTR(n) = lda->copier (lda, dptr, dlen);
		if (DPTR(n) == QSE_NULL) 
		{
			QSE_MMGR_FREE (lda->mmgr, n);
			return QSE_NULL;
		}
	}

	DLEN(n) = dlen; 

	return n;
}

lda_t* qse_lda_open (mmgr_t* mmgr, size_t ext, size_t capa)
{
	lda_t* lda;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	lda = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(lda_t) + ext);
	if (lda == QSE_NULL) return QSE_NULL;

	if (qse_lda_init (lda, mmgr, capa) <= -1)
	{
		QSE_MMGR_FREE (mmgr, lda);
		return QSE_NULL;
	}

	return lda;
}

void qse_lda_close (lda_t* lda)
{
	qse_lda_fini (lda);
	QSE_MMGR_FREE (lda->mmgr, lda);
}

int qse_lda_init (lda_t* lda, mmgr_t* mmgr, size_t capa)
{
	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (lda, 0, QSE_SIZEOF(*lda));

	lda->mmgr = mmgr;
	lda->size = 0;
	lda->capa = 0;
	lda->slot = QSE_NULL;

	lda->copier = QSE_LDA_COPIER_SIMPLE;
	lda->comper = default_comparator;

	return (qse_lda_setcapa (lda, capa) == QSE_NULL)? -1: 0;
}

void qse_lda_fini (lda_t* lda)
{
	qse_lda_clear (lda);

	if (lda->slot != QSE_NULL) 
	{
		QSE_MMGR_FREE (lda->mmgr, lda->slot);
		lda->slot = QSE_NULL;
		lda->capa = 0;
	}
}

int qse_lda_getscale (lda_t* lda)
{
	return lda->scale;
}

void qse_lda_setscale (lda_t* lda, int scale)
{
	QSE_ASSERTX (scale > 0 && scale <= QSE_TYPE_MAX(qse_byte_t), 
		"The scale should be larger than 0 and less than or "
		"equal to the maximum value that the qse_byte_t type can hold");

	if (scale <= 0) scale = 1;
	if (scale > QSE_TYPE_MAX(qse_byte_t)) scale = QSE_TYPE_MAX(qse_byte_t);

	lda->scale = scale;
}

copier_t qse_lda_getcopier (lda_t* lda)
{
	return lda->copier;
}

void qse_lda_setcopier (lda_t* lda, copier_t copier)
{
	if (copier == QSE_NULL) copier = QSE_LDA_COPIER_SIMPLE;
	lda->copier = copier;
}

freeer_t qse_lda_getfreeer (lda_t* lda)
{
	return lda->freeer;
}

void qse_lda_setfreeer (lda_t* lda, freeer_t freeer)
{
	lda->freeer = freeer;
}

comper_t qse_lda_getcomper (lda_t* lda)
{
	return lda->comper;
}

void qse_lda_setcomper (lda_t* lda, comper_t comper)
{
	if (comper == QSE_NULL) comper = default_comparator;
	lda->comper = comper;
}

keeper_t qse_lda_getkeeper (lda_t* lda)
{
	return lda->keeper;
}

void qse_lda_setkeeper (lda_t* lda, keeper_t keeper)
{
	lda->keeper = keeper;
}

sizer_t qse_lda_getsizer (lda_t* lda)
{
	return lda->sizer;
}

void qse_lda_setsizer (lda_t* lda, sizer_t sizer)
{
	lda->sizer = sizer;
}

size_t qse_lda_getsize (lda_t* lda)
{
	return lda->size;
}

size_t qse_lda_getcapa (lda_t* lda)
{
	return lda->capa;
}

lda_t* qse_lda_setcapa (lda_t* lda, size_t capa)
{
	void* tmp;

	if (capa == lda->capa) return lda;

	if (lda->size > capa) 
	{
		/* to trigger freeers on the items truncated */
		qse_lda_delete (lda, capa, lda->size - capa);
		QSE_ASSERT (lda->size <= capa);
	}

	if (capa > 0) 
	{
		if (lda->mmgr->realloc != QSE_NULL && lda->slot != QSE_NULL)
		{
			tmp = (slot_t**) QSE_MMGR_REALLOC (
				lda->mmgr, lda->slot,
				QSE_SIZEOF(*lda->slot)*capa);
			if (tmp == QSE_NULL) return QSE_NULL;
		}
		else
		{
			tmp = (slot_t**) QSE_MMGR_ALLOC (
				lda->mmgr, QSE_SIZEOF(*lda->slot)*capa);
			if (tmp == QSE_NULL) return QSE_NULL;

			if (lda->slot != QSE_NULL)
			{
				size_t x;
				x = (capa > lda->capa)? lda->capa: capa;
				QSE_MEMCPY (tmp, lda->slot, 
					QSE_SIZEOF(*lda->slot)*x);
				QSE_MMGR_FREE (lda->mmgr, lda->slot);
			}
		}
	}
	else 
	{
		if (lda->slot != QSE_NULL) 
		{
			qse_lda_clear (lda);
			QSE_MMGR_FREE (lda->mmgr, lda->slot);
		}

		tmp = QSE_NULL;
	}

	lda->slot = tmp;
	lda->capa = capa;
	
	return lda;
}

size_t qse_lda_search (lda_t* lda, size_t pos, const void* dptr, size_t dlen)
{
	size_t i;

	for (i = pos; i < lda->size; i++) 
	{
		if (lda->slot[i] == QSE_NULL) continue;

		if (lda->comper (lda, 
			DPTR(lda->slot[i]), DLEN(lda->slot[i]),
			dptr, dlen) == 0) return i;
	}

	return QSE_LDA_NIL;
}

size_t qse_lda_rsearch (lda_t* lda, size_t pos, const void* dptr, size_t dlen)
{
	size_t i;

	if (lda->size > 0)
	{
		if (pos >= lda->size) pos = lda->size - 1;

		for (i = pos + 1; i-- > 0; ) 
		{
			if (lda->slot[i] == QSE_NULL) continue;

			if (lda->comper (lda, 
				DPTR(lda->slot[i]), DLEN(lda->slot[i]),
				dptr, dlen) == 0) return i;
		}
	}

	return QSE_LDA_NIL;
}

size_t qse_lda_upsert (lda_t* lda, size_t pos, void* dptr, size_t dlen)
{
	if (pos < lda->size) return qse_lda_update (lda, pos, dptr, dlen);
	return qse_lda_insert (lda, pos, dptr, dlen);
}

size_t qse_lda_insert (lda_t* lda, size_t pos, void* dptr, size_t dlen)
{
	size_t i;
	slot_t* slot;

	/* allocate the slot first */
	slot = alloc_slot (lda, dptr, dlen);
	if (slot == QSE_NULL) return QSE_LDA_NIL;

	/* do resizeing if necessary. 
	 * resizing is performed after slot allocation because that way, it 
	 * doesn't modify lda on any errors */
	if (pos >= lda->capa || lda->size >= lda->capa) 
	{
		size_t capa, mincapa;

		/* get the minimum capacity needed */
		mincapa = (pos >= lda->size)? (pos + 1): (lda->size + 1);

		if (lda->sizer)
		{
			capa = lda->sizer (lda, mincapa);
		}
		else
		{
			if (lda->capa <= 0) 
			{
				QSE_ASSERT (lda->size <= 0);
				capa = (pos < 16)? 16: (pos + 1);
			}
			else 
			{
				size_t bound = (pos >= lda->size)? pos: lda->size;
				do { capa = lda->capa * 2; } while (capa <= bound);
			}
		}
		
		do
		{
			if (qse_lda_setcapa(lda,capa) != QSE_NULL) break;

			if (capa <= mincapa)
			{
				if (lda->freeer) 
					lda->freeer (lda, DPTR(slot), DLEN(slot));
				QSE_MMGR_FREE (lda->mmgr, slot);
				return QSE_LDA_NIL;
			}

			capa--; /* let it retry after lowering the capacity */
		} 
		while (1);
	}

	if (pos >= lda->capa || lda->size >= lda->capa) 
	{
		/* the buffer is not still enough after resizing */
		if (lda->freeer) 
			lda->freeer (lda, DPTR(slot), DLEN(slot));
		QSE_MMGR_FREE (lda->mmgr, slot);
		return QSE_LDA_NIL;
	}

	/* fill in the gap with QSE_NULL */
	for (i = lda->size; i < pos; i++) lda->slot[i] = QSE_NULL;

	/* shift values to the next cell */
	for (i = lda->size; i > pos; i--) lda->slot[i] = lda->slot[i-1];

	/*  set the value */
	lda->slot[pos] = slot;

	if (pos > lda->size) lda->size = pos + 1;
	else lda->size++;

	return pos;
}

size_t qse_lda_update (lda_t* lda, size_t pos, void* dptr, size_t dlen)
{
	slot_t* c;

	if (pos >= lda->size) return QSE_LDA_NIL;

	c = lda->slot[pos];
	if (c == QSE_NULL)
	{
		/* no previous data */
		lda->slot[pos] = alloc_slot (lda, dptr, dlen);
		if (lda->slot[pos] == QSE_NULL) return QSE_LDA_NIL;
	}
	else
	{
		if (dptr == DPTR(c) && dlen == DLEN(c))
		{
			/* updated to the same data */
			if (lda->keeper) lda->keeper (lda, dptr, dlen);	
		}
		else
		{
			/* updated to different data */
			slot_t* slot = alloc_slot (lda, dptr, dlen);
			if (slot == QSE_NULL) return QSE_LDA_NIL;

			if (lda->freeer != QSE_NULL)
				lda->freeer (lda, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (lda->mmgr, c);

			lda->slot[pos] = slot;
		}
	}

	return pos;
}

size_t qse_lda_delete (lda_t* lda, size_t index, size_t count)
{
	size_t i;

	if (index >= lda->size) return 0;
	if (count > lda->size - index) count = lda->size - index;

	i = index;

	for (i = index; i < index + count; i++)
	{
		slot_t* c = lda->slot[i];

		if (c != QSE_NULL)
		{
			if (lda->freeer != QSE_NULL)
				lda->freeer (lda, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (lda->mmgr, c);

			lda->slot[i] = QSE_NULL;
		}
	}

	for (i = index + count; i < lda->size; i++)
	{
		lda->slot[i-count] = lda->slot[i];
	}
	lda->slot[lda->size-1] = QSE_NULL;

	lda->size -= count;
	return count;
}

size_t qse_lda_uplete (lda_t* lda, size_t index, size_t count)
{
	size_t i;

	if (index >= lda->size) return 0;
	if (count > lda->size - index) count = lda->size - index;

	i = index;

	for (i = index; i < index + count; i++)
	{
		slot_t* c = lda->slot[i];

		if (c != QSE_NULL)
		{
			if (lda->freeer != QSE_NULL)
				lda->freeer (lda, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (lda->mmgr, c);

			lda->slot[i] = QSE_NULL;
		}
	}

	return count;
}

void qse_lda_clear (lda_t* lda)
{
	size_t i;

	for (i = 0; i < lda->size; i++) 
	{
		slot_t* c = lda->slot[i];
		if (c != QSE_NULL)
		{
			if (lda->freeer)
				lda->freeer (lda, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (lda->mmgr, c);
			lda->slot[i] = QSE_NULL;
		}
	}

	lda->size = 0;
}

size_t qse_lda_walk (lda_t* lda, walker_t walker, void* ctx)
{
	qse_lda_walk_t w = QSE_LDA_WALK_FORWARD;
	size_t i = 0, nwalks = 0;

	if (lda->size <= 0) return 0;

	while (1)	
	{
		if (lda->slot[i] != QSE_NULL) 
		{
               w = walker (lda, i, ctx);
			nwalks++;
		}

		if (w == QSE_LDA_WALK_STOP) break;

		if (w == QSE_LDA_WALK_FORWARD) 
		{
			i++;
			if (i >= lda->size) break;
		}
		if (w == QSE_LDA_WALK_BACKWARD) 
		{
			if (i <= 0) break;
			i--;
		}
	}

	return nwalks;
}

size_t qse_lda_rwalk (lda_t* lda, walker_t walker, void* ctx)
{
	qse_lda_walk_t w = QSE_LDA_WALK_BACKWARD;
	size_t i, nwalks = 0;

	if (lda->size <= 0) return 0;
	i = lda->size - 1;

	while (1)	
	{
		if (lda->slot[i] != QSE_NULL) 
		{
                	w = walker (lda, i, ctx);
			nwalks++;
		}

		if (w == QSE_LDA_WALK_STOP) break;

		if (w == QSE_LDA_WALK_FORWARD) 
		{
			i++;
			if (i >= lda->size) break;
		}
		if (w == QSE_LDA_WALK_BACKWARD) 
		{
			if (i <= 0) break;
			i--;
		}
	}

	return nwalks;
}

size_t qse_lda_pushstack (lda_t* lda, void* dptr, size_t dlen)
{
	return qse_lda_insert (lda, lda->size, dptr, dlen);
}

void qse_lda_popstack (lda_t* lda)
{
	QSE_ASSERT (lda->size > 0);
	qse_lda_delete (lda, lda->size - 1, 1);
}

#define HEAP_PARENT(x) (((x)-1) / 2)
#define HEAP_LEFT(x)   ((x)*2 + 1)
#define HEAP_RIGHT(x)  ((x)*2 + 2)

size_t qse_lda_pushheap (lda_t* lda, void* dptr, size_t dlen)
{
	size_t cur, par;
	int n;

	/* add a value to the bottom */
	cur = lda->size;
	if (qse_lda_insert (lda, cur, dptr, dlen) == QSE_LDA_NIL)
		return QSE_LDA_NIL;

	while (cur != 0)
	{
		slot_t* tmp;

		/* compare with the parent */
		par = HEAP_PARENT(cur);
		n = lda->comper (lda,
			DPTR(lda->slot[cur]), DLEN(lda->slot[cur]),
			DPTR(lda->slot[par]), DLEN(lda->slot[par]));
		if (n <= 0) break; /* ok */

		/* swap the current with the parent */
		tmp = lda->slot[cur];
		lda->slot[cur] = lda->slot[par];
		lda->slot[par] = tmp;

		cur = par;
	}

	return lda->size;
}

void qse_lda_popheap (lda_t* lda)
{
	size_t cur, child;
	slot_t* tmp;

	QSE_ASSERT (lda->size > 0);

	/* destroy the top */
	tmp = lda->slot[0];
	if (lda->freeer) lda->freeer (lda, DPTR(tmp), DLEN(tmp));
	QSE_MMGR_FREE (lda->mmgr, tmp);

	/* move the last item to the top position also shrink the size */
	lda->slot[0] = lda->slot[--lda->size];

	if (lda->size <= 1) return; /* only 1 element. nothing further to do */

	for (cur = 0; cur < lda->size; cur = child)
	{
		size_t left, right;
		int n;

		left = HEAP_LEFT(cur);
		right = HEAP_RIGHT(cur);

		if (left >= lda->size) 
		{
			/* the left child does not exist. 
			 * reached the bottom. abort exchange */
			break;
		}

		if (right >= lda->size) 
		{
			/* the right child does not exist. only the left */
			child = left;	
		}
		else
		{
			/* get the larger child of the two */
			n = lda->comper (lda,
				DPTR(lda->slot[left]), DLEN(lda->slot[left]),
				DPTR(lda->slot[right]), DLEN(lda->slot[right]));
			child = (n > 0)? left: right;
		}
		
		/* compare the current one with the child */
		n = lda->comper (lda,
			DPTR(lda->slot[cur]), DLEN(lda->slot[cur]),
			DPTR(lda->slot[child]), DLEN(lda->slot[child]));
		if (n > 0) break; /* current one is larger. stop exchange */

		/* swap the current with the child */
		tmp = lda->slot[cur];
		lda->slot[cur] = lda->slot[child];
		lda->slot[child] = tmp;
	}
}
