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

#include <qse/cmn/tmr.h>
#include "mem.h"

#define HEAP_PARENT(x) (((x) - 1) / 2)
#define HEAP_LEFT(x)   ((x) * 2 + 1)
#define HEAP_RIGHT(x)  ((x) * 2 + 2)

#define YOUNGER_THAN(x,y) (qse_cmptime(&(x)->when, &(y)->when) < 0)

qse_tmr_t* qse_tmr_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_size_t capa)
{
	qse_tmr_t* tmr;

	tmr = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_tmr_t) + xtnsize);
	if (tmr)
	{
		if (qse_tmr_init (tmr, mmgr, capa) <= -1)
		{
			QSE_MMGR_FREE (mmgr, tmr);
			return QSE_NULL;
		}

		else QSE_MEMSET (QSE_XTN(tmr), 0, xtnsize);
	}

	return tmr;
}


void qse_tmr_close (qse_tmr_t* tmr)
{
	qse_tmr_fini (tmr);
	QSE_MMGR_FREE (tmr->mmgr, tmr);
}

int qse_tmr_init (qse_tmr_t* tmr, qse_mmgr_t* mmgr, qse_size_t capa)
{
	qse_tmr_event_t* tmp;

	QSE_MEMSET (tmr, 0, QSE_SIZEOF(*tmr));

	if (capa <= 0) capa = 1;

	tmp = QSE_MMGR_ALLOC (mmgr, capa * QSE_SIZEOF(*tmp));
	if (tmp == QSE_NULL) return -1;

	tmr->mmgr = mmgr;
	tmr->capa = capa;
	tmr->event = tmp;

	return 0;
}

void qse_tmr_fini (qse_tmr_t* tmr)
{
	qse_tmr_clear (tmr);
	if (tmr->event) QSE_MMGR_FREE (tmr->mmgr, tmr->event);
}

qse_mmgr_t* qse_tmr_getmmgr (qse_tmr_t* tmr)
{
	return tmr->mmgr;
}

void* qse_tmr_getxtn (qse_tmr_t* tmr)
{
	return QSE_XTN (tmr);
}

void qse_tmr_clear (qse_tmr_t* tmr)
{
	while (tmr->size > 0) qse_tmr_remove (tmr, 0);
}

static qse_tmr_index_t sift_up (qse_tmr_t* tmr, qse_tmr_index_t index, int notify)
{
	qse_size_t parent;

	parent = HEAP_PARENT(index);
	if (index > 0 && YOUNGER_THAN(&tmr->event[index], &tmr->event[parent]))
	{
		qse_tmr_event_t item;
		qse_size_t old_index;

		item = tmr->event[index]; 
		old_index = index;

		do
		{
			/* move down the parent to my current position */
			tmr->event[index] = tmr->event[parent];
			tmr->event[index].updater (tmr, parent, index, &tmr->event[index]);

			/* traverse up */
			index = parent;
			parent = HEAP_PARENT(parent);
		}
		while (index > 0 && YOUNGER_THAN(&item, &tmr->event[parent]));

		/* we send no notification if the item is added with qse_tmr_insert()
		 * or updated with qse_tmr_update(). the caller of the funnctions must
		 * reply on the return value. */
		tmr->event[index] = item;
		if (notify && index != old_index)
			tmr->event[index].updater (tmr, old_index, index, &tmr->event[index]);
	}

	return index;
}

static qse_tmr_index_t sift_down (qse_tmr_t* tmr, qse_tmr_index_t index, int notify)
{
	qse_size_t base = tmr->size / 2;

	if (index < base) /* at least 1 child is under the 'index' positmrn */
	{
		qse_tmr_event_t item;
		qse_size_t old_index;

		item = tmr->event[index];
		old_index = index;

		do
		{
			qse_size_t left, right, younger;

			left= HEAP_LEFT(index);
			right = HEAP_RIGHT(index);

			if (right < tmr->size && YOUNGER_THAN(&tmr->event[right], &tmr->event[left]))
			{
				younger = right;
			}
			else
			{
				younger = left;
			}

			if (YOUNGER_THAN(&item, &tmr->event[younger])) break;

			tmr->event[index] = tmr->event[younger];
			tmr->event[index].updater (tmr, younger, index, &tmr->event[index]);

			index = younger;
		}
		while (index < base);
		
		tmr->event[index] = item;
		if (notify && index != old_index)
			tmr->event[index].updater (tmr, old_index, index, &tmr->event[index]);
	}

	return index;
}

void qse_tmr_remove (qse_tmr_t* tmr, qse_tmr_index_t index)
{
	qse_tmr_event_t item;

	QSE_ASSERT (index < tmr->size);

	item = tmr->event[index];
	tmr->event[index].updater (tmr, index, QSE_TMR_INVALID_INDEX, &tmr->event[index]);

	tmr->size = tmr->size - 1;
	if (tmr->size > 0 && index != tmr->size)
	{
		tmr->event[index] = tmr->event[tmr->size];
		tmr->event[index].updater (tmr, tmr->size, index, &tmr->event[index]);
		YOUNGER_THAN(&tmr->event[index], &item)? sift_up(tmr, index, 1): sift_down(tmr, index, 1);
	}
}

qse_tmr_index_t qse_tmr_insert (qse_tmr_t* tmr, const qse_tmr_event_t* event)
{
	qse_tmr_index_t index = tmr->size;

	if (index >= tmr->capa)
	{
		qse_tmr_event_t* tmp;
		qse_size_t new_capa;

		new_capa = tmr->capa * 2;
		tmp = QSE_MMGR_REALLOC (tmr->mmgr, tmr->event, new_capa * QSE_SIZEOF(*tmp));
		if (tmp == QSE_NULL) return QSE_TMR_INVALID_INDEX;

		tmr->event = tmp;
		tmr->capa = new_capa;
	}

	tmr->size = tmr->size + 1;
	tmr->event[index] = *event;
	return sift_up (tmr, index, 0);
}

qse_size_t qse_tmr_update (qse_tmr_t* tmr, qse_size_t index, const qse_tmr_event_t* event)
{
	qse_tmr_event_t item;

	item = tmr->event[index];
	tmr->event[index] = *event;
	return YOUNGER_THAN(event, &item)? sift_up (tmr, index, 0): sift_down (tmr, index, 0);
}

qse_size_t qse_tmr_fire (qse_tmr_t* tmr, const qse_ntime_t* tm)
{
	qse_ntime_t now;
	qse_tmr_event_t event;
	qse_size_t fire_count = 0;

	/* if the current time is not specified, get it from the system */
	if (tm) now = *tm;
	else if (qse_gettime (&now) <= -1) return -1;

	while (tmr->size > 0)
	{
		if (qse_cmptime(&tmr->event[0].when, &now) > 0) break;

		event = tmr->event[0];
		qse_tmr_remove (tmr, 0); /* remove the registered event structure */

		fire_count++;
		event.handler (tmr, &now, &event); /* then fire the event */
	}

	return fire_count;
}

int qse_tmr_gettmout (qse_tmr_t* tmr, const qse_ntime_t* tm, qse_ntime_t* tmout)
{
	qse_ntime_t now;

	/* time-out can't be calculated when there's no event scheduled */
	if (tmr->size <= 0) return -1;

	/* if the current time is not specified, get it from the system */
	if (tm) now = *tm;
	else if (qse_gettime (&now) <= -1) return -1;

	qse_subtime (&tmr->event[0].when, &now, tmout);
	if (tmout->sec < 0) qse_cleartime (tmout);

	return 0;
}

qse_tmr_event_t* qse_tmr_getevent (qse_tmr_t* tmr, qse_tmr_index_t index)
{
	return (index < 0 || index >= tmr->size)? QSE_NULL: &tmr->event[index];
}
