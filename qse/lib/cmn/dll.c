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

#include <qse/cmn/dll.h>
#include "mem-prv.h"

#define TOB(dll,len) ((len)*(dll)->scale)
#define DPTR(node) QSE_DLL_DPTR(node)
#define DLEN(node) QSE_DLL_DLEN(node)

static int default_comper (
	qse_dll_t* dll, 
	const void* dptr1, qse_size_t dlen1, 
	const void* dptr2, qse_size_t dlen2)
{
	if (dlen1 == dlen2) return QSE_MEMCMP (dptr1, dptr2, TOB(dll,dlen1));
	/* it just returns 1 to indicate that they are different. */
	return 1;

#if 0
	qse_size_t min = (dlen1 < dlen2)? dlen1: dlen2;
	int n = QSE_MEMCMP (dptr1, dptr2, TOB(dll,min));
	if (n == 0 && dlen1 != dlen2)
	{
		n = (dlen1 > dlen2)? 1: -1;
	}
	return n;
#endif
}

qse_dll_t* qse_dll_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_dll_t* dll;

	dll = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_dll_t) + xtnsize);
	if (dll == QSE_NULL) return QSE_NULL;

	if (qse_dll_init (dll, mmgr) <= -1)
	{
		QSE_MMGR_FREE (mmgr, dll);
		return QSE_NULL;
	}

	QSE_MEMSET (QSE_XTN(dll), 0, xtnsize);
	return dll;
}

void qse_dll_close (qse_dll_t* dll)
{
	qse_dll_fini (dll);
	QSE_MMGR_FREE (dll->mmgr, dll);
}

int qse_dll_init (qse_dll_t* dll, qse_mmgr_t* mmgr)
{
	/* do not zero out the extension */
	QSE_MEMSET (dll, 0, QSE_SIZEOF(*dll));

	dll->mmgr = mmgr;
	dll->scale = 1;

	dll->comper = default_comper;
	dll->copier = QSE_DLL_COPIER_SIMPLE;

	QSE_GDL_INIT (&dll->gdl);
	return 0;
}

void qse_dll_fini (qse_dll_t* dll)
{
	qse_dll_clear (dll);
}

qse_mmgr_t* qse_dll_getmmgr (qse_dll_t* dll)
{
	return dll->mmgr;
}

void* qse_dll_getxtn (qse_dll_t* dll)
{
	return QSE_XTN (dll);
}

int qse_dll_getscale (qse_dll_t* dll)
{
	return dll->scale;
}

void qse_dll_setscale (qse_dll_t* dll, int scale)
{
	QSE_ASSERTX (scale > 0 && scale <= QSE_TYPE_MAX(qse_byte_t), 
		"The scale should be larger than 0 and less than or equal to the maximum value that the qse_byte_t type can hold"
	);

	if (scale <= 0) scale = 1;
	if (scale > QSE_TYPE_MAX(qse_byte_t)) scale = QSE_TYPE_MAX(qse_byte_t);

	dll->scale = scale;
}

qse_dll_copier_t qse_dll_getcopier (qse_dll_t* dll)
{
	return dll->copier;
}

void qse_dll_setcopier (qse_dll_t* dll, qse_dll_copier_t copier)
{
	if (copier == QSE_NULL) copier = QSE_DLL_COPIER_SIMPLE;
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

qse_dll_comper_t qse_dll_getcomper (qse_dll_t* dll)
{
	return dll->comper;
}

void qse_dll_setcomper (qse_dll_t* dll, qse_dll_comper_t comper)
{
	if (comper == QSE_NULL) comper = default_comper;
	dll->comper = comper;
}

qse_size_t qse_dll_getsize (qse_dll_t* dll)
{
	return QSE_DLL_SIZE(dll);
}

qse_dll_node_t* qse_dll_gethead (qse_dll_t* dll)
{
	if (QSE_GDL_ISEMPTY(&dll->gdl)) return QSE_NULL;
	return QSE_GDL_CONTAINER(QSE_GDL_HEAD(&dll->gdl), qse_dll_node_t, link);
}

qse_dll_node_t* qse_dll_gettail (qse_dll_t* dll)
{
	if (QSE_GDL_ISEMPTY(&dll->gdl)) return QSE_NULL;
	return QSE_GDL_CONTAINER(QSE_GDL_TAIL(&dll->gdl), qse_dll_node_t, link);
}

static qse_dll_node_t* alloc_node (qse_dll_t* dll, void* dptr, qse_size_t dlen)
{
	qse_dll_node_t* n;

	if (dll->copier == QSE_DLL_COPIER_SIMPLE)
	{
		n = QSE_MMGR_ALLOC (dll->mmgr, QSE_SIZEOF(qse_dll_node_t));
		if (n == QSE_NULL) return QSE_NULL;
		DPTR(n) = dptr;
	}
	else if (dll->copier == QSE_DLL_COPIER_INLINE)
	{
		n = QSE_MMGR_ALLOC (dll->mmgr, 
			QSE_SIZEOF(qse_dll_node_t) + TOB(dll,dlen));
		if (n == QSE_NULL) return QSE_NULL;

		QSE_MEMCPY (n + 1, dptr, TOB(dll,dlen));
		DPTR(n) = n + 1;
	}
	else
	{
		n = QSE_MMGR_ALLOC (dll->mmgr, QSE_SIZEOF(qse_dll_node_t));
		if (n == QSE_NULL) return QSE_NULL;
		DPTR(n) = dll->copier (dll, dptr, dlen);
		if (DPTR(n) == QSE_NULL)
		{
			QSE_MMGR_FREE (dll->mmgr, n);
			return QSE_NULL;
		}
	}

	DLEN(n) = dlen; 
	return n;
}

static QSE_INLINE void free_node (qse_dll_t* dll, qse_dll_node_t* node)
{
	if (dll->freeer != QSE_NULL)
	{
		/* free the actual data */
		dll->freeer (dll, DPTR(node), DLEN(node));
	}

	/* free the node */
	QSE_MMGR_FREE (dll->mmgr, node);
}

qse_dll_node_t* qse_dll_search (
	qse_dll_t* dll, qse_dll_node_t* pos, const void* dptr, qse_size_t dlen)
{
	if (QSE_GDL_ISEMPTY(&dll->gdl)) return QSE_NULL;

	if (pos == QSE_NULL) 
		pos = QSE_GDL_CONTAINER (QSE_GDL_HEAD(&dll->gdl), qse_dll_node_t, link);

	while (QSE_GDL_ISLINK (&dll->gdl, &pos->link))
	{
		if (dll->comper (dll, DPTR(pos), DLEN(pos), dptr, dlen) == 0)
		{
			return pos;
		}

		pos = QSE_GDL_CONTAINER (QSE_GDL_NEXT(&pos->link), qse_dll_node_t, link);
	}	

	return QSE_NULL;
}

qse_dll_node_t* qse_dll_rsearch (
	qse_dll_t* dll, qse_dll_node_t* pos, const void* dptr, qse_size_t dlen)
{
	if (QSE_GDL_ISEMPTY(&dll->gdl)) return QSE_NULL;
	if (pos == QSE_NULL) 
		pos = QSE_GDL_CONTAINER (QSE_GDL_TAIL(&dll->gdl), qse_dll_node_t, link);

	while (QSE_GDL_ISLINK (&dll->gdl, &pos->link))
	{
		if (dll->comper (dll, DPTR(pos), DLEN(pos), dptr, dlen) == 0)
		{
			return pos;
		}

		pos = QSE_GDL_CONTAINER (QSE_GDL_PREV(&pos->link), qse_dll_node_t, link);
	}	

	return QSE_NULL;
}

qse_dll_node_t* qse_dll_insert (
	qse_dll_t* dll, qse_dll_node_t* pos, void* dptr, qse_size_t dlen)
{
	qse_dll_node_t* n = alloc_node (dll, dptr, dlen);
	if (n)
	{
		if (pos == QSE_NULL)
		{
			/* insert at the end */
			QSE_GDL_APPEND (&dll->gdl, &n->link);
			dll->size++;
		}
		else
		{
			/* insert in front of the positional node */
			QSE_GDL_CHAIN (&dll->gdl, QSE_GDL_PREV(&pos->link), &n->link, &pos->link);
			dll->size++;
		}
	}

	return n;
}

void qse_dll_delete (qse_dll_t* dll, qse_dll_node_t* pos)
{
	if (pos == QSE_NULL || !QSE_GDL_ISLINK(&dll->gdl,&pos->link)) return;

	QSE_GDL_UNCHAIN (&dll->gdl, &pos->link);
	free_node (dll, pos);
	dll->size--;
}

void qse_dll_clear (qse_dll_t* dll)
{
	while (!QSE_GDL_ISEMPTY(&dll->gdl))
	{
		qse_dll_delete (dll, QSE_GDL_CONTAINER (QSE_GDL_HEAD(&dll->gdl), qse_dll_node_t, link));
	}
}

void qse_dll_walk (qse_dll_t* dll, qse_dll_walker_t walker, void* ctx)
{
	qse_gdl_link_t* n = QSE_GDL_HEAD (&dll->gdl);
	qse_dll_walk_t w = QSE_DLL_WALK_FORWARD;

	while (QSE_GDL_ISLINK(&dll->gdl,n))
	{
		qse_gdl_link_t tmp;

		tmp = *n;
		w = walker (dll, QSE_GDL_CONTAINER (n, qse_dll_node_t, link), ctx);

		if (w == QSE_DLL_WALK_FORWARD) n = QSE_GDL_NEXT (&tmp);
		else if (w == QSE_DLL_WALK_BACKWARD) n = QSE_GDL_PREV (&tmp);
		else break;
	}
}

void qse_dll_rwalk (qse_dll_t* dll, qse_dll_walker_t walker, void* ctx)
{
	qse_gdl_link_t* n = QSE_GDL_TAIL (&dll->gdl);
	qse_dll_walk_t w = QSE_DLL_WALK_BACKWARD;

	while (QSE_GDL_ISLINK(&dll->gdl,n))
	{
		qse_gdl_link_t tmp;

		tmp = *n;
		w = walker (dll, QSE_GDL_CONTAINER (n, qse_dll_node_t, link), ctx);

		if (w == QSE_DLL_WALK_FORWARD) n = QSE_GDL_NEXT (&tmp);
		else if (w == QSE_DLL_WALK_BACKWARD) n = QSE_GDL_PREV (&tmp);
		else break;
	}
}

qse_dll_node_t* qse_dll_pushhead (qse_dll_t* dll, void* data, qse_size_t size)
{
	return qse_dll_insert (dll, QSE_GDL_CONTAINER (QSE_GDL_HEAD (&dll->gdl), qse_dll_node_t, link), data, size);
}

qse_dll_node_t* qse_dll_pushtail (qse_dll_t* dll, void* data, qse_size_t size)
{
	return qse_dll_insert (dll, QSE_NULL, data, size);
}

void qse_dll_pophead (qse_dll_t* dll)
{
	QSE_ASSERT (!QSE_GDL_ISEMPTY(&dll->gdl));
	qse_dll_delete (dll, QSE_GDL_CONTAINER (QSE_GDL_HEAD (&dll->gdl), qse_dll_node_t, link));
}

void qse_dll_poptail (qse_dll_t* dll)
{
	QSE_ASSERT (!QSE_GDL_ISEMPTY(&dll->gdl));
	qse_dll_delete (dll, QSE_GDL_CONTAINER (QSE_GDL_TAIL (&dll->gdl), qse_dll_node_t, link));
}


