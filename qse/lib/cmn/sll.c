/*
 * $Id: sll.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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

#include <qse/cmn/sll.h>
#include "mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (sll)

#define sll_t    qse_sll_t
#define node_t   qse_sll_node_t
#define copier_t qse_sll_copier_t
#define freeer_t qse_sll_freeer_t
#define comper_t qse_sll_comper_t
#define walker_t qse_sll_walker_t

#define HEAD(s) QSE_SLL_HEAD(s)
#define TAIL(s) QSE_SLL_TAIL(s)
#define SIZE(s) QSE_SLL_SIZE(s)

#define DPTR(n) QSE_SLL_DPTR(n)
#define DLEN(n) QSE_SLL_DLEN(n)
#define NEXT(n) QSE_SLL_NEXT(n)

#define TOB(sll,len) ((len)*(sll)->scale)

#define SIZEOF(x) QSE_SIZEOF(x)
#define size_t    qse_size_t
#define mmgr_t    qse_mmgr_t

static int default_comper (sll_t* sll, 
	const void* dptr1, size_t dlen1, 
	const void* dptr2, size_t dlen2)
{
	if (dlen1 == dlen2) return QSE_MEMCMP (dptr1, dptr2, TOB(sll,dlen1));
	/* it just returns 1 to indicate that they are different. */
	return 1;

#if 0
	size_t min = (dlen1 < dlen2)? dlen1: dlen2;
	int n = QSE_MEMCMP (dptr1, dptr2, TOB(sll,min));
	if (n == 0 && dlen1 != dlen2)
	{
		n = (dlen1 > dlen2)? 1: -1;
	}
	return n;
#endif
}

static node_t* alloc_node (sll_t* sll, void* dptr, size_t dlen)
{
	node_t* n;

	if (sll->copier == QSE_SLL_COPIER_SIMPLE)
	{
		n = QSE_MMGR_ALLOC (sll->mmgr, SIZEOF(node_t));
		if (n == QSE_NULL) return QSE_NULL;
		DPTR(n) = dptr;
	}
	else if (sll->copier == QSE_SLL_COPIER_INLINE)
	{
		n = QSE_MMGR_ALLOC (sll->mmgr, 
			SIZEOF(node_t) + TOB(sll,dlen));
		if (n == QSE_NULL) return QSE_NULL;

		QSE_MEMCPY (n + 1, dptr, TOB(sll,dlen));
		DPTR(n) = n + 1;
	}
	else
	{
		n = QSE_MMGR_ALLOC (sll->mmgr, SIZEOF(node_t));
		if (n == QSE_NULL) return QSE_NULL;
		DPTR(n) = sll->copier (sll, dptr, dlen);
		if (DPTR(n) == QSE_NULL) 
		{
			QSE_MMGR_FREE (sll->mmgr, n);
			return QSE_NULL;
		}
	}

	DLEN(n) = dlen; 
	NEXT(n) = QSE_NULL;	

	return n;
}

sll_t* qse_sll_open (mmgr_t* mmgr, size_t ext)
{
	sll_t* sll;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	sll = QSE_MMGR_ALLOC (mmgr, SIZEOF(sll_t) + ext);
	if (sll == QSE_NULL) return QSE_NULL;

	if (qse_sll_init (sll, mmgr) <= -1)
	{
		QSE_MMGR_FREE (mmgr, sll);
		return QSE_NULL;
	}

	return sll;
}

void qse_sll_close (sll_t* sll)
{
	qse_sll_fini (sll);
	QSE_MMGR_FREE (sll->mmgr, sll);
}

int qse_sll_init (sll_t* sll, mmgr_t* mmgr)
{
	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	/* do not zero out the extension */
	QSE_MEMSET (sll, 0, SIZEOF(*sll));

	sll->mmgr = mmgr;
	sll->size = 0;
	sll->scale = 1;

	sll->comper = default_comper;
	sll->copier = QSE_SLL_COPIER_SIMPLE;
	return 0;
}

void qse_sll_fini (sll_t* sll)
{
	qse_sll_clear (sll);
}

int qse_sll_getscale (sll_t* sll)
{
	return sll->scale;
}

void qse_sll_setscale (sll_t* sll, int scale)
{
	QSE_ASSERTX (scale > 0 && scale <= QSE_TYPE_MAX(qse_byte_t), 
		"The scale should be larger than 0 and less than or equal to the maximum value that the qse_byte_t type can hold");

	if (scale <= 0) scale = 1;
	if (scale > QSE_TYPE_MAX(qse_byte_t)) scale = QSE_TYPE_MAX(qse_byte_t);

	sll->scale = scale;
}

copier_t qse_sll_getcopier (sll_t* sll)
{
	return sll->copier;
}

void qse_sll_setcopier (sll_t* sll, copier_t copier)
{
	if (copier == QSE_NULL) copier = QSE_SLL_COPIER_SIMPLE;
	sll->copier = copier;
}

freeer_t qse_sll_getfreeer (sll_t* sll)
{
	return sll->freeer;
}

void qse_sll_setfreeer (sll_t* sll, freeer_t freeer)
{
	sll->freeer = freeer;
}

comper_t qse_sll_getcomper (sll_t* sll)
{
	return sll->comper;
}

void qse_sll_setcomper (sll_t* sll, comper_t comper)
{
	if (comper == QSE_NULL) comper = default_comper;
	sll->comper = comper;
}

size_t qse_sll_getsize (sll_t* sll)
{
	return SIZE(sll);
}

node_t* qse_sll_gethead (sll_t* sll)
{
	return HEAD(sll);
}

node_t* qse_sll_gettail (sll_t* sll)
{
	return TAIL(sll);
}

node_t* qse_sll_search (sll_t* sll, node_t* pos, const void* dptr, size_t dlen)
{
	pos = (pos == QSE_NULL)? sll->head: NEXT(pos);

	while (pos != QSE_NULL)
	{
		if (sll->comper (sll, DPTR(pos), DLEN(pos), dptr, dlen) == 0)
		{
			return pos;
		}
		pos = NEXT(pos);
	}	

	return QSE_NULL;
}

node_t* qse_sll_insert (
	sll_t* sll, node_t* pos, void* dptr, size_t dlen)
{
	node_t* n = alloc_node (sll, dptr, dlen);
	if (n == QSE_NULL) return QSE_NULL;

	if (pos == QSE_NULL)
	{
		/* insert at the end */
		if (HEAD(sll) == QSE_NULL)
		{
			QSE_ASSERT (TAIL(sll) == QSE_NULL);
			HEAD(sll) = n;
		}
		else NEXT(TAIL(sll)) = n;

		TAIL(sll) = n;
	}
	else
	{
		/* insert in front of the positional node */
		NEXT(n) = pos;
		if (pos == HEAD(sll)) HEAD(sll) = n;
		else
		{
			/* take note of performance penalty */
			node_t* n2 = HEAD(sll);
			while (NEXT(n2) != pos) n2 = NEXT(n2);
			NEXT(n2) = n;
		}
	}

	SIZE(sll)++;
	return n;
}

void qse_sll_delete (sll_t* sll, node_t* pos)
{
	if (pos == QSE_NULL) return; /* not a valid node */

	if (pos == HEAD(sll))
	{
		/* it is simple to delete the head node */
		HEAD(sll) = NEXT(pos);
		if (HEAD(sll) == QSE_NULL) TAIL(sll) = QSE_NULL;
	}
	else 
	{
		/* but deletion of other nodes has significant performance
		 * penalty as it has look for the predecessor of the 
		 * target node */
		node_t* n2 = HEAD(sll);
		while (NEXT(n2) != pos) n2 = NEXT(n2);

		NEXT(n2) = NEXT(pos);

		/* update the tail node if necessary */
		if (pos == TAIL(sll)) TAIL(sll) = n2;
	}

	if (sll->freeer != QSE_NULL)
	{
		/* free the actual data */
		sll->freeer (sll, DPTR(pos), DLEN(pos));
	}

	/* free the node */
	QSE_MMGR_FREE (sll->mmgr, pos);

	/* decrement the number of elements */
	SIZE(sll)--;
}

void qse_sll_clear (sll_t* sll)
{
	while (HEAD(sll) != QSE_NULL) qse_sll_delete (sll, HEAD(sll));
	QSE_ASSERT (TAIL(sll) == QSE_NULL);
}

void qse_sll_walk (sll_t* sll, walker_t walker, void* ctx)
{
	node_t* n = HEAD(sll);

	while (n != QSE_NULL)
	{
		qse_sll_node_t* nxt = NEXT(n);
		if (walker(sll,n,ctx) == QSE_SLL_WALK_STOP) return;
		n = nxt;
	}
}

node_t* qse_sll_pushhead (sll_t* sll, void* data, size_t size)
{
	return qse_sll_insert (sll, HEAD(sll), data, size);
}

node_t* qse_sll_pushtail (sll_t* sll, void* data, size_t size)
{
	return qse_sll_insert (sll, QSE_NULL, data, size);
}

void qse_sll_pophead (sll_t* sll)
{
	qse_sll_delete (sll, HEAD(sll));
}

void qse_sll_poptail (sll_t* sll)
{
	qse_sll_delete (sll, TAIL(sll));
}


