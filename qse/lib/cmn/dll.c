/*
 * $Id$
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

#include <qse/cmn/dll.h>
#include "mem.h"

qse_dll_t* qse_dll_open (qse_mmgr_t* mmgr, qse_size_t ext)
{
	qse_dll_t* dll;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	dll = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_dll_t) + ext);
	if (dll == QSE_NULL) return QSE_NULL;

	/* do not zero the extension */
	QSE_MEMSET (dll, 0, QSE_SIZEOF(qse_dll_t));
	dll->mmgr = mmgr;

	return dll;
}

void qse_dll_close (qse_dll_t* dll)
{
	qse_dll_clear (dll);
	QSE_MMGR_FREE (dll->mmgr, dll);
}

void qse_dll_clear (qse_dll_t* dll)
{
	while (dll->head != QSE_NULL) qse_dll_delete (dll, dll->head);
	QSE_ASSERT (dll->tail == QSE_NULL);
}

void* qse_dll_getxtn (qse_dll_t* dll)
{
	return dll + 1;
}

qse_mmgr_t* qse_dll_getmmgr (qse_dll_t* dll)
{
        return dll->mmgr;
}

void qse_dll_setmmgr (qse_dll_t* dll, qse_mmgr_t* mmgr)
{
	dll->mmgr = mmgr;
}

qse_size_t qse_dll_getsize (qse_dll_t* dll)
{
	return dll->size;
}

qse_dll_node_t* qse_dll_gethead (qse_dll_t* dll)
{
	return dll->head;
}

qse_dll_node_t* qse_dll_gettail (qse_dll_t* dll)
{
	return dll->tail;
}

qse_dll_copier_t qse_dll_getcopier (qse_dll_t* dll)
{
	return dll->copier;
}

void qse_dll_setcopier (qse_dll_t* dll, qse_dll_copier_t copier)
{
	dll->copier = copier;
}

qse_dll_freeer_t qse_dll_getfreeer (qse_dll_t* dll)
{
	return dll->freeer;
}

void qse_dll_setfreeer (qse_dll_t* dll, qse_dll_freeer_t freeer)
{
	dll->freeer = freeer;
}

static qse_dll_node_t* alloc_node (qse_dll_t* dll, void* dptr, qse_size_t dlen)
{
	qse_dll_node_t* n;

	if (dll->copier == QSE_NULL)
	{
		n = QSE_MMGR_ALLOC (dll->mmgr, QSE_SIZEOF(qse_dll_node_t));
		if (n == QSE_NULL) return QSE_NULL;
		n->dptr = dptr;
	}
	else if (dll->copier == QSE_DLL_COPIER_INLINE)
	{
		n = QSE_MMGR_ALLOC (dll->mmgr, QSE_SIZEOF(qse_dll_node_t) + dlen);
		if (n == QSE_NULL) return QSE_NULL;

		QSE_MEMCPY (n + 1, dptr, dlen);
		n->dptr = n + 1;
	}
	else
	{
		n = QSE_MMGR_ALLOC (dll->mmgr, QSE_SIZEOF(qse_dll_node_t));
		if (n == QSE_NULL) return QSE_NULL;
		n->dptr = dll->copier (dll, dptr, dlen);
		if (n->dptr == QSE_NULL)
		{
			QSE_MMGR_FREE (dll->mmgr, n);
			return QSE_NULL;
		}
	}

	n->dlen = dlen; 
	n->next = QSE_NULL;	
	n->prev = QSE_NULL;

	return n;
}

qse_dll_node_t* qse_dll_insert (
	qse_dll_t* dll, qse_dll_node_t* pos, void* dptr, qse_size_t dlen)
{
	qse_dll_node_t* n = alloc_node (dll, dptr, dlen);
	if (n == QSE_NULL) return QSE_NULL;

	if (pos == QSE_NULL)
	{
		/* insert at the end */
		if (dll->head == QSE_NULL)
		{
			QSE_ASSERT (dll->tail == QSE_NULL);
			dll->head = n;
		}
		else dll->tail->next = n;

		dll->tail = n;
	}
	else
	{
		/* insert in front of the positional node */
		n->next = pos;
		if (pos == dll->head) dll->head = n;
		else
		{
			/* take note of performance penalty */
			qse_dll_node_t* n2 = dll->head;
			while (n2->next != pos) n2 = n2->next;
			n2->next = n;
		}
	}

	dll->size++;
	return n;
}

qse_dll_node_t* qse_dll_pushhead (qse_dll_t* dll, void* data, qse_size_t size)
{
	return qse_dll_insert (dll, dll->head, data, size);
}

qse_dll_node_t* qse_dll_pushtail (qse_dll_t* dll, void* data, qse_size_t size)
{
	return qse_dll_insert (dll, QSE_NULL, data, size);
}

void qse_dll_delete (qse_dll_t* dll, qse_dll_node_t* pos)
{
	if (pos == QSE_NULL) return; /* not a valid node */

	if (pos == dll->head)
	{
		/* it is simple to delete the head node */
		dll->head = pos->next;
		if (dll->head == QSE_NULL) dll->tail = QSE_NULL;
	}
	else 
	{
		/* but deletion of other nodes has significant performance
		 * penalty as it has look for the predecessor of the 
		 * target node */
		qse_dll_node_t* n2 = dll->head;
		while (n2->next != pos) n2 = n2->next;

		n2->next = pos->next;

		/* update the tail node if necessary */
		if (pos == dll->tail) dll->tail = n2;
	}

	if (dll->freeer != QSE_NULL)
	{
		/* free the actual data */
		dll->freeer (dll, pos->dptr, pos->dlen);
	}

	/* free the node */
	QSE_MMGR_FREE (dll->mmgr, pos);

	/* decrement the number of elements */
	dll->size--;
}

void qse_dll_pophead (qse_dll_t* dll)
{
	qse_dll_delete (dll, dll->head);
}

void qse_dll_poptail (qse_dll_t* dll)
{
	qse_dll_delete (dll, dll->tail);
}

void qse_dll_walk (qse_dll_t* dll, qse_dll_walker_t walker, void* arg)
{
	qse_dll_node_t* n = dll->head;

	while (n != QSE_NULL)
	{
		if (walker(dll,n,arg) == QSE_DLL_WALK_STOP) return;
		n = n->next;
	}
}

void* qse_dll_copyinline (qse_dll_t* dll, void* dptr, qse_size_t dlen)
{
	/* this is a dummy copier */
	return QSE_NULL;
}

