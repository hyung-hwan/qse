/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#include <qse/cmn/arr.h>
#include "mem-prv.h"

#define arr_t    qse_arr_t
#define slot_t   qse_arr_slot_t
#define copier_t qse_arr_copier_t
#define freeer_t qse_arr_freeer_t
#define comper_t qse_arr_comper_t
#define sizer_t  qse_arr_sizer_t
#define keeper_t qse_arr_keeper_t
#define walker_t qse_arr_walker_t

#define mmgr_t   qse_mmgr_t
#define size_t   qse_size_t

#define TOB(arr,len) ((len)*(arr)->scale)
#define DPTR(slot)   ((slot)->val.ptr)
#define DLEN(slot)   ((slot)->val.len)

/* the default comparator is not proper for number comparision.
 * the first different byte decides whice side is greater */
static int default_comparator (arr_t* arr, 
	const void* dptr1, size_t dlen1, 
	const void* dptr2, size_t dlen2)
{
	/*
	if (dlen1 == dlen2) return QSE_MEMCMP (dptr1, dptr2, TOB(arr,dlen1));
	return 1;
	*/

	int n;
	size_t min;

	min = (dlen1 < dlen2)? dlen1: dlen2;
	n = QSE_MEMCMP (dptr1, dptr2, TOB(arr,min));
	if (n == 0 && dlen1 != dlen2) 
	{
		n = (dlen1 > dlen2)? 1: -1;
	}

	return n;
}

static QSE_INLINE slot_t* alloc_slot (arr_t* arr, void* dptr, size_t dlen)
{
	slot_t* n;

	if (arr->copier == QSE_ARR_COPIER_SIMPLE)
	{
		n = QSE_MMGR_ALLOC (arr->mmgr, QSE_SIZEOF(slot_t));
		if (n == QSE_NULL) return QSE_NULL;
		DPTR(n) = dptr;
	}
	else if (arr->copier == QSE_ARR_COPIER_INLINE)
	{
		n = QSE_MMGR_ALLOC (arr->mmgr, 
			QSE_SIZEOF(slot_t) + TOB(arr,dlen));
		if (n == QSE_NULL) return QSE_NULL;

		QSE_MEMCPY (n + 1, dptr, TOB(arr,dlen));
		DPTR(n) = n + 1;
	}
	else
	{
		n = QSE_MMGR_ALLOC (arr->mmgr, QSE_SIZEOF(slot_t));
		if (n == QSE_NULL) return QSE_NULL;
		DPTR(n) = arr->copier (arr, dptr, dlen);
		if (DPTR(n) == QSE_NULL) 
		{
			QSE_MMGR_FREE (arr->mmgr, n);
			return QSE_NULL;
		}
	}

	DLEN(n) = dlen; 

	return n;
}

arr_t* qse_arr_open (mmgr_t* mmgr, size_t xtnsize, size_t capa)
{
	arr_t* arr;

	arr = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(arr_t) + xtnsize);
	if (arr == QSE_NULL) return QSE_NULL;

	if (qse_arr_init (arr, mmgr, capa) <= -1)
	{
		QSE_MMGR_FREE (mmgr, arr);
		return QSE_NULL;
	}

	QSE_MEMSET (QSE_XTN(arr), 0, xtnsize);
	return arr;
}

void qse_arr_close (arr_t* arr)
{
	qse_arr_fini (arr);
	QSE_MMGR_FREE (arr->mmgr, arr);
}

int qse_arr_init (arr_t* arr, mmgr_t* mmgr, size_t capa)
{
	QSE_MEMSET (arr, 0, QSE_SIZEOF(*arr));

	arr->mmgr = mmgr;
	arr->size = 0;
	arr->capa = 0;
	arr->slot = QSE_NULL;
	arr->scale = 1;
	arr->heap_pos_offset = QSE_ARR_NIL;

	arr->copier = QSE_ARR_COPIER_SIMPLE;
	arr->comper = default_comparator;

	return (qse_arr_setcapa (arr, capa) == QSE_NULL)? -1: 0;
}

void qse_arr_fini (arr_t* arr)
{
	qse_arr_clear (arr);

	if (arr->slot != QSE_NULL) 
	{
		QSE_MMGR_FREE (arr->mmgr, arr->slot);
		arr->slot = QSE_NULL;
		arr->capa = 0;
	}
}

qse_mmgr_t* qse_arr_getmmgr (qse_arr_t* arr)
{
	return arr->mmgr;
}

void* qse_arr_getxtn (qse_arr_t* arr)
{
	return QSE_XTN (arr);
}

int qse_arr_getscale (arr_t* arr)
{
	return arr->scale;
}

void qse_arr_setscale (arr_t* arr, int scale)
{
	QSE_ASSERTX (scale > 0 && scale <= QSE_TYPE_MAX(qse_byte_t), 
		"The scale should be larger than 0 and less than or equal to the maximum value that the qse_byte_t type can hold");

	if (scale <= 0) scale = 1;
	if (scale > QSE_TYPE_MAX(qse_byte_t)) scale = QSE_TYPE_MAX(qse_byte_t);

	arr->scale = scale;
}

copier_t qse_arr_getcopier (arr_t* arr)
{
	return arr->copier;
}

void qse_arr_setcopier (arr_t* arr, copier_t copier)
{
	if (copier == QSE_NULL) copier = QSE_ARR_COPIER_SIMPLE;
	arr->copier = copier;
}

freeer_t qse_arr_getfreeer (arr_t* arr)
{
	return arr->freeer;
}

void qse_arr_setfreeer (arr_t* arr, freeer_t freeer)
{
	arr->freeer = freeer;
}

comper_t qse_arr_getcomper (arr_t* arr)
{
	return arr->comper;
}

void qse_arr_setcomper (arr_t* arr, comper_t comper)
{
	if (comper == QSE_NULL) comper = default_comparator;
	arr->comper = comper;
}

keeper_t qse_arr_getkeeper (arr_t* arr)
{
	return arr->keeper;
}

void qse_arr_setkeeper (arr_t* arr, keeper_t keeper)
{
	arr->keeper = keeper;
}

sizer_t qse_arr_getsizer (arr_t* arr)
{
	return arr->sizer;
}

void qse_arr_setsizer (arr_t* arr, sizer_t sizer)
{
	arr->sizer = sizer;
}

size_t qse_arr_getsize (arr_t* arr)
{
	return arr->size;
}

size_t qse_arr_getcapa (arr_t* arr)
{
	return arr->capa;
}

arr_t* qse_arr_setcapa (arr_t* arr, size_t capa)
{
	void* tmp;

	if (capa == arr->capa) return arr;

	if (arr->size > capa) 
	{
		/* to trigger freeers on the items truncated */
		qse_arr_delete (arr, capa, arr->size - capa);
		QSE_ASSERT (arr->size <= capa);
	}

	if (capa > 0) 
	{
		if (arr->mmgr->realloc != QSE_NULL && arr->slot != QSE_NULL)
		{
			tmp = (slot_t**) QSE_MMGR_REALLOC (
				arr->mmgr, arr->slot,
				QSE_SIZEOF(*arr->slot)*capa);
			if (tmp == QSE_NULL) return QSE_NULL;
		}
		else
		{
			tmp = (slot_t**) QSE_MMGR_ALLOC (
				arr->mmgr, QSE_SIZEOF(*arr->slot)*capa);
			if (tmp == QSE_NULL) return QSE_NULL;

			if (arr->slot != QSE_NULL)
			{
				size_t x;
				x = (capa > arr->capa)? arr->capa: capa;
				QSE_MEMCPY (tmp, arr->slot, 
					QSE_SIZEOF(*arr->slot)*x);
				QSE_MMGR_FREE (arr->mmgr, arr->slot);
			}
		}
	}
	else 
	{
		if (arr->slot != QSE_NULL) 
		{
			qse_arr_clear (arr);
			QSE_MMGR_FREE (arr->mmgr, arr->slot);
		}

		tmp = QSE_NULL;
	}

	arr->slot = tmp;
	arr->capa = capa;
	
	return arr;
}

size_t qse_arr_search (arr_t* arr, size_t pos, const void* dptr, size_t dlen)
{
	size_t i;

	for (i = pos; i < arr->size; i++) 
	{
		if (arr->slot[i] == QSE_NULL) continue;

		if (arr->comper (arr, 
			DPTR(arr->slot[i]), DLEN(arr->slot[i]),
			dptr, dlen) == 0) return i;
	}

	return QSE_ARR_NIL;
}

size_t qse_arr_rsearch (arr_t* arr, size_t pos, const void* dptr, size_t dlen)
{
	size_t i;

	if (arr->size > 0)
	{
		if (pos >= arr->size) pos = arr->size - 1;

		for (i = pos + 1; i-- > 0; ) 
		{
			if (arr->slot[i] == QSE_NULL) continue;

			if (arr->comper (arr, 
				DPTR(arr->slot[i]), DLEN(arr->slot[i]),
				dptr, dlen) == 0) return i;
		}
	}

	return QSE_ARR_NIL;
}

size_t qse_arr_upsert (arr_t* arr, size_t pos, void* dptr, size_t dlen)
{
	if (pos < arr->size) return qse_arr_update (arr, pos, dptr, dlen);
	return qse_arr_insert (arr, pos, dptr, dlen);
}

size_t qse_arr_insert (arr_t* arr, size_t pos, void* dptr, size_t dlen)
{
	size_t i;
	slot_t* slot;

	/* allocate the slot first */
	slot = alloc_slot (arr, dptr, dlen);
	if (slot == QSE_NULL) return QSE_ARR_NIL;

	/* do resizeing if necessary. 
	 * resizing is performed after slot allocation because that way, it 
	 * doesn't modify arr on any errors */
	if (pos >= arr->capa || arr->size >= arr->capa) 
	{
		size_t capa, mincapa;

		/* get the minimum capacity needed */
		mincapa = (pos >= arr->size)? (pos + 1): (arr->size + 1);

		if (arr->sizer)
		{
			capa = arr->sizer (arr, mincapa);
		}
		else
		{
			if (arr->capa <= 0) 
			{
				QSE_ASSERT (arr->size <= 0);
				capa = (pos < 16)? 16: (pos + 1);
			}
			else 
			{
				size_t bound = (pos >= arr->size)? pos: arr->size;
				do { capa = arr->capa * 2; } while (capa <= bound);
			}
		}
		
		do
		{
			if (qse_arr_setcapa(arr,capa) != QSE_NULL) break;

			if (capa <= mincapa)
			{
				if (arr->freeer) arr->freeer (arr, DPTR(slot), DLEN(slot));
				QSE_MMGR_FREE (arr->mmgr, slot);
				return QSE_ARR_NIL;
			}

			capa--; /* let it retry after lowering the capacity */
		} 
		while (1);
	}

	if (pos >= arr->capa || arr->size >= arr->capa) 
	{
		/* the buffer is not still enough after resizing */
		if (arr->freeer) arr->freeer (arr, DPTR(slot), DLEN(slot));
		QSE_MMGR_FREE (arr->mmgr, slot);
		return QSE_ARR_NIL;
	}

	/* fill in the gap with QSE_NULL */
	for (i = arr->size; i < pos; i++) arr->slot[i] = QSE_NULL;

	/* shift values to the next cell */
	for (i = arr->size; i > pos; i--) arr->slot[i] = arr->slot[i-1];

	/*  set the value */
	arr->slot[pos] = slot;

	if (pos > arr->size) arr->size = pos + 1;
	else arr->size++;

	return pos;
}

size_t qse_arr_update (arr_t* arr, size_t pos, void* dptr, size_t dlen)
{
	slot_t* c;

	if (pos >= arr->size) return QSE_ARR_NIL;

	c = arr->slot[pos];
	if (c == QSE_NULL)
	{
		/* no previous data */
		arr->slot[pos] = alloc_slot (arr, dptr, dlen);
		if (arr->slot[pos] == QSE_NULL) return QSE_ARR_NIL;
	}
	else
	{
		if (dptr == DPTR(c) && dlen == DLEN(c))
		{
			/* updated to the same data */
			if (arr->keeper) arr->keeper (arr, dptr, dlen);
		}
		else
		{
			/* updated to different data */
			slot_t* slot = alloc_slot (arr, dptr, dlen);
			if (slot == QSE_NULL) return QSE_ARR_NIL;

			if (arr->freeer) arr->freeer (arr, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (arr->mmgr, c);

			arr->slot[pos] = slot;
		}
	}

	return pos;
}

size_t qse_arr_delete (arr_t* arr, size_t index, size_t count)
{
	size_t i;

	if (index >= arr->size) return 0;
	if (count > arr->size - index) count = arr->size - index;

	i = index;

	for (i = index; i < index + count; i++)
	{
		slot_t* c = arr->slot[i];

		if (c != QSE_NULL)
		{
			if (arr->freeer) arr->freeer (arr, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (arr->mmgr, c);

			arr->slot[i] = QSE_NULL;
		}
	}

	for (i = index + count; i < arr->size; i++)
	{
		arr->slot[i-count] = arr->slot[i];
	}
	arr->slot[arr->size-1] = QSE_NULL;

	arr->size -= count;
	return count;
}

size_t qse_arr_uplete (arr_t* arr, size_t index, size_t count)
{
	size_t i;

	if (index >= arr->size) return 0;
	if (count > arr->size - index) count = arr->size - index;

	i = index;

	for (i = index; i < index + count; i++)
	{
		slot_t* c = arr->slot[i];

		if (c != QSE_NULL)
		{
			if (arr->freeer) arr->freeer (arr, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (arr->mmgr, c);

			arr->slot[i] = QSE_NULL;
		}
	}

	return count;
}

void qse_arr_clear (arr_t* arr)
{
	size_t i;

	for (i = 0; i < arr->size; i++) 
	{
		slot_t* c = arr->slot[i];
		if (c != QSE_NULL)
		{
			if (arr->freeer)
				arr->freeer (arr, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (arr->mmgr, c);
			arr->slot[i] = QSE_NULL;
		}
	}

	arr->size = 0;
}

size_t qse_arr_walk (arr_t* arr, walker_t walker, void* ctx)
{
	qse_arr_walk_t w = QSE_ARR_WALK_FORWARD;
	size_t i = 0, nwalks = 0;

	if (arr->size <= 0) return 0;

	while (1)	
	{
		if (arr->slot[i] != QSE_NULL) 
		{
               w = walker (arr, i, ctx);
			nwalks++;
		}

		if (w == QSE_ARR_WALK_STOP) break;

		if (w == QSE_ARR_WALK_FORWARD) 
		{
			i++;
			if (i >= arr->size) break;
		}
		if (w == QSE_ARR_WALK_BACKWARD) 
		{
			if (i <= 0) break;
			i--;
		}
	}

	return nwalks;
}

size_t qse_arr_rwalk (arr_t* arr, walker_t walker, void* ctx)
{
	qse_arr_walk_t w = QSE_ARR_WALK_BACKWARD;
	size_t i, nwalks = 0;

	if (arr->size <= 0) return 0;
	i = arr->size - 1;

	while (1)	
	{
		if (arr->slot[i] != QSE_NULL) 
		{
                	w = walker (arr, i, ctx);
			nwalks++;
		}

		if (w == QSE_ARR_WALK_STOP) break;

		if (w == QSE_ARR_WALK_FORWARD) 
		{
			i++;
			if (i >= arr->size) break;
		}
		if (w == QSE_ARR_WALK_BACKWARD) 
		{
			if (i <= 0) break;
			i--;
		}
	}

	return nwalks;
}

size_t qse_arr_pushstack (arr_t* arr, void* dptr, size_t dlen)
{
	return qse_arr_insert (arr, arr->size, dptr, dlen);
}

void qse_arr_popstack (arr_t* arr)
{
	QSE_ASSERT (arr->size > 0);
	qse_arr_delete (arr, arr->size - 1, 1);
}

#define HEAP_PARENT(x) (((x)-1) / 2)
#define HEAP_LEFT(x)   ((x)*2 + 1)
#define HEAP_RIGHT(x)  ((x)*2 + 2)

#define HEAP_UPDATE_POS(arr, index) \
	do { \
		if (arr->heap_pos_offset != QSE_ARR_NIL) \
			*(qse_size_t*)((qse_byte_t*)DPTR(arr->slot[index]) + arr->heap_pos_offset) = index; \
	} while(0)

qse_size_t sift_up (arr_t* arr, qse_size_t index)
{
	qse_size_t parent;

	if (index > 0)
	{
		int n;

		parent = HEAP_PARENT(index);
		n = arr->comper (arr,
			DPTR(arr->slot[index]), DLEN(arr->slot[index]),
			DPTR(arr->slot[parent]), DLEN(arr->slot[parent]));
		if (n > 0)
		{
			slot_t* tmp;

			tmp = arr->slot[index];

			while (1)
			{
				arr->slot[index] = arr->slot[parent];
				HEAP_UPDATE_POS (arr, index);

				index = parent;
				parent = HEAP_PARENT(parent);

				if (index <= 0) break;

				n = arr->comper (arr,
					DPTR(tmp), DLEN(tmp),
					DPTR(arr->slot[parent]), DLEN(arr->slot[parent]));
				if (n <= 0) break;
			} 

			arr->slot[index] = tmp;
			HEAP_UPDATE_POS (arr, index);
		}
	}
	return index;
}

size_t sift_down (arr_t* arr, size_t index)
{
	size_t base;

	base = arr->size / 2;

	if (index < base) /* at least 1 child is under the 'index' position */
	{
		slot_t* tmp;

		tmp = arr->slot[index];

		do
		{
			qse_size_t left, right, child;
			int n;

			left= HEAP_LEFT(index);
			right = HEAP_RIGHT(index);

			if (right < arr->size)
			{
				n = arr->comper (arr,
					DPTR(arr->slot[right]), DLEN(arr->slot[right]),
					DPTR(arr->slot[left]), DLEN(arr->slot[left]));
				child = (n > 0)? right: left;
			}
			else
			{
				child = left;
			}

			n = arr->comper (arr,
				DPTR(tmp), DLEN(tmp),
				DPTR(arr->slot[child]), DLEN(arr->slot[child]));
			if (n > 0) break;

			arr->slot[index] = arr->slot[child];
			HEAP_UPDATE_POS (arr, index);
			index = child;
		}
		while (index < base);

		arr->slot[index] = tmp;
		HEAP_UPDATE_POS (arr, index);
	}

	return index;
}

size_t qse_arr_pushheap (arr_t* arr, void* dptr, size_t dlen)
{
	size_t index;

	/* add a value at the back of the array  */
	index = arr->size;
	if (qse_arr_insert (arr, index, dptr, dlen) == QSE_ARR_NIL) return QSE_ARR_NIL;
	HEAP_UPDATE_POS (arr, index);

	QSE_ASSERT (arr->size == index + 1);

	/* move the item upto the top if it's greater than the parent items */
	sift_up (arr, index);
	return arr->size;
}

void qse_arr_popheap (arr_t* arr)
{
	QSE_ASSERT (arr->size > 0);
	qse_arr_deleteheap (arr, 0);
}

void qse_arr_deleteheap (arr_t* arr, qse_size_t index)
{
	slot_t* tmp;

	QSE_ASSERT (arr->size > 0);
	QSE_ASSERT (index < arr->size);

	/* remember the item to destroy */
	tmp = arr->slot[index];

	arr->size = arr->size - 1;
	if (arr->size > 0 && index != arr->size)
	{
		int n;

		/* move the last item to the deleting position */
		arr->slot[index] = arr->slot[arr->size];
		HEAP_UPDATE_POS (arr, index);

		/* move it up if the last item is greater than the item to be deleted,
		 * move it down otherwise. */
		n = arr->comper (arr,
			DPTR(arr->slot[index]), DLEN(arr->slot[index]),
			DPTR(tmp), DLEN(tmp));
		if (n > 0) sift_up (arr, index);
		else if (n < 0) sift_down (arr, index);
	}

	/* destroy the actual item */
	if (arr->freeer) arr->freeer (arr, DPTR(tmp), DLEN(tmp));
	QSE_MMGR_FREE (arr->mmgr, tmp);

	/* empty the last slot */
	arr->slot[arr->size] = QSE_NULL;
}

qse_size_t qse_arr_updateheap (qse_arr_t* arr, qse_size_t index, void* dptr, qse_size_t dlen)
{
	slot_t* tmp;
	int n;

	tmp = arr->slot[index];
	QSE_ASSERT (tmp != QSE_NULL);

	n = arr->comper (arr, dptr, dlen, DPTR(tmp), DLEN(tmp));
	if (n)
	{
		if (qse_arr_update (arr, index, dptr, dlen) == QSE_ARR_NIL) return QSE_ARR_NIL;
		HEAP_UPDATE_POS (arr, index);

		if (n > 0) sift_up (arr, index);
		else sift_down (arr, index);
	}

	return index;
}

qse_size_t qse_arr_getheapposoffset (qse_arr_t* arr)
{
	return arr->heap_pos_offset;
}

void qse_arr_setheapposoffset (qse_arr_t* arr, qse_size_t offset)
{
	arr->heap_pos_offset = offset;
}
