/*
 * $Id: lda.c 337 2008-08-20 09:17:25Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/cmn/lda.h>
#include "mem.h"

#define lda_t    qse_lda_t
#define node_t   qse_lda_node_t
#define copier_t qse_lda_copier_t
#define freeer_t qse_lda_freeer_t
#define comper_t qse_lda_comper_t
#define sizer_t  qse_lda_sizer_t
#define keeper_t qse_lda_keeper_t
#define walker_t qse_lda_walker_t

#define mmgr_t   qse_mmgr_t
#define size_t   qse_size_t

#define TOB(lda,len) ((len)*(lda)->scale)
#define DPTR(node)   ((node)->dptr)
#define DLEN(node)   ((node)->dlen)

static int comp_data (lda_t* lda, 
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

static node_t* alloc_node (lda_t* lda, void* dptr, size_t dlen)
{
	node_t* n;

	if (lda->copier == QSE_LDA_COPIER_SIMPLE)
	{
		n = QSE_MMGR_ALLOC (lda->mmgr, QSE_SIZEOF(node_t));
		if (n == QSE_NULL) return QSE_NULL;
		DPTR(n) = dptr;
	}
	else if (lda->copier == QSE_LDA_COPIER_INLINE)
	{
		n = QSE_MMGR_ALLOC (lda->mmgr, 
			QSE_SIZEOF(node_t) + TOB(lda,dlen));
		if (n == QSE_NULL) return QSE_NULL;

		QSE_MEMCPY (n + 1, dptr, TOB(lda,dlen));
		DPTR(n) = n + 1;
	}
	else
	{
		n = QSE_MMGR_ALLOC (lda->mmgr, QSE_SIZEOF(node_t));
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

        if (qse_lda_init (lda, mmgr, capa) == QSE_NULL)
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

lda_t* qse_lda_init (lda_t* lda, mmgr_t* mmgr, size_t capa)
{
	QSE_MEMSET (lda, 0, QSE_SIZEOF(*lda));

	lda->mmgr = mmgr;
	lda->size = 0;
	lda->capa = 0;
	lda->node = QSE_NULL;

	lda->copier = QSE_LDA_COPIER_SIMPLE;
	lda->comper = comp_data;

	if (qse_lda_setcapa (lda, capa) == QSE_NULL) return QSE_NULL;
	return lda;
}

void qse_lda_fini (lda_t* lda)
{
	qse_lda_clear (lda);

	if (lda->node != QSE_NULL) 
	{
		QSE_MMGR_FREE (lda->mmgr, lda->node);
		lda->node = QSE_NULL;
		lda->capa = 0;
	}
}

void* qse_lda_getxtn (lda_t* lda)
{
	return lda + 1;
}

mmgr_t* qse_lda_getmmgr (lda_t* lda)
{
	return lda->mmgr;
}

void qse_lda_setmmgr (lda_t* lda, mmgr_t* mmgr)
{
	lda->mmgr = mmgr;
}

int qse_lda_getscale (lda_t* lda)
{
	return lda->scale;
}

void qse_lda_setscale (lda_t* lda, int scale)
{
	QSE_ASSERTX (scale > 0 && scale <= QSE_TYPE_MAX(qse_byte_t), 
		"The scale should be larger than 0 and less than or equal to the maximum value that the qse_byte_t type can hold");

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
	if (comper == QSE_NULL) comper = comp_data;
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

	if (lda->size > capa) 
	{
		qse_lda_delete (lda, capa, lda->size - capa);
		QSE_ASSERT (lda->size <= capa);
	}

	if (capa > 0) 
	{
		if (lda->mmgr->realloc != QSE_NULL && lda->node != QSE_NULL)
		{
			tmp = (qse_lda_node_t**)QSE_MMGR_REALLOC (
				lda->mmgr, lda->node, QSE_SIZEOF(*lda->node)*capa);
			if (tmp == QSE_NULL) return QSE_NULL;
		}
		else
		{
			tmp = (qse_lda_node_t**) QSE_MMGR_ALLOC (
				lda->mmgr, QSE_SIZEOF(*lda->node)*capa);
			if (tmp == QSE_NULL) return QSE_NULL;

			if (lda->node != QSE_NULL)
			{
				size_t x;
				x = (capa > lda->capa)? lda->capa: capa;
				QSE_MEMCPY (tmp, lda->node, 
					QSE_SIZEOF(*lda->node) * x);
				QSE_MMGR_FREE (lda->mmgr, lda->node);
			}
		}
	}
	else 
	{
		if (lda->node != QSE_NULL) 
		{
			qse_lda_clear (lda);
			QSE_MMGR_FREE (lda->mmgr, lda->node);
		}

		tmp = QSE_NULL;
	}

	lda->node = tmp;
	lda->capa = capa;
	
	return lda;
}

size_t qse_lda_search (lda_t* lda, size_t pos, const void* dptr, size_t dlen)
{
	size_t i;

	for (i = pos; i < lda->size; i++) 
	{
		if (lda->node[i] == QSE_NULL) continue;

		if (lda->comper (lda, 
			DPTR(lda->node[i]), DLEN(lda->node[i]),
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
			if (lda->node[i] == QSE_NULL) continue;

			if (lda->comper (lda, 
				DPTR(lda->node[i]), DLEN(lda->node[i]),
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
	node_t* node;

	/* allocate the node first */
	node = alloc_node (lda, dptr, dlen);
	if (node == QSE_NULL) return QSE_LDA_NIL;

	/* do resizeing if necessary. 
	 * resizing is performed after node allocation because that way, it 
	 * doesn't modify lda on any errors */
	if (pos >= lda->capa || lda->size >= lda->capa) 
	{
		size_t capa;

		if (lda->sizer)
		{
			capa = (pos >= lda->size)? (pos + 1): (lda->size + 1);
			capa = lda->sizer (lda, capa);
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
		
		if (qse_lda_setcapa(lda,capa) == QSE_NULL) 
		{
			if (lda->freeer) 
				lda->freeer (lda, DPTR(node), DLEN(node));
			QSE_MMGR_FREE (lda->mmgr, node);
			return QSE_LDA_NIL;
		}
	}

	if (pos >= lda->capa || lda->size >= lda->capa) 
	{
		/* the buffer is not still enough after resizing */
		if (lda->freeer) 
			lda->freeer (lda, DPTR(node), DLEN(node));
		QSE_MMGR_FREE (lda->mmgr, node);
		return QSE_LDA_NIL;
	}

	/* fill in the gap with QSE_NULL */
	for (i = lda->size; i < pos; i++) lda->node[i] = QSE_NULL;

	/* shift values to the next cell */
	for (i = lda->size; i > pos; i--) lda->node[i] = lda->node[i-1];

	/*  set the value */
	lda->node[pos] = node;

	if (pos > lda->size) lda->size = pos + 1;
	else lda->size++;

	return pos;
}

size_t qse_lda_update (lda_t* lda, size_t pos, void* dptr, size_t dlen)
{
	node_t* c;

	if (pos >= lda->size) return QSE_LDA_NIL;

	c = lda->node[pos];
	if (c == QSE_NULL)
	{
		/* no previous data */
		lda->node[pos] = alloc_node (lda, dptr, dlen);
		if (lda->node[pos] == QSE_NULL) return QSE_LDA_NIL;
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
			node_t* node = alloc_node (lda, dptr, dlen);
			if (node == QSE_NULL) return QSE_LDA_NIL;

			if (lda->freeer != QSE_NULL)
				lda->freeer (lda, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (lda->mmgr, c);

			lda->node[pos] = node;
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
		node_t* c = lda->node[i];

		if (c != QSE_NULL)
		{
			if (lda->freeer != QSE_NULL)
				lda->freeer (lda, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (lda->mmgr, c);

			lda->node[i] = QSE_NULL;
		}
	}

	for (i = index + count; i < lda->size; i++)
	{
		lda->node[i-count] = lda->node[i];
	}
	lda->node[lda->size-1] = QSE_NULL;

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
		node_t* c = lda->node[i];

		if (c != QSE_NULL)
		{
			if (lda->freeer != QSE_NULL)
				lda->freeer (lda, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (lda->mmgr, c);

			lda->node[i] = QSE_NULL;
		}
	}

	return count;
}

void qse_lda_clear (lda_t* lda)
{
	size_t i;

	for (i = 0; i < lda->size; i++) 
	{
		node_t* c = lda->node[i];
		if (c != QSE_NULL)
		{
			if (lda->freeer)
				lda->freeer (lda, DPTR(c), DLEN(c));
			QSE_MMGR_FREE (lda->mmgr, c);
			lda->node[i] = QSE_NULL;
		}
	}

	lda->size = 0;
}

void qse_lda_walk (lda_t* lda, walker_t walker, void* arg)
{
	qse_lda_walk_t w = QSE_LDA_WALK_FORWARD;
	size_t i = 0;

	while (1)	
	{
		if (lda->node[i] != QSE_NULL) 
                	w = walker (lda, i, arg);

		if (w == QSE_LDA_WALK_STOP) return;

		if (w == QSE_LDA_WALK_FORWARD) 
		{
			i++;
			if (i >= lda->size) return;
		}
		if (w == QSE_LDA_WALK_BACKWARD) 
		{
			if (i <= 0) return;
			i--;
		}
	}
}

void qse_lda_rwalk (lda_t* lda, walker_t walker, void* arg)
{
	qse_lda_walk_t w = QSE_LDA_WALK_BACKWARD;
	size_t i;

	if (lda->size <= 0) return;
	i = lda->size - 1;

	while (1)	
	{
		if (lda->node[i] != QSE_NULL) 
                	w = walker (lda, i, arg);

		if (w == QSE_LDA_WALK_STOP) return;

		if (w == QSE_LDA_WALK_FORWARD) 
		{
			i++;
			if (i >= lda->size) return;
		}
		if (w == QSE_LDA_WALK_BACKWARD) 
		{
			if (i <= 0) return;
			i--;
		}
	}
}

